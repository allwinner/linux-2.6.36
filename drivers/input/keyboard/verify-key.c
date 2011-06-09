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
*
* Copyright (c) 2011
*
* ChangeLog
*
*
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h> 

#define  KEY_DEBUG
//#define  KEY_DEBUG_LEVEL2
#define  KEY_MAX_CNT  13

#define  IRQ_KEY             (31)
#define  KEY_BASSADDRESS     (0xf1c22800)
#define  LRADC_CTRL          (0x00)
#define  LRADC_INTC          (0x04)
#define  LRADC_INTS          (0x08)
#define  LRADC_DATA0         (0x0c)
#define  LRADC_DATA1         (0x10)

#define  FIRST_CONCERT_DLY   (2<<24)
#define  CHAN                (0)
#define  ADC_CHAN_SELECT     (CHAN<<22)
#define  KEY_MODE            (1)
#define  KEY_MODE_SELECT     (KEY_MODE<<12)
#define  LEVELB_VOL          (0<<4)



#define  LRADC_SAMPLE_32HZ   (3<<2)
#define  LRADC_SAMPLE_62HZ   (2<<2)
#define  LRADC_SAMPLE_125HZ  (1<<2)
#define  LRADC_SAMPLE_250HZ  (0<<2)


#define  LRADC_EN            (1<<0)



#define  LRADC_ADC1_UP_EN    (1<<12)
#define  LRADC_ADC1_DOWN_EN  (1<<9)
#define  LRADC_ADC1_DATA_EN  (1<<8)

#define  LRADC_ADC0_UP_EN    (1<<4)
#define  LRADC_ADC0_DOWN_EN  (1<<1)
#define  LRADC_ADC0_DATA_EN  (1<<0)

#define  LRADC_ADC1_UPPEND   (1<<12)
#define  LRADC_ADC1_DOWNPEND (1<<9)
#define  LRADC_ADC1_DATAPEND (1<<8)


#define  LRADC_ADC0_UPPEND   (1<<4)
#define  LRADC_ADC0_DOWNPEND (1<<1)
#define  LRADC_ADC0_DATAPEND (1<<0)


MODULE_AUTHOR(" <@>");
MODULE_DESCRIPTION("sun3i keyboard driver");
MODULE_LICENSE("GPL");

#define EVB
//#define CUSTUM

#define ONE_CHANNEL
#define MODE_0V2
//#define MODE_0V15


//#define TWO_CHANNEL


#ifdef MODE_0V2
//standard of key maping
//0.2V mode	 
static unsigned char keypad_mapindex[64] =
{
    0,0,0,0,0,0,0,0,               //key 1, 8个， 0-7
    1,1,1,1,1,1,1,                 //key 2, 7个， 8-14
    2,2,2,2,2,2,2,                 //key 3, 7个， 15-21
    3,3,3,3,3,3,                   //key 4, 6个， 22-27
    4,4,4,4,4,4,                   //key 5, 6个， 28-33
    5,5,5,5,5,5,                   //key 6, 6个， 34-39
    6,6,6,6,6,6,6,6,6,6,           //key 7, 10个，40-49
    7,7,7,7,7,7,7,7,7,7,7,7,7,7    //key 8, 17个，50-63
};
#endif
                        
#ifdef MODE_0V15
//0.15V mode
static unsigned char keypad_mapindex[64] =
{
	0,0,0,                      //key1
	1,1,1,1,1,                  //key2
	2,2,2,2,2,
	3,3,3,3,
	4,4,4,4,4,
	5,5,5,5,5,
	6,6,6,6,6,
	7,7,7,7,
	8,8,8,8,8,
	9,9,9,9,9,
	10,10,10,10,
	11,11,11,11,
	12,12,12,12,12,12,12,12,12,12 //key13
};
#endif

