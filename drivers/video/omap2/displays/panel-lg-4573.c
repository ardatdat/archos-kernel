/*
 * panel-lg-4573.c
 *
 *  Created on: Jan 26, 2010
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

#define DEBUG

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/completion.h>
#include <linux/lg-4573.h>
#include <linux/leds.h>

#include <mach/display.h>

/* DSI Virtual channel. Hardcoded for now. */
#define TCH 0

#define DCS_NOP		0x00
#define DCS_SWRESET		0x01
#define DCS_READ_NUM_ERRORS	0x05
#define DCS_READ_POWER_MODE	0x0a
#define DCS_READ_MADCTL	0x0b
#define DCS_READ_PIXEL_FORMAT	0x0c
#define DCS_SLEEP_IN		0x10
#define DCS_SLEEP_OUT		0x11
#define DCS_DISPLAY_OFF	0x28
#define DCS_DISPLAY_ON		0x29
#define DCS_COLUMN_ADDR	0x2a
#define DCS_PAGE_ADDR		0x2b
#define DCS_MEMORY_WRITE	0x2c
#define DCS_TEAR_OFF		0x34
#define DCS_TEAR_ON		0x35
#define DCS_MEM_ACC_CTRL	0x36
#define DCS_PIXEL_FORMAT	0x3a
#define DCS_BRIGHTNESS		0x51
#define DCS_CTRL_DISPLAY	0x53
#define DCS_WRITE_CABC		0x55
#define DCS_READ_CABC		0x56
#define DCS_GET_ID1		0xda
#define DCS_GET_ID2		0xdb
#define DCS_GET_ID3		0xdc

#define pw(x...) dsi_vc_dcs_write_nosync(0, (u8[]){x}, sizeof((u8[]){x}))

#define POWER 0
#define GAMMA_ADJUST	1
#if GAMMA_ADJUST
struct gamma_val {
	unsigned char add;
	unsigned char par[9];
};

struct gamma_par {

	struct gamma_val red_p;
	struct gamma_val red_n;
	struct gamma_val green_p;
	struct gamma_val green_n;
	struct gamma_val blue_p;
	struct gamma_val blue_n;
};

#define gpw(x)	pw( x.add, x.par[0], x.par[1], x.par[2], x.par[3], \
		x.par[4], x.par[5], x.par[6], \
		x.par[7], x.par[8])

static struct gamma_par lg_gamma = {
	{ 0xd0,
	{ 0x00, 0x44, 0x74, 0x47, 0x22, 0x12, 0x61, 0x36, 0x05 }},
	{ 0xd1,
	{ 0x00, 0x44, 0x70, 0x47, 0x22, 0x02, 0x61, 0x36, 0x03 }},
	{ 0xd2,
	{ 0x00, 0x44, 0x74, 0x47, 0x22, 0x12, 0x61, 0x46, 0x05 }},
	{ 0xd3,
	{ 0x00, 0x44, 0x70, 0x47, 0x22, 0x02, 0x61, 0x46, 0x03 }},
	{ 0xd4,
	{ 0x00, 0x44, 0x74, 0x47, 0x22, 0x12, 0x61, 0x46, 0x05 }},
	{ 0xd5,
	{ 0x00, 0x44, 0x70, 0x47, 0x22, 0x02, 0x61, 0x46, 0x03 }},
};

