/*
 * archos-lcd-lg-fwvga43.c
 *
 *  Created on: Jan 22, 2010
 *      Author: Matthias Welwarsky <welwarsky@archos.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <asm/mach-types.h>

#include <mach/gpio.h>
#include <mach/mux.h>
#include <mach/display.h>
#include <mach/archos-gpio.h>
#include <mach/board-archos.h>
#include <mach/dmtimer.h>
#include <linux/spi/spi.h>
#include <mach/mcspi.h>


static struct archos_disp_conf display_gpio;
static int panel_state;

static int panel_init(struct omap_dss_device *ddata)
{

	pr_debug("panel_init [%s]\n", ddata->name);

	// init dsi bus, should be ever made in boot
	omap_cfg_reg(AG22_34XX_DSI_DX0);
	omap_cfg_reg(AH22_34XX_DSI_DY0);
	omap_cfg_reg(AG23_34XX_DSI_DX1);
	omap_cfg_reg(AH23_34XX_DSI_DY1);
	omap_cfg_reg(AG24_34XX_DSI_DX2);
	omap_cfg_reg(AH24_34XX_DSI_DY2);

	GPIO_INIT_OUTPUT(display_gpio.lcd_pwon);
	GPIO_INIT_OUTPUT(display_gpio.lcd_rst);

#if !defined(CONFIG_FB_OMAP_BOOTLOADER_INIT)
	if (GPIO_EXISTS(display_gpio.lcd_pwon))
		gpio_set_value( GPIO_PIN(display_gpio.lcd_pwon), 0);
	if (GPIO_EXISTS(display_gpio.lcd_rst))
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 0);
#endif
	return 0;
}

static int panel_enable(struct omap_dss_device *disp)
{
	pr_info("panel_enable [%s]\n", disp->name);

	if ( panel_state == 1)
		return -1;

	if (GPIO_EXISTS(display_gpio.lcd_pwon)) {
		gpio_set_value( GPIO_PIN(display_gpio.lcd_pwon), 1 );
		msleep(10);		// min 1ms
	}

	if (GPIO_EXISTS(display_gpio.lcd_rst)) {
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 1 );
		msleep(50);
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 0 );
		msleep(50);		// min 10ms
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 1 );
		msleep(50);		// min 10ms

	}
	panel_state = 1;
	return 0;
}

static void panel_disable(struct omap_dss_device *disp)
{
	pr_debug("panel_disable [%s]\n", disp->name);

	if (GPIO_EXISTS(display_gpio.lcd_rst))
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 0 );
	if (GPIO_EXISTS(display_gpio.lcd_pwon))
		gpio_set_value( GPIO_PIN(display_gpio.lcd_pwon), 0 );

	panel_state = 0;
}

static struct omap_dss_device lg_fwvga_43_panel = {
	.type = OMAP_DISPLAY_TYPE_DSI,
	.name = "lcd",
	.driver_name = "lg_fwvga_43",
	.phy.dsi = {
		.clk_lane = 1,			// at pos 1
		.clk_pol = 0,			// dx = + , dy = -
		.data1_lane = 3,		// dx2 = + , dy2 = -
		.data1_pol = 0,			// dx = + , dy = -
		.data2_lane = 2,		// dx1 = + , dy1 = -
		.data2_pol = 0,			// dx = + , dy = -
		.lp_clk_hz = 5400000,		
		.ddr_clk_hz = 162000000, 	/* for NDL=2, 24bpp */
		.ext_te = 0,
	},
	.ctrl = {
		.pixel_size = 24,
	},
	.panel = {
		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_RF | OMAP_DSS_LCD_ONOFF),
		/* timings for NDL=2, RGB888 */
		.timings = {
			.x_res		= 480,
			.y_res		= 854,
			// 60 hz
			.pixel_clock	= 27000,	/* 27Mhz, (ddr_clk_hz/4)* 2/3 */
			.hsw		= 0,		/* horizontal sync pulse width */
			.hfp		= 25,		/* horizontal front porch */
			.hbp		= 30,		/* horizontal back porch */
			.vsw		= 4,		/* vertical sync pulse width */
			.vfp		= 12,		/* vertical front porch */
			.vbp		= 8,		/* vertical back porch */		
		},
	},
	.platform_enable = panel_enable,
	.platform_disable = panel_disable,
};

int __init panel_fwvga_43_init(struct omap_dss_device *disp_data)
{
	const struct archos_display_config *disp_cfg;
	int ret = -ENODEV;
	printk("panel_fwvga_43_init\n");

	disp_cfg = omap_get_config( ARCHOS_TAG_DISPLAY, struct archos_display_config );
	if (disp_cfg == NULL)
		return ret;

	if ( hardware_rev >= disp_cfg->nrev ) {
		printk(KERN_DEBUG "archos_display_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, disp_cfg->nrev);
		return ret;
	}

	display_gpio = disp_cfg->rev[hardware_rev];

	panel_init(&lg_fwvga_43_panel);

	*disp_data = lg_fwvga_43_panel;

	return 0;
}
