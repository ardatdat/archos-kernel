/*
 * board-archos-a43.c
 *
 *  Created on: Jan 15, 2010
 *      Author: Matthias Welwarsky <welwarsky@archos.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>
#include <linux/regulator/machine.h>
#include <linux/i2c/twl4030.h>
#include <linux/mmc/host.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/mcspi.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/common.h>
#include <mach/gpmc.h>
#include <mach/usb.h>

#include <mach/mux.h>

#include <asm/io.h>
#include <asm/delay.h>
#include <mach/control.h>
#include <mach/mux.h>
#include <mach/display.h>
#include <mach/board-archos.h>

#include <linux/mma7660fc.h>

#ifdef CONFIG_WL127X_RFKILL
#include <linux/wl127x-rfkill.h>
#endif

#include "mmc-twl4030.h"
#include "omap3-opp.h"
#include "sdram-elpida-edk2132c2pb.h"

#ifdef CONFIG_PM
#include <../drivers/media/video/omap/omap_voutdef.h>
#endif

static struct mma7660fc_pdata board_mma7660fc_pdata;

extern int __init archos_audio_gpio_init(void);

static void __init board_init_irq(void)
{
	omap2_init_common_hw(edk2132c2pd_50_sdrc_params, omap3630_mpu_rate_table,
			     omap3630_dsp_rate_table, omap3630_l3_rate_table);

	omap_init_irq();
	omap_gpio_init();
}

static struct archos_display_config display_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	{ 181,	Y4_34XX_GPIO181 },
		.hdmi_int = 	{ 110,	C26_3430_GPIO110 },
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.spi = {
			.spi_clk  = UNUSED_GPIO,
			.spi_data = UNUSED_GPIO,
			.spi_cs   = UNUSED_GPIO,
		},
	},
	.rev[1] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	{ 181,	Y4_34XX_GPIO181 },
		.hdmi_int = 	{ 110,	C26_3430_GPIO110 },
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.spi = {
			.spi_clk  = UNUSED_GPIO,
			.spi_data = UNUSED_GPIO,
			.spi_cs   = UNUSED_GPIO,
		},
        },
	.rev[2] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	{ 181,	Y4_34XX_GPIO181 },
		.hdmi_int = 	{ 110,	C26_3430_GPIO110 },
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.spi = {
			.spi_clk  = UNUSED_GPIO,
			.spi_data = UNUSED_GPIO,
			.spi_cs   = UNUSED_GPIO,
		},
        },
	.rev[3] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	{ 181,	Y4_34XX_GPIO181 },
		.hdmi_int = 	{ 110,	C26_3430_GPIO110 },
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.spi = {
			.spi_clk  = UNUSED_GPIO,
			.spi_data = UNUSED_GPIO,
			.spi_cs   = UNUSED_GPIO,
		},
        },
	.rev[4] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	{ 181,	Y4_34XX_GPIO181 },
		.hdmi_int = 	{ 110,	C26_3430_GPIO110 },
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.spi = {
			.spi_clk  = UNUSED_GPIO,
			.spi_data = UNUSED_GPIO,
			.spi_cs   = UNUSED_GPIO,
		},
        },
	.rev[5] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	{ 181,	Y4_34XX_GPIO181 },
		.hdmi_int = 	{ 110,	C26_3430_GPIO110 },
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.spi = {
			.spi_clk  = UNUSED_GPIO,
			.spi_data = UNUSED_GPIO,
			.spi_cs   = UNUSED_GPIO,
		},
        },
};

static int __init archos_lcd_panel_init(struct omap_dss_device *disp_data)
{
	switch (hardware_rev) {
	default:
		return panel_fwvga_43_init(disp_data);
	}
}

static struct omap_uart_config uart_config __initdata = {
	.enabled_uarts	= ((1 << 0) /*| (1 << 1)*/ | (1 << 2)),
};


static struct archos_tsp_config tsp_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 179, .mux_cfg = Y2_3430_GPIO179 },
		.x_plate_ohms = 290,
		.pressure_max = 1500,
		.inversion_flags = X_INV | Y_INV,
	},
	.rev[1] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 179, .mux_cfg = Y2_3430_GPIO179 },
		.x_plate_ohms = 290,
		.pressure_max = 1500,
		.inversion_flags = X_INV | Y_INV,
	},
	.rev[2] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 179, .mux_cfg = Y2_3430_GPIO179 },
		.x_plate_ohms = 290,
		.pressure_max = 1500,
		.inversion_flags = X_INV | Y_INV,
	},
	.rev[3] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 179, .mux_cfg = Y2_3430_GPIO179 },
		.x_plate_ohms = 290,
		.pressure_max = 1500,
		.inversion_flags = X_INV | Y_INV,
	},
	.rev[4] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 179, .mux_cfg = Y2_3430_GPIO179 },
		.x_plate_ohms = 290,
		.pressure_max = 1500,
		.inversion_flags = X_INV | Y_INV,
	},
	.rev[5] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 179, .mux_cfg = Y2_3430_GPIO179 },
		.x_plate_ohms = 290,
		.pressure_max = 1500,
		.inversion_flags = X_INV | Y_INV,
	},
};

static struct archos_audio_config audio_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.spdif = UNUSED_GPIO,
		.hp_on = { .nb = 170, .mux_cfg = J25_34XX_GPIO170},
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = -1,
	},
	.rev[1] = {
		.spdif = UNUSED_GPIO,
		.hp_on = { .nb = 170, .mux_cfg = J25_34XX_GPIO170},
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = AE22_3430_CLKOUT2,
	},
	.rev[2] = {
		.spdif = UNUSED_GPIO,
		.hp_on = { .nb = 170, .mux_cfg = J25_34XX_GPIO170},
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = AE22_3430_CLKOUT2,
	},
	.rev[3] = {
		.spdif = UNUSED_GPIO,
		.hp_on = { .nb = 170, .mux_cfg = J25_34XX_GPIO170},
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = AE22_3430_CLKOUT2,
	},
	.rev[4] = {
		.spdif = UNUSED_GPIO,
		.hp_on = { .nb = 170, .mux_cfg = J25_34XX_GPIO170},
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = AE22_3430_CLKOUT2,
	},
	.rev[5] = {
		.spdif = UNUSED_GPIO,
		.hp_on = { .nb = 170, .mux_cfg = J25_34XX_GPIO170},
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = AE22_3430_CLKOUT2,
	},
};

