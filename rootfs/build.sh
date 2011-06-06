#!/bin/bash

#Execute this within fakeroot
#arg1 - the new rootfs name
regenerate_rootfs()
{
    mkdir tmp;

    #Extract the rootfs to the tmp dir
    gzip -dc sun4i_rootfs.cpio.gz |(cd tmp; cpio -iv)

    #Copy skel to the tmp dir
    (cd skel; tar -c *) |tar -C tmp -x

    #Re-generate the built-in rootfs
    (cd tmp; find . |cpio -o -Hnewc |gzip > ../$1)

    rm -rf tmp
}

if [ -z "$1" ]; then
echo "Please input the new rootfs name"
else
regenerate_rootfs $1
fi

