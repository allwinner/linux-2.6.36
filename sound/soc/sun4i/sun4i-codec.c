/*
 *  Driver for CODEC on M1 soundcard
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License.
 * 
* 
***************************************************************************************************/
//#define DEBUG 0
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <mach/dma.h>

#define Debug_Level0
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif
#include <asm/mach-types.h>
//#include <asm/dma.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include <sound/initval.h>
#include "sun4i-codec.h"

static int flag_id = 0;
/*
 * these are the address and sizes used to fill the xmit buffer
 * so we can get a clock in record only mode
 */
#define FORCE_CLOCK_ADDR		(dma_addr_t)FLUSH_BASE_PHYS
#define FORCE_CLOCK_SIZE		4096 // was 2048
#define SW_DAC_FIFO         0x01c23c04
#define SW_ADC_FIFO         0x01c23c20


//CCM register
#define CCM_BASE    0xf1c20000

#define CCM_AC320_MACC_REG 			(0x00)
#define CCM_AUDIO_HOSC_REG 			(0x04)
#define CCM_AHB_APB_REG    			(0x08)
#define CCM_AHB_GATING_REG 			(0x0c)
#define CCM_APB_GATING_REG 			(0x10)
#define CCM_NFC_MS_REG     			(0x14)
#define CCM_SD01_REG       			(0x18)
#define CCM_SD23_REG			 	(0x1c)
#define CCM_DRAM_PLL_REG   			(0x20)
#define CCM_DE_REG         			(0x24)
#define CCM_LCD_MACC_REG   			(0x28)
#define CCM_TV_CSI_REG     			(0x2c)
#define CCM_VIDEO_PLL_REG  			(0x30)
#define CCM_IR_CLK_REG     			(0x34)
#define CCM_AUDIO_CLK_REG  			(0x38)
#define CCM_TS_CLK_REG   			  (0x3c)
#define CCM_AVS_USB_CLK_REG 		(0x40)
#define CCM_PID_CLK_REG       	(0xd0)
#define CCM_WAKEUP_PENDING_REG	(0xd4)

#define codec_RATES SNDRV_PCM_RATE_8000_48000
#define codec_FORMATS (SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_S16_LE |\
        SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_S16_LE )


/*In-data addresses are hard-coded into the reg-cache values*/
static const char codec_reg[SW_CODEC_REGS_NUM] = {
	0x00,0x04,0x0c,0x10,0x14,0x18,0x1c,
	0x20,0x24,0x28,0x2c,0x30,0x34
};
enum dma_ch {
	CODEC_PLAY,
	CODEC_CAPTURE
};

static const char *micinput_names[] = {
	"micmuteall",
	"mic2rs",
	"mic2ls",
	"mic2lrs",
	"mic1rs",
	"mic1rs_mic2rs",
	"mic1rs_mic2ls",
	"mic1rs_mic2lrs",
	"mic1ls",
	"mic1ls_mic2rs",
	"mic1ls_mic2ls",
	"mic1ls_mic2lrs",
	"mic1lrs",
	"mic1lrs_mic2rs",
	"mic1lrs_mic2ls",
	"mic1lrs_mic2lrs",
};

static const char *adcinput_names[] = {
	"linelr",
	"fmlr",
	"mic1",
	"mic2",
	"mic12",
	"micdiff",
	"mixlr",
	"null",
};


static const int codec_enum_items[] = {
	16, //mic input - 
	8,  //adc input -
};

static const char ** codec_enum_names[] = {
	micinput_names,   //
	adcinput_names,  //
};

static volatile unsigned int dmasrc = 0;
static volatile unsigned int dmadst = 0;
static volatile unsigned int pst_src = 0;
static volatile unsigned int dma_flg = 0;




/* Structure/enum declaration ------------------------------- */
typedef struct codec_board_info {
	
	void __iomem	*codec_vbase;	/* mac I/O base address */
	void __iomem	*ccmu_vbase;	/* ccmu I/O base address */
	u16		 irq;		/* IRQ */
	
	
	unsigned int	flags;
	unsigned int	in_suspend :1;
	int		debug_level;
	
	struct device	*dev;	     /* parent device */
	
	struct resource	*codec_base_res;   /* resources found */
	struct resource	*ccmu_base_res;   /* resources found */
	
	struct resource	*codec_base_req;   /* resources found */
	struct resource	*ccmu_base_req;   /* resources found */
	
	spinlock_t	lock;
} codec_board_info_t;



//static char * id = "sun4i-CODEC";

//static char *id = NULL;	/* ID for this card */
static struct sw_dma_client sw_codec_dma_client_play = {
	.name		= "CODEC PCM Stereo PLAY"
};

static struct sw_dma_client sw_codec_dma_client_capture = {
	.name		= "CODEC PCM Stereo CAPTURE"
};

static struct sw_pcm_dma_params sw_codec_pcm_stereo_play = {
	.client		= &sw_codec_dma_client_play,
	.channel	= DMACH_NADDA,
	.dma_addr	= CODEC_BASSADDRESS + SW_DAC_TXDATA,
	.dma_size	= 4,
};

