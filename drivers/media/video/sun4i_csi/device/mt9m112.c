/*
 * A V4L2 driver for Micron mt9m112 cameras.
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
MODULE_DESCRIPTION("A low-level driver for Micron mt9m112 sensors");
MODULE_LICENSE("GPL");


#define MCLK (24*1000*1000)
#define VREF_POL	CSI_HIGH
#define HREF_POL	CSI_HIGH
#define CLK_POL		CSI_RISING
//#define IO_CFG		0						//0 for csi0

//define the voltage level of control signal
#define CSI_STBY_ON			1
#define CSI_STBY_OFF 		0
#define CSI_RST_ON			0
#define CSI_RST_OFF			1
#define CSI_PWR_ON			1
#define CSI_PWR_OFF			0


#define V4L2_IDENT_SENSOR 0x1320

#define REG_TERM 0xff
#define VAL_TERM 0xff


#define REG_ADDR_STEP 1
#define REG_DATA_STEP 2
#define REG_STEP 			(REG_ADDR_STEP+REG_DATA_STEP)


/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define SXGA_WIDTH	1280
#define SXGA_HEIGHT	1024
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
#define SENSOR_FRAME_RATE 20

/*
 * The Micron mt9m112 sits on i2c with ID 0xBA
 */
#define I2C_ADDR 0xBA

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
//	.iocfg	= IO_CFG,
};

struct sensor_info {
	struct v4l2_subdev sd;
	struct sensor_format_struct *fmt;  /* Current format */
	__csi_subdev_info_t *ccm_info;
	int	width;
	int	height;
	int brightness;
	int	contrast;
	int saturation;
	int hue;
	int hflip;
	int vflip;
	int gain;
	int autogain;
	int exp;
	enum v4l2_exposure_auto_type autoexp;
	int autowb;
	enum v4l2_whiteblance wb;
	enum v4l2_colorfx clrfx;
	u8 clkrc;			/* Clock divider value */
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}


struct regval_list {
	unsigned char reg_num[REG_ADDR_STEP];
	unsigned char value[REG_DATA_STEP];
};


