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
* Copyright (c) 2009
*
* ChangeLog
*
*
*/
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/fs.h> 
#include <asm/uaccess.h> 
#include <linux/mm.h> 
#include <linux/slab.h>

static int tp_flag = 0;

//tp status value
#define TP_DOWN                (0)
#define TP_UP                  (1) 
#define TP_DATA_VA             (2)  
//?
#define DUAL_TOUCH             (50)
#define TOUCH_CHANGE           (3)
#define TP_DATA_AV_NO          (0x3)

//#define CONFIG_TOUCHSCREEN_SUN4I_DEBUG

#define IRQ_TP                 (29)
#define TP_BASSADDRESS         (0xf1c25000)
#define TP_CTRL0               (0x00)
#define TP_CTRL1               (0x04)
#define TP_CTRL2               (0x08)
#define TP_CTRL3               (0x0c)
#define TP_INT_FIFOC           (0x10)
#define TP_INT_FIFOS           (0x14)
#define TP_TPR                 (0x18)
#define TP_CDAT                (0x1c)
#define TEMP_DATA              (0x20)
#define TP_DATA                (0x24)


#define ADC_FIRST_DLY          (0x1<<24)
#define ADC_FIRST_DLY_MODE     (0x1<<23) 
#define ADC_CLK_SELECT         (0x0<<22)
#define ADC_CLK_DIVIDER        (0x2<<20)    
#define CLK                    (7)
#define FS_DIV                 (CLK<<16)
#define ACQ                    (0x3f)
#define T_ACQ                  (ACQ)

#define STYLUS_UP_DEBOUNCE     (0<<12)
#define STYLUS_UP_DEBOUCE_EN   (0<<9)
#define TOUCH_PAN_CALI_EN      (1<<6)
#define TP_DUAL_EN             (1<<5)
#define TP_MODE_EN             (1<<4)
#define TP_ADC_SELECT          (0<<3)
#define ADC_CHAN_SELECT        (0)

#define TP_SENSITIVE_ADJUST    (0xc<<28)
#define TP_MODE_SELECT         (0x1<<26)
#define PRE_MEA_EN             (0x1<<24)
#define PRE_MEA_THRE_CNT       (0xFFF<<0)

#define FILTER_EN              (1<<2)
#define FILTER_TYPE            (0x01<<0)

#define TP_DATA_IRQ_EN         (1<<16)
#define TP_DATA_XY_CHANGE      (1<<13)
#define TP_FIFO_TRIG_LEVEL     (3<<8)
#define TP_FIFO_FLUSH          (1<<4)
#define TP_UP_IRQ_EN           (1<<1)
#define TP_DOWN_IRQ_EN         (1<<0)

#define FIFO_DATA_PENDING      (1<<16)
#define TP_UP_PENDING          (1<<1)
#define TP_DOWN_PENDING        (1<<0)


                               
#define TPDATA_MASK            (0xfff)



struct sun4i_ts_data {
	struct resource *res;
	struct input_dev *input;
	void __iomem *base_addr;
	int irq;
	unsigned int x1;
	unsigned int y1;
	unsigned int x;
	unsigned int y;
	unsigned int dx;
	unsigned int dy;
	unsigned int z1;
	unsigned int z2;
	int status;
	int count;
	int touchflag;
	char phys[32];
};

struct sun4i_ts_data * mtTsData =NULL;	

void tp_do_tasklet(unsigned long data);
DECLARE_TASKLET(tp_tasklet,tp_do_tasklet,0);
	
