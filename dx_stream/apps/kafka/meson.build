project('kafka_consume_example', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++14'])

librdkafka_dep = dependency('rdkafka')
json_glib_dep = dependency('json-glib-1.0', version : '>= 1.0')
opencv_dep = dependency('opencv4', required: true)

# Kafka Consumer example
kafka_consume = executable(
  'kafka_consume_example',
  'kafka_consume_example.cpp',
  dependencies: [librdkafka_dep, json_glib_dep, opencv_dep],
  install: true,
  install_dir: get_option('datadir') / 'gstdxstream' / 'bin'
)

install_data(
  ['kafka_consume_example.py', 'README.md'],
  install_dir: get_option('datadir') / 'gstdxstream' / 'bin'
)
