/*
 *    archos-keys.c : 10/02/2010
 *    g.revaillot, revaillot@archos.com
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <mach/archos-gpio.h>
#include <mach/board-archos.h>

#include <mach/gpio.h>
#include <mach/mux.h>

static struct gpio_keys_button gpio_keys_buttons_list[] = {
	[0] = {
		.code		= KEY_POWER,
		.gpio		= 0,
		.desc		= "power sw",
		.type		= EV_KEY,
		.active_low	= 0,
		.wakeup		= 0,
	},
};

static struct gpio_keys_button gpio_keys_buttons[1];

static struct gpio_keys_platform_data gpio_keys = {
	.buttons		= gpio_keys_buttons,
	.nbuttons		= 0,
	.rep			= 1,
};

static struct platform_device gpio_keys_device = {
	.name			= "gpio-keys",
	.id			= -1,
	.dev			= {
		.platform_data	= &gpio_keys,
	},
};

static int __init _keys_config(void)
{
	const struct archos_keys_config *keys_cfg;
	
	keys_cfg = omap_get_config( ARCHOS_TAG_KEYS, struct archos_keys_config );
	if (keys_cfg == NULL) {
		printk(KERN_DEBUG "archos_keys_init: no board configuration found\n");
		return -ENODEV;
	}

	if ( hardware_rev >= keys_cfg->nrev ) {
		printk(KERN_DEBUG "archos_keys_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, keys_cfg->nrev);
		return 0;
	}

	if (GPIO_EXISTS( keys_cfg->rev[hardware_rev].power )) {
		omap_cfg_reg(GPIO_MUX(keys_cfg->rev[hardware_rev].power));
		memcpy(&gpio_keys_buttons[gpio_keys.nbuttons], &gpio_keys_buttons_list[0], sizeof(struct gpio_keys_button));
		gpio_keys_buttons[gpio_keys.nbuttons].gpio = GPIO_PIN( keys_cfg->rev[hardware_rev].power );
		gpio_keys.nbuttons++;
	}

	return 0;
}

int __init archos_keys_init(void)
{
	int ret;
	if ((ret = _keys_config()) < 0)
		return ret;

	return platform_device_register(&gpio_keys_device);
}