static int  tp_init(void)
{
    //struct sun4i_ts_data *ts_data = (struct sun4i_ts_data *)platform_get_drvdata(pdev);
    struct sun4i_ts_data *ts_data = mtTsData;	
    //TP_CTRL0: 0x0027003f
    writel(ADC_CLK_DIVIDER|FS_DIV|T_ACQ, TP_BASSADDRESS + TP_CTRL0);	   
    //TP_CTRL2: 0xc4000000
    writel(TP_SENSITIVE_ADJUST|TP_MODE_SELECT,TP_BASSADDRESS + TP_CTRL2);
    //TP_CTRL3: 0x05
    writel(FILTER_EN|FILTER_TYPE,TP_BASSADDRESS + TP_CTRL3);
    //TP_INT_FIFOC: 0x00010313
    writel(TP_DATA_IRQ_EN|TP_FIFO_TRIG_LEVEL|TP_FIFO_FLUSH|TP_UP_IRQ_EN|TP_DOWN_IRQ_EN, ts_data->base_addr + TP_INT_FIFOC);
    //TP_CTRL1: 0x00000070 -> 0x00000030
    writel(STYLUS_UP_DEBOUNCE|STYLUS_UP_DEBOUCE_EN|TP_DUAL_EN|TP_MODE_EN,TP_BASSADDRESS + TP_CTRL1);
    

  /*
       //put_wvalue(TP_CTRL0 ,0x02a6007f);  //512us TACQ  4ms FS 60 point
      //put_wvalue(TP_CTRL0 ,0x0aa7003f); 
      put_wvalue(TP_CTRL0 ,0x00a7003f);   //100 point
      put_wvalue(TP_CTRL1 ,0x00000030);
      //put_wvalue(TP_INT_FIFOC,0x12f13);
      put_wvalue(TP_INT_FIFOC,0x10313);
      //put_wvalue(TP_CTRL2,0x90003e8); 
      //put_wvalue(TP_CTRL2,0x9002710); 
      put_wvalue(TP_CTRL2,0xc4002710);
      put_wvalue(TP_CTRL3,0x00000005);
   */
   
   /*
    writel(0x00b70100,TP_BASSADDRESS + TP_CTRL0);
    writel(0xfd000000,TP_BASSADDRESS + TP_CTRL2);
    writel(0x4,TP_BASSADDRESS + TP_CTRL3);
    writel(0x10513,TP_BASSADDRESS +TP_INT_FIFOC);
    writel(0x00005230,TP_BASSADDRESS + TP_CTRL1);*/
    return (0);
}

