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

import sys
sys.path.append('.')
import client_api


def call_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('--test_config')
    args = parser.parse_args()

    if not args.test_config:
        raise SyntaxError('test config file not specified')

    test_config = json.load(open(args.test_config))
    
    server_config = test_config.get('server', None)
    if not server_config:
        raise ValueError('server not configured')
 
    ip = server_config.get('ip', '')
    if not ip:
        raise ValueError('server IP not configured')
    else:
        print("IP Address: ", ip)

    port = server_config.get('port', '')
    if not port:
        raise ValueError('server port not configured')
    else:
        print("Port: ", port)
    
    api_version = server_config.get('api_version', '')
    if not api_version:
        raise ValueError('API version not configured')
    else:
        print("API Version: ", api_version)

    tests = test_config.get('tests', None)
    print(tests)
    if not tests:
        raise ValueError('no tests defined')

    client = client_api.AIAAClient(ip, port, api_version)
    for test in tests:
        name = test.get('name')
        if not name:
            raise ValueError('missing name in test definition')

        api = test.get('api')
        if not api:
            raise ValueError('missing api in test definition')

        temp_folder = test.get('temp_folder') 
        model_name = test.get('model')
        image_path = test.get('image')
        result_prefix = test.get('result_prefix')
        params = test.get('params')
        disabled = test.get('disabled', False)
        pad = test.get('pad', 20)
        roi_size = test.get('roi_size', '128x128x128')
        sigma = test.get('sigma', 3)

        if disabled:
            continue

        if not result_prefix:
            raise ValueError('missing result_prefix in test "{}"'.format(name))

        if api == 'models':
            files = client.model_list(result_prefix)
            continue

        if not image_path:
            raise ValueError('missing image in test "{}"'.format(name))
        if not params:
            raise ValueError('missing params in test "{}"'.format(name))

        if api == 'dextr3d':
            if not model_name:
                raise ValueError('missing model name in test "{}"'.format(name))
            if not temp_folder:
                raise ValueError('missing temp folder name in test "{}"'.format(name))            
            files = client.dextr3d(model_name, temp_folder, image_path, result_prefix+"_result_aas.nii.gz", params, pad, roi_size, sigma)
        elif api == 'mask2polygon':
            files = client.mask2polygon(image_path, result_prefix, params)
        elif api == 'fixpolygon':
            files = client.fixpolygon(image_path, result_prefix, params)
        else:
            raise ValueError("Invalid API: {}".format(args.api))

        print("\nGenerated file for Test '{}':".format(name))
        print(*files, sep='\n')


if __name__ == '__main__':
    call_server()
