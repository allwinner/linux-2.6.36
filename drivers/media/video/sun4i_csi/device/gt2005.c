/*
 * A V4L2 driver for GalaxyCore gt2005 cameras.
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
MODULE_DESCRIPTION("A low-level driver for GalaxyCore GT2005 sensors");
MODULE_LICENSE("GPL");


#define MCLK (24*1000*1000)
#define VREF_POL	CSI_HIGH
#define HREF_POL	CSI_HIGH
#define CLK_POL		CSI_RISING
//#define IO_CFG		0						//0 for csi0
#define V4L2_IDENT_SENSOR 0x2005

//define the voltage level of control signal
#define CSI_STBY_ON			0
#define CSI_STBY_OFF 		1
#define CSI_RST_ON			0
#define CSI_RST_OFF			1
#define CSI_PWR_ON			1
#define CSI_PWR_OFF			0

#define REG_TERM 0xff
#define VAL_TERM 0xff


#define REG_ADDR_STEP 2
#define REG_DATA_STEP 1
#define REG_STEP 			(REG_ADDR_STEP+REG_DATA_STEP)

/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define UXGA_WIDTH		1600
#define UXGA_HEIGHT		1200
#define HD720_WIDTH 	1280
#define HD720_HEIGHT	720
#define SVGA_WIDTH		800
#define SVGA_HEIGHT 	600
#define VGA_WIDTH			640
#define VGA_HEIGHT		480
#define QVGA_WIDTH		320
#define QVGA_HEIGHT		240
#define CIF_WIDTH			352
#define CIF_HEIGHT		288
#define QCIF_WIDTH		176
#define	QCIF_HEIGHT		144

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The gt2005 sits on i2c with ID 0x78
 */
#define I2C_ADDR 0x78

/* Registers */


