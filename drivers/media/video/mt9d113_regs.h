/*
 *    mt9d113_regs.h : 02/03/2010
 *    g.revaillot, revaillot@archos.com
 */

#ifndef _MT9D113_REGS_H
#define _MT9D113_REGS_H

#define DRIVER_NAME 					"mt9d113"

#define I2C_RETRY_COUNT					10

// SYSCTL Registers :

// Model ID
#define MT9D113_R0_CHIP_ID				0x0000
#define		SYSCTL_CHIP_ID				0x2580

// PLL Dividers	
#define MT9D113_R16_PLL_DIVIDERS			0x0010

// PLL P Dividers
#define MT9D113_R18_PLL_P_DIVIDERS			0x0012

// PLL Control
#define MT9D113_R20_PLL_CONTROL				0x0014
#define 	SYSCTL_PLL_LOCK				(1<<15)

// Clock Control
#define MT9D113_R22_CLOCK_CONTROL			0x0016
#define 	SYSCTL_CLK_CLKIN_EN			(1<<9)

// Standby control and status.
#define MT9D113_R24_STANDBY_CONTROL			0x0018
#define		SYSCTL_STANDBY_DONE			(1<<14)
#define		SYSCTL_ENABLE_XIRQ			(1<<3)
#define		SYSCTL_POWERUP_STOP_MCU 		(1<<2)
#define		SYSCTL_STANDBY_I2C			(1<<0)

// Reset & misc control
#define MT9D113_R26_RESET_CONTROL 			0x001A
#define		SYSCTL_PARALLEL_ENABLE			(1<<9)
#define		SYSCTL_OE_N_ENABLE			(1<<8)
#define		SYSCTL_MIPI_TX_ENABLE			(1<<3)
#define		SYSCTL_MIPI_TX_RESET			(1<<1)
#define		SYSCTL_RESET_SOC_I2C			(1<<0)

// MCU Boot mode
#define MT9D113_R28_MCU_BOOT_MODE			0x001C
#define		SYSCTL_MCU_SOFT_RESET			(1<<0)

// Slew rate control
#define MT9D113_R30_PAD_SLEW				0x001E
#define		SYSCTL_SLEW_RATE_MASK			(0x7)
#define		SYSCTL_PCLK_SLEW_RATE_SHIFT		(8)
#define		SYSCTL_GPIO_SLEW_RATE_SHIFT		(4)
#define		SYSCTL_DATA_SLEW_RATE_SHIFT		(0)


// SOC1 Registers :

#define MT9D113_R12828_OUTPUT_CONTROL			0x321C
#define 	SOC1_CPIPE_FORMAT			0x03
#define 	SOC1_TEST_PCLK				0x07

// XDMA Registers :

// About XDMA MCU address :
// with logical access (14:13 = 01)
		// 12:8 Logical ID
		// 7:0  Offset
// with physical access :
		// 12:0 Address

// XDMA address register
#define MT9D113_R2444_MCU_ADDRESS			0x098C
#define 	XDMA_8BIT_MODE				(1<<15)
#define		XDMA_LOGICAL_MODE			(1<<13)

// XDMA data exchange table registers
#define MT9D113_R2448_VARIABLE_DATA0			0x0990
#define MT9D113_R2450_VARIABLE_DATA1			0x0992
#define MT9D113_R2452_VARIABLE_DATA2			0x0994
#define MT9D113_R2454_VARIABLE_DATA3			0x0996
#define MT9D113_R2456_VARIABLE_DATA4			0x0998
#define MT9D113_R2458_VARIABLE_DATA5			0x099A
#define MT9D113_R2460_VARIABLE_DATA6			0x099C
#define MT9D113_R2462_VARIABLE_DATA7			0x099E


// MCU Registers (XDMA)
#define XDMA_SEQUENCER_ID 				0x01
#define 	XDMA_SEQ_MODE				0x02
#define 		XDMA_SEQ_MODE_AE_EN		(0 << 1)
#define 		XDMA_SEQ_MODE_FD_EN		(1 << 1)
#define 		XDMA_SEQ_MODE_AWB_EN		(2 << 1)
#define 		XDMA_SEQ_MODE_HG_EN		(3 << 1)
#define 	XDMA_SEQ_AE_FAST_BUFF			0x09
#define 	XDMA_SEQ_CAP_MODE			0x15
#define 		XDMA_SEQ_MODE_VIDEO_EN		(1 << 1)
#define 		XDMA_SEQ_MODE_CAP_AE_EN		(1 << 4)
#define 		XDMA_SEQ_MODE_CAP_AWB_EN	(1 << 5)
#define 		XDMA_SEQ_MODE_CAP_FD_EN		(1 << 6)
#define 	XDMA_SEQ_PREV_0_AE			0x17
#define 	XDMA_SEQ_PREV_0_FD			0x18
#define 	XDMA_SEQ_PREV_0_AWB			0x19
#define 	XDMA_SEQ_PREV_0_HG			0x1A
#define 	XDMA_SEQ_PREV_1_AE			0x1D
#define 	XDMA_SEQ_PREV_1_FD			0x1E
#define 	XDMA_SEQ_PREV_1_AWB			0x1F
#define 	XDMA_SEQ_PREV_1_HG			0x20
#define 	XDMA_SEQ_PREV_2_AE			0x23
#define 	XDMA_SEQ_PREV_2_FD			0x24
#define 	XDMA_SEQ_PREV_2_AWB			0x25
#define 	XDMA_SEQ_PREV_2_HG			0x26
#define 	XDMA_SEQ_PREV_3_AE			0x29
#define 	XDMA_SEQ_PREV_3_FD			0x2A
#define 	XDMA_SEQ_PREV_3_AWB			0x2B
#define 	XDMA_SEQ_PREV_3_HG			0x2C

