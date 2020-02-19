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
  if (1 == 0) {
    try {
      std::string response =
          "[[], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [[[181, 79]], [[185, 78], [185, 79]], [[183, 64]], [[185, 63], [193, 70], [190, 75], [189, 78], [197, 74], [201, 70], [195, 64]], [[162, 63], [152, 70], [156, 77], [166, 81], [177, 80], [173, 75], [166, 70], [168, 63]], [[175, 77]]], [[[176, 60], [165, 61], [154, 67], [150, 75], [157, 80], [167, 84], [178, 84], [189, 81], [200, 77], [206, 68], [196, 63], [186, 60]]], [[[171, 59], [160, 62], [149, 69], [148, 77], [158, 82], [168, 85], [179, 85], [190, 82], [201, 78], [208, 70], [200, 63], [190, 60], [179, 59]]], [[[176, 58], [165, 60], [154, 64], [145, 72], [151, 79], [160, 84], [171, 86], [182, 85], [193, 83], [204, 78], [212, 71], [205, 65], [195, 61], [184, 59]]], [[[173, 58], [162, 61], [151, 66], [144, 75], [152, 81], [162, 85], [173, 86], [184, 86], [195, 83], [206, 78], [213, 71], [205, 64], [195, 60], [184, 58]]], [[[174, 57], [163, 59], [152, 64], [144, 72], [148, 79], [157, 83], [168, 86], [179, 87], [190, 85], [201, 81], [212, 77], [211, 67], [202, 61], [191, 59], [181, 58]]], [[[173, 57], [162, 60], [151, 64], [142, 72], [148, 80], [157, 84], [168, 87], [179, 88], [190, 85], [201, 82], [212, 78], [213, 69], [205, 63], [195, 60], [184, 57]]], [[[169, 57], [158, 60], [147, 66], [141, 74], [148, 81], [158, 85], [169, 87], [180, 88], [191, 85], [202, 82], [213, 79], [215, 70], [207, 64], [197, 60], [186, 57], [175, 57]]], [[[167, 57], [156, 61], [145, 66], [138, 74], [146, 80], [155, 84], [165, 87], [176, 88], [187, 87], [198, 84], [209, 83], [218, 76], [214, 67], [204, 62], [194, 58], [184, 57], [173, 57]]], [[[178, 56], [167, 57], [156, 60], [145, 66], [138, 76], [147, 81], [157, 84], [167, 88], [178, 89], [189, 87], [200, 84], [211, 83], [218, 74], [213, 66], [204, 61], [193, 58], [182, 57]]], [[[171, 56], [157, 58], [153.0, 66.0], [145, 73], [143, 81], [151, 83], [159, 87], [169, 86], [180, 89], [191, 86], [202, 84], [213, 82], [220, 73], [213, 66], [203, 61], [193, 57], [182, 56]]], [[[171, 56], [157, 58], [153.0, 65.75], [142, 72], [143, 81], [151, 84], [159, 87], [170, 86], [180, 89], [191, 87], [202, 84], [213, 83], [220, 73], [213, 66], [204, 60], [194, 57], [183, 56], [172, 56]]], [[[174, 55], [160, 57], [147, 61], [138, 73], [143, 82], [147, 83], [154, 84], [163, 86], [172, 90], [183, 89], [194, 86], [205, 84], [216, 81], [220, 71], [213, 64], [203, 60], [193, 57], [182, 56]]], [[[171, 55], [158, 57], [149, 68], [148, 73], [143, 82], [148, 83], [156, 86], [164, 88], [175, 90], [186, 89], [197, 85], [208, 84], [218, 80], [221, 71], [213, 64], [203, 60], [193, 57], [182, 55]]], [[[169, 55], [154, 58], [151, 68], [146, 73], [143, 82], [150, 84], [158, 87], [167, 91], [178, 90], [189, 88], [200, 84], [211, 84], [222, 77], [219, 68], [210, 61], [199, 59], [189, 56], [178, 55]]], [[[167, 55], [153, 58], [150, 68], [144, 75], [143, 83], [149, 83], [156, 87], [165, 91], [176, 90], [187, 89], [198, 84], [209, 84], [220, 80], [222, 71], [214, 64], [204, 59], [194, 57], [183, 55], [172, 55]]], [[[167, 55], [152, 58], [150, 67], [143, 72], [143, 82], [149, 84], [156, 87], [166, 91], [177, 91], [188, 88], [199, 84], [210, 84], [221, 80], [222, 70], [213, 64], [203, 59], [192, 56], [181, 55], [170, 55]]], [[[178, 54], [167, 55], [154, 57], [150, 68], [145, 74], [143, 82], [146, 83], [153, 86], [160, 89], [170, 92], [181, 90], [192, 86], [203, 84], [214, 83], [223, 77], [221, 68], [211, 63], [201, 58], [190, 55], [179, 55]]], [[[176, 53], [165, 55], [151, 58], [149, 66], [144, 73], [143, 83], [147, 84], [154, 87], [162, 89], [172, 92], [183, 90], [194, 85], [205, 84], [216, 83], [224, 75], [218, 66], [209, 61], [199, 57], [188, 55], [177, 54]]], [[[174, 53], [163, 55], [156, 66], [148, 73], [139, 75], [143, 83], [148, 85], [155, 88], [165, 90], [175, 92], [186, 88], [197, 84], [208, 84], [219, 82], [222, 71], [215, 64], [205, 59], [195, 57], [184, 54]]], [[[171, 53], [160, 56], [146, 65], [146, 67], [142, 78], [143, 83], [148, 85], [154, 89], [163, 92], [174, 92], [185, 88], [196, 84], [207, 84], [218, 82], [221, 72], [216, 65], [207, 60], [196, 56], [186, 53], [175, 53]]], [[[173, 53], [162, 55], [151, 59], [140, 63], [130, 71], [132, 81], [142, 84], [152, 88], [163, 92], [174, 92], [185, 88], [196, 84], [207, 85], [218, 82], [221, 71], [216, 65], [207, 60], [196, 56], [186, 53], [175, 53]]], [[[173, 53], [162, 55], [151, 58], [140, 63], [130, 70], [131, 79], [141, 83], [151, 88], [161, 92], [172, 92], [183, 88], [194, 83], [205, 85], [216, 83], [216, 74], [218, 65], [210, 60], [199, 57], [188, 54], [178, 53]]], [[[171, 53], [160, 55], [149, 59], [138, 63], [129, 72], [130, 81], [141, 84], [151, 88], [161, 92], [172, 92], [183, 86], [194, 82], [205, 85], [216, 83], [215, 73], [217, 66], [209, 60], [199, 57], [188, 54], [178, 53]]], [[[171, 53], [160, 55], [149, 59], [138, 63], [130, 71], [130, 80], [139, 84], [149, 88], [159, 92], [170, 93], [181, 87], [192, 82], [202, 84], [213, 85], [212, 77], [220, 67], [211, 61], [201, 57], [190, 55], [180, 53]]], [[[169, 53], [158, 56], [147, 59], [136, 65], [127, 74], [132, 82], [142, 85], [151, 89], [161, 93], [172, 92], [181, 85], [192, 81], [202, 84], [212, 82], [212, 73], [216, 65], [207, 59], [197, 56], [187, 54], [176, 53]]], [[[169, 53], [158, 56], [147, 59], [136, 65], [128, 75], [132, 82], [143, 86], [152, 92], [163, 94], [174, 91], [183, 83], [194, 81], [204, 83], [210, 75], [218, 66], [209, 60], [199, 56], [188, 53], [177, 53]]], [[[167, 53], [156, 56], [145, 59], [134, 66], [128, 76], [134, 83], [144, 87], [153, 92], [164, 95], [175, 90], [184, 81], [195, 77], [205, 71], [215, 67], [208, 59], [198, 56], [188, 53], [177, 53]]], [[[167, 53], [156, 56], [145, 60], [134, 67], [130, 78], [137, 84], [147, 90], [157, 94], [168, 95], [178, 88], [185, 79], [195, 73], [206, 68], [207, 60], [196, 57], [186, 53], [175, 53]], [[182, 71], [184, 76]], [[152, 67], [156, 73], [149, 72]]], [[[169, 53], [158, 56], [147, 59], [136, 66], [133, 76], [137, 84], [147, 90], [157, 94], [167, 95], [176, 89], [182, 80], [179, 71], [189, 68], [196, 71], [206, 67], [206, 59], [196, 58], [187, 55], [177, 53]], [[149, 64], [158, 67], [156, 76], [146, 79], [145, 69]], [[188, 60], [189, 59], [190, 60], [189, 61]]], [[[166, 55], [168, 62], [164, 73], [157, 82], [150, 85], [150, 89], [160, 93], [171, 92], [176, 82], [171, 74], [176, 65], [181, 56], [173, 55]], [[157, 86]], [[159, 85], [162, 88]], [[163, 83], [164, 82], [165, 83], [164, 84]], [[159, 83], [160, 82], [161, 83], [160, 84]], [[167, 72], [168, 71], [169, 72], [168, 73]]], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], []]";

      //response = "[[], [], [ [[170, 66],[162, 73],[169, 77],[180, 76],[185, 68],[175, 66]], [[1,2]], [] ]]";
      nvidia::aiaa::PolygonsList polys = nvidia::aiaa::PolygonsList::fromJson(response);
      std::cout << "Result:: " << polys.toJson() << std::endl;
    } catch (nvidia::aiaa::exception &e) {
      std::cerr << "nvidia::aiaa::exception => nvidia.aiaa.error." << e.id << "; description: " << e.name() << std::endl;
    }
    return 0;
  }

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
