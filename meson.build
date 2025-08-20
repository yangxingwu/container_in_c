project('barcov2', 'c',
  version : '0.1',
  default_options : {
    'warning_level' : '3',
    'default_library' : 'static',
  })

argtable3_project = subproject('argtable3', required: true)
argtable3_dependency = argtable3_project.get_variable('argtable3_dep')

include_dirs = []

subdir('include')
subdir('libs')
subdir('src')
