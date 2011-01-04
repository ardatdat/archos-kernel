/*
 * panel-mt-mtf0280.c
 *
 *  Created on: MAr 16, 2010
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

static inline void mtf_write(u8 index, u16 val, int len)
{
        omap_rfbi_write_command(&index, 2);
        omap_rfbi_write_data(&val, len<<1);
}

static inline void mtf_read(u8 index, u16 *buf, int len)
{
	omap_rfbi_write_command(&index, 2);
	omap_rfbi_read_data(buf, len<<1);
}

static inline int ili932x_read_reg(void *dev, u8 reg)  
{  
	u16 data;
	mtf_read(reg, &data, 1);
	return data;
}  

static inline int ili932x_write_reg(void *dev, u8 reg, u16 data)  
{  
	BUG_ON(reg > ILI9325_TIMING_CTRL_3);
	mtf_write(reg, data, 1);
	return 0;
}

static inline void ILI9325_SET_INDEX(struct display_device *dev, u8 reg)
{
	u16 index = (u16) reg;
        omap_rfbi_write_command(&index, 2);
}

/* Power On sequence */
static void ili9325_pow_on(struct display_device *dev)
{
	ili932x_write_reg(dev, ILI9325_POW_CTRL_1, 0x0000); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
	ili932x_write_reg(dev, ILI9325_POW_CTRL_2, 0x0007); /* DC1[2:0], DC0[2:0], VC[2:0]  */
	ili932x_write_reg(dev, ILI9325_POW_CTRL_3, 0x0000); /* VREG1OUT voltage  */
	ili932x_write_reg(dev, ILI9325_POW_CTRL_4, 0x0000); /* VDV[4:0] for VCOM amplitude  */
	msleep(200); /* Dis-charge capacitor power voltage */

	ili932x_write_reg(dev, ILI9325_POW_CTRL_1, 0x1690); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
	ili932x_write_reg(dev, ILI9325_POW_CTRL_2, 0x0227); /* R11h=0x0221 at VCI=3.3V, DC1[2:0], DC0[2:0], VC[2:0]  */
	msleep(50);

	ili932x_write_reg(dev, ILI9325_POW_CTRL_3, 0x001D); /* External reference voltage= Vci  */
	msleep(50);

	ili932x_write_reg(dev, ILI9325_POW_CTRL_4, 0x0800); /* R13=1D00 when R12=009D;VDV[4:0] for VCOM amplitude  */
	ili932x_write_reg(dev, ILI9325_POW_CTRL_7, 0x0012); /* R29=0013 when R12=009D;VCM[5:0] for VCOMH  */
	ili932x_write_reg(dev, ILI9325_FRM_RATE_COLOR, 0x000B); /* Frame Rate = 70Hz  */
	msleep(50);

    return;
}

/* Adjust the Gamma Curve  */
static void ili9325_gamma_adjust(struct display_device *dev)
{
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_1, 0x0007);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_2, 0x0707);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_3, 0x0006);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_4, 0x0704);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_5, 0x1F04);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_6, 0x0004);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_7, 0x0000);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_8, 0x0706);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_9, 0x0701);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_10, 0x000F);
}

/*  Partial Display Control */  
static void ili9325_par_dis_ctrl(struct display_device *dev)  
{  
	ili932x_write_reg(dev, ILI9325_PAR_IMG1_POS, 0);
	ili932x_write_reg(dev, ILI9325_PAR_IMG1_START, 0);
	ili932x_write_reg(dev, ILI9325_PAR_IMG1_END, 0);
	ili932x_write_reg(dev, ILI9325_PAR_IMG2_POS, 0);
	ili932x_write_reg(dev, ILI9325_PAR_IMG2_START, 0);
	ili932x_write_reg(dev, ILI9325_PAR_IMG2_END, 0);
}

/* Panel Control */
static void ili9325_pan_ctrl(struct display_device *dev)  
{
	ili932x_write_reg(dev, ILI9325_PAN_CTRL_1, 0x0010);
	ili932x_write_reg(dev, ILI9325_PAN_CTRL_2, 0x0000);
	ili932x_write_reg(dev, ILI9325_PAN_CTRL_3, 0x0003);
	ili932x_write_reg(dev, ILI9325_PAN_CTRL_4, 0x0110);
	ili932x_write_reg(dev, ILI9325_PAN_CTRL_5, 0x0000);
	ili932x_write_reg(dev, ILI9325_PAN_CTRL_6, 0x0000);
	ili932x_write_reg(dev, ILI9325_DIS_CTRL_1, 0x0133); /* 262K color and display ON */
}

