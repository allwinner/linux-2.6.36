/* 
 * drivers/input/touchscreen/ft5x0x_ts.c
 *
 * FocalTech ft5x TouchScreen driver. 
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *	note: only support mulititouch	Wenfs 2010-10-01
 *  for this touchscreen to work, it's slave addr must be set to 0x7e
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include "ft5x_ts.h"
#ifdef CONFIG_HAS_EARLYSUSPEND
    #include <linux/pm.h>
#endif
#include <linux/interrupt.h>
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

#define FOR_TSLIB_TEST
#define PRINT_POINT_INFO
//#define PRINT_INT_INFO

//#define CONFIG_FT5X0X_MULTITOUCH

static struct i2c_client *this_client;

static int gpio_int_hdle = 0;
static int gpio_wakeup_hdle = 0;

static void* __iomem gpio_addr = NULL;

struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
	u16	x3;
	u16	y3;
	u16	x4;
	u16	y4;
	u16	x5;
	u16	y5;
	u16	pressure;
    u8  touch_point;
};

struct ft5x_ts_data {
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
};

static struct i2c_board_info __initdata i2c_info_ft5x_ts[] =  {
	{
	    //need to be modify, according the menuconfig 
		//I2C_BOARD_INFO("ft5x_ts", 0x7e),
		I2C_BOARD_INFO("ft5x_ts", 0x00),
		.platform_data	= NULL,
	},
};


static int ft5x_i2c_rxdata(char *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr>>1,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= this_client->addr>>1,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};

        //printk("IIC add = %x\n",this_client->addr);
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		printk("msg %s i2c read error: %d\n", __func__, ret);
	
	return ret;
}

//static int ft5x_i2c_txdata(char *txdata, int length)
//{
//	int ret;
//
//	struct i2c_msg msg[] = {
//		{
//			.addr	= this_client->addr>>1,
//			.flags	= 0,
//			.len	= length,
//			.buf	= txdata,
//		},
//	};
//
//   	//msleep(1);
//	ret = i2c_transfer(this_client->adapter, msg, 1);
//	if (ret < 0)
//		pr_err("%s i2c write error: %d\n", __func__, ret);
//
//	return ret;
//}
//
//static int ft5x_set_reg(u8 addr, u8 para)
//{
//    u8 buf[3];
//    int ret = -1;
//
//    buf[0] = addr;
//    buf[1] = para;
//    ret = ft5x_i2c_txdata(buf, 2);
//    if (ret < 0) {
//        pr_err("write reg failed! %#x ret: %d", buf[0], ret);
//        return -1;
//    }
//    
//    return 0;
//}
//

static void ft5x_ts_release(void)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
#ifdef CONFIG_FT5X0X_MULTITOUCH	
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
#else
	input_report_abs(data->input_dev, ABS_PRESSURE, 0);
	input_report_key(data->input_dev, BTN_TOUCH, 0);
#endif
	input_sync(data->input_dev);

}

static int ft5x_read_data(void)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	unsigned char buf[32]={0};
	int ret = -1;

#ifdef CONFIG_FT5X0X_MULTITOUCH

	ret = ft5x_i2c_rxdata(buf, 31);
#else
    ret = ft5x_i2c_rxdata(buf, 7);
#endif
    if (ret < 0) {
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));

	event->touch_point = buf[2] & 0x07;// 000 0111
#ifdef PRINT_POINT_INFO
	printk("touch point = %d\n",event->touch_point);
#endif
    if (event->touch_point == 0) {
        ft5x_ts_release();
        return 1; 
    }

#ifdef CONFIG_FT5X0X_MULTITOUCH
    switch (event->touch_point) {
		case 5:
			event->x5 = (s16)(buf[0x1b] & 0x0F)<<8 | (s16)buf[0x1c];
			event->y5 = (s16)(buf[0x1d] & 0x0F)<<8 | (s16)buf[0x1e];
		case 4:
			event->x4 = (s16)(buf[0x15] & 0x0F)<<8 | (s16)buf[0x16];
			event->y4 = (s16)(buf[0x17] & 0x0F)<<8 | (s16)buf[0x18];
		case 3:
			event->x3 = (s16)(buf[0x0f] & 0x0F)<<8 | (s16)buf[0x10];
			event->y3 = (s16)(buf[0x11] & 0x0F)<<8 | (s16)buf[0x12];
		case 2:
			event->x2 = (s16)(buf[9] & 0x0F)<<8 | (s16)buf[10];
			event->y2 = (s16)(buf[11] & 0x0F)<<8 | (s16)buf[12];
		case 1:
			event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
			event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
		break;
		default:
		    return -1;
	}
#else
    if (event->touch_point == 1) {
    	event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
	event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
    }
#endif
    event->pressure = 200;

	dev_dbg(&this_client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
		event->x1, event->y1, event->x2, event->y2);


    return 0;
}

static void ft5x_report_value(void)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;

		//printk("==ft5x_report_value =\n");
#ifdef CONFIG_FT5X0X_MULTITOUCH
	switch(event->touch_point) {
		case 5:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x5);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y5);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			#ifdef PRINT_POINT_INFO
			    printk("===x5 = %d,y5 = %d ====\n",event->x2,event->y2);
			#endif
		case 4:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x4);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y4);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			#ifdef PRINT_POINT_INFO
			    printk("===x4 = %d,y4 = %d ====\n",event->x2,event->y2);
			#endif
		case 3:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x3);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y3);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
            #ifdef PRINT_POINT_INFO
			    printk("===x3 = %d,y3 = %d ====\n",event->x2,event->y2);
			#endif
		case 2:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x2);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y2);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			#ifdef PRINT_POINT_INFO
			    printk("===x2 = %d,y2 = %d ====\n",event->x2,event->y2);
			#endif
		case 1:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x1);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y1);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			#ifdef PRINT_POINT_INFO
			    printk("===x1 = %d,y1 = %d ====\n",event->x1,event->y1);
			#endif
		default:
//			printk("==touch_point default =\n");
			break;
	}
#else	/* CONFIG_FT5X0X_MULTITOUCH*/
	if (event->touch_point == 1) {
		input_report_abs(data->input_dev, ABS_X, event->x1);
		input_report_abs(data->input_dev, ABS_Y, event->y1);
		input_report_abs(data->input_dev, ABS_PRESSURE, event->pressure);
	}
	input_report_key(data->input_dev, BTN_TOUCH, 1);
