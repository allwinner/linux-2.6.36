/*********************************************************************************
 * sun4i-i2s.c  --  ALSA Soc Audio Layer
 *
 *  ALLwinner Microelectronic Co., Ltd. 
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 **********************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/gpio.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <mach/hardware.h>
//#include <mach/gpio.h>
//#include <mach/clock.h>
#include <asm/dma.h>
#include <mach/dma.h>

#include "sun4i-hdmipcm.h"
#include "sun4i-hdmiaudio.h"

static int regsave[8];

static struct sw_dma_client sun4i_dma_client_out = {
	.name = "HDMIAUDIO PCM Stereo out"
};

static struct sw_dma_client sun4i_dma_client_in = {
	.name = "HDMIAUDIO PCM Stereo in"
};

static struct sun4i_dma_params sun4i_hdmiaudio_pcm_stereo_out = {
	.client		=	&sun4i_dma_client_out,
	//.channel	=	DMACH_NIIS,
	.channel	=	DMACH_HDMIAUDIO,
	.dma_addr 	=	0,
	.dma_size 	=   4,               /* dma transfer 32bits */
};

static struct sun4i_dma_params sun4i_hdmiaudio_pcm_stereo_in = {
	.client		=	&sun4i_dma_client_in,
	//.channel	=	DMACH_NIIS,
	.channel	=	DMACH_HDMIAUDIO,
	.dma_addr 	=	SUN4I_HDMIAUDIOBASE + SUN4I_HDMIAUDIORXFIFO,
	.dma_size 	=   4,               /* dma transfer 32bits */
};


 struct sun4i_hdmiaudio_info sun4i_hdmiaudio;
 static struct clk *hdmiaudio_apbclk, *hdmiaudio_pll2clk, *hdmiaudio_pllx8, *hdmiaudio_moduleclk;

void sun4i_snd_txctrl_hdmiaudio(int on)
{
//	u32 i;
	u32 reg_val;
//	printk("[IIS]Entered %s\n", __func__);
//	printk("[IIS]in function %s, on = %d, line = %d\n", __func__, on, __LINE__);
	
		//flush TX FIFO
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFCTL);
		reg_val |= SUN4I_HDMIAUDIOFCTL_FTX;	
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFCTL);
	
		//clear TX counter
		writel(0, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOTXCNT);

	if(on){
		/* IIS TX ENABLE */
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val |= SUN4I_HDMIAUDIOCTL_TXEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
			
		/* enable DMA DRQ mode for play */
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
		reg_val |= SUN4I_HDMIAUDIOINT_TXDRQEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
	//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
		
		//Global Enable Digital Audio Interface
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val |= SUN4I_HDMIAUDIOCTL_GEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);

	}else{
		/* HDMIAUDIO TX DISABLE */
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val &= ~SUN4I_HDMIAUDIOCTL_TXEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	//	printk("[HDMIAUDIO]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
			
		/* DISBALE dma DRQ mode */
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
		reg_val &= ~SUN4I_HDMIAUDIOINT_TXDRQEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
	//		printk("[HDMIAUDIO]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
			
		//Global disable Digital Audio Interface
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val &= ~SUN4I_HDMIAUDIOCTL_GEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	//		printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
	}
		
}

void sun4i_snd_rxctrl_hdmiaudio(int on)
{
	u32 reg_val;
	
	//	printk("[IIS]Entered %s\n", __func__);
	
	//flush RX FIFO
	reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFCTL);
	reg_val |= SUN4I_HDMIAUDIOFCTL_FRX;	
	writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFCTL);

	//clear RX counter
	writel(0, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIORXCNT);
	
	if(on){
		/* IIS RX ENABLE */
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val |= SUN4I_HDMIAUDIOCTL_RXEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
			
		/* enable DMA DRQ mode for record */
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
		reg_val |= SUN4I_HDMIAUDIOINT_RXDRQEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
			
		//Global Enable Digital Audio Interface
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val |= SUN4I_HDMIAUDIOCTL_GEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
			
	}else{
		/* IIS RX DISABLE */
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val &= ~SUN4I_HDMIAUDIOCTL_RXEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
			
		/* DISBALE dma DRQ mode */
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
		reg_val &= ~SUN4I_HDMIAUDIOINT_RXDRQEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
				
		//Global disable Digital Audio Interface
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val &= ~SUN4I_HDMIAUDIOCTL_GEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	}
		
}