/* Start Initial Sequence */
static void ili9325_chip_init(struct display_device *dev)
{
	pr_debug("ili9325_chip_init.\n");

	ili932x_write_reg(dev, ILI9325_DRV_OUTPUT_CTRL_1, 0x0100); /* set SS and SM bit */
	ili932x_write_reg(dev, ILI9325_LCD_DRV_CTRL, 0x0700); /* set 1 line inversion */
	// Set it early, suppress lcd bad start sometimes
	ili9325_pow_on(dev);  

	ili932x_write_reg(dev, ILI9325_ENTRY_MOD, 0x9030); /* 18bit mode, set GRAM write direction and BGR=1. */

	ili932x_write_reg(dev, ILI9325_RESIZE_CTRL, 0x0000); /* Resize register. */
	ili932x_write_reg(dev, ILI9325_DIS_CTRL_2, 0x0207); /* set the back porch and front porch. */
	ili932x_write_reg(dev, ILI9325_DIS_CTRL_3, 0x0000); /* set non-display area refresh cycle ISC[3:0]. */
	ili932x_write_reg(dev, ILI9325_DIS_CTRL_4, 0x0000); /* FMARK function. */
	ili932x_write_reg(dev, ILI9325_RGB_CTRL_1, 0x0000); /* RGB interface setting. */
	ili932x_write_reg(dev, ILI9325_FRAME_MARKER_POS, 0x0000); /* Frame marker Position. */
	ili932x_write_reg(dev, ILI9325_RGB_CTRL_2, 0x0000); /* RGB interface polarity. */

	ili9325_gamma_adjust(dev);

	/* Set GRAM area */
	ili932x_write_reg(dev, ILI9325_HOR_ADDR_START, 0); /* Horizontal GRAM Start Address. */
	ili932x_write_reg(dev, ILI9325_HOR_ADDR_END, 239); /* Horizontal GRAM End Address. */
	ili932x_write_reg(dev, ILI9325_VET_ADDR_START, 0); /* Vertical GRAM Start Address. */
	ili932x_write_reg(dev, ILI9325_VET_ADDR_END, 319); /* Vertical GRAM End Address. */
	ili932x_write_reg(dev, ILI9325_DRV_OUTPUT_CTRL_2, 0xA700); /* Gate Scan Line. */
	ili932x_write_reg(dev, ILI9325_BASE_IMG_CTRL, 0x0001); /* NDL,VLE, REV. */
	ili932x_write_reg(dev, ILI9325_VSCROLL_CTRL, 0x0000); /* set scrolling line. */

	ili9325_par_dis_ctrl(dev);
	ili9325_pan_ctrl(dev);
	msleep(20);

	ili932x_write_reg(dev, ILI9325_GRAM_HADDR, 0x0000); /* GRAM horizontal Address. */
	ili932x_write_reg(dev, ILI9325_GRAM_VADDR, 0x0000); /* GRAM Vertical Address. */

	ILI9325_SET_INDEX(dev, ILI9325_GRAM_DATA);
}  

#define CONFIG_9331
#ifdef CONFIG_9331
/* Panel Control */  
static void ili9331_pan_ctrl(struct display_device *dev)
{
	ili932x_write_reg(dev, ILI9325_PAN_CTRL_1, 0x0010);
	ili932x_write_reg(dev, ILI9325_PAN_CTRL_2, 0x0600);
	ili932x_write_reg(dev, ILI9325_DIS_CTRL_1, 0x0133); /* 262K color and display ON */
}  

/* Adjust the Gamma Curve  */
static void ili9331_gamma_adjust(struct display_device *dev)
{
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_1, 0x0000);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_2, 0x0106);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_3, 0x0000);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_4, 0x0204);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_5, 0x160a);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_6, 0x0707);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_7, 0x0106);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_8, 0x0707);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_9, 0x0402);
	ili932x_write_reg(dev, ILI9325_GAMMA_CTRL_10, 0x0c0F);
}  

/* Power On sequence */
static void ili9331_pow_on(struct display_device *dev)
{
	ili932x_write_reg(dev, ILI9325_POW_CTRL_1, 0x0000); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
	ili932x_write_reg(dev, ILI9325_POW_CTRL_2, 0x0007); /* DC1[2:0], DC0[2:0], VC[2:0]  */
	ili932x_write_reg(dev, ILI9325_POW_CTRL_3, 0x0000); /* VREG1OUT voltage  */
	 ili932x_write_reg(dev, ILI9325_POW_CTRL_4, 0x0000); /* VDV[4:0] for VCOM amplitude  */
	 msleep(200); /* Dis-charge capacitor power voltage */
 
	ili932x_write_reg(dev, ILI9325_POW_CTRL_1, 0x1690); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
	ili932x_write_reg(dev, ILI9325_POW_CTRL_2, 0x0227); /* R11h=0x0221 at VCI=3.3V, DC1[2:0], DC0[2:0], VC[2:0]  */
	msleep(50);

	ili932x_write_reg(dev, ILI9325_POW_CTRL_3, 0x000c); /* External reference voltage= Vci  */
	msleep(50);

	ili932x_write_reg(dev, ILI9325_POW_CTRL_4, 0x0800); /* R13=1D00 when R12=009D;VDV[4:0] for VCOM amplitude  */
	ili932x_write_reg(dev, ILI9325_POW_CTRL_7, 0x0011); /* R29=0013 when R12=009D;VCM[5:0] for VCOMH  */
	ili932x_write_reg(dev, ILI9325_FRM_RATE_COLOR, 0x000B); /* Frame Rate = 70Hz  */
	 msleep(50);

    return;
}