/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {
	{{0xf0},{0x00,0x00}},   //Page 0
	{{0x0d},{0x00,0x09}},   
	{{0x0d},{0x00,0x29}},   
	{{0x0d},{0x00,0x08}},   //reset sensor core
	{{0xf0},{0x00,0x01}},   //Page 1
	{{0x9d},{0x3c,0xa0}},  
	{{0xf0},{0x00,0x02}},   //Page 2                  
	{{0x24},{0x5f,0x20}},   //MATRIX_ADJ_LIMITS       
	{{0x28},{0xef,0x02}},   //AWB_ADVANCED_CONTROL_REG
	{{0x5f},{0x41,0x43}},   //RATIO_DELTA_REG         
	{{0x02},{0x00,0xee}},   //BASE_MATRIX_SIGNS       
	{{0x03},{0x29,0x1a}},   //BASE_MATRIX_SCALE_K1_K5 
	{{0x04},{0x02,0xa4}},   //BASE_MATRIX_SCALE_K6_K9 
	{{0x09},{0x00,0x68}},   //BASE_MATRIX_COEF_K1     
	{{0x0a},{0x00,0x2a}},   //BASE_MATRIX_COEF_K2     
	{{0x0b},{0x00,0x04}},   //BASE_MATRIX_COEF_K3     
	{{0x0c},{0x00,0x93}},   //BASE_MATRIX_COEF_K4     
	{{0x0d},{0x00,0x82}},   //BASE_MATRIX_COEF_K5     
	{{0x0e},{0x00,0x40}},   //BASE_MATRIX_COEF_K6     
	{{0x0f},{0x00,0x5f}},   //BASE_MATRIX_COEF_K7     
	{{0x10},{0x00,0x4e}},   //BASE_MATRIX_COEF_K8     
	{{0x11},{0x00,0x5b}},   //BASE_MATRIX_COEF_K9     
	{{0x15},{0x00,0xc9}},   //DELTA_COEFS_SIGNS       
	{{0x16},{0x00,0x5e}},   //DELTA_MATRIX_COEF_D1    
	{{0x17},{0x00,0x9d}},   //DELTA_MATRIX_COEF_D2    
	{{0x18},{0x00,0x06}},   //DELTA_MATRIX_COEF_D3    
	{{0x19},{0x00,0x89}},   //DELTA_MATRIX_COEF_D4    
	{{0x1a},{0x00,0x12}},   //DELTA_MATRIX_COEF_D5    
	{{0x1b},{0x00,0xa1}},   //DELTA_MATRIX_COEF_D6    
	{{0x1c},{0x00,0xe4}},   //DELTA_MATRIX_COEF_D7    
	{{0x1d},{0x00,0x7a}},   //DELTA_MATRIX_COEF_D8    
	{{0x1e},{0x00,0x64}},   //DELTA_MATRIX_COEF_D9    
	{{0xf6},{0x00,0x5f}},   // FLASH_WB_POS           
	{{0xf0},{0x00,0x01}},   //Page 1
	{{0x81},{0x00,0x00}},   //lens
	{{0x82},{0x00,0x00}},   
	{{0x83},{0x00,0x00}},   
	{{0x84},{0x00,0x00}},   
	{{0x85},{0x00,0x00}},   
	{{0x86},{0x00,0x00}},   
	{{0x87},{0x00,0x00}},   
	{{0x88},{0x00,0x00}},   
	{{0x89},{0x00,0x00}},   
	{{0x8a},{0x00,0x00}},   
	{{0x8b},{0x00,0x00}},   
	{{0x8c},{0x00,0x00}},   
	{{0x8d},{0x00,0x00}},   
	{{0x8e},{0x00,0x00}},   
	{{0x8f},{0x00,0x00}},   
	{{0x90},{0x00,0x00}},   
	{{0x91},{0x00,0x00}},   
	{{0x92},{0x00,0x00}},   
	{{0x93},{0x00,0x00}},   
	{{0x94},{0x00,0x00}},   
	{{0x95},{0x00,0x00}},   
	{{0xb6},{0x00,0x00}},   
	{{0xb7},{0x00,0x00}},   
	{{0xb8},{0x00,0x00}},   
	{{0xb9},{0x00,0x00}},   
	{{0xba},{0x00,0x00}},   
	{{0xbb},{0x00,0x00}},   
	{{0xbc},{0x00,0x00}},   
	{{0xbd},{0x00,0x00}},   
	{{0xbe},{0x00,0x00}},   
	{{0xbf},{0x00,0x00}},   
	{{0xc0},{0x00,0x00}},   
	{{0xc1},{0x00,0x00}},   
	{{0xc2},{0x00,0x00}},   
	{{0xc3},{0x00,0x00}},   
	{{0xc4},{0x00,0x00}}, 
	{{0xf0},{0x00,0x01}},   //Page 1
	{{0x53},{0x1c,0x12}},   //Gamma
	{{0x54},{0x40,0x2a}},   
	{{0x55},{0x7c,0x62}},   
	{{0x56},{0xa9,0x94}},   
	{{0x57},{0xcf,0xbc}},   
	{{0x58},{0xe0,0x00}},   
	{{0xdc},{0x1c,0x12}},   
	{{0xdd},{0x40,0x2a}},   
	{{0xde},{0x7c,0x62}},   
	{{0xdf},{0xa9,0x94}},   
	{{0xe0},{0xcf,0xbc}},   
	{{0xe1},{0xe0,0x00}},   
	{{0x34},{0x00,0x10}},   
	{{0x35},{0xf0,0x10}},
	{{0xf0},{0x00,0x00}},   //Page 0  //27Mhz 640*512 preview //Snapshot 1280*1024...                              
	{{0x05},{0x00,0xf1}},   // Context B (full-res) Horizontal Blank
	{{0x06},{0x00,0x0d}},   // Context B (full-res) Vertical Blank  
	{{0x07},{0x00,0xd9}},   // Context A (preview) Horizontal Blank 
	{{0x08},{0x00,0x0d}},   // Context A (preview) Vertical Blank   
	{{0x20},{0x01,0x00}},   // Read Mode Context B                                                
	{{0x21},{0x04,0x00}},   // Read Mode Context A                  
	{{0x22},{0x0d,0x2b}},   // Dark col / rows                      
	{{0x24},{0x80,0x00}},   // Extra Reset                          
	{{0x59},{0x00,0x18}},   // Black Rows                           	          
	{{0xf0},{0x00,0x02}},   //Page 2                                   
	{{0x39},{0x06,0xc2}},   // AE Line size Context A                  
	{{0x3a},{0x05,0xf9}},   // AE Line size Context B                  
	{{0x3b},{0x04,0x10}},   // AE shutter delay limit Context A        
	{{0x3c},{0x04,0x9d}},   // AE shutter delay limit Context B        
	{{0x57},{0x01,0x04}},   // Context A Flicker full frame time (60Hz)
	{{0x58},{0x01,0x38}},   // Context A Flicker full frame time (50Hz)
	{{0x59},{0x01,0x26}},   // Context B Flicker full frame time (60Hz)
	{{0x5a},{0x01,0x61}},   // Context B Flicker full frame time (50Hz)
	{{0x5c},{0x12,0x0d}},   // 60Hz Flicker Search Range               
	{{0xf0},{0x00,0x01}},   //Page 1
	{{0x9b},{0x02,0x02}},   //YCbCr 
	{{0x3a},{0x02,0x02}},   
	{{0xf0},{0x00,0x02}},  	//Page 2.                   
	{{0xd2},{0x00,0x7f}},   //ViewFinder OFF.. 1280*1024
	{{0x5b},{0x00,0x00}},   //Auto Flicker 50Hz 
	{{0xcc},{0x00,0x04}},   
	{{0xcb},{0x00,0x01}},   
	{{0xf0},{0x00,0x00}}, 	//Page 0                               
};

