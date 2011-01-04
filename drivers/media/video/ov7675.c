/*
 * A V4L2 driver for OmniVision OV7675 cameras.
 *
 * Based on ov7670.c
 *
 * Copyright 2006 One Laptop Per Child Association, Inc.  Written
 * by Jonathan Corbet with substantial inspiration from Mark
 * McClelland's ovcamchip code.
 *
 * Copyright 2006-7 Jonathan Corbet <corbet@lwn.net>
 *
 * Copyright 2010 Archos SA
 *
 * This file may be distributed under the terms of the GNU General
 * Public License, version 2.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <media/v4l2-int-device.h>
#include <media/ov7675.h>

#include "isp/isp.h"
#include "ov7675_regs.h"


MODULE_AUTHOR("Archos SA");
MODULE_DESCRIPTION("A low-level driver for OmniVision ov7675 sensors");
MODULE_LICENSE("GPL");

/* Module Name */
#define OV7675_MODULE_NAME		"ov7675"

#define MANUFACTURER_ID			0x7FA2
#define PRODUCT_ID			0x7673

#define I2C_RETRY_COUNT                 (5)


/*
 * Information we maintain about a known sensor.
 */

struct ov7675_decoder {
	struct v4l2_int_device *v4l2_int_device;
	const struct ov7675_platform_data *pdata;
	struct i2c_client *client;
	enum ov7675_state state;
	struct i2c_device_id *id;
	struct ov7675_format_struct *fmt;	/* Current format */
	struct v4l2_pix_format pix;
	unsigned char sat;			/* Saturation value */
	int hue;				/* Hue value */
	int current_colorfx;			/* Current Color FX */
	int current_unbanding_mode;		/* Current Unbanding Mode */
	int current_night_mode;			/* Current Night Mode */
	int current_manual_wb;			/* Current Manual White Balance */
};

static unsigned long xclk_current = OV7675_CLOCK_24MHZ;

static int ov7675_t_sat(struct i2c_client *client, int value);
static int ov7675_t_hue(struct i2c_client *client, int value);
static int ov7675_t_brightness(struct i2c_client *client, int value);
static int ov7675_t_contrast(struct i2c_client *client, int value);
static int ov7675_t_hflip(struct i2c_client *client, int value);
static int ov7675_t_vflip(struct i2c_client *client, int value);
static int ov7675_t_unbanding(struct i2c_client *client, int value);
static int ov7675_t_night_mode(struct i2c_client *client, int value);
static int ov7675_t_colorfx(struct i2c_client *client, int value);


/*
 * The default register settings, as obtained from OmniVision.  There
 * is really no making sense of most of these - lots of "reserved" values
 * and such.
 *
 * These settings give VGA YUYV.
 */
static struct regval_list ov7675_default_regs[] = {
/*
 * Clock scale: 3 = 15fps
 *              2 = 20fps
 *              1 = 30fps
 */
	{ REG_CLKRC, 0x80 },	/* OV: clock scale (30 fps) */
	{ REG_TSLB,  0x04 },	/* OV */
	{ REG_COM7, 0 },	/* VGA */
	/*
	 * Set the hardware window.  These values from OV don't entirely
	 * make sense - hstop is less than hstart.  But they work...
	 */
	{ REG_HSTART, 0x13 },	{ REG_HSTOP, 0x01 },
	{ REG_HREF, 0xb6 },	{ REG_VSTART, 0x03 },
	{ REG_VSTOP, 0x7b },	{ REG_VREF, 0x0a },

	{ REG_COM3, 0 },	{ REG_COM14, 0 },
	/* Mystery scaling numbers */
	{ 0x70, 0x3a },		{ 0x71, 0x35 },
	{ 0x72, 0x11 },		{ 0x73, 0xf0 },
	// Vsync positive
	{ 0xa2, 0x02 },		{ REG_COM10, 0x00 },

	/* Gamma curve values */
	{ 0x7a, 0x18 },		{ 0x7b, 0x04 },
	{ 0x7c, 0x09 },		{ 0x7d, 0x18 },
	{ 0x7e, 0x38 },		{ 0x7f, 0x47 },
	{ 0x80, 0x56 },		{ 0x81, 0x66 },
	{ 0x82, 0x74 },		{ 0x83, 0x7f },
	{ 0x84, 0x89 },		{ 0x85, 0x9a },
	{ 0x86, 0xA9 },		{ 0x87, 0xC4 },
	{ 0x88, 0xDb },		{ 0x89, 0xEe },

	/* AGC and AEC parameters.  Note we start by disabling those features,
	   then turn them only after tweaking the values. */
	{ REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT },
	{ REG_BLUE, 0x50 },
	{ REG_RED, 0x68 },
	{ REG_GAIN, 0 },	{ REG_AECH, 0 },
	{ REG_COM4, 0x40 }, /* magic reserved bit */
	{ REG_COM9, 0x48 }, /* 32x gain + magic rsvd bit */
	{ REG_BD50MAX, 0x07 },	{ REG_BD60MAX, 0x08 },
	{ REG_AEW, 0x60 },	{ REG_AEB, 0x50 },
	{ REG_VPT, 0xe3 },	{ REG_HAECC1, 0x78 },
	{ REG_HAECC2, 0x68 },	{ 0xa1, 0x03 }, /* magic */
	{ REG_HAECC3, 0xd8 },	{ REG_HAECC4, 0xd8 },
	{ REG_HAECC5, 0xf0 },	{ REG_HAECC6, 0x90 },
	{ REG_HAECC7, 0x14 },
	{ REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC },

	/* Almost all of these are magic "reserved" values.  */
	{ REG_COM5, 0x61 },	{ REG_COM6, 0x4b },
	{ 0x16, 0x02 },		{ REG_MVFP, 0x07 },
	{ 0x21, 0x02 },		{ 0x22, 0x91 },
	{ 0x29, 0x07 },		{ 0x33, 0x0b },
	{ 0x35, 0x0b },		{ 0x37, 0x1d },
	{ 0x38, 0x71 },		{ 0x39, 0x2a },
	{ REG_COM12, 0x78 },	{ 0x4d, 0x40 },
	{ 0x4e, 0x20 },		{ REG_GFIX, 0 },
	{ 0x6b, 0x0a },		{ 0x74, 0x10 },
	{ 0x8d, 0x4f },		{ 0x8e, 0 },
	{ 0x8f, 0 },		{ 0x90, 0 },
	{ 0x91, 0 },		{ 0x92, 0x66 },
	{ 0x96, 0 },		{ 0x9a, 0x80 },
	{ 0xb0, 0x84 },		{ 0xb1, 0x0c },
	{ 0xb2, 0x0e },		{ 0xb3, 0x82 },
	{ 0xb8, 0x0a },

	/* More reserved magic, some of which tweaks white balance */
	{ 0x43, 0x14 },		{ 0x44, 0xf0 },
	{ 0x45, 0x41 },		{ 0x46, 0x66 },
	{ 0x47, 0x2a },		{ 0x48, 0x3e },
	{ 0x59, 0x8d },		{ 0x5a, 0x8e },
	{ 0x5b, 0x53 },		{ 0x5c, 0x83 },
	{ 0x5d, 0x4f },		{ 0x5e, 0x0e },
	{ 0x6c, 0x0a },		{ 0x6d, 0x55 },
	{ 0x6e, 0x11 },		{ 0x6f, 0x9e }, /* "9e for advance AWB" */
	{ 0x62, 0x90 },		{ 0x63, 0x30 },
	{ 0x64, 0x11 },		{ 0x65, 0x00 },
	{ 0x66, 0x05 },		{ 0x94, 0x11 },
	{ 0x95, 0x18 },
	{ REG_GREEN, 0x40 },		{ REG_BLUE, 0x40 },
	{ REG_RED, 0x40 },
	{ REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC|COM8_AWB },

