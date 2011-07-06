/*
 *  Driver for CODEC on M1 soundcard
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License.
 * 
* 
***************************************************************************************************/
//#define DEBUG 0
#ifndef CONFIG_PM
#define CONFIG_PM
#endif
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
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include <sound/initval.h>
#include <linux/clk.h>
#include "sun4i-codec.h"

static int flag_id = 0;
/*
 * these are the address and sizes used to fill the xmit buffer
 * so we can get a clock in record only mode
 */
#define FORCE_CLOCK_ADDR		(dma_addr_t)FLUSH_BASE_PHYS
#define FORCE_CLOCK_SIZE		4096 
#define SW_DAC_FIFO         0x01c22c04
#define SW_ADC_FIFO         0x01c22c20


struct clk *codec_apbclk,*codec_pll2clk,*codec_moduleclk;
static unsigned long suspend_codecreate = 0;

#define codec_RATES SNDRV_PCM_RATE_8000_192000
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

static const int codec_enum_items[] = {
	16, //mic input - 
	8,  //adc input -
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
	struct resource	*codec_base_req;   /* resources found */

	spinlock_t	lock;
} codec_board_info_t;

/* ID for this card */
static struct sw_dma_client sw_codec_dma_client_play = {
	.name		= "CODEC PCM Stereo PLAY"
};

static struct sw_dma_client sw_codec_dma_client_capture = {
	.name		= "CODEC PCM Stereo CAPTURE"
};

static struct sw_pcm_dma_params sw_codec_pcm_stereo_play = {
	.client		= &sw_codec_dma_client_play,
	.channel	= DMACH_NADDA,
	.dma_addr	= CODEC_BASSADDRESS + SW_DAC_TXDATA,//发送数据地址
	.dma_size	= 4,
};

static struct sw_pcm_dma_params sw_codec_pcm_stereo_capture = {
	.client		= &sw_codec_dma_client_capture,
	.channel	= DMACH_NADDA,  //only support half full
	.dma_addr	= CODEC_BASSADDRESS + SW_ADC_RXDATA,//接收数据地址
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
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,//最大的缓冲区大小
	.period_bytes_min	= 1024*16,//最小周期大小
	.period_bytes_max	= 1024*32,//最大周期大小
	.periods_min		= 4,//最小周期数
	.periods_max		= 8,//最大周期数
	.fifo_size	     	= 32,//fifo字节数
};

struct sw_codec{
	struct snd_card *card;
	struct snd_pcm *pcm;
	long samplerate;
};

