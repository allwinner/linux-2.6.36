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
 *  for this touchscreen to work, it's slave addr must be set to 0x7e | 0x70
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include "ft5x_ts.h"
#ifdef CONFIG_HAS_EARLYSUSPEND
    #include <linux/pm.h>
    #include <linux/earlysuspend.h>
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
#include <mach/script_v2.h>

#define FOR_TSLIB_TEST
//#define PRINT_INT_INFO
//#define PRINT_POINT_INFO
#define DEBUG
//#define TOUCH_KEY_SUPPORT
//#define TOUCH_KEY_LIGHT_SUPPORT
static struct i2c_client *this_client;

static int gpio_int_hdle = 0;
static int gpio_wakeup_hdle = 0;
static int gpio_reset_hdle = 0;
#ifdef TOUCH_KEY_LIGHT_SUPPORT
static int gpio_light_hdle = 0;
#endif
#ifdef TOUCH_KEY_SUPPORT
static int key_tp  = 0;
static int key_val = 0;
#endif

static void* __iomem gpio_addr = NULL;

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static int ctp_reset_enable = 0;
static int ctp_wakeup_enable = 0;

#define SCREEN_MAX_X    (screen_max_x)
#define SCREEN_MAX_Y    (screen_max_y)
#define PRESS_MAX       255

static int ft5x_i2c_rxdata(char *rxdata, int length);

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
	s16 touch_ID1;
	s16 touch_ID2;
	s16 touch_ID3;
	s16 touch_ID4;
	s16 touch_ID5;
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


/* ---------------------------------------------------------------------
*
*   Focal Touch panel upgrade related driver
*
*
----------------------------------------------------------------------*/
//#define CONFIG_SUPPORT_FTS_CTP_UPG


#ifdef CONFIG_SUPPORT_FTS_CTP_UPG

typedef enum
{
    ERR_OK,
    ERR_MODE,
    ERR_READID,
    ERR_ERASE,
    ERR_STATUS,
    ERR_ECC,
    ERR_DL_ERASE_FAIL,
    ERR_DL_PROGRAM_FAIL,
    ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

typedef unsigned char         FTS_BYTE;     //8 bit
typedef unsigned short        FTS_WORD;    //16 bit
typedef unsigned int          FTS_DWRD;    //16 bit
typedef unsigned char         FTS_BOOL;    //8 bit 

#define FTS_NULL                0x0
#define FTS_TRUE                0x01
#define FTS_FALSE               0x0

#define I2C_CTPM_ADDRESS       (0x70>>1)

void delay_ms(FTS_WORD  w_ms)
{
    //platform related, please implement this function
    msleep( w_ms );
}


/*
[function]: 
    callback: read data from ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[out]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/
int i2c_read_interface(u8 bt_ctpm_addr, u8* pbt_buf, u16 dw_lenth)
{
    int ret;
    
    ret=i2c_master_recv(this_client, pbt_buf, dw_lenth);

    if(ret != dw_lenth)
    {
        printk("ret = %d. \n", ret);
        printk("i2c_read_interface error\n");
        return FTS_FALSE;
    }
  
    return FTS_TRUE;
}

/*
[function]: 
    callback: write data to ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[in]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/
int i2c_write_interface(u8 bt_ctpm_addr, u8* pbt_buf, u16 dw_lenth)
{
    int ret;
    ret=i2c_master_send(this_client, pbt_buf, dw_lenth);
    if(ret != dw_lenth)
    {
        printk("i2c_write_interface error\n");
        return FTS_FALSE;
    }

    return FTS_TRUE;
}


/***************************************************************************************/

/*
[function]: 
    read out the register value.
[parameters]:
    e_reg_name[in]    :register name;
    pbt_buf[out]    :the returned register value;
    bt_len[in]        :length of pbt_buf, should be set to 2;        
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
u8 fts_register_read(u8 e_reg_name, u8* pbt_buf, u8 bt_len)
{
    u8 read_cmd[3]= {0};
    u8 cmd_len     = 0;

    read_cmd[0] = e_reg_name;
    cmd_len = 1;    

    /*call the write callback function*/
//    if(!i2c_write_interface(I2C_CTPM_ADDRESS, &read_cmd, cmd_len))
//    {
//        return FTS_FALSE;
//    }


    if(!i2c_write_interface(I2C_CTPM_ADDRESS, read_cmd, cmd_len))//change by zhengdixu
    {
        return FTS_FALSE;
    }

    /*call the read callback function to get the register value*/        
    if(!i2c_read_interface(I2C_CTPM_ADDRESS, pbt_buf, bt_len))
    {
        return FTS_FALSE;
    }
    return FTS_TRUE;
}

