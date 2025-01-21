project('mqtt_sub_example', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

mosquitto_dep = dependency('libmosquitto')
json_glib_dep = dependency('json-glib-1.0', version : '>= 1.0')

mqtt_sub = executable(
  'mqtt_sub_example',
  'mqtt_sub_example.cpp',
  dependencies: [mosquitto_dep, json_glib_dep],
  install: true,
  install_dir: '/usr/share/dx-stream/bin'
)

install_data(
  ['mqtt_sub_example.py', 'mqtt_sub_example_frame.py'],
  install_dir: '/usr/share/dx-stream/bin'
)
