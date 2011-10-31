/*
	Copyright (c) 2010 by ilitek Technology.
	All rights reserved.

	ilitek I2C touch screen driver for Android platform

	Author:	 Steward Fu
	Version: 1
	History:
		2010/10/26 Firstly released
		2010/10/28 Combine both i2c and hid function together
		2010/11/02 Support interrupt trigger for I2C interface
		2010/11/10 Rearrange code and add new IOCTL command
		2010/11/23 Support dynamic to change I2C address
		2010/12/21 Support resume and suspend functions
		2010/12/23 Fix synchronous problem when application and driver work at the same time
		2010/12/28 Add erasing background before calibrating touch panel
		2011/01/13 Rearrange code and add interrupt with polling method
		2011/01/14 Add retry mechanism
		2011/01/17 Support multi-point touch
		2011/01/21 Support early suspend function
		2011/02/14 Support key button function
		2011/02/18 Rearrange code
		2011/03/21 Fix counld not report first point
		2011/03/25 Support linux 2.36.x 
*/
#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
//#include "ft5x_ts.h"
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/version.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#endif

#include <mach/gpio_v2.h>
#include <mach/irqs.h>
#include <mach/script_v2.h>

int touch_key_hold_press = 0;
int touch_key_code[] = {KEY_BACK, KEY_MENU, KEY_HOME, KEY_F12};
int touch_key_press[] = {0, 0, 0, 0};
//
#define	PRINT_INT_INFO
// definitions
#define ILITEK_I2C_RETRY_COUNT			3
#define ILITEK_I2C_DEVICE_NAME			"ilitek_ts"
#define ILITEK_I2C_DRIVER_NAME			"ilitek_ts"
#define ILITEK_FILE_DRIVER_NAME			"ilitek_file"
#define ILITEK_DEBUG_LEVEL			KERN_INFO
#define ILITEK_ERROR_LEVEL			KERN_ALERT

// i2c command for ilitek touch screen
#define ILITEK_TP_CMD_READ_DATA			0x10
#define ILITEK_TP_CMD_READ_SUB_DATA		0x11
#define ILITEK_TP_CMD_GET_RESOLUTION		0x20
#define ILITEK_TP_CMD_GET_FIRMWARE_VERSION	0x40
#define ILITEK_TP_CMD_GET_PROTOCOL_VERSION	0x42
#define	ILITEK_TP_CMD_CALIBRATION		0xCC
#define ILITEK_TP_CMD_ERASE_BACKGROUND		0xCE

// define the application command
#define ILITEK_IOCTL_BASE                       100
#define ILITEK_IOCTL_I2C_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 0, unsigned char*)
#define ILITEK_IOCTL_I2C_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 1, int)
#define ILITEK_IOCTL_I2C_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 2, unsigned char*)
#define ILITEK_IOCTL_I2C_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 3, int)
#define ILITEK_IOCTL_USB_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 4, unsigned char*)
#define ILITEK_IOCTL_USB_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 5, int)
#define ILITEK_IOCTL_USB_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 6, unsigned char*)
#define ILITEK_IOCTL_USB_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 7, int)
#define ILITEK_IOCTL_I2C_UPDATE_RESOLUTION      _IOWR(ILITEK_IOCTL_BASE, 8, int)
#define ILITEK_IOCTL_USB_UPDATE_RESOLUTION      _IOWR(ILITEK_IOCTL_BASE, 9, int)
#define ILITEK_IOCTL_I2C_SET_ADDRESS            _IOWR(ILITEK_IOCTL_BASE, 10, int)
#define ILITEK_IOCTL_I2C_UPDATE                 _IOWR(ILITEK_IOCTL_BASE, 11, int)
#define ILITEK_IOCTL_STOP_READ_DATA             _IOWR(ILITEK_IOCTL_BASE, 12, int)
#define ILITEK_IOCTL_START_READ_DATA            _IOWR(ILITEK_IOCTL_BASE, 13, int)
// gpio base address
#define PIO_BASE_ADDRESS             (0x01c20800)
#define PIO_RANGE_SIZE               (0x400)
#define CONFIG_FT5X0X_MULTITOUCH     (1)

#define IRQ_EINT21                   (21) 
#define IRQ_EINT29                   (29) 

#define PIO_INT_STAT_OFFSET          (0x214)
#define PIO_INT_CTRL_OFFSET          (0x210)
#define PIO_INT_CFG2_OFFSET          (0x208)
#define PIO_INT_CFG3_OFFSET          (0x20c)
// module information
MODULE_AUTHOR("Steward_Fu");
MODULE_DESCRIPTION("ILITEK I2C touch driver for Android platform");
MODULE_LICENSE("GPL");

// all implemented global functions must be defined in here 
// in order to know how many function we had implemented

static int ilitek_i2c_register_device(void);
static void ilitek_set_input_param(struct input_dev*, int, int, int);
static int ilitek_i2c_read_tp_info(void);
static int ilitek_init(void);
static void ilitek_exit(void);

