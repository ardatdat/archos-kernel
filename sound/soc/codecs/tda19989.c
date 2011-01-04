/*
 * tda19989.c  --  TDA19989 (HDMI) ALSA Soc Audio driver
 *
 * Copyright 2010 Archos SA.
 *
 * Authors:
 * Jean-Christophe RONA		<rona@archos.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "tda19989.h"

/*
 * There is no real interaction with the codec here for now...
 * This file allows to register the codec with the SoC layer, that's all.
 * But we may need to add some audio stuff here if no other driver
 * handles the audio part of the HDMI chip...
 */

#define AUDIO_NAME "tda19989"
#define TDA19989_VERSION "0.1"

/* codec private data */
struct tda19989_priv {
	/* inclk is the input clock, (not the generated one when the chip is Master) */
	unsigned int inclk;
};

static int tda19989_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	return 0;
}

static int tda19989_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	return 0;
}

static int tda19989_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

#define TDA19989_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000)

#define TDA19989_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE)

struct snd_soc_dai tda19989_dai = {
	.name = "TDA19989 HiFi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = TDA19989_RATES,
		.formats = TDA19989_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = TDA19989_RATES,
		.formats = TDA19989_FORMATS,},
	.ops = {
		.hw_params = tda19989_hw_params,
		.digital_mute = tda19989_mute,
		.set_fmt = tda19989_set_dai_fmt,
	},
};
EXPORT_SYMBOL_GPL(tda19989_dai);

static int tda19989_set_bias_level(struct snd_soc_codec *codec,
	enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
	case SND_SOC_BIAS_OFF:
	default:
		break;
	}
	codec->bias_level = level;
	return 0;
}


static int tda19989_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	tda19989_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int tda19989_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	tda19989_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	tda19989_set_bias_level(codec, codec->suspend_bias_level);
	return 0;
}

/*
 * initialise the TDA19989 driver
 * register the mixer and dsp interfaces with the kernel
 */
static int tda19989_init(struct snd_soc_device* socdev)
{
	struct snd_soc_codec *codec = socdev->codec;
	int ret = 0;

	codec->name = "TDA19989";
	codec->owner = THIS_MODULE;
	codec->set_bias_level = tda19989_set_bias_level;
	codec->bias_level = SND_SOC_BIAS_OFF;
	codec->dai = &tda19989_dai;
	codec->num_dai = 1;

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if(ret < 0) {
		printk(KERN_ERR "tda19989: failed to create pcms\n");
		goto pcm_err;
	}

	/* power on device */
	tda19989_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

//	wm8985_add_controls(codec);
//	wm8985_add_widgets(codec);
	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
      	printk(KERN_ERR "tda19989: failed to register card\n");
		goto card_err;
    }
	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	return ret;
}

static int tda19989_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct tda19989_setup_data *setup;
	struct snd_soc_codec *codec;
	struct tda19989_priv *tda19989;
	int ret = 0;

	pr_info("TDA19989 (HDMI) Audio Codec %s\n", TDA19989_VERSION);

	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	tda19989 = kzalloc(sizeof(struct tda19989_priv), GFP_KERNEL);
	if (tda19989 == NULL) {
		kfree(codec);
		return -ENOMEM;
	}

	codec->private_data = tda19989;
	socdev->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

//	tda19989_socdev = socdev;
	tda19989_init(socdev);

	return ret;
}

/* power down chip */
static int tda19989_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	if (codec->control_data)
		tda19989_set_bias_level(codec, SND_SOC_BIAS_OFF);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);

	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_tda19989 = {
	.probe = 	tda19989_probe,
	.remove = 	tda19989_remove,
	.suspend = 	tda19989_suspend,
	.resume =	tda19989_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_tda19989);

static int __init tda19989_modinit(void)
{
	return snd_soc_register_dai(&tda19989_dai);
}
module_init(tda19989_modinit);

static void __exit tda19989_exit(void)
{
	snd_soc_unregister_dai(&tda19989_dai);
}
module_exit(tda19989_exit);

MODULE_DESCRIPTION("ASoC TDA19989 (HDMI) driver");
MODULE_AUTHOR("Jean-Christophe Rona");
MODULE_LICENSE("GPL");