#endif
static void gamma_settings(void){
#ifdef GAMMA
	pw( 0xd0, 0x00, 0x44, 0x74, 0x57, 0x15, 0x03, 0x61, 0x46, 0x03 );
	pw( 0xd1, 0x00, 0x44, 0x74, 0x57, 0x15, 0x03, 0x61, 0x46, 0x03 );
	pw( 0xd2, 0x00, 0x44, 0x74, 0x57, 0x15, 0x03, 0x61, 0x46, 0x03 );
	pw( 0xd3, 0x00, 0x44, 0x74, 0x57, 0x15, 0x03, 0x61, 0x46, 0x03 );
	pw( 0xd4, 0x00, 0x44, 0x74, 0x57, 0x15, 0x03, 0x61, 0x46, 0x03 );
	pw( 0xd5, 0x00, 0x44, 0x74, 0x57, 0x15, 0x03, 0x61, 0x46, 0x03 );
#else
#ifdef GAMMA_ADJUST
	gpw( lg_gamma.red_p );
	gpw( lg_gamma.red_n );
	gpw( lg_gamma.green_p );
	gpw( lg_gamma.green_n );
	gpw( lg_gamma.blue_p );
	gpw( lg_gamma.blue_n );
#else
	pw( 0xd0, 0x00, 0x44, 0x44, 0x07, 0x00, 0x12, 0x61, 0x12, 0x05 );
	pw( 0xd1, 0x00, 0x44, 0x44, 0x07, 0x00, 0x02, 0x61, 0x16, 0x03 );
	pw( 0xd2, 0x00, 0x44, 0x44, 0x07, 0x00, 0x12, 0x61, 0x16, 0x05 );
	pw( 0xd3, 0x00, 0x44, 0x44, 0x07, 0x00, 0x02, 0x61, 0x16, 0x03 );
	pw( 0xd4, 0x00, 0x44, 0x44, 0x07, 0x00, 0x12, 0x61, 0x16, 0x05 );
	pw( 0xd5, 0x00, 0x44, 0x44, 0x07, 0x00, 0x02, 0x61, 0x16, 0x03 );
#endif
#endif
}

static void panel_initial_settings(void)
{
	pr_debug("panel_initial_settings\n");

	pw( 0x3a, 0x77 );
	pw( 0x36, 0x00 ); 	/* RGB, no H/V flip */
	
	pw( 0xb2, 0x20, 0xd6 );
	pw( 0xb3, 0x02 );
	
	pw( 0xb4, 0x04 );	/* display mode control "dithering off" */
	pw( 0xb5, 0x10, 0x0f, 0x0f, 0x00, 0x00 );

	/* DISPCTL2: DISPLAY OK!! 
	 * param 1: ASG=1, SDM=1, FHN=1, GSWAP=0, FVST=0 
	 * param 2: CLW=0x15
	 * param 3: GTO=0x02
	 * param 4: GNO=0x0f
	 * param 5: FTI=0x0f
	 * param 6: GPM=0x1f
	 */
	pw( 0xb6, 0x03, 0x15, 0x02, 0x0f, 0x0f, 0x1f );
	
	/* power control */
	pw( 0xc0, 0x01, 0x18 );		/* enable internal osc. */
	pw( 0xc1, 0x00, 0x01 );
	pw( 0xc3, 0x07, 0x04, 0x04, 0x04, 0x07 );
	pw( 0xc4, 0x12, 0x33, 0x1a, 0x1a, 0x07, 0x49 );
	pw( 0xc5, 0x6d );
	pw( 0xc6, 0x44, 0x63, 0x00 );
	
	gamma_settings();
	
}

static void panel_sleep(int enable)
{
	printk("panel_sleep(%i)\n", enable);
	
	if (enable) {
		pw( 0x10 );
		msleep(500);
		pw( 0x28 );
		msleep(500);
	} else {
		pw( 0x11 );
		msleep(500);
		pw( 0x29 );
		msleep(500);
	}
}

struct panel_data {
	unsigned long	hw_guard_end;	/* next value of jiffies when we can
					 * issue the next sleep in/out command
					 */
	unsigned long	hw_guard_wait;	/* max guard time in jiffies */

	struct omap_dss_device *dssdev;

	bool enabled;
	u8 rotate;
	bool mirror;

	bool te_enabled;
	bool use_ext_te;
	struct completion te_completion;

	bool use_dsi_bl;

	bool cabc_broken;
	unsigned cabc_mode;

	bool intro_printed;
};

static void panel_get_timings(struct omap_dss_device *dssdev,
			struct omap_video_timings *timings)
{
	*timings = dssdev->panel.timings;
}

