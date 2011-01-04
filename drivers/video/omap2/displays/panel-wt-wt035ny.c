/*
 * panel-wt-wt035ny.c
 *
 *  Created on: Sept 24, 2010
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

/* ILI9325 control registers */
#define ILI9325_DRV_CODE			0x00  /* Driver code read, RO.*/
#define ILI9325_DRV_OUTPUT_CTRL_1		0x01  /* Driver output control 1, W.*/
#define ILI9325_LCD_DRV_CTRL			0x02  /* LCD driving control, W.*/
#define ILI9325_ENTRY_MOD			0x03  /* Entry mode, W.*/
#define ILI9325_RESIZE_CTRL			0x04  /* Resize Control, W.*/

#define ILI9325_DIS_CTRL_1			0x07  /* Display Control 1, W.*/
#define ILI9325_DIS_CTRL_2			0x08  /* Display Control 2, W.*/
#define ILI9325_DIS_CTRL_3			0x09  /* Display Control 3, W.*/
#define ILI9325_DIS_CTRL_4			0x0A  /* Display Control 4, W.*/

#define ILI9325_RGB_CTRL_1			0x0C  /* RGB display interface control 1, W.*/
#define ILI9325_FRAME_MARKER_POS		0x0D  /* Frame Marker Position, W.*/
#define ILI9325_RGB_CTRL_2			0x0F  /* RGB display interface control 2, W.*/

#define ILI9325_POW_CTRL_1			0x10  /* Power control 1, W.*/
#define ILI9325_POW_CTRL_2			0x11  /* Power control 2, W.*/
#define ILI9325_POW_CTRL_3			0x12  /* Power control 3, W.*/
#define ILI9325_POW_CTRL_4			0x13  /* Power control 4, W.*/

#define ILI9325_GRAM_HADDR			0x20  /* Horizontal GRAM address set, W.*/
#define ILI9325_GRAM_VADDR			0x21  /* Vertical GRAM address set, W.*/
#define ILI9325_GRAM_DATA			0x22  /* Write Data to GRAM, W.*/

#define ILI9325_POW_CTRL_7			0x29  /* Power control 7, W.*/

#define ILI9325_FRM_RATE_COLOR			0x2B  /* Frame Rate and Color Control, W. */

#define ILI9325_GAMMA_CTRL_1			0x30  /* Gamma Control 1, W. */
#define ILI9325_GAMMA_CTRL_2			0x31  /* Gamma Control 2, W. */
#define ILI9325_GAMMA_CTRL_3			0x32  /* Gamma Control 3, W. */
#define ILI9325_GAMMA_CTRL_4			0x35  /* Gamma Control 4, W. */
#define ILI9325_GAMMA_CTRL_5			0x36  /* Gamma Control 5, W. */
#define ILI9325_GAMMA_CTRL_6			0x37  /* Gamma Control 6, W. */
#define ILI9325_GAMMA_CTRL_7			0x38  /* Gamma Control 7, W. */
#define ILI9325_GAMMA_CTRL_8			0x39  /* Gamma Control 8, W. */
#define ILI9325_GAMMA_CTRL_9			0x3C  /* Gamma Control 9, W. */
#define ILI9325_GAMMA_CTRL_10			0x3D  /* Gamma Control l0, W. */

#define ILI9325_HOR_ADDR_START			0x50  /* Horizontal Address Start, W. */
#define ILI9325_HOR_ADDR_END			0x51  /* Horizontal Address End Position, W. */
#define ILI9325_VET_ADDR_START			0x52  /* Vertical Address Start, W. */
#define ILI9325_VET_ADDR_END			0x53  /* Vertical Address Start, W. */

#define ILI9325_DRV_OUTPUT_CTRL_2		0x60  /* Driver output control 2, W.*/

#define ILI9325_BASE_IMG_CTRL			0x61  /* Base Image Display Control, W.*/
#define ILI9325_VSCROLL_CTRL			0x6A  /* Vertical Scroll Control, W.*/

#define ILI9325_PAR_IMG1_POS			0x80  /* Partial Image 1 Display Position, W.*/
#define ILI9325_PAR_IMG1_START			0x81  /* Partial Image 1 Area (Start Line), W.*/
#define ILI9325_PAR_IMG1_END			0x82  /* Partial Image 1 Area (End Line), W.*/
#define ILI9325_PAR_IMG2_POS			0x83  /* Partial Image 2 Display Position, W.*/
#define ILI9325_PAR_IMG2_START			0x84  /* Partial Image 2 Area (Start Line), W.*/
#define ILI9325_PAR_IMG2_END			0x85  /* Partial Image 2 Area (End Line), W.*/