static struct sw_pcm_dma_params sw_codec_pcm_stereo_capture = {
	.client		= &sw_codec_dma_client_capture,
	.channel	= DMACH_NADDA,  //only support half full
	.dma_addr	= CODEC_BASSADDRESS + SW_ADC_RXDATA,
	.dma_size	= 4,
};

struct sw_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t   dma_start;
	dma_addr_t   dma_pos;
	dma_addr_t	 dma_end;
	struct sw_pcm_dma_params	*params;
};

static struct snd_pcm_hardware sw_pcm_hardware =
{
	.info			= (SNDRV_PCM_INFO_INTERLEAVED |
				   SNDRV_PCM_INFO_BLOCK_TRANSFER |
				   SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				   SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates			= (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |SNDRV_PCM_RATE_11025 |\
				   SNDRV_PCM_RATE_22050| SNDRV_PCM_RATE_32000 |\
				   SNDRV_PCM_RATE_44100| SNDRV_PCM_RATE_48000 |\
				   SNDRV_PCM_RATE_KNOT),
	.rate_min		= 8000,
	.rate_max		= 48000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 64*1024,
	.period_bytes_min	= 1024*8,
	.period_bytes_max	= 1024*16,
	.periods_min		= 4,
	.periods_max		= 8,
	.fifo_size		= 32,
};

struct sw_codec{
	struct snd_card *card;
	struct snd_pcm *pcm;
	long samplerate;
};

static unsigned int rates[] = {
	8000,11025,12000,16000,
	22050,24000,24000,32000,
	44100,48000,
};

static struct snd_pcm_hw_constraint_list hw_constraints_rates = {
	.count	= ARRAY_SIZE(rates),
	.list	= rates,
	.mask	= 0,
};


/**
* codec_wrreg_bits - update codec register bits
* @reg: codec register
* @mask: register mask
* @value: new value
*
* Writes new register value.
* Return 1 for change else 0.
*/
int codec_wrreg_bits(unsigned short reg, unsigned int	mask,	unsigned int value)
{
	int change;
	unsigned int old, new;
	
	//mutex_lock(&io_mutex);
	old	=	codec_rdreg(reg);
	new	=	(old & ~mask) | value;
	change = old != new;

	if (change)
	{       //printk("reg = %x,reg_val = %x\n",reg,new);
		codec_wrreg(reg,new);
	}
		
	//mutex_unlock(&io_mutex);
	return change;
}



/**
*	snd_codec_info_volsw	-	single	mixer	info	callback
*	@kcontrol:	mixer control
*	@uinfo:	control	element	information
*	Callback to provide information about a single mixer control
*
*	Returns 0 for success
*/
int snd_codec_info_volsw(struct snd_kcontrol *kcontrol,
		struct	snd_ctl_elem_info	*uinfo)
{
	struct	codec_mixer_control *mc	= (struct codec_mixer_control*)kcontrol->private_value;
	int	max	=	mc->max;
	unsigned int shift  = mc->shift;
	unsigned int rshift = mc->rshift;
	
	if(max	== 1)
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	
	uinfo->count = shift ==	rshift	?	1:	2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = max;
	return	0;
}


/**
*	snd_codec_get_volsw	-	single	mixer	get	callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to get the value of a single mixer control
*
*	return 0 for success.
*/
int snd_codec_get_volsw(struct snd_kcontrol	*kcontrol,
		struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int	max = mc->max;
	unsigned int mask = (1 << fls(max)) -1;
	unsigned int invert = mc->invert;
	unsigned int reg = mc->reg;
	
	ucontrol->value.integer.value[0] =	
		(codec_rdreg(reg)>>	shift) & mask;
	if(shift != rshift)
		ucontrol->value.integer.value[1] =
			(codec_rdreg(reg) >> rshift) & mask;

	if(invert){
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];
		if(shift != rshift)
			ucontrol->value.integer.value[1] =
				max - ucontrol->value.integer.value[1];
		}
		
		return 0;
}

/**
*	snd_codec_put_volsw	-	single	mixer put callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to put the value of a single mixer control
*
* return 0 for success.
*/
int snd_codec_put_volsw(struct	snd_kcontrol	*kcontrol,
	struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1<<fls(max))-1;
	unsigned int invert = mc->invert;
	unsigned int	val, val2, val_mask;
	
	val = (ucontrol->value.integer.value[0] & mask);
	if(invert)
		val = max - val;
	val <<= shift;
	val_mask = mask << shift;
	if(shift != rshift){
		val2	= (ucontrol->value.integer.value[1] & mask);
		if(invert)
			val2	=	max	- val2;
		val_mask |= mask <<rshift;
		val |= val2 <<rshift;
	}
	pr_debug("Val = %d\n",val);
	return codec_wrreg_bits(reg,val_mask,val);
}

