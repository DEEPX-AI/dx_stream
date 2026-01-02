common_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', project_version)
cdata.set_quoted('PACKAGE', 'gst-dxstream')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer DEEPX DX-Stream Plugins')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://github.com/DEEPX-AI/dx_stream')

if have_librga_flag
  cdata.set('HAVE_LIBRGA', true)
endif

if v3_mode
  cdata.set('DEEPX_V3', true)
endif


configure_file(output : 'config.h', configuration : cdata)

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

    './../metadata/gst-dxframemeta.cpp',
    './../metadata/gst-dxobjectmeta.cpp',
    './../metadata/gst-dxusermeta.cpp',

    'tracker/common/src/TrackerFactory.cpp',
    'tracker/OC_SORT/src/Association.cpp',
    'tracker/OC_SORT/src/KalmanBoxTracker.cpp',
    'tracker/OC_SORT/src/KalmanFilter.cpp',
    'tracker/OC_SORT/src/lapjv.cpp',
    'tracker/OC_SORT/src/OCSort.cpp',
    'tracker/OC_SORT/src/Utilities.cpp'
]

sources += [
    'utils/libyuv_transform/libyuv_transform.cpp',
    'preprocessors/preprocessor.cpp',
    'preprocessors/preprocessor_factory.cpp',
    'preprocessors/libyuv_preprocessor.cpp',
    'preprocessors/rga_preprocessor.cpp'
]

if v3_mode == false
  sources += [
    'gst-dxmsgconv.cpp',
    'gst-dxmsgbroker.cpp',
    'brokers/dx_msgbrokerl_mqtt.cpp',
    'brokers/dx_msgbrokerl_kafka.cpp',
    './../metadata/gst-dxmsgmeta.cpp',
  ]
endif

# Install to lib directory relative to prefix
gstdxstream = library('gstdxstream',
    sources,
    cpp_args: common_args,
    include_directories: include_dirs,
    dependencies : extra_deps,
    install : true,
    install_dir : 'lib/gstreamer-1.0',
)
