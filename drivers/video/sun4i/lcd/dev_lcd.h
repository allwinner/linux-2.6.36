#ifndef __DEV_LCD_H__
#define __DEV_LCD_H__

#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()?°Èkthread_run()
#include <linux/err.h> //IS_ERR()?°ÈPTR_ERR()
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>

#include <linux/drv_display.h>

extern void LCD_get_panel_funs_0(__lcd_panel_fun_t * fun);
extern void LCD_get_panel_funs_1(__lcd_panel_fun_t * fun);
extern void LCD_set_panel_funs(__lcd_panel_fun_t * lcd0_cfg, __lcd_panel_fun_t * lcd1_cfg);
extern __s32 DRV_DISP_Init(void);

#define __wrn(msg...) {printk("[LCD WRN],file:%s,line:%d:    ",__FILE__,__LINE__); printk(msg);}

#if 1
#define __inf(msg...) do{}while(0)
#define __msg(msg...) do{}while(0)
#define __here__ do{}while(0)
#else
#define __inf(msg...) {printk("[LCD] "); printk(msg);}
#define __msg(msg...) {printk("[LCD] file:%s,line:%d:    ",__FILE__,__LINE__); printk(msg);}
#define __here__      {printk("[LCD] file:%s,line:%d\n",__FILE__,__LINE__);}
#endif


#endif