/**
*	snd_codec_info_enum	-	enum mixer info	callback
*	@kcontrol:	mixer control
*	@uinfo:	control	element	information
*	Callback to provide information about a enum mixer control
*
*	Returns 0 for success
*/
int snd_codec_info_enum(struct snd_kcontrol *kcontrol,
		struct	snd_ctl_elem_info	*uinfo)
{
	
	int where = kcontrol->private_value&31;
	const char **texts;

	if(!codec_enum_items[where])
		return -EINVAL;
	
	uinfo->type  = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = codec_enum_items[where];

	if(uinfo->value.enumerated.item > codec_enum_items[where])
		uinfo->value.enumerated.item = codec_enum_items[where] -1;
		
	texts = codec_enum_names[where];

	strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);

	return 0;
}



/**
*	snd_codec_get_enum	-enum mixer get	callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to get the value of a single mixer control

*	return 0 for success.
*/	
int snd_codec_get_enum(struct snd_kcontrol	*kcontrol,
		struct	snd_ctl_elem_value	*ucontrol)
{
	//int where = kcontrol->private_value & 31;
	ucontrol->value.enumerated.item[0] =  (codec_rdreg( (kcontrol->private_value>>5) & 63)>>( kcontrol->private_value>>11))&(kcontrol->private_value>>16);	
	//printk("get enum\n");
	return 0;		
}

/**
*	snd_codec_put_enum	- enum mixer put callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to put the value of a enum mixer control
*
*       return 0 for success.
*/
int snd_codec_put_enum(struct	snd_kcontrol	*kcontrol,
	struct	snd_ctl_elem_value	*ucontrol)
{
   
	int  reg_val;
        reg_val = codec_rdreg(kcontrol->private_value>>5)&(~(kcontrol->private_value>>16));
        codec_wrreg(kcontrol->private_value>>5,reg_val|ucontrol->value.enumerated.item[0]);
	return 0;
}

int codec_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	codec_wrreg_bits(reg, mask, reg_val);
	return 0;
}

int codec_rd_control(u32 reg, u32 bit, u32 *val)
{
	return 0;
}
static  void codec_clock(unsigned char comd, unsigned char clk_mode)
{
	unsigned int reg_val;
	if(clk_mode){
       reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
	   reg_val |= (clk_mode << 26) | (clk_mode << 29);
	   writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
	}
	else{
	   reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
	   reg_val &= ~(1 << 26)|~(1 << 29);
	   writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
	}

	if(comd){

    	reg_val = readl(CCM_BASE + CCM_APB_GATING_REG);
		reg_val |= (1 << 9);
		writel(reg_val, CCM_BASE + CCM_APB_GATING_REG);
		
    	reg_val = readl(CCM_BASE + CCM_AUDIO_CLK_REG);
		reg_val |= (1 << 3);
		writel(reg_val, CCM_BASE + CCM_AUDIO_CLK_REG);
	}
	else{

    	reg_val = readl(CCM_BASE + CCM_APB_GATING_REG);
		reg_val &= ~(1 << 9);
		writel(reg_val, CCM_BASE + CCM_APB_GATING_REG);
		

        reg_val = readl(CCM_BASE + CCM_AUDIO_CLK_REG);
		reg_val &= ~(1 << 3);
		writel(reg_val, CCM_BASE + CCM_AUDIO_CLK_REG);
    }
}
/**
*	codec_reset - reset the codec 
* @codec	SoC Audio Codec
* Reset the codec, set the register of codec default value
* Return 0 for success
*/
static  int codec_init(void)
{
	u8   clk_open;
	clk_open = 1;
	codec_clock(clk_open,1);
	//enable dac digital 
	codec_wr_control(SW_DAC_DPC ,  0x1, DAC_EN, 0x1);  
	//set digital volume to maximum
	codec_wr_control(SW_DAC_DPC, 0x6, DIGITAL_VOL, 0x0);
	//pa mute
	codec_wr_control(SW_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	//enable PA
	codec_wr_control(SW_ADC_ACTL, 0x1, PA_ENABLE, 0x1);
	//enable headphone direct 
	codec_wr_control(SW_ADC_ACTL, 0x1, HP_DIRECT, 0x1);
	//set volume
	codec_wr_control(SW_DAC_ACTL, 0x6, VOLUME, 0x3f);
	
	return 0;
	
}

static int codec_play_open(void)
{
	//flush TX FIFO
	codec_wr_control(SW_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);
	//set TX FIFO send drq level
	codec_wr_control(SW_DAC_FIFOC ,0x4, TX_TRI_LEVEL, 0x7);
	//send last sample when dac fifo under run
	codec_wr_control(SW_DAC_FIFOC ,0x1, LAST_SE, 0x1);
	//enable dac analog
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACAEN_L, 0x1);
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACAEN_R, 0x1);
	//enable dac to pa
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACPAS, 0x1);
	return 0;
}

