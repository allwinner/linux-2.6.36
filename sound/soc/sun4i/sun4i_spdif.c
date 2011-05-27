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

#include "sun4i_spdma.h"
#include "sun4i_spdif.h"

static struct sw_dma_client sun4i_dma_client_out = {
	.name = "SPDIF out"
};

static struct sw_dma_client sun4i_dma_client_in = {
	.name = "SPDIF in"
};

static struct sun4i_dma_params sun4i_spdif_stereo_out = {
	.client		=	&sun4i_dma_client_out,
	.channel	=	DMACH_NSPDIF,
	.dma_addr 	=	SUN4I_SPDIFBASE + SUN4I_SPDIF_TXFIFO,
	.dma_size 	=   4,               /* dma transfer 32bits */
};

static struct sun4i_dma_params sun4i_spdif_stereo_in = {
	.client		=	&sun4i_dma_client_in,
	.channel	=	DMACH_NSPDIF,
	.dma_addr 	=	SUN4I_SPDIFBASE + SUN4I_SPDIF_RXFIFO,
	.dma_size 	=   4,               /* dma transfer 32bits */
};

struct sun4i_spdif_info sun4i_spdif;

void sun4i_snd_txctrl(int on)
{
	u32 reg_val;
	
		//soft reset SPDIF
		writel(0x1, sun4i_spdif.regs + SUN4I_SPDIF_CTL);
	
		//MCLK OUTPUT enable
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		reg_val |= SUN4I_SPDIF_CTL_MCLKOUTEN;
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		
		//flush TX FIFO
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_FCTL);
		reg_val |= SUN4I_SPDIF_FCTL_FTX;	
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_FCTL);
	
		//clear interrupt status
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_ISTA);
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_ISTA);
		
		//clear TX counter
		writel(0, sun4i_spdif.regs + SUN4I_SPDIF_TXCNT);
		
	if(on){
		//SPDIF TX ENBALE
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCFG);
		reg_val |= SUN4I_SPDIF_TXCFG_TXEN;	
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCFG);
		
		//DRQ ENABLE
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_INT);
		reg_val |= SUN4I_SPDIF_INT_TXDRQEN;	
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_INT);
		
		//global enable
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		reg_val |= SUN4I_SPDIF_CTL_GEN;	
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		
	}else{
		//SPDIF TX DISABALE
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCFG);
		reg_val &= ~SUN4I_SPDIF_TXCFG_TXEN;	
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCFG);
		
		//DRQ DISABLE
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_INT);
		reg_val &= ~SUN4I_SPDIF_INT_TXDRQEN;	
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_INT);
		
		//global disable
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		reg_val &= ~SUN4I_SPDIF_CTL_GEN;	
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_CTL);
	}
}

void sun4i_snd_rxctrl(int on)
{
}

static inline int sun4i_snd_is_clkmaster(void)
{
	return 0;
}

static int sun4i_spdif_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val;
		
	reg_val = 0;
	reg_val &= ~SUN4I_SPDIF_TXCFG_SINGLEMOD;
	reg_val |= SUN4I_SPDIF_TXCFG_ASS;
	reg_val &= ~SUN4I_SPDIF_TXCFG_NONAUDIO;
//	reg_val |= SUN4I_SPDIF_TXCFG_NONAUDIO;
	reg_val |= SUN4I_SPDIF_TXCFG_FMT16BIT;
	reg_val |= SUN4I_SPDIF_TXCFG_CHSTMODE;
