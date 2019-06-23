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

import json
import os
import re

import nibabel as nib
import numpy as np
import requests
import skimage.measure as skm
from requests_toolbelt.multipart import decoder
from skimage.transform import resize


class AIAAClient:
    """
    The AIAAClient object is constructed with the server information

    :param server_ip: AIAA Server IP address
    :param server_port: AIAA Server Port
    :param api_version: AIAA Serverversion
    """

    def __init__(self, server_ip, server_port, api_version='v1'):
        self.server_ip = server_ip
        self.server_port = server_port
        self.api_version = api_version

    def _call_server(self, api, args_dict=None, params=None, files=None):
        """
        Compose http message and get response
    
        :param args_dict: for method selection
        :param params: parameters
        :param files: files to be sent to AIAA Server
        
        :return: It returns response from AIAA Server 
        """

        endpoint = os.path.join("http://" + self.server_ip + ":" + self.server_port, self.api_version, api)
        url = endpoint
        if args_dict:
            sep = '?'
            for key, value in args_dict.items():
                url = url + sep + key + "=" + value
                sep = '&'

        print("Connecting to: ", url)

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

    def model_list(self, result_file_prefix):
        """
        Get the current supported model list
        :param result_file_prefix: File name to store the result
        :return: returns json containing list of models and details supported for DEXTR3D
        """

        response = self._call_server("models")
        file_names = _process_response(response, result_file_prefix)
        print(file_names)
        return file_names

    # noinspection PyUnresolvedReferences
    def dextr3d(self, model_name, image_file_path, result_file_prefix, point_set, pad=20,
                roi_size='128x128x128', sigma=3):
        """
        3D image segmentation using DEXTR3D method

        :param model_name: model name according to the output of model_list()
        :param image_file_path: input 3D image file name
        :param point_set: point set json containing the extreme points' indices
        :param result_file_prefix: output files will be stored under this prefix
        :param pad: padding size (default is 20)
        :param roi_size:  image resize value (default is 128x128x128)
        :param sigma: sigma value to be used for sigma (default is 3)

        Output 3D binary mask will be saved to the specified file
        """

        # Extract extreme points information
        points = json.loads(point_set)
        points = points.get('points')
        pts_str = points.replace('[[', '')
        pts_str = pts_str.replace(']]', '')
        pts_str = pts_str.split('],[')
        points = np.zeros((6, 3), dtype=np.int)
        for i, pt_str in enumerate(pts_str):
            points[i, ::] = tuple(map(int, pt_str.split(',')))

        assert (np.shape(points)[0] >= 6)  # at least six points
        assert (np.shape(points)[1] == 3)  # 3D points

        # Assign temporary file names
        tmp_image_file_path = result_file_prefix + "_cropped_input_image.nii.gz"

        # Read image
        image = nib.load(image_file_path)
        affine = image.affine
        image = image.dataobj
        orig_size = np.shape(image)
        spacing = [abs(affine[(0, 0)]), abs(affine[(1, 1)]), abs(affine[(2, 2)])]

        # Preprocess - crop and resize
        points, crop = _image_pre_process(image, tmp_image_file_path, points, pad, roi_size, spacing)

        # Compose request message
        json_data = {'points': str(points.tolist()), 'sigma': sigma}

        files = _make_files(tmp_image_file_path)
        params = json.dumps(json_data)
        response = self._call_server("dextr3d", {"model": model_name}, params, files)

        # Process response
        file_names = _process_response(response, result_file_prefix)
        print(file_names)

        # Postprocess - recover resize and crop
        out_image_file_path = result_file_prefix + "_resized_output_image.nii.gz"
        roi_image_file_path = None
        for image_name in file_names:
            if image_name.find("_aas_result.nii.gz") != -1:
                roi_image_file_path = image_name

        result = _image_post_processing(roi_image_file_path, crop, orig_size)
        nib.save(nib.Nifti1Image(result.astype(np.uint8), affine), out_image_file_path)

        file_names.append(out_image_file_path)
        return file_names

    def segmentation(self, model_name, image_file_path, result_file_prefix, params):
        """
        3D image segmentation method
    
        :param model_name: model name according to the output of model_list()
        :param image_file_path: input 3D image file name
        :param result_file_prefix: result file from AIAA Server
        :param params: additional params if applicable for segmentation

        Output 3D binary mask and extreme points will be saved into files with result_file_prefix
        """

        # Compose request message
        files = _make_files(image_file_path)
        response = self._call_server("segmentation", {"model": model_name}, params, files)

        # Process response
        file_names = _process_response(response, result_file_prefix)
        print(file_names)

        return file_names

    def mask2polygon(self, image_file_path, result_file_prefix, point_ratio):
        """
        3D binary mask to polygon representation conversion
    
        :param image_file_path: input 3D binary mask image file name
        :param result_file_prefix: result file from AIAA Server
        :param point_ratio: point ratio controlling how many polygon vertices will be generated

        :return: A json containing the indices of all polygon vertices slice by slice.
        """

        json_data = {'more_points': point_ratio}
        params = json.dumps(json_data)

        files = _make_files(image_file_path)

        response = self._call_server("mask2polygon", None, params, files)

        file_names = _process_response(response, result_file_prefix)
        print(file_names)
        return file_names

    def fixpolygon(self, image_file_path, result_file_prefix, params):
        """
        2D polygon update with single point edit
    
        :param image_file_path: input 2D image file name
        :param result_file_prefix: output 2D mask image file name
        :param params: json containing parameters of polygon editing
        :return: A json containing the indices of updated polygon vertices
        
        json parameters will have
            1. Neighborhood size 
            2. Index of the changed polygon 
            3. Index of the changed vertex
            4. Polygon before editing
            5. Polygon after editing


        Output binary mask will be saved to the specified name
        """

        files = _make_files(image_file_path)
        response = self._call_server("fixpolygon", None, params, files)
        return _process_response(response, result_file_prefix)


