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

#include "../include/nvidia/aiaa/log.h"
#include "../include/nvidia/aiaa/exception.h"

#include <itkResampleImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itkConstantPadImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkNearestNeighborInterpolateImageFunction.h>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageIOFactory.h>

#include <algorithm>
#include <sstream>
#include "../include/nvidia/aiaa/aiaautils.h"

namespace nvidia {
namespace aiaa {

template<class TImage>
typename TImage::Pointer resizeImage(typename TImage::Pointer image, typename TImage::SizeType targetSize, bool linearInterpolate) {

  auto imageSize = image->GetLargestPossibleRegion().GetSize();
  auto imageSpacing = image->GetSpacing();

  using ImageType = TImage;
  unsigned int dimension = image->GetImageDimension();
  constexpr unsigned int ImageDimension = 3;

  typename ImageType::SpacingType targetSpacing;
  for (unsigned int i = 0; i < dimension; i++) {
    targetSpacing[i] = imageSpacing[i] * (static_cast<double>(imageSize[i]) / static_cast<double>(targetSize[i]));
  }

  auto filter = itk::ResampleImageFilter<ImageType, ImageType>::New();
  filter->SetInput(image);

  filter->SetSize(targetSize);
  filter->SetOutputSpacing(targetSpacing);
  filter->SetTransform(itk::IdentityTransform<double, ImageDimension>::New());
  filter->SetOutputOrigin(image->GetOrigin());
  filter->SetOutputDirection(image->GetDirection());

  if (linearInterpolate) {
    filter->SetInterpolator(itk::LinearInterpolateImageFunction<ImageType, double>::New());
  } else {
    filter->SetInterpolator(itk::NearestNeighborInterpolateImageFunction<ImageType, double>::New());
  }

  filter->UpdateLargestPossibleRegion();
  return filter->GetOutput();
}

template<class TImage>
void readImage(const std::string &fileName, typename TImage::Pointer image) {
  using ImageType = TImage;
  using ImageReaderType = itk::ImageFileReader<ImageType>;

  typename ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(fileName.c_str());

  try {
    reader->Update();
    AIAA_LOG_DEBUG("Reading File completed: " << fileName);
  } catch (itk::ExceptionObject &e) {
    throw exception(exception::ITK_PROCESS_ERROR, "Failed to read Input Image");
  }

  image->Graft(reader->GetOutput());
}

template<class TImage>
PointSet preProcessImage(const PointSet &pointSet, typename TImage::Pointer image, const std::string &outputImage, ImageInfo &imageInfo, double PAD,
                         const Point &ROI) {
  using ImageType = TImage;
  unsigned int dimension = image->GetImageDimension();
  AIAA_LOG_DEBUG("Image Dimension: " << dimension);

  typename ImageType::SizeType imageSize = image->GetLargestPossibleRegion().GetSize();
  typename ImageType::IndexType indexMin;
  typename ImageType::IndexType indexMax;
  typename ImageType::SpacingType spacing = image->GetSpacing();
  for (unsigned int i = 0; i < dimension; i++) {
    indexMin[i] = INT_MAX;
    indexMax[i] = INT_MIN;
  }

  typename ImageType::IndexType index;
  int pointCount = 0;
  for (auto point : pointSet.points) {
    pointCount++;
    for (unsigned int i = 0; i < dimension; i++) {
      int vxPad = (int) (spacing[i] > 0 ? (PAD / spacing[i]) : PAD);
      if (pointCount == 1) {
        AIAA_LOG_DEBUG("[DIM " << i << "] Padding: " << PAD << "; Spacing: " << spacing[i] << "; VOXEL Padding: " << vxPad);
      }

      index[i] = point[i];
      indexMin[i] = std::min(std::max((int) (index[i] - vxPad), 0), (int) (indexMin[i]));
      indexMax[i] = std::max(std::min((int) (index[i] + vxPad), (int) (imageSize[i] - 1)), (int) (indexMax[i]));

      if (indexMin[i] > indexMax[i]) {
        AIAA_LOG_ERROR("Invalid PointSet w.r.t. input Image; [i=" << i << "] MinIndex: " << indexMin[i] << "; MaxIndex: " << indexMax[i]);
        throw exception(exception::INVALID_ARGS_ERROR, "Invalid PointSet w.r.t. input Image");
      }
    }
  }

  // Output min max index (ROI region)
  AIAA_LOG_DEBUG("Min index: " << indexMin << "; Max index: " << indexMax);

  // Extract ROI image
  typename ImageType::IndexType cropIndex;
  typename ImageType::SizeType cropSize;
  for (unsigned int i = 0; i < dimension; i++) {
    cropIndex[i] = indexMin[i];
    cropSize[i] = indexMax[i] - indexMin[i];

    imageInfo.cropSize[i] = cropSize[i];
    imageInfo.imageSize[i] = imageSize[i];
    imageInfo.cropIndex[i] = cropIndex[i];
  }

  AIAA_LOG_DEBUG("ImageInfo >>>> " << imageInfo.dump());

  auto cropFilter = itk::RegionOfInterestImageFilter<ImageType, ImageType>::New();
  cropFilter->SetInput(image);

  typename ImageType::RegionType cropRegion(cropIndex, cropSize);
  cropFilter->SetRegionOfInterest(cropRegion);
  cropFilter->Update();

  auto croppedItkImage = cropFilter->GetOutput();
  AIAA_LOG_DEBUG("++++ Cropped Image: " << croppedItkImage->GetLargestPossibleRegion());

  // Resize to 128x128x128x128
  typename ImageType::SizeType roiSize;
  for (unsigned int i = 0; i < dimension; i++) {
    roiSize[i] = ROI[i];
  }

  auto resampledImage = resizeImage<ImageType>(cropFilter->GetOutput(), roiSize, true);
  AIAA_LOG_DEBUG("ResampledImage completed");

  // Adjust extreme points index to cropped and resized image
  PointSet pointSetROI;
  for (auto p : pointSet.points) {
    for (unsigned int i = 0; i < dimension; i++) {
      index[i] = p[i];
    }

    // First convert to world coordinates within original image
    // Then convert back to index within resized image
    typename ImageType::PointType point;
    image->TransformIndexToPhysicalPoint(index, point);
    resampledImage->TransformPhysicalPointToIndex(point, index);

    Point pointROI;
    for (unsigned int i = 0; i < dimension; i++) {
      pointROI.push_back(index[i]);
    }
    pointSetROI.points.push_back(pointROI);
  }

  AIAA_LOG_DEBUG("PointSetROI: " << pointSetROI.toJson());

  // Write the ROI image out to temp folder
  auto writer = itk::ImageFileWriter<ImageType>::New();
  writer->SetInput(resampledImage);
  writer->SetFileName(outputImage);
  writer->Update();

  return pointSetROI;
}

template<class TImage>
PointSet postProcessImage(typename TImage::Pointer image, const std::string &outputImage, ImageInfo &imageInfo) {
  using ImageType = TImage;
  unsigned int dimension = image->GetImageDimension();
  AIAA_LOG_DEBUG("Image Dimension: " << dimension);

  auto segLocalImage = image;

  // Recover the ROI segmentation back to original image space for storage
  // Reverse the resize and crop operation for result image
  // Reverse the resizing by resizing
  typename ImageType::SizeType recoverSize;
  for (unsigned int i = 0; i < dimension; i++) {
    recoverSize[i] = imageInfo.cropSize[i];
  }

  AIAA_LOG_DEBUG("Recover resizing... ");
  typename ImageType::Pointer segLocalResizeImage;
  segLocalResizeImage = resizeImage<ImageType>(segLocalImage, recoverSize, false);

  // Reverse the cropping by adding proper padding
  typename ImageType::Pointer segRecoverImage = ImageType::New();
  typename ImageType::SizeType padLowerBound;
  typename ImageType::SizeType padUpperBound;
  for (unsigned int i = 0; i < dimension; i++) {
    padLowerBound[i] = imageInfo.cropIndex[i];
    padUpperBound[i] = imageInfo.imageSize[i] - imageInfo.cropSize[i] - imageInfo.cropIndex[i];
  }

  AIAA_LOG_DEBUG("Recover cropping... ");
  AIAA_LOG_DEBUG("Padding => Lower Bound: " << padLowerBound << "; Upper Bound: " << padUpperBound);

  typename ImageType::PixelType constantPixel = 0;

  auto padFilter = itk::ConstantPadImageFilter<ImageType, ImageType>::New();
  padFilter->SetInput(segLocalResizeImage);
  padFilter->SetPadLowerBound(padLowerBound);
  padFilter->SetPadUpperBound(padUpperBound);
  padFilter->SetConstant(constantPixel);
  padFilter->Update();

  segRecoverImage = padFilter->GetOutput();
  segRecoverImage->SetOrigin(segLocalImage->GetOrigin());

  auto writer = itk::ImageFileWriter<ImageType>::New();
  writer->SetInput(segRecoverImage);
  writer->SetFileName(outputImage);
  writer->Update();

  return PointSet();
}

template<unsigned int VDimension>
PointSet processImage(const itk::ImageIOBase::IOComponentType componentType, const PointSet &pointSet, const std::string &inputFileName,
                      const std::string &outputImage, ImageInfo &imageInfo, double PAD, const Point &ROI, bool pre) {
  switch (componentType) {
    case itk::ImageIOBase::UCHAR: {
      using PixelType = unsigned char;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      readImage<ImageType>(inputFileName, image);
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }

    case itk::ImageIOBase::CHAR: {
      using PixelType = char;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }

    case itk::ImageIOBase::USHORT: {
      using PixelType = unsigned short;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      readImage<ImageType>(inputFileName, image);
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }

    case itk::ImageIOBase::SHORT: {
      using PixelType = short;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      readImage<ImageType>(inputFileName, image);
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }

    case itk::ImageIOBase::UINT: {
      using PixelType = unsigned int;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      readImage<ImageType>(inputFileName, image);
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }

    case itk::ImageIOBase::INT: {
      using PixelType = int;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      readImage<ImageType>(inputFileName, image);
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }

    case itk::ImageIOBase::ULONG: {
      using PixelType = unsigned long;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      readImage<ImageType>(inputFileName, image);
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }

    case itk::ImageIOBase::LONG: {
      using PixelType = long;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      readImage<ImageType>(inputFileName, image);
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }

    case itk::ImageIOBase::FLOAT: {
      using PixelType = float;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      readImage<ImageType>(inputFileName, image);
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }

    case itk::ImageIOBase::DOUBLE: {
      using PixelType = double;
      using ImageType = itk::Image<PixelType, VDimension>;

      typename ImageType::Pointer image = ImageType::New();
      readImage<ImageType>(inputFileName, image);
      if (pre) {
        return preProcessImage<ImageType>(pointSet, image, outputImage, imageInfo, PAD, ROI);
      }
      return postProcessImage<ImageType>(image, outputImage, imageInfo);
    }
  }

  AIAA_LOG_ERROR("Unknown and unsupported component type!");
  throw exception(exception::ITK_PROCESS_ERROR, "Unknown and unsupported component type!");
}

PointSet imageProcess(const PointSet &pointSet, const std::string &inputImage, const std::string &outputImage, ImageInfo &imageInfo, double PAD,
                      const Point &ROI, bool pre = true) {

  try {
    AIAA_LOG_DEBUG("Input Image: " << inputImage);

    const char *inputFileName = inputImage.c_str();
    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(inputFileName, itk::ImageIOFactory::FileModeType::ReadMode);

    imageIO->SetFileName(inputFileName);
    imageIO->ReadImageInformation();

    using IOPixelType = itk::ImageIOBase::IOPixelType;
    const IOPixelType pixelType = imageIO->GetPixelType();

    AIAA_LOG_DEBUG("Pixel Type is " << itk::ImageIOBase::GetPixelTypeAsString(pixelType));

    using IOComponentType = itk::ImageIOBase::IOComponentType;
    const IOComponentType componentType = imageIO->GetComponentType();
    AIAA_LOG_DEBUG("Component Type is " << imageIO->GetComponentTypeAsString(componentType));

    const unsigned int imageDimension = imageIO->GetNumberOfDimensions();
    AIAA_LOG_DEBUG("Image Dimension is " << imageDimension);

    if (pixelType == itk::ImageIOBase::SCALAR) {
      if (imageDimension == 3) {
        return processImage<3>(componentType, pointSet, inputImage, outputImage, imageInfo, PAD, ROI, pre);
      }
    }

    AIAA_LOG_ERROR("ImageReader: not implemented yet!");
    throw exception(exception::ITK_PROCESS_ERROR, "ImageReader: not implemented yet!");

  } catch (itk::ExceptionObject &e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::ITK_PROCESS_ERROR, e.what());
  }
}

PointSet AiaaUtils::imagePreProcess(const PointSet &pointSet, const std::string &inputImage, const std::string &outputImage, ImageInfo &imageInfo,
                                    double PAD, const Point &ROI) {
  return imageProcess(pointSet, inputImage, outputImage, imageInfo, PAD, ROI, true);
}

void AiaaUtils::imagePostProcess(const std::string &inputImage, const std::string &outputImage, const ImageInfo &imageInfo) {
  ImageInfo info = imageInfo;
  imageProcess(PointSet(), inputImage, outputImage, info, 0.0, Point(), false);
}

}
}
