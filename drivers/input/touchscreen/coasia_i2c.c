/****************************************************************************************
 * driver/input/touchscreen/hannstar_coasia.c
 *Copyright 	:ROCKCHIP  Inc
 *Author	: 	sfm
 *Date		:  2010.2.5
 *This driver use for rk28 chip extern touchscreen. Use i2c IF ,the chip is Hannstar
 *description：
 ********************************************************************************************/
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/async.h>
//#include <mach/gpio.h>
#include <linux/irq.h>
//#include <mach/board.h>
//
//#include <mach/gpio_v2.h>
//#include <mach/irqs.h>
//#include <mach/script_v2.h>
//#include "aw_platform_ops.h"
//
//#define MAX_SUPPORT_POINT	2// //  4
//#define PACKGE_BUFLEN		10
//#define CTP_NAME                        "coasia"
//#define CONFIG_COASIA_MAX_X (screen_max_x)
//#define CONFIG_COASIA_MAX_Y (screen_max_y)
//
////#define Singltouch_Mode
//#define SAKURA_DBG                  0
//#if SAKURA_DBG 
//#define sakura_dbg_msg(fmt,...)       do {                                      \
//                                   printk("sakura dbg msg------>"                       \
//                                          " (func-->%s ; line-->%d) " fmt, __func__, __LINE__ , ##__VA_ARGS__); \
//                                  } while(0)
//#define sakura_dbg_report_key_msg(fmt,...)      do{                                                     \
//                                                    printk("sakura report " fmt,##__VA_ARGS__);          \
//                                                }while(0)
//#else
//#define sakura_dbg_msg(fmt,...)       do {} while(0)
//#define sakura_dbg_report_key_msg(fmt,...)      do{}while(0)
//#endif
//
////////////////////////////////////////////////////////
//static void* __iomem gpio_addr = NULL;
//static int gpio_int_hdle = 0;
//static int gpio_wakeup_hdle = 0;
//static int gpio_reset_hdle = 0;
//static int gpio_wakeup_enable = 1;
//static int gpio_reset_enable = 1;
//
//static int screen_max_x = 0;
//static int screen_max_y = 0;
//static int revert_x_flag = 0;
//static int revert_y_flag = 0;
//
///*
// * aw_get_pendown_state  : get the int_line data state, 
// * 
// * return value:
// *             return PRESS_DOWN: if down
// *             return FREE_UP: if up,
// *             return 0: do not need process, equal free up.          
// */
//static int aw_get_pendown_state(void)
//{
//	unsigned int reg_val;
//	static int state = FREE_UP;
//
//    //get the input port state
//    reg_val = readl(gpio_addr + PIOH_DATA);
//	  //printk("reg_val = %x\n",reg_val);
//    if(!(reg_val & (1<<IRQ_NO))) 
//    {
//        state = PRESS_DOWN;
//        //printk("pen down\n");
//        return PRESS_DOWN;
//    }
//    //touch panel is free up
//    else   
//    {
//        state = FREE_UP;
//        return FREE_UP;
//    }
//}
//
///**
// * aw_clear_penirq - clear int pending
// *
// */
//static void aw_clear_penirq(void)
//{
//	int reg_val;
//	reg_val = readl(gpio_addr + PIO_INT_STAT_OFFSET);         
//  writel(reg_val,gpio_addr + PIO_INT_STAT_OFFSET);
//
//}
//
///**
// * aw_set_irq_mode - according sysconfig's subkey "ctp_int_port" to config int port.
// * 
// * return value: 
// *              0:      success;
// *              others: fail; 
// */
//static int aw_set_irq_mode(void)
//{
//    int reg_val;
//    int ret = 0;
//    //printk("INT21 INTERRUPT CONFIG\n");
//    reg_val = readl(gpio_addr + PIO_PH_CFG2);
//    reg_val &=(~(7<<20));
//    reg_val |=(6<<20);
//    writel(reg_val,gpio_addr + PIO_PH_CFG2);
//    
//    
//    //Config IRQ_EINT21 Negative Edge Interrupt
//    reg_val = readl(gpio_addr + PIO_INT_CFG2_OFFSET);
//    reg_val &=(~(7<<20));
//    reg_val |=(1<<20);  
//    writel(reg_val,gpio_addr + PIO_INT_CFG2_OFFSET);
//    
//    //Enable IRQ_EINT21 of PIO Interrupt
//    reg_val = readl(gpio_addr + PIO_INT_CTRL_OFFSET);
//    reg_val |=(1<<IRQ_EINT21);
//    writel(reg_val,gpio_addr + PIO_INT_CTRL_OFFSET);
//    msleep(2);
//
//
//    return ret;  
//}
//
///**
// * aw_set_gpio_mode - according sysconfig's subkey "ctp_io_port" to config io port.
// *
// * return value: 
// *              0:      success;
// *              others: fail; 
// */
//static int aw_set_gpio_mode(void)
//{
//    //int reg_val;
//    int ret = 0;
//    int reg_val;
//    //config gpio to io mode
//    reg_val = readl(gpio_addr + PIO_INT_CTRL_OFFSET);
//    reg_val &=(~(1<<IRQ_EINT21));
//    writel(reg_val,gpio_addr + PIO_INT_CTRL_OFFSET);
// 
//    reg_val = readl(gpio_addr + PIO_PH_CFG2);
//    reg_val &=(~(7<<20));
//    writel(reg_val,gpio_addr + PIO_PH_CFG2);
//    return ret;
//}
//
//
///**
// * aw_judge_int_occur - whether interrupt occur.
// *
// * return value: 
// *              0:      int occur;
// *              others: no int occur; 
// */
//static int aw_judge_int_occur(void)
//{
//    int reg_val;
//    int ret = -1;
//
//    reg_val = readl(gpio_addr + PIO_INT_STAT_OFFSET);
//    if(reg_val&(1<<(IRQ_NO)))
//    {
//        ret = 0;
//    }
//    return ret; 	
//}
//
///**
// * aw_free_platform_resource - corresponding with aw_init_platform_resource
// *
// */
//static void aw_free_platform_resource(void)
//{
//    if(gpio_addr){
//        iounmap(gpio_addr);
//    }
//    if(gpio_int_hdle)
//    {
//    	gpio_release(gpio_int_hdle, 2);
//    }
//    if(gpio_wakeup_hdle){
//        gpio_release(gpio_wakeup_hdle, 2);
//    }
//    if(gpio_reset_hdle){
//        gpio_release(gpio_reset_hdle, 2);
//    }
//    
//    return;
//}
//
//
///**
// * aw_init_platform_resource - initialize platform related resource
// * return value: 0 : success
// *               -EIO :  i/o err.
// *
// */
//static int aw_init_platform_resource(void)
//{
//	int ret = 0;
//	
//        gpio_addr = ioremap(PIO_BASE_ADDRESS, PIO_RANGE_SIZE);
//        //printk("%s, gpio_addr = 0x%x. \n", __func__, gpio_addr);
//   if(!gpio_addr) {
//	    ret = -EIO;
//	    goto exit_ioremap_failed;	
//	 }
//	 
//    gpio_wakeup_hdle = gpio_request_ex("ctp_para", "ctp_wakeup");
//    if(!gpio_wakeup_hdle) {
//        pr_warning("touch panel tp_wakeup request gpio fail!\n");
//        //ret = EIO;
//        gpio_wakeup_enable = 0;
//        //goto exit_gpio_wakeup_request_failed;
//    }
//
//    gpio_reset_hdle = gpio_request_ex("ctp_para", "ctp_reset");
//    if(!gpio_reset_hdle) {
//        pr_warning("touch panel tp_reset request gpio fail!\n");
//        //ret = EIO;
//        gpio_reset_enable = 0;
//        //goto exit_gpio_reset_request_failed;
//        
//    }
//    
// 
//    ret = aw_set_irq_mode();
//    if(ret){
//        ret = -EIO;
//        goto exit_gpio_int_request_failed;
//    }
//    return ret;
//    
//exit_gpio_int_request_failed: 
//exit_ioremap_failed:
//aw_free_platform_resource();
//    return ret;
//}
//
//
///**
// * aw_fetch_sysconfig_para - get config info from sysconfig.fex file.
// * return value:  
// *                    = 0; success;
// *                    < 0; err
// */
//static int aw_fetch_sysconfig_para(void)
//{
//    int ret = -1;
//    int ctp_used = -1;
//    char name[I2C_NAME_SIZE];
//    script_parser_value_type_t type = SCIRPT_PARSER_VALUE_TYPE_STRING;
//
//    printk("%s. \n", __func__);
//    
//    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_used", &ctp_used, 1)){
//        pr_err("ilitek_ts: script_parser_fetch err. \n");
//        goto script_parser_fetch_err;
//    }
//    if(1 != ctp_used){
//        pr_err("ilitek_ts: ctp_unused. \n");
//        //ret = 1;
//        return ret;
//    }
//
//    if(SCRIPT_PARSER_OK != script_parser_fetch_ex("ctp_para", "ctp_name", (int *)(&name), &type, sizeof(name)/sizeof(int))){
//            pr_err("ilitek_ts: script_parser_fetch err. \n");
//            goto script_parser_fetch_err;
//    }
//    if(strcmp(CTP_NAME, name)){
//        pr_err("ilitek_ts: name %s does not match CTP_NAME. \n", name);
//        pr_err(CTP_NAME);
//        //ret = 1;
//        return ret;
//    }
//
//    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_x", &screen_max_x, 1)){
//        pr_err("ilitek_ts: script_parser_fetch err. \n");
//        goto script_parser_fetch_err;
//    }
//    pr_info("ilitek_ts: screen_max_x = %d. \n", screen_max_x);
//
//    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_y", &screen_max_y, 1)){
//        pr_err("ilitek_ts: script_parser_fetch err. \n");
//        goto script_parser_fetch_err;
//    }
//    pr_info("ilitek_ts: screen_max_y = %d. \n", screen_max_y);
//
//    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_x_flag", &revert_x_flag, 1)){
//        pr_err("ilitek_ts: script_parser_fetch err. \n");
//        goto script_parser_fetch_err;
//    }
//    pr_info("ilitek_ts: revert_x_flag = %d. \n", revert_x_flag);
//
//    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_y_flag", &revert_y_flag, 1)){
//        pr_err("ilitek_ts: script_parser_fetch err. \n");
//        goto script_parser_fetch_err;
//    }
//    pr_info("ilitek_ts: revert_y_flag = %d. \n", revert_y_flag);
//
//    return 0;
//
//script_parser_fetch_err:
//    pr_notice("=========script_parser_fetch_err============\n");
//    return ret;
//}
//
///**
// * aw_ts_reset - function
// *
// */
//static void aw_ts_reset(void)
//{
//    printk("%s. \n", __func__);
//    if(1 == gpio_reset_enable){
//        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 0, "ctp_reset")){
//            printk("ilitek_ts_reset: err when operate gpio. \n");
//        }
//    }
//    
//    if(1 == gpio_wakeup_enable){  
//        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup")){
//            printk("ts_resume: err when operate gpio. \n");
//        }
//
//    }
//    
//}
//
///**
// * aw_ts_wakeup - function
// *
// */
//static void aw_ts_wakeup(void)
//{
//    if(1 == gpio_reset_enable){
//        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 1, "ctp_reset")){
//            printk("ilitek_ts_reset: err when operate gpio. \n");
//        }
//        msleep(10);
//    }
//    
//    if(1 == gpio_wakeup_enable){  
//        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup")){
//            printk("ts_resume: err when operate gpio. \n");
//        }
//        msleep(10);
//    }
//    
//    return;
//}
//////////////////////////////////////////////////////////////////
//
//static struct aw_platform_ops aw_ops = {
//	.get_pendown_state = aw_get_pendown_state,
//	.clear_penirq	   = aw_clear_penirq,
//	.set_irq_mode      = aw_set_irq_mode,
//	.set_gpio_mode     = aw_set_gpio_mode,	
//	.judge_int_occur   = aw_judge_int_occur,
//	.init_platform_resource = aw_init_platform_resource,
//	.free_platform_resource = aw_free_platform_resource,
//	.fetch_sysconfig_para = aw_fetch_sysconfig_para,
//	.ts_reset =          aw_ts_reset,
//	.ts_wakeup =         aw_ts_wakeup,
//};
//
//struct point_data {	
//	short status;	
//	short x;	
//	short y;
//    short z;
//};
//
//struct multitouch_event{
//   	u16 x1;
//	u16 y1;
//	u16 x2;
//	u16 y2;
//};
//
//struct coasia_dev {
//	struct input_dev	*input;
//	char			phys[32];
//	struct delayed_work	work;
//	struct workqueue_struct *wq;
//
//	struct i2c_client	*client;
//	u16			model;
//	
//	u32         power_pin;
//    u32         rest_pin;
//    u32         int_pin;
//	
//	bool		pendown;
//	bool 	 	status;
//	int			irq;
//	int 		has_relative_report;
//};
//struct coasia_dev *gcoasia_ts = NULL;
//
///**********************************************************************************************/
//
//#define COMMAND_XY 0x00
//#define SIZE_8_4
//#define OUTSIZEX 0
//#define OUTSIZEY 0
//
//
//#define MAX_FINGER_NUM 3
//#define DEVICE_ADDRESS 0x5C//0x8C
//#define COMMAND_BUFFER_INDEX 0x15
//#define TEST_REG             0x37
//#define QUERY_BUFFER_INDEX 0x80
//#define COMMAND_RESPONSE_BUFFER_INDEX 0xA0
//#define POINT_BUFFER_INDEX 0xE0
//#define QUERY_SUCCESS 0x00
//#define QUERY_BUSY 0x01
//#define QUERY_ERROR 0x02
//#define QUERY_POINT 0x80
//#define POINT_X_HIGH_MASK 0x0f
//#define POINT_Y_HIGH_MASK 0xf0
//#define POINT_INFO_MASK 0x07
//#define POINT_PLAM_MASK 0x01
//
//
//
//static u8 calibration_flag = 0;
//
///*read the gtt8205 register ,used i2c bus*/
//static int cj3b_read_regs(struct i2c_client *client, u8 reg, u8 buf[], unsigned len)
//{
//	struct i2c_msg msgs[2];
//	int ret=-1;
//	//发送写地址
//	msgs[0].flags = !I2C_M_RD;
//	msgs[0].addr = client->addr;
//	msgs[0].len = 1;		//data address
//	msgs[0].buf = &reg;
//	//接收数据
//	msgs[1].flags = I2C_M_RD;//读消息
//	msgs[1].addr = client->addr;
//	//msgs[1].len = len-1;
//	msgs[1].len = len;
//	msgs[1].buf = buf;
//	
//	//printk("%s. \n", __func__);
//	ret=i2c_transfer(client->adapter, msgs, 2);
//	if(ret<0){
//            printk("%s. i2c_transfer err. \n", __func__);
//	}
//	return ret;
//}
//
//
//
//
//static int cj3b_set_regs(struct i2c_client *client, u8 reg, u8 buf[], unsigned short len)
//{
//	struct i2c_msg msg;
//	int ret=-1;
//	u8  buff[2];
//	buff[0] = reg; 
//	buff[1] = buf[0];
//	msg.flags = !I2C_M_RD;//写消息
//	msg.addr = client->addr;
//	msg.len = len + 1;
//	msg.buf = buff;		
//	ret=i2c_transfer(client->adapter, &msg,1);
//	if(ret<0){
//            printk("%s. i2c_transfer err. \n", __func__);
//	}else
//	{
//		        printk("write data ok ret = %d,addr = %x\n",ret,client->addr);
//	}
//	return ret;
//	
//	return ret;
//	
//}
//
//
//static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, uint16_t len)
//{
//	struct i2c_msg msgs[2];
//	int ret=-1;
//	//·￠?íD?μ??·
//	msgs[0].flags = !I2C_M_RD;
//	msgs[0].addr = client->addr;
//	msgs[0].len = 1;		//data address
//	msgs[0].buf = buf;
//	//?óê?êy?Y
//	msgs[1].flags = I2C_M_RD;//?á???￠
//	msgs[1].addr = client->addr;
//	msgs[1].len = len-1;
//	msgs[1].buf = buf+1;
//	
//	ret=i2c_transfer(client->adapter, msgs, 2);
//	return ret;
//}
//
////Function as i2c_write_bytes, and return 1 if operation is successful. 
//static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
//{
//	struct i2c_msg msg;
//	int ret=-1;
//	
//	msg.flags = !I2C_M_RD;//D????￠
//	msg.addr = client->addr;
//	msg.len = len;
//	msg.buf = data;		
//	
//	ret=i2c_transfer(client->adapter, &msg,1);
//	return ret;
//}
//
//
//
//#define TP_STATE_IDLE         0
//#define TP_STATE_PRE_DOWN     1
//#define TP_STATE_DOWN         2
//#define TP_STATE_PRE_IDLE     3
//
//unsigned char tp_state = TP_STATE_IDLE;
//
//
//int read_point(struct coasia_dev *ts_dev )
//{
//		u8 pucPoint[10];
//		int X,Y,X1,Y1;
//		short Status;
//		int ret;
//		u8 data;
//		
//		cj3b_read_regs(ts_dev->client,COMMAND_XY,pucPoint,10);
//		
//		Status = 0;
//		X =  ((pucPoint[1]<<8) | pucPoint[0]);
//	  Y =  ((pucPoint[3]<<8) | pucPoint[2]);
//		
//		X1 = ((pucPoint[5]<<8) | pucPoint[4]);
//	  Y1 = ((pucPoint[7]<<8) | pucPoint[6]);
//  
//	  if((X != 0) || (Y != 0) || (X1 != 0) || (Y1 != 0)){
//			data = 113;
//			ret = i2c_write_bytes(tsdata->client, &data, 1);
//			ret = i2c_read_bytes(tsdata->client,, &data, 1);
//			Status = data>>5;
//		}
//
//    if(Status == 1)
//    {	
//        input_report_abs(ts_dev->input, ABS_MT_TRACKING_ID, 0);									     
//        input_report_abs(ts_dev->input, ABS_MT_TOUCH_MAJOR, 1);
//        input_report_abs(ts_dev->input, ABS_MT_WIDTH_MAJOR, 0);				
//        input_report_abs(ts_dev->input, ABS_MT_POSITION_X, X);				
//        input_report_abs(ts_dev->input, ABS_MT_POSITION_Y, Y);				
//        input_mt_sync(ts_dev->input);
//        input_sync(ts_dev->input);
//        ts_dev->pendown =1;
//        //printk("state=%d, send coordinate: x=%d, y=%d\n", Status, X, Y);
//    }
//    else if(Status == 2)
//    {
//        input_report_abs(ts_dev->input, ABS_MT_TRACKING_ID, 0);
//        input_report_abs(ts_dev->input, ABS_MT_TOUCH_MAJOR, 1);
//        input_report_abs(ts_dev->input, ABS_MT_WIDTH_MAJOR, 0);        
//        input_report_abs(ts_dev->input, ABS_MT_POSITION_X, X);                    
//        input_report_abs(ts_dev->input, ABS_MT_POSITION_Y, Y);
//        input_mt_sync(ts_dev->input);   
//    
//        input_report_abs(ts_dev->input, ABS_MT_TRACKING_ID, 1);
//        input_report_abs(ts_dev->input, ABS_MT_TOUCH_MAJOR, 1);
//        input_report_abs(ts_dev->input, ABS_MT_WIDTH_MAJOR, 0);        
//        input_report_abs(ts_dev->input, ABS_MT_POSITION_X, X1);                    
//        input_report_abs(ts_dev->input, ABS_MT_POSITION_Y, Y1);                    
//                
//        input_mt_sync(ts_dev->input);   
//        input_sync(ts_dev->input);     	
//        ts_dev->pendown =1;
//        //printk("state=%d, send coordinate: x=%d, y=%d , x1 = %d, y1 = %d\n", Status, X, Y, X1, Y1);
//    }
//    else if(Status == 0)
//    {
//        input_report_abs(ts_dev->input, ABS_MT_TOUCH_MAJOR,0);
//        input_mt_sync(ts_dev->input);
//        input_sync(ts_dev->input);	
//    }
//    
//	return 0;
//}
//
//int consia_irq_flag = 0;
//static void coasia_work(struct work_struct *work)
//{
//	int rt;
//	struct coasia_dev *ts = container_of(to_delayed_work(work), struct coasia_dev, work);
//  msleep(2);
//  if(aw_ops.get_pendown_state() == PRESS_DOWN)
//  {
//		  rt = read_point(ts);
//			queue_delayed_work(ts->wq, &ts->work, msecs_to_jiffies(20));
//	}else
//	{
//		rt = read_point(ts);
//		aw_ops.set_irq_mode();
//	}
//}
//
//static irqreturn_t coasia_irq(int irq, void *handle)
//{
//	struct coasia_dev *ts = handle;
//	if(!aw_judge_int_occur())
//	{	
//		aw_clear_penirq();
//		aw_set_gpio_mode();
//		queue_delayed_work(ts->wq, &ts->work, 5);
//	}
//	else
//	{
//	    printk("Other Interrupt\n");
//	}
//	
//    
//	return IRQ_HANDLED;
//}
//
//static void coasia_free_irq(struct coasia_dev *ts)
//{
//	free_irq(ts->irq, ts);
//	if (cancel_delayed_work_sync(&ts->work)) {
//		/*
//		 * Work was pending, therefore we need to enable
//		 * IRQ here to balance the disable_irq() done in the
//		 * interrupt handler.
//		 */
//	}
//}
//
//#ifdef CONFIG_HAS_EARLYSUSPEND
//static void coasia_early_suspend(struct early_suspend *handler)
//{
//	printk("===Enter early suspend mode====\n");
//	aw_ops.ts_reset();
//}
//
//static void coasia_early_resume(struct early_suspend *handler)
//{
//	printk("===Enter early Resume mode====\n");
//	aw_ops.ts_wakeup();
//}
//
//static struct early_suspend coasia_i2c_early_suspend = {
//	.suspend = coasia_early_suspend,
//	.resume  = coasia_early_resume,
//	.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
//};
//#endif
//
//#ifdef CONFIG_PM
//static int coasia_suspend(struct i2c_client *client, pm_message_t mesg)
//{
//	printk("===Coasia Enter suspend mode====\n");
//  aw_ops.ts_reset();
//	
//	return 0;
//}
//
//static int coasia_resume(struct i2c_client *client)
//{
//	printk("===Coasia Enter Resume mode====\n");
//	aw_ops.ts_wakeup();
//	return 0;
//}
//#endif
//
//static ssize_t touch_mode_show(struct class *cls, char *_buf)
//{
//	if(calibration_flag == 1)
//	{
//	    calibration_flag = 0;
//		return sprintf(_buf,"successful");
//	}
//	else
//	{
//	    calibration_flag = 0;
//		return sprintf(_buf,"fail");
//	}
//}
//
//static ssize_t touch_mode_store(struct class *cls, const char *_buf, size_t _count)
//{
//    u8  ucData;
//        
//    calibration_flag = 0;
//    if(!strncmp(_buf,"tp_cal" , strlen("tp_cal")))
//    {
//        //printk("TP Calibration is start!!! \n");
//	    calibration_flag = 1;	
//	    
//	    ucData = 0x03;
//	    cj3b_set_regs(gcoasia_ts->client,TEST_REG,&ucData,1);
//	    //mdelay(5000);
//	    //cj3b_read_regs(gcoasia_ts->client,TEST_REG,&ucData,1);
//
//    }
//    return _count;
//} 
//static struct class *tp_class = NULL;
////static CLASS_ATTR(touchcalibration, 0666, touch_mode_show, touch_mode_store);
//
//static int __devinit coasia_probe(struct i2c_client *client,
//				   const struct i2c_device_id *id)
//{
//	struct coasia_dev *ts;
//	client->dev.platform_data = (void *)&aw_ops;
//	struct coasia_platform_data *pdata  = client->dev.platform_data;
//	struct input_dev *input_dev;
//	u8  ucData = 0x08;
//	int err;
//
//
//	printk("======================================%s=====================\n", __func__);
//	err = aw_ops.init_platform_resource();
//	if(0 != err){
//	    printk("%s:aw_ops.init_platform_resource err. \n", __func__);    
//	}
//	
//	if (!pdata) {
//		dev_err(&client->dev, "platform data is required!\n");
//		return -EINVAL;
//	}
//
//	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
//		return -EIO;
//
//	ts = kzalloc(sizeof(struct coasia_dev), GFP_KERNEL);
//	input_dev = input_allocate_device();
//	if (!ts || !input_dev) {
//		err = -ENOMEM;
//		goto err_free_mem;
//	}
//    
//    gcoasia_ts = ts;
//	ts->client  = client;
//	ts->irq     = SW_INT_IRQNO_PIO;
//	ts->input   = input_dev;
//	ts->status  = 0 ;                                   
//	ts->pendown = 0;                                    
//
//	//ts->wq = create_rt_workqueue("coasia_wq");
//	INIT_DELAYED_WORK(&ts->work, coasia_work);
//         ts->wq = create_singlethread_workqueue(dev_name(&client->dev));
//	if (!ts->wq) 
//	{		
//	         err = -ESRCH;
//		goto exit_create_singlethread;
//	}
//	snprintf(ts->phys, sizeof(ts->phys),"%s/input0", dev_name(&client->dev));
//
//	input_dev->name = "coasia Touchscreen";
//	input_dev->phys = ts->phys;
//	input_dev->id.bustype = BUS_I2C;
//
//#if defined (Singltouch_Mode)
//	set_bit(EV_SYN, input_dev->evbit);
//	set_bit(EV_KEY, input_dev->evbit);
//	set_bit(BTN_TOUCH, input_dev->keybit);
//	set_bit(BTN_2, input_dev->keybit);
//	set_bit(EV_ABS, input_dev->evbit);
//	input_set_abs_params(input_dev,ABS_X,0,CONFIG_COASIA_MAX_X,0,0);
//	input_set_abs_params(input_dev,ABS_Y,0,CONFIG_COASIA_MAX_Y,0,0);
//#else
//	ts->has_relative_report = 0;
//	input_dev->evbit[0] = BIT_MASK(EV_ABS)|BIT_MASK(EV_KEY)|BIT_MASK(EV_SYN);
//	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
//	input_dev->keybit[BIT_WORD(BTN_2)] = BIT_MASK(BTN_2); //jaocbchen for dual
//	input_set_abs_params(input_dev, ABS_X, 0, CONFIG_COASIA_MAX_X, 0, 0);
//	input_set_abs_params(input_dev, ABS_Y, 0, CONFIG_COASIA_MAX_Y, 0, 0);
//	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
//	input_set_abs_params(input_dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);
//	input_set_abs_params(input_dev, ABS_HAT0X, 0, CONFIG_COASIA_MAX_X, 0, 0);
//	input_set_abs_params(input_dev, ABS_HAT0Y, 0, CONFIG_COASIA_MAX_Y, 0, 0);
//	input_set_abs_params(input_dev, ABS_MT_POSITION_X,0, CONFIG_COASIA_MAX_X, 0, 0);
//	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,CONFIG_COASIA_MAX_Y, 0, 0);
//	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
//	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
//	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 10, 0, 0);   
//#endif
//
//	
//	err = input_register_device(input_dev);
//	if (err)
//	{
//		goto err_free_mem;
//    }
//	i2c_set_clientdata(client, ts);
//	mdelay(500);
//	
//#ifdef CONFIG_HAS_EARLYSUSPEND
//	register_early_suspend(&coasia_i2c_early_suspend);
//#endif
//
//	
//#if 0
//	ucData = 0x03;
//	cj3b_set_regs(ts->client,TEST_REG,&ucData,1);
//	mdelay(5000);
//	cj3b_read_regs(ts->client,TEST_REG,&ucData,1);
//	printk("read test cst is %d \n",ucData);
//#else
///*
//	tp_class = class_create(THIS_MODULE, "touchpanel");
//    if (IS_ERR(tp_class)) 
//	{
//	    printk("Create class touchpanel failed.\n");
//	    err = -ENOMEM;
//	    goto err_free_mem;
// 	}
//	err = class_create_file(tp_class,&class_attr_touchcalibration);
//	printk("Create class touchpanel is sucess\n");	       
//	*/
//#endif
//	
//	ucData = 0x0a;
//	cj3b_set_regs(ts->client,COMMAND_BUFFER_INDEX,&ucData,1);
//	msleep(100);
//	
//	if (!ts->irq) 
//	{
//		dev_dbg(&ts->client->dev, "no IRQ?\n");
//		err = -ENODEV;
//		goto err_free_mem;
//	}else
//	{
//		//ts->irq = gpio_to_irq(ts->irq);
//		//aw_ops.set_irq_mode();
//		
//	}
//
//
//	err = request_irq(ts->irq, coasia_irq, IRQF_TRIGGER_FALLING,client->dev.driver->name, ts);
//	if (err < 0) 
//	{
//		dev_err(&client->dev, "irq %d busy?\n", ts->irq);
//		goto err_free_irq;
//	}
//
//	printk("====%s ok=====.  \n", __func__);
//	return 0;
//
//err_free_irq:
//	coasia_free_irq(ts);
//err_free_mem:
//	input_free_device(input_dev);
//	kfree(ts);
//exit_create_singlethread:
//
//	return err;
//}
//
//static int __devexit coasia_remove(struct i2c_client *client)
//{
//	struct coasia_dev *ts = i2c_get_clientdata(client);
//	coasia_free_irq(ts);
//	
//#ifdef CONFIG_HAS_EARLYSUSPEND
//	unregister_early_suspend(&coasia_i2c_early_suspend);
//#endif
//
//
//
//	input_unregister_device(ts->input);
//	kfree(ts);
//
//	aw_ops.free_platform_resource();
//
//	return 0;
//}
//
//static struct i2c_device_id coasia_idtable[] = {
//	{ CTP_NAME, 0 },
//	{ }
//};
//
//MODULE_DEVICE_TABLE(i2c, coasia_idtable);
//
//static struct i2c_driver coasia_driver = {
//	.driver = {
//		.owner	= THIS_MODULE,
//		.name	= CTP_NAME
//	},
//	.id_table	= coasia_idtable,
//	.probe		= coasia_probe,
//	.remove		= __devexit_p(coasia_remove),
//#ifdef CONFIG_PM
//	.suspend    = coasia_suspend,
//	.resume     = coasia_resume,
//#endif
//};
//
//static void __init coasia_init_async(void *unused, async_cookie_t cookie)
//{
//	//printk("--------> %s <-------------\n",__func__);
//	i2c_add_driver(&coasia_driver);
//}
//
//static int __init coasia_init(void)
//{
//	if (aw_ops.fetch_sysconfig_para)
//	{
//		if(aw_ops.fetch_sysconfig_para()){
//         printk("%s: err.\n", __func__);
//         return -1;
//		}
//	}
//	async_schedule(coasia_init_async, NULL);
//	return 0;
//}
//
//static void __exit coasia_exit(void)
//{
//	return i2c_del_driver(&coasia_driver);
//}
//module_init(coasia_init);
//module_exit(coasia_exit);
//MODULE_LICENSE("GPL");

