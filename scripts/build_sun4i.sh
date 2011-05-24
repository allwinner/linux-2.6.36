#!/bin/bash
#########################################################################
#
#          Simple build scripts to build krenel(with rootfs)  -- by Benn
#
#########################################################################


if [ ! -e .config ];then
	echo -e "\n\t\tUsing default config... ...!\n"

	if [ "$1" == "debug" ]; then
		echo "build debug kernel"
		cp arch/arm/configs/sun4i_defconfig .config
	else
		echo "build release kernel"
		cp arch/arm/configs/sun4i_defconfig .config
	fi
fi
make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- -j4 uImage
arm-none-linux-gnueabi-objcopy -R .note.gnu.build-id -S -O binary vmlinux vmlinux.bin

cp vmlinux.bin output/bImage
cp arch/arm/boot/zImage output/zImage


#
# Put kernel image and all modules to output dir
#
rm -rf output/bImage
for file in $(find  ! -path output -name "*.ko");
	do mkdir -p output/`dirname $file`;
	cp $file output/$file;
done

cp vmlinux.bin output/bImage
cp arch/arm/boot/zImage output/zImage
cp arch/arm/boot/uImage output/uImage
cp modules.builtin output/ 
cp modules.order output/
cp .config output/config


echo "Building Linux Done"