static struct archos_charge_config charge_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.charge_enable 	= { .nb = 27, .mux_cfg = AH7_3430_GPIO27 },
		.charge_high	= { .nb = 28, .mux_cfg = AG8_3430_GPIO28 },
		.charge_low 	= { .nb = 22, .mux_cfg = AF9_3430_GPIO22 },
	},
	.rev[1] = {
		.charge_enable 	= { .nb = 27, .mux_cfg = AH7_3430_GPIO27 },
		.charge_high	= { .nb = 28, .mux_cfg = AG8_3430_GPIO28 },
		.charge_low 	= { .nb = 22, .mux_cfg = AF9_3430_GPIO22 },
	},
	.rev[2] = {
		.charge_enable 	= { .nb = 27, .mux_cfg = AH7_3430_GPIO27 },
		.charge_high	= { .nb = 28, .mux_cfg = AG8_3430_GPIO28 },
		.charge_low 	= { .nb = 22, .mux_cfg = AF9_3430_GPIO22 },
	},
	.rev[3] = {
		.charge_enable 	= { .nb = 27, .mux_cfg = AH7_3430_GPIO27 },
		.charge_high	= { .nb = 28, .mux_cfg = AG8_3430_GPIO28 },
		.charge_low 	= { .nb = 22, .mux_cfg = AF9_3430_GPIO22 },
	},
	.rev[4] = {
		.charge_enable 	= { .nb = 27, .mux_cfg = AH7_3430_GPIO27 },
		.charge_high	= { .nb = 28, .mux_cfg = AG8_3430_GPIO28 },
		.charge_low 	= { .nb = 22, .mux_cfg = AF9_3430_GPIO22 },
	},
	.rev[5] = {
		.charge_enable 	= { .nb = 27, .mux_cfg = AH7_3430_GPIO27 },
		.charge_high	= { .nb = 28, .mux_cfg = AG8_3430_GPIO28 },
		.charge_low 	= { .nb = 22, .mux_cfg = AF9_3430_GPIO22 },
	},
};


static struct archos_wifi_bt_config wifi_bt_dev_conf __initdata = {
	.nrev = 6,
	.rev[0] = {
		.wifi_power 	= { .nb = 111, .mux_cfg = B26_3430_GPIO111 },
		.wifi_irq 	= { .nb = 114, .mux_cfg = AG18_3430_GPIO114_UP },
		.bt_power 	= { .nb = 162, .mux_cfg = W21_3430_GPIO162 },
	},
	.rev[1] = {
		.wifi_power 	= { .nb = 111, .mux_cfg = B26_3430_GPIO111 },
		.wifi_irq 	= { .nb = 114, .mux_cfg = AG18_3430_GPIO114_UP },
		.bt_power 	= { .nb = 162, .mux_cfg = W21_3430_GPIO162 },
	},
	.rev[2] = {
		.wifi_power 	= { .nb = 111, .mux_cfg = B26_3430_GPIO111 },
		.wifi_irq 	= { .nb = 114, .mux_cfg = AG18_3430_GPIO114_UP },
		.bt_power 	= { .nb = 162, .mux_cfg = W21_3430_GPIO162 },
	},
	.rev[3] = {
		.wifi_power 	= { .nb = 111, .mux_cfg = B26_3430_GPIO111 },
		.wifi_irq 	= { .nb = 114, .mux_cfg = AG18_3430_GPIO114_UP },
		.bt_power 	= { .nb = 162, .mux_cfg = W21_3430_GPIO162 },
	},
	.rev[4] = {
		.wifi_power 	= { .nb = 111, .mux_cfg = B26_3430_GPIO111 },
		.wifi_irq 	= { .nb = 114, .mux_cfg = AG18_3430_GPIO114_UP },
		.bt_power 	= { .nb = 162, .mux_cfg = W21_3430_GPIO162 },
		.wifi_pa_type	= PA_TYPE_RF3482,
	},
	.rev[5] = {
		.wifi_power 	= { .nb = 111, .mux_cfg = B26_3430_GPIO111 },
		.wifi_irq 	= { .nb = 114, .mux_cfg = AG18_3430_GPIO114_UP },
		.bt_power 	= { .nb = 162, .mux_cfg = W21_3430_GPIO162 },
		.wifi_pa_type	= PA_TYPE_TQM67002A,
	},
};


static struct archos_gps_config gps_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.gps_enable = { .nb = 26, .mux_cfg = AG7_3430_GPIO26 },
		.gps_int = { .nb = 177, .mux_cfg = AB2_34XX_GPIO177 },
		.gps_reset = UNUSED_GPIO,
	},
	.rev[1] = {
		.gps_enable = { .nb = 26, .mux_cfg = AG7_3430_GPIO26 },
		.gps_int = { .nb = 177, .mux_cfg = AB2_34XX_GPIO177 },
		.gps_reset = UNUSED_GPIO,
	},
	.rev[2] = {
		.gps_enable = { .nb = 26, .mux_cfg = AG7_3430_GPIO26 },
		.gps_int = { .nb = 177, .mux_cfg = AB2_34XX_GPIO177 },
		.gps_reset = UNUSED_GPIO,
	},
	.rev[3] = {
		.gps_enable = { .nb = 26, .mux_cfg = AG7_3430_GPIO26 },
		.gps_int = { .nb = 177, .mux_cfg = AB2_34XX_GPIO177 },
		.gps_reset = UNUSED_GPIO,
	},
	.rev[4] = {
		.gps_enable = { .nb = 26, .mux_cfg = AG7_3430_GPIO26 },
		.gps_int = { .nb = 177, .mux_cfg = AB2_34XX_GPIO177 },
		.gps_reset = UNUSED_GPIO,
	},
	.rev[5] = {
		.gps_enable = { .nb = 26, .mux_cfg = AG7_3430_GPIO26 },
		.gps_int = { .nb = 177, .mux_cfg = AB2_34XX_GPIO177 },
		.gps_reset = UNUSED_GPIO,
	},
};

