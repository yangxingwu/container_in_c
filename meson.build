project('barcov2', 'c',
  version : '0.1',
  default_options : {
    'warning_level' : '3',
    'default_library' : 'static',
  })

argtable3_project = subproject('argtable3', required: true)
argtable3_dependency = argtable3_project.get_variable('argtable3_dep')

libseccomp_dependency = dependency('libseccomp', required: false)
if not libseccomp_dependency.found()
  error('libseccomp not found')
endif

libcap_dependency = dependency('libcap', required: false)
if not libcap_dependency.found()
  error('libcap not found')
endif

deps = [
  argtable3_dependency,
  libseccomp_dependency,
  libcap_dependency,
]

include_dirs = []

subdir('include')
subdir('libs')
subdir('src')
