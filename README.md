[![Build Status](https://travis-ci.org/obilaniu/libpfc.svg?branch=master)](https://travis-ci.org/obilaniu/libpfc)

# libpfc

_A small library and kernel module for easy access to x86 performance monitor counters under Linux._

## Intro

`libpfc` is both a library and a Linux Loadable Kernel Module (LKM) that permits near-0-overhead use of the performance monitoring capabilities of modern x86 processors.

## Getting the code, building and installing

`libpfc`'s source code can be found on [Github](https://github.com/obilaniu/libpfc). It uses the [Meson 0.41.0](http://mesonbuild.com/) build system, which itself requires [Ninja](https://ninja-build.org/), but itself has no dependencies. It can be build as follows.

```shell
    cd       /path/to/some/folder
    git      clone 'https://github.com/obilaniu/libpfc.git' libpfc
    mkdir    libpfc/build
    cd       libpfc/build
    meson.py ..  -Dbuildtype=release --prefix=/path/to/prefixdir  # Such as $HOME/.local
    ninja
    ninja    install
```

Make sure a path to `libpfc.so` is present in `LD_LIBRARY_PATH`.

## Loading the kernel module `pfc.ko`

The kernel module is installed in `/path/to/prefixdir/bin/pfc.ko`. To load it into the kernel, ensure that no other entity is using the `perf` functionality, including other kernel modules such as NMI watchdogs. As of Linux 4.4.5, this can be done, with root privileges, as follows:

```shell
    modprobe -ar iTCO_wdt iTCO_vendor_support
    echo 0 > /proc/sys/kernel/nmi_watchdog
    echo 2 > /sys/bus/event_source/devices/cpu/rdpmc
    insmod /path/to/prefixdir/bin/pfc.ko
```

## Using `libpfc` in user-space

### Include

`#include "libpfc.h"`

This provides the API as well as two data types, `PFC_CNT` and `PFC_CFG`, which are both 64-bit 2's complement integers.

### Initialize the library

`pfcInit()`

Initializes the library. If the magic files exposed by `pfc.ko` (`/sys/module/pfc/config` and `/sys/module/pfc/counts`) cannot be opened, prints an error message.

### Pin thread to single core

`pfcPinThread(coreNum)`

Pins the thread to one selected core. This is important both because core migration interferes with performance statistics, and because `pfc.ko` does not virtualize or track counter values in any way. It is thus crucial that the process occupies a single core and that no other processor occupy it.

### Parse configurations

`pfcParseCfg()`

Takes a string describing a general-purpose counter event to track, and computes the 64-bit `PFC_CFG` value to be written to the MSR that would cause it to monitor said event.

The fixed-function performance counters are enabled using configuration `2` and disabled with configuration `0`. No other configuration is allowed.

### Read/Write Configs & Counts

```c
    pfcRdCfgs(k, n, cfgs);
    pfcWrCfgs(k, n, cfgs);
    pfcRdCnts(k, n, cnts);
    pfcWrCnts(k, n, cnts);
```

Reads and writes hardware counter and configuration values for the `n` counters starting at counter `k`. On Haswell, counters 0, 1 and 2 are fixed-function counters, while counters 3, 4, 5 and 6 are general-purpose counters.

## Timing Code

`libpfc.h` defines two assembler macros and one function for timing.

- `PFCSTART(cnts)` reads the 7 hardware counters using `rdpmc` in a carefully-timed manner and _subtracts_ them out of the current value in `cnt[0..6]`.
- `PFCEND(cnts)` reads the 7 hardware counters using `rdpmc` in a carefully-timed manner and _adds_ them into `cnt[0..6]`.
- Since `PFCSTART/PFCEND` have non-negligible but identical cost and highly-predictable behaviour, the bias that they introduce in the recorded counter values can be computed and removed.
  
  `pfcRemoveBias(cnts, mul)` measures the cost of a pair `PFCSTART/PFCEND` with nothing in-between with the current counter configurations, and subtracts `mul` copies of that cost out of `cnts`.

Therefore, to measure a snippet of code, one does as follows:

```c
    /* Initialization */
    pfcInit();
    pfcPinThread(3);  /* Ideally, to any core other than 0 */
    
    /* Compute configurations, possibly using `pfcParseConfig()` */
    PFC_CFG cfgs[7] = {...};
    PFC_CNT cnts[7] = {0,0,0,0,0,0,0};
    
    /* Write configurations and starting values */
    pfcWrCfgs(0, 7, cfgs);
    pfcWrCnts(0, 7, cnts);
    
    /**** HOT SECTION ****/
    PFCSTART(cnts);
    /* Snippet to time */
    PFCEND(cnts);
    /**** END HOT SECTION ****/
    
    /* Bias compensation */
    pfcRemoveBias(cnts);
    
    /* Use values in cnts[0..6]. */
```

An example of this process is in [`pfcdemo.c:71`](https://github.com/obilaniu/libpfc/blob/master/src/pfcdemo.c#L71).
