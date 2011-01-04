/*
 * mt9d113.c : 02/03/2010
 * g.revaillot, revaillot@archos.com
 * Jean-Christophe Rona, rona@archos.com
 *
 * V4L2 driver for Aptina MT9D113 cameras.
 *
 * Leverage mt9p012.c and imx046.c drivers
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/i2c.h>
#include <linux/delay.h>

#include <media/v4l2-int-device.h>
#include <media/mt9d113.h>

#include "omap34xxcam.h"
#include "isp/isp.h"
#include "isp/ispcsi2.h"

#include "mt9d113_regs.h"

/* Define this to limit the camera output in 720p @ 25fps */
#define LIMIT_720P_AT_25FPS 0
#define LIMIT_720P_AT_24FPS 1

static struct i2c_driver mt9d113sensor_i2c_driver;

const static struct v4l2_fmtdesc mt9d113_formats[] = {
	{
		.description	= "YUYV 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	},
};
#define NUM_CAPTURE_FORMATS ARRAY_SIZE(mt9d113_formats)

/* mt9d113_framesizes and mt9d113_framesizes_settings elements HAVE to be in the same order */
const static struct v4l2_frmsize_discrete mt9d113_framesizes[] = {
	{ 176, 144 },
	{ 320, 240 },
	{ 640, 480 },
	{ 800, 480 },
	{ 800, 600 },
	{ 1280, 720 },
	{ 1600, 1200},
};
#define NUM_FRAME_SIZES ARRAY_SIZE(mt9d113_framesizes)

/*
 * mt9d113_framesizes and mt9d113_framesizes_settings elements HAVE to be in the same order
 *
 * Some explanations :
 *    "active lines" = sensor_row_end - sensor_row_start
 *    "active pixels" = sensor_col_end - sensor_col_start
 *    If the context is A (Preview):
 *       frame_length = "active lines"/2 + "needed blank lines"
 *       line_length = "active pixels"/2 + "needed blank pixels"
 *    If the context is B (Capture/Video):
 *       frame_length = "active lines" + "needed blank lines"
 *       line_length = "active pixels" + "needed blank pixels"
 *
 * FPS = 1/((frame_length*line_length*2)/pixel_clock)
 * We are in YUV (2 Bytes per pixel), that's why a "*2" is needed
 *
 * Flicker Detection (from developer guide):
 *    AC @ 50 Hz -> lights flicker @ 100Hz
 *    AC @ 60 Hz -> lights flicker @ 120Hz
 *
 *    R9_step_50 = (frame_length * FPS)/100
 *    R9_step_60 = (frame_length * FPS)/120
 *
 *    fd_search_f1_N ~ R9_step_N/5 - 1
 *    fd_search_f2_N ~ R9_step_N/5 + 1
 *
 *    Aptina/Quadrant default values do not seem to respect that at all !!
 */
const static struct mt9d113_framesize_settings mt9d113_framesizes_settings[] = {
	/* QCIF */
	{
		.clk_cfg = NULL,
		.regs = NULL,
		.ctx = CONTEXT_A,

		.crop_X0 = 0,
		.crop_X1 = 799,
		.crop_Y0 = 0,
		.crop_Y1 = 599,

		/* Default values from Aptina */
		.sensor_row_start = 0,
		.sensor_row_end = 1213,
		.sensor_col_start = 0,
		.sensor_col_end = 1613,

		.frame_length = 699,
		.line_length = 2184,

		.R9_step = 160,
		.R9_step_50 = 192,
		.R9_step_60 = 160,
		.fd_search_f1_50 = 38,
		.fd_search_f2_50 = 41,
		.fd_search_f1_60 = 46,
		.fd_search_f2_60 = 49,
	},
	/* QVGA */
	{
		.clk_cfg = NULL,
		.regs = NULL,
		.ctx = CONTEXT_A,

		.crop_X0 = 0,
		.crop_X1 = 799,
		.crop_Y0 = 0,
		.crop_Y1 = 599,

		/* Default values from Aptina */
		.sensor_row_start = 0,
		.sensor_row_end = 1213,
		.sensor_col_start = 0,
		.sensor_col_end = 1613,

		.frame_length = 699,
		.line_length = 2184,

		.R9_step = 160,
		.R9_step_50 = 192,
		.R9_step_60 = 160,
		.fd_search_f1_50 = 38,
		.fd_search_f2_50 = 41,
		.fd_search_f1_60 = 46,
		.fd_search_f2_60 = 49,
	},
	/* VGA */
	{
		.clk_cfg = NULL,
		.regs = NULL,
		.ctx = CONTEXT_A,

		.crop_X0 = 0,
		.crop_X1 = 799,
		.crop_Y0 = 0,
		.crop_Y1 = 599,

		/* Default values from Aptina */
		.sensor_row_start = 0,
		.sensor_row_end = 1213,
		.sensor_col_start = 0,
		.sensor_col_end = 1613,

		.frame_length = 699,
		.line_length = 2184,

		.R9_step = 160,
		.R9_step_50 = 192,
		.R9_step_60 = 160,
		.fd_search_f1_50 = 38,
		.fd_search_f2_50 = 41,
		.fd_search_f1_60 = 46,
		.fd_search_f2_60 = 49,
	},
	/* WVGA */
	{
		.clk_cfg = NULL,
		.regs = NULL,
		.ctx = CONTEXT_A,

		.crop_X0 = 0,
		.crop_X1 = 799,
		.crop_Y0 = 60,
		.crop_Y1 = 539,

		/* Default values from Aptina */
		.sensor_row_start = 0,
		.sensor_row_end = 1213,
		.sensor_col_start = 0,
		.sensor_col_end = 1613,

		.frame_length = 699,
		.line_length = 2184,

		.R9_step = 160,
		.R9_step_50 = 192,
		.R9_step_60 = 160,
		.fd_search_f1_50 = 38,
		.fd_search_f2_50 = 41,
		.fd_search_f1_60 = 46,
		.fd_search_f2_60 = 49,
	},
	/* SVGA */
	{
		.clk_cfg = NULL,
		.regs = NULL,
		.ctx = CONTEXT_A,

		.crop_X0 = 0,
		.crop_X1 = 799,
		.crop_Y0 = 0,
		.crop_Y1 = 599,

		/* Default values from Aptina */
		.sensor_row_start = 0,
		.sensor_row_end = 1213,
		.sensor_col_start = 0,
		.sensor_col_end = 1613,

		.frame_length = 699,
		.line_length = 2184,

		.R9_step = 160,
		.R9_step_50 = 192,
		.R9_step_60 = 160,
		.fd_search_f1_50 = 38,
		.fd_search_f2_50 = 41,
		.fd_search_f1_60 = 46,
		.fd_search_f2_60 = 49,
	},
	/* 720p */
	{
		.clk_cfg = NULL,
		.regs = NULL,
		.ctx = CONTEXT_B,

		.crop_X0 = 0,
		.crop_X1 = 1279,
		.crop_Y0 = 0,
		.crop_Y1 = 719,

		/* These values are optimized for 720p
		   (active pixels and signal "geometry"),
		   and give about 31 fps */
		.sensor_row_start = 246,
		.sensor_row_end = 973,
		.sensor_col_start = 166,
		.sensor_col_end = 1453,

#ifdef LIMIT_720P_AT_24FPS
		.frame_length = 950,
		.line_length = 1840,
#elif LIMIT_720P_AT_25FPS
		.frame_length = 950,
		.line_length = 1730,
#else
		.frame_length = 816,		// We are close to the limits : 813 does not work
		.line_length = 1656,		// We are close to the limits : 1650 does not work
#endif

		.R9_step = 173,
#ifdef LIMIT_720P_AT_24FPS
		.R9_step_50 = 228,		// (950*25,5)/(2*50)
		.R9_step_60 = 190,		// (950*25,5)/(2*60)
		.fd_search_f1_50 = 45,
		.fd_search_f2_50 = 47,
		.fd_search_f1_60 = 37,
		.fd_search_f2_60 = 49,
#elif LIMIT_720P_AT_25FPS
		.R9_step_50 = 242,		// (950*25,5)/(2*50)
		.R9_step_60 = 202,		// (950*25,5)/(2*60)
		.fd_search_f1_50 = 47,
		.fd_search_f2_50 = 49,
		.fd_search_f1_60 = 39,
		.fd_search_f2_60 = 41,
#else
		.R9_step_50 = 253,		// (816*31)/(2*50)
		.R9_step_60 = 211,		// (816*31)/(2*60)
		.fd_search_f1_50 = 50,
		.fd_search_f2_50 = 52,
		.fd_search_f1_60 = 41,
		.fd_search_f2_60 = 43,
#endif

	},
	/* UXGA */
	{
		.clk_cfg = NULL,
		.regs = NULL,
		.ctx = CONTEXT_B,

		.crop_X0 = 0,
		.crop_X1 = 1599,
		.crop_Y0 = 0,
		.crop_Y1 = 1199,

		/* Default values from Aptina */
		.sensor_row_start = 4,
		.sensor_row_end = 1213,
		.sensor_col_start = 4,
		.sensor_col_end = 1611,

		.frame_length = 1313,
		.line_length = 2184,

		.R9_step = 160,
		.R9_step_50 = 192,
		.R9_step_60 = 160,
		.fd_search_f1_50 = 38,
		.fd_search_f2_50 = 41,
		.fd_search_f1_60 = 46,
		.fd_search_f2_60 = 49,
	},
};

const struct v4l2_fract mt9d113_frameintervals[] = {
	{  .numerator = 1, .denominator = 30 },
	//{  .numerator = 1, .denominator = 15 },
};
#define NUM_FRAME_INTERVALS ARRAY_SIZE(mt9d113_frameintervals)

static struct v4l2_queryctrl mt9d113_controls[] = {
	{
		.id = V4L2_CID_BRIGHTNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Brightness",
		.minimum = 0,
		.maximum = 255,
		.step = 1,
		.default_value = 40,
		.flags = V4L2_CTRL_FLAG_SLIDER
	},
	{
		.id = V4L2_CID_GAMMA,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Gamma",
		.minimum = 0,
		.maximum = 255,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_SLIDER
	},
	{
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Exposure",
		.minimum = 0,
		.maximum = 65535,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_SLIDER
	},
	{
		.id = V4L2_CID_UNBANDING,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Unbanding 60Hz/50Hz",
		.minimum = 0,
		.maximum = 3,
		.step = 1,
		.default_value = 3,
	},
	{
		.id = V4L2_CID_COLORFX,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Color Effects",
		.minimum = 0,
		.maximum = 8,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_NIGHT_MODE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Night Mode",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Manual White Balance",
		.minimum = 0,
		.maximum = 20000,
		.step = 1,
		.default_value = 6000,
	},
	{
		.id = V4L2_CID_AUTO_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Auto White Balance",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 1,
	},
};
#define NUM_CONTROLS ARRAY_SIZE(mt9d113_controls)

const static struct reg_value rev2_errata_issue02[] = {
	{ 0x3084, 0x240C },
	{ 0x3092, 0x0A4C },
	{ 0x3094, 0x4C4C },
	{ 0x3096, 0x4C54 },
	{ 0, 0}
};

const static struct reg_value sequencer_sync[] = {
	{ MT9D113_R2444_MCU_ADDRESS, 0xA103 },  
	{ MT9D113_R2448_VARIABLE_DATA0, 0x06 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xA103 },  
	{ MT9D113_R2448_VARIABLE_DATA0, 0x05 },
	{ 0, 0}
};

const static struct reg_value preview_mode[] = {
	{ MT9D113_R2444_MCU_ADDRESS, 0xA103 },  
	{ MT9D113_R2448_VARIABLE_DATA0, 0x01 },
	{ 0, 0}
};

const static struct reg_value capture_mode[] = {
	{ MT9D113_R2444_MCU_ADDRESS, 0xA116 },  
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xA103 },  
	{ MT9D113_R2448_VARIABLE_DATA0, 0x02 },
	{ 0, 0}
};