/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */
struct snesor_colorfx_struct; /* coming later */
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
	int hflip;
	int vflip;
	int gain;
	int autogain;
	int exp;
	enum v4l2_exposure_auto_type autoexp;
	enum v4l2_colorfx clrfx;
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
	{{0x01,	0x01} , {0x00}},
	{{0x01,	0x03} , {0x00}},

	//Hcount&Vcount
	{{0x01,	0x05} , {0x00}},
	{{0x01,	0x06} , {0xF0}},
	{{0x01,	0x07} , {0x00}},
	{{0x01,	0x08} , {0x1C}},

	//Binning&Resoultion
	//1600*1200
	{{0x01,	0x09} , {0x01}},
	{{0x01,	0x0A} , {0x00}},
	{{0x01,	0x0B} , {0x00}},
	{{0x01,	0x0C} , {0x00}},
	{{0x01,	0x0D} , {0x08}},
	{{0x01,	0x0E} , {0x00}},
	{{0x01,	0x0F} , {0x08}},
	{{0x01,	0x10} , {0x06}},
	{{0x01,	0x11} , {0x40}},
	{{0x01,	0x12} , {0x04}},
	{{0x01,	0x13} , {0xB0}},

	//YUV Mode
	{{0x01,	0x14} , {0x04}},//YUYV

	//Picture Effect
	{{0x01,	0x15} , {0x00}},

	//PLL&Frame Rate
	{{0x01,	0x16} , {0x02}},
	{{0x01,	0x17} , {0x00}},
	{{0x01,	0x18} , {0x67}},
	{{0x01,	0x19} , {0x02}},
	{{0x01,	0x1A} , {0x04}},
	{{0x01,	0x1B} , {0x01}},

	//DCLK Polarity
	{{0x01,	0x1C} , {0x00}},

	//Do not change
	{{0x01,	0x1D} , {0x02}},
	{{0x01,	0x1E} , {0x00}},
	
	{{0x01,	0x1F} , {0x00}},
	{{0x01,	0x20} , {0x1C}},
	{{0x01,	0x21} , {0x00}},
	{{0x01,	0x22} , {0x04}},
	{{0x01,	0x23} , {0x00}},
	{{0x01,	0x24} , {0x00}},
	{{0x01,	0x25} , {0x00}},
	{{0x01,	0x26} , {0x00}},
	{{0x01,	0x27} , {0x00}},
	{{0x01,	0x28} , {0x00}},

	//Contrast
	{{0x02,	0x00} , {0x00}},

	//Brightness
	{{0x02,	0x01} , {0x00}},

	//Saturation
	{{0x02,	0x02} , {0x40}},

	//Do not change
	{{0x02,	0x03} , {0x00}},
	{{0x02,	0x04} , {0x03}},
	{{0x02,	0x05} , {0x1F}},
	{{0x02,	0x06} , {0x0B}},
	{{0x02,	0x07} , {0x20}},
	{{0x02,	0x08} , {0x00}},
	{{0x02,	0x09} , {0x2A}},
	{{0x02,	0x0A} , {0x01}},
                       
	//Sharpness          
	{{0x02,	0x0B} , {0x48}},
	{{0x02,	0x0C} , {0x64}},

	//Do not change
	{{0x02,	0x0D}, {0xC8}},
	{{0x02,	0x0E}, {0xBC}},
	{{0x02,	0x0F}, {0x08}},
	{{0x02,	0x10}, {0xD6}},
	{{0x02,	0x11}, {0x00}},
	{{0x02,	0x12}, {0x20}},
	{{0x02,	0x13}, {0x81}},
	{{0x02,	0x14}, {0x15}},
	{{0x02,	0x15}, {0x00}},
	{{0x02,	0x16}, {0x00}},
	{{0x02,	0x17}, {0x00}},
	{{0x02,	0x18}, {0x46}},
	{{0x02,	0x19}, {0x30}},
	{{0x02,	0x1A}, {0x03}},
	{{0x02,	0x1B}, {0x28}},
	{{0x02,	0x1C}, {0x02}},
	{{0x02,	0x1D}, {0x60}},
	{{0x02,	0x1E}, {0x00}},
	{{0x02,	0x1F}, {0x00}},
	{{0x02,	0x20}, {0x08}},
	{{0x02,	0x21}, {0x08}},
	{{0x02,	0x22}, {0x04}},
	{{0x02,	0x23}, {0x00}},
	{{0x02,	0x24}, {0x1F}},
	{{0x02,	0x25}, {0x1E}},
	{{0x02,	0x26}, {0x18}},
	{{0x02,	0x27}, {0x1D}},
	{{0x02,	0x28}, {0x1F}},
	{{0x02,	0x29}, {0x1F}},
	{{0x02,	0x2A}, {0x01}},
	{{0x02,	0x2B}, {0x04}},
	{{0x02,	0x2C}, {0x05}},
	{{0x02,	0x2D}, {0x05}},
	{{0x02,	0x2E}, {0x04}},
	{{0x02,	0x2F}, {0x03}},
	{{0x02,	0x30}, {0x02}},
	{{0x02,	0x31}, {0x1F}},
	{{0x02,	0x32}, {0x1A}},
	{{0x02,	0x33}, {0x19}},
	{{0x02,	0x34}, {0x19}},
	{{0x02,	0x35}, {0x1B}},
	{{0x02,	0x36}, {0x1F}},
	{{0x02,	0x37}, {0x04}},
	{{0x02,	0x38}, {0xEE}},
	{{0x02,	0x39}, {0xFF}},
	{{0x02,	0x3A}, {0x00}},
	{{0x02,	0x3B}, {0x00}},
	{{0x02,	0x3C}, {0x00}},
	{{0x02,	0x3D}, {0x00}},
	{{0x02,	0x3E}, {0x00}},
	{{0x02,	0x3F}, {0x00}},
	{{0x02,	0x40}, {0x00}},
	{{0x02,	0x41}, {0x00}},
	{{0x02,	0x42}, {0x00}},
	{{0x02,	0x43}, {0x21}},
	{{0x02,	0x44}, {0x42}},
	{{0x02,	0x45}, {0x53}},
	{{0x02,	0x46}, {0x54}},
	{{0x02,	0x47}, {0x54}},
	{{0x02,	0x48}, {0x54}},
	{{0x02,	0x49}, {0x33}},
	{{0x02,	0x4A}, {0x11}},
	{{0x02,	0x4B}, {0x00}},
	{{0x02,	0x4C}, {0x00}},
	{{0x02,	0x4D}, {0xFF}},
	{{0x02,	0x4E}, {0xEE}},
	{{0x02,	0x4F}, {0xDD}},
	{{0x02,	0x50}, {0x00}},
	{{0x02,	0x51}, {0x00}},
	{{0x02,	0x52}, {0x00}},
	{{0x02,	0x53}, {0x00}},
	{{0x02,	0x54}, {0x00}},
	{{0x02,	0x55}, {0x00}},
	{{0x02,	0x56}, {0x00}},
	{{0x02,	0x57}, {0x00}},
	{{0x02,	0x58}, {0x00}},
	{{0x02,	0x59}, {0x00}},
	{{0x02,	0x5A}, {0x00}},
	{{0x02,	0x5B}, {0x00}},
	{{0x02,	0x5C}, {0x00}},
	{{0x02,	0x5D}, {0x00}},
	{{0x02,	0x5E}, {0x00}},
	{{0x02,	0x5F}, {0x00}},
	{{0x02,	0x60}, {0x00}},
	{{0x02,	0x61}, {0x00}},
	{{0x02,	0x62}, {0x00}},
	{{0x02,	0x63}, {0x00}},
	{{0x02,	0x64}, {0x00}},
	{{0x02,	0x65}, {0x00}},
	{{0x02,	0x66}, {0x00}},
	{{0x02,	0x67}, {0x00}},
	{{0x02,	0x68}, {0x8F}},
	{{0x02,	0x69}, {0xA3}},
	{{0x02,	0x6A}, {0xB4}},
	{{0x02,	0x6B}, {0x90}},
	{{0x02,	0x6C}, {0x00}},
	{{0x02,	0x6D}, {0xD0}},
	{{0x02,	0x6E}, {0x60}},
	{{0x02,	0x6F}, {0xA0}},
	{{0x02,	0x70}, {0x40}},
	{{0x03,	0x00}, {0x81}},
	{{0x03,	0x01}, {0x80}},
	{{0x03,	0x02}, {0x22}},
	{{0x03,	0x03}, {0x06}},
	{{0x03,	0x04}, {0x03}},
	{{0x03,	0x05}, {0x83}},
	{{0x03,	0x06}, {0x00}},
	{{0x03,	0x07}, {0x22}},
	{{0x03,	0x08}, {0x00}},
	{{0x03,	0x09}, {0x55}},
	{{0x03,	0x0A}, {0x55}},
	{{0x03,	0x0B}, {0x55}},
	{{0x03,	0x0C}, {0x54}},
	{{0x03,	0x0D}, {0x1F}},
	{{0x03,	0x0E}, {0x0A}},
	{{0x03,	0x0F}, {0x10}},
	{{0x03,	0x10}, {0x04}},
	{{0x03,	0x11}, {0xFF}},
	{{0x03,	0x12}, {0x08}},
	{{0x03,	0x13}, {0x28}},
	{{0x03,	0x14}, {0x66}},
	{{0x03,	0x15}, {0x96}},
	{{0x03,	0x16}, {0x26}},
	{{0x03,	0x17}, {0x02}},
	{{0x03,	0x18}, {0x08}},
	{{0x03,	0x19}, {0x0C}},

