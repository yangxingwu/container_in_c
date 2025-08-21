src_files = [
  'main.c',
  'mount.c',
  'user.c',
  'cgroupsv2.c',
  'sec.c',
  'container.c',
]

executable('barcov2', src_files,
  dependencies : deps,
  link_with: [log_lib],
  include_directories: include_dirs,
  install : true)