	/* Matrix coefficients */
	{ 0x4f, 0x80 },		{ 0x50, 0x80 },
	{ 0x51, 0 },		{ 0x52, 0x22 },
	{ 0x53, 0x5e },		{ 0x54, 0x80 },
	{ 0x58, 0x9e },

	{ REG_COM16, COM16_AWBGAIN },	{ REG_EDGE, 0 },
	{ 0x75, 0x03 },		{ 0x76, 0xe1 },
	{ 0x4c, 0 },		{ 0x77, 0x00 },
	{ REG_COM13, 0xc2 },	{ 0x4b, 0x09 },
	{ 0xc9, 0x60 },		{ REG_COM16, 0x38 },
	{ 0x56, 0x3A },

	{ 0x34, 0x11 },		{ REG_COM11, COM11_EXP|COM11_50HZ },
	{ 0xa4, 0x88 },		{ 0x96, 0 },
	{ 0x97, 0x30 },		{ 0x98, 0x20 },
	{ 0x99, 0x30 },		{ 0x9a, 0x84 },
	{ 0x9b, 0x29 },		{ 0x9c, 0x03 },
	{ 0x9d, 0x98 },		{ 0x9e, 0x3f },
	{ 0x78, 0x04 },

	/* Extra-weird stuff.  Some sort of multiplexor register */
	{ 0x79, 0x01 },		{ 0xc8, 0xf0 },
	{ 0x79, 0x0f },		{ 0xc8, 0x00 },
	{ 0x79, 0x10 },		{ 0xc8, 0x7e },
	{ 0x79, 0x0a },		{ 0xc8, 0x80 },
	{ 0x79, 0x0b },		{ 0xc8, 0x01 },
	{ 0x79, 0x0c },		{ 0xc8, 0x0f },
	{ 0x79, 0x0d },		{ 0xc8, 0x20 },
	{ 0x79, 0x09 },		{ 0xc8, 0x80 },
	{ 0x79, 0x02 },		{ 0xc8, 0xc0 },
	{ 0x79, 0x03 },		{ 0xc8, 0x40 },
	{ 0x79, 0x05 },		{ 0xc8, 0x30 },
	{ 0x79, 0x26 },		{ 0x2d, 0x00 },
	{ 0x2e, 0x00 },

	// 30 fps, PCLK = 24Mhz
	{ REG_CLKRC, 0x40 },
	{ 0x6b, 0x0a },
	{ 0x2a, 0x00 },
	{ 0x2b, 0x00 },
	{ 0x2d, 0x00 },
	{ 0x2e, 0x00 },
	{ 0xca, 0x00 },
	{ 0x92, 0x66 },
	{ 0x93, 0x00 },
	{ REG_COM11, COM11_EXP|COM11_50HZ },
	{ 0xcf, 0x8c },

	{ 0x9d, 0x98 },	//50Hz banding filter
	{ 0x9e, 0x7f },	//60Hz banding filter
	{ 0xa5, 0x02 },	//3 step for 50hz
	{ 0xab, 0x03 },	//4 step for 60hz

	{ 0xff, 0xff },	/* END MARKER */
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 * RGB656 and YUV422 come from OV; RGB444 is homebrewed.
 *
 * IMPORTANT RULE: the first entry must be for COM7, see ov7675_s_fmt for why.
 */


static struct regval_list ov7675_fmt_yuv422[] = {
	{ REG_COM7, 0x0 },  /* Selects YUV mode */
	{ REG_RGB444, 0 },	/* No RGB444 please */
	{ REG_COM1, 0 },
	{ REG_COM15, COM15_R00FF },
	{ REG_COM9, 0x48 }, /* 32x gain ceiling; 0x8 is reserved bit */
	{ 0x4f, 0x80 }, 	/* "matrix coefficient 1" */
	{ 0x50, 0x80 }, 	/* "matrix coefficient 2" */
	{ 0x51, 0    },		/* vb */
	{ 0x52, 0x22 }, 	/* "matrix coefficient 4" */
	{ 0x53, 0x5e }, 	/* "matrix coefficient 5" */
	{ 0x54, 0x80 }, 	/* "matrix coefficient 6" */
	{ REG_COM13, COM13_GAMMA|COM13_UVSAT },
	{ 0xff, 0xff },
};

static struct regval_list ov7675_fmt_rgb565[] = {
	{ REG_COM7, COM7_RGB },	/* Selects RGB mode */
	{ REG_RGB444, 0 },	/* No RGB444 please */
	{ REG_COM1, 0x0 },
	{ REG_COM15, COM15_RGB565 },
	{ REG_COM9, 0x48 }, 	/* 32x gain ceiling; 0x8 is reserved bit */
	{ 0x4f, 0xb3 }, 	/* "matrix coefficient 1" */
	{ 0x50, 0xb3 }, 	/* "matrix coefficient 2" */
	{ 0x51, 0    },		/* vb */
	{ 0x52, 0x3d }, 	/* "matrix coefficient 4" */
	{ 0x53, 0xa7 }, 	/* "matrix coefficient 5" */
	{ 0x54, 0xe4 }, 	/* "matrix coefficient 6" */
	{ REG_COM13, COM13_GAMMA|COM13_UVSAT },
	{ 0xff, 0xff },
};

static struct regval_list ov7675_fmt_rgb444[] = {
	{ REG_COM7, COM7_RGB },	/* Selects RGB mode */
	{ REG_RGB444, R444_ENABLE },	/* Enable xxxxrrrr ggggbbbb */
	{ REG_COM1, 0x40 },	/* Magic reserved bit */
	{ REG_COM15, COM15_R01FE|COM15_RGB565 }, /* Data range needed? */
	{ REG_COM9, 0x48 }, 	/* 32x gain ceiling; 0x8 is reserved bit */
	{ 0x4f, 0xb3 }, 	/* "matrix coefficient 1" */
	{ 0x50, 0xb3 }, 	/* "matrix coefficient 2" */
	{ 0x51, 0    },		/* vb */
	{ 0x52, 0x3d }, 	/* "matrix coefficient 4" */
	{ 0x53, 0xa7 }, 	/* "matrix coefficient 5" */
	{ 0x54, 0xe4 }, 	/* "matrix coefficient 6" */
	{ REG_COM13, COM13_GAMMA|COM13_UVSAT|0x2 },  /* Magic rsvd bit */
	{ 0xff, 0xff },
};

static struct regval_list ov7675_fmt_raw[] = {
	{ REG_COM7, COM7_BAYER },
	{ REG_COM13, 0x08 }, /* No gamma, magic rsvd bit */
	{ REG_COM16, 0x3d }, /* Edge enhancement, denoise */
	{ REG_REG76, 0xe1 }, /* Pix correction, magic rsvd */
	{ 0xff, 0xff },
};



/*
 * Low-level register I/O.
 */

#if 0
static int ov7675_read(struct i2c_client *c, unsigned char reg,
		unsigned char *value)
{
	int ret;
	int retry = 0;

read_again:
	ret = i2c_smbus_read_byte_data(c, reg);
	if (ret >= 0) {
		*value = (unsigned char) ret;
		ret = 0;
	} else if (retry <= I2C_RETRY_COUNT) {
		v4l_warn(c, "Read: retry ... %d, err = %d\n", retry, ret);
		retry++;
		msleep_interruptible(10);
		goto read_again;
	}
	return ret;
}


static int ov7675_write(struct i2c_client *c, unsigned char reg,
		unsigned char value)
{
	int ret;
	int retry = 0;

write_again:
	ret = i2c_smbus_write_byte_data(c, reg, value);
	if (ret) {
		if (retry <= I2C_RETRY_COUNT) {
			v4l_warn(c, "Write: retry ... %d, err = %d\n", retry, ret);
			retry++;
			msleep_interruptible(10);
			goto write_again;
		}
	}
	if (reg == REG_COM7 && (value & COM7_RESET))
		msleep(2);  /* Wait for reset to run */
	return ret;
}

#else

static int ov7675_read(struct i2c_client *c, unsigned char reg,
		unsigned char *value)
{
	int ret;
	int retry = 0;
	struct i2c_msg msgs[2];

read_again:
	msgs[0].addr = c->addr,
	msgs[0].flags = 0,
	msgs[0].len = 1,
	msgs[0].buf = &reg,

	msgs[1].addr = c->addr,
	msgs[1].flags = I2C_M_RD,
	msgs[1].len = 1,
	msgs[1].buf = value,

	ret = i2c_transfer(c->adapter, msgs, 2);
	if (ret < 0) {
		if (retry <= I2C_RETRY_COUNT) {
			v4l_warn(c, "Read: retry ... %d, err = %d\n", retry, ret);
			retry++;
			msleep_interruptible(10);
			goto read_again;
		} else
			return ret;
	}

	return 0;
}

static int ov7675_write(struct i2c_client *c, unsigned char reg,
		unsigned char value)
{
	int ret;
	int retry = 0;
	unsigned char data[2];
	struct i2c_msg msgs[1];

write_again:
	data[0] = reg;
	data[1] = value;

	msgs[0].addr = c->addr,
	msgs[0].flags = 0,
	msgs[0].len = 2,
	msgs[0].buf = data,

	ret = i2c_transfer(c->adapter, msgs, 1);
	if (ret < 0) {
		if (retry <= I2C_RETRY_COUNT) {
			v4l_warn(c, "Write: retry ... %d, err = %d\n", retry, ret);
			retry++;
			msleep_interruptible(10);
			goto write_again;
		} else
			return ret;
	}

	return 0;
}
#endif

/*
 * Write a list of register settings; ff/ff stops the process.
 */
static int ov7675_write_array(struct i2c_client *c, struct regval_list *vals)
{
	while (vals->reg_num != 0xff || vals->value != 0xff) {
		int ret = ov7675_write(c, vals->reg_num, vals->value);
		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
}


/*
 * Stuff that knows about the sensor.
 */
static void ov7675_reset(struct i2c_client *client)
{
	ov7675_write(client, REG_COM7, COM7_RESET);
	msleep(1);
}

static int ov7675_init(struct i2c_client *client)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);
	int unbanding_mode, night_mode, colorfx;

