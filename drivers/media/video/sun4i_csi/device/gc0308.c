/*
 * A V4L2 driver for GalaxyCore GC0308 cameras.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-i2c-drv.h>

#include <mach/gpio_v2.h>
#include "../include/sun4i_csi_core.h"
#include "../include/sun4i_dev_csi.h"

MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for GalaxyCore GC0308 sensors");
MODULE_LICENSE("GPL");


#define MCLK (24*1000*1000)
#define VREF_POL	CSI_HIGH
#define HREF_POL	CSI_HIGH
#define CLK_POL		CSI_RISING
#define IO_CFG		0						//0 for csi0

#define V4L2_IDENT_SENSOR 0x0308

#define REG_TERM 0xff
#define VAL_TERM 0xff


#define REG_ADDR_STEP 1
#define REG_DATA_STEP 1
#define REG_STEP 			(REG_ADDR_STEP+REG_DATA_STEP)


/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define VGA_WIDTH		640
#define VGA_HEIGHT	480
#define QVGA_WIDTH	320
#define QVGA_HEIGHT	240
#define CIF_WIDTH		352
#define CIF_HEIGHT	288
#define QCIF_WIDTH	176
#define	QCIF_HEIGHT	144

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 25

/*
 * The gc0308 sits on i2c with ID 0x42
 */
#define I2C_ADDR 0x42>>1

/* Registers */


/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */
__csi_subdev_info_t ccm_info_con = 
{
	.mclk 	= MCLK,
	.vref 	= VREF_POL,
	.href 	= HREF_POL,
	.clock	= CLK_POL,
	.iocfg	= IO_CFG,
};

struct sensor_info {
	struct v4l2_subdev sd;
	struct sensor_format_struct *fmt;  /* Current format */
	__csi_subdev_info_t *ccm_info;
	u8 clkrc;			/* Clock divider value */
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}



/*
 * The default register settings
 *
 */

struct regval_list {
	unsigned char reg_num[REG_ADDR_STEP];
	unsigned char value[REG_DATA_STEP];
};