#endif	/* CONFIG_FT5X0X_MULTITOUCH*/
	input_sync(data->input_dev);

	dev_dbg(&this_client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
		event->x1, event->y1, event->x2, event->y2);
}	/*end ft5x_report_value*/

static void ft5x_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
	//printk("==work 1=\n");
	ret = ft5x_read_data();	
	if (ret == 0) {	
		ft5x_report_value();
	}
	//enable_irq(SW_INT_IRQNO_PIO);

}

static irqreturn_t ft5x_ts_interrupt(int irq, void *dev_id)
{
	struct ft5x_ts_data *ft5x_ts = dev_id;
	int reg_val;

#ifdef PRINT_INT_INFO		
	printk("==========------ft5x_ts TS Interrupt-----============\n"); 
#endif
	
	//clear the IRQ_EINT21 interrupt pending
	reg_val = readl(gpio_addr + PIO_INT_STAT_OFFSET);
     
    if(reg_val&(1<<(IRQ_EINT21)))
    {	
        #ifdef PRINT_INT_INFO
            printk("==IRQ_EINT21=\n");
        #endif
        writel(reg_val&(1<<(IRQ_EINT21)),gpio_addr + PIO_INT_STAT_OFFSET);
        //disable_irq(SW_INT_IRQNO_PIO);
        if (!work_pending(&ft5x_ts->pen_event_work)) 
        {
            #ifdef PRINT_INT_INFO
        	    printk("Enter work\n");
        	#endif
        	queue_work(ft5x_ts->ts_workqueue, &ft5x_ts->pen_event_work);
        }
	}
    else
	{
	    #ifdef PRINT_INT_INFO
	        printk("Other Interrupt\n");
	    #endif
	    //For Debug 
	    //writel(reg_val,gpio_addr + PIO_INT_STAT_OFFSET);
		//enable_irq(IRQ_EINT);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ft5x_ts_suspend(struct early_suspend *handler)
{
//	struct ft5x_ts_data *ts;
//	ts =  container_of(handler, struct ft5x_ts_data, early_suspend);

	printk("==ft5x_ts_suspend=\n");
//	disable_irq(this_client->irq);
//	disable_irq(IRQ_EINT(6));
//	cancel_work_sync(&ts->pen_event_work);
//	flush_workqueue(ts->ts_workqueue);
	// ==set mode ==, 
//    	ft5x_set_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);

   /*    //gpio i28 output low
	printk("==ft5x_ts_suspend=\n");
	//enter HIBERNATE mode
    ft5x_set_reg(0x3a,PMODE_HIBERNATE);
	*/
    //suspend 
    gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "tp_wakeup");        
}

static void ft5x_ts_resume(struct early_suspend *handler)
{
	printk("==ft5x_ts_resume=\n");
	// wake the mode
//	__gpio_as_output(GPIO_FT5X0X_WAKE);		
//	__gpio_clear_pin(GPIO_FT5X0X_WAKE);		//set wake = 0,base on system
//	 msleep(100);
//	__gpio_set_pin(GPIO_FT5X0X_WAKE);			//set wake = 1,base on system
//	msleep(100);
//	enable_irq(this_client->irq);
//	enable_irq(IRQ_EINT(6));
***************

    //gpio i28 output high
	printk("==ft5x_ts_resume=\n");
    //wake up
    gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "tp_wakeup");
    mdelay(5);
    gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "tp_wakeup");

}
#endif  //CONFIG_HAS_EARLYSUSPEND

