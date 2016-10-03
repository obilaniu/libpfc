# This folder contains the sources to the kernel module. It is self-contained.

kmodSrcs = files('pfckmod.c', 'Makefile', 'mesonbuildkmod.sh')


kmod = custom_target('kmod',
  output      : ['pfc.ko'],
  input       : kmodSrcs,
  command     : [meson.source_root()+'/kmod/mesonbuildkmod.sh', meson.source_root(), meson.build_root()],
  install     : false
)
install_data([meson.build_root()+'/kmod/pfc.ko'], install_dir: 'bin')
