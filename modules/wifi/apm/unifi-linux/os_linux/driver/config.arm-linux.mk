ARCH := arm
CROSS_COMPILE := arm-none-linux-gnueabi-

KDIR :=~/work/trunk/kernel/linux-v2.6.36.4/
ifeq ($(KDIR),)
$(error Set KDIR to the kernel tree to build against)
endif

include config.generic.mk
