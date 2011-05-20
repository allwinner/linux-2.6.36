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
#include <linux/i2c/tsc2007.h>

#define GPIO_ENABLE

/* TSC2007 Touchscreen */
// gpio base address
#define PIO_BASE_ADDRESS             (0x01c20800)
#define PIO_RANGE_SIZE               (0x400)
#define CONFIG_FT5X0X_MULTITOUCH     1
#define IRQ_EINT29                   (29) 
#define PIO_INT_STAT_OFFSET          (0x214)
#define PIO_INT_CTRL_OFFSET          (0x210)
#define PIO_INT_CFG3_OFFSET          (0x20c)
#define PIO_PN_DAT_OFFSET(n)         ((n)*0x24 + 0x10) 
#define PIOI_DATA                    (0x130)
#define PIOI_CFG3_OFFSET             (0x12c)

#define PRESS_DOWN                   1
#define FREE_UP                      0

static int gpio_hdle                 = 0;
static void* __iomem gpio_addr       = NULL;

static int aw_set_irq_mode(void);
static int aw_set_gpio_mode(void);



/*
 * function  : get the input data state, 
 * return value:
 *             return PRESS_DOWN: if down
 *             return FREE_UP: if up,
 *             return 0: do not need process, equal free up.          
 */
static int aw_get_pendown_state(void)
{
	unsigned int reg_val;
	static int state = FREE_UP;

    //get the input port state
    reg_val = readl(gpio_addr + PIOI_DATA);
	//printk("reg_val = %x\n",reg_val);
    if(!(reg_val & (1<<IRQ_EINT29))) 
    {
        state = PRESS_DOWN;
        //printk("pen down\n");
        return PRESS_DOWN;
    }
    //touch panel is free up
    else   
    {
        state = FREE_UP;
        return FREE_UP;
    }
}


static void aw_clear_penirq(void)
{
	int reg_val;
	//clear the IRQ_EINT29 interrupt pending
	//printk("clear pend irq pending\n");
	reg_val = readl(gpio_addr + PIO_INT_STAT_OFFSET);
	//writel(reg_val,gpio_addr + PIO_INT_STAT_OFFSET);
    
    if((reg_val = (reg_val&(1<<(IRQ_EINT29)))))
    {
        //printk("==IRQ_EINT29=\n");              
        writel(reg_val,gpio_addr + PIO_INT_STAT_OFFSET);
    }
}

static int aw_init_ts(void)
{
	int err = 0;
	
    gpio_addr = ioremap(PIO_BASE_ADDRESS, PIO_RANGE_SIZE);
    if(!gpio_addr) {
	    err = -EIO;
	    //printk("error: ioremap \n");	
	}
	
    printk("TP IRQ INITAL\n");
    aw_set_irq_mode();
	
	return err;
}

static int aw_set_irq_mode(void)
{
    int reg_val;

    //config gpio to int mode
    printk("config gpio to int mode. \n");
    #ifndef GPIO_ENABLE
        reg_val = readl(gpio_addr + PIOI_CFG3_OFFSET);
        reg_val &= (~(7<<20)); 
        reg_val |= (3<<20);
        writel(reg_val,gpio_addr + PIOI_CFG3_OFFSET);
    #else
        if(gpio_hdle)
        {
            gpio_release(gpio_hdle, 2);
        }
        gpio_hdle = gpio_request_ex("tp_para", "tp_int_port");
        if(!gpio_hdle)
        {
            printk("request tp_int_port failed. \n");
        }
    #endif
    //Config IRQ_EINT29 Negative Edge Interrupt
    reg_val = readl(gpio_addr + PIO_INT_CFG3_OFFSET);
    reg_val &=(~(7<<20));
    reg_val |=(1<<20);  
    writel(reg_val,gpio_addr + PIO_INT_CFG3_OFFSET);
    
    aw_clear_penirq();
    
    //Enable IRQ_EINT29 of PIO Interrupt
    reg_val = readl(gpio_addr + PIO_INT_CTRL_OFFSET);
    reg_val |=(1<<IRQ_EINT29);
    writel(reg_val,gpio_addr + PIO_INT_CTRL_OFFSET);
	
    mdelay(2);
    
    return 0;  
}

static int aw_set_gpio_mode(void)
{
    //int reg_val;

    //config gpio to io mode
    printk("config gpio to io mode. \n");
    #ifndef GPIO_ENABLE
        reg_val = readl(gpio_addr + PIOI_CFG3_OFFSET);
        reg_val &= (~(7<<20)); 
        //reg_val |= (0<<20);
        writel(reg_val,gpio_addr + PIOI_CFG3_OFFSET);
    #else
        if(gpio_hdle)
        {
            gpio_release(gpio_hdle, 2);
        }
        gpio_hdle = gpio_request_ex("tp_para", "tp_io_port");
        if(!gpio_hdle)
        {
            printk("request tp_io_port failed. \n");
        }
    #endif
    
    return 0;
}

static int aw_judge_int_occur(void)
{
    //int reg_val[3];
    int reg_val;
    int ret = -1;

    reg_val = readl(gpio_addr + PIO_INT_STAT_OFFSET);
    if(reg_val&(1<<(IRQ_EINT29)))
    {
        ret = 0;
    }
    return ret; 	
}

static void aw_exit_ts(void)
{
    if(gpio_addr){
        iounmap(gpio_addr);
    }
	if(gpio_hdle)
	{
		gpio_release(gpio_hdle, 0);
	}
    
}

struct tsc2007_platform_data tsc2007_data = {
	.model = 2007,
	.x_plate_ohms = 180,
	.get_pendown_state = aw_get_pendown_state,
	.clear_penirq	   = aw_clear_penirq,
	.init_platform_hw  = aw_init_ts,
	.exit_platform_hw  = aw_exit_ts,
	.set_irq_mode      = aw_set_irq_mode,
	.set_gpio_mode     = aw_set_gpio_mode,	
	.judge_int_occur   = aw_judge_int_occur,
};


/* I2C clients */
static struct i2c_board_info __initdata tsc2007_i2c_clients[] = {
	[0] = {
		I2C_BOARD_INFO("tsc2007", 0x90>>1),
		.platform_data	= &tsc2007_data,
		.irq		= SW_INT_IRQNO_PIO,
	},
};


static int __init tsc2007_board_init(void)
{
	printk("tsc2007_board_init\n");

	return i2c_register_board_info(0, tsc2007_i2c_clients,
				ARRAY_SIZE(tsc2007_i2c_clients));
}
fs_initcall(tsc2007_board_init);
