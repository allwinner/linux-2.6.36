#include "disp_display.h"
#include "disp_de.h"
#include "disp_lcd.h"
#include "disp_tv.h"
#include "disp_event.h"
#include "disp_sprite.h"
#include "disp_combined.h"
#include "disp_scaler.h"
#include "disp_video.h"
#include "disp_clk.h"

__disp_dev_t gdisp;


#define VIDEO_PLL0        270  //when tv mode enable, this value will be changed
#define VIDEO_PLL1        270  //when tv mode enable, this value will be changed

#define DEIM0_CLK_SEL     2  //image0 clock source //0: video pll0, 1: video pll1, 2: dram pll
#define DEIM0_CLK_DIV     1  //image0 clock divide //0: im0pll/1,  1: im0pll/2,  2: im0pll/3....15:im0pll/16
#define DEIM1_CLK_SEL     2  //image1 clock source //0: video pll0, 1: video pll1, 2: dram pll
#define DEIM1_CLK_DIV     1  //image1 clock divide //0: im1pll/1,  1: im1pll/2,  2: im1pll/3...,15:im1pll/16

#define SCAL0_CLK_SEL     0  //scaler0 clock source //0: video pll0, 1: video pll1, 2: dram pll
#define SCAL0_CLK_DIV     0  //scaler0 clock divide //0: sc0pll/1,  1: sc0pll/2..... 15:sc0pll/16
#define SCAL1_CLK_SEL     0  //scaler1 clock source //0: video pll0, 1: video pll1, 2: dram pll
#define SCAL1_CLK_DIV     0  //scaler1 clock divide //0: sc1pll/1,  1: sc1pll/2.....15:sc1pll/16

#define TCON0_CLK_SEL     0  //tcon0 clcok source  //0: video pll0(1x), 1: video pll1(1x), 2: video pll0(2x), 3: video pll1(2x)
#define TCON1_CLK_SEL     0  //tcon1 clock divide  //0: video pll0(1x), 1: video pll1(1x), 2: video pll0(2x), 3: video pll1(2x)
#define TCON0_CH1_CLK_SEL     1  //tcon0 ch1(tve0) clock source  //0: video pll0(1x), 1: video pll1(1x), 2: video pll0(2x), 3: video pll1(2x)
#define TCON0_CH1_CLK_DIV     4  //tcon0 ch1(tve0) clock divide  //0: tve0pll/1,  1: tve0pll/2,...,15: tve0pll/16
#define TCON1_CH1_CLK_SEL     0  //tcon1 ch1(tve1)clock divide  //0: video pll0(1x), 1: video pll1(1x), 2: video pll0(2x), 3: video pll1(2x)
#define TCON1_CH1_CLK_DIV     0  //tcon0 ch1(tve1) clock divide  //0: tve1pll/1,  1: tve1pll/2,...,15: tve1pll/16

#define HDMI_CLK_SEL      0  //HDMI clock source //0:pll3(1x), 1:pll7(1x), 2: pll3(2x), 3:pll7(2x)
#define HDMI_CLK_DIV      0  //HDMI clcok divide //0:hdmipll/1, 1:hdmipll/2, 2:hdmipll/2,,,15:hdmipll/16



//CCM register
#define CCM_BASE           			0xf1c20000

#define CCM_CORE_PLL_REG 			(CCM_BASE+0x00)
#define CCM_CORE_PLL_TUNE_REG   	(CCM_BASE+0x04)
#define CCM_AUDIO_PLL_REG 			(CCM_BASE+0x08)
#define CCM_AUDIO_PLL_TUNE_REG		(CCM_BASE+0x0c)
#define CCM_VIDEO_PLL3_REG    		(CCM_BASE+0x10)
#define CCM_VIDEO_PLL3_TUNE_REG		(CCM_BASE+0x14)
#define CCM_VE_PLL_REG 			    (CCM_BASE+0x18)
#define CCM_VE_PLL_TUNE_REG		    (CCM_BASE+0x1c)
#define CCM_SDRAM_PLL_REG  			(CCM_BASE+0x20)
#define CCM_SDRAM_PLL_TUNE_REG		(CCM_BASE+0x24)
#define CCM_SATA_PLL_REG       		(CCM_BASE+0x28)
#define CCM_SATA_PLL_TUNE_REG       (CCM_BASE+0x2c)
#define CCM_VIDEO_PLL7_REG			(CCM_BASE+0x30)
#define CCM_VIDEO_PLL7_TUNE_REG		(CCM_BASE+0x34)

