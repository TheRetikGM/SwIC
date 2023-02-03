project('SwIC', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

deps = [
  dependency('imguiwrapper', default_options : [ 'nlohmann_json=enabled' ])
]

src = [
  'main.cpp',
  'device_manager.cpp'
]

executable('swic', src, dependencies : deps)
