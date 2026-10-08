rga_deps = [
    gstreamer_dep,
    dependency('gstreamer-video-1.0', version: '>=1.16'),
    dependency('gstreamer-base-1.0', version: '>=1.16'),
    dependency('gstreamer-allocators-1.0', version: '>=1.0'),
    librga_dep,
    dependency('libdrm', required: true),
]

sources = [
    'transforms/rga_transform_kernel.cpp',
    'transforms/rga_transform_kernel_entry.cpp',
]

rga_include_dirs = [
    include_directories('.'),
    include_directories('../general'),
    include_directories('./../src/transforms'),
]

rga_kwargs = {
    'cpp_args': common_args,
    'include_directories': rga_include_dirs,
    'dependencies': rga_deps,
    'link_with': gstdxstream,
    'install': true,
    'install_dir': gst_plugin_dir,
}

if host_machine.system() == 'windows'
  gstdxstream_rga = shared_library('gstdxstream-rga', sources, kwargs: rga_kwargs)
else
  gstdxstream_rga = shared_library('gstdxstream-rga',
    sources,
    version: project_version,
    soversion: '0',
    install_rpath: '$ORIGIN',
    kwargs: rga_kwargs,
  )
endif