static struct archos_accel_config accel_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.accel_int1 = { .nb = 115, .mux_cfg = AH18_3430_GPIO115},
		.accel_int2 = UNUSED_GPIO,
	},
	.rev[1] = {
		.accel_int1 = { .nb = 115, .mux_cfg = AH18_3430_GPIO115},
		.accel_int2 = UNUSED_GPIO,
	},
	.rev[2] = {
		.accel_int1 = { .nb = 115, .mux_cfg = AH18_3430_GPIO115},
		.accel_int2 = UNUSED_GPIO,
	},
	.rev[3] = {
		.accel_int1 = { .nb = 115, .mux_cfg = AH18_3430_GPIO115},
		.accel_int2 = UNUSED_GPIO,
	},
	.rev[4] = {
		.accel_int1 = { .nb = 115, .mux_cfg = AH18_3430_GPIO115},
		.accel_int2 = UNUSED_GPIO,
	},
	.rev[5] = {
		.accel_int1 = { .nb = 115, .mux_cfg = AH18_3430_GPIO115},
		.accel_int2 = UNUSED_GPIO,
	},
};

static struct archos_camera_config camera_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.pwr_down = { .nb = 144, .mux_cfg = AB26_34XX_GPIO144},
		.reset = { .nb = 178, .mux_cfg = AA3_3430_GPIO178},
	},
	.rev[1] = {
		.pwr_down = { .nb = 129, .mux_cfg = R25_3430_GPIO129},
		.reset = { .nb = 178, .mux_cfg = AA3_3430_GPIO178},
	},
	.rev[2] = {
		.pwr_down = { .nb = 144, .mux_cfg = AB26_34XX_GPIO144},
		.reset = { .nb = 178, .mux_cfg = AA3_3430_GPIO178},
	},
	.rev[3] = {
		.pwr_down = { .nb = 144, .mux_cfg = AB26_34XX_GPIO144},
		.reset = { .nb = 178, .mux_cfg = AA3_3430_GPIO178},
	},
	.rev[4] = {
		.pwr_down = { .nb = 144, .mux_cfg = AB26_34XX_GPIO144},
		.reset = { .nb = 178, .mux_cfg = AA3_3430_GPIO178},
	},
	.rev[5] = {
		.pwr_down = { .nb = 144, .mux_cfg = AB26_34XX_GPIO144},
		.reset = { .nb = 178, .mux_cfg = AA3_3430_GPIO178},
	},
};

static struct archos_leds_config leds_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.backlight_power = UNUSED_GPIO,
	},
	.rev[1] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.bkl_max = 200,
		.backlight_power = UNUSED_GPIO,
	},
	.rev[2] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.bkl_max = 200,
		.backlight_power = UNUSED_GPIO,
	},
	.rev[3] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.bkl_max = 200,
		.backlight_power = UNUSED_GPIO,
	},
	.rev[4] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.bkl_max = 200,
		.backlight_power = UNUSED_GPIO,
	},
	.rev[5] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.bkl_max = 200,
		.backlight_power = UNUSED_GPIO,
	},
};

static struct archos_sd_config sd_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.sd_power = { .nb = 158, .mux_cfg = V21_3430_GPIO158 },
		.sd_detect = { .nb = 65, .mux_cfg = J8_3430_GPIO65 },
		.sd_prewarn = { .nb = 58, .mux_cfg = N8_3430_GPIO58_IN },
	},
	.rev[1] = {
		.sd_power = { .nb = 158, .mux_cfg = V21_3430_GPIO158 },
		.sd_detect = { .nb = 65, .mux_cfg = J8_3430_GPIO65 },
		.sd_prewarn = { .nb = 58, .mux_cfg = N8_3430_GPIO58_IN },
	},
	.rev[2] = {
		.sd_power = { .nb = 158, .mux_cfg = V21_3430_GPIO158 },
		.sd_detect = { .nb = 65, .mux_cfg = J8_3430_GPIO65 },
		.sd_prewarn = { .nb = 58, .mux_cfg = N8_3430_GPIO58_IN },
	},
	.rev[3] = {
		.sd_power = { .nb = 158, .mux_cfg = V21_3430_GPIO158 },
		.sd_detect = { .nb = 65, .mux_cfg = J8_3430_GPIO65 },
		.sd_prewarn = { .nb = 58, .mux_cfg = N8_3430_GPIO58_IN },
	},
	.rev[4] = {
		.sd_power = { .nb = 158, .mux_cfg = V21_3430_GPIO158 },
		.sd_detect = { .nb = 65, .mux_cfg = J8_3430_GPIO65 },
		.sd_prewarn = { .nb = 58, .mux_cfg = N8_3430_GPIO58_IN },
	},
	.rev[5] = {
		.sd_power = { .nb = 158, .mux_cfg = V21_3430_GPIO158 },
		.sd_detect = { .nb = 65, .mux_cfg = J8_3430_GPIO65 },
		.sd_prewarn = { .nb = 58, .mux_cfg = N8_3430_GPIO58_IN },
	},
};

