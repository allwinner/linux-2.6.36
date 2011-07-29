#!/bin/bash
#########################################################################
#
#          Simple build scripts to build krenel(with rootfs)  -- by Benn
#
#########################################################################


#Setup common variables
export ARCH=arm
export CROSS_COMPILE=arm-unknown-linux-uclibcgnueabi-
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

build_standby()
{
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} KDIR=${LICHEE_KDIR} \
	-C ${LICHEE_KDIR}/arch/arm/mach-sun4i/pm/standby all
}
build_kernel()
{
    if [ ! -e .config ]; then
	echo -e "\n\t\tUsing default config... ...!\n"
	cp arch/arm/configs/sun4i-debug_defconfig .config

    fi
    build_standby

    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} uImage modules

    if [ ! -d output ]; then
	mkdir output
    fi

    rm -rf output/*
    ${OBJCOPY} -R .note.gnu.build-id -S -O binary vmlinux output/bImage
    cp -vf arch/arm/boot/[zu]Image output/
    cp .config output/

    mkdir -p ${LICHEE_MOD_DIR}

    for file in $(find drivers sound modules crypto block fs security net -name "*.ko"); do
	cp $file ${LICHEE_MOD_DIR}
    done

    rm -rf output/kernel-source
    scripts/gen_kernel_src.sh output/kernel-source
}

build_modules()
{
    make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
	CONFIG_CHIP_ID=${CONFIG_CHIP_ID} install

    make -C modules/wifi/nano-c047.12 LICHEE_MOD_DIR=${LICHEE_MOD_DIR} KERNEL_DIR=${LICHEE_KDIR} \
	CONFIG_CHIP_ID=${CONFIG_CHIP_ID} HOST=${CROSS_COMPILE} INSTALL_DIR=${LICHEE_MOD_DIR} all install

	make -C modules/wifi/usi-bcm4329/v4.218.248.15/open-src/src/dhd/linux \
			CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
			LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
			INSTALL_DIR=${LICHEE_MOD_DIR} dhd-cdc-sdmmc-gpl-debug
	
	make -C modules/wifi/ar6302/AR6K_SDK_ISC.build_3.1_RC.329/host CROSS_COMPILE=${CROSS_COMPILE} \
			ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
			all install

    #make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} KERNEL_DIR=${LICHEE_KDIR} \
    #    CONFIG_CHIP_ID=${CONFIG_CHIP_ID} HOST=${CROSS_COMPILE} INSTALL_DIR=${LICHEE_MOD_DIR} all

    export LANG=en_US.UTF-8
    unset LANGUAGE

    cd modules/mali/sun4i/DX910-SW-99002-r2p1-05rel1/src/devicedrv/mali
    USING_MMU=1 USING_UMP=0 USING_PMM=0 BUILD=release CONFIG=ca8-virtex820-m400-1 KDIR=${LICHEE_KDIR} make
    cp mali.ko ${LICHEE_MOD_DIR}
    cd -
}

clean_kernel()
{
    make clean
    rm -rf output/*
}

clean_modules()
{
    #make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean
    make -C modules/wifi/nano-c047.12 LICHEE_MOD_DIR=${LICHEE_MOD_DIR} KERNEL_DIR=${LICHEE_KDIR} \
	CONFIG_CHIP_ID=${CONFIG_CHIP_ID} HOST=${CROSS_COMPILE} INSTALL_DIR=${LICHEE_MOD_DIR} clean
	make -C modules/wifi/usi-bcm4329/v4.218.248.15/open-src/src/dhd/linux \
			CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
			LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
			INSTALL_DIR=${LICHEE_MOD_DIR} clean 
	make -C modules/wifi/ar6302/AR6K_SDK_ISC.build_3.1_RC.329/host CROSS_COMPILE=${CROSS_COMPILE} \
			ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
			clean	

}

#####################################################################
#
#                      Main Runtine
#
#####################################################################

LICHEE_ROOT=`(cd ${LICHEE_KDIR}/..; pwd)`

export PATH=${LICHEE_ROOT}/buildroot/output/host/usr/bin:$PATH

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
all)
    build_kernel
    build_modules
    ;;
*)
    show_help
    ;;
esac


