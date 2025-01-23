project('dx_stream_dx_msgconvl', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

gst_dep = dependency('gstreamer-1.0', version : '>=1.14',
    required : true, fallback : ['gstreamer', 'gst_dep'])
opencv_dep = dependency('opencv4', required: true)
json_glib_dep = dependency('json-glib-1.0', version : '>= 1.0')

dx_stream_dep = declare_dependency(
    include_directories : include_directories('/usr/local/include/dx_stream'),
    link_args : ['-L/usr/local/lib', '-lgstdxstream'],
)

dx_msgconvl_lib = shared_library('dx_msgconvl', 
    [
    'dx_msgconvl.cpp',
    'dx_msgconvl_priv.cpp'
    ],
    dependencies : [gst_dep, opencv_dep, json_glib_dep, dx_stream_dep],
    install: true,
    install_dir: '/usr/local/lib'
)