/*
[function]: 
    write a value to register.
[parameters]:
    e_reg_name[in]    :register name;
    pbt_buf[in]        :the returned register value;
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
int fts_register_write(u8 e_reg_name, u8 bt_value)
{

    FTS_BYTE write_cmd[2] = {0};

    write_cmd[0] = e_reg_name;
    write_cmd[1] = bt_value;

    /*call the write callback function*/
    //return i2c_write_interface(I2C_CTPM_ADDRESS, &write_cmd, 2);
    return i2c_write_interface(I2C_CTPM_ADDRESS, write_cmd, 2); //change by zhengdixu
}

/*
[function]: 
    send a command to ctpm.
[parameters]:
    btcmd[in]        :command code;
    btPara1[in]    :parameter 1;    
    btPara2[in]    :parameter 2;    
    btPara3[in]    :parameter 3;    
    num[in]        :the valid input parameter numbers, if only command code needed and no parameters followed,then the num is 1;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
int cmd_write(u8 btcmd,u8 btPara1,u8 btPara2,u8 btPara3,u8 num)
{
    FTS_BYTE write_cmd[4] = {0};

    write_cmd[0] = btcmd;
    write_cmd[1] = btPara1;
    write_cmd[2] = btPara2;
    write_cmd[3] = btPara3;
    //return i2c_write_interface(I2C_CTPM_ADDRESS, &write_cmd, num);
    return i2c_write_interface(I2C_CTPM_ADDRESS, write_cmd, num);//change by zhengdixu
}

/*
[function]: 
    write data to ctpm , the destination address is 0.
[parameters]:
    pbt_buf[in]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
int byte_write(u8* pbt_buf, u16 dw_len)
{
    
    return i2c_write_interface(I2C_CTPM_ADDRESS, pbt_buf, dw_len);
}

/*
[function]: 
    read out data from ctpm,the destination address is 0.
[parameters]:
    pbt_buf[out]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
int byte_read(u8* pbt_buf, u8 bt_len)
{
    return i2c_read_interface(I2C_CTPM_ADDRESS, pbt_buf, bt_len);
    //ft5x_i2c_rxdata
}


/*
[function]: 
    burn the FW to ctpm.
[parameters]:(ref. SPEC)
    pbt_buf[in]    :point to Head+FW ;
    dw_lenth[in]:the length of the FW + 6(the Head length);    
    bt_ecc[in]    :the ECC of the FW
[return]:
    ERR_OK        :no error;
    ERR_MODE    :fail to switch to UPDATE mode;
    ERR_READID    :read id fail;
    ERR_ERASE    :erase chip fail;
    ERR_STATUS    :status error;
    ERR_ECC        :ecc error.
*/


#define    FTS_PACKET_LENGTH        2//4//8//16//32//64//128//256

static unsigned char CTPM_FW[]=
{
#include "ft_app.i"
};

