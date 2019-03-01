# MITK Plugin
This is the plugin source for MITK which integrates AIAA client with MITK Workbench

## Features
 - NVIDIA Segmentation *(3D Tool)*
 - NVIDIA Smart Polygon Fix *(2D Tool)*

## Quick Start
The project follows the template recommended by MITK to fit the extension mechanism of MITK v2018.04. 
Here's how it basically works:

1. Clone MITK
2. Clone ai-assisted-annotation-client
3. Configure the MITK superbuild and set the advanced CMake cache variable `MITK_EXTENSION_DIRS` to ai-assisted-annotation-client/mitk-plugin
4. Generate and build the MITK superbuild


## Supported platforms and other requirements
See the [MITK documentation](http://docs.mitk.org/2018.04/)
