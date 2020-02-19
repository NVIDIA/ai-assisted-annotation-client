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

#include "../include/nvidia/aiaa/pointset.h"
#include "../include/nvidia/aiaa/log.h"
#include "../include/nvidia/aiaa/exception.h"

#include <nlohmann/json.hpp>

namespace nvidia {
namespace aiaa {

// [[70,172,86],[105,161,180],[125,147,164],[56,174,124],[91,119,143],[77,219,120]]

bool PointSet::empty() const {
  return points.empty();
}

size_t PointSet::size() const {
  return points.size();
}

void PointSet::push_back(Point point) {
  points.push_back(point);
}

PointSet PointSet::fromJson(const std::string &json) {
  try {
    nlohmann::json j = nlohmann::json::parse(json);
    PointSet pointSet;
    for (auto e : j) {
      Point point;
      for (auto n : e) {
        point.push_back(n);
      }
      pointSet.push_back(point);
    }
    return pointSet;
  } catch (nlohmann::json::parse_error &e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  } catch (nlohmann::json::type_error &e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  }
}

std::string PointSet::toJson(int space) const {
  nlohmann::json j = points;
  return space ? j.dump(space) : j.dump();
}

}
}