static struct archos_compass_config compass_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.reset = { .nb = 128, .mux_cfg = R27_3430_GPIO128},
	},
	.rev[1] = {
		.reset = { .nb = 128, .mux_cfg = R27_3430_GPIO128},
	},
	.rev[2] = {
		.reset = { .nb = 128, .mux_cfg = R27_3430_GPIO128},
	},
	.rev[3] = {
		.reset = { .nb = 128, .mux_cfg = R27_3430_GPIO128},
	},
	.rev[4] = {
		.reset = { .nb = 128, .mux_cfg = R27_3430_GPIO128},
	},
	.rev[5] = {
		.reset = { .nb = 128, .mux_cfg = R27_3430_GPIO128},
	},
};

static struct archos_keys_config keys_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.power = { .nb = 64, .mux_cfg = K8_3430_GPIO64},
	},
	.rev[1] = {
		.power = { .nb = 64, .mux_cfg = K8_3430_GPIO64},
	},
	.rev[2] = {
		.power = { .nb = 64, .mux_cfg = K8_3430_GPIO64},
	},
	.rev[3] = {
		.power = { .nb = 64, .mux_cfg = K8_3430_GPIO64},
	},
	.rev[4] = {
		.power = { .nb = 64, .mux_cfg = K8_3430_GPIO64},
	},
	.rev[5] = {
		.power = { .nb = 64, .mux_cfg = K8_3430_GPIO64},
	},
};

static struct omap_board_config_kernel board_config[] __initdata = {
	{ OMAP_TAG_UART,	&uart_config },
	{ ARCHOS_TAG_DISPLAY,	&display_config },
	{ ARCHOS_TAG_TSP,	&tsp_config },
	{ ARCHOS_TAG_CHARGE,	&charge_config},
	{ ARCHOS_TAG_AUDIO,     &audio_config},
	{ ARCHOS_TAG_WIFI_BT,	&wifi_bt_dev_conf},
	{ ARCHOS_TAG_ACCEL,	&accel_config},
	{ ARCHOS_TAG_CAMERA,	&camera_config},
	{ ARCHOS_TAG_LEDS,	&leds_config},
	{ ARCHOS_TAG_GPS,	&gps_config},
	{ ARCHOS_TAG_SD,    	&sd_config},
	{ ARCHOS_TAG_COMPASS,	&compass_config},
	{ ARCHOS_TAG_KEYS,	&keys_config},
};

#ifdef CONFIG_WL127X_RFKILL
static struct wl127x_rfkill_platform_data wl127x_plat_data = {
	.bt_nshutdown_gpio = 162, 	/* Bluetooth Enable GPIO */
	.fm_enable_gpio = 0,		/* FM Enable GPIO */
};

static struct platform_device board_wl127x_device = {
	.name           = "wl127x-rfkill",
	.id             = -1,
	.dev.platform_data = &wl127x_plat_data,
};
#endif

#ifdef CONFIG_OMAP2_DSS
static struct omap_dss_device board_lcd_device;

static int board_panel_enable_hdmi(struct omap_dss_device *dssdev)
{
	return 0;
}

static void board_panel_disable_hdmi(struct omap_dss_device *dssdev)
{
}

#ifdef CONFIG_OMAP2_DSS_DUMMY
static struct omap_dss_device board_dummy_device = {
	.name = "dummy",
	.driver_name = "generic_panel",
	.type = OMAP_DISPLAY_TYPE_DUMMY,
	.phy.dpi.data_lines = 24,
};
#endif

static struct omap_dss_device board_hdmi_device = {
	.name = "hdmi",
	.driver_name = "hdmi_panel",
	.type = OMAP_DISPLAY_TYPE_HDMI,
	.phy.dpi.data_lines = 24,
	.platform_enable = board_panel_enable_hdmi,
	.platform_disable = board_panel_disable_hdmi,
};

#ifdef CONFIG_OMAP2_DSS_DUMMY
static struct omap_dss_device *board_dss_devices[3];
#else
static struct omap_dss_device *board_dss_devices[2];
#endif

static struct omap_dss_board_info board_dss_data = {
	.get_last_off_on_transaction_id = get_last_off_on_transaction_id,
	.num_devices = 0,
	.devices = board_dss_devices,
	.default_device = NULL,
};

static struct platform_device board_dss_device = {
	.name		= "omapdss",
	.id		= -1,
	.dev            = {
		.platform_data = &board_dss_data,
	},
};

static struct resource board_vout_resource[3 - CONFIG_FB_OMAP2_NUM_FBS] = {
};

#ifdef CONFIG_PM
struct vout_platform_data a43_vout_data = {
	.set_min_bus_tput = omap_pm_set_min_bus_tput,
	.set_max_mpu_wakeup_lat =  omap_pm_set_max_mpu_wakeup_lat,
	.set_vdd1_opp = omap_pm_set_min_mpu_freq,
	.set_cpu_freq = omap_pm_cpu_set_freq,
};
#endif

static struct platform_device board_vout_device = {
	.name		= "omap_vout",
	.num_resources	= ARRAY_SIZE(board_vout_resource),
	.resource 	= &board_vout_resource[0],
	.id		= -1,
#ifdef CONFIG_PM
	.dev		= {
		.platform_data = &a43_vout_data,
	}
#else
	.dev		= {
		.platform_data = NULL,
	}
#endif
};
#endif

static struct regulator_consumer_supply board_vdda_dac_supply = {
	.supply		= "vdda_dac",
#ifdef CONFIG_OMAP2_DSS
	.dev		= &board_dss_device.dev,
#endif
};

static struct regulator_consumer_supply board_avcc_supply = {
	.supply		= "avcc",
};

extern struct platform_device a43_camera_device;

