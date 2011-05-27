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

#include "sun4i-pcm.h"
#include "sun4i-i2s.h"

static struct sw_dma_client sun4i_dma_client_out = {
	.name = "I2S PCM Stereo out"
};

static struct sw_dma_client sun4i_dma_client_in = {
	.name = "I2S PCM Stereo in"
};

static struct sun4i_dma_params sun4i_i2s_pcm_stereo_out = {
	.client		=	&sun4i_dma_client_out,
	.channel	=	DMACH_NIIS,
	.dma_addr 	=	SUN4I_IISBASE + SUN4I_IISTXFIFO,
	.dma_size 	=   4,               /* dma transfer 32bits */
};

static struct sun4i_dma_params sun4i_i2s_pcm_stereo_in = {
	.client		=	&sun4i_dma_client_in,
	.channel	=	DMACH_NIIS,
	.dma_addr 	=	SUN4I_IISBASE + SUN4I_IISRXFIFO,
	.dma_size 	=   4,               /* dma transfer 32bits */
};


 struct sun4i_i2s_info sun4i_i2s;

void sun4i_snd_txctrl_iis(int on)
{
//	u32 i;
	u32 reg_val;
//	printk("[IIS]Entered %s\n", __func__);
//	printk("[IIS]in function %s, on = %d, line = %d\n", __func__, on, __LINE__);
	
		//flush TX FIFO
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISFCTL);
		reg_val |= SUN4I_IISFCTL_FTX;	
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISFCTL);
	
		//clear TX counter
		writel(0, sun4i_i2s.regs + SUN4I_IISTXCNT);

	if(on){
		/* IIS TX ENABLE */
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val |= SUN4I_IISCTL_TXEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
	//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
			
		/* enable DMA DRQ mode for play */
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISINT);
		reg_val |= SUN4I_IISINT_TXDRQEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISINT);
	//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
		
		//Global Enable Digital Audio Interface
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val |= SUN4I_IISCTL_GEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
	//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);

	}else{
		/* IIS TX DISABLE */
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val &= ~SUN4I_IISCTL_TXEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
	//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
			
		/* DISBALE dma DRQ mode */
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISINT);
		reg_val &= ~SUN4I_IISINT_TXDRQEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISINT);
	//		printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
			
		//Global disable Digital Audio Interface
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val &= ~SUN4I_IISCTL_GEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
	//		printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
	}
		
}

void sun4i_snd_rxctrl_iis(int on)
{
	u32 reg_val;
	
	//	printk("[IIS]Entered %s\n", __func__);
	
	//flush RX FIFO
	reg_val = readl(sun4i_i2s.regs + SUN4I_IISFCTL);
	reg_val |= SUN4I_IISFCTL_FRX;	
	writel(reg_val, sun4i_i2s.regs + SUN4I_IISFCTL);

	//clear RX counter
	writel(0, sun4i_i2s.regs + SUN4I_IISRXCNT);
	
	if(on){
		/* IIS RX ENABLE */
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val |= SUN4I_IISCTL_RXEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
			
		/* enable DMA DRQ mode for record */
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISINT);
		reg_val |= SUN4I_IISINT_RXDRQEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISINT);
			
		//Global Enable Digital Audio Interface
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val |= SUN4I_IISCTL_GEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
			
	}else{
		/* IIS RX DISABLE */
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val &= ~SUN4I_IISCTL_RXEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
			
		/* DISBALE dma DRQ mode */
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISINT);
		reg_val &= ~SUN4I_IISINT_RXDRQEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISINT);
				
		//Global disable Digital Audio Interface
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val &= ~SUN4I_IISCTL_GEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
	}
		
}

static inline int sun4i_snd_is_clkmaster(void)
{
	return ((readl(sun4i_i2s.regs + SUN4I_IISCTL) & SUN4I_IISCTL_MS) ? 0 : 1);
}

