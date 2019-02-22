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

#pragma once

#include "pointset.h"
#include "imageinfo.h"

#include <string>
#include <itkImage.h>

namespace nvidia {
namespace aiaa {

template<typename TPixel, unsigned int VImageDimension>
class ITKUtils {
 public:
  /// Pre Process wrt input image
  static PointSet imagePreProcess(const PointSet &pointSet, itk::Image<TPixel, VImageDimension> *itkImage, const std::string &outputImage,
                                  ImageInfo &imageInfo, double PAD, const Point& ROI);

  // Pre Process wrt input file
  static PointSet imagePreProcess(const PointSet &pointSet, const std::string &inputImage, const std::string &outputImage, ImageInfo &imageInfo,
                                  double PAD, const Point& ROI);

  /// Post Process
  static void imagePostProcess(const std::string &inputImage, const std::string &outputImage, const ImageInfo &imageInfo);

 private:
  static typename itk::Image<TPixel, VImageDimension>::Pointer resizeImage(itk::Image<TPixel, VImageDimension> *itkImage,
                                                                           typename itk::Image<TPixel, VImageDimension>::SizeType targetSize,
                                                                           bool linearInterpolate);
};

typedef ITKUtils<char, 2> ITKUtils2DChar;
typedef ITKUtils<char, 3> ITKUtils3DChar;
typedef ITKUtils<char, 4> ITKUtils4DChar;
typedef ITKUtils<unsigned char, 2> ITKUtils2DUChar;
typedef ITKUtils<unsigned char, 3> ITKUtils3DUChar;
typedef ITKUtils<unsigned char, 4> ITKUtils4DUChar;
typedef ITKUtils<short, 2> ITKUtils2DShort;
typedef ITKUtils<short, 3> ITKUtils3DShort;
typedef ITKUtils<short, 4> ITKUtils4DShort;
typedef ITKUtils<unsigned short, 2> ITKUtils2DUShort;
typedef ITKUtils<unsigned short, 3> ITKUtils3DUShort;
typedef ITKUtils<unsigned short, 4> ITKUtils4DUShort;
typedef ITKUtils<int, 2> ITKUtils2DInt;
typedef ITKUtils<int, 3> ITKUtils3DInt;
typedef ITKUtils<int, 4> ITKUtils4DInt;
typedef ITKUtils<unsigned int, 2> ITKUtils2DUInt;
typedef ITKUtils<unsigned int, 3> ITKUtils3DUInt;
typedef ITKUtils<unsigned int, 4> ITKUtils4DUInt;
typedef ITKUtils<long, 2> ITKUtils2DLong;
typedef ITKUtils<long, 3> ITKUtils3DLong;
typedef ITKUtils<long, 4> ITKUtils4DLong;
typedef ITKUtils<unsigned long, 2> ITKUtils2DULong;
typedef ITKUtils<unsigned long, 3> ITKUtils3DULong;
typedef ITKUtils<unsigned long, 4> ITKUtils4DULong;
typedef ITKUtils<float, 2> ITKUtils2DFloat;
typedef ITKUtils<float, 3> ITKUtils3DFloat;
typedef ITKUtils<float, 4> ITKUtils4DFloat;
typedef ITKUtils<double, 2> ITKUtils2DDouble;
typedef ITKUtils<double, 3> ITKUtils3DDouble;
typedef ITKUtils<double, 4> ITKUtils4DDouble;

}
}
