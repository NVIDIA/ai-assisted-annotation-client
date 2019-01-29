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

#include "../include/nvidia/aiaa/itkutils.h"
#include "../include/nvidia/aiaa/log.h"
#include "../include/nvidia/aiaa/exception.h"

#include <itkResampleImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itkConstantPadImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkNearestNeighborInterpolateImageFunction.h>

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

#include <algorithm>
#include <sstream>

namespace nvidia {
namespace aiaa {

std::string ImageInfo::dump() {
  std::stringstream ss;
  ss << "ImageSize: [" << imageSize[0] << "," << imageSize[1] << "," << imageSize[2] << "]; ";
  ss << "CropSize: [" << cropSize[0] << "," << cropSize[1] << "," << cropSize[2] << "]; ";
  ss << "CropIndex: [" << cropIndex[0] << "," << cropIndex[1] << "," << cropIndex[2] << "]; ";

  return ss.str();
}

template<typename TPixel, unsigned int VImageDimension>
typename itk::Image<TPixel, VImageDimension>::Pointer ITKUtils::resizeImage(itk::Image<TPixel, VImageDimension> *itkImage,
                                                                            typename itk::Image<TPixel, VImageDimension>::SizeType targetSize, bool linearInterpolate) {

  typedef itk::Image<TPixel, VImageDimension> InputImageType;

  auto imageSize = itkImage->GetLargestPossibleRegion().GetSize();
  auto imageSpacing = itkImage->GetSpacing();

  typename InputImageType::SpacingType targetSpacing;
  for (unsigned int i = 0; i < VImageDimension; i++) {
    targetSpacing[i] = imageSpacing[i] * (static_cast<double>(imageSize[i]) / static_cast<double>(targetSize[i]));
  }

  auto filter = itk::ResampleImageFilter<InputImageType, InputImageType>::New();
  filter->SetInput(itkImage);

  filter->SetSize(targetSize);
  filter->SetOutputSpacing(targetSpacing);
  filter->SetTransform(itk::IdentityTransform<double, VImageDimension>::New());
  filter->SetOutputOrigin(itkImage->GetOrigin());
  filter->SetOutputDirection(itkImage->GetDirection());

  if (linearInterpolate) {
    filter->SetInterpolator(itk::LinearInterpolateImageFunction<InputImageType, double>::New());
  } else {
    filter->SetInterpolator(itk::NearestNeighborInterpolateImageFunction<InputImageType, double>::New());
  }

  filter->UpdateLargestPossibleRegion();
  return filter->GetOutput();
}

template<class TImageType>
typename TImageType::Pointer ITKUtils::getLargestConnectedComponent(TImageType *itkImage) {
  auto connected = itk::ConnectedComponentImageFilter<TImageType, TImageType>::New();
  connected->SetInput(itkImage);
  connected->Update();

  AIAA_LOG_DEBUG("Number of objects: " << connected->GetObjectCount());

  auto filter = itk::LabelShapeKeepNObjectsImageFilter < TImageType > ::New();
  filter->SetInput(connected->GetOutput());

  filter->SetBackgroundValue(0);
  filter->SetNumberOfObjects(1);
  filter->SetAttribute(itk::LabelShapeKeepNObjectsImageFilter < TImageType > ::LabelObjectType::NUMBER_OF_PIXELS);

  filter->Update();
  return filter->GetOutput();
}

Point3DSet ITKUtils::imagePreProcess(const Point3DSet &inputPointSet, const std::string &inputImageName, const std::string &outputImageName, ImageInfo &imageInfo, double PAD,
                                     const std::vector<int>& ROI_SIZE) {
  AIAA_LOG_DEBUG("Total Points: " << inputPointSet.points.size());
  AIAA_LOG_DEBUG("PAD: " << PAD);
  AIAA_LOG_DEBUG("ROI_SIZE: " << ROI_SIZE[0] << " x " << ROI_SIZE[1] << " x " << ROI_SIZE[2]);

  try {
    // Preprocessing (crop and resize)
    typedef itk::Image<int, DIM3> ImageType;

    // Instantiate ITK filters
    auto reader = itk::ImageFileReader < ImageType > ::New();
    reader->SetFileName(inputImageName);
    reader->Update();
    auto itkImage = reader->GetOutput();

    AIAA_LOG_DEBUG("Reading File completed: " << inputImageName);
    AIAA_LOG_DEBUG("++++ Input Image: " << itkImage->GetLargestPossibleRegion());

    typename ImageType::SizeType imageSize = itkImage->GetLargestPossibleRegion().GetSize();
    typename ImageType::IndexType indexMin = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
    typename ImageType::IndexType indexMax = { 0, 0, 0 };
    typename ImageType::SpacingType spacing = itkImage->GetSpacing();

    typename ImageType::IndexType index;
    int pointCount = 0;
    for (auto point : inputPointSet.points) {
      pointCount++;
      for (unsigned int i = 0; i < DIM3; i++) {
        int vxPad = (int) (spacing[i] > 0 ? (PAD / spacing[i]) : PAD);
        if (pointCount == 1) {
          AIAA_LOG_DEBUG("[DIM " << i << "] Padding: " << PAD << "; Spacing: " << spacing[i] << "; VOXEL Padding: " << vxPad);
        }

        index[i] = point[i];
        indexMin[i] = std::min(std::max((int) (index[i] - vxPad), 0), (int) (indexMin[i]));
        indexMax[i] = std::max(std::min((int) (index[i] + vxPad), (int) (imageSize[i] - 1)), (int) (indexMax[i]));

        assert(indexMin[i] < indexMax[i]);
      }
    }

    // Output min max index (ROI region)
    AIAA_LOG_DEBUG("Min index: " << indexMin << "; Max index: " << indexMax);

    // Extract ROI image
    typename ImageType::IndexType cropIndex;
    typename ImageType::SizeType cropSize;
    for (int i = 0; i < DIM3; i++) {
      cropIndex[i] = indexMin[i];
      cropSize[i] = indexMax[i] - indexMin[i];

      imageInfo.cropSize[i] = cropSize[i];
      imageInfo.imageSize[i] = imageSize[i];
      imageInfo.cropIndex[i] = cropIndex[i];
    }

    AIAA_LOG_DEBUG("ImageInfo >>>> " << imageInfo.dump());

    auto cropFilter = itk::RegionOfInterestImageFilter<ImageType, ImageType>::New();
    cropFilter->SetInput(reader->GetOutput());

    typename ImageType::RegionType cropRegion(cropIndex, cropSize);
    cropFilter->SetRegionOfInterest(cropRegion);
    cropFilter->Update();

    auto croppedItkImage = cropFilter->GetOutput();
    AIAA_LOG_DEBUG("++++ Cropped Image: " << croppedItkImage->GetLargestPossibleRegion());

    // Resize to 128x128x128
    typename ImageType::SizeType roiSize;
    for (int i = 0; i < DIM3; i++) {
      roiSize[i] = ROI_SIZE[i];
    }

    auto resampledImage = ITKUtils::resizeImage<int, DIM3>(cropFilter->GetOutput(), roiSize, true);
    AIAA_LOG_DEBUG("ResampledImage completed");

    // Adjust extreme points index to cropped and resized image
    Point3DSet pointSetROI;
    for (auto p : inputPointSet.points) {
      for (int i = 0; i < DIM3; i++) {
        index[i] = p[i];
      }

      // First convert to world coordinates within original image
      // Then convert back to index within resized image
      typename ImageType::PointType point;
      reader->GetOutput()->TransformIndexToPhysicalPoint(index, point);
      resampledImage->TransformPhysicalPointToIndex(point, index);

      Point3DSet::Point3D pointROI;
      for (int i = 0; i < DIM3; i++) {
        pointROI.push_back(index[i]);
      }
      pointSetROI.points.push_back(pointROI);
    }

    AIAA_LOG_DEBUG("pointSetROI: " << pointSetROI.toJson());

    // Write the ROI image out to temp folder
    auto writer = itk::ImageFileWriter < ImageType > ::New();
    writer->SetInput(resampledImage);
    writer->SetFileName(outputImageName);
    writer->Update();

    return pointSetROI;
  } catch (itk::ExceptionObject& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::ITK_PROCESS_ERROR, e.what());
  }
}

void ITKUtils::imagePostProcess(const std::string &inputImageName, const std::string &outputImageName, const ImageInfo &imageInfo) {
  try {
    typedef itk::Image<unsigned char, DIM3> ImageType;

    // Instantiate ITK filters
    auto reader = itk::ImageFileReader < ImageType > ::New();
    reader->SetFileName(inputImageName);
    reader->Update();

    // Select largest connected component
    auto segLocalImage = ITKUtils::getLargestConnectedComponent<ImageType>(reader->GetOutput());

    // Recover the ROI segmentation back to original image space for storage
    // Reverse the resize and crop operation for result image
    // Reverse the resizing by resizing
    typename ImageType::SizeType recoverSize;
    for (unsigned int i = 0; i < DIM3; i++) {
      recoverSize[i] = imageInfo.cropSize[i];
    }

    AIAA_LOG_DEBUG("Recover resizing... ");
    typename ImageType::Pointer segLocalResizeImage;
    segLocalResizeImage = ITKUtils::resizeImage<ImageType::PixelType, DIM3>(segLocalImage, recoverSize, false);

    // Reverse the cropping by adding proper padding
    typename ImageType::Pointer segRecoverImage = ImageType::New();
    typename ImageType::SizeType padLowerBound;
    typename ImageType::SizeType padUpperBound;
    for (unsigned int i = 0; i < DIM3; i++) {
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

    auto writer = itk::ImageFileWriter < ImageType > ::New();
    writer->SetInput(segRecoverImage);
    writer->SetFileName(outputImageName);
    writer->Update();
  } catch (itk::ExceptionObject& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::ITK_PROCESS_ERROR, e.what());
  }
}

}
}
