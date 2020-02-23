# Check $ARCH and $CROSS_COMPILE before running make
# CROSS_COMPILE should not be set for native compiler
#

ifneq ($(KERNELRELEASE),)

obj-m := src/hdmi-dev-w-gpio.o

else
KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)/src

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

loadit: src/hdmi-dev-w-gpio.ko
	insmod src/hdmi-dev-w-gpio.ko

unloadit:
	-rmmod src/hdmi-dev-w-gpio

.PHONY: loadit unloadit

endif