#ifndef A_LIGHT_CORRECTION
	//Normal AWB Setting
	{{0x03,	0x1A} , {0x81}},
	{{0x03,	0x1B} , {0x00}},
	{{0x03,	0x1C} , {0x3D}},
	{{0x03,	0x1D} , {0x00}},
	{{0x03,	0x1E} , {0xF9}},
	{{0x03,	0x1F} , {0x00}},
	{{0x03,	0x20} , {0x24}},
	{{0x03,	0x21} , {0x14}},
	{{0x03,	0x22} , {0x1A}},
	{{0x03,	0x23} , {0x24}},
	{{0x03,	0x24} , {0x08}},
	{{0x03,	0x25} , {0xF0}},
	{{0x03,	0x26} , {0x30}},
	{{0x03,	0x27} , {0x17}},
	{{0x03,	0x28} , {0x11}},
	{{0x03,	0x29} , {0x22}},
	{{0x03,	0x2A} , {0x2F}},
	{{0x03,	0x2B} , {0x21}},
	{{0x03,	0x2C} , {0xDA}},
	{{0x03,	0x2D} , {0x10}},
	{{0x03,	0x2E} , {0xEA}},
	{{0x03,	0x2F} , {0x18}},
	{{0x03,	0x30} , {0x29}},
	{{0x03,	0x31} , {0x25}},
	{{0x03,	0x32} , {0x12}},
	{{0x03,	0x33} , {0x0F}},
	{{0x03,	0x34} , {0xE0}},
	{{0x03,	0x35} , {0x13}},
	{{0x03,	0x36} , {0xFF}},
	{{0x03,	0x37} , {0x20}},
	{{0x03,	0x38} , {0x46}},
	{{0x03,	0x39} , {0x04}},
	{{0x03,	0x3A} , {0x04}},
	{{0x03,	0x3B} , {0xFF}},
	{{0x03,	0x3C} , {0x01}},
	{{0x03,	0x3D} , {0x00}},