E_UPGRADE_ERR_TYPE  fts_ctpm_fw_upgrade(u8* pbt_buf, u16 dw_lenth)
{
    u8 reg_val[2] = {0};
    FTS_BOOL i_ret = 0;
    u16 i = 0;
    

    u16  packet_number;
    u16  j;
    u16  temp;
    u16  lenght;
    u8  packet_buf[FTS_PACKET_LENGTH + 6];
    u8  auc_i2c_write_buf[10];
    u8 bt_ecc;

    /*********Step 1:Reset  CTPM *****/
    /*write 0xaa to register 0xfc*/
    delay_ms(1500);
    fts_register_write(0xfc,0xaa);
    delay_ms(50);
     /*write 0x55 to register 0xfc*/
    fts_register_write(0xfc,0x55);
    printk("Step 1: Reset CTPM test\n");

    delay_ms(30);

    /*********Step 2:Enter upgrade mode *****/
     auc_i2c_write_buf[0] = 0x55;
     auc_i2c_write_buf[1] = 0xaa;
     i = 0;
     do{
        i++;
        i_ret = i2c_write_interface(I2C_CTPM_ADDRESS, auc_i2c_write_buf, 2);
        printk("Step 2: Enter update mode. \n");
        delay_ms(5);
     }while((FTS_FALSE == i_ret) && i<5);

    /*********Step 3:check READ-ID***********************/
    /*send the opration head*/
    i = 0;
    do{
        if(i > 3)
        {
          cmd_write(0x07,0x00,0x00,0x00,1);
		  return ERR_READID; 
        }
        /*read out the CTPM ID*/
        printk("====Step 3:check READ-ID====");
        cmd_write(0x90,0x00,0x00,0x00,4);
        byte_read(reg_val,2);
        i++;
        delay_ms(5);
        printk("Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
    }while(reg_val[1] != 0x03);//while(reg_val[0] != 0x79 || reg_val[1] != 0x03);

     /*********Step 4:erase app*******************************/
    cmd_write(0x61,0x00,0x00,0x00,1);
    delay_ms(1500);
    printk("Step 4: erase. \n");

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;
    printk("Step 5: start upgrade. \n");
    dw_lenth = dw_lenth - 8;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    for (j=0;j<packet_number;j++)
    {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(lenght>>8);
        packet_buf[5] = (FTS_BYTE)lenght;

        for (i=0;i<FTS_PACKET_LENGTH;i++)
        {
            packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }
        
        byte_write(&packet_buf[0],FTS_PACKET_LENGTH + 6);
        delay_ms(FTS_PACKET_LENGTH/6 + 1);
        if ((j * FTS_PACKET_LENGTH % 1024) == 0)
        {
              printk("upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
    {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;

        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;

        for (i=0;i<temp;i++)
        {
            packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }

        byte_write(&packet_buf[0],temp+6);    
        delay_ms(20);
    }

    //send the last six byte
    for (i = 0; i<6; i++)
    {
        temp = 0x6ffa + i;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        temp =1;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;
        packet_buf[6] = pbt_buf[ dw_lenth + i]; 
        bt_ecc ^= packet_buf[6];

        byte_write(&packet_buf[0],7);  
        delay_ms(20);
    }

    /*********Step 6: read out checksum***********************/
    /*send the opration head*/
    //cmd_write(0xcc,0x00,0x00,0x00,1);//把0xcc当作寄存器地址，去读出一个字节
   // byte_read(reg_val,1);//change by zhengdixu

	fts_register_read(0xcc, reg_val,1);
	
    printk("Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    if(reg_val[0] != bt_ecc)
    {
       cmd_write(0x07,0x00,0x00,0x00,1);
		return ERR_ECC;
    }

    /*********Step 7: reset the new FW***********************/
    cmd_write(0x07,0x00,0x00,0x00,1);
    msleep(1500);
    return ERR_OK;
}

void getVerNo(u8* buf, int len)
{
	u8 start_reg=0x0;
	int ret = -1;
	//int status = 0;
	int i = 0;
	start_reg = 0xa6;

#if 0
	printk("read 0xa6 one time. \n");
	if(FTS_FALSE == fts_register_read(0xa6, buf, len)){
        return ;
	}
	
	for (i=0; i< len; i++) 
	{
		printk("=========buf[%d] = 0x%x \n", i, buf[i]);
	}
	
	printk("read 0xa8. \n");
	if(FTS_FALSE == fts_register_read(0xa8, buf, len)){
        return ;
	}
	for (i=0; i< len; i++) 
	{
		printk("=========buf[%d] = 0x%x \n", i, buf[i]);
	}

	ft5x_i2c_rxdata(buf, len);
	
    for (i=0; i< len; i++) 
        {
            printk("=========buf[%d] = 0x%x \n", i, buf[i]);
        }

    byte_read(buf, len);
    for (i=0; i< len; i++) 
    {
        printk("=========buf[%d] = 0x%x \n", i, buf[i]);
    }
          
#endif

	ret =fts_register_read(0xa6, buf, len);
	//et = ft5406_read_regs(ft5x0x_ts_data_test->client,start_reg, buf, 2);
    if (ret < 0) 
	{
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return;
	}
	for (i=0; i<2; i++) 
	{
		printk("=========buf[%d] = 0x%x \n", i, buf[i]);
	}


	return;
}

int fts_ctpm_fw_upgrade_with_i_file(void)
{
   FTS_BYTE*     pbt_buf = FTS_NULL;
   int i_ret;
   u8 a,b;
#define BUFFER_LEN (2)            //len == 2 
   unsigned char buf[BUFFER_LEN] = {0};
   
	//=========FW upgrade========================*/
	pbt_buf = CTPM_FW;
	msleep(1500);
	cmd_write(0x07,0x00,0x00,0x00,1);
	msleep(1500);
	getVerNo(buf, BUFFER_LEN);
	a = buf[0];
	b = pbt_buf[0];
	printk("buf[0] == %hu, pbt_buf[0] == %hu \n", buf[0], pbt_buf[0]);
	if (a >= b )
	{
		return 0; //强制升级触摸IC  phm 
	}
	/*call the upgrade function*/
	i_ret =  fts_ctpm_fw_upgrade(&pbt_buf[1],sizeof(CTPM_FW)-1);//因为在i文件中在第一个字节插入了定义的版本号，所以要减去1

   if (i_ret != 0)
   {
       //error handling ... 
       //TBD
   }

   return i_ret;
}

unsigned char fts_ctpm_get_upg_ver(void)
{
    unsigned int ui_sz;
    ui_sz = sizeof(CTPM_FW);
    if (ui_sz > 2)
    {
        return CTPM_FW[0];
        
    }
    else
    {
        //TBD, error handling?
        return 0xff; //default value
    }
}
#endif

static int ft5x_i2c_rxdata(char *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= this_client->addr,
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

static int ft5x_i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

   	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

static int ft5x_set_reg(u8 addr, u8 para)
{
    u8 buf[3];
    int ret = -1;

    buf[0] = addr;
    buf[1] = para;
    ret = ft5x_i2c_txdata(buf, 2);
    if (ret < 0) {
        pr_err("write reg failed! %#x ret: %d", buf[0], ret);
        return -1;
    }
    
    return 0;
}

static void ft5x_ts_release(void)
{
	struct ft5x_ts_data *data = i2c_get_clientdata(this_client);
#ifdef CONFIG_FT5X0X_MULTITOUCH	
#ifdef TOUCH_KEY_SUPPORT
  if(1 == key_tp)
  {
		input_report_key(data->input_dev, key_val, 0);
#ifdef PRINT_POINT_INFO
		printk("Release Key = %d\n",key_val);
#endif		
	}
	else
	  input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
#else
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
#endif
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
			if(1 == revert_x_flag){
                event->x5 = SCREEN_MAX_X - event->x5;
			}
			if(1 == revert_y_flag){
                event->y5 = SCREEN_MAX_Y - event->y5;
			}
			//printk("before swap: event->x5 = %d, event->y5 = %d. \n", event->x5, event->y5);
			if(1 == exchange_x_y_flag){
                                  swap(event->x5, event->y5);
			}
			//printk("after swap: event->x5 = %d, event->y5 = %d. \n", event->x5, event->y5);
			event->touch_ID5=(s16)(buf[0x1D] & 0xF0)>>4;
		case 4:
			event->x4 = (s16)(buf[0x15] & 0x0F)<<8 | (s16)buf[0x16];
			event->y4 = (s16)(buf[0x17] & 0x0F)<<8 | (s16)buf[0x18];
			if(1 == revert_x_flag){
                event->x4 = SCREEN_MAX_X - event->x4;
			}
			if(1 == revert_y_flag){
                event->y4 = SCREEN_MAX_Y - event->y4;
			}
			//printk("before swap: event->x4 = %d, event->y4 = %d. \n", event->x4, event->y4);
			if(1 == exchange_x_y_flag){
                                  swap(event->x4, event->y4);
			}
			//printk("after swap: event->x4 = %d, event->y4 = %d. \n", event->x4, event->y4);
			event->touch_ID4=(s16)(buf[0x17] & 0xF0)>>4;
		case 3:
			event->x3 = (s16)(buf[0x0f] & 0x0F)<<8 | (s16)buf[0x10];
			event->y3 = (s16)(buf[0x11] & 0x0F)<<8 | (s16)buf[0x12];
			if(1 == revert_x_flag){
                event->x3 = SCREEN_MAX_X - event->x3;
			}
			if(1 == revert_y_flag){
                event->y3 = SCREEN_MAX_Y - event->y3;
			}
			//printk("before swap: event->x3 = %d, event->y3 = %d. \n", event->x3, event->y3);
			if(1 == exchange_x_y_flag){
                                  swap(event->x3, event->y3);
			}
			//printk("after swap: event->x3 = %d, event->y3 = %d. \n", event->x3, event->y3);
			event->touch_ID3=(s16)(buf[0x11] & 0xF0)>>4;
		case 2:
			event->x2 = (s16)(buf[9] & 0x0F)<<8 | (s16)buf[10];
			event->y2 = (s16)(buf[11] & 0x0F)<<8 | (s16)buf[12];
			if(1 == revert_x_flag){
                event->x2 = SCREEN_MAX_X - event->x2;
			}
			if(1 == revert_y_flag){
                event->y2 = SCREEN_MAX_Y - event->y2;
			}
			//printk("before swap: event->x2 = %d, event->y2 = %d. \n", event->x2, event->y2);
			if(1 == exchange_x_y_flag){
                                  swap(event->x2, event->y2);
			}
			//printk("after swap: event->x2 = %d, event->y2 = %d. \n", event->x2, event->y2);
		    event->touch_ID2=(s16)(buf[0x0b] & 0xF0)>>4;
		case 1:
			event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
			event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
			if(event->x1 < 60000)
			{
				if(1 == revert_x_flag){
	                event->x1 = SCREEN_MAX_X - event->x1;
				}
				if(1 == revert_y_flag){
	                event->y1 = SCREEN_MAX_Y - event->y1;
				}
				//printk("before swap: event->x1 = %d, event->y1 = %d. \n", event->x1, event->y1);
				if(1 == exchange_x_y_flag){
	                                  swap(event->x1, event->y1);
				}
		 }

			//printk("after swap: event->x1 = %d, event->y1 = %d. \n", event->x1, event->y1);
			event->touch_ID1=(s16)(buf[0x05] & 0xF0)>>4;
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
#ifdef TOUCH_KEY_SUPPORT
  if((1==event->touch_point)&&(event->x1 > 60000))
  {
  	key_tp = 1;
  	if(event->y1 < 40)
  	{
  		key_val = 1;
	    input_report_key(data->input_dev, key_val, 1);
	    input_sync(data->input_dev);  
			#ifdef PRINT_POINT_INFO
			    printk("===KEY 1====\n");
			#endif	     
  	}else if(event->y1 < 90)
  	{
  		key_val = 2;
      input_report_key(data->input_dev, key_val, 1);
      input_sync(data->input_dev);     
			#ifdef PRINT_POINT_INFO
			    printk("===KEY 2 ====\n");
			#endif     		
  	}else
  	{
  		key_val = 3;
      input_report_key(data->input_dev, key_val, 1);
      input_sync(data->input_dev);     
			#ifdef PRINT_POINT_INFO
			    printk("===KEY 3====\n");
			#endif		
  	}
	   #ifdef TOUCH_KEY_LIGHT_SUPPORT
    if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_light_hdle, 1, "ctp_light")){
        printk("ft5x_ts_light: err when operate gpio. \n");
    }    
    msleep(15);
    if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_light_hdle, 0, "ctp_light")){
        printk("ft5x_ts_light: err when operate gpio. \n");
    }         
    #endif	
  	
  }
  else
  {
  	key_tp = 0;
	switch(event->touch_point) {
		case 5:
			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID5);	
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x5);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y5);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			#ifdef PRINT_POINT_INFO
			    printk("===x5 = %d,y5 = %d ====\n",event->x2,event->y2);
			#endif
		case 4:
			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID4);	
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x4);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y4);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			#ifdef PRINT_POINT_INFO
			    printk("===x4 = %d,y4 = %d ====\n",event->x2,event->y2);
			#endif
		case 3:
			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID3);	
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x3);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y3);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
            #ifdef PRINT_POINT_INFO
			    printk("===x3 = %d,y3 = %d ====\n",event->x2,event->y2);
			#endif
		case 2:
			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID2);	
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x2);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y2);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
			#ifdef PRINT_POINT_INFO
			    printk("===x2 = %d,y2 = %d ====\n",event->x2,event->y2);
			#endif
		case 1:
			/*if(event->x1 >= 1280)
			    event->x1 = 1279;*/
			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID1);	
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
	}
