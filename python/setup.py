import sys
import os
import glob
import setuptools

from setuptools import setup

__version__ = "0.1.0"

from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext

class get_pybind_include(object):
    def __str__(self):
        import pybind11
        return pybind11.get_include()

class get_numpy_include(object):
    def __str__(self):
        import numpy
        return numpy.get_include()

root = os.path.abspath(os.path.dirname(__file__))
platform = os.getenv("ROUTIO_PYTHON_PLATFORM", sys.platform)

if platform.startswith('linux'):
    library_suffix = '.so'
elif platform in ['darwin']:
    library_suffix = '.dylib'
elif platform.startswith('win'):
    library_suffix = '.dll'

class build_ext_ctypes(build_ext):

    def build_extension(self, ext):
        self._ctypes = isinstance(ext, CTypes)

        def _realize(c):
            from typing import Callable
            if isinstance(c, Callable):
                return c()
            else:
                return c

        ct = self.compiler.compiler_type
        opts = [] #self.c_opts.get(ct, [])
        #link_opts = self.l_opts.get(ct, [])
        if ct == 'unix':
            opts.append("-std=c++17")

        ext.include_dirs = [_realize(x) for x in ext.include_dirs]

        ext.extra_compile_args = opts

        return super().build_extension(ext)

    def get_export_symbols(self, ext):
        if self._ctypes:
            return ext.export_symbols
        return super().get_export_symbols(ext)

    def get_ext_filename(self, ext_name):
        if self._ctypes:
            return ext_name + library_suffix
        return super().get_ext_filename(ext_name)

class CTypes(Extension): 
    pass

varargs = dict()
varargs["package_data"] = {}
varargs["cmdclass"] = {}

try:
    from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

    class bdist_wheel(_bdist_wheel):

        def finalize_options(self):
            _bdist_wheel.finalize_options(self)
            # Mark us as not a pure python package
            self.root_is_pure = False

        def get_tag(self):
            python, abi, plat = _bdist_wheel.get_tag(self)
            # We don't contain any python source
            python, abi = 'py2.py3', 'none'
            return python, abi, plat

    varargs["cmdclass"] = {'bdist_wheel': bdist_wheel}
    varargs["setup_requires"] = ['wheel']

except ImportError:
    pass

if os.path.isfile(os.path.join("routio", "pyroutio" + library_suffix)):
    varargs["package_data"]["routio"] = ["pyroutio" + library_suffix]

elif os.path.isfile(os.path.join("routio", "routio", "loop.h")):
    sources = glob.glob("routio/routio/*.cpp")
    args = dict(sources=sources, include_dirs=[os.path.join(root, "routio"), get_numpy_include(), get_pybind_include()], define_macros=[("ROUTIO_EXPORTS", "1")])
    args["language"] = "c++"
    varargs["ext_modules"] = [CTypes("routio.pyroutio", **args)]
    varargs["cmdclass"] = {'build_ext': build_ext_ctypes}
    varargs["setup_requires"] = ["pybind11>=2.5.0", "numpy>=1.16"]
    varargs["exclude_package_data"] = {'routio.pyroutio': ['*.cpp']}

varargs["package_data"]['routio.messages.library'] = ['*.msg']
varargs["package_data"]['routio.messages.templates'] = ['*.tpl']

setup(
    name='routio',
    version=__version__,
    author='Luka Cehovin Zajc',
    author_email='luka.cehovin@gmail.com',
    url='https://github.com/vicoslab/routio',
    description='Python bindings for routio',
    long_description='',
    packages=setuptools.find_packages(),
    include_package_data=True,
    python_requires='>=2.7,!=3.0.*,!=3.1.*,!=3.2.*,!=3.3.*,!=3.4.*,!=3.5.*',
    install_requires=["numpy>=1.16", "jinja2", "pyparsing", "future"],
    zip_safe=False,
    entry_points={
        'console_scripts': [
            'routio-daemon = routio.__main__:main',
            'routio-router = routio.__main__:main',
            'routio-generate = routio.messages.cli:main',
        ],
        'ignition': [
            'routio_remap = routio.ignition:Mapping',
        ],
    },
    **varargs
)
