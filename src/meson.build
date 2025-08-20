src_files = [
  'main.c',
  'mount.c',
  'user.c',
]

executable('barcov2', src_files,
  dependencies : argtable3_dependency,
  link_with: [log_lib],
  include_directories: include_dirs,
  install : true)