	/* NOTE: No need to init the manual white balance
	 *       here since the default is "auto white balance".
	 */

	unbanding_mode = decoder->current_unbanding_mode;
	night_mode = decoder->current_night_mode;
	colorfx = decoder->current_colorfx;

	decoder->current_unbanding_mode = -1;
	decoder->current_night_mode = -1;
	decoder->current_colorfx = -1;

	ov7675_reset(client);
	// OV says we have to wait 3 ms
	msleep(5);
	ov7675_write_array(client, ov7675_default_regs);

	ov7675_t_unbanding(client, unbanding_mode);
	ov7675_t_night_mode(client, night_mode);
	ov7675_t_colorfx(client, colorfx);

	return 0;
}

static int ov7675_detect(struct i2c_client *client)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);
	unsigned char v;
	unsigned short mid, pid;
	int ret;

	ret = ov7675_read(client, REG_MIDH, &v);
	if (ret < 0)
		return ret;
	mid = v << 8;
	ret = ov7675_read(client, REG_MIDL, &v);
	if (ret < 0)
		return ret;
	mid |= v;

	/*
	 * OK, we know we have an OmniVision chip...but which one?
	 */
	ret = ov7675_read(client, REG_PID, &v);
	if (ret < 0)
		return ret;
	pid = v << 8;
	ret = ov7675_read(client, REG_VER, &v);
	if (ret < 0)
		return ret;
	pid |= v;

	v4l_info(client,"Detected IDs: MID = 0x%04X, PID = 0x%04X\n",
			mid, pid);

	if (mid != MANUFACTURER_ID || pid != PRODUCT_ID) {
		v4l_err(client,
			"Chip IDs mismatch: MID:0x%04X PID:0x%04X\n",
			MANUFACTURER_ID, PRODUCT_ID);
		return -ENODEV;
	}

	decoder->state = STATE_DETECTED;
	v4l_info(client,"%s found at 0x%x (%s)\n", client->name,
			client->addr,
			client->adapter->name);

	return 0;
}

/* Not useful for now, but may be used later to dynamically change
 * the clock frequency.
 */
static unsigned long ov7675_calc_xclk(struct i2c_client *c)
{
	//struct ov7675_decoder *decoder = i2c_get_clientdata(c);

	xclk_current = OV7675_CLOCK_12MHZ;

	return xclk_current;
}


/*
 * Store information about the video data format.  The color matrix
 * is deeply tied into the format, so keep the relevant values here.
 * The magic matrix nubmers come from OmniVision.
 */
static struct ov7675_format_struct ov7675_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.regs 		= ov7675_fmt_yuv422,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
/*	{
		.desc		= "RGB 444",
		.pixelformat	= V4L2_PIX_FMT_RGB444,
		.regs		= ov7675_fmt_rgb444,
		.cmatrix	= { 179, -179, 0, -61, -176, 228 },
		.bpp		= 2,
	},
	{
		.desc		= "RGB 565",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
		.regs		= ov7675_fmt_rgb565,
		.cmatrix	= { 179, -179, 0, -61, -176, 228 },
		.bpp		= 2,
	},
	{
		.desc		= "Raw RGB Bayer",
		.pixelformat	= V4L2_PIX_FMT_SBGGR8,
		.regs 		= ov7675_fmt_raw,
		.cmatrix	= { 0, 0, 0, 0, 0, 0 },
		.bpp		= 1
	},*/
};
#define N_OV7675_FMTS ARRAY_SIZE(ov7675_formats)


/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

/*
 * QCIF mode is done (by OV) in a very strange way - it actually looks like
 * VGA with weird scaling options - they do *not* use the canned QCIF mode
 * which is allegedly provided by the sensor.  So here's the weird register
 * settings.
 */
static struct regval_list ov7675_qcif_regs[] = {
	{ REG_COM3, COM3_SCALEEN|COM3_DCWEN },
	{ REG_COM3, COM3_DCWEN },
	{ REG_COM14, COM14_DCWEN | 0x01},
	{ 0x73, 0xf1 },
	{ 0xa2, 0x52 },
	{ 0x7b, 0x1c },
	{ 0x7c, 0x28 },
	{ 0x7d, 0x3c },
	{ 0x7f, 0x69 },
	{ REG_COM9, 0x48 },
	{ 0xa1, 0x0b },
	{ 0x74, 0x19 },
	{ 0x9a, 0x80 },
	{ 0x43, 0x14 },
	{ REG_COM13, 0xc0 },
	{ 0xff, 0xff },
};

static struct ov7675_win_size ov7675_win_sizes[] = {
	/* VGA */
	{
		.width		= VGA_WIDTH,
		.height		= VGA_HEIGHT,
		.com7_bit	= COM7_FMT_VGA,
		.hstart		= 158,		/* These values from */
		.hstop		=  14,		/* Omnivision */
		.vstart		=  14,
		.vstop		= 494,
		.regs 		= NULL,
	},
#if 0	// These formats (from the OV7670) do not seem to work for the OV7675 (even if at least QVGA is supposed to be supported).
	/* CIF */
	{
		.width		= CIF_WIDTH,
		.height		= CIF_HEIGHT,
		.com7_bit	= COM7_FMT_CIF,
		.hstart		= 170,		/* Empirically determined */
		.hstop		=  90,
		.vstart		=  14,
		.vstop		= 494,
		.regs 		= NULL,
	},
	/* QVGA */
	{
		.width		= QVGA_WIDTH,
		.height		= QVGA_HEIGHT,
		.com7_bit	= COM7_FMT_QVGA,
		.hstart		= 164,		/* Empirically determined */
		.hstop		=  20,
		.vstart		=  14,
		.vstop		= 494,
		.regs 		= NULL,
	},
	/* QCIF */
	{
		.width		= QCIF_WIDTH,
		.height		= QCIF_HEIGHT,
		.com7_bit	= COM7_FMT_VGA, /* see comment above */
		.hstart		= 456,		/* Empirically determined */
		.hstop		=  24,
		.vstart		=  14,
		.vstop		= 494,
		.regs 		= ov7675_qcif_regs,
	},
#endif
};