static int codec_capture_open(void)
{

	 //enable mic1 pa
	 codec_wr_control(SW_ADC_ACTL, 0x1, MIC1_EN, 0x1);
	 //enable mic2 pa
	 codec_wr_control(SW_ADC_ACTL, 0x1, MIC2_EN, 0x1);	 
	  //enable VMIC
	 codec_wr_control(SW_ADC_ACTL, 0x1, VMIC_EN, 0x1);
	 //select mic souce
	 pr_debug("ADC SELECT \n");
	 codec_wr_control(SW_ADC_ACTL, 0x7, ADC_SELECT, 0x4);
	 //enable adc digital
	 codec_wr_control(SW_ADC_FIFOC, 0x1,ADC_DIG_EN, 0x1);
	 //set RX FIFO mode
	 codec_wr_control(SW_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	 //flush RX FIFO
	 codec_wr_control(SW_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);
	 //set RX FIFO rec drq level
	 codec_wr_control(SW_ADC_FIFOC, 0xf, RX_TRI_LEVEL, 0x7);
	 //enable adc1 analog
	 codec_wr_control(SW_ADC_ACTL, 0x3,  ADC_EN, 0x3);
	 mdelay(100);
	 return 0;
}



static int codec_play_start(void)
{
	//flush TX FIFO
	codec_wr_control(SW_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);
	//enable dac drq
	codec_wr_control(SW_DAC_FIFOC ,0x1, DAC_DRQ, 0x1);
	//pa unmute
	codec_wr_control(SW_DAC_ACTL, 0x1, PA_MUTE, 0x1);
	return 0;
}
static int codec_play_stop(void)
{
	//disable dac drq
	codec_wr_control(SW_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);
	//pa mute
	codec_wr_control(SW_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	return 0;
}
static int codec_capture_start(void)
{
	//enable adc drq
	codec_wr_control(SW_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	return 0;
}

static int codec_capture_stop(void)
{
	//disable adc drq
	codec_wr_control(SW_ADC_FIFOC ,0x1, ADC_DRQ, 0x0);
	return 0;
}

static int codec_dev_free(struct snd_device *device)
{
	return 0;
};


static const struct snd_kcontrol_new codec_snd_controls[] = {
	CODEC_SINGLE("Master Playback Volume", SW_DAC_ACTL,0,0x3f,0),
	CODEC_SINGLE("Playback Switch", SW_DAC_ACTL,6,1,0),
	CODEC_SINGLE("Capture Volume",SW_ADC_ACTL,20,7,0),
	CODEC_SINGLE("Fm Volume",SW_DAC_ACTL,23,7,0),
	CODEC_SINGLE("Line Volume",SW_DAC_ACTL,26,7,0),
	CODEC_SINGLE("MicL Volume",SW_ADC_ACTL,25,3,0),
	CODEC_SINGLE("MicR Volume",SW_ADC_ACTL,23,3,0),
	CODEC_SINGLE("FmL Switch",SW_DAC_ACTL,17,1,0),
	CODEC_SINGLE("FmR Switch",SW_DAC_ACTL,16,1,0),
	CODEC_SINGLE("LineL Switch",SW_DAC_ACTL,19,1,0),
	CODEC_SINGLE("LineR Switch",SW_DAC_ACTL,18,1,0),
	//#define CODEC_ENUM(xname, reg, where, shift, max, invert,value)
	//CODEC_ENUM("Mic Input Mux",SW_DAC_ACTL,0,9,3,0),
	//CODEC_ENUM("ADC Input Mux",SW_ADC_ACTL,1,17,7,0),
	CODEC_SINGLE("Mic Input Mux",SW_DAC_ACTL,9,15,0),
	CODEC_SINGLE("ADC Input Mux",SW_ADC_ACTL,17,7,0),

};

int __init snd_chip_codec_mixer_new(struct snd_card *card)
{
  static struct snd_device_ops ops = {
  	.dev_free	=	codec_dev_free,
  };
  unsigned char *clnt = "codec";
	int idx, err;



	for (idx = 0; idx < ARRAY_SIZE(codec_snd_controls); idx++) {
		if ((err = snd_ctl_add(card, snd_ctl_new1(&codec_snd_controls[idx],clnt))) < 0) {
			return err;
		}
	}

	if ((err = snd_device_new(card, SNDRV_DEV_CODEC, clnt, &ops)) < 0) {
		//codec_free(clnt);
		return err;
	}

	
	strcpy(card->mixername, "codec Mixer");
	       
	return 0;
}


/* sw_pcm_enqueue
 *
 * place a dma buffer onto the queue for the dma system
 * to handle.
*/
static void sw_pcm_enqueue(struct snd_pcm_substream *substream)
{
  struct sw_runtime_data *prtd = substream->runtime->private_data;
	dma_addr_t pos = prtd->dma_pos;
	int ret;

	//pr_debug("Entered %s\n", __func__);
       // pr_debug("prtd->dma_loaded: %8x\n",prtd->dma_loaded);
	//while (prtd->dma_loaded < (prtd->dma_limit)) {
	unsigned long len = prtd->dma_period;
	
	//printk("666 Enter Pcm\n");



	if ((pos + len) > prtd->dma_end) {
		len  = prtd->dma_end - pos;
		//pr_debug(KERN_DEBUG "%s: corrected dma len %ld\n",
		  //     __func__, len);
		//pr_debug("corrected dma len %ld\n",len);
	}
	//pr_debug("corrected dma len %ld\n",len);
	ret = sw_dma_enqueue(prtd->params->channel,
		substream,  __bus_to_virt(pos), len);
	pr_debug("665 pos :%x,ret = %d\n",pos,ret);
	if (ret == 0) {
		prtd->dma_loaded++;
		pos += prtd->dma_period;
		if (pos >= prtd->dma_end)
			pos = prtd->dma_start;
	 }else
	 {
		//break;
	 }
			
	//}
	pr_debug(" quit pcm quenue...\n");

	prtd->dma_pos = pos;
}



static void sw_audio_buffdone(struct sw_dma_chan *channel,
				void *dev_id, int size, enum sw_dma_buffresult result)
{
	struct snd_pcm_substream *substream = dev_id;
	struct sw_runtime_data *prtd;

	pr_debug("Entered %s\n", __func__);
	if (result == SW_RES_ABORT || result == SW_RES_ERR)
		return;
	prtd = substream->runtime->private_data;
	
	if (substream)
	{
		//printk("E\n");
		snd_pcm_period_elapsed(substream);
		pr_debug("state0  %d, %d\n",prtd->state,ST_RUNNING);
	}
	
	spin_lock(&prtd->lock);

        pr_debug("state1  %d, %d\n",prtd->state,ST_RUNNING);
	
	pr_debug("state2  %d, %d\n",prtd->state,ST_RUNNING);
	if (prtd->state & ST_RUNNING) {
		pr_debug("buffdone prtd->dma_loaded:%x\n",prtd->dma_loaded);
		prtd->dma_loaded--;

                //printk("enqueue\n");
		sw_pcm_enqueue(substream);


		
	}
	
	
  
	spin_unlock(&prtd->lock);
	

}


static snd_pcm_uframes_t snd_sw_codec_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sw_runtime_data *prtd = runtime->private_data;
	unsigned long res;
        //printk("GET POINTER\n");
	spin_lock(&prtd->lock);
	sw_dma_getcurposition(DMACH_NADDA, (dma_addr_t*)&dmasrc, (dma_addr_t*)&dmadst);
	pr_debug("dmasrc: %x\n",dmasrc);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		res = dmadst + prtd->dma_period - prtd->dma_start;
	else
	{
		res = dmasrc + prtd->dma_period - prtd->dma_start;
	}
	


	spin_unlock(&prtd->lock);

	/* we seem to be getting the odd error from the pcm library due
	 * to out-of-bounds pointers. this is maybe due to the dma engine
	 * not having loaded the new values for the channel before being
	 * callled... (todo - fix )
	 */
        
	if (res >= snd_pcm_lib_buffer_bytes(substream)) {
		if (res == snd_pcm_lib_buffer_bytes(substream))
			res = 0;
	}
        //printk("R\n");
	return bytes_to_frames(substream->runtime, res);	
}
static int sw_codec_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sw_runtime_data *prtd = runtime->private_data;
	int    stream_id = substream->stream;
	unsigned long totbytes = params_buffer_bytes(params);
        int ret;
        snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));

	pr_debug("Entered %s\n", __func__);

  
	if(prtd->params == NULL){
		
		switch(stream_id){
			case SNDRV_PCM_STREAM_PLAYBACK :
				prtd->params = &sw_codec_pcm_stereo_play;
				break;

			case SNDRV_PCM_STREAM_CAPTURE :
				prtd->params = &sw_codec_pcm_stereo_capture;
				break;

			default :
				ret = -EINVAL;
				return ret;
				break;
			}


	pr_debug("params %p, client %p, channel %d\n", prtd->params, prtd->params->client,prtd->params->channel);
	ret = sw_dma_request(prtd->params->channel,prtd->params->client,NULL);
 
	if(ret < 0){
	pr_debug(KERN_ERR "failed to get dma channel. ret == %d\n", ret);
	return ret;
	}
  }
	
	//sw_dma_set_halfdone_fn(prtd->params->channel,
				//sw_audio_buffdone);
        sw_dma_set_buffdone_fn(prtd->params->channel,
				sw_audio_buffdone);
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = totbytes;
        spin_lock_irq(&prtd->lock);
	prtd->dma_loaded = 0;
	prtd->dma_limit = runtime->hw.periods_min;
	prtd->dma_period = params_period_bytes(params);
	prtd->dma_start = runtime->dma_addr;
	pr_debug("prtd->dma_start:%x\n",prtd->dma_start);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dmasrc = prtd->dma_start;
	else
		dmadst = prtd->dma_start;
	prtd->dma_pos = prtd->dma_start;
	prtd->dma_end = prtd->dma_start + totbytes;

	spin_unlock_irq(&prtd->lock);

	return 0;


	
}