#else
	//A LIGHT CORRECTION
	{{0x03,	0x1A} , {0x81}},
	{{0x03,	0x1B} , {0x00}},
	{{0x03,	0x1C} , {0x1D}},
	{{0x03,	0x1D} , {0x00}},
	{{0x03,	0x1E} , {0xFD}},
	{{0x03,	0x1F} , {0x00}},
	{{0x03,	0x20} , {0xE1}},
	{{0x03,	0x21} , {0x1A}},
	{{0x03,	0x22} , {0xDE}},
	{{0x03,	0x23} , {0x11}},
	{{0x03,	0x24} , {0x1A}},
	{{0x03,	0x25} , {0xEE}},
	{{0x03,	0x26} , {0x50}},
	{{0x03,	0x27} , {0x18}},
	{{0x03,	0x28} , {0x25}},
	{{0x03,	0x29} , {0x37}},
	{{0x03,	0x2A} , {0x24}},
	{{0x03,	0x2B} , {0x32}},
	{{0x03,	0x2C} , {0xA9}},
	{{0x03,	0x2D} , {0x32}},
	{{0x03,	0x2E} , {0xFF}},
	{{0x03,	0x2F} , {0x7F}},
	{{0x03,	0x30} , {0xBA}},
	{{0x03,	0x31} , {0x7F}},
	{{0x03,	0x32} , {0x7F}},
	{{0x03,	0x33} , {0x14}},
	{{0x03,	0x34} , {0x81}},
	{{0x03,	0x35} , {0x14}},
	{{0x03,	0x36} , {0xFF}},
	{{0x03,	0x37} , {0x20}},
	{{0x03,	0x38} , {0x46}},
	{{0x03,	0x39} , {0x04}},
	{{0x03,	0x3A} , {0x04}},
	{{0x03,	0x3B} , {0x00}},
	{{0x03,	0x3C} , {0x00}},
	{{0x03,	0x3D} , {0x00}},