#define CCM_HOSC_REG   			    (CCM_BASE+0x50)
#define CCM_BUS_RATIO_REG       	(CCM_BASE+0x54)
#define CCM_APB_CLK_REG    			(CCM_BASE+0x58)
#define CCM_AXI_GATING_REG   		(CCM_BASE+0x5c)
#define CCM_AHB_GATING_REG0   		(CCM_BASE+0x60)
#define CCM_AHB_GATING_REG1   		(CCM_BASE+0x64)
#define CCM_APB_GATING_REG   		(CCM_BASE+0x68)

#define CCM_NFC_CLK_REG  			(CCM_BASE+0x80)
#define CCM_MS_CLK_REG     			(CCM_BASE+0x84)
#define CCM_SD0_CLK_REG  			(CCM_BASE+0x88)
#define CCM_SD1_CLK_REG  			(CCM_BASE+0x8c)
#define CCM_SD2_CLK_REG  			(CCM_BASE+0x90)
#define CCM_SD3_CLK_REG  			(CCM_BASE+0x94)
#define CCM_TS_CLK_REG  			(CCM_BASE+0x98)
#define CCM_SS_CLK_REG  			(CCM_BASE+0x9c)
#define CCM_SPI0_CLK_REG  			(CCM_BASE+0xa0)
#define CCM_SPI1_CLK_REG  			(CCM_BASE+0xa4)
#define CCM_SPI2_CLK_REG  			(CCM_BASE+0xa8)
#define CCM_PATA_CLK_REG  			(CCM_BASE+0xac)
#define CCM_IR0_CLK_REG  			(CCM_BASE+0xb0)
#define CCM_IR1_CLK_REG  			(CCM_BASE+0xb4)
#define CCM_I2S_CLK_REG  			(CCM_BASE+0xb8)
#define CCM_AC97_CLK_REG  			(CCM_BASE+0xbc)
#define CCM_SPDIF_CLK_REG 			(CCM_BASE+0xc0)
#define CCM_KP_CLK_REG 			    (CCM_BASE+0xc4)
#define CCM_SATA_CLK_REG		    (CCM_BASE+0xc8)
#define CCM_USB_CLK_REG		      	(CCM_BASE+0xcc)
#define CCM_GPS_CLK_REG		      	(CCM_BASE+0xd0)

#define CCM_SDRAM_CLK_REG	      	(CCM_BASE+0x100)
#define CCM_BE0_CLK_REG	      		(CCM_BASE+0x104)
#define CCM_BE1_CLK_REG	      		(CCM_BASE+0x108)
#define CCM_FE0_CLK_REG	      		(CCM_BASE+0x10c)
#define CCM_FE1_CLK_REG	      		(CCM_BASE+0x110)
#define CCM_MP_CLK_REG	      		(CCM_BASE+0x114)
#define CCM_LCD0_CLK_REG	      	(CCM_BASE+0x118)
#define CCM_LCD1_CLK_REG	      	(CCM_BASE+0x11c)
#define CCM_ISP_CLK_REG	      	    (CCM_BASE+0x120)
#define CCM_TVD_CLK_REG	      		(CCM_BASE+0x128)
#define CCM_TVE0_CLK_REG	     	(CCM_BASE+0x12c)
#define CCM_TVE1_CLK_REG	     	(CCM_BASE+0x130)
#define CCM_CSI0_CLK_REG	     	(CCM_BASE+0x134)
#define CCM_CSI1_CLK_REG	     	(CCM_BASE+0x138)
#define CCM_VE_CLK_REG	     		(CCM_BASE+0x13c)
#define CCM_AUDIO_CLK_REG	      	(CCM_BASE+0x140)
#define CCM_AVS_CLK_REG				(CCM_BASE+0x144)
#define CCM_ACE_CLK_REG				(CCM_BASE+0x148)
#define CCM_AUDIO_CLK_REG	      	(CCM_BASE+0x140)
#define CCM_AVS_CLK_REG				(CCM_BASE+0x144)
#define CCM_ACE_CLK_REG				(CCM_BASE+0x148)
#define CCM_LVDS_CLK_REG			(CCM_BASE+0x14c)
#define CCM_HDMI_CLK_REG			(CCM_BASE+0x150)
#define CCM_MALI_CLK_REG            (CCM_BASE+0x154)

