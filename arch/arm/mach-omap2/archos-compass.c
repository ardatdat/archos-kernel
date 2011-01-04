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
#include <mach/display.h>
#include <mach/board-archos.h>

static struct archos_gpio gpio_compass_reset = UNUSED_GPIO;

static struct platform_device compass_device = {
  .name = "compass",
  .id   = -1, 
};

static ssize_t show_compass_reset(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_PIN(gpio_compass_reset)));
};

static ssize_t store_compass_reset(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count)
{
	int reset_state = simple_strtol(buf, NULL, 10);
	gpio_set_value(GPIO_PIN(gpio_compass_reset), reset_state ? 1:0);
	return count;
}

static DEVICE_ATTR(reset, S_IRUGO|S_IWUSR, show_compass_reset, store_compass_reset);

int __init archos_compass_init(void)
{
	const struct archos_compass_config *compass_config;
	int ret = 0;
	
	compass_config = omap_get_config( ARCHOS_TAG_COMPASS, struct archos_compass_config );

	if (compass_config == NULL) {
		printk(KERN_DEBUG "%s: no board configuration found\n",__FUNCTION__);
		return -ENODEV;
	}

	if ( hardware_rev >= compass_config->nrev ) {
		printk(KERN_DEBUG "%s: hardware_rev (%i) >= nrev (%i)\n", __FUNCTION__, hardware_rev, compass_config->nrev);
		return -ENODEV;
	}

	printk(KERN_DEBUG "%s\n",__FUNCTION__);

        gpio_compass_reset = compass_config->rev[hardware_rev].reset;
        GPIO_INIT_OUTPUT(gpio_compass_reset);

        gpio_set_value(GPIO_PIN(gpio_compass_reset), 1);

	if (( ret = platform_device_register(&compass_device)))
		return ret;

        if (( ret = device_create_file(&compass_device.dev, &dev_attr_reset)))
                return ret;

	return ret;
}

