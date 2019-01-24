# AIAA Client
Wait for more details.

### Building from Source

##### Requirements
 - [CMake](https://cmake.org/) (version >= 3.12.4)
 - C++ 11 or higher
 - For Windows Visual Studio 2017 [community version is free](https://visualstudio.microsoft.com/vs/community/)

##### Build Instructions
```
git clone https://github.com/nvidia/AIAAClient.git NvidiaAIAAClient

# For Windows, checkout into shorter path like C:\NvidiaAIAAClient 
# to avoid ITK build errors due to very longer path

# For e.g. IF you are working on ai-infro repository, then consider to
# move C:\Projects\ai-infra\medical\annotation\AIAAClient to C:\NvidiaAIAAClient 

cd NvidiaAIAAClient
mkdir build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ../

# If you have already ITK installed then
cmake  -DCMAKE_INSTALL_PREFIX=/usr/local -DEXTERNAL_ITK_DIR=/home/xyz/install/lib/cmake/ITK-4.13 ../

# If you have already Poco installed then
cmake  -DCMAKE_INSTALL_PREFIX=/usr/local -DEXTERNAL_Poco_DIR=/home/xyz/install/lib/cmake/Poco ../


#####################################################################
# Linux/Mac
make -j16

# Install/Package
cd NvidiaAIAAClient-Build
make install
make package


#####################################################################
# Windows
msbuild NvidiaAIAAClient-superbuild.sln -m:4

# Install/Package
cd NvidiaAIAAClient-Build

# Open NvidiaAIAAClient.sln in IDE and run INSTALL/PACKAGE (currently works through IDE only)

```

> On Windows use CMAKE GUI client (and select Release for CMAKE_CONFIGURATION_TYPES) to configure and generate the files.


##### To use from CMake:
```
cmake_minimum_required(VERSION 3.12.4)
project(main)

set(CMAKE_CXX_STANDARD 11)

find_package(NvidiaAIAAClient REQUIRED)
include_directories(${NvidiaAIAAClient_INCLUDE_DIRS})

add_executable(main main.cpp)
target_link_libraries(main ${NvidiaAIAAClient_LIBRARY})
```

Generate CMake files for main by

```
cmake -DNvidiaAIAAClient_DIR=/user/xyz/myinstall/lib/cmake/NvidiaAIAAClient
```


##### API Docs
Generate Doxygen Documents for AIAA Client by running the following target

```
make doxygen
```

#### Command-Line Tools
The build will output the following useful command-line utilites

```
make install

# If install path is not /usr/local then
export PATH=$PATH:myinstall/bin
export LD_LIBRARY_PATH=myinstall/lib         # for Linux
export DYLD_LIBRARY_PATH=myinstall/lib       # for MacOS

#fetch model list
nvidiaAIAAListModels \
  -server http://10.110.45.66:5000/v1

#fetch matching model
nvidiaAIAAListModels \
  -server http://10.110.45.66:5000/v1 \
  -label spleen

#call dextr3d from command line
#You can download the sample _image.nii.gz from https://drive.google.com/open?id=1WzxapzANOe7aOO-PNc76U5gXepk_ZBQh
nvidiaAIAADEXTR3D \
  -server http://10.110.45.66:5000/v1 \
  -label spleen \
  -points `cat ../test/data/pointset.json` \
  -image _image.nii.gz \
  -output tmp_out.nii.gz

#call mask2Polygon
nvidiaAIAAMaskPolygon \
  -server http://10.110.45.66:5000/v1 \
  -image tmp_out.nii.gz \
  -output polygonlist.json

#call fixPolygon
nvidiaAIAAFixPolygon \
  -server http://10.110.45.66:5000/v1 \
  -neighbor 10 \
  -poly `cat ../test/data/polygons.json` \
  -ppoly `cat ../test/data/polygons.json` \
  -pindex 0 \
  -vindex 17 \
  -image ../test/data/image_slice_2D.png \
  -output updated_image_2d.png
```

> For more details on the parameters run *nvidiaAIAAxxx -h* to print the help information


### Supported Platforms
AIAAClient is a cross-platform C++/Python Client API to communicate with AIAA Server for NVidia and it officially supports:

 - Windows
 - Linux
 - macOS