static int 
ft5x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft5x_ts_data *ft5x_ts;
	struct input_dev *input_dev;
	int err = 0;
	int reg_val;
	
	printk("=====capacitor touchscreen driver register ================\n");
	printk("===================ft5x_ts_probe========================\n");
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}


	ft5x_ts = kzalloc(sizeof(*ft5x_ts), GFP_KERNEL);
	if (!ft5x_ts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

    gpio_addr = ioremap(PIO_BASE_ADDRESS, PIO_RANGE_SIZE);
    if(!gpio_addr) {
	    err = -EIO;
	    goto exit_ioremap_failed;	
	}
	//printk("touch panel gpio addr: = 0x%x", gpio_addr);
	this_client = client;
	i2c_set_clientdata(client, ft5x_ts);

//	printk("==INIT_WORK=\n");
	INIT_WORK(&ft5x_ts->pen_event_work, ft5x_ts_pen_irq_work);

	ft5x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ft5x_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}
    //config gpio:
    gpio_int_hdle = gpio_request_ex("tp_para", "tp_int_port");
    if(!gpio_int_hdle) {
        pr_warning("touch panel IRQ_EINT21_para request gpio fail!\n");
        goto exit_gpio_int_request_failed;
    }
    gpio_wakeup_hdle = gpio_request_ex("tp_para", "tp_wakeup");
    if(!gpio_wakeup_hdle) {
        pr_warning("touch panel tp_wakeup request gpio fail!\n");
        goto exit_gpio_wakeup_request_failed;
    }
    
#ifdef AW_GPIO_INT_API_ENABLE

#else
        //Config IRQ_EINT21 Negative Edge Interrupt
        reg_val = readl(gpio_addr + PIO_INT_CFG2_OFFSET);
        reg_val &=(~(7<<20));
        reg_val |=(1<<20);  
        writel(reg_val,gpio_addr + PIO_INT_CFG2_OFFSET);
        
        //Enable IRQ_EINT21 of PIO Interrupt
        reg_val = readl(gpio_addr + PIO_INT_CTRL_OFFSET);
        reg_val |=(1<<IRQ_EINT21);
        writel(reg_val,gpio_addr + PIO_INT_CTRL_OFFSET);
	    //disable_irq(IRQ_EINT);
#endif
    gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "tp_wakeup");
    mdelay(5);
    gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "tp_wakeup");


	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	ft5x_ts->input_dev = input_dev;