static int snd_sw_codec_hw_free(struct snd_pcm_substream *substream)
{
	struct sw_runtime_data *prtd = substream->runtime->private_data;

	pr_debug("Entered %s\n", __func__);


	/* TODO - do we need to ensure DMA flushed */
	if(prtd->params)
  	sw_dma_ctrl(prtd->params->channel, SW_DMAOP_FLUSH);
	snd_pcm_set_runtime_buffer(substream, NULL);
  

	if (prtd->params) {
		sw_dma_free(prtd->params->channel, prtd->params->client);
		prtd->params = NULL;
	}

	return 0;
}


static int snd_sw_codec_prepare(struct	snd_pcm_substream	*substream)
{
	struct sw_runtime_data *prtd = substream->runtime->private_data;
	struct dma_hw_conf *codec_dma_conf;
	int ret ;
	unsigned int reg_val;
	ret = 0;
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	{
		switch(substream->runtime->rate)
		{
			case 44100:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val &=~(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				
				break;
			case 22050:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val &=~(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 11025:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val &=~(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 48000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 32000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 24000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 16000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 12000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 8000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			default:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);		
				break;
		}
		
		switch(substream->runtime->channels)
		{
			case 1:
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val |=(1<<7);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);			
				break;
			case 2:
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			default:
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
		}
        }else
        {
		switch(substream->runtime->rate)
		{
			case 44100:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val &=~(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				
				break;
			case 22050:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val &=~(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 11025:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val &=~(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 48000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 32000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 24000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 16000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 12000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 8000:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			default:
				reg_val = readl(CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val |=(1<<26);
				writel(reg_val, CCM_BASE + CCM_AUDIO_HOSC_REG);
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);		
				break;
		}
		
		switch(substream->runtime->channels)
		{
			case 1:
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val |=(1<<7);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);			
			break;
			case 2:
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
			break;
			default:
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
			break;
		}        	
	}
	


	codec_dma_conf = kmalloc(sizeof(struct dma_hw_conf), GFP_KERNEL);
	if (!codec_dma_conf)   
	{
	   ret =  - ENOMEM;
	   pr_debug("Can't audio malloc dma configure memory\n");
	   return ret;
	}
	pr_debug("Entered %s\n", __func__);

	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if (!prtd->params)
		return 0;

   if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
   	 //open the dac channel register
   	codec_play_open();    
  	codec_dma_conf->drqsrc_type  = D_DRQSRC_SDRAM;
	codec_dma_conf->drqdst_type  = DRQ_TYPE_AUDIO;
	codec_dma_conf->xfer_type    = DMAXFER_D_BHALF_S_BHALF;
	codec_dma_conf->address_type = DMAADDRT_D_FIX_S_INC;
	codec_dma_conf->dir          = SW_DMA_WDEV;
	codec_dma_conf->reload       = 0;
	codec_dma_conf->hf_irq       = SW_DMA_IRQ_FULL;
	codec_dma_conf->from         = prtd->dma_start;
	codec_dma_conf->to           = prtd->params->dma_addr;
	pr_debug("Set DMA parameter %8x, %8x\n",prtd->dma_start,prtd->params->dma_addr);
  	ret = sw_dma_config(prtd->params->channel,codec_dma_conf);
  	pr_debug("codec_dma_conf->reload: %d\n",codec_dma_conf->reload);

	}
   else {
   	//open the adc channel register
   	codec_capture_open();
   	//set the dma
   	pr_debug("dma start address : %8x\n",prtd->dma_start);
   	codec_dma_conf->drqsrc_type  = DRQ_TYPE_AUDIO;
	codec_dma_conf->drqdst_type  = D_DRQSRC_SDRAM;
	codec_dma_conf->xfer_type    = DMAXFER_D_BHALF_S_BHALF;
	codec_dma_conf->address_type = DMAADDRT_D_INC_S_FIX;
	codec_dma_conf->dir          = SW_DMA_RDEV;
	codec_dma_conf->reload       = 0;
	codec_dma_conf->hf_irq       = SW_DMA_IRQ_FULL;
	codec_dma_conf->from         = prtd->params->dma_addr;
	codec_dma_conf->to           = prtd->dma_start;
  	sw_dma_config(prtd->params->channel,codec_dma_conf);
  	//sw_dma_setflags(prtd->params->channel,SW_DMAF_AUTOSTART);
   	 
   	}
	/* flush the DMA channel */
	sw_dma_ctrl(prtd->params->channel, SW_DMAOP_FLUSH);
	prtd->dma_loaded = 0;
	prtd->dma_pos = prtd->dma_start;

	/* enqueue dma buffers */
	sw_pcm_enqueue(substream);

	return ret;	
}



static int snd_sw_codec_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct sw_runtime_data *prtd = substream->runtime->private_data;
	int ret ;

	

	spin_lock(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		prtd->state |= ST_RUNNING;
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			codec_play_start();
		else
			codec_capture_start();
		sw_dma_ctrl(prtd->params->channel, SW_DMAOP_START);
		//printk("dma start %s\n", __func__);
		//sw_dma_ctrl(prtd->params->channel, SW_DMAOP_STARTED);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			codec_play_stop();
		else
			codec_capture_stop();
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	        //printk("dma stop %s\n", __func__);
		prtd->state &= ~ST_RUNNING;
		sw_dma_ctrl(prtd->params->channel, SW_DMAOP_STOP);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->state &= ~ST_RUNNING;
		sw_dma_ctrl(prtd->params->channel, SW_DMAOP_STOP);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock(&prtd->lock);
	return 0;
}
static int snd_card_sw_codec_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err;
	struct sw_runtime_data *prtd;

	pr_debug("Entered %s\n", __func__);
	prtd = kzalloc(sizeof(struct sw_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;
    
	runtime->hw = sw_pcm_hardware;
	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;
	if ((err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_constraints_rates)) < 0)
		return err;
        
	return 0;
}



static int snd_card_sw_codec_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	//release_mem_region(CODEC_BASSADDRESS, 0x100);
	kfree(runtime->private_data);

	return 0;
}




static struct snd_pcm_ops sw_pcm_ops = {
	.open			= snd_card_sw_codec_open,
	.close			= snd_card_sw_codec_close,
	.ioctl			= snd_pcm_lib_ioctl,
	.hw_params	        = sw_codec_pcm_hw_params,
	.hw_free	        = snd_sw_codec_hw_free,
	.prepare		= snd_sw_codec_prepare,
	.trigger		= snd_sw_codec_trigger,
	.pointer		= snd_sw_codec_pointer,
};


static int __init snd_card_sw_codec_pcm(struct sw_codec *sw_codec, int device)
{
	struct snd_pcm *pcm;
	int err;

	if ((err = snd_pcm_new(sw_codec->card, "M1 PCM", device, 1, 1, &pcm)) < 0)
		return err;

	/*
	 * this sets up our initial buffers and sets the dma_type to isa.
	 * isa works but I'm not sure why (or if) it's the right choice
	 * this may be too large, trying it for now
	 */
	 
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV, 
					      snd_dma_isa_data(),
					      64*1024, 64*1024);

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &sw_pcm_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &sw_pcm_ops);
	pcm->private_data = sw_codec;
	pcm->info_flags = 0;
	strcpy(pcm->name, "sun4i PCM");
	/* setup DMA controller */
   
	return 0;
}





