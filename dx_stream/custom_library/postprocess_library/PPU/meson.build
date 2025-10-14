project('postprocess_ppu', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

gst_dep = dependency('gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep'])

dx_stream_dep = declare_dependency(
    include_directories : include_directories(join_paths(get_option('prefix'), 'include')),
    link_args : ['-L' + join_paths(get_option('prefix'), 'lib', 'gstreamer-1.0'), '-lgstdxstream'],
)

shared_library('postprocess_ppu', 
    'postprocess.cpp',
    dependencies: [gst_dep, dx_stream_dep],
    install: true,
    install_dir: 'lib/postprocess'
)
