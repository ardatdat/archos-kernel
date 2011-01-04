/*
 * archos-lcd-qvga28.c
 *
 *  Created on: Fev 02, 2010
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
	pr_debug("[archos-lcd-qvga28] panel_init [%s]\n", ddata->name);

	/* Safe mode for alternative DSS_DATA0 - DSS_DATA5 */
	omap_cfg_reg(H26_3630_SAFE);
	omap_cfg_reg(H25_3630_SAFE);
	omap_cfg_reg(E28_3630_SAFE);
	omap_cfg_reg(J26_3630_SAFE);
	omap_cfg_reg(AC27_3630_SAFE);
	omap_cfg_reg(AC28_3630_SAFE);

	omap_cfg_reg(D28_3630_DSS_PCLK);   /* RE   */
	omap_cfg_reg(D26_3630_DSS_HSYNC);  /* CS0  */
	omap_cfg_reg(D27_3630_DSS_VSYNC);  /* WE   */
	omap_cfg_reg(E27_3630_DSS_ACBIAS); /* RS   */
	omap_cfg_reg(AG22_3630_DSS_DATA0); /* DATA... */
	omap_cfg_reg(AH22_3630_DSS_DATA1);
	omap_cfg_reg(AG23_3630_DSS_DATA2);
	omap_cfg_reg(AH23_3630_DSS_DATA3);
	omap_cfg_reg(AG24_3630_DSS_DATA4);
	omap_cfg_reg(AH24_3630_DSS_DATA5);
	omap_cfg_reg(E26_3630_DSS_DATA6);
	omap_cfg_reg(F28_3630_DSS_DATA7);
	omap_cfg_reg(F27_3630_DSS_DATA8);
	omap_cfg_reg(G26_3630_DSS_DATA9);
	omap_cfg_reg(AD28_3630_DSS_DATA10);
	omap_cfg_reg(AD27_3630_DSS_DATA11);
	omap_cfg_reg(AB28_3630_DSS_DATA12);
	omap_cfg_reg(AB27_3630_DSS_DATA13);
	omap_cfg_reg(AA28_3630_DSS_DATA14);
	omap_cfg_reg(AA27_3630_DSS_DATA15);	 
	
	GPIO_INIT_OUTPUT(display_gpio.lcd_pwon);
	GPIO_INIT_OUTPUT(display_gpio.lcd_rst);

#if !defined(CONFIG_FB_OMAP_BOOTLOADER_INIT)
	if (GPIO_EXISTS(display_gpio.lcd_rst))
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 1);
	if (GPIO_EXISTS(display_gpio.lcd_pwon))
		gpio_set_value( GPIO_PIN(display_gpio.lcd_pwon), 0);
#endif
	return 0;
}

static void panel_reset(void)
{  
	if (GPIO_EXISTS(display_gpio.lcd_rst)) {
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 0);
		msleep(1);  
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 1);
		msleep(1);  
	}
}  

static int panel_enable(struct omap_dss_device *disp)
{
	pr_info("[archos-lcd-qvga28] panel_enable [%s]\n", disp->name);
	if (panel_state == 1)
		return -1;

	if (GPIO_EXISTS(display_gpio.lcd_pwon)) {
		gpio_set_value( GPIO_PIN(display_gpio.lcd_pwon), 1 );
		msleep(10);		// min 1ms
	}

	panel_reset();	
	panel_state = 1;
	return 0;
}

static void panel_disable(struct omap_dss_device *disp)
{
	pr_debug("[archos-lcd-qvga28] panel_disable [%s]\n", disp->name);

	if (GPIO_EXISTS(display_gpio.lcd_rst))
		gpio_set_value( GPIO_PIN(display_gpio.lcd_rst), 0 );
	if (GPIO_EXISTS(display_gpio.lcd_pwon))
		gpio_set_value( GPIO_PIN(display_gpio.lcd_pwon), 0 );

	panel_state = 0;
}

static struct omap_dss_device mt_qvga_28_panel = {
	.type = OMAP_DISPLAY_TYPE_DBI,
	.name = "lcd",
	.driver_name = "mt_qvga_28",
	.platform_enable = panel_enable,
	.platform_disable = panel_disable,
	.phy.rfbi = {
		.channel = 0,
		.data_lines = 16,
	},
	.panel = {
		.config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_RF | OMAP_DSS_LCD_ONOFF),
		/* timings for NDL=2, RGB888 */
		.timings = {
			.x_res		= 240,
			.y_res		= 320,
			// 60 hz
			.pixel_clock	= 27000,	/* 27Mhz, (ddr_clk_hz/4)* 2/3 */
			.hsw		= 0,		/* horizontal sync pulse width */
			.hfp		= 25,		/* horizontal front porch */
			.hbp		= 30,		/* horizontal back porch */
			.vsw		= 6,		/* vertical sync pulse width */
			.vfp		= 6,		/* vertical front porch (forced to 0, hardware restriction */
			.vbp		= 12,		/* vertical back porch */		
		},
	},
	.ctrl = {
		.pixel_size = 18,
		.rfbi_timings = {
               		.cs_on_time     = 10000,

                	.we_on_time     = 10000,
                	.we_off_time    = 60000,
                	.we_cycle_time  = 100000,

                	.re_on_time     = 10000,
                	.re_off_time    = 160000,
                	.re_cycle_time  = 300000,

                	.access_time    = 110000,
			.cs_off_time    = 160000,
	
			.cs_pulse_width = 0,
		},
	},

};

int __init panel_qvga_28_init(struct omap_dss_device *disp_data)
{
	const struct archos_display_config *disp_cfg;
	int ret = -ENODEV;

	disp_cfg = omap_get_config( ARCHOS_TAG_DISPLAY, struct archos_display_config );
	if (disp_cfg == NULL)
		return ret;

	if ( hardware_rev >= disp_cfg->nrev ) {
		printk(KERN_DEBUG "archos_display_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, disp_cfg->nrev);
		return ret;
	}

	display_gpio = disp_cfg->rev[hardware_rev];

	panel_init(&mt_qvga_28_panel);

	*disp_data = mt_qvga_28_panel;

	return 0;
}
