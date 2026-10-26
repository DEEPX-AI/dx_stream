v3_sources = [
  'transforms/v3_dsp_transform_kernel.cpp',
  'transforms/v3_dsp_transform_kernel_entry.cpp',
]

v3_kwargs = {
  'cpp_args': common_args,
  'include_directories': [
    include_directories('.'),
    include_directories('./../general'),
    include_directories('./../src/transforms'),
  ],
  'dependencies': [gstreamer_dep, dxdsp_dep],
  'link_with': gstdxstream,
  'install': true,
  'install_dir': gst_plugin_dir,
}

if host_machine.system() == 'windows'
  gstdxstream_v3 = shared_library('gstdxstream-v3', v3_sources, kwargs: v3_kwargs)
else
  gstdxstream_v3 = shared_library('gstdxstream-v3',
    v3_sources,
    version: project_version,
    soversion: '0',
    install_rpath: '$ORIGIN',
    kwargs: v3_kwargs,
  )
endif
