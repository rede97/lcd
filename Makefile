ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m  := usblcd.o
usblcd-y := usblcd_probe.o #usblcd_ops.o
else

# ARCH ?= arm
# CROSS_COMPILE ?= arm-linux-gnueabihf-

KDIR ?=  /usr/src/linux-headers-5.10.0-9-amd64/
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean: 
	$(MAKE) -C $(KDIR) M=$(PWD) clean

endif