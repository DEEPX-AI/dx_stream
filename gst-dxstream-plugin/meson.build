project('gst-dxstream', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

arch = host_machine.cpu_family()

cc = meson.get_compiler('cpp')

gst_version = meson.project_version()

api_version = '1.0'

eigen_dep = dependency('eigen3', required: true)

gst_dep = dependency('gstreamer-1.0', version : '>=1.14',
    required : true, fallback : ['gstreamer', 'gst_dep'])

gst_video_dep = dependency('gstreamer-video-1.0', version : '>=1.14')

gstbase_dep = dependency('gstreamer-base-1.0', version : '>=1.14',
    fallback : ['gstreamer', 'gst_base_dep'])

json_glib_dep = dependency('json-glib-1.0', version : '>= 1.0')
dl_dep = cc.find_library('dl', required : true)

# import python3
pymodule = import('python3')
python3 = pymodule.find_python()

# Declare dxrt dependency
dxrt_dep = declare_dependency(
    include_directories : include_directories('/usr/local/include'),
    link_args : ['-L/usr/local/lib', '-ldxrt'],
)

zlib_dep = dependency('zlib', required: true)

opencv_dep = dependency('opencv4', required: true)

libyuv_dep = cc.find_library('yuv')

# mqtt: mosquitto
mosquitto_dep = dependency('libmosquitto')

# librdkafka
librdkafka_dep = dependency('rdkafka')

general_inc = [include_directories('./general')]
general_headers = [
  './general/dxcommon.hpp',
  './src/utils/utils.hpp',
  './src/utils/memory_pool.hpp',
  './src/utils/format_convert.hpp'
]
install_headers(general_headers, install_dir: '/usr/local/include/dx_stream')

subdir('metadata')
subdir('src')