const static struct reg_value init_wizard_setup[] = {
	/* Revision 3 init */
	// MODE_SENSOR_FINE_CORRECTION_A
	{ MT9D113_R2444_MCU_ADDRESS, 0x2719 }, 	//  sensor_fine_correction (A)
	{ MT9D113_R2448_VARIABLE_DATA0, 0x005A }, 	//        = 90
	// MODE_SENSOR_FINE_IT_MIN_A
	{ MT9D113_R2444_MCU_ADDRESS, 0x271B }, 	//  sensor_fine_IT_min (A)
	{ MT9D113_R2448_VARIABLE_DATA0, 0x01BE }, 	//        = 446
	// MODE_SENSOR_FINE_IT_MAX_MARGIN_A
	{ MT9D113_R2444_MCU_ADDRESS, 0x271D }, 	//  sensor_fine_IT_max_margin (A)
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0131 }, 	//        = 305
	//  MODE_SENSOR_FRAME_LENGTH_A
	{ MT9D113_R2444_MCU_ADDRESS, 0x271F }, 	//  Frame Lines (A)
	{ MT9D113_R2448_VARIABLE_DATA0, 699 }, 	//        = 699 
	// MODE_SENSOR_LINE_LENGTH_PCK_A
	{ MT9D113_R2444_MCU_ADDRESS, 0x2721 }, 	//  Line Length (A)
	{ MT9D113_R2448_VARIABLE_DATA0, 2184 }, //        = 2184

	// MODE_SENSOR_FINE_CORRECTION_B
	{ MT9D113_R2444_MCU_ADDRESS, 0x272F }, 	//  sensor_fine_correction (B)
	{ MT9D113_R2448_VARIABLE_DATA0, 58 }, 	//        = 58
	// MODE_SENSOR_FINE_IT_MIN_B
	{ MT9D113_R2444_MCU_ADDRESS, 0x2731 }, 	//  sensor_fine_IT_min (B)
	{ MT9D113_R2448_VARIABLE_DATA0, 246 }, 	//        = 246
	// MODE_SENSOR_FINE_IT_MAX_MARGIN_B
	{ MT9D113_R2444_MCU_ADDRESS, 0x2733 }, 	//  sensor_fine_IT_max_margin (B)
	{ MT9D113_R2448_VARIABLE_DATA0, 139 }, 	//        = 139

	{ MT9D113_R2444_MCU_ADDRESS, 0x2735 }, 	//  Frame Lines (B)
	{ MT9D113_R2448_VARIABLE_DATA0, 1313 }, //        = 1313
	{ MT9D113_R2444_MCU_ADDRESS, 0x2737 }, 	//  Line Length (B)
	{ MT9D113_R2448_VARIABLE_DATA0, 2184 }, //        = 2184

	{ MT9D113_R2444_MCU_ADDRESS, 0x275F }, 	// MODE_COMMONMODESETTINGS_BRIGHT_COLOR_KILL
	{ MT9D113_R2448_VARIABLE_DATA0, 404 }, 	// VAR= 7, 0x005F, 404
	{ MT9D113_R2444_MCU_ADDRESS, 0x2761 },	// MODE_COMMONMODESETTINGS_DARK_COLOR_KILL
	{ MT9D113_R2448_VARIABLE_DATA0, 20 },	//        = 20
	{ MT9D113_R2444_MCU_ADDRESS, 0xA765 },  // MODE_COMMONMODESETTINGS_FILTER_MODE
	{ MT9D113_R2448_VARIABLE_DATA0, 68 }, 	// VAR8= 7, 0x0065, 68

	{ MT9D113_R2444_MCU_ADDRESS, 0xA24F }, 	// AE_BASETARGET
	{ MT9D113_R2448_VARIABLE_DATA0, 40 }, 	// VAR8= 2, 0x004F, 40
	{ MT9D113_R2444_MCU_ADDRESS, 0xA20E },  // AE_MAX_VIRTGAIN decreased to ensure gain does not roll over in normal lighting
	{ MT9D113_R2448_VARIABLE_DATA0, 160 },  //        = 160
	{ MT9D113_R2444_MCU_ADDRESS, 0xA20C },  // AE_MAX_INDEX
	{ MT9D113_R2448_VARIABLE_DATA0, 14 }, 	// VAR8= 2, 0x000C, 19 sets fps to be a low of 5fps
	{ MT9D113_R2444_MCU_ADDRESS, 0x2212 },  // ae_max_dgain_ae1
	{ MT9D113_R2448_VARIABLE_DATA0, 494 }, 	//        = 494

	{ MT9D113_R2444_MCU_ADDRESS, 0x222D }, 	//  R9 Step
	{ MT9D113_R2448_VARIABLE_DATA0, 173 }, 	//        = 212
	{ MT9D113_R2444_MCU_ADDRESS, 0xA408 }, 	//  search_f1_50
	{ MT9D113_R2448_VARIABLE_DATA0, 41 }, 	//        = 51
	{ MT9D113_R2444_MCU_ADDRESS, 0xA409 }, 	//  search_f2_50
	{ MT9D113_R2448_VARIABLE_DATA0, 44 }, 	//        = 54
	{ MT9D113_R2444_MCU_ADDRESS, 0xA40A }, 	//  search_f1_60
	{ MT9D113_R2448_VARIABLE_DATA0, 49 }, 	//        = 61
	{ MT9D113_R2444_MCU_ADDRESS, 0xA40B }, 	//  search_f2_60
	{ MT9D113_R2448_VARIABLE_DATA0, 52 }, 	//        = 64
	{ MT9D113_R2444_MCU_ADDRESS, 0x2411 }, 	//  R9_Step_60 (A)
	{ MT9D113_R2448_VARIABLE_DATA0, 173 }, 	//        = 212
	{ MT9D113_R2444_MCU_ADDRESS, 0x2413 }, 	//  R9_Step_50 (A)
	{ MT9D113_R2448_VARIABLE_DATA0, 207 }, 	//        = 255
	{ MT9D113_R2444_MCU_ADDRESS, 0x2415 }, 	//  R9_Step_60 (B)
	{ MT9D113_R2448_VARIABLE_DATA0, 203 }, 	//        = 162
	{ MT9D113_R2444_MCU_ADDRESS, 0x2417 }, 	//  R9_Step_50 (B)
	{ MT9D113_R2448_VARIABLE_DATA0, 244 }, 	//        = 194

	// Set the maximum data slew rate (needed to capture a proper image @ 84 MHz)
	{ MT9D113_R30_PAD_SLEW, (4 & SYSCTL_SLEW_RATE_MASK) << SYSCTL_PCLK_SLEW_RATE_SHIFT |
				(7 & SYSCTL_SLEW_RATE_MASK) << SYSCTL_DATA_SLEW_RATE_SHIFT },

	// Output YUYV data
	{ MT9D113_R2444_MCU_ADDRESS, 0x2755 },  
	{ MT9D113_R2448_VARIABLE_DATA0, 0x02 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2757 },  
	{ MT9D113_R2448_VARIABLE_DATA0, 0x02 },

	{ MT9D113_R2444_MCU_ADDRESS, 0xA404 }, 	//  FD Mode
	{ MT9D113_R2448_VARIABLE_DATA0, 0x10 }, 	//        = 16
	{ MT9D113_R2444_MCU_ADDRESS, 0xA40D }, 	//  Stat_min
	{ MT9D113_R2448_VARIABLE_DATA0, 0x02 }, 	//        = 2
	{ MT9D113_R2444_MCU_ADDRESS, 0xA40E }, 	//  Stat_max
	{ MT9D113_R2448_VARIABLE_DATA0, 0x03 }, 	//        = 3
	{ MT9D113_R2444_MCU_ADDRESS, 0xA410 }, 	//  Min_amplitude
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0A }, 	//        = 10

	{ MT9D113_R2444_MCU_ADDRESS, 0xA103 }, 	//  Refresh Sequencer Mode
	{ MT9D113_R2448_VARIABLE_DATA0, 0x06 }, 	//        = 6
	{ MT9D113_R2444_MCU_ADDRESS, 0xA103 },  
	{ MT9D113_R2448_VARIABLE_DATA0, 0x05 },
	{ 0, 0}
};

const static struct reg_value low_light_settings[] = {
	{ MT9D113_R2444_MCU_ADDRESS, 0x2B28 },		// HG_LL_BRIGHTNESSSTART (VAR= 11, 0x0028), 13800
	{ MT9D113_R2448_VARIABLE_DATA0, 13800 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2B2A },		// HG_LL_BRIGHTNESSSTOP (VAR= 11, 0x002A), 46000
	{ MT9D113_R2448_VARIABLE_DATA0, 46000 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB20 },		// HG_LL_SAT1 (VAR8= 11, 0x0020), 75
	{ MT9D113_R2448_VARIABLE_DATA0, 75 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB24 },		// HG_LL_SAT2 (VAR8= 11, 0x0024), 0
	{ MT9D113_R2448_VARIABLE_DATA0, 0 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB25 },		// HG_LL_INTERPTHRESH2 (VAR8= 11, 0x0025), 255
	{ MT9D113_R2448_VARIABLE_DATA0, 255 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB30 },		// hg_nr_stop_r
	{ MT9D113_R2448_VARIABLE_DATA0, 255 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB31 },		// hg_nr_stop_g
	{ MT9D113_R2448_VARIABLE_DATA0, 255 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB32 },		// hg_nr_stop_b
	{ MT9D113_R2448_VARIABLE_DATA0, 255 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB33 },		// HG_NR_STOP_OL (VAR8= 11, 0x0033), 87
	{ MT9D113_R2448_VARIABLE_DATA0, 87 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB34 },		// HG_NR_GAINSTART (VAR8= 11, 0x0034), 128
	{ MT9D113_R2448_VARIABLE_DATA0, 128 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB35 },		// HG_NR_GAINSTOP (VAR8= 11, 0x0035), 255
	{ MT9D113_R2448_VARIABLE_DATA0, 255 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB36 },		// HG_CLUSTERDC_TH
	{ MT9D113_R2448_VARIABLE_DATA0, 20 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xAB37 },		// HG_GAMMA_MORPH_CTRL Set to automatic mode
	{ MT9D113_R2448_VARIABLE_DATA0, 3 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2B38 },		// HG_GAMMASTARTMORPH @ 100 lux
	{ MT9D113_R2448_VARIABLE_DATA0, 13000 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2B3A },		// HG_GAMMASTOPMORPH @ 20 lux
	{ MT9D113_R2448_VARIABLE_DATA0, 31000 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2B62 },		// HG_FTB_START_BM Disable FTB
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFFFE },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2B64 },		// HG_FTB_STOP_BM Disable FTB
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFFFF },
	{ 0, 0}
};

