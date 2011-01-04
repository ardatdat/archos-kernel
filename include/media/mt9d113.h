/*
 *    mt9d113.h : 02/03/2010
 *    g.revaillot, revaillot@archos.com
 */

#ifndef _MT9D113_H
#define _MT9D113_H

// bcause SADDR=0L and addr on 8bit
#define MT9D113_I2C_ADDR		0x78  >> 1

#define MT9D113_USE_XCLKA	0
#define MT9D113_USE_XCLKB	1

#define MT9D113_CLOCK_14_4MHZ	14400000
#define MT9D113_CLOCK_24MHZ	24000000

/*
 * Our nominal (default) frame rate.
 */
#define MT9D113_FRAME_RATE	30

struct mt9d113_platform_data {
	int (*reset_pulse) (struct v4l2_int_device *s);
	int (*power_set) (struct v4l2_int_device *s, enum v4l2_power on);
	int (*ifparm) (struct v4l2_int_device *s, struct v4l2_ifparm *p);
	int (*priv_data_set) (struct v4l2_int_device *s, void *);
	u32 (*set_xclk)(struct v4l2_int_device *s, u32 xclkfreq);
};
struct i2c_client *mt9d113_client;

#endif /* _MT9D113_H */
