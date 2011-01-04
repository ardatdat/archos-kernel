/*
 * Vibrator Board configuration
 *
 */

#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <linux/module.h>
#include <../drivers/staging/android/timed_gpio.h>

#include <mach/gpio.h>
#include <mach/mux.h>
#include <mach/archos-gpio.h>
#include <mach/display.h>
#include <mach/board-archos.h>

static struct archos_gpio vibrator_gpio;

static struct timed_gpio vibrator_timed_gpio = {
	.name = "vibrator",
	.gpio = -1,
	.max_timeout = 100000,
	.active_low = 1,
};

static struct timed_gpio_platform_data vibrator_data = {
	.num_gpios = 1,
	.gpios = &vibrator_timed_gpio,
};

static struct platform_device vibrator_device = {
	.name		= TIMED_GPIO_NAME,
	.id		= -1,
	.dev            = {
		.platform_data = &vibrator_data,
	},
};

int __init archos_vibrator_init(void)
{
	const struct archos_vibrator_config *vibrator_cfg;

	printk("Vibrator Init\n");

	vibrator_cfg = omap_get_config( ARCHOS_TAG_VIBRATOR, struct archos_vibrator_config );
	if (vibrator_cfg == NULL) {
		printk(KERN_DEBUG "archos_vibrator_init: no board configuration found\n");
		return -ENODEV;
	}
	if ( hardware_rev >= vibrator_cfg->nrev ) {
		printk(KERN_DEBUG "archos_vibrator_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, vibrator_cfg->nrev);
		return -ENODEV;
	}

	vibrator_gpio = vibrator_cfg->rev[hardware_rev].vibrator;

	GPIO_INIT_OUTPUT(vibrator_gpio);
	vibrator_timed_gpio.gpio = GPIO_PIN(vibrator_gpio);
	gpio_direction_output(GPIO_PIN(vibrator_gpio), 1);

	return platform_device_register(&vibrator_device);
}