#endif

	//Do not change
	{{0x03,	0x3E} , {0x03}},
	{{0x03,	0x3F} , {0x28}},
	{{0x03,	0x40} , {0x02}},
	{{0x03,	0x41} , {0x60}},
	{{0x03,	0x42} , {0xAC}},
	{{0x03,	0x43} , {0x97}},
	{{0x03,	0x44} , {0x7F}},
	{{0x04,	0x00} , {0xE8}},
	{{0x04,	0x01} , {0x40}},
	{{0x04,	0x02} , {0x00}},
	{{0x04,	0x03} , {0x00}},
	{{0x04,	0x04} , {0xF8}},
	{{0x04,	0x05} , {0x03}},
	{{0x04,	0x06} , {0x03}},
	{{0x04,	0x07} , {0x85}},
	{{0x04,	0x08} , {0x44}},
	{{0x04,	0x09} , {0x1F}},
	{{0x04,	0x0A} , {0x40}},
	{{0x04,	0x0B} , {0x33}},

	//Lens Shading Correction
	{{0x04,	0x0C} , {0xA0}},
	{{0x04,	0x0D} , {0x00}},
	{{0x04,	0x0E} , {0x00}},
	{{0x04,	0x0F} , {0x00}},
	{{0x04,	0x10} , {0x0D}},
	{{0x04,	0x11} , {0x0D}},
	{{0x04,	0x12} , {0x0C}},
	{{0x04,	0x13} , {0x04}},
	{{0x04,	0x14} , {0x00}},
	{{0x04,	0x15} , {0x00}},
	{{0x04,	0x16} , {0x07}},
	{{0x04,	0x17} , {0x09}},
	{{0x04,	0x18} , {0x16}},
	{{0x04,	0x19} , {0x14}},
	{{0x04,	0x1A} , {0x11}},
	{{0x04,	0x1B} , {0x14}},
	{{0x04,	0x1C} , {0x07}},
	{{0x04,	0x1D} , {0x07}},
	{{0x04,	0x1E} , {0x06}},
	{{0x04,	0x1F} , {0x02}},
	{{0x04,	0x20} , {0x42}},
	{{0x04,	0x21} , {0x42}},
	{{0x04,	0x22} , {0x47}},
	{{0x04,	0x23} , {0x39}},
	{{0x04,	0x24} , {0x3E}},
	{{0x04,	0x25} , {0x4D}},
	{{0x04,	0x26} , {0x46}},
	{{0x04,	0x27} , {0x3A}},
	{{0x04,	0x28} , {0x21}},
	{{0x04,	0x29} , {0x21}},
	{{0x04,	0x2A} , {0x26}},
	{{0x04,	0x2B} , {0x1C}},
	{{0x04,	0x2C} , {0x25}},
	{{0x04,	0x2D} , {0x25}},
	{{0x04,	0x2E} , {0x28}},
	{{0x04,	0x2F} , {0x20}},
	{{0x04,	0x30} , {0x3E}},
	{{0x04,	0x31} , {0x3E}},
	{{0x04,	0x32} , {0x33}},
	{{0x04,	0x33} , {0x2E}},
	{{0x04,	0x34} , {0x54}},
	{{0x04,	0x35} , {0x53}},
	{{0x04,	0x36} , {0x3C}},
	{{0x04,	0x37} , {0x51}},
	{{0x04,	0x38} , {0x2B}},
	{{0x04,	0x39} , {0x2B}},
	{{0x04,	0x3A} , {0x38}},
	{{0x04,	0x3B} , {0x22}},
	{{0x04,	0x3C} , {0x3B}},
	{{0x04,	0x3D} , {0x3B}},
	{{0x04,	0x3E} , {0x31}},
	{{0x04,	0x3F} , {0x37}},

	//PWB Gain
	{{0x04,	0x40} , {0x00}},
	{{0x04,	0x41} , {0x4B}},
	{{0x04,	0x42} , {0x00}},
	{{0x04,	0x43} , {0x00}},
	{{0x04,	0x44} , {0x31}},

	{{0x04,	0x45} , {0x00}},
	{{0x04,	0x46} , {0x00}},
	{{0x04,	0x47} , {0x00}},
	{{0x04,	0x48} , {0x00}},
	{{0x04,	0x49} , {0x00}},
	{{0x04,	0x4A} , {0x00}},
	{{0x04,	0x4D} , {0xE0}},
	{{0x04,	0x4E} , {0x05}},
	{{0x04,	0x4F} , {0x07}},
	{{0x04,	0x50} , {0x00}},
	{{0x04,	0x51} , {0x00}},
	{{0x04,	0x52} , {0x00}},
	{{0x04,	0x53} , {0x00}},
	{{0x04,	0x54} , {0x00}},
	{{0x04,	0x55} , {0x00}},
	{{0x04,	0x56} , {0x00}},
	{{0x04,	0x57} , {0x00}},
	{{0x04,	0x58} , {0x00}},
	{{0x04,	0x59} , {0x00}},
	{{0x04,	0x5A} , {0x00}},
	{{0x04,	0x5B} , {0x00}},
	{{0x04,	0x5C} , {0x00}},
	{{0x04,	0x5D} , {0x00}},
	{{0x04,	0x5E} , {0x00}},
	{{0x04,	0x5F} , {0x00}},

	//GAMMA Correction
	{{0x04,	0x60} , {0x80}},
	{{0x04,	0x61} , {0x10}},
	{{0x04,	0x62} , {0x10}},
	{{0x04,	0x63} , {0x10}},
	{{0x04,	0x64} , {0x08}},
	{{0x04,	0x65} , {0x08}},
	{{0x04,	0x66} , {0x11}},
	{{0x04,	0x67} , {0x09}},
	{{0x04,	0x68} , {0x23}},
	{{0x04,	0x69} , {0x2A}},
	{{0x04,	0x6A} , {0x2A}},
	{{0x04,	0x6B} , {0x47}},
	{{0x04,	0x6C} , {0x52}},
	{{0x04,	0x6D} , {0x42}},
	{{0x04,	0x6E} , {0x36}},
	{{0x04,	0x6F} , {0x46}},
	{{0x04,	0x70} , {0x3A}},
	{{0x04,	0x71} , {0x32}},
	{{0x04,	0x72} , {0x32}},
	{{0x04,	0x73} , {0x38}},
	{{0x04,	0x74} , {0x3D}},
	{{0x04,	0x75} , {0x2F}},
	{{0x04,	0x76} , {0x29}},
	{{0x04,	0x77} , {0x48}},

	//Do not change
	{{0x06,	0x00} , {0x00}},
	{{0x06,	0x01} , {0x24}},
	{{0x06,	0x02} , {0x45}},
	{{0x06,	0x03} , {0x0E}},
	{{0x06,	0x04} , {0x14}},
	{{0x06,	0x05} , {0x2F}},
	{{0x06,	0x06} , {0x01}},
	{{0x06,	0x07} , {0x0E}},
	{{0x06,	0x08} , {0x0E}},
	{{0x06,	0x09} , {0x37}},
	{{0x06,	0x0A} , {0x18}},
	{{0x06,	0x0B} , {0xA0}},
	{{0x06,	0x0C} , {0x20}},
	{{0x06,	0x0D} , {0x07}},
	{{0x06,	0x0E} , {0x47}},
	{{0x06,	0x0F} , {0x90}},
	{{0x06,	0x10} , {0x06}},
	{{0x06,	0x11} , {0x0C}},
	{{0x06,	0x12} , {0x28}},
	{{0x06,	0x13} , {0x13}},
	{{0x06,	0x14} , {0x0B}},
	{{0x06,	0x15} , {0x10}},
	{{0x06,	0x16} , {0x14}},
	{{0x06,	0x17} , {0x19}},
	{{0x06,	0x18} , {0x52}},
	{{0x06,	0x19} , {0xA0}},
	{{0x06,	0x1A} , {0x11}},
	{{0x06,	0x1B} , {0x33}},
	{{0x06,	0x1C} , {0x56}},
	{{0x06,	0x1D} , {0x20}},
	{{0x06,	0x1E} , {0x28}},
	{{0x06,	0x1F} , {0x2B}},
	{{0x06,	0x20} , {0x22}},
	{{0x06,	0x21} , {0x11}},
	{{0x06,	0x22} , {0x75}},
	{{0x06,	0x23} , {0x49}},
	{{0x06,	0x24} , {0x6E}},
	{{0x06,	0x25} , {0x80}},
	{{0x06,	0x26} , {0x02}},
	{{0x06,	0x27} , {0x0C}},
	{{0x06,	0x28} , {0x51}},
	{{0x06,	0x29} , {0x25}},
	{{0x06,	0x2A} , {0x01}},
	{{0x06,	0x2B} , {0x3D}},
	{{0x06,	0x2C} , {0x04}},
	{{0x06,	0x2D} , {0x01}},
	{{0x06,	0x2E} , {0x0C}},
	{{0x06,	0x2F} , {0x2C}},
	{{0x06,	0x30} , {0x0D}},
	{{0x06,	0x31} , {0x14}},
	{{0x06,	0x32} , {0x12}},
	{{0x06,	0x33} , {0x34}},
	{{0x06,	0x34} , {0x00}},
	{{0x06,	0x35} , {0x00}},
	{{0x06,	0x36} , {0x00}},
	{{0x06,	0x37} , {0xB1}},
	{{0x06,	0x38} , {0x22}},
	{{0x06,	0x39} , {0x32}},
	{{0x06,	0x3A} , {0x0E}},
	{{0x06,	0x3B} , {0x18}},
	{{0x06,	0x3C} , {0x88}},
	{{0x06,	0x40} , {0xB2}},
	{{0x06,	0x41} , {0xC0}},
	{{0x06,	0x42} , {0x01}},
	{{0x06,	0x43} , {0x26}},
	{{0x06,	0x44} , {0x13}},
	{{0x06,	0x45} , {0x88}},
	{{0x06,	0x46} , {0x64}},
	{{0x06,	0x47} , {0x00}},
	{{0x06,	0x81} , {0x1B}},
	{{0x06,	0x82} , {0xA0}},
	{{0x06,	0x83} , {0x28}},
	{{0x06,	0x84} , {0x00}},
	{{0x06,	0x85} , {0xB0}},
	{{0x06,	0x86} , {0x6F}},
	{{0x06,	0x87} , {0x33}},
	{{0x06,	0x88} , {0x1F}},
	{{0x06,	0x89} , {0x44}},
	{{0x06,	0x8A} , {0xA8}},
	{{0x06,	0x8B} , {0x44}},
	{{0x06,	0x8C} , {0x08}},
	{{0x06,	0x8D} , {0x08}},
	{{0x06,	0x8E} , {0x00}},
	{{0x06,	0x8F} , {0x00}},
	{{0x06,	0x90} , {0x01}},
	{{0x06,	0x91} , {0x00}},
	{{0x06,	0x92} , {0x01}},
	{{0x06,	0x93} , {0x00}},
	{{0x06,	0x94} , {0x00}},
	{{0x06,	0x95} , {0x00}},
	{{0x06,	0x96} , {0x00}},
	{{0x06,	0x97} , {0x00}},
	{{0x06,	0x98} , {0x2A}},
	{{0x06,	0x99} , {0x80}},
	{{0x06,	0x9A} , {0x1F}},
	{{0x06,	0x9B} , {0x00}},
	{{0x06,	0x9C} , {0x02}},
	{{0x06,	0x9D} , {0xF5}},
	{{0x06,	0x9E} , {0x03}},
	{{0x06,	0x9F} , {0x6D}},
	{{0x06,	0xA0} , {0x0C}},
	{{0x06,	0xA1} , {0xB8}},
	{{0x06,	0xA2} , {0x0D}},
	{{0x06,	0xA3} , {0x74}},
	{{0x06,	0xA4} , {0x00}},
	{{0x06,	0xA5} , {0x2F}},
	{{0x06,	0xA6} , {0x00}},
	{{0x06,	0xA7} , {0x2F}},
	{{0x0F,	0x00} , {0x00}},
	{{0x0F,	0x01} , {0x00}},

	//Output Enable
	{{0x01,	0x00} , {0x01}},
	{{0x01,	0x02} , {0x02}},
	{{0x01,	0x04} , {0x03}},                    
};