void pll_cfg(void)
{
	__u32 reg_val;
	
	//set up video pll3
	reg_val = sys_get_wvalue(CCM_VIDEO_PLL3_REG);
	reg_val &= (~(0x1<<15));    //mode select
	reg_val |= (0x0<<15);       //mode select, 0: integer mode, 1: fractional mode
	reg_val &= (~(0x7f<<0));
	reg_val |= (VIDEO_PLL0/3)<<0;
	reg_val |= 0x1<<31;			//video PLL0 enable
	sys_put_wvalue(CCM_VIDEO_PLL3_REG, reg_val);
	
	//set up video pll7
	reg_val = sys_get_wvalue(CCM_VIDEO_PLL7_REG);
	reg_val &= (~(0x1<<15));    //mode select
	reg_val |= (0x0<<15);       //mode select, 0: integer mode, 1: fractional mode
	reg_val &= (~(0x7f<<0));
	reg_val |= (VIDEO_PLL1/3)<<0;
	reg_val |= 0x1<<31;			//video PLL1 enable
	sys_put_wvalue(CCM_VIDEO_PLL7_REG, reg_val);
}

__s32  de_ccu_set(void)
{
	__u32 reg_val;

	pll_cfg();
	
    //==== 1    set pll factor ============================
	//set CCM_VIDEO_PLL_REG 
	/*//set up video pll0
	reg_val = sys_get_wvalue(CCM_VIDEO_PLL3_REG);
	reg_val &= (~(0x7f<<0));
	reg_val |= (VIDEO_PLL0/3)<<0;
	reg_val |= 0x1<<31;			//video PLL0 enable
	put_wvalue(CCM_VIDEO_PLL3_REG, reg_val);*/
	
	//====2   set module pll select and divide=====================
	//debe0 clock select and divide
	reg_val = 0x0;
	reg_val |= DEIM0_CLK_SEL<<24;  //de image0 clcok sel
	reg_val |= DEIM0_CLK_DIV<<0;   //de image0 clcok div
	sys_put_wvalue(CCM_BE0_CLK_REG,reg_val);
	
	//debe0 clock select and divide
	reg_val = 0x0;
	reg_val |= DEIM1_CLK_SEL<<24;  //de image1 clcok sel
	reg_val |= DEIM1_CLK_DIV<<0;   //de image1 clcok div
	sys_put_wvalue(CCM_BE1_CLK_REG,reg_val);
	
	//defe0 clock select and divide
	reg_val = 0x0;
	reg_val |= SCAL0_CLK_SEL<<24;  //de scaler0 clcok sel
	reg_val |= SCAL0_CLK_DIV<<0;   //de scaler0 clcok div
	sys_put_wvalue(CCM_FE0_CLK_REG,reg_val);
	
	//defe1 clock select and divide
	reg_val = 0x0;
	reg_val |= SCAL1_CLK_SEL<<24;  //de scaler1 clcok sel
	reg_val |= SCAL1_CLK_DIV<<0;   //de scaler1 clcok div
	sys_put_wvalue(CCM_FE1_CLK_REG,reg_val);
	
	//demp clock select and divide
	//reg_val = 0x0;
	//reg_val |= DEMP_CLK_SEL<<24;  //de mp clcok sel
	//reg_val |= DEMP_CLK_DIV<<0;   //de mp clcok div
	//sys_put_wvalue(CCM_MP_CLK_REG,reg_val);
	
	//lcd0 ch0 clock select
	reg_val = 0x0;
	reg_val |= TCON0_CLK_SEL<<24;  //lcd0 ch0 clcok sel
	sys_put_wvalue(CCM_LCD0_CLK_REG,reg_val);
	
	//lcd1 ch0 clock select
	reg_val = 0x0;
	reg_val |= TCON1_CLK_SEL<<24;  //lcd1 ch0 clcok sel
	sys_put_wvalue(CCM_LCD1_CLK_REG,reg_val);
	
	//tvd clock select
	//reg_val = 0x0;
	//reg_val |= TVD_CLK_SEL<<24;  //tvd clcok sel
	//sys_put_wvalue(CCM_TVD_CLK_REG,reg_val);
	
	//lcd0 ch1 clock select and divide, tve0 
	reg_val = 0x0;
	reg_val |= TCON0_CH1_CLK_SEL<<24;  //lcd0 ch1 2x clcok sel
	reg_val |= TCON0_CH1_CLK_DIV<<0;   //pre-divide befor 2x
	reg_val |= 1<<11;                  //lcd0 ch1 1x clock
	sys_put_wvalue(CCM_TVE0_CLK_REG,reg_val);
	
	//lcd1 ch1 clock select and divide, tve1 
	reg_val = 0x0;
	reg_val |= TCON1_CH1_CLK_SEL<<24;  //lcd0 ch1 2x clcok sel
	reg_val |= TCON1_CH1_CLK_DIV<<0;   //pre-divide befor 2x
	reg_val |= 1<<11;                  //lcd0 ch1 1x clock
	sys_put_wvalue(CCM_TVE1_CLK_REG,reg_val);
	
	//csi0 clock select and divide
	//reg_val = 0x0;
	//reg_val |= CSI0_CLK_SEL<<24;  //csi0 clcok sel
	//reg_val |= CSI0_CLK_DIV<<0;   //csi0 clcok div
	//sys_put_wvalue(CCM_CSI0_CLK_REG,reg_val);
	
	//csi1 clock select and divide
	//reg_val = 0x0;
	//reg_val |= CSI1_CLK_SEL<<24;  //csi1 clcok sel
	//reg_val |= CSI1_CLK_DIV<<0;   //csi1 clcok div
	//sys_put_wvalue(CCM_CSI1_CLK_REG,reg_val);
	
	//HDMI clock select and divide
	reg_val = 0x0;
	reg_val |= HDMI_CLK_SEL<<24;  //csi1 clcok sel
	reg_val |= HDMI_CLK_DIV<<0;   //csi1 clcok div
	sys_put_wvalue(CCM_HDMI_CLK_REG,reg_val);
	
	//mali400 clock select and divide
	//reg_val = 0x0;
	//reg_val |= MALI_CLK_SEL<<24;  //csi1 clcok sel
	//reg_val |= MALI_CLK_DIV<<0;   //csi1 clcok div
	//sys_put_wvalue(CCM_MALI_CLK_REG,reg_val);

	
	//====3   enable pll============================================
	//set CCM_VIDEO_PLL_REG 
	/*
	//set up video pll0
	reg_val = sys_get_wvalue(CCM_VIDEO_PLL3_REG);
	reg_val |= 0x1<<31;			//video PLL0 enable
	sys_put_wvalue(CCM_VIDEO_PLL3_REG, reg_val);
	
	//set up video pll1
	reg_val = sys_get_wvalue(CCM_VIDE1_PLL7_REG);
	reg_val |= 0x1<<31;			//video PLL1 enable
	sys_put_wvalue(CCM_VIDEO_PLL7_REG, reg_val);	
	*/
	
	//====4   gate on module clock===================================
	//ahb gating
	reg_val = sys_get_wvalue(CCM_AHB_GATING_REG1);
	//reg_val |= 0x1<<20;  //gpu 3d
	//reg_val |= 0x1<<19;  //gpu 2d
	//reg_val |= 0x1<<18;  //mp
	reg_val |= 0x1<<15;  //scaler1 1
	reg_val |= 0x1<<14;  //scaler 0
	reg_val |= 0x1<<13;  //image 1
	reg_val |= 0x1<<12;  //image 0
	reg_val |= 0x1<<11;  //hdmi
	//reg_val |= 0x1<<9;  //csi 1
	//reg_val |= 0x1<<8;  //csi 0
	reg_val |= 0x1<<5;  //lcd 1
	reg_val |= 0x1<<4;  //lcd 0
	reg_val |= 0x1<<3;  //tve 1
	reg_val |= 0x1<<2;  //tve 0
	//reg_val |= 0x1<<1;  //tvd
	sys_put_wvalue(CCM_AHB_GATING_REG1,reg_val);
	
	
	//dram gating
	reg_val = sys_get_wvalue(CCM_SDRAM_CLK_REG);
	//reg_val |= 0x1<<30;  //gpu 3d
	//reg_val |= 0x1<<29;  //gpu 2d avg
	//reg_val |= 0x1<<28;  //demp
	reg_val |= 0x1<<27;  //image 1
	reg_val |= 0x1<<26;  //image 0
	reg_val |= 0x1<<25;  //scaler 1
	reg_val |= 0x1<<24;  //scaler 0
	sys_put_wvalue(CCM_SDRAM_CLK_REG,reg_val);
	
	//debe0 clock gating
	reg_val = sys_get_wvalue(CCM_BE0_CLK_REG);;
	reg_val |= 0x1<<31;
	sys_put_wvalue(CCM_BE0_CLK_REG,reg_val);
	
	//debe0 clock gating
	reg_val = sys_get_wvalue(CCM_BE1_CLK_REG);
	reg_val |= 0x1<<31;
	sys_put_wvalue(CCM_BE1_CLK_REG,reg_val);
	
	//defe0 clock gating
	reg_val = sys_get_wvalue(CCM_FE0_CLK_REG);;
	reg_val |= 0x1<<31;
	sys_put_wvalue(CCM_FE0_CLK_REG,reg_val);
	
	//defe1 clock gating
	reg_val = sys_get_wvalue(CCM_FE1_CLK_REG);;
	reg_val |= 0x1<<31;
	sys_put_wvalue(CCM_FE1_CLK_REG,reg_val);
	
	//demp clock gating
	//reg_val = sys_get_wvalue(CCM_MP_CLK_REG);;
	//reg_val |= 0x1<<31;
	//sys_put_wvalue(CCM_MP_CLK_REG,reg_val);
	
	//lcd0 ch0 clock gating
	reg_val = sys_get_wvalue(CCM_LCD0_CLK_REG);;
	reg_val |= 0x1<<31;
	sys_put_wvalue(CCM_LCD0_CLK_REG,reg_val);
	
	//lcd1 ch0 clock gating
	reg_val = sys_get_wvalue(CCM_LCD1_CLK_REG);;
	reg_val |= 0x1<<31;
	sys_put_wvalue(CCM_LCD1_CLK_REG,reg_val);
	
	//tvd clock clock gating
	//reg_val = sys_get_wvalue(CCM_TVD_CLK_REG);;
	//reg_val |= 0x1<<31;
	//sys_put_wvalue(CCM_TVD_CLK_REG,reg_val);
	
	//lcd0 ch1 clock gating, tve0 
	reg_val = sys_get_wvalue(CCM_TVE0_CLK_REG);;
	reg_val |= 0x1<<31;  //for gating 2x
	reg_val |= 0x1<<15;  //for gating 1x
	sys_put_wvalue(CCM_TVE0_CLK_REG,reg_val);
	
	//lcd1 ch1 clock gating, tve1 
	reg_val = sys_get_wvalue(CCM_TVE1_CLK_REG);;
	reg_val |= 0x1<<31;
	reg_val |= 0x1<<15;  //for gating 1x
	sys_put_wvalue(CCM_TVE1_CLK_REG,reg_val);
	
	//csi0 clock gating
	//reg_val = sys_get_wvalue(CCM_CSI0_CLK_REG);;
	//reg_val |= 0x1<<31;
	//sys_put_wvalue(CCM_CSI0_CLK_REG,reg_val);
	
	//csi1 clock gating
	//reg_val = sys_get_wvalue(CCM_CSI1_CLK_REG);;
	//reg_val |= 0x1<<31;
	//sys_put_wvalue(CCM_CSI1_CLK_REG,reg_val);
	
	//HDMI clock gating
	reg_val = sys_get_wvalue(CCM_HDMI_CLK_REG);;
	reg_val |= 0x1<<31;
	sys_put_wvalue(CCM_HDMI_CLK_REG,reg_val);
	
	//MALI clock gating
	//reg_val = sys_get_wvalue(CCM_MALI_CLK_REG);;
	//reg_val |= 0x1<<31;
	//sys_put_wvalue(CCM_MALI_CLK_REG,reg_val);
	 
	 //scal0, scal1, image0, image1 reset bit
	//debe0 reset release
	reg_val = sys_get_wvalue(CCM_BE0_CLK_REG);;
	reg_val |= 0x1<<30;
	sys_put_wvalue(CCM_BE0_CLK_REG,reg_val);
	
	//debe1 reset release
	reg_val = sys_get_wvalue(CCM_BE1_CLK_REG);
	reg_val |= 0x1<<30;
	sys_put_wvalue(CCM_BE1_CLK_REG,reg_val);
	
	//defe0 reset release
	reg_val = sys_get_wvalue(CCM_FE0_CLK_REG);;
	reg_val |= 0x1<<30;
	sys_put_wvalue(CCM_FE0_CLK_REG,reg_val);
	
	//defe1 reset release
	reg_val = sys_get_wvalue(CCM_FE1_CLK_REG);;
	reg_val |= 0x1<<30;
	sys_put_wvalue(CCM_FE1_CLK_REG,reg_val);
	
	//demp reset release
	//reg_val = sys_get_wvalue(CCM_MP_CLK_REG);;
	//reg_val |= 0x1<<30;
	//sys_put_wvalue(CCM_MP_CLK_REG,reg_val);
	
	//lcd0 ch0 reset release
	reg_val = sys_get_wvalue(CCM_LCD0_CLK_REG);;
	reg_val |= 0x1<<30;
	sys_put_wvalue(CCM_LCD0_CLK_REG,reg_val);
	
	//lcd1 ch0 reset release
	reg_val = sys_get_wvalue(CCM_LCD1_CLK_REG);;
	reg_val |= 0x1<<30;
	sys_put_wvalue(CCM_LCD1_CLK_REG,reg_val);
	
	//tvd reset release
	//reg_val = sys_get_wvalue(CCM_TVD_CLK_REG);;
	//reg_val |= 0x1<<30;
	//sys_put_wvalue(CCM_TVD_CLK_REG,reg_val);
	
	//lcd0 ch1 reset release, tve0 
	reg_val = sys_get_wvalue(CCM_TVE0_CLK_REG);;
	reg_val |= 0x1<<30;  //for gating 2x
	sys_put_wvalue(CCM_TVE0_CLK_REG,reg_val);
	
	//lcd1 ch1 reset release, tve1 
	reg_val = sys_get_wvalue(CCM_TVE1_CLK_REG);;
	reg_val |= 0x1<<30;
	sys_put_wvalue(CCM_TVE1_CLK_REG,reg_val);
	
	//csi0 reset release
	//reg_val = sys_get_wvalue(CCM_CSI0_CLK_REG);;
	//reg_val |= 0x1<<30;
	//sys_put_wvalue(CCM_CSI0_CLK_REG,reg_val);
	
	//csi1 reset release
	//reg_val = sys_get_wvalue(CCM_CSI1_CLK_REG);;
	//reg_val |= 0x1<<30;
	//sys_put_wvalue(CCM_CSI1_CLK_REG,reg_val);
	
	//LVDS reset release
	//reg_val = sys_get_wvalue(CCM_LVDS_CLK_REG);;
	//reg_val |= 0x1<<30;
	//sys_put_wvalue(CCM_LVDS_CLK_REG,reg_val);
	
	//MALI reset release
	//reg_val = sys_get_wvalue(CCM_MALI_CLK_REG);;
	//reg_val |= 0x1<<30;
	//sys_put_wvalue(CCM_MALI_CLK_REG,reg_val);
	
	//lcd0 io
	/*sys_put_wvalue(0xf1c2086c,0x22222222);
	sys_put_wvalue(0xf1c20870,0x22222222);
	sys_put_wvalue(0xf1c20874,0x22222222);
	sys_put_wvalue(0xf1c20878,0x22222222);*/
	return 0;
}


