project('kafka_consume_example', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

librdkafka_dep = dependency('rdkafka')
json_glib_dep = dependency('json-glib-1.0', version : '>= 1.0')

# Kafka Consumer example
kafka_consume = executable(
  'kafka_consume_example',
  'kafka_consume_example.cpp',
  dependencies: [librdkafka_dep, json_glib_dep],
  install: true,
  install_dir: '/usr/share/dx-stream/bin'
)

install_data(
  ['kafka_consume_example.py', 'kafka_consume_example_frame.py'],
  install_dir: '/usr/share/dx-stream/bin'
)
