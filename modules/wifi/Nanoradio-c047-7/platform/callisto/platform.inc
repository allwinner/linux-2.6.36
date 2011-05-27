KERNEL_DIR=/home/nanoradio/ANRD_MAIN/Pilot/DEV/Project/Callisto/Android/out/target/product/GT-I5510/obj/KERNEL_OBJ
CROSS_COMPILE=/home/nanoradio/ANRD_MAIN/Pilot/DEV/Project/Callisto/Android/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-
KERNEL_MAJOR_VERSION=2.6
export ARCH=arm

SDIO_HOST=qrz
TRANSPORT=ksdio
HOST=arm-none-linux-gnueabi

PLATFORM_FLAGS += -D__linux__
PLATFORM_FLAGS += -DKSDIO_FW_CHUNK_LEN=512
PLATFORM_FLAGS += -DKSDIO_NO_BLOCK_MODE
PLATFORM_FLAGS += -DKSDIO_HOST_WAKEUP_PIN=19
# PLATFORM_FLAGS += -DKSDIO_HOST_RESET_PIN=78
PLATFORM_FLAGS += -DWORKAROUND_MISSING_DATA_HT_RATE_SUPPORT

PLATFORM_OBJ=kernel/ksdio/callisto/host.o