static unsigned int rates[] = {
	8000,11025,12000,16000,
	22050,24000,24000,32000,
	44100,48000,96000,192000
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

	if (change){       
		//printk("reg = %x,reg_val = %x\n",reg,new);
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
* 	info()函数用于获得该control的详细信息，该函数必须填充传递给它的第二个参数snd_ctl_elem_info结构体
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
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;//the info of type
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	
	uinfo->count = shift ==	rshift	?	1:	2;	//the info of elem count
	uinfo->value.integer.min = 0;				//the info of min value
	uinfo->value.integer.max = max;				//the info of max value
	return	0;
}


/**
*	snd_codec_get_volsw	-	single	mixer	get	callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to get the value of a single mixer control
*	get()函数用于得到control的目前值并返回用户空间
*	return 0 for success.
*/
int snd_codec_get_volsw(struct snd_kcontrol	*kcontrol,
		struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int	max = mc->max;
	/*fls(7) = 3,fls(1)=1,fls(0)=0,fls(15)=4,fls(3)=2,fls(23)=5*/
	unsigned int mask = (1 << fls(max)) -1;
	unsigned int invert = mc->invert;
	unsigned int reg = mc->reg;
	
	ucontrol->value.integer.value[0] =	
		(codec_rdreg(reg)>>	shift) & mask;
	if(shift != rshift)
		ucontrol->value.integer.value[1] =
			(codec_rdreg(reg) >> rshift) & mask;

	/*将获得的值写入snd_ctl_elem_value*/
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
*	put()用于从用户空间写入值，如果值被改变，该函数返回1，否则返回0.
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

/**
*	codec_reset - reset the codec 
* @codec	SoC Audio Codec
* Reset the codec, set the register of codec default value
* Return 0 for success
*/
static  int codec_init(void)
{
	//enable dac digital 
	codec_wr_control(SW_DAC_DPC ,  0x1, DAC_EN, 0x1);  
	//codec version seting
	codec_wr_control(SW_DAC_DPC ,  0x1, DAC_VERSION, 0x1);
	//set digital volume to maximum
	codec_wr_control(SW_DAC_DPC, 0x6, DIGITAL_VOL, 0x0);
	//pa mute
	codec_wr_control(SW_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	//enable PA
	codec_wr_control(SW_ADC_ACTL, 0x1, PA_ENABLE, 0x1);
	//enable headphone direct 
	codec_wr_control(SW_ADC_ACTL, 0x1, HP_DIRECT, 0x1);
	//set volume
	codec_wr_control(SW_DAC_ACTL, 0x6, VOLUME, 0x01);
	
	return 0;
	
}

static int codec_play_open(void)
{
	//flush TX FIFO
	codec_wr_control(SW_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);
	//set TX FIFO send drq level
	codec_wr_control(SW_DAC_FIFOC ,0x1fff, TX_TRI_LEVEL, 0xf);

	codec_wr_control(SW_DAC_FIFOC ,0x3,DRA_LEVEL,0x3);
	//send last sample when dac fifo under run
	codec_wr_control(SW_DAC_FIFOC ,0x1, LAST_SE, 0x1);
	//set TX FIFO MODE
	codec_wr_control(SW_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);//TX_FIFO_MODE == 24
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
	 //codec_wr_control(SW_ADC_ACTL, 0x7, ADC_SELECT, 0x4);//2011-6-17 11:41:12 hx del
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

/*	对sun4i-codec.c各寄存器的各种设定，或读取。主要实现函数有三个.
* 	.info = snd_codec_info_volsw, .get = snd_codec_get_volsw,\.put = snd_codec_put_volsw, 
*/
static const struct snd_kcontrol_new codec_snd_controls[] = {
	//FOR NORMAL VERSION
	//CODEC_SINGLE("Master Playback Volume", SW_DAC_ACTL,0,0x3f,0),
	//For A VERSION
	CODEC_SINGLE("Master Playback Volume", SW_DAC_DPC,12,0x3f,0),//62 steps, 3e + 1 = 3f 主音量控制
	CODEC_SINGLE("Playback Switch", SW_DAC_ACTL,6,1,0),//全局输出开关
	CODEC_SINGLE("Capture Volume",SW_ADC_ACTL,20,7,0),//录音音量
	CODEC_SINGLE("Fm Volume",SW_DAC_ACTL,23,7,0),//Fm 音量
	CODEC_SINGLE("Line Volume",SW_DAC_ACTL,26,1,0),//Line音量
	CODEC_SINGLE("MicL Volume",SW_ADC_ACTL,25,3,0),//mic左音量
	CODEC_SINGLE("MicR Volume",SW_ADC_ACTL,23,3,0),//mic右音量
	CODEC_SINGLE("FmL Switch",SW_DAC_ACTL,17,1,0),//Fm左开关
	CODEC_SINGLE("FmR Switch",SW_DAC_ACTL,16,1,0),//Fm右开关
	CODEC_SINGLE("LineL Switch",SW_DAC_ACTL,19,1,0),//Line左开关
	CODEC_SINGLE("LineR Switch",SW_DAC_ACTL,18,1,0),//Line右开关
	CODEC_SINGLE("Ldac Left Mixer",SW_DAC_ACTL,15,1,0),
	CODEC_SINGLE("Rdac Right Mixer",SW_DAC_ACTL,14,1,0),
	CODEC_SINGLE("Ldac Right Mixer",SW_DAC_ACTL,13,1,0),
	CODEC_SINGLE("Mic Input Mux",SW_DAC_ACTL,9,15,0),//from bit 9 to bit 12.Mic（麦克风）输入静音
	CODEC_SINGLE("ADC Input Mux",SW_ADC_ACTL,17,7,0),//ADC输入静音
};

int __init snd_chip_codec_mixer_new(struct snd_card *card)
{
  	/*
  	*	每个alsa预定义的组件在构造时需调用snd_device_new()，而每个组件的析构方法则在函数集中被包含
  	*	对于PCM、AC97此类预定义组件，我们不需要关心它们的析构，而对于自定义的组件，则需要填充snd_device_ops
  	*	中的析构函数指针dev_free，这样，当snd_card_free()被调用时，组件将被自动释放。
  	*/
  	static struct snd_device_ops ops = {
  		.dev_free	=	codec_dev_free,
  	};
  	unsigned char *clnt = "codec";
	int idx, err;
	/*
	*	snd_ctl_new1函数用于创建一个snd_kcontrol并返回其指针，
	*	snd_ctl_add函数用于将创建的snd_kcontrol添加到对应的card中。
	*/
	for (idx = 0; idx < ARRAY_SIZE(codec_snd_controls); idx++) {
		if ((err = snd_ctl_add(card, snd_ctl_new1(&codec_snd_controls[idx],clnt))) < 0) {
			return err;
		}
	}
	/*
	*	当card被创建后，设备（组件）能够被创建并关联于该card。第一个参数是snd_card_create
	*	创建的card指针，第二个参数type指的是device-level即设备类型，形式为SNDRV_DEV_XXX,包括
	*	SNDRV_DEV_CODEC、SNDRV_DEV_CONTROL、SNDRV_DEV_PCM、SNDRV_DEV_RAWMIDI等、用户自定义的
	*	设备的device-level是SNDRV_DEV_LOWLEVEL，ops参数是1个函数集（snd_device_ops结构体）的
	*	指针，device_data是设备数据指针，snd_device_new本身不会分配设备数据的内存，因此事先应
	*	分配。在这里在snd_card_create分配。
	*/
	if ((err = snd_device_new(card, SNDRV_DEV_CODEC, clnt, &ops)) < 0) {
		//codec_free(clnt);
		printk("the func is: %s,the line is:%d\n", __func__, __LINE__);
		return err;
	}

	
	strcpy(card->mixername, "codec Mixer");
	       
	return 0;
}

static void sw_pcm_enqueue(struct snd_pcm_substream *substream)
{
	struct sw_runtime_data *prtd = substream->runtime->private_data;
	dma_addr_t pos = prtd->dma_pos;
	unsigned int limit;
	int ret;
	
	unsigned long len = prtd->dma_period;
  	limit = prtd->dma_limit;
  	while(prtd->dma_loaded < limit){
		if((pos + len) > prtd->dma_end){
			len  = prtd->dma_end - pos;
			//	printk("[SPDIF]%s: corrected dma len %ld\n", __func__, len);
		}
		ret = sw_dma_enqueue(prtd->params->channel, substream, __bus_to_virt(pos),  len);
		//printk("[SPDIF]%s: corrected dma len %d, pos = %#x\n", __func__, len, pos);
		if(ret == 0){
			prtd->dma_loaded++;
			pos += prtd->dma_period;
			if(pos >= prtd->dma_end)
				pos = prtd->dma_start;
		}else{
			break;
		}	  
	}
	prtd->dma_pos = pos;
	//	printk("[SPDIF]In the end of %s, dma_start = %#x, dma_end = %#x, dma_pos = %#x\n", __func__, prtd->dma_start, prtd->dma_end, prtd->dma_pos);
}

static void sw_audio_buffdone(struct sw_dma_chan *channel, 
		                                  void *dev_id, int size,
		                                  enum sw_dma_buffresult result)
{
//    	printk("[SPDIF]Buffer Done \n");
		struct sw_runtime_data *prtd;
		struct snd_pcm_substream *substream = dev_id;

		if (result == SW_RES_ABORT || result == SW_RES_ERR)
			return;
			
		prtd = substream->runtime->private_data;
			if (substream){
				//	printk("[SPDIF]Enter Elapsed\n");
				snd_pcm_period_elapsed(substream);
			}	
	
		spin_lock(&prtd->lock);
		{
			prtd->dma_loaded--;
			sw_pcm_enqueue(substream);
		}
		spin_unlock(&prtd->lock);
}

static snd_pcm_uframes_t snd_sw_codec_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sw_runtime_data *prtd = runtime->private_data;
	unsigned long res = 0;
    //printk("GET POINTER\n");
	spin_lock(&prtd->lock);
	sw_dma_getcurposition(DMACH_NADDA, (dma_addr_t*)&dmasrc, (dma_addr_t*)&dmadst);
	//printk("dmasrc: %x\n",dmasrc);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE){
		res = dmadst + prtd->dma_period - prtd->dma_start;
	}else{
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
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		switch(substream->runtime->rate){
			case 44100:			
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				
				break;
			case 22050:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}		
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 11025:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 48000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}					
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 96000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(7<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 192000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}					
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(6<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 32000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 24000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 16000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 12000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}					
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			case 8000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			default:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}									
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);		
				break;
		}
		
		switch(substream->runtime->channels){
			case 1:
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val |=(1<<6);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);			
				break;
			case 2:
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
			default:
				reg_val = readl(baseaddr + SW_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SW_DAC_FIFOC);
				break;
		}
	}else{
		switch(substream->runtime->rate){
			case 44100:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);					
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}			
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				
				break;
			case 22050:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);	
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}						
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 11025:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);	
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}							
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 48000:				
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}					
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 32000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}					
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 24000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 16000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 12000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			case 8000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);
				break;
			default:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);	
				if (-1 == clk_enable(codec_moduleclk)){
					//printk("open codec_moduleclk failed; \n");
				}				
				reg_val = readl(baseaddr + SW_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SW_ADC_FIFOC);		
				break;
		}
		
		switch(substream->runtime->channels){
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
	if (!codec_dma_conf){
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
	}else {
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
	  	ret = sw_dma_config(prtd->params->channel,codec_dma_conf);  	   	 
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
	/*获得PCM运行时信息指针*/
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
	
	/* ensure that buffer size is a multiple of period size */
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
	.open			= snd_card_sw_codec_open,//打开
	.close			= snd_card_sw_codec_close,//关闭
	.ioctl			= snd_pcm_lib_ioctl,//I/O控制
	.hw_params	    = sw_codec_pcm_hw_params,//硬件参数
	.hw_free	    = snd_sw_codec_hw_free,//资源释放
	.prepare		= snd_sw_codec_prepare,//准备
	.trigger		= snd_sw_codec_trigger,//在pcm被开始、停止或暂停时调用
	.pointer		= snd_sw_codec_pointer,//当前缓冲区的硬件位置
};


static int __init snd_card_sw_codec_pcm(struct sw_codec *sw_codec, int device)
{
	struct snd_pcm *pcm;
	int err;
	/*创建PCM实例*/
	if ((err = snd_pcm_new(sw_codec->card, "M1 PCM", device, 1, 1, &pcm)) < 0){	
		printk("the func is: %s,the line is:%d\n", __func__, __LINE__);
		return err;
	}

	/*
	 * this sets up our initial buffers and sets the dma_type to isa.
	 * isa works but I'm not sure why (or if) it's the right choice
	 * this may be too large, trying it for now
	 */
	 
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV, 
					      snd_dma_isa_data(),
					      64*1024, 64*1024);
	/*
	*	设置PCM操作，第1个参数是snd_pcm的指针，第2 个参数是SNDRV_PCM_STREAM_PLAYBACK
	*	或SNDRV_ PCM_STREAM_CAPTURE，而第3 个参数是PCM 操作结构体snd_pcm_ops
	*/
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &sw_pcm_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &sw_pcm_ops);
	pcm->private_data = sw_codec;//置pcm->private_data为芯片特定数据
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
    printk("enter sun4i Audio codec!!!\n"); 
	/* register the soundcard */
	ret = snd_card_create(0, "sun4i-codec", THIS_MODULE, sizeof(struct sw_codec), 
			      &card);
	if (ret != 0) {
		return -ENOMEM;
	}
	/*从private_data中取出分配的内存大小*/
	chip = card->private_data;
	/*声卡芯片的专用数据*/
	card->private_free = snd_sw_codec_free;//card私有数据释放
	chip->card = card;
	chip->samplerate = AUDIO_RATE_DEFAULT;

	/* 
	*	mixer,注册control(mixer)接口
	*	创建一个control至少要实现snd_kcontrol_new中的info(),get()和put()这三个成员函数
	*/
	if ((err = snd_chip_codec_mixer_new(card)))
		goto nodev;

	/* 
	*	PCM,录音放音相关，注册PCM接口
	*/
	if ((err = snd_card_sw_codec_pcm(chip, 0)) < 0)
	    goto nodev;
        
	strcpy(card->driver, "sun4i-CODEC");
	strcpy(card->shortname, "sun4i-CODEC");
	sprintf(card->longname, "sun4i-CODEC  Audio Codec");
        
	snd_card_set_dev(card, &pdev->dev);
	
	//注册card
	if ((err = snd_card_register(card)) == 0) {
		pr_debug( KERN_INFO "sun4i audio support initialized\n" );
		platform_set_drvdata(pdev, card);
	}else{
      return err;
	}

	db = kzalloc(sizeof(*db), GFP_KERNEL);
	if (!db)
		return -ENOMEM; 
  	/* codec_apbclk */
	codec_apbclk = clk_get(NULL,"apb_audio_codec");
	if (-1 == clk_enable(codec_apbclk)) {
		printk("codec_apbclk failed; \n");
	}
	/* codec_pll2clk */
	codec_pll2clk = clk_get(NULL,"audio_pll");
			 
	/* codec_moduleclk */
	codec_moduleclk = clk_get(NULL,"audio_codec");

	if (clk_set_parent(codec_moduleclk, codec_pll2clk)) {
		printk("try to set parent of codec_moduleclk to codec_pll2clk failed!\n");		
	}
	if (clk_set_rate(codec_moduleclk, 24576000)) {
			printk("set codec_moduleclk clock freq 24576000 failed!\n");
	}
	if (-1 == clk_enable(codec_moduleclk)){
		printk("open codec_moduleclk failed; \n");
	}
	db->codec_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	db->dev = &pdev->dev;
	
	if (db->codec_base_res == NULL) {
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

	 kfree(db);
	 codec_init(); 

	 printk("sun4i Audio codec successfully loaded..\n"); 
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

/*	suspend state,先disable左右声道，然后静音，再disable pa(放大器)，
 *	disable 耳机，disable dac->pa，最后disable DAC
 * 	顺序不可调，否则刚关闭声卡的时候可能出现噪音
 */
static int snd_sw_codec_suspend(struct platform_device *pdev,pm_message_t state)
{
	pr_debug("enter snd_sw_codec_suspend:%s,%d\n",__func__,__LINE__);
    //disable dac analog
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACAEN_L, 0x0);
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACAEN_R, 0x0);
	mdelay(100);
	//pa mute
	codec_wr_control(SW_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	 mdelay(100);
	//disable PA
	codec_wr_control(SW_ADC_ACTL, 0x1, PA_ENABLE, 0x0);
	 mdelay(100);
	//disable headphone direct 
	codec_wr_control(SW_ADC_ACTL, 0x1, HP_DIRECT, 0x0);
 	mdelay(100);
	//disable dac to pa
	codec_wr_control(SW_DAC_ACTL, 0x1, 	DACPAS, 0x0);
	 mdelay(100);
	codec_wr_control(SW_DAC_DPC ,  0x1, DAC_EN, 0x0);  
	 mdelay(100);
	 suspend_codecreate =clk_get_rate(codec_moduleclk);
	clk_disable(codec_moduleclk);
	clk_disable(codec_apbclk);
	pr_debug("[codec suspend reg]\n");
	pr_debug("codec_module CLK:0xf1c20140 is:%x\n", *(volatile int *)0xf1c20140);
	pr_debug("codec_pll2 CLK:0xf1c20008 is:%x\n", *(volatile int *)0xf1c20008);
	pr_debug("codec_apb CLK:0xf1c20068 is:%x\n", *(volatile int *)0xf1c20068);
	pr_debug("[codec suspend reg]\n");
	return 0;	
}

