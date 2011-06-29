
==========================================
compile driver guide
==========================================
1. modify platform information in platform/aw16xx/platform.inc
	a. define KERNEL_DIR=PATH_OF_LINUX_KERNEL
	b. define CROSS_COMPILE=your-cross-compiler-prefix
	c. define HOST=your-cross-compiler-prefix

	eg:
	KERNEL_DIR=xxx/linux-2.6.36
	CROSS_COMPILE=arm-none-linux-gnueabi-
	HOST=arm-none-linux-gnueabi-

2. make driver with specified platform

	eg: make PLATFORM=aw16xx

3. if you do not want define these variables in file platfrom.inc,
	you can also define them in make argument list.
	
	eg: make PLATFORM=aw16xx KERNEL_DIR=PATH_OF_LINUX_KERNEL CROSS_COMPILE=arm-none-linux-gnueabi-

4. You can define your install destination directory for *.ko files and firmware, 
   with argument INSTALL_DIR, driver files and firmware will be copyed to your 
   INSTALL_DIR  when you do "make install" operation. Without this argument, driver 
   files will be generated in ./obj dir by default.

	eg: make PLATFORM=aw16xx KERNEL_DIR=PATH_OF_LINUX_KERNEL CROSS_COMPILE=arm-none-linux-gnueabi- install

5. After compiling driver files, nano_if.ko and nano_ksdio.ko will be generated, 
   which will be used in insmoding drivers

6. x_mac.axf is firmware running in WIFI card, you should put it in somewhere of system

============================================
install driver in OS guide
============================================

1. insmod nano_if.ko nrx_config=xxx nrx_ifname=abc 
   (xxx is the path of firmware and abc is the interface name you want to specify)
2. insmod nano_ksdio.ko

when insmod these two driver files, the console will show some infomation as below

# ls
nano_if.ko 		nano_ksdio.ko 		x_mac.axf
# insmod nano_if.ko nrx_config=./ nrx_ifname=wlan0
# insmod nano_ksdio.ko 
[   57.360000] mmc1:0001:1: sdio_nrx_probe (class 0, vendor 03bb, device 200a), target is NRX600
[   57.370000] mmc1:0001:1: Clock 50MHz
[   86.100000] nano_ksdio mmc1:0001:1: Registered interface wlan0

then you can use wireless tools like iwconfig or wpa_supplicant to configure "wlan0", 
scan wireless network and associate APs...

3. If you did not specify interface name(nrx_ifname) when insmod nano_if.ko, default 
   "wlan0" will be used.
   If you did not specify firmware path(nrx_config) when insmod nano_if.ko, driver will probe
   card only, you can cat firmware into frimware filenode with proc file system like this:

# ls
nano_if.ko      nano_ksdio.ko       x_mac.axf
# insmod nano_if.ko
# insmod nano_ksdio.ko 
[   57.360000] mmc1:0001:1: sdio_nrx_probe (class 0, vendor 03bb, device 200a), target is NRX600
[   57.370000] mmc1:0001:1: Clock 50MHz
#ls /proc/drivers/wlan0
config    counters  mib       scan      status
core      firmware  registry  ssid
#cat x_mac.axf > /proc/drivers/wlan0/fimware
[   86.100000] nano_ksdio mmc1:0001:1: Registered interface wlan0





