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
              "  |-h        (Help) Print this information                                                |\n"
              "  |-server   Server URI {default: http://10.110.45.66:5000/v1}                            |\n"
              " *|-label    Input Label Name  [either -label or -model is required]                      |\n"
              " *|-model    Model Name        [either -label or -model is required]                      |\n"
              " *|-points   3D Points [[x,y,z]+]     Example: [[70,172,86],...,[105,161,180]]            |\n"
              "  |-pad      Padding Size to be used {default: 20.0}                                      |\n"
              "  |-roi      ROI Image Size to be used for inference {default: 128x128x128}               |\n"
              "  |-sigma    Sigma Value for inference {default: 3.0}                                     |\n"
              " *|-image    Input Image File                                                             |\n"
              "  |-pixel    Input Image Pixel Type {default: short}                                      |\n"
              " *|-output   Output Image File                                                            |\n"
              "  |-ts       Print API Latency                                                            |\n";
    return 0;
  }

  std::string serverUri = getCmdOption(argv, argv + argc, "-server", "http://10.110.45.66:5000/v1");
  std::string label = getCmdOption(argv, argv + argc, "-label");
  std::string model = getCmdOption(argv, argv + argc, "-model");
  std::string points = getCmdOption(argv, argv + argc, "-points");
  double pad = ::atof(getCmdOption(argv, argv + argc, "-pad", "20.0").c_str());
  std::string roi = getCmdOption(argv, argv + argc, "-roi", "128x128x128");
  double sigma = ::atof(getCmdOption(argv, argv + argc, "-sigma", "3.0").c_str());
  std::string inputImageFile = getCmdOption(argv, argv + argc, "-image");
  nvidia::aiaa::Pixel::Type pixelType = nvidia::aiaa::getPixelType(getCmdOption(argv, argv + argc, "-pixel", "unsigned short"));
  std::string outputImageFile = getCmdOption(argv, argv + argc, "-output");
  bool printTs = cmdOptionExists(argv, argv + argc, "-ts") ? true : false;

  if (label.empty() && model.empty()) {
    std::cerr << "Either Label or Model is required\n";
    return -1;
  }
  if (points.empty()) {
    std::cerr << "Pointset is empty\n";
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
    nvidia::aiaa::PointSet pointSet = nvidia::aiaa::PointSet::fromJson(points);
    nvidia::aiaa::Client client(serverUri);

    nvidia::aiaa::ModelList models = client.models();
    nvidia::aiaa::Model m = models.getMatchingModel(label);

    // overrides
    if (!model.empty()) {
      m.name = model;
    }
    if (cmdOptionExists(argv, argv + argc, "-pad")) {
      m.padding = pad;
    }
    if (cmdOptionExists(argv, argv + argc, "-sigma")) {
      m.sigma = sigma;
    }
    if (cmdOptionExists(argv, argv + argc, "-roi")) {
      m.roi = nvidia::aiaa::Utils::stringToPoint(roi, 'x');
    }

    auto begin = std::chrono::high_resolution_clock::now();
    int ret = client.dextr3D(m, pointSet, inputImageFile, pixelType, outputImageFile);

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "Return Code: " << ret << (ret ? " (FAILED) " : " (SUCCESS) ") << std::endl;
    if (printTs) {
      std::cout << "API Latency (in milli sec): " << ms << std::endl;
    }
    return ret;
  } catch (nvidia::aiaa::exception& e) {
    std::cerr << "nvidia::aiaa::exception => nvidia.aiaa.error." << e.id << "; description: " << e.name() << std::endl;
  }
  return -1;
}
