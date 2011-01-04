/*
 * Led(s) Board configuration
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
#include <linux/leds.h>

#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/mux.h>
#include <mach/archos-gpio.h>
#include <mach/display.h>
#include <mach/board-archos.h>

static struct gpio_led gpio_leds[] = {
	{
		.name			= "power",
		.default_trigger	= "default-on",
		.gpio			= -1,
		.active_low		= 0,
	},
};

static struct omap_pwm_led_platform_data board_backlight_data = {
	.name			= "lcd-backlight",
	.default_trigger	= "backlight",
};

static struct archos_gpio bkl_power_gpio;

static struct gpio_led_platform_data board_led_data = {
	.leds		= gpio_leds,
	.num_leds	= ARRAY_SIZE(gpio_leds),
};

static struct platform_device board_led_device = {
	.name		= "leds-gpio",
	.id		= 0,
	.dev            = {
		.platform_data = &board_led_data,
	},
};

static struct platform_device board_backlight_device = {
	.name		= "omap_pwm_led",
	.id		= 0,
	.dev.platform_data = &board_backlight_data,
};

static void bkl_set_power(struct omap_pwm_led_platform_data *self, int on_off)
{
	if (GPIO_EXISTS(bkl_power_gpio))
		gpio_set_value( GPIO_PIN(bkl_power_gpio), on_off );
}

int __init archos_leds_init(void)
{
	const struct archos_leds_config *leds_cfg;
	struct archos_gpio power_led;
	struct archos_pwm_conf backlight_led;
	int ret;
	
	leds_cfg = omap_get_config( ARCHOS_TAG_LEDS, struct archos_leds_config );
	if (leds_cfg == NULL) {
		printk(KERN_DEBUG "archos_leds_init: no board configuration found\n");
		return -ENODEV;
	}
	if ( hardware_rev >= leds_cfg->nrev ) {
		printk(KERN_DEBUG "archos_leds_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, leds_cfg->nrev);
		return -ENODEV;
	}

	power_led = leds_cfg->rev[hardware_rev].power_led;
	backlight_led = leds_cfg->rev[hardware_rev].backlight_led;

	omap_cfg_reg(GPIO_MUX(power_led));
	gpio_leds[0].gpio = GPIO_PIN(power_led);
	gpio_leds[0].active_low = leds_cfg->rev[hardware_rev].pwr_invert;
	
	omap_cfg_reg(backlight_led.mux_cfg);
	board_backlight_data.intensity_timer = backlight_led.timer;
	board_backlight_data.bkl_max = leds_cfg->rev[hardware_rev].bkl_max;
	board_backlight_data.bkl_freq = leds_cfg->rev[hardware_rev].bkl_freq;
	board_backlight_data.invert = leds_cfg->rev[hardware_rev].bkl_invert;
	board_backlight_data.set_power = &bkl_set_power;

	bkl_power_gpio = leds_cfg->rev[hardware_rev].backlight_power;
	if (GPIO_EXISTS(bkl_power_gpio))
		archos_gpio_init_output(&bkl_power_gpio, "bkl_power");
	
	// set backlight power to 0 
	bkl_set_power(NULL, 0);

	ret = platform_device_register(&board_led_device);
	if (ret < 0)
		pr_err("unable to register power LED\n");

	ret = platform_device_register(&board_backlight_device);
	if (ret < 0)
		pr_err("unable to register backlight LED\n");
	
	return 0;
}
