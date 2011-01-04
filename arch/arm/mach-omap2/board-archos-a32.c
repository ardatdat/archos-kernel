/*
 * board-archos-a32.c
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
	omap2_init_common_hw(edk2132c2pd_60_sdrc_params, omap3630_mpu_rate_table,
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
		.hdmi_pwr = 	UNUSED_GPIO,
		.hdmi_int = 	UNUSED_GPIO,
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.cvbs_on =	UNUSED_GPIO,
		.spi = {
			.spi_clk  = { .nb = 178, .mux_cfg = AA3_3430_MCSPI2_CLK },
			.spi_data = { .nb = 179, .mux_cfg = Y2_3430_MCSPI2_SIMO  },
			.spi_cs   = { .nb = 181, .mux_cfg = Y4_3430_MCSPI2_CS0  },
		},
	},
	.rev[1] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	UNUSED_GPIO,
		.hdmi_int = 	UNUSED_GPIO,
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.cvbs_on =	UNUSED_GPIO,
		.spi = {
			.spi_clk  = { .nb = 178, .mux_cfg = AA3_3430_MCSPI2_CLK },
			.spi_data = { .nb = 179, .mux_cfg = Y2_3430_MCSPI2_SIMO  },
			.spi_cs   = { .nb = 181, .mux_cfg = Y4_3430_MCSPI2_CS0  },
		},
	},
	.rev[2] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	UNUSED_GPIO,
		.hdmi_int = 	UNUSED_GPIO,
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.cvbs_on =	UNUSED_GPIO,
		.spi = {
			.spi_clk  = { .nb = 178, .mux_cfg = AA3_3430_MCSPI2_CLK },
			.spi_data = { .nb = 179, .mux_cfg = Y2_3430_MCSPI2_SIMO  },
			.spi_cs   = { .nb = 181, .mux_cfg = Y4_3430_MCSPI2_CS0  },
		},
	},
	.rev[3] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	UNUSED_GPIO,
		.hdmi_int = 	UNUSED_GPIO,
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.cvbs_on =	{ 29,	AH8_34XX_GPIO29 },
		.spi = {
			.spi_clk  = { .nb = 178, .mux_cfg = AA3_3430_MCSPI2_CLK },
			.spi_data = { .nb = 179, .mux_cfg = Y2_3430_MCSPI2_SIMO  },
			.spi_cs   = { .nb = 181, .mux_cfg = Y4_3430_MCSPI2_CS0  },
		},
	},
	.rev[4] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	UNUSED_GPIO,
		.hdmi_int = 	UNUSED_GPIO,
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.cvbs_on =	{ 29,	AH8_34XX_GPIO29 },
		.spi = {
			.spi_clk  = { .nb = 178, .mux_cfg = AA3_3430_MCSPI2_CLK },
			.spi_data = { .nb = 179, .mux_cfg = Y2_3430_MCSPI2_SIMO  },
			.spi_cs   = { .nb = 181, .mux_cfg = Y4_3430_MCSPI2_CS0  },
		},
	},
	.rev[5] = {
		.lcd_pwon = 	{ 157,	AA21_34XX_GPIO157_OUT },
		.lcd_rst = 	{ 25,	AF7_3430_GPIO25_OUT },
		.lcd_pci = 	UNUSED_GPIO,
		.hdmi_pwr = 	UNUSED_GPIO,
		.hdmi_int = 	UNUSED_GPIO,
		.vcom_pwm = 	{ .timer = -1, .mux_cfg = -1 },
		.cvbs_on =	{ 29,	AH8_34XX_GPIO29 },
		.spi = {
			.spi_clk  = { .nb = 178, .mux_cfg = AA3_3430_MCSPI2_CLK },
			.spi_data = { .nb = 179, .mux_cfg = Y2_3430_MCSPI2_SIMO  },
			.spi_cs   = { .nb = 181, .mux_cfg = Y4_3430_MCSPI2_CS0  },
		},
	},
};

static int __init archos_lcd_panel_init(struct omap_dss_device *disp_data)
{
	switch (hardware_rev) {
	default:
		return panel_wqvga_32_init(disp_data);
	}
}

static struct omap_uart_config uart_config __initdata = {
	.enabled_uarts	= ((1 << 0) | /*(1 << 1) |*/ (1 << 2)),
};