void snd_sw_codec_free(struct snd_card *card)
{
  
}



static int __init sw_codec_probe(struct platform_device *pdev)
{
	int err;
	int ret;
	struct snd_card *card;
	struct sw_codec *chip;
	struct codec_board_info  *db;
        flag_id = 0; 
        pst_src = 0;
	/* register the soundcard */
#if 0
	card = snd_card_new(-1, id, THIS_MODULE, sizeof(struct sw_codec));
	if (card == NULL)
		return -ENOMEM;
#endif
	ret = snd_card_create(0, "sun4i-codec", THIS_MODULE, sizeof(struct sw_codec), 
			      &card);
	if (ret != 0) {
		return -ENOMEM;
	}

	chip = card->private_data;

	card->private_free = snd_sw_codec_free;
	chip->card = card;
	chip->samplerate = AUDIO_RATE_DEFAULT;

	// mixer
	if ((err = snd_chip_codec_mixer_new(card)))
		goto nodev;

	// PCM
	if ((err = snd_card_sw_codec_pcm(chip, 0)) < 0)
	    goto nodev;

        
	strcpy(card->driver, "sun4i-CODEC");
	strcpy(card->shortname, "sun4i-CODEC");
	sprintf(card->longname, "sun4i-CODEC  Audio Codec");
        
	snd_card_set_dev(card, &pdev->dev);

	if ((err = snd_card_register(card)) == 0) {
		pr_debug( KERN_INFO "sun4i audio support initialized\n" );
		platform_set_drvdata(pdev, card);
	}
	else
    {
      return err;
	}

          	

	 db = kzalloc(sizeof(*db), GFP_KERNEL);
	 if (!db)
		 return -ENOMEM; 
 
	 db->codec_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	 db->ccmu_base_res =  platform_get_resource(pdev, IORESOURCE_MEM, 1);
	 db->dev = &pdev->dev;
 
 
 if (db->codec_base_res == NULL || 
	 db->ccmu_base_res == NULL 
	 ) {
	 ret = -ENOENT;
	 pr_debug("codec insufficient resources\n");
	 goto out;
 }
	 /* codec address remap */
	 db->codec_base_req = request_mem_region(db->codec_base_res->start, 0x40,
					   pdev->name);
	 if (db->codec_base_req == NULL) {
		 ret = -EIO;
		 pr_debug("cannot claim codec address reg area\n");
		 goto out;
	 }
	 baseaddr = ioremap(db->codec_base_res->start, 0x40);
	
	 
	 pr_debug("baseaddr = %p\n",baseaddr);
	 
	 if (baseaddr == NULL) {
		 ret = -EINVAL;
		 dev_err(db->dev,"failed to ioremap codec address reg\n");
		 goto out;
	 }
 
 
	 /* ccmu address remap */
	 db->ccmu_base_req = request_mem_region(db->ccmu_base_res->start, 256,
					   pdev->name);

	 if (db->ccmu_base_req == NULL) {
		 ret = -EIO;
		 pr_debug("cannot claim ccmu address reg area\n");
		 goto out;
	 }
	 //clkbaseaddr = ioremap(db->ccmu_base_res->start, 256);
	 
//	 if (clkbaseaddr == NULL) {
//		 ret = -EINVAL;
//		 pr_debug("failed to ioremap ccmu address reg\n");
//		 goto out;
//	 };
	 kfree(db);
	 codec_init(); 
	 //iounmap(clkbaseaddr);
	 //release_resource(db->ccmu_base_req); 
	 
	 //codec_debug(); 
	 printk("m1 Audio successfully loaded..\n"); 
	 #ifdef Debug_Level0
	 pr_debug("sun4i Intenal Codec Initial Ok\n");
	 #endif
	 return 0;
	 
	 out:
		 dev_err(db->dev, "not found (%d).\n", ret);
	
	 nodev:
		snd_card_free(card);
		return err;
}