__s32 BSP_disp_init(__disp_bsp_init_para * para)
{
    __u32 i = 0, screen_id = 0;

    memset(&gdisp,0x00,sizeof(__disp_dev_t));

    for(screen_id = 0; screen_id < 2; screen_id++)
    {
        gdisp.screen[screen_id].max_layers = 4;
        for(i = 0;i < gdisp.screen[screen_id].max_layers;i++)
        {
            gdisp.screen[screen_id].layer_manage[i].para.prio = IDLE_PRIO;
        }
        gdisp.screen[screen_id].image_output_type = IMAGE_OUTPUT_LCDC;
        
        gdisp.screen[screen_id].bright = 50;
        gdisp.screen[screen_id].contrast = 50;
        gdisp.screen[screen_id].saturation = 50;
        
        gdisp.scaler[screen_id].bright = 32;
        gdisp.scaler[screen_id].contrast = 32;
        gdisp.scaler[screen_id].saturation = 32;
        gdisp.scaler[screen_id].hue = 32;
    }
    memcpy(&gdisp.init_para,para,sizeof(__disp_bsp_init_para));
    memset(g_video,0,sizeof(g_video));

    DE_Set_Reg_Base(0, para->base_image0);
    DE_Set_Reg_Base(1, para->base_image1);
    DE_SCAL_Set_Reg_Base(0, para->base_scaler0);
    DE_SCAL_Set_Reg_Base(1, para->base_scaler1);
    LCDC_set_reg_base(0,para->base_lcdc0);
    LCDC_set_reg_base(1,para->base_lcdc1);
    TVE_set_reg_base(0, para->base_tvec0);
    TVE_set_reg_base(1, para->base_tvec1);

	disp_pll_init();
	disp_clk_init();
	
	de_ccu_set();//tmp

    Scaler_Init(0);
    Scaler_Init(1);
    Image_init(0);
    Image_init(1);
    Disp_lcdc_init(0);
    Disp_lcdc_init(1);
    Disp_TVEC_Init(0);
    Disp_TVEC_Init(1);

    return DIS_SUCCESS;
}