#ifdef EVB
static unsigned int sun3i_scankeycodes[KEY_MAX_CNT]=
{
	[0 ] = KEY_NEXTSONG,       
	[1 ] = KEY_PREVIOUSSONG,      
	[2 ] = KEY_VOLUMEUP,         
	[3 ] = KEY_VOLUMEDOWN,       
	[4 ] = KEY_ENTER,   
	[5 ] = KEY_RESERVED, 
	[6 ] = KEY_RESERVED,        
	[7 ] = KEY_RESERVED,
	[8 ] = KEY_RESERVED,
	[9 ] = KEY_RESERVED,
	[10] = KEY_RESERVED,
	[11] = KEY_RESERVED,
	[12] = KEY_RESERVED,
};
#endif


static volatile unsigned int key_val;

static struct input_dev *sun3ikbd_dev;
static unsigned char scancode;



static irqreturn_t sun3i_isr_key(int irq, void *dummy)
{
	unsigned int  reg_val;
	
	#ifdef KEY_DEBUG
	printk("Key Interrupt\n");
  	#endif
	reg_val  = readl(KEY_BASSADDRESS+LRADC_INT_STA);
	writel(reg_val,KEY_BASSADDRESS+LRADC_INT_STA);
	if(reg_val&LRADC_ADC0_DOWNPEND)
	{
		#ifdef KEY_DEBUG
		printk("key down\n");
		#endif
	}
	
	if(reg_val&LRADC_ADC0_DATAPEND)
	{
		key_val = readl(KEY_BASSADDRESS+LRADC_DATA0);
		scancode = keypad_mapindex[key_val&0x3f];
		input_report_key(sun3ikbd_dev, sun3i_scankeycodes[scancode], 1);
		input_sync(sun3ikbd_dev);			
	}   
        
	if(reg_val&LRADC_ADC0_UPPEND)
	{
		input_report_key(sun3ikbd_dev, sun3i_scankeycodes[scancode], 0);
		input_sync(sun3ikbd_dev);	
		#ifdef KEY_DEBUG
		printk("key up \n");
		#endif	
	}
	writel(reg_val,KEY_BASSADDRESS+LRADC_INT_STA);
	return IRQ_HANDLED;
}

static int __init sun3ikbd_init(void)
{
	int i;
	int err =0;	

	sun3ikbd_dev = input_allocate_device();
	if (!sun3ikbd_dev) {
		printk(KERN_ERR "sun3ikbd: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sun3ikbd_dev->name = "sun3i Keyboard";  
	sun3ikbd_dev->phys = "sun3ikbd/input0"; 
	sun3ikbd_dev->id.bustype = BUS_HOST;      
	sun3ikbd_dev->id.vendor = 0x0001;
	sun3ikbd_dev->id.product = 0x0001;
	sun3ikbd_dev->id.version = 0x0100;

	sun3ikbd_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP) ;

	for (i = 0; i < KEY_MAX_CNT; i++)
		set_bit(sun3i_scankeycodes[i], sun3ikbd_dev->keybit);
	
	#ifdef ONE_CHANNEL
	writel(LRADC_ADC0_DOWN_EN|LRADC_ADC0_UP_EN|LRADC_ADC0_DATA_EN,KEY_BASSADDRESS+LRADC_INTC);	
	writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|ADC_CHAN_SELECT|LRADC_SAMPLE_62HZ|LRADC_EN,KEY_BASSADDRESS+LRADC_CTRL);
	#else
	#endif


	if (request_irq(IRQ_KEY, sun3i_isr_key, 0, "sun3ikbd",
			sun3i_isr_key)) {
		err = -EBUSY;
		goto fail2;
	}
	  



	err = input_register_device(sun3ikbd_dev);
	if (err)
		goto fail3;

	return 0;

 fail3:	free_irq(IRQ_KEY, sun3i_isr_key);
 fail2:	input_free_device(sun3ikbd_dev);
 fail1:	;
 return err;
}

static void __exit sun3ikbd_exit(void)
{
	free_irq(IRQ_KEY, sun3i_isr_key);
	input_unregister_device(sun3ikbd_dev);
}

module_init(sun3ikbd_init);
module_exit(sun3ikbd_exit);