// i2c functions
static int ilitek_i2c_transfer(struct i2c_client*, struct i2c_msg*, int);
static int ilitek_i2c_read(struct i2c_client*, uint8_t, uint8_t*, int);
static int ilitek_i2c_process_and_report(void);
static int ilitek_i2c_suspend(struct i2c_client*, pm_message_t);
static int ilitek_i2c_resume(struct i2c_client*);
static void ilitek_i2c_shutdown(struct i2c_client*);
static int ilitek_i2c_probe(struct i2c_client*, const struct i2c_device_id*);
static int ilitek_i2c_remove(struct i2c_client*);
#ifdef CONFIG_HAS_EARLYSUSPEND
        static void ilitek_i2c_early_suspend(struct early_suspend *h);
        static void ilitek_i2c_late_resume(struct early_suspend *h);
#endif
static int ilitek_i2c_polling_thread(void*);
static irqreturn_t ilitek_i2c_isr(int, void*);
static void ilitek_i2c_irq_work_queue_func(struct work_struct*);
static void ilitek_ts_reset(void);
// file operation functions
static int ilitek_file_ioctl(struct inode*, struct file*, unsigned int, unsigned long);
static int ilitek_file_open(struct inode*, struct file*);
static ssize_t ilitek_file_write(struct file*, const char*, size_t, loff_t*);
static ssize_t ilitek_file_read(struct file*, char*, size_t, loff_t*);
static int ilitek_file_close(struct inode*, struct file*);

static void* __iomem gpio_addr = NULL;
static int gpio_int_hdle = 0;
static int gpio_wakeup_hdle = 0;
static int gpio_reset_hdle = 0;

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;

// declare touch point data
struct touch_data {
	// x, y value
	int x, y;
	// check wehther this point is valid or not
	int valid;
	// id information
	int id;
};

// declare i2c data member
struct i2c_data {
	// input device
        struct input_dev *input_dev;
        // i2c client
        struct i2c_client *client;
        // polling thread
        struct task_struct *thread;
        // maximum x
        int max_x;
        // maximum y
        int max_y;
	// maximum touch point
	int max_tp;
	// maximum key button
	int max_btn;
        // the total number of x channel
        int x_ch;
        // the total number of y channel
        int y_ch;
        // check whether i2c driver is registered success
        int valid_i2c_register;
        // check whether input driver is registered success
        int valid_input_register;
	// check whether the i2c enter suspend or not
	int stop_polling;
	// read semaphore
	struct semaphore wr_sem;
	// protocol version
	int protocol_ver;
	// valid irq request
	int valid_irq_request;
	// work queue for interrupt use only
	struct workqueue_struct *irq_work_queue;
	// work struct for work queue
	struct work_struct irq_work;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

// device data
struct dev_data {
        // device number
        dev_t devno;
        // character device
        struct cdev cdev;
        // class device
        struct class *class;
};

// global variables
static struct i2c_data i2c;
static struct dev_data dev;

// i2c id table
static const struct i2c_device_id ilitek_i2c_id[] ={
	{ILITEK_I2C_DRIVER_NAME, 0}, {}
};
MODULE_DEVICE_TABLE(i2c, ilitek_i2c_id);

// declare i2c function table
static struct i2c_driver ilitek_i2c_driver = {
	.id_table = ilitek_i2c_id,
	.driver = {.name = ILITEK_I2C_DRIVER_NAME},
	.resume = ilitek_i2c_resume,
    .suspend  = ilitek_i2c_suspend,
	.shutdown = ilitek_i2c_shutdown,
	.probe = ilitek_i2c_probe,
	.remove = ilitek_i2c_remove,
};

// declare file operations
struct file_operations ilitek_fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
	.unlocked_ioctl = ilitek_file_ioctl,
#else
	.ioctl = ilitek_file_ioctl,
#endif
	.read = ilitek_file_read,
	.write = ilitek_file_write,
	.open = ilitek_file_open,
	.release = ilitek_file_close,
};

/*
description
	open function for character device driver
prarmeters
	inode
	    inode
	filp
	    file pointer
return
	status
*/
static int 
ilitek_file_open(
	struct inode *inode, struct file *filp)
{
	return 0; 
}

/*
description
	write function for character device driver
prarmeters
	filp
	    file pointer
	buf
	    buffer
	count
	    buffer length
	f_pos
	    offset
return
	status
*/
static ssize_t 
ilitek_file_write(
	struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	unsigned char buffer[128]={0};
        struct i2c_msg msgs[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = count, .buf = buffer,}
	};
        
	// before sending data to touch device, we need to check whether the device is working or not
	if(i2c.valid_i2c_register == 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c device driver doesn't be registered\n", __func__);
		return -1;
	}

	// check the buffer size whether it exceeds the local buffer size or not
	if(count > 128){
		printk(ILITEK_ERROR_LEVEL "%s, buffer exceed 128 bytes\n", __func__);
		return -1;
	}

	// copy data from user space
	ret = copy_from_user(buffer, buf, count-1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, copy data from user space, failed", __func__);
		return -1;
	}

	// parsing command
        if(strcmp(buffer, "calibrate") == 0){
		buffer[0] = ILITEK_TP_CMD_ERASE_BACKGROUND;
                msgs[0].len = 1;
                ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
                if(ret < 0){
                        printk(ILITEK_DEBUG_LEVEL "%s, i2c erase background, failed\n", __func__);
                }
                else{
                        printk(ILITEK_DEBUG_LEVEL "%s, i2c erase background, success\n", __func__);
                }

		buffer[0] = ILITEK_TP_CMD_CALIBRATION;
                msgs[0].len = 1;
                msleep(2000);
                ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
		if(ret < 0){
                        printk(ILITEK_DEBUG_LEVEL "%s, i2c calibration, failed\n", __func__);
                }
		else{
                	printk(ILITEK_DEBUG_LEVEL "%s, i2c calibration, success\n", __func__);
		}
		msleep(1000);
                return count;
	}
	return -1;
}

