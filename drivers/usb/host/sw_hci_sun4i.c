/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_xhci_sun4i.c
*
* Author 		: yangnaitian
*
* Description 	: Include file for AW1623 HCI Host Controller Driver
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*    yangnaitian      2011-5-24            1.0          create this file
*
*************************************************************************************
*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/dma-mapping.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/irqs.h>

#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

#include  "sw_hci_sun4i.h"

//---------------------------------------------------------------
//  EHCI0
//---------------------------------------------------------------

#define  SW_EHCI0_NAME		"sw-ehci0"
static const char ehci0_name[] = SW_EHCI0_NAME;

/*
static struct resource sw_ehci0_resources[] = {
	[0] = {
		.start		= SW_USB1_BASE,
		.end		= (0x1000 - 1),
		.flags		= IORESOURCE_MEM,
	},

	[1] = {
		.start		= SW_SRAM_BASE,
		.end		= (SW_SRAM_BASE + SW_SRAM_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},

	[2] = {
		.start		= SW_CCMU_BASE,
		.end		= (SW_CCMU_BASE + SW_CCMU_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},

	[3] = {
		.start		= SW_GPIO_BASE,
		.end		= (SW_GPIO_BASE + SW_GPIO_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},

	[4] = {
		.start		= SW_SDRAM_BASE,
		.end		= (SW_SDRAM_BASE + SW_SDRAM_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},
	
	[5] = {
		.start		= SW_HCI0_PASS_BY_BASE,
		.end		= (SW_HCI0_PASS_BY_BASE + SW_HCI0_PASS_BY_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},
};
*/

static u64 sw_ehci0_dmamask = DMA_BIT_MASK(32);

static struct platform_device sw_usb_ehci0_device = {
	.name		= ehci0_name,
	.id			= -1,
	.dev 		= {
		.dma_mask			= &sw_ehci0_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},

//	.num_resources	= ARRAY_SIZE(sw_ehci0_resources),
//	.resource		= sw_ehci0_resources,
};

//---------------------------------------------------------------
//  OHCI0
//---------------------------------------------------------------
#define  SW_OHCI0_NAME		"sw-ohci0"
static const char ohci0_name[] = SW_OHCI0_NAME;

/*
static struct resource sw_ohci0_resources[] = {
	[0] = {
		.start		= SW_USB1_BASE,
		.end		= (0x1000 - 1),
		.flags		= IORESOURCE_MEM,
	},

	[1] = {
		.start		= SW_SRAM_BASE,
		.end		= (SW_SRAM_BASE + SW_SRAM_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},

	[2] = {
		.start		= SW_CCMU_BASE,
		.end		= (SW_CCMU_BASE + SW_CCMU_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},

	[3] = {
		.start		= SW_GPIO_BASE,
		.end		= (SW_GPIO_BASE + SW_GPIO_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},

	[4] = {
		.start		= SW_SDRAM_BASE,
		.end		= (SW_SDRAM_BASE + SW_SDRAM_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},
	
	[5] = {
		.start		= SW_HCI0_PASS_BY_BASE,
		.end		= (SW_HCI0_PASS_BY_BASE + SW_HCI0_PASS_BY_BASE_LEN - 1),
		.flags		= IORESOURCE_MEM,
	},
};
*/

static u64 sw_ohci0_dmamask = DMA_BIT_MASK(32);

static struct platform_device sw_usb_ohci0_device = {
	.name		= ohci0_name,
	.id			= -1,
	.dev 		= {
		.dma_mask			= &sw_ohci0_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},

//	.num_resources	= ARRAY_SIZE(sw_ohci0_resources),
//	.resource		= sw_ohci0_resources,
};


/*
*******************************************************************************
*                     sw_xhci_sun4i_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static int __init sw_hci0_sun4i_init(void)
{
	platform_device_register(&sw_usb_ehci0_device);
    platform_device_register(&sw_usb_ohci0_device);

    return 0;
}

/*
*******************************************************************************
*                     usb_manager_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void __exit sw_hci0_sun4i_exit(void)
{
   
  	platform_device_unregister(&sw_usb_ohci0_device);
	platform_device_unregister(&sw_usb_ehci0_device);
	
    return ;
}

module_init(sw_hci0_sun4i_init);
module_exit(sw_hci0_sun4i_exit);

