#define DSS_SUBSYS_NAME "DUMMY"

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/errno.h>

#include <mach/board.h>
#include <mach/display.h>
#include <mach/cpu.h>

#include "dss.h"

struct omap_dss_device *omap_dss_dummy_device;

static void dummy_set_timings(struct omap_dss_device *dssdev,
			struct omap_video_timings *timings);

static int dummy_set_dispc_clk(bool is_tft, unsigned long pck_req,
		unsigned long *fck, int *lck_div, int *pck_div)
{
	return 0;
}

static int dummy_set_mode(struct omap_dss_device *dssdev)
{
	return 0;
}

static int dummy_basic_init(struct omap_dss_device *dssdev)
{
	return 0;
}

static int dummy_enable_display(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = omap_dss_start_device(dssdev);
	if (r) {
		DSSERR("failed to start device\n");
		goto err0;
	}

	if (dssdev->state != OMAP_DSS_DISPLAY_DISABLED) {
		DSSERR("display already enabled\n");
		r = -EINVAL;
		goto err1;
	}

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
err1:
	omap_dss_stop_device(dssdev);
err0:
	return r;
}

static void dummy_disable_display(struct omap_dss_device *dssdev)
{
	DSSDBG("dummy_disable_display\n");
	if (dssdev->state == OMAP_DSS_DISPLAY_DISABLED)
		return;

	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;

	omap_dss_stop_device(dssdev);
}

static int dummy_display_suspend(struct omap_dss_device *dssdev)
{
	if (dssdev->state != OMAP_DSS_DISPLAY_ACTIVE)
		return -EINVAL;

	DSSDBG("dummy_display_suspend\n");

	if (dssdev->driver->suspend)
		dssdev->driver->suspend(dssdev);

	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;

	return 0;
}

static int dummy_display_resume(struct omap_dss_device *dssdev)
{
	int r = 0;
	if (dssdev->state != OMAP_DSS_DISPLAY_SUSPENDED)
		return -EINVAL;

	DSSDBG("dummy_display_resume\n");

	if (dssdev->driver->resume) {
		r = dssdev->driver->resume(dssdev);
		if (r)
			goto err1;
	}

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
err1:
	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;
	return -EINVAL;
}

static void dummy_get_timings(struct omap_dss_device *dssdev,
			struct omap_video_timings *timings)
{
	*timings = dssdev->panel.timings;
}

static void dummy_set_timings(struct omap_dss_device *dssdev,
			struct omap_video_timings *timings)
{
	DSSDBG("dummy_set_timings\n");
	dssdev->panel.timings = *timings;
	if (dssdev->state == OMAP_DSS_DISPLAY_ACTIVE) {
		dummy_set_mode(dssdev);
		dispc_go(OMAP_DSS_CHANNEL_LCD);
	}
}

static int dummy_check_timings(struct omap_dss_device *dssdev,
			struct omap_video_timings *timings)
{
	bool is_tft;
	int r;
	int lck_div, pck_div;
	unsigned long fck;
	unsigned long pck;

	DSSDBG("dummy_check_timings\n");
	if (!dispc_lcd_timings_ok(timings))
		return -EINVAL;

	if (timings->pixel_clock == 0)
		return -EINVAL;

	is_tft = (dssdev->panel.config & OMAP_DSS_LCD_TFT) != 0;

#ifdef CONFIG_OMAP2_DSS_USE_DSI_PLL_FOR_HDMI
	{
		struct dsi_clock_info cinfo;
		r = dsi_pll_calc_pck(is_tft, timings->pixel_clock * 1000,
				&cinfo);

		if (r)
			return r;

		fck = cinfo.dsi1_pll_fclk;
		lck_div = cinfo.lck_div;
		pck_div = cinfo.pck_div;
	}
#else
	{
		struct dispc_clock_info cinfo;
		r = dispc_calc_clock_div(is_tft, timings->pixel_clock * 1000,
				&cinfo);

		if (r)
			return r;

		fck = cinfo.fck;
		lck_div = cinfo.lck_div;
		pck_div = cinfo.pck_div;
	}
#endif

	pck = fck / lck_div / pck_div / 1000;

	timings->pixel_clock = pck;

	return 0;
}

void dummy_exit(void)
{
	omap_dss_dummy_device->driver->remove(omap_dss_dummy_device);
}

int dummy_init_display(struct omap_dss_device *dssdev)
{
	DSSDBG("init_display\n");
	dssdev->enable = dummy_enable_display;
	dssdev->disable = dummy_disable_display;
	dssdev->suspend = dummy_display_suspend;
	dssdev->resume = dummy_display_resume;
	dssdev->get_timings = dummy_get_timings;
	dssdev->set_timings = dummy_set_timings;
	dssdev->check_timings = dummy_check_timings;
	
	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;

	/* store the dss device as we need this to unregister the
	 * dss_dummy driver when exiting
	 */
	omap_dss_dummy_device = dssdev;

	return 0;
}

