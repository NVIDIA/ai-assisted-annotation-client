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

.. attention::
 Always use **-h** option to get the **latest information** on *options available* when you run these tools


List Models
-----------

Provides implementation for ``nvidia::aiaa::Client::model()`` API.
For more details refer `aiaa-model.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-model.cpp>`_
 
Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: 15, 40, 15, 30

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://0.0.0.0:5000
   -label,Label Name for matching,,-label liver
   -type,Find Matching Model of type (segmentation/annotation),,-type segmentation
   -output,Save output result into a file,,-output models.json

Example

.. code-block:: bash

   nvidiaAIAAListModels \
      -server http://0.0.0.0:5000 \
      -label spleen


Session
------------

Provides implementation for ``nvidia::aiaa::Client::create_session()`` API.
For more details refer `aiaa-session.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-session.cpp>`_

Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: auto

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://0.0.0.0:5000
   -op,Session Operation (create|get|delete),,-op create
   -image,Input image filename where image is stored in AIAA Session,,-image image.nii.gz
   -expiry,Session expiry in seconds,,-expiry 3600
   -session,Session ID incase of get or delete operation,,-session "9ad970be-530e-11ea-84e3-0242ac110007"

Example

.. code-block:: bash

   nvidiaAIAASession \
      -server http://0.0.0.0:5000 \
      -op create \
      -image _image.nii.gz

   nvidiaAIAASession \
      -server http://0.0.0.0:5000 \
      -op delete \
      -session "9ad970be-530e-11ea-84e3-0242ac110007"

DEXTR3D (Annotation)
--------------------

Provides implementation for ``nvidia::aiaa::Client::dextra3d()`` API.
For more details refer `aiaa-dextra3d.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-dextra3d.cpp>`_
 
Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: auto

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://0.0.0.0:5000
   -label,Label Name for matching;  Either -model or -label is required,,-label liver
   -model,Model Name,,-model Dextr3DLiver
   -points,"JSON Array of 3D Points (Image Indices) in [[x,y,z]+] format",,"-points [[70,172,86],...,[105,161,180]]"
   -image,Input image filename where image is stored in 3D format,,-image image.nii.gz
   -pixel,Input Image Pixel Type,short,short, unsigned int etc..
   -output,File name to store 3D binary mask image result from AIAA server,,-output result.nii.gz
   -pad,Padding size for input Image,20,-pad 20
   -roi,ROI Image size in XxYxZ format which is used while training the AIAA Model,128x128x128,-roi 96x96x96
   -sigma,Sigma Value for AIAA Server,3,-sigma 3
   -session,Session ID instead of -image option,,-session "9ad970be-530e-11ea-84e3-0242ac110007"

Example

.. code-block:: bash

   nvidiaAIAADEXTR3D \
      -server http://0.0.0.0:5000 \
      -label spleen \
      -points `cat ../test/data/pointset.json` \
      -image _image.nii.gz \
      -output tmp_out.nii.gz \
      -pad 20 \
      -roi 128x128x128 \
      -sigma 3
 
   #(using model instead of label)
 
   nvidiaAIAADEXTR3D \
      -server http://0.0.0.0:5000 \
      -model annotation_ct_spleen \
      -points `cat ../test/data/pointset.json` \
      -image _image.nii.gz \
      -output tmp_out.nii.gz \
      -pad 20 \
      -roi 128x128x128 \
      -sigma 3


Segmentation
------------

Provides implementation for ``nvidia::aiaa::Client::segmentation()`` API.
For more details refer `aiaa-segmentation.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-segmentation.cpp>`_
 
Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: auto

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://0.0.0.0:5000
   -label,Label Name for matching;  Either -model or -label is required,,-label liver
   -model,Model Name,,-model segmentation_ct_spleen
   -image,Input image filename where image is stored in 3D format,,-image image.nii.gz
   -output,File name to store 3D binary mask image result from AIAA server,,-output result.nii.gz
   -session,Session ID instead of -image option,,-session "9ad970be-530e-11ea-84e3-0242ac110007"

Example

.. code-block:: bash

   nvidiaAIAASegmentation \
      -server http://0.0.0.0:5000 \
      -label spleen \
      -image _image.nii.gz \
      -output tmp_out.nii.gz
 
   #(using model instead of label)
 
   nvidiaAIAASegmentation \
      -server http://0.0.0.0:5000 \
      -model segmentation_spleen \
      -image _image.nii.gz \
      -output tmp_out.nii.gz