static struct regval_list sensor_uxga_regs[] = {
	//Resoltion Setting : 1600*1200
	{{0x01,	0x09} , {0x01}},
	{{0x01,	0x0A} , {0x00}},	
	{{0x01,	0x0B} , {0x00}},					
	{{0x01,	0x10} , {0x06}},					
	{{0x01,	0x11} , {0x40}},					
	{{0x01,	0x12} , {0x04}},					
	{{0x01,	0x13} , {0xb0}},
	
	//PLL&Frame Rate 15fps
	{{0x01,	0x16} , {0x02}},
	{{0x01,	0x18} , {0x69}},//0x67
	{{0x01,	0x19} , {0x01}},//0x02
	{{0x01,	0x1A} , {0x04}},
	{{0x01,	0x1B} , {0x00}},//PCLK DIV
	
	//banding
	{{0x03,	0x15} , {0x16}},
	{{0x03,	0x13} , {0x38}},//0x2866 for uxga ,0x388b  for hd720,0x3536 for svga 
	{{0x03,	0x14} , {0x8B}},//
};

static struct regval_list sensor_hd720_regs[] = {
//Resolution Setting : 1280*720
	{{0x01,	0x09} , {0x00}},
	{{0x01,	0x0A} , {0x00}},	
	{{0x01,	0x0B} , {0x03}},					
	{{0x01,	0x10} , {0x05}},					
	{{0x01,	0x11} , {0x00}},					
	{{0x01,	0x12} , {0x02}},					
	{{0x01,	0x13} , {0xD0}},

