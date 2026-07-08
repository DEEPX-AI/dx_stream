common_args = [
  '-DPACKAGE_VERSION="' + project_version + '"',
  '-DPACKAGE="gst-dxstream"',
  '-DGST_LICENSE="LGPL"',
  '-DGST_PACKAGE_NAME="GStreamer DEEPX DX-Stream Plugins"',
  '-DGST_PACKAGE_ORIGIN="https://github.com/DEEPX-AI/dx_stream"',
  '-DDX_BUILDING'
]

# Add feature flags
if librga_flag
  common_args += '-DHAVE_LIBRGA'
endif

if v3_flag
  common_args += '-DDEEPX_V3'
endif

if dxvnpu_flag
  common_args += '-DHAVE_DXVNPU'
endif

if dxrt_flag
  common_args += '-DHAVE_DXRT'
endif

sources = [
    'gst-dxstream.cpp',
    'gst-dxpreprocess.cpp',
    'gst-dxinfer.cpp',
    'gst-dxpostprocess.cpp',
    'gst-dxosd.cpp',
    'gst-dxrate.cpp',
    'gst-dxtracker.cpp',
    'gst-dxgather.cpp',
    'gst-dxinputselector.cpp',
  	'gst-dxoutputselector.cpp',
    'dxosd_common.cpp',

    './../metadata/gst-dxframemeta.cpp',
    './../metadata/gst-dxobjectmeta.cpp',
    './../metadata/gst-dxusermeta.cpp',

    'tracker/common/src/TrackerFactory.cpp',
    'tracker/OC_SORT/src/Association.cpp',
    'tracker/OC_SORT/src/KalmanBoxTracker.cpp',
    'tracker/OC_SORT/src/KalmanFilter.cpp',
    'tracker/OC_SORT/src/lapjv.cpp',
    'tracker/OC_SORT/src/OCSort.cpp',
    'tracker/OC_SORT/src/Utilities.cpp',

    'preprocessors/preprocessor.cpp',
    'preprocessors/preprocessor_factory.cpp',

    'transforms/video_transform_factory.cpp',
    'transforms/transform_kernel_base.cpp',
    'transforms/libyuv_transform_kernel.cpp',

    'gst-dxscale.cpp',
    'gst-dxconvert.cpp',

    'infer_backend/infer_backend_factory.cpp',
]

if dxrt_flag
  sources += ['infer_backend/dxrt_backend.cpp']
endif

if v3_flag
  sources += [
    'transforms/v3_dsp_transform_kernel.cpp'
  ]
elif librga_flag
  sources += [
    'transforms/rga_transform_kernel.cpp'
  ]
endif


if v3_flag == false
  sources += [
    'gst-dxmsgconv.cpp',
    'gst-dxmsgbroker.cpp',
    'brokers/dx_msgbrokerl_mqtt.cpp',
    'brokers/dx_msgbrokerl_kafka.cpp',
    './../metadata/gst-dxmsgmeta.cpp',
  ]
endif

if dxvnpu_flag
  sources += [
    'gst-dxvnpudec.cpp',
    'gst-dxvnpuenc.cpp',
    'gst-dxvnpupipeline.cpp',
    'gst-dxvnpuoverlay.cpp',
    'transforms/vnpu_transform_kernel.cpp',
    'infer_backend/dxvnpu_backend.cpp',
  ]
endif

# Install to GStreamer plugin directory under libdir
gst_plugin_dir = get_option('libdir') / 'gstreamer-1.0'

message('GStreamer plugin will be installed to: ' + get_option('prefix') / gst_plugin_dir)

# Build and install the shared library
gstdxstream_kwargs = {
    'cpp_args': common_args,
    'include_directories': include_dirs,
    'dependencies': extra_deps,
    'install': true,
    'install_dir': gst_plugin_dir,
}
if host_machine.system() == 'windows'
  gstdxstream = shared_library('gstdxstream',
    sources,
    kwargs: gstdxstream_kwargs,
  )
else
  gstdxstream = shared_library('gstdxstream',
    sources,
    version: project_version,
    soversion: '0',
    kwargs: gstdxstream_kwargs,
  )
endif

# Generate pkg-config file
pkg = import('pkgconfig')
pkg.generate(
    gstdxstream,
    name: 'gstdxstream',
    filebase: 'gstdxstream',
    description: 'DeepX Stream GStreamer Plugin',
    requires: ['gstreamer-1.0 >= 1.0'],
    variables: [
        'plugindir=' + '${libdir}' / 'gstreamer-1.0'
    ],
    # Install to lib/pkgconfig (standard location, not lib/arch/pkgconfig!)
    install_dir: join_paths(get_option('prefix'), 'lib', 'pkgconfig')
)
