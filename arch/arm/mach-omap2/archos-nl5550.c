/*
 *    archos-nl5550.c : 16/02/2010
 *    g.revaillot, revaillot@archos.com
 */

#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>

#include <mach/archos-gpio.h>
#include <mach/board-archos.h>

static struct nl5550_struct {
	struct archos_gps_conf gps_conf;
	struct platform_device *pdev;
	struct work_struct work;
	struct wake_lock wake_lock;
} nl5550_obj;

/*
 * NL5550 device and attributes
 */
static ssize_t show_nl5550_enable(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	struct nl5550_struct *nl5550 = dev->platform_data;
	struct archos_gps_conf *conf = &nl5550->gps_conf;
	
	return snprintf(buf, PAGE_SIZE, "%d\n", 
			gpio_get_value(GPIO_PIN(conf->gps_enable)));
};

static ssize_t store_nl5550_enable(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count)
{
	struct nl5550_struct *nl5550 = dev->platform_data;
	struct archos_gps_conf *conf = &nl5550->gps_conf;
	
	int on_off = simple_strtol(buf, NULL, 10);
	
	if( on_off > 1 || on_off < -1 )
		return -EINVAL;

	gpio_set_value(GPIO_PIN(conf->gps_enable), on_off);	
	return count;
}
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR, show_nl5550_enable, store_nl5550_enable);

static ssize_t show_nl5550_intr(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	struct nl5550_struct *nl5550 = dev->platform_data;
	struct archos_gps_conf *conf = &nl5550->gps_conf;
	
	return snprintf(buf, PAGE_SIZE, "%d\n", 
			gpio_get_value(GPIO_PIN(conf->gps_int)));
};
static DEVICE_ATTR(intr, S_IRUGO, show_nl5550_intr, NULL);

static struct platform_device nl5550_device = {
	.name = "nl5550",
	.id = -1,
	.dev.platform_data = &nl5550_obj,
};

/*
 * NL5550 device driver
 */
static void nl5550_irq_worker(struct work_struct *work)
{
	struct nl5550_struct *nl5550 = container_of(work, struct nl5550_struct, work);
	struct platform_device *pdev = nl5550->pdev;

	static const char *envp[] = {
		"INTERRUPT",
		NULL,
	};
	dev_dbg(&pdev->dev, "nl5550_irq_worker: sending uevent\n");
	kobject_uevent_env(&pdev->dev.kobj, KOBJ_CHANGE, (char**)envp);
}

static irqreturn_t nl5550_isr(int irq, void *dev_id)
{
	struct nl5550_struct *nl5550 = dev_id;
	struct platform_device *pdev = nl5550->pdev;
	
	dev_dbg(&pdev->dev, "nl5550_isr: irq %i called\n", irq);
	schedule_work(&nl5550->work);
	return IRQ_HANDLED;
}

static int nl5550_probe(struct platform_device *pdev)
{
	int ret;
	struct nl5550_struct *nl5550 = pdev->dev.platform_data;
	struct archos_gps_conf *conf = &nl5550->gps_conf;
	
	GPIO_INIT_OUTPUT(conf->gps_enable);
	GPIO_INIT_INPUT(conf->gps_int);
	
	omap_cfg_reg(AD25_34XX_UART2_RX);
	omap_cfg_reg(AA25_34XX_UART2_TX);

	INIT_WORK(&nl5550->work, nl5550_irq_worker);

	ret = request_irq(OMAP_GPIO_IRQ(GPIO_PIN(conf->gps_int)), 
			nl5550_isr, IRQF_TRIGGER_RISING, "nl5550", nl5550);
	if (ret < 0) {
		dev_err(&pdev->dev, "nl5550_probe: cannot register irq %d\n", 
				OMAP_GPIO_IRQ(GPIO_PIN(conf->gps_int)));
		return ret;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_enable);
	if (ret < 0)
		dev_dbg(&pdev->dev, "cannot add enable attr\n");
	ret = device_create_file(&pdev->dev, &dev_attr_intr);
	if (ret < 0)
		dev_dbg(&pdev->dev, "cannot add intr attr\n");
	
	wake_lock_init(&nl5550->wake_lock, WAKE_LOCK_SUSPEND, "nl5550");

	nl5550->pdev = pdev;
	return 0;
}

static struct platform_driver nl5550_driver = {
	.probe = nl5550_probe,
#ifdef CONFIG_PM
	//.suspend = nl5550_suspend,
	//.resume = nl5550_resume,
#endif
	.driver = {
		.owner = THIS_MODULE,
		.name  = "nl5550",
	},
};

static int __init archos_nl5550_init(void)
{
	int ret;
	
	const struct archos_gps_config *gps_config = 
			omap_get_config(ARCHOS_TAG_GPS, struct archos_gps_config);
	
	pr_debug("archos_nl5550_init\n");
	
	if (gps_config == NULL) {
		pr_debug("archos_nl5550_init: no gps configuration\n");
		return -ENODEV;
	}
	
	if (hardware_rev >= gps_config->nrev) {
		pr_err("archos_nl5550_init: no configuration for hardware_rev %d\n", 
				hardware_rev);
		return -ENODEV;
	}
		
	nl5550_obj.gps_conf = gps_config->rev[hardware_rev];
	
	ret = platform_device_register(&nl5550_device);
	if (ret < 0) {
		pr_err("archos_nl5550_init: failed to register platform device\n");
		return -ENODEV;
	}
	return platform_driver_register(&nl5550_driver);
}

device_initcall(archos_nl5550_init);