static int sun4i_i2s_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val;
	u32 reg_val1;

	//	printk("[IIS]Entered %s\n", __func__);
	//SDO ON
	reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
	reg_val |= (SUN4I_IISCTL_SDO0EN | SUN4I_IISCTL_SDO1EN | SUN4I_IISCTL_SDO2EN | SUN4I_IISCTL_SDO3EN); 
	writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);

	/* master or slave selection */
	reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master */
			reg_val |= SUN4I_IISCTL_MS;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave */
			reg_val &= ~SUN4I_IISCTL_MS;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
	
	
	/* pcm or i2s mode selection */
	reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
	reg_val1 = readl(sun4i_i2s.regs + SUN4I_IISFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val1, __LINE__);
	reg_val1 &= ~SUN4I_IISFAT0_FMT_RVD;
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK){
		case SND_SOC_DAIFMT_I2S:        /* I2S mode */
			reg_val &= ~SUN4I_IISCTL_PCM;
			reg_val1 |= SUN4I_IISFAT0_FMT_I2S;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val &= ~SUN4I_IISCTL_PCM;
			reg_val1 |= SUN4I_IISFAT0_FMT_RGT;
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val &= ~SUN4I_IISCTL_PCM;
			reg_val1 |= SUN4I_IISFAT0_FMT_LFT;
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
			reg_val |= SUN4I_IISCTL_PCM;
			reg_val1 &= ~SUN4I_IISFAT0_LRCP;
			break;
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			reg_val |= SUN4I_IISCTL_PCM;
			reg_val1 |= SUN4I_IISFAT0_LRCP;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
	writel(reg_val1, sun4i_i2s.regs + SUN4I_IISFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val1, __LINE__);
	
	/* DAI signal inversions */
	reg_val1 = readl(sun4i_i2s.regs + SUN4I_IISFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val1, __LINE__);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
			reg_val1 &= ~SUN4I_IISFAT0_LRCP;
			reg_val1 &= ~SUN4I_IISFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val1 |= SUN4I_IISFAT0_LRCP;
			reg_val1 &= ~SUN4I_IISFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val1 &= ~SUN4I_IISFAT0_LRCP;
			reg_val1 |= SUN4I_IISFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
			reg_val1 |= SUN4I_IISFAT0_LRCP;
			reg_val1 |= SUN4I_IISFAT0_BCP;
			break;
	}
	writel(reg_val1, sun4i_i2s.regs + SUN4I_IISFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val1, __LINE__);
	
	/* word select size */
	reg_val = readl(sun4i_i2s.regs + SUN4I_IISFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
	reg_val &= ~SUN4I_IISFAT0_WSS_32BCLK;
	if(sun4i_i2s.ws_size == 16)
		reg_val |= SUN4I_IISFAT0_WSS_16BCLK;
	else if(sun4i_i2s.ws_size == 20) 
		reg_val |= SUN4I_IISFAT0_WSS_20BCLK;
	else if(sun4i_i2s.ws_size == 24)
		reg_val |= SUN4I_IISFAT0_WSS_24BCLK;
	else
		reg_val |= SUN4I_IISFAT0_WSS_32BCLK;
	writel(reg_val, sun4i_i2s.regs + SUN4I_IISFAT0);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);

	/* PCM REGISTER setup */
	reg_val = sun4i_i2s.pcm_txtype&0x3;
	reg_val |= sun4i_i2s.pcm_rxtype<<2;
	
	if(!sun4i_i2s.pcm_sync_type)
		reg_val |= SUN4I_IISFAT1_SSYNC;							//short sync		
	if(sun4i_i2s.pcm_sw == 16)
		reg_val |= SUN4I_IISFAT1_SW;
			
	reg_val |=((sun4i_i2s.pcm_start_slot - 1)&0x3)<<6;		//start slot index
		
	reg_val |= sun4i_i2s.pcm_lsb_first<<9;			//MSB or LSB first
		
	if(sun4i_i2s.pcm_sync_period == 256)
		reg_val |= 0x4<<12;
	else if (sun4i_i2s.pcm_sync_period == 128)
		reg_val |= 0x3<<12;
	else if (sun4i_i2s.pcm_sync_period == 64)
		reg_val |= 0x2<<12;
	else if (sun4i_i2s.pcm_sync_period == 32)
		reg_val |= 0x1<<12;
	writel(reg_val, sun4i_i2s.regs + SUN4I_IISFAT1);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
	
	/* set FIFO control register */
	reg_val = 1 & 0x3;
	reg_val |= (1 & 0x1)<<2;
	reg_val |= SUN4I_IISFCTL_RXTL(0xf);				//RX FIFO trigger level
	reg_val |= SUN4I_IISFCTL_TXTL(0x40);				//TX FIFO empty trigger level
	writel(reg_val, sun4i_i2s.regs + SUN4I_IISFCTL);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.regs, reg_val, __LINE__);
	return 0;
}

