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

# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# http://www.sphinx-doc.org/en/master/config

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
sys.path.insert(0, os.path.abspath('../py-client'))

from builtins import str
import os
import re
import sphinx_rtd_theme
import subprocess
import textwrap

# -- Project information -----------------------------------------------------
project = u'NVIDIA AI-Assisted Annotation Client'
copyright = u'2019, NVIDIA Corporation'
author = u'NVIDIA Corporation'

release = '1.0.1'
version = '1.0.1'

# -- General configuration ---------------------------------------------------
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.mathjax',
    'sphinx.ext.napoleon',
    'sphinx.ext.ifconfig',
    'sphinx.ext.extlinks',
    'nbsphinx',
    'breathe',
    'exhale'
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['templates']

source_suffix = '.rst'
master_doc = 'index'

language = None
exclude_patterns = [u'build', 'Thumbs.db', '.DS_Store', '**.ipynb_checkpoints']
pygments_style = 'sphinx'

# Setup the breathe extension
breathe_projects = {
    "BreatheAIAAClient": "./doxyoutput/xml"
}
breathe_default_project = "BreatheAIAAClient"

# Setup the exhale extension
exhale_args = {
    # These arguments are required
    "containmentFolder":     "./cpp_api",
    "rootFileName":          "cpp_api_root.rst",
    "rootFileTitle":         "C++ API",
    "doxygenStripFromPath":  "..",

    # Suggested optional arguments
    "createTreeView":        True,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin": textwrap.dedent('''
        JAVADOC_AUTOBRIEF = YES
        INPUT = ../cpp-client/include/nvidia/aiaa/client.h ../cpp-client/include/nvidia/aiaa/common.h ../cpp-client/include/nvidia/aiaa/model.h ../cpp-client/include/nvidia/aiaa/pointset.h ../cpp-client/include/nvidia/aiaa/polygon.h ../cpp-client/include/nvidia/aiaa/utils.h ../cpp-client/include/nvidia/aiaa/imageinfo.h ../cpp-client/include/nvidia/aiaa/exception.h
    ''')
}

highlight_language = 'text'

# -- Options for HTML output -------------------------------------------------
html_theme = 'sphinx_rtd_theme'
html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]
html_theme_options = {
    'collapse_navigation': False,
    'display_version': True,
    'logo_only': False,
}