const static struct reg_value awb_and_ccms[] = {
	{ MT9D113_R2444_MCU_ADDRESS, 0x2306 },		// AWB_CCM_L_0,   0x0180
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0180 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2308 },		// AWB_CCM_L_1,   0xFF00
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFF00 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x230A },		// AWB_CCM_L_2,   0x0080
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0080 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x230C },		// AWB_CCM_L_3,   0xFF66
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFF66 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x230E },		// AWB_CCM_L_4,   0x0180
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0180 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2310 },		// AWB_CCM_L_5,   0xFFEE
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFFEE },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2312 },		// AWB_CCM_L_6,   0xFFCD
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFFCD },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2314 },		// AWB_CCM_L_7,   0xFECD
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFECD },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2316 },		// AWB_CCM_L_8,   0x019A
	{ MT9D113_R2448_VARIABLE_DATA0, 0x019A },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2318 },		// AWB_CCM_L_9,   0x0020
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0020 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x231A },		// AWB_CCM_L_10,  0x0033
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0033 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x231C },		// AWB_CCM_RL_0,  0x0100
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0100 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x231E },		// AWB_CCM_RL_1,  0xFF9A
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFF9A },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2320 },		// AWB_CCM_RL_2,  0x0000
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0000 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2322 },		// AWB_CCM_RL_3,  0x004D
	{ MT9D113_R2448_VARIABLE_DATA0, 0x004D },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2324 },		// AWB_CCM_RL_4,  0xFFCD
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFFCD },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2326 },		// AWB_CCM_RL_5,  0xFFB8
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFFB8 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2328 },		// AWB_CCM_RL_6,  0x004D
	{ MT9D113_R2448_VARIABLE_DATA0, 0x004D },
	{ MT9D113_R2444_MCU_ADDRESS, 0x232A },		// AWB_CCM_RL_7,  0x0080
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0080 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x232C },		// AWB_CCM_RL_8,  0xFF66
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFF66 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x232E },		// AWB_CCM_RL_9,  0x0008
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0008 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x2330 },		// AWB_CCM_RL_10, 0xFFF7
	{ MT9D113_R2448_VARIABLE_DATA0, 0xFFF7 },

	{ MT9D113_R2444_MCU_ADDRESS, 0xA363 },		// AWB_TG_MIN0, 210
	{ MT9D113_R2448_VARIABLE_DATA0, 210 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xA364 },		// AWB_TG_MAX0, 238
	{ MT9D113_R2448_VARIABLE_DATA0, 238 },

	{ 0x3244, 0x0328 },				// AWB_CONFIG4, AWB fine tuning
	{ 0x323E, 0xC22C },				// AWB_CONFIG1, AWB fine tuning, bits [11-15] controlled by MCU
	{ 0, 0}
};

const static struct reg_value rev3_SOC2030_patch[] = {
	// K25A_REV03_PATCH01_REV2
	{ MT9D113_R2444_MCU_ADDRESS, 0x415 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0xF601 },
	{ MT9D113_R2450_VARIABLE_DATA1, 0x42C1 },
	{ MT9D113_R2452_VARIABLE_DATA2, 0x0326 },
	{ MT9D113_R2454_VARIABLE_DATA3, 0x11F6 },
	{ MT9D113_R2456_VARIABLE_DATA4, 0x0143 },
	{ MT9D113_R2458_VARIABLE_DATA5, 0xC104 },
	{ MT9D113_R2460_VARIABLE_DATA6, 0x260A },
	{ MT9D113_R2462_VARIABLE_DATA7, 0xCC04 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x425 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0x33BD },
	{ MT9D113_R2450_VARIABLE_DATA1, 0xA362 },
	{ MT9D113_R2452_VARIABLE_DATA2, 0xBD04 },
	{ MT9D113_R2454_VARIABLE_DATA3, 0x3339 },
	{ MT9D113_R2456_VARIABLE_DATA4, 0xC6FF },
	{ MT9D113_R2458_VARIABLE_DATA5, 0xF701 },
	{ MT9D113_R2460_VARIABLE_DATA6, 0x6439 },
	{ MT9D113_R2462_VARIABLE_DATA7, 0xFE02 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x435 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0xBD18 },
	{ MT9D113_R2450_VARIABLE_DATA1, 0xCE03 },
	{ MT9D113_R2452_VARIABLE_DATA2, 0x25CC },
	{ MT9D113_R2454_VARIABLE_DATA3, 0x0011 },
	{ MT9D113_R2456_VARIABLE_DATA4, 0xBDC2 },
	{ MT9D113_R2458_VARIABLE_DATA5, 0xB8CC },
	{ MT9D113_R2460_VARIABLE_DATA6, 0x0470 },
	{ MT9D113_R2462_VARIABLE_DATA7, 0xFD03 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x445 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0x33CC },
	{ MT9D113_R2450_VARIABLE_DATA1, 0x0325 },
	{ MT9D113_R2452_VARIABLE_DATA2, 0xFD02 },
	{ MT9D113_R2454_VARIABLE_DATA3, 0xBDDE },
	{ MT9D113_R2456_VARIABLE_DATA4, 0x0018 },
	{ MT9D113_R2458_VARIABLE_DATA5, 0xCE03 },
	{ MT9D113_R2460_VARIABLE_DATA6, 0x37CC },
	{ MT9D113_R2462_VARIABLE_DATA7, 0x0037 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x455 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0xBDC2 },
	{ MT9D113_R2450_VARIABLE_DATA1, 0xB8CC },
	{ MT9D113_R2452_VARIABLE_DATA2, 0x0497 },
	{ MT9D113_R2454_VARIABLE_DATA3, 0xFD03 },
	{ MT9D113_R2456_VARIABLE_DATA4, 0x5BCC },
	{ MT9D113_R2458_VARIABLE_DATA5, 0x0337 },
	{ MT9D113_R2460_VARIABLE_DATA6, 0xDD00 },
	{ MT9D113_R2462_VARIABLE_DATA7, 0xC601 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x465 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0xF701 },
	{ MT9D113_R2450_VARIABLE_DATA1, 0x645C },
	{ MT9D113_R2452_VARIABLE_DATA2, 0xF701 },
	{ MT9D113_R2454_VARIABLE_DATA3, 0x657F },
	{ MT9D113_R2456_VARIABLE_DATA4, 0x0166 },
	{ MT9D113_R2458_VARIABLE_DATA5, 0x39BD },
	{ MT9D113_R2460_VARIABLE_DATA6, 0xD661 },
	{ MT9D113_R2462_VARIABLE_DATA7, 0xF602 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x475 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0xF4C1 },
	{ MT9D113_R2450_VARIABLE_DATA1, 0x0126 },
	{ MT9D113_R2452_VARIABLE_DATA2, 0x0BFE },
	{ MT9D113_R2454_VARIABLE_DATA3, 0x02BD },
	{ MT9D113_R2456_VARIABLE_DATA4, 0xEE10 },
	{ MT9D113_R2458_VARIABLE_DATA5, 0xFC02 },
	{ MT9D113_R2460_VARIABLE_DATA6, 0xF5AD },
	{ MT9D113_R2462_VARIABLE_DATA7, 0x0039 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x485 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0xF602 },
	{ MT9D113_R2450_VARIABLE_DATA1, 0xF4C1 },
	{ MT9D113_R2452_VARIABLE_DATA2, 0x0226 },
	{ MT9D113_R2454_VARIABLE_DATA3, 0x0AFE },
	{ MT9D113_R2456_VARIABLE_DATA4, 0x02BD },
	{ MT9D113_R2458_VARIABLE_DATA5, 0xEE10 },
	{ MT9D113_R2460_VARIABLE_DATA6, 0xFC02 },
	{ MT9D113_R2462_VARIABLE_DATA7, 0xF7AD },
	{ MT9D113_R2444_MCU_ADDRESS, 0x495 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0x0039 },
	{ MT9D113_R2450_VARIABLE_DATA1, 0x3CBD },
	{ MT9D113_R2452_VARIABLE_DATA2, 0xB059 },
	{ MT9D113_R2454_VARIABLE_DATA3, 0xCC00 },
	{ MT9D113_R2456_VARIABLE_DATA4, 0x28BD },
	{ MT9D113_R2458_VARIABLE_DATA5, 0xA558 },
	{ MT9D113_R2460_VARIABLE_DATA6, 0x8300 },
	{ MT9D113_R2462_VARIABLE_DATA7, 0x0027 },
	{ MT9D113_R2444_MCU_ADDRESS, 0x4A5 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0xBCC },
	{ MT9D113_R2450_VARIABLE_DATA1, 0x0026 },
	{ MT9D113_R2452_VARIABLE_DATA2, 0x30ED },
	{ MT9D113_R2454_VARIABLE_DATA3, 0x00C6 },
	{ MT9D113_R2456_VARIABLE_DATA4, 0x03BD },
	{ MT9D113_R2458_VARIABLE_DATA5, 0xA544 },
	{ MT9D113_R2460_VARIABLE_DATA6, 0x3839 },

	// Execute the patch (Monitor ID = 0)
	{ MT9D113_R2444_MCU_ADDRESS, 0x2006 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0X0415 },
	{ MT9D113_R2444_MCU_ADDRESS, 0xA005 },
	{ MT9D113_R2448_VARIABLE_DATA0, 0x01 },
	{ 0, 0}
};

struct mt9d113_pll default_pll_config = {
	// If fIn (XclkA) = 24 MHz
	// fBit = 492 MHz
	// fWord = 61,5 MHz
	// fSensor = 41 MHz
	// fSystem = 82 MHz
	.n = 3,
	.m = 41,
	.p1 = 5,
	.p3 = 0,
	.wcd = 0,
};

struct mt9d113_clk_config default_clk_config = {
	.ext_clk = MT9D113_CLOCK_24MHZ,
	.pll_config = &default_pll_config,
};

static struct mt9d113_sensor mt9d113_dev = {
	.clk_config = &default_clk_config,
};

static int find_vctrl_idx(int id)
{
	int idx;

	for (idx = 0; idx < NUM_CONTROLS; idx++) {
		if (mt9d113_controls[idx].id == id)
			return idx;
	}

	return -1;
}

static int mt9d113_read_reg(struct i2c_client *client, u16 reg, u16 *val)
{
	int err = 0;
	struct i2c_msg msg[2];
	unsigned char data[2];
	int retry = 0;

	//v4l_info(client,"%s 0x%04x\n", __FUNCTION__, reg);

	if (!client->adapter)
		return -ENODEV;

	*val = 0;

read_again:
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = data;

	data[0] = (u8) (reg >> 8);;
	data[1] = (u8) (reg & 0xff);

	err = i2c_transfer(client->adapter, msg, 2);
	if (err < 0) {
		if (retry <= I2C_RETRY_COUNT) {
			v4l_warn(client, "%s: retry ... %d, err = %d\n", __FUNCTION__, retry, err);
			retry++;
			msleep_interruptible(100);
			goto read_again;
		} else
			return err;
	}

	*val = data[1] + (data[0] << 8);

	//v4l_info(client,"^-= 0x%04x\n", *val);

	return 0;
}