#else
		switch(event->touch_point) {
			case 5:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID5);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x5);
				input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y5);
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				#ifdef PRINT_POINT_INFO
				    printk("===x5 = %d,y5 = %d ====\n",event->x2,event->y2);
				#endif
			case 4:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID4);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x4);
				input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y4);
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				#ifdef PRINT_POINT_INFO
				    printk("===x4 = %d,y4 = %d ====\n",event->x2,event->y2);
				#endif
			case 3:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID3);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x3);
				input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y3);
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
	            #ifdef PRINT_POINT_INFO
				    printk("===x3 = %d,y3 = %d ====\n",event->x2,event->y2);
				#endif
			case 2:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID2);	
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
				input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x2);
				input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y2);
				input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
				input_mt_sync(data->input_dev);
				#ifdef PRINT_POINT_INFO
				    printk("===x2 = %d,y2 = %d ====\n",event->x2,event->y2);
				#endif
			case 1:
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID1);	
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
#endif
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
	   return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static void ft5x_ts_wakeup(void)
{

    if(1 == ctp_wakeup_enable){
        //wake up
        printk("ft5x_ts_wakeup. \n");
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup")){
            printk("ft5x_ts_resume: err when operate gpio. \n");
        }
        mdelay(15);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup")){
            printk("ft5x_ts_resume: err when operate gpio. \n");
        }
        mdelay(15);

    }
    
    return;
}