static struct regulator_consumer_supply board_2v8d_cam_supply = {
	.supply 	= "2v8d_cam",
	.dev		= &a43_camera_device.dev,
};

/* consumer for vdds_dsi which is permanently enabled */
static struct regulator_consumer_supply board_vdds_dsi_supply = {
	.supply		= "vdds_dsi",
#ifdef CONFIG_OMAP2_DSS
	.dev		= &board_dss_device.dev,
#endif
};

/* ------ TPS65921 init data ---------- */

#define TWL4030_MSECURE_GPIO	159

static int __init msecure_init(void)
{
	int ret = 0;

#ifdef CONFIG_RTC_DRV_TWL4030
	void __iomem *msecure_pad_config_reg =
		omap_ctrl_base_get() + 0x192;
	int mux_mask = 0x04;
	u16 tmp;

	ret = gpio_request(TWL4030_MSECURE_GPIO, "msecure");
	if (ret < 0) {
		printk(KERN_ERR "msecure_init: can't"
			"reserve GPIO:%d !\n", TWL4030_MSECURE_GPIO);
		goto out;
	}
	
	/*
	 * TWL4030 will be in secure mode if msecure line from OMAP
	 * is low. Make msecure line high in order to change the
	 * TWL4030 RTC time and calender registers.
	 */
	tmp = __raw_readw(msecure_pad_config_reg);
	tmp &= 0xF8;	/* To enable mux mode 03/04 = GPIO_RTC */
	tmp |= mux_mask;/* To enable mux mode 03/04 = GPIO_RTC */
	__raw_writew(tmp, msecure_pad_config_reg);

	gpio_direction_output(TWL4030_MSECURE_GPIO, 1);
out:
#endif
	return ret;
}

/* VOLUME UP/DOWN */
static int board_twl4030_keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(1, 0, KEY_VOLUMEDOWN),
	0
};

static struct twl4030_keypad_data board_kp_twl4030_data = {
	.rows		= 1,
	.cols		= 2,
	.keymap		= board_twl4030_keymap,
	.keymapsize	= ARRAY_SIZE(board_twl4030_keymap),
	.rep		= 1,
};


static struct regulator_init_data board_vdac = {
	.constraints = {
		.min_uV                 = 1800000,
		.max_uV                 = 1800000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &board_vdda_dac_supply,
};

static struct regulator_init_data board_vmmc1 = {
	.constraints = {
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.always_on		= 1,
		.apply_uV		= 1,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &board_avcc_supply,
};

static struct regulator_init_data board_vaux2 = {
	.constraints = {
		.min_uV			= 2800000,
		.max_uV			= 2800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.always_on		= 1,
		.apply_uV		= 1,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &board_2v8d_cam_supply,
};

static struct twl4030_usb_data board_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
	.enable_charge_detect = 1, 
};

/* TODO: might not be needed
static struct twl4030_gpio_platform_data board_gpio_data = {
	.gpio_base	= OMAP_MAX_GPIO_LINES,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.setup		= zoom2_twl_gpio_setup,
};
*/
static struct twl4030_madc_platform_data board_madc_data = {
	.irq_line	= 1,
};

static struct twl4030_ins __initdata sleep_on_seq[] = {

	/* Turn OFF VAUX2 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VAUX2, RES_STATE_OFF), 2},
	/* Turn off HFCLKOUT */
	{MSG_SINGULAR(DEV_GRP_P1, RES_HFCLKOUT, RES_STATE_OFF), 2},
	/* Turn OFF VDD1 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD1, RES_STATE_OFF), 2},
	/* Turn OFF VDD2 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD2, RES_STATE_OFF), 2},
	/* Turn OFF VPLL1 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VPLL1, RES_STATE_OFF), 2},
};

static struct twl4030_script sleep_on_script __initdata = {
	.script	= sleep_on_seq,
	.size	= ARRAY_SIZE(sleep_on_seq),
	.flags	= TRITON_SLEEP_SCRIPT,
};

static struct twl4030_ins wakeup_p12_seq[] __initdata = {
	/* Turn on VAUX2 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VAUX2, RES_STATE_ACTIVE), 2},
	/* Turn on HFCLKOUT */
	{MSG_SINGULAR(DEV_GRP_P1, RES_HFCLKOUT, RES_STATE_ACTIVE), 2},
	/* Turn ON VDD1 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD1, RES_STATE_ACTIVE), 2},
	/* Turn ON VDD2 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD2, RES_STATE_ACTIVE), 2},
	/* Turn ON VPLL1 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VPLL1, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wakeup_p12_script __initdata = {
	.script = wakeup_p12_seq,
	.size   = ARRAY_SIZE(wakeup_p12_seq),
	.flags  = TRITON_WAKEUP12_SCRIPT,
};

static struct twl4030_ins wakeup_p3_seq[] __initdata = {
	{MSG_SINGULAR(DEV_GRP_P1, 0x19, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wakeup_p3_script __initdata = {
	.script = wakeup_p3_seq,
	.size   = ARRAY_SIZE(wakeup_p3_seq),
	.flags  = TRITON_WAKEUP3_SCRIPT,
};

static struct twl4030_ins wrst_seq[] __initdata = {
/*
 * Reset twl4030.
 * Reset VDD1 regulator.
 * Reset VDD2 regulator.
 * Reset VPLL1 regulator.
 * Enable sysclk output.
 * Reenable twl4030.
 */
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_OFF), 2},
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD1, RES_STATE_WRST), 15},
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD2, RES_STATE_WRST), 15},
	{MSG_SINGULAR(DEV_GRP_P1, RES_VPLL1, RES_STATE_WRST), 0x60},
	{MSG_SINGULAR(DEV_GRP_P1, RES_HFCLKOUT, RES_STATE_ACTIVE), 2},
	{MSG_SINGULAR(DEV_GRP_P1, RES_VAUX2, RES_STATE_ACTIVE), 2},
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wrst_script __initdata = {
	.script = wrst_seq,
	.size   = ARRAY_SIZE(wrst_seq),
	.flags  = TRITON_WRST_SCRIPT,
};