//	reg_val &= ~SUN4I_SPDIF_TXCFG_CHSTMODE;
	writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCFG);
	
	reg_val = 0;
	reg_val &= ~SUN4I_SPDIF_FCTL_FIFOSRC;
	reg_val |= SUN4I_SPDIF_FCTL_TXTL(16);
	reg_val |= SUN4I_SPDIF_FCTL_RXTL(15);
	reg_val |= SUN4I_SPDIF_FCTL_TXIM(1);
	reg_val |= SUN4I_SPDIF_FCTL_RXOM(3);
	writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_FCTL);
	
	if(!fmt) //PCM
	{
		reg_val = 0;
		reg_val |= (SUN4I_SPDIF_TXCHSTA0_CHNUM(2));
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
		
		reg_val = 0;
		reg_val |= (SUN4I_SPDIF_TXCHSTA1_SAMWORDLEN(1));
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
	}else  //non PCM
	{
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCFG);
		reg_val |= SUN4I_SPDIF_TXCFG_NONAUDIO;
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCFG);
		
		reg_val = 0;
		reg_val |= (SUN4I_SPDIF_TXCHSTA0_CHNUM(2));
		reg_val |= SUN4I_SPDIF_TXCHSTA0_AUDIO;
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
		
		reg_val = 0;
		reg_val |= (SUN4I_SPDIF_TXCHSTA1_SAMWORDLEN(1));
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
	}
	
	return 0;
}

static int sun4i_spdif_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun4i_dma_params *dma_data;
	
	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sun4i_spdif_stereo_out;
	else
		dma_data = &sun4i_spdif_stereo_in;
		
	snd_soc_dai_set_dma_data(rtd->dai->cpu_dai, substream, dma_data);
	
	return 0;
}						

static int sun4i_spdif_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun4i_dma_params *dma_data = 
					snd_soc_dai_get_dma_data(rtd->dai->cpu_dai, substream);

//	printk("[SPDIF]Entered %s\n", __func__);
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
				{
					sun4i_snd_rxctrl(1);
				}
			else
				{
					sun4i_snd_txctrl(1);
				}
			sw_dma_ctrl(dma_data->channel, SW_DMAOP_STARTED);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			{
				sun4i_snd_rxctrl(0);
			}
			else
			{
			  sun4i_snd_txctrl(0);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

		return ret;
}					

//freq:   1: 22.5792MHz   0: 24.576MHz  
static int sun4i_spdif_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id, 
                                 unsigned int freq, int dir)
{
	u32 reg_val = 0;
//	printk("[SPDIF]Entered %s\, the MCLK freq = %d\n", __func__, freq);
	if (!freq)
	{
		reg_val = readl(sun4i_spdif.ccmregs + SUN4I_CCMBASE_AUDIOHOSCPLL);
		reg_val &= ~(1<<26);
		reg_val |= SUN4I_CCMBASE_AUDIOHOSCPLL_24576M;
		writel(reg_val, sun4i_spdif.ccmregs + SUN4I_CCMBASE_AUDIOHOSCPLL);
	}	
	else
	{
		reg_val = readl(sun4i_spdif.ccmregs + SUN4I_CCMBASE_AUDIOHOSCPLL);
		reg_val &= ~(1<<26);
		reg_val |= SUN4I_CCMBASE_AUDIOHOSCPLL_225792M;
		writel(reg_val, sun4i_spdif.ccmregs + SUN4I_CCMBASE_AUDIOHOSCPLL);
	}

	return 0;
}		