static struct regval_list sensor_default_regs[] = {
{{0xfe},{0x00}},																					 
{{0x0f},{0x00}},	                                         
{{0x01},{0x6a}},                                           
{{0x02},{0x70}},                                           
{{0xe2},{0x00}},                                           
{{0xe3},{0x96}},                                           
{{0xe4},{0x02}},                                           
{{0xe5},{0x58}},                                           
{{0xe6},{0x02}},                                           
{{0xe7},{0x58}},                                           
{{0xe8},{0x02}},                                           
{{0xe9},{0x58}},                                           
{{0xea},{0x0e}},                                           
{{0xeb},{0xa6}},                                           
{{0xec},{0x20}},                                           
{{0x05},{0x00}},                                           
{{0x06},{0x00}},                                           
{{0x07},{0x00}},                                           
{{0x08},{0x00}},                                           
{{0x09},{0x01}},                                           
{{0x0a},{0xe8}},                                           
{{0x0b},{0x02}},                                           
{{0x0c},{0x88}},                                           
{{0x0d},{0x02}},                                           
{{0x0e},{0x02}},                                           
{{0x10},{0x26}},                                           
{{0x11},{0x0d}},                                           
{{0x12},{0x2a}},                                           
{{0x13},{0x00}},                                           
{{0x14},{0x10}},                                           
{{0x15},{0x0a}},                                           
{{0x16},{0x05}},                                           
{{0x17},{0x01}},                                           
{{0x18},{0x44}},                                           
{{0x19},{0x44}},                                           
{{0x1a},{0x2a}},                                           
{{0x1b},{0x00}},                                           
{{0x1c},{0x49}},                                           
{{0x1d},{0x9a}},                                           
{{0x1e},{0x61}},                                           
{{0x1f},{0x16}},                                           
{{0x20},{0xff}},                                           
{{0x21},{0xfa}},                                           
{{0x22},{0x57}},                                           
{{0x24},{0xa2}},	//YCbYCr                                 
{{0x25},{0x0f}},                                           
{{0x26},{0x03}}, // 0x01                                   
{{0x2f},{0x01}},                                           
{{0x30},{0xf7}},                                           
{{0x31},{0x50}},                                           
{{0x32},{0x00}},                                           
{{0x39},{0x04}},                                           
{{0x3a},{0x20}},                                           
{{0x3b},{0x20}},                                           
{{0x3c},{0x00}},                                           
{{0x3d},{0x00}},                                           
{{0x3e},{0x00}},                                           
{{0x3f},{0x00}},                                           
{{0x50},{0x16}}, // 0x14                                   
{{0x53},{0x80}},                                           
{{0x54},{0x87}},                                           
{{0x55},{0x87}},                                           
{{0x56},{0x80}},                                           
{{0x8b},{0x10}},                                           
{{0x8c},{0x10}},                                           
{{0x8d},{0x10}},                                           
{{0x8e},{0x10}},                                           
{{0x8f},{0x10}},                                           
{{0x90},{0x10}},                                           
{{0x91},{0x3c}},                                           
{{0x92},{0x50}},                                           
{{0x5d},{0x12}},                                           
{{0x5e},{0x1a}},                                           
{{0x5f},{0x24}},                                           
{{0x60},{0x07}},                                           
{{0x61},{0x15}},                                           
{{0x62},{0x0f}}, // 0x08                                   
{{0x64},{0x01}},  // 0x03                                  
{{0x66},{0xe8}},                                           
{{0x67},{0x86}},                                           
{{0x68},{0xa2}},                                           
{{0x69},{0x18}},                                           
{{0x6a},{0x0f}},                                           
{{0x6b},{0x00}},                                           
{{0x6c},{0x5f}},                                           
{{0x6d},{0x8f}},                                           
{{0x6e},{0x55}},                                           
{{0x6f},{0x38}},                                           
{{0x70},{0x15}},                                           
{{0x71},{0x33}},                                           
{{0x72},{0xdc}},                                           
{{0x73},{0x80}},                                           
{{0x74},{0x02}},                                           
{{0x75},{0x3f}},                                           
{{0x76},{0x02}},                                           
{{0x77},{0x57}}, // 0x47                                   
{{0x78},{0x88}},                                           
{{0x79},{0x81}},                                           
{{0x7a},{0x81}},                                           
{{0x7b},{0x22}},                                           
{{0x7c},{0xff}},                                           
{{0x93},{0x46}},                                           
{{0x94},{0x00}},                                           
{{0x95},{0x03}},                                           
{{0x96},{0xd0}},                                           
{{0x97},{0x40}},                                           
{{0x98},{0xf0}},                                           
{{0xb1},{0x3c}},                                           
{{0xb2},{0x3c}},                                           
{{0xb3},{0x44}}, //0x40                                    
{{0xb6},{0xe0}},                                           
{{0xbd},{0x3C}},                                           
{{0xbe},{0x36}},                                           
{{0xd0},{0xC9}},                                           
{{0xd1},{0x10}},                                           
{{0xd2},{0x90}},                                           
{{0xd3},{0x88}},                                           
{{0xd5},{0xF2}},                                           
{{0xd6},{0x10}},                                           
{{0xdb},{0x92}},                                           
{{0xdc},{0xA5}},                                           
{{0xdf},{0x23}},                                           
{{0xd9},{0x00}},                                           
{{0xda},{0x00}},                                           
{{0xe0},{0x09}},                                           
{{0xed},{0x04}},                                           
{{0xee},{0xa0}},                                           
{{0xef},{0x40}},                                           
{{0x80},{0x03}},                                           
                                                          
{{0x9F},{0x14}},                                           
{{0xA0},{0x28}},                                           
{{0xA1},{0x44}},                                           
{{0xA2},{0x5d}},                                           
{{0xA3},{0x72}},                                           
{{0xA4},{0x86}},                                           
{{0xA5},{0x95}},                                           
{{0xA6},{0xb1}},                                           
{{0xA7},{0xc6}},                                           
{{0xA8},{0xd5}},                                           
{{0xA9},{0xe1}},                                           
{{0xAA},{0xea}},                                           
{{0xAB},{0xf1}},                                           
{{0xAC},{0xf5}},                                           
{{0xAD},{0xFb}},                                           
{{0xAE},{0xFe}},                                           
{{0xAF},{0xFF}},                                                     
                                                           
{{0xc0},{0x00}},                                           
{{0xc1},{0x14}},                                           
{{0xc2},{0x21}},                                           
{{0xc3},{0x36}},                                           
{{0xc4},{0x49}},                                           
{{0xc5},{0x5B}},                                           
{{0xc6},{0x6B}},                                           
{{0xc7},{0x7B}},                                           
{{0xc8},{0x98}},                                           
{{0xc9},{0xB4}},                                           
{{0xca},{0xCE}},                                           
{{0xcb},{0xE8}},                                           
{{0xcc},{0xFF}},                                           
{{0xf0},{0x02}},                                           
{{0xf1},{0x01}},                                           
{{0xf2},{0x04}},                                           
{{0xf3},{0x30}},                                           
{{0xf9},{0x9f}},                                           
{{0xfa},{0x78}},                                           
{{0xfe},{0x01}},                                           
{{0x00},{0xf5}},                                           
{{0x02},{0x20}},                                           
{{0x04},{0x10}},                                           
{{0x05},{0x10}},                                           
{{0x06},{0x20}},                                           
{{0x08},{0x15}},                                           
{{0x0a},{0xa0}},                                           
{{0x0b},{0x64}},                                           
{{0x0c},{0x08}},                                           
{{0x0e},{0x4C}},                                           
{{0x0f},{0x39}},                                           
{{0x10},{0x41}},                                           
{{0x11},{0x37}},                                           
{{0x12},{0x24}},                                           
{{0x13},{0x39}},                                           
{{0x14},{0x45}},                                           
{{0x15},{0x45}},                                           
{{0x16},{0xc2}},                                           
{{0x17},{0xA8}},                                           
{{0x18},{0x18}},                                           
{{0x19},{0x55}},                                           
{{0x1a},{0xd8}},                                           
{{0x1b},{0xf5}},                                           
{{0x70},{0x40}},                                           
{{0x71},{0x58}},                                           
{{0x72},{0x30}},                                           
{{0x73},{0x48}},                                           
{{0x74},{0x20}},                                           
{{0x75},{0x60}},                                           
{{0x77},{0x20}},                                           
{{0x78},{0x32}},                                           
{{0x30},{0x03}},                                           
{{0x31},{0x40}},                                           
{{0x32},{0x10}},                                           
{{0x33},{0xe0}},                                           
{{0x34},{0xe0}},                                           
{{0x35},{0x00}},                                           
{{0x36},{0x80}},                                           
{{0x37},{0x00}},                                           
{{0x38},{0x04}},                                           
{{0x39},{0x09}},                                           
{{0x3a},{0x12}},                                           
{{0x3b},{0x1C}},                                           
{{0x3c},{0x28}},                                           
{{0x3d},{0x31}},                                           
{{0x3e},{0x44}},                                           
{{0x3f},{0x57}},                                           
{{0x40},{0x6C}},                                           
{{0x41},{0x81}},                                           
{{0x42},{0x94}},                                           
{{0x43},{0xA7}},                                           
{{0x44},{0xB8}},                                           
{{0x45},{0xD6}},                                           
{{0x46},{0xEE}},                                           
{{0x47},{0x0d}},                                           
{{0xfe},{0x00}},                                          
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 * 
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	
	{{0x24},{0xa2}}	//YCbYCr
};


