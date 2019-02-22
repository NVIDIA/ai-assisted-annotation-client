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

#include "../include/nvidia/aiaa/imageinfo.h"

#include <sstream>
#include <map>

namespace nvidia {
namespace aiaa {

bool ImageInfo::empty() const {
  return imageSize[0] == 0 && imageSize[1] == 0 && imageSize[2] == 0 && imageSize[3] == 0;
}

std::string ImageInfo::dump() {
  std::stringstream ss;
  ss << "{";
  ss << "\"imageSize\": [" << imageSize[0] << "," << imageSize[1] << "," << imageSize[2] << "," << imageSize[3] << "], ";
  ss << "\"cropSize\": [" << cropSize[0] << "," << cropSize[1] << "," << cropSize[2] << "," << cropSize[3] << "], ";
  ss << "\"cropIndex\": [" << cropIndex[0] << "," << cropIndex[1] << "," << cropIndex[2] << "," << cropIndex[3] << "]";
  ss << "}";

  return ss.str();
}

std::string getPixelTypeStr(Pixel::Type t) {
  static std::map<Pixel::Type, std::string> PIXEL_TYPESTR = { { Pixel::CHAR, "char" }, { Pixel::UCHAR, "unsigned char" }, { Pixel::SHORT, "short" }, {
      Pixel::USHORT, "unsigned short" }, { Pixel::INT, "int" }, { Pixel::UINT, "unsigned int" }, { Pixel::LONG, "long" }, { Pixel::ULONG,
      "unsigned long" }, { Pixel::FLOAT, "float" }, { Pixel::DOUBLE, "double" } };
  auto it = PIXEL_TYPESTR.find(t);
  if (it == PIXEL_TYPESTR.end()) {
    return "unknown";
  }
  return it->second;
}

Pixel::Type getPixelType(const std::string &type) {
  static std::map<std::string, Pixel::Type> PIXEL_TYPES = { { "char", Pixel::CHAR }, { "unsigned char", Pixel::UCHAR }, { "short", Pixel::SHORT }, {
      "unsigned short", Pixel::USHORT }, { "int", Pixel::INT }, { "unsigned int", Pixel::UINT }, { "long", Pixel::ULONG }, { "unsigned long",
      Pixel::ULONG }, { "float", Pixel::FLOAT }, { "double", Pixel::DOUBLE } };
  auto it = PIXEL_TYPES.find(type);
  if (it == PIXEL_TYPES.end()) {
    return Pixel::UNKNOWN;
  }
  return it->second;
}

}
}