static void ft5x_ts_reset(void)
{
    if(1 == ctp_reset_enable){
        printk("ft5x_ts_reset. \n");
       if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 0, "ctp_reset")){
            printk("ft5x_ts_reset: err when operate gpio. \n");
        }
        mdelay(15);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 1, "ctp_reset")){
            printk("ft5x_ts_reset: err when operate gpio. \n");
            }
            mdelay(15);
    }
    return;
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
    //gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup");
    // ==set mode ==, 
    printk("ft5x_ts_suspend: write FT5X0X_REG_PMODE .\n");
    ft5x_set_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);       
}

static void ft5x_ts_resume(struct early_suspend *handler)
{
	printk("==ft5x_ts_resume== \n");
	// wake the mode
//	__gpio_as_output(GPIO_FT5X0X_WAKE);		
//	__gpio_clear_pin(GPIO_FT5X0X_WAKE);		//set wake = 0,base on system
//	 msleep(100);
//	__gpio_set_pin(GPIO_FT5X0X_WAKE);			//set wake = 1,base on system
//	msleep(100);
//	enable_irq(this_client->irq);
//	enable_irq(IRQ_EINT(6));

    //gpio i28 output high
	//printk("==ft5x_ts_resume=\n");

    //reset
    ft5x_ts_reset();
    //wakeup
    ft5x_ts_wakeup();
    
}
#endif  //CONFIG_HAS_EARLYSUSPEND

