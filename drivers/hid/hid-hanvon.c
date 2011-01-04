/*
 *  HID driver for the usb multitouch panel Hanvon 10 and clones, from COASIA.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/hid.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/usb.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>

#include "usbhid/usbhid.h"
#include "hid-ids.h"
#include "../drivers/usb/core/usb.h"
#include "../drivers/usb/core/hcd.h"

#ifdef CONFIG_MACH_ARCHOS
#include <mach/board-archos.h>
#endif

MODULE_VERSION("1.00");
MODULE_AUTHOR("Archos SA");
MODULE_DESCRIPTION("Hanvon dual-touch panel");
MODULE_LICENSE("GPL");

#define MAX_NUM_POINTERS	2

#define HDBG		if (0) printk
#define STDBG		if (0) printk
#define EVDBG		if (0) printk

#define MS_TO_NS(x)	(x * 1E6L)

enum {
	TS_WAITING = 0,
	TS_READING,
	TS_RELEASING,
	TS_DEBOUNCING,
	TS_RECOVERING,
};

struct pointer_data {
	__u8 id;
	__u16 x;
	__u16 y;
	__u16 z;
};

struct pointer_info {
	// pointer state
	int state;

	// pointer data
	struct pointer_data data;
	
	// pointer timer (devounce & release)
	struct hrtimer timer;

	// nb sample
	int sample_count;
};

struct hanvon_data {
	struct input_dev *dev;
	struct hid_device *hdev;

	struct pointer_info pointer[MAX_NUM_POINTERS];

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend es;
	struct delayed_work work;
	struct wake_lock wakelock;
#endif
};

static void (*usb_tsp_enable)(int on_off);
static unsigned long usb_tsp_flags;

static int x_scale, x_offset;
static int y_scale, y_offset;

enum hrtimer_restart pointer_timer_callback( struct hrtimer *timer_handle )
{
	struct pointer_info * p = container_of(timer_handle, struct pointer_info, timer);
	struct hanvon_data * td = container_of(p, struct hanvon_data, pointer[p->data.id]);

	switch (p->state) {
		case TS_READING:
			// thats bad, tsp stopped reporting positions.

			// issue a release.
			input_event(td->dev, EV_ABS, ABS_MT_TRACKING_ID, p->data.id);
			input_event(td->dev, EV_ABS, ABS_MT_POSITION_X, p->data.x);
			input_event(td->dev, EV_ABS, ABS_MT_POSITION_Y, p->data.y);
			input_event(td->dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);

			input_mt_sync(td->dev);
			input_event(td->dev, EV_SYN, 0, 0);

#ifdef CONFIG_HID_HANVON_10_MONO_TSP_EMULATION
			input_event(td->dev, EV_KEY, BTN_TOUCH, 0);
			input_sync(td->dev);
#endif

			// and go to recovery until proper release.
			p->state = TS_RECOVERING;
	
			break;
		case TS_DEBOUNCING:
			// debouncing iz over.
			p->state = TS_WAITING;
			break;

		default:
			break;
	}

	return HRTIMER_NORESTART;
}

static int hanvon_input_mapping(struct hid_device *hdev, struct hid_input *hi,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	switch (usage->hid & HID_USAGE_PAGE) {

		case HID_UP_GENDESK:
			switch (usage->hid) {
				case HID_GD_X:
					hid_map_usage(hi, usage, bit, max,
							EV_ABS, ABS_MT_POSITION_X);
					/* touchscreen emulation */
					input_set_abs_params(hi->input, ABS_X,
							field->logical_minimum,
							field->logical_maximum, 0, 0);
					return 1;
				case HID_GD_Y:
					hid_map_usage(hi, usage, bit, max,
							EV_ABS, ABS_MT_POSITION_Y);
					/* touchscreen emulation */
					input_set_abs_params(hi->input, ABS_Y,
							field->logical_minimum,
							field->logical_maximum, 0, 0);
					return 1;
			}
			return 0;

		case HID_UP_DIGITIZER:
			switch (usage->hid) {
				case HID_DG_CONFIDENCE:
				case HID_DG_INPUTMODE:
				case HID_DG_DEVICEINDEX:
				case HID_DG_CONTACTCOUNT:
				case HID_DG_CONTACTMAX:
				case HID_DG_TIPPRESSURE:
				case HID_DG_WIDTH:
				case HID_DG_HEIGHT:
					return -1;
				case HID_DG_INRANGE:
					/* touchscreen emulation */
					hid_map_usage(hi, usage, bit, max,
							EV_KEY, BTN_TOUCH);
					return 1;

				case HID_DG_TIPSWITCH:
					hid_map_usage(hi, usage, bit, max,
							EV_ABS, ABS_MT_TOUCH_MAJOR);
					return 1;

				case HID_DG_CONTACTID:
					hid_map_usage(hi, usage, bit, max,
							EV_ABS, ABS_MT_TRACKING_ID);
					return 1;

			}
			return 0;

		case 0xff000000:
			/* ignore HID features */
			return -1;
	}

	return 0;
}