static struct archos_tsp_config tsp_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 175, .mux_cfg = AC3_34XX_GPIO175 },
		.x_plate_ohms = 250,
		.pressure_max = 1500,
		.inversion_flags = XY_SWAP | X_INV,
	},
	.rev[1] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 175, .mux_cfg = AC3_34XX_GPIO175 },
		.x_plate_ohms = 250,
		.pressure_max = 1500,
		.inversion_flags = XY_SWAP | X_INV,
	},
	.rev[2] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 175, .mux_cfg = AC3_34XX_GPIO175 },
		.x_plate_ohms = 250,
		.pressure_max = 1500,
		.inversion_flags = XY_SWAP | X_INV,
	},
	.rev[3] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 175, .mux_cfg = AC3_34XX_GPIO175 },
		.x_plate_ohms = 250,
		.pressure_max = 1500,
		.inversion_flags = XY_SWAP | X_INV,
	},
	.rev[4] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 175, .mux_cfg = AC3_34XX_GPIO175 },
		.x_plate_ohms = 250,
		.pressure_max = 1500,
		.inversion_flags = XY_SWAP | X_INV,
	},
	.rev[5] = {
		.irq_gpio = { .nb = 24, .mux_cfg = AE7_3430_GPIO24 },
		.pwr_gpio = { .nb = 175, .mux_cfg = AC3_34XX_GPIO175 },
		.x_plate_ohms = 250,
		.pressure_max = 1500,
		.inversion_flags = XY_SWAP | X_INV,
	},
};

