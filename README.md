# Nvidia AI-Assisted Annotation Client
Nvidia AI-Assisted Annotation SDK follows a client-server approach to integrate into an application.  Once a user has been granted early access user can use either C++ or Python client to integrate the SDK into an existing medical imaging application.

[![Documentation](https://img.shields.io/badge/docs-doxygen-blue.svg)](http://docs.nvidia.com)
[![GitHub license](https://img.shields.io/badge/license-BSD-blue.svg)](https://raw.githubusercontent.com/NVIDIA/ai-assisted-annotation-client/master/LICENSE)
[![GitHub Releases](https://img.shields.io/github/release/NVIDIA/ai-assisted-annotation-client.svg)](https://github.com/NVIDIA/ai-assisted-annotation-client/releases)
[![GitHub Issues](https://img.shields.io/github/issues/NVIDIA/ai-assisted-annotation-client.svg)](http://github.com/NVIDIA/ai-assisted-annotation-client/issues)


### Integration

#### Supported Platforms
AI-Assisted Annotation is a cross-platform C++/Python Client API to communicate with AI-Assisted Annotation Server for NVidia and it officially supports:
 - Windows
 - Linux
 - MacOS
 
#### Python Client Integration
Need to write some here...

### C++ Client Integration
Follow the [build instructions](http://github.com/NVIDIA/ai-assisted-annotation-client/cpp-client/BuildInstractions) to compile and install from the source code.  Or directly download the binaries available for certain platforms [released here](https://github.com/NVIDIA/ai-assisted-annotation-client/releases).
Following is a sample code to invoke Segmentation API.

```cpp
#include <nvidia/aiaa/client.h>
nvidia::aiaa::Client client("http://10.110.45.66:5000/v1");

// List all models
nvidia::aiaa::ModelList modelList = client.models();

// Get matching model for organ Spleen
nvidia::aiaa::Model model = modelList.getMatchingModel("spleen");

// Call 3D Segmentation for a given model, points and input 3D image
int ret = client.dextr3d(model, pointSet, input3dImageFile, outputDextra3dImageFile);

```
More details on C++ Client APIs can be found [here](http://docs.nvidia.com)


##### CMake
You can also use the NvidiaAIAAClient interface target in CMake. This target populates the appropriate usage requirements for INTERFACE_INCLUDE_DIRS to point to the appropriate include directories and INTERFACE_LIBRARY for linking the necessary Libraries.

###### External
To use this library from a CMake project, you can locate it directly with find_package() and use the namespaced imported target from the generated package configuration:

```cmake
#CMakeLists.txt
find_package(NvidiaAIAAClient REQUIRED)
...
include_directories(${NvidiaAIAAClient_INCLUDE_DIRS})
...
target_link_libraries(foo ${NvidiaAIAAClient_LIBRARY})
```

The package configuration file, NvidiaAIAAClientConfig.cmake, can be used either from an install tree or directly out of the build tree.
For example, you can specify the -DNvidiaAIAAClient_DIR option while generating the CMake targets for project foo.

```console
cmake -DNvidiaAIAAClient_DIR=/user/xyz/myinstall/lib/cmake/NvidiaAIAAClient
```

###### Embedded
You can achieve this by adding External Project in CMake.

```cmake
#CMakeLists.txt
...
ExternalProject_Add(${proj}
   GIT_REPOSITORY https://github.com/NVIDIA/ai-assisted-annotation-client.git
   GIT_TAG v1.0.0
)
...
target_link_libraries(foo ${NvidiaAIAAClient_LIBRARY})
```

### License

### Contributions

