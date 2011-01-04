/*
 * panel-lg-lb070wv6.c
 *
 *  Created on: Apri 13, 2010
 *      Author: Matthias Welwarsky <welwarsky@archos.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/display.h>
#include <linux/leds.h>

#include <mach/gpio.h>
#include <linux/fb.h>
#include <mach/display.h>
#include <linux/delay.h>

static void panel_remove(struct omap_dss_device *disp)
{
}

static int panel_enable(struct omap_dss_device *disp)
{
	int ret = 0;

	if (disp->platform_enable)
		ret = disp->platform_enable(disp);
	if (ret < 0)
		return ret;
	
	return ret;
}

static void panel_disable(struct omap_dss_device *disp)
{
	if (disp->platform_disable)
		disp->platform_disable(disp);
}

static int panel_suspend(struct omap_dss_device *disp)
{
	panel_disable(disp);

	return 0;
}

static int panel_resume(struct omap_dss_device *disp)
{
	panel_enable(disp);

	return 0;
}

#define TEST_FLICKER
static struct omap_video_timings lg_panel_timings = {
	.x_res		= 800,
	.y_res		= 480,
#ifdef TEST_FLICKER
	.pixel_clock	= 41575,	// reduce flicker
#else
	.pixel_clock	= 33260,	// std value from manufacturer spec
#endif

	.hsw		= 64,		/* horizontal sync pulse width */
	.hfp		= 64,		/* horizontal front porch */
	.hbp		= 128,		/* horizontal back porch */

	.vsw		= 3,		/* vertical sync pulse width */
	.vfp		= 20,		/* vertical front porch */
	.vbp		= 22,		/* vertical back porch */
};

static int panel_probe(struct omap_dss_device *dssdev)
{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
		OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_RF | OMAP_DSS_LCD_ONOFF | OMAP_DSS_LCD_IPC,
	dssdev->panel.timings = lg_panel_timings;
	dssdev->panel.recommended_bpp = 24;

	return 0;
}

static struct omap_dss_driver lg_panel = {

	.driver		= {
		.name	= "lg_wvga_7",
		.owner 	= THIS_MODULE,
	},
	.probe		= panel_probe,
	.remove		= panel_remove,
	.enable		= panel_enable,
	.disable	= panel_disable,
	.suspend	= panel_suspend,
	.resume		= panel_resume,
};

static int __init panel_drv_init(void)
{
	printk("panel lg wvga 7 init\n");

	omap_dss_register_driver(&lg_panel);
	return 0;
}

static void __exit panel_drv_exit(void)
{
	omap_dss_unregister_driver(&lg_panel);
}

module_init(panel_drv_init);
module_exit(panel_drv_exit);
