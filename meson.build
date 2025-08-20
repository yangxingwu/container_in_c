project('barcov2', 'c',
  version : '0.1',
  default_options : {
    'warning_level' : '3',
    'default_library' : 'static',
  })


argtable3_project = subproject('argtable3', required: true)
argtable3_dependency = argtable3_project.get_variable('argtable3_dep')

conf_data = configuration_data()
conf_data.set('VERSION', meson.project_version())

configure_file(
  input : 'version.h.in',
  output : 'version.h',
  configuration : conf_data
)

subdir('libs')

exe = executable('barcov2', 'barcov2.c',
  dependencies : argtable3_dependency,
  link_with: [log_lib],
  include_directories: [lib_includes],
  install : true)

test('basic', exe)