#define ILI9325_PAN_CTRL_1			0x90  /* Panel Interface Control 1, W.*/
#define ILI9325_PAN_CTRL_2			0x92  /* Panel Interface Control 2, W.*/
#define ILI9325_PAN_CTRL_3			0x93  /* Panel Interface Control 3, W.*/
#define ILI9325_PAN_CTRL_4			0x95  /* Panel Interface Control 4, W.*/


#define ILI9325_OTP_PROG_CTRL			0xA1  /* OTP VCM Programming Control, W.*/
#define ILI9325_OTP_STATUS			0xA2  /* OTP VCM Status and Enable, W.*/
#define ILI9325_OTP_ID_KEY			0xA5  /* OTP Programming ID Key, W.*/

/* Registers not depicted on the datasheet :-( */
#define ILI9325_TIMING_CTRL_1			0xE3  /* Timing control. */
#define ILI9325_TIMING_CTRL_2			0xE7  /* Timing control. */
#define ILI9325_TIMING_CTRL_3			0xEF  /* Timing control. */

#define ILI9325_PAN_CTRL_5			0x97  /* Panel Interface Control 5, W.*/
#define ILI9325_PAN_CTRL_6			0x98  /* Panel Interface Control 6, W.*/


// disp ctrl1 bits
#define D0 1<<0
#define D1 1<<1
#define CL 1<<2
#define DTE 1<<4
#define GON 1<<5
#define BASEE 1<<8
#define PTDE0 1<<12
#define PTDE1 1<<13

// pw ctrl1 bits
#define STB 1<<0
#define SLP 1<<1
#define DSTB 1<<2
#define AP0 1<<4
#define AP1 1<<5
#define AP2 1<<6
#define APE 1<<7
#define BT0 1<<8
#define BT1 1<<9
#define BT2 1<<10
#define SAP 1<<12


// pw ctrl2 bits
#define VC0 1<<0
#define VC1 1<<1
#define VC2 1<<2
#define DC00 1<<4
#define DC01 1<<5
#define DC02 1<<6
#define DC10 1<<8
#define DC11 1<<9
#define DC12 1<<10

// pw ctrl3 bits
#define VRH0 1<<0
#define VRH1 1<<1
#define VRH2 1<<2
#define VRH3 1<<3
#define PON 1<<4
#define VCIRE 1<<7

static struct display_device *display_dev;

static inline void WriteCommand_Addr(u16 index)
{
        omap_rfbi_write_command(&index, 2);
}

static inline void WriteCommand_Data(u16 val)
{
        omap_rfbi_write_data(&val, 2);
}

/* Adjust the Gamma Curve  */
static void ili9481_gamma_adjust(void)
{
	WriteCommand_Addr(0xC8);	// Gamma Setting
	WriteCommand_Data(0x00);	// KP1[2:0],KP0[2:0]
	WriteCommand_Data(0x45);	// KP3[2:0],KP2[2:0]
	WriteCommand_Data(0x22);	// KP5[2:0],KP4[2:0]
	WriteCommand_Data(0x11);	// RP1[2:0],RP0[2:0]
	WriteCommand_Data(0x08);	// VRP0[3:0]
	WriteCommand_Data(0x00);	// VRP1[4:0]
	WriteCommand_Data(0x55);	// KN1[2:0],KN0[2:0]
	WriteCommand_Data(0x13);	// KN3[2:0],KN2[2:0]
	WriteCommand_Data(0x77);	// KN5[2:0],KN4[2:0]
	WriteCommand_Data(0x11);	// RN1[2:0],RN0[2:0]
	WriteCommand_Data(0x08);	// VRN0[3:0]
	WriteCommand_Data(0x00);	// VRN1[4:0]
}  

