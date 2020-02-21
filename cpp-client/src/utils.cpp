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

#include "../include/nvidia/aiaa/utils.h"
#include "../include/nvidia/aiaa/log.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace nvidia {
namespace aiaa {

bool Utils::iequals(const std::string &a, const std::string &b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const char &a, const char &b) {
    return (std::tolower(a) == std::tolower(b));
  });
}

std::string Utils::to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return s;
}

std::string Utils::tempfilename() {
#pragma warning(suppress:4996)
  return std::tmpnam(nullptr);
}

std::vector<std::string> Utils::split(const std::string &str, char delim) {
  std::vector<std::string> strings;
  std::istringstream f(str);

  std::string s;
  while (std::getline(f, s, delim)) {
    strings.push_back(s);
  }
  return strings;
}

Point Utils::stringToPoint(const std::string &str, char delim) {
  std::vector<std::string> pstr = split(str, delim);
  Point point;

  for (size_t i = 0; i < pstr.size(); i++) {
    point.push_back(lexical_cast<int>(pstr[i]));
  }
  return point;
}

}
}
