# NVIDIA AI-Assisted Annotation Client
NVIDIA AI-Assisted Annotation SDK follows a client-server approach to integrate into an application.  Once a user has been granted early access user can use either C++ or Python client to integrate the SDK into an existing medical imaging application.

[![Documentation](https://img.shields.io/badge/NVIDIA-documentation-brightgreen.svg)](https://docs.nvidia.com/deeplearning/sdk/ai-assisted-annotation-client-guide)
[![GitHub license](https://img.shields.io/badge/license-BSD3-blue.svg)](/LICENSE)
[![GitHub Releases](https://img.shields.io/github/release/NVIDIA/ai-assisted-annotation-client.svg)](https://github.com/NVIDIA/ai-assisted-annotation-client/releases)
[![GitHub Issues](https://img.shields.io/github/issues/NVIDIA/ai-assisted-annotation-client.svg)](https://github.com/NVIDIA/ai-assisted-annotation-client/issues)

## Supported Platforms
AI-Assisted Annotation is a cross-platform C++/Python Client API to communicate with AI-Assisted Annotation Server for NVIDIA and it officially supports:
 - Linux (Ubuntu16+)
 - macOS (High Sierra and above)
 - Windows (Windows 10)

## Quick Start
Follow the [Quick Start](https://docs.nvidia.com/deeplearning/sdk/ai-assisted-annotation-client-guide/quickstart.html) guide to build/install AI-Assisted Annotation Client Libraries for C++/Python and run some basic tools to verify few important functionalities like *dextr3D*, *fixPolygon* over an existing AI-Assisted Annotation Server.

>C++ Client Library is provides support for CMake project

## External Use-Case
The [Medical Imaging Interaction Toolkit](http://mitk.org/wiki/MITK) (MITK) is a free open-source software system for development of interactive medical image processing software.
Please follow [mitk-nvidia-plugin](https://github.com/NVIDIA/mitk-nvidia-plugin) to get a demo on Auto Segmentation *(dextr3D)*, Smart Polygon Fix *(fixPolygon)* features supported through AI-Assisted Annotation Client.

## Contributions
Contributions to NVIDIA AI-Assisted Annotation Client are more than welcome. To contribute make a pull request and follow the guidelines outlined in the [CONTRIBUTING](/CONTRIBUTING.md) document.