/* Start Initial Sequence */
static void ili9481_chip_init(struct display_device *dev)
{  
	pr_debug("ili9481_chip_init.\n");

	WriteCommand_Addr(0x11);	// exit sleep mode
	msleep(20);
	WriteCommand_Addr(0xD0);	// power settings
	WriteCommand_Data(0x07);	// VC[2:0]
	WriteCommand_Data(0x41);	// PON,BT[2:0]
	WriteCommand_Data(0x05);	//11 VCIRE,VRH[3:0]

	WriteCommand_Addr(0xD1);	// VCOM control
	WriteCommand_Data(0x00);	// SELVCM
	WriteCommand_Data(0x24);	// VCM[5:0]
	WriteCommand_Data(0x11);	// VDV[4:0]

	WriteCommand_Addr(0xD2);	// power for normal mode
	WriteCommand_Data(0x01);	// AP0[2:0]
	WriteCommand_Data(0x11);	// DC10[2:0],DC00[2:0]

	WriteCommand_Addr(0xC0);	// Panel settings
	WriteCommand_Data(0x10);	// REV & SM & GS
	WriteCommand_Data(0x3B);	// NL[5:0] 480 lignes
	WriteCommand_Data(0x00);	// SCN[6:0]
	WriteCommand_Data(0x02);	// NDL , PTS[2:0]
	WriteCommand_Data(0x11);	// PTG , ISC[3:0]

	WriteCommand_Addr(0xC5);	// Frame rate
	WriteCommand_Data(0x02);	// 85 Hz

	WriteCommand_Addr(0xC6);	// interface control
	WriteCommand_Data(0x13);	

	ili9481_gamma_adjust();

	WriteCommand_Addr(0xE4);
	WriteCommand_Data(0xA0);

	WriteCommand_Addr(0xF0);
	WriteCommand_Data(0x01);

	WriteCommand_Addr(0xF3);
	WriteCommand_Data(0x20);	//40
	WriteCommand_Data(0x0F);	//pre buffer

	WriteCommand_Addr(0xF7);
	WriteCommand_Data(0x80);

	WriteCommand_Addr(0x36);	// Set_address_mode
	WriteCommand_Data(0x8b);	// 83Bit3: RGB/BGR

	msleep(120);

	WriteCommand_Addr(0x3A);	// data format
	WriteCommand_Data(0x66);	// 18 bit

	WriteCommand_Addr(0x29);	// set display on

	WriteCommand_Addr(0x2A);	// Page Address Set
	WriteCommand_Data(0x00);	// from 
	WriteCommand_Data(0x18);
	WriteCommand_Data(0x01);	// to
	WriteCommand_Data(0x27);

	WriteCommand_Addr(0x2B);	//Column Address Set
	WriteCommand_Data(0x00);	// from 0
	WriteCommand_Data(0x00);
	WriteCommand_Data(0x01);	// to 479
	WriteCommand_Data(0xDF);

	WriteCommand_Addr(0xb3);	// memory access
	WriteCommand_Data(0x00);	// 
	WriteCommand_Data(0x00);
	WriteCommand_Data(0x00);	//
	WriteCommand_Data(0x00);	// dfm = 0

	WriteCommand_Addr(0x2c);	// start transfert image

}

static void panel_remove(struct omap_dss_device *disp)
{
}

static int panel_enable(struct omap_dss_device *disp)
{
	int ret = 0;

	pr_debug("[WT035] call panel enable\n");

	if (disp->platform_enable)
		ret = disp->platform_enable(disp);
	if (ret < 0)
		return ret;

	ili9481_chip_init(display_dev);		//

	return ret;
}

static void panel_disable(struct omap_dss_device *disp)
{
	pr_debug("[WT035] call panel disable\n");

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

static int panel_probe(struct omap_dss_device *dssdev)
{
	return 0;
}
static int panel_enable_te(struct omap_dss_device *display, bool enable)
{
	return 0;
}

static int panel_rotate(struct omap_dss_device *display, u8 rotate)
{
	return 0;
}

static int panel_mirror(struct omap_dss_device *display, bool enable)
{
	return 0;
}

static int panel_run_test(struct omap_dss_device *display, int test_num)
{
	return 0;
}

static void panel_setup_update(struct omap_dss_device *display,
				    u16 x, u16 y, u16 w, u16 h)
{
	WriteCommand_Addr(0x2c);	// start transfert image
}

static struct omap_dss_driver mt_panel = {

	.driver		= {
		.name	= "wt_lcd_35",
		.owner 	= THIS_MODULE,
	},
	.probe		= panel_probe,
	.remove		= panel_remove,
	.enable		= panel_enable,
	.disable	= panel_disable,
	.suspend	= panel_suspend,
	.resume		= panel_resume,
	.setup_update	= panel_setup_update,
	.enable_te	= panel_enable_te,
	.set_rotate	= panel_rotate,
	.set_mirror	= panel_mirror,
	.run_test	= panel_run_test,

};

static int __init panel_drv_init(void)
{
	return omap_dss_register_driver(&mt_panel);
}

static void __exit panel_drv_exit(void)
{
	omap_dss_unregister_driver(&mt_panel);
}

module_init(panel_drv_init);
module_exit(panel_drv_exit);
