# Check $ARCH and $CROSS_COMPILE before running make
# CROSS_COMPILE should not be set for native compiler
#

ifneq ($(KERNELRELEASE),)

obj-m := hdmi-dev-v2.o

else
KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	make -C $(KDIR) M=$(PWD) modules
	make clean
clean:
	-rm *.mod.c *.o .*.cmd Module.symvers modules.order 2>/dev/null || :
	-rm -rf .tmp_versions 2>/dev/null || :

.PHONY: clean

# : -> true (: -> null operator)
# ${@D} = /run/udev/rules.d
# $? -> exit status of the last command 

loadit: hdmi-dev-v2.ko
	insmod hdmi-dev-v2.ko

unloadit:
	-rmmod hdmi-dev-v2

.PHONY: loadit unloadit

endif