static struct twl4030_script *twl4030_scripts[] __initdata = {
	&sleep_on_script,
	&wakeup_p12_script,
	&wakeup_p3_script,
	&wrst_script,
};

static struct twl4030_resconfig twl4030_rconfig[] = {
	{ .resource = RES_HFCLKOUT, .devgroup = DEV_GRP_P3, .type = -1,
		.type2 = -1 },
/* XXX removed, breaks booting after power-off
	{ .resource = RES_VDD1, .devgroup = DEV_GRP_P1, .type = -1,
		.type2 = -1 },
	{ .resource = RES_VDD2, .devgroup = DEV_GRP_P1, .type = -1,
		.type2 = -1 },
*/
	{ 0, 0},
};

static struct twl4030_resconfig twl4030_rconfig_vdd_cam_off[] = {
	{ .resource = RES_HFCLKOUT, .devgroup = DEV_GRP_P3, .type = -1,
		.type2 = -1 },
 	{ .resource = RES_VAUX2, .devgroup = DEV_GRP_P1, .type = -1,
		.type2 = -1 },
	{ 0, 0},
};

static struct twl4030_power_data board_t2scripts_data __initdata = {
	.scripts	= twl4030_scripts,
	.size		= ARRAY_SIZE(twl4030_scripts),
	.resource_config = twl4030_rconfig,
};

static struct twl4030_platform_data board_tps65921_pdata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,

	/* platform_data for children goes here */
	.madc		= &board_madc_data,
	.usb		= &board_usb_data,
	.keypad		= &board_kp_twl4030_data,
	.power		= &board_t2scripts_data,
	.vmmc1          = &board_vmmc1, /* used for VACC :-( */
	.vdac		= &board_vdac,
	.vaux2          = &board_vaux2,
};

/* fixed dummy regulator for 1.8v vdds_dsi rail */
static struct regulator_init_data board_vdds_dsi = {
	.constraints = {
		.name			= "vdds_dsi",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.always_on		= 1,
		.boot_on		= 1,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &board_vdds_dsi_supply,
};

static struct platform_device board_vdds_dsi_device = {
	.name		= "reg-fixed-voltage",
	.id		= 0,
	.dev.platform_data = &board_vdds_dsi,
};

static struct regulator_consumer_supply board_vmmc_ext_supply = {
	.supply	= "vmmc",
};
static struct regulator_init_data board_vmmc_ext = {
	.constraints = {
		.name 			= "vmmc",
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.always_on		= 0,
		.boot_on		= 0,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &board_vmmc_ext_supply,
};

static struct platform_device board_vmmc_ext_device = {
	.name		= "reg-fsw-voltage",
	.id		= 0,
	.dev.platform_data = &board_vmmc_ext,
};

static struct regulator_consumer_supply board_vmmc2_supply = {
	.supply	= "vmmc",
};
static struct regulator_init_data board_vmmc2 = {
	.constraints = {
		.name 			= "vmmc",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.always_on		= 1,
		.boot_on		= 1,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &board_vmmc2_supply,
};

static struct platform_device board_vmmc2_device = {
	.name		= "reg-fixed-voltage",
	.id		= 1,
	.dev.platform_data = &board_vmmc2,
};

static struct i2c_board_info __initdata board_i2c_bus1_info[] = {
	{
		I2C_BOARD_INFO("tps65921", 0x48),	// id to fix
		.flags = I2C_CLIENT_WAKE,
		.irq = INT_34XX_SYS_NIRQ,
		.platform_data = &board_tps65921_pdata,
	},
};

static void archos_hdmi_power(struct i2c_client *cl, int on_off)
{
	struct device *dev = &cl->dev;
	struct archos_hdmi_platform_data *pdata = dev->platform_data;
	
	if (on_off)
		gpio_direction_output(pdata->pwr_gpio, 1);
	else
		gpio_direction_input(pdata->pwr_gpio);
}

static struct archos_hdmi_platform_data board_hdmi_pdata = {
	.hdmi_pwr = archos_hdmi_power,
};

static struct i2c_board_info __initdata board_i2c_bus2_info[] = {
	[0] = {
		I2C_BOARD_INFO("wm8988", 0x1a),
		.flags = I2C_CLIENT_WAKE,
	},
	[1] = {
		I2C_BOARD_INFO("tda998X", 0x70),
		.flags = I2C_CLIENT_WAKE,
		.platform_data = &board_hdmi_pdata,
	},
	[2] = {
		I2C_BOARD_INFO("tda99Xcec", 0x34),
		.flags = I2C_CLIENT_WAKE,
		.platform_data = &board_hdmi_pdata,
	}
};

static struct i2c_board_info __initdata board_i2c_bus3_info[] = {
	{
		I2C_BOARD_INFO("mma7660fc", 0x4c),	// to fix
		.flags = I2C_CLIENT_WAKE,
		.platform_data = &board_mma7660fc_pdata,
	},
	{
		I2C_BOARD_INFO("akm8973", 0x1c),
		.flags = I2C_CLIENT_WAKE,
	},
};

static int __init omap_i2c_init(void)
{
	/* Disable OMAP 3630 internal pull-ups for I2Ci */
	if (cpu_is_omap3630()) {

		u32 prog_io;

		prog_io = omap_ctrl_readl(OMAP343X_CONTROL_PROG_IO1);
		/* Program (bit 19)=1 to disable internal pull-up on I2C1 */
		prog_io |= OMAP3630_PRG_I2C1_PULLUPRESX;
		/* Program (bit 0)=1 to disable internal pull-up on I2C2 */
		prog_io |= OMAP3630_PRG_I2C2_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP343X_CONTROL_PROG_IO1);

		prog_io = omap_ctrl_readl(OMAP36XX_CONTROL_PROG_IO2);
		/* Program (bit 7)=1 to disable internal pull-up on I2C3 */
		prog_io |= OMAP3630_PRG_I2C3_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP36XX_CONTROL_PROG_IO2);
		prog_io = omap_ctrl_readl(OMAP36XX_CONTROL_PROG_IO_WKUP1);
		/* Program (bit 5)=1 to disable internal pull-up on I2C4(SR) */
		prog_io |= OMAP3630_PRG_SR_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP36XX_CONTROL_PROG_IO_WKUP1);
	}