static inline int sun4i_snd_is_clkmaster(void)
{
	return ((readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL) & SUN4I_HDMIAUDIOCTL_MS) ? 0 : 1);
}

static int sun4i_hdmiaudio_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val;
	u32 reg_val1;

	//	printk("[IIS]Entered %s\n", __func__);
	//SDO ON
	reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	reg_val |= (SUN4I_HDMIAUDIOCTL_SDO0EN | SUN4I_HDMIAUDIOCTL_SDO1EN | SUN4I_HDMIAUDIOCTL_SDO2EN | SUN4I_HDMIAUDIOCTL_SDO3EN); 
	writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);

	/* master or slave selection */
	reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master */
			reg_val |= SUN4I_HDMIAUDIOCTL_MS;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave */
			reg_val &= ~SUN4I_HDMIAUDIOCTL_MS;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val, __LINE__);
	
	
	/* pcm or hdmiaudio mode selection */
	reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val, __LINE__);
	reg_val1 = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val1, __LINE__);
	reg_val1 &= ~SUN4I_HDMIAUDIOFAT0_FMT_RVD;
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK){
		case SND_SOC_DAIFMT_I2S:        /* I2S mode */
			reg_val &= ~SUN4I_HDMIAUDIOCTL_PCM;
			reg_val1 |= SUN4I_HDMIAUDIOFAT0_FMT_I2S;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val &= ~SUN4I_HDMIAUDIOCTL_PCM;
			reg_val1 |= SUN4I_HDMIAUDIOFAT0_FMT_RGT;
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val &= ~SUN4I_HDMIAUDIOCTL_PCM;
			reg_val1 |= SUN4I_HDMIAUDIOFAT0_FMT_LFT;
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
			reg_val |= SUN4I_HDMIAUDIOCTL_PCM;
			reg_val1 &= ~SUN4I_HDMIAUDIOFAT0_LRCP;
			break;
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			reg_val |= SUN4I_HDMIAUDIOCTL_PCM;
			reg_val1 |= SUN4I_HDMIAUDIOFAT0_LRCP;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val, __LINE__);
	writel(reg_val1, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val1, __LINE__);
	
	/* DAI signal inversions */
	reg_val1 = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val1, __LINE__);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
			reg_val1 &= ~SUN4I_HDMIAUDIOFAT0_LRCP;
			reg_val1 &= ~SUN4I_HDMIAUDIOFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val1 |= SUN4I_HDMIAUDIOFAT0_LRCP;
			reg_val1 &= ~SUN4I_HDMIAUDIOFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val1 &= ~SUN4I_HDMIAUDIOFAT0_LRCP;
			reg_val1 |= SUN4I_HDMIAUDIOFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
			reg_val1 |= SUN4I_HDMIAUDIOFAT0_LRCP;
			reg_val1 |= SUN4I_HDMIAUDIOFAT0_BCP;
			break;
	}
	writel(reg_val1, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val1, __LINE__);
	
	/* word select size */
	reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val, __LINE__);
	reg_val &= ~SUN4I_HDMIAUDIOFAT0_WSS_32BCLK;
	if(sun4i_hdmiaudio.ws_size == 16)
		reg_val |= SUN4I_HDMIAUDIOFAT0_WSS_16BCLK;
	else if(sun4i_hdmiaudio.ws_size == 20) 
		reg_val |= SUN4I_HDMIAUDIOFAT0_WSS_20BCLK;
	else if(sun4i_hdmiaudio.ws_size == 24)
		reg_val |= SUN4I_HDMIAUDIOFAT0_WSS_24BCLK;
	else
		reg_val |= SUN4I_HDMIAUDIOFAT0_WSS_32BCLK;
	writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val, __LINE__);

	/* PCM REGISTER setup */
	reg_val = sun4i_hdmiaudio.pcm_txtype&0x3;
	reg_val |= sun4i_hdmiaudio.pcm_rxtype<<2;
	
	if(!sun4i_hdmiaudio.pcm_sync_type)
		reg_val |= SUN4I_HDMIAUDIOFAT1_SSYNC;							//short sync		
	if(sun4i_hdmiaudio.pcm_sw == 16)
		reg_val |= SUN4I_HDMIAUDIOFAT1_SW;
			
	reg_val |=((sun4i_hdmiaudio.pcm_start_slot - 1)&0x3)<<6;		//start slot index
		
	reg_val |= sun4i_hdmiaudio.pcm_lsb_first<<9;			//MSB or LSB first
		
	if(sun4i_hdmiaudio.pcm_sync_period == 256)
		reg_val |= 0x4<<12;
	else if (sun4i_hdmiaudio.pcm_sync_period == 128)
		reg_val |= 0x3<<12;
	else if (sun4i_hdmiaudio.pcm_sync_period == 64)
		reg_val |= 0x2<<12;
	else if (sun4i_hdmiaudio.pcm_sync_period == 32)
		reg_val |= 0x1<<12;
	writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT1);
