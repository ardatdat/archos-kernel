/*
 * drivers/media/video/omap/panel_lg_4573.c
 *
 *  Created on: Jan 26, 2010
 *      Author: archos.com
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

#ifndef _LG4573_H_
#define _LG4573_H_

#define LG4573_DRV_NAME 		"lg-fwvga-43"

// USER COMMAND SET

#define LG_UCS_NOP		0x00
#define LG_UCS_SWRESET		0x01
#define LG_UCS_RDDPM		0x0a
#define LG_UCS_RDDMADCTL	0x0b
#define LG_UCS_RDDCOLMOD	0x0c
#define LG_UCS_RDDIN		0x0d
#define LG_UCS_SLPIN		0x10
#define LG_UCS_SLPOUT		0x11
#define LG_UCS_INVOFF		0x20
#define LG_UCS_INVON		0x21
#define LG_UCS_DISPOFF		0x28
#define LG_UCS_DISPON		0x29
#define LG_UCS_MADCTL		0x36
#define LG_UCS_IDMOFF		0x38
#define LG_UCS_IDMON		0x39
#define LG_UCS_COLMOD		0x3a
#define LG_UCS_WRDISBV		0x51
#define LG_UCS_RDDISVB		0x52
#define LG_UCS_WRCTRLD		0x53
#define LG_UCS_RDCTRLD		0x54
#define LG_UCS_WRCABC		0x55
#define LG_UCS_RDCABC		0x56
#define LG_UCS_WRCABCMB		0x5e
#define LG_UCS_RDCABCMB		0x5f


// MANUFACTURER COMMAND SET

#define LG_MCS_RGBIF		0xb1
#define LG_MCS_PANELSET		0xb2
#define LG_MCS_PANELDRV		0xb3
#define LG_MCS_DISPMODE		0xb4
#define LG_MCS_DISPCTL1		0xb5
#define LG_MCS_DISPCTL2		0xb6
#define LG_MCS_OSCSET		0xc0
#define LG_MCS_PWRCTL1		0xc1
#define LG_MCS_PWRCTL2		0xc2
#define LG_MCS_PWRCTL3		0xc3
#define LG_MCS_PWRCTL4		0xc4
#define LG_MCS_PWRCTL5		0xc5
#define LG_MCS_PWRCTL6		0xc6
#define LG_MCS_OFCCTL		0xc7
#define LG_MCS_BLCTL		0xc8
#define LG_MCS_RGAMMAP		0xd0
#define LG_MCS_RGAMMAN		0xd1
#define LG_MCS_GGAMMAP		0xd2
#define LG_MCS_GGAMMAN		0xd3
#define LG_MCS_BGAMMAP		0xd4
#define LG_MCS_BGAMMAN		0xd5
#define LG_MCS_TEST1		0xf0
#define LG_MCS_OTP1		0xf8
#define LG_MCS_OTP2		0xf9
#define LG_MCS_OTP3		0xfa

#define PIX_FMT_16	0x40
#define PIX_FMT_18	0x60
#define PIX_FMT_24	0x70

#endif

