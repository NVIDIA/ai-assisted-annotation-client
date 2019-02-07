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
   @return Client object
   */
  Client(const std::string& serverUri);

  /*!
   @brief This API is used to fetch all the possible Models support by AIAA Server
   @return ModelList object representing a list of Models

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  ModelList models() const;

  /*!
   @brief This API is used to sample the input image
   @param[in] model  Model to be used
   @param[in] pointSet  Point3DSet object which represents a set of points in 3-Dimensional for the organ. Minimum Client::MIN_POINTS_FOR_DEXTR3D are expected
   @param[in] inputImageFile  Input image filename where image is stored in 3D format
   @param[in] outputImageFile  File name to store 3D binary mask image result from AIAA server
   @param[out] imageInfo  ImageInfo for sampled image
   @return Point3DSet object representing new PointSet for sampled image

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  Point3DSet sampling3d(const Model &model, const Point3DSet &pointSet, const std::string &inputImageFile, const std::string &outputImageFile, Image3DInfo &imageInfo) const;

  /*!
   @brief 3D image segmentation using DEXTR3D method
   @param[in] label  The organ name
   @param[in] pointSet  Point3DSet object which represents a set of points in 3-Dimensional for the organ. Minimum Client::MIN_POINTS_FOR_DEXTR3D are expected
   @param[in] inputImageFile  Input image filename where image is stored in 3D format
   @param[in] outputImageFile  File name to store 3D binary mask image result from AIAA server

   @retval 0 Success
   @retval -1 Insufficient Points in the input
   @retval -2 Can not find correct model for the input label

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  int dextr3d(const std::string &label, const Point3DSet &pointSet, const std::string &inputImageFile, const std::string &outputImageFile) const;

  /*!
   @brief 3D image segmentation using DEXTR3D method
   @param[in] label  The organ name
   @param[in] pointSet  Point3DSet object which represents a set of points in 3-Dimensional for the organ. Minimum Client::MIN_POINTS_FOR_DEXTR3D are expected
   @param[in] inputImageFile  Input image filename where image is stored in 3D format
   @param[in] outputImageFile  File name to store 3D binary mask image result from AIAA server
   @param[in] PAD  Padding to be used for image sampling (example: 20.0)
   @param[in] ROI_SIZE  image sample size to be sent to AIAA server in (X-coordinate x Y-coordinate x Z-coordinate) format (example: 128x128x128)
   @param[in] SIGMA  Sigma value for inference (example: 3.0)

   @retval 0 Success
   @retval -1 Insufficient Points in the input
   @retval -2 Can not find correct model for the input label

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  int dextr3d(const std::string &label, const Point3DSet &pointSet, const std::string &inputImageFile, const std::string &outputImageFile, double PAD, const std::string& ROI_SIZE,
              double SIGMA) const;

  /*!
   @brief 3D image segmentation using DEXTR3D method
   @param[in] model  Model to be used
   @param[in] pointSet  Point3DSet object which represents a set of points in 3-Dimensional for the organ. Minimum Client::MIN_POINTS_FOR_DEXTR3D are expected
   @param[in] inputImageFile  Input image filename where image is stored in 3D format
   @param[in] outputImageFile  File name to store 3D binary mask image result from AIAA server

   @retval 0 Success
   @retval -1 Insufficient Points in the input
   @retval -2 Input Model name is empty

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  int dextr3d(const Model &model, const Point3DSet &pointSet, const std::string &inputImageFile, const std::string &outputImageFile) const;

  /*!
   @brief 3D binary mask to polygon representation conversion
   @param[in] pointRatio  Point Ratio
   @param[in] inputImageFile  Input image filename where image is stored in 3D format

   @return PolygonsList object representing a list of Polygons across each image slice

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  PolygonsList mask2Polygon(int pointRatio, const std::string &inputImageFile) const;

  /*!
   @brief 2D polygon update with single point edit
   @param[in] newPoly  Set of new Polygons which needs update
   @param[in] oldPrev  Set of current or old Polygons
   @param[in] neighborhoodSize  NeighborHood Size for propagation
   @param[in] polyIndex  Polygon index among new Poly which needs an update
   @param[in] vertexIndex  Vertex among the polygon which needs an update
   @param[in] inputImageFile  Input 2D Slice Image File
   @param[in] outputImageFile  Output Image File

   @return Polygons object representing set of updated polygons

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  Polygons fixPolygon(const Polygons &newPoly, const Polygons &oldPrev, int neighborhoodSize, int polyIndex, int vertexIndex, const std::string &inputImageFile,
                      const std::string &outputImageFile) const;

  /// Minimum Number of Points required for dextr3d
  static const int MIN_POINTS_FOR_DEXTR3D = 6;

 private:
  /// Server URI
  std::string serverUri;
};

}
}
