gles_deps = [
    gstreamer_dep,
    egl_dep,
    glesv2_dep,
]
gles_args = common_args
# GBM gives a headless render-node display; without it the surfaceless/default EGL display is used.
if gbm_dep.found()
  gles_deps += gbm_dep
  gles_args += '-DHAVE_GBM'
endif

sources = [
    'transforms/gles_device.cpp',
    'transforms/gles_transform_kernel.cpp',
    'transforms/gles_transform_kernel_entry.cpp',
]

gles_include_dirs = [
    include_directories('.'),
    include_directories('./../general'),
    include_directories('./../src/transforms'),
]

gstdxstream_gles = shared_library('gstdxstream-gles',
    sources,
    version: project_version,
    soversion: '0',
    install_rpath: '$ORIGIN',
    cpp_args: gles_args,
    include_directories: gles_include_dirs,
    dependencies: gles_deps,
    link_with: gstdxstream,
    override_options: ['cpp_std=c++17'],
    install: true,
    install_dir: gst_plugin_dir,
)