static int mt9d113_write_reg(struct i2c_client *client, u16 reg, u16 val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[4];
	int retry = 0;

	//v4l_info(client,"%s 0x%04x->0x%04x\n", __FUNCTION__, val, reg);

	if (!client->adapter)
		return -ENODEV;

write_again:
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 4;
	msg->buf = data;

	data[0] = (u8) (reg >> 8);
	data[1] = (u8) (reg & 0xff);
	data[2] = (val >> 8) & 0xff;
	data[3] = val & 0xff;

	err = i2c_transfer(client->adapter, msg, 1);
	if (err < 0) {
		if (retry <= I2C_RETRY_COUNT) {
			v4l_warn(client, "%s: retry ... %d, err = %d\n", __FUNCTION__, retry, err);
			retry++;
			msleep_interruptible(100);
			goto write_again;
		} else
			return err;
	}

	return 0;
}

static int mt9d113_write_reg_seq(struct i2c_client *client, const struct reg_value *reg_list)
{
	int err = 0;
	//printk("%s\n", __FUNCTION__);

	while ( !((reg_list->reg == 0) && (reg_list->val == 0)) && (err >= 0)) {
		err = mt9d113_write_reg(client, reg_list->reg, reg_list->val);
		reg_list++;
	}

	return err;
}

void mt9d113_dump_regs(struct i2c_client *client, u16 reg_base, u16 dump_len)
{
	u16 ctrl;
	u16 reg;

	for (reg = reg_base; reg < reg_base+2*dump_len; reg+=2) {
		int i;
		mt9d113_read_reg(client, reg, &ctrl);
		printk("reg 0x%04x val 0x%04x : ", reg, ctrl);
		for (i=15; i>=0; i--) {
			printk("%d", !!(ctrl & (1<<i)));
			if ((i%4) == 0) printk(" ");
		}
		printk("\n");
	}
}

static int mt9d113_read_xdma(struct i2c_client *client, int id, u8 reg, u16 *val, int read_num, int length)
{
	u16 data_reg;
	u16 data_reg_end;
	int ret;

	if (read_num > 8)
		return -EINVAL;

	//v4l_info(client,"%s mod %d, offset 0x%04x, size %d\n", __FUNCTION__, id, reg, read_num);

	ret = mt9d113_write_reg(client, MT9D113_R2444_MCU_ADDRESS, XDMA_LOGICAL_MODE | (id << 8) | reg | (length ? 1 : 0) << 15);
	if (ret < 0)
		return ret;

	data_reg = MT9D113_R2448_VARIABLE_DATA0;
	data_reg_end = data_reg + read_num * 2;

	// XXX: not nice, must use i2c burst read.
	while (data_reg < data_reg_end) {
		mt9d113_read_reg(client, data_reg, val);
		if (ret < 0)
			return ret;

		data_reg+=2;
		val++;
	}

	return 0;
}

static int mt9d113_write_xdma(struct i2c_client *client, int id, u8 reg, u16 *val, int write_num, int length)
{
	u16 data_reg;
	u16 data_reg_end;
	int ret;

	if (write_num > 8)
		return -EINVAL;

	//v4l_info(client,"%s mod %d, offset 0x%04x, size %d\n", __FUNCTION__, id, reg, write_num);

	ret = mt9d113_write_reg(client, MT9D113_R2444_MCU_ADDRESS, XDMA_LOGICAL_MODE | (id << 8) | reg | (length ? 1 : 0) << 15);
	if (ret < 0)
		return ret;

	data_reg = MT9D113_R2448_VARIABLE_DATA0;
	data_reg_end = data_reg + write_num * 2;

	while (data_reg < data_reg_end) {
		mt9d113_write_reg(client, data_reg, *val);
		if (ret < 0)
			return ret;

		data_reg+=2;
		val++;
	}

	return 0;
}
static int mt9d113_detect(struct i2c_client *client)
{
	u16 chip_id = -1;

	//printk("%s\n", __FUNCTION__);

	if (!client)
		return -ENODEV;

	if (mt9d113_read_reg(client, MT9D113_R0_CHIP_ID, &chip_id))
		return -ENODEV;

	if (chip_id != SYSCTL_CHIP_ID) {
		v4l_warn(client, "Chip ID == 0x%04x, waiting for 0x%04x\n", chip_id, SYSCTL_CHIP_ID);
		return -ENODEV;
	}

	v4l_info(client, "Detected Chip ID = 0x%04x\n", chip_id);

	return 0;
}

// Setup PLL 
//
// PLL output frequency = VCO / (P1 + 1)
// VCO = (Fin * 2 * M divider) / ( N divider + 1 )
//
// So : Vpll = ( Fin * 2 * M)
//             ---------------
//              (n+1) * (p1+1)
//
// N : [0,63]
// M : [0,255]
// P1/P3 : {0,1,3,5,7,9,11,13,15}
//
static int mt9d113_setup_pll(struct v4l2_int_device *s, struct mt9d113_pll *pll_cfg)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 ctrl;
	int retry = 0;
	int ret = 0;

	v4l_info(client, "setup PLL to : m=%d - n=%d - p1=%d - p3=%d - wcd=%d\n", 
			pll_cfg->m, pll_cfg->n, pll_cfg->p1, pll_cfg->p3, pll_cfg->wcd);

	// set HYSTERESIS_ENABLED | RESET_COUNTER | PLL_DISABLED | PLL_BYPASSED
	// + some magic reserved bits that we dont care.
	ret |= mt9d113_write_reg(client, MT9D113_R20_PLL_CONTROL, 0x21f9);

	// PLL Configuration (Quandrant sequence, for 24M clk)
	// PLL Dividers :
	// REG = 0x0010, 0x011C	//PLL Dividers = 284
	//
	// set n-divider = 1 (was 3)
	// and m-divider = 0x1c (was 0x66) 
	//
	// PLL P Dividers :
	// REG = 0x0012, 0x1FF7	//PLL P Dividers = 8183
	//
	// set p1 to 7
	// set p3 to 15 (XXX what for ?)
	// set word clock divider to 1 (XXX what for ?)
	//
	// Quadrant VCO = 24000000*2*28/2 = 672Mhz
	// Fpll = VCO/(7+1) = 84Mhz
	//

	// For some reasons, 24M is not possible (we're at 24.675714Mhz) :
	// either decrease m to 27 : Fpll = 83.28Mhz
	// but that's not really ok, since we're not at 84mhz, and I guess that
	// timings  will diverge from the one from Quadrant,
	// or lock to 84mhz by another xclk frequency :
	//
	// clkin = 14.4mhz, set M = 35 / N = 1 / P1 = 5
	// -> VCO = 14.4M * 2 * 35 / (5+1) = 168mhz
	// -> Fclk = VCO / (1+1) = 84mhz

	ctrl = (pll_cfg->n << 8) | (pll_cfg->m);
	ret |= mt9d113_write_reg(client, MT9D113_R16_PLL_DIVIDERS, ctrl);

	ctrl = (pll_cfg->wcd << 12) | (pll_cfg->p3 << 8) | (0xf0) | (pll_cfg->p1);
	ret |= mt9d113_write_reg(client, MT9D113_R18_PLL_P_DIVIDERS, ctrl);

	// enable pll
	ret |= mt9d113_write_reg(client, MT9D113_R20_PLL_CONTROL, 0x21FB);

	// reset PLL counters. Note that pll detection bit should be set
	ret |= mt9d113_write_reg(client, MT9D113_R20_PLL_CONTROL, 0x20FB);

	// poll & wait for pll to lock (PLL_LOCK set)
	do {
		if (retry++ > 100) {
			v4l_err(client, "Unable to make " DRIVER_NAME " sensor lock PLL\n");
			return -ENODEV;
		}

		msleep(10);

		mt9d113_read_reg(client, MT9D113_R20_PLL_CONTROL, &ctrl);

	} while (((ctrl & SYSCTL_PLL_LOCK) == 0) );

	// disable pll bypass
	ret |= mt9d113_write_reg(client, MT9D113_R20_PLL_CONTROL, 0x20FA);

	return ret;
}

static int mt9d113_setup_clocks(struct v4l2_int_device *s, struct mt9d113_clk_config *clk_cfg)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int ret = 0;

	if ((clk_cfg == NULL) || (clk_cfg->pll_config == NULL) || (clk_cfg->ext_clk == 0))
		return -EINVAL;

	v4l_info(client, "targeted System (and Pixelclock) frequency is %d\n",
			(clk_cfg->ext_clk * 2 * clk_cfg->pll_config->m) / ((clk_cfg->pll_config->n+1)*(clk_cfg->pll_config->p1+1)));


	ret |= mt9d113_setup_pll(s, clk_cfg->pll_config);

	if (sensor->pdata->set_xclk)
		ret |= sensor->pdata->set_xclk(s, sensor->clk_config->ext_clk);

	return ret;
}

// reset the whole sensor via soft (equivalent to hard reset, via reset_bar)
static void mt9d113_soft_reset(struct i2c_client *client)
{
	u16 ctrl;
	//printk("%s\n", __FUNCTION__);

	mt9d113_read_reg(client, MT9D113_R26_RESET_CONTROL, &ctrl);
	mt9d113_write_reg(client, MT9D113_R26_RESET_CONTROL, ctrl | SYSCTL_RESET_SOC_I2C | SYSCTL_MIPI_TX_RESET);

	msleep(5);

	mt9d113_read_reg(client, MT9D113_R26_RESET_CONTROL, &ctrl);
	mt9d113_write_reg(client, MT9D113_R26_RESET_CONTROL, (ctrl & ~SYSCTL_RESET_SOC_I2C) & ~SYSCTL_MIPI_TX_RESET);
}


// reset sensor mcu to its default values, using soft reset.
static void mt9d113_mcu_reset(struct i2c_client *client)
{
	u16 ctrl;
	//printk("%s\n", __FUNCTION__);

	mt9d113_read_reg(client, MT9D113_R28_MCU_BOOT_MODE, &ctrl);
	mt9d113_write_reg(client, MT9D113_R28_MCU_BOOT_MODE, ctrl | SYSCTL_MCU_SOFT_RESET);

	msleep(5);

	mt9d113_read_reg(client, MT9D113_R28_MCU_BOOT_MODE, &ctrl);
	mt9d113_write_reg(client, MT9D113_R28_MCU_BOOT_MODE, ctrl & ~SYSCTL_MCU_SOFT_RESET);
}

static int mt9d113_exit_soft_standby(struct i2c_client *client)
{
	u16 ctrl;
	int retry = 0;

	mt9d113_read_reg(client, MT9D113_R24_STANDBY_CONTROL, &ctrl);
	mt9d113_write_reg(client, MT9D113_R24_STANDBY_CONTROL, ctrl & ~SYSCTL_STANDBY_I2C);

	// poll and wait for standby to exit (STANDBY_BIT cleared)
	do {
		if (retry++ > 10) {
			v4l_err(client, "Unable to make " DRIVER_NAME " sensor exit standby \n");
			return -ENODEV;
		}

		msleep(100);

		mt9d113_read_reg(client, MT9D113_R24_STANDBY_CONTROL, &ctrl);

	} while ((ctrl & SYSCTL_STANDBY_DONE) != 0);

	return 0;
}

