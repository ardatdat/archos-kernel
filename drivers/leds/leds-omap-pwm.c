/* drivers/leds/leds-omap_pwm.c
 *
 * Driver to blink LEDs using OMAP PWM timers
 *
 * Copyright (C) 2006 Nokia Corporation
 * Author: Timo Teras
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <linux/clk.h>
#include <asm/delay.h>
#include <mach/board.h>
#include <mach/dmtimer.h>

struct omap_pwm_led {
	struct led_classdev cdev;
	struct work_struct work;
	struct omap_pwm_led_platform_data *pdata;
	struct omap_dm_timer *intensity_timer;
	struct omap_dm_timer *blink_timer;
	int powered;
	int resumed;
	unsigned int on_period, off_period;
	enum led_brightness brightness;
};

static inline struct omap_pwm_led *pdev_to_omap_pwm_led(struct platform_device *pdev)
{
	return platform_get_drvdata(pdev);
}

static inline struct omap_pwm_led *cdev_to_omap_pwm_led(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct omap_pwm_led, cdev);
}

static inline struct omap_pwm_led *work_to_omap_pwm_led(struct work_struct *work)
{
	return container_of(work, struct omap_pwm_led, work);
}

static void omap_pwm_led_set_blink(struct omap_pwm_led *led)
{
	if (!led->powered)
		return;

	if (led->on_period != 0 && led->off_period != 0) {
		unsigned long load_reg, cmp_reg;

		load_reg = 32768 * (led->on_period + led->off_period) / 1000;
		cmp_reg = 32768 * led->on_period / 1000;

		omap_dm_timer_stop(led->blink_timer);
		omap_dm_timer_set_load(led->blink_timer, 1, -load_reg);
		omap_dm_timer_set_match(led->blink_timer, 1, -cmp_reg);
		omap_dm_timer_set_pwm(led->blink_timer, 1, 1,
				      OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);
		omap_dm_timer_write_counter(led->blink_timer, -2);
		omap_dm_timer_start(led->blink_timer);
	} else {
		omap_dm_timer_set_pwm(led->blink_timer, 1, 1,
				      OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);
		omap_dm_timer_stop(led->blink_timer);
	}
}

static void omap_pwm_led_power_on(struct omap_pwm_led *led)
{
	if (led->powered)
		return;
	led->powered = 1;

	/* Select clock */
	omap_dm_timer_enable(led->intensity_timer);
	omap_dm_timer_set_source(led->intensity_timer, OMAP_TIMER_SRC_SYS_CLK);

	/* Turn voltage on */
	if (led->pdata->set_power != NULL)
		led->pdata->set_power(led->pdata, 1);

	/* Enable PWM timers */
	if (led->blink_timer != NULL) {
		omap_dm_timer_enable(led->blink_timer);
		omap_dm_timer_set_source(led->blink_timer,
					 OMAP_TIMER_SRC_32_KHZ);
		omap_pwm_led_set_blink(led);
	}
}

static void omap_pwm_led_power_off(struct omap_pwm_led *led)
{
	if (!led->powered)
		return;
	led->powered = 0;

	/* Everything off */
	omap_dm_timer_stop(led->intensity_timer);
	omap_dm_timer_disable(led->intensity_timer);

	if (led->blink_timer != NULL) {
		omap_dm_timer_stop(led->blink_timer);
		omap_dm_timer_disable(led->blink_timer);
	}

	if (led->pdata->set_power != NULL)
		led->pdata->set_power(led->pdata, 0);
}

static void pwm_set_speed(struct omap_dm_timer *gpt,
		int frequency, int duty_cycle)
{
	u32 val;
	u32 period;
	struct clk *timer_fclk;

	/* and you will have an overflow in 1 sec         */
	/* so,                              */
	/* freq_timer     -> 1s             */
	/* carrier_period -> 1/carrier_freq */
	/* => carrier_period = freq_timer/carrier_freq */

	timer_fclk = omap_dm_timer_get_fclk(gpt);
	period = clk_get_rate(timer_fclk) / frequency;

	val = 0xFFFFFFFF+1-period;
	omap_dm_timer_set_load(gpt, 1, val);

	val = 0xFFFFFFFF+1-(period*duty_cycle/256);
	omap_dm_timer_set_match(gpt, 1, val);

	/* assume overflow first: no toogle if first trig is match */
	omap_dm_timer_write_counter(gpt, 0xFFFFFFFE);
}

