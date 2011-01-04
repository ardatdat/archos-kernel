/* linux/arch/arm/mach-omap2/archos-wifi.c
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>

#include <asm/gpio.h>
#include <asm/io.h>

#include <mach/board-archos.h>
#include <linux/wifi_tiwlan.h>

static int archos_wifi_cd = 0;		/* WIFI virtual 'card detect' status */
static int archos_wifi_reset_state;
static int archos_wifi_pa_type = 0;
static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;
static struct archos_gpio wifi_irq;
static struct archos_gpio wifi_pmena;

int omap_wifi_status_register(void (*callback)(int card_present,
						void *dev_id), void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

int archos_wifi_status(int irq)
{
	return archos_wifi_cd;
}

int archos_wifi_set_carddetect(int val)
{
	printk("%s: %d\n", __func__, val);
	archos_wifi_cd = val;
	if (wifi_status_cb) {
		wifi_status_cb(val, wifi_status_cb_devid);
	} else
		printk(KERN_WARNING "%s: Nobody to notify\n", __func__);
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(archos_wifi_set_carddetect);
#endif

static int archos_wifi_power_state;

int archos_wifi_power(int on)
{
	printk("%s: %d\n", __func__, on);
	gpio_set_value(GPIO_PIN(wifi_pmena), on);
	archos_wifi_power_state = on;
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(archos_wifi_power);
#endif

int archos_wifi_reset(int on)
{
	printk("%s: %d\n", __func__, on);
	archos_wifi_reset_state = on;
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(archos_wifi_reset);
#endif

static ssize_t store_wifi_reset(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count)
{
	int on_off = simple_strtol(buf, NULL, 10);
	
	if( on_off > 1 || on_off < -1 )
		return -EINVAL;

	archos_wifi_power(on_off);

	return count;
}
static DEVICE_ATTR(wifi_reset, S_IRUGO|S_IWUSR, NULL, store_wifi_reset);

static ssize_t show_wifi_pa_type(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	switch (archos_wifi_pa_type) {
		case PA_TYPE_TQM67002_NEW_BOM:
			return snprintf(buf, PAGE_SIZE, "%s\n", "TQM67002_NEW_BOM");
		case PA_TYPE_TQM67002A:
			return snprintf(buf, PAGE_SIZE, "%s\n", "TQM67002A");
		case PA_TYPE_RF3482:
			return snprintf(buf, PAGE_SIZE, "%s\n", "RF3482");
		default:
			return snprintf(buf, PAGE_SIZE, "%s\n", "TQM67002");
	}
};
static DEVICE_ATTR(wifi_pa_type, S_IRUGO|S_IWUSR, show_wifi_pa_type, NULL);

struct wifi_platform_data archos_wifi_control = {
        .set_power	= archos_wifi_power,
	.set_reset	= archos_wifi_reset,
	.set_carddetect	= archos_wifi_set_carddetect,
};

#ifdef CONFIG_WIFI_CONTROL_FUNC
static struct resource archos_wifi_resources[] = {
	[0] = {
		.name		= "device_wifi_irq",
		.flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE,
	},
};

static struct platform_device archos_wifi_device = {
        .name           = "device_wifi",
        .id             = 1,
        .num_resources  = ARRAY_SIZE(archos_wifi_resources),
        .resource       = archos_wifi_resources,
        .dev            = {
                .platform_data = &archos_wifi_control,
        },
};
#endif
#ifdef CONFIG_BUILD_TI_ST
static int conn_gpios[] = { -1, -1, -1 };

static struct platform_device conn_device = {
	.name = "kim",		/* named after init manager for ST */
	.id = -1,
	.dev.platform_data = &conn_gpios,
};
#endif

static int __init archos_wifi_init(void)
{
	int ret;
	const struct archos_wifi_bt_config *wifi_bt_cfg;
	struct archos_gpio bt_nshutdown;
	
	wifi_bt_cfg = omap_get_config(ARCHOS_TAG_WIFI_BT, struct archos_wifi_bt_config);
	
	/* might be NULL */
	if (wifi_bt_cfg == NULL) {
		printk(KERN_DEBUG "archos_wifi_init: no board configuration found\n");
		return -ENODEV;
	}
	if ( hardware_rev >= wifi_bt_cfg->nrev ) {
		printk(KERN_DEBUG "archos_wifi_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, wifi_bt_cfg->nrev);
		return -ENODEV;
	}

	wifi_irq = wifi_bt_cfg->rev[hardware_rev].wifi_irq;
	wifi_pmena = wifi_bt_cfg->rev[hardware_rev].wifi_power;
	archos_wifi_pa_type = wifi_bt_cfg->rev[hardware_rev].wifi_pa_type;

	printk("%s: start\n", __func__);
	ret = gpio_request(GPIO_PIN(wifi_irq), "wifi_irq");
	if (ret < 0) {
		printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
			GPIO_PIN(wifi_irq));
		goto out;
	}
	ret = gpio_request(GPIO_PIN(wifi_pmena), "wifi_pmena");
	if (ret < 0) {
		printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
			GPIO_PIN(wifi_pmena));
		gpio_free(GPIO_PIN(wifi_irq));
		goto out;
	}
	omap_cfg_reg(GPIO_MUX(wifi_irq));
	omap_cfg_reg(GPIO_MUX(wifi_pmena));
	gpio_direction_input(GPIO_PIN(wifi_irq));
	gpio_direction_output(GPIO_PIN(wifi_pmena), 0);
	
	/* MUX setup for MMC3 */
	omap_cfg_reg(AF10_3430_MMC3_CLK);
	omap_cfg_reg(AE10_3430_MMC3_CMD);
	omap_cfg_reg(AE11_3430_MMC3_DAT0);
	omap_cfg_reg(AH9_3430_MMC3_DAT1);
	omap_cfg_reg(AF13_3430_MMC3_DAT2);
	omap_cfg_reg(AE13_3430_MMC3_DAT3);

#ifdef CONFIG_WIFI_CONTROL_FUNC
	archos_wifi_resources[0].start = gpio_to_irq(GPIO_PIN(wifi_irq));
	archos_wifi_resources[0].end = gpio_to_irq(GPIO_PIN(wifi_irq));
	ret = platform_device_register(&archos_wifi_device);
	if (ret < 0)
		goto out;
#endif
	
	/* also establish pin mux for bt kill switch */
	bt_nshutdown = wifi_bt_cfg->rev[hardware_rev].bt_power;
	if (GPIO_EXISTS(bt_nshutdown)) {
		omap_cfg_reg(GPIO_MUX(bt_nshutdown));
#ifdef CONFIG_BUILD_TI_ST
		conn_gpios[0] = GPIO_PIN(bt_nshutdown);
		ret = platform_device_register(&conn_device);
		if (ret < 0)
			goto out;
#endif
	}

	ret = device_create_file(&archos_wifi_device.dev, &dev_attr_wifi_reset);
	if (ret < 0)
		goto out;

	ret = device_create_file(&archos_wifi_device.dev, &dev_attr_wifi_pa_type);

out:
        return ret;
}

device_initcall(archos_wifi_init);