void tp_do_tasklet(unsigned long data)
{
    //struct sun4i_ts_data *ts_data = (struct sun4i_ts_data *)platform_get_drvdata(pdev);

	struct sun4i_ts_data *ts_data = mtTsData;	
	//int x1,x2,y1,y2;

	
	
  	switch(ts_data->status)
  	{
  		case TP_DOWN:
  		{
			ts_data->touchflag = 0; 
			ts_data->count     = 0;
  			break;
  		}
  		case TP_UP :
    	{
	        ts_data->touchflag = 0; 
	        ts_data->count     = 0;
		    input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,0);
		    input_sync(ts_data->input);
		    break;
  		}

  		case TP_DATA_VA:
  		{
            input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,800);
            input_report_abs(ts_data->input, ABS_MT_POSITION_X, ts_data->x);
            input_report_abs(ts_data->input, ABS_MT_POSITION_Y, ts_data->y);	
            #ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG
                printk("x = %d, y = %d\n",ts_data->x,ts_data->y);
            #endif
            input_mt_sync(ts_data->input);	        		
            input_sync(ts_data->input);	

/*
  			if(((ts_data->dx) > DUAL_TOUCH)&&((ts_data->dy) > DUAL_TOUCH))
  			{
  				ts_data->touchflag = 2;
  				ts_data->count = 0;
	  			input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,800);
	  			x1 = 2048 - (ts_data->dx<<2);
	  			y1 = 2048 - (ts_data->dy<<2);
			  	input_report_abs(ts_data->input, ABS_MT_POSITION_X, x1);
			  	input_report_abs(ts_data->input, ABS_MT_POSITION_Y, y1);
			  	input_mt_sync(ts_data->input);
			  	input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,800);
	  			x2 = 2048 + (ts_data->dx<<2);
	  			y2 = 2048 + (ts_data->dy<<2);
			  	input_report_abs(ts_data->input, ABS_MT_POSITION_X, x2);
			  	input_report_abs(ts_data->input, ABS_MT_POSITION_Y, y2);
				input_mt_sync(ts_data->input);
				input_sync(ts_data->input);
		        }else{
//		        	 if((ts_data->touchflag == 2)&&((ts_data->count++)>TOUCH_CHANGE))
//		        	 {
//		        	 	 input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,800);
//				  	     input_report_abs(ts_data->input, ABS_MT_POSITION_X, ts_data->x);
//				  	     input_report_abs(ts_data->input, ABS_MT_POSITION_Y, ts_data->y);	
//				  	     input_mt_sync(ts_data->input);	        		
//					     input_sync(ts_data->input);		        		
//		        	 }

		        	 
		        	 if(ts_data->touchflag == 0)
		        	 {
		        	 	 input_report_abs(ts_data->input, ABS_MT_TOUCH_MAJOR,800);
				  	     input_report_abs(ts_data->input, ABS_MT_POSITION_X, ts_data->x);
				  	     input_report_abs(ts_data->input, ABS_MT_POSITION_Y, ts_data->y);	
				  	     #ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG
		  				     printk("x = %d, y = %d\n",ts_data->x,ts_data->y);
						 #endif
				  	     input_mt_sync(ts_data->input);	        		
					     input_sync(ts_data->input);				        		
		        	 }
		        	
		        }
		     */
  			break;
  		}
  		
		default:	
  			break;
  	
  	}
}
/*
static irqreturn_t sun4i_isr_tp(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct sun4i_ts_data *ts_data = (struct sun4i_ts_data *)platform_get_drvdata(pdev);

	unsigned int reg_val;
	//unsigned int i;
	int x1,x2,y1,y2,z1,z2;
	

	reg_val  = readl(TP_BASSADDRESS + TP_INT_FIFOS);
	if(reg_val&TP_DOWN_PENDING)
	{
	    #ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG
		    printk("enter press the screen \n");
		#endif
	}
	
	if(reg_val&TP_UP_PENDING)
	{
	    #ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG
	        printk("enter up the screen \n");
        #endif
        
		ts_data->status = TP_UP;
		ts_data->count  = 0;

	}

	if(reg_val&FIFO_DATA_PENDING)
	{    		
		x1  = readl(TP_BASSADDRESS + TP_DATA);
		y1  = readl(TP_BASSADDRESS + TP_DATA);		
   	    x2  = readl(TP_BASSADDRESS + TP_DATA);
        y2  = readl(TP_BASSADDRESS + TP_DATA);	
        z1  = readl(TP_BASSADDRESS + TP_DATA);
        z2  = readl(TP_BASSADDRESS + TP_DATA);
        #ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG			
		    printk("%d,%d,%d,%d,%d,%d\n",x1,y1,x2,y2,z1,z2);
		#endif
  	}
  	
  	writel(reg_val,TP_BASSADDRESS + TP_INT_FIFOS);
	return IRQ_HANDLED;
}
*/