/*
description
        ioctl function for character device driver
prarmeters
	inode
		file node
        filp
            file pointer
        cmd
            command
        arg
            arguments
return
        status
*/
static int 
ilitek_file_ioctl(
	struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	static unsigned char buffer[64]={0};
	static int len=0;
	int ret;
	struct i2c_msg msgs[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = len, .buf = buffer,}
        };

	// parsing ioctl command
	switch(cmd){
	case ILITEK_IOCTL_I2C_WRITE_DATA:
		ret = copy_from_user(buffer, (unsigned char*)arg, len);
		if(ret < 0){
                	printk(ILITEK_ERROR_LEVEL "%s, copy data from user space, failed\n", __func__);
                	return -1;
        	}
		ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
		if(ret < 0){
			printk(ILITEK_ERROR_LEVEL "%s, i2c write, failed\n", __func__);
			return -1;
		}
		break;
	case ILITEK_IOCTL_I2C_READ_DATA:
		msgs[0].flags = I2C_M_RD;
		ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
		if(ret < 0){
                        printk(ILITEK_ERROR_LEVEL "%s, i2c read, failed\n", __func__);
			return -1;
                }
		ret = copy_to_user((unsigned char*)arg, buffer, len);
		if(ret < 0){
                        printk(ILITEK_ERROR_LEVEL "%s, copy data to user space, failed\n", __func__);
                        return -1;
                }
		break;
	case ILITEK_IOCTL_I2C_WRITE_LENGTH:
	case ILITEK_IOCTL_I2C_READ_LENGTH:
		len = arg;
		break;
	case ILITEK_IOCTL_I2C_UPDATE_RESOLUTION:
	case ILITEK_IOCTL_I2C_SET_ADDRESS:
	case ILITEK_IOCTL_I2C_UPDATE:
		break;
	case ILITEK_IOCTL_START_READ_DATA:
		i2c.stop_polling = 0;
		break;
	case ILITEK_IOCTL_STOP_READ_DATA:
		i2c.stop_polling = 1;
                break;
	default:
		return -1;
	}
    	return 0;
}

/*
description
	read function for character device driver
prarmeters
	filp
	    file pointer
	buf
	    buffer
	count
	    buffer length
	f_pos
	    offset
return
	status
*/
static ssize_t
ilitek_file_read(
        struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

/*
description
	close function
prarmeters
	inode
	    inode
	filp
	    file pointer
return
	status
*/
static int 
ilitek_file_close(
	struct inode *inode, struct file *filp)
{
        return 0;
}

/*
description
	set input device's parameter
prarmeters
	input
		input device data
	max_tp
		single touch or multi touch
	max_x
		maximum	x value
	max_y
		maximum y value
return
	nothing
*/
static void 
ilitek_set_input_param(
	struct input_dev *input, 
	int max_tp, 
	int max_x, 
	int max_y)
{
	int key;
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
        input_set_abs_params(input, ABS_MT_POSITION_X, 0, screen_max_x, 0, 0);
        printk("ilitek_set_input_param: max_x == %d. \n", max_x);
	printk("ilitek_set_input_param: max_y == %d. \n", max_y);
        input_set_abs_params(input, ABS_MT_POSITION_Y, 0, screen_max_y, 0, 0);
        input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
        input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, max_tp, 0, 0);
	for(key=0; key<sizeof(touch_key_code); key++){
        	if(touch_key_code[key] <= 0){
            		continue;
		}
        	set_bit(touch_key_code[key] & KEY_MAX, input->keybit);
	}
	input->name = ILITEK_I2C_DRIVER_NAME;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &(i2c.client)->dev;
}

/*
description
	send message to i2c adaptor
parameter
	client
		i2c client
	msgs
		i2c message
	cnt
		i2c message count
return
	>= 0 if success
	others if error
*/
static int 
ilitek_i2c_transfer(
	struct i2c_client *client, struct i2c_msg *msgs, int cnt)
{
	int ret, count=ILITEK_I2C_RETRY_COUNT;
	while(count >= 0){
		count-= 1;
		ret = down_interruptible(&i2c.wr_sem);
                ret = i2c_transfer(client->adapter, msgs, cnt);
                up(&i2c.wr_sem);
                if(ret < 0){
                        msleep(500);
			continue;
                }
		break;
	}
	return ret;
}