static struct archos_audio_config audio_config __initdata = {
	.nrev = 6,
	.rev[0] = {
		.spdif = UNUSED_GPIO,
		.hp_on = UNUSED_GPIO,
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = -1,
	},
	.rev[1] = {
		.spdif = UNUSED_GPIO,
		.hp_on = UNUSED_GPIO,
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = AE22_3430_CLKOUT2,
	},
	.rev[2] = {
		.spdif = UNUSED_GPIO,
		.hp_on = UNUSED_GPIO,
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = AE22_3430_CLKOUT2,
	},
	.rev[3] = {
		.spdif = UNUSED_GPIO,
		.hp_on = UNUSED_GPIO,
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = AE22_3430_CLKOUT2,
	},
	.rev[4] = {
		.spdif = UNUSED_GPIO,
		.hp_on = UNUSED_GPIO,
		.headphone_plugged = { .nb = 109, .mux_cfg = B25_3430_GPIO109},
		.clk_mux = AE22_3430_CLKOUT2,
	},
	.rev[5] = {
		.spdif = UNUSED_GPIO,
		.hp_on = UNUSED_GPIO,
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
		.pwr_down = { .nb = 23, .mux_cfg = AG9_3430_GPIO23},
		.reset = { .nb = 26, .mux_cfg = AG7_3430_GPIO26},
	},
	.rev[1] = {
		.pwr_down = { .nb = 23, .mux_cfg = AG9_3430_GPIO23},
		.reset = { .nb = 26, .mux_cfg = AG7_3430_GPIO26},
	},
	.rev[2] = {
		.pwr_down = { .nb = 23, .mux_cfg = AG9_3430_GPIO23},
		.reset = { .nb = 26, .mux_cfg = AG7_3430_GPIO26},
	},
	.rev[3] = {
		.pwr_down = { .nb = 23, .mux_cfg = AG9_3430_GPIO23},
		.reset = { .nb = 26, .mux_cfg = AG7_3430_GPIO26},
	},
	.rev[4] = {
		.pwr_down = { .nb = 23, .mux_cfg = AG9_3430_GPIO23},
		.reset = { .nb = 26, .mux_cfg = AG7_3430_GPIO26},
	},
	.rev[5] = {
		.pwr_down = { .nb = 23, .mux_cfg = AG9_3430_GPIO23},
		.reset = { .nb = 26, .mux_cfg = AG7_3430_GPIO26},
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
		.backlight_power = UNUSED_GPIO,
	},
	.rev[2] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.backlight_power = UNUSED_GPIO,
	},
	.rev[3] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.backlight_power = UNUSED_GPIO,
	},
	.rev[4] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.backlight_power = UNUSED_GPIO,
	},
	.rev[5] = {
		.power_led = { .nb = 57, .mux_cfg = P8_34XX_GPIO57_OFF_LOW},
		.backlight_led = { .timer = 8, .mux_cfg = V3_3430_GPT08 },
		.backlight_power = UNUSED_GPIO,
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
static struct archos_gpio cvbs_on = UNUSED_GPIO;

static void panel_tv_init(void)
{
	if (hardware_rev >= display_config.nrev)
		return;

	pr_info("panel_tv_init\n");

	if (GPIO_EXISTS(display_config.rev[hardware_rev].cvbs_on)) {
		cvbs_on = display_config.rev[hardware_rev].cvbs_on;
		archos_gpio_init_output(&cvbs_on, "cvbs enable");
		gpio_set_value( GPIO_PIN(cvbs_on), 0 );
	}
}

static int panel_enable_tv(struct omap_dss_device *display)
{
	pr_info("panel_enable_tv\n");

	if (GPIO_EXISTS(cvbs_on)) {
		gpio_set_value( GPIO_PIN(cvbs_on), 1 );
		msleep(1);
	}
	return 0;
}

static void panel_disable_tv(struct omap_dss_device *display)
{
	pr_info("panel_disable_tv\n");

	if (GPIO_EXISTS(cvbs_on))
		gpio_set_value( GPIO_PIN(cvbs_on), 0 );

}

static struct omap_dss_device board_tv_device = {
	.name = "tv",
	.driver_name = "venc",
	.type = OMAP_DISPLAY_TYPE_VENC,
	.phy.venc.type = OMAP_DSS_VENC_TYPE_COMPOSITE,
	.platform_enable = panel_enable_tv,
	.platform_disable = panel_disable_tv,
};

static struct omap_dss_device *board_dss_devices[2];

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
struct vout_platform_data a32_vout_data = {
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
		.platform_data = &a32_vout_data,
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

extern struct platform_device camera_device;

static struct regulator_consumer_supply board_vdd_cam_supply = {
	.supply 	= "vdd_cam",
	.dev		= &camera_device.dev,
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

/* A32 only VOLUME UP/DOWN */
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
	.consumer_supplies	= &board_vdd_cam_supply,
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
	{MSG_SINGULAR(DEV_GRP_P1, RES_HFCLKOUT, RES_STATE_ACTIVE), 2},
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
/* FIXME: adding VDD1/2 to DEV_GRP_P1 causes device to hang after power-off.
 * 	commenting this fixes it but what is the impact on power consumption 
 * 	in OFF mode?
 * 
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
	.vmmc1          = &board_vmmc1,
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

static struct i2c_board_info __initdata board_i2c_bus2_info[] = {
	{
		I2C_BOARD_INFO("wm8988", 0x1a),
		.flags = I2C_CLIENT_WAKE,
	}
};

static struct i2c_board_info __initdata board_i2c_bus3_info[] = {
	{
		I2C_BOARD_INFO("mma7660fc", 0x4c),	// to fix
		.flags = I2C_CLIENT_WAKE,
		.platform_data = &board_mma7660fc_pdata,
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
	
	/* make sure I2C4 is configured correctly */
	omap_cfg_reg(AD26_34XX_I2C4_SCL);
	omap_cfg_reg(AE26_34XX_I2C4_SDA);

	return 0;
}


static struct platform_device *board_devices[] __initdata = {
	&board_vdds_dsi_device,
	&board_vmmc2_device,
#ifdef CONFIG_OMAP2_DSS
	&board_dss_device,
	&board_vout_device,
#endif
#ifdef CONFIG_WL127X_RFKILL
	&board_wl127x_device,
#endif
};

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
//	{
//		.mmc		= 1,
//		.wires		= 4,
//		.gpio_wp	= -EINVAL,
//	},
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

static void __init board_init(void)
{
	int num_displays = 0;
	
	omap_board_config = board_config;
	omap_board_config_size = ARRAY_SIZE(board_config);

	msecure_init();
	/* offmode config, before I2C config! */
	board_offmode_config();
	omap_i2c_init();
	
	/* Fix to prevent VIO leakage on wl127x */
	wl127x_vio_leakage_fix();
	
#if defined CONFIG_OMAP2_DSS
	if (archos_lcd_panel_init(&board_lcd_device) == 0) {
		board_dss_devices[num_displays++] = &board_lcd_device;
		board_dss_data.default_device = &board_lcd_device;
	}
	board_dss_devices[num_displays++] = &board_tv_device;
	board_dss_data.num_devices = num_displays;
	panel_tv_init();
#endif

	platform_add_devices(board_devices, ARRAY_SIZE(board_devices));

	omap_cfg_reg(H20_3430_UART3_RX_IRRX);
	omap_serial_init();
	usb_musb_init();

	archos_accel_init(&board_mma7660fc_pdata);

	ads7846_dev_init();

	twl4030_mmc_init(mmc);
	board_vmmc2_supply.dev = mmc[0].dev;

	archos_audio_gpio_init();
	archos_camera_ov7675_init();
	archos_leds_init();
	archos_keys_init();
	enable_board_wakeup_source();
}

static void __init board_map_io(void)
{
	omap2_set_globals_343x();
	omap2_map_common_io();
}

MACHINE_START(ARCHOS_A32, "Archos A32 board")
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xfa000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.fixup		= fixup_archos,
	.map_io		= board_map_io,
	.init_irq	= board_init_irq,
	.init_machine	= board_init,
	.timer		= &omap_timer,
MACHINE_END
