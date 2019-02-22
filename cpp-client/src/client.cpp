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

#include "../include/nvidia/aiaa/client.h"
#include "../include/nvidia/aiaa/log.h"
#include "../include/nvidia/aiaa/utils.h"
#include "../include/nvidia/aiaa/curlutils.h"
#include "../include/nvidia/aiaa/itkutils.h"

namespace nvidia {
namespace aiaa {

const std::string EP_MODELS = "/models";
const std::string EP_DEXTRA_3D = "/dextr3d";
const std::string EP_MASK_2_POLYGON = "/mask2polygon";
const std::string EP_FIX_POLYGON = "/fixpolygon";
const std::string IMAGE_FILE_EXTENSION = ".nii.gz";

Client::Client(const std::string& uri)
    : serverUri(uri) {
  if (serverUri.back() == '/') {
    serverUri.pop_back();
  }
}

ModelList Client::models() const {
  std::string uri = serverUri + EP_MODELS;
  AIAA_LOG_DEBUG("URI: " << uri);

  return ModelList::fromJson(CurlUtils::doGet(uri));
}

template<typename TPixel>
PointSet samplingT(const Model &model, const PointSet &pointSet, void *inputImage, int dimension, const std::string &outputImageFile,
                   ImageInfo &imageInfo) {
  switch (dimension) {
    case 2: {
      typedef itk::Image<TPixel, 2> ImageType;
      ImageType *itkImage = static_cast<ImageType*>(inputImage);
      return ITKUtils<TPixel, 2>::imagePreProcess(pointSet, itkImage, outputImageFile, imageInfo, model.padding, model.roi);
    }
    case 3: {
      typedef itk::Image<TPixel, 3> ImageType;
      ImageType *itkImage = static_cast<ImageType*>(inputImage);
      return ITKUtils<TPixel, 3>::imagePreProcess(pointSet, itkImage, outputImageFile, imageInfo, model.padding, model.roi);
    }
    case 4: {
      typedef itk::Image<TPixel, 4> ImageType;
      ImageType *itkImage = static_cast<ImageType*>(inputImage);
      return ITKUtils<TPixel, 4>::imagePreProcess(pointSet, itkImage, outputImageFile, imageInfo, model.padding, model.roi);
    }
  }
  throw exception(exception::INVALID_ARGS_ERROR, "UnSupported Dimension (only 2D/3D/4D are supported)");
}

template<typename TPixel>
PointSet samplingT(const Model &model, const PointSet &pointSet, const std::string &inputImageFile, int dimension, const std::string &outputImageFile,
                   ImageInfo &imageInfo) {
  switch (dimension) {
    case 2: {
      return ITKUtils<TPixel, 2>::imagePreProcess(pointSet, inputImageFile, outputImageFile, imageInfo, model.padding, model.roi);
    }
    case 3: {
      return ITKUtils<TPixel, 3>::imagePreProcess(pointSet, inputImageFile, outputImageFile, imageInfo, model.padding, model.roi);
    }
    case 4: {
      return ITKUtils<TPixel, 4>::imagePreProcess(pointSet, inputImageFile, outputImageFile, imageInfo, model.padding, model.roi);
    }
  }
  throw exception(exception::INVALID_ARGS_ERROR, "UnSupported Dimension (only 2D/3D/4D are supported)");
}

PointSet Client::sampling(const Model &model, const PointSet &pointSet, void *inputImage, Pixel::Type pixelType, int dimension,
                          const std::string &outputImageFile, ImageInfo &imageInfo) const {
  if (pointSet.points.size() < MIN_POINTS_FOR_SEGMENTATION) {
    std::string msg = "Insufficient Points; Minimum Points required for input PointSet: " + std::to_string(MIN_POINTS_FOR_SEGMENTATION);
    AIAA_LOG_WARN(msg);
    throw exception(exception::INVALID_ARGS_ERROR, msg.c_str());
  }

  switch (pixelType) {
    case Pixel::CHAR:
      return samplingT<char>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::UCHAR:
      return samplingT<unsigned char>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::SHORT:
      return samplingT<short>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::USHORT:
      return samplingT<unsigned short>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::INT:
      return samplingT<int>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::UINT:
      return samplingT<unsigned int>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::LONG:
      return samplingT<long>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::ULONG:
      return samplingT<unsigned long>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::FLOAT:
      return samplingT<float>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::DOUBLE:
      return samplingT<double>(model, pointSet, inputImage, dimension, outputImageFile, imageInfo);
    case Pixel::UNKNOWN:
      throw exception(exception::INVALID_ARGS_ERROR, "UnSupported Pixel Type (only basic types are supported)");
  }
  throw exception(exception::INVALID_ARGS_ERROR, "UnSupported Pixel Type (only basic types are supported)");
}

PointSet Client::sampling(const Model &model, const PointSet &pointSet, const std::string &inputImageFile, Pixel::Type pixelType, int dimension,
                          const std::string &outputImageFile, ImageInfo &imageInfo) const {

  if (pointSet.points.size() < MIN_POINTS_FOR_SEGMENTATION) {
    std::string msg = "Insufficient Points; Minimum Points required for input PointSet: " + std::to_string(MIN_POINTS_FOR_SEGMENTATION);
    AIAA_LOG_WARN(msg);
    throw exception(exception::INVALID_ARGS_ERROR, msg.c_str());
  }

  switch (pixelType) {
    case Pixel::CHAR:
      return samplingT<char>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::UCHAR:
      return samplingT<unsigned char>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::SHORT:
      return samplingT<short>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::USHORT:
      return samplingT<unsigned short>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::INT:
      return samplingT<int>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::UINT:
      return samplingT<unsigned int>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::LONG:
      return samplingT<long>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::ULONG:
      return samplingT<unsigned long>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::FLOAT:
      return samplingT<float>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::DOUBLE:
      return samplingT<double>(model, pointSet, inputImageFile, dimension, outputImageFile, imageInfo);
    case Pixel::UNKNOWN:
      throw exception(exception::INVALID_ARGS_ERROR, "UnSupported Pixel Type (only basic types are supported)");
  }
  throw exception(exception::INVALID_ARGS_ERROR, "UnSupported Pixel Type (only basic types are supported)");
}

int Client::segmentation(const Model &model, const PointSet &pointSet, const std::string &inputImageFile, int dimension,
                      const std::string &outputImageFile, const ImageInfo &imageInfo) const {
  if (dimension != 3) {
    throw exception(exception::INVALID_ARGS_ERROR, "UnSupported Dimension (only 3D is supported)");
  }
  if (model.name.empty()) {
    AIAA_LOG_WARN("Selected model is EMPTY");
    return -2;
  }
  if (pointSet.points.size() < MIN_POINTS_FOR_SEGMENTATION) {
    std::string msg = "Insufficient Points; Minimum Points required for input PointSet: " + std::to_string(MIN_POINTS_FOR_SEGMENTATION);
    AIAA_LOG_WARN(msg);
    throw exception(exception::INVALID_ARGS_ERROR, msg.c_str());
  }

  bool postProcess = imageInfo.empty() ? false : true;
  std::string tmpResultFile = postProcess ? (Utils::tempfilename() + IMAGE_FILE_EXTENSION) : outputImageFile;
  AIAA_LOG_DEBUG("TmpResultFile: " << tmpResultFile << "; PostProcess: " << postProcess);

  std::string uri = serverUri + EP_DEXTRA_3D + "?model=" + model.name;
  std::string paramStr = "{\"sigma\":" + std::to_string(model.sigma) + ",\"points\":\"" + pointSet.toJson() + "\"}";
  std::string response = CurlUtils::doPost(uri, paramStr, inputImageFile, tmpResultFile);

  // Perform post-processing to recover crop and re-sample and save to user-specified location
  if (postProcess) {
    ITKUtils3DUChar::imagePostProcess(tmpResultFile, outputImageFile, imageInfo);
    std::remove(tmpResultFile.c_str());
  }
  return 0;
}

int Client::dextr3D(const Model &model, const PointSet &pointSet, const std::string &inputImageFile, Pixel::Type pixelType,
                    const std::string &outputImageFile) const {
  if (model.name.empty()) {
    AIAA_LOG_WARN("Selected model is EMPTY");
    return -2;
  }
  if (pointSet.points.size() < MIN_POINTS_FOR_SEGMENTATION) {
    std::string msg = "Insufficient Points; Minimum Points required for input PointSet: " + std::to_string(MIN_POINTS_FOR_SEGMENTATION);
    AIAA_LOG_WARN(msg);
    throw exception(exception::INVALID_ARGS_ERROR, msg.c_str());
  }

  std::string tmpSampleFile = Utils::tempfilename() + IMAGE_FILE_EXTENSION;

  AIAA_LOG_DEBUG("PointSet: " << pointSet.toJson());
  AIAA_LOG_DEBUG("Model: " << model.toJson());
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
  AIAA_LOG_DEBUG("PixelType: " << pixelType);
  AIAA_LOG_DEBUG("SampleImageFile: " << tmpSampleFile);
  AIAA_LOG_DEBUG("OutputImageFile: " << outputImageFile);

  // Perform pre-processing of crop/re-sample and re-compute point index
  const int dimension = 3;
  ImageInfo imageInfo;
  PointSet pointSetROI = sampling(model, pointSet, inputImageFile, pixelType, dimension, tmpSampleFile, imageInfo);
  segmentation(model, pointSetROI, tmpSampleFile, dimension, outputImageFile, imageInfo);

  // Cleanup temporary files
  std::remove(tmpSampleFile.c_str());
  return 0;
}

PolygonsList Client::maskToPolygon(int pointRatio, const std::string &inputImageFile) const {
  std::string uri = serverUri + EP_MASK_2_POLYGON;
  std::string paramStr = "{\"more_points\":" + std::to_string(pointRatio) + "}";

  AIAA_LOG_DEBUG("URI: " << uri);
  AIAA_LOG_DEBUG("Parameters: " << paramStr);
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);