static void omap_pwm_led_set_pwm_cycle(struct omap_pwm_led *led, int cycle)
{
	int pwm_frequency = 10000;

	if (led->pdata->bkl_max)
		cycle = ( (cycle * led->pdata->bkl_max ) / 255);

	if (led->pdata->bkl_freq)
		pwm_frequency = led->pdata->bkl_freq;

	if (cycle == LED_FULL) {
		if (led->pdata->invert)
			omap_dm_timer_set_pwm(led->intensity_timer, 0, 1,
				      OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);
		else
			omap_dm_timer_set_pwm(led->intensity_timer, 1, 1,
				      OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);
		omap_dm_timer_stop(led->intensity_timer);
	} else {
		if (led->pdata->invert)
			omap_dm_timer_set_pwm(led->intensity_timer, 1, 1,
				      OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);
		else
			omap_dm_timer_set_pwm(led->intensity_timer, 0, 1,
				      OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);

		pwm_set_speed(led->intensity_timer, pwm_frequency, 256-cycle);
		omap_dm_timer_start(led->intensity_timer);
	}
}

static void omap_pwm_led_set(struct led_classdev *led_cdev,
			     enum led_brightness value)
{
	struct omap_pwm_led *led = cdev_to_omap_pwm_led(led_cdev);

	led->brightness = value;
	schedule_work(&led->work);
}

static void omap_pwm_led_work(struct work_struct *work)
{
	struct omap_pwm_led *led = work_to_omap_pwm_led(work);

	if (led->brightness != LED_OFF) {
		omap_pwm_led_power_on(led);
		omap_pwm_led_set_pwm_cycle(led, led->brightness);
	} else {
		if (led->pdata->invert && led->resumed) {
			omap_pwm_led_power_on(led);
			omap_dm_timer_set_pwm(led->intensity_timer, 1, 1,
				      	OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);
		}
		omap_pwm_led_power_off(led);
	}
	
	led->resumed = 0;
}

static ssize_t omap_pwm_led_on_period_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct omap_pwm_led *led = cdev_to_omap_pwm_led(led_cdev);

	return sprintf(buf, "%u\n", led->on_period) + 1;
}

static ssize_t omap_pwm_led_on_period_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct omap_pwm_led *led = cdev_to_omap_pwm_led(led_cdev);
	int ret = -EINVAL;
	unsigned long val;
	char *after;
	size_t count;

	val = simple_strtoul(buf, &after, 10);
	count = after - buf;
	if (*after && isspace(*after))
		count++;

	if (count == size) {
		led->on_period = val;
		omap_pwm_led_set_blink(led);
		ret = count;
	}

	return ret;
}

static ssize_t omap_pwm_led_off_period_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct omap_pwm_led *led = cdev_to_omap_pwm_led(led_cdev);

	return sprintf(buf, "%u\n", led->off_period) + 1;
}

static ssize_t omap_pwm_led_off_period_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct omap_pwm_led *led = cdev_to_omap_pwm_led(led_cdev);
	int ret = -EINVAL;
	unsigned long val;
	char *after;
	size_t count;

	val = simple_strtoul(buf, &after, 10);
	count = after - buf;
	if (*after && isspace(*after))
		count++;

	if (count == size) {
		led->off_period = val;
		omap_pwm_led_set_blink(led);
		ret = count;
	}

	return ret;
}

static DEVICE_ATTR(on_period, 0644, omap_pwm_led_on_period_show,
				omap_pwm_led_on_period_store);
static DEVICE_ATTR(off_period, 0644, omap_pwm_led_off_period_show,
				omap_pwm_led_off_period_store);