static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	
	{{0x24},{0xa3}}	//YCrYCb
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	
	{{0x24},{0xa1}}	//CrYCbY
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	
	{{0x24},{0xa0}}	//CbYCrY
};

static struct regval_list sensor_fmt_raw[] = {
	
	{{0x24},{0xb7}}//raw
};



/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
//static int sensor_read(struct v4l2_subdev *sd, unsigned char *reg,
//		unsigned char *value)
//{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	u8 data[REG_STEP];
//	struct i2c_msg msg;
//	int ret,i;
//	
//	for(i = 0; i < REG_ADDR_STEP; i++)
//		data[i] = reg[i];
//	
//	data[REG_ADDR_STEP] = 0xff;
//	/*
//	 * Send out the register address...
//	 */
//	msg.addr = client->addr;
//	msg.flags = 0;
//	msg.len = REG_ADDR_STEP;
//	msg.buf = data;
//	ret = i2c_transfer(client->adapter, &msg, 1);
//	if (ret < 0) {
//		printk(KERN_ERR "Error %d on register write\n", ret);
//		return ret;
//	}
//	/*
//	 * ...then read back the result.
//	 */
//	
//	msg.flags = I2C_M_RD;
//	msg.len = REG_DATA_STEP;
//	msg.buf = &data[REG_ADDR_STEP];
//	
//	ret = i2c_transfer(client->adapter, &msg, 1);
//	if (ret >= 0) {
//		for(i = 0; i < REG_DATA_STEP; i++)
//			value[i] = data[i+REG_ADDR_STEP];
//		ret = 0;
//	}
//
//	return ret;
//}