/*	resume state,先unmute，
 *	再enable DAC，enable L/R DAC,enable PA，
 * 	enable 耳机，enable dac to pa
 *	顺序不可调，否则刚打开声卡的时候可能出现噪音
 */
static int snd_sw_codec_resume(struct platform_device *pdev)
{
	pr_debug("enter snd_sw_codec_resume:%s,%d\n",__func__,__LINE__);

	if (-1 == clk_enable(codec_moduleclk)){
		printk("open codec_moduleclk failed; \n");
	}
	if (-1 == clk_enable(codec_apbclk)){
		printk("open codec_moduleclk failed; \n");
	}
	clk_set_rate(codec_moduleclk, suspend_codecreate);	
		//pa unmute
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
	/*for clk test*/
	pr_debug("[codec resume reg]\n");
	pr_debug("codec_module CLK:0xf1c20140 is:%x\n", *(volatile int *)0xf1c20140);
	pr_debug("codec_pll2 CLK:0xf1c20008 is:%x\n", *(volatile int *)0xf1c20008);
	pr_debug("codec_apb CLK:0xf1c20068 is:%x\n", *(volatile int *)0xf1c20068);
	pr_debug("[codec resume reg]\n");
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
};

/*data relating*/
static struct platform_device sw_device_codec = {
	.name = "sun4i-codec",
	.id = -1,
	.num_resources = ARRAY_SIZE(sw_codec_resource),
	.resource = sw_codec_resource,     	   
};

/*method relating*/
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
	clk_disable(codec_moduleclk);
	//释放codec_pll2clk时钟句柄
	clk_put(codec_pll2clk);
	//释放codec_apbclk时钟句柄
	clk_put(codec_apbclk);

	platform_driver_unregister(&sw_codec_driver);

}

module_init(sw_codec_init);
module_exit(sw_codec_exit);

MODULE_DESCRIPTION("sun4i CODEC ALSA codec driver");
MODULE_AUTHOR("software");
MODULE_LICENSE("GPL");

