..
  # Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
  #
  # Redistribution and use in source and binary forms, with or without
  # modification, are permitted provided that the following conditions
  # are met:
  #  * Redistributions of source code must retain the above copyright
  #    notice, this list of conditions and the following disclaimer.
  #  * Redistributions in binary form must reproduce the above copyright
  #    notice, this list of conditions and the following disclaimer in the
  #    documentation and/or other materials provided with the distribution.
  #  * Neither the name of NVIDIA CORPORATION nor the names of its
  #    contributors may be used to endorse or promote products derived
  #    from this software without specific prior written permission.
  #
  # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
  # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
  # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

C++ Tools
=========

Simple C++ implementations for the client API’s are provided in tools/aiaa folder.
These tools are built and installed as part of the source build.  Or you can download it directly from the release page.


List Models
-----------

Provides implementation for ``nvidia::aiaa::Client::model()`` API.
For more details refer `aiaa-model.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-model.cpp>`_
 
Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: 15, 40, 15, 30

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://10.110.45.66:5000/v1
   -label,Label Name for matching,,-label liver
   -output,Save output result into a file,,-output models.json

Example

.. code-block:: bash

   nvidiaAIAAListModels \
      -server http://10.110.45.66:5000/v1 \
      -label spleen


DEXTR3D
-------

Provides implementation for ``nvidia::aiaa::Client::dextra3d()`` API.
For more details refer `aiaa-dextra3d.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-dextra3d.cpp>`_
 
Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: auto

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://10.110.45.66:5000/v1
   -label,Label Name for matching;  Either -model or -label is required,,-label liver
   -model,Model Name,,-model Dextr3DLiver
   -points,"JSON Array of 3D Points (Image Indices) in [[x,y,z]+] format",,"-points [[70,172,86],...,[105,161,180]]"
   -image,Input image filename where image is stored in 3D format,,-image image.nii.gz
   -pixel,Input Image Pixel Type,short,short, unsigned int etc..
   -output,File name to store 3D binary mask image result from AIAA server,,-output result.nii.gz
   -pad,Padding size for input Image,20,-pad 20
   -roi,ROI Image size in XxYxZ format which is used while training the AIAA Model,128x128x128,-roi 96x96x96
   -sigma,Sigma Value for AIAA Server,3,-sigma 3

Example

.. code-block:: bash

   nvidiaAIAADEXTR3D \
      -server http://10.110.45.66:5000/v1 \
      -label spleen \
      -points `cat ../test/data/pointset.json` \
      -image _image.nii.gz \
      -output tmp_out.nii.gz \
      -pad 20 \
      -roi 128x128x128 \
      -sigma 3
 
   #(using model instead of label)
 
   nvidiaAIAADEXTR3D \
      -server http://10.110.45.66:5000/v1 \
      -model Dextr3DSpleen \
      -points `cat ../test/data/pointset.json` \
      -image _image.nii.gz \
      -output tmp_out.nii.gz \
      -pad 20 \
      -roi 128x128x128 \
      -sigma 3


Segmentation
-------

Provides implementation for ``nvidia::aiaa::Client::segmentation()`` API.
For more details refer `aiaa-segmentation.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-segmentation.cpp>`_
 
Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: auto

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://10.110.45.66:5000/v1
   -label,Label Name for matching;  Either -model or -label is required,,-label liver
   -model,Model Name,,-model Dextr3DLiver
   -image,Input image filename where image is stored in 3D format,,-image image.nii.gz
   -output,File name to store 3D binary mask image result from AIAA server,,-output result.nii.gz

Example

.. code-block:: bash

   nvidiaAIAADSegmentation \
      -server http://10.110.45.66:5000/v1 \
      -label spleen \
      -image _image.nii.gz \
      -output tmp_out.nii.gz
 
   #(using model instead of label)
 
   nvidiaAIAADSegmentation \
      -server http://10.110.45.66:5000/v1 \
      -model segmentation_spleen \
      -image _image.nii.gz \
      -output tmp_out.nii.gz


Mask 2D Polygon
---------------

Provides implementation for ``nvidia::aiaa::Client::mask2Polygon()`` API.
For more details refer `aiaa-mask-polygon.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-mask-polygon.cpp>`_
 
Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: auto

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://10.110.45.66:5000/v1
   -ratio,Point Ratio,10,-ratio 10
   -input,Input 3D binary mask image file name (which is an output of dextra3d),,-input tmp_out.nii.gz
   -output,Save output result (JSON Array) representing the list of polygons per slice to a file,,-output polygonlist.json

Example

.. code-block:: bash

   nvidiaAIAAMaskPolygon \
      -server http://10.110.45.66:5000/v1 \
      -image tmp_out.nii.gz \
      -output polygonlist.json


Fix Polygon
-----------

Provides implementation for ``nvidia::aiaa::Client::mask2Polygon()`` API.
For more details refer `aiaa-fix-polygon.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-fix-polygon.cpp>`_
 
Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: auto

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://10.110.45.66:5000/v1
   -neighbor,Neighborhood size for propagation,1,-neighbor 1
   -poly,"New 2D Polygon Array in [[[x,y]+]] format",,"-poly [[[54,162],…,[62,140]]]"
   -ppoly,"Current or Old 2D Polygon Array in [[[x,y]+]] format",,"-poly [[[53,162],…,[62,140]]]"
   -pindex,Polygon Index within new Polygon Array which needs to be updated,,-pindex 0
   -vindex,Vertical Index within new Polygon Array which needs to be updated,,-vindex 17
   -image,Input 2D image slice,,-image image_slice_2D.png
   -output,Output file name to the updated image,,-output updated_image_2D.png

Example

.. code-block:: bash

   nvidiaAIAAFixPolygon \
      -server http://10.110.45.66:5000/v1 \
      -neighbor 1 \
      -poly `cat ../test/data/polygons.json` \
      -ppoly `cat ../test/data/polygons.json` \
      -pindex 0 \
      -vindex 17 \
      -image ../test/data/image_slice_2D.png \
      -output updated_image_2D.png