static int hanvon_input_mapped(struct hid_device *hdev, struct hid_input *hi,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	if (usage->type == EV_KEY || usage->type == EV_ABS)
		clear_bit(usage->code, *bit);

	return 0;
}
static unsigned long debounce_delay(int sample)
{
	if ((sample > 20) && (usb_tsp_flags & USB_TSP_FLAGS_SLOW_EVENT_RATE)) {
		return MS_TO_NS(500);
	} else if (sample > 100) {
		return MS_TO_NS(500);
	}

	return MS_TO_NS(100);
}

/*
 * this function is called when a whole finger has been parsed,
 * so that it can decide what to send to the input layer.
 */
static void hanvon_filter_event(struct hanvon_data *td, struct pointer_data * read_buffer)
{
	struct pointer_info * p;
	struct input_dev *input = td->dev;
	int id = read_buffer->id;

	p = &td->pointer[id];

	// push new pointer buffer at right place.
	memcpy(&p->data, read_buffer, sizeof(struct pointer_data));


	switch (p->state) {
		case TS_WAITING:
			// we were waiting : ts press detected.
			p->sample_count = 0;

#ifdef CONFIG_HID_HANVON_10_MONO_TSP_EMULATION
			// issue button press
			input_event(input, EV_KEY, BTN_TOUCH, 1);
#endif
			// next time, we'll only rearm timer and 
			// issue position update if ts emulation.
			p->state = TS_READING;

			// start "reading" now.

		case TS_READING:
			p->sample_count++;

			// if ts still pressed
			if (p->data.z) {
				// issue mt position
				input_event(input, EV_ABS, ABS_MT_TRACKING_ID, p->data.id);
				input_event(input, EV_ABS, ABS_MT_POSITION_X, p->data.x);
				input_event(input, EV_ABS, ABS_MT_POSITION_Y, p->data.y);
				input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, p->data.z);

				// and mt sync 
				input_mt_sync(input);

#ifdef CONFIG_HID_HANVON_10_MONO_TSP_EMULATION
				// issue position
				input_event(input, EV_ABS, ABS_X, p->data.x);
				input_event(input, EV_ABS, ABS_Y, p->data.y);
				input_event(input, EV_ABS, ABS_PRESSURE, p->data.z);

				// and sync
				input_sync(input);
#endif
				// and (re)arm release timer
				if (usb_tsp_flags & USB_TSP_FLAGS_RELEASE_WA)
					hrtimer_start( &p->timer, ktime_set( 0, MS_TO_NS(1000)), HRTIMER_MODE_REL );

				// and stop here.
				break;
			}

			p->state = TS_RELEASING;

			// release for good.

		case TS_RELEASING:
			// cancel pending release timer
			if (usb_tsp_flags & USB_TSP_FLAGS_RELEASE_WA)
				hrtimer_try_to_cancel(&p->timer);

			// start release debouncer
	
			// issue release
			input_event(input, EV_ABS, ABS_MT_TRACKING_ID, p->data.id);
			input_event(input, EV_ABS, ABS_MT_POSITION_X, p->data.x);
			input_event(input, EV_ABS, ABS_MT_POSITION_Y, p->data.y);
			input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, p->data.z);
			input_mt_sync(input);

#ifdef CONFIG_HID_HANVON_10_MONO_TSP_EMULATION
			// issue button release
			input_event(input, EV_KEY, BTN_TOUCH, 0);
			input_sync(input);