#define N_WIN_SIZES (ARRAY_SIZE(ov7675_win_sizes))


/*
 * Store a set of start/stop values into the camera.
 */
static int ov7675_set_hw(struct i2c_client *client, int hstart, int hstop,
		int vstart, int vstop)
{
	int ret;
	unsigned char v;
/*
 * Horizontal: 11 bits, top 8 live in hstart and hstop.  Bottom 3 of
 * hstart are in href[2:0], bottom 3 of hstop in href[5:3].  There is
 * a mystery "edge offset" value in the top two bits of href.
 */
	ret =  ov7675_write(client, REG_HSTART, (hstart >> 3) & 0xff);
	ret += ov7675_write(client, REG_HSTOP, (hstop >> 3) & 0xff);
	ret += ov7675_read(client, REG_HREF, &v);
	v = (v & 0xc0) | ((hstop & 0x7) << 3) | (hstart & 0x7);
	msleep(10);
	ret += ov7675_write(client, REG_HREF, v);
/*
 * Vertical: similar arrangement, but only 10 bits.
 */
	ret += ov7675_write(client, REG_VSTART, (vstart >> 2) & 0xff);
	ret += ov7675_write(client, REG_VSTOP, (vstop >> 2) & 0xff);
	ret += ov7675_read(client, REG_VREF, &v);
	v = (v & 0xf0) | ((vstop & 0x3) << 2) | (vstart & 0x3);
	msleep(10);
	ret += ov7675_write(client, REG_VREF, v);
	return ret;
}


static int ov7675_enum_fmt(struct i2c_client *c, struct v4l2_fmtdesc *fmt)
{
	struct ov7675_format_struct *ofmt;

	if (fmt->index >= N_OV7675_FMTS)
		return -EINVAL;

	ofmt = ov7675_formats + fmt->index;
	fmt->flags = 0;
	strcpy(fmt->description, ofmt->desc);
	fmt->pixelformat = ofmt->pixelformat;
	return 0;
}


static int ov7675_try_fmt(struct i2c_client *c, struct v4l2_format *fmt,
		struct ov7675_format_struct **ret_fmt,
		struct ov7675_win_size **ret_wsize)
{
	int index;
	struct ov7675_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	for (index = 0; index < N_OV7675_FMTS; index++)
		if (ov7675_formats[index].pixelformat == pix->pixelformat)
			break;
	if (index >= N_OV7675_FMTS) {
		/* default to first format */
		index = 0;
		pix->pixelformat = ov7675_formats[0].pixelformat;
	}
	if (ret_fmt != NULL)
		*ret_fmt = ov7675_formats + index;
	/*
	 * Fields: the OV devices claim to be progressive.
	 */
	pix->field = V4L2_FIELD_NONE;
	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = ov7675_win_sizes; wsize < ov7675_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (pix->width >= wsize->width && pix->height >= wsize->height)
			break;
	if (wsize >= ov7675_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	pix->width = wsize->width;
	pix->height = wsize->height;
	pix->bytesperline = pix->width*ov7675_formats[index].bpp;
	pix->sizeimage = pix->height*pix->bytesperline;
	return 0;
}

/*
 * Set a format.
 */
static int ov7675_s_fmt(struct i2c_client *c, struct v4l2_format *fmt)
{
	int ret;
	struct ov7675_format_struct *ovfmt;
	struct ov7675_win_size *wsize;
	struct ov7675_decoder *decoder = i2c_get_clientdata(c);
	unsigned char com7, clkrc;

	ret = ov7675_try_fmt(c, fmt, &ovfmt, &wsize);
	if (ret)
		return ret;
	/*
	 * HACK: if we're running rgb565 we need to grab then rewrite
	 * CLKRC.  If we're *not*, however, then rewriting clkrc hoses
	 * the colors.
	 */
	if (fmt->fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
		ret = ov7675_read(c, REG_CLKRC, &clkrc);
		if (ret)
			return ret;
	}
	/*
	 * COM7 is a pain in the ass, it doesn't like to be read then
	 * quickly written afterward.  But we have everything we need
	 * to set it absolutely here, as long as the format-specific
	 * register sets list it first.
	 */
	com7 = ovfmt->regs[0].value;
	com7 |= wsize->com7_bit;
	ov7675_write(c, REG_COM7, com7);
	/*
	 * Now write the rest of the array.  Also store start/stops
	 */
	ov7675_write_array(c, ovfmt->regs + 1);
	ov7675_set_hw(c, wsize->hstart, wsize->hstop, wsize->vstart,
			wsize->vstop);
	ret = 0;
	if (wsize->regs)
		ret = ov7675_write_array(c, wsize->regs);
	decoder->fmt = ovfmt;
	/* Store pix format */
	memcpy(&decoder->pix, &fmt->fmt.pix, sizeof(struct v4l2_pix_format));

	if (fmt->fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565 && ret == 0)
		ret = ov7675_write(c, REG_CLKRC, clkrc);
	return ret;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int ov7675_g_parm(struct i2c_client *c, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	unsigned char clkrc;
	int ret;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	ret = ov7675_read(c, REG_CLKRC, &clkrc);
	if (ret < 0)
		return ret;
	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = OV7675_FRAME_RATE;
	if ((clkrc & CLK_EXT) == 0 && (clkrc & CLK_SCALE) > 1)
		cp->timeperframe.denominator /= (clkrc & CLK_SCALE);
	return 0;
}

static int ov7675_s_parm(struct i2c_client *c, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct v4l2_fract *tpf = &cp->timeperframe;
	unsigned char clkrc;
	int ret, div;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	/*
	 * CLKRC has a reserved bit, so let's preserve it.
	 */
	ret = ov7675_read(c, REG_CLKRC, &clkrc);
	if (ret < 0)
		return ret;
	/* Change the framerate only if the pre-scaler is enabled */
	if (!(clkrc & CLK_EXT)) {
		if (tpf->numerator == 0 || tpf->denominator == 0)
			div = 1;  /* Reset to full rate */
		else
			div = (tpf->numerator*OV7675_FRAME_RATE)/tpf->denominator;
		if (div == 0)
			div = 1;
		else if (div > CLK_SCALE)
			div = CLK_SCALE;
		clkrc = (clkrc & 0xc0) | div;
		tpf->numerator = 1;
		tpf->denominator = OV7675_FRAME_RATE/div;
		ret = ov7675_write(c, REG_CLKRC, clkrc);
		if (ret < 0)
			return ret;
	}
	return ret;
}



/*
 * Code for dealing with controls.
 */

static int ov7675_store_cmatrix(struct i2c_client *client,
		int matrix[CMATRIX_LEN])
{
	int i, ret;
	unsigned char signbits = 0;

	/*
	 * Weird crap seems to exist in the upper part of
	 * the sign bits register, so let's preserve it.
	 */
	ret = ov7675_read(client, REG_CMATRIX_SIGN, &signbits);
	signbits &= 0xc0;

	for (i = 0; i < CMATRIX_LEN; i++) {
		unsigned char raw;

		if (matrix[i] < 0) {
			signbits |= (1 << i);
			if (matrix[i] < -255)
				raw = 0xff;
			else
				raw = (-1 * matrix[i]) & 0xff;
		}
		else {
			if (matrix[i] > 255)
				raw = 0xff;
			else
				raw = matrix[i] & 0xff;
		}
		ret += ov7675_write(client, REG_CMATRIX_BASE + i, raw);
	}
	ret += ov7675_write(client, REG_CMATRIX_SIGN, signbits);
	return ret;
}


/*
 * Hue also requires messing with the color matrix.  It also requires
 * trig functions, which tend not to be well supported in the kernel.
 * So here is a simple table of sine values, 0-90 degrees, in steps
 * of five degrees.  Values are multiplied by 1000.
 *
 * The following naive approximate trig functions require an argument
 * carefully limited to -180 <= theta <= 180.
 */
#define SIN_STEP 5
static const int ov7675_sin_table[] = {
	   0,	 87,   173,   258,   342,   422,
	 499,	573,   642,   707,   766,   819,
	 866,	906,   939,   965,   984,   996,
	1000
};

static int ov7675_sine(int theta)
{
	int chs = 1;
	int sine;

	if (theta < 0) {
		theta = -theta;
		chs = -1;
	}
	if (theta <= 90)
		sine = ov7675_sin_table[theta/SIN_STEP];
	else {
		theta -= 90;
		sine = 1000 - ov7675_sin_table[theta/SIN_STEP];
	}
	return sine*chs;
}

static int ov7675_cosine(int theta)
{
	theta = 90 - theta;
	if (theta > 180)
		theta -= 360;
	else if (theta < -180)
		theta += 360;
	return ov7675_sine(theta);
}




static void ov7675_calc_cmatrix(struct ov7675_decoder *decoder,
		int matrix[CMATRIX_LEN])
{
	int i;
	/*
	 * Apply the current saturation setting first.
	 */
	for (i = 0; i < CMATRIX_LEN; i++)
		matrix[i] = (decoder->fmt->cmatrix[i]*decoder->sat) >> 7;
	/*
	 * Then, if need be, rotate the hue value.
	 */
	if (decoder->hue != 0) {
		int sinth, costh, tmpmatrix[CMATRIX_LEN];

		memcpy(tmpmatrix, matrix, CMATRIX_LEN*sizeof(int));
		sinth = ov7675_sine(decoder->hue);
		costh = ov7675_cosine(decoder->hue);

		matrix[0] = (matrix[3]*sinth + matrix[0]*costh)/1000;
		matrix[1] = (matrix[4]*sinth + matrix[1]*costh)/1000;
		matrix[2] = (matrix[5]*sinth + matrix[2]*costh)/1000;
		matrix[3] = (matrix[3]*costh - matrix[0]*sinth)/1000;
		matrix[4] = (matrix[4]*costh - matrix[1]*sinth)/1000;
		matrix[5] = (matrix[5]*costh - matrix[2]*sinth)/1000;
	}
}



static int ov7675_t_sat(struct i2c_client *client, int value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);
	int matrix[CMATRIX_LEN];
	int ret;

	decoder->sat = value;
	ov7675_calc_cmatrix(decoder, matrix);
	ret = ov7675_store_cmatrix(client, matrix);
	return ret;
}

static int ov7675_q_sat(struct i2c_client *client, __s32 *value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);