	//PLL Setting 15FPS Under 24MHz PCLK
	{{0x01,	0x16} , {0x02}},
	{{0x01,	0x18} , {0x69}}, 
	{{0x01,	0x19} , {0x01}},
	{{0x01,	0x1a} , {0x04}},	
	{{0x01,	0x1B} , {0x00}},//PCLK DIV
	
	//banding
	{{0x03,	0x15} , {0x16}},                  			
	{{0x03,	0x13} , {0x38}},
	{{0x03,	0x14} , {0x8B}},
};

static struct regval_list sensor_svga_regs[] = {
	//Resolution Setting : 800*600
	{{0x01,	0x09} , {0x00}},
	{{0x01,	0x0A} , {0x04}},	
	{{0x01,	0x0B} , {0x03}},					
	{{0x01,	0x10} , {0x03}},					
	{{0x01,	0x11} , {0x20}},					
	{{0x01,	0x12} , {0x02}},					
	{{0x01,	0x13} , {0x58}},

	//PLL Setting 30FPS Under 24MHz PCLK
	{{0x01,	0x16} , {0x02}},
	{{0x01,	0x18} , {0x69}},//0x40 
	{{0x01,	0x19} , {0x01}},
	{{0x01,	0x1a} , {0x04}},	
	{{0x01,	0x1B} , {0x00}},//PCLK DIV

	//banding
	{{0x03,	0x15} , {0x16}},                  			
	{{0x03,	0x13} , {0x38}},//0x35
	{{0x03,	0x14} , {0x8B}},//0x36
};

static struct regval_list sensor_vga_regs[] = {
	//Resolution Setting : 640*480
	{{0x01,	0x09} , {0x00}},
	{{0x01,	0x0A} , {0x04}},	
	{{0x01,	0x0B} , {0x03}},					
	{{0x01,	0x10} , {0x02}},					
	{{0x01,	0x11} , {0x80}},					
	{{0x01,	0x12} , {0x01}},					
	{{0x01,	0x13} , {0xE0}},

	//PLL Setting 30FPS Under 24MHz PCLK
	{{0x01,	0x16} , {0x02}},
	{{0x01,	0x18} , {0x69}},//0x40 
	{{0x01,	0x19} , {0x01}},
	{{0x01,	0x1a} , {0x04}},	
	{{0x01,	0x1B} , {0x00}},//PCLK DIV

