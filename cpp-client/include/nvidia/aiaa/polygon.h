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

#include <vector>
#include <utility>
#include <string>

namespace nvidia {
namespace aiaa {

////////////
// Polygons //
////////////

/*!
 @brief AIAA Polygons
 */

struct AIAA_CLIENT_API Polygons {
  /// Type Definition for Point
  typedef std::vector<int> Point;

  /// Type Definition for Polygon
  typedef std::vector<Point> Polygon;

  /// Array of 2D Points to represent [[x,y]+]
  std::vector<Polygon> polys;

  /// Checks if polys is empty
  bool empty() const;

  /// Count of Polygons
  size_t size() const;

  /// Append new Polygon to polys list
  void push_back(Polygon poly);

  /// Flip X,Y points to Y,X
  void flipXY();

  /*!
   @brief create Model from JSON String
   @param[in] polygons  Polygons to compare against
   @param[in,out] polyIndex  First Polygon Index where the Polygon is not matching
   @param[in,out] vertexIndex  Vertex Index where the Point is not matching

   @return True if non-matching polygon + vertex is found
   */
  bool findFirstNonMatching(const Polygons &polygons, int &polyIndex, int &vertexIndex) const;

  /*!
   @brief create Model from JSON String
   @param[in] json  JSON String.

   Example:
   @code
   [ [[170, 66],[162, 73],[169, 77],[180, 76],[185, 68],[175, 66]], [[1,2]], [] ]
   @endcode

   @return Polygons object
   */
  static Polygons fromJson(const std::string &json);

  /*!
   @brief convert Polygons to JSON String
   @param[in] space  If space > 0; then JSON string will be formatted accordingly
   @return JSON String
   */
  std::string toJson(int space = 0) const;
};

/*!
 @brief AIAA List of Polygons
 */

struct AIAA_CLIENT_API PolygonsList {

  /// List of Polygons
  std::vector<Polygons> list;

  /// Checks if Polygons list is empty
  bool empty() const;

  /// Count of Polygons list
  size_t size() const;

  /// Append new Polygons to the list
  void push_back(Polygons polygons);

  /// Flip X,Y points to Y,X
  void flipXY();

  /*!
   @brief create PolygonsList from JSON String
   @param[in] json  JSON String.

   Example:
   @code
   [
   [],
   [[[169,66],[163,74],[173,77],[183,75],[184,68],[174,66]]],
   [[[169,66],[163,74],[172,78],[183,76],[184,69],[175,66]]]
   ]
   @endcode

   @return PolygonsList object
   */
  static PolygonsList fromJson(const std::string &json);

  /*!
   @brief convert PolygonsList to JSON String
   @param[in] space  If space > 0; then JSON string will be formatted accordingly
   @return JSON String
   */
  std::string toJson(int space = 0) const;
};

}
}