//static struct regval_list sensor_vga_regs[] = {
//	//NULL
//};
//
//static struct regval_list sensor_qvga_regs[] = {
//	//NULL
//};

/*
 * The white balance settings
 * Here only tune the R G B channel gain. 
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
//static struct regval_list sensor_wb_auto_regs[] = {
//	//NULL
//};

static struct regval_list sensor_wb_cloud_regs[] = {
	//NULL
};

static struct regval_list sensor_wb_daylight_regs[] = {
	//tai yang guang
	//NULL
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	//bai re guang
	//NULL
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	//ri guang deng
	//NULL
};

static struct regval_list sensor_wb_tungsten_regs[] = {
	//wu si deng
	//NULL
};

/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_bw_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_negative_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_emboss_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
	//NULL
};

/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {
	//NULL
};

static struct regval_list sensor_brightness_neg3_regs[] = {
	//NULL
};

static struct regval_list sensor_brightness_neg2_regs[] = {
	//NULL
};

static struct regval_list sensor_brightness_neg1_regs[] = {
	//NULL
};

static struct regval_list sensor_brightness_zero_regs[] = {
	//NULL
};

static struct regval_list sensor_brightness_pos1_regs[] = {
	//NULL
};

static struct regval_list sensor_brightness_pos2_regs[] = {
	//NULL
};

static struct regval_list sensor_brightness_pos3_regs[] = {
	//NULL
};

static struct regval_list sensor_brightness_pos4_regs[] = {
	//NULL
};

/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_neg3_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_neg2_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_neg1_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_zero_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_pos1_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_pos2_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_pos3_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_pos4_regs[] = {
	//NULL
};

/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_neg3_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_neg2_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_neg1_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_zero_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_pos1_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_pos2_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_pos3_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_pos4_regs[] = {
	//NULL
};

/*
 * The exposure target setttings
 */
static struct regval_list sensor_ev_neg4_regs[] = {
	//NULL
};

static struct regval_list sensor_ev_neg3_regs[] = {
	//NULL
};

static struct regval_list sensor_ev_neg2_regs[] = {
	//NULL
};

static struct regval_list sensor_ev_neg1_regs[] = {
	//NULL
};

static struct regval_list sensor_ev_zero_regs[] = {
	//NULL
};

static struct regval_list sensor_ev_pos1_regs[] = {
	//NULL
};

static struct regval_list sensor_ev_pos2_regs[] = {
	//NULL
};

static struct regval_list sensor_ev_pos3_regs[] = {
	//NULL
};

static struct regval_list sensor_ev_pos4_regs[] = {
	//NULL
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 * 
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{{0xf0},{0x00,0x01}},   //Page 1
	{{0x9b},{0x02,0x02}},   //Context B YCbYCr 
	{{0x3a},{0x02,0x02}},   //Context A YCbYCr
};


static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{{0xf0},{0x00,0x01}},   //Page 1
	{{0x9b},{0x02,0x03}},   //Context B YCrYCb 
	{{0x3a},{0x02,0x03}},   //Context A YCrYCb
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{{0xf0},{0x00,0x01}},   //Page 1
	{{0x9b},{0x02,0x01}},   //Context B CrYCbY 
	{{0x3a},{0x02,0x01}},   //Context A CrYCbY
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{{0xf0},{0x00,0x01}},   //Page 1
	{{0x9b},{0x02,0x00}},   //Context B CbYCrY 
	{{0x3a},{0x02,0x00}},   //Context A CbYCrY
};

