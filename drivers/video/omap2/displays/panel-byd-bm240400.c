/*
 * panel-byd-bm240400.c
 *
 *  Created on: Jan 13, 2010
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
#include <linux/hx8352.h>

/*
 * Panel SPI driver
 */
struct hx8352_spi_t {
	struct spi_device	*spi;
	struct spi_driver	driver;
	struct spi_transfer	t[2];
	struct spi_message	msg;
	unsigned short 	dummy;
};

static struct hx8352_spi_t *hx8352_spi;

#define HX_SET( x, y) hx8352_spi_reg_write(x, y)

static int hx8352_spi_reg_write( unsigned int addr, unsigned int value) 
{
	unsigned char data[3];
	
	//pr_debug("spi_reg_write: addr=%d, val=%02X\n", addr, value);

	/* Now we prepare the command for transferring */
	data[0] = HX8352_ID;
	data[1] = (unsigned char) addr;

	spi_message_init(&hx8352_spi->msg);

	memset(&hx8352_spi->t, 0, sizeof(hx8352_spi->t));
	hx8352_spi->t[0].tx_buf = data;
	hx8352_spi->t[0].rx_buf = NULL;
	hx8352_spi->t[0].len = 2;
	spi_message_add_tail(&hx8352_spi->t[0], &hx8352_spi->msg);
	spi_sync( hx8352_spi->spi, &hx8352_spi->msg);


	data[0] = HX8352_ID | HX8552_RS;
	data[1] = (unsigned char) value;

	spi_message_init(&hx8352_spi->msg);

	memset(&hx8352_spi->t, 0, sizeof(hx8352_spi->t));
	hx8352_spi->t[0].tx_buf = data;
	hx8352_spi->t[0].rx_buf = NULL;
	hx8352_spi->t[0].len = 2;
	spi_message_add_tail(&hx8352_spi->t[0], &hx8352_spi->msg);
	spi_sync( hx8352_spi->spi, &hx8352_spi->msg);

	return 0;
}

// Configurations tables
// ref BYD provisory doc module BM240400-8578FTGB file FMLCD-MA-000010-10A
/*
static void hx8352_set_window( int x_start, int x_end, int y_start, int y_end)
{ 

	HX_SET( HX8352_COLSTART, (unsigned char)(x_start >> 8));
	HX_SET( HX8352_COLSTART1, (unsigned char)(x_start));
	HX_SET( HX8352_COLSTOP, (unsigned char)(x_end >> 8));
	HX_SET( HX8352_COLSTOP1, (unsigned char)(x_end));
	HX_SET( HX8352_ROWSTART, (unsigned char)(y_start >> 8));
	HX_SET( HX8352_ROWSTART1, (unsigned char)(y_start));
	HX_SET( HX8352_ROWSTOP, (unsigned char)(y_end >> 8));
	HX_SET( HX8352_ROWSTOP1, (unsigned char)(y_end));
}
*/

