.PHONY : all clean

vpath %.c src
vpath %.h include

CFLAGS += -Wall -Winvalid-pch -O0 -g -Iinclude
SHAREDLIB_FLAGS = -Wl,--no-undefined -Wl,--as-needed -shared -fPIC -Wl,-soname,libpfc.so '-Wl,-rpath,$$ORIGIN/' -Wl,-rpath-link,/home/tdowns/dev/uarch-bench/libpfc/build/src

all : libpfc.so pfcdemo pfc.ko

libpfc.o : libpfc.c libpfc.h
	$(CC) $(CFLAGS) -fPIC -c $< -o libpfc.o

libpfc.so : libpfc.o
	$(CC) $(SHAREDLIB_FLAGS) libpfc.o -o libpfc.so

pfcdemo.o : pfcdemo.c libpfc.h
	$(CC) $(CFLAGS) -c $< -o pfcdemo.o

pfcdemo : pfcdemo.o libpfc.so
	$(CC) '-Wl,-rpath,$$ORIGIN/' -L. pfcdemo.o -lpfc -lm -o pfcdemo

pfc.ko: kmod/pfckmod.c kmod/Makefile
	rm -rf kmod.build
	cp -r kmod kmod.build
	cd kmod.build && $(MAKE) MAKEFLAGS=
	mv kmod.build/pfc.ko .
	rm -rf kmod.build

clean :
	rm -f *.o libpfc.so pfcdemo pfc.ko
	rm -rf kmod.build
