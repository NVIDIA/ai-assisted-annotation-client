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

#include "../include/nvidia/aiaa/aiaautils.h"
#include "../include/nvidia/aiaa/log.h"
#include "../include/nvidia/aiaa/utils.h"
#include "../include/nvidia/aiaa/curlutils.h"

#include <nlohmann/json.hpp>
#include <set>

namespace nvidia {
namespace aiaa {

const std::string EP_MODELS = "/v1/models";
const std::string EP_DEXTRA_3D = "/v1/dextr3d";
const std::string EP_DEEPGROW = "/v1/deepgrow";
const std::string EP_SEGMENTATION = "/v1/segmentation";
const std::string EP_MASK_TO_POLYGON = "/v1/mask2polygon";
const std::string EP_FIX_POLYGON = "/v1/fixpolygon";
const std::string EP_SESSION = "/session/";

const std::string IMAGE_FILE_EXTENSION = ".nii.gz";

const int Client::MIN_POINTS_FOR_SEGMENTATION = 6;

class AutoRemoveFiles {
  std::set<std::string> files;

 public:
  void add(const std::string &f) {
    files.insert(f);
  }

  ~AutoRemoveFiles() {
    for (auto it = files.begin(); it != files.end(); it++) {
      std::remove((*it).c_str());
    }
  }
};

Client::Client(const std::string &uri, int timeout)
    :
    serverUri(uri),
    timeoutInSec(timeout) {
  while (serverUri.back() == '/') {
    serverUri.pop_back();
  }
}

ModelList Client::models() const {
  std::string uri = serverUri + EP_MODELS;
  AIAA_LOG_DEBUG("URI: " << uri);

  return ModelList::fromJson(CurlUtils::doMethod("GET", uri, timeoutInSec));
}

ModelList Client::models(const std::string &label, const Model::ModelType type) const {
  std::string uri = serverUri + EP_MODELS;
  bool first = true;
  if (!label.empty()) {
    uri += "?label=" + CurlUtils::encode(label);
    first = false;
  }

  if (type != Model::unknown) {
    uri += (first ? "?" : "&");
    uri += std::string("type=") + Model::toString(type);
  }
  AIAA_LOG_DEBUG("URI: " << uri);

  return ModelList::fromJson(CurlUtils::doMethod("GET", uri, timeoutInSec));
}

PointSet Client::segmentation(const Model &model, const std::string &inputImageFile, const std::string &outputImageFile,
                              const std::string &sessionId) const {
  if (model.name.empty()) {
    AIAA_LOG_WARN("Selected model is EMPTY");
    throw exception(exception::INVALID_ARGS_ERROR, "Model is EMPTY");
  }

  AIAA_LOG_DEBUG("Model: " << model.toJson());
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
  AIAA_LOG_DEBUG("OutputImageFile: " << outputImageFile);
  AIAA_LOG_DEBUG("SessionId: " << sessionId);

  std::string m = CurlUtils::encode(model.name);
  std::string uri = serverUri + EP_SEGMENTATION + "?model=" + m;
  if (!sessionId.empty()) {
    uri += "&session_id=" + CurlUtils::encode(sessionId);
  }
  std::string paramStr = "{}";

  std::string response = CurlUtils::doMethod("POST", uri, paramStr, inputImageFile, outputImageFile, timeoutInSec);
  return PointSet::fromJson(response);
}

int Client::dextr3D(const Model &model, const PointSet &pointSet, const std::string &inputImageFile, const std::string &outputImageFile,
                    bool preProcess, const std::string &sessionId) const {
  if (model.name.empty()) {
    AIAA_LOG_WARN("Selected model is EMPTY");
    return -1;
  }

  if (pointSet.points.size() < MIN_POINTS_FOR_SEGMENTATION) {
    std::string msg = "Minimum Points required for input PointSet: " + Utils::lexical_cast<std::string>(MIN_POINTS_FOR_SEGMENTATION);
    AIAA_LOG_WARN(msg);
    return -2;
  }

  AIAA_LOG_DEBUG("PointSet: " << pointSet.toJson());
  AIAA_LOG_DEBUG("Model: " << model.toJson());
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
  AIAA_LOG_DEBUG("OutputImageFile: " << outputImageFile);
  AIAA_LOG_DEBUG("PreProcess: " << preProcess);
  AIAA_LOG_DEBUG("SessionId: " << sessionId);

  // Pre process
  ImageInfo imageInfo;
  std::string croppedInputFile = inputImageFile;
  std::string croppedOutputFile = outputImageFile;
  PointSet pointSetROI = pointSet;
  AutoRemoveFiles autoRemoveFiles;

  if (preProcess) {
    croppedInputFile = Utils::tempfilename();
    croppedOutputFile = Utils::tempfilename();
    pointSetROI = AiaaUtils::imagePreProcess(pointSet, inputImageFile, croppedInputFile, imageInfo, model.padding, model.roi);

    autoRemoveFiles.add(croppedInputFile);
  }

  std::string m = CurlUtils::encode(model.name);
  std::string uri = serverUri + EP_DEXTRA_3D + "?model=" + m;
  if (!preProcess && !sessionId.empty()) {
    uri += "&session_id=" + CurlUtils::encode(sessionId);
  }
  std::string paramStr = "{\"points\":\"" + pointSetROI.toJson() + "\"}";

  CurlUtils::doMethod("POST", uri, paramStr, croppedInputFile, croppedOutputFile, timeoutInSec);
  if (preProcess) {
    autoRemoveFiles.add(croppedOutputFile);
    AiaaUtils::imagePostProcess(croppedInputFile, croppedOutputFile, imageInfo);
  }

  return 0;
}

int Client::deepgrow(const Model &model, const PointSet &foregroundPointSet, const PointSet &backgroundPointSet, const std::string &inputImageFile,
                     const std::string &outputImageFile, const std::string &sessionId) const {
  if (model.name.empty()) {
    AIAA_LOG_WARN("Selected model is EMPTY");
    return -1;
  }

  if (foregroundPointSet.points.empty() && backgroundPointSet.points.empty()) {
    std::string msg = "Neither foreground nor background points are provided";
    AIAA_LOG_WARN(msg);
    return -2;
  }

  AIAA_LOG_DEBUG("Model: " << model.toJson());
  AIAA_LOG_DEBUG("Foreground: " << foregroundPointSet.toJson());
  AIAA_LOG_DEBUG("Background: " << backgroundPointSet.toJson());
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
  AIAA_LOG_DEBUG("OutputImageFile: " << outputImageFile);
  AIAA_LOG_DEBUG("SessionId: " << sessionId);

  std::string m = CurlUtils::encode(model.name);
  std::string uri = serverUri + EP_DEEPGROW + "?model=" + m;
  if (!sessionId.empty()) {
    uri += "&session_id=" + CurlUtils::encode(sessionId);
  }
  std::string paramStr = "{\"foreground\":\"" + foregroundPointSet.toJson() + "\", \"background\":\"" + backgroundPointSet.toJson() + "\"}";

  CurlUtils::doMethod("POST", uri, paramStr, inputImageFile, outputImageFile, timeoutInSec);
  return 0;
}

PolygonsList Client::maskToPolygon(int pointRatio, const std::string &inputImageFile) const {
  std::string uri = serverUri + EP_MASK_TO_POLYGON;
  std::string paramStr = "{\"more_points\":" + Utils::lexical_cast<std::string>(pointRatio) + "}";

  AIAA_LOG_DEBUG("Parameters: " << paramStr);
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);

