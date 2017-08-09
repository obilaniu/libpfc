# Library source files
libpfcSrcs = files(
    'libpfc.c'
)


# Dependencies
#
# To be reviewed:
#
cc   = meson.get_compiler('c')
mDep = cc .find_library('m', required : false)

libpfcDeps = [mDep]
libpfcIncs = [libpfcIncs, kmodIncs]

libpfc = library('pfc', libpfcSrcs,
                 include_directories: libpfcIncs,
                 dependencies:        libpfcDeps,
                 install:             true)


# Demo applications
pfcdemoSrcs = files('pfcdemo.c')
pfcdemoDeps = [mDep]

pfcdemo = executable('pfcdemo', pfcdemoSrcs,
                     include_directories: libpfcIncs,
                     link_with:           [libpfc],
                     dependencies:        pfcdemoDeps,
                     install:             true)


reftscdemoSrcs = files('reftscdemo.cpp')
reftscdemoDeps = [mDep]

reftscdemo = executable('reftscdemo', reftscdemoSrcs,
                        include_directories: libpfcIncs,
                        link_with:           [libpfc],
                        dependencies:        reftscdemoDeps,
                        install:             true)