static void panel_set_timings(struct omap_dss_device *dssdev, 
		struct omap_video_timings *timings)
{
	dssdev->panel.timings = *timings;
}

static void panel_get_resolution(struct omap_dss_device *dssdev,
		u16 *xres, u16 *yres)
{
//	struct panel_data *td = dev_get_drvdata(&dssdev->dev);

//	if (td->rotate == 0 || td->rotate == 2) {
		*xres = dssdev->panel.timings.x_res;
		*yres = dssdev->panel.timings.y_res;
//	} else {
//		*yres = dssdev->panel.timings.x_res;
//		*xres = dssdev->panel.timings.y_res;
//	}
}


static void panel_remove(struct omap_dss_device *dssdev)
{
}

static int panel_enable(struct omap_dss_device *disp)
{
	struct panel_data *td = dev_get_drvdata(&disp->dev);
	int ret = 0;
	
	printk("panel enable\n");

	if (disp->platform_enable)
		ret = disp->platform_enable(disp);
	if (ret < 0)
		return ret;
	
	panel_initial_settings();
	panel_sleep(0);

	td->enabled = 1;

	return 0;
}

static void panel_disable(struct omap_dss_device *disp)
{
	struct panel_data *td = dev_get_drvdata(&disp->dev);

	if (disp->platform_disable)
		disp->platform_disable(disp);
	else
		panel_sleep(1);

	td->enabled = 0;

}

static int panel_suspend(struct omap_dss_device *dssdev)
{

	return 0;
}

static int panel_resume(struct omap_dss_device *dssdev)
{

	return 0;
}

static void panel_setup_update(struct omap_dss_device *dssdev,
				    u16 x, u16 y, u16 w, u16 h)
{

}

static int panel_rotate(struct omap_dss_device *dssdev, u8 rotate)
{
	return 0;
}

static u8 panel_get_rotate(struct omap_dss_device *dssdev)
{
	struct panel_data *td = dev_get_drvdata(&dssdev->dev);
	return td->rotate;
}

static int panel_mirror(struct omap_dss_device *dssdev, bool enable)
{

	struct panel_data *td = dev_get_drvdata(&dssdev->dev);
	td->mirror = enable;

	return 0;
}

static bool panel_get_mirror(struct omap_dss_device *dssdev)
{
	struct panel_data *td = dev_get_drvdata(&dssdev->dev);
	return td->mirror;
}

static int panel_probe(struct omap_dss_device *dssdev)
{
	struct panel_data *td;
	int r = 0;

	dev_dbg(&dssdev->dev, "probe\n");

	td = kzalloc(sizeof(*td), GFP_KERNEL);
	if (!td) {
		r = -ENOMEM;
		goto out;
	}

	dev_set_drvdata(&dssdev->dev, td);

	dssdev->get_timings = panel_get_timings;
	dssdev->set_timings = panel_set_timings;
	dssdev->get_resolution = panel_get_resolution;
	
	dssdev->caps = OMAP_DSS_DISPLAY_CAP_VIDEO_MODE;
	
 out:
	return r;
}


static struct omap_dss_driver lg_panel = {

	.driver         = {
		.name   = "lg_fwvga_43",
		.owner  = THIS_MODULE,
	},
	.probe		= panel_probe,
	.remove		= panel_remove,

	.enable		= panel_enable,
	.disable	= panel_disable,
	.suspend	= panel_suspend,
	.resume		= panel_resume,

	.setup_update	= panel_setup_update,
	.set_rotate	= panel_rotate,
	.get_rotate	= panel_get_rotate,
	.set_mirror	= panel_mirror,
	.get_mirror	= panel_get_mirror,
};

static int __init panel_drv_init(void)
{
 	omap_dss_register_driver(&lg_panel);
	return 0;
}

static void __exit panel_drv_exit(void)
{
	omap_dss_unregister_driver(&lg_panel);
}

module_init(panel_drv_init);
module_exit(panel_drv_exit);

MODULE_DESCRIPTION("LG dsi Driver");
MODULE_LICENSE("GPL");