  std::string response = CurlUtils::doMethod("POST", uri, paramStr, inputImageFile, timeoutInSec);
  AIAA_LOG_DEBUG("Response: \n" << response);

  PolygonsList polygonsList = PolygonsList::fromJson(response);
  return polygonsList;
}

Polygons Client::fixPolygon(const Polygons &poly, int neighborhoodSize, int polyIndex, int vertexIndex, const int vertexOffset[2],
                            const std::string &inputImageFile, const std::string &outputImageFile) const {
  std::string uri = serverUri + EP_FIX_POLYGON;

  std::string paramStr = "{\"propagate_neighbor\":" + Utils::lexical_cast<std::string>(neighborhoodSize) + ",";
  paramStr = paramStr + "\"dimension\":2,";
  paramStr = paramStr + "\"polygonIndex\":" + Utils::lexical_cast<std::string>(polyIndex) + ",";
  paramStr = paramStr + "\"vertexIndex\":" + Utils::lexical_cast<std::string>(vertexIndex) + ",";
  paramStr = paramStr + "\"vertexOffset\": [" + Utils::lexical_cast<std::string>(vertexOffset[0]) + ","
      + Utils::lexical_cast<std::string>(vertexOffset[1]) + "],";
  paramStr = paramStr + "\"prev_poly\":" + poly.toJson() + "}";

  AIAA_LOG_DEBUG("Parameters: " << paramStr);
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
  AIAA_LOG_DEBUG("OutputImageFile: " << outputImageFile);

  std::string response = CurlUtils::doMethod("POST", uri, paramStr, inputImageFile, outputImageFile, timeoutInSec);
  AIAA_LOG_DEBUG("Response: \n" << response);

  return PolygonsList::fromJson(response).list[0];
}

PolygonsList Client::fixPolygon(const PolygonsList &poly, int neighborhoodSize, int neighborhoodSize3D, int sliceIndex, int polyIndex,
                                int vertexIndex, const int vertexOffset[2], const std::string &inputImageFile,
                                const std::string &outputImageFile) const {
  std::string uri = serverUri + EP_FIX_POLYGON;

  std::string paramStr = "{\"propagate_neighbor\":" + Utils::lexical_cast<std::string>(neighborhoodSize) + ",";
  paramStr = paramStr + "\"propagate_neighbor_3d\":" + Utils::lexical_cast<std::string>(neighborhoodSize3D) + ",";
  paramStr = paramStr + "\"dimension\":3,";
  paramStr = paramStr + "\"sliceIndex\":" + Utils::lexical_cast<std::string>(sliceIndex) + ",";
  paramStr = paramStr + "\"polygonIndex\":" + Utils::lexical_cast<std::string>(polyIndex) + ",";
  paramStr = paramStr + "\"vertexIndex\":" + Utils::lexical_cast<std::string>(vertexIndex) + ",";
  paramStr = paramStr + "\"vertexOffset\": [" + Utils::lexical_cast<std::string>(vertexOffset[0]) + ","
      + Utils::lexical_cast<std::string>(vertexOffset[1]) + "],";
  paramStr = paramStr + "\"prev_poly\":" + poly.toJson() + "}";

  AIAA_LOG_DEBUG("Parameters: " << paramStr);
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
  AIAA_LOG_DEBUG("OutputImageFile: " << outputImageFile);

  std::string response = CurlUtils::doMethod("POST", uri, paramStr, inputImageFile, outputImageFile, timeoutInSec);
  AIAA_LOG_DEBUG("Response: \n" << response);

  return PolygonsList::fromJson(response);
}

std::string Client::createSession(const std::string &inputImageFile, const int expiry) const {
  AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
  AIAA_LOG_DEBUG("Expiry: " << expiry);

  std::string uri = serverUri + EP_SESSION;
  std::string paramStr = "{}";

  std::string response = CurlUtils::doMethod("PUT", uri, paramStr, inputImageFile, timeoutInSec);
  AIAA_LOG_DEBUG("Response: \n" << response);

  std::string sessionID;

  try {
    nlohmann::json j = nlohmann::json::parse(response);
    sessionID = j.find("session_id") != j.end() ? j["session_id"].get<std::string>() : std::string();
  } catch (nlohmann::json::parse_error &e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  } catch (nlohmann::json::type_error &e) {
    AIAA_LOG_ERROR(e.what());
    throw exception(exception::RESPONSE_PARSE_ERROR, e.what());
  }

  AIAA_LOG_DEBUG("New Session ID: " << sessionID);
  return sessionID;
}

std::string Client::getSession(const std::string &sessionId) const {
  if (sessionId.empty()) {
    AIAA_LOG_ERROR("Invalid Session ID");
    return std::string();
  }

  std::string uri = serverUri + EP_SESSION;
  uri += CurlUtils::encode(sessionId);

  AIAA_LOG_DEBUG("URI: " << uri);

  std::string response = CurlUtils::doMethod("GET", uri, timeoutInSec);
  AIAA_LOG_DEBUG("Response: \n" << response);
  return response;
}

void Client::closeSession(const std::string &sessionId) const {
  if (sessionId.empty()) {
    AIAA_LOG_ERROR("Invalid Session ID");
    return;
  }

  std::string uri = serverUri + EP_SESSION;
  uri += CurlUtils::encode(sessionId);

  AIAA_LOG_DEBUG("URI: " << uri);

  std::string response = CurlUtils::doMethod("DELETE", uri, timeoutInSec);
  AIAA_LOG_DEBUG("Response: \n" << response);
}

}
}
