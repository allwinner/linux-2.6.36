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

//#define CONFIG_TOUCHSCREEN_m1_DEBUG

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
#define ACQ                    (0xF)
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
#define FILTER_TYPE            (0<<0)

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



struct m1ts_data {
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

struct m1ts_data * mtTsData =NULL;	

void tp_do_tasklet(unsigned long data);
DECLARE_TASKLET(tp_tasklet,tp_do_tasklet,0);
	
static int  tp_init(void)
{
   /*
    struct m1ts_data *m1 = mtTsData;	
    writel(ADC_FIRST_DLY|ADC_FIRST_DLY_MODE|ADC_CLK_DIVIDER|FS_DIV|T_ACQ, TP_BASSADDRESS + TP_CTRL0);	
    writel(TP_SENSITIVE_ADJUST|TP_MODE_SELECT,TP_BASSADDRESS + TP_CTRL2);
    writel(STYLUS_UP_DEBOUNCE|STYLUS_UP_DEBOUCE_EN|TOUCH_PAN_CALI_EN|TP_DUAL_EN|TP_MODE_EN,TP_BASSADDRESS + TP_CTRL1);
    writel(FILTER_EN|FILTER_TYPE,TP_BASSADDRESS + TP_CTRL3);
    writel(TP_DATA_IRQ_EN|TP_FIFO_TRIG_LEVEL|TP_FIFO_FLUSH|TP_UP_IRQ_EN|TP_DOWN_IRQ_EN, m1->base_addr+TP_INT_FIFOC);
    */
    writel(0x00b70100,TP_BASSADDRESS + TP_CTRL0);
    writel(0xfd000000,TP_BASSADDRESS + TP_CTRL2);
    writel(0x4,TP_BASSADDRESS + TP_CTRL3);
    writel(0x10513,TP_BASSADDRESS +TP_INT_FIFOC);
    writel(0x00005230,TP_BASSADDRESS + TP_CTRL1);
    return (0);
}

void tp_do_tasklet(unsigned long data)
{
	struct m1ts_data *m1 = mtTsData;	
	//int x1,x2,y1,y2;

	
	
  	switch(m1->status)
  	{
  		case TP_DOWN:
  		{
			m1->touchflag = 0; 
			m1->count     = 0;
  			break;
  		}
  		case TP_UP :
    	{
	        m1->touchflag = 0; 
	        m1->count     = 0;
		    input_report_abs(m1->input, ABS_MT_TOUCH_MAJOR,0);
		    input_sync(m1->input);
		    break;
  		}

  		case TP_DATA_VA:
  		{
            input_report_abs(m1->input, ABS_MT_TOUCH_MAJOR,800);
            input_report_abs(m1->input, ABS_MT_POSITION_X, m1->x);
            input_report_abs(m1->input, ABS_MT_POSITION_Y, m1->y);	
            #ifdef CONFIG_TOUCHSCREEN_m1_DEBUG
                printk("x = %d, y = %d\n",m1->x,m1->y);
            #endif
            input_mt_sync(m1->input);	        		
            input_sync(m1->input);	

/*
  			if(((m1->dx) > DUAL_TOUCH)&&((m1->dy) > DUAL_TOUCH))
  			{
  				m1->touchflag = 2;
  				m1->count = 0;
	  			input_report_abs(m1->input, ABS_MT_TOUCH_MAJOR,800);
	  			x1 = 2048 - (m1->dx<<2);
	  			y1 = 2048 - (m1->dy<<2);
			  	input_report_abs(m1->input, ABS_MT_POSITION_X, x1);
			  	input_report_abs(m1->input, ABS_MT_POSITION_Y, y1);
			  	input_mt_sync(m1->input);
			  	input_report_abs(m1->input, ABS_MT_TOUCH_MAJOR,800);
	  			x2 = 2048 + (m1->dx<<2);
	  			y2 = 2048 + (m1->dy<<2);
			  	input_report_abs(m1->input, ABS_MT_POSITION_X, x2);
			  	input_report_abs(m1->input, ABS_MT_POSITION_Y, y2);
				input_mt_sync(m1->input);
				input_sync(m1->input);
		        }else{
//		        	 if((m1->touchflag == 2)&&((m1->count++)>TOUCH_CHANGE))
//		        	 {
//		        	 	 input_report_abs(m1->input, ABS_MT_TOUCH_MAJOR,800);
//				  	     input_report_abs(m1->input, ABS_MT_POSITION_X, m1->x);
//				  	     input_report_abs(m1->input, ABS_MT_POSITION_Y, m1->y);	
//				  	     input_mt_sync(m1->input);	        		
//					     input_sync(m1->input);		        		
//		        	 }

		        	 
		        	 if(m1->touchflag == 0)
		        	 {
		        	 	 input_report_abs(m1->input, ABS_MT_TOUCH_MAJOR,800);
				  	     input_report_abs(m1->input, ABS_MT_POSITION_X, m1->x);
				  	     input_report_abs(m1->input, ABS_MT_POSITION_Y, m1->y);	
				  	     #ifdef CONFIG_TOUCHSCREEN_m1_DEBUG
		  				     printk("x = %d, y = %d\n",m1->x,m1->y);
						 #endif
				  	     input_mt_sync(m1->input);	        		
					     input_sync(m1->input);				        		
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
static irqreturn_t m1_isr_tp(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct m1ts_data *m1 = (struct m1ts_data *)platform_get_drvdata(pdev);

	unsigned int reg_val;
	//unsigned int i;
	int x1,x2,y1,y2,z1,z2;
	

	reg_val  = readl(TP_BASSADDRESS + TP_INT_FIFOS);
	if(reg_val&TP_DOWN_PENDING)
	{
	    #ifdef CONFIG_TOUCHSCREEN_m1_DEBUG
		    printk("enter press the screen \n");
		#endif
	}
	
	if(reg_val&TP_UP_PENDING)
	{
	    #ifdef CONFIG_TOUCHSCREEN_m1_DEBUG
	        printk("enter up the screen \n");
        #endif
        
		m1->status = TP_UP;
		m1->count  = 0;

	}

	if(reg_val&FIFO_DATA_PENDING)
	{    		
		x1  = readl(TP_BASSADDRESS + TP_DATA);
		y1  = readl(TP_BASSADDRESS + TP_DATA);		
   	    x2  = readl(TP_BASSADDRESS + TP_DATA);
        y2  = readl(TP_BASSADDRESS + TP_DATA);	
        z1  = readl(TP_BASSADDRESS + TP_DATA);
        z2  = readl(TP_BASSADDRESS + TP_DATA);
        #ifdef CONFIG_TOUCHSCREEN_m1_DEBUG			
		    printk("%d,%d,%d,%d,%d,%d\n",x1,y1,x2,y2,z1,z2);
		#endif
  	}
  	
  	writel(reg_val,TP_BASSADDRESS + TP_INT_FIFOS);
	return IRQ_HANDLED;
}
*/

static irqreturn_t m1_isr_tp(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct m1ts_data *m1 = (struct m1ts_data *)platform_get_drvdata(pdev);

	unsigned int reg_val;
	//unsigned int i;
	//int       ret = -1;
	//int x1,x2,y1,y2,z1,z2;
	

	reg_val  = readl(TP_BASSADDRESS + TP_INT_FIFOS);
	if(reg_val&TP_DOWN_PENDING)
	{
		writel(reg_val,TP_BASSADDRESS + TP_INT_FIFOS);
		#ifdef CONFIG_TOUCHSCREEN_m1_DEBUG
		    printk("enter press the screen \n");
		#endif
		m1->status = TP_DOWN;
		m1->count  = 0;
		tp_do_tasklet(0);	
		return IRQ_HANDLED;
	}
	
	if(reg_val&TP_UP_PENDING)
	{
		writel(reg_val,TP_BASSADDRESS + TP_INT_FIFOS);
		#ifdef CONFIG_TOUCHSCREEN_m1_DEBUG
		    printk("enter up the screen \n");
		#endif
		m1->status = TP_UP;
		m1->count  = 0;
		tp_do_tasklet(0);	
		return IRQ_HANDLED;
	}

	if(reg_val&FIFO_DATA_PENDING)
	{    		
		m1->x      = readl(TP_BASSADDRESS + TP_DATA);
		m1->y      = readl(TP_BASSADDRESS + TP_DATA);		
   	    m1->dx     = readl(TP_BASSADDRESS + TP_DATA);
        m1->dy     = readl(TP_BASSADDRESS + TP_DATA);	
        m1->z1     = readl(TP_BASSADDRESS + TP_DATA);
        m1->z2     = readl(TP_BASSADDRESS + TP_DATA);		
		m1->status = TP_DATA_VA;		
		//writel(TP_DATA_IRQ_EN|TP_FIFO_TRIG_LEVEL|TP_FIFO_FLUSH|TP_UP_IRQ_EN|TP_DOWN_IRQ_EN, m1->base_addr+TP_INT_FIFOC);	
		writel(reg_val,TP_BASSADDRESS + TP_INT_FIFOS);
		tp_do_tasklet(0);	
		return IRQ_HANDLED;
  	}
  	
  	return IRQ_NONE;
}

static int m1ts_open(struct input_dev *dev)
{
	/* enable clock */
	return 0;
}

static void m1ts_close(struct input_dev *dev)
{
	/* disable clock */
}




static struct m1ts_data *m1ts_data_alloc(struct platform_device *pdev)
{
	 
	struct m1ts_data *m1 = kzalloc(sizeof(*m1), GFP_KERNEL);

	if (!m1)
		return NULL;

	m1->input = input_allocate_device();
	if (!m1->input) {
		kfree(m1);
		return NULL;
	}

	
	m1->input->evbit[0] =  BIT(EV_SYN) | BIT(EV_KEY) | BIT(EV_ABS);
	set_bit(BTN_TOUCH, m1->input->keybit);
	
    input_set_abs_params(m1->input, ABS_MT_TOUCH_MAJOR, 0, 1000, 0, 0);
    input_set_abs_params(m1->input, ABS_MT_POSITION_X, 0, 4095, 0, 0);
    input_set_abs_params(m1->input, ABS_MT_POSITION_Y, 0, 4095, 0, 0);


	m1->input->name = pdev->name;
	m1->input->phys = "m1ts/input0";
	m1->input->id.bustype = BUS_HOST ;
	m1->input->id.vendor = 0x0001;
	m1->input->id.product = 0x0001;
	m1->input->id.version = 0x0100;

	m1->input->open = m1ts_open;
	m1->input->close = m1ts_close;
	m1->input->dev.parent = &pdev->dev; 
 
	return m1;
}




static void m1ts_data_free(struct m1ts_data *m1)
{
	if (!m1)
		return;
	if (m1->input)
		input_free_device(m1->input);
	kfree(m1);
}


static int __devinit m1ts_probe(struct platform_device *pdev)
{
	int err =0;
	int irq = platform_get_irq(pdev, 0);	
	struct m1ts_data *m1;	
	tp_flag = 0;

    #ifdef CONFIG_TOUCHSCREEN_m1_DEBUG
	    printk( "m1-ts.c: m1ts_probe: start...\n");
	#endif

	m1 = m1ts_data_alloc(pdev);
	if (!m1) {
		dev_err(&pdev->dev, "Cannot allocate driver structures\n");
		err = -ENOMEM;
		goto err_out;
	}

	mtTsData = m1; 
	
	#ifdef CONFIG_TOUCHSCREEN_m1_DEBUG
	    printk("begin get platform resourec\n");
    #endif
    
	m1->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!m1->res) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "Can't get the MEMORY\n");
		goto err_out1;
	}

    m1->base_addr = (void __iomem *)TP_BASSADDRESS;

	m1->irq = irq;
    
	err = request_irq(irq, m1_isr_tp,
		IRQF_DISABLED, pdev->name, pdev);
	if (err) {
		dev_err(&pdev->dev, "Cannot request keypad IRQ\n");
		goto err_out2;
	}	  

	
	platform_set_drvdata(pdev, m1);	

	//printk("Input request \n");
	/* All went ok, so register to the input system */
	err = input_register_device(m1->input);
	if (err)
		goto err_out3;

	#ifdef CONFIG_TOUCHSCREEN_m1_DEBUG    
        printk("tp init\n");
    #endif
    
    tp_init();

    #ifdef CONFIG_TOUCHSCREEN_m1_DEBUG
	    printk( "m1-ts.c: m1ts_probe: end\n");
    #endif
    
	return 0;

 err_out3:
	if (m1->irq)
		free_irq(m1->irq, pdev);
err_out2:
err_out1:
	m1ts_data_free(m1);
err_out: 	
    #ifdef CONFIG_TOUCHSCREEN_m1_DEBUG     
	    printk( "m1-ts.c: m1ts_probe: failed!\n");
	#endif
	
	return err;
}




static int __devexit m1ts_remove(struct platform_device *pdev)
{
	
	struct m1ts_data *m1 = platform_get_drvdata(pdev);	

	input_unregister_device(m1->input);
	free_irq(m1->irq, pdev);	
	m1ts_data_free(m1);
	platform_set_drvdata(pdev, NULL);

	return 0;	
}
	
	

#ifdef CONFIG_PM
static int m1ts_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int m1ts_resume(struct platform_device *pdev)
{
	return 0;
}
#endif

static struct platform_driver m1ts_driver = {
	.probe		= m1ts_probe,
	.remove		= __devexit_p(m1ts_remove),

#ifdef CONFIG_PM
	.suspend	= m1ts_suspend,
	.resume		= m1ts_resume,
#endif

	.driver		= {
		.name	= "m1-TouchScreen",
	},
};


static void m1ts_nop_release(struct device *dev)
{
	/* Nothing */
}

static struct resource m1ts_resource[] = {
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

struct platform_device m1ts_device = {
	.name		= "m1-TouchScreen",
	.id		    = -1,
	.dev = {
		.release = m1ts_nop_release,
		},
	.resource	= m1ts_resource,
	.num_resources	= ARRAY_SIZE(m1ts_resource),
};


static int __init m1ts_init(void)
{
    #ifdef CONFIG_TOUCHSCREEN_m1_DEBUG     
	    printk("m1-ts.c: m1ts_init: start ...\n");
	#endif
	platform_device_register(&m1ts_device);
	return platform_driver_register(&m1ts_driver);
}

static void __exit m1ts_exit(void)
{
	platform_driver_unregister(&m1ts_driver);
	platform_device_unregister(&m1ts_device);

}

module_init(m1ts_init);
module_exit(m1ts_exit);

MODULE_AUTHOR("zhengdixu <@>");
MODULE_DESCRIPTION("m1 touchscreen driver");
MODULE_LICENSE("GPL");