static int sensor_write(struct v4l2_subdev *sd, unsigned char *reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[REG_STEP];
	int ret,i;
	
	for(i = 0; i < REG_ADDR_STEP; i++)
			data[i] = reg[i];
	for(i = REG_ADDR_STEP; i < REG_STEP; i++)
			data[i] = value[i-REG_ADDR_STEP];
	
//	for(i = 0; i < REG_STEP; i++)
//		printk("data[%x]=%x\n",i,data[i]);
			
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = REG_STEP;
	msg.buf = data;

	
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;
	return ret;
}



/*
 * Write a list of register settings;
 */
static int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *vals , uint size)
{
	int i,ret;
//	unsigned char rd;
	
	for(i = 0; i < size ; i++)
	{
		ret = sensor_write(sd, vals->reg_num, vals->value);
		if (ret < 0)
			{
				csi_err("sensor_write_err!\n");
				return ret;
			}
			
		
//		ret = sensor_read(sd, vals->reg_num, &rd);
//		
//		if (ret < 0)
//			{
//				printk("sensor_read_err!\n");
//				return ret;
//			}
//		if(rd != *vals->value)
//			printk("read_val = %x\n",rd);
		
		vals++;
	}

	return 0;
}


/*
 * Stuff that knows about the sensor.
 */
 
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct sensor_info *info = to_state(sd);
	char csi_pwr_en[20];
	
//	printk("sensor_power,ccm_inf->iocfg=%d\n",info->ccm_info->iocfg);
	
	if(info->ccm_info->iocfg == 0)
		strcpy(csi_pwr_en,"CSI0_POWER_EN");
	else if(info->ccm_info->iocfg == 1)
		strcpy(csi_pwr_en,"CSI1_POWER_EN");
	
	switch(on)
	{
		case 0:
			gpio_write_one_pin_value(dev->csi_pin_hd,0,csi_pwr_en);
			break;
		case 1:
			gpio_write_one_pin_value(dev->csi_pin_hd,1,csi_pwr_en);
			break;
		default:
			return -EINVAL;
			
	}
	return 0;
}
 
static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct sensor_info *info = to_state(sd);
	char csi_reset[20];
	
//	printk("sensor_reset,ccm_inf->iocfg=%d\n",info->ccm_info->iocfg);
	
	if(info->ccm_info->iocfg == 0)
		strcpy(csi_reset,"CSI0_RESET");
	else if(info->ccm_info->iocfg == 1)
		strcpy(csi_reset,"CSI1_RESET");
	
	//0:reset release; 1:reset enable 2:reset pulse 
	switch(val)
	{
		case 0:
			gpio_write_one_pin_value(dev->csi_pin_hd,1,csi_reset);
			break;
		case 1:
			gpio_write_one_pin_value(dev->csi_pin_hd,0,csi_reset);
			break;
		case 2:
			gpio_write_one_pin_value(dev->csi_pin_hd,1,csi_reset);
			msleep(10);
			gpio_write_one_pin_value(dev->csi_pin_hd,0,csi_reset);
			msleep(100);
			gpio_write_one_pin_value(dev->csi_pin_hd,1,csi_reset);
			break;
		default:
			return -EINVAL;
	}

	return 0;
}