static void hx8352_exit_standby( void)
{ 

	// power supply settings
	HX_SET( HX8352_OSC1_CTRL, 0x91 );	// RADJ = 100, osc_en = 1
	HX_SET( HX8352_CYC_CTRL1, 0xf9 );	// n_dcdc  = f9
	// add delay 10ms
	msleep(10);
	HX_SET( HX8352_PW3_CTRL, 0x14 );	// bt = 001, ap = 100
	HX_SET( HX8352_PW2_CTRL, 0x11 );	// vc3 = 001, vc1 = 001
	HX_SET( HX8352_PW4_CTRL, 0x0d );	// vrh = 1101
	HX_SET( HX8352_VCOM, 0x42 );		// vcm = 100 0010
	// add delay 20ms
	msleep(20);
	HX_SET( HX8352_PW1_CTRL, 0x0a );	// dk = 1, tri = 1
	HX_SET( HX8352_PW1_CTRL, 0x1a );	// pon = 1 dk = 1, tri = 1
	// add delay 40ms
	msleep(40);
	HX_SET( HX8352_PW1_CTRL, 0x12 );	// pon = 1 dk = 0, tri = 1
	// add delay 40ms
	msleep(40);
	HX_SET( HX8352_PW6_CTRL, 0x2c );	// vcomg = 1, vdv = 0 1100
	// add delay 100ms
	msleep(100);

	//display on settings
	HX_SET( HX8352_SRC_CTRL1, 0x60 );	// n sap = 0110000
	HX_SET( HX8352_SRC_CTRL2, 0x40 );	// i sap = 01000000
	HX_SET( HX8352_CYC_CTRL10, 0x38 );	// eqs = 00111000
	HX_SET( HX8352_CYC_CTRL11, 0x38 );	// eqp = 00111000
	HX_SET( HX8352_DISP_CTRL2, 0x38 );	// pt = 00, gon = 1, dte = 1, d = 10
	// add delay 40ms
	msleep(40);
	HX_SET( HX8352_DISP_CTRL2, 0x3c );	// pt = 00, gon = 1, dte = 1, d = 11
	HX_SET( HX8352_MEM_CTRL, 0xc8 );	// my = 1, mx = 1, mv = 0, bgr = 1, ss = 0, srl sm = 0
	HX_SET( HX8352_DISPMODE, 0x02 );	// norno = 1
	HX_SET( HX8352_PANEL_CTRL, 0x0c );	//  ss=1 gs = 1 inverted and vertical switch


}

static void hx8352_initial_settings( void)
{ 

	HX_SET( HX8352_TEST, 0x02 );		// testm = 1
	HX_SET( HX8352_VDD_CTRL, 0x03 );	// vdc_sel = 11
	HX_SET( HX8352_GAMMAR2, 0x93 );		// regs_vgs = 1
	HX_SET( HX8352_SYNC, 0x01 );		// sync = 1
	HX_SET( HX8352_TEST, 0x00 );		// testm = 0
#if 0
	// gamma settings 
	HX_SET( HX8352_GAMMA1, 0xb0 );
	HX_SET( HX8352_GAMMA2, 0x03 );
	HX_SET( HX8352_GAMMA3, 0x10 );
	HX_SET( HX8352_GAMMA4, 0x56 );
	HX_SET( HX8352_GAMMA5, 0x13 );
	HX_SET( HX8352_GAMMA6, 0x46 );
	HX_SET( HX8352_GAMMA7, 0x23 );
	HX_SET( HX8352_GAMMA8, 0x76 );
	HX_SET( HX8352_GAMMA9, 0x00 );
	HX_SET( HX8352_GAMMA10, 0x5e );
	HX_SET( HX8352_GAMMA11, 0x4f );
	HX_SET( HX8352_GAMMA12, 0x40 );
#else
	// new gamma settings, to be near A5x visual effect
	HX_SET( HX8352_GAMMA1, 0xa0 );	// high than 90 for stability
	HX_SET( HX8352_GAMMA2, 0x02 );
	HX_SET( HX8352_GAMMA3, 0x10 );
	HX_SET( HX8352_GAMMA4, 0x56 );
	HX_SET( HX8352_GAMMA5, 0x17 );
	HX_SET( HX8352_GAMMA6, 0x46 );
	HX_SET( HX8352_GAMMA7, 0x23 );
	HX_SET( HX8352_GAMMA8, 0x76 );
	HX_SET( HX8352_GAMMA9, 0x00 );
	HX_SET( HX8352_GAMMA10, 0x54 );
	HX_SET( HX8352_GAMMA11, 0x4c );
	HX_SET( HX8352_GAMMA12, 0x40 );
#endif
	HX_SET( HX8352_IF_CTRL1, 0xc8 ); // csel 18 bit, epl = 0, vspl=0, hspl = 0, dpl = 1 

	hx8352_exit_standby();
}