static void enter_test_mode(struct i2c_client *client, u16 test_mode)
{
	const static struct reg_value start_test[] = {
		{ MT9D113_R2444_MCU_ADDRESS, 0xA103 },  
		{ MT9D113_R2448_VARIABLE_DATA0, 0x06 },
		{ MT9D113_R2444_MCU_ADDRESS, 0xA103 },  
		{ MT9D113_R2448_VARIABLE_DATA0, 0x05 },
		{ 0, 0}
	};

	mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, 0x66, &test_mode, 1, LENGTH_8_BIT);

	mt9d113_write_reg_seq(client, start_test);
}

static int mt9d113_switch_mode(struct i2c_client *client, u16 mode)
{
	int retry = 0;
	u16 ctrl;

	/* The current mode may already be ok */
	mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, 4, &ctrl, 1, LENGTH_8_BIT);
	if (ctrl == mode) {
		v4l_info(client, "Already in mode %d\n", mode);
		/* Sync the sequencer */
		mt9d113_write_reg_seq(client, sequencer_sync);
		return 0;
	}

	switch (mode) {
		case MODE_PREVIEW:
			mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_MODE, &ctrl, 1, LENGTH_8_BIT);
			ctrl |= XDMA_SEQ_MODE_AE_EN | XDMA_SEQ_MODE_HG_EN;
			mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_MODE, &ctrl, 1, LENGTH_8_BIT);
			mt9d113_write_reg_seq(client, preview_mode);
			break;
		case MODE_CAPTURE:
			mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_CAP_MODE, &ctrl, 1, LENGTH_8_BIT);
			ctrl |= XDMA_SEQ_MODE_CAP_AE_EN | XDMA_SEQ_MODE_VIDEO_EN;
			mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_CAP_MODE, &ctrl, 1, LENGTH_8_BIT);
			mt9d113_write_reg_seq(client, capture_mode);
			break;
	}

	do {
		if (retry++ > 10) {
			v4l_err(client, "Unable to make " DRIVER_NAME " sequencer enter mode %d (current mode is %d)\n", mode, ctrl);
			return -EIO;
		}

		msleep(100);
		
		mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, 4, &ctrl, 1, LENGTH_8_BIT);

	} while (ctrl != mode);

	return 0;
}

// configure and make mt9d113 exit standby
static int mt9d113_configure(struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int retry = 0;
	u16 ctrl;

//	printk("%s\n", __FUNCTION__);

	// enable parallel interface (disable mipi iface)
	mt9d113_read_reg(client, MT9D113_R26_RESET_CONTROL, &ctrl);
	mt9d113_write_reg(client, MT9D113_R26_RESET_CONTROL, (ctrl & ~SYSCTL_MIPI_TX_ENABLE) | SYSCTL_PARALLEL_ENABLE);

	// disable MIPI interface according to the developer guide
	mt9d113_read_reg(client, 0x3400, &ctrl);
	mt9d113_write_reg(client, 0x3400, (ctrl & ~0x200));
	
	// disable OE_BAR on gpio for parallel interface (default)
	mt9d113_read_reg(client, MT9D113_R26_RESET_CONTROL, &ctrl);
	mt9d113_write_reg(client, MT9D113_R26_RESET_CONTROL, ctrl & ~SYSCTL_OE_N_ENABLE);

	// enable mclk pad, sound quite usefull to generate a pll 
	// XXX: uncertain ??
	mt9d113_read_reg(client, MT9D113_R22_CLOCK_CONTROL, &ctrl);
	mt9d113_write_reg(client, MT9D113_R22_CLOCK_CONTROL, ctrl | SYSCTL_CLK_CLKIN_EN);

	// enable pll (XCLKA is already set, so no need to call mt9d113_setup_clocks)
	mt9d113_setup_pll(s, sensor->clk_config->pll_config);

	// set powerup stop bit
	mt9d113_read_reg(client, MT9D113_R24_STANDBY_CONTROL, &ctrl);
	mt9d113_write_reg(client, MT9D113_R24_STANDBY_CONTROL, ctrl | SYSCTL_POWERUP_STOP_MCU);

	// exit standby.
	mt9d113_exit_soft_standby(client);

	// now set some sensor bits, from quadrant init.

	// REG = 0x321C, 0x0003	// By Pass TxFIFO = 3 
	mt9d113_read_reg(client, MT9D113_R12828_OUTPUT_CONTROL, &ctrl);
	mt9d113_write_reg(client, MT9D113_R12828_OUTPUT_CONTROL, SOC1_CPIPE_FORMAT);
	// XXX: note test PCLK fct is there --^ (SOC1_TEST_PCLK)
	// see 0x3290 too, and Page 25 of doc.

	// init mode register setup
	// XXX: sync sensor->pix with what we're going to program
	mt9d113_write_reg_seq(client, init_wizard_setup);

	// LOAD=Errata for Rev2: issue02
	mt9d113_write_reg_seq(client, rev2_errata_issue02);

	// Load PGA curve from EEPROM LOAD=EEPROM Lens Correction

	// Low light setting LOAD=Low light setting
	mt9d113_write_reg_seq(client, low_light_settings);

	// AWB and CCM settings LOAD=AWB and CCMs
	mt9d113_write_reg_seq(client, awb_and_ccms);

	// Load the patch LOAD=SOC2030_patch
	mt9d113_write_reg_seq(client, rev3_SOC2030_patch);
	// wait for the patch to complete initialization
	do {
		if (retry++ > 10) {
			v4l_err(client, "Patch not apply \n");
			break;
		}

		msleep(100);
		
		mt9d113_read_xdma(client, 0, 0x24, &ctrl, 1, LENGTH_8_BIT);

	} while (ctrl == 0x0);

	// clear powerup stop bit
	mt9d113_read_reg(client, MT9D113_R24_STANDBY_CONTROL, &ctrl);
	mt9d113_write_reg(client, MT9D113_R24_STANDBY_CONTROL, ctrl & ~SYSCTL_POWERUP_STOP_MCU);

	// poll sequencer, wait for preview state
	retry = 0;
	do {
		if (retry++ > 10) {
			v4l_err(client, "Unable to make" DRIVER_NAME " sequencer enter preview \n");
			break;
		}

		msleep(100);
		
		mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, 4, &ctrl, 1, LENGTH_8_BIT);

	} while (ctrl != 0x03);

	// refresh sequencer
	mt9d113_write_reg_seq(client, sequencer_sync);

	// profit.

	return 0;
}


static int mt9d113_enum_fmt_cap(struct v4l2_int_device *c,
		struct v4l2_fmtdesc *fmt)
{
//	printk("%s\n", __FUNCTION__);

	if (fmt->index >= NUM_CAPTURE_FORMATS)
		return -EINVAL;

	fmt->flags = mt9d113_formats[fmt->index].flags;
	strlcpy(fmt->description, mt9d113_formats[fmt->index].description, sizeof(fmt->description));
	fmt->pixelformat = mt9d113_formats[fmt->index].pixelformat;

	printk("[%d] %s - %X\n", fmt->index, fmt->description, fmt->pixelformat);

	return 0;
}


static int mt9d113_try_fmt_cap(struct v4l2_int_device *s,
		struct v4l2_format *f, int *fmt, int *size)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int ifmt, isize;

//	printk("%s\n", __FUNCTION__);

	// find correct pixelformat
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) 
		if (pix->pixelformat == mt9d113_formats[ifmt].pixelformat)
			break;

	if (ifmt == NUM_CAPTURE_FORMATS)
		ifmt = 0;

	pix->pixelformat = mt9d113_formats[ifmt].pixelformat;

	// find correct frame size
	for (isize = 0; isize <= NUM_FRAME_SIZES; isize++) {
		if ((mt9d113_framesizes[isize].height >= pix->height) &&
			(mt9d113_framesizes[isize].width >= pix->width)) {
			break;
		}
	}

	pix->width = mt9d113_framesizes[isize].width;
	pix->height = mt9d113_framesizes[isize].height;

	pix->bytesperline = pix->width * 2;
	pix->sizeimage = pix->bytesperline * pix->height;

	// seems usefull
	pix->field = V4L2_FIELD_NONE;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->priv = 0;

	if (size != NULL)
		*size = isize;

	if (fmt != NULL)
		*fmt = ifmt;

	return 0;
}

int mt9d113_get_unbanding(__u32 *value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v = 0;
	int ret;

	switch (sensor->ctx) {
		case CONTEXT_A:
			ret = mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_MODE,
				(u16*)&v, 1, LENGTH_8_BIT);
			if (!(v & XDMA_SEQ_MODE_FD_EN)) {
				*value = V4L2_UNBANDING_NONE;
				return ret;
			}
			break;
		case CONTEXT_B:
			ret = mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_CAP_MODE,
				(u16*)&v, 1, LENGTH_8_BIT);
			if (!(v & XDMA_SEQ_MODE_CAP_FD_EN)) {
				*value = V4L2_UNBANDING_NONE;
				return ret;
			}
			break;
		default:
			return -EINVAL;
	}

	ret |= mt9d113_read_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_MODE,
		(u16*)&v, 1, LENGTH_8_BIT);
	if (!(v & XDMA_FD_MODE_MANUAL)) {
		*value = V4L2_UNBANDING_AUTO;
		return ret;
	}

	ret |= mt9d113_read_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_MODE,
		(u16*)&v, 1, LENGTH_8_BIT);
	if (v & XDMA_FD_MODE_50HZ)
		*value = V4L2_UNBANDING_50HZ;
	else
		*value = V4L2_UNBANDING_60HZ;

	return ret;
}

int mt9d113_set_unbanding(u32 value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v = 0;
	int ret;

	/*  Context A first */
	ret = mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_MODE,
		(u16*)&v, 1, LENGTH_8_BIT);
	if (value == V4L2_UNBANDING_NONE)
		v &= ~XDMA_SEQ_MODE_FD_EN;
	else
		v |= XDMA_SEQ_MODE_FD_EN;


	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_MODE,
		(u16*)&v, 1, LENGTH_8_BIT);

	/*  Then strange intermediate state registers */
	switch (value) {
		case V4L2_UNBANDING_NONE:
			v = 0;
			break;
		case V4L2_UNBANDING_AUTO:
			v = 1;
			break;
		case V4L2_UNBANDING_50HZ:
		case V4L2_UNBANDING_60HZ:
		default:
			v = 2;
			break;
	}
	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_PREV_0_FD,
		(u16*)&v, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_PREV_1_FD,
		(u16*)&v, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_PREV_2_FD,
		(u16*)&v, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_PREV_3_FD,
		(u16*)&v, 1, LENGTH_8_BIT);

	/*  Finaly Context B */
	ret |= mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_CAP_MODE,
		(u16*)&v, 1, LENGTH_8_BIT);
	if (value == V4L2_UNBANDING_NONE) {
		v &= ~XDMA_SEQ_MODE_CAP_FD_EN;

		ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_CAP_MODE,
			(u16*)&v, 1, LENGTH_8_BIT);
		return ret;
	}

	v |= XDMA_SEQ_MODE_CAP_FD_EN;

	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_CAP_MODE,
		(u16*)&v, 1, LENGTH_8_BIT);

	ret |= mt9d113_read_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_MODE,
		(u16*)&v, 1, LENGTH_8_BIT);
	if (value == V4L2_UNBANDING_AUTO) {
		v &= ~XDMA_FD_MODE_MANUAL;
		ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_MODE,
			(u16*)&v, 1, LENGTH_8_BIT);
		return ret;
	}

	v |= XDMA_FD_MODE_MANUAL;

	/*
	 * Even if Aptina says that the status bit is read only, its value has to be the
	 * opposite of the unbanding mode we want to set if we want the internal firmware
	 * to switch to the correct mode.
	 */
	switch (value) {
		case V4L2_UNBANDING_50HZ:
			v |= XDMA_FD_MODE_50HZ;
			v &= ~XDMA_FD_MODE_STATUS_50HZ;
			break;
		case V4L2_UNBANDING_60HZ:
			v &= ~XDMA_FD_MODE_50HZ;
			v |= XDMA_FD_MODE_STATUS_50HZ;
			break;
		default:
			break;
	}

	ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_MODE,
		(u16*)&v, 1, LENGTH_8_BIT);

	return ret;
}