static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	
	ret = sensor_power(sd,1);
	if(ret < 0)
		return ret;
		
	msleep(10);
	
	ret = sensor_reset(sd,2);
	if(ret < 0)
		return ret;
	
	return sensor_write_array(sd, sensor_default_regs , ARRAY_SIZE(sensor_default_regs));
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret=0;
	
	switch(cmd){
		case CSI_SUBDEV_CMD_GET_INFO: 
		{
			struct sensor_info *info = to_state(sd);
			__csi_subdev_info_t *ccm_info = arg;
			
//			printk("CSI_SUBDEV_CMD_GET_INFO\n");
			
			ccm_info->mclk 	=	info->ccm_info->mclk ;
			ccm_info->vref 	=	info->ccm_info->vref ;
			ccm_info->href 	=	info->ccm_info->href ;
			ccm_info->clock	=	info->ccm_info->clock;
			ccm_info->iocfg	=	info->ccm_info->iocfg;
			
//			printk("ccm_info.mclk=%x\n ",info->ccm_info->mclk);
//			printk("ccm_info.vref=%x\n ",info->ccm_info->vref);
//			printk("ccm_info.href=%x\n ",info->ccm_info->href);
//			printk("ccm_info.clock=%x\n ",info->ccm_info->clock);
//			printk("ccm_info.iocfg=%x\n ",info->ccm_info->iocfg);
			break;
		}
		case CSI_SUBDEV_CMD_SET_INFO:
		{
			struct sensor_info *info = to_state(sd);
			__csi_subdev_info_t *ccm_info = arg;
			
//			printk("CSI_SUBDEV_CMD_SET_INFO\n");
			
			info->ccm_info->mclk 	=	ccm_info->mclk 	;
			info->ccm_info->vref 	=	ccm_info->vref 	;
			info->ccm_info->href 	=	ccm_info->href 	;
			info->ccm_info->clock	=	ccm_info->clock	;
			info->ccm_info->iocfg	=	ccm_info->iocfg	;
			
//			printk("ccm_info.mclk=%x\n ",info->ccm_info->mclk);
//			printk("ccm_info.vref=%x\n ",info->ccm_info->vref);
//			printk("ccm_info.href=%x\n ",info->ccm_info->href);
//			printk("ccm_info.clock=%x\n ",info->ccm_info->clock);
//			printk("ccm_info.iocfg=%x\n ",info->ccm_info->iocfg);
			
			break;
		}
		default:
			break;
	}		
		return ret;
}


/*
 * Store information about the video data format. 
 */
static struct sensor_format_struct {
	__u8 *desc;
	__u32 pixelformat;
	struct regval_list *regs;
	int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.regs 		= sensor_fmt_yuv422_yuyv,
		.bpp		= 2,
	},
	{
		.desc		= "YVYU 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YVYU,
		.regs 		= sensor_fmt_yuv422_yvyu,
		.bpp		= 2,
	},
	{
		.desc		= "UYVY 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
		.regs 		= sensor_fmt_yuv422_uyvy,
		.bpp		= 2,
	},
	{
		.desc		= "VYUY 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_VYUY,
		.regs 		= sensor_fmt_yuv422_vyuy,
		.bpp		= 2,
	},
	{
		.desc		= "Raw RGB Bayer",
		.pixelformat	= V4L2_PIX_FMT_SBGGR8,
		.regs 		= sensor_fmt_raw,
		.bpp		= 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)


/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */


static struct sensor_win_size {
	int	width;
	int	height;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} sensor_win_sizes[] = {
	/* VGA */
	{
		.width		= VGA_WIDTH,
		.height		= VGA_HEIGHT,
		.hstart		= 158,		
		.hstop		=  14,		
		.vstart		=  10,
		.vstop		= 490,
		.regs 		= NULL,
	}
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))


static int sensor_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt)
{
	struct sensor_format_struct *ofmt;

	if (fmt->index >= N_FMTS)
		return -EINVAL;

	ofmt = sensor_formats + fmt->index;
	fmt->flags = 0;
	strcpy(fmt->description, ofmt->desc);
	fmt->pixelformat = ofmt->pixelformat;
	return 0;
}


static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_format *fmt,
		struct sensor_format_struct **ret_fmt,
		struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].pixelformat == pix->pixelformat)
			break;
	
	if (index >= N_FMTS) {
		/* default to first format */
		index = 0;
		pix->pixelformat = sensor_formats[0].pixelformat;
	}
	
	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;
		
	/*
	 * Fields: the sensor devices claim to be progressive.
	 */
	pix->field = V4L2_FIELD_NONE;
	
	
	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (pix->width >= wsize->width && pix->height >= wsize->height)
			break;
	
	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	pix->width = wsize->width;
	pix->height = wsize->height;
	pix->bytesperline = pix->width*sensor_formats[index].bpp;
	pix->sizeimage = pix->height*pix->bytesperline;
	
	return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);
	
	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;
	
		
	sensor_write_array(sd, sensor_fmt->regs , sizeof((sensor_fmt->regs)[0]) / REG_STEP);
	
	ret = 0;
	if (wsize->regs)
		{
			ret = sensor_write_array(sd, wsize->regs , sizeof((wsize->regs)[0]) / REG_STEP);
		}
	
	info->fmt = sensor_fmt;
	
	return ret;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	//struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = SENSOR_FRAME_RATE;
	
	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