	//banding
	{{0x03,	0x15} , {0x16}},                  			
	{{0x03,	0x13} , {0x38}},//0x35
	{{0x03,	0x14} , {0x8B}},//0x36
};



/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 * 
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	
	{{0x01,	0x14} , {0x04}}	//YCbYCr
};


static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	
	{{0x01,	0x14} , {0x06}}	//YCrYCb
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	
	{{0x01,	0x14} , {0x02}}	//CrYCbY
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	
	{{0x01,	0x14} , {0x00}}	//CbYCrY
};

static struct regval_list sensor_fmt_raw[] = {
	
	{{0x01,	0x14} , {0x01}}//raw
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
//	for(i = REG_ADDR_STEP; i < REG_STEP; i++)
//		data[i] = 0xff;
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

//	printk("msg.addr=%x\n",msg.addr);
	
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
//		msleep(100);	
		
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
			
//			printk("CSI_SUBDEV_CMD_GET_INFO\n");
			
			ccm_info->mclk 	=	info->ccm_info->mclk ;
			ccm_info->vref 	=	info->ccm_info->vref ;
			ccm_info->href 	=	info->ccm_info->href ;
			ccm_info->clock	=	info->ccm_info->clock;
//			ccm_info->iocfg	=	info->ccm_info->iocfg;
			
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
//			info->ccm_info->iocfg	=	ccm_info->iocfg	;
			
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
	{
		.desc		= "Raw RGB Bayer",
		.pixelformat	= V4L2_PIX_FMT_SBGGR8,
		.regs 		= sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
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
	int regs_size;
/* h/vref stuff */
} sensor_win_sizes[] = {
	/* UXGA */
	{
		.width			= UXGA_WIDTH,
		.height			= UXGA_HEIGHT,
		.regs 			= sensor_uxga_regs,
		.regs_size	= ARRAY_SIZE(sensor_uxga_regs),
	},
	/* 720p */
	{
		.width			= HD720_WIDTH,
		.height			= HD720_HEIGHT,
		.regs				= sensor_hd720_regs,
		.regs_size	= ARRAY_SIZE(sensor_hd720_regs),
	},
	/* SVGA */
	{
		.width			= SVGA_WIDTH,
		.height			= SVGA_HEIGHT,
		.regs				= sensor_svga_regs,
		.regs_size	= ARRAY_SIZE(sensor_svga_regs),
	},
	/* VGA */
	{
		.width			= VGA_WIDTH,
		.height			= VGA_HEIGHT,
		.regs				= sensor_vga_regs,
		.regs_size	= ARRAY_SIZE(sensor_vga_regs),
	},
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
	
	
	sensor_write_array(sd, sensor_fmt->regs , sensor_fmt->regs_size);
	
	ret = 0;
	if (wsize->regs)
		{
			ret = sensor_write_array(sd, wsize->regs , wsize->regs_size);
		}
	
	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	
	return ret;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	
	if (info->width > SVGA_WIDTH && info->height > SVGA_HEIGHT) {
		cp->timeperframe.denominator = SENSOR_FRAME_RATE/2;
	} 
	else {
		cp->timeperframe.denominator = SENSOR_FRAME_RATE;
	}
	
	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
//	struct v4l2_captureparm *cp = &parms->parm.capture;
//	struct v4l2_fract *tpf = &cp->timeperframe;
//	struct sensor_info *info = to_state(sd);
//	int div;

//	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
//		return -EINVAL;
//	if (cp->extendedmode != 0)
//		return -EINVAL;

//	if (tpf->numerator == 0 || tpf->denominator == 0)
//		div = 1;  /* Reset to full rate */
//	else {
//		if (info->width > SVGA_WIDTH && info->height > SVGA_HEIGHT) {
//			div = (tpf->numerator*SENSOR_FRAME_RATE/2)/tpf->denominator;
//		}
//		else {
//			div = (tpf->numerator*SENSOR_FRAME_RATE)/tpf->denominator;
//		}
//	}	
//	
//	if (div == 0)
//		div = 1;
//	else if (div > 8)
//		div = 8;
//	
//	switch()
//	
//	info->clkrc = (info->clkrc & 0x80) | div;
//	tpf->numerator = 1;
//	tpf->denominator = sensor_FRAME_RATE/div;
//	
//	sensor_write(sd, REG_CLKRC, info->clkrc);
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
		return sensor_g_colorfx(sd, &ctrl->value);
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
	{ "gt2005", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = "gt2005",
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
