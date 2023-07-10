project('SwIC', 'cpp',
  version : '0.2.4',
  license : 'MIT',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

deps = [
  dependency('imguiwrapper', default_options : [ 'nlohmann_json=enabled' ])
]

src = files(
  './src/main.cpp',
  './src/device_manager.cpp',
  './src/config.cpp',
  './src/gui/gui.cpp',
  './src/gui/DeviceEditor.cpp',
  './src/gui/MenuBar.cpp',
)

executable('swic', src, dependencies : deps)

