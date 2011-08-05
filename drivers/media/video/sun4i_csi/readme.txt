===========================================

Version: v1_20

Author:  raymonxiu

Date:     2011-8-5 10:49:39

Description:

newest module list:(X = 0 or 1)
insmod sun4i_csiX.ko ccm="ov7670" i2c_addr=0x42
insmod sun4i_csiX.ko ccm="gc0308" i2c_addr=0x42
insmod sun4i_csiX.ko ccm="gt2005" i2c_addr=0x78

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

