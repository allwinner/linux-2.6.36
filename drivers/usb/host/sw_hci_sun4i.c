/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_hci_sun4i.c
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
//  EHCI
//---------------------------------------------------------------

#define  SW_EHCI_NAME		"sw-ehci"
static const char ehci_name[] = SW_EHCI_NAME;

static struct sw_hci_hcd sw_ehci0;
static struct sw_hci_hcd sw_ehci1;

static u64 sw_ehci_dmamask = DMA_BIT_MASK(32);

static struct platform_device sw_usb_ehci_device[] = {
	[0] = {
		.name		= ehci_name,
		.id			= 1,
		.dev 		= {
			.dma_mask			= &sw_ehci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sw_ehci0,
		},
	},

	[1] = {
		.name		= ehci_name,
		.id			= 2,
		.dev 		= {
			.dma_mask			= &sw_ehci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sw_ehci1,
		},
	},
};

//---------------------------------------------------------------
//  OHCI
//---------------------------------------------------------------
#define  SW_OHCI_NAME		"sw-ohci"
static const char ohci_name[] = SW_OHCI_NAME;

static struct sw_hci_hcd sw_ohci0;
static struct sw_hci_hcd sw_ohci1;

static u64 sw_ohci_dmamask = DMA_BIT_MASK(32);

static struct platform_device sw_usb_ohci_device[] = {
	[0] = {
		.name		= ohci_name,
		.id			= 1,
		.dev 		= {
			.dma_mask			= &sw_ohci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sw_ohci0,
		},
	},

	[1] = {
		.name		= ohci_name,
		.id			= 2,
		.dev 		= {
			.dma_mask			= &sw_ohci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sw_ohci1,
		},
	},
};

/*
*******************************************************************************
*                     sw_hci_sun4i_init
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
static int __init sw_hci_sun4i_init(void)
{
#ifdef  CONFIG_USB_SW_SUN4I_EHCI0
	platform_device_register(&sw_usb_ehci_device[0]);
#endif

#ifdef  CONFIG_USB_SW_SUN4I_EHCI1
 	platform_device_register(&sw_usb_ehci_device[1]);
#endif

#ifdef  CONFIG_USB_SW_SUN4I_OHCI0
  	platform_device_register(&sw_usb_ohci_device[0]);
#endif
 
#ifdef  CONFIG_USB_SW_SUN4I_OHCI1
 	platform_device_register(&sw_usb_ohci_device[1]);
#endif

    return 0;
}

/*
*******************************************************************************
*                     sw_hci_sun4i_exit
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
static void __exit sw_hci_sun4i_exit(void)
{
#ifdef  CONFIG_USB_SW_SUN4I_EHCI0
	platform_device_unregister(&sw_usb_ehci_device[0]);
#endif

#ifdef  CONFIG_USB_SW_SUN4I_EHCI1
 	platform_device_unregister(&sw_usb_ehci_device[1]);
#endif

#ifdef  CONFIG_USB_SW_SUN4I_OHCI0
  	platform_device_unregister(&sw_usb_ohci_device[0]);
#endif
 
#ifdef  CONFIG_USB_SW_SUN4I_OHCI1
 	platform_device_unregister(&sw_usb_ohci_device[1]);
#endif

    return ;
}

module_init(sw_hci_sun4i_init);
module_exit(sw_hci_sun4i_exit);