static int sun4i_spdif_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int div)
{
	u32 reg_val = 0;

	reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
	reg_val &= ~(SUN4I_SPDIF_TXCHSTA0_SAMFREQ(0xf));
	writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
		
	reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
	reg_val &= ~(SUN4I_SPDIF_TXCHSTA1_ORISAMFREQ(0xf));
  writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);	
	
			switch(div_id){
		case SUN4I_DIV_MCLK:
			{
				reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCFG);
				reg_val &= ~SUN4I_SPDIF_TXCFG_TXRATIO(0x1F);	
				reg_val |= SUN4I_SPDIF_TXCFG_TXRATIO(div-1);
				writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCFG);
				
		//		printk("[SPDIF]div = %d\n", div);
			if(readl(sun4i_spdif.ccmregs + SUN4I_CCMBASE_AUDIOHOSCPLL) & SUN4I_CCMBASE_AUDIOHOSCPLL_24576M) //24.576MHz
			{
				
				switch(div)
				{
					//32KHZ
					case 24:
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						reg_val |= (SUN4I_SPDIF_TXCHSTA0_SAMFREQ(0x3));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						reg_val |= (SUN4I_SPDIF_TXCHSTA1_ORISAMFREQ(0xC));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						break;
						
					//48KHZ
					case 16:
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						reg_val |= (SUN4I_SPDIF_TXCHSTA0_SAMFREQ(0x2));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
				
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						reg_val |= (SUN4I_SPDIF_TXCHSTA1_ORISAMFREQ(0xD));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);				
						break;
						
					//96KHZ
					case 8:
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						reg_val |= (SUN4I_SPDIF_TXCHSTA0_SAMFREQ(0xA));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						reg_val |= (SUN4I_SPDIF_TXCHSTA1_ORISAMFREQ(0x5));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						break;
						
					//192KHZ
					case 4:
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						reg_val |= (SUN4I_SPDIF_TXCHSTA0_SAMFREQ(0xE));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
					
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);	
						reg_val |= (SUN4I_SPDIF_TXCHSTA1_ORISAMFREQ(0x1));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						break;
						
					default:
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						reg_val |= (SUN4I_SPDIF_TXCHSTA0_SAMFREQ(1));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
				
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						reg_val |= (SUN4I_SPDIF_TXCHSTA1_ORISAMFREQ(0));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						break;
				}
			}else  //22.5792MHz
			{
				switch(div)
				{
					//44.1KHZ
					case 16:
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						reg_val |= (SUN4I_SPDIF_TXCHSTA0_SAMFREQ(0x0));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
				
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						reg_val |= (SUN4I_SPDIF_TXCHSTA1_ORISAMFREQ(0xF));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);				
						break;
			
					//176.4KHZ
					case 4:
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						reg_val |= (SUN4I_SPDIF_TXCHSTA0_SAMFREQ(0xC));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
					
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						reg_val |= (SUN4I_SPDIF_TXCHSTA1_ORISAMFREQ(0x3));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						break;
						
					default:
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
						reg_val |= (SUN4I_SPDIF_TXCHSTA0_SAMFREQ(1));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA0);
				
						reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						reg_val |= (SUN4I_SPDIF_TXCHSTA1_ORISAMFREQ(0));
						writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_TXCHSTA1);
						break;
					}
			}
				
				
			}
			  break;
		case SUN4I_DIV_BCLK:
				break;
			
		default:
			return -EINVAL;
	}

	
	
		
	return 0;
}

