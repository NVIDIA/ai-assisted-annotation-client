# Nvidia 3DSlicer-Extension

## Installation
- Download and install 3D Slicer from http://download.slicer.org/
- Start Slicer, in the Extension manager install NvidiaAIAssistedAnnotation extension (in Segmentation category), click Restart
- Open menu: Edit / Application settings / NVidia section
- Enter IP address and port of annotation server (if you do not know of an available server, you can set up your own as described [here](https://docs.nvidia.com/clara/aiaa/tlt-mi-ai-an-sdk-getting-started/index.html))

## Demo
For a short demo video [Click Here](https://drive.google.com/open?id=1cQgCBl4v3OqI3Hh8hzMvR01xiEBO2GLE)

## Tutorial

- Install 3D Slicer and NvidiaAIAssistedAnnotation as described above
- Load data set to be segmented (you can use your own, or download sample data sets from http://medicaldecathlon.com/)
- Go to **Segment Editor**
- Create a segment, double-click on its name to set organ to be segmented
- Click "Nvidia AIAA" effect
- Click "Fetch models". All models that can segment the chosen organ name will be listed in "Auto-segmentation" and "Annotation" sections. If no suitable model is found, you can disable "Filter by label" option to list all available models (selected segment name is then ignored)
- Click "Run segmentation" to automatically segment all segments that are defined in the model

Example result of automatic segmentation:
![](snapshot.png?raw=true "Example segmentation result")

If needed, refine the segmentation result by selecting a segment and using "Annotation" section:
- Select annotation Model
- Click "Place a markup point"
- Place markup points at the structure's boundaries
- Click "Run DExtr3D"

## For developers
The plugin can be downloaded and installed directly from GitHub:
- git clone https://github.com/NVIDIA/ai-assisted-annotation-client.git
- Open 3D Slicer: Go to Edit -> Application Settings -> Modules -> Additional Module Paths
   1) Add New Module Path: <FULL_PATH>/slicer-plugin/NvidiaAIAA
   2) Restart
- To build extension package, build 3D Slicer then configure ai-assisted-annotation-client project using CMake, defining these variables: -DSlicer_DIR:PATH=... -DNvidiaAIAssistedAnnotation_BUILD_SLICER_EXTENSION:BOOL=ON
