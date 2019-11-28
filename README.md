# NVIDIA AI-Assisted Annotation Client
NVIDIA AI-Assisted Annotation SDK follows a client-server approach to integrate into an application.  Clara Train SDK container on [ngc.nvidia.com](https://ngc.nvidia.com/) is generally available for download, annotation server is included in the package.  Once a user has setup the AI-Assisted Annotation Server, user can use either C++ or Python client to integrate the SDK into an existing medical imaging application.

[![Documentation](https://img.shields.io/badge/NVIDIA-documentation-brightgreen.svg)](https://docs.nvidia.com/clara/aiaa/sdk-api/docs/index.html)
[![GitHub license](https://img.shields.io/badge/license-BSD3-blue.svg)](/LICENSE)
[![GitHub Releases](https://img.shields.io/github/release/NVIDIA/ai-assisted-annotation-client.svg)](https://github.com/NVIDIA/ai-assisted-annotation-client/releases)
[![GitHub Issues](https://img.shields.io/github/issues/NVIDIA/ai-assisted-annotation-client.svg)](https://github.com/NVIDIA/ai-assisted-annotation-client/issues)

## Supported Platforms
AI-Assisted Annotation is a cross-platform C++/Python Client API to communicate with AI-Assisted Annotation Server for NVIDIA and it officially supports:
 - Linux (Ubuntu16+)
 - macOS (High Sierra and above)
 - Windows (Windows 10)

## Plugins
Following plugins integrate with NVIDIA AI-Assisted Annotation through either C++/Python Client APIs
- [NVIDIA MITK Plugin](/mitk-plugin) *(based on c++ client APIs)*
- [NVIDIA 3D Slicer](/slicer-plugin) *(based on py-client APIs)*

## Quick Start
Follow the [Quick Start](https://docs.nvidia.com/clara/aiaa/sdk-api/docs/quickstart.html) guide to build/install AI-Assisted Annotation Client Libraries for C++/Python and run some basic tools to verify few important functionalities like *dextr3D*, *segmentation*, *fixPolygon* over an existing AI-Assisted Annotation Server.

>C++ Client Library provides support for CMake project

## Contributions
Contributions to NVIDIA AI-Assisted Annotation Client are more than welcome. To contribute make a pull request and follow the guidelines outlined in the [CONTRIBUTING](/CONTRIBUTING.md) document.