//static struct regval_list sensor_fmt_raw[] = {
//	
//};



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
//		csi_err("Error %d on register write\n", ret);
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
//	else {
//		csi_err("Error %d on register read\n", ret);
//	}
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
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
		csi_err("sensor_write error!\n");
	}
	return ret;
}


/*
 * Write a list of register settings;
 */
static int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *vals , uint size)
{
	int i,ret;
//	unsigned char rd;
	
	if (size == 0)
		return -EINVAL;
	
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

	switch(on)
	{
		case CSI_SUBDEV_STBY_ON:
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_STBY_ON,"csi_stby");
			msleep(10);
			break;
		case CSI_SUBDEV_STBY_OFF:
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_STBY_OFF,"csi_stby");
			msleep(10);
			break;
		default:
			return -EINVAL;	
	}
	return 0;
}
 
static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);

	switch(val)
	{
		case CSI_SUBDEV_RST_OFF:
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_RST_OFF,"csi_reset");
			msleep(10);
			break;
		case CSI_SUBDEV_RST_ON:
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_RST_ON,"csi_reset");
			msleep(10);
			break;
		case CSI_SUBDEV_RST_PUL:
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_RST_OFF,"csi_reset");
			msleep(10);
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_RST_ON,"csi_reset");
			msleep(100);
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_RST_OFF,"csi_reset");
			msleep(10);
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	int ret;
	
	switch(val) {
		case CSI_SUBDEV_INIT_FULL:
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_PWR_ON,"csi_power_en");
			msleep(10);
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_STBY_ON,"csi_stby");
			msleep(10);
			gpio_write_one_pin_value(dev->csi_pin_hd,CSI_STBY_OFF,"csi_stby");
			msleep(10);
		case CSI_SUBDEV_INIT_SIMP:
			ret = sensor_reset(sd,CSI_SUBDEV_RST_PUL);
			if(ret < 0)
				return ret;
			break;
		default:
			return -EINVAL;
	}
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
			
			ccm_info->mclk 	=	info->ccm_info->mclk ;
			ccm_info->vref 	=	info->ccm_info->vref ;
			ccm_info->href 	=	info->ccm_info->href ;
			ccm_info->clock	=	info->ccm_info->clock;
//			info->ccm_info->iocfg	=	ccm_info->iocfg	;
			break;
		}
		case CSI_SUBDEV_CMD_SET_INFO:
		{
			struct sensor_info *info = to_state(sd);
			__csi_subdev_info_t *ccm_info = arg;
			
			info->ccm_info->mclk 	=	ccm_info->mclk 	;
			info->ccm_info->vref 	=	ccm_info->vref 	;
			info->ccm_info->href 	=	ccm_info->href 	;
			info->ccm_info->clock	=	ccm_info->clock	;
//			info->ccm_info->iocfg	=	ccm_info->iocfg	;
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
	int	regs_size;
	int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.regs 		= sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp		= 2,
	},
	{
		.desc		= "YVYU 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YVYU,
		.regs 		= sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp		= 2,
	},
	{
		.desc		= "UYVY 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
		.regs 		= sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp		= 2,
	},
	{
		.desc		= "VYUY 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_VYUY,
		.regs 		= sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp		= 2,
	},
//	{
//		.desc		= "Raw RGB Bayer",
//		.pixelformat	= V4L2_PIX_FMT_SBGGR8,
//		.regs 		= sensor_fmt_raw,
//		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
//		.bpp		= 1
//	},
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
	int regs_size;
