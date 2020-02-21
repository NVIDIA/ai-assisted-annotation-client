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
              "  |-server   Server URI {default: http://0.0.0.0:5000}                                    |\n"
              " *|-op       Operation (create|get|delete)                                                |\n"
              "  |-image    Input Image File in case of (create) operation                               |\n"
              "  |-expiry   Session expiry time in minutes (default: 0)                                  |\n"
              "  |-session  Session ID in case of (get|delete) operation                                 |\n"
              "  |-timeout  Timeout In Seconds {default: 60}                                             |\n"
              "  |-ts       Print API Latency                                                            |\n";
    return 0;
  }

  std::string serverUri = getCmdOption(argv, argv + argc, "-server", "http://0.0.0.0:5000");

  std::string operation = getCmdOption(argv, argv + argc, "-op");
  std::string inputImageFile = getCmdOption(argv, argv + argc, "-image");
  int expiry = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-expiry", "0"));
  std::string sessionId = getCmdOption(argv, argv + argc, "-session");

  int timeout = nvidia::aiaa::Utils::lexical_cast<int>(getCmdOption(argv, argv + argc, "-timeout", "60"));
  bool printTs = cmdOptionExists(argv, argv + argc, "-ts") ? true : false;

  if (operation.empty()) {
    std::cerr << "Operation is Missing\n";
    return -1;
  }
  if (!(operation == "create" || operation == "get" || operation == "delete")) {
    std::cerr << "Operation is Invalid\n";
    return -1;
  }
  if (operation == "create" && inputImageFile.empty()) {
    std::cerr << "Input Image file is missing (Either session-id or input image should be provided)\n";
    return -1;
  }
  if ((operation == "get" || operation == "delete") && sessionId.empty()) {
    std::cerr << "Session ID is missing\n";
    return -1;
  }

  try {
    nvidia::aiaa::Client client(serverUri, timeout);

    auto begin = std::chrono::high_resolution_clock::now();
    if (operation == "create") {
      std::string result = client.createSession(inputImageFile, expiry);
      std::cout << "New Session ID: " << result << std::endl;
    } else if (operation == "get") {
      std::string result = client.getSession(sessionId);
      std::cout << "Session Info: " << result << std::endl;
    } else if (operation == "delete") {
      client.closeSession(sessionId);
      std::cout << "Session Closed: " << sessionId << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    if (printTs) {
      std::cout << "API Latency (in milli sec): " << ms << std::endl;
    }
    return 0;
  } catch (nvidia::aiaa::exception &e) {
    std::cerr << "nvidia::aiaa::exception => nvidia.aiaa.error." << e.id << "; description: " << e.name() << std::endl;
  }
  return -1;
}
