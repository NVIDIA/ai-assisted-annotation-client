# Copyright (c) 2019 - 2021, NVIDIA CORPORATION. All rights reserved.
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
import sys

import client_api


# Support Python 2.7 json load
def json_load_byteified(file_handle):
    return _byteify(
        json.load(file_handle, object_hook=_byteify),
        ignore_dicts=True
    )


def json_loads_byteified(json_text):
    return _byteify(
        json.loads(json_text, object_hook=_byteify),
        ignore_dicts=True
    )


def _byteify(data, ignore_dicts=False):
    if sys.version_info[0] >= 3:
        return data

    if isinstance(data, unicode):
        return data.encode('utf-8')
    if isinstance(data, list):
        return [_byteify(item, ignore_dicts=True) for item in data]
    if isinstance(data, dict) and not ignore_dicts:
        return {
            _byteify(key, ignore_dicts=True): _byteify(value, ignore_dicts=True)
            for key, value in data.iteritems()
        }
    return data


def call_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--server_url', required=True, help='AIAA Server URI')
    parser.add_argument('-c', '--test_config', required=True, help='Test JSON Config')
    parser.add_argument('-n', '--name', help='Execute single test from config of this name')
    parser.add_argument('-d', '--debug', default=False, action='store_true', help='Enable debug logs')

    args = parser.parse_args()
    print('Using ARGS: {}'.format(args))

    test_config = json_load_byteified(open(args.test_config))
    if args.debug:
        logging.basicConfig(level=logging.DEBUG,
                            format='[%(asctime)s.%(msecs)03d][%(levelname)5s](%(name)s:%(funcName)s) - %(message)s',
                            datefmt='%Y-%m-%d %H:%M:%S')

    tests = test_config.get('tests', None)
    if not tests:
        raise ValueError('no tests defined')

    client = client_api.AIAAClient(args.server_url)
    session_id = None
    print('Total tests available: {}'.format(len(tests)))
    disabled_list = []
    for test in tests:
        name = test.get('name')
        disabled = test.get('disabled', False)
        disabled = False if args.name and args.name == name else True if args.name else disabled
        api = test.get('api')

        if disabled:
            disabled_list.append(name)
            continue

        print('')
        print('---------------------------------------------------------------------')
        print('Running Test: {}'.format(name))
        print('---------------------------------------------------------------------')

        if name is None or api is None:
            raise ValueError('missing name: {} or api: {} in test'.format(name, api))

        if api == 'models':
            label = test.get('label')

            models = client.model_list(label)
            print('++++ Listed Models: {}'.format(json.dumps(models)))
            continue

        if api == 'create_session':
            image_in = test.get('image_in')

            response = client.create_session(image_in)
            print('++++ Session Response: {}'.format(json.dumps(response)))
            session_id = response.get('session_id')
            continue

        if api == 'get_session':
            response = client.get_session(session_id)
            print('++++ Session Info: {}'.format(json.dumps(response)))
            continue

        if api == 'close_session':
            response = client.close_session(session_id)
            print('++++ Session ({}) closed: {}'.format(session_id, response))
            continue

        if api == 'segmentation':
            model = test.get('model')
            image_in = test.get('image_in')
            image_out = test.get('image_out')

            result = client.segmentation(model, image_in, image_out)
            print('++++ Segmentation JSON  Result: {}'.format(json.dumps(result)))
            print('++++ Segmentation Image result: {}'.format(image_out))
            continue

        if api == 'dextr3d':
            model = test.get('model')
            point_set = test.get('point_set')
            image_in = test.get('image_in')
            image_out = test.get('image_out')
            pad = test.get('pad', 20)
            roi_size = test.get('roi_size', '128x128x128')

            result = client.dextr3d(model, point_set, image_in, image_out, pad, roi_size)
            print('++++ dextr3d JSON  Result: {}'.format(json.dumps(result)))
            print('++++ dextr3d Image Result: {}'.format(image_out))
            continue

        if api == 'deepgrow':
            model = test.get('model')
            foreground = test.get('foreground')
            background = test.get('background')
            image_in = test.get('image_in')
            image_out = test.get('image_out')

            result = client.deepgrow(model, foreground, background, image_in, image_out, foreground[-1])
            print('++++ Deepgrow JSON  Result: {}'.format(json.dumps(result)))
            print('++++ Deepgrow Image Result: {}'.format(image_out))
            continue

        if api == 'inference':
            model = test.get('model')
            params = test.get('params')
            image_in = test.get('image_in')
            image_out = test.get('image_out')

            result = client.inference(model, params, image_in, image_out)
            print('++++ Inference JSON  Result: {}'.format(json.dumps(result)))
            print('++++ Inference Image Result: {}'.format(image_out))
            continue

        if api == 'mask2polygon':
            image_in = test.get('image_in')
            point_ratio = test.get('point_ratio')
            polygons = client.mask2polygon(image_in, point_ratio)

            print('++++ Mask2Polygons: {}'.format(json.dumps(polygons)))
            continue

        if api == 'fixpolygon':
            image_in = test.get('image_in')
            image_out = test.get('image_out')
            polygons = test.get('polygons')
            index = test.get('index')
            vertex_offset = test.get('vertex_offset')
            propagate_neighbor = test.get('propagate_neighbor')

            updated_poly = client.fixpolygon(image_in, image_out, polygons, index, vertex_offset, propagate_neighbor)
            print('++++ FixPolygons: {}'.format(json.dumps(updated_poly)))
            continue

        raise ValueError("Invalid API: {}".format(args.api))

    if len(disabled_list) and not args.name:
        print('\nDisabled Tests ({}): {}'.format(len(disabled_list), ', '.join(disabled_list)))
    if args.name and len(disabled_list) == len(tests):
        print('"{}" Test NOT found; Available test names: {}'.format(args.name, disabled_list))


if __name__ == '__main__':
    call_server()