/* h/vref stuff */
} sensor_win_sizes[] = {
	/* SXGA */
	{
		.width		= SXGA_WIDTH,
		.height		= SXGA_HEIGHT,
		.regs 		= NULL,
		.regs_size	= 0,
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
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
static int sensor_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	/* see sensor_s_parm and sensor_g_parm for the meaning of value */
	
	switch (qc->id) {
//	case V4L2_CID_BRIGHTNESS:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_CONTRAST:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_SATURATION:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_HUE:
//		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
//	case V4L2_CID_VFLIP:
//	case V4L2_CID_HFLIP:
//		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
//	case V4L2_CID_GAIN:
//		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
//	case V4L2_CID_AUTOGAIN:
//		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
//	case V4L2_CID_EXPOSURE:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
//	case V4L2_CID_EXPOSURE_AUTO:
//		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
//	case V4L2_CID_DO_WHITE_BALANCE:
//		return v4l2_ctrl_query_fill(qc, 0, 5, 1, 0);
//	case V4L2_CID_AUTO_WHITE_BALANCE:
//		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
//	case V4L2_CID_COLORFX:
//		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 0);
	}
	return -EINVAL;
}

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
//	int ret;
//	struct sensor_info *info = to_state(sd);
//	struct regval_list regs;
//	
//	regs.reg_num[0] = 0x03;
//	regs.value[0] = 0x00; //PAGEMODE 0x00
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_g_hflip!\n");
//		return ret;
//	}
//	
//	regs.reg_num[0] = 0x11;
//	ret = sensor_read(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_read err at sensor_g_hflip!\n");
//		return ret;
//	}
//
//	printk("read_val = %x\n",regs.value[0]);
//	
//	regs.value[0] &= (1<<0);
//	regs.value[0] = regs.value[0]>>0;		//0x11 bit0 is hflip enable
//	
//	*value = regs.value[0];
//	info->hflip = regs.value[0];
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
//	int ret;
//	struct sensor_info *info = to_state(sd);
//	struct regval_list regs;
//	
//	regs.reg_num[0] = 0x03;
//	regs.value[0] = 0x00; //PAGEMODE 0x00
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_s_hflip!\n");
//		return ret;
//	}
//	
//	regs.reg_num[0] = 0x11;
//	ret = sensor_read(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_read err at sensor_s_hflip!\n");
//		return ret;
//	}
//
//	printk("read_val = %x\n",regs.value[0]);
//	
//	switch(value) {
//	case 0:
//		regs.value[0] &= 0xfe;
//		break;
//	case 1:
//		regs.value[0] |= 0x01;
//		break;
//	default:
//		break;
//	}	
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_s_hflip!\n");
//		return ret;
//	}
//	
//	info->hflip = value;
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
//	int ret;
//	struct sensor_info *info = to_state(sd);
//	struct regval_list regs;
//	
//	regs.reg_num[0] = 0x03;
//	regs.value[0] = 0x00; //PAGEMODE 0x00
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_g_vflip!\n");
//		return ret;
//	}
//	
//	regs.reg_num[0] = 0x11;
//	ret = sensor_read(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_read err at sensor_g_vflip!\n");
//		return ret;
//	}
//
//	printk("read_val = %x\n",regs.value[0]);
//	
//	regs.value[0] &= (1<<1);
//	regs.value[0] = regs.value[0]>>1;		//0x11 bit1 is vflip enable
//	
//	*value = regs.value[0];
//	info->hflip = regs.value[0];
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
//	int ret;
//	struct sensor_info *info = to_state(sd);
//	struct regval_list regs;
//	
//	regs.reg_num[0] = 0x03;
//	regs.value[0] = 0x00; //PAGEMODE 0x00
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_s_vflip!\n");
//		return ret;
//	}
//	
//	regs.reg_num[0] = 0x11;
//	ret = sensor_read(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_read err at sensor_s_vflip!\n");
//		return ret;
//	}
//
//	printk("read_val = %x\n",regs.value[0]);
//	
//	switch(value) {
//	case 0:
//		regs.value[0] &= 0xfd;
//		break;
//	case 1:
//		regs.value[0] |= 0x02;
//		break;
//	default:
//		break;
//	}	
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_s_vflip!\n");
//		return ret;
//	}
//	
//	info->hflip = value;
	return 0;
}

