/*
 *    archos-usb-ehci.c : 14/04/2010
 *    g.revaillot, revaillot@archos.com
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/usb/musb.h>
#include <linux/err.h>
#include <linux/clk.h>

#include <asm/mach-types.h>

#include <mach/usb.h>
#include <mach/gpio.h>
#include <mach/archos-gpio.h>
#include <mach/mux.h>
#include <mach/board-archos.h>

#if     defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_EHCI_HCD_MODULE)
static struct resource ehci_resources[] = {
	[0] = {
		.start   = OMAP34XX_HSUSB_HOST_BASE + 0x800,
		.end     = OMAP34XX_HSUSB_HOST_BASE + 0x800 + SZ_1K - 1,
		.flags   = IORESOURCE_MEM,
	},
	[1] = {         /* general IRQ */
		.start   = INT_34XX_EHCI_IRQ,
		.flags   = IORESOURCE_IRQ,
	}
};

static u64 ehci_dmamask = ~(u32)0;
static struct platform_device ehci_device = {
	.name           = "ehci-omap",
	.id             = 0,
	.dev = {
		.dma_mask               = &ehci_dmamask,
		.coherent_dma_mask      = 0xffffffff,
		.platform_data          = NULL,
	},
	.num_resources  = ARRAY_SIZE(ehci_resources),
	.resource       = ehci_resources,
};
#endif

static int ehci_phy_enable;

static struct archos_gpio gio_ehci_enable = UNUSED_GPIO;
static struct archos_gpio gio_hub_enable = UNUSED_GPIO;
static struct archos_gpio gio_5v_enable = UNUSED_GPIO;

static ssize_t show_hsusb(struct device *dev, 
		struct device_attribute *attr, char *buf);
static ssize_t store_hsusb(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count);

static void setup_ehci_io_mux(void)
{
	// ehci on port 2 for gen8
	omap_cfg_reg(AE7_3430_USB2HS_PHY_CLK);
	omap_cfg_reg(AF7_3430_USB2HS_PHY_STP);
	omap_cfg_reg(AG7_3430_USB2HS_PHY_DIR);
	omap_cfg_reg(AH7_3430_USB2HS_PHY_NXT);
	omap_cfg_reg(AG8_3430_USB2HS_PHY_DATA0);
	omap_cfg_reg(AH8_3430_USB2HS_PHY_DATA1);
	omap_cfg_reg(AB2_3430_USB2HS_PHY_DATA2);
	omap_cfg_reg(V3_3430_USB2HS_PHY_DATA3);
	omap_cfg_reg(Y2_3430_USB2HS_PHY_DATA4);
	omap_cfg_reg(Y3_3430_USB2HS_PHY_DATA5);
	omap_cfg_reg(Y4_3430_USB2HS_PHY_DATA6);
	omap_cfg_reg(AA3_3430_USB2HS_PHY_DATA7);

	return;
}

int archos_enable_ehci( int enable )
{
	if (enable) {
		if ( GPIO_EXISTS( gio_5v_enable ) )
			gpio_set_value( GPIO_PIN( gio_5v_enable ), 1);
		
		if ( GPIO_EXISTS( gio_ehci_enable ) ) 
			gpio_set_value( GPIO_PIN( gio_ehci_enable ), 1);
	} else {
		if ( GPIO_EXISTS( gio_ehci_enable ) ) 
			gpio_set_value( GPIO_PIN( gio_ehci_enable ), 0);

		if ( GPIO_EXISTS( gio_5v_enable ) )
			gpio_set_value( GPIO_PIN( gio_5v_enable ), 0);
	}
	
	if ( GPIO_EXISTS( gio_hub_enable ) ) {
		msleep(300);	// fix me

		if ( enable )
			gpio_set_value( GPIO_PIN( gio_hub_enable ), 1);
		else
			gpio_set_value( GPIO_PIN( gio_hub_enable ), 0);
	}

	ehci_phy_enable = enable;
	
	return 0;
}
EXPORT_SYMBOL(archos_enable_ehci);

static DEVICE_ATTR(ehci_enable, S_IWUSR|S_IRUGO, show_hsusb, store_hsusb);
static DEVICE_ATTR(hub_enable, S_IWUSR|S_IRUGO, show_hsusb, store_hsusb);

