/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
* Copyright (c) 2009
*
* ChangeLog
*
*
*/
#ifndef _AW_PLATFORM_OPS_H_
#define _AW_PLATFORM_OPS_H_
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <mach/gpio_v2.h>
#include <mach/irqs.h>
#include <linux/i2c.h>

// gpio base address
#define PIO_BASE_ADDRESS             (0x01c20800)
#define PIO_RANGE_SIZE               (0x400)

#define IRQ_EINT21                   (21) 
#define IRQ_EINT29                   (29) 

#define TS_POLL_DELAY			    10/* ms delay between samples */
#define TS_POLL_PERIOD			    10 /* ms delay between samples */

#define GPIO_ENABLE
#define SYSCONFIG_GPIO_ENABLE

#define PIO_INT_STAT_OFFSET          (0x214)
#define PIO_INT_CTRL_OFFSET          (0x210)
#define PIO_INT_CFG2_OFFSET          (0x208)
#define PIO_INT_CFG3_OFFSET          (0x20c)
#define PIO_PN_DAT_OFFSET(n)         ((n)*0x24 + 0x10) 
//#define PIOI_DATA                    (0x130)
#define PIOH_DATA                    (0x10c)
#define PIOI_CFG3_OFFSET             (0x12c)

#define PRESS_DOWN                   1
#define FREE_UP                      0

#define TS_RESET_LOW_PERIOD       (1)
#define TS_INITIAL_HIGH_PERIOD   (100)
#define TS_WAKEUP_LOW_PERIOD  (10)
#define TS_WAKEUP_HIGH_PERIOD (10)
#define IRQ_NO                           (IRQ_EINT21)

struct aw_platform_ops{
    int         irq;
	bool        pendown;
	int	        (*get_pendown_state)(void);
	void        (*clear_penirq)(void);
	int         (*set_irq_mode)(void);
	int         (*set_gpio_mode)(void);
	int         (*judge_int_occur)(void);
    int         (*init_platform_resource)(void);
    void        (*free_platform_resource)(void);
	int         (*fetch_sysconfig_para)(void);
	void        (*ts_reset)(void);
	void        (*ts_wakeup)(void);
};


#endif /*_AW_PLATFORM_OPS_H_*/