static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
//	int ret;
//	struct sensor_info *info = to_state(sd);
//	struct regval_list regs;
//	
//	regs.reg_num[0] = 0x03;
//	regs.value[0] = 0x20;		//PAGEMODE 0x20
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_g_autoexp!\n");
//		return ret;
//	}
//	
//	regs.reg_num[0] = 0x10;
//	ret = sensor_read(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_read err at sensor_g_autoexp!\n");
//		return ret;
//	}
//
//	printk("read_val = %x\n",regs.value[0]);
//	
//	regs.value[0] &= 0x80;
//	if (regs.value[0] == 0x80) {
//		*value = V4L2_EXPOSURE_AUTO;
//	}
//	else
//	{
//		*value = V4L2_EXPOSURE_MANUAL;
//	}
//	
//	info->autoexp = *value;
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{
//	int ret;
//	struct sensor_info *info = to_state(sd);
//	struct regval_list regs;
//	
//	regs.reg_num[0] = 0x03;
//	regs.value[0] = 0x20;		//PAGEMODE 0x20
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_s_autoexp!\n");
//		return ret;
//	}
//	
//	regs.reg_num[0] = 0x10;
//	ret = sensor_read(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_read err at sensor_s_autoexp!\n");
//		return ret;
//	}
//
//	printk("read_val = %x\n",regs.value[0]);
//	
//	switch (value) {
//		case V4L2_EXPOSURE_AUTO:
//		  regs.value[0] |= 0x80;
//			break;
//		case V4L2_EXPOSURE_MANUAL:
//			regs.value[0] &= 0x7f;
//			break;
//		case V4L2_EXPOSURE_SHUTTER_PRIORITY:
//			return -EINVAL;    
//		case V4L2_EXPOSURE_APERTURE_PRIORITY:
//			return -EINVAL;
//		default:
//			return -EINVAL;
//	}
//		
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_s_autoexp!\n");
//		return ret;
//	}
//	
//	info->autoexp = value;
	
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
//	int ret;
//	struct sensor_info *info = to_state(sd);
//	struct regval_list regs;
//	
//	regs.reg_num[0] = 0x03;
//	regs.value[0] = 0x22;
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_g_autowb!\n");
//		return ret;
//	}
//	
//	regs.reg_num[0] = 0x10;
//	ret = sensor_read(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_read err at sensor_g_autowb!\n");
//		return ret;
//	}
//
//	printk("read_val = %x\n",regs.value[0]);
//	
//	regs.value[0] &= (1<<7);
//	regs.value[0] = regs.value[0]>>7;		//0x10 bit7 is awb enable
//		
//	*value = regs.value[0];
//	info->autowb = *value;
	
	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
//	int ret;
//	struct sensor_info *info = to_state(sd);
//	struct regval_list regs;
//	
//	ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
//	if (ret < 0) {
//		csi_err("sensor_write_array err at sensor_s_autowb!\n");
//		return ret;
//	}
//	
//	regs.reg_num[0] = 0x10;
//	ret = sensor_read(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_read err at sensor_s_autowb!\n");
//		return ret;
//	}
//
//	printk("read_val = %x\n",regs.value[0]);
//	
//	switch(value) {
//	case 0:
//		regs.value[0] &= 0x7f;
//		break;
//	case 1:
//		regs.value[0] |= 0x80;
//		break;
//	default:
//		break;
//	}	
//	ret = sensor_write(sd, regs.reg_num, regs.value);
//	if (ret < 0) {
//		csi_err("sensor_write err at sensor_s_autowb!\n");
//		return ret;
//	}
//	
//	info->autowb = value;
	return 0;
}

static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_hue(struct v4l2_subdev *sd, int value)
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
/* *********************************************end of ******************************************** */

