/* cg5121sc light sensor driver for a10 platform 
 * mostly copy from mma7660 g-sensor driver
 *                          sun4i-ts touchscreen driver
 *                          sun4i-keyboard driver.
 *  by young <yanggq@allwinnertech.com>
 *
 *  Copyright (C) 2011 AllWinner Technology.
 *  Copyright (C) 2009-2010 Freescale Semiconductor Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/input-polldev.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/ioport.h>

//#define PRINT_CALL_INFO
#define PRINT_SYSFS_INFO
//#define VALUE_MAPPING

#define CG5121SC_DRV_NAME	 "cg5121sc"
#define POLL_INTERVAL_MAX	10000
#define POLL_INTERVAL		100
#define ABS_LUX                           (ABS_MISC)
#define MIN_LUX                          (0)
#define MAX_LUX                         (63)
#define INPUT_FUZZ	4
#define INPUT_FLAT	4

/*
 *adc related configuration
 */
 #define  KEY_BASSADDRESS     (0xf1c22800)
#define  LRADC_CTRL          (0x00)
#define  LRADC_INTC          (0x04)
#define  LRADC_INT_STA       (0x08)
#define  LRADC_DATA0         (0x0c)
#define  LRADC_DATA1         (0x10)

#define  FIRST_CONCERT_DLY   (2<<24)
#define  CHAN                (0x3)
#define  ADC_CHAN_SELECT     (CHAN<<22)
#define ADC_CHAN_SELECT_MASK (0x3<<22)
#define  LRADC_KEY_MODE      (0)
#define  KEY_MODE_SELECT     (LRADC_KEY_MODE<<12)
#define  LEVELB_VOL          (0<<4)

#define  LRADC_HOLD_EN        (1<<6)

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

#ifdef PRINT_CALL_INFO 
#define print_call_info(fmt, args...)     \
        do{                              \
                printk(fmt, ##args);     \
        }while(0)
#else
#define print_call_info(fmt, args...)   //
#endif

#ifdef PRINT_SYSFS_INFO 
#define print_sysfs_info(fmt, args...)     \
        do{                              \
                printk(fmt, ##args);     \
        }while(0)
#else
#define print_sysfs_info(fmt, args...)   //
#endif

#ifdef VALUE_MAPPING
static unsigned char value_mapindex[64] =
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

static struct cg5121sc_data_s {
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	int suspend_indator;
#endif
    struct input_polled_dev *input_polled_dev;
    int enable;
    int delay;
};

static struct cg5121sc_data_s cg5121sc_data;
static struct input_polled_dev *cg5121sc_idev;

struct platform_device cg5121sc_device = {
	.name		= CG5121SC_DRV_NAME,
	.id		        = -1,
};

static void cg5121sc_set_enable(struct device *dev, int enable)
{
	int pre_enable = cg5121sc_data.enable;
	
	if (enable) {
		if (0 == pre_enable) {
		        cg5121sc_data.input_polled_dev->input->open(cg5121sc_data.input_polled_dev->input);
			cg5121sc_data.enable = 1;
		}
	} else {
		if (1 == pre_enable) {
                        cg5121sc_data.input_polled_dev->input->close(cg5121sc_data.input_polled_dev->input);
			cg5121sc_data.enable = 0;
		} 
	}
	
	//print_sysfs_info("%s, %d \n", __func__, cg5121sc_data.enable);
	return;
	
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cg5121sc_early_suspend(struct early_suspend *h);
static void cg5121sc_late_resume(struct early_suspend *h);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cg5121sc_early_suspend(struct early_suspend *h)
{
	printk(KERN_INFO "cg5121sc early suspend\n v0.1");
	cg5121sc_data.suspend_indator = 1;
	//input_close_polled_device(cg5121sc_idev.input_polled_dev->input);
	cg5121sc_set_enable(NULL, 0);
	return;
}

static void cg5121sc_late_resume(struct early_suspend *h)
{
        static uint32_t tmp = 0;
	printk(KERN_INFO "cg5121sc late resume\n");
	cg5121sc_data.suspend_indator = 0;
	
	tmp = readl(KEY_BASSADDRESS + LRADC_CTRL);
        tmp &= (~(ADC_CHAN_SELECT_MASK));
        tmp |= (ADC_CHAN_SELECT);
        writel(tmp, KEY_BASSADDRESS + LRADC_CTRL);

        //input_open_polled_device(cg5121sc_data.input_polled_dev->input);
        cg5121sc_set_enable(NULL, 1);
	return;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static ssize_t cg5121sc_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	print_sysfs_info("%s, %d \n", __func__, cg5121sc_data.input_polled_dev->poll_interval);
	return sprintf(buf, "%d \n", cg5121sc_data.input_polled_dev->poll_interval);

}

static ssize_t cg5121sc_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (data > cg5121sc_data.input_polled_dev->poll_interval_max)
		data = cg5121sc_data.input_polled_dev->poll_interval_max;
		
	cg5121sc_data.input_polled_dev->poll_interval = data;
        print_sysfs_info("%s, %d \n", __func__, cg5121sc_data.input_polled_dev->poll_interval);
        
	return count;
}


static ssize_t cg5121sc_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	print_sysfs_info("%s, %d \n", __func__, cg5121sc_data.enable);
	return sprintf(buf, "%d\n", cg5121sc_data.enable);
}

static ssize_t cg5121sc_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if ((data == 0)||(data==1)) {
		cg5121sc_set_enable(dev,data);
	}

	return count;
}