/*
description
	read data from i2c device
parameter
	client
		i2c client data
	addr
		i2c address
	data
		data for transmission
	length
		data length
return
	status
*/
static int 
ilitek_i2c_read(
	struct i2c_client *client,
	uint8_t cmd, 
	uint8_t *data, 
	int length)
{
	int ret;
        struct i2c_msg msgs[] = {
		{.addr = client->addr, .flags = 0, .len = 1, .buf = &cmd,},
		{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = data,}
        };

        ret = ilitek_i2c_transfer(client, msgs, 2);
        //ret = i2c_transfer(client->adapter, msgs, 2);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
	}
	return ret;
}

/*
description
	process i2c data and then report to kernel
parameters
	none
return
	status
*/
static int 
ilitek_i2c_process_and_report(
	void)
{
	int i, len, ret, x, y, key;
	struct input_dev *input = i2c.input_dev;
        unsigned char buf[9]={0};

       	// read i2c data from device
       	ret = ilitek_i2c_read(i2c.client, ILITEK_TP_CMD_READ_DATA, buf, 9);
	if(ret < 0){
		return ret;
	}
	#ifdef PRINT_INT_INFO
		printk(ILITEK_ERROR_LEVEL "%s, ilitek read data success\n", __func__);
	#endif
	// multipoint process
	if(i2c.protocol_ver & 0x200){
		len = buf[0];
		#ifdef PRINT_INT_INFO
		printk(ILITEK_ERROR_LEVEL "%s, ilitek multipoint process\n", __func__);
		#endif
		// read touch point
		for(i=0; i<len; i++){
			// parse point
			if(ilitek_i2c_read(i2c.client, ILITEK_TP_CMD_READ_SUB_DATA, buf, 5)){
				x = (((int)buf[1]) << 8) + buf[2];
				y = (((int)buf[3]) << 8) + buf[4];

				// no touch
				if((buf[0] & 0x80) == 0 && buf[0]!=0x01){
					continue;
				}
			
				// report to android system
				input_event(input, EV_ABS, ABS_MT_TRACKING_ID, buf[0] & 0x3F);
                        	input_event(input, EV_ABS, ABS_MT_POSITION_X, x);
                        	input_event(input, EV_ABS, ABS_MT_POSITION_Y, y);
                        	input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
                        	input_mt_sync(input);
                        	//fly
				printk(ILITEK_DEBUG_LEVEL "%s, %X, %d, %d\n", __func__, buf[0], x, y);
			}
		}
		// release point
                if(buf[0] == 0){
                        input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
                        input_mt_sync(input);
                }
                input_sync(input);
	}
	else{
		// parse point
			   
		for(i=0; i<i2c.max_tp; i++){
			unsigned char tp_id, key_id;
			tp_id = buf[0];
			key_id = buf[1] - 1;
        	x = (int)buf[1 + (i * 4)] + ((int)buf[2 + (i * 4)] * 256);
        	y = (int)buf[3 + (i * 4)] + ((int)buf[4 + (i * 4)] * 256);
		
		#ifdef PRINT_INT_INFO
               		printk(ILITEK_ERROR_LEVEL "%s, ilitek_report_value i2c.max_tp= %d,tp_id=%d\n", __func__,i2c.max_tp,tp_id);			    
		#endif
			// check whether the point is valid or not
			if((tp_id & (1 << i)) == 0){
				continue;
			}
		#ifdef PRINT_INT_INFO
		printk(ILITEK_ERROR_LEVEL "%s, ilitek_report_value to  android system\n", __func__);
		#endif	
	
			// report to android system
			switch(tp_id){
			case 0x81:
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
    				input_mt_sync(input);
				if((touch_key_press[key_id] == 0) && (touch_key_hold_press == 0)){
					input_report_key(input, touch_key_code[key_id], 1);
					touch_key_press[key_id] = 1;
					touch_key_hold_press = 1;
					printk("Key:%d ID:%d press\n", touch_key_code[key_id], key_id);
				}
				#ifdef PRINT_INT_INFO
				printk(ILITEK_ERROR_LEVEL "%s, ilitek_report_value 0x81\n", __func__);
				#endif
				break;
			case 0x80:
			default:
				for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
					if(touch_key_press[key]){
						input_report_key(input, touch_key_code[key], 0);
						touch_key_press[key] = 0;
						printk("Key:%d ID:%d release\n", touch_key_code[key], key);
                   	}
				}
				touch_key_hold_press = 0;
				input_event(i2c.input_dev, EV_ABS, ABS_MT_TRACKING_ID, i);
               	input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_X, x);
              	input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_Y, y);
               	input_event(i2c.input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
               	input_mt_sync(i2c.input_dev);
               	#ifdef PRINT_INT_INFO
			printk(ILITEK_ERROR_LEVEL "%s, ilitek_report_value 0x80\n", __func__);
		#endif
               	#ifdef PRINT_INT_INFO
               		printk(ILITEK_ERROR_LEVEL "%s, ilitek_report_value x= %d,y= %d\n", __func__,x,y);
			    
		#endif	    
			}
			
		}

		// release point
		if(buf[0] == 0){
			for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
                                if(touch_key_press[key]){
                                        input_report_key(input, touch_key_code[key], 0);
                                        touch_key_press[key] = 0;
                                        printk("-Key:%d ID:%d release\n", touch_key_code[key], key);
                                }
                        }
                        touch_key_hold_press = 0;
			input_event(i2c.input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
                        input_mt_sync(i2c.input_dev);
		}
                input_sync(i2c.input_dev);
	}
        return 0;
}