static int omap_pwm_led_probe(struct platform_device *pdev)
{
	struct omap_pwm_led_platform_data *pdata = pdev->dev.platform_data;
	struct omap_pwm_led *led;
	int ret;

	led = kzalloc(sizeof(struct omap_pwm_led), GFP_KERNEL);
	if (led == NULL) {
		dev_err(&pdev->dev, "No memory for device\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, led);
	led->cdev.brightness_set = omap_pwm_led_set;
	led->cdev.default_trigger = pdata->default_trigger;
	led->cdev.name = pdata->name;
	led->pdata = pdata;
	led->brightness = LED_OFF;
	INIT_WORK(&led->work, omap_pwm_led_work);

	dev_info(&pdev->dev, "OMAP PWM LED (%s) at GP timer %d/%d\n",
		 pdata->name, pdata->intensity_timer, pdata->blink_timer);

	/* register our new led device */
	ret = led_classdev_register(&pdev->dev, &led->cdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "led_classdev_register failed\n");
		goto error_classdev;
	}

	/* get related dm timers */
	led->intensity_timer = omap_dm_timer_request_specific(pdata->intensity_timer);
	if (led->intensity_timer == NULL) {
		dev_err(&pdev->dev, "failed to request intensity pwm timer\n");
		ret = -ENODEV;
		goto error_intensity;
	}

	if (led->pdata->invert)
		omap_dm_timer_set_pwm(led->intensity_timer, 1, 1,
				      OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);

	omap_dm_timer_disable(led->intensity_timer);

	if (pdata->blink_timer != 0) {
		led->blink_timer = omap_dm_timer_request_specific(pdata->blink_timer);
		if (led->blink_timer == NULL) {
			dev_err(&pdev->dev, "failed to request blinking pwm timer\n");
			ret = -ENODEV;
			goto error_blink1;
		}
		omap_dm_timer_disable(led->blink_timer);

		ret = device_create_file(led->cdev.dev,
					       &dev_attr_on_period);
		if(ret)
			goto error_blink2;

		ret = device_create_file(led->cdev.dev,
					&dev_attr_off_period);
		if(ret)
			goto error_blink3;

	}

	return 0;

error_blink3:
	device_remove_file(led->cdev.dev,
				 &dev_attr_on_period);
error_blink2:
	dev_err(&pdev->dev, "failed to create device file(s)\n");
error_blink1:
	omap_dm_timer_free(led->intensity_timer);
error_intensity:
	led_classdev_unregister(&led->cdev);
error_classdev:
	kfree(led);
	return ret;
}

static int omap_pwm_led_remove(struct platform_device *pdev)
{
	struct omap_pwm_led *led = pdev_to_omap_pwm_led(pdev);

	device_remove_file(led->cdev.dev,
				 &dev_attr_on_period);
	device_remove_file(led->cdev.dev,
				 &dev_attr_off_period);
	led_classdev_unregister(&led->cdev);

	omap_pwm_led_set(&led->cdev, LED_OFF);
	if (led->blink_timer != NULL)
		omap_dm_timer_free(led->blink_timer);
	omap_dm_timer_free(led->intensity_timer);
	kfree(led);

	return 0;
}

#ifdef CONFIG_PM
static int omap_pwm_led_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct omap_pwm_led *led = pdev_to_omap_pwm_led(pdev);

	led_classdev_suspend(&led->cdev);
	return 0;
}

static int omap_pwm_led_resume(struct platform_device *pdev)
{
	struct omap_pwm_led *led = pdev_to_omap_pwm_led(pdev);

	led->resumed = 1;
	led_classdev_resume(&led->cdev);
	return 0;
}
#else
#define omap_pwm_led_suspend NULL
#define omap_pwm_led_resume NULL
#endif

static struct platform_driver omap_pwm_led_driver = {
	.probe		= omap_pwm_led_probe,
	.remove		= omap_pwm_led_remove,
	.suspend	= omap_pwm_led_suspend,
	.resume		= omap_pwm_led_resume,
	.driver		= {
		.name		= "omap_pwm_led",
		.owner		= THIS_MODULE,
	},
};

static int __init omap_pwm_led_init(void)
{
	return platform_driver_register(&omap_pwm_led_driver);
}

static void __exit omap_pwm_led_exit(void)
{
	platform_driver_unregister(&omap_pwm_led_driver);
}

module_init(omap_pwm_led_init);
module_exit(omap_pwm_led_exit);

MODULE_AUTHOR("Timo Teras");
MODULE_DESCRIPTION("OMAP PWM LED driver");
MODULE_LICENSE("GPL");
