/*
 * A43 camera (MT9D113) Board configuration
 *
 */

#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <linux/module.h>

#include <mach/gpio.h>
#include <mach/mux.h>
#include <mach/archos-gpio.h>
#include <mach/board-archos.h>

static struct archos_gpio reset_gpio;
static struct archos_gpio pwdn_gpio;

static int gpio_nb[2];

struct platform_device a43_camera_device = {
	.name		= "omap-mt9d113-isp",
	.id		= -1,
	.dev            = {
		.platform_data = gpio_nb,
	},
};

int __init archos_camera_mt9d113_init(void)
{
	const struct archos_camera_config *camera_cfg;

	printk("%s\n", __FUNCTION__);

	camera_cfg = omap_get_config( ARCHOS_TAG_CAMERA, struct archos_camera_config );
	if (camera_cfg == NULL) {
		printk(KERN_DEBUG "archos_camera_init: no board configuration found\n");
		return -ENODEV;
	}
	if ( hardware_rev >= camera_cfg->nrev ) {
		printk(KERN_DEBUG "archos_camera_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, camera_cfg->nrev);
		return -ENODEV;
	}

	reset_gpio = camera_cfg->rev[hardware_rev].reset;
	pwdn_gpio = camera_cfg->rev[hardware_rev].pwr_down;

	GPIO_INIT_OUTPUT(reset_gpio);
	gpio_nb[0] = GPIO_PIN(reset_gpio);
	gpio_direction_output(GPIO_PIN(reset_gpio), 0);
	gpio_set_value(GPIO_PIN(reset_gpio), 0);

	GPIO_INIT_OUTPUT(pwdn_gpio);
	gpio_nb[1] = GPIO_PIN(pwdn_gpio);
	gpio_direction_output(GPIO_PIN(pwdn_gpio), 0);
	gpio_set_value(GPIO_PIN(pwdn_gpio), 0);

	omap_cfg_reg(A24_34XX_CAM_HS);
	omap_cfg_reg(A23_34XX_CAM_VS);
	omap_cfg_reg(C25_34XX_CAM_XCLKA);
	omap_cfg_reg(C27_34XX_CAM_PCLK);
	omap_cfg_reg(C23_34XX_CAM_FLD);
	omap_cfg_reg(AG17_34XX_CAM_D0);
	omap_cfg_reg(AH17_34XX_CAM_D1);
	omap_cfg_reg(B24_34XX_CAM_D2);
	omap_cfg_reg(C24_34XX_CAM_D3);
	omap_cfg_reg(D24_34XX_CAM_D4);
	omap_cfg_reg(A25_34XX_CAM_D5);
	omap_cfg_reg(K28_34XX_CAM_D6);
	omap_cfg_reg(L28_34XX_CAM_D7);
	omap_cfg_reg(B23_34XX_CAM_WEN);

	return platform_device_register(&a43_camera_device);
}