static ssize_t show_hsusb(struct device *dev, 
		struct device_attribute *attr, char *buf) 
{
	if (attr == &dev_attr_ehci_enable) {
		return snprintf(buf, PAGE_SIZE, "%i\n", 
				gpio_get_value(GPIO_PIN(gio_ehci_enable))); 
	}
	if (attr == &dev_attr_hub_enable) {
		return snprintf(buf, PAGE_SIZE, "%i\n", 
				gpio_get_value(GPIO_PIN(gio_hub_enable)));
	}
	
	return -EINVAL;
}

static ssize_t store_hsusb(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count) 
{
	int on_off = simple_strtoul(buf, NULL, 10);
	
	if (attr == &dev_attr_ehci_enable) {
		gpio_set_value(GPIO_PIN(gio_ehci_enable), on_off);
	} else
	if (attr == &dev_attr_hub_enable) {
		gpio_set_value(GPIO_PIN(gio_hub_enable), on_off);
	}
	
	return count;
}


static int archos_ehci_probe(struct platform_device *dev)
{
	return 0;
}

static int archos_ehci_suspend(struct platform_device *dev, pm_message_t pm)
{
	printk("%s\n", __FUNCTION__);
	if ( GPIO_EXISTS( gio_ehci_enable ) )
		gpio_set_value( GPIO_PIN(gio_ehci_enable), 0);
	if ( GPIO_EXISTS( gio_5v_enable ) )
		gpio_set_value( GPIO_PIN(gio_5v_enable), 0);
	return 0;
}

static int archos_ehci_resume(struct platform_device *dev)
{
	printk("%s\n", __FUNCTION__);
	if ( GPIO_EXISTS( gio_5v_enable ) )
		gpio_set_value( GPIO_PIN(gio_5v_enable), ehci_phy_enable);
	if ( GPIO_EXISTS( gio_ehci_enable ) )
		gpio_set_value( GPIO_PIN( gio_ehci_enable ), ehci_phy_enable);
	return 0;
}

static struct platform_driver archos_ehci_driver = {
	.driver.name = "archos_ehci",
	.probe = archos_ehci_probe,
#ifdef CONFIG_PM
	.suspend_late = archos_ehci_suspend,
	.resume_early = archos_ehci_resume,
#endif
};

static struct platform_device archos_ehci_device = {
	.name = "archos_ehci",
	.id = -1,
};

void __init archos_usb_ehci_init(void)
{
	const struct archos_usb_config *usb_cfg;
	usb_cfg = omap_get_config( ARCHOS_TAG_USB, struct archos_usb_config );
	if (usb_cfg == NULL) {
		printk(KERN_DEBUG "%s: no board configuration found\n", __FUNCTION__);
		return;
	}
	if ( hardware_rev >= usb_cfg->nrev ) {
		printk(KERN_DEBUG "%s: hardware_rev (%i) >= nrev (%i)\n",
			__FUNCTION__, hardware_rev, usb_cfg->nrev);
		return;
	}

	if ( GPIO_PIN (usb_cfg->rev[hardware_rev].enable_usb_hub) != 0 ) {
		gio_hub_enable = usb_cfg->rev[hardware_rev].enable_usb_hub;
		archos_gpio_init_output(&gio_hub_enable, "hub enable");
	}

	if ( GPIO_PIN (usb_cfg->rev[hardware_rev].enable_5v) != 0 ) {
		gio_5v_enable = usb_cfg->rev[hardware_rev].enable_5v;
		archos_gpio_init_output(&gio_5v_enable, "5v enable");
	}

	if ( GPIO_PIN (usb_cfg->rev[hardware_rev].enable_usb_ehci) != 0 ) {

		gio_ehci_enable = usb_cfg->rev[hardware_rev].enable_usb_ehci;

		archos_gpio_init_output(&gio_ehci_enable, "ehci enable");

		archos_enable_ehci( 0 );

		setup_ehci_io_mux();
	}

#if     defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_EHCI_HCD_MODULE)
	if (platform_device_register(&ehci_device) < 0) {
		printk(KERN_ERR "Unable to register HS-USB (EHCI) device\n");
		return;
	}
#endif

	if (platform_device_register(&archos_ehci_device) < 0)
		printk(KERN_ERR "Unable to register Archos EHCI device\n");

	device_create_file(&archos_ehci_device.dev, &dev_attr_ehci_enable);
	device_create_file(&archos_ehci_device.dev, &dev_attr_hub_enable);

	platform_driver_register(&archos_ehci_driver);

}