static int 
ft5x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft5x_ts_data *ft5x_ts;
	struct input_dev *input_dev;
	int err = 0;
	int reg_val;
#ifdef TOUCH_KEY_SUPPORT
	int i = 0;
#endif



	printk("=====capacitor touchscreen driver register ================\n");
	printk("===================ft5x_ts_probe v1.0========================\n");
	
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
	
	//printk("ft5x_ts_probe : client->addr = %d. \n", client->addr);
	this_client->addr = client->addr;
	//printk("ft5x_ts_probe : client->addr = %d. \n", client->addr);
	i2c_set_clientdata(client, ft5x_ts);
	
	    //config gpio:
    gpio_int_hdle = gpio_request_ex("ctp_para", "ctp_int_port");
    if(!gpio_int_hdle) {
        pr_warning("touch panel IRQ_EINT21_para request gpio fail!\n");
        goto exit_gpio_int_request_failed;
    }
    
    gpio_wakeup_hdle = gpio_request_ex("ctp_para", "ctp_wakeup");
    if(!gpio_wakeup_hdle) {
        ctp_wakeup_enable = 0; 
    }else{
        ctp_wakeup_enable = 1; 
    }
    printk("ctp_wakeup_enable = %d. \n", ctp_wakeup_enable);
 
    gpio_reset_hdle = gpio_request_ex("ctp_para", "ctp_reset");
    if(!gpio_reset_hdle) {
        ctp_reset_enable = 0;
    }else{
        ctp_reset_enable = 1;
    }
    printk("ctp_reset_enable = %d. \n", ctp_reset_enable);

    #ifdef TOUCH_KEY_LIGHT_SUPPORT
    gpio_light_hdle = gpio_request_ex("ctp_para", "ctp_light");
    #endif
        //reset
    ft5x_ts_reset();
    //wakeup
    ft5x_ts_wakeup();
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
#ifdef CONFIG_SUPPORT_FTS_CTP_UPG

	#if 0

	#define PROBE_BUFFER_LEN  (1)
	        unsigned char up_flg=0;
	        unsigned char buf[PROBE_BUFFER_LEN] = {0};

	        up_flg = fts_ctpm_get_upg_ver();
	        if(FTS_FALSE == fts_register_read(0xa6, buf, PROBE_BUFFER_LEN)){
	            printk("ft5x_ts_probe: fts_register_read failed. \n");
	            goto exit_upgrade_failed;
	        }
	        
	        printk("up_flg == %hu, buf[0] == %hu \n", up_flg, buf[0]);
	        if(FTS_FALSE == fts_register_read(0xa8, buf, PROBE_BUFFER_LEN)){
	            printk("ft5x_ts_probe: fts_register_read failed. \n");
	            goto exit_upgrade_failed;
	        }
	        printk("buf[0] == %hu \n", buf[0]);

	#endif

	  	fts_ctpm_fw_upgrade_with_i_file();