	omap_register_i2c_bus(1, 400, board_i2c_bus1_info,
			ARRAY_SIZE(board_i2c_bus1_info));
	omap_register_i2c_bus(2, 100, board_i2c_bus2_info,
			ARRAY_SIZE(board_i2c_bus2_info));
	omap_register_i2c_bus(3, 100, board_i2c_bus3_info,
			ARRAY_SIZE(board_i2c_bus3_info));

	return 0;
}

static struct platform_device *board_devices[] __initdata = {
	&board_vdds_dsi_device,
	&board_vmmc_ext_device,
	&board_vmmc2_device,
#ifdef CONFIG_OMAP2_DSS
	&board_dss_device,
	&board_vout_device,
#endif
#ifdef CONFIG_WL127X_RFKILL
	&board_wl127x_device,
#endif
};

static void __init archos_hdmi_gpio_init(
		const struct archos_disp_conf* disp_conf)
{
	/* driver will manage the GPIO, just apply the pin multiplexing */
	omap_cfg_reg(GPIO_MUX(disp_conf->hdmi_int));
	
	archos_gpio_init_output(&disp_conf->hdmi_pwr, "hdmi pwr");
	/* FIXME: make userspace configurable */
	gpio_set_value(GPIO_PIN(disp_conf->hdmi_pwr), 1);

	/* patch power gpio into platform data */
	board_hdmi_pdata.pwr_gpio = GPIO_PIN(disp_conf->hdmi_pwr);
	
	/* patch IRQ into HDMI I2C bus info */
	board_i2c_bus2_info[1].irq = gpio_to_irq(GPIO_PIN(disp_conf->hdmi_int));
	board_i2c_bus2_info[2].irq = gpio_to_irq(GPIO_PIN(disp_conf->hdmi_int));

	omap_cfg_reg(H26_3630_DSS_DATA0);
	omap_cfg_reg(H25_3630_DSS_DATA1);
	omap_cfg_reg(E28_3630_DSS_DATA2);
	omap_cfg_reg(J26_3630_DSS_DATA3);
	omap_cfg_reg(AC27_3630_DSS_DATA4);
	omap_cfg_reg(AC28_3630_DSS_DATA5);
	
	omap_cfg_reg(D26_3630_DSS_HSYNC);
	omap_cfg_reg(D27_3630_DSS_VSYNC);
	omap_cfg_reg(D28_3630_DSS_PCLK);
	omap_cfg_reg(E27_3630_DSS_ACBIAS);
	omap_cfg_reg(E26_3630_DSS_DATA6);
	omap_cfg_reg(F28_3630_DSS_DATA7);
	omap_cfg_reg(F27_3630_DSS_DATA8);
	omap_cfg_reg(G26_3630_DSS_DATA9);
	omap_cfg_reg(AD28_3630_DSS_DATA10);
	omap_cfg_reg(AD27_3630_DSS_DATA11);
	omap_cfg_reg(AB28_3630_DSS_DATA12);
	omap_cfg_reg(AB27_3630_DSS_DATA13);
	omap_cfg_reg(AA28_3630_DSS_DATA14);
	omap_cfg_reg(AA27_3630_DSS_DATA15);
	omap_cfg_reg(G25_3630_DSS_DATA16);
	omap_cfg_reg(H27_3630_DSS_DATA17);

	omap_cfg_reg(AH26_3630_DSS_DATA18);
	omap_cfg_reg(AG26_3630_DSS_DATA19);
	omap_cfg_reg(AF18_3630_DSS_DATA20);
	omap_cfg_reg(AF19_3630_DSS_DATA21);
	omap_cfg_reg(AE21_3630_DSS_DATA22);
	omap_cfg_reg(AF21_3630_DSS_DATA23);
}

static int __init wl127x_vio_leakage_fix(void)
{
	int ret = 0;
	const struct archos_wifi_bt_config *conf = &wifi_bt_dev_conf;
	struct archos_gpio bten_gpio;
	
	if (hardware_rev >= conf->nrev)
		return -ENODEV;
	
	bten_gpio = conf->rev[hardware_rev].bt_power;
	
	ret = gpio_request(GPIO_PIN(bten_gpio), "wl127x_bten");
	if (ret < 0) {
		printk(KERN_ERR "wl127x_bten gpio_%d request fail",
						GPIO_PIN(bten_gpio));
		goto fail;
	}

	gpio_direction_output(GPIO_PIN(bten_gpio), 1);
	mdelay(10);
	gpio_direction_output(GPIO_PIN(bten_gpio), 0);
	udelay(64);

	gpio_free(GPIO_PIN(bten_gpio));

fail:
	return ret;
}

static struct twl4030_hsmmc_info mmc[] __initdata = {
	{
		.mmc		= 1,
		.wires		= 4,
		.gpio_wp	= -EINVAL,
		.gpio_cd	= -EINVAL,
	},
	{
		.mmc		= 2,
		.wires		= 8,
		.gpio_wp	= -EINVAL,
		.gpio_cd	= -EINVAL,
		.ocr_mask	= MMC_VDD_165_195,
	},
	{
		.mmc		= 3,
		.wires		= 4,
		.gpio_wp	= -EINVAL,
		.gpio_cd	= -EINVAL,
	},
	{}      /* Terminator */
};

