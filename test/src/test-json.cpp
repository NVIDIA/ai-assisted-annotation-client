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

#include <nvidia/aiaa/client.h>
#include <iostream>
#include <vector>
#include <cassert>

const std::string SERVER_URI = "http://10.110.45.66:5000/v1";

void testJsonModelList() {
	std::cout << "\n\n******************************** [" << __func__ << "] ********************************\n";
	std::string json =
			"[{\"decription\":\"\",\"internal name\":\"Dextr3dCroppedEngine\",\"labels\":[\"brain_tumor_core\"],\"name\":\"Dextr3DBrainTC\"},{\"decription\":\"\",\"internal name\":\"Dextr3dCroppedEngine\",\"labels\":[\"liver\"],\"name\":\"Dextr3DLiver\"},{\"decription\":\"\",\"internal name\":\"Dextr3dCroppedEngine\",\"labels\":[\"brain_whole_tumor\"],\"name\":\"Dextr3DBrainWT\"}]";

	nvidia::aiaa::ModelList modelList = nvidia::aiaa::ModelList::fromJson(json);

	std::cout << "MODEL-LIST (raw ): " << json << std::endl;
	std::cout << "MODEL-LIST (json): " << modelList.toJson() << std::endl;
	assert(json == modelList.toJson());
}

void testJsonPointSet() {
	std::cout << "\n\n******************************** [" << __func__ << "] ********************************\n";
	std::string json = "[[70,172,86],[105,161,180],[125,147,164],[56,174,124],[91,119,143],[77,219,120]]";
	nvidia::aiaa::Point3DSet pointSet = nvidia::aiaa::Point3DSet::fromJson(json);

	std::cout << "3D-POINT SET (raw ): " << json << std::endl;
	std::cout << "3D-POINT SET (json): " << pointSet.toJson() << std::endl;
	assert(json == pointSet.toJson());
}

void testJsonPolygons() {
	std::cout << "\n\n******************************** [" << __func__ << "] ********************************\n";
	std::string json = "[[[69,167],[73,156],[78,146],[87,137],[98,131],[108,130],[118,132],[123,141],[139,155],[119,161],[109,166],[98,170],[89,176],[80,183],[71,182],[69,172]]]";
	nvidia::aiaa::Polygons polygons = nvidia::aiaa::Polygons::fromJson(json);

	std::cout << "POLYGONS (raw ): " << json << std::endl;
	std::cout << "POLYGONS (json): " << polygons.toJson() << std::endl;
	assert(json == polygons.toJson());
}

void testJsonPolygonsList() {
	std::cout << "\n\n******************************** [" << __func__ << "] ********************************\n";
	std::string json =
			"[[],[[[69,167],[73,156],[78,146],[87,137],[98,131],[108,130],[118,132],[123,141],[139,155],[119,161],[109,166],[98,170],[89,176],[80,183],[71,182],[69,172]]]]";
	nvidia::aiaa::PolygonsList polygonsList = nvidia::aiaa::PolygonsList::fromJson(json);

	std::cout << "POLYGONS-LIST (raw ): " << json << std::endl;
	std::cout << "POLYGONS-LIST (json): " << polygonsList.toJson() << std::endl;
	assert(json == polygonsList.toJson());
}

int main(int argc, char **argv) {
	testJsonModelList();
	testJsonPointSet();
	testJsonPolygons();
	testJsonPolygonsList();
	return 0;
}