#endif
			// start debouncer.
			p->state = TS_DEBOUNCING;
			hrtimer_start( &p->timer, ktime_set( 0, debounce_delay(p->sample_count) ), HRTIMER_MODE_REL );

			break;

		case TS_DEBOUNCING:
			STDBG("%s : killing rebounce %d\n", __FUNCTION__, id);
			break;

		case TS_RECOVERING:
			// mask stupid and late samples until ts understood
			// finger has been released - handle better buggy TS
			// no incidence on 7 and 10" DMT to MP TS
			if (p->data.z)
				p->state = TS_WAITING;

			break;
		
		default:
			// Illegal state. Reset state machine
			p->state = TS_WAITING;
			break;
	}

	STDBG("ptr 0 : st : %d - z : %d - timer state : %lu / ", td->pointer[0].state, td->pointer[0].data.z, td->pointer[0].timer.state);
	STDBG("ptr 1 : st : %d - z : %d - timer state : %lu\n", td->pointer[1].state, td->pointer[1].data.z, td->pointer[1].timer.state);
}

static int hanvon_rescale(int abs_value, int abs_min, int abs_max, int abs_scale, int abs_offset)
{
	int value;

	// scale and center
	value = abs_value + ((abs_value - abs_max / 2) * abs_scale) / 1000 + abs_offset;

	// normalize
	if (value < abs_min)
		value = abs_min;

	if (value > abs_max)
		value = abs_max;

	return value;
}

static int hanvon_event(struct hid_device *hid, struct hid_field *field,
				struct hid_usage *usage, __s32 value)
{
	static struct pointer_data read_buffer;
	struct hanvon_data *td = hid_get_drvdata(hid);
	static int valid;

	if (hid->claimed & HID_CLAIMED_INPUT) {
		td->dev = field->hidinput->input;

		switch (usage->hid) {
			case HID_DG_TIPSWITCH:
				EVDBG("\n> tipswich : %d - ", value);
				read_buffer.z = value;
				break;

			case HID_DG_INRANGE:
				EVDBG("inrange : %d - ", value);
				break;

			case HID_DG_CONFIDENCE:
				EVDBG("confidence : %d -", value);
				valid = !!value;
				break;

			case HID_DG_CONTACTID:
				EVDBG("id : %d - ", value);
				read_buffer.id = value;
				break;

			case HID_GD_X:
				EVDBG("x : %d - ", value);
				read_buffer.x = hanvon_rescale(value, td->dev->absmin[ABS_X], td->dev->absmax[ABS_X], x_scale, x_offset);
				break;

			case HID_GD_Y:
				EVDBG("y : %d - ", value);
				read_buffer.y = hanvon_rescale(value, td->dev->absmin[ABS_Y], td->dev->absmax[ABS_Y], y_scale, y_offset);

				// last interesting data, push pointer to filter, if valid.
				if (valid)
					hanvon_filter_event(td, &read_buffer);

				break;

			case HID_DG_CONTACTCOUNT:
				EVDBG("count : %d\n", value);
				// not used : useless (1 OR 2)
				break;

			default:
				/* fallback to the generic hidinput handling */
				EVDBG("default : %d\n", value);
				return 0;
		}
	}

	/* we have handled the hidinput part, now remains hiddev */
	if (hid->claimed & HID_CLAIMED_HIDDEV && hid->hiddev_hid_event)
		hid->hiddev_hid_event(hid, field, usage, value);

	return 1;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void hanvon_early_suspend_work(struct work_struct *work)
{
	struct hanvon_data *td = container_of(work, struct hanvon_data, work.work);
	struct hid_device *hdev = td->hdev;
	struct usbhid_device *usbhid = hdev->driver_data;
	struct usb_device *udev = interface_to_usbdev(usbhid->intf);
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);
	
	if (!HC_IS_SUSPENDED(hcd->state)) {
		schedule_delayed_work(&td->work, msecs_to_jiffies(500));
		return;
	}
		
	printk(KERN_DEBUG "hanvon_early_suspend_work\n");
	if (usb_tsp_enable)
		usb_tsp_enable(0);

	printk(KERN_DEBUG "hanvon_early_suspend_work: releasing wake lock\n");
	wake_unlock(&td->wakelock);
}

