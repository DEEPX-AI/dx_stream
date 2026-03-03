project('dx_stream_dx_msgconvl', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++14'])

opencv_dep = dependency('opencv4', required: true)
json_glib_dep = dependency('json-glib-1.0', version : '>= 1.0')

gst_dep = dependency('gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep'])

dx_stream_dep = dependency('gstdxstream')
dxrt_dep = declare_dependency(link_args: ['-ldxrt'])

dx_msgconvl_lib = shared_library('dx_msgconvl', 
    [
    'dx_msgconvl.cpp',
    'dx_msgconvl_priv.cpp'
    ],
    dependencies : [gst_dep, opencv_dep, json_glib_dep, dx_stream_dep, dxrt_dep],
    install: true,
    install_dir: get_option('datadir') / 'gstdxstream' / 'lib'
)
