..
  # Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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

Installing C++ Client
=============================

.. code-block:: cpp

	#include <nvidia/aiaa/client.h>
	nvidia::aiaa::Client client("http://10.110.45.66:5000/v1");
	
	// List all models
	nvidia::aiaa::ModelList modelList = client.models();
	
	// Get matching model for organ Spleen
	nvidia::aiaa::Model model = modelList.getMatchingModel("spleen");
	
	// Call 3D Segmentation for a given model, points and input 3D image
	int ret = client.dextr3d(model, pointSet, input3dImageFile, outputDextra3dImageFile);


Compile your example code as::

	$ gcc -std=c++11 -I/home/xyz/install/include -L/home/xyz/install/lib -lNvidiaAIAAClient example.cpp

More details on C++ Client APIs can be found [here](http://docs.nvidia.com)


CMake Support
-------------
You can also use the NvidiaAIAAClient interface target in CMake. This target populates the appropriate usage requirements for INTERFACE_INCLUDE_DIRS to point to the appropriate include directories and INTERFACE_LIBRARY for linking the necessary Libraries.

External
++++++++
To use this library from a CMake project, you can locate it directly with find_package() and use the namespaced imported target from the generated package configuration:

.. code-block:: guess

	#CMakeLists.txt
	find_package(NvidiaAIAAClient REQUIRED)
	...
	include_directories(${NvidiaAIAAClient_INCLUDE_DIRS})
	...
	target_link_libraries(foo ${NvidiaAIAAClient_LIBRARY})


The package configuration file, NvidiaAIAAClientConfig.cmake, can be used either from an install tree or directly out of the build tree.
For example, you can specify the -DNvidiaAIAAClient_DIR option while generating the CMake targets for project foo::

	$ cmake -DNvidiaAIAAClient_DIR=/user/xyz/myinstall/lib/cmake/NvidiaAIAAClient


Embedded
++++++++
You can achieve this by adding External Project in CMake.

.. code-block:: guess

	#CMakeLists.txt
	...
	ExternalProject_Add(NvidiaAIAAClient
	   GIT_REPOSITORY https://github.com/NVIDIA/ai-assisted-annotation-client.git
	   GIT_TAG v1.0.0
	)
	...
	target_link_libraries(foo ${NvidiaAIAAClient_LIBRARY})
