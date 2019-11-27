cmake_minimum_required(VERSION 3.5)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://github.com/NVIDIA/ai-assisted-annotation-client/tree/master/slicer-plugin")
set(EXTENSION_CATEGORY "Segmentation")
set(EXTENSION_CONTRIBUTORS "Sachidanand Alle (NVIDIA)")
set(EXTENSION_DESCRIPTION "This extension offers automatic segmentation using NVIDIA AI-Assisted Annotation framework (Powered by the Clara Train SDK).")
set(EXTENSION_ICONURL "https://raw.githubusercontent.com/NVIDIA/ai-assisted-annotation-client/master/slicer-plugin/NvidiaAIAssistedAnnotation.png")
set(EXTENSION_SCREENSHOTURLS "https://raw.githubusercontent.com/NVIDIA/ai-assisted-annotation-client/master/slicer-plugin/snapshot.png")
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
