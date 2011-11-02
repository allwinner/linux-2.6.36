#
# Make configuration file for 3D Labs DMS-05 EVM with
# mobstor SDIO driver.
#
# Assumes that the mobstor package are unpacked in /opt/s5toolsl21/mobstor
# and the mobstor header files are in /opt/s5toolsl21/mobstor/src
#
ARCH := arm
CROSS_COMPILE := /opt/s5toolsl21/lx_eabi/bin/arm-3d-linux-gnueabi-
KDIR := /opt/s5toolsl21/evmrelr2_eabi/release/src.build/

MOBSTOR_DIR := /opt/s5toolsl21/mobstor/src
export MOBSTOR_DIR

#EXTRA_DRV_CFLAGS := -DUNIFI_NET_NAME=\"wifi\"
#export EXTRA_DRV_CFLAGS

SDIO_DRIVER := mobstor

include config.generic.mk