	*value = decoder->sat;
	return 0;
}

static int ov7675_t_hue(struct i2c_client *client, int value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);
	int matrix[CMATRIX_LEN];
	int ret;

	if (value < -180 || value > 180)
		return -EINVAL;
	decoder->hue = value;
	ov7675_calc_cmatrix(decoder, matrix);
	ret = ov7675_store_cmatrix(client, matrix);
	return ret;
}

static int ov7675_q_hue(struct i2c_client *client, __s32 *value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);

	*value = decoder->hue;
	return 0;
}

/*
 * Some weird registers seem to store values in a sign/magnitude format!
 */
static unsigned char ov7675_sm_to_abs(unsigned char v)
{
	if ((v & 0x80) == 0)
		return v + 128;
	else
		return 128 - (v & 0x7f);
}


static unsigned char ov7675_abs_to_sm(unsigned char v)
{
	if (v > 127)
		return v & 0x7f;
	else
		return (128 - v) | 0x80;
}

static int ov7675_t_brightness(struct i2c_client *client, int value)
{
	unsigned char com8 = 0, v;
	int ret;

	ov7675_read(client, REG_COM8, &com8);
	com8 &= ~COM8_AEC;
	ov7675_write(client, REG_COM8, com8);
	v = ov7675_abs_to_sm(value);
	ret = ov7675_write(client, REG_BRIGHT, v);
	return ret;
}

static int ov7675_q_brightness(struct i2c_client *client, __s32 *value)
{
	unsigned char v = 0;
	int ret = ov7675_read(client, REG_BRIGHT, &v);

	*value = ov7675_sm_to_abs(v);
	return ret;
}


static int ov7675_t_contrast(struct i2c_client *client, int value)
{
	return ov7675_write(client, REG_CONTRAS, (unsigned char) value);
}

static int ov7675_q_contrast(struct i2c_client *client, __s32 *value)
{
	unsigned char v = 0;
	int ret = ov7675_read(client, REG_CONTRAS, &v);

	*value = v;
	return ret;
}


static int ov7675_q_hflip(struct i2c_client *client, __s32 *value)
{
	int ret;
	unsigned char v = 0;

	ret = ov7675_read(client, REG_MVFP, &v);
	*value = (v & MVFP_MIRROR) == MVFP_MIRROR;
	return ret;
}

static int ov7675_t_hflip(struct i2c_client *client, int value)
{
	unsigned char v = 0;
	int ret;

	ret = ov7675_read(client, REG_MVFP, &v);
	if (value)
		v |= MVFP_MIRROR;
	else
		v &= ~MVFP_MIRROR;
	msleep(10);  /* FIXME */
	ret += ov7675_write(client, REG_MVFP, v);
	return ret;
}


static int ov7675_q_vflip(struct i2c_client *client, __s32 *value)
{
	int ret;
	unsigned char v = 0;

	ret = ov7675_read(client, REG_MVFP, &v);
	*value = (v & MVFP_FLIP) == MVFP_FLIP;
	return ret;
}

static int ov7675_t_vflip(struct i2c_client *client, int value)
{
	unsigned char v = 0;
	int ret;

	ret = ov7675_read(client, REG_MVFP, &v);
	if (value)
		v |= MVFP_FLIP;
	else
		v &= ~MVFP_FLIP;
	msleep(10);  /* FIXME */
	ret += ov7675_write(client, REG_MVFP, v);
	return ret;
}


static int ov7675_q_unbanding(struct i2c_client *client, __s32 *value)
{
#if 0
	int ret;
	unsigned char v = 0;

	ret = ov7675_read(client, REG_COM8, &v);
	if (!(v & COM8_BFILT)) {
		*value = V4L2_UNBANDING_NONE;
		return ret;
	}

	ret += ov7675_read(client, REG_COM11, &v);
	if (v & COM11_50HZ)
		*value = V4L2_UNBANDING_50HZ;
	else
		*value = V4L2_UNBANDING_60HZ;

	return ret;
#else
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);

	*value = decoder->current_unbanding_mode;
	return 0;
#endif
}

static int ov7675_t_unbanding(struct i2c_client *client, int value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);
	unsigned char v = 0;
	int ret;

	if (decoder->current_unbanding_mode == value)
		return 0;

	decoder->current_unbanding_mode = value;

	if (value == V4L2_UNBANDING_NONE) {
		ret = ov7675_read(client, REG_COM8, &v);
		msleep(10);  /* FIXME */
		ret += ov7675_write(client, REG_COM8, v & ~COM8_BFILT);
		return ret;
	}

	ret += ov7675_read(client, REG_COM11, &v);
	msleep(10);  /* FIXME */
	switch (value) {
		case V4L2_UNBANDING_60HZ:
			ret += ov7675_write(client, REG_COM11, v & ~COM11_50HZ);
			break;
		case V4L2_UNBANDING_50HZ:
			ret += ov7675_write(client, REG_COM11, v | COM11_50HZ);
			break;
		default:
			break;
	}

	ret = ov7675_read(client, REG_COM8, &v);
	msleep(10);  /* FIXME */

	/* It seems that sometimes Antibanding has to be turned off and on again to work properly */
	ret += ov7675_write(client, REG_COM8, v & ~COM8_BFILT);
	msleep(10);  /* FIXME */
	ret += ov7675_write(client, REG_COM8, v | COM8_BFILT);

	return ret;
}