static void hanvon_early_suspend(struct early_suspend *h)
{
	struct hanvon_data *td = container_of(h, struct hanvon_data, es);
	struct hid_device *hdev = td->hdev;
	struct usbhid_device *usbhid = hdev->driver_data;
	struct usb_device *udev = interface_to_usbdev(usbhid->intf);
	
	usb_external_suspend_device(udev, PMSG_SUSPEND);
	
	if(usb_tsp_flags & USB_TSP_FLAGS_EARLY_POWER_OFF) {
	
		if (delayed_work_pending(&td->work)) {
			cancel_delayed_work(&td->work);		
		}

		printk(KERN_DEBUG "hanvon_early_suspend: taking wake lock\n");
		wake_lock(&td->wakelock);
		
		INIT_DELAYED_WORK(&td->work, hanvon_early_suspend_work);
	
		schedule_delayed_work(&td->work, msecs_to_jiffies(500));
	}
}

static void hanvon_early_resume_work(struct work_struct *work)
{
	struct hanvon_data *td = container_of(work, struct hanvon_data, work.work);
	struct hid_device *hdev = td->hdev;
	struct usbhid_device *usbhid = hdev->driver_data;
	struct usb_device *udev = interface_to_usbdev(usbhid->intf);

	usb_external_resume_device(udev, PMSG_RESUME);
}

static void hanvon_early_resume(struct early_suspend *h)
{
	struct hanvon_data *td = container_of(h, struct hanvon_data, es);
	
	if(usb_tsp_flags & USB_TSP_FLAGS_EARLY_POWER_OFF) {

		if (usb_tsp_enable)
			usb_tsp_enable(1);
	
		if (delayed_work_pending(&td->work)) {
			cancel_delayed_work(&td->work);
			printk(KERN_DEBUG "hanvon_early_resume: releasing wake lock\n");
			wake_unlock(&td->wakelock);
		}
	
		INIT_DELAYED_WORK(&td->work, hanvon_early_resume_work);

		schedule_delayed_work(&td->work, msecs_to_jiffies(500));
	} else {
		struct hid_device *hdev = td->hdev;
		struct usbhid_device *usbhid = hdev->driver_data;
		struct usb_device *udev = interface_to_usbdev(usbhid->intf);
		
		usb_external_resume_device(udev, PMSG_RESUME);
	}
}
#endif

static int hanvon_set(struct hid_device *hid)
{
	struct hid_report *report;
	struct list_head *report_list =
			&hid->report_enum[HID_OUTPUT_REPORT].report_list;

	if (list_empty(report_list)) {
		HDBG( "no output report found\n");
		return -ENODEV;
	}

	report = list_entry(report_list->next, struct hid_report, list);

	if (report->maxfield < 1) {
		HDBG( "output report is empty\n");
		return -ENODEV;
	}
	if (report->field[0]->report_count < 7) {
		HDBG( "not enough values in the field\n");
		return -ENODEV;
	}

	report->field[0]->value[0] = 0x02;

	usbhid_submit_report(hid, report, USB_DIR_OUT);

	return 0;
}


static int hanvon_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	struct hanvon_data *td;
	int ret;
	int i;

	td = kzalloc(sizeof(struct hanvon_data), GFP_KERNEL);
	if (!td) {
		dev_err(&hdev->dev, "cannot allocate hanvon data\n");
		return -ENOMEM;
	}

	for (i = 0; i < MAX_NUM_POINTERS; i++) {
		hrtimer_init(&td->pointer[i].timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		td->pointer[i].timer.function = pointer_timer_callback;
		td->pointer[i].state = TS_WAITING;
	}

	hid_set_drvdata(hdev, td);
	td->hdev = hdev;
	
	ret = hid_parse(hdev);

	switch (hdev->product) {
		case USB_DEVICE_ID_UNITEC_7_TS_V0:
		case USB_DEVICE_ID_UNITEC_10_TS_V0:
			usb_tsp_flags |= USB_TSP_FLAGS_SLOW_EVENT_RATE;

		case USB_DEVICE_ID_UNITEC_PROTO_TS:
			usb_tsp_flags |= USB_TSP_FLAGS_RELEASE_WA;

			break;
	}

	if (ret == 0)
		ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT | HID_CONNECT_HIDINPUT_FORCE);

	if (ret == 0) {
		hanvon_set(hdev);
#ifdef CONFIG_HAS_EARLYSUSPEND
		wake_lock_init(&td->wakelock, WAKE_LOCK_SUSPEND, "hid-hanvon");
		td->es.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
		td->es.suspend = hanvon_early_suspend;
		td->es.resume = hanvon_early_resume;
		register_early_suspend(&td->es);
#endif
	} else {
		kfree(td);
	}

	return ret;
}