/* Start Initial Sequence */
static void ili9331_chip_init(struct display_device *dev)
{  
	pr_debug("ili9331_chip_init.\n");
	ili932x_write_reg(dev, ILI9325_TIMING_CTRL_2, 0x1014); /* Set internal timing*/


	ili932x_write_reg(dev, ILI9325_DRV_OUTPUT_CTRL_1, 0x0100); /* set SS and SM bit */
	ili932x_write_reg(dev, ILI9325_LCD_DRV_CTRL, 0x0200); /* set 1 line inversion */
	ili9331_pow_on(dev);

// ESIND says 0x1030 but bad image, setting bit 15 make it works
// revert id to make image good
	ili932x_write_reg(dev, ILI9325_ENTRY_MOD, 0x9010); /* 18bit mode, set GRAM write direction and BGR=1. */

	ili932x_write_reg(dev, ILI9325_RESIZE_CTRL, 0x0000); /* Resize register. */
	ili932x_write_reg(dev, ILI9325_DIS_CTRL_2, 0x0202); /* set the back porch and front porch. */
	ili932x_write_reg(dev, ILI9325_DIS_CTRL_3, 0x0000); /* set non-display area refresh cycle ISC[3:0]. */
	ili932x_write_reg(dev, ILI9325_DIS_CTRL_4, 0x0000); /* FMARK function. */
	ili932x_write_reg(dev, ILI9325_RGB_CTRL_1, 0x0000); /* RGB interface setting. */
	ili932x_write_reg(dev, ILI9325_FRAME_MARKER_POS, 0x0000); /* Frame marker Position. */
	ili932x_write_reg(dev, ILI9325_RGB_CTRL_2, 0x0000); /* RGB interface polarity. */

	ili932x_write_reg(dev, ILI9325_GRAM_HADDR, 0x0000); /* GRAM horizontal Address. */
	ili932x_write_reg(dev, ILI9325_GRAM_VADDR, 0x0000); /* GRAM Vertical Address. */

	ili9331_gamma_adjust(dev);  

	/* Set GRAM area */  
	ili932x_write_reg(dev, ILI9325_HOR_ADDR_START, 0); /* Horizontal GRAM Start Address. */
	ili932x_write_reg(dev, ILI9325_HOR_ADDR_END, 239); /* Horizontal GRAM End Address. */
	ili932x_write_reg(dev, ILI9325_VET_ADDR_START, 0); /* Vertical GRAM Start Address. */
	ili932x_write_reg(dev, ILI9325_VET_ADDR_END, 319); /* Vertical GRAM End Address. */
	ili932x_write_reg(dev, ILI9325_DRV_OUTPUT_CTRL_2, 0xA700); /* Gate Scan Line. */
	ili932x_write_reg(dev, ILI9325_BASE_IMG_CTRL, 0x0001); /* NDL,VLE, REV. */

	ili9325_par_dis_ctrl(dev);
	ili9331_pan_ctrl(dev);
	msleep(20);

	ILI9325_SET_INDEX(dev, ILI9325_GRAM_DATA);
}
#endif

#define CONFIG_7781
#ifdef CONFIG_7781
/* Panel Control */  
static void st7781_pan_ctrl(struct display_device *dev)
{
	ili932x_write_reg(dev, ILI9325_PAN_CTRL_1, 0x0033);
	ili932x_write_reg(dev, ILI9325_DIS_CTRL_1, 0x0133); /* 262K color and display ON */
}  

/* Adjust the Gamma Curve  */
static void st7781_gamma_adjust(struct display_device *dev)
{
	ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_1, 0x0000);
        ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_2, 0x0507);
        ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_3, 0x0003);

        ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_4, 0x0200);
        ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_5, 0x0106);
        ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_6, 0x0000);
        ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_7, 0x0507);
        ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_8, 0x0104);

        ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_9, 0x0200);
        ili932x_write_reg(dev,ILI9325_GAMMA_CTRL_10, 0x0005);
}  

