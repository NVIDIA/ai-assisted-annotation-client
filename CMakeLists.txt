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

cmake_minimum_required(VERSION 3.12.4)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Logging Enable/Disable
set(AIAA_LOG_DEBUG_ENABLED "0" CACHE STRING "Enable Debug Level logs for AIAA Client")
set(AIAA_LOG_INFO_ENABLED  "1" CACHE STRING "Enable Info Level logs for AIAA Client")

# 3D Slicer's extension build tool defines NvidiaAIAssistedAnnotation_BUILD_SLICER_EXTENSION:BOOL=ON
# to indicate that this project is being built as a 3D Slicer extension.
if (NvidiaAIAssistedAnnotation_BUILD_SLICER_EXTENSION)
  message(STATUS "Building project as a Slicer extension")
  project(NvidiaAIAssistedAnnotation)
  add_subdirectory(slicer-plugin)
  return()
endif()

if (NOT DEFINED USE_SUPERBUILD)
  project(NvidiaAIAAClient-superbuild)
  include (cpp-client/CMakeExternals/SuperBuild.cmake)
  return()
endif()

project(NvidiaAIAAClient)

# Version
file(STRINGS "${PROJECT_SOURCE_DIR}/VERSION" PACKAGE_VERSION)
message(STATUS "NvidiaAIAAClient package version: ${PACKAGE_VERSION}")

string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" CPACK_PACKAGE_VERSION_MAJOR ${PACKAGE_VERSION})
string(REGEX REPLACE "[0-9]+\\.([0-9])+\\.[0-9]+.*" "\\1" CPACK_PACKAGE_VERSION_MINOR ${PACKAGE_VERSION})
string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" CPACK_PACKAGE_VERSION_PATCH ${PACKAGE_VERSION})

set(COMPLETE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
set(PROJECT_VERSION ${COMPLETE_VERSION})

# Output Directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


# Global compiler flags
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


#############################################################
include_directories(cpp-client/include)
add_subdirectory(cpp-client)
#add_subdirectory(cpp-client/test)
add_subdirectory(cpp-client/tools)


#############################################################
# Package
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NVIDIA AIAA Client Libraries")
set(CPACK_PACKAGE_VENDOR "NVIDIA")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "NvidiaAIAAClient")
include(CPack)



#############################################################
# Doxygen
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN  NO)
    set(DOXYGEN_PROJECT_NAME  "NVIDIA AIAA Client C++")
    set(DOXYGEN_PROJECT_BRIEF "NVIDIA AIAA Client C++ API to connect to AIAA Server to execute operations like dextr3d(), fixPolygon()...")

    doxygen_add_docs(
        doxygen
        ${PROJECT_SOURCE_DIR}/cpp-client/include/nvidia/aiaa/client.h
        ${PROJECT_SOURCE_DIR}/cpp-client/include/nvidia/aiaa/aiaautils.h
        ${PROJECT_SOURCE_DIR}/cpp-client/include/nvidia/aiaa/common.h
        ${PROJECT_SOURCE_DIR}/cpp-client/include/nvidia/aiaa/model.h
        ${PROJECT_SOURCE_DIR}/cpp-client/include/nvidia/aiaa/pointset.h
        ${PROJECT_SOURCE_DIR}/cpp-client/include/nvidia/aiaa/polygon.h
        ${PROJECT_SOURCE_DIR}/cpp-client/include/nvidia/aiaa/utils.h
        ${PROJECT_SOURCE_DIR}/cpp-client/include/nvidia/aiaa/imageinfo.h
        ${PROJECT_SOURCE_DIR}/cpp-client/include/nvidia/aiaa/exception.h
        COMMENT "Generate doxygen html for NVIDIA AIAA cpp-client API"
    )
endif(DOXYGEN_FOUND)
