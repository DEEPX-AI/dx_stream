import os
import sys
import subprocess
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

# Get project root from environment
project_root = os.environ.get('PROJECT_ROOT', os.path.abspath('../../..'))
install_dir = os.path.join(project_root, 'install')

# Constants
GSTREAMER_PACKAGE = 'gstreamer-1.0'

# Get GStreamer include/library paths using pkg-config
def get_pkg_config(package, option):
    try:
        return subprocess.check_output(['pkg-config', option, package]).decode('utf-8').strip().split()
    except Exception:
        return []

gst_includes = get_pkg_config(GSTREAMER_PACKAGE, '--cflags-only-I')
gst_includes = [i[2:] for i in gst_includes]  # Remove '-I' prefix
gst_libs = get_pkg_config(GSTREAMER_PACKAGE, '--libs-only-l')
gst_libs = [l[2:] for l in gst_libs]  # Remove '-l' prefix
gst_lib_dirs = get_pkg_config(GSTREAMER_PACKAGE, '--libs-only-L')
gst_lib_dirs = [d[2:] for d in gst_lib_dirs]  # Remove '-L' prefix

# Get gstdxstream paths using pkg-config
gstdxstream_includes = get_pkg_config('gstdxstream', '--cflags-only-I')
gstdxstream_includes = [i[2:] for i in gstdxstream_includes] if gstdxstream_includes else [os.path.join(install_dir, 'include')]
gstdxstream_lib_dirs = get_pkg_config('gstdxstream', '--libs-only-L')
gstdxstream_lib_dirs = [d[2:] for d in gstdxstream_lib_dirs] if gstdxstream_lib_dirs else [os.path.join(install_dir, 'lib')]

# Prepare rpath for runtime library loading
# This embeds library search paths into the .so file so LD_LIBRARY_PATH is not needed
rpath_dirs = []
if gstdxstream_lib_dirs:
    for libdir in gstdxstream_lib_dirs:
        rpath_dirs.append(libdir)
        # Also add gstreamer-1.0 subdirectory where libgstdxstream.so actually lives
        rpath_dirs.append(os.path.join(libdir, 'gstreamer-1.0'))
rpath_dirs.extend(gst_lib_dirs)
rpath_flags = [f'-Wl,-rpath,{d}' for d in rpath_dirs if d]

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
    pybind11_includes = [pybind11.get_include()]
except ImportError:
    raise ImportError(
        "pybind11 is required but not installed.\n"
        "Install it with: pip install pybind11"
    )

ext_modules = [
    Extension(
        'pydxs',
        sources=['src/metadata_binding.cpp'],
        include_dirs=[
            os.path.join(project_root, 'gst-dxstream-plugin', 'metadata'),
            os.path.join(project_root, 'gst-dxstream-plugin', 'general'),
        ] + pybind11_includes + gstdxstream_includes + gst_includes,
        library_dirs=gstdxstream_lib_dirs + gst_lib_dirs,
        libraries=['gstdxstream'] + gst_libs,
        extra_compile_args=['-std=c++14'],
        extra_link_args=rpath_flags,  # Add rpath for runtime library loading
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