  std::string response = CurlUtils::doPost(uri, paramStr, inputImageFile);
  AIAA_LOG_DEBUG("Response: \n" << response);

  PolygonsList polygonsList = PolygonsList::fromJson(response);
  return polygonsList;
}

Polygons Client::fixPolygon(const Polygons &newPoly, const Polygons &oldPrev, int neighborhoodSize, int polyIndex, int vertexIndex,
                            const std::string &inputImageFile, const std::string &outputImageFile) const {
  // NOTE:: Flip Input/Output Polygons to support AIAA server as it currently expects input in (y,x) format
  Polygons p1 = newPoly;
  p1.flipXY();

  Polygons p2 = oldPrev;
  p2.flipXY();

  std::string uri = serverUri + EP_FIX_POLYGON;
  std::string paramStr = "{\"propagate_neighbor\":" + std::to_string(neighborhoodSize) + ",";
  paramStr = paramStr + "\"polygonIndex\":" + std::to_string(polyIndex) + ",";
  paramStr = paramStr + "\"vertexIndex\":" + std::to_string(vertexIndex) + ",";
  paramStr = paramStr + "\"poly\":" + p1.toJson() + ",\"prev_poly\":" + p2.toJson() + "}";

  AIAA_LOG_DEBUG("URI: " << uri);
  AIAA_LOG_DEBUG("Parameters: " << paramStr);
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
  AIAA_LOG_DEBUG("OutputImageFile: " << outputImageFile);

  std::string response = CurlUtils::doPost(uri, paramStr, inputImageFile, outputImageFile);
  AIAA_LOG_DEBUG("Response: \n" << response);

  // TODO:: Ask AIAA Server to remove redundant [] to return the updated Polygons (same as input)
  Polygons result = PolygonsList::fromJson(response).list[0];
  result.flipXY();
  return result;
}
}
}
