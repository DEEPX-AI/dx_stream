import os
import sys
import subprocess
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

# Get project root from environment
project_root = os.environ.get('PROJECT_ROOT', os.path.abspath('../../..'))
install_dir = os.path.join(project_root, 'install')

# Get GStreamer include/library paths using pkg-config
def get_pkg_config(package, option):
    try:
        return subprocess.check_output(['pkg-config', option, package]).decode('utf-8').strip().split()
    except:
        return []

gst_includes = get_pkg_config('gstreamer-1.0', '--cflags-only-I')
gst_includes = [i[2:] for i in gst_includes]  # Remove '-I' prefix
gst_libs = get_pkg_config('gstreamer-1.0', '--libs-only-l')
gst_libs = [l[2:] for l in gst_libs]  # Remove '-l' prefix
gst_lib_dirs = get_pkg_config('gstreamer-1.0', '--libs-only-L')
gst_lib_dirs = [d[2:] for d in gst_lib_dirs]  # Remove '-L' prefix

class BuildExt(build_ext):
    def build_extensions(self):
        # Add C++14 support
        for ext in self.extensions:
            ext.extra_compile_args.append('-std=c++14')
            if sys.platform == 'darwin':
                ext.extra_compile_args.append('-stdlib=libc++')
        build_ext.build_extensions(self)

# Get pybind11 include paths
try:
    import pybind11
    pybind11_includes = pybind11.get_include()
except ImportError:
    pybind11_includes = []

ext_modules = [
    Extension(
        'pydxs',
        sources=['src/metadata_binding.cpp'],
        include_dirs=[
            os.path.join(install_dir, 'include'),
            os.path.join(project_root, 'gst-dxstream-plugin', 'metadata'),
            os.path.join(project_root, 'gst-dxstream-plugin', 'general'),
            pybind11_includes,
        ] + gst_includes,
        library_dirs=[
            os.path.join(install_dir, 'lib'),
        ] + gst_lib_dirs,
        libraries=['gstdxstream'] + gst_libs,
        extra_compile_args=['-std=c++14'],
        language='c++'
    ),
]

setup(
    name='pydxs',
    version='0.1.0',
    author='DeepX',
    description='Python bindings for DX Stream',
    ext_modules=ext_modules,
    cmdclass={'build_ext': BuildExt},
    zip_safe=False,
    python_requires='>=3.6',
)
