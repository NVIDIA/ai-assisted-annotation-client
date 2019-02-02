..
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

Getting Started
===============

Nvidia AI-Assisted Annotation SDK follows a client-server approach to integrate into an application.  Once a user has been granted early access user can use either C++ or Python client to integrate the SDK into an existing medical imaging application.

It officially supports following 3 platforms
   - Windows (Windows 10)
   - Linux (Ubuntu16+)
   - macOS (High Sierra and above)


Installing prebuilt C++ packages
--------------------------------
Download the binary packages for corresponding OS supported from here.


Linux/MacOS
^^^^^^^^^^^

.. code-block:: bash

   wget http://my.release/mac_os.tar.gz
   tar xvf mac_os.tar.gz ~/install

   export LD_LIBRARY_PATH=~/install/lib # For Linux
   export DYLD_LIBRARY_PATH=~/install/lib # For MacOS
   
   gcc -std=c++11 -I~/install/include -lNvidiaAIAAClient example.cpp -o example
   ./example
   

Windows
^^^^^^^

.. code-block:: bash

   wget http://my.release/windows_os.zip
   tar xvf windows_os.tar.gz c:/install

   export PATH=~/install/lib:%PATH%
   cl -std=c++11 -Ic:/install/include -Lc:/install/lib -lNvidiaAIAAClient example.cpp -o example
   ./example


Following is the code snippet to start using AIAA Client APIs

.. code-block:: cpp

   // example.cpp
   #include <nvidia/aiaa/client.h>
   #include <iostream>

   int main() {
      // Create AIAA Client object
      nvidia::aiaa::Client client("http://my-aiaa-server.com:5000/v1");
   
      // List all models
      nvidia::aiaa::ModelList modelList = client.models();
      std::cout << "Models Supported by AIAA Server: " << modelList.toJson() << std::endl();
   
      // Get matching model for organ Spleen
      nvidia::aiaa::Model model = modelList.getMatchingModel("spleen");
      std::cout << "Selected AIAA Model for organ 'Spleen' is: " << model.toJson(2) << std::endl();
   
      // More API calls can follow here...
      return 0;
   }

More details on C++ Client APIs can be found `client.h
<https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/include/nvidia/aiaa/client.h>`_



CMake Support
-------------
You can also use the NvidiaAIAAClient interface target in CMake. This target populates the appropriate usage requirements for ``NvidiaAIAAClient_INCLUDE_DIRS`` to point to the appropriate include directories and ``NvidiaAIAAClient_LIBRARY`` for linking the necessary Libraries.


Find Package
^^^^^^^^^^^^
To use this library from a CMake project, you can locate it directly with find_package() and use the namespaced imported target from the generated package configuration:

.. code-block:: guess

   # CMakeLists.txt
   find_package(NvidiaAIAAClient REQUIRED)
   ...
   include_directories(${NvidiaAIAAClient_INCLUDE_DIRS})
   ...
   target_link_libraries(foo ${NvidiaAIAAClient_LIBRARY})


The package configuration file, NvidiaAIAAClientConfig.cmake, can be used either from an install tree or directly out of the build tree.
For example, you can specify the ``-DNvidiaAIAAClient_DIR`` option while generating the CMake targets for project foo::

   $ cmake -DNvidiaAIAAClient_DIR=/user/xyz/myinstall/lib/cmake/NvidiaAIAAClient


External Project
^^^^^^^^^^^^^^^^
You can achieve this by adding External Project in CMake.

.. code-block:: guess

   # CMakeLists.txt
   ...
   ExternalProject_Add(NvidiaAIAAClient
      GIT_REPOSITORY https://github.com/NVIDIA/ai-assisted-annotation-client.git
      GIT_TAG v1.0.0
   )
   ...
   target_link_libraries(foo ${NvidiaAIAAClient_LIBRARY})