/*
description
	work queue function for irq use
parameter
	work
		work queue
return
	nothing
*/
static void 
ilitek_i2c_irq_work_queue_func(
	struct work_struct *work)
{
	if(ilitek_i2c_process_and_report() < 0){
		msleep(3000);
		printk(ILITEK_ERROR_LEVEL "%s, process  error\n", __func__);
	}
	msleep(5);
	//enable_irq(i2c.client->irq);
}

/*
description
	i2c interrupt service routine
parameters
	irq
		interrupt number
	dev_id
		device parameter
return
	return status
*/
static irqreturn_t 
ilitek_i2c_isr(
	int irq, void *dev_id)
{
	
	int reg_val;

#ifdef PRINT_INT_INFO		
	printk("==========------ilitek_ts TS Interrupt-----============\n"); 
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
        if (!work_pending(&i2c.irq_work)) 
        {
            #ifdef PRINT_INT_INFO
        	    printk("Enter work\n");
        	#endif
        	//disable_irq_nosync(i2c.client->irq);
        	queue_work(i2c.irq_work_queue, &i2c.irq_work);
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
	//disable_irq_nosync(i2c.client->irq);
	//queue_work(i2c.irq_work_queue, &i2c.irq_work);
	return IRQ_HANDLED;
}

/*
description
        i2c polling thread
parameters
        arg
                arguments
return
        return status
*/
static int 
ilitek_i2c_polling_thread(
	void *arg)
{
	// check input parameter
	printk(ILITEK_DEBUG_LEVEL "%s, enter\n", __func__);

	// mainloop
	while(1){
		// check whether we should exit or not
		if(kthread_should_stop()){
			printk(ILITEK_DEBUG_LEVEL "%s, stop\n", __func__);
			break;
		}

		// this delay will influence the CPU usage and response latency
		msleep(10);
		
		// when i2c is in suspend or shutdown mode, we do nothing
		if(i2c.stop_polling){
			msleep(1000);
			continue;
		}

		// read i2c data
		if(ilitek_i2c_process_and_report() < 0){
			msleep(3000);
			printk(ILITEK_ERROR_LEVEL "%s, process error\n", __func__);
		}
	}
	printk(ILITEK_DEBUG_LEVEL "%s, exit\n", __func__);
	return 0;
}

/*
description
	i2c early suspend function
parameters
	h
		early suspend pointer
return
	nothing
*/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void ilitek_i2c_early_suspend(struct early_suspend *h)
{
	ilitek_i2c_suspend(i2c.client, PMSG_SUSPEND);
	printk("%s\n", __func__);
}
#endif

/*
description
        i2c later resume function
parameters
        h
                early suspend pointer
return
        nothing
*/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void ilitek_i2c_late_resume(struct early_suspend *h)
{
	ilitek_i2c_resume(i2c.client);
	printk("%s\n", __func__);
}
#endif

/*
description
        i2c suspend function
parameters
        client
		i2c client data
	mesg
		suspend data
return
        return status
*/

static int 
ilitek_i2c_suspend(
	struct i2c_client *client, pm_message_t mesg)
{
	if(i2c.valid_irq_request != 0){
		//disable_irq(i2c.client->irq);
               // printk(ILITEK_DEBUG_LEVEL "%s, disable i2c irq\n", __func__);
               printk(ILITEK_DEBUG_LEVEL "%s, enter suspend\n", __func__);
	}
	else{
		i2c.stop_polling = 1;
 	       	printk(ILITEK_DEBUG_LEVEL "%s, stop i2c thread polling\n", __func__);
  	}
	return 0;
}

/*
description
        i2c resume function
parameters
        client
		i2c client data
return
        return status
*/
static int 
ilitek_i2c_resume(
	struct i2c_client *client)
{
        if(i2c.valid_irq_request != 0){
               // enable_irq(i2c.client->irq);
                //printk(ILITEK_DEBUG_LEVEL "%s, enable i2c irq\n", __func__);
                printk(ILITEK_DEBUG_LEVEL "%s,resume\n", __func__);
        }
	else{
		i2c.stop_polling = 0;
        	printk(ILITEK_DEBUG_LEVEL "%s, start i2c thread polling\n", __func__);
	}
	return 0;
}

/*
description
        i2c shutdown function
parameters
        client
                i2c client data
return
        nothing
*/
static void
ilitek_i2c_shutdown(
        struct i2c_client *client)
{
        printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);
        i2c.stop_polling = 1;
}
/*
description
        ilitek_ts reset
parameters
        client
                void
return
        nothing
*/
static void ilitek_ts_reset(void)
{
    printk("ilitek_ts_reset. \n");
    if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 0, "ctp_reset")){
        printk("ilitek_ts_reset: err when operate gpio. \n");
    }
    mdelay(15);
    if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 1, "ctp_reset")){
        printk("ilitek_ts_reset: err when operate gpio. \n");
    }
    mdelay(15);
}

//Function as i2c_master_send, and return 1 if operation is successful. 
static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret=-1;
	
	msg.flags = !I2C_M_RD;//Ð´ÏûÏ¢
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;		
	
	ret=i2c_transfer(client->adapter, &msg,1);
	return ret;
}

