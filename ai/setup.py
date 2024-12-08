from setuptools import setup, Extension, find_packages

import platform
import os
import sys

DIR = os.path.abspath(os.path.dirname(__file__))
sys.path.append(os.path.join(DIR, "../"))
from pybind11.setup_helpers import Pybind11Extension, build_ext
del sys.path[-1]

if platform.system() == 'Linux':
    depend_libs = ['rt']
else:
    depend_libs = []

nrai_module = Pybind11Extension(
    "_nrai",
    sources=["nrai/pybind11_module.cc"],
    extra_compile_args=['-std=c++14', '-Wno-reorder-ctor', '-Wno-sometimes-uninitialized',
        '-Wno-reorder', '-Wno-maybe-uninitialized'],
    libraries=depend_libs,
)

setup(
    name="nrai",
    version="1.0.0",
    description="NuoRui for python",
    packages=find_packages(),
    ext_modules=[nrai_module]
    if platform.system() == "Linux"
    else [nrai_module]
    if platform.system() == "Darwin"
    else [],
)