static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->brightness;
	return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_brightness_neg4_regs, ARRAY_SIZE(sensor_brightness_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_brightness_neg3_regs, ARRAY_SIZE(sensor_brightness_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_brightness_neg2_regs, ARRAY_SIZE(sensor_brightness_neg2_regs));
			break;   
		case -1:
			ret = sensor_write_array(sd, sensor_brightness_neg1_regs, ARRAY_SIZE(sensor_brightness_neg1_regs));
			break;
		case 0:   
			ret = sensor_write_array(sd, sensor_brightness_zero_regs, ARRAY_SIZE(sensor_brightness_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_brightness_pos1_regs, ARRAY_SIZE(sensor_brightness_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_brightness_pos2_regs, ARRAY_SIZE(sensor_brightness_pos2_regs));
			break;	
		case 3:
			ret = sensor_write_array(sd, sensor_brightness_pos3_regs, ARRAY_SIZE(sensor_brightness_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_brightness_pos4_regs, ARRAY_SIZE(sensor_brightness_pos4_regs));
			break;
		default:
			return -EINVAL;
	}
	
	if (ret < 0) {
		csi_err("sensor_write_array err at sensor_s_brightness!\n");
		return ret;
	}
	
	info->brightness = value;
	return 0;
}

static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->contrast;
	return 0;
}

static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_contrast_neg4_regs, ARRAY_SIZE(sensor_contrast_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_contrast_neg3_regs, ARRAY_SIZE(sensor_contrast_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_contrast_neg2_regs, ARRAY_SIZE(sensor_contrast_neg2_regs));
			break;   
		case -1:
			ret = sensor_write_array(sd, sensor_contrast_neg1_regs, ARRAY_SIZE(sensor_contrast_neg1_regs));
			break;
		case 0:   
			ret = sensor_write_array(sd, sensor_contrast_zero_regs, ARRAY_SIZE(sensor_contrast_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_contrast_pos1_regs, ARRAY_SIZE(sensor_contrast_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_contrast_pos2_regs, ARRAY_SIZE(sensor_contrast_pos2_regs));
			break;	
		case 3:
			ret = sensor_write_array(sd, sensor_contrast_pos3_regs, ARRAY_SIZE(sensor_contrast_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_contrast_pos4_regs, ARRAY_SIZE(sensor_contrast_pos4_regs));
			break;
		default:
			return -EINVAL;
	}
	
	if (ret < 0) {
		csi_err("sensor_write_array err at sensor_s_contrast!\n");
		return ret;
	}
	
	info->contrast = value;
	return 0;
}

static int sensor_g_saturation(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->saturation;
	return 0;
}

static int sensor_s_saturation(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_saturation_neg4_regs, ARRAY_SIZE(sensor_saturation_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_saturation_neg3_regs, ARRAY_SIZE(sensor_saturation_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_saturation_neg2_regs, ARRAY_SIZE(sensor_saturation_neg2_regs));
			break;   
		case -1:
			ret = sensor_write_array(sd, sensor_saturation_neg1_regs, ARRAY_SIZE(sensor_saturation_neg1_regs));
			break;
		case 0:   
			ret = sensor_write_array(sd, sensor_saturation_zero_regs, ARRAY_SIZE(sensor_saturation_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_saturation_pos1_regs, ARRAY_SIZE(sensor_saturation_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_saturation_pos2_regs, ARRAY_SIZE(sensor_saturation_pos2_regs));
			break;	
		case 3:
			ret = sensor_write_array(sd, sensor_saturation_pos3_regs, ARRAY_SIZE(sensor_saturation_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_saturation_pos4_regs, ARRAY_SIZE(sensor_saturation_pos4_regs));
			break;
		default:
			return -EINVAL;
	}
	
	if (ret < 0) {
		csi_err("sensor_write_array err at sensor_s_saturation!\n");
		return ret;
	}
	
	info->saturation = value;
	return 0;
}

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->exp;
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_ev_neg4_regs, ARRAY_SIZE(sensor_ev_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_ev_neg3_regs, ARRAY_SIZE(sensor_ev_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_ev_neg2_regs, ARRAY_SIZE(sensor_ev_neg2_regs));
			break;   
		case -1:
			ret = sensor_write_array(sd, sensor_ev_neg1_regs, ARRAY_SIZE(sensor_ev_neg1_regs));
			break;
		case 0:   
			ret = sensor_write_array(sd, sensor_ev_zero_regs, ARRAY_SIZE(sensor_ev_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_ev_pos1_regs, ARRAY_SIZE(sensor_ev_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_ev_pos2_regs, ARRAY_SIZE(sensor_ev_pos2_regs));
			break;	
		case 3:
			ret = sensor_write_array(sd, sensor_ev_pos3_regs, ARRAY_SIZE(sensor_ev_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_ev_pos4_regs, ARRAY_SIZE(sensor_ev_pos4_regs));
			break;
		default:
			return -EINVAL;
	}
	
	if (ret < 0) {
		csi_err("sensor_write_array err at sensor_s_exp!\n");
		return ret;
	}
	
	info->exp = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_whiteblance *wb_type = (enum v4l2_whiteblance*)value;
	
	*wb_type = info->wb;
	
	return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd,
		enum v4l2_whiteblance value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	if (value == V4L2_WB_AUTO) {
		ret = sensor_s_autowb(sd, 1);
		return ret;
	} 
	else {
		ret = sensor_s_autowb(sd, 0);
		if(ret < 0) {
			csi_err("sensor_s_autowb error, return %x!\n",ret);
			return ret;
		}
		
		switch (value) {
			case V4L2_WB_CLOUD:
			  ret = sensor_write_array(sd, sensor_wb_cloud_regs, ARRAY_SIZE(sensor_wb_cloud_regs));
				break;
			case V4L2_WB_DAYLIGHT:
				ret = sensor_write_array(sd, sensor_wb_daylight_regs, ARRAY_SIZE(sensor_wb_daylight_regs));
				break;
			case V4L2_WB_INCANDESCENCE:
				ret = sensor_write_array(sd, sensor_wb_incandescence_regs, ARRAY_SIZE(sensor_wb_incandescence_regs));
				break;    
			case V4L2_WB_FLUORESCENT:
				ret = sensor_write_array(sd, sensor_wb_fluorescent_regs, ARRAY_SIZE(sensor_wb_fluorescent_regs));
				break;
			case V4L2_WB_TUNGSTEN:   
				ret = sensor_write_array(sd, sensor_wb_tungsten_regs, ARRAY_SIZE(sensor_wb_tungsten_regs));
				break;
			default:
				return -EINVAL;
		} 
	}
	
	if (ret < 0) {
		csi_err("sensor_s_wb error, return %x!\n",ret);
		return ret;
	}
	
	info->wb = value;
	msleep(100);
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd,
		__s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx*)value;
	
	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd,
		enum v4l2_colorfx value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	
	switch (value) {
	case V4L2_COLORFX_NONE:
	  ret = sensor_write_array(sd, sensor_colorfx_none_regs, ARRAY_SIZE(sensor_colorfx_none_regs));
		break;
	case V4L2_COLORFX_BW:
		ret = sensor_write_array(sd, sensor_colorfx_bw_regs, ARRAY_SIZE(sensor_colorfx_bw_regs));
		break;  
	case V4L2_COLORFX_SEPIA:
		ret = sensor_write_array(sd, sensor_colorfx_sepia_regs, ARRAY_SIZE(sensor_colorfx_sepia_regs));
		break;   
	case V4L2_COLORFX_NEGATIVE:
		ret = sensor_write_array(sd, sensor_colorfx_negative_regs, ARRAY_SIZE(sensor_colorfx_negative_regs));
		break;
	case V4L2_COLORFX_EMBOSS:   
		ret = sensor_write_array(sd, sensor_colorfx_emboss_regs, ARRAY_SIZE(sensor_colorfx_emboss_regs));
		break;
	case V4L2_COLORFX_SKETCH:     
		ret = sensor_write_array(sd, sensor_colorfx_sketch_regs, ARRAY_SIZE(sensor_colorfx_sketch_regs));
		break;
	case V4L2_COLORFX_SKY_BLUE:
		ret = sensor_write_array(sd, sensor_colorfx_sky_blue_regs, ARRAY_SIZE(sensor_colorfx_sky_blue_regs));
		break;
	case V4L2_COLORFX_GRASS_GREEN:
		ret = sensor_write_array(sd, sensor_colorfx_grass_green_regs, ARRAY_SIZE(sensor_colorfx_grass_green_regs));
		break;
	case V4L2_COLORFX_SKIN_WHITEN:
		ret = sensor_write_array(sd, sensor_colorfx_skin_whiten_regs, ARRAY_SIZE(sensor_colorfx_skin_whiten_regs));
		break;
	case V4L2_COLORFX_VIVID:
		ret = sensor_write_array(sd, sensor_colorfx_vivid_regs, ARRAY_SIZE(sensor_colorfx_vivid_regs));
		break;
	default:
		return -EINVAL;
	}
	
	if (ret < 0) {
		csi_err("sensor_s_colorfx error, return %x!\n",ret);
		return ret;
	}
	
	info->clrfx = value;
	msleep(100);
	return 0;
}


static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_g_contrast(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_g_saturation(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return sensor_g_hue(sd, &ctrl->value);	
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
	case V4L2_CID_DO_WHITE_BALANCE:
		return sensor_g_wb(sd, &ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_g_autowb(sd, &ctrl->value);
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
	case V4L2_CID_SATURATION:
		return sensor_s_saturation(sd, ctrl->value);
	case V4L2_CID_HUE:
		return sensor_s_hue(sd, ctrl->value);		
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
	case V4L2_CID_DO_WHITE_BALANCE:
		return sensor_s_wb(sd,
				(enum v4l2_whiteblance) ctrl->value);	
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
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
	
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp = 0;
	info->autoexp = 0;
	info->autowb = 1;
	info->wb = 0;
	info->clrfx = 0;
	
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
	{ "mt9m112", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = "mt9m112",
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