/*
static void hx8352_go_standby( void)
{ 

	HX_SET( HX8352_DISP_CTRL2, 0x38 );	// pt = 0, gon = 1, dte = 1, d = 10
	// add delay 40ms
	msleep(40);
	HX_SET( HX8352_DISP_CTRL2, 0x28 );	// pt = 0, gon = 1, dte = 0, d = 10
	// add delay 40ms
	msleep(40);
	HX_SET( HX8352_DISP_CTRL2, 0x20 );	// pt = 0, gon = 1, dte = 0, d = 00

	HX_SET( HX8352_SRC_CTRL1, 0x00 );			// n_sap = 0
	HX_SET( HX8352_PW3_CTRL, 0x10 );			// bt = 1, ap = 0
	HX_SET( HX8352_PW1_CTRL, 0x0a );	// pon = 0, dk = 1
	HX_SET( HX8352_PW6_CTRL, 0x00 );
	HX_SET( HX8352_PW1_CTRL, 0x01 );	// stb = 1
	HX_SET( HX8352_OSC1_CTRL, 0x00 );			// osc_en = 0
}
*/

static int __devinit hx8352_probe( struct spi_device *spi )
{
	int r;

	pr_debug("hx8352_probe bus %x\n", spi->master->bus_num);

	hx8352_spi = kzalloc( sizeof(*hx8352_spi), GFP_KERNEL );
	if (hx8352_spi == NULL) {
		dev_err(&spi->dev, "out of mem\n");
		return -ENOMEM;
	}

	spi_set_drvdata( spi, hx8352_spi);
	hx8352_spi->spi = spi;

	hx8352_spi->spi->mode = SPI_MODE_3;
	hx8352_spi->spi->bits_per_word = 8;
	if ((r = spi_setup( hx8352_spi->spi )) < 0) {
		dev_err(&spi->dev, "SPI setup failed\n");
		goto err;
	}

	dev_info(&spi->dev, "initialized\n");

	return 0;
err:
	kfree( hx8352_spi );
	return r;
}

static int hx8352_remove(struct spi_device *spi)
{
	kfree( hx8352_spi );
	return 0;
}


static struct spi_driver hx8352_driver = {
	.driver = {
		.name	= "hx8352",
		.owner	= THIS_MODULE,
	},
	.probe =	hx8352_probe,
	.remove =	__devexit_p(hx8352_remove),
};

/*
 * END Panel SPI driver
 */

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
	
	hx8352_initial_settings();

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


// timings references to define, waiting for visual  !!!
#define BYD_LCD_PIXCLOCK	10000		/* 10 MHz */

static struct omap_video_timings byd_panel_timings = {
		.x_res		= 240,
		.y_res		= 400,

		.pixel_clock	= BYD_LCD_PIXCLOCK,	/* Mhz */
// 60hz
		.hsw		= 4,		/* horizontal sync pulse width */
		.hfp		= 4+44,		/* horizontal front porch */
		.hbp		= 8+44,		/* horizontal back porch */
		.vsw		= 4,		/* vertical sync pulse width */
		.vfp		= 4,		/* vertical front porch */
		.vbp		= 84,		/* vertical back porch */
};

static int panel_probe(struct omap_dss_device *dssdev)
{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
		OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_RF | OMAP_DSS_LCD_ONOFF,
	dssdev->panel.timings = byd_panel_timings;
	dssdev->panel.recommended_bpp = 24;

	return 0;
}


static struct omap_dss_driver byd_panel = {

	.driver		= {
		.name	= "byd_wqvga_32",
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
        int err;
	printk( "panel bm240400 init\n");

        err = spi_register_driver( &hx8352_driver );
        if (err < 0)
                pr_err("Failed to register hx8352 client.\n");

	omap_dss_register_driver(&byd_panel);
	return 0;
}

static void __exit panel_drv_exit(void)
{
	omap_dss_unregister_driver(&byd_panel);
        spi_unregister_driver(&hx8352_driver);
}

module_init(panel_drv_init);
module_exit(panel_drv_exit);
