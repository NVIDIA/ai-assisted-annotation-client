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

template<typename TPixel, unsigned int VImageDimension>
typename itk::Image<TPixel, VImageDimension>::Pointer ITKUtils<TPixel, VImageDimension>::resizeImage(
    itk::Image<TPixel, VImageDimension> *itkImage, typename itk::Image<TPixel, VImageDimension>::SizeType targetSize, bool linearInterpolate) {

  auto imageSize = itkImage->GetLargestPossibleRegion().GetSize();
  auto imageSpacing = itkImage->GetSpacing();

  typedef itk::Image<TPixel, VImageDimension> ImageType;
  typename ImageType::SpacingType targetSpacing;
  for (unsigned int i = 0; i < VImageDimension; i++) {
    targetSpacing[i] = imageSpacing[i] * (static_cast<double>(imageSize[i]) / static_cast<double>(targetSize[i]));
  }

  auto filter = itk::ResampleImageFilter<ImageType, ImageType>::New();
  filter->SetInput(itkImage);

  filter->SetSize(targetSize);
  filter->SetOutputSpacing(targetSpacing);
  filter->SetTransform(itk::IdentityTransform<double, VImageDimension>::New());
  filter->SetOutputOrigin(itkImage->GetOrigin());
  filter->SetOutputDirection(itkImage->GetDirection());

  if (linearInterpolate) {
    filter->SetInterpolator(itk::LinearInterpolateImageFunction<ImageType, double>::New());
  } else {
    filter->SetInterpolator(itk::NearestNeighborInterpolateImageFunction<ImageType, double>::New());
  }

  filter->UpdateLargestPossibleRegion();
  return filter->GetOutput();
}