int mt9d113_get_colorfx(__u32 *value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v = 0, vsepia = 0;
	int ret;

	switch (sensor->ctx) {
		case CONTEXT_A:
			ret = mt9d113_read_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_SPEC_EFFECTS_A,
				(u16*)&v, 1, LENGTH_16_BIT);
			break;
		case CONTEXT_B:
			ret = mt9d113_read_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_SPEC_EFFECTS_B,
				(u16*)&v, 1, LENGTH_16_BIT);
			break;
		default:
			return -EINVAL;
	}

	switch (v & 0x7) {
		case 0:
			*value = V4L2_COLORFX_NONE;
			break;
		case 1:
			*value = V4L2_COLORFX_BW;
			break;
		case 2:
			*value = V4L2_COLORFX_SEPIA;
			ret |= mt9d113_read_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_COMMON_SEPIA_SETTINGS,
				(u16*)&vsepia, 1, LENGTH_16_BIT);
			switch (vsepia) {
				case 0xB023:
					*value = V4L2_COLORFX_SEPIA;
					break;
				case 0x40C0:
					*value = V4L2_COLORFX_AQUA;
					break;
				case 0x4000:
					*value = V4L2_COLORFX_BLUISH;
					break;
				case 0xC0C0:
					*value = V4L2_COLORFX_GREENISH;
					break;
				case 0x0040:
					*value = V4L2_COLORFX_REDISH;
					break;
			}
			break;
		case 3:
			*value = V4L2_COLORFX_NEGATIVE;
			break;
		case 5:
			*value = V4L2_COLORFX_SOLARIZE;
			break;
	}

	return ret;
}

int mt9d113_set_colorfx(u32 value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 va = 0, vb = 0, vsepia;
	int ret = 0;

	ret = mt9d113_read_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_SPEC_EFFECTS_A,
		(u16*)&va, 1, LENGTH_16_BIT);

	ret |= mt9d113_read_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_SPEC_EFFECTS_B,
		(u16*)&vb, 1, LENGTH_16_BIT);

	va &= ~(0x07);
	vb &= ~(0x07);

	switch (value) {
		case V4L2_COLORFX_NONE:
			va |= 0;
			vb |= 0;
			vsepia = 0;
			break;
		case V4L2_COLORFX_BW:
			va |= 1;
			vb |= 1;
			vsepia = 0;
			break;
		case V4L2_COLORFX_SEPIA:
			va |= 2;
			vb |= 2;
			vsepia = 0xB023;
			break;
		case V4L2_COLORFX_NEGATIVE:
			va |= 3;
			vb |= 3;
			vsepia = 0;
			break;
		case V4L2_COLORFX_SOLARIZE:
			va |= 5;
			vb |= 5;
			vsepia = 0;
			break;
		case V4L2_COLORFX_AQUA:
			va |= 2;
			vb |= 2;
			vsepia = 0x40C0;
			break;
		case V4L2_COLORFX_BLUISH:
			va |= 2;
			vb |= 2;
			vsepia = 0x4000;
			break;
		case V4L2_COLORFX_GREENISH:
			va |= 2;
			vb |= 2;
			vsepia = 0xC0C0;
			break;
		case V4L2_COLORFX_REDISH:
			va |= 2;
			vb |= 2;
			vsepia = 0x0040;
			break;
	}

	ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_COMMON_SEPIA_SETTINGS,
		(u16*)&vsepia, 1, LENGTH_16_BIT);

	ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_SPEC_EFFECTS_A,
		(u16*)&va, 1, LENGTH_16_BIT);

	ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_SPEC_EFFECTS_B,
		(u16*)&vb, 1, LENGTH_16_BIT);

	/* Refresh */
	mt9d113_write_reg_seq(client, sequencer_sync);

	return ret;
}

int mt9d113_set_nightmode(u32 value, struct v4l2_int_device *s, int refresh)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v_min_index = 0, v_max_index = 0;
	u16 v_min_virtgain = 0, v_max_virtgain = 0;
	int ret = 0;

	if (value) {
		/* Integration time can be increased */
		v_min_index = 0;
		v_max_index = 14;
		v_min_virtgain = 16;
		v_max_virtgain = 160;
	} else {
		/* Seems to keep the frame rate high enough (about 25 fps) */
		v_min_index = 0;
		v_max_index = 4;
		v_min_virtgain = 16;
		v_max_virtgain = 160;
	}

	ret = mt9d113_write_xdma(client, XDMA_AUTO_EXPOSURE_ID, XDMA_AE_MIN_INDEX,
		(u16*)&v_min_index, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_AUTO_EXPOSURE_ID, XDMA_AE_MAX_INDEX,
		(u16*)&v_max_index, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_AUTO_EXPOSURE_ID, XDMA_AE_MIN_VIRT_GAIN,
		(u16*)&v_min_virtgain, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_AUTO_EXPOSURE_ID, XDMA_AE_MAX_VIRT_GAIN,
		(u16*)&v_max_virtgain, 1, LENGTH_8_BIT);

	if (refresh) {
		/* Refresh */
		mt9d113_write_reg_seq(client, sequencer_sync);
	}

	return ret;
}

int mt9d113_get_brightness(__u32 *value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v_brightness;
	int ret = 0;

	ret = mt9d113_read_xdma(client, XDMA_AUTO_EXPOSURE_ID, XDMA_AE_BASE_TARGET,
		(u16*)&v_brightness, 1, LENGTH_8_BIT);

	*value = v_brightness;

	return ret;
}

int mt9d113_set_brightness(u32 value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v_brightness;
	int ret = 0;

	v_brightness = value;

	ret = mt9d113_write_xdma(client, XDMA_AUTO_EXPOSURE_ID, XDMA_AE_BASE_TARGET,
		(u16*)&v_brightness, 1, LENGTH_8_BIT);

	return ret;
}

int mt9d113_set_gamma(u32 value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v_gamma, reg;
	int ret = 0;

	reg = 0x3C + ((value >> 16) & 0xFF) * 19 + ((value >> 8) & 0xFF);
	v_gamma = value & 0xFF;

	ret = mt9d113_write_xdma(client, XDMA_HISTOGRAM_VARIABLE_ID, reg,
		(u16*)&v_gamma, 1, LENGTH_8_BIT);

	return ret;
}

int mt9d113_set_ae(u32 value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v, reg;
	int ret = 0, length;

	v = value & 0xFFFF;

	switch ((value >> 16) & 0xFF) {
		case 0:
			reg = XDMA_AE_MIN_VIRT_GAIN;
			length = LENGTH_8_BIT;
			break;
		case 1:
			reg = XDMA_AE_MAX_VIRT_GAIN;
			length = LENGTH_8_BIT;
			break;
		case 2:
			reg = XDMA_AE_MIN_TARGET;
			length = LENGTH_8_BIT;
			break;
		case 3:
			reg = XDMA_AE_MAX_TARGET;
			length = LENGTH_8_BIT;
			break;
		case 4:
			reg = XDMA_AE_MIN_INDEX;
			length = LENGTH_8_BIT;
			break;
		case 5:
			reg = XDMA_AE_MAX_INDEX;
			length = LENGTH_8_BIT;
			break;
		case 6:
			reg = XDMA_AE_MAX_D_GAIN_AE1;
			length = LENGTH_16_BIT;
			break;
		case 7:
			reg = XDMA_AE_GATE;
			length = LENGTH_8_BIT;
			break;
		default:
			return -1;
	}

	ret = mt9d113_write_xdma(client, XDMA_AUTO_EXPOSURE_ID, reg,
		(u16*)&v, 1, length);

	/* Refresh */
	mt9d113_write_reg_seq(client, sequencer_sync);

	return ret;
}

int mt9d113_set_manual_wb(u32 value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v1 = 0, v2 = 0, v3 = 0;
	int ret;

	/*
         * A gain of x1 for the Red channel, of x1 for the Green channel
         * and of x1 for the Blue channel have been added to compensate
         * the lens tint. According to Aptina Datasheet: Gain = (Register Value)/0x80
         */
	switch (value) {
		case 2600:
			// TUNGSTEN / INCANDESCENT 40W
			v1 = 0x80;
			v2 = 0xA5;
			v3 = 0xE4;
			break;
		case 2850:
			// TUNGSTEN / INCANDESCENT 100W
			v1 = 0x80;
			v2 = 0x98;
			v3 = 0xC0;
			break;
		case 4200:
			// FLUORESCENT
			v1 = 0x80;
			v2 = 0x85;
			v3 = 0x86;
			break;
		case 6000:
			// DAYLIGHT
			v1 = 0x80;
			v2 = 0x80;
			v3 = 0x80;
			break;
		case 7000:
			// CLOUDY_DAYLIGHT
			v1 = 0xA2;
			v2 = 0x90;
			v3 = 0x80;
			break;
		case 9500:
			// SHADE
			v1 = 0xC2;
			v2 = 0xA6;
			v3 = 0x80;
			break;
		default:
			return -1;
	}

	ret = mt9d113_write_xdma(client, XDMA_AUTO_WHITE_BALANCE_ID, XDMA_WB_GAIN_RED,
		(u16*)&v1, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_AUTO_WHITE_BALANCE_ID, XDMA_WB_GAIN_GREEN,
		(u16*)&v2, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_AUTO_WHITE_BALANCE_ID, XDMA_WB_GAIN_BLUE,
		(u16*)&v3, 1, LENGTH_8_BIT);

	return ret;
}

int mt9d113_get_auto_wb(__u32 *value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v;
	int ret;

	switch (sensor->ctx) {
		case CONTEXT_A:
			ret = mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_MODE,
				(u16*)&v, 1, LENGTH_8_BIT);
			*value = (v & XDMA_SEQ_MODE_AWB_EN) == XDMA_SEQ_MODE_AWB_EN;
			break;
		case CONTEXT_B:
			ret = mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_CAP_MODE,
				(u16*)&v, 1, LENGTH_8_BIT);
			*value = (v & XDMA_SEQ_MODE_CAP_AWB_EN) == XDMA_SEQ_MODE_CAP_AWB_EN;
			break;
		default:
			return -EINVAL;
	}

	return ret;
}

