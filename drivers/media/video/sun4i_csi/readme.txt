===========================================

Version: V1_60

Author:  raymonxiu

Date:     2011-11-8 18:56:31

Description:

newest module list:(X = 0 or 1)
insmod sun4i_csiX.ko ccm="ov7670" i2c_addr=0x42
insmod sun4i_csiX.ko ccm="gc0308" i2c_addr=0x42
insmod sun4i_csiX.ko ccm="gt2005" i2c_addr=0x78
insmod sun4i_csiX.ko ccm="hi704"  i2c_addr=0x60
insmod sun4i_csiX.ko ccm="sp0838" i2c_addr=0x30
insmod sun4i_csiX.ko ccm="mt9m112" i2c_addr=0xba
insmod sun4i_csiX.ko ccm="mt9m113" i2c_addr=0x78
insmod sun4i_csiX.ko ccm="ov2655" i2c_addr=0x60

V1_60
CSI: Support dual sensor at one csi and add new module
1)Support dual sensor at one csi interface 
2)Add flash light io support
3)Modify the pm , set io to hi-z state when standby 
4)Add mt9m113 new module
5)Add ov2655 new module

V1_52 CSI: Update mt9m112 module
1) mt9m112 support VGA@25fps and SXGA@15fps
2) initial iovdd , avdd and dvdd pointer to NULL

V1_51 CSI: Hold the camera reset when enter standby

V1_50 CSI: Fetch "csi_stby_mode" and csi power from sys_config1
1) Fetch "csi_stby_mode","csi_iovdd","csi_avdd","csi_dvdd" from sys_config1
2) Modify the standby power management

V1_40 CSI: Fetch "csi_vflip" and "csi_hflip" from sys_config1, modify Kconfig and Makefile
1) Fetch "csi_vflip" and "csi_hflip" from sys_config1
2) Add vflip and hflip default setting at sensor initial
3) Add a function poiter set_size at sensor_win_size struct 
4) Modify the drivers/media/video/Kconfig and Makefile
5) Add Kconfig and Makefile in sun4i_csi

V1_34
1) Support V4L2_PIX_FMT_NV61 and V4L2_PIX_FMT_NV21 format

V1_33
1) Fix gc0308 y/c seperation bug

V1_32
1) Fix the bug that return error when setting AWB
2) Flip Y in sp0838 module
3) Add mt9m112 module

V1_31
1) Add sp0838 module
2) Fix the bug when using yvyu,uyvy,vyuy and different size in gc0308,hi704

V1_30
1) Add brightness,contrast,etc interface in gc0308 and gt2005
2) Add Hynix hi704 module
3) Fix the bug that the video frame buffer is not 4k aligned when using cedarx reserved memory
4) gc0308 hflip
5) ov7670 add io 

V1_20
1) Fetch the ccm and i2c_addr from sys_config
2) Add White Balance, Exposure, Special Effect interface in gc0308 and gt2005
3) Modify the Power, Standby and Reset control

V1_10
1) Support Dual Sensor
2) Support gc0308 and gt2005 module
3) insmod xxx.ko ccm="xxx" i2c_addr=0xXX
4) Add DMA Contig memory reserved MACRO define
5) Fix the bug that when capture off, the fist_flag should reset to 0

V1_00
1) Support YUV422/YUV420 planar/UV combined YUYV,YVYU,UYVY,VYUY, RAW format  
2) Support OV7670 module
3) V4L2 API standard
4) video device registered name is video0
5) buffer address double buffered with FRAME_DONE signal

