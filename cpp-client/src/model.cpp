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

#include "../include/nvidia/aiaa/model.h"
#include "../include/nvidia/aiaa/log.h"
#include "../include/nvidia/aiaa/utils.h"
#include "../include/nvidia/aiaa/exception.h"

#include <nlohmann/json.hpp>

namespace nvidia {
namespace aiaa {

const double DEFAULT_SIGMA = 3.0;
const double DEFAULT_PADDING = 20.0;
const int DEFAULT_ROI = 128;

Model::Model() {
  sigma = DEFAULT_SIGMA;
  padding = DEFAULT_PADDING;
  roi = {DEFAULT_ROI, DEFAULT_ROI, DEFAULT_ROI};
}

// {"labels": ["brain_tumor_core"], "internal name": "Dextr3dCroppedEngine", "description": "", "name": "Dextr3DBrainTC", "padding": 20.0 "roi": [128,128,128], "sigma": 3.0}

Model Model::fromJson(const std::string &json) {
  try {
    nlohmann::json j = nlohmann::json::parse(json);

    Model model;
    model.name = j["name"].get<std::string>();
    model.internal_name = j["internal name"].get<std::string>();
    model.description = j.find("description") != j.end() ? j["description"].get<std::string>() : j["decription"].get<std::string>();
    // TODO:: Ask server to correct the spelling for description

    for (auto e : j["labels"]) {
      model.labels.insert(e.get<std::string>());
    }

    model.sigma = j.find("sigma") != j.end() ? j["sigma"].get<double>() : DEFAULT_SIGMA;
    model.padding = j.find("padding") != j.end() ? j["padding"].get<double>() : DEFAULT_PADDING;

    model.roi.clear();
    if (j.find("roi") != j.end()) {
      for (auto n : j["roi"]) {
        model.roi.push_back(n);
      }
    }
    if (model.roi.empty()) {
      model.roi.push_back(DEFAULT_ROI);
    }
    while (model.roi.size() < 3) {
      model.roi.push_back(model.roi[model.roi.size() - 1]);
    }

    return model;
  } catch (nlohmann::json::parse_error& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  } catch (nlohmann::json::type_error& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  }
}

std::string Model::toJson(int space) const {
  nlohmann::json j;
  j["labels"] = labels;
  j["internal name"] = internal_name;
  j["description"] = description;
  j["name"] = name;
  j["sigma"] = sigma;
  j["padding"] = padding;
  j["roi"] = roi;

  std::string str = j.dump(space);
  if (!space) {
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
  }
  return str;
}

// [
//   {"labels": ["brain_tumor_core"], "internal name": "Dextr3dCroppedEngine", "description": "", "name": "Dextr3DBrainTC"},
//   {"labels": ["liver"], "internal name": "Dextr3dCroppedEngine", "description": "", "name": "Dextr3DLiver"},
//   {"labels": ["brain_whole_tumor"], "internal name": "Dextr3dCroppedEngine", "description": "", "name": "Dextr3DBrainWT"}
// ]

ModelList ModelList::fromJson(const std::string &json) {
  try {
    nlohmann::json j = nlohmann::json::parse(json);
    ModelList modelList;
    for (auto e : j) {
      modelList.models.push_back(Model::fromJson(e.dump()));
    }
    return modelList;
  } catch (nlohmann::json::parse_error& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  } catch (nlohmann::json::type_error& e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  }
}

std::string ModelList::toJson(int space) const {
  nlohmann::json j;
  for (auto m : models) {
    j.push_back(nlohmann::json::parse(m.toJson()));
  }

  return space ? j.dump(space) : j.dump();
}

Model ModelList::getMatchingModel(const std::string &labelName) {
  // Exact Match (first preference)
  for (auto model : models) {
    for (auto label : model.labels) {
      AIAA_LOG_DEBUG("Exact Match: " << label << " and " << labelName);
      if (Utils::iequals(labelName, label)) {
        return model;
      }
    }
  }

  // Prefix Match (find as prefix in either)
  std::string l1 = Utils::to_lower(labelName);
  for (auto model : models) {
    for (auto label : model.labels) {
      std::string l2 = Utils::to_lower(label);
      AIAA_LOG_DEBUG("Prefix Match: " << l2 << " and " << labelName);
      if (l1.find(l2) != std::string::npos || l2.find(l1) != std::string::npos) {
        return model;
      }
    }
  }

  return Model();
}

}
}

