
This distribution includes binary files in the following subdirectories:

firmware/     -- binary images (and their ascii bin-arrays) for download 
                 to the dongle device
firmware_test --
host/         -- linux kernel modules: host drivers for the dongle
apps/         -- tools for dongle download and runtime WLAN commands

A brief overview of how to use these files can be found in ReleaseNotes.html.

Source code for rebuilding the host-side binaries is provided in the src
directory; an overview of that structure is:

src/include	-- various header files
src/shared	-- source files which may be shared by several binaries
src/dongle	-- dongle-specific header
src/dhd/linux	-- build directory for dhd linux module (dhd and sdstd)
src/dhd/sys	-- source code for dhd driver
src/bcmsdio/sys	-- source code for sdstd driver (for standard SDIO host)

To rebuild the dhd kernel module, which incorporates both the dongle host
driver (dhd) and the SDIO host controller driver (sdmmc):
  % cd src/dhd/linux
  % make dhd-cdc-sdmmc-gpl-debug

The resulting module file (dhd.o or dhd.ko) will be placed in a subdirectory
named dhd-cdc-sdstd-<version>, where <version> is the Linux kernel version of
the compiling system.

How to load the driver module on the Linux(Android) system

  1. Copy the driver dhd.ko to the system directory in the system. 
  2. Copy bcm4329.bin and nvram.txt to the system diretory.
  3. Load the driver module as follows:
       cd /system
       insmod ./dhd.ko "firmware_path=/system/bcm4329.bin nvram_path=/system/nvram.txt"
  4. If the driver is loaded and attach to the dongle successfully, then you can find the ethX as follows:
     ls /sys/class/net/


------------------------------------------------------------------------