#endif


//	printk("==INIT_WORK=\n");
	INIT_WORK(&ft5x_ts->pen_event_work, ft5x_ts_pen_irq_work);

	ft5x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ft5x_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}

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
	input_set_abs_params(input_dev,
			     ABS_MT_TRACKING_ID, 0, 4, 0, 0);
#ifdef TOUCH_KEY_SUPPORT
	key_tp = 0;
	input_dev->evbit[0] = BIT_MASK(EV_KEY);
	for (i = 1; i < 4; i++)
		set_bit(i, input_dev->keybit);
#endif
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

#ifdef CONFIG_FT5X0X_MULTITOUCH
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
    
    ft5x_set_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);	

   

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
    gpio_release(gpio_int_hdle, 2);
    gpio_release(gpio_wakeup_hdle, 2);
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
    int ret = -1;
    int ctp_used = -1;
    char name[I2C_NAME_SIZE];
    script_parser_value_type_t type = SCIRPT_PARSER_VALUE_TYPE_STRING;

    pr_notice("=========ft5x-ts-init============\n");	

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_used", &ctp_used, 1)){
        pr_err("ft5x_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    if(1 != ctp_used){
        pr_err("ft5x_ts: ctp_unused. \n");
        return 0;
    }

    if(SCRIPT_PARSER_OK != script_parser_fetch_ex("ctp_para", "ctp_name", (int *)(&name), &type, sizeof(name)/sizeof(int))){
            pr_err("ft5x_ts_init: script_parser_fetch err. \n");
            goto script_parser_fetch_err;
    }
    if(strcmp(FT5X_NAME, name)){
        pr_err("ft5x_ts_init: name %s does not match FT5X_NAME. \n", name);
        return 0;
    }

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_x", &screen_max_x, 1)){
        pr_err("ft5x_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    pr_info("ft5x_ts: screen_max_x = %d. \n", screen_max_x);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_y", &screen_max_y, 1)){
        pr_err("ft5x_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    pr_info("ft5x_ts: screen_max_y = %d. \n", screen_max_y);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_x_flag", &revert_x_flag, 1)){
        pr_err("ft5x_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    pr_info("ft5x_ts: revert_x_flag = %d. \n", revert_x_flag);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_y_flag", &revert_y_flag, 1)){
        pr_err("ft5x_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    pr_info("ft5x_ts: revert_y_flag = %d. \n", revert_y_flag);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_exchange_x_y_flag", &exchange_x_y_flag, 1)){
        pr_err("ft5x_ts: script_parser_fetch err. \n");
        goto script_parser_fetch_err;
    }
    pr_info("ft5x_ts: exchange_x_y_flag = %d. \n", exchange_x_y_flag);
    
    ret = i2c_add_driver(&ft5x_ts_driver);
	
script_parser_fetch_err:
	return ret;
}

static void __exit ft5x_ts_exit(void)
{
	printk("==ft5x_ts_exit==\n");
	i2c_del_driver(&ft5x_ts_driver);
}

late_initcall(ft5x_ts_init);
module_exit(ft5x_ts_exit);

MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x TouchScreen driver");
MODULE_LICENSE("GPL");