//	printk("[HDMIAUDIO]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val, __LINE__);
	
	/* set FIFO control register */
	reg_val = 0 & 0x3;
	reg_val |= (0 & 0x1)<<2;
	reg_val |= SUN4I_HDMIAUDIOFCTL_RXTL(0xf);				//RX FIFO trigger level
	reg_val |= SUN4I_HDMIAUDIOFCTL_TXTL(0x40);				//TX FIFO empty trigger level
	writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFCTL);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.regs, reg_val, __LINE__);
	return 0;
}

static int sun4i_hdmiaudio_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun4i_dma_params *dma_data;
//	printk("[IIS]Entered %s\n", __func__);
	
	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sun4i_hdmiaudio_pcm_stereo_out;
	else
		dma_data = &sun4i_hdmiaudio_pcm_stereo_in;

	snd_soc_dai_set_dma_data(rtd->dai->cpu_dai, substream, dma_data);
	
	return 0;
}

static int sun4i_hdmiaudio_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun4i_dma_params *dma_data = 
					snd_soc_dai_get_dma_data(rtd->dai->cpu_dai, substream);

//	printk("[IIS]Entered %s\n", __func__);
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
				{
					sun4i_snd_rxctrl_hdmiaudio(1);
				}
			else
				{
					sun4i_snd_txctrl_hdmiaudio(1);
				}
			sw_dma_ctrl(dma_data->channel, SW_DMAOP_STARTED);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			{
				sun4i_snd_rxctrl_hdmiaudio(0);
			}
			else
			{
			  sun4i_snd_txctrl_hdmiaudio(0);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

		return ret;
}

//freq:   1: 22.5792MHz   0: 24.576MHz  
static int sun4i_hdmiaudio_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id, 
                                 unsigned int freq, int dir)
{
//	u32 reg_val = 0;
//	printk("[IIS]Entered %s\, the MCLK freq = %d\n", __func__, freq);
	if (!freq)
	{
		/*
		reg_val = readl(sun4i_hdmiaudio.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
		reg_val &= ~(1<<27);
		reg_val |= SUN4I_CCM_AUDIO_HOSC_PLL_REG_FRE24576MHZ;
		reg_val &= ~(0x1f<<0);
		reg_val |= 0x9<<0;
		reg_val &= ~(0x7f<<8);
		reg_val |= 0x53<<8;
		writel(reg_val, sun4i_hdmiaudio.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
		*/
		clk_set_rate(hdmiaudio_pll2clk, 24576000);
	}	
	else
	{
		/*
		reg_val = readl(sun4i_hdmiaudio.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
		reg_val &= ~(1<<27);
		reg_val |= SUN4I_CCM_AUDIO_HOSC_PLL_REG_FRE225792MHZ;
		reg_val &= ~(0x1f<<0);
		reg_val |= 0xA<<0;
		reg_val &= ~(0x7F<<8);
		reg_val |= 0x5E<<8;
		writel(reg_val, sun4i_hdmiaudio.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
		*/
		clk_set_rate(hdmiaudio_pll2clk, 22579200);
	}

	return 0;
}

