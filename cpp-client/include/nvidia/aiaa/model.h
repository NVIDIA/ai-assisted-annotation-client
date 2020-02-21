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

#include <set>
#include <vector>
#include <string>

namespace nvidia {
namespace aiaa {

////////////
// Model //
////////////

/*!
 @brief AIAA Model
 */
struct AIAA_CLIENT_API Model {
  /// Set of Label strings supported by this particular AIAA model
  std::set<std::string> labels;

  /// Internal Name used by AIAA Server
  std::string internal_name;

  /// Detailed description
  std::string description;

  /// Official Name for the AIAA Model.  For example, this name shall be used for inference/dextr3d API
  std::string name;

  /// Padding used while training images;  This shall be used for Annotation Models
  double padding;

  /// Image ROI size used while training [x,y,z,w] Format;  This shall be used for Annotation Models
  std::vector<int> roi;

  enum ModelType {
    segmentation,
    annotation,
    classification,
    deepgrow,
    others,
    unknown
  };

  /// Type of Model (segmentation/annotation)
  ModelType type;

  /// Version of Model
  std::string version;

  /*!
   @brief create Model from JSON String
   @param[in] json  JSON String.

   Example:
   @code
   {"labels": ["brain_tumor_core"], "internal name": "Dextr3dCroppedEngine", "description": "", "name": "Dextr3DBrainTC"}
   @endcode

   @return Model object
   */
  static Model fromJson(const std::string &json);

  /*!
   @brief convert string to ModelType
   @param[in] type  String representing a valid model type
   @return ModelType
   */
  static ModelType toModelType(const std::string &type);

  /*!
   @brief convert ModelType to string
   @param[in] type  A valid model type
   @return string
   */
  static std::string toString(ModelType type);

  /*!
   @brief convert Model to JSON String
   @param[in] space  If space > 0; then JSON string will be formatted accordingly
   @return JSON String
   */
  std::string toJson(int space = 0) const;

  /// Default constructor
  Model();
};

/*!
 @brief AIAA ModelList

 This class provides APIs to connect to AIAA server and perform operations like dextra3d, fixPolygon etc...
 */
struct AIAA_CLIENT_API ModelList {
  /// List of Model Objects where each Model Object carries relevant information about the model being supported by AIAA Server
  std::vector<Model> models;

  /*!
   @brief Get the first matching model for a given label
   First preference goes to exact match.  Otherwise prefix match will be preferred.
   @note Here, the matching type is case-insensitive
   @param[in] label  Organ Name
   @param[in] type  Model Type
   @return Model
   */
  Model getMatchingModel(const std::string &label, Model::ModelType type = Model::annotation);

  /// Checks if Polygons list is empty
  bool empty() const;

  /// Count of Polygons list
  size_t size() const;

  /*!
   @brief create Model from JSON String
   @param[in] json  JSON String.

   Example:
   @code
   [
   {"labels": ["brain_tumor_core"], "internal name": "Dextr3dCroppedEngine", "description": "", "name": "Dextr3DBrainTC", "padding": 20.0 "roi": [128,128,128], "sigma": 3.0},
   {"labels": ["liver"], "internal name": "Dextr3dCroppedEngine", "description": "", "name": "Dextr3DLiver", "padding": 10.0 "roi": [96,96,96], "sigma": 3.0},
   {"labels": ["brain_whole_tumor"], "internal name": "Dextr3dCroppedEngine", "description": "", "name": "Dextr3DBrainWT", "padding": 20.0 "roi": [128,128,128], "sigma": 3.0}
   ]
   @endcode

   @return ModelList object
   */
  static ModelList fromJson(const std::string &json);

  /*!
   @brief convert ModelList to JSON String
   @param[in] space  If space > 0; then JSON string will be formatted accordingly
   @return JSON String
   */
  std::string toJson(int space = 0) const;
};

}
}