static int snd_sw_codec_suspend(struct platform_device *pdev,pm_message_t state)
{


        //enable dac analog
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACAEN_L, 0x0);
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACAEN_R, 0x0);
	mdelay(100);
	//pa mute
	codec_wr_control(SW_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	 mdelay(100);
	//enable PA
	codec_wr_control(SW_ADC_ACTL, 0x1, PA_ENABLE, 0x0);
	 mdelay(100);
	//enable headphone direct 
	codec_wr_control(SW_ADC_ACTL, 0x1, HP_DIRECT, 0x0);
 	mdelay(100);
	//enable dac to pa
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACPAS, 0x0);
	 mdelay(100);
	codec_wr_control(SW_DAC_DPC ,  0x1, DAC_EN, 0x0);  
	 mdelay(100);
	
	return 0;	
}

static int snd_sw_codec_resume(struct platform_device *pdev)
{
	//pa mute
	codec_wr_control(SW_DAC_ACTL, 0x1, PA_MUTE, 0x1);	
	mdelay(400);
	codec_wr_control(SW_DAC_DPC ,  0x1, DAC_EN, 0x1);  
	mdelay(20);
        //enable dac analog
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACAEN_L, 0x1);
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACAEN_R, 0x1);
	mdelay(20);
	//enable PA
	codec_wr_control(SW_ADC_ACTL, 0x1, PA_ENABLE, 0x1);
	//enable headphone direct 
	mdelay(20);
	codec_wr_control(SW_ADC_ACTL, 0x1, HP_DIRECT, 0x1);

	//enable dac to pa
	 mdelay(20);
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACPAS, 0x1);
	mdelay(30);
	return 0;	
}


