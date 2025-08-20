conf_data = configuration_data()
conf_data.set('VERSION', meson.project_version())

configure_file(
  input : 'version.h.in',
  output : 'version.h',
  configuration : conf_data,
)

include_dirs += include_directories('.')

