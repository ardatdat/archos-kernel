/*
 *    archos-usb-touchscreen.c : 07/06/2010
 *    g.revaillot, revaillot@archos.com
 */

#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>

#include <mach/archos-gpio.h>
#include <mach/board-archos.h>

#include <linux/delay.h>

static struct archos_gpio gpio_tsp_enable = UNUSED_GPIO;
static struct archos_gpio gpio_tsp_reset = UNUSED_GPIO;

static int tsp_is_enabled = -1;
static int tsp_is_reset = -1;

static struct usb_tsp_platform_data tsp_platform_data;

static struct platform_device usb_tsp_device = {
	.name = "usb_tsp",
	.id = -1,
	.dev.platform_data = &tsp_platform_data,
};

static void usb_tsp_enable(int on_off)
{
	if (on_off == tsp_is_enabled)
		return;

	gpio_set_value(GPIO_PIN(gpio_tsp_enable), on_off);

	tsp_is_enabled = on_off;
}

static ssize_t show_usb_tsp_enable(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", tsp_is_reset);
};

static ssize_t store_usb_tsp_enable(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count)
{
	int on_off = simple_strtol(buf, NULL, 10);
	
	if( on_off > 1 || on_off < -1 )
		return -EINVAL;

	usb_tsp_enable(on_off);

	return count;
}
static DEVICE_ATTR(tsp_enable, S_IRUGO|S_IWUSR, show_usb_tsp_enable, store_usb_tsp_enable);

static void usb_tsp_reset(int on_off)
{
	if (on_off == tsp_is_reset)
		return;

	gpio_set_value(GPIO_PIN(gpio_tsp_reset), on_off);

	tsp_is_reset = on_off;
}

static ssize_t show_usb_tsp_reset(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", tsp_is_reset);
};

static ssize_t store_usb_tsp_reset(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count)
{
	int on_off = simple_strtol(buf, NULL, 10);
	
	if( on_off > 1 || on_off < -1 )
		return -EINVAL;

	usb_tsp_reset(on_off);

	return count;
}
static DEVICE_ATTR(tsp_reset, S_IRUGO|S_IWUSR, show_usb_tsp_reset, store_usb_tsp_reset);

static int __init archos_usb_tsp_init(void)
{
	int ret;
	const struct archos_usb_tsp_config *usb_tsp_config = 
			omap_get_config(ARCHOS_TAG_USB_TSP, struct archos_usb_tsp_config);
	const struct archos_usb_tsp_conf* conf;
	
	printk(KERN_DEBUG "%s\n", __FUNCTION__);
	
	if (usb_tsp_config == NULL) {
		return -ENODEV;
	}
	
	if (hardware_rev >= usb_tsp_config->nrev) {
		pr_err("archos_usb_tsp_init: no configuration for hardware_rev %d\n", 
				hardware_rev);
		return -ENODEV;
	}
	
	conf = &(usb_tsp_config->rev[hardware_rev]);
	
	tsp_platform_data.flags = conf->suspend_flags;
	tsp_platform_data.x_scale = conf->x_scale;
	tsp_platform_data.x_offset = conf->x_offset;
	tsp_platform_data.y_scale = conf->y_scale;
	tsp_platform_data.y_offset = conf->y_offset;
	
	ret = platform_device_register(&usb_tsp_device);
	if (ret < 0)
		return -ENODEV;

	if (GPIO_EXISTS(conf->enable)) {
		gpio_tsp_enable = conf->enable;
		archos_gpio_init_output(&gpio_tsp_enable, "tsp_enable");
		gpio_set_value(GPIO_PIN(gpio_tsp_enable), 0);

		ret = device_create_file(&usb_tsp_device.dev, &dev_attr_tsp_enable);
		if (ret == 0) {
			usb_tsp_enable(1);
		}
		
		tsp_platform_data.panel_power = usb_tsp_enable;
	}

	if (GPIO_EXISTS(conf->reset)) {
		gpio_tsp_reset = conf->reset;
		archos_gpio_init_output(&gpio_tsp_reset, "tsp_reset");
		gpio_set_value(GPIO_PIN(gpio_tsp_reset), 1);

		ret = device_create_file(&usb_tsp_device.dev, &dev_attr_tsp_reset);
		if (ret == 0) {
			usb_tsp_reset(1);
			msleep(200);
			usb_tsp_reset(0);
		}
	}

	return 0;
}

device_initcall(archos_usb_tsp_init);
