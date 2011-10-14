#!/bin/bash
set -e
#########################################################################
#
#          Simple build scripts to build krenel(with rootfs)  -- by Benn
#
#########################################################################


#Setup common variables
export ARCH=arm
export CROSS_COMPILE=arm-none-linux-gnueabi-
export AS=${CROSS_COMPILE}as
export LD=${CROSS_COMPILE}ld
export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export NM=${CROSS_COMPILE}nm
export STRIP=${CROSS_COMPILE}strip
export OBJCOPY=${CROSS_COMPILE}objcopy
export OBJDUMP=${CROSS_COMPILE}objdump

#KERNEL_VERSION=`cat include/generated/utsrelease.h |awk -F\" '{print $2}'`
KERNEL_VERSION="2.6.36-android+"

if [ -r include/generated/utsrelease.h ]; then
    KERNEL_VERSION=`cat include/generated/utsrelease.h |awk -F\" '{print $2}'`
fi

LICHEE_KDIR=`pwd`
LICHEE_MOD_DIR=${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}
CONFIG_CHIP_ID=1123

show_help()
{
    printf "Build script for Lichee system\n"
    printf "  Invalid Option:\n"
    printf "  help      - show this help\n"
    printf "  kernel    - build kernel for sun4i\n"
    printf "  modules   - build external modules for sun4i\n"
    printf "  clean     - clean all\n"
    printf "\n"
}

build_kernel()
{
    if [ ! -e .config ]; then
	echo -e "\n\t\tUsing default config... ...!\n"
	cp arch/arm/configs/sun4i_defconfig .config

    fi

    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} uImage modules

    rm -rf output/*
    ${OBJCOPY} -R .note.gnu.build-id -S -O binary vmlinux output/bImage
    cp -vf arch/arm/boot/[zu]Image output/
    cp .config output/

    mkdir -p ${LICHEE_MOD_DIR}

    for file in $(find drivers sound modules crypto block fs security net -name "*.ko"); do
	cp $file ${LICHEE_MOD_DIR}
    done
}

build_modules()
{
    make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
	CONFIG_CHIP_ID=${CONFIG_CHIP_ID} install

    make -C modules/wifi/Nanoradio-c047-7 LICHEE_MOD_DIR=${LICHEE_MOD_DIR} KERNEL_DIR=${LICHEE_KDIR} \
	CONFIG_CHIP_ID=${CONFIG_CHIP_ID} HOST=${CROSS_COMPILE} INSTALL_DIR=${LICHEE_MOD_DIR} all install

    make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} KERNEL_DIR=${LICHEE_KDIR} \
        CONFIG_CHIP_ID=${CONFIG_CHIP_ID} HOST=${CROSS_COMPILE} INSTALL_DIR=${LICHEE_MOD_DIR} all install
}

clean_kernel()
{
    make clean
    rm -rf output/*
}

clean_modules()
{
    make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean
    make -C modules/wifi/Nanoradio-c047-7 LICHEE_MOD_DIR=${LICHEE_MOD_DIR} KERNEL_DIR=${LICHEE_KDIR} \
	CONFIG_CHIP_ID=${CONFIG_CHIP_ID} HOST=${CROSS_COMPILE} INSTALL_DIR=${LICHEE_MOD_DIR} clean
}

#####################################################################
#
#                      Main Runtine
#
#####################################################################
case "$1" in
kernel)
    build_kernel
    ;;
modules)
    build_modules
    ;;
clean)
    clean_kernel
    clean_modules
    ;;
*)
    show_help
    ;;
esac