static int ov7675_q_night_mode(struct i2c_client *client, __s32 *value)
{
#if 0
	int ret;
	unsigned char v = 0;

	ret = ov7675_read(client, REG_COM11, &v);
	*value = (v & COM11_NIGHT) == COM11_NIGHT;
	return ret;
#else
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);

	*value = decoder->current_night_mode;
	return 0;
#endif
}

static int ov7675_t_night_mode(struct i2c_client *client, int value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);
	unsigned char v = 0;
	int ret;

	if (decoder->current_night_mode == value)
		return 0;

	decoder->current_night_mode = value;

	ret = ov7675_read(client, REG_COM11, &v);
	msleep(10);  /* FIXME */
	if (value) {
		ret += ov7675_write(client, REG_COM11, v | COM11_NIGHT);
	} else {
		ret += ov7675_write(client, REG_COM11, v & ~COM11_NIGHT);
		/* Reset the framerate to 30 fps */
		ret += ov7675_write(client, REG_CLKRC, 0x40);
		ret += ov7675_write(client, 0x6b, 0x0a);
		ret += ov7675_write(client, 0x2a, 0x00);
		ret += ov7675_write(client, 0x2b, 0x00);
		ret += ov7675_write(client, 0x2d, 0x00);
		ret += ov7675_write(client, 0x2e, 0x00);
		ret += ov7675_write(client, 0xca, 0x00);
		ret += ov7675_write(client, 0x92, 0x66);
		ret += ov7675_write(client, 0x93, 0x00);
	}

	return ret;
}


static int ov7675_q_colorfx(struct i2c_client *client, __s32 *value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);

	*value = decoder->current_colorfx;
	return 0;
}

static int ov7675_t_colorfx(struct i2c_client *client, int value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);
	unsigned char v1 = 0, v2 = 0, v3 = 0;
	int ret;

	if (decoder->current_colorfx == value)
		return 0;

	decoder->current_colorfx = value;

	ret = ov7675_read(client, REG_TSLB, &v1);

	v1 &= ~(0x30);

	switch (value) {
		case V4L2_COLORFX_NONE:
			v1 |= 0x00;
			v2 |= 0x80;
			v3 |= 0x80;
			break;
		case V4L2_COLORFX_BW:
			v1 |= 0x10;
			v2 |= 0x80;
			v3 |= 0x80;
			break;
		case V4L2_COLORFX_SEPIA:
			v1 |= 0x10;
			v2 |= 0xa0;
			v3 |= 0x40;
			break;
		case V4L2_COLORFX_NEGATIVE:
			v1 |= 0x20;
			v2 |= 0x80;
			v3 |= 0x80;
			break;
		case V4L2_COLORFX_BLUISH:
			v1 |= 0x10;
			v2 |= 0x80;
			v3 |= 0xc0;
			break;
		case V4L2_COLORFX_GREENISH:
			v1 |= 0x10;
			v2 |= 0x40;
			v3 |= 0x40;
			break;
		case V4L2_COLORFX_REDISH:
			v1 |= 0x10;
			v2 |= 0xc0;
			v3 |= 0x80;
			break;
		case V4L2_COLORFX_AQUA:
			v1 |= 0x10;
			v2 |= 0x40;
			v3 |= 0xc0;
			break;
	}

	ret += ov7675_write(client, REG_TSLB, v1);
	ret += ov7675_write(client, REG_MANU, v2);
	ret += ov7675_write(client, REG_MANV, v3);

	return ret;
}


static int ov7675_q_manual_wb(struct i2c_client *client, __s32 *value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);

	*value = decoder->current_manual_wb;
	return 0;
}

static int ov7675_t_manual_wb(struct i2c_client *client, int value)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);
	unsigned char v1 = 0, v2 = 0, v3 = 0;
	int ret;

	decoder->current_manual_wb = value;

	/*
         * A gain of x1,28125 for the Red channel, of x1 for the Green channel
         * and of x1,21875 for the Blue channel have been added to compensate
         * the lens tint. According to OV Datasheet: Gain = (Register Value)/0x40
         */
	switch (value) {
		case 2600:
			// TUNGSTEN / INCANDESCENT 40W
			// Without compensation (R,G,B): 0x40, 0x52, 0x72
			v1 = 0x52;
			v2 = 0x52;
			v3 = 0x8A;
			break;
		case 2850:
			// TUNGSTEN / INCANDESCENT 100W
			// Without compensation (R,G,B): 0x40, 0x4C, 0x60
			v1 = 0x52;
			v2 = 0x4C;
			v3 = 0x75;
			break;
		case 4200:
			// FLUORESCENT
			// Without compensation (R,G,B): 0x40, 0x42, 0x43
			v1 = 0x52;
			v2 = 0x42;
			v3 = 0x51;
			break;
		case 6000:
			// DAYLIGHT
			// Without compensation (R,G,B): 0x40, 0x40, 0x40
			v1 = 0x52;
			v2 = 0x40;
			v3 = 0x4E;
			break;
		case 7000:
			// CLOUDY_DAYLIGHT
			// Without compensation (R,G,B): 0x51, 0x48, 0x40
			v1 = 0x67;
			v2 = 0x48;
			v3 = 0x4E;
			break;
		case 9500:
			// SHADE
			// Without compensation (R,G,B): 0x61, 0x53, 0x40
			v1 = 0x7C;
			v2 = 0x53;
			v3 = 0x4E;
			break;
		default:
			return -1;
	}

	ret = ov7675_write(client, REG_RED, v1);
	ret += ov7675_write(client, REG_GREEN, v2);
	ret += ov7675_write(client, REG_BLUE, v3);

	return ret;
}


static int ov7675_q_auto_wb(struct i2c_client *client, __s32 *value)
{
	int ret;
	unsigned char v = 0;

	ret = ov7675_read(client, REG_COM8, &v);
	*value = (v & COM8_AWB) == COM8_AWB;
	return ret;
}

static int ov7675_t_auto_wb(struct i2c_client *client, int value)
{
	unsigned char v = 0;
	int ret;

	ret = ov7675_read(client, REG_COM8, &v);
	if (value)
		v |= COM8_AWB;
	else
		v &= ~COM8_AWB;
	msleep(10);  /* FIXME */
	ret += ov7675_write(client, REG_COM8, v);
	return ret;
}


static int ov7675_q_do_wb(struct i2c_client *client, __s32 *value)
{
	*value = 0;
	return 0;
}

static int ov7675_t_do_wb(struct i2c_client *client, int value)
{
	unsigned char v = 0;
	int ret;

	ret = ov7675_t_auto_wb(client, 1);
	/* Wait for AWB */
	msleep(500);
	ret += ov7675_t_auto_wb(client, 0);
	return ret;
}