def _image_pre_process(image, tmp_image_file_path, point_set, pad, roi_size, spacing):
    """
    Image pre-processing, crop ROI according to extreme points and resample to 128^3 
    
    :param point_set: json containing the extreme points' indices 
    :param image: input 3D image file name
    :param tmp_image_file_path: output temporary 3D ROI image file name
    :param pad: padding size
    :param roi_size: image resize value
    :return: json containing the extreme points' indices after cropping and resizing
    """

    target_size = tuple(map(int, roi_size.split('x')))

    image_size = np.shape(image)
    points = np.asanyarray(point_set)

    # get bounding box
    x1 = max(0, np.min(points[::, 0]) - int(pad / spacing[0]))
    x2 = min(image_size[0], np.max(points[::, 0]) + int(pad / spacing[0]))
    y1 = max(0, np.min(points[::, 1]) - int(pad / spacing[1]))
    y2 = min(image_size[1], np.max(points[::, 1]) + int(pad / spacing[1]))
    z1 = max(0, np.min(points[::, 2]) - int(pad / spacing[2]))
    z2 = min(image_size[2], np.max(points[::, 2]) + int(pad / spacing[2]))
    crop = [[x1, x2], [y1, y2], [z1, z2]]

    # crop
    points[::, 0] = points[::, 0] - x1
    points[::, 1] = points[::, 1] - y1
    points[::, 2] = points[::, 2] - z1
    image = image[x1:x2, y1:y2, z1:z2]
    cropped_size = np.shape(image)

    # resize
    ratio = np.divide(np.asanyarray(target_size, dtype=np.float), np.asanyarray(cropped_size, dtype=np.float))
    image = resize(image, target_size, preserve_range=True, order=1)  # linear interpolation
    points[::, 0] = points[::, 0] * ratio[0]
    points[::, 1] = points[::, 1] * ratio[1]
    points[::, 2] = points[::, 2] * ratio[2]

    nib.save(nib.Nifti1Image(image, np.eye(4)), tmp_image_file_path)

    return points, crop


def _image_post_processing(roi_image_file_path, crop, orig_size):
    """
    Image post-processing, recover the cropping and resizing 
    
    :param roi_image_file_path: 3D temporary ROI binary mask image file name
    :param crop: Crop Size output
    :param orig_size: Original Image Size
    
    :return: Recovered 3D binary mask
    """
    result = np.zeros(orig_size, np.uint8)

    roi_result = nib.load(roi_image_file_path)
    roi_result = roi_result.dataobj

    orig_crop_size = [crop[0][1] - crop[0][0],
                      crop[1][1] - crop[1][0],
                      crop[2][1] - crop[2][0]]

    resize_image = resize(roi_result, orig_crop_size, order=0)

    result[crop[0][0]:crop[0][1], crop[1][0]:crop[1][1], crop[2][0]:crop[2][1]] = resize_image
    return _get_largest_cc(result)


def _get_largest_cc(result):
    """
    Get Largest Connected Component
    """
    labels = skm.label(result)

    largest_label = None
    largest_area = 0
    for r in skm.regionprops(labels):
        if r.area > largest_area:
            largest_area = r.area
            largest_label = r.label

    largest_cc = labels == largest_label
    return largest_cc


def _make_files(image_file_path):
    """
    Load file for http request
    
    :param image_file_path: image file name
    :return: data for binary transmission
    """

    return [('datapoint', (image_file_path, open(image_file_path, 'rb'), 'application/octet-stream'))]


def _process_response(r, result_prefix):
    """
    Parse http response
    
    :param r: http response
    :param result_prefix: result saving path
    :return: saved file path
    """

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
            header_info = header.split(";")

            if header_info[0] != 'form-data':
                continue

            name_header = re.findall("name=\"(.+)\"|$", header_info[1])[0]

            result_path = ""

            if name_header == 'points':
                result_path = result_prefix + '_points.txt'
            elif name_header == 'results':
                result_path = result_prefix + '.json'
            elif name_header == "file":
                filename_header = re.findall("name=\"(.+)\"|$", header_info[2])[0]
                result_path = result_prefix + "_" + filename_header
                if filename_header.find(".nii.gz") != -1:
                    result_path = result_prefix + "_aas_result.nii.gz"
                if filename_header.find(".png") != -1:
                    result_path = result_prefix + "_aas_result.png"

            if result_path:
                print("Write returned file to ", result_path)
                with open(result_path, 'wb') as f:
                    f.write(part.content)
                    files.append(result_path)

    return files
