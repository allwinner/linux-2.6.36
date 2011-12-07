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
KERNEL_VERSION="2.6.36-android"

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
	cp arch/arm/configs/sun4i_defconfig .config

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
    cp rootfs/sun4i_rootfs.cpio.gz output/

    mkdir -p ${LICHEE_MOD_DIR}
 
          mkbootimg --kernel output/bImage \
                          --ramdisk output/sun4i_rootfs.cpio.gz \
                          --board 'sun4i' \
                          --base 0x40000000 \
                          -o output/boot.img

    

    for file in $(find drivers sound modules crypto block fs security net -name "*.ko"); do
	cp $file ${LICHEE_MOD_DIR}
    done
    cp -f Module.symvers modules.* ${LICHEE_MOD_DIR}

    #rm -rf output/kernel-source
    #scripts/gen_kernel_src.sh output/kernel-source
    
    #copy bcm4330 firmware and nvram.txt
    cp drivers/net/wireless/bcm4330/firmware/bcm4330.bin ${LICHEE_MOD_DIR}
    cp drivers/net/wireless/bcm4330/firmware/bcm4330.hcd ${LICHEE_MOD_DIR}
    cp drivers/net/wireless/bcm4330/firmware/nvram.txt ${LICHEE_MOD_DIR}/bcm4330_nvram.txt
}

build_modules()
{
    make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
	CONFIG_CHIP_ID=${CONFIG_CHIP_ID} install
	
	#build swl-n20 sdio wifi module
    make -C modules/wifi/nano-c047.12 LICHEE_MOD_DIR=${LICHEE_MOD_DIR} KERNEL_DIR=${LICHEE_KDIR} \
	CONFIG_CHIP_ID=${CONFIG_CHIP_ID} HOST=${CROSS_COMPILE} INSTALL_DIR=${LICHEE_MOD_DIR} all install

	#build usi-bmc4329 sdio wifi module
	make -C modules/wifi/usi-bcm4329/v4.218.248.15/open-src/src/dhd/linux \
			CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
			LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
			INSTALL_DIR=${LICHEE_MOD_DIR} dhd-cdc-sdmmc-gpl 
	
	#build huawei-mw269 sdio wifi module
	make -C modules/wifi/hw-mw269/v4.218.248.27/open-src/src/dhd/linux \
			CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
			LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
			INSTALL_DIR=${LICHEE_MOD_DIR} dhd-cdc-sdmmc-gpl
	
	#build ar6302 sdio wifi module
	make -C modules/wifi/ar6302/AR6K_SDK_ISC.build_3.1_RC.329/host CROSS_COMPILE=${CROSS_COMPILE} \
	        ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
	        all install
    
	#build ar6003 sdio wifi module
	make -C modules/wifi/ar6003/AR6kSDK.build_3.1_RC.514/host CROSS_COMPILE=${CROSS_COMPILE} \
	        ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
	        all
    
    #build apm sdio wifi module
    make -C modules/wifi/apm/unifi-linux/os_linux/driver/ CONFIG=android-arm ARCH=arm KDIR=${LICHEE_KDIR} \
            CROSS_COMPILE=${CROSS_COMPILE} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR}
	
    #make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} KERNEL_DIR=${LICHEE_KDIR} \
    #    CONFIG_CHIP_ID=${CONFIG_CHIP_ID} HOST=${CROSS_COMPILE} INSTALL_DIR=${LICHEE_MOD_DIR} all install
    export LANG=en_US.UTF-8
    unset LANGUAGE

    cd modules/mali/sun4i/DX910-SW-99002-r2p1-05rel1/src/devicedrv/ump
    CONFIG=ca8-virtex820-m400-1 KDIR=${LICHEE_KDIR} make
    cp ump.ko ${LICHEE_MOD_DIR}
    
    cd ../mali
    #cd modules/mali/sun4i/DX910-SW-99002-r2p1-05rel1/src/devicedrv/mali
    USING_MMU=1 USING_UMP=1 USING_PMM=1 BUILD=release CONFIG=ca8-virtex820-m400-1 KDIR=${LICHEE_KDIR} make
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
    make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean

	#clean swl-n20 sdio wifi module
    make -C modules/wifi/nano-c047.12 LICHEE_MOD_DIR=${LICHEE_MOD_DIR} KERNEL_DIR=${LICHEE_KDIR} \
		CONFIG_CHIP_ID=${CONFIG_CHIP_ID} HOST=${CROSS_COMPILE} INSTALL_DIR=${LICHEE_MOD_DIR} clean

	#cledn usi bcm4329 wifi module
    make -C modules/wifi/usi-bcm4329/v4.218.248.15/open-src/src/dhd/linux \
		CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
		LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
		INSTALL_DIR=${LICHEE_MOD_DIR} clean 
	
	#clean huawei-mw269 sdio wifi module
	make -C modules/wifi/hw-mw269/v4.218.248.27/open-src/src/dhd/linux \
			CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
			LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
			INSTALL_DIR=${LICHEE_MOD_DIR} clean
			
	#clean ar6302 sdio wifi module
	make -C modules/wifi/ar6302/AR6K_SDK_ISC.build_3.1_RC.329/host CROSS_COMPILE=${CROSS_COMPILE} \
		ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
		clean

	#build ar6003 sdio wifi module
	make -C modules/wifi/ar6003/AR6kSDK.build_3.1_RC.514/host CROSS_COMPILE=${CROSS_COMPILE} \
	        ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
	        clean
    
    #clean apm sdio wifi module
    make -C modules/wifi/apm/unifi-linux/os_linux/driver/ CONFIG=android-arm ARCH=arm KDIR=${LICHEE_KDIR} \
            CROSS_COMPILE=${CROSS_COMPILE} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} clean

}

#####################################################################
#
#                      Main Runtine
#
#####################################################################

LICHEE_ROOT=`(cd ${LICHEE_KDIR}/..; pwd)`
export PATH=${LICHEE_ROOT}/buildroot/output/external-toolchain/bin:${LICHEE_ROOT}/tools/pack/pctools/linux/android:$PATH

case "$1" in
standby)
    build_standby
    ;;
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


