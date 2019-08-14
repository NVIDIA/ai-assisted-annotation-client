cmake_minimum_required(VERSION 3.5)

project(NvidiaAIAssistedAnnotation)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "http://slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/NvidiaAIAssistedAnnotation")
set(EXTENSION_CATEGORY "Segmentation")
set(EXTENSION_CONTRIBUTORS "Sachidanand Alle (NVIDIA)")
set(EXTENSION_DESCRIPTION "This is a simple extension to demonstrate the features of NVIDIA AI-Assisted Annotation framework")
set(EXTENSION_ICONURL "http://www.example.com/Slicer/Extensions/NvidiaAIAssistedAnnotation.png")
set(EXTENSION_SCREENSHOTURLS "http://www.example.com/Slicer/Extensions/NvidiaAIAssistedAnnotation/Screenshots/1.png")
set(EXTENSION_DEPENDS "NA") # Specified as a space separated string, a list or 'NA' if any

#-----------------------------------------------------------------------------
# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(NvidiaAIAA)
## NEXT_MODULE

#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_GENERATE_CONFIG})
include(${Slicer_EXTENSION_CPACK})