static struct ov7675_control ov7675_controls[] =
{
	{
		.qc = {
			.id = V4L2_CID_BRIGHTNESS,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Brightness",
			.minimum = 0,
			.maximum = 255,
			.step = 1,
			.default_value = 0x80,
			.flags = V4L2_CTRL_FLAG_SLIDER
		},
		.tweak = ov7675_t_brightness,
		.query = ov7675_q_brightness,
	},
	{
		.qc = {
			.id = V4L2_CID_CONTRAST,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Contrast",
			.minimum = 0,
			.maximum = 127,
			.step = 1,
			.default_value = 0x40,   /* XXX ov7675 spec */
			.flags = V4L2_CTRL_FLAG_SLIDER
		},
		.tweak = ov7675_t_contrast,
		.query = ov7675_q_contrast,
	},
	{
		.qc = {
			.id = V4L2_CID_SATURATION,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Saturation",
			.minimum = 0,
			.maximum = 256,
			.step = 1,
			.default_value = 0x80,
			.flags = V4L2_CTRL_FLAG_SLIDER
		},
		.tweak = ov7675_t_sat,
		.query = ov7675_q_sat,
	},
	{
		.qc = {
			.id = V4L2_CID_HUE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "HUE",
			.minimum = -180,
			.maximum = 180,
			.step = 5,
			.default_value = 0,
			.flags = V4L2_CTRL_FLAG_SLIDER
		},
		.tweak = ov7675_t_hue,
		.query = ov7675_q_hue,
	},
	{
		.qc = {
			.id = V4L2_CID_VFLIP,
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.name = "Vertical flip",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 0,
		},
		.tweak = ov7675_t_vflip,
		.query = ov7675_q_vflip,
	},
	{
		.qc = {
			.id = V4L2_CID_HFLIP,
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.name = "Horizontal mirror",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 0,
		},
		.tweak = ov7675_t_hflip,
		.query = ov7675_q_hflip,
	},
	{
		.qc = {
			.id = V4L2_CID_UNBANDING,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Unbanding 60Hz/50Hz",
			.minimum = 0,
			.maximum = 2,
			.step = 1,
			.default_value = 0,
		},
		.tweak = ov7675_t_unbanding,
		.query = ov7675_q_unbanding,
	},
	{
		.qc = {
			.id = V4L2_CID_NIGHT_MODE,
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.name = "Night Mode",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 0,
		},
		.tweak = ov7675_t_night_mode,
		.query = ov7675_q_night_mode,
	},
	{
		.qc = {
			.id = V4L2_CID_COLORFX,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Color Effects",
			.minimum = 0,
			.maximum = 8,
			.step = 1,
			.default_value = 0,
		},
		.tweak = ov7675_t_colorfx,
		.query = ov7675_q_colorfx,
	},
	{
		.qc = {
			.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "Manual White Balance",
			.minimum = 0,
			.maximum = 20000,
			.step = 1,
			.default_value = 6000,
		},
		.tweak = ov7675_t_manual_wb,
		.query = ov7675_q_manual_wb,
	},
	{
		.qc = {
			.id = V4L2_CID_AUTO_WHITE_BALANCE,
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.name = "Auto White Balance",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 1,
		},
		.tweak = ov7675_t_auto_wb,
		.query = ov7675_q_auto_wb,
	},
	{
		.qc = {
			.id = V4L2_CID_DO_WHITE_BALANCE,
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.name = "Do White Balance",
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 0,
		},
		.tweak = ov7675_t_do_wb,
		.query = ov7675_q_do_wb,
	},
};
#define N_CONTROLS (ARRAY_SIZE(ov7675_controls))

static struct ov7675_control *ov7675_find_control(__u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++)
		if (ov7675_controls[i].qc.id == id)
			return ov7675_controls + i;
	return NULL;
}


static int ov7675_queryctrl(struct i2c_client *client,
		struct v4l2_queryctrl *qc)
{
	struct ov7675_control *ctrl = ov7675_find_control(qc->id);

	if (ctrl == NULL)
		return -EINVAL;
	*qc = ctrl->qc;
	return 0;
}

static int ov7675_g_ctrl(struct i2c_client *client, struct v4l2_control *ctrl)
{
	struct ov7675_control *octrl = ov7675_find_control(ctrl->id);
	int ret;

	if (octrl == NULL)
		return -EINVAL;
	ret = octrl->query(client, &ctrl->value);
	if (ret >= 0)
		return 0;
	return ret;
}

static int ov7675_s_ctrl(struct i2c_client *client, struct v4l2_control *ctrl)
{
	struct ov7675_control *octrl = ov7675_find_control(ctrl->id);
	int ret;

	if (octrl == NULL)
		return -EINVAL;
	ret =  octrl->tweak(client, ctrl->value);
	if (ret >= 0)
		return 0;
	return ret;
}


/*
 * V4L2 IOCTLs
 */

static int ioctl_enum_framesizes(struct v4l2_int_device *s,
					struct v4l2_frmsizeenum *frms)
{
	int ifmt;
	

	for (ifmt = 0; ifmt < N_OV7675_FMTS; ifmt++) {
		if (frms->pixel_format == ov7675_formats[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == N_OV7675_FMTS)
		return -EINVAL;

	/* Do we already reached all discrete framesizes? */
	if (frms->index >= N_WIN_SIZES)
		return -EINVAL;

	frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frms->discrete.width = ov7675_win_sizes[frms->index].width;
	frms->discrete.height = ov7675_win_sizes[frms->index].height;

	return 0;
}

const struct v4l2_fract ov7675_frameintervals[] = {
	{  .numerator = 1, .denominator = 30 },
	{  .numerator = 1, .denominator = 25 },
	{  .numerator = 1, .denominator = 15 },
};

static int ioctl_enum_frameintervals(struct v4l2_int_device *s,
					struct v4l2_frmivalenum *frmi)
{
	int ifmt;
	

	for (ifmt = 0; ifmt < N_OV7675_FMTS; ifmt++) {
		if (frmi->pixel_format == ov7675_formats[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == N_OV7675_FMTS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frmi->index >= ARRAY_SIZE(ov7675_frameintervals))
		return -EINVAL;


	frmi->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frmi->discrete.numerator =
				ov7675_frameintervals[frmi->index].numerator;
	frmi->discrete.denominator =
				ov7675_frameintervals[frmi->index].denominator;
	

	return 0;
}

static int ioctl_enum_fmt_cap(struct v4l2_int_device *s, struct v4l2_fmtdesc *fmt)
{
	struct ov7675_decoder *decoder = s->priv;

	return ov7675_enum_fmt(decoder->client, fmt);
}

static int ioctl_try_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct ov7675_decoder *decoder = s->priv;

	return ov7675_try_fmt(decoder->client, f, NULL, NULL);
}

static int ioctl_s_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct ov7675_decoder *decoder = s->priv;

	return ov7675_s_fmt(decoder->client, f);
}

static int ioctl_g_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct ov7675_decoder *decoder = s->priv;
	
	f->fmt.pix = decoder->pix;
	
	return 0;
}

static int ioctl_g_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	struct ov7675_decoder *decoder = s->priv;

	return ov7675_g_parm(decoder->client, a);
}

static int ioctl_s_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	struct ov7675_decoder *decoder = s->priv;

	return ov7675_s_parm(decoder->client, a);
}

static int ioctl_queryctrl(struct v4l2_int_device *s, struct v4l2_queryctrl *qctrl)
{
	struct ov7675_decoder *decoder = s->priv;

	return ov7675_queryctrl(decoder->client, qctrl);
}

static int ioctl_g_ctrl(struct v4l2_int_device *s, struct v4l2_control *ctrl)
{
	struct ov7675_decoder *decoder = s->priv;

	return ov7675_g_ctrl(decoder->client, ctrl);
}

static int ioctl_s_ctrl(struct v4l2_int_device *s, struct v4l2_control *ctrl)
{
	struct ov7675_decoder *decoder = s->priv;

	return ov7675_s_ctrl(decoder->client, ctrl);
}

static int ioctl_g_ifparm(struct v4l2_int_device *s, struct v4l2_ifparm *p)
{
	struct ov7675_decoder *decoder = s->priv;
	int rval;

	if (p == NULL)
		return -EINVAL;

	if (NULL == decoder->pdata->ifparm)
		return -EINVAL;

	rval = decoder->pdata->ifparm(s, p);
	if (rval) {
		v4l_err(decoder->client, "g_ifparm.Err[%d]\n", rval);
		return rval;
	}

	return 0;
}