int mt9d113_set_auto_wb(u32 value, struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	u16 v, va, vb;
	int ret;

	ret = mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_MODE,
		(u16*)&va, 1, LENGTH_8_BIT);
	ret |= mt9d113_read_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_CAP_MODE,
		(u16*)&vb, 1, LENGTH_8_BIT);

	if (value) {
		va |= XDMA_SEQ_MODE_AWB_EN;
		v = 1;
		vb |= XDMA_SEQ_MODE_CAP_AWB_EN;
	} else {
		va &= ~XDMA_SEQ_MODE_AWB_EN;
		v = 0;
		vb &= ~XDMA_SEQ_MODE_CAP_AWB_EN;
	}

	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_MODE,
		(u16*)&va, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_PREV_0_AWB,
		(u16*)&v, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_PREV_1_AWB,
		(u16*)&v, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_PREV_2_AWB,
		(u16*)&v, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_PREV_3_AWB,
		(u16*)&v, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_SEQUENCER_ID, XDMA_SEQ_CAP_MODE,
		(u16*)&vb, 1, LENGTH_8_BIT);

	return ret;
}

static int mt9d113_s_fmt_cap(struct v4l2_int_device *s,
		struct v4l2_format *f)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct v4l2_pix_format *pix = &sensor->pix;
	struct i2c_client *client = sensor->i2c_client;
	struct mt9d113_clk_config *clk_save = sensor->clk_config;
	int isize, ifmt;
	int ret;

//	printk("%s\n", __FUNCTION__);

	ret = mt9d113_try_fmt_cap(s, f, &ifmt, &isize);
	if (ret)
		return ret;

	// apply sizes change to sensor
	if (mt9d113_framesizes_settings[isize].clk_cfg)
		sensor->clk_config = mt9d113_framesizes_settings[isize].clk_cfg;
	else
		/* Default clock config */
		sensor->clk_config = &default_clk_config;

	ret = mt9d113_setup_clocks(s, sensor->clk_config);

	if (ret < 0) {
		v4l_err(client, "Unable to apply PLL settings\n");
		sensor->clk_config = clk_save;
		return ret;
	}

	if (mt9d113_framesizes_settings[isize].regs)
		ret = mt9d113_write_reg_seq(client, mt9d113_framesizes_settings[isize].regs);

	/* Set Auto Exposure stuff */
	ret |= mt9d113_write_xdma(client, XDMA_AUTO_EXPOSURE_ID, XDMA_AE_R9_STEP,
		(u16*)&mt9d113_framesizes_settings[isize].R9_step, 1, LENGTH_16_BIT);
//	ret |= mt9d113_set_nightmode(sensor->current_nightmode, s, 0);

	/* Set Flicker Detection stuff (common) */
	ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_SEARCH_F1_50,
		(u16*)&mt9d113_framesizes_settings[isize].fd_search_f1_50, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_SEARCH_F2_50,
		(u16*)&mt9d113_framesizes_settings[isize].fd_search_f2_50, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_SEARCH_F1_60,
		(u16*)&mt9d113_framesizes_settings[isize].fd_search_f1_60, 1, LENGTH_8_BIT);
	ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_SEARCH_F2_60,
		(u16*)&mt9d113_framesizes_settings[isize].fd_search_f2_60, 1, LENGTH_8_BIT);

	switch (mt9d113_framesizes_settings[isize].ctx) {
		case CONTEXT_A:
			/* Set final output size */
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_OUTPUT_WIDTH_A,
				(u16*)&mt9d113_framesizes[isize].width, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_OUTPUT_HEIGHT_A,
				(u16*)&mt9d113_framesizes[isize].height, 1, LENGTH_16_BIT);

			/* Set cropped size */
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_CROP_X0_A,
				(u16*)&mt9d113_framesizes_settings[isize].crop_X0, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_CROP_X1_A,
				(u16*)&mt9d113_framesizes_settings[isize].crop_X1, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_CROP_Y0_A,
				(u16*)&mt9d113_framesizes_settings[isize].crop_Y0, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_CROP_Y1_A,
				(u16*)&mt9d113_framesizes_settings[isize].crop_Y1, 1, LENGTH_16_BIT);

			/* Set active pixels (low level crop) */
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_ROW_START_A,
				(u16*)&mt9d113_framesizes_settings[isize].sensor_row_start, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_COL_START_A,
				(u16*)&mt9d113_framesizes_settings[isize].sensor_col_start, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_ROW_END_A,
				(u16*)&mt9d113_framesizes_settings[isize].sensor_row_end, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_COL_END_A,
				(u16*)&mt9d113_framesizes_settings[isize].sensor_col_end, 1, LENGTH_16_BIT);

			/* Set signal "geometry" */
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_FRAME_LENGTH_A,
				(u16*)&mt9d113_framesizes_settings[isize].frame_length, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_LINE_LENGTH_A,
				(u16*)&mt9d113_framesizes_settings[isize].line_length, 1, LENGTH_16_BIT);

			/* Set Flicker Detection stuff */
			ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_R9_STEP_F60_A,
				(u16*)&mt9d113_framesizes_settings[isize].R9_step_60, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_R9_STEP_F50_A,
				(u16*)&mt9d113_framesizes_settings[isize].R9_step_50, 1, LENGTH_16_BIT);

			/* Enter Preview mode */
			ret |= mt9d113_switch_mode(client, MODE_PREVIEW);
			break;
		case CONTEXT_B:
			/* Set final output size */
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_OUTPUT_WIDTH_B,
				(u16*)&mt9d113_framesizes[isize].width, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_OUTPUT_HEIGHT_B,
				(u16*)&mt9d113_framesizes[isize].height, 1, LENGTH_16_BIT);

			/* Set cropped size */
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_CROP_X0_B,
				(u16*)&mt9d113_framesizes_settings[isize].crop_X0, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_CROP_X1_B,
				(u16*)&mt9d113_framesizes_settings[isize].crop_X1, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_CROP_Y0_B,
				(u16*)&mt9d113_framesizes_settings[isize].crop_Y0, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_CROP_Y1_B,
				(u16*)&mt9d113_framesizes_settings[isize].crop_Y1, 1, LENGTH_16_BIT);

			/* Set active pixels (low level crop) */
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_ROW_START_B,
				(u16*)&mt9d113_framesizes_settings[isize].sensor_row_start, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_COL_START_B,
				(u16*)&mt9d113_framesizes_settings[isize].sensor_col_start, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_ROW_END_B,
				(u16*)&mt9d113_framesizes_settings[isize].sensor_row_end, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_COL_END_B,
				(u16*)&mt9d113_framesizes_settings[isize].sensor_col_end, 1, LENGTH_16_BIT);

			/* Set signal "geometry" */
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_FRAME_LENGTH_B,
				(u16*)&mt9d113_framesizes_settings[isize].frame_length, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_MODE_VARIABLE_ID, XDMA_LINE_LENGTH_B,
				(u16*)&mt9d113_framesizes_settings[isize].line_length, 1, LENGTH_16_BIT);

			/* Set Flicker Detection stuff */
			ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_R9_STEP_F60_B,
				(u16*)&mt9d113_framesizes_settings[isize].R9_step_60, 1, LENGTH_16_BIT);
			ret |= mt9d113_write_xdma(client, XDMA_ANTI_FLICKER_ID, XDMA_FD_R9_STEP_F50_B,
				(u16*)&mt9d113_framesizes_settings[isize].R9_step_50, 1, LENGTH_16_BIT);

			/* Enter Capture mode */
			ret |= mt9d113_switch_mode(client, MODE_CAPTURE);
			break;
	}

	if (ret < 0) {
		v4l_err(client, "Unable to apply framesize settings\n");
		return ret;
	}

	/*
	 * Unbanding mode needs to be set again when the framesize is changed,
	 * and we need to wait a little before setting it.
	 */
	msleep(100);
	ret = mt9d113_set_unbanding(sensor->current_unbanding, s);
	if (ret < 0) {
		v4l_warn(client, "Warning: Unable to apply unbanding mode\n");
	}

	// store sensor pix fmt
	pix->pixelformat = mt9d113_formats[ifmt].pixelformat;

	pix->width = mt9d113_framesizes[isize].width;
	pix->height = mt9d113_framesizes[isize].height;

	pix->bytesperline = pix->width * 2;
	pix->sizeimage = pix->bytesperline * pix->height;

	pix->field = V4L2_FIELD_NONE;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->priv = 0;

	sensor->ctx = mt9d113_framesizes_settings[isize].ctx;

	printk("w - %d, h - %d, pixfmt - %X\n",
			pix->width, pix->height, pix->pixelformat);

	return 0;
}

static int ioctl_queryctrl(struct v4l2_int_device *s,
				struct v4l2_queryctrl *qc)
{
	int i;

//	printk("%s\n", __FUNCTION__);

	i = find_vctrl_idx(qc->id);
	if (i < 0)
		return -EINVAL;

	*qc = mt9d113_controls[i];

	return 0;
}

static int ioctl_g_ctrl(struct v4l2_int_device *s,
			     struct v4l2_control *vc)
{
	struct mt9d113_sensor *sensor = s->priv;
	int i, ret = 0;

//	printk("%s\n", __FUNCTION__);

	i = find_vctrl_idx(vc->id);
	if (i < 0)
		return -EINVAL;

	switch (vc->id) {
		case  V4L2_CID_UNBANDING:
			//ret = mt9d113_get_unbanding(&vc->value, s);
			vc->value = sensor->current_unbanding;
			break;
		case  V4L2_CID_COLORFX:
			ret = mt9d113_get_colorfx(&vc->value, s);
			break;
		case  V4L2_CID_NIGHT_MODE:
			vc->value = sensor->current_nightmode;
			break;
		case  V4L2_CID_BRIGHTNESS:
			ret = mt9d113_get_brightness(&vc->value, s);
			break;
		case  V4L2_CID_WHITE_BALANCE_TEMPERATURE:
			vc->value = sensor->current_manual_wb;
			break;
		case  V4L2_CID_AUTO_WHITE_BALANCE:
			ret = mt9d113_get_auto_wb(&vc->value, s);
			break;
	}

	return ret;
}

static int ioctl_s_ctrl(struct v4l2_int_device *s,
			     struct v4l2_control *vc)
{
	struct mt9d113_sensor *sensor = s->priv;
	int i, ret = 0;

//	printk("%s\n", __FUNCTION__);

	i = find_vctrl_idx(vc->id);
	if (i < 0)
		return -EINVAL;

	switch (vc->id) {
		case  V4L2_CID_UNBANDING:
			ret = mt9d113_set_unbanding(vc->value, s);
			if (ret == 0)
				sensor->current_unbanding = vc->value;
			break;
		case  V4L2_CID_COLORFX:
			ret = mt9d113_set_colorfx(vc->value, s);
			break;
		case  V4L2_CID_NIGHT_MODE:
			ret = mt9d113_set_nightmode(vc->value, s, 1);
			if (ret == 0)
				sensor->current_nightmode = vc->value;
			break;
		case  V4L2_CID_BRIGHTNESS:
			ret = mt9d113_set_brightness(vc->value, s);
			break;
		case  V4L2_CID_GAMMA:
			ret = mt9d113_set_gamma(vc->value, s);
			break;
		case  V4L2_CID_EXPOSURE:
			ret = mt9d113_set_ae(vc->value, s);
			break;
		case  V4L2_CID_WHITE_BALANCE_TEMPERATURE:
			ret = mt9d113_set_manual_wb(vc->value, s);
			if (ret == 0)
				sensor->current_manual_wb = vc->value;
			break;
		case  V4L2_CID_AUTO_WHITE_BALANCE:
			ret = mt9d113_set_auto_wb(vc->value, s);
			break;
	}

	return ret;
}