static irqreturn_t sun4i_isr_tp(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct sun4i_ts_data *ts_data = (struct sun4i_ts_data *)platform_get_drvdata(pdev);

	unsigned int reg_val;
	//unsigned int i;
	//int       ret = -1;
	//int x1,x2,y1,y2,z1,z2;
	

	reg_val  = readl(TP_BASSADDRESS + TP_INT_FIFOS);
	if(reg_val&TP_DOWN_PENDING)
	{
		writel(reg_val,TP_BASSADDRESS + TP_INT_FIFOS);
		#ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG
		    printk("enter press the screen \n");
		#endif
		ts_data->status = TP_DOWN;
		ts_data->count  = 0;
		tp_do_tasklet(0);	
		return IRQ_HANDLED;
	}
	
	if(reg_val&TP_UP_PENDING)
	{
		writel(reg_val,TP_BASSADDRESS + TP_INT_FIFOS);
		#ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG
		    printk("enter up the screen \n");
		#endif
		ts_data->status = TP_UP;
		ts_data->count  = 0;
		tp_do_tasklet(0);	
		return IRQ_HANDLED;
	}

	if(reg_val&FIFO_DATA_PENDING)
	{    		
		ts_data->x      = readl(TP_BASSADDRESS + TP_DATA);
		ts_data->y      = readl(TP_BASSADDRESS + TP_DATA);		
   	    ts_data->dx     = readl(TP_BASSADDRESS + TP_DATA);
        ts_data->dy     = readl(TP_BASSADDRESS + TP_DATA);	
        //ts_data->z1     = readl(TP_BASSADDRESS + TP_DATA);
        //ts_data->z2     = readl(TP_BASSADDRESS + TP_DATA);		
		ts_data->status = TP_DATA_VA;		
		//writel(TP_DATA_IRQ_EN|TP_FIFO_TRIG_LEVEL|TP_FIFO_FLUSH|TP_UP_IRQ_EN|TP_DOWN_IRQ_EN, ts_data->base_addr+TP_INT_FIFOC);	
		writel(reg_val,TP_BASSADDRESS + TP_INT_FIFOS);
		tp_do_tasklet(0);	
		return IRQ_HANDLED;
  	}
  	
  	return IRQ_NONE;
}

static int sun4i_ts_open(struct input_dev *dev)
{
	/* enable clock */
	return 0;
}

static void sun4i_ts_close(struct input_dev *dev)
{
	/* disable clock */
}




static struct sun4i_ts_data *sun4i_ts_data_alloc(struct platform_device *pdev)
{
	 
	struct sun4i_ts_data *ts_data = kzalloc(sizeof(*ts_data), GFP_KERNEL);

	if (!ts_data)
		return NULL;

	ts_data->input = input_allocate_device();
	if (!ts_data->input) {
		kfree(ts_data);
		return NULL;
	}

	
	ts_data->input->evbit[0] =  BIT(EV_SYN) | BIT(EV_KEY) | BIT(EV_ABS);
	set_bit(BTN_TOUCH, ts_data->input->keybit);
	
    input_set_abs_params(ts_data->input, ABS_MT_TOUCH_MAJOR, 0, 1000, 0, 0);
    input_set_abs_params(ts_data->input, ABS_MT_POSITION_X, 0, 4095, 0, 0);
    input_set_abs_params(ts_data->input, ABS_MT_POSITION_Y, 0, 4095, 0, 0);


	ts_data->input->name = pdev->name;
	ts_data->input->phys = "sun4i_ts/input0";
	ts_data->input->id.bustype = BUS_HOST ;
	ts_data->input->id.vendor = 0x0001;
	ts_data->input->id.product = 0x0001;
	ts_data->input->id.version = 0x0100;

	ts_data->input->open = sun4i_ts_open;
	ts_data->input->close = sun4i_ts_close;
	ts_data->input->dev.parent = &pdev->dev; 
 
	return ts_data;
}




static void sun4i_ts_data_free(struct sun4i_ts_data *ts_data)
{
	if (!ts_data)
		return;
	if (ts_data->input)
		input_free_device(ts_data->input);
	kfree(ts_data);
}


