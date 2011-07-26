/*
**************************************************************************************************************
*											         eLDK
*						            the Easy Portable/Player Develop Kits
*									           desktop system 
*
*						        	 (c) Copyright 2009-2012, ,HUANGXIN China
*											 All Rights Reserved
*
* File    	: dma_15.h
* By      	: HUANGXIN
* Func		: 
* Version	: v1.0
* ============================================================================================================
* 2011-5-9 10:36:54  HUANGXIN create this file, implements the fundemental interface;
**************************************************************************************************************
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>

#include <mach/dma.h>
#include <mach/system.h>

static struct sw_dma_map __initdata sw_dma_mappings[DMACH_MAX] = {
	[DMACH_NSPI0] = {
		.name		= "spi0",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
				0,0,0,0,0,0,0,0,},
	},
	[DMACH_NSPI1] = {
		.name		= "spi1",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
				0,0,0,0,0,0,0,0,},
	},
	[DMACH_NSPI2] = {
	.name		= "spi2",
	.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NSPI3] = {
	.name		= "spi3",
	.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NUART0] = {
		.name		= "uart0",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NUART1] = {
		.name		= "uart1",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NUART2] = {
		.name		= "uart2",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NUART3] = {
		.name		= "uart3",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NUART4] = {
		.name		= "uart4",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NUART5] = {
		.name		= "uart5",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NUART6] = {
		.name		= "uart6",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NUART7] = {
		.name		= "uart7",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NSRAM] = {
		.name		= "nsram",
		.channels = {DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,0,0,0,0,0,0,0,0},
	},
	[DMACH_NSDRAM] = {
		.name		= "nsdram",
		.channels = {DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,0,0,0,0,0,0,0,0},
	},
	[DMACH_NTPAD] = {
		.name		= "tpadc",
		.channels = {DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,0,0,0,0,0,0,0,0},
	},
	[DMACH_NADDA_PLAY] = {
		.name		= "adda_play",
		.channels = {DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,
				0,0,0,0,0,0,0,0,},
	},
	[DMACH_NADDA_CAPTURE] = {
		.name		= "adda_capture",
		.channels = {DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,
				0,0,0,0,0,0,0,0,},
	},
  	[DMACH_NIIS] = {
		.name		= "iis",
		.channels = {DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,
				0,0,0,0,0,0,0,0,},
	},
	[DMACH_NIR0] = {
		.name		= "ir0",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NIR1] = {
		.name		= "ir1",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NSPDIF] = {
		.name		= "spdif",
		.channels = {DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,
				0,0,0,0,0,0,0,0,},
	},
	[DMACH_NAC97] = {
		.name		= "ac97",
		.channels = {DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID,
			0,0,0,0,0,0,0,0,},
	},
	[DMACH_NHDMI] = {
		.name		= "hdmi",
		.channels = {DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,DMA_CH_VALID,
				0,0,0,0,0,0,0,0,},
	},
	[DMACH_DSRAM] = {
		.name		= "dsram",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
		},
	[DMACH_DSDRAM] = {
		.name		= "dsdram",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
		},
	[DMACH_DPATA] = {
		.name		= "dpata",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
		},
	[DMACH_DNAND] = {
		.name		= "dnand",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
		},
	[DMACH_DUSB0] = {
		.name		= "usb0",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
		},			
	[DMACH_DEMACR] = {
		.name		= "EMACRX_DMA",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},	
	[DMACH_DEMACT] = {
		.name		= "EMACTX_DMA",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_DSPI1] = {
		.name		= "dspi1",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_DSSR] = {
		.name		= "dssr",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_DSST] = {
		.name		= "dsst",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_TCON0] = {
		.name		= "tcon0",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_TCON1] = {
		.name		= "tcon1",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_HDMIAUDIO] = {
		.name		= "hdmiaudio",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_DMS] = {
		.name		= "dms",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_DSPI0] = {
		.name		= "dspi0",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_DSPI2] = {
		.name		= "dspi2",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
	[DMACH_DSPI3] = {
		.name		= "dspi3",
		.channels = {0,0,0,0,0,0,0,0,
			DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID, DMA_CH_VALID},
	},
};

static struct sw_dma_selection __initdata sw_dma_sel = {
	.dcon_mask	= 0xffffffff,
	.map		= sw_dma_mappings,
	.map_size	= ARRAY_SIZE(sw_dma_mappings),
};

static int __init sw_dma_add(struct sys_device *sysdev)
{
	sw15_dma_init();
	return sw_dma_init_map(&sw_dma_sel);
}

static struct sysdev_driver sw_dma_driver = {
	.add	= sw_dma_add,
};

static int __init sw_dma_drvinit(void)
{
	return sysdev_driver_register(&sw_sysclass, &sw_dma_driver);
}

arch_initcall(sw_dma_drvinit);