u32 sun4i_spdif_get_clockrate(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(sun4i_spdif_get_clockrate);

static int sun4i_spdif_probe(struct platform_device *pdev, struct snd_soc_dai *dai)
{
		int reg_val = 0;
		
		sun4i_spdif.regs = ioremap(SUN4I_SPDIFBASE, 0x100);
		if(sun4i_spdif.regs == NULL)
			return -ENXIO;
			
		sun4i_spdif.ccmregs = ioremap(SUN4I_CCMBASE, 0x100);
		if(sun4i_spdif.ccmregs == NULL)
			return -ENXIO;
			
		sun4i_spdif.ioregs = ioremap(0x01c20800, 0x100);
		if(sun4i_spdif.ioregs == NULL)
			return -ENXIO;
			
		//audio pll enalble
		reg_val = readl(sun4i_spdif.ccmregs + SUN4I_CCMBASE_AUDIOHOSCPLL);
		reg_val |= SUN4I_CCMBASE_AUDIOHOSCPLL_EN;
		writel(reg_val, sun4i_spdif.ccmregs + SUN4I_CCMBASE_AUDIOHOSCPLL);
		
		//spdif apb gating
		reg_val = readl(sun4i_spdif.ccmregs + SUN4I_CCMBASE_APBGATE);
		reg_val |= SUN4I_CCMBASE_APBGATE_SPDIFGATE;
		writel(reg_val, sun4i_spdif.ccmregs + SUN4I_CCMBASE_APBGATE);
		
		//SPDIF SPECAIL GATING
		reg_val = readl(sun4i_spdif.ccmregs + SUN4I_CCMBASE_AUDIOCLK);
		reg_val |= SUN4I_CCMBASE_AUDIOCLK_SPDIFSPEGATE;
		writel(reg_val, sun4i_spdif.ccmregs + SUN4I_CCMBASE_AUDIOCLK);
		
		//global enbale
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		reg_val |= SUN4I_SPDIF_CTL_GEN;
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		
		//spdif gpio setting
		//for IC
		/*
			reg_val = readl(sun4i_spdif.ioregs + 0x24);
			reg_val &= ~0x0000F000;
			reg_val |= 0x00004000;
			writel(reg_val, sun4i_spdif.ioregs + 0x24); 
			
			reg_val = readl(sun4i_spdif.ioregs + 0x28);
			reg_val &= ~0x00FF0000;
			reg_val |= 0x00440000;
			writel(reg_val, sun4i_spdif.ioregs + 0x28); 
		*/
		//FOR FPGA
			reg_val = readl(sun4i_spdif.ioregs + 0x4C);
			reg_val &= ~0x00000FFF;
			reg_val |= 0x00000444;
			writel(reg_val, sun4i_spdif.ioregs + 0x4C); 

			sun4i_snd_txctrl(0);
			sun4i_snd_rxctrl(0);
			
			iounmap(sun4i_spdif.ioregs);
			
	return 0;
}

#ifdef CONFIG_PM
	static int sun4i_spdif_suspend(struct snd_soc_dai *cpu_dai)
	{
		u32 reg_val;
		
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		reg_val &= ~SUN4I_SPDIF_CTL_GEN;
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		
		return 0;
	}
	static int sun4i_spdif_resume(struct snd_soc_dai *cpu_dai)
	{
		u32 reg_val;
		
		reg_val = readl(sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		reg_val |= SUN4I_SPDIF_CTL_GEN;
		writel(reg_val, sun4i_spdif.regs + SUN4I_SPDIF_CTL);
		
		return 0;
	}
#else
	#define sun4i_spdif_suspend NULL
	#define sun4i_spdif_resume  NULL
#endif

#define SUN4I_SPDIF_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sun4i_spdif_dai_ops = {
		.trigger 		= sun4i_spdif_trigger,
		.hw_params 	= sun4i_spdif_hw_params,
		.set_fmt 		= sun4i_spdif_set_fmt,
		.set_clkdiv = sun4i_spdif_set_clkdiv,
		.set_sysclk = sun4i_spdif_set_sysclk, 
};
struct snd_soc_dai sun4i_spdif_dai = {
	.name 		= "sun4i-spdif",
	.id 			= 0,
	.probe 		= sun4i_spdif_probe,
	.suspend 	= sun4i_spdif_suspend,
	.resume 	= sun4i_spdif_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN4I_SPDIF_RATES,
	.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN4I_SPDIF_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &sun4i_spdif_dai_ops,
};		
EXPORT_SYMBOL_GPL(sun4i_spdif_dai);

static int __init sun4i_spdif_init(void)
{
	return snd_soc_register_dai(&sun4i_spdif_dai);
}
module_init(sun4i_spdif_init);

static void __exit sun4i_spdif_exit(void)
{
	snd_soc_unregister_dai(&sun4i_spdif_dai);
}
module_exit(sun4i_spdif_exit);

/* Module information */
MODULE_AUTHOR("ALLWINNER");
MODULE_DESCRIPTION("sun4i SPDIF SoC Interface");
MODULE_LICENSE("GPL");