#define XDMA_AUTO_EXPOSURE_ID				0x02
#define 	XDMA_AE_GATE				0x07
#define 	XDMA_AE_MIN_INDEX			0x0B
#define 	XDMA_AE_MAX_INDEX			0x0C
#define 	XDMA_AE_MIN_VIRT_GAIN			0x0D
#define 	XDMA_AE_MAX_VIRT_GAIN			0x0E
#define 	XDMA_AE_MAX_D_GAIN_AE1			0x12
#define 	XDMA_AE_R9_STEP				0x2D
#define 	XDMA_AE_MIN_TARGET			0x4A
#define 	XDMA_AE_MAX_TARGET			0x4B
#define 	XDMA_AE_BASE_TARGET			0x4F
#define XDMA_AUTO_WHITE_BALANCE_ID			0x03
#define 	XDMA_WB_GAIN_RED			0x4E
#define 	XDMA_WB_GAIN_GREEN			0x4F
#define 	XDMA_WB_GAIN_BLUE			0x50
#define XDMA_ANTI_FLICKER_ID				0x04
#define 	XDMA_FD_MODE				0x04
#define 		XDMA_FD_MODE_MANUAL		(1 << 7)
#define 		XDMA_FD_MODE_50HZ		(1 << 6)
#define 		XDMA_FD_MODE_STATUS_50HZ	(1 << 5)
#define 	XDMA_FD_SEARCH_F1_50			0x08
#define 	XDMA_FD_SEARCH_F2_50			0x09
#define 	XDMA_FD_SEARCH_F1_60			0x0A
#define 	XDMA_FD_SEARCH_F2_60			0x0B
#define 	XDMA_FD_R9_STEP_F60_A			0x11
#define 	XDMA_FD_R9_STEP_F50_A			0x13
#define 	XDMA_FD_R9_STEP_F60_B			0x15
#define 	XDMA_FD_R9_STEP_F50_B			0x17
#define XDMA_MODE_VARIABLE_ID				0x07
#define 	XDMA_OUTPUT_WIDTH_A			0x03
#define 	XDMA_OUTPUT_HEIGHT_A			0x05
#define 	XDMA_OUTPUT_WIDTH_B			0x07
#define 	XDMA_OUTPUT_HEIGHT_B			0x09
#define 	XDMA_ROW_START_A			0x0D
#define 	XDMA_COL_START_A			0x0F
#define 	XDMA_ROW_END_A				0x11
#define 	XDMA_COL_END_A				0x13
#define 	XDMA_FRAME_LENGTH_A			0x1F
#define 	XDMA_LINE_LENGTH_A			0x21
#define 	XDMA_ROW_START_B			0x23
#define 	XDMA_COL_START_B			0x25
#define 	XDMA_ROW_END_B				0x27
#define 	XDMA_COL_END_B				0x29
#define 	XDMA_FRAME_LENGTH_B			0x35
#define 	XDMA_LINE_LENGTH_B			0x37
#define 	XDMA_CROP_X0_A				0x39
#define 	XDMA_CROP_X1_A				0x3B
#define 	XDMA_CROP_Y0_A				0x3D
#define 	XDMA_CROP_Y1_A				0x3F
#define 	XDMA_CROP_X0_B				0x47
#define 	XDMA_CROP_X1_B				0x49
#define 	XDMA_CROP_Y0_B				0x4B
#define 	XDMA_CROP_Y1_B				0x4D
#define 	XDMA_OUTPUT_FORMAT_A			0x55
#define 	XDMA_OUTPUT_FORMAT_B			0x57
#define 	XDMA_SPEC_EFFECTS_A			0x59
#define 	XDMA_SPEC_EFFECTS_B			0x5B
#define 	XDMA_COMMON_SEPIA_SETTINGS		0x63

#define XDMA_HISTOGRAM_VARIABLE_ID			0x0B

enum {
	LENGTH_16_BIT,
	LENGTH_8_BIT,
};

enum context {
	/* Preview */
	CONTEXT_A,
	/* Capture/Video */
	CONTEXT_B,
};

	/* Values need to be the ones from SEQ_STATE variable */
enum {
	MODE_PREVIEW = 0x3,
	MODE_CAPTURE = 0x7,
};

struct mt9d113_pll {
	u8 n;
	u8 m;
	u8 p1;
	u8 p3;
	u8 wcd;
};

struct mt9d113_clk_config {
	u32 ext_clk;
	struct mt9d113_pll * pll_config;
};

struct mt9d113_sensor {
	const struct mt9d113_platform_data *pdata;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;

	struct v4l2_pix_format pix;
	enum context ctx;
	enum v4l2_unbanding current_unbanding;
	int current_nightmode;
	int current_manual_wb;

	struct v4l2_fract timeperframe;

	struct mt9d113_clk_config *clk_config;
};

struct reg_value {
	u16	reg;
	u16 	val;
};

struct mt9d113_framesize_settings {
	struct mt9d113_clk_config *clk_cfg;
	struct reg_value *regs;
	u16 crop_X0;
	u16 crop_X1;
	u16 crop_Y0;
	u16 crop_Y1;
	u16 sensor_row_start;
	u16 sensor_row_end;
	u16 sensor_col_start;
	u16 sensor_col_end;
	u16 frame_length;
	u16 line_length;
	u16 R9_step;
	u16 R9_step_50;
	u16 R9_step_60;
	u8 fd_search_f1_50;
	u8 fd_search_f2_50;
	u8 fd_search_f1_60;
	u8 fd_search_f2_60;
	enum context ctx;
};

#endif /* _MT9D113_REGS_H */
