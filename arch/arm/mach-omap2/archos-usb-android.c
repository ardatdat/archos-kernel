/* File: linux/arch/arm/mach-omap2/archos-usb-android.c
 *
 *  This file contains Archos platform-specific data for the Android USB
 * gadget driver.
 *
 * Copyright © 2009 Chidambar Zinnoury - Archos S.A.
 */

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/usb/android.h>
#include <linux/usb/ch9.h>

#include <mach/board-archos.h>
#include <mach/archos-dieid.h>

#if defined(CONFIG_USB_ANDROID) || defined(CONFIG_USB_ANDROID_MODULE)
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= 0x0e79,
	.product_id		= 0x1410,
	.adb_product_id		= 0x1411,
	.version		= 0x0100,
	.product_name		= "unknown",
	.manufacturer_name	= "Archos",
	.serial_number		= "A5X-00000000-00000000-00000000-00000000",
	.nluns			= 2,
};

static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.vendor			= "ARCHOS",
	.product		= "unknown",
	.release		= 0,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};
static struct platform_device android_usb_mass_storage= {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &usb_mass_storage_pdata,
	},
};
static char* A28 = "A28";
static char* A32 = "A32";
static char* A32SD = "A32SD";
static char* A35 = "A35";
static char* A35DM = "A35DM";
static char* A43 = "A43";
static char* A70S = "A70S";
static char* A70H = "A70H";
static char* A10 = "A101it";
#endif

static int __init usb_android_init(void)
{
#if defined(CONFIG_USB_ANDROID) || defined(CONFIG_USB_ANDROID_MODULE)

	u32 prod_id[4];

	get_dieid(prod_id);

	if (machine_is_archos_a32()) {
		sprintf(android_usb_pdata.serial_number, "A32-%08X-%08X-%08X-%08X", prod_id[0],prod_id[1],prod_id[2],prod_id[3]);
		android_usb_pdata.product_id = 0x1416;
		android_usb_pdata.product_name = A32;
		android_usb_pdata.nluns = 1;
		usb_mass_storage_pdata.product = A32;
	} else if ( machine_is_archos_a28() ) {
		sprintf(android_usb_pdata.serial_number, "A28-%08X-%08X-%08X-%08X", prod_id[0],prod_id[1],prod_id[2],prod_id[3]);
		android_usb_pdata.product_id = 0x1415;
		android_usb_pdata.product_name = A28;
		android_usb_pdata.nluns = 1;
		usb_mass_storage_pdata.product = A28;
	} else if ( machine_is_archos_a35() ) {
		sprintf(android_usb_pdata.serial_number, "A35-%08X-%08X-%08X-%08X", prod_id[0],prod_id[1],prod_id[2],prod_id[3]);
		android_usb_pdata.product_id = 0x1424;
		android_usb_pdata.product_name = A35;
		android_usb_pdata.nluns = 2;
		usb_mass_storage_pdata.product = A35;
	} else if ( machine_is_archos_a35dm() ) {
		sprintf(android_usb_pdata.serial_number, "A35DM-%08X-%08X-%08X-%08X", prod_id[0],prod_id[1],prod_id[2],prod_id[3]);
		android_usb_pdata.product_id = 0x1425;
		android_usb_pdata.product_name = A35DM;
		android_usb_pdata.nluns = 2;
		usb_mass_storage_pdata.product = A35DM;
	} else if ( machine_is_archos_a43() ) {
		sprintf(android_usb_pdata.serial_number, "A43-%08X-%08X-%08X-%08X", prod_id[0],prod_id[1],prod_id[2],prod_id[3]);
		android_usb_pdata.product_id = 0x1417;
		android_usb_pdata.product_name = A43;
		android_usb_pdata.nluns = 2;
		usb_mass_storage_pdata.product = A43;
	} else if ( machine_is_archos_a101it() ) {
		sprintf(android_usb_pdata.serial_number, "A10-%08X-%08X-%08X-%08X", prod_id[0],prod_id[1],prod_id[2],prod_id[3]);
		android_usb_pdata.product_id = 0x1419;
		android_usb_pdata.product_name = A10;
		android_usb_pdata.nluns = 2;
		usb_mass_storage_pdata.product = A10;
	} else if ( machine_is_archos_a70s() ||  machine_is_archos_a70h() ) {
		sprintf(android_usb_pdata.serial_number, "A70-%08X-%08X-%08X-%08X", prod_id[0],prod_id[1],prod_id[2],prod_id[3]);
		android_usb_pdata.product_id = 0x1420;
		if( machine_is_archos_a70s()){
			android_usb_pdata.product_id = 0x1420;
			android_usb_pdata.product_name = A70S;
			android_usb_pdata.nluns = 2;
			usb_mass_storage_pdata.product = A70S;
		}else{
			android_usb_pdata.product_id = 0x1422;
			android_usb_pdata.product_name = A70H;
			android_usb_pdata.nluns = 1;
			usb_mass_storage_pdata.product = A70H;
		}
	} else if ( machine_is_archos_a32sd() ) {
		sprintf(android_usb_pdata.serial_number, "A32-%08X-%08X-%08X-%08X", prod_id[0],prod_id[1],prod_id[2],prod_id[3]);
		android_usb_pdata.product_id = 0x1423;
		android_usb_pdata.product_name = A32SD;
		android_usb_pdata.nluns = 2;
		usb_mass_storage_pdata.product = A32SD;
	} else {
		sprintf(android_usb_pdata.serial_number, "A5x-%08X-%08X-%08X-%08X", prod_id[0],prod_id[1],prod_id[2],prod_id[3]);
	}
	
	if ( machine_charges_from_USB() ) {
		android_usb_pdata.bmAttributes = USB_CONFIG_ATT_ONE;
		android_usb_pdata.bMaxPower = 250; /* 500mA */
	} else {
		android_usb_pdata.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
		android_usb_pdata.bMaxPower	= CONFIG_USB_GADGET_VBUS_DRAW / 2;
	}
	
	printk(KERN_ERR "registering Android USB device (%s)\n", android_usb_pdata.serial_number);

	if (platform_device_register(&android_usb_device) < 0) {
		printk(KERN_ERR "Unable to register Android USB device\n");
		return -ENODEV;
	}
	printk(KERN_ERR "registering Android USB mass storage device  (%s)\n", android_usb_pdata.serial_number);
	if (platform_device_register(&android_usb_mass_storage) < 0) {
		printk(KERN_ERR "Unable to register Android USB mass storage device\n");
		return -ENODEV;
	}
#endif

	return 0;
}

late_initcall(usb_android_init);
