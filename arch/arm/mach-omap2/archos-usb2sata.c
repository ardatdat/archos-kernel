/*
 *    archos-usb2sata.c : 14/04/2010
 *    g.revaillot, revaillot@archos.com
 */

#include <linux/types.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <mach/gpio.h>
#include <mach/archos-gpio.h>
#include <mach/prcm.h>
#include <mach/mux.h>
#include <mach/board-archos.h>

#include "clock.h"

#define DELAY_500GB_SEAGATE

static struct archos_gpio gpio_sata_pwron = UNUSED_GPIO;
static struct archos_gpio gpio_hdd_pwron = UNUSED_GPIO;
static struct archos_gpio gpio_sata_rdy = UNUSED_GPIO;

static int satavcc = -1;

extern int archos_enable_ehci( int enable );

static struct platform_device usb2sata_device = {
	.name = "usb2sata",
	.id   = -1,
};

static void sataclk_enable(int on_off)
{
	struct clk *sys_clkout;

	sys_clkout = clk_get(NULL, "sys_clkout2");

	if (!IS_ERR(sys_clkout)) {
		if (on_off) {
			if (clk_enable(sys_clkout) != 0)
				printk(KERN_ERR "failed to enable clkout2\n");
		} else {
			clk_disable(sys_clkout);
		}

		clk_put(sys_clkout);
	} else {
		printk(KERN_ERR "failed to get clkout2\n");
	}
}

void usbsata_controller_power( int on_off)
{
	if (satavcc == on_off)
		return;

	gpio_set_value(GPIO_PIN(gpio_sata_pwron), on_off);
	sataclk_enable(on_off);
	
	satavcc = on_off;
}
EXPORT_SYMBOL(usbsata_controller_power);

void usbsata_power(int on_off)
{
	gpio_set_value(GPIO_PIN(gpio_hdd_pwron), on_off);
	if (on_off) {
		/* give the drive a bit of time before we enable the SATA */
		msleep(100);
#ifdef DELAY_500GB_SEAGATE
		msleep(400);
#endif
	}
	usbsata_controller_power(on_off);
#ifdef DELAY_500GB_SEAGATE
	if (on_off)
		msleep(500);
#endif
}
EXPORT_SYMBOL(usbsata_power);


static ssize_t show_usb2sata_satardy(struct device* dev, 
		struct device_attribute *attr, char* buf)
{
	int satardy = gpio_get_value(GPIO_PIN(gpio_sata_rdy));
	return snprintf(buf, PAGE_SIZE, "%d\n", satardy); 
}

static ssize_t show_usb2sata_satavcc(struct device* dev,
		struct device_attribute *attr, char* buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", satavcc); 
}

static ssize_t set_usb2sata_satavcc(struct device* dev,
		struct device_attribute *attr, const char* buf, size_t len)
{
	int on_off = simple_strtol(buf, NULL, 10);

	if( on_off > 1 || on_off < -1 )
		return -EINVAL;

	usbsata_power(on_off);

	return len;
}

static DEVICE_ATTR(satardy, S_IRUGO, show_usb2sata_satardy, NULL);
static DEVICE_ATTR(satavcc, S_IRUGO|S_IWUSR, show_usb2sata_satavcc, set_usb2sata_satavcc);

int __init archos_usb2sata_init(void)
{
	int ret;
	const struct archos_sata_config *sata_cfg;

	printk(KERN_DEBUG "%s\n", __FUNCTION__);

	sata_cfg = omap_get_config( ARCHOS_TAG_SATA, struct archos_sata_config );

	if (sata_cfg == NULL) {
		printk(KERN_DEBUG "%s: no board configuration found\n",
				__FUNCTION__);
		return -ENODEV;
	}

	if (hardware_rev >= sata_cfg->nrev) {
		printk(KERN_DEBUG "%s: hardware_rev (%i) >= nrev (%i)\n",
				__FUNCTION__, hardware_rev, sata_cfg->nrev);
		return -ENODEV;
	}

	ret = platform_device_register(&usb2sata_device);
	if (ret)
		return ret;

	/* SATA bridge */
	gpio_sata_pwron = sata_cfg->rev[hardware_rev].sata_power;

	ret = device_create_file(&usb2sata_device.dev, &dev_attr_satavcc);
	if (ret == 0) {
		archos_gpio_init_output(&gpio_sata_pwron, "sata pwr");
		gpio_set_value(GPIO_PIN(gpio_sata_pwron), 0);
	}

	/* HDD power switch */
	gpio_hdd_pwron = sata_cfg->rev[hardware_rev].hdd_power;

	archos_gpio_init_output(&gpio_hdd_pwron, "hdd pwr");

	gpio_set_value(GPIO_PIN(gpio_hdd_pwron), 0);

	/* SATA_RDY signal */
	gpio_sata_rdy = sata_cfg->rev[hardware_rev].sata_ready;

	ret = device_create_file(&usb2sata_device.dev, &dev_attr_satardy);
	if (ret == 0) {
		archos_gpio_init_input(&gpio_sata_rdy, "sata ready");
	}

	archos_enable_ehci(1);
	msleep( 500);		// fix me
	usbsata_power(1);

	return 0;
}