/* Power On sequence */
static void st7781_pow_on(struct display_device *dev)
{
	ili932x_write_reg(dev,ILI9325_POW_CTRL_1, 0x0790);
        ili932x_write_reg(dev,ILI9325_POW_CTRL_2, 0x0005);
	ili932x_write_reg(dev,ILI9325_POW_CTRL_3, 0x0000);
	ili932x_write_reg(dev,ILI9325_POW_CTRL_4, 0x0000);
	msleep(200); /* Dis-charge capacitor power voltage */
 
	ili932x_write_reg(dev,ILI9325_POW_CTRL_1, 0x14B0);
	ili932x_write_reg(dev,ILI9325_POW_CTRL_2, 0x0007);
	msleep(50);

	ili932x_write_reg(dev,ILI9325_POW_CTRL_3, 0x008c);
	msleep(50);

	ili932x_write_reg(dev,ILI9325_POW_CTRL_4, 0x1700);
	ili932x_write_reg(dev,ILI9325_POW_CTRL_7, 0x001A);
	msleep(50);

}

/* Start Initial Sequence */
static void st7781_chip_init(struct display_device *dev)
{  
	pr_debug("st7781_chip_init.\n");

	// config as 2.8" in 7781 doc
        ili932x_write_reg(dev,ILI9325_DRV_OUTPUT_CTRL_1, 0x0100);	// Driver output control
	ili932x_write_reg(dev,ILI9325_LCD_DRV_CTRL, 0x0700);	// driving wave control
	//Entry Mode	18 bits in 2 transfert ( 16 + 2) tri=1 dfm=0
	ili932x_write_reg(dev,ILI9325_ENTRY_MOD, 0x9030);		
 	ili932x_write_reg(dev,ILI9325_DIS_CTRL_2, 0x0302);		//Display control
	ili932x_write_reg(dev,ILI9325_DIS_CTRL_3, 0x0200);		//display control
	ili932x_write_reg(dev,ILI9325_DIS_CTRL_4, 0x0000);		//Display control

	ili932x_write_reg(dev, ILI9325_GRAM_HADDR, 0x0000); /* GRAM horizontal Address. */
	ili932x_write_reg(dev, ILI9325_GRAM_VADDR, 0x0000); /* GRAM Vertical Address. */

	st7781_gamma_adjust(dev);

	/* Set GRAM area */  
	ili932x_write_reg(dev, ILI9325_HOR_ADDR_START, 0); /* Horizontal GRAM Start Address. */
	ili932x_write_reg(dev, ILI9325_HOR_ADDR_END, 239); /* Horizontal GRAM End Address. */
	ili932x_write_reg(dev, ILI9325_VET_ADDR_START, 0); /* Vertical GRAM Start Address. */
	ili932x_write_reg(dev, ILI9325_VET_ADDR_END, 319); /* Vertical GRAM End Address. */
	ili932x_write_reg(dev, ILI9325_DRV_OUTPUT_CTRL_2, 0xA700); /* Gate Scan Line. */

	ili932x_write_reg(dev, ILI9325_BASE_IMG_CTRL, 0x0001); /* NDL,VLE, REV. */
	ili932x_write_reg(dev, ILI9325_VSCROLL_CTRL, 0x0000); /* set scrolling line. */

	st7781_pan_ctrl(dev);
	st7781_pow_on(dev);

	ILI9325_SET_INDEX(dev, ILI9325_GRAM_DATA);
}
#endif

static void panel_remove(struct omap_dss_device *disp)
{
}

static int panel_enable(struct omap_dss_device *disp)
{
	int ret = 0;
	u16 dev_code;

	pr_debug("[mtf0280] call panel enable\n");

	if (disp->platform_enable)
		ret = disp->platform_enable(disp);
	if (ret < 0)
		return ret;
	
	dev_code = ili932x_read_reg(display_dev, ILI9325_DRV_CODE);

	pr_info("mtf0280] panel_enable - device code: %04x\n", dev_code);

#ifdef CONFIG_9331
	// only this way for now to differentiate ESIND from Microtek
	// first protos from ESIND works with 9325 config ( with reverted image)
	// ESIND send their best config applied here
	// should be fixed after hardware/bom/software? detection
	if ( dev_code == 0x9331)
		ili9331_chip_init(display_dev);		// for ESIND manufacturer
	else
#endif
#ifdef CONFIG_7781
	// Microtek lcd with st7781 chip inside ref MTF0280CMST-13
	if ( dev_code == 0x7783)
		st7781_chip_init(display_dev);		// 
	else
#endif
		ili9325_chip_init(display_dev);		// for Microtek manufacturer

	return ret;
}

static void panel_disable(struct omap_dss_device *disp)
{
	pr_debug("[mtf0280] call panel disable\n");

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

}

static struct omap_dss_driver mt_panel = {

	.driver		= {
		.name	= "mt_qvga_28",
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