//	struct v4l2_captureparm *cp = &parms->parm.capture;
	//struct v4l2_fract *tpf = &cp->timeperframe;
	//struct sensor_info *info = to_state(sd);
	//int div;

//	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
//		return -EINVAL;
//	if (cp->extendedmode != 0)
//		return -EINVAL;

//	if (tpf->numerator == 0 || tpf->denominator == 0)
//		div = 1;  /* Reset to full rate */
//	else
//		div = (tpf->numerator*SENSOR_FRAME_RATE)/tpf->denominator;
//		
//	if (div == 0)
//		div = 1;
//	else if (div > CLK_SCALE)
//		div = CLK_SCALE;
//	info->clkrc = (info->clkrc & 0x80) | div;
//	tpf->numerator = 1;
//	tpf->denominator = sensor_FRAME_RATE/div;
//sensor_write(sd, REG_CLKRC, info->clkrc);
	return -EINVAL;
}



/*
 * Code for dealing with controls.
 */


static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}


static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}


static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}


static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}


static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}


static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}


static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}


static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}


static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}


static int sensor_s_gain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}



static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}


static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}


static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_exp(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}


static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}


static int sensor_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{
	return -EINVAL;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd,
		__s32 *value)
{
	return -EINVAL;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd,
		enum v4l2_colorfx value)
{
//	switch (value) {
//	case V4L2_COLORFX_NONE:
//	  return sensor_s_colorfx_none();
//	case V4L2_COLORFX_BW:
//		return sensor_s_colorfx_bw();   
//	case V4L2_COLORFX_SEPIA:
//		return sensor_s_colorfx_sepia();    
//	case V4L2_COLORFX_NEGATIVE:
//		return sensor_s_colorfx_negative();
//	case V4L2_COLORFX_EMBOSS:   
//		return sensor_s_colorfx_emboss();  
//	case V4L2_COLORFX_SKETCH:     
//		return sensor_s_colorfx_sketch();
//	case V4L2_COLORFX_SKY_BLUE:
//		return sensor_s_colorfx_sky_blue();
//	case V4L2_COLORFX_GRASS_GREEN:
//		return sensor_s_colorfx_grass_green();
//	case V4L2_COLORFX_SKIN_WHITEN:
//		return sensor_s_colorfx_skin_whiten();
//	case V4L2_COLORFX_VIVID:
//		return sensor_s_colorfx_vivid();
//	}
	return -EINVAL;
}


static int sensor_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	switch (qc->id) {
	case V4L2_CID_BRIGHTNESS:
		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
	case V4L2_CID_CONTRAST:
		return v4l2_ctrl_query_fill(qc, 0, 127, 1, 64);
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
	case V4L2_CID_AUTOGAIN:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 65535, 1, 500);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_COLORFX:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 0);
	}
	return -EINVAL;
}


static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_g_contrast(sd, &ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_g_hflip(sd, &ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_g_autoexp(sd, &ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_g_colorfx(sd,	&ctrl->value);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_s_brightness(sd, ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_s_contrast(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_s_autoexp(sd,
				(enum v4l2_exposure_auto_type) ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_colorfx(sd,
				(enum v4l2_colorfx) ctrl->value);
	}
	return -EINVAL;
}

static int sensor_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}


/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_chip_ident = sensor_g_chip_ident,
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.enum_fmt = sensor_enum_fmt,
	.try_fmt = sensor_try_fmt,
	.s_fmt = sensor_s_fmt,
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
};

/* ----------------------------------------------------------------------- */

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
//	int ret;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &sensor_ops);

	info->fmt = &sensor_formats[0];
	info->ccm_info = &ccm_info_con;
	
	
	
//	info->clkrc = 1;	/* 30fps */

	return 0;
}


static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{ "gc0308", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = "gc0308",
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
