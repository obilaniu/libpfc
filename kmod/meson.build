# This folder contains the sources to the kernel module. It is self-contained.

kmodIncs = include_directories('../include')
kmodSrcs = files('pfckmod.c', 'Makefile', 'mesonbuildkmod.sh')


kmod = custom_target('kmod',
  output      : ['pfc.ko'],
  input       : kmodSrcs,
  command     : [meson.source_root()+'/kmod/mesonbuildkmod.sh', meson.source_root(), meson.build_root()],
  install     : true,
  install_dir : 'bin',
  build_always: true
)