static void hanvon_remove(struct hid_device *hdev)
{
	struct hanvon_data *td = hid_get_drvdata(hdev);
	int i;
	
	hid_hw_stop(hdev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	if(usb_tsp_flags & USB_TSP_FLAGS_EARLY_POWER_OFF) {
		if (delayed_work_pending(&td->work)) {
			cancel_delayed_work(&td->work);
		}
	}
#endif

	for (i = 0; i < MAX_NUM_POINTERS; i++) {
		hrtimer_cancel(&td->pointer[i].timer);
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&td->es);
	wake_lock_destroy(&td->wakelock);
#endif
	hid_set_drvdata(hdev, NULL);
	kfree(td);
}

static const struct hid_device_id hanvon_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_HANVON, USB_DEVICE_ID_HANVON_TS_10_1) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_UNITEC, USB_DEVICE_ID_UNITEC_PROTO_TS) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_UNITEC, USB_DEVICE_ID_UNITEC_7_TS_V0) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_UNITEC, USB_DEVICE_ID_UNITEC_7_TS_V1) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_UNITEC, USB_DEVICE_ID_UNITEC_10_TS_V0) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_UNITEC, USB_DEVICE_ID_UNITEC_10_TS_V1) },
	{ }
};

MODULE_DEVICE_TABLE(hid, hanvon_devices);

static const struct hid_usage_id hanvon_grabbed_usages[] = {
	{ HID_ANY_ID, HID_ANY_ID, HID_ANY_ID },
	{ HID_ANY_ID - 1, HID_ANY_ID - 1, HID_ANY_ID - 1}
};

static struct hid_driver hanvon_driver = {
	.name = "hanvon",
	.id_table = hanvon_devices,
	.probe = hanvon_probe,
	.remove = hanvon_remove,
	.input_mapping = hanvon_input_mapping,
	.input_mapped = hanvon_input_mapped,
	.usage_table = hanvon_grabbed_usages,
	.event = hanvon_event,
};

/* ---------------------------------------------------------------------- */

static int hanvon_platform_probe(struct platform_device *pdev)
{
	struct usb_tsp_platform_data *pdata = pdev->dev.platform_data;

	usb_tsp_enable = pdata->panel_power;
	usb_tsp_flags = pdata->flags;

	x_offset = pdata->x_offset;
	x_scale = pdata->x_scale;

	y_offset = pdata->y_offset;
	y_scale = pdata->y_scale;
	
	return hid_register_driver(&hanvon_driver);
}

static int hanvon_platform_remove(struct platform_device *pdev)
{
	hid_unregister_driver(&hanvon_driver);
	
	usb_tsp_enable = NULL;
	
	return 0;
}

static int hanvon_platform_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct usb_tsp_platform_data *pdata = pdev->dev.platform_data;
	
	if ((pdata->flags & USB_TSP_FLAGS_POWER_OFF) && pdata->panel_power)
		pdata->panel_power(0);
	
	return 0;
}

static int hanvon_platform_resume(struct platform_device *pdev)
{
	struct usb_tsp_platform_data *pdata = pdev->dev.platform_data;

	if ((pdata->flags & USB_TSP_FLAGS_POWER_OFF) && pdata->panel_power)
		pdata->panel_power(1);

	return 0;
}

static struct platform_driver hanvon_pdrv = {
	.driver.name = "usb_tsp",
	.probe = hanvon_platform_probe,
	.remove = hanvon_platform_remove,
#ifdef CONFIG_PM
	.suspend_late = hanvon_platform_suspend,
	.resume_early = hanvon_platform_resume,
#endif
};

static int __init hanvon_init(void)
{
	return platform_driver_register(&hanvon_pdrv);
}

static void __exit hanvon_exit(void)
{
	platform_driver_unregister(&hanvon_pdrv);
}

module_init(hanvon_init);
module_exit(hanvon_exit);

