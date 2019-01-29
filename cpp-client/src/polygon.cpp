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

#include "../include/nvidia/aiaa/polygon.h"
#include "../include/nvidia/aiaa/log.h"
#include "../include/nvidia/aiaa/exception.h"

#include <nlohmann/json.hpp>

namespace nvidia {
namespace aiaa {

// [ [[170, 66],[162, 73],[169, 77],[180, 76],[185, 68],[175, 66]], [[1,2]], [] ]
bool Polygons::empty() const {
  return polys.empty();
}

size_t Polygons::size() const {
  return polys.size();
}

void Polygons::push_back(Polygon poly) {
  polys.push_back(poly);
}

void Polygons::flipXY() {
  for (auto& poly : polys) {
    for (auto& point : poly) {
      if (!point.empty()) {
        int t = point[0];
        point[0] = point[1];
        point[1] = t;
      }
    }
  }
}

bool Polygons::findFirstNonMatching(const Polygons &polygons, int &polyIndex, int &vertexIndex) const {
  for (size_t i = 0; i < polys.size() && i < polygons.polys.size(); i++) {
    const Polygon& p1 = polys[i];
    const Polygon& p2 = polygons.polys[i];

    for (size_t j = 0; j < p1.size() && p2.size(); j++) {
      const Point& pt1 = p1[j];
      const Point& pt2 = p2[j];

      for (size_t k = 0; k < pt1.size() && k < pt2.size(); k++) {
        if (pt1[k] != pt2[k]) {
          polyIndex = (int) i;
          vertexIndex = (int) j;
          return true;
        }
      }
    }
  }

  return false;
}

Polygons Polygons::fromJson(const std::string &json) {
  try {
    nlohmann::json j = nlohmann::json::parse(json);

    Polygons polygons;
    for (auto poly : j) {
      Polygons::Polygon polygon;
      for (auto pt : poly) {
        Polygons::Point point;
        for (auto n : pt) {
          point.push_back(n);
        }
        polygon.push_back(point);
      }
      polygons.push_back(polygon);
    }

    return polygons;
  } catch (nlohmann::json::parse_error& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  } catch (nlohmann::json::type_error& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  }
}

std::string Polygons::toJson(int space) const {
  nlohmann::json j = polys;
  return space ? j.dump(space) : j.dump();
}

bool PolygonsList::empty() const {
  return list.empty();
}

size_t PolygonsList::size() const {
  return list.size();
}

void PolygonsList::push_back(Polygons polygons) {
  list.push_back(polygons);
}

void PolygonsList::flipXY() {
  for (auto& polys : list) {
    polys.flipXY();
  }
}

// [[], [ [[170, 66],[162, 73],[169, 77],[180, 76],[185, 68],[175, 66]], [[1,2]], [] ]]
PolygonsList PolygonsList::fromJson(const std::string &json) {
  try {
    nlohmann::json j = nlohmann::json::parse(json);

    PolygonsList polygonsList;
    for (auto p : j) {
      Polygons polygons = Polygons::fromJson(p.dump());
      polygonsList.push_back(polygons);
    }

    return polygonsList;
  } catch (nlohmann::json::parse_error& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  } catch (nlohmann::json::type_error& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  }
}

std::string PolygonsList::toJson(int space) const {
  nlohmann::json j;
  for (auto p : list) {
    j.push_back(nlohmann::json::parse(p.toJson()));
  }

  return space ? j.dump(space) : j.dump();
}

}
}

std::ostream& operator<<(std::ostream& os, const nvidia::aiaa::Polygons& p) {
  os << p.toJson();
  return os;
}

std::ostream& operator<<(std::ostream& os, const nvidia::aiaa::PolygonsList& p) {
  os << p.toJson();
  return os;
}

