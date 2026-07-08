import os
import re
import sys
import subprocess
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

IS_WINDOWS = sys.platform == 'win32'

# Get project root from environment
project_root = os.environ.get('PROJECT_ROOT', os.path.abspath('../../..'))
install_dir = os.path.join(project_root, 'install')

# Constants
GSTREAMER_PACKAGE = 'gstreamer-1.0'

def _split_pkg_config(output):
    """Split pkg-config output on unescaped spaces.

    pkg-config emits forward-slash paths and backslash-escapes embedded spaces
    (e.g. "-IC:/Program\\ Files/gstreamer/..."). We split only on spaces that
    are NOT preceded by a backslash, then unescape "\\ " back to " ". This keeps
    "C:/Program Files" as a single token without disturbing any other character.
    """
    tokens = re.split(r'(?<!\\) ', output)
    return [t.replace('\\ ', ' ') for t in tokens if t]

# Get GStreamer include/library paths using pkg-config
def get_pkg_config(package, option):
    try:
        output = subprocess.check_output(
            ['pkg-config', option, package],
            stderr=subprocess.DEVNULL
        ).decode('utf-8').strip()
        return _split_pkg_config(output)
    except Exception:
        return []

def _get_gstreamer_root_win():
    """Find GStreamer MSVC installation root on Windows."""
    root = os.environ.get('GSTREAMER_1_0_ROOT_MSVC_X86_64', '')
    if root and os.path.isdir(root):
        return root
    for candidate in [
        r'C:\Program Files\gstreamer\1.0\msvc_x86_64',
        r'C:\gstreamer\1.0\msvc_x86_64',
    ]:
        if os.path.isdir(candidate):
            return candidate
    return ''

gst_includes = get_pkg_config(GSTREAMER_PACKAGE, '--cflags-only-I')
gst_includes = [i[2:] for i in gst_includes]  # Remove '-I' prefix
gst_libs = get_pkg_config(GSTREAMER_PACKAGE, '--libs-only-l')
gst_libs = [l[2:] for l in gst_libs]  # Remove '-l' prefix
gst_lib_dirs = get_pkg_config(GSTREAMER_PACKAGE, '--libs-only-L')
gst_lib_dirs = [d[2:] for d in gst_lib_dirs]  # Remove '-L' prefix

# Windows fallback when pkg-config is unavailable
if IS_WINDOWS and not gst_includes:
    _gst_root = _get_gstreamer_root_win()
    if _gst_root:
        gst_includes = [
            os.path.join(_gst_root, 'include', 'gstreamer-1.0'),
            os.path.join(_gst_root, 'include', 'glib-2.0'),
            os.path.join(_gst_root, 'lib', 'glib-2.0', 'include'),
        ]
        gst_lib_dirs = [os.path.join(_gst_root, 'lib')]
        gst_libs = ['gstreamer-1.0', 'gobject-2.0', 'glib-2.0']
    else:
        print("WARNING: GStreamer not found. Set GSTREAMER_1_0_ROOT_MSVC_X86_64.")

# Get gstdxstream paths using pkg-config
gstdxstream_includes = get_pkg_config('gstdxstream', '--cflags-only-I')
gstdxstream_includes = [i[2:] for i in gstdxstream_includes] if gstdxstream_includes else [os.path.join(install_dir, 'include')]
gstdxstream_lib_dirs = get_pkg_config('gstdxstream', '--libs-only-L')
gstdxstream_lib_dirs = [d[2:] for d in gstdxstream_lib_dirs] if gstdxstream_lib_dirs else [os.path.join(install_dir, 'lib')]

# Platform-specific link settings
extra_link_args = []
library_dirs = list(gstdxstream_lib_dirs) + list(gst_lib_dirs)

if IS_WINDOWS:
    # gstdxstream.lib may reside in install/lib/gstreamer-1.0/
    plugin_lib_dir = os.path.join(install_dir, 'lib', 'gstreamer-1.0')
    if os.path.isdir(plugin_lib_dir):
        library_dirs.insert(0, plugin_lib_dir)
else:
    # Embed rpath so LD_LIBRARY_PATH is not needed at runtime
    rpath_dirs = []
    if gstdxstream_lib_dirs:
        for libdir in gstdxstream_lib_dirs:
            rpath_dirs.append(libdir)
            rpath_dirs.append(os.path.join(libdir, 'gstreamer-1.0'))
    rpath_dirs.extend(gst_lib_dirs)
    extra_link_args = [f'-Wl,-rpath,{d}' for d in rpath_dirs if d]

class BuildExt(build_ext):
    def build_extensions(self):
        for ext in self.extensions:
            if self.compiler.compiler_type == 'msvc':
                ext.extra_compile_args.extend(['/std:c++14', '/EHsc'])
            else:
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
        library_dirs=library_dirs,
        libraries=['gstdxstream'] + gst_libs,
        extra_link_args=extra_link_args,
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