static bool ilitek_ts_i2c_test(struct i2c_client * client)
{
	int ret, retry;
	uint8_t test_data[1] = { 0 };	//only write a data address.
	
	for(retry=0; retry < 5; retry++)
	{
		ret =i2c_write_bytes(client, test_data, 1);	//Test i2c.
		if (ret == 1)
			break;
		msleep(5);
	}
	
	return ret==1 ? true : false;
}
/*
description
	when adapter detects the i2c device, this function will be invoked.
parameters
	client
		i2c client data
	id
		i2c data
return
	status
*/


static int 
ilitek_i2c_probe(
	struct i2c_client *client, 
	const struct i2c_device_id *id)
{
	int ret = 0;
	int reg_val;
	printk("enter ilitek_i2c_probe.\n");
	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
                printk(ILITEK_ERROR_LEVEL "%s, I2C_FUNC_I2C not support\n", __func__);
                return -1;
        }
        //=======================fly=================================
        gpio_addr = ioremap(PIO_BASE_ADDRESS, PIO_RANGE_SIZE);
    if(!gpio_addr) {
	    ret = -EIO;
	    goto exit_ioremap_failed;	
	}
	
    //config gpio:
    gpio_int_hdle = gpio_request_ex("ctp_para", "ctp_int_port");
    if(!gpio_int_hdle) {
        pr_warning("touch panel IRQ_EINT21_para request gpio fail!\n");
        goto exit_gpio_int_request_failed;
    }
    gpio_wakeup_hdle = gpio_request_ex("ctp_para", "ctp_wakeup");
    if(!gpio_wakeup_hdle) {
        pr_warning("touch panel tp_wakeup request gpio fail!\n");
      //  goto exit_gpio_wakeup_request_failed;
    }

    gpio_reset_hdle = gpio_request_ex("ctp_para", "ctp_reset");
    if(!gpio_reset_hdle) {
        pr_warning("touch panel tp_reset request gpio fail!\n");
        
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

    /*
    gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup");
    mdelay(5);
  */  
    gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup");

      // ilitek_ts_reset();
        //=============================================================
        client->irq = SW_INT_IRQNO_PIO;
        
	i2c.client = client;
	printk("i2c.client addr == 0x%x. \n", i2c.client->addr);
        printk(ILITEK_DEBUG_LEVEL "%s, i2c new style format\n", __func__);
	printk("%s, IRQ: 0x%X\n", __func__, client->irq);
#ifdef TEST_I2C_TRANSFER
	//TODO: used to set speed of i2c transfer. Should be change as your paltform.
   	 pr_info("Begin ilitek i2c test\n");
	ret = ilitek_ts_i2c_test(client);
	if(!ret)
	{
		pr_info("Warnning: I2C connection might be something wrong!\n");
		goto err_i2c_failed;
	}
	pr_info("===== ilitek i2c test ok=======\n");
#endif
	// register i2c device
	ret = ilitek_i2c_register_device();
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, register i2c device, error\n", __func__);
		return ret;
	}

	// allocate character device driver buffer
	ret = alloc_chrdev_region(&dev.devno, 0, 1, ILITEK_FILE_DRIVER_NAME);
    	if(ret){
        	printk(ILITEK_ERROR_LEVEL "%s, can't allocate chrdev\n", __func__);
		return ret;
	}
    	printk(ILITEK_DEBUG_LEVEL "%s, register chrdev(%d, %d)\n", __func__, MAJOR(dev.devno), MINOR(dev.devno));
	
	// initialize character device driver
	cdev_init(&dev.cdev, &ilitek_fops);
	dev.cdev.owner = THIS_MODULE;
    	ret = cdev_add(&dev.cdev, dev.devno, 1);
    	if(ret < 0){
        	printk(ILITEK_ERROR_LEVEL "%s, add character device error, ret %d\n", __func__, ret);
		return ret;
	}
	dev.class = class_create(THIS_MODULE, ILITEK_FILE_DRIVER_NAME);
	if(IS_ERR(dev.class)){
        	printk(ILITEK_ERROR_LEVEL "%s, create class, error\n", __func__);
		return ret;
    	}
    	device_create(dev.class, NULL, dev.devno, NULL, "ilitek_ctrl");
	return 0;
exit_ioremap_failed:
    if(gpio_addr){
        iounmap(gpio_addr);
    }	
exit_gpio_int_request_failed:    
exit_gpio_wakeup_request_failed:	
exit_gpio_reset_request_failed:	
err_i2c_failed:	
     return ret;
}

/*
description
	when the i2c device want to detach from adapter, this function will be invoked.
parameters
	client
		i2c client data
return
	status
*/
static int 
ilitek_i2c_remove(
	struct i2c_client *client)
{
	printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);
	i2c.stop_polling = 1;
	return 0;
}