static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		cg5121sc_delay_show, cg5121sc_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		cg5121sc_enable_show, cg5121sc_enable_store);

static struct attribute *cg5121sc_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group cg5121sc_attribute_group = {
	.attrs = cg5121sc_attributes
};

/*
 * Initialization function
 */
 static int hw_init(void)
 {
         //make sure light sensor init after keyboard, initialize adc1, and not affetc keyboard.
        static uint32_t tmp = 0;
        tmp = readl(KEY_BASSADDRESS+LRADC_INTC);
        tmp &= (~(LRADC_ADC1_UP_EN |LRADC_ADC1_DOWN_EN |LRADC_ADC1_DATA_EN));
        writel(tmp,KEY_BASSADDRESS + LRADC_INTC);
      /*  
        udelay(10);
        tmp = readl(KEY_BASSADDRESS+LRADC_INTC);
        printk("current LRADC_INTC: 0x%x. \n", tmp);
*/
        /*
        tmp = readl(KEY_BASSADDRESS + LRADC_CTRL);
        tmp &= (~(ADC_CHAN_SELECT_MASK));
        tmp |= (ADC_CHAN_SELECT);
        writel(tmp |FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT|LRADC_SAMPLE_62HZ|LRADC_EN, KEY_BASSADDRESS + LRADC_CTRL);
        */
        writel(0x2c00069,KEY_BASSADDRESS + LRADC_CTRL);
     /*   
        udelay(10);
        tmp = readl(KEY_BASSADDRESS+LRADC_CTRL);
        printk("current LRADC_CTRL: 0x%x. \n", tmp);
       */ 
        return 0;
 }

static void report_abs(void)
{
	static uint32_t key_val = 0;
#ifdef VALUE_MAPPING
	static uint32_t report_val = 0;
#endif
        print_call_info("%s. \n", __func__);
	/* convert signed 8bits to signed 16bits */
        key_val = readl(KEY_BASSADDRESS+LRADC_DATA1);
        print_call_info("%s, key_val == %d. \n", __func__, key_val );
        if(key_val <= 0x3f){
#ifdef VALUE_MAPPING
                report_val = value_mapindex[key_val];
            	input_report_abs(cg5121sc_idev->input, ABS_LUX, report_val);
#else
                input_report_abs(cg5121sc_idev->input, ABS_LUX, key_val);
#endif
	        input_sync(cg5121sc_idev->input);
        }
        return;
}

