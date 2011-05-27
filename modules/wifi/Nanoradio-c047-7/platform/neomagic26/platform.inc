KERNEL_DIR=$(NEOMAGIC26_KERNEL_DIR)
CROSS_COMPILE=$(NEOMAGIC26_CROSS_COMPILE)
KERNEL_MAJOR_VERSION=2.6

HOST_PLATFORM_SRC_DIR=mm6
TRANSPORT=sdio
HOST=arm-linux

# avoid dynamic linkage to old glibc when building with newer GNU tools
LDFLAGS=-static 

PLATFORM_FLAGS=-DCONFIG_MM6PLUS_CLOCK
PLATFORM_FLAGS+=-DNEOMAGIC26

# To work with nrx510A_nrx701B chipset, disable 4-bit SDIO:
# insmod nano_if.ko nrx_4bit_sdio=0
PLATFORM_FLAGS+=-DUSE_4BIT_SDIO
