/*
********************************************************************************************************
*                          SUN4I----HDMI AUDIO
*                   (c) Copyright 2002-2004, All winners Co,Ld.
*                          All Right Reserved
*
* FileName: sndhdmi.c   author:chenpailin 
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

#include "sndhdmi.h"

#define HDMI

#ifdef HDMI
__audio_hdmi_func g_hdmi_func;

void audio_set_hdmi_func(__audio_hdmi_func * hdmi_func)
{
	g_hdmi_func.hdmi_audio_enable = hdmi_func->hdmi_audio_enable;
	g_hdmi_func.hdmi_set_audio_para = hdmi_func->hdmi_set_audio_para;
}

EXPORT_SYMBOL(audio_set_hdmi_func);
#endif

#define SNDHDMI_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define SNDHDMI_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE)

hdmi_audio_t hdmi_para;

static int sndhdmi_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}


static int sndhdmi_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}


static void sndhdmi_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
}


static int sndhdmi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	hdmi_para.sample_rate = params_rate(params);
	
	#ifdef HDMI
		g_hdmi_func.hdmi_audio_enable(1, 1);

	#endif

	return 0;
}

static int sndhdmi_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int sndhdmi_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{

	hdmi_para.fs_between = div;
	
	return 0;
}


static int sndhdmi_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

//codec dai operation
struct snd_soc_dai_ops sndhdmi_dai_ops = {
		.startup = sndhdmi_startup,
		.shutdown = sndhdmi_shutdown,
		.hw_params = sndhdmi_hw_params,
		.digital_mute = sndhdmi_mute,
		.set_sysclk = sndhdmi_set_dai_sysclk,
		.set_clkdiv = sndhdmi_set_dai_clkdiv,
		.set_fmt = sndhdmi_set_dai_fmt,
};

//codec dai
struct snd_soc_dai sndhdmi_dai = {
	.name = "SNDHDMI",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDHDMI_RATES,
		.formats = SNDHDMI_FORMATS,
	},
	/* pcm operations */
	.ops = &sndhdmi_dai_ops,
};
EXPORT_SYMBOL(sndhdmi_dai);

static int sndhdmi_soc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = -ENOMEM;
	
	(socdev->card->codec) = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (socdev->card->codec == NULL)
		return ret;

	codec = socdev->card->codec;

	mutex_init(&codec->mutex);

	codec->name = "SNDHDMI";
	codec->owner = THIS_MODULE;
	codec->dai = &sndhdmi_dai;
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

static int sndhdmi_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
	snd_soc_free_pcms(socdev);

	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_sndhdmi = {
	.probe =        sndhdmi_soc_probe,
	.remove =       sndhdmi_soc_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_sndhdmi);

static int __init sndhdmi_init(void)
{
	return snd_soc_register_dai(&sndhdmi_dai);
}
module_init(sndhdmi_init);

static void __exit sndhdmi_exit(void)
{
	snd_soc_unregister_dai(&sndhdmi_dai);
}
module_exit(sndhdmi_exit);

MODULE_DESCRIPTION("SNDHDMI ALSA soc codec driver");
MODULE_AUTHOR("Zoltan Devai, Christian Pellegrin <chripell@evolware.org>");
MODULE_LICENSE("GPL");
