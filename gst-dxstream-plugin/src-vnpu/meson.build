sources = [
    'gst-dxstream-vnpu.cpp',
    'gst-dxvnpudec.cpp',
    'gst-dxvnpuenc.cpp',
    'infer_backend/dxvnpu_backend.cpp',
    'infer_backend/dxvnpu_backend_entry.cpp',
    'transforms/vnpu_transform_kernel.cpp',
    'transforms/vnpu_transform_kernel_entry.cpp',
]

vnpu_deps = [
    gstreamer_dep,
    dependency('gstreamer-video-1.0', version: '>=1.16'),
    dependency('gstreamer-base-1.0', version: '>=1.16'),
    dxvnpu_dep,
    dependency('threads'),
]

vnpu_include_dirs = [
    include_directories('.'),
    include_directories('./../src'),
    include_directories('./../src/infer_backend'),
    include_directories('./../src/transforms'),
    include_directories('./../general'),
    include_directories('./../metadata'),
]

vnpu_kwargs = {
    'cpp_args': common_args,
    'include_directories': vnpu_include_dirs,
    'dependencies': vnpu_deps,
    'link_with': gstdxstream,
    'install': true,
    'install_dir': gst_plugin_dir,
}

if host_machine.system() == 'windows'
  gstdxstream_vnpu = shared_library('gstdxstream-vnpu', sources, kwargs: vnpu_kwargs)
else
  gstdxstream_vnpu = shared_library('gstdxstream-vnpu',
    sources,
    version: project_version,
    soversion: '0',
    install_rpath: '$ORIGIN',
    kwargs: vnpu_kwargs,
  )
endif