template<typename TPixel, unsigned int VImageDimension>
PointSet ITKUtils<TPixel, VImageDimension>::imagePreProcess(const PointSet &pointSet, itk::Image<TPixel, VImageDimension> *itkImage,
                                                            const std::string &outputImage, ImageInfo &imageInfo, double PAD, const Point& ROI) {
  AIAA_LOG_DEBUG("Total Points: " << pointSet.points.size());
  AIAA_LOG_DEBUG("PAD: " << PAD);
  //AIAA_LOG_DEBUG("ROI: " << ROI);

  AIAA_LOG_DEBUG("Input Image: " << itkImage);
  AIAA_LOG_DEBUG("Input Image Region: " << itkImage->GetLargestPossibleRegion());

  try {
    // Preprocessing (crop and resize)
    typedef itk::Image<TPixel, VImageDimension> ImageType;
    typename ImageType::SizeType imageSize = itkImage->GetLargestPossibleRegion().GetSize();
    typename ImageType::IndexType indexMin;
    typename ImageType::IndexType indexMax;
    typename ImageType::SpacingType spacing = itkImage->GetSpacing();
    for (unsigned int i = 0; i < VImageDimension; i++) {
      indexMin[i] = INT_MAX;
      indexMax[i] = INT_MIN;
    }

    typename ImageType::IndexType index;
    int pointCount = 0;
    for (auto point : pointSet.points) {
      pointCount++;
      for (unsigned int i = 0; i < VImageDimension; i++) {
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
    for (unsigned int i = 0; i < VImageDimension; i++) {
      cropIndex[i] = indexMin[i];
      cropSize[i] = indexMax[i] - indexMin[i];

      imageInfo.cropSize[i] = cropSize[i];
      imageInfo.imageSize[i] = imageSize[i];
      imageInfo.cropIndex[i] = cropIndex[i];
    }

    AIAA_LOG_DEBUG("ImageInfo >>>> " << imageInfo.dump());

    auto cropFilter = itk::RegionOfInterestImageFilter<ImageType, ImageType>::New();
    cropFilter->SetInput(itkImage);

    typename ImageType::RegionType cropRegion(cropIndex, cropSize);
    cropFilter->SetRegionOfInterest(cropRegion);
    cropFilter->Update();

    auto croppedItkImage = cropFilter->GetOutput();
    AIAA_LOG_DEBUG("++++ Cropped Image: " << croppedItkImage->GetLargestPossibleRegion());

    // Resize to 128x128x128x128
    typename ImageType::SizeType roiSize;
    for (unsigned int i = 0; i < VImageDimension; i++) {
      roiSize[i] = ROI[i];
    }

    auto resampledImage = resizeImage(cropFilter->GetOutput(), roiSize, true);
    AIAA_LOG_DEBUG("ResampledImage completed");

    // Adjust extreme points index to cropped and resized image
    PointSet pointSetROI;
    for (auto p : pointSet.points) {
      for (unsigned int i = 0; i < VImageDimension; i++) {
        index[i] = p[i];
      }

      // First convert to world coordinates within original image
      // Then convert back to index within resized image
      typename ImageType::PointType point;
      itkImage->TransformIndexToPhysicalPoint(index, point);
      resampledImage->TransformPhysicalPointToIndex(point, index);

      Point pointROI;
      for (unsigned int i = 0; i < VImageDimension; i++) {
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
  } catch (itk::ExceptionObject& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::ITK_PROCESS_ERROR, e.what());
  }
}

template<typename TPixel, unsigned int VImageDimension>
PointSet ITKUtils<TPixel, VImageDimension>::imagePreProcess(const PointSet &inputPointSet, const std::string &inputImageName,
                                                            const std::string &outputImageName, ImageInfo &imageInfo, double PAD, const Point& ROI) {
  // Preprocessing (crop and resize)
  typedef itk::Image<TPixel, VImageDimension> ImageType;

  // Instantiate ITK filters
  auto reader = itk::ImageFileReader<ImageType>::New();
  reader->SetFileName(inputImageName);
  reader->Update();
  auto itkImage = reader->GetOutput();

  AIAA_LOG_DEBUG("Reading File completed: " << inputImageName);
  AIAA_LOG_DEBUG("++++ Input Image: " << itkImage->GetLargestPossibleRegion());

  return imagePreProcess(inputPointSet, itkImage, outputImageName, imageInfo, PAD, ROI);
}

template<typename TPixel, unsigned int VImageDimension>
void ITKUtils<TPixel, VImageDimension>::imagePostProcess(const std::string &inputImageName, const std::string &outputImageName,
                                                         const ImageInfo &imageInfo) {
  try {
    typedef itk::Image<TPixel, VImageDimension> ImageType;

    // Instantiate ITK filters
    auto reader = itk::ImageFileReader<ImageType>::New();
    reader->SetFileName(inputImageName);
    reader->Update();

    auto segLocalImage = reader->GetOutput();

    // Recover the ROI segmentation back to original image space for storage
    // Reverse the resize and crop operation for result image
    // Reverse the resizing by resizing
    typename ImageType::SizeType recoverSize;
    for (unsigned int i = 0; i < VImageDimension; i++) {
      recoverSize[i] = imageInfo.cropSize[i];
    }

    AIAA_LOG_DEBUG("Recover resizing... ");
    typename ImageType::Pointer segLocalResizeImage;
    segLocalResizeImage = resizeImage(segLocalImage, recoverSize, false);

    // Reverse the cropping by adding proper padding
    typename ImageType::Pointer segRecoverImage = ImageType::New();
    typename ImageType::SizeType padLowerBound;
    typename ImageType::SizeType padUpperBound;
    for (unsigned int i = 0; i < VImageDimension; i++) {
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
    writer->SetFileName(outputImageName);
    writer->Update();
  } catch (itk::ExceptionObject& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::ITK_PROCESS_ERROR, e.what());
  }
}

template class ITKUtils<char, 2> ;
template class ITKUtils<char, 3> ;
template class ITKUtils<char, 4> ;
template class ITKUtils<unsigned char, 2> ;
template class ITKUtils<unsigned char, 3> ;
template class ITKUtils<unsigned char, 4> ;
template class ITKUtils<short, 2> ;
template class ITKUtils<short, 3> ;
template class ITKUtils<short, 4> ;
template class ITKUtils<unsigned short, 2> ;
template class ITKUtils<unsigned short, 3> ;
template class ITKUtils<unsigned short, 4> ;
template class ITKUtils<int, 2> ;
template class ITKUtils<int, 3> ;
template class ITKUtils<int, 4> ;
template class ITKUtils<unsigned int, 2> ;
template class ITKUtils<unsigned int, 3> ;
template class ITKUtils<unsigned int, 4> ;
template class ITKUtils<long, 2> ;
template class ITKUtils<long, 3> ;
template class ITKUtils<long, 4> ;
template class ITKUtils<unsigned long, 2> ;
template class ITKUtils<unsigned long, 3> ;
template class ITKUtils<unsigned long, 4> ;
template class ITKUtils<float, 2> ;
template class ITKUtils<float, 3> ;
template class ITKUtils<float, 4> ;
template class ITKUtils<double, 2> ;
template class ITKUtils<double, 3> ;
template class ITKUtils<double, 4> ;

}
}