static int archos_mmc1_setup_gpios(struct twl4030_hsmmc_info *c, struct regulator_init_data *reg_init_data)
{
 	const struct archos_sd_config *hsmmc_cfg;
	struct archos_gpio gpio_hsmmc_power, gpio_hsmmc_cd,gpio_hsmmc_cpd;
	
	/* sd */
	hsmmc_cfg = omap_get_config( ARCHOS_TAG_SD, struct archos_sd_config );
	if (hsmmc_cfg == NULL) {
		printk(KERN_DEBUG "%s: no board configuration found\n", __FUNCTION__);
		return -ENODEV;
	}
	if ( hardware_rev >= hsmmc_cfg->nrev ) {
		printk(KERN_DEBUG "%s: hardware_rev (%i) >= nrev (%i)\n",
			__FUNCTION__, hardware_rev, hsmmc_cfg->nrev);
		return -ENODEV;
	}
	
	printk(KERN_DEBUG "%s\n", __FUNCTION__);

	gpio_hsmmc_power = hsmmc_cfg->rev[hardware_rev].sd_power;
	GPIO_INIT_OUTPUT( gpio_hsmmc_power );

	reg_init_data->driver_data = (void*)GPIO_PIN( gpio_hsmmc_power );
	
	gpio_hsmmc_cd = hsmmc_cfg->rev[hardware_rev].sd_detect;
	gpio_hsmmc_cpd = hsmmc_cfg->rev[hardware_rev].sd_prewarn;
	
	/* card detect */
	c->gpio_cd = GPIO_PIN(gpio_hsmmc_cd);	

	/* Need to be here, before omap2_init_mmc which will correctly set the IRQ stuff */
	GPIO_INIT_INPUT(gpio_hsmmc_cd);	
	gpio_free(c->gpio_cd);
	
	return 0;
}

static void enable_board_wakeup_source(void)
{
	omap_cfg_reg(AF26_34XX_SYS_NIRQ);
}

static void board_offmode_config(void)
{
	/* mass production boards have a proper power supply for the 26MHz XO
	 * and we can turn off the +1V8 supply rail in OFF mode */
	if (hardware_rev >= 3) {
		gpio_request(161, "pwren2");
		omap_cfg_reg( K26_OMAP34XX_GPIO161_OFF_IPU ); // PWREN2 test
		gpio_direction_input(161); // we have a pull-down while active and a pull-up while in OFF mode

		/* if we can turn off the 1.8V, we need to turn off the VDD_CAM, too */
		board_tps65921_pdata.power->resource_config = twl4030_rconfig_vdd_cam_off;
		
	}
}

static void init_unused(void)
{
	gpio_request(23, "vibrate");
	omap_cfg_reg( AG9_3430_GPIO23 ); 
	gpio_direction_input(23); // we have a pull-down

	gpio_request(145, "compa_rst");
	omap_cfg_reg( AB25_3630_GPIO145 );
	gpio_direction_input(145); // we have a pull-down
}

static void __init board_init(void)
{
	int num_displays = 0;

	omap_board_config = board_config;
	omap_board_config_size = ARRAY_SIZE(board_config);

	/* initialize gpios reserved for unsoldered parts */
	init_unused();
	
	/* offmode config, before I2C config! */
	board_offmode_config();
	
	/* before omap_i2c_init() or IRQ will not forwarded to driver */
	if (display_config.nrev > hardware_rev)
		archos_hdmi_gpio_init(&display_config.rev[hardware_rev]);
	
	omap_i2c_init();
	/* Fix to prevent VIO leakage on wl127x */
	wl127x_vio_leakage_fix();

#if defined CONFIG_OMAP2_DSS
	if (archos_lcd_panel_init(&board_lcd_device) == 0) {
		board_dss_devices[num_displays++] = &board_lcd_device;
		board_dss_data.default_device = &board_lcd_device;
	}
	board_dss_devices[num_displays++] = &board_hdmi_device;
#ifdef CONFIG_OMAP2_DSS_DUMMY
	board_dss_devices[num_displays++] = &board_dummy_device;
	board_dss_data.default_device = &board_dummy_device;
#endif /* CONFIG_OMAP2_DSS_DUMMY */
	board_dss_data.num_devices = num_displays;
#endif/* CONFIG_OMAP2_DSS */

	platform_add_devices(board_devices, ARRAY_SIZE(board_devices));

	msecure_init();
	omap_cfg_reg(H20_3430_UART3_RX_IRRX);
	omap_serial_init();
	usb_musb_init();

	archos_accel_init(&board_mma7660fc_pdata);

	ads7846_dev_init();

	archos_mmc1_setup_gpios(&mmc[0], &board_vmmc_ext);
	twl4030_mmc_init(mmc);
	board_vmmc_ext_supply.dev = mmc[0].dev;
	board_vmmc2_supply.dev = mmc[1].dev;
	
	archos_audio_gpio_init();

	archos_camera_mt9d113_init();
	archos_leds_init();
	archos_keys_init();
	archos_compass_init();
	enable_board_wakeup_source();
}

static void __init board_map_io(void)
{
	omap2_set_globals_343x();
	omap2_map_common_io();
}

MACHINE_START(ARCHOS_A43, "Archos A43 board")
	.phys_io	= 0x48000000,
	.io_pg_offst    = ((0xfa000000) >> 18) & 0xfffc, 
	.boot_params	= 0x80000100,
	.fixup		= fixup_archos,
	.map_io		= board_map_io,
	.init_irq	= board_init_irq,
	.init_machine	= board_init,
	.timer		= &omap_timer,
MACHINE_END