static int __devinit sun4i_ts_probe(struct platform_device *pdev)
{
	int err =0;
	int irq = platform_get_irq(pdev, 0);	
	struct sun4i_ts_data *ts_data;	
	tp_flag = 0;

    #ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG
	    printk( "sun4i-ts.c: sun4i_ts_probe: start...\n");
	#endif

	ts_data = sun4i_ts_data_alloc(pdev);
	if (!ts_data) {
		dev_err(&pdev->dev, "Cannot allocate driver structures\n");
		err = -ENOMEM;
		goto err_out;
	}

	mtTsData = ts_data; 
	
	#ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG
	    printk("begin get platform resourec\n");
    #endif
    
	ts_data->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!ts_data->res) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "Can't get the MEMORY\n");
		goto err_out1;
	}

    ts_data->base_addr = (void __iomem *)TP_BASSADDRESS;

	ts_data->irq = irq;
    
	err = request_irq(irq, sun4i_isr_tp,
		IRQF_DISABLED, pdev->name, pdev);
	if (err) {
		dev_err(&pdev->dev, "Cannot request keypad IRQ\n");
		goto err_out2;
	}	  

	
	platform_set_drvdata(pdev, ts_data);	

	//printk("Input request \n");
	/* All went ok, so register to the input system */
	err = input_register_device(ts_data->input);
	if (err)
		goto err_out3;

	#ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG    
        printk("tp init\n");
    #endif
    
    tp_init();

    #ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG
	    printk( "sun4i-ts.c: sun4i_ts_probe: end\n");
    #endif
    
	return 0;

 err_out3:
	if (ts_data->irq)
		free_irq(ts_data->irq, pdev);
err_out2:
err_out1:
	sun4i_ts_data_free(ts_data);
err_out: 	
    #ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG     
	    printk( "sun4i-ts.c: sun4i_ts_probe: failed!\n");
	#endif
	
	return err;
}




static int __devexit sun4i_ts_remove(struct platform_device *pdev)
{
	
	struct sun4i_ts_data *ts_data = platform_get_drvdata(pdev);	

	input_unregister_device(ts_data->input);
	free_irq(ts_data->irq, pdev);	
	sun4i_ts_data_free(ts_data);
	platform_set_drvdata(pdev, NULL);

	return 0;	
}
	
	

#ifdef CONFIG_PM
static int sun4i_ts_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int sun4i_ts_resume(struct platform_device *pdev)
{
	return 0;
}
#endif

static struct platform_driver sun4i_ts_driver = {
	.probe		= sun4i_ts_probe,
	.remove		= __devexit_p(sun4i_ts_remove),

#ifdef CONFIG_PM
	.suspend	= sun4i_ts_suspend,
	.resume		= sun4i_ts_resume,
#endif

	.driver		= {
		.name	= "sun4i-ts",
	},
};


static void sun4i_ts_nop_release(struct device *dev)
{
	/* Nothing */
}

static struct resource sun4i_ts_resource[] = {
	{
	.flags  = IORESOURCE_IRQ,
	.start  = IRQ_TP ,
	.end    = IRQ_TP ,
	},

	{
	.flags	= IORESOURCE_MEM,
	.start	= TP_BASSADDRESS,
	.end	= TP_BASSADDRESS + 0x100-1,
	},
};

struct platform_device sun4i_ts_device = {
	.name		= "sun4i-ts",
	.id		    = -1,
	.dev = {
		.release = sun4i_ts_nop_release,
		},
	.resource	= sun4i_ts_resource,
	.num_resources	= ARRAY_SIZE(sun4i_ts_resource),
};


static int __init sun4i_ts_init(void)
{
    #ifdef CONFIG_TOUCHSCREEN_SUN4I_DEBUG     
	    printk("sun4i-ts.c: sun4i_ts_init: start ...\n");
	#endif
	platform_device_register(&sun4i_ts_device);
	return platform_driver_register(&sun4i_ts_driver);
}

static void __exit sun4i_ts_exit(void)
{
	platform_driver_unregister(&sun4i_ts_driver);
	platform_device_unregister(&sun4i_ts_device);

}

module_init(sun4i_ts_init);
module_exit(sun4i_ts_exit);

MODULE_AUTHOR("zhengdixu <@>");
MODULE_DESCRIPTION("sun4i touchscreen driver");
MODULE_LICENSE("GPL");