static int sun4i_hdmiaudio_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int div)
{
	u32 reg;
//	printk("[IIS]Entered %s\n", __func__);
	switch (div_id) {
	case SUN4I_DIV_MCLK:
		if(div <= 8)
			div  = (div >>1);
		else if(div  == 12)
			div  = 0x5;
		else if(div  == 16)
			div  = 0x6;
		else if(div == 24)
			div = 0x7;
		else if(div == 32)
			div = 0x8;
		else if(div == 48)
			div = 0x9;
		else if(div == 64)
			div = 0xa;
		reg = (readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCLKD) & ~SUN4I_HDMIAUDIOCLKD_MCLK_MASK) | (div << SUN4I_HDMIAUDIOCLKD_MCLK_OFFS);
		writel(reg, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCLKD);
		break;
	case SUN4I_DIV_BCLK:
		if(div <= 8)
			div = (div>>1) - 1;
		else if(div == 12)
			div = 0x4;
		else if(div == 16)
			div = 0x5;
		else if(div == 32)
			div = 0x6;
		else if(div == 64)
			div = 0x7;
		reg = (readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCLKD) & ~SUN4I_HDMIAUDIOCLKD_BCLK_MASK) | (div <<SUN4I_HDMIAUDIOCLKD_BCLK_OFFS);
		writel(reg, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCLKD);
		break;
	default:
		return -EINVAL;
	}
	
	//diable MCLK output when high samplerate
	reg = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCLKD);
	if(!(reg & 0xF))
	{
		reg &= ~SUN4I_HDMIAUDIOCLKD_MCLKOEN;
		writel(reg, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCLKD);
	}
	else
	{
		reg |= SUN4I_HDMIAUDIOCLKD_MCLKOEN;
		writel(reg, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCLKD);
	}
	
	return 0;
}

