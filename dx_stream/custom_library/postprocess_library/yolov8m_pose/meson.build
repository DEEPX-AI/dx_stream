project('postprocess_yolov8m_pose', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++14'])

gst_dep = dependency('gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep'])

dx_stream_dep = dependency('gstdxstream')
dxrt_dep = declare_dependency(link_args: ['-ldxrt'])

shared_library('postprocess_yolov8m_pose', 
    'postprocess.cpp',
    dependencies: [gst_dep, dx_stream_dep, dxrt_dep],
    install: true,
    install_dir: get_option('datadir') / 'gstdxstream' / 'lib'
)