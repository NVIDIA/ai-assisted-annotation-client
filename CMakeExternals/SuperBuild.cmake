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

include (ExternalProject)

set(proj_DEPENDENCIES )

# ITK
if(EXTERNAL_ITK_DIR)
  set(ITK_DIR ${EXTERNAL_ITK_DIR})
else()
  if(NOT DEFINED ITK_DIR)
    include(CMakeExternals/ITK.cmake)
    list(APPEND proj_DEPENDENCIES ITK)
    set(ITK_DIR ${CMAKE_BINARY_DIR}/3rdparty/lib/cmake/ITK-4.13)
  endif()
endif()

# Poco
if(EXTERNAL_Poco_DIR)
  set(Poco_DIR ${EXTERNAL_Poco_DIR})
else()
  if(NOT DEFINED Poco_DIR)
    include(CMakeExternals/Poco.cmake)
    list(APPEND proj_DEPENDENCIES Poco)
    set(Poco_DIR ${CMAKE_BINARY_DIR}/3rdparty/lib/cmake/Poco)
  endif()
endif()

# Others
include(CMakeExternals/nlohmann_json.cmake)
list(APPEND proj_DEPENDENCIES nlohmann_json)

message(STATUS "(SuperBuild: ${USE_SUPERBUILD}) Dependenceies:  ${proj_DEPENDENCIES}")
message(STATUS "(SuperBuild: ${USE_SUPERBUILD}) Using ITK_DIR:  ${ITK_DIR}")
message(STATUS "(SuperBuild: ${USE_SUPERBUILD}) Using Poco_DIR: ${Poco_DIR}")

ExternalProject_Add(
  NvidiaAIAAClient
  PREFIX NvidiaAIAAClient
  BINARY_DIR ${CMAKE_BINARY_DIR}/NvidiaAIAAClient-Build

  DEPENDS ${proj_DEPENDENCIES}
 
  DOWNLOAD_COMMAND ""
  SOURCE_DIR ${CMAKE_SOURCE_DIR}
  BUILD_ALWAYS 1

  CMAKE_ARGS
    -DUSE_SUPERBUILD=OFF
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
    -DTHIRDPARTY_BINARY_DIR=${CMAKE_BINARY_DIR}/3rdparty
    -DITK_DIR:PATH=${ITK_DIR}
    -DPoco_DIR:PATH=${Poco_DIR}

  TEST_COMMAND ""
)

# Install
install(DIRECTORY ${CMAKE_BINARY_DIR}/install/
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)

set(AIAA_INCLUDE_DIR   ${CMAKE_INSTALL_PREFIX}/include)
set(AIAA_LIBRARY_DIR   ${CMAKE_INSTALL_PREFIX}/lib)
set(AIAA_LIBRARY_NAME  NvidiaAIAAClient)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in "${CMAKE_BINARY_DIR}/NvidiaAIAAClientConfig.cmake" @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ConfigVersion.cmake.in "${CMAKE_BINARY_DIR}/NvidiaAIAAClientConfigVersion.cmake" @ONLY)

install(FILES
  "${CMAKE_BINARY_DIR}/NvidiaAIAAClientConfig.cmake"
  "${CMAKE_BINARY_DIR}/NvidiaAIAAClientConfigVersion.cmake"
  DESTINATION "lib/cmake/NvidiaAIAAClient" COMPONENT dev)

