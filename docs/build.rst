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

Building
========

C++ Library
-----------

Build Requirements
++++++++++++++++++

 - [CMake](https://cmake.org/) (version >= 3.12.4)
 - C++ 11 or higher
 - For Windows Visual Studio 2017 [community version is free](https://visualstudio.microsoft.com/vs/community/)

Build Instructions
+++++++++++++++++++
.. code-block:: guess

	git clone https://github.com/NVIDIA/ai-assisted-annotation-client.git NvidiaAIAAClient
	
	# For Windows, checkout into shorter path like C:\NvidiaAIAAClient 
	# to avoid ITK build errors due to very longer path
	
	cd NvidiaAIAAClient
	mkdir build
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ../
	
	# If you have already ITK installed then
	cmake  -DCMAKE_INSTALL_PREFIX=/usr/local -DEXTERNAL_ITK_DIR=/home/xyz/install/lib/cmake/ITK-4.13 ../
	
	# If you have already Poco installed then
	cmake  -DCMAKE_INSTALL_PREFIX=/usr/local -DEXTERNAL_Poco_DIR=/home/xyz/install/lib/cmake/Poco ../


For Linux/Mac run::

	make -j16

	# To Install/Package
	cd NvidiaAIAAClient-Build
	make install
	make package



For Windows::

	msbuild NvidiaAIAAClient-superbuild.sln -m:4

	#Install/Package
	cd NvidiaAIAAClient-Build

> Open NvidiaAIAAClient.sln in IDE and run INSTALL/PACKAGE (currently works through IDE only)
> On Windows use CMAKE GUI client (and select Release for CMAKE_CONFIGURATION_TYPES) to configure and generate the files.