static int sun4i_i2s_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun4i_dma_params *dma_data;
//	printk("[IIS]Entered %s\n", __func__);
	
	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sun4i_i2s_pcm_stereo_out;
	else
		dma_data = &sun4i_i2s_pcm_stereo_in;

	snd_soc_dai_set_dma_data(rtd->dai->cpu_dai, substream, dma_data);
	
	return 0;
}

static int sun4i_i2s_trigger(struct snd_pcm_substream *substream,
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
					sun4i_snd_rxctrl_iis(1);
				}
			else
				{
					sun4i_snd_txctrl_iis(1);
				}
			sw_dma_ctrl(dma_data->channel, SW_DMAOP_STARTED);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			{
				sun4i_snd_rxctrl_iis(0);
			}
			else
			{
			  sun4i_snd_txctrl_iis(0);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

		return ret;
}

//freq:   1: 22.5792MHz   0: 24.576MHz  
static int sun4i_i2s_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id, 
                                 unsigned int freq, int dir)
{
	u32 reg_val = 0;
//	printk("[IIS]Entered %s\, the MCLK freq = %d\n", __func__, freq);
	if (!freq)
	{
		reg_val = readl(sun4i_i2s.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
		reg_val &= ~(1<<26);
		reg_val |= SUN4I_CCM_AUDIO_HOSC_PLL_REG_FRE24576MHZ;
		writel(reg_val, sun4i_i2s.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
	}	
	else
	{
		reg_val = readl(sun4i_i2s.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
		reg_val &= ~(1<<26);
		reg_val |= SUN4I_CCM_AUDIO_HOSC_PLL_REG_FRE225792MHZ;
		writel(reg_val, sun4i_i2s.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
	}

	return 0;
}

static int sun4i_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int div)
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
		reg = (readl(sun4i_i2s.regs + SUN4I_IISCLKD) & ~SUN4I_IISCLKD_MCLK_MASK) | (div << SUN4I_IISCLKD_MCLK_OFFS);
		writel(reg, sun4i_i2s.regs + SUN4I_IISCLKD);
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
		reg = (readl(sun4i_i2s.regs + SUN4I_IISCLKD) & ~SUN4I_IISCLKD_BCLK_MASK) | (div <<SUN4I_IISCLKD_BCLK_OFFS);
		writel(reg, sun4i_i2s.regs + SUN4I_IISCLKD);
		break;
	default:
		return -EINVAL;
	}
	
	//diable MCLK output when high samplerate
	reg = readl(sun4i_i2s.regs + SUN4I_IISCLKD);
	if(!(reg & 0xF))
	{
		reg &= ~SUN4I_IISCLKD_MCLKOEN;
		writel(reg, sun4i_i2s.regs + SUN4I_IISCLKD);
	}
	else
	{
		reg |= SUN4I_IISCLKD_MCLKOEN;
		writel(reg, sun4i_i2s.regs + SUN4I_IISCLKD);
	}
	
	return 0;
}

u32 sun4i_i2s_get_clockrate(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(sun4i_i2s_get_clockrate);

#define VA_I2S_GPIO_CFG0_BASE  (0xF1C20800 + 0x24)
#define VA_I2S_GPIO_CFG1_BASE  (0xF1C20800 + 0x28)


extern int gpio_request_(unsigned gpio, const char *label);


static int sun4i_i2s_probe(struct platform_device *pdev, struct snd_soc_dai *dai)
{
//		int ret;
		int reg_val = 0;
//	printk("[IIS]Entered %s\n", __func__);

	sun4i_i2s.regs = ioremap(SUN4I_IISBASE, 0x100);
	if (sun4i_i2s.regs == NULL)
		return -ENXIO;
		
	sun4i_i2s.ccmregs = ioremap(SUN4I_CCMBASE, 0x100);
	if (sun4i_i2s.ccmregs == NULL)
		return -ENXIO;
		
	sun4i_i2s.ioregs = ioremap(0x01C20800, 0x100);
	if (sun4i_i2s.ioregs == NULL)
		return -ENXIO;
	
	//audio pll enable
	reg_val = readl(sun4i_i2s.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.ccmregs, reg_val, __LINE__);
	reg_val |= SUN4I_CCM_AUDIO_HOSC_PLL_REG_AUDIOEN;
	writel(reg_val, sun4i_i2s.ccmregs + SUN4I_CCM_AUDIO_HOSC_PLL_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.ccmregs, reg_val, __LINE__);
	
	//IIS APB gating
	reg_val = readl(sun4i_i2s.ccmregs + SUN4I_CCM_APB_GATE_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.ccmregs, reg_val, __LINE__);
	reg_val |= SUN4I_CCM_APB_GATE_REG_IISGATE;
	writel(reg_val, sun4i_i2s.ccmregs + SUN4I_CCM_APB_GATE_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.ccmregs, reg_val, __LINE__);
	
	//iis special clock gating
	reg_val = readl(sun4i_i2s.ccmregs + SUN4I_CCM_AUDIO_CLK_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.ccmregs, reg_val, __LINE__);
	reg_val |= SUN4I_CCM_AUDIO_CLK_REG_IISSPECIALGATE;
	writel(reg_val, sun4i_i2s.ccmregs + SUN4I_CCM_AUDIO_CLK_REG);
//	printk("[IIS]in function %s, base addr = %x, reg_val = %x, line = %d\n", __func__, sun4i_i2s.ccmregs, reg_val, __LINE__);
	
	reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
	reg_val |= SUN4I_IISCTL_GEN;
	writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);

/* for ic
	// i2s gpio setting 
	reg_val = readl(sun4i_i2s.ioregs + 0x24);
	reg_val &= ~0xFFF00000;
	reg_val |= 0x22200000;
	writel(reg_val, sun4i_i2s.ioregs + 0x24); 
	
	reg_val = readl(sun4i_i2s.ioregs + 0x28);
	reg_val &= ~0x000FFFFF;
	reg_val |= 0x00022222;
	writel(reg_val, sun4i_i2s.ioregs + 0x28); 
*/
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
    	
	sun4i_snd_txctrl_iis(0);
	sun4i_snd_rxctrl_iis(0);
	
	iounmap(sun4i_i2s.ioregs);
	
	return 0;
}



#ifdef CONFIG_PM
	static int sun4i_i2s_suspend(struct snd_soc_dai *cpu_dai)
	{
		u32 reg_val;
//	printk("[IIS]Entered %s\n", __func__);

		//Global Enable Digital Audio Interface
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val &= ~SUN4I_IISCTL_GEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
		
		return 0;
	}
	static int sun4i_i2s_resume(struct snd_soc_dai *cpu_dai)
	{
		u32 reg_val;
//	printk("[IIS]Entered %s\n", __func__);
		//Global Enable Digital Audio Interface
		reg_val = readl(sun4i_i2s.regs + SUN4I_IISCTL);
		reg_val |= SUN4I_IISCTL_GEN;
		writel(reg_val, sun4i_i2s.regs + SUN4I_IISCTL);
		
		return 0;
	}
#else
	#define sun4i_i2s_suspend NULL
	#define sun4i_i2s_resume  NULL
#endif

#define SUN4I_I2S_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sun4i_i2s_dai_ops = {
		.trigger 		= sun4i_i2s_trigger,
		.hw_params 	= sun4i_i2s_hw_params,
		.set_fmt 		= sun4i_i2s_set_fmt,
		.set_clkdiv = sun4i_i2s_set_clkdiv,
		.set_sysclk = sun4i_i2s_set_sysclk, 
};
struct snd_soc_dai sun4i_i2s_dai = {
	.name 		= "sun4i-i2s",
	.id 			= 0,
	.probe 		= sun4i_i2s_probe,
	.suspend 	= sun4i_i2s_suspend,
	.resume 	= sun4i_i2s_resume,
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
	.ops = &sun4i_i2s_dai_ops,
};		
EXPORT_SYMBOL_GPL(sun4i_i2s_dai);

static int __init sun4i_i2s_init(void)
{
	return snd_soc_register_dai(&sun4i_i2s_dai);
}
module_init(sun4i_i2s_init);

static void __exit sun4i_i2s_exit(void)
{
	snd_soc_unregister_dai(&sun4i_i2s_dai);
}
module_exit(sun4i_i2s_exit);

/* Module information */
MODULE_AUTHOR("ALLWINNER");
MODULE_DESCRIPTION("sun4i I2S SoC Interface");
MODULE_LICENSE("GPL");
