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


Client::Client(const std::string& uri) :
		serverUri(uri) {
	if (serverUri.back() == '/') {
        serverUri.pop_back();
    }
}

ModelList Client::models() const {
	std::string uri = serverUri + EP_MODELS;
	AIAA_LOG_DEBUG("URI: " << uri);

	return ModelList::fromJson(CurlUtils::doGet(uri));
}

int Client::dextr3d(const std::string &label, const Point3DSet &pointSet, const std::string &inputImageFile, const std::string &outputImageFile) const {
	ModelList modelList = this->models();
	return dextr3d(modelList.getMatchingModel(label), pointSet, inputImageFile, outputImageFile);
}

//TODO:: Remove this API once AIAA Server supports PAD, ROI, SIGMA values in Model.json
int Client::dextr3d(const std::string &label, const Point3DSet &pointSet, const std::string &inputImageFile, const std::string &outputImageFile, double PAD,
		const std::string& ROI_SIZE, double SIGMA) const {
	ModelList modelList = this->models();
	Model model = modelList.getMatchingModel(label);

	model.padding = PAD;
	model.sigma = SIGMA;

	int xyz[3];
	Utils::stringToPoint3D(ROI_SIZE, 'x', xyz);
	model.roi[0] = xyz[0];
	model.roi[1] = xyz[1];
	model.roi[2] = xyz[2];

	return dextr3d(model, pointSet, inputImageFile, outputImageFile);
}

int Client::dextr3d(const Model &model, const Point3DSet &pointSet, const std::string &inputImageFile, const std::string &outputImageFile) const {
	if (pointSet.points.size() < MIN_POINTS_FOR_DEXTR3D) {
		AIAA_LOG_ERROR("Insufficient Points; Minimum Points required for input PointSet: " << MIN_POINTS_FOR_DEXTR3D);
		return -1;
	}

	if (model.name.empty()) {
		AIAA_LOG_WARN("Selected model is EMPTY");
		return -2;
	}

	AIAA_LOG_DEBUG("PointSet: " << pointSet.toJson());
	AIAA_LOG_DEBUG("Model: " << model.toJson());
	AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
	AIAA_LOG_DEBUG("OutputImageFile: " << outputImageFile);

	std::string tmpImageFile = Utils::tempfilename() + IMAGE_FILE_EXTENSION;
	std::string tmpResultFile = Utils::tempfilename() + IMAGE_FILE_EXTENSION;

	// Perform pre-processing of crop/re-sample and re-compute point index
	ImageInfo imageInfo;
	Point3DSet pointSetROI = ITKUtils::imagePreProcess(pointSet, inputImageFile, tmpImageFile, imageInfo, model.padding, model.roi);

	// TODO:: Ask AIAA Server to make value of points to JSON Array
	std::string uri = serverUri + EP_DEXTRA_3D + "?model=" + model.name;
	std::string paramStr = "{\"sigma\":" + std::to_string(model.sigma) + ",\"points\":\"" + pointSetROI.toJson() + "\"}";

	AIAA_LOG_DEBUG("URI: " << uri);
	AIAA_LOG_DEBUG("Parameters: " << paramStr);
	AIAA_LOG_DEBUG("TmpImageFile: " << tmpImageFile);
	AIAA_LOG_DEBUG("TmpResultFile: " << tmpResultFile);

	// Inference
	std::string response = CurlUtils::doPost(uri, paramStr, tmpImageFile, tmpResultFile);
	AIAA_LOG_DEBUG("Response: \n" << response);

	// Perform post-processing to recover crop and re-sample and save to user-specified location
	ITKUtils::imagePostProcess(tmpResultFile, outputImageFile, imageInfo);

	// Cleanup temporary files
	std::remove(tmpImageFile.c_str());
	std::remove(tmpResultFile.c_str());
	return 0;
}

PolygonsList Client::mask2Polygon(int pointRatio, const std::string &inputImageFile) const {
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

Polygons Client::fixPolygon(const Polygons &newPoly, const Polygons &oldPrev, int neighborhoodSize, int polyIndex, int vertexIndex, const std::string &inputImageFile,
		const std::string &outputImageFile) const {

	std::string uri = serverUri + EP_FIX_POLYGON;
	std::string paramStr = "{\"propagate_neighbor\":" + std::to_string(neighborhoodSize) + ",";
	paramStr = paramStr + "\"polygonIndex\":" + std::to_string(polyIndex) + ",";
	paramStr = paramStr + "\"vertexIndex\":" + std::to_string(vertexIndex) + ",";
	paramStr = paramStr + "\"poly\":" + newPoly.toJson() + ",\"prev_poly\":" + oldPrev.toJson() + "}";

	AIAA_LOG_DEBUG("URI: " << uri);
	AIAA_LOG_DEBUG("Parameters: " << paramStr);
	AIAA_LOG_DEBUG("InputImageFile: " << inputImageFile);
	AIAA_LOG_DEBUG("OutputImageFile: " << outputImageFile);

	std::string response = CurlUtils::doPost(uri, paramStr, inputImageFile, outputImageFile);
	AIAA_LOG_DEBUG("Response: \n" << response);

	// TODO:: Ask AIAA Server to remove redundant [] to return the updated Polygons (same as input)
	return PolygonsList::fromJson(response).list[0];
}

}
}
