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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import cgi

try:
    # Python3
    # noinspection PyUnresolvedReferences
    import http.client as httplib
except ModuleNotFoundError as e:
    # Python2
    # noinspection PyUnresolvedReferences
    import httplib

import json
import logging
import mimetypes
import os
import sys
import tempfile
import urllib

import SimpleITK
import numpy as np


class AIAAClient:
    """
    The AIAAClient object is constructed with the server information

    :param server_ip: AIAA Server IP address
    :param server_port: AIAA Server Port
    :param api_version: AIAA Serverversion
    """

    def __init__(self, server_ip, server_port, api_version='v1'):
        self.server_url = server_ip + ':' + str(server_port)
        self.api_version = api_version

    def model_list(self, label=None):
        """
        Get the current supported model list
        :param label: Filter models which are matching the label
        :return: returns json containing list of models and details
        """
        logger = logging.getLogger(__name__)
        logger.debug('Fetching Model Details')

        selector = '/' + self.api_version + '/models'
        if label is not None and len(label) > 0:
            selector += '?label=' + AIAAUtils.urllib_quote_plus(label)
        return json.loads(AIAAUtils.get_method(self.server_url, selector))

    def segmentation(self, model, image_in, image_out):
        """
        3D image segmentation using segmentation method
        :param model: model name according to the output of model_list()
        :param image_in: input 3D image file name
        :param image_out: output files will be stored

        Output 3D binary mask will be saved to the specified file
        """
        logger = logging.getLogger(__name__)
        logger.debug('Preparing for Segmentation Action')

        selector = '/' + self.api_version + '/segmentation?model=' + AIAAUtils.urllib_quote_plus(model)
        fields = {'params': '{}'}
        files = {'datapoint': image_in}

        logger.debug('Using Selector: {}'.format(selector))
        logger.debug('Using Fields: {}'.format(fields))
        logger.debug('Using Files: {}'.format(files))

        form, files = AIAAUtils.post_multipart(self.server_url, selector, fields, files)
        AIAAUtils.save_result(files, image_out)
        return form

    def dextr3d(self, model, point_set, image_in, image_out, pad=20, roi_size='128x128x128'):
        """
        3D image segmentation using DEXTR3D method
        :param model: model name according to the output of model_list()
        :param point_set: point set json containing the extreme points' indices
        :param image_in: input 3D image file name
        :param image_out: output files will be stored
        :param pad: padding size (default is 20)
        :param roi_size:  image resize value (default is 128x128x128)

        Output 3D binary mask will be saved to the specified file
        """
        logger = logging.getLogger(__name__)
        logger.debug('Preparing for Annotation/Dextr3D Action')

        # Pre Process
        cropped_file = tempfile.NamedTemporaryFile(suffix='.nii.gz').name
        points, crop = AIAAUtils.image_pre_process(image_in, cropped_file, point_set, pad, roi_size)

        # Dextr3D
        selector = '/' + self.api_version + '/dextr3d?model=' + AIAAUtils.urllib_quote_plus(model)
        params = dict()
        params['points'] = json.dumps(points)

        fields = {'params': json.dumps(params)}
        files = {'datapoint': cropped_file}

        logger.debug('Using Selector: {}'.format(selector))
        logger.debug('Using Fields: {}'.format(fields))
        logger.debug('Using Files: {}'.format(files))

        form, files = AIAAUtils.post_multipart(self.server_url, selector, fields, files)

        # Post Process
        if len(files) > 0:
            cropped_out_file = tempfile.NamedTemporaryFile(suffix='.nii.gz').name
            AIAAUtils.save_result(files, cropped_out_file)
            AIAAUtils.image_post_processing(cropped_out_file, image_out, crop, image_in)
            os.unlink(cropped_out_file)

        os.unlink(cropped_file)
        return form

    def mask2polygon(self, image_in, point_ratio):
        """
        3D binary mask to polygon representation conversion
    
        :param image_in: input 3D binary mask image file name
        :param point_ratio: point ratio controlling how many polygon vertices will be generated

        :return: A json containing the indices of all polygon vertices slice by slice.
        """
        logger = logging.getLogger(__name__)
        logger.debug('Preparing for Mask2Polygon Action')

        selector = '/' + self.api_version + '/mask2polygon'
        params = dict()
        params['more_points'] = point_ratio

        fields = {'params': json.dumps(params)}
        files = {'datapoint': image_in}

        response = AIAAUtils.post_multipart(self.server_url, selector, fields, files, False)
        return json.loads(response)

    def fixpolygon(self, image_in, image_out, polygons, index, vertex_offset, propagate_neighbor):
        """
        2D/3D polygon update with single point edit
    
        :param image_in: input 2D/3D image file name
        :param image_out: output 2D/3D mask image file name
        :param polygons: list of polygons 2D/3D
        :param index: index of vertex which needs to be updated
                      1) for 2D [polygon_index, vertex_index]
                      2) for 3D [slice_index, polygon_index, vertex_index]
        :param vertex_offset: offset (2D/3D) needs to be added to get the updated vertex in [x,y] format
        :param propagate_neighbor: neighborhood size
                      1) for 2D: single value (polygon_neighborhood_size)
                      2) for 3D: [slice_neighborhood_size, polygon_neighborhood_size]
        :return: A json containing the indices of updated polygon vertices


        Output binary mask will be saved to the specified name
        """
        logger = logging.getLogger(__name__)
        logger.debug('Preparing for FixPolygon Action')

        selector = '/' + self.api_version + '/fixpolygon'

        dimension = len(index)

        params = dict()
        params['prev_poly'] = polygons
        params['vertex_offset'] = vertex_offset
        params['dimension'] = dimension

        if dimension == 3:
            params['sliceIndex'] = index[0]
            params['polygonIndex'] = index[1]
            params['vertexIndex'] = index[2]
            params['propagate_neighbor_3d'] = propagate_neighbor[0]
            params['propagate_neighbor'] = propagate_neighbor[1]
        else:
            params['polygonIndex'] = index[0]
            params['vertexIndex'] = index[1]
            params['propagate_neighbor'] = propagate_neighbor

        fields = {'params': json.dumps(params)}
        files = {'datapoint': image_in}

        form, files = AIAAUtils.post_multipart(self.server_url, selector, fields, files)
        AIAAUtils.save_result(files, image_out)
        return form