static void cg5121sc_dev_poll(struct input_polled_dev *dev)
{
    print_call_info("%s. \n", __func__);
#ifdef CONFIG_HAS_EARLYSUSPEND
	if(0 == cg5121sc_data.suspend_indator){
		report_abs();
	}
#else
	report_abs();
#endif
} 

/*
 * I2C init/probing/exit functions
 */
static int cg5121sc_probe(void)
{
	int result;
	struct input_dev *idev;
 
	printk(KERN_INFO "cg5121sc probe. \n");

	/* Initialize work env */
	hw_init();
	
  	//printk("Input request \n");
	/* All went ok, so register to the input system */
	/*input poll device register */
	cg5121sc_idev = input_allocate_polled_device();
	if (!cg5121sc_idev) {
		printk("alloc poll device failed!\n");
		result = -ENOMEM;
		return result;
	}
	cg5121sc_idev->poll = cg5121sc_dev_poll;
	cg5121sc_idev->poll_interval = POLL_INTERVAL;
	cg5121sc_idev->poll_interval_max = POLL_INTERVAL_MAX;
	idev = cg5121sc_idev->input;
	//input_dev = poll_dev->input;
	//input_dev->name = "AAED-2000 Keyboard";

	idev->name = CG5121SC_DRV_NAME;
	idev->id.bustype = BUS_HOST;
	idev->phys = "cg5121sc/input0";
	idev->id.bustype = BUS_HOST;
	idev->id.vendor = 0x0001;
	idev->id.product = 0x0001;
	idev->id.version = 0x0100;

	idev->evbit[0] = BIT_MASK(EV_ABS);

	input_set_abs_params(idev, ABS_LUX, MIN_LUX, MAX_LUX, INPUT_FUZZ, INPUT_FLAT);
	
	result = input_register_polled_device(cg5121sc_idev);
	if (result) {
		printk("register poll device failed!\n");
		return result;
	}
	
        cg5121sc_data.input_polled_dev = cg5121sc_idev;

#ifdef CONFIG_HAS_EARLYSUSPEND
	cg5121sc_data.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	cg5121sc_data.early_suspend.suspend = cg5121sc_early_suspend;
	cg5121sc_data.early_suspend.resume = cg5121sc_late_resume;
	register_early_suspend(&cg5121sc_data.early_suspend);
	cg5121sc_data.suspend_indator = 0;
#endif

	result = sysfs_create_group(&cg5121sc_data.input_polled_dev->input->dev.kobj,
						 &cg5121sc_attribute_group);
	if (result < 0)
	{
		printk("%s: sysfs_create_group err\n", __func__);
		return result;
	}
	
	//printk("open polled dev. \n");
	cg5121sc_set_enable(NULL, 1);
        //platform_set_drvdata(pdev, &cg5121sc_data);
        //printk("cg5121sc_probe ok. \n");
	return result;
}

static int  cg5121sc_remove(void)
{
	int result = 0;

        printk("%s. \n", __func__);
        sysfs_remove_group(&cg5121sc_data.input_polled_dev->input->dev.kobj, &cg5121sc_attribute_group);
        cg5121sc_set_enable(NULL, 0);
	#ifdef CONFIG_HAS_EARLYSUSPEND
	    unregister_early_suspend(&cg5121sc_data.early_suspend);
	#endif
	input_unregister_polled_device(cg5121sc_data.input_polled_dev);
        input_free_polled_device(cg5121sc_data.input_polled_dev);
	return result;
}

static int __init cg5121sc_init(void)
{
	/* register driver */
	int ret = 0;
	
        ret = cg5121sc_probe();
	return ret;
}

static void __exit cg5121sc_exit(void)
{
	printk(KERN_INFO "remove cg5121sc driver.\n");
	cg5121sc_remove();

}

MODULE_AUTHOR("young <yanggq@allwinnertech.com>");
MODULE_DESCRIPTION("CG5121SC light Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

late_initcall(cg5121sc_init);
module_exit(cg5121sc_exit);

