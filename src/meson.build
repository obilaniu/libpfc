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



# Utilities
pfcutilSrcs = files('pfcutil.c')
pfcutilDeps = [mDep]
pfcutil = executable('pfcutil', pfcutilSrcs,
                     include_directories: libpfcIncs,
                     link_with:           [libpfc],
                     dependencies:        pfcutilDeps,
                     install:             true)


# Demo applications

# Simple FMA+VPADDD loop
pfcdemoSrcs = files('pfcdemo.c')
pfcdemoDeps = [mDep]

pfcdemo = executable('pfcdemo', pfcdemoSrcs,
                     include_directories: libpfcIncs,
                     link_with:           [libpfc],
                     dependencies:        pfcdemoDeps,
                     install:             true)


# A much more complicated example to analyze the behaviour of cycle counters
# under TurboBoost.
pfcdemoreftscSrcs = files('pfcdemoreftsc.cpp')
pfcdemoreftscDeps = [mDep]

pfcdemoreftsc = executable('pfcdemoreftsc', pfcdemoreftscSrcs,
                           include_directories: libpfcIncs,
                           link_with:           [libpfc],
                           dependencies:        pfcdemoreftscDeps,
                           install:             true)


# Determine the TurboBoost hits.
pfctbhitSrcs = files('pfctbhit.c', 'pfctbhitasm.S')
pfctbhitDeps = [mDep]

pfctbhit = executable('pfctbhit', pfctbhitSrcs,
                     include_directories: libpfcIncs,
                     link_with:           [libpfc],
                     dependencies:        pfctbhitDeps,
                     install:             true)

