# Library source files
libpfcSrcs = files(
    'libpfc.c'
)


# Dependencies
#
# To be reviewed:
#
cc   = meson.get_compiler('c')
mDep = cc.find_library('m', required : false)
#    executable(..., dependencies : m_dep)

libpfcDeps = []
libpfcIncs = [libpfcIncs, kmodIncs]

libpfc = library('pfc', libpfcSrcs,
                 include_directories: libpfcIncs,
                 dependencies:        libpfcDeps,
                 install:             true)


# Demo application
pfcdemoSrcs = files('pfcdemo.c')
pfcdemoDeps = [mDep]

pfcdemo = executable('pfcdemo', pfcdemoSrcs,
                     include_directories: libpfcIncs,
                     link_with:           [libpfc],
                     dependencies:        pfcdemoDeps,
                     install:             true,
                     extra_files:         '../include/libpfc.h')
