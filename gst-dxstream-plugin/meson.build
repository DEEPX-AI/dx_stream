project('gst-dxstream', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

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

gstreamer_dep = dependency(
    'gstreamer-1.0', version : '>=1.14',
    required : true, fallback : ['gstreamer', 'gst_dep']
)
gstreamer_base_dep = dependency(
    'gstreamer-base-1.0', version : '>=1.14',
    fallback : ['gstreamer', 'gst_base_dep']
)
extra_deps += gstreamer_dep
extra_deps += gstreamer_base_dep
extra_deps += dependency('eigen3', required: true)
extra_deps += dependency('gstreamer-video-1.0', version : '>=1.14')
extra_deps += dependency('json-glib-1.0', version : '>= 1.0')
extra_deps += cc.find_library('dl', required : true)
extra_deps += dependency('zlib', required: true)
extra_deps += dependency('opencv4', required: true)
extra_deps += cc.find_library('yuv')
extra_deps += dependency('libmosquitto')
extra_deps += dependency('rdkafka')

extra_deps += declare_dependency(
    include_directories : include_directories('/usr/local/include'),
    link_args : ['-L/usr/local/lib', '-ldxrt'],
)

have_librga_flag = false
librga_dep_maybe = dependency('librga', required: false)
if librga_dep_maybe.found()
    message('Found librga library, adding to dependencies.')
    librga_dep = librga_dep_maybe
    extra_deps += librga_dep
    libdrm_dep = dependency('libdrm', required: true)
    have_librga_flag = true
endif

include_dirs = [
    include_directories('general'),
    include_directories('metadata'),
    include_directories('src/tracker/common/include'),
    include_directories('src/tracker/OC_SORT/include'),
    include_directories('src/utils'),
    include_directories('src/brokers')
]

install_headers = [
  './general/dxcommon.hpp',
  './metadata/gst-dxmeta.hpp',
  './metadata/gst-dxframemeta.hpp',
  './metadata/gst-dxobjectmeta.hpp',
  './metadata/gst-dxmsgmeta.hpp'
]

install_headers += [
  './src/utils/libyuv_transform/libyuv_transform.hpp'
]


install_headers(install_headers, install_dir: '/usr/local/include/dx_stream')

subdir('src')
