install:

	echo "uudecode /dev/ttyDB" > /dev/ttyS0
	uuencode /target/root/4.9.10/x_mac-4.9.5.bin x_mac-4.9.5.bin > /dev/ttyS0

	echo "uudecode /dev/ttyDB" > /dev/ttyS0
	uuencode /home/angr/nanoradio/4.9/HostDriver/linux/kernel/sdio/nano_sdio.ko nano_sdio.ko > /dev/ttyS0

	echo "uudecode /dev/ttyDB" > /dev/ttyS0
	uuencode /home/angr/nanoradio/4.9/HostDriver/linux/kernel/ic/nano_if.ko nano_if.ko > /dev/ttyS0

#	echo "uudecode /dev/ttyDB" > /dev/ttyS0
#	uuencode /home/angr/nanoradio/4.9/HostDriver/linux/userspace/libnrx/autotest/nrxpriv nrxpriv > /dev/ttyS0
	
	
