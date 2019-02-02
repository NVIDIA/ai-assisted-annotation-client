# Nvidia AI-Assisted Annotation Client
Nvidia AI-Assisted Annotation SDK follows a client-server approach to integrate into an application.  Once a user has been granted early access user can use either C++ or Python client to integrate the SDK into an existing medical imaging application.

[![Documentation](https://img.shields.io/badge/Nvidia-documentation-brightgreen.svg)](https://docs.nvidia.com/deeplearning/sdk/ai-assisted-annotation-client-guide)
[![GitHub license](https://img.shields.io/badge/license-BSD-blue.svg)](https://raw.githubusercontent.com/NVIDIA/ai-assisted-annotation-client/master/LICENSE)
[![GitHub Releases](https://img.shields.io/github/release/NVIDIA/ai-assisted-annotation-client.svg)](https://github.com/NVIDIA/ai-assisted-annotation-client/releases)
[![GitHub Issues](https://img.shields.io/github/issues/NVIDIA/ai-assisted-annotation-client.svg)](http://github.com/NVIDIA/ai-assisted-annotation-client/issues)

## Supported Platforms
AI-Assisted Annotation is a cross-platform C++/Python Client API to communicate with AI-Assisted Annotation Server for Nvidia and it officially supports:
 - Windows
 - Linux
 - MacOS

## Quick Start
Follow the [Quick Start](https://docs.nvidia.com/deeplearning/sdk/ai-assisted-annotation-client-guide#quickstart) guide to build/install AI-Assisted Annotation Client Libraries for C++/Python and run some basic tools to verify few important functionalities like *dextr3D*, *fixPolygon* over an existing AI-Assisted Annotation Server.

>C++ Client Library is provides support for CMake project

## External Use-Case
The [Medical Imaging Interaction Toolkit](http://mitk.org/wiki/MITK) (MITK) is a free open-source software system for development of interactive medical image processing software.
Please follow [mitk-nvidia-plugin](https://github.com/SachidanandAlle/mitk-nvidia-plugin) to get a demo on Auto Segmentation *(dextr3D)*, Smart Polygon Fix *(fixPolygon)* features supported through AI-Assisted Annotation Client.

## Contributions
Contributions to Nvidia AI-Assisted Annotation Client are more than welcome. To contribute make a pull request and follow the guidelines outlined in the [CONTRIBUTING](/CONTRIBUTING.md) document.
