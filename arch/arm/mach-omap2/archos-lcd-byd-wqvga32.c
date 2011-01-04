/*
 * archos-lcd-byd.c
 *
 *  Created on: Jan 13, 2010
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
//#include <mach/omapfb.h>
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

	archos_gpio_init_output(&display_gpio.lcd_pwon, "lcd_pwon");
	archos_gpio_init_output(&display_gpio.lcd_rst, "lcd_rst");

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
	pr_debug("panel_enable [%s]\n", disp->name);

	if ( panel_state == 1 )
		return -1;

	if (GPIO_EXISTS(display_gpio.lcd_pwon)) {
		gpio_set_value( GPIO_PIN(display_gpio.lcd_pwon), 1 );
		msleep(50);
	}
	if (GPIO_EXISTS(display_gpio.lcd_rst)) {
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 1 );
		msleep(100);
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

static struct omap2_mcspi_device_config lcd_mcspi_config = {
	.turbo_mode		= 0,
	.single_channel		= 1,  /* 0: slave, 1: master */
	.mode			= OMAP2_MCSPI_MASTER,
	.dma_mode		= 0,
	.force_cs_mode		= 0,
	.fifo_depth		= 0,
};

static struct spi_board_info lcd_spi_board_info[] = {
	[0] = {
		.modalias	= "hx8352",
		.bus_num	= 2,
		.chip_select	= 0,
		.max_speed_hz   = 1500000,
		.controller_data= &lcd_mcspi_config,
	}
};

static struct omap_dss_device byd_wqvga_32_panel = {
	.type = OMAP_DISPLAY_TYPE_DPI,
	.name = "lcd",
	.driver_name = "byd_wqvga_32",
	.phy.dpi.data_lines = 18,
	.platform_enable = panel_enable,
	.platform_disable = panel_disable,
	.phy.dpi.dither = OMAP_DSS_DITHER_SPATIAL,
/*	.dev		= {
		.platform_data = &lcd_data,
	},
*/
};

int __init panel_wqvga_32_init(struct omap_dss_device *disp_data)
{
	const struct archos_display_config *disp_cfg;
	int ret = -ENODEV;

	pr_debug("panel_wqvga_32_init\n");

	disp_cfg = omap_get_config( ARCHOS_TAG_DISPLAY, struct archos_display_config );
	if (disp_cfg == NULL)
		return ret;

	if ( hardware_rev >= disp_cfg->nrev ) {
		printk(KERN_DEBUG "archos_display_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, disp_cfg->nrev);
		return ret;
	}

	display_gpio = disp_cfg->rev[hardware_rev];

	/* init spi, bus line is used as mcspi */
	omap_cfg_reg(display_gpio.spi.spi_clk.mux_cfg);
	omap_cfg_reg(display_gpio.spi.spi_cs.mux_cfg);
	omap_cfg_reg(display_gpio.spi.spi_data.mux_cfg);

	spi_register_board_info(lcd_spi_board_info, 1);

	panel_init(&byd_wqvga_32_panel);

#if defined(CONFIG_FB_OMAP_BOOTLOADER_INIT)
	panel_set_backlight_level(NULL, 255);
#endif
	if ( machine_is_archos_a32() && ( hardware_rev < 3 ) ) {
		byd_wqvga_32_panel.phy.dpi.data_lines = 24;
		byd_wqvga_32_panel.phy.dpi.dither = OMAP_DSS_DITHER_NONE;
	}
	*disp_data = byd_wqvga_32_panel;

	return 0;
}