class AIAAUtils:
    @staticmethod
    def resample_image(itk_image, out_size, linear):
        spacing = list(itk_image.GetSpacing())
        size = list(itk_image.GetSize())

        out_spacing = []
        for i in range(len(size)):
            out_spacing.append(float(spacing[i]) * float(size[i]) / float(out_size[i]))

        resample = SimpleITK.ResampleImageFilter()
        resample.SetOutputSpacing(out_spacing)
        resample.SetSize(out_size)
        resample.SetOutputDirection(itk_image.GetDirection())
        resample.SetOutputOrigin(itk_image.GetOrigin())

        if linear:
            resample.SetInterpolator(SimpleITK.sitkLinear)
        else:
            resample.SetInterpolator(SimpleITK.sitkNearestNeighbor)
        return resample.Execute(itk_image)

    @staticmethod
    def image_pre_process(input_file, output_file, point_set, pad, roi_size):
        logger = logging.getLogger(__name__)

        itk_image = SimpleITK.ReadImage(input_file)
        spacing = itk_image.GetSpacing()
        image_size = itk_image.GetSize()

        target_size = tuple(map(int, roi_size.split('x')))
        points = np.asanyarray(np.array(point_set).astype(int))

        logger.debug('Image Size: {}'.format(image_size))
        logger.debug('Image Spacing: {}'.format(spacing))
        logger.debug('Target Size: {}'.format(target_size))
        logger.debug('Input Points: {}'.format(json.dumps(points.tolist())))

        index_min = [sys.maxsize, sys.maxsize, sys.maxsize]
        index_max = [0, 0, 0]
        vx_pad = [0, 0, 0]
        for point in points:
            for i in range(3):
                vx_pad[i] = int((pad / spacing[i]) if spacing[i] > 0 else pad)
                index_min[i] = min(max(int(point[i] - vx_pad[i]), 0), int(index_min[i]))
                index_max[i] = max(min(int(point[i] + vx_pad[i]), int(image_size[i] - 1)), int(index_max[i]))

        logger.debug('Voxel Padding: {}'.format(vx_pad))
        logger.debug('Min Index: {}'.format(index_min))
        logger.debug('Max Index: {}'.format(index_max))

        crop_index = [0, 0, 0]
        crop_size = [0, 0, 0]
        crop = []
        for i in range(3):
            crop_index[i] = index_min[i]
            crop_size[i] = index_max[i] - index_min[i]
            crop.append([crop_index[i], crop_index[i] + crop_size[i]])
        logger.debug('crop_index: {}'.format(crop_index))
        logger.debug('crop_size: {}'.format(crop_size))
        logger.debug('crop: {}'.format(crop))

        # get bounding box
        x1 = crop[0][0]
        x2 = crop[0][1]
        y1 = crop[1][0]
        y2 = crop[1][1]
        z1 = crop[2][0]
        z2 = crop[2][1]

        # crop
        points[::, 0] = points[::, 0] - x1
        points[::, 1] = points[::, 1] - y1
        points[::, 2] = points[::, 2] - z1

        cropped_image = itk_image[x1:x2, y1:y2, z1:z2]
        cropped_size = cropped_image.GetSize()
        logger.debug('Cropped size: {}'.format(cropped_size))

        # resize
        out_image = AIAAUtils.resample_image(cropped_image, target_size, True)
        logger.debug('Cropped Image Size: {}'.format(out_image.GetSize()))
        SimpleITK.WriteImage(out_image, output_file, True)

        # pointsROI
        ratio = np.divide(np.asanyarray(target_size, dtype=np.float), np.asanyarray(cropped_size, dtype=np.float))
        points[::, 0] = points[::, 0] * ratio[0]
        points[::, 1] = points[::, 1] * ratio[1]
        points[::, 2] = points[::, 2] * ratio[2]
        return points.astype(int).tolist(), crop

    @staticmethod
    def image_post_processing(input_file, output_file, crop, orig_file):
        itk_image = SimpleITK.ReadImage(input_file)
        orig_crop_size = [crop[0][1] - crop[0][0], crop[1][1] - crop[1][0], crop[2][1] - crop[2][0]]
        resize_image = AIAAUtils.resample_image(itk_image, orig_crop_size, False)

        orig_image = SimpleITK.ReadImage(orig_file)
        orig_size = orig_image.GetSize()

        image = SimpleITK.GetArrayFromImage(resize_image)
        result = np.zeros(orig_size[::-1], np.uint8)
        result[crop[2][0]:crop[2][1], crop[1][0]:crop[1][1], crop[0][0]:crop[0][1]] = image

        itk_result = SimpleITK.GetImageFromArray(result)
        itk_result.SetDirection(orig_image.GetDirection())
        itk_result.SetSpacing(orig_image.GetSpacing())
        itk_result.SetOrigin(orig_image.GetOrigin())

        SimpleITK.WriteImage(itk_result, output_file, True)

    @staticmethod
    def get_method(server_url, selector):
        logger = logging.getLogger(__name__)
        logger.debug('Using Selector: {}'.format(selector))

        conn = httplib.HTTPConnection(server_url)
        conn.request('GET', selector)
        response = conn.getresponse()
        return response.read()

    @staticmethod
    def post_multipart(server_url, selector, fields, files, multipart_response=True):
        logger = logging.getLogger(__name__)
        logger.debug('Using Selector: {}'.format(selector))

        content_type, body = AIAAUtils.encode_multipart_formdata(fields, files)
        conn = httplib.HTTPConnection(server_url)
        headers = {'content-type': content_type, 'content-length': str(len(body))}
        conn.request('POST', selector, body, headers)

        response = conn.getresponse()
        logger.debug('Error Code: {}'.format(response.status))
        logger.debug('Error Message: {}'.format(response.reason))
        logger.debug('Headers: {}'.format(response.getheaders()))

        if multipart_response:
            form, files = AIAAUtils.parse_multipart(response, response.msg)
            logger.debug('Response FORM: {}'.format(form))
            logger.debug('Response FILES: {}'.format(files.keys()))
            return form, files
        return response.read()

    @staticmethod
    def save_result(files, result_file):
        logger = logging.getLogger(__name__)
        if len(files) > 0:
            for name in files:
                data = files[name]
                logger.debug('Saving {} to {}; Size: {}'.format(name, result_file, len(data)))
                with open(result_file, "wb") as f:
                    f.write(data.encode('utf-8') if isinstance(data, str) else data)
                    f.close()
                break

    @staticmethod
    def encode_multipart_formdata(fields, files):
        limit = '----------lImIt_of_THE_fIle_eW_$'
        lines = []
        for (key, value) in fields.items():
            lines.append('--' + limit)
            lines.append('Content-Disposition: form-data; name="%s"' % key)
            lines.append('')
            lines.append(value)
        for (key, filename) in files.items():
            lines.append('--' + limit)
            lines.append('Content-Disposition: form-data; name="%s"; filename="%s"' % (key, filename))
            lines.append('Content-Type: %s' % AIAAUtils.get_content_type(filename))
            lines.append('')
            with open(filename, mode='rb') as f:
                data = f.read()
                lines.append(data)
        lines.append('--' + limit + '--')
        lines.append('')

        blines = []
        for l in lines:
            blines.append(l.encode('utf-8') if isinstance(l, str) else l)
        body = b'\r\n'.join(blines)

        content_type = 'multipart/form-data; boundary=%s' % limit
        return content_type, body

    @staticmethod
    def get_content_type(filename):
        return mimetypes.guess_type(filename)[0] or 'application/octet-stream'

    @staticmethod
    def parse_multipart(fp, headers):
        logger = logging.getLogger(__name__)
        fs = cgi.FieldStorage(
            fp=fp,
            environ={'REQUEST_METHOD': 'POST'},
            headers=headers,
            keep_blank_values=True
        )
        form = {}
        files = {}
        for f in fs.list:
            logger.debug('FILE-NAME: {}; NAME: {}; SIZE: {}'.format(f.filename, f.name, len(f.value)))
            if f.filename:
                files[f.filename] = f.value
            else:
                form[f.name] = f.value
        return form, files

    # noinspection PyUnresolvedReferences
    @staticmethod
    def urllib_quote_plus(s):
        try:
            # Python3
            return urllib.parse.quote_plus(s)
        except AttributeError:
            # Python2
            return urllib.quote_plus(s)
