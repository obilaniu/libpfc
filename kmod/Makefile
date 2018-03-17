# This variable specifies the kernel name/version string that we'll try to
# build against: normally you never want to set this since just want to pick
# up the current kernel version using uname -r. It's here to allow you to 
# "cross compile" against a different kernel version which is useful on 
# Travis-CI, for example, where the available linux-header versions don't 
# match the actual kernel version
ifndef PFC_KMOD_KERNEL_NAME
PFC_KMOD_KERNEL_NAME = $(shell uname -r)
endif

all:
	make -C /lib/modules/$(PFC_KMOD_KERNEL_NAME)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(PFC_KMOD_KERNEL_NAME)/build M=$(PWD) clean
