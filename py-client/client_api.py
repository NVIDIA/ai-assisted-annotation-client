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

import os
import requests
import json
import re
import numpy as np
import nibabel as nib

from skimage.transform import resize
from skimage.measure import label, regionprops

from requests_toolbelt.multipart import decoder

class AIAAClient:

    """
    The AIAAClient object is constructed with the server information

    @param IP address, port, and version 
    """

    def __init__(self, server_ip, server_port, api_version='v1'):
        self.server_ip = server_ip
        self.server_port = server_port 
        self.api_version = api_version
        
    """
    Compose http message and get response

    @param args_dict for method selection, parameters, and files 
    @return response from server
    """
    def _call_server(self, api, args_dict=None, params=None, files=None):
        endpoint = os.path.join("http://"+self.server_ip+":"+self.server_port, self.api_version, api)
        url = endpoint
        if args_dict:
            sep = '?'
            for key, value in args_dict.items():
                url = url + sep + key + "=" + value
                sep = '&'
                    
        data = None

        print("Connecting to: ",url)
        
        if not params:
            response = requests.get(url)
        else:
            data = {'params': params}
            print(data)
            if not files:
                response = requests.post(url, data=data)
            else:
                response = requests.post(url, data=data, files=files)

        return response

    """
    Get the current supported model list

    @param none
    @return json containing current supported object names
    along with their corresponding object model names for DEXTR3D
    """
    def model_list(self,result_file_prefix):
        response = self._call_server("models")
        file_names = _process_response(response, result_file_prefix)
        print(file_names)
        return(file_names)


    """
    3D image segmentation using DEXTR3D method

    @param object model name, according to the output of model_list()
    temporary folder path, needed for http request/response
    point set: json containing the extreme points' indices 
    input 3D image file name
    output 3D binary mask image file name 
    pad padding size (default is 20)
    roi_size image rezize value (default is 128x128x128) 
    sigma value to be used for sigma (default is 3) 
    @return no explicit return
    output 3D binary mask will be saved to the specified file
    """
    def dextr3d(self, model_name, temp_folder_path, image_file_path, out_image_file_path, point_set, pad = 20, roi_size = '128x128x128', sigma = 3):

        # Extract extreme points information
        points = json.loads(point_set)
        points = points.get('points')
        pts_str = points.replace('[[','')
        pts_str = pts_str.replace(']]','')
        pts_str = pts_str.split('],[')
        points = np.zeros((6,3),dtype=np.int)
        for i, pt_str in enumerate(pts_str):
            points[i,::] = tuple(map(int, pt_str.split(',')))

        assert(np.shape(points)[0]>=6) # at least six points
        assert(np.shape(points)[1]==3) # 3D points       
        
        # Assign temporary file names
        tmp_image_file_path = temp_folder_path + "image.nii.gz"
        result_prefix = temp_folder_path + "dextr3D"

        # Read image
        image = nib.load(image_file_path)
        affine = image.affine
        image = image.dataobj
        orig_size = np.shape(image)
        spacing = [abs(affine[(0,0)]),abs(affine[(1,1)]),abs(affine[(2,2)])]

        # Preprocess - crop and resize
        points, crop = _image_pre_process( image, tmp_image_file_path, points, pad, roi_size, spacing)

        # Compose request message
        json_data = {}
        json_data['points'] = str(points.tolist())
        json_data['sigma'] = sigma

        files = _make_files(tmp_image_file_path)
        params = json.dumps(json_data)
        response = self._call_server("dextr3d", {"model": model_name}, params, files)

        # Process response
        file_names = _process_response(response, result_prefix)
        print(file_names)

        # Postprocess - recover resize and crop
        for image_name in file_names:
            if image_name.find(".nii.gz") != -1:
                roi_image_file_path = image_name

        result = _image_post_processing(roi_image_file_path, crop, orig_size)
        nib.save(nib.Nifti1Image(result.astype(np.uint8), affine), out_image_file_path)

        file_names.append(out_image_file_path)
        return(file_names)


    """
    3D binary mask to polygon representation conversion

    @param point ratio controlling how many polygon vertices will be generated 
    input 3D binary mask image file name
    @return a json containing the indices of all polygon vertices slice by slice.
    """

    def mask2polygon(self, image_file_path, result_file_prefix, pointRatio):
        json_data = {}
        json_data['more_points'] = pointRatio
        params = json.dumps(json_data)

        files = _make_files(image_file_path)

        response = self._call_server("mask2polygon", None, params, files)

        file_names = _process_response(response, result_file_prefix)
        print(file_names)
        return(file_names)


    """
    2D polygon update with single point edit

    @param json containing parameters of polygon editing: 
    [Neighborhood size 
    Index of the changed polygon 
    Index of the changed vertex
    Polygon before editing
    Polygon after editing]
    input 2D image file name
    output 2D mask image file name
    @return json containing the indices of updated polygon vertices
    output binary mask will be saved to the specified name
    """
    def fixpolygon(self, image_file_path, result_file_prefix, params):
        files = _make_files(image_file_path)
        response = self._call_server("fixpolygon", None, params, files)
        return _process_response(response, result_file_prefix)

