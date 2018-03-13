# Makefile for libpfc userland and kernel components.
# The preferred build method for libpfc is the combination of meson and ninja, and so this Makefile
# may not always be up to date. It is provided for projects that prefer make-only builds.

.PHONY : all clean

vpath %.c src
vpath %.h include

ifeq ($(CC), pgcc)
CFLAGS += -O0 -g -Iinclude -c99
else
CFLAGS += -Wall -Winvalid-pch -O0 -g -Iinclude -std=gnu99
endif

SHAREDLIB_FLAGS = -Wl,--no-undefined -Wl,--as-needed -shared -fPIC -Wl,-soname,libpfc.so '-Wl,-rpath,$$ORIGIN/'

all : libpfc.so pfcdemo pfc.ko

libpfc.o : libpfc.c libpfc.h libpfcmsr.h
	$(CC) $(CFLAGS) -fPIC -c $< -o libpfc.o

libpfc.so : libpfc.o
	$(CC) $(SHAREDLIB_FLAGS) libpfc.o -o libpfc.so

pfcdemo.o : pfcdemo.c libpfc.h
	$(CC) $(CFLAGS) -c $< -o pfcdemo.o

pfcdemo : pfcdemo.o libpfc.so
	$(CC) '-Wl,-rpath,$$ORIGIN/' -L. pfcdemo.o -lpfc -lm -o pfcdemo

pfc.ko : kmod/pfckmod.c kmod/Makefile
	rm -rf kmod.build
	cp -r kmod kmod.build
	cd kmod.build && $(MAKE) MAKEFLAGS=
	mv kmod.build/pfc.ko .
	rm -rf kmod.build

clean :
	rm -f *.o libpfc.so pfcdemo pfc.ko
	rm -rf kmod.build