/*
description
	read data from i2c device with delay between cmd & return data
parameter
	client
		i2c client data
	addr
		i2c address
	data
		data for transmission
	length
		data length
return
	status
*/
static int 
ilitek_i2c_read_info(
	struct i2c_client *client,
	uint8_t cmd, 
	uint8_t *data, 
	int length)
{
	int ret;
	struct i2c_msg msgs_cmd[] = {
	{.addr = client->addr, .flags = 0, .len = 1, .buf = &cmd,},
	};
	
	struct i2c_msg msgs_ret[] = {
	{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = data,}
	};

	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
	}
	
	msleep(10);
	ret = ilitek_i2c_transfer(client, msgs_ret, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);		
	}
	return ret;
}

/*
description
	read touch information
parameters
	none
return
	status
*/
static int
ilitek_i2c_read_tp_info(
	void)
{
	int res_len;
	unsigned char buf[32]={0};

	// read firmware version
        if(ilitek_i2c_read_info(i2c.client, ILITEK_TP_CMD_GET_FIRMWARE_VERSION, buf, 4) < 0){
		return -1;
	}
	printk(ILITEK_DEBUG_LEVEL "%s, firmware version %d.%d.%d.%d\n", __func__, buf[0], buf[1], buf[2], buf[3]);

	// read protocol version
        res_len = 6;
        if(ilitek_i2c_read_info(i2c.client, ILITEK_TP_CMD_GET_PROTOCOL_VERSION, buf, 2) < 0){
		return -1;
	}	
        i2c.protocol_ver = (((int)buf[0]) << 8) + buf[1];
        printk(ILITEK_DEBUG_LEVEL "%s, protocol version: %d.%d\n", __func__, buf[0], buf[1]);
        if(i2c.protocol_ver >= 0x200){
               	res_len = 8;
        }

        // read touch resolution
	i2c.max_tp = 2;
        if(ilitek_i2c_read_info(i2c.client, ILITEK_TP_CMD_GET_RESOLUTION, buf, res_len) < 0){
		return -1;
	}
        if(i2c.protocol_ver >= 0x200){
                // maximum touch point
                i2c.max_tp = buf[6];
                // maximum button number
                i2c.max_btn = buf[7];
        }
	// calculate the resolution for x and y direction
        i2c.max_x = buf[0];
        i2c.max_x+= ((int)buf[1]) * 256;
        i2c.max_y = buf[2];
        i2c.max_y+= ((int)buf[3]) * 256;
        i2c.x_ch = buf[4];
        i2c.y_ch = buf[5];
        printk(ILITEK_DEBUG_LEVEL "%s, max_x: %d, max_y: %d, ch_x: %d, ch_y: %d\n", 
		__func__, i2c.max_x, i2c.max_y, i2c.x_ch, i2c.y_ch);
        printk(ILITEK_DEBUG_LEVEL "%s, max_tp: %d, max_btn: %d\n", __func__, i2c.max_tp, i2c.max_btn);
	return 0;
}

/*
description
	register i2c device and its input device
parameters
	none
return
	status
*/
static int 
ilitek_i2c_register_device(
	void)
{
	int ret = 0;

	i2c.valid_i2c_register = 1;
	printk(ILITEK_DEBUG_LEVEL "%s, add i2c device, success\n", __func__);
	if(i2c.client == NULL){
		printk(ILITEK_ERROR_LEVEL "%s, no i2c board information\n", __func__);
		return -1;
	}
	printk(ILITEK_DEBUG_LEVEL "%s, client.addr: 0x%X\n", __func__, (unsigned int)i2c.client->addr);
	printk(ILITEK_DEBUG_LEVEL "%s, client.adapter: 0x%X\n", __func__, (unsigned int)i2c.client->adapter);
	printk(ILITEK_DEBUG_LEVEL "%s, client.driver: 0x%X\n", __func__, (unsigned int)i2c.client->driver);
	if((i2c.client->addr == 0) || (i2c.client->adapter == 0) || (i2c.client->driver == 0)){
		printk(ILITEK_ERROR_LEVEL "%s, invalid register\n", __func__);
		return ret;
	}
	
	// read touch parameter
	ilitek_i2c_read_tp_info();

	// register input device
	i2c.input_dev = input_allocate_device();
	if(i2c.input_dev == NULL){
		printk(ILITEK_ERROR_LEVEL "%s, allocate input device, error\n", __func__);
		return -1;
	}
	
	ilitek_set_input_param(i2c.input_dev, i2c.max_tp, i2c.max_x, i2c.max_y);
        	ret = input_register_device(i2c.input_dev);
        	if(ret){
               		printk(ILITEK_ERROR_LEVEL "%s, register input device, error\n", __func__);
                	return ret;
        	}
               	printk(ILITEK_ERROR_LEVEL "%s, register input device, success\n", __func__);
		i2c.valid_input_register = 1;

#ifdef CONFIG_HAS_EARLYSUSPEND
		i2c.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		i2c.early_suspend.suspend = ilitek_i2c_early_suspend;
		i2c.early_suspend.resume = ilitek_i2c_late_resume;
		register_early_suspend(&i2c.early_suspend);
#endif

	if(i2c.client->irq != 0 ){
		i2c.irq_work_queue = create_singlethread_workqueue("ilitek_i2c_irq_queue");
		if(i2c.irq_work_queue){
			INIT_WORK(&i2c.irq_work, ilitek_i2c_irq_work_queue_func);
			if(request_irq(i2c.client->irq, ilitek_i2c_isr, IRQF_TRIGGER_LOW, "ilitek_i2c_irq", &i2c)){
				printk(ILITEK_ERROR_LEVEL "%s, request irq, error\n", __func__);
			}
			else{
				i2c.valid_irq_request = 1;
                        	printk(ILITEK_ERROR_LEVEL "%s, request irq, success\n", __func__);
			}
		}
	}
	else{
		i2c.stop_polling = 0;
        	i2c.thread = kthread_create(ilitek_i2c_polling_thread, NULL, "ilitek_i2c_thread");
        	if(i2c.thread == (struct task_struct*)ERR_PTR){
            		i2c.thread = NULL;
            		printk(ILITEK_ERROR_LEVEL "%s, kthread create, error\n", __func__);
        	}
        	else{
            		wake_up_process(i2c.thread);
        	}
	}


	return 0;
}

