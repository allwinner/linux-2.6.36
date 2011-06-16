How to add files to the built-in rootfs?

1. Add your new files to the skel/ dir

2. Run the following command to re-generate the rootfs

   $fakeroot ./build.sh sun4i_rootfs.cpio.gz

3. Don't forget to rebuild your linux kernel

4. If you want to generate a totally new rootfs, put all your new rootfs in skel/ dir, delete sun4i_rootfs.cpio.gz and repeat 2 and 3


