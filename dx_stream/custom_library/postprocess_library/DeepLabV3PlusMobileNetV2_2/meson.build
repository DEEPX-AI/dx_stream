project('postprocess_deeplabv3plusmobilenetv2_2', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

dx_stream_dep = declare_dependency(
    include_directories : include_directories('/usr/local/include/dx_stream'),
    link_args : ['-L/usr/local/lib', '-lgstdxstream'],
)

dxrt_dep = declare_dependency(
    include_directories : include_directories('/usr/local/include'),
    link_args : ['-L/usr/local/lib', '-ldxrt'],
)

gst_dep = dependency('gstreamer-1.0', version : '>=1.14',
    required : true, fallback : ['gstreamer', 'gst_dep'])

opencv_dep = dependency('opencv4', required: true)

yolo_postprocess_lib = shared_library('postprocess_deeplabv3plusmobilenetv2_2', 
    'postprocess.cpp',
    dependencies: [opencv_dep, gst_dep, dx_stream_dep, dxrt_dep],
    install: true,
    install_dir: '/usr/share/dx-stream/lib'
)
