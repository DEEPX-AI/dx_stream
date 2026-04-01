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
extra_deps += dependency('json-glib-1.0', version : '>= 1.0')
extra_deps += cc.find_library('dl', required : true)
extra_deps += dependency('zlib', required: true)
extra_deps += dependency('opencv4', required: true)
extra_deps += cc.find_library('yuv')
extra_deps += cc.find_library('dxrt')

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

include_dirs = [
    include_directories('general'),
    include_directories('metadata'),
    include_directories('src/tracker/common/include'),
    include_directories('src/tracker/OC_SORT/include'),
    include_directories('src/transforms'),
    include_directories('src/preprocessors')
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
