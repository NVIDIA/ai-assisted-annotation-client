/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkMetaDataObject.h>
#include <itkImageSeriesReader.h>
#include <itkImageFileWriter.h>

#include <iostream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <gdcmGlobal.h>

//The program converts Dicom series to Nifti format with capability of multiple series read and get patient name as output  
void trimTrailing(char * str) {
	int index, i;

	/* Find last index of non-white space character */
	i = 0;
	while (str[i] != '\0') {
		if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
			index = i;
		}

		i++;
	}

	/* Mark next character to last non-white space character as NULL */
	str[index + 1] = '\0';
}

int main(int argc, char * argv[]) {
	if (argc < 4) {
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << " <DicomDirectory> <outputFileName> <Head Option> (Head Options 0:specified; 1:head with pid+date)" << std::endl;
		return EXIT_FAILURE;
	}

	//ITK settings
	const unsigned int Dimension = 3;
	typedef int PixelType;
	typedef itk::Image<PixelType, Dimension> ImageType;
	typedef itk::ImageSeriesReader<ImageType> ReaderType;
	typedef itk::GDCMImageIO ImageIOType;
	typedef itk::GDCMSeriesFileNames NamesGeneratorType;
	typedef itk::ImageFileWriter<ImageType> WriterType;

	//Filters
	ReaderType::Pointer reader = ReaderType::New();
	ImageIOType::Pointer dicomIO = ImageIOType::New();
	NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
	WriterType::Pointer writer = WriterType::New();

	//Parameters
	nameGenerator->SetDirectory(argv[1]);
	char outFileName[50];
	bool option = atoi(argv[3]);
	bool closeMessage;
	if (argc == 5)
		closeMessage = true;

	//Pipeline
	reader->SetImageIO(dicomIO);
	nameGenerator->SetUseSeriesDetails(true);
	nameGenerator->AddSeriesRestriction("0008|0021");
	writer->SetInput(reader->GetOutput());

	typedef std::vector<std::string> SeriesIdContainer;
	const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
	SeriesIdContainer::const_iterator seriesItr = seriesUID.begin();
	SeriesIdContainer::const_iterator seriesEnd = seriesUID.end();

	if (!closeMessage) {
		std::cout << std::endl << "The directory: ";
		std::cout << std::endl << argv[1] << std::endl << std::endl;
		std::cout << "Contains the following DICOM Series: ";
		std::cout << std::endl;
	}
	while (seriesItr != seriesEnd) {
		if (!closeMessage)
			std::cout << seriesItr->c_str() << std::endl;
		++seriesItr;
	}

	seriesItr = seriesUID.begin();

	while (seriesItr != seriesEnd) {
		std::strcpy(outFileName, argv[2]);
		std::string seriesIdentifier;
		seriesIdentifier = seriesItr->c_str();
		if (!closeMessage) {
			std::cout << std::endl;
			std::cout << "Now reading series: " << std::endl;
			std::cout << seriesIdentifier << std::endl;
		}
		typedef std::vector<std::string> FileNamesContainer;
		FileNamesContainer fileNames;
		fileNames = nameGenerator->GetFileNames(seriesIdentifier);
		reader->SetFileNames(fileNames);
		reader->Update();
		typedef itk::MetaDataDictionary DictionaryType;
		const DictionaryType & dictionary = dicomIO->GetMetaDataDictionary();
		typedef itk::MetaDataObject<std::string> MetaDataStringType;
		DictionaryType::ConstIterator itr = dictionary.Begin();
		DictionaryType::ConstIterator end = dictionary.End();
		std::string entryId = "0010|0020";
		DictionaryType::ConstIterator tagItr = dictionary.Find(entryId);
		MetaDataStringType::ConstPointer entryvalue = dynamic_cast<const MetaDataStringType *>(tagItr->second.GetPointer());
		std::string tagvalue1 = entryvalue->GetMetaDataObjectValue();
		if (!closeMessage) {
			std::cout << "Patient's Name (" << entryId << ") ";
			std::cout << " is: " << tagvalue1.c_str() << std::endl;
		}
		entryId = "0008|0020";
		tagItr = dictionary.Find(entryId);
		entryvalue = dynamic_cast<const MetaDataStringType *>(tagItr->second.GetPointer());
		std::string tagvalue2 = entryvalue->GetMetaDataObjectValue();
		if (!closeMessage) {
			std::cout << "Study Date (" << entryId << ") ";
			std::cout << " is: " << tagvalue2.c_str() << std::endl;
		}
		char temp[100];
		strcpy(temp, tagvalue1.c_str());
		trimTrailing(temp);
		std::strcat(outFileName, temp);
		std::strcat(outFileName, "_");
		strcpy(temp, tagvalue2.c_str());
		trimTrailing(temp);
		std::strcat(outFileName, temp);
		std::strcat(outFileName, ".nii");

		if (option) {
			if (!closeMessage) {
				std::cout << "Writing the image as " << std::endl;
				std::cout << outFileName << std::endl << std::endl;
			}
			writer->SetFileName(outFileName);
		} else {
			if (!closeMessage) {
				std::cout << "Writing the image as " << std::endl;
				std::cout << argv[2] << std::endl << std::endl;
			}
			writer->SetFileName(argv[2]);
		}
		writer->Update();

		++seriesItr;
	}

	return EXIT_SUCCESS;
}
