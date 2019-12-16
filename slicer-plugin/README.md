# Nvidia AI-assisted annotation (AIAA) for 3D Slicer

Nvidia AI-assisted segmentation is available in [3D Slicer](https://www.slicer.org), a popular free, open-source medical image visualization and analysis application. The tool is available in Segment Editor module of the application.

The tool has two modes:
- Fully automatic segmentation: no user inputs required. In "Auto-segmentation" section, choose model, and click Start. Segmentation process may take several minutes (up to about 5-10 minutes for large data sets on computers with slow network upload speed).
- Boundary-points based segmentation: requires user to specify input points near the edge of the structure of interest, one on each side. Segmentation typcally takes less than a minute.

Example result of automatic segmentation:
![](snapshot.jpg?raw=true "Example segmentation result")

If you have any questions or suggestions, post it on [3D Slicer forum](https://discourse.slicer.org).

## Setup
- Download and install recent 3D Slicer Preview Release (4.11.x) from http://download.slicer.org/
- Start 3D Slicer and open the Extension manager
- Install NvidiaAIAssistedAnnotation extension (in Segmentation category), wait for the installation to complete, and click Restart
- Optional: set annotation server settings in menu: Edit / Application settings / NVidia. If server address is left empty then a default public server will be used. There is no guarantee for availability of the server or quality of the offered models.

To set up your own segmentation server, follow [these instructions](https://docs.nvidia.com/clara/aiaa/tlt-mi-ai-an-sdk-getting-started/index.html). For running pre-trained models A moderate desktop computer with strong NVidia GPU (for example, a few years old computer with 8GB memory and an RTX 2070 GPU) will suffice. Linux operating system is required on the server.

The computer running 3D Slicer does not have any special requirements to run AI-assisted segmentation. Fast network upload speed is recommended for large data sets, as segmentation typically takes just seconds, while image data transfer takes minutes.

## Tutorials and examples

### Boundary-point based segmentation of brain tumor MRI:

- Go to **Sample Data** module and load "MRBrainTumor1" data set
- Go to **Segment Editor**
- Create a new segment
- Click "Nvidia AIAA" effect
- In "Segment from boundary points" section, select "annotation_mri_brain_tumors_t1ce_tc" (model trained to segment tumor on contrast-enhanced brain MRI)
- Click "Place markup point" button, and click near the edge of the tumor on all 6 sides in slice views, then click "Start" (if a popup is displayed about sending image data to a remote server, click OK to acknowledge it)

![](snapshot-annotation-points-brain.jpg?raw=true "Brain tumor input boundary points")

- Within about 10 seconds, automatic segmentation results appear in slice views. To visualize results in 3D, click "Show 3D" button above the segment list. To show slice image in 3D, click the "pushpin" icon in the top-left corner of a slice view then click the "eye" icon.

![](snapshot-annotation-result-brain.jpg?raw=true "Brain tumor annotation results")

### Boundary-point based segmentation of liver on CT:

- Go to **Sample Data** module and load "CTACardio" data set
- Go to **Segment Editor**
- Create a new segment
- Double-click the segment name and type "liver" to specify the segment content
- Click "Nvidia AIAA" effect, "Segment from boundary points" section
- Optional: click "filter" icon to list only those models that contain "liver" in their name
- Select "annotation_ct_liver" (model trained to segment liver in portal-venous-phase CT image)
- Click "Place markup point" button, and click near the edge of the liver on all 6 sides in slice views, then click "Start"

![](snapshot-annotation-points-liver.jpg?raw=true "Liver input boundary points")

- Within about 30 seconds, automatic segmentation results appear in slice views. To visualize results in 3D, click "Show 3D" button above the segment list.
- To adjust previously placed boundary points after segmentation is completed, click the "edit" icon next to "Start" button.

![](snapshot-annotation-result-liver.jpg?raw=true "Liver annotation results")

### Fully automatic segmentation of liver and tumor on CT:

- Download Task03_Liver\imagesTr\liver_102.nii.gz data set from http://medicaldecathlon.com/ and load it into 3D Slicer
- Go to **Segment Editor**
- Click "Nvidia AIAA" effect
- In "Auto-segmentation" section, select "segmentation_ct_liver_and_tumor" model, then click "Start"
- Automatic segmentation result should be displayed within 3-5 minutes
- Optional: Go to **Segmentations** module to edit display settings

![](snapshot-segmentation-result-liver.jpg?raw=true "Automatic liver and tumor segmentation results")

## Advanced

- For locally set up servers or computers with fast upload speed, disable compression in Application Settings / NVidia (since compression may take more time than the time saved by transferring less data)
- To filter models based on labels attached to them, set label text in "Models" section of Nvidia AIAA effect user interface.

## For developers
The plugin can be downloaded and installed directly from GitHub:
- git clone https://github.com/NVIDIA/ai-assisted-annotation-client.git
- Open 3D Slicer: Go to Edit -> Application Settings -> Modules -> Additional Module Paths
   1) Add New Module Path: <FULL_PATH>/slicer-plugin/NvidiaAIAA
   2) Restart
- To build extension package, build 3D Slicer then configure ai-assisted-annotation-client project using CMake, defining these variables: -DSlicer_DIR:PATH=... -DNvidiaAIAssistedAnnotation_BUILD_SLICER_EXTENSION:BOOL=ON