static int __devexit sw_codec_remove(struct platform_device *devptr)
{
	snd_card_free(platform_get_drvdata(devptr));
	platform_set_drvdata(devptr, NULL);
	return 0;
}


static struct resource sw_codec_resource[] = {
      [0] = {    
      	         .start = CODEC_BASSADDRESS,
                 .end   = CODEC_BASSADDRESS + 0x40,
                 .flags = IORESOURCE_MEM,
      },
      [1] = {
		.start  = CCM_BASE,
		.end    = CCM_BASE+1024,
		.flags  = IORESOURCE_MEM, 							
      }
};

static struct platform_device sw_device_codec = {
             .name = "sun4i-codec",
             .id = -1,
             .num_resources = ARRAY_SIZE(sw_codec_resource),
             .resource = sw_codec_resource,
     	   
};

static struct platform_driver sw_codec_driver = {
	.probe		= sw_codec_probe,
	.remove		= sw_codec_remove,
#ifdef CONFIG_PM
	.suspend	= snd_sw_codec_suspend,
	.resume		= snd_sw_codec_resume,
#endif
	.driver		= {
		.name	= "sun4i-codec",
	},
};


static int __init sw_codec_init(void)
{
	int err = 0;
	if((platform_device_register(&sw_device_codec))<0)
		return err;

	if ((err = platform_driver_register(&sw_codec_driver)) < 0)
		return err;
	
	return 0;
}

static void __exit sw_codec_exit(void)
{
	platform_driver_unregister(&sw_codec_driver);
}

module_init(sw_codec_init);
module_exit(sw_codec_exit);

MODULE_DESCRIPTION("sun4i CODEC ALSA codec driver");
MODULE_AUTHOR("software");
MODULE_LICENSE("GPL");

