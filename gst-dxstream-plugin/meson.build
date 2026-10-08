project('gst-dxstream', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++14'])

fs = import('fs')
version_file = './../release.ver'
if not fs.is_file(version_file)
  error('Not Exist: ' + version_file)
endif
raw_version = fs.read(version_file, encoding: 'utf-8').strip()
if raw_version == ''
  error('Empty: ' + version_file)
endif
if raw_version.startswith('v')
  project_version = raw_version.substring(1)
else
  project_version = raw_version
endif

arch = host_machine.cpu_family()
host_sys = host_machine.system()
is_windows = host_sys == 'windows'
cc = meson.get_compiler('cpp')

extra_deps = []

with_dxv3 = get_option('with_dxv3')
with_dxvnpu = get_option('with_dxvnpu')
with_rkrga = get_option('with_rkrga')
with_gles = get_option('with_gles')

gstreamer_dep = dependency(
    'gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep']
)

extra_deps += gstreamer_dep
extra_deps += dependency('eigen3', required: true)
extra_deps += dependency('gstreamer-video-1.0', version : '>=1.16')
extra_deps += dependency('gstreamer-base-1.0', version : '>=1.16')
extra_deps += dependency('json-glib-1.0', version : '>= 1.0')

# dl: POSIX-only. Windows uses LoadLibrary via dx_dlfcn.h shim.
if not is_windows
  extra_deps += cc.find_library('dl', required : true)
endif

extra_deps += dependency('zlib', required: true)
extra_deps += dependency('opencv4', required: true)
extra_deps += cc.find_library(is_windows ? 'libyuv' : 'yuv')

# DEEPX internal libraries — cc.find_library (no .pc files)
dxrt_flag = false
dxrt_dep_maybe = cc.find_library('dxrt', required: false)
if dxrt_dep_maybe.found()
  message('Found dxrt library, adding to dependencies.')
  extra_deps += dxrt_dep_maybe
  dxrt_flag = true
endif

dxdsp_dep = cc.find_library('dxdsp', required: with_dxv3)
dxdsp_enabled = dxdsp_dep.found()
if dxdsp_enabled
  message('Found dxdsp library, building DX V3 hardware backend plugin.')
endif
extra_deps += dependency('libmosquitto')
extra_deps += dependency('rdkafka')

# gstreamer-allocators is a standard GStreamer dep (no extra hardware lib
# required) — always available so core can detect DMA-buf memory generically,
# independent of whether the RGA hardware backend is present.
extra_deps += dependency('gstreamer-allocators-1.0', version: '>=1.0', required: true)

dxvnpu_dep = dependency('dxvnpu', required: with_dxvnpu)
dxvnpu_enabled = dxvnpu_dep.found()
if dxvnpu_enabled
  message('Found dxvnpu library, building DX VNPU hardware backend plugin.')
endif

librga_dep = dependency('librga', required: with_rkrga)
librga_enabled = librga_dep.found()
if librga_enabled
  message('Found librga library, building RGA hardware backend plugin.')
endif

# auto only builds GLES on ARM SoCs (validated on Raspberry Pi 5); other hosts need -Dwith_gles=enabled.
gles_enabled = false
if with_gles.enabled() or (with_gles.auto() and arch in ['aarch64', 'arm'])
  egl_dep = dependency('egl', required: with_gles)
  glesv2_dep = dependency('glesv2', required: with_gles)
  gbm_dep = dependency('gbm', required: false)
  gles_enabled = egl_dep.found() and glesv2_dep.found() and not is_windows
endif
if gles_enabled
  message('Found EGL/GLESv2, building OpenGL ES hardware backend plugin.')
endif

if not dxrt_flag
  error('dxrt backend (libdxrt) is required to build the core plugin')
endif

include_dirs = [
    include_directories('general'),
    include_directories('metadata'),
    include_directories('src/tracker/common/include'),
    include_directories('src/tracker/OC_SORT/include'),
    include_directories('src/transforms'),
    include_directories('src/preprocessors'),
    include_directories('src/infer_backend')
]

include_dirs += include_directories('src/brokers')

install_headers = [
  './general/dxcommon.hpp',
  './metadata/gst-dxframemeta.hpp',
  './metadata/gst-dxobjectmeta.hpp',
  './metadata/gst-dxusermeta.hpp',
]

install_headers += './metadata/gst-dxmsgmeta.hpp'

# Install headers to includedir/gstdxstream
install_headers(install_headers, install_dir: get_option('includedir') / 'gstdxstream')

subdir('src')

if dxvnpu_enabled
  subdir('src-vnpu')
endif

if librga_enabled
  subdir('src-rga')
endif

if gles_enabled
  subdir('src-gles')
endif

if dxdsp_enabled
  subdir('src-v3')
endif