u32 sun4i_hdmiaudio_get_clockrate(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(sun4i_hdmiaudio_get_clockrate);

#define VA_I2S_GPIO_CFG0_BASE  (0xF1C20800 + 0x24)
#define VA_I2S_GPIO_CFG1_BASE  (0xF1C20800 + 0x28)


extern int gpio_request_(unsigned gpio, const char *label);


static int sun4i_hdmiaudio_probe(struct platform_device *pdev, struct snd_soc_dai *dai)
{
//		int ret;
		int reg_val = 0;
//	printk("[IIS]Entered %s\n", __func__);

	sun4i_hdmiaudio.regs = ioremap(SUN4I_HDMIAUDIOBASE, 0x100);
	if (sun4i_hdmiaudio.regs == NULL)
		return -ENXIO;
		
	sun4i_hdmiaudio.ccmregs = ioremap(SUN4I_CCMBASE, 0x100);
	if (sun4i_hdmiaudio.ccmregs == NULL)
		return -ENXIO;
		
	sun4i_hdmiaudio.ioregs = ioremap(0x01C20800, 0x100);
	if (sun4i_hdmiaudio.ioregs == NULL)
		return -ENXIO;
	/*
	//audio pll enable
	reg_val = readl(sun4i_hdmiaudio.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.ccmregs, reg_val, __LINE__);
	reg_val |= SUN4I_CCM_AUDIO_HOSC_PLL_REG_AUDIOEN;
	writel(reg_val, sun4i_hdmiaudio.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.ccmregs, reg_val, __LINE__);
	
	//IIS APB gating
	reg_val = readl(sun4i_hdmiaudio.ccmregs + SUN4I_CCM_APB_GATE_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.ccmregs, reg_val, __LINE__);
	reg_val |= SUN4I_CCM_APB_GATE_REG_IISGATE;
	writel(reg_val, sun4i_hdmiaudio.ccmregs + SUN4I_CCM_APB_GATE_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.ccmregs, reg_val, __LINE__);
	
	//iis special clock gating
	reg_val = readl(sun4i_hdmiaudio.ccmregs + SUN4I_CCM_AUDIO_CLK_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.ccmregs, reg_val, __LINE__);
	reg_val |= SUN4I_CCM_AUDIO_CLK_REG_IISSPECIALGATE;
	reg_val &= ~(SUN4I_CCM_AUDIO_CLK_REG_DIV(3));
	reg_val |= SUN4I_CCM_AUDIO_CLK_REG_DIV(3);
	writel(reg_val, sun4i_hdmiaudio.ccmregs + SUN4I_CCM_AUDIO_CLK_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_hdmiaudio.ccmregs, reg_val, __LINE__);
	*/

	//hdmiaudio apbclk
		hdmiaudio_apbclk = clk_get(NULL, "apb_i2s");
		if(-1 == clk_enable(hdmiaudio_apbclk)){
			printk("hdmiaudio_apbclk failed! line = %d\n", __LINE__);
		}

		hdmiaudio_pllx8 = clk_get(NULL, "audio_pllx8");

		//hdmiaudio pll2clk
		hdmiaudio_pll2clk = clk_get(NULL, "audio_pll");

		//hdmiaudio module clk
		hdmiaudio_moduleclk = clk_get(NULL, "i2s");

		if(clk_set_parent(hdmiaudio_moduleclk, hdmiaudio_pll2clk)){
			printk("try to set parent of hdmiaudio_moduleclk to hdmiaudio_pll2ck failed! line = %d\n",__LINE__);
		}

	//	printk("spdif_pll2clk = %ld, line = %d\n", clk_get_rate(spdif_pll2clk), __LINE__);
	//	printk("spdif_pllx8 = %ld, line = %d\n", clk_get_rate(spdif_pllx8), __LINE__);
	//	printk("spdif_moduleclk = %ld, line = %d\n", clk_get_rate(spdif_moduleclk), __LINE__);
	//	printk("spdif_apbclk = %ld, line = %d\n", clk_get_rate(spdif_apbclk), __LINE__);
		
		if(clk_set_rate(hdmiaudio_moduleclk, 24576000/8)){
			printk("set hdmiaudio_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
		}
		
	//	printk("spdif_moduleclk = %ld, line = %d\n", clk_get_rate(spdif_moduleclk), __LINE__);
		

		if(-1 == clk_enable(hdmiaudio_moduleclk)){
			printk("open hdmiaudio_moduleclk failed! line = %d\n", __LINE__);
		}
	
	reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	reg_val |= SUN4I_HDMIAUDIOCTL_GEN;
	writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);

 //for ic
 /*
	// hdmiaudio gpio setting 
	reg_val = readl(sun4i_i2s.ioregs + 0x24);
	reg_val &= ~0xFFF00000;
	reg_val |= 0x22200000;
	writel(reg_val, sun4i_i2s.ioregs + 0x24); 
	
	reg_val = readl(sun4i_i2s.ioregs + 0x28);
	reg_val &= ~0x000FFFFF;
	reg_val |= 0x00022222;
	writel(reg_val, sun4i_i2s.ioregs + 0x28); 

    //for FPGA
    reg_val = readl(sun4i_i2s.ioregs + 0x48);
	reg_val &= ~0xFFFFF000;
	reg_val |=  0x44444000;
	writel(reg_val, sun4i_i2s.ioregs + 0x48); 
	
	reg_val = readl(sun4i_i2s.ioregs + 0x4C);
	reg_val &= ~0x0FFF0000;
	reg_val |=  0x04440000;
	writel(reg_val, sun4i_i2s.ioregs + 0x4C); 
	
	reg_val = readl(sun4i_i2s.ioregs + 0x50);
	reg_val &= ~0x000000F0;
	reg_val |=  0x00000040;
	writel(reg_val, sun4i_i2s.ioregs + 0x50); 
	
	reg_val = readl(sun4i_i2s.ioregs + 0x54);
	reg_val &= ~0x0000000F;
	reg_val |=  0x00000004;
	writel(reg_val, sun4i_i2s.ioregs + 0x54); 
 */   	
	sun4i_snd_txctrl_hdmiaudio(0);
	sun4i_snd_rxctrl_hdmiaudio(0);
	
	iounmap(sun4i_hdmiaudio.ioregs);
	
	return 0;
}

static void hdmiaudioregsave(void)
{
	regsave[0] = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	regsave[1] = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT0);
	regsave[2] = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT1);
	regsave[3] = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFCTL) | (0x3<<24);
	regsave[4] = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
	regsave[5] = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCLKD);
	regsave[6] = readl(sun4i_hdmiaudio.regs + SUN4I_TXCHSEL);
	regsave[7] = readl(sun4i_hdmiaudio.regs + SUN4I_TXCHMAP);
}

