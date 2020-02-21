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
  if (cmdOptionExists(argv, argv + argc, "-h")) {
    std::cout << "Usage:: <COMMAND> <OPTIONS>\n"
        "  |-h        (Help) Print this information                                                |\n"
        "  |-server   Server URI {default: http://0.0.0.0:5000}                                    |\n"
        "  |-label    Find Matching Model for this label; If absent, output full Model List        |\n"
        "  |-type     Find Matching Model of type (segmentation/annotation)                        |\n"
        "  |-output   Output File Name to store result                                             |\n"
        "  |-format   Format Output Json                                                           |\n"
        "  |-timeout  Timeout In Seconds {default: 60}                                             |\n"
        "  |-ts       Print API Latency                                                            |\n";

    return 0;
  }

  std::string serverUri = getCmdOption(argv, argv + argc, "-server", "http://0.0.0.0:5000");
  std::string label = getCmdOption(argv, argv + argc, "-label");
  std::string type = getCmdOption(argv, argv + argc, "-type");
  std::string outputJsonFile = getCmdOption(argv, argv + argc, "-output");
  int jsonSpace = cmdOptionExists(argv, argv + argc, "-format") ? 2 : 0;
  int timeout = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-timeout", "60"));
  bool printTs = cmdOptionExists(argv, argv + argc, "-ts") ? true : false;

  try {
    auto begin = std::chrono::high_resolution_clock::now();
    nvidia::aiaa::Client client(serverUri, timeout);

    nvidia::aiaa::Model::ModelType mt = nvidia::aiaa::Model::toModelType(type);
    nvidia::aiaa::ModelList modelList = type.empty() && label.empty() ? client.models() : client.models(label, mt);

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast < std::chrono::milliseconds > (end - begin).count();

    if (outputJsonFile.empty()) {
      std::cout << modelList.toJson(jsonSpace) << std::endl;
    } else {
      stringToFile(modelList.toJson(jsonSpace), outputJsonFile);
    }

    if (printTs) {
      std::cout << "API Latency (in milli sec): " << ms << std::endl;
    }
    return 0;
  } catch (nvidia::aiaa::exception& e) {
    std::cerr << "nvidia::aiaa::exception => nvidia.aiaa.error." << e.id << "; description: " << e.name() << std::endl;
  }

  return -1;
}