/*
description
	initiali function for driver to invoke.
parameters

	nothing
return
	status
*/
static int 
ilitek_init(
	void)
{
	int ret = -1;
    	int ctp_used = -1;
    	char name[I2C_NAME_SIZE];
    	script_parser_value_type_t type = SCIRPT_PARSER_VALUE_TYPE_STRING;
    	pr_notice("=========ilitek-ts-init============\n");	
	// initialize global variable
    	memset(&dev, 0, sizeof(struct dev_data));
    	memset(&i2c, 0, sizeof(struct i2c_data));
    	
    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_used", &ctp_used, 1)){
        pr_err("ilitek_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    if(1 != ctp_used){
        pr_err("ilitek_ts: ctp_unused. \n");
        return 0;
    }

    if(SCRIPT_PARSER_OK != script_parser_fetch_ex("ctp_para", "ctp_name", (int *)(&name), &type, sizeof(name)/sizeof(int))){
            pr_err("ilitek_ts: script_parser_fetch err. \n");
            goto script_parser_fetch_err;
    }
    if(strcmp(ILITEK_I2C_DEVICE_NAME, name)){
        pr_err("ilitek_ts: name %s does not match ILITEK_I2C_DEVICE_NAME. \n", name);
        return 0;
    }

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_x", &screen_max_x, 1)){
        pr_err("ilitek_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    pr_info("ilitek_ts: screen_max_x = %d. \n", screen_max_x);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_y", &screen_max_y, 1)){
        pr_err("ilitek_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    pr_info("ilitek_ts: screen_max_y = %d. \n", screen_max_y);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_x_flag", &revert_x_flag, 1)){
        pr_err("ilitek_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    pr_info("ilitek_ts: revert_x_flag = %d. \n", revert_x_flag);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_y_flag", &revert_y_flag, 1)){
        pr_err("ilitek_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    pr_info("ilitek_ts: revert_y_flag = %d. \n", revert_y_flag);
    
	printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);

	// initialize mutex object
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)	
	init_MUTEX(&i2c.wr_sem);
#else
	sema_init(&i2c.wr_sem,1);
#endif

	i2c.wr_sem.count = 1;
	
	ret = i2c_add_driver(&ilitek_i2c_driver);
	if(0 != ret){
		printk(ILITEK_ERROR_LEVEL "%s, add i2c device, error\n", __func__);
		return ret;	
	}
	 
	pr_notice("=========ilitek-ts-init success============\n");
	return 0;
script_parser_fetch_err:
	pr_notice("=========ilitek-ts-init error============\n");
	return ret;	
}

/*
description
	driver exit function
parameters
	none
return
	nothing
*/
static void 
ilitek_exit(
	void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&i2c.early_suspend);
#endif
	// delete i2c driver
	if(i2c.client->irq != 0){
        	if(i2c.valid_irq_request != 0){
                	free_irq(i2c.client->irq, &i2c);
                	printk(ILITEK_DEBUG_LEVEL "%s, free irq\n", __func__);
                	if(i2c.irq_work_queue){
                        	destroy_workqueue(i2c.irq_work_queue);
                        	printk(ILITEK_DEBUG_LEVEL "%s, destory work queue\n", __func__);
                	}
        	}
	}
	else{
        	if(i2c.thread != NULL){
                	kthread_stop(i2c.thread);
                	printk(ILITEK_DEBUG_LEVEL "%s, stop i2c thread\n", __func__);
        	}
	}
        if(i2c.valid_i2c_register != 0){
                i2c_del_driver(&ilitek_i2c_driver);
                printk(ILITEK_DEBUG_LEVEL "%s, delete i2c driver\n", __func__);
        }
        if(i2c.valid_input_register != 0){
                input_unregister_device(i2c.input_dev);
                printk(ILITEK_DEBUG_LEVEL "%s, unregister i2c input device\n", __func__);
        }
        
	// delete character device driver
	cdev_del(&dev.cdev);
	unregister_chrdev_region(dev.devno, 1);
	device_destroy(dev.class, dev.devno);
	class_destroy(dev.class);
	printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);
	  gpio_release(gpio_int_hdle, 0);
    	gpio_release(gpio_wakeup_hdle, 0);
}

/* set init and exit function for this module */
late_initcall(ilitek_init);
module_exit(ilitek_exit);