static int ioctl_enum_fmt_cap(struct v4l2_int_device *s,
				   struct v4l2_fmtdesc *fmt)
{
//	printk("%s\n", __FUNCTION__);

	return mt9d113_enum_fmt_cap(s, fmt);
}

static int ioctl_try_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
//	printk("%s\n", __FUNCTION__);

	return mt9d113_try_fmt_cap(s, f, NULL, NULL);
}

static int ioctl_s_fmt_cap(struct v4l2_int_device *s,
				struct v4l2_format *f)
{
//	printk("%s\n", __FUNCTION__);

	return mt9d113_s_fmt_cap(s, f);
}

static int ioctl_g_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct mt9d113_sensor *sensor = s->priv;

//	printk("%s\n", __FUNCTION__);

	f->fmt.pix = sensor->pix;

	printk("bytesprline %d, w - %d, h - %d, pixfmt - %X\n",
			sensor->pix.bytesperline,
			sensor->pix.width, sensor->pix.height,
			sensor->pix.pixelformat);

	return 0;
}

static int ioctl_priv_g_pixclk(struct v4l2_int_device *s, u32 *pixclk)
{
//	printk("%s\n", __FUNCTION__);
	return 0;
}

static int ioctl_priv_g_activesize(struct v4l2_int_device *s,
			      struct v4l2_pix_format *pix)
{
//	printk("%s\n", __FUNCTION__);
	return 0;
}

static int ioctl_priv_g_fullsize(struct v4l2_int_device *s,
			    struct v4l2_pix_format *pix)
{
//	printk("%s\n", __FUNCTION__);
	return 0;
}

static int ioctl_g_parm(struct v4l2_int_device *s,
			     struct v4l2_streamparm *a)
{
//	printk("%s\n", __FUNCTION__);
	return 0;
}

static int ioctl_s_parm(struct v4l2_int_device *s,
			     struct v4l2_streamparm *a)
{
//	printk("%s\n", __FUNCTION__);
	return 0;
}

static int ioctl_g_priv(struct v4l2_int_device *s, void *p)
{
	struct mt9d113_sensor *sensor = s->priv;

	if (sensor->pdata->priv_data_set == NULL)
		return -EINVAL;

	return sensor->pdata->priv_data_set(s, p);
}

static int ioctl_s_power(struct v4l2_int_device *s, enum v4l2_power on)
{
	static enum v4l2_power current_power_state;
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int ret = 0;

//	printk("%s (from st %d to st %d)\n",
//			__FUNCTION__, current_power_state, on); 

	switch (on) {
		case V4L2_POWER_OFF:
			// disable clock.
			if (sensor->pdata->set_xclk)
				ret = sensor->pdata->set_xclk(s, 0);

			// disable power.
			if (sensor->pdata->power_set)
				ret |= sensor->pdata->power_set(s, on);

			break;

		case V4L2_POWER_STANDBY:
		case V4L2_POWER_ON:
			if (current_power_state == V4L2_POWER_OFF) {
				if (sensor->pdata->power_set)
					ret = sensor->pdata->power_set(s, on);

				// enable clock
				if (sensor->pdata->set_xclk)
					ret |= sensor->pdata->set_xclk(s, sensor->clk_config->ext_clk);

				// wait t3 (clk stabilisation : min 1tclk))
				udelay(500);

				// pulse reset low for t4 (min 10xtclk) 
				if (sensor->pdata->reset_pulse)
					ret |= sensor->pdata->reset_pulse(s);

				// wait at least 6000tclk for device to go standby
				msleep(5);

				ret |= mt9d113_detect(client);

				if (ret < 0) {
					v4l_err(client, "Unable to detect sensor\n");
					if (sensor->pdata->set_xclk)
						sensor->pdata->set_xclk(s, 0);
					return -ENODEV;
				}
			
				ret = mt9d113_configure(s);
				if (ret < 0) {
					v4l_err(client, "Unable to configure " DRIVER_NAME " sensor\n");
					return ret;
				}
			}

			break;
	}
	current_power_state = on;

	return ret;
}

static int ioctl_init(struct v4l2_int_device *s)
{
//	printk("%s\n", __FUNCTION__);

	return 0;
}

static int ioctl_dev_exit(struct v4l2_int_device *s)
{
//	printk("%s\n", __FUNCTION__);

	return 0;
}

static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct mt9d113_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	int err;

	err = mt9d113_detect(client);
	if (err < 0) {
		v4l_err(client, "Unable to detect " DRIVER_NAME " sensor\n");
		// Master does not care about powering down slaves if no
		// detection : Shut down power manually, and probably unsync
		// with the power state the master kept, and then maybe have
		// regulator unbalanced supplies oops, but anyway, not 
		// detecting device is BAD, and is probably caused
		// by an HW issue, so avoid fire on board.
		ioctl_s_power(s, V4L2_POWER_OFF);
	}

	return err;
}

static int ioctl_enum_framesizes(struct v4l2_int_device *s,
					struct v4l2_frmsizeenum *frms)
{
	int ifmt;
//	printk("%s\n", __FUNCTION__);

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frms->pixel_format == mt9d113_formats[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frms->index >= NUM_FRAME_SIZES)
		return -EINVAL;

	frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frms->discrete.width = mt9d113_framesizes[frms->index].width;
	frms->discrete.height = mt9d113_framesizes[frms->index].height;

	printk("[%d] w:%d - h:%d\n", frms->index, frms->discrete.width , frms->discrete.height );

	return 0;
}

static int ioctl_enum_frameintervals(struct v4l2_int_device *s,
					struct v4l2_frmivalenum *frmi)
{
	int ifmt;
//	printk("%s\n", __FUNCTION__);

	/* Check that the requested format is one we support */
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frmi->pixel_format == mt9d113_formats[ifmt].pixelformat)
			break;
	}

	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frmi->index >= NUM_FRAME_INTERVALS)
		return -EINVAL;

	frmi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	frmi->discrete.numerator = mt9d113_frameintervals[frmi->index].numerator;
	frmi->discrete.denominator = mt9d113_frameintervals[frmi->index].denominator;

	printk("[%d] %d/%d\n", frmi->index, frmi->discrete.numerator, frmi->discrete.denominator);

	return 0;
}

static struct v4l2_int_ioctl_desc mt9d113_ioctl_desc[] = {
	{ .num = vidioc_int_enum_framesizes_num, .func = (v4l2_int_ioctl_func *)ioctl_enum_framesizes},
	{ .num = vidioc_int_enum_frameintervals_num, .func = (v4l2_int_ioctl_func *)ioctl_enum_frameintervals},
	{ .num = vidioc_int_dev_init_num, .func = (v4l2_int_ioctl_func *)ioctl_dev_init},
	{ .num = vidioc_int_dev_exit_num, .func = (v4l2_int_ioctl_func *)ioctl_dev_exit},
	{ .num = vidioc_int_s_power_num, .func = (v4l2_int_ioctl_func *)ioctl_s_power },
	{ .num = vidioc_int_g_priv_num, .func = (v4l2_int_ioctl_func *)ioctl_g_priv },
	{ .num = vidioc_int_init_num, .func = (v4l2_int_ioctl_func *)ioctl_init },
	{ .num = vidioc_int_enum_fmt_cap_num, .func = (v4l2_int_ioctl_func *)ioctl_enum_fmt_cap },
	{ .num = vidioc_int_try_fmt_cap_num, .func = (v4l2_int_ioctl_func *)ioctl_try_fmt_cap },
	{ .num = vidioc_int_g_fmt_cap_num, .func = (v4l2_int_ioctl_func *)ioctl_g_fmt_cap },
	{ .num = vidioc_int_s_fmt_cap_num, .func = (v4l2_int_ioctl_func *)ioctl_s_fmt_cap },
	{ .num = vidioc_int_g_parm_num, .func = (v4l2_int_ioctl_func *)ioctl_g_parm },
	{ .num = vidioc_int_s_parm_num, .func = (v4l2_int_ioctl_func *)ioctl_s_parm },
	{ .num = vidioc_int_queryctrl_num, .func = (v4l2_int_ioctl_func *)ioctl_queryctrl },
	{ .num = vidioc_int_g_ctrl_num, .func = (v4l2_int_ioctl_func *)ioctl_g_ctrl },
	{ .num = vidioc_int_s_ctrl_num, .func = (v4l2_int_ioctl_func *)ioctl_s_ctrl },
	{ .num = vidioc_int_priv_g_pixclk_num, .func = (v4l2_int_ioctl_func *)ioctl_priv_g_pixclk },
	{ .num = vidioc_int_priv_g_activesize_num, .func = (v4l2_int_ioctl_func *)ioctl_priv_g_activesize },
	{ .num = vidioc_int_priv_g_fullsize_num, .func = (v4l2_int_ioctl_func *)ioctl_priv_g_fullsize },
};

static struct v4l2_int_slave mt9d113_slave = {
	.ioctls = mt9d113_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(mt9d113_ioctl_desc),
};

static struct v4l2_int_device mt9d113_int_device = {
	.module = THIS_MODULE,
	.name = DRIVER_NAME,
	.priv = &mt9d113_dev,
	.type = v4l2_int_type_slave,
	.u = {
		.slave = &mt9d113_slave,
	},
};

static int __devinit mt9d113_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct mt9d113_sensor *sensor = &mt9d113_dev;
	int err;

//	printk("%s\n", __FUNCTION__);

	if (i2c_get_clientdata(client))
		return -EBUSY;

	sensor->pdata = client->dev.platform_data;
	if (!sensor->pdata) {
		v4l_err(client, "no platform data ?\n");
		return -ENODEV;
	}

	sensor->v4l2_int_device = &mt9d113_int_device;
	sensor->i2c_client = client;
	sensor->current_unbanding = V4L2_UNBANDING_AUTO;
	sensor->current_nightmode = 1;
	sensor->current_manual_wb = 6000;

	i2c_set_clientdata(client, sensor);

	err = v4l2_int_device_register(sensor->v4l2_int_device);
	if (err) {
		i2c_set_clientdata(client, NULL);
		v4l_err(client, "Unable to register to v4l2. Err[%d]\n", err);
	}


	return 0;
}

static int __exit
mt9d113_remove(struct i2c_client *client)
{
	struct mt9d113_sensor *sensor = i2c_get_clientdata(client);

//	printk("%s\n", __FUNCTION__);

	if (!client->adapter)
		return -ENODEV;

	v4l2_int_device_unregister(sensor->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id mt9d113_id[] = {
	{ DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, mt9d113_id);

static struct i2c_driver mt9d113sensor_i2c_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = mt9d113_probe,
	.remove = __exit_p(mt9d113_remove),
	.id_table = mt9d113_id,
};

static int __init mt9d113sensor_init(void)
{
	int err;

//	printk("%s\n", __FUNCTION__);

	err = i2c_add_driver(&mt9d113sensor_i2c_driver);
	if (err) {
		printk(KERN_ERR "Failed to register" DRIVER_NAME ".\n");
		return err;
	}
	return 0;
}
late_initcall(mt9d113sensor_init);

static void __exit mt9d113sensor_cleanup(void)
{
//	printk("%s\n", __FUNCTION__);
	i2c_del_driver(&mt9d113sensor_i2c_driver);
}
module_exit(mt9d113sensor_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("mt9d113 camera driver");
