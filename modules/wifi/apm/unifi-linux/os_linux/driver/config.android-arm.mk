ARCH := arm
TMP_DIR := $(PWD)
SDIO_DRIVER := mmc
#KDIR := $(TMP_DIR)/kernel_imx
#CROSS_COMPILE := $(TMP_DIR)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-
#CROSS_COMPILE ?= arm-none-linux-gnueabi-
#KDIR :=~/work/trunk/kernel/linux-v2.6.36.4/
#KDIR ?=/home/myeh/workdir/lichee/linux-2.6.36
EXTRA_DRV_CFLAGS := -DUNIFI_NET_NAME=\"wlan\" -DANDROID_BUILD
export EXTRA_DRV_CFLAGS

include config.generic.mk
