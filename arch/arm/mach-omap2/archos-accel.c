/*
 * MMA6060FC Accelerometer board configuration
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mma7660fc.h>

#include <mach/gpio.h>
#include <mach/mux.h>
#include <mach/archos-gpio.h>
#include <asm/mach-types.h>
#include <mach/board.h>
#include <mach/display.h>
#include <mach/board-archos.h>

int __init archos_accel_init(struct mma7660fc_pdata *pdata)
{
	struct archos_accel_conf accel_gpio;
	const struct archos_accel_config *accel_cfg;
	
	accel_cfg = omap_get_config( ARCHOS_TAG_ACCEL, struct archos_accel_config );
	if (accel_cfg == NULL) {
		printk(KERN_DEBUG "archos_accel_init: no board configuration found\n");
		return -ENODEV;
	}
	if ( hardware_rev >= accel_cfg->nrev ) {
		printk(KERN_DEBUG "archos_accel_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, accel_cfg->nrev);
		return -ENODEV;
	}

	accel_gpio = accel_cfg->rev[hardware_rev];

	/* irq needed by the driver */
	if (GPIO_PIN( accel_gpio.accel_int1 ) != -1)
		pdata->irq = gpio_to_irq(GPIO_PIN( accel_gpio.accel_int1 ));
	else
		pdata->irq = -1;
	printk("archos_accel_init: irq %d\n",pdata->irq);

	archos_gpio_init_input( &accel_gpio.accel_int1, "accel_int1");
	return 0;
}
