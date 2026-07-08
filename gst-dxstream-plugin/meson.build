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

v3_flag = get_option('v3_flag')

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

if v3_flag == true
  message('Building in V3 mode (with dxdsp)')
  extra_deps += cc.find_library('dxdsp')
else
  extra_deps += dependency('libmosquitto')
  extra_deps += dependency('rdkafka')
endif

librga_flag = false
librga_dep_maybe = dependency('librga', required: false)
if librga_dep_maybe.found()
    message('Found librga library, adding to dependencies.')
    librga_dep = librga_dep_maybe
    extra_deps += librga_dep
    libdrm_dep = dependency('libdrm', required: true)
    gst_allocators_dep = dependency('gstreamer-allocators-1.0', version: '>=1.0', required: true)
    extra_deps += gst_allocators_dep
    librga_flag = true
endif

dxvnpu_flag = false
if get_option('dxvnpu_flag') == true
    dxvnpu_dep = dependency('dxvnpu', required: true)
    message('DXVNPU mode enabled, adding VNPU elements.')
    extra_deps += dxvnpu_dep
    extra_deps += dependency('threads')
    dxvnpu_flag = true
endif

if not dxrt_flag and not dxvnpu_flag
  error('At least one inference backend (dxrt or dxvnpu) is required')
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

if v3_flag == false
  include_dirs += include_directories('src/brokers')
endif

install_headers = [
  './general/dxcommon.hpp',
  './metadata/gst-dxframemeta.hpp',
  './metadata/gst-dxobjectmeta.hpp',
  './metadata/gst-dxusermeta.hpp',
]

if v3_flag == false
  install_headers += './metadata/gst-dxmsgmeta.hpp'
endif

# Install headers to includedir/gstdxstream
install_headers(install_headers, install_dir: get_option('includedir') / 'gstdxstream')

subdir('src')
