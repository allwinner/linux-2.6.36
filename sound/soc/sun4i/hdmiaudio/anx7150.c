/*
********************************************************************************************************
*                          SUN4I----HDMI AUDIO
*                   (c) Copyright 2002-2004, All winners Co,Ld.
*                          All Right Reserved
*
* FileName: anx7150.c   author:chenpailin 
* Description: 
* Others: 
* History:
*   <author>      <time>      <version>   <desc> 
*   chenpailin   2011-07-19     1.0      modify this module 
********************************************************************************************************
*/
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "anx7150.h"

//#define CONFIG_LYCHEE_HDMI_SUN4I

#ifdef CONFIG_LYCHEE_HDMI_SUN4I
extern __s32 Hdmi_Set_Audio_Para(hdmi_audio_t * audio_para);
extern __s32 Hdmi_Audio_Enable(__u8 mode,  __u8 channel);
#endif


#define ANX7150_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define ANX7150_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE)

hdmi_audio_t hdmi_para;

static int anx7150_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}


static int anx7150_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}


static void anx7150_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
}


static int anx7150_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	hdmi_para.sample_rate = params_rate(params);
	
	#ifdef CONFIG_LYCHEE_HDMI_SUN4I
		Hdmi_Audio_Enable(1, 1);
		Hdmi_Set_Audio_Para(&hdmi_para);
	#endif

	return 0;
}

static int anx7150_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int anx7150_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{

	hdmi_para.fs_between = div;
	
	return 0;
}


static int anx7150_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

//codec dai operation
struct snd_soc_dai_ops anx7150_dai_ops = {
		.startup = anx7150_startup,
		.shutdown = anx7150_shutdown,
		.hw_params = anx7150_hw_params,
		.digital_mute = anx7150_mute,
		.set_sysclk = anx7150_set_dai_sysclk,
		.set_clkdiv = anx7150_set_dai_clkdiv,
		.set_fmt = anx7150_set_dai_fmt,
};

//codec dai
struct snd_soc_dai anx7150_dai = {
	.name = "ANX7150",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ANX7150_RATES,
		.formats = ANX7150_FORMATS,
	},
	/* pcm operations */
	.ops = &anx7150_dai_ops,
};
EXPORT_SYMBOL(anx7150_dai);

static int anx7150_soc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = -ENOMEM;
	
	(socdev->card->codec) = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (socdev->card->codec == NULL)
		return ret;

	codec = socdev->card->codec;

	mutex_init(&codec->mutex);

	codec->name = "ANX7150";
	codec->owner = THIS_MODULE;
	codec->dai = &anx7150_dai;
	codec->num_dai = 1;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		goto pcm_err;
	}

	return 0;


pcm_err:
	kfree(codec);
	return ret;
}

static int anx7150_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
	snd_soc_free_pcms(socdev);

	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_anx7150 = {
	.probe =        anx7150_soc_probe,
	.remove =       anx7150_soc_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_anx7150);

static int __init anx7150_init(void)
{
	return snd_soc_register_dai(&anx7150_dai);
}
module_init(anx7150_init);

static void __exit anx7150_exit(void)
{
	snd_soc_unregister_dai(&anx7150_dai);
}
module_exit(anx7150_exit);

MODULE_DESCRIPTION("ANX7150 ALSA soc codec driver");
MODULE_AUTHOR("Zoltan Devai, Christian Pellegrin <chripell@evolware.org>");
MODULE_LICENSE("GPL");
