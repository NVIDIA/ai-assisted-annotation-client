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

# NvidiaAIAAClientTools
#cmake_minimum_required(VERSION 3.12.4)
#project(AIAAClientTools)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ITK
find_package(ITK)
include(${ITK_USE_FILE})

# DICOM NIFTI Utils
add_executable(DicomToNifti dicom/dicom-to-nifti.cpp)
target_link_libraries(DicomToNifti ${ITK_LIBRARIES})

add_executable(NiftiToDicom dicom/nifti-to-dicom.cpp)
target_link_libraries(NiftiToDicom ${ITK_LIBRARIES})


# AIAA Client API implementations
# AIAA Client API implementations
add_executable(nvidiaAIAAListModels aiaa/aiaa-model.cpp)
target_link_libraries(nvidiaAIAAListModels NvidiaAIAAClient ${CMAKE_DL_LIBS})

add_executable(nvidiaAIAADEXTR3D aiaa/aiaa-dextra3d.cpp)
target_link_libraries(nvidiaAIAADEXTR3D NvidiaAIAAClient ${CMAKE_DL_LIBS})

add_executable(nvidiaAIAASegmentation aiaa/aiaa-segmentation.cpp)
target_link_libraries(nvidiaAIAASegmentation NvidiaAIAAClient ${CMAKE_DL_LIBS})

add_executable(nvidiaAIAASampling3D aiaa/aiaa-sampling3d.cpp)
target_link_libraries(nvidiaAIAASampling3D NvidiaAIAAClient ${CMAKE_DL_LIBS})

add_executable(nvidiaAIAAFixPolygon aiaa/aiaa-fix-polygon.cpp)
target_link_libraries(nvidiaAIAAFixPolygon NvidiaAIAAClient ${CMAKE_DL_LIBS})

add_executable(nvidiaAIAAMaskPolygon aiaa/aiaa-mask-polygon.cpp)
target_link_libraries(nvidiaAIAAMaskPolygon NvidiaAIAAClient ${CMAKE_DL_LIBS})

add_executable(nvidiaAIAASession aiaa/aiaa-session.cpp)
target_link_libraries(nvidiaAIAASession NvidiaAIAAClient ${CMAKE_DL_LIBS})

add_executable(nvidiaAIAADeepgrow aiaa/aiaa-deepgrow.cpp)
target_link_libraries(nvidiaAIAADeepgrow NvidiaAIAAClient ${CMAKE_DL_LIBS})

add_executable(nvidiaAIAAInference aiaa/aiaa-inference.cpp)
target_link_libraries(nvidiaAIAAInference NvidiaAIAAClient ${CMAKE_DL_LIBS})

# Install Targets
install(TARGETS DicomToNifti           DESTINATION bin)
install(TARGETS NiftiToDicom           DESTINATION bin)

install(TARGETS nvidiaAIAAListModels   DESTINATION bin)
install(TARGETS nvidiaAIAADEXTR3D      DESTINATION bin)
install(TARGETS nvidiaAIAASegmentation DESTINATION bin)
install(TARGETS nvidiaAIAAFixPolygon   DESTINATION bin)
install(TARGETS nvidiaAIAAMaskPolygon  DESTINATION bin)
install(TARGETS nvidiaAIAASampling3D   DESTINATION bin)
install(TARGETS nvidiaAIAASession      DESTINATION bin)
install(TARGETS nvidiaAIAADeepgrow     DESTINATION bin)
install(TARGETS nvidiaAIAAInference    DESTINATION bin)
