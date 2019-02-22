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
#include "../commonutils.h"
#include <chrono>

int main(int argc, char **argv) {
  if (argc < 2 || cmdOptionExists(argv, argv + argc, "-h")) {
    std::cout << "Usage:: <COMMAND> <OPTIONS>\n"
              "  |-h            (Help) Print this information                                            |\n"
              "  |-server       Server URI  {default: http://10.110.45.66:5000/v1}                       |\n"
              "  |-neighbor     NeighborHood Size for propagation {default: 1}                           |\n"
              " *|-poly         New Polygons Array [[[x,y]+]] Example: [[[70,172,86],...,[125,147,164]]] |\n"
              " *|-ppoly        Old Polygons Array [[[x,y]+]]                                            |\n"
              " *|-pindex       Polygon Index which needs to be updated                                  |\n"
              " *|-vindex       Vertext Index which needs to be updated                                  |\n"
              " *|-image        Input 2D Slice Image File                                                |\n"
              " *|-output       Output Image File                                                        |\n"
              "  |-format       Format Output Json                                                       |\n"
              "  |-ts           Print API Latency                                                        |\n";
    return 0;
  }

  std::string serverUri = getCmdOption(argv, argv + argc, "-server", "http://10.110.45.66:5000/v1");
  int neighborhoodSize = ::atoi(getCmdOption(argv, argv + argc, "-neighbor", "1").c_str());
  std::string polygon = getCmdOption(argv, argv + argc, "-poly");
  std::string prevPoly = getCmdOption(argv, argv + argc, "-ppoly");
  int polygonIndex = ::atoi(getCmdOption(argv, argv + argc, "-pindex", "0").c_str());
  int vertexIndex = ::atoi(getCmdOption(argv, argv + argc, "-vindex", "0").c_str());
  std::string inputImageFile = getCmdOption(argv, argv + argc, "-image");
  std::string outputImageFile = getCmdOption(argv, argv + argc, "-output");
  int jsonSpace = cmdOptionExists(argv, argv + argc, "-format") ? 2 : 0;
  bool printTs = cmdOptionExists(argv, argv + argc, "-ts") ? true : false;

  if (polygon.empty()) {
    std::cerr << "Input Polygon List missing\n";
    return -1;
  }
  if (prevPoly.empty()) {
    std::cerr << "Input Previous Polygon List is missing\n";
    return -1;
  }
  if (inputImageFile.empty()) {
    std::cerr << "Input Image file is missing\n";
    return -1;
  }
  if (outputImageFile.empty()) {
    std::cerr << "Output Image file is missing\n";
    return -1;
  }

  try {
    nvidia::aiaa::Polygons p1 = nvidia::aiaa::Polygons::fromJson(polygon);
    nvidia::aiaa::Polygons p2 = nvidia::aiaa::Polygons::fromJson(prevPoly);

    auto begin = std::chrono::high_resolution_clock::now();
    nvidia::aiaa::Client client(serverUri);
    nvidia::aiaa::Polygons result = client.fixPolygon(p1, p2, neighborhoodSize, polygonIndex, vertexIndex, inputImageFile, outputImageFile);

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << result.toJson(jsonSpace) << std::endl;
    if (printTs) {
      std::cout << "Time taken (in milli sec): " << ms << std::endl;
    }
    return 0;
  } catch (nvidia::aiaa::exception& e) {
    std::cerr << "nvidia::aiaa::exception => nvidia.aiaa.error." << e.id << "; description: " << e.name() << std::endl;
  }

  return -1;
}
