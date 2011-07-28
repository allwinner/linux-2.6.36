#!/bin/bash

###############################################################################
# Description:
#   To generates kernel source, so that our customer can build kmodule without
# kernel source
#
# Arguments:
#   $1 : Install path
#
##############################################################################

INSTALL_PATH=$1


if [ -z $1 ]; then
	printf "\n\tUsage: ./gen_kernel_src \$KERNEL_SOURCE_INSTALL_PATH\n\n"
	exit 1
fi

if [ -d $INSTALL_PATH ]; then
	read -p "Delete all files under $INSTALL_PATH ? (N/y)"

	case $REPLY in
	y)
		rm -rf $INSTALL_PATH/*
		;;
	N)
		;;
	*)
		;;
	esac
else
	mkdir -p $INSTALL_PATH
fi

for item in $(find . -name "Kconfig*" -prune -o -name "Makefile*"); do
	DIRNAME=`dirname $item`
	mkdir -p $INSTALL_PATH/$DIRNAME
	cp $item $INSTALL_PATH/$item
done

for item in $(find arch/ -type d -name include); do
	mkdir -p $INSTALL_PATH/$item
	cp -rf $item/* $INSTALL_PATH/$item/
done

cp Makefile* Module.symvers System.map modules.* vmlinux .config $INSTALL_PATH/
cp -r scripts tools include $INSTALL_PATH/

echo "Done"



