/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: usb_manager.c
*
* Author 		: javen
*
* Description 	: USB π‹¿Ì≥Ã–Ú
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2011-4-14            1.0          create this file
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

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/irqs.h>

/*
*******************************************************************************
*                     usb_manager_init
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
static int __init usb_manager_init(void)
{
    printk("[sw usb]: usb_manager_init\n");
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
static void __exit usb_manager_exit(void)
{
    printk("[sw usb]: usb_manager_exit\n");

    return ;
}

module_init(usb_manager_init);
module_exit(usb_manager_exit);

