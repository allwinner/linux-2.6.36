/*
********************************************************************************************************
*                          SUN4I----HDMI AUDIO
*                   (c) Copyright 2002-2004, All winners Co,Ld.
*                          All Right Reserved
*
* FileName: codecchip.c   author:chenpailin  date:2011-07-19 
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
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>
#include <linux/io.h>

#include "codecchip.h"

static int i2s_used = 0;
#define codecchip_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define codecchip_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE)

hdmi_audio_t hdmi_parameter;

static int codecchip_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int codecchip_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static void codecchip_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
}

static int codecchip_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	hdmi_parameter.sample_rate = params_rate(params);

	return 0;
}

static int codecchip_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int codecchip_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{

	hdmi_parameter.fs_between = div;
	
	return 0;
}

static int codecchip_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}
struct snd_soc_dai_ops codecchip_dai_ops = {
		.startup = codecchip_startup,
		.shutdown = codecchip_shutdown,
		.hw_params = codecchip_hw_params,
		.digital_mute = codecchip_mute,
		.set_sysclk = codecchip_set_dai_sysclk,
		.set_clkdiv = codecchip_set_dai_clkdiv,
		.set_fmt = codecchip_set_dai_fmt,
};
struct snd_soc_dai codecchip_dai = {
	.name = "codecchip",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = codecchip_RATES,
		.formats = codecchip_FORMATS,
	},
	/* pcm operations */
	.ops = &codecchip_dai_ops,
};
EXPORT_SYMBOL(codecchip_dai);

static int codecchip_soc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = -ENOMEM;
	
	(socdev->card->codec) = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (socdev->card->codec == NULL)
		return ret;

	codec = socdev->card->codec;

	mutex_init(&codec->mutex);

	codec->name = "codecchip";
	codec->owner = THIS_MODULE;
	codec->dai = &codecchip_dai;
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

/* power down chip */
static int codecchip_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
	snd_soc_free_pcms(socdev);

	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_codecchip = {
	.probe =        codecchip_soc_probe,
	.remove =       codecchip_soc_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_codecchip);

static int __init codecchip_init(void)
{
	int ret;
	ret = script_parser_fetch("i2s_para","i2s_used", &i2s_used, sizeof(int));
	
	if (ret)
    {
        printk("[I2S]codecchip_init fetch i2s using configuration failed\n");
    } 
    
    if(i2s_used)
    {
		return snd_soc_register_dai(&codecchip_dai);
	}
	else
	{
		printk("[I2S]codecchip cannot find any using configuration for controllers, return directly!\n");
        return 0;
	}
}
module_init(codecchip_init);

static void __exit codecchip_exit(void)
{
	if(i2s_used)
	{
		i2s_used = 0;
		snd_soc_unregister_dai(&codecchip_dai);
	}
}
module_exit(codecchip_exit);

MODULE_DESCRIPTION("codecchip ALSA soc codec driver");
MODULE_AUTHOR("Zoltan Devai, Christian Pellegrin <chripell@evolware.org>");
MODULE_LICENSE("GPL");
