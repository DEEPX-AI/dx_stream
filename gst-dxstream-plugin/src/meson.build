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

configure_file(output : 'config.h', configuration : cdata)

sources = [
    'gst-dxstream.cpp',
    'gst-dxinfer.cpp',
    'gst-dxmuxer.cpp',
    'gst-dxosd.cpp',
    'gst-dxpreprocess.cpp',
    'gst-dxpostprocess.cpp',
    'gst-dxrate.cpp',
    'gst-dxtiler.cpp',
    'gst-dxtracker.cpp',
    'gst-dxmsgconv.cpp',
    'gst-dxmsgbroker.cpp',
    'gst-dxgather.cpp',
    'gst-dxrouter.cpp',
    'utils/memory_pool.cpp',
    'brokers/dx_msgbrokerl_mqtt.cpp',
    'brokers/dx_msgbrokerl_kafka.cpp',

    './../metadata/gst-dxmeta.cpp',
    './../metadata/gst-dxframemeta.cpp',
    './../metadata/gst-dxobjectmeta.cpp',
    './../metadata/gst-dxmsgmeta.cpp',

    'tracker/common/src/TrackerFactory.cpp',
    'tracker/OC_SORT/src/Association.cpp',
    'tracker/OC_SORT/src/KalmanBoxTracker.cpp',
    'tracker/OC_SORT/src/KalmanFilter.cpp',
    'tracker/OC_SORT/src/lapjv.cpp',
    'tracker/OC_SORT/src/OCSort.cpp',
    'tracker/OC_SORT/src/Utilities.cpp'
]

sources += [
    'utils/libyuv_transform/libyuv_transform.cpp'
  ]

gstdxstream = library('gstdxstream',
    sources,
    cpp_args: common_args,
    include_directories: include_dirs,
    dependencies : extra_deps,
    install : true,
    install_dir : '/usr/lib/' + arch + '-linux-gnu/gstreamer-1.0',
)
