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

#-----------------------------------------------------------------------------
# NvidiaAIAAClient 
#-----------------------------------------------------------------------------

# Sanity checks
if(DEFINED NvidiaAIAAClient_DIR AND NOT EXISTS ${NvidiaAIAAClient_DIR})
  message(FATAL_ERROR "NvidiaAIAAClient_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj NvidiaAIAAClient)
set(proj_DEPENDENCIES )
set(${proj}_DEPENDS ${proj})

if(NOT DEFINED NvidiaAIAAClient_DIR)

  set(additional_cmake_args )

  if(EXTERNAL_ITK_DIR)
    list(APPEND additional_cmake_args -DITK_DIR:PATH=${EXTERNAL_ITK_DIR})
  else()
    list(APPEND proj_DEPENDENCIES ITK)
    list(APPEND additional_cmake_args -DITK_DIR:PATH=${ep_prefix}/lib/cmake/ITK-4.13)
  endif()

  if(EXTERNAL_Poco_DIR)
    list(APPEND additional_cmake_args -DPoco_DIR:PATH=${EXTERNAL_Poco_DIR})
  else()
    list(APPEND proj_DEPENDENCIES Poco)
    list(APPEND additional_cmake_args -DPoco_DIR:PATH=${ep_prefix}/lib/cmake/Poco)
  endif()

  list(APPEND additional_cmake_args -DAIAA_LOG_DEBUG_ENABLED=1)

  message(STATUS "NvidiaAIAAClient CMAKE_CURRENT_LIST_DIR = ${CMAKE_CURRENT_LIST_DIR}")
  ExternalProject_Add(${proj}
     SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../
     DOWNLOAD_COMMAND ""
     #BUILD_ALWAYS 1

     CMAKE_GENERATOR ${gen}
     CMAKE_ARGS
       ${ep_common_args}
       ${additional_cmake_args}
     DEPENDS ${proj_DEPENDENCIES}
    )

  set(${proj}_DIR ${ep_prefix})

else()
  mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
endif()
