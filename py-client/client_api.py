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
import ssl

try:
    # Python3
    # noinspection PyUnresolvedReferences
    import http.client as httplib
    # noinspection PyUnresolvedReferences,PyCompatibility
    from urllib.parse import quote_plus
    # noinspection PyUnresolvedReferences,PyCompatibility
    from urllib.parse import urlparse
except ImportError as e:
    # Python2
    # noinspection PyUnresolvedReferences
    import httplib
    # noinspection PyUnresolvedReferences
    from urllib import quote_plus
    # noinspection PyUnresolvedReferences
    from urlparse import urlparse

import json
import logging
import mimetypes
import os
import sys
import tempfile

import SimpleITK
import numpy as np


class AIAAClient:
    """
    The AIAAClient object is constructed with the server information

    :param server_url: AIAA Server URL (example: 'http://0.0.0.0:5000')
    :param api_version: AIAA Server API version
    """

    def __init__(self, server_url='http://0.0.0.0:5000', api_version='v1'):
        self.server_url = server_url
        self.api_version = api_version

        self.doc_id = None

    def model(self, model):
        """
        Get the model details

        :param model: A valid Model Name which exists in AIAA Server
        :return: returns json containing the model details
        """
        logger = logging.getLogger(__name__)
        logger.debug('Fetching Model Details')

        selector = '/' + self.api_version + '/models'
        selector += '?model=' + AIAAUtils.urllib_quote_plus(model)

        response = AIAAUtils.http_get_method(self.server_url, selector)
        response = response.decode('utf-8') if isinstance(response, bytes) else response
        return json.loads(response)

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

        response = AIAAUtils.http_get_method(self.server_url, selector)
        response = response.decode('utf-8') if isinstance(response, bytes) else response
        return json.loads(response)

    def segmentation(self, model, image_in, image_out, save_doc=False):
        """
        2D/3D image segmentation using segmentation method

        :param model: model name according to the output of model_list()
        :param image_in: input 2D/3D image file name
        :param image_out: output mask will be stored
        :param save_doc: save input image in server for future reference; server will return doc id in result json
        :return: returns json containing extreme points for the segmentation mask and other info

        Output 2D/3D binary mask will be saved to the specified file
        """
        logger = logging.getLogger(__name__)
        logger.debug('Preparing for Segmentation Action')

        selector = '/' + self.api_version + '/segmentation?model=' + AIAAUtils.urllib_quote_plus(model)
        selector += '&save_doc=' + ('true' if save_doc else 'false')
        if self.doc_id:
            selector += '&doc=' + AIAAUtils.urllib_quote_plus(self.doc_id)

        fields = {'params': '{}'}
        files = {'datapoint': image_in} if self.doc_id is None else {}

        logger.debug('Using Selector: {}'.format(selector))
        logger.debug('Using Fields: {}'.format(fields))
        logger.debug('Using Files: {}'.format(files))

        form, files = AIAAUtils.http_post_multipart(self.server_url, selector, fields, files)
        form = json.loads(form) if isinstance(form, str) else form

        params = form.get('params')
        if params is None:
            points = json.loads(form.get('points', '[]'))
            params = {'points': (json.loads(points) if isinstance(points, str) else points)}
        else:
            params = json.loads(params) if isinstance(params, str) else params

        self.doc_id = params.get('doc')
        logger.info('Saving Doc-ID: {}'.format(self.doc_id))

        AIAAUtils.save_result(files, image_out)
        return params

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

        fields = {'params': json.dumps({'points': json.dumps(points)})}
        files = {'datapoint': cropped_file}

        logger.debug('Using Selector: {}'.format(selector))
        logger.debug('Using Fields: {}'.format(fields))
        logger.debug('Using Files: {}'.format(files))

        form, files = AIAAUtils.http_post_multipart(self.server_url, selector, fields, files)
        params = form.get('params')
        if params is None:
            points = json.loads(form.get('points'))
            params = {'points': (json.loads(points) if isinstance(points, str) else points)}
        else:
            params = json.loads(params) if isinstance(params, str) else params

        # Post Process
        if len(files) > 0:
            cropped_out_file = tempfile.NamedTemporaryFile(suffix='.nii.gz').name
            AIAAUtils.save_result(files, cropped_out_file)

            AIAAUtils.image_post_processing(cropped_out_file, image_out, crop, image_in)
            os.unlink(cropped_out_file)

        os.unlink(cropped_file)
        return params

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

        response = AIAAUtils.http_post_multipart(self.server_url, selector, fields, files, False)
        response = response.decode('utf-8') if isinstance(response, bytes) else response
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

        form, files = AIAAUtils.http_post_multipart(self.server_url, selector, fields, files)
        AIAAUtils.save_result(files, image_out)
        return form


class AIAAUtils:
    def __init__(self):
        pass

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
        logger.debug('Reading Image from: {}'.format(input_file))

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
    def http_get_method(server_url, selector):
        logger = logging.getLogger(__name__)
        logger.debug('Using Selector: {}{}'.format(server_url, selector))

        parsed = urlparse(server_url)
        if parsed.scheme == 'https':
            logger.debug('Using HTTPS mode')
            # noinspection PyProtectedMember
            conn = httplib.HTTPSConnection(parsed.hostname, parsed.port, context=ssl._create_unverified_context())
        else:
            conn = httplib.HTTPConnection(parsed.hostname, parsed.port)
        conn.request('GET', selector)
        response = conn.getresponse()
        return response.read()

    @staticmethod
    def http_post_multipart(server_url, selector, fields, files, multipart_response=True):
        logger = logging.getLogger(__name__)
        logger.debug('Using Selector: {}{}'.format(server_url, selector))

        parsed = urlparse(server_url)
        if parsed.scheme == 'https':
            logger.debug('Using HTTPS mode')
            # noinspection PyProtectedMember
            conn = httplib.HTTPSConnection(parsed.hostname, parsed.port, context=ssl._create_unverified_context())
        else:
            conn = httplib.HTTPConnection(parsed.hostname, parsed.port)

        content_type, body = AIAAUtils.encode_multipart_formdata(fields, files)
        headers = {'content-type': content_type, 'content-length': str(len(body))}
        conn.request('POST', selector, body, headers)

        response = conn.getresponse()
        logger.debug('Error Code: {}'.format(response.status))
        logger.debug('Error Message: {}'.format(response.reason))
        logger.debug('Headers: {}'.format(response.getheaders()))

        if multipart_response:
            form, files = AIAAUtils.parse_multipart(response.fp if response.fp else response, response.msg)
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

                dir_path = os.path.dirname(os.path.realpath(result_file))
                if not os.path.exists(dir_path):
                    os.makedirs(dir_path)

                with open(result_file, "wb") as f:
                    if isinstance(data, bytes):
                        f.write(data)
                    else:
                        f.write(data.encode('utf-8'))
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

        body = bytearray()
        for l in lines:
            body.extend(l if isinstance(l, bytes) else l.encode('utf-8'))
            body.extend(b'\r\n')

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
        if hasattr(fs, 'list') and isinstance(fs.list, list):
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
        return quote_plus(s)
