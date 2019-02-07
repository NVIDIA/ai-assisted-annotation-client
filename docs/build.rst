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

Building
========


Building C++ Client
-------------------

Prerequisites
^^^^^^^^^^^^^
   - `CMake <https://cmake.org>`_ (version >= 3.12.4)
   - For Linux/Mac gcc which supports C++ 11 or higher
   - For Windows `Visual Studio 2017 <https://visualstudio.microsoft.com/downloads>`_

External Dependencies
^^^^^^^^^^^^^^^^^^^^^
   - `ITK <https://itk.org>`_ (version = 4.13..1)
   - `Poco <https://pocoproject.org>`_  (version = 1.9.0)
   - `nlohmann_json <https://github.com/nlohmann/json>`_
   
.. note::
   Above are source only dependencies for CMake project.
   They will get downloaded and built as part of super-build project for AIAA Client.
   
Building Binaries
^^^^^^^^^^^^^^^^^
Following the below instructions to get the source code and build the project

.. code-block:: bash

   git clone https://github.com/NVIDIA/ai-assisted-annotation-client.git NvidiaAIAAClient
   cd NvidiaAIAAClient
   mkdir build
   cmake -DCMAKE_BUILD_TYPE=Release ../
   
   # If ITK is installed locally
   export MYINSTALL_DIR=/home/xyz/install
   cmake -DCMAKE_BUILD_TYPE=Release -DITK_DIR=${MYINSTALL_DIR}/lib/cmake/ITK-4.13 ../
   
   # If ITK and Poco are installed locally
   cmake -DCMAKE_BUILD_TYPE=Release -DITK_DIR=${MYINSTALL_DIR}/lib/cmake/ITK-4.13 -DPoco_DIR=${MYINSTALL_DIR}/lib/cmake/Poco ../


.. note::
   - Use Release mode for faster build.
   - For Windows, use CMAKE GUI client to configure and generate the files.  
   - For Windows, if ITK is not externally installed, then use shorter path for *ROOT* Folder e.g. ``C:/NvidiaAIAAClient`` to avoid ``LongPath`` error.

Following are some additional CMake Flags helpful while configuring the project.
   -  ``ITK_DIR`` - use already installed ITK libraries and includes
   -  ``Poco_DIR`` - use already installed Poco libraries and includes
   -  ``AIAA_LOG_DEBUG_ENABLED`` - enable/disable Debug-level Logging (default: 0)
   -  ``AIAA_LOG_INFO_ENABLED`` - enable/disable Info-level Logging (default: 1)

To Build binaries/package on Linux/MacOS::
   make -j6
   cd NvidiaAIAAClient-Build || make package

To Build binaries/package on Windows::
   - open ``NvidiaAIAAClient-superbuild.sln`` and run ``ALL_BUILD`` target
   - open ``NvidiaAIAAClient.sln`` under NvidiaAIAAClient-Build and run ``PACKAGE`` target to build *(in Release mode)* an installable-package


Building the Documentation
--------------------------

The NVIDIA AI-Assisted Annotation Client documentation is found in the docs/ directory and is based
on `Sphinx <http://www.sphinx-doc.org>`_.  `Doxygen <http://www.doxygen.org/>`_ integrated with `Exhale <https://github.com/svenevs/exhale>`_ is 
used for C++ API docuementation.

To build the docs install the required dependencies::

  $ apt-get update
  $ apt-get install -y --no-install-recommends doxygen
  $ pip install --upgrade sphinx sphinx-rtd-theme nbsphinx exhale breathe numpy requests_toolbelt nibabel

Then use Sphinx to build the documentation into the build/html
directory::

  $ cd docs
  $ make clean html
