# Copyright (c) 2019-2021, NVIDIA CORPORATION. All rights reserved.
import os
from setuptools import setup, find_packages

with open('README.md') as f:
    readme = f.read()

with open('LICENSE') as f:
    license = f.read()

with open(os.path.join("py_client", "requirements.txt")) as f:
    requirements = f.read().splitlines()

setup(
    name='aiaa_client',
    version='1.0.2',
    description='NVIDIA Clara AIAA client APIs',
    long_description=readme,
    long_description_content_type="text/markdown",
    author='NVIDIA Clara',
    author_email='nvidia.clara@nvidia.com',
    url='https://developer.nvidia.com/clara',
    license=license,
    packages=find_packages(include=['py_client']),
    install_requires=requirements
)
