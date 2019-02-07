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

# NvidiaAIAAClient-CPP
#cmake_minimum_required(VERSION 3.12.4)
#project(NvidiaAIAAClient-CPP)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

message(STATUS "AIAA_LOG_DEBUG_ENABLED: ${AIAA_LOG_DEBUG_ENABLED}")
message(STATUS "AIAA_LOG_INFO_ENABLED: ${AIAA_LOG_INFO_ENABLED}")

add_compile_definitions(AIAA_LOG_DEBUG_ENABLED=${AIAA_LOG_DEBUG_ENABLED})
add_compile_definitions(AIAA_LOG_INFO_ENABLED=${AIAA_LOG_INFO_ENABLED})
add_compile_definitions(AIAA_MAKEDLL=1)
add_compile_definitions(POCO_NO_AUTOMATIC_LIBS=1)


include_directories(include)
file(GLOB NvidiaAIAAClient_headers include/*.h)
file(GLOB NvidiaAIAAClient_sources src/*.cpp)

add_library(NvidiaAIAAClient SHARED ${NvidiaAIAAClient_headers} ${NvidiaAIAAClient_sources})

target_include_directories(NvidiaAIAAClient PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)
    
####################################################################
# 3rdParty Dependencies
include_directories(${THIRDPARTY_BINARY_DIR}/include)

# Poco
find_package(Poco REQUIRED Foundation Util Net)
target_link_libraries(NvidiaAIAAClient Poco::Foundation Poco::Util Poco::Net)
if(MSVC)
    target_link_libraries(NvidiaAIAAClient iphlpapi.lib)
endif()

# ITK
find_package(ITK)
include(${ITK_USE_FILE})
target_link_libraries(NvidiaAIAAClient ${ITK_LIBRARIES})
####################################################################

include(CMakePackageConfigHelpers)

install(TARGETS NvidiaAIAAClient EXPORT NvidiaAIAAClientTargets
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin)

install(FILES 
         include/nvidia/aiaa/client.h
         include/nvidia/aiaa/common.h
         include/nvidia/aiaa/model.h
         include/nvidia/aiaa/pointset.h
         include/nvidia/aiaa/polygon.h
         include/nvidia/aiaa/utils.h
         include/nvidia/aiaa/imageinfo.h
         include/nvidia/aiaa/exception.h
       DESTINATION include/nvidia/aiaa)

install(EXPORT NvidiaAIAAClientTargets DESTINATION lib/cmake/NvidiaAIAAClient)

#export(TARGETS NvidiaAIAAClient FILE NvidiaAIAAClientConfig.cmake)