#ifdef CONFIG_FT5X0X_MULTITOUCH
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);	

	#ifdef FOR_TSLIB_TEST
	    set_bit(BTN_TOUCH, input_dev->keybit);
    #endif

	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
#else
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_PRESSURE, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_PRESSURE, 0, PRESS_MAX, 0 , 0);
#endif

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	input_dev->name		= FT5X_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"ft5x_ts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	printk("==register_early_suspend =\n");
	ft5x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft5x_ts->early_suspend.suspend = ft5x_ts_suspend;
	ft5x_ts->early_suspend.resume	= ft5x_ts_resume;
	register_early_suspend(&ft5x_ts->early_suspend);
#endif

#ifdef    CONFIG_FT5X0X_MULTITOUCH
    printk("CONFIG_FT5X0X_MULTITOUCH is defined. \n");
#endif

   
	err = request_irq(SW_INT_IRQNO_PIO, ft5x_ts_interrupt, IRQF_TRIGGER_FALLING | IRQF_SHARED, "ft5x_ts", ft5x_ts);
   
	if (err < 0) {
		dev_err(&client->dev, "ft5x_ts_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

	printk("==probe over =\n");
    return 0;

exit_irq_request_failed:
	cancel_work_sync(&ft5x_ts->pen_event_work);
	destroy_workqueue(ft5x_ts->ts_workqueue);
	enable_irq(SW_INT_IRQNO_PIO);
exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(SW_INT_IRQNO_PIO, ft5x_ts);
exit_gpio_wakeup_request_failed:
exit_gpio_int_request_failed:
exit_create_singlethread:
	printk("==singlethread error =\n");
	i2c_set_clientdata(client, NULL);
	kfree(ft5x_ts);
exit_ioremap_failed:
    if(gpio_addr){
        iounmap(gpio_addr);
    }
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

static int __devexit ft5x_ts_remove(struct i2c_client *client)
{
	
	struct ft5x_ts_data *ft5x_ts = i2c_get_clientdata(client);  
	printk("==ft5x_ts_remove=\n");
	#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ft5x_ts->early_suspend);
	#endif
	free_irq(SW_INT_IRQNO_PIO, ft5x_ts);
	input_unregister_device(ft5x_ts->input_dev);
	kfree(ft5x_ts);
	cancel_work_sync(&ft5x_ts->pen_event_work);
	destroy_workqueue(ft5x_ts->ts_workqueue);
	i2c_set_clientdata(client, NULL);
    if(gpio_addr)
    {
        iounmap(gpio_addr);
    }	
    gpio_release(gpio_int_hdle, 0);
    gpio_release(gpio_wakeup_hdle, 0);
	return 0;
}

static const struct i2c_device_id ft5x_ts_id[] = {
	{ FT5X_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ft5x_ts_id);

static struct i2c_driver ft5x_ts_driver = {
	.probe		= ft5x_ts_probe,
	.remove		= __devexit_p(ft5x_ts_remove),
	.id_table	= ft5x_ts_id,
	.driver	= {
		.name	= FT5X_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init ft5x_ts_init(void)
{
    int status = 0;
    
	printk("=========ft5x-ts-init============\n");

	if(0 == CONFIG_TOUCHSCREEN_FT5X_TS_SLAVE_ADDR){
        i2c_info_ft5x_ts[0].addr = 0x7e;
	}else if(1 == CONFIG_TOUCHSCREEN_FT5X_TS_SLAVE_ADDR){
        i2c_info_ft5x_ts[0].addr = 0x70;
	}
	
    status = i2c_register_board_info(CONFIG_TOUCHSCREEN_FT5X_TS_BUS_NUM, i2c_info_ft5x_ts, ARRAY_SIZE(i2c_info_ft5x_ts));
    printk("===============ft5x_ts=============, status = %d ===\n",status);
 
	return i2c_add_driver(&ft5x_ts_driver);
}

static void __exit ft5x_ts_exit(void)
{
	printk("==ft5x_ts_exit==\n");
	i2c_del_driver(&ft5x_ts_driver);
}

module_init(ft5x_ts_init);
module_exit(ft5x_ts_exit);

MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x TouchScreen driver");
MODULE_LICENSE("GPL");