static int ioctl_g_priv(struct v4l2_int_device *s, void *p)
{
	struct ov7675_decoder *decoder = s->priv;

	if (NULL == decoder->pdata->priv_data_set)
		return -EINVAL;

	return decoder->pdata->priv_data_set(s, p);
}

static int ioctl_s_power(struct v4l2_int_device *s, enum v4l2_power on)
{
	struct ov7675_decoder *decoder = s->priv;
	int err = 0;

	switch (on) {
	case V4L2_POWER_OFF:
		/* Disable sensor clock */
		if (decoder->pdata->set_xclk)
			err = decoder->pdata->set_xclk(s, 0);

		/* Power Down Sequence */
		/* Disable mux for OV7675 sensor data path */
		if (decoder->pdata->power_set)
			err |= decoder->pdata->power_set(s, on);
		decoder->state = STATE_NOT_DETECTED;
		break;

	case V4L2_POWER_STANDBY:
//		if (decoder->pdata->power_set)
//			err = decoder->pdata->power_set(s, on);
//		break;

	case V4L2_POWER_ON:
		if ((decoder->pdata->power_set) &&
				(decoder->state == STATE_NOT_DETECTED)) {
			/* Power Up Sequence */
			/* Enable mux for OV7675 sensor data path */
			err = decoder->pdata->power_set(s, on);

			/* Enable sensor clock */
			if (decoder->pdata->set_xclk)
				err |= decoder->pdata->set_xclk(s, xclk_current);

			/* Wait a little before any I2C access (OV datasheet says 1 ms) */
			msleep(10);

			/* Detect the sensor */
			err |= ov7675_detect(decoder->client);
			if (err) {
				v4l_err(decoder->client,
						"Unable to detect decoder\n");
				/* Disable sensor clock */
				if (decoder->pdata->set_xclk)
					err = decoder->pdata->set_xclk(s, 0);
				return err;
			}
			err |= ov7675_init(decoder->client);
		}
		// A little delay is needed to get a good first frame.
		msleep(10);
		break;

	default:
		err = -ENODEV;
		break;
	}

	return err;
}

static int ioctl_init(struct v4l2_int_device *s)
{
	struct ov7675_decoder *decoder = s->priv;

	return ov7675_init(decoder->client);
}

static int ioctl_reset(struct v4l2_int_device *s)
{
	struct ov7675_decoder *decoder = s->priv;

	ov7675_reset(decoder->client);

	return 0;
}

static int ioctl_dev_exit(struct v4l2_int_device *s)
{
	return 0;
}

static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct ov7675_decoder *decoder = s->priv;
	int err;

	err = ov7675_detect(decoder->client);
	if (err < 0) {
		v4l_err(decoder->client,
			"Unable to detect decoder\n");
		return err;
	}

	return 0;
}


static struct v4l2_int_ioctl_desc ov7675_ioctl_desc[] = {
	{vidioc_int_enum_framesizes_num, (v4l2_int_ioctl_func *)ioctl_enum_framesizes},
	{vidioc_int_enum_frameintervals_num, (v4l2_int_ioctl_func *)ioctl_enum_frameintervals},
	{vidioc_int_dev_init_num, (v4l2_int_ioctl_func*) ioctl_dev_init},
	{vidioc_int_dev_exit_num, (v4l2_int_ioctl_func*) ioctl_dev_exit},
	{vidioc_int_s_power_num, (v4l2_int_ioctl_func*) ioctl_s_power},
	{vidioc_int_g_priv_num, (v4l2_int_ioctl_func*) ioctl_g_priv},
	{vidioc_int_g_ifparm_num, (v4l2_int_ioctl_func*) ioctl_g_ifparm},
	{vidioc_int_init_num, (v4l2_int_ioctl_func*) ioctl_init},
	{vidioc_int_reset_num, (v4l2_int_ioctl_func*) ioctl_reset},
	{vidioc_int_enum_fmt_cap_num,
	 (v4l2_int_ioctl_func *) ioctl_enum_fmt_cap},
	{vidioc_int_try_fmt_cap_num,
	 (v4l2_int_ioctl_func *) ioctl_try_fmt_cap},
	{vidioc_int_g_fmt_cap_num,
	 (v4l2_int_ioctl_func *) ioctl_g_fmt_cap},
	{vidioc_int_s_fmt_cap_num,
	 (v4l2_int_ioctl_func *) ioctl_s_fmt_cap},
	{vidioc_int_g_parm_num, (v4l2_int_ioctl_func *) ioctl_g_parm},
	{vidioc_int_s_parm_num, (v4l2_int_ioctl_func *) ioctl_s_parm},
	{vidioc_int_queryctrl_num,
	 (v4l2_int_ioctl_func *) ioctl_queryctrl},
	{vidioc_int_g_ctrl_num, (v4l2_int_ioctl_func *) ioctl_g_ctrl},
	{vidioc_int_s_ctrl_num, (v4l2_int_ioctl_func *) ioctl_s_ctrl},
};

static struct v4l2_int_slave ov7675_slave = {
	.ioctls = ov7675_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(ov7675_ioctl_desc),
};

static struct ov7675_decoder ov7675_dev = {
	.state = STATE_NOT_DETECTED,
	.fmt = &ov7675_formats[0],
	.pix = {
		.width = VGA_WIDTH,
		.height = VGA_HEIGHT,
		.pixelformat = V4L2_PIX_FMT_YUYV,
	},
	.sat = 128,
	.current_colorfx = V4L2_COLORFX_NONE,
	.current_unbanding_mode = V4L2_UNBANDING_50HZ,
	.current_night_mode = 0,
	.current_manual_wb = 6000,
};

static struct v4l2_int_device ov7675_int_device = {
	.module = THIS_MODULE,
	.name = OV7675_MODULE_NAME,
	.priv = &ov7675_dev,
	.type = v4l2_int_type_slave,
	.u = {
	      .slave = &ov7675_slave,
	      },
};


/*
 * Basic i2c stuff.
 */

static int ov7675_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ov7675_decoder *decoder = &ov7675_dev;
	int err;

	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	decoder->pdata = client->dev.platform_data;
	if (!decoder->pdata) {
		v4l_err(client, "No platform data!!\n");
		return -ENODEV;
	}

	/*
	 * Save the id data
	 */
	decoder->id = (struct i2c_device_id *)id;
	decoder->v4l2_int_device = &ov7675_int_device;
	decoder->client = client;
	i2c_set_clientdata(client, decoder);

	/* Register with V4L2 layer as slave device */
	err = v4l2_int_device_register(decoder->v4l2_int_device);
	if (err) {
		i2c_set_clientdata(client, NULL);
		v4l_err(client,
			"Unable to register to v4l2. Err[%d]\n", err);

	}

	return err;
}


static int __exit ov7675_remove(struct i2c_client *client)
{
	struct ov7675_decoder *decoder = i2c_get_clientdata(client);

	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */

	v4l2_int_device_unregister(decoder->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}


static const struct i2c_device_id ov7675_id[] = {
	{"ov7675", 0},
	{},
};

static struct i2c_driver ov7675_driver = {
	.driver = {
		.name = OV7675_MODULE_NAME,
		.owner = THIS_MODULE,
	},
	.probe = ov7675_probe,
	.remove = __exit_p(ov7675_remove),
	.id_table = ov7675_id,
};


/*
 * Module initialization
 */
static int __init ov7675_mod_init(void)
{
	printk(KERN_NOTICE "OmniVision ov7675 sensor driver, at your service\n");
	return i2c_add_driver(&ov7675_driver);
}

static void __exit ov7675_mod_exit(void)
{
	i2c_del_driver(&ov7675_driver);
}

module_init(ov7675_mod_init);
module_exit(ov7675_mod_exit);
