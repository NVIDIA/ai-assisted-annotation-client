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

if(NOT DEFINED Poco_DIR)
  ExternalProject_Add(
    Poco
    PREFIX Poco
    BINARY_DIR Poco-Build

    #DOWNLOAD_COMMAND ""
    URL https://pocoproject.org/releases/poco-1.9.0/poco-1.9.0-all.tar.bz2
    URL_MD5 790bc520984616ae27eb0f95bafd2c8f
  
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/3rdparty/Poco"
    CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/3rdparty
       -DBUILD_SHARED_LIBS:BOOL=OFF
       -DPOCO_MT:BOOL=OFF
       -DPOCO_STATIC:BOOL=ON
       -DENABLE_XML:BOOL=ON
       -DENABLE_JSON:BOOL=ON
       -DENABLE_MONGODB:BOOL=OFF
       -DENABLE_PDF:BOOL=OFF
       -DENABLE_UTIL:BOOL=ON
       -DENABLE_NET:BOOL=ON
       -DENABLE_NETSSL:BOOL=OFF
       -DENABLE_NETSSL_WIN:BOOL=OFF
       -DENABLE_CRYPTO:BOOL=OFF
       -DENABLE_DATA:BOOL=OFF
       -DENABLE_DATA_SQLITE:BOOL=OFF
       -DENABLE_DATA_MYSQL:BOOL=OFF
       -DENABLE_DATA_ODBC:BOOL=OFF
       -DENABLE_SEVENZIP:BOOL=OFF
       -DENABLE_ZIP:BOOL=OFF
       -DENABLE_APACHECONNECTOR:BOOL=OFF
       -DENABLE_CPPPARSER:BOOL=OFF
       -DENABLE_POCODOC:BOOL=OFF
       -DENABLE_PAGECOMPILER:BOOL=OFF
       -DENABLE_PAGECOMPILER_FILE2PAGE:BOOL=OFF
  
    TEST_COMMAND ""
  )
  set(Poco_DIR ${CMAKE_BINARY_DIR}/Poco-Build)
endif()
