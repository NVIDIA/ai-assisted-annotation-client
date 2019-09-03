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

"""Test annotation assistant server."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import json
import logging

import client_api


def call_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('--server_ip', default='0.0.0.0')
    parser.add_argument('--server_port', default=5000)
    parser.add_argument('--api_version', default='v1')
    parser.add_argument('--test_config', required=True)
    parser.add_argument('--debug', default=False)

    args = parser.parse_args()
    print('Using ARGS: {}'.format(args))

    test_config = json.load(open(args.test_config))
    if args.debug:
        logging.basicConfig(level=logging.DEBUG,
                            format='[%(asctime)s.%(msecs)03d][%(levelname)5s](%(name)s:%(funcName)s) - %(message)s',
                            datefmt='%Y-%m-%d %H:%M:%S')

    tests = test_config.get('tests', None)
    if not tests:
        raise ValueError('no tests defined')

    client = client_api.AIAAClient(args.server_ip, args.server_port, args.api_version)
    for test in tests:
        name = test.get('name')
        disabled = test.get('disabled', False)
        api = test.get('api')

        print('')
        print('---------------------------------------------------------------------')
        print('Running Test: {}'.format(name))
        print('---------------------------------------------------------------------')

        if disabled:
            print('This test is skipped')
            continue

        if name is None or api is None:
            raise ValueError('missing name: {} or api: {} in test'.format(name, api))

        # dir_path = os.path.dirname(os.path.realpath(result_prefix))
        # os.makedirs(dir_path, exist_ok=True)

        if api == 'models':
            label = test.get('label')

            models = client.model_list(label)
            print('++++ Listed Models: {}'.format(models))
            continue

        if api == 'segmentation':
            model = test.get('model')
            image_in = test.get('image_in')
            image_out = test.get('image_out')

            client.segmentation(model, image_in, image_out)
            continue

        if api == 'dextr3d':
            model = test.get('model')
            point_set = test.get('point_set')
            image_in = test.get('image_in')
            image_out = test.get('image_out')
            pad = test.get('pad', 20)
            roi_size = test.get('roi_size', '128x128x128')

            client.dextr3d(model, point_set, image_in, image_out, pad, roi_size)
            continue

        if api == 'mask2polygon':
            image_in = test.get('image_in')
            point_ratio = test.get('point_ratio')
            polygons = client.mask2polygon(image_in, point_ratio)

            print('++++ Mask2Polygons: {}'.format(polygons))
            continue

        if api == 'fixpolygon':
            image_in = test.get('image_in')
            image_out = test.get('image_out')
            polygons = test.get('polygons')
            index = test.get('index')
            vertex_offset = test.get('vertex_offset')
            propagate_neighbor = test.get('propagate_neighbor')

            updated_poly = client.fixpolygon(image_in, image_out, polygons, index, vertex_offset, propagate_neighbor)
            print('++++ FixPolygons: {}'.format(updated_poly))
            continue

        raise ValueError("Invalid API: {}".format(args.api))


if __name__ == '__main__':
    call_server()
