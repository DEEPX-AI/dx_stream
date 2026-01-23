project('test_usermeta', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

gst_dep = dependency('gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep'])

glib_dep = dependency('glib-2.0', required : true)

# Use the installed dx-stream library
dx_stream_inc = include_directories(join_paths('..', '..', 'install', 'include'))
dx_stream_lib_dir = join_paths('..', '..', 'install', 'lib', 'gstreamer-1.0')

dx_stream_dep = declare_dependency(
    include_directories : dx_stream_inc,
    link_args : ['-L' + dx_stream_lib_dir, '-lgstdxstream'],
)

executable('test_usermeta', 
    'test_usermeta.cpp',
    dependencies: [gst_dep, glib_dep, dx_stream_dep],
    install: true,
    install_dir: 'bin',
    cpp_args: ['-DGST_USE_UNSTABLE_API']
)