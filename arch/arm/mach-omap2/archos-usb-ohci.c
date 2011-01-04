/*
 * linux/arch/arm/mach-omap2/usb-ohci.c
 *
 * This file will contain the board specific details for the
 * Synopsys OHCI host controller on OMAP3430
 *
 * Copyright (C) 2009 Texas Instruments
 * Author: Vikram Pandita <vikram.pandita@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <mach/mux.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/usb.h>
#include <mach/gpio.h>
#include <mach/archos-gpio.h>
#include <mach/board-archos.h>

/* OHCI platform specific data */
#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)
static struct resource ohci_resources[] = {
	[0] = {
		.start   = OMAP34XX_HSUSB_HOST_BASE + 0x400,
		.end     = OMAP34XX_HSUSB_HOST_BASE + 0x400 + SZ_1K - 1,
		.flags   = IORESOURCE_MEM,
	},
	[1] = {         /* general IRQ */
		.start   = 76,
		.flags   = IORESOURCE_IRQ,
	}
};

/* The dmamask must be set for OHCI to work */
static u64 ohci_dmamask = ~(u32)0;

static void usb_release(struct device *dev)
{
	/* normally not freed */
}

static struct platform_device ohci_device = {
	.name           = "ohci-omap",
	.id             = 0,
	.dev = {
		.release		= usb_release,
		.dma_mask               = &ohci_dmamask,
		.coherent_dma_mask      = 0xffffffff,
		.platform_data          = NULL,
	},
	.num_resources  = ARRAY_SIZE(ohci_resources),
	.resource       = ohci_resources,
};
#endif /* OHCI specific data */


static struct archos_gpio gio_ohci_enable = UNUSED_GPIO;
static struct archos_gpio gio_ohci_suspend = UNUSED_GPIO;

static ssize_t show_fsusb(struct device *dev, 
		struct device_attribute *attr, char *buf);
static ssize_t store_fsusb(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count);

int archos_enable_ohci( int enable )
{
printk(" enable ohci interface %d \n", enable);
	if ( GPIO_EXISTS( gio_ohci_enable ) ) {
		if ( enable )
			gpio_set_value( GPIO_PIN( gio_ohci_enable ), 1);
		else
			gpio_set_value( GPIO_PIN( gio_ohci_enable ), 0);
	}

	return 0;
}
EXPORT_SYMBOL(archos_enable_ohci);

static DEVICE_ATTR(fsusb_enable, S_IWUSR|S_IRUGO, show_fsusb, store_fsusb);
static DEVICE_ATTR(fsusb_suspend, S_IWUSR|S_IRUGO, show_fsusb, store_fsusb);

static ssize_t show_fsusb(struct device *dev, 
		struct device_attribute *attr, char *buf) 
{
	if (attr == &dev_attr_fsusb_enable) {
		return snprintf(buf, PAGE_SIZE, "%i\n", 
				gpio_get_value(GPIO_PIN(gio_ohci_enable))); 
	}
	if (attr == &dev_attr_fsusb_suspend) {
		return snprintf(buf, PAGE_SIZE, "%i\n", 
				gpio_get_value(GPIO_PIN(gio_ohci_suspend)));
	}
	
	return -EINVAL;
}

static ssize_t store_fsusb(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count) 
{
	int on_off = simple_strtoul(buf, NULL, 10);
	
	if (attr == &dev_attr_fsusb_enable) {
		gpio_set_value(GPIO_PIN(gio_ohci_enable), on_off);
	} else
	if (attr == &dev_attr_fsusb_suspend) {
		gpio_set_value(GPIO_PIN(gio_ohci_suspend), on_off);
	}
	
	return count;
}

void __init archos_usb_ohci_init(void)
{
#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)

	const struct archos_fsusb_config *usb_cfg;
	
	usb_cfg = omap_get_config( ARCHOS_TAG_FSUSB, struct archos_fsusb_config );
	if (usb_cfg == NULL) {
		printk(KERN_DEBUG "%s: no board configuration found\n", __FUNCTION__);
		return;
	}
	if ( hardware_rev >= usb_cfg->nrev ) {
		printk(KERN_DEBUG "%s: hardware_rev (%i) >= nrev (%i)\n",
			__FUNCTION__, hardware_rev, usb_cfg->nrev);
		return;
	}

	printk("archos_usb_ohci_init\n");

	if ( GPIO_EXISTS(usb_cfg->rev[hardware_rev].enable_usb_ohci)) {

		gio_ohci_enable = usb_cfg->rev[hardware_rev].enable_usb_ohci;

		archos_gpio_init_output( &gio_ohci_enable, "fsusb_enable" );

		archos_enable_ohci( 0 );

		gio_ohci_suspend = usb_cfg->rev[hardware_rev].suspend;
		archos_gpio_init_output( &gio_ohci_suspend, "fsusb_suspend" );
		gpio_set_value( GPIO_PIN(gio_ohci_suspend), 0);

		archos_enable_ohci( 1 );

	}

	omap_cfg_reg(AG8_3430_USB2FS_RCV);
	omap_cfg_reg(AH8_3430_USB2FS_VM);
	omap_cfg_reg(AB2_3430_USB2FS_VP);
	omap_cfg_reg(V3_3430_USB2FS_EN);

	if (platform_device_register(&ohci_device) < 0) {
		printk(KERN_ERR "Unable to register FS-USB (OHCI) device\n");
		return;
	}
	
	device_create_file(&ohci_device.dev, &dev_attr_fsusb_enable);
	device_create_file(&ohci_device.dev, &dev_attr_fsusb_suspend);
#endif
}