"""
Image pre-processing, crop ROI according to extreme points and resample to 128^3 

@param json containing the extreme points' indices 
input 3D image file name
output temporary 3D ROI image file name
pad size
roi size
@return json containing the extreme points' indices after cropping and resizing
temporary cropped and resize image
crop location for later recovery
"""    
def _image_pre_process( image, tmp_image_file_path, point_set, pad, roi_size, spacing):
    target_size = tuple(map(int, roi_size.split('x')))

    image_size = np.shape(image)
    points = np.asanyarray(point_set)

    # get bounding box
    x1 = max(0, np.min(points[::,0])-int(pad/spacing[0]))
    x2 = min(image_size[0],np.max(points[::,0]) + int(pad/spacing[0]))
    y1 = max(0, np.min(points[::,1])-int(pad/spacing[1]))
    y2 = min(image_size[1],np.max(points[::,1]) + int(pad/spacing[1]))
    z1 = max(0, np.min(points[::,2])-int(pad/spacing[2]))
    z2 = min(image_size[2],np.max(points[::,2]) + int(pad/spacing[2]))
    crop = [[x1,x2],[y1,y2],[z1,z2]]

    # crop
    points[::,0] = points[::,0]-x1
    points[::,1] = points[::,1]-y1
    points[::,2] = points[::,2]-z1
    image = image[x1:x2,y1:y2,z1:z2]
    cropped_size = np.shape(image)
    
    # resize
    ratio = np.divide(np.asanyarray(target_size,dtype=np.float),np.asanyarray(cropped_size,dtype=np.float))
    image = resize(image,target_size,preserve_range=True,order=1) # linear interpolation
    points[::,0] = points[::,0]*ratio[0] 
    points[::,1] = points[::,1]*ratio[1]
    points[::,2] = points[::,2]*ratio[2]

    nib.save(nib.Nifti1Image(image,np.eye(4)), tmp_image_file_path)
                    
    return points, crop



"""
Image post-processing, recover the cropping and resizing 

@param 3D temporary ROI binary mask image file name
output 3D binary mask image file name
@return no explicit return
output 3D binary mask will be saved to the specified file
"""
def _image_post_processing(roi_image_file_path, crop, orig_size):
    result = np.zeros(orig_size,np.uint8)

    roi_result = nib.load(roi_image_file_path)
    roi_result = roi_result.dataobj

    orig_crop_size = [crop[0][1]-crop[0][0], \
                      crop[1][1]-crop[1][0], \
                      crop[2][1]-crop[2][0]]

    resize_image = resize(roi_result,orig_crop_size,order=0);

    result[crop[0][0]:crop[0][1], \
           crop[1][0]:crop[1][1], \
           crop[2][0]:crop[2][1]] = resize_image

    labels = label(result)

    largest_area = 0
    for r in regionprops(result):
        if(r.area>largest_area):
            largest_area = r.area
            largest_label = r.label

    largestCC = labels == largest_label

    return largestCC



"""
Load file for http request

@param image file name
@return data for binary transmission
"""
def _make_files(image_file_path):
    return [('datapoint', (image_file_path, open(image_file_path, 'rb'), 'application/octet-stream'))]



"""
Parse http response

@param http response
result saving path
@return saved file path
"""
def _process_response(r, result_prefix):
    response_info = r.headers['Content-Type']
    print(response_info)

    files = []
    if response_info.find("json") != -1:
        # save json
        result_path = result_prefix + '.json'
        with open(result_path, 'wb') as f:
            f.write(r.content)
            print('Write returned json to', result_path)
            files.append(result_path)
    elif response_info.find("multipart") != -1:
        # parse multipart data
        multipart_data = decoder.MultipartDecoder.from_response(r)
        for part in multipart_data.parts:
            print(part.headers)

            header = part.headers[b'Content-Disposition']

            header = header.decode('utf-8')
            headerInfo = header.split(";")

            if headerInfo[0]!='form-data':
                continue

            name_header = re.findall("name=\"(.+)\"|$", headerInfo[1])[0]

            result_path = ""

            if name_header == 'points':
                result_path = result_prefix + '_points.txt'
            elif name_header == 'results':
                result_path = result_prefix + '.json'
            elif name_header == "file":
                filename_header = re.findall("name=\"(.+)\"|$", headerInfo[2])[0]
                result_path = result_prefix + "_" + filename_header

            if result_path:
                print("Write returned file to ",result_path)
                with open(result_path, 'wb') as f:
                    f.write(part.content)
                    files.append(result_path)

    return files



