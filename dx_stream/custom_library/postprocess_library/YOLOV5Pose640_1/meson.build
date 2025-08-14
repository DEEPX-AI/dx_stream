project('postprocess_yolo_pose', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

dx_stream_dep = declare_dependency(
    include_directories : include_directories('/usr/local/include/dx_stream'),
    link_args : ['-L/usr/local/lib', '-lgstdxstream'],
)

gst_dep = dependency('gstreamer-1.0', version : '>=1.14',
    required : true, fallback : ['gstreamer', 'gst_dep'])

shared_library('postprocess_yolo_pose', 
    'postprocess.cpp',
    dependencies: [gst_dep, dx_stream_dep],
    install: true,
    install_dir: '/usr/share/dx-stream/lib'
)