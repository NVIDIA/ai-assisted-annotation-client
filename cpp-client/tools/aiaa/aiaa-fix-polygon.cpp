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
#include <nvidia/aiaa/utils.h>
#include "../commonutils.h"
#include <chrono>

int main(int argc, char **argv) {
  if (argc < 2 || cmdOptionExists(argv, argv + argc, "-h")) {
    std::cout << "Usage:: <COMMAND> <OPTIONS>\n"
              "  |-h            (Help) Print this information                                            |\n"
              "  |-server       Server URI  {default: http://0.0.0.0:5000}                               |\n"
              "  |-neighbor     NeighborHood Size for propagation {default: 1}                           |\n"
              "  |-neighbor3d   3D NeighborHood Size for propagation {default: 1}                        |\n"
              "  |-dim          Dimension (2|3) {default: 2}                                             |\n"
              " *|-poly         Current 2D/3D Polygons Array 2D: [[[x,y]+]] 3D: [[[[x,y]+]],]            |\n"
              "  |-sindex       Slice Index (in case of 3D) which needs to be updated                    |\n"
              " *|-pindex       Polygon Index which needs to be updated                                  |\n"
              " *|-vindex       Vertex Index which needs to be updated                                   |\n"
              " *|-xoffset      X offset which needs to be added to vertex                               |\n"
              " *|-yoffset      Y offset which needs to be added to vertex                               |\n"
              " *|-image        Input 2D Slice Image File                                                |\n"
              " *|-output       Output Image File                                                        |\n"
              "  |-format       Format Output Json                                                       |\n"
              "  |-timeout      Timeout In Seconds {default: 60}                                         |\n"
              "  |-ts           Print API Latency                                                        |\n";
    return 0;
  }

  std::string serverUri = getCmdOption(argv, argv + argc, "-server", "http://0.0.0.0:5000");

  int neighborhoodSize = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-neighbor", "1"));
  int neighborhoodSize3D = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-neighbor3d", "1"));
  int dim = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-dim", "2"));

  std::string polygon = getCmdOption(argv, argv + argc, "-poly");
  int sliceIndex = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-sindex", "0"));
  int polygonIndex = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-pindex", "0"));
  int vertexIndex = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-vindex", "0"));
  int xOffset = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-xoffset", "0"));
  int yOffset = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-yoffset", "0"));

  std::string inputImageFile = getCmdOption(argv, argv + argc, "-image");
  std::string outputImageFile = getCmdOption(argv, argv + argc, "-output");

  int jsonSpace = cmdOptionExists(argv, argv + argc, "-format") ? 2 : 0;
  int timeout = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-timeout", "60"));
  bool printTs = cmdOptionExists(argv, argv + argc, "-ts") ? true : false;

  if (polygon.empty()) {
    std::cerr << "Input Polygon List missing\n";
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
    int vertexOffset[2] = { xOffset, yOffset };
    nvidia::aiaa::Polygons poly2D, result2D;
    nvidia::aiaa::PolygonsList poly3D, result3D;
    if (dim == 2) {
      poly2D = nvidia::aiaa::Polygons::fromJson(polygon);
    } else {
      poly3D = nvidia::aiaa::PolygonsList::fromJson(polygon);
    }

    auto begin = std::chrono::high_resolution_clock::now();
    nvidia::aiaa::Client client(serverUri, timeout);
    if (dim == 2) {
      result2D = client.fixPolygon(poly2D, neighborhoodSize, polygonIndex, vertexIndex, vertexOffset, inputImageFile, outputImageFile);
    } else {
      result3D = client.fixPolygon(poly3D, neighborhoodSize, neighborhoodSize3D, sliceIndex, polygonIndex, vertexIndex, vertexOffset, inputImageFile,
                                   outputImageFile);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    if (dim == 2) {
      std::cout << result2D.toJson(jsonSpace) << std::endl;
    } else {
      std::cout << result3D.toJson(jsonSpace) << std::endl;
    }
    if (printTs) {
      std::cout << "Time taken (in milli sec): " << ms << std::endl;
    }
    return 0;
  } catch (nvidia::aiaa::exception &e) {
    std::cerr << "nvidia::aiaa::exception => nvidia.aiaa.error." << e.id << "; description: " << e.name() << std::endl;
  }

  return -1;
}
