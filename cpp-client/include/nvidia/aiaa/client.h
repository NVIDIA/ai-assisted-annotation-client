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
   @param[in] serverUri  AIAA Server end-point. For example: "http://10.110.45.66:5000/"
   @param[in] timeoutInSec  AIAA Server operation timeout. Default is 60 seconds
   @return Client object
   */
  Client(const std::string &serverUri, const int timeoutInSec = 60);

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
   @brief This API is used to Create New Session.  Input image will be saved as part of the created session.
   @param[in] inputImageFile  Filter models by matching label
   @param[in] expiry  Expiry in seconds.  min(AIAASessionExpiry, expiry) will be selected by AIAA
   @return String representing a valid session id for future use

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  std::string createSession(const std::string &inputImageFile, const int expiry = 0) const;

  /*!
   @brief This API is used to get an existing Session Info
   @param[in] sessionId  A valid session id of an existing AIAA session
   @return String representing session info (JSON format)

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  std::string getSession(const std::string &sessionId) const;

  /*!
   @brief This API is used to close an existing Session
   @param[in] sessionId  A valid session id of an existing AIAA session

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  void closeSession(const std::string &sessionId) const;

  /*!
   @brief This API is used to run segmentation on input image
   @param[in] model  Model to be used
   @param[in] inputImageFile  Input image filename which will be sent to AIAA for segmentation action
   @param[in] outputImageFile  Output image file where Result mask is stored
   @param[in] sessionId  If *session_id* is not empty then *inputImageFile* will be ignored
   @return PointSet object representing extreme points on label image

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  PointSet segmentation(const Model &model, const std::string &inputImageFile, const std::string &outputImageFile,
                        const std::string &sessionId = "") const;

  /*!
   @brief 3D image annotation using DEXTR3D method
   @param[in] model  Model to be used
   @param[in] pointSet  PointSet object which represents a set of extreme points in 3-Dimensional for the organ. Minimum Client::MIN_POINTS_FOR_SEGMENTATION are expected
   @param[in] inputImageFile  Input image filename which will be sent to AIAA for dextr3d action
   @param[in] outputImageFile  Output image file where Result mask is stored
   @param[in] preProcess  Pre-process input image (crop) for dextr3d before sending it to AIAA
   @param[in] sessionId  If *session_id* is not empty and preProcess is false then *inputImageFile* will be ignored

   @retval 0 Success
   @retval -1 Input Model name is empty
   @retval -2 Insufficient Points in the input

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  int dextr3D(const Model &model, const PointSet &pointSet, const std::string &inputImageFile, const std::string &outputImageFile, bool preProcess,
              const std::string &sessionId = "") const;

  /*!
   @brief This API is used to run deepgrow on input image
   @param[in] model  Model to be used
   @param[in] foregroundPointSet  PointSet object which represents a set of foreground points (+ve clicks)
   @param[in] backgroundPointSet  PointSet object which represents a set of background points (-ve clicks)
   @param[in] inputImageFile  Input image filename which will be sent to AIAA for deepgrow action
   @param[in] outputImageFile  Output image file where Result mask is stored
   @param[in] sessionId  If *session_id* is not empty then *inputImageFile* will be ignored

   @retval 0 Success
   @retval -1 Input Model name is empty
   @retval -2 Insufficient Points in the input

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.103 if case of ITK error related to image processing
   */
  int deepgrow(const Model &model, const PointSet &foregroundPointSet, const PointSet &backgroundPointSet, const std::string &inputImageFile,
               const std::string &outputImageFile, const std::string &sessionId = "") const;

  /*!
   @brief 3D binary mask to polygon representation conversion
   @param[in] pointRatio  Point Ratio
   @param[in] inputImageFile  Input image filename which will be sent to AIAA

   @return PolygonsList object representing a list of Polygons across each image slice

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  PolygonsList maskToPolygon(int pointRatio, const std::string &inputImageFile) const;

  /*!
   @brief 2D polygon update with single point edit
   @param[in] poly  Set of current or old Polygons
   @param[in] neighborhoodSize  NeighborHood Size for propagation (across polygons)
   @param[in] polyIndex  Polygon index which needs an update
   @param[in] vertexIndex  Vertex among the polygon which needs an update
   @param[in] vertexOffset  [x,y] offset which will be added to corresponding poly[polyIndex][vertexIndex][x,y] to the new polygon
   @param[in] inputImageFile  Input 2D Slice Image File in PNG format
   @param[in] outputImageFile  Output Image File in PNG format

   @return Polygons object representing set of updated polygons

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  Polygons fixPolygon(const Polygons &poly, int neighborhoodSize, int polyIndex, int vertexIndex, const int vertexOffset[2],
                      const std::string &inputImageFile, const std::string &outputImageFile) const;

  /*!
   @brief 3D polygon update with single point edit
   @param[in] poly  Set of current or old Polygons
   @param[in] neighborhoodSize  NeighborHood Size for propagation (across polygons)
   @param[in] neighborhoodSize3D  3D NeighborHood Size for propagation (across slices)
   @param[in] sliceIndex  Slice Index to get the corresponding polygons for editing
   @param[in] polyIndex  Polygon index among which needs an update
   @param[in] vertexIndex  Vertex among the polygon which needs an update
   @param[in] vertexOffset  [x,y] offset which will be added to corresponding poly[polyIndex][vertexIndex][x,y] to the new polygon
   @param[in] inputImageFile  Input 3D Slice Image File in NIFTI format
   @param[in] outputImageFile  Output Image File in NIFTI format

   @return Polygons object representing set of updated polygons

   @throw nvidia.aiaa.error.101 in case of connect error
   @throw nvidia.aiaa.error.102 if case of response parsing
   */
  PolygonsList fixPolygon(const PolygonsList &poly, int neighborhoodSize, int neighborhoodSize3D, int sliceIndex, int polyIndex, int vertexIndex,
                          const int vertexOffset[2], const std::string &inputImageFile, const std::string &outputImageFile) const;

  /// Minimum Number of Points required for segmentation/sampling
  static const int MIN_POINTS_FOR_SEGMENTATION;

 private:
  /// Server URI
  std::string serverUri;
  int timeoutInSec;
};

}
}
