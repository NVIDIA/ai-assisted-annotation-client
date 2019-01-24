# Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include(ExternalProject)

ExternalProject_Add(
  nlohmann_json
  PREFIX JSON
  BINARY_DIR JSON-Build

  #DOWNLOAD_COMMAND ""
  URL https://github.com/nlohmann/json/archive/v3.4.0.tar.gz
  URL_MD5 ebe637e7f9b0abe4f57f4f56dd2e5e74
  
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/3rdparty/nlohmann_json"
  CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DJSON_BuildTests=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/3rdparty

  TEST_COMMAND ""
)

set(nlohmann_json_INCLUDE_DIRS  "${CMAKE_BINARY_DIR}/3rdparty/include")
