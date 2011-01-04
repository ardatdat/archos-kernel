/*
 * ledtrig-earlysuspend.c
 *
 *  Created on: Jun 9, 2010
 *      Author: matthias welwarsky <welwarsky@archos.com>
 *      
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#include <linux/device.h>

#include "leds.h"

struct suspend_trig_data {
	struct led_classdev *led_cdev;
	struct early_suspend suspend;
	struct notifier_block pm_notifier;
};

static void ledtrig_earlyresume(struct early_suspend *h)
{
	struct suspend_trig_data *trig_data = container_of(h, struct suspend_trig_data, suspend);
	struct led_classdev *led_cdev = trig_data->led_cdev;

	led_set_brightness(led_cdev, LED_FULL);
}

static int ledtrig_awake_pm_callback(struct notifier_block *nfb,
					unsigned long action,
					void *ignored)
{
	struct suspend_trig_data *trig_data = 
			container_of(nfb, struct suspend_trig_data, pm_notifier);
	struct led_classdev *led_cdev = trig_data->led_cdev;
	
	switch (action) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		/* switch the LED off on device suspend */
		led_set_brightness(led_cdev, LED_OFF);
		return NOTIFY_OK;
		
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		/* Don't do anything on resume until the display unblanks,
		 * which is caught by the earlysuspend notifier */
		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}

static void awake_trig_activate(struct led_classdev *led_cdev)
{
	struct suspend_trig_data *trig_data;
	
	trig_data = kzalloc(sizeof(struct suspend_trig_data), GFP_KERNEL);
	if (!trig_data) {
		dev_err(led_cdev->dev, 
			"out of memory error while activating LED awake trigger\n");
		return;
	}
	
	trig_data->suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	trig_data->suspend.resume = ledtrig_earlyresume;
	trig_data->pm_notifier.notifier_call = ledtrig_awake_pm_callback;
	trig_data->pm_notifier.priority = 0;
	trig_data->led_cdev = led_cdev;

	led_cdev->trigger_data = trig_data;
	
	/* 
	 * it's valid for the use case to set the LED to full 
	 * brightness when the trigger is activated. Activating the trigger
	 * will always happen *before* a suspend.
	 */ 
	led_set_brightness(led_cdev, LED_FULL);
	register_early_suspend(&trig_data->suspend);
	register_pm_notifier(&trig_data->pm_notifier);
}

static void awake_trig_deactivate(struct led_classdev *led_cdev)
{
	struct suspend_trig_data *trig_data = led_cdev->trigger_data;
	
	unregister_early_suspend(&trig_data->suspend);
	unregister_pm_notifier(&trig_data->pm_notifier);
	kfree(trig_data);
}

static struct led_trigger awake_led_trigger = {
	.name     = "awake",
	.activate = awake_trig_activate,
	.deactivate = awake_trig_deactivate,
};

static int __init earlysuspend_trig_init(void)
{
	return led_trigger_register(&awake_led_trigger);
}

static void __exit earlysuspend_trig_exit(void)
{
	led_trigger_unregister(&awake_led_trigger);
}

module_init(earlysuspend_trig_init);
module_exit(earlysuspend_trig_exit);

MODULE_AUTHOR("Matthias Welwarsky <welwarsky@archos.com>");
MODULE_DESCRIPTION("Awake LED trigger");
MODULE_LICENSE("GPL");
