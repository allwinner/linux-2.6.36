#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "anx7150sp.h"


#define ANX7150SP_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define ANX7150SP_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)

static int anx7150sp_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int anx7150sp_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static void anx7150sp_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
}

static int anx7150sp_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	return 0;
}

static int anx7150sp_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int anx7150sp_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{
	return 0;
}

static int anx7150sp_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}
struct snd_soc_dai_ops anx7150sp_dai_ops = {
		.startup = anx7150sp_startup,
		.shutdown = anx7150sp_shutdown,
		.hw_params = anx7150sp_hw_params,
		.digital_mute = anx7150sp_mute,
		.set_sysclk = anx7150sp_set_dai_sysclk,
		.set_clkdiv = anx7150sp_set_dai_clkdiv,
		.set_fmt = anx7150sp_set_dai_fmt,
};
struct snd_soc_dai anx7150sp_dai = {
	.name = "ANX7150SP",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ANX7150SP_RATES,
		.formats = ANX7150SP_FORMATS,
	},
	/* pcm operations */
	.ops = &anx7150sp_dai_ops,
};
EXPORT_SYMBOL(anx7150sp_dai);

static int anx7150sp_soc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = -ENOMEM;
	
	(socdev->card->codec) = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (socdev->card->codec == NULL)
		return ret;

	codec = socdev->card->codec;

	mutex_init(&codec->mutex);

	codec->name = "ANX7150SP";
	codec->owner = THIS_MODULE;
	codec->dai = &anx7150sp_dai;
	codec->num_dai = 1;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
//		printk(KERN_ERR "ANX7150: failed to register pcms\n");
		goto pcm_err;
	}

	return 0;

pcm_err:
	kfree(codec);
	return ret;
}

/* power down chip */
static int anx7150sp_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
//	printk("[SPDIF]Entered %s, %s, %d\n", __func__, __FILE__, __LINE__);
	snd_soc_free_pcms(socdev);

	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_anx7150sp = {
	.probe =        anx7150sp_soc_probe,
	.remove =       anx7150sp_soc_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_anx7150sp);


static int __init anx7150sp_init(void)
{
	return snd_soc_register_dai(&anx7150sp_dai);
}
module_init(anx7150sp_init);

static void __exit anx7150sp_exit(void)
{
	snd_soc_unregister_dai(&anx7150sp_dai);
}
module_exit(anx7150sp_exit);

MODULE_DESCRIPTION("ANX7150SP ALSA soc codec driver");
MODULE_AUTHOR("Zoltan Devai, Christian Pellegrin <chripell@evolware.org>");
MODULE_LICENSE("GPL");