static void hdmiaudioregrestore(void)
{
	writel(regsave[0], sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
	writel(regsave[1], sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT0);
	writel(regsave[2], sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFAT1);
	writel(regsave[3], sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOFCTL);
	writel(regsave[4], sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOINT);
	writel(regsave[5], sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCLKD);
	writel(regsave[6], sun4i_hdmiaudio.regs + SUN4I_TXCHSEL);
	writel(regsave[7], sun4i_hdmiaudio.regs + SUN4I_TXCHMAP);
}


	static int sun4i_hdmiaudio_suspend(struct snd_soc_dai *cpu_dai)
	{
		u32 reg_val;
     	printk("[HDMIAUDIO]Entered %s\n", __func__);

		//Global Enable Digital Audio Interface
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val &= ~SUN4I_HDMIAUDIOCTL_GEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		
		hdmiaudioregsave();
		//release the module clock
		clk_disable(hdmiaudio_moduleclk);
	
		clk_disable(hdmiaudio_apbclk);
		
		//release the module clock
		//clk_disable(hdmiaudio_pll2clk);

		//release the module clock
		//clk_disable(hdmiaudio_pllx8);
		
		//printk("[HDMIAUDIO]PLL2 0x01c20008 = %#x, line = %d\n", *(volatile int*)0xF1C20008, __LINE__);
		printk("[HDMIAUDIO]SPECIAL CLK 0x01c20068 = %#x, line= %d\n", *(volatile int*)0xF1C20068, __LINE__);
		printk("[HDMIAUDIO]SPECIAL CLK 0x01c200B8 = %#x, line = %d\n", *(volatile int*)0xF1C200B8, __LINE__);
		
		return 0;
	}
	static int sun4i_hdmiaudio_resume(struct snd_soc_dai *cpu_dai)
	{
		u32 reg_val;
		printk("[HDMIAUDIO]Entered %s\n", __func__);

		//release the module clock
		//clk_enable(hdmiaudio_pllx8);

		//release the module clock
		//clk_enable(hdmiaudio_pll2clk);
		clk_enable(hdmiaudio_apbclk);
		//release the module clock
		clk_enable(hdmiaudio_moduleclk);
		
		hdmiaudioregrestore();
		
//	printk("[IIS]Entered %s\n", __func__);
		//Global Enable Digital Audio Interface
		reg_val = readl(sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		reg_val |= SUN4I_HDMIAUDIOCTL_GEN;
		writel(reg_val, sun4i_hdmiaudio.regs + SUN4I_HDMIAUDIOCTL);
		
		//printk("[HDMIAUDIO]PLL2 0x01c20008 = %#x, line = %d\n", *(volatile int*)0xF1C20008, __LINE__);
		printk("[HDMIAUDIO]SPECIAL CLK 0x01c20068 = %#x, line= %d\n", *(volatile int*)0xF1C20068, __LINE__);
		printk("[HDMIAUDIO]SPECIAL CLK 0x01c200B8 = %#x, line = %d\n", *(volatile int*)0xF1C200B8, __LINE__);
		
		return 0;
	}
//#else
//	#define sun4i_hdmiaudio_suspend NULL
//	#define sun4i_hdmiaudio_resume  NULL


#define SUN4I_I2S_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sun4i_hdmiaudio_dai_ops = {
		.trigger 		= sun4i_hdmiaudio_trigger,
		.hw_params 	= sun4i_hdmiaudio_hw_params,
		.set_fmt 		= sun4i_hdmiaudio_set_fmt,
		.set_clkdiv = sun4i_hdmiaudio_set_clkdiv,
		.set_sysclk = sun4i_hdmiaudio_set_sysclk, 
};
struct snd_soc_dai sun4i_hdmiaudio_dai = {
	.name 		= "sun4i-hdmiaudio",
	.id 			= 0,
	.probe 		= sun4i_hdmiaudio_probe,
	.suspend 	= sun4i_hdmiaudio_suspend,
	.resume 	= sun4i_hdmiaudio_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN4I_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN4I_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,},
	.ops = &sun4i_hdmiaudio_dai_ops,
};		
EXPORT_SYMBOL_GPL(sun4i_hdmiaudio_dai);

static int __init sun4i_hdmiaudio_init(void)
{
	return snd_soc_register_dai(&sun4i_hdmiaudio_dai);
}
module_init(sun4i_hdmiaudio_init);

static void __exit sun4i_hdmiaudio_exit(void)
{	
	//release the module clock
	clk_disable(hdmiaudio_moduleclk);

	//release pllx8clk
	clk_put(hdmiaudio_pllx8);
	
	//release pll2clk
	clk_put(hdmiaudio_pll2clk);

	//release apbclk
	clk_put(hdmiaudio_apbclk);

	snd_soc_unregister_dai(&sun4i_hdmiaudio_dai);
}
module_exit(sun4i_hdmiaudio_exit);

/* Module information */
MODULE_AUTHOR("ALLWINNER");
MODULE_DESCRIPTION("sun4i hdmiaudio SoC Interface");
MODULE_LICENSE("GPL");