__s32 BSP_disp_exit(__u32 mode)
{
    if(mode == DISP_EXIT_MODE_CLEAN_ALL)
    {
        BSP_disp_close();
        
        Scaler_Exit(0);
        Scaler_Exit(1);
        Image_exit(0);
        Image_exit(1);
        Disp_lcdc_exit(0);
        Disp_lcdc_exit(1);
        Disp_TVEC_Exit(0);
        Disp_TVEC_Exit(1);
    }
    else if(mode == DISP_EXIT_MODE_CLEAN_PARTLY)
    {
        OSAL_InterruptDisable(INTC_IRQNO_LCDC0);
        OSAL_UnRegISR(INTC_IRQNO_LCDC0,Disp_lcdc_event_proc,(void*)0);

        OSAL_InterruptDisable(INTC_IRQNO_LCDC1);
        OSAL_UnRegISR(INTC_IRQNO_LCDC1,Disp_lcdc_event_proc,(void*)0);

        OSAL_InterruptDisable(INTC_IRQNO_SCALER0);
        OSAL_UnRegISR(INTC_IRQNO_SCALER0,Scaler_event_proc,(void*)0);

        OSAL_InterruptDisable(INTC_IRQNO_SCALER1);
        OSAL_UnRegISR(INTC_IRQNO_SCALER1,Scaler_event_proc,(void*)0);
    }
    
    return DIS_SUCCESS;
}

__s32 BSP_disp_open(void)
{
    return DIS_SUCCESS;
}

__s32 BSP_disp_close(void)
{
    __u32 sel = 0;

    for(sel = 0; sel<2; sel++)
    {
       Image_close(sel);
        if(gdisp.scaler[sel].status & SCALER_USED)
        {
            Scaler_close(sel);
        }
        if(gdisp.screen[sel].lcdc_status & LCDC_TCON0_USED)
        {
            TCON0_close(sel);
            LCDC_close(sel);
        }
        else if(gdisp.screen[sel].lcdc_status & LCDC_TCON1_USED)
        {
    	    TCON1_close(sel);
    	    LCDC_close(sel);
        }
        else if(gdisp.screen[sel].status & (TV_ON | VGA_ON))
        {
        	TVE_close(sel);
        }
    }
    

    gdisp.screen[sel].status &= (IMAGE_USED_MASK & LCD_OFF & TV_OFF & VGA_OFF & HDMI_OFF);
    gdisp.screen[sel].lcdc_status &= (LCDC_TCON0_USED_MASK & LCDC_TCON1_USED_MASK);
    return DIS_SUCCESS;
}

