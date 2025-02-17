common_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-dxstream')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'DX-Stream')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://gstreamer.freedesktop.org')
configure_file(output : 'config.h', configuration : cdata)

dxstream_sources = [
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
    'utils/format_convert.cpp',
    'utils/memory_pool.cpp',
    'utils/utils.cpp',
    'brokers/dx_msgbrokerl_mqtt.cpp',
    'brokers/dx_msgbrokerl_kafka.cpp'
]

dxmeta_sources = [
    './../metadata/gst-dxmeta.cpp',
    './../metadata/gst-dxframemeta.cpp',
    './../metadata/gst-dxobjectmeta.cpp',
    './../metadata/gst-dxmsgmeta.cpp'
]
dxmeta_inc = include_directories('./../metadata')

src_files = [
    'tracker/common/src/TrackerFactory.cpp',
    'tracker/OC_SORT/src/Association.cpp',
    'tracker/OC_SORT/src/KalmanBoxTracker.cpp',
    'tracker/OC_SORT/src/KalmanFilter.cpp',
    'tracker/OC_SORT/src/lapjv.cpp',
    'tracker/OC_SORT/src/OCSort.cpp',
    'tracker/OC_SORT/src/Utilities.cpp'
]
tracker_inc = [include_directories('tracker/common/include'), include_directories('tracker/OC_SORT/include')]
utils_inc = [include_directories('utils')]
brokers_inc = [include_directories('brokers')]

gstdxstream = library('gstdxstream',
    dxstream_sources + dxmeta_sources + src_files,
    cpp_args: common_args,
    include_directories: [general_inc, dxmeta_inc] + tracker_inc + utils_inc + brokers_inc,
    dependencies : [opencv_dep, gst_dep, gstbase_dep, dxrt_dep, json_glib_dep, dl_dep, gst_video_dep, libyuv_dep, mosquitto_dep, librdkafka_dep, zlib_dep],
    install : true,
    install_dir : '/usr/lib/' + arch + '-linux-gnu/gstreamer-1.0',
)

dxstream_inc = [include_directories('./')]

dxstream_dep = declare_dependency(
    include_directories: dxstream_inc,
    link_with: [gstdxstream]
)