Deepgrow
------------

Provides implementation for ``nvidia::aiaa::Client::deepgrow()`` API.
For more details refer `aiaa-deepgrow.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-deepgrow.cpp>`_

Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: auto

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://0.0.0.0:5000
   -model,Model Name,,-model clara_deepgrow
   -image,Input image filename where image is stored in 3D format,,-image image.nii.gz
   -fpoints,"Foreground Points or clicks in [x,y,z] format",,"-fpoints [[285,207,105]]"
   -bpoints,"Background Points or clicks in [x,y,z] format",,"-fpoints [[283,204,105]]"
   -output,File name to store 3D binary mask image result from AIAA server,,-output result.nii.gz
   -session,Session ID instead of -image option,,-session "9ad970be-530e-11ea-84e3-0242ac110007"

Example

.. code-block:: bash

   nvidiaAIAADeepgrow \
      -server http://0.0.0.0:5000 \
      -model clara_deepgrow \
      -image _image.nii.gz \
      -fpoints [[283,204,105]] \
      -output tmp_out.nii.gz

   nvidiaAIAADeepgrow \
      -server http://0.0.0.0:5000 \
      -model clara_deepgrow \
      -session "9ad970be-530e-11ea-84e3-0242ac110007" \
      -fpoints [[283,204,105]] \
      -output tmp_out.nii.gz

Mask To Polygon
------------------

Provides implementation for ``nvidia::aiaa::Client::mask2Polygon()`` API.
For more details refer `aiaa-mask-polygon.cpp <https://github.com/NVIDIA/ai-assisted-annotation-client/blob/master/src/cpp-client/tools/aiaa/aiaa-mask-polygon.cpp>`_
 
Following are the options available

.. csv-table::
   :header: Option,Description,Default,Example
   :widths: auto

   -h,Prints the help information,,
   -server,Server URI for AIAA Server,,-server http://0.0.0.0:5000
   -ratio,Point Ratio,10,-ratio 10
   -input,Input 3D binary mask image file name (which is an output of dextra3d),,-input tmp_out.nii.gz
   -output,Save output result (JSON Array) representing the list of polygons per slice to a file,,-output polygonlist.json

Example

.. code-block:: bash

   nvidiaAIAAMaskPolygon \
      -server http://0.0.0.0:5000 \
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
   -server,Server URI for AIAA Server,,-server http://0.0.0.0:5000
   -neighbor,Neighborhood size for propagation,1,-neighbor 1
   -neighbor3d,3DNeighborhood size for propagation,1,-neighbor3d 1
   -dim,Dimension 2D or 3D,2,-dim 2
   -poly,"Current or Old 2D Polygon Array in [[[x,y]+]] format",,"-poly [[[53,162],…,[62,140]]]"
   -sindex,Slice Index in-case of 3D volume which needs to be updated,,-sindex 0
   -pindex,Polygon Index within new Polygon Array which needs to be updated,,-pindex 0
   -vindex,Vertical Index within new Polygon Array which needs to be updated,,-vindex 17
   -xoffset,X Offset needs to be added to get new vertex value,,-xoffset 2
   -yoffset,Y Offset needs to be added to get new vertex value,,-yoffset -4
   -image,Input 2D/3D image,,-image image_slice_2D.png
   -output,Output file name to the updated image,,-output updated_image_2D.png

Example

.. code-block:: bash

   nvidiaAIAAFixPolygon \
      -server http://0.0.0.0:5000 \
      -dim 2
      -neighbor 1 \
      -poly `cat ../test/data/polygons.json` \
      -pindex 0 \
      -vindex 17 \
      -xoffset 8 \
      -yoffset 5 \
      -image ../test/data/image_slice_2D.png \
      -output updated_image_2D.png

   nvidiaAIAAFixPolygon \
      -server http://0.0.0.0:5000 \
      -dim 3
      -neighbor 1 \
      -neighbor3d 1 \
      -poly `cat ../test/data/polygons3d.json` \
      -sindex 104 \
      -pindex 0 \
      -vindex 4 \
      -xoffset 8 \
      -yoffset 5 \
      -image _image.nii.gz \
      -output image_mask.nii.gz

