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

#include "common.h"
#include "model.h"
#include "pointset.h"
#include "polygon.h"
#include "imageinfo.h"
#include "exception.h"

#include <string>

namespace nvidia {
namespace aiaa {

////////////
// Client //
////////////

/*!
 @brief AIAA Client

 This class provides APIs to connect to AIAA server and perform operations like dextra3d, fixPolygon etc...
 */

class AIAA_CLIENT_API Client {
 public:

  /*!
   @brief create AIAA Client object
   @param[in] serverUri  AIAA Server end-point. For example: "http://10.110.45.66:5000/v1"
   @param[in] timeoutInSec  AIAA Server operation timeout. Default is 60 seconds
   @return Client object
   */
  Client(const std::string& serverUri, const int timeoutInSec = 60);

  /*!
   @brief This API is used to fetch all the possible Models support by AIAA Server
   @return ModelList object representing a list of Models

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  ModelList models() const;

  /*!
   @brief This API is used to fetch all the possible Models support by AIAA Server for matching label and model type
   @param[in] label  Filter models by matching label
   @param[in] type  Filter models by matching model type (segmentation/annotation)
   @return ModelList object representing a list of Models

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  ModelList models(const std::string &label, const Model::ModelType type) const;

  /*!
   @brief This API is used to sample the input image
   @param[in] model  Model to be used
   @param[in] pointSet  PointSet object which represents a set of points in 2D/3D/4D for the organ. Minimum Client::MIN_POINTS_FOR_SEGMENTATION are expected
   @param[in] inputImageFile  Input image filename where image is stored in 2D/3D/4D format
   @param[in] pixelType  Pixel::Type for Input Image
   @param[in] dimension  Dimension for Input Image
   @param[in] outputImageFile  Sampled Output Imaged stored in itk::Image<unsigned short, *> format
   @param[out] imageInfo  ImageInfo for sampled image
   @return PointSet object representing new PointSet for sampled image

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  PointSet sampling(const Model &model, const PointSet &pointSet, const std::string &inputImageFile, Pixel::Type pixelType, int dimension,
                    const std::string &outputImageFile, ImageInfo &imageInfo) const;

  /*!
   @brief This API is used to sample the input image
   @param[in] model  Model to be used
   @param[in] pointSet  PointSet object which represents a set of points in 3-Dimensional for the organ. Minimum Client::MIN_POINTS_FOR_SEGMENTATION are expected
   @param[in] inputImage  Input image pointer 2D/3D/4D format which is itk::Image<?, *> to avoid extra copy at client side
   @param[in] pixelType  Pixel::Type for Input Image
   @param[in] dimension  Dimension for Input Image
   @param[in] outputImageFile  Sampled Output Imaged stored in itk::Image<unsigned short, *> format
   @param[out] imageInfo  ImageInfo for sampled image
   @return PointSet object representing new PointSet for sampled image

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  PointSet sampling(const Model &model, const PointSet &pointSet, void *inputImage, Pixel::Type pixelType, int dimension,
                    const std::string &outputImageFile, ImageInfo &imageInfo) const;

  /*!
   @brief 3D image segmentation/annotation over sampled input image and PointSet depending on input Model
   @param[in] model  Model to be used (it can be for segmentation or annotation)
   @param[in] pointSet  PointSet object which represents a set of points in 3-Dimensional for the organ.
   @param[in] dimension  Dimension for Input Image
   @param[in] inputImageFile  Sampled Input image filename where image is stored in itk::Image<unsigned short, *> format
   @param[in] outputImageFile  File name to store 3D binary mask image result from AIAA server in itk::Image<unsigned char, *> format
   @param[in] imageInfo  Optional Original ImageInfo to recover in case of annotation models after inference

   @retval New/Updated Pointset in case of segmentation which represents a set of extreme points in 3-Dimensional for the organ.

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  PointSet segmentation(const Model &model, const PointSet &pointSet, const std::string &inputImageFile, int dimension, const std::string &outputImageFile,
                const ImageInfo &imageInfo = ImageInfo()) const;

  /*!
   @brief 3D image annotation using DEXTR3D method  (this combines sampling + segmentation into single operation for 3D images)
   @param[in] model  Model to be used
   @param[in] pointSet  PointSet object which represents a set of extreme points in 3-Dimensional for the organ. Minimum Client::MIN_POINTS_FOR_SEGMENTATION are expected
   @param[in] inputImageFile  Input image filename where image is stored in itk::Image<?, 3> format
   @param[in] pixelType  PixelType for Input Image
   @param[in] outputImageFile  File name to store 3D binary mask image result from AIAA server in itk::Image<unsigned char, 3> format

   @retval 0 Success
   @retval -1 Insufficient Points in the input
   @retval -2 Input Model name is empty

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  int dextr3D(const Model &model, const PointSet &pointSet, const std::string &inputImageFile, Pixel::Type pixelType,
              const std::string &outputImageFile) const;

  /*!
   @brief 3D binary mask to polygon representation conversion
   @param[in] pointRatio  Point Ratio
   @param[in] inputImageFile  Input image filename where image is stored in itk::Image<unsigned char, *> format

   @return PolygonsList object representing a list of Polygons across each image slice

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  PolygonsList maskToPolygon(int pointRatio, const std::string &inputImageFile) const;

  /*!
   @brief 2D polygon update with single point edit
   @param[in] newPoly  Set of new Polygons which needs update
   @param[in] oldPrev  Set of current or old Polygons
   @param[in] neighborhoodSize  NeighborHood Size for propagation
   @param[in] polyIndex  Polygon index among new Poly which needs an update
   @param[in] vertexIndex  Vertex among the polygon which needs an update
   @param[in] inputImageFile  Input 2D Slice Image File in PNG format
   @param[in] outputImageFile  Output Image File in PNG format

   @return Polygons object representing set of updated polygons

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  Polygons fixPolygon(const Polygons &newPoly, const Polygons &oldPrev, int neighborhoodSize, int polyIndex, int vertexIndex,
                      const std::string &inputImageFile, const std::string &outputImageFile) const;

  /// Minimum Number of Points required for segmentation/sampling
  static const int MIN_POINTS_FOR_SEGMENTATION;

 private:
  /// Server URI
  std::string serverUri;
  int timeoutInSec;
};

}
}
