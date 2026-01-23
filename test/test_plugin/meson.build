project('gstdxtest', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

gst_dep = dependency('gstreamer-1.0', version : '>=1.14',
    required : true, fallback : ['gstreamer', 'gst_dep'])

gstbase_dep = dependency('gstreamer-base-1.0', version : '>=1.14',
    fallback : ['gstreamer', 'gst_base_dep'])

dx_stream_dep = declare_dependency(
    include_directories : include_directories('/usr/local/include/dx_stream'),
    link_args : ['-L/usr/local/lib', '-lgstdxstream'],
)

gstdxstream = library('gstdxtest',
    sources: ['dxtest.cpp', 'dxgenobj.cpp', 'dxgenbuffer.cpp'],
    dependencies : [gst_dep, gstbase_dep, dx_stream_dep],
    install : true,
    install_dir : meson.current_source_dir() + '/install',
)

