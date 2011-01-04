/**
 * twl4030-pwrbutton.c - TWL4030 Power Button Input Driver
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Written by Peter De Schrijver <peter.de-schrijver@nokia.com>
 * Several fixes by Felipe Balbi <felipe.balbi@nokia.com>
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c/twl4030.h>

#define PWR_PWRON_IRQ (1 << 0)

#define STS_HW_CONDITIONS 0xf

struct twl4030_pwrbutton {
	struct input_dev *input;
};

extern int archos_get_dcin_plug(void);

static irqreturn_t powerbutton_irq(int irq, void *_pwr)
{
	struct twl4030_pwrbutton *pwrbt = _pwr;
	int err;
	u8 value;

#ifdef CONFIG_LOCKDEP
	/* WORKAROUND for lockdep forcing IRQF_DISABLED on us, which
	 * we don't want and can't tolerate since this is a threaded
	 * IRQ and can sleep due to the i2c reads it has to issue.
	 * Although it might be friendlier not to borrow this thread
	 * context...
	 */
	local_irq_enable();
#endif

	err = twl4030_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &value,
				  STS_HW_CONDITIONS);
	if (!err) {

		if ( value & PWR_PWRON_IRQ ) {
			// don't send input event for dcin plug
			if (archos_get_dcin_plug())
				return IRQ_HANDLED;
		}

		input_report_key(pwrbt->input, KEY_POWER, value & PWR_PWRON_IRQ);
		input_sync(pwrbt->input);
	} else {
		dev_err(pwrbt->input->dev.parent, "twl4030: i2c error %d while reading"
			" TWL4030 PM_MASTER STS_HW_CONDITIONS register\n", err);
	}

	return IRQ_HANDLED;
}

static int __devinit twl4030_pwrbutton_probe(struct platform_device *pdev)
{
	struct twl4030_pwrbutton *pwrbt;
	int irq = platform_get_irq(pdev, 0);
	int err;
	
	pwrbt = kzalloc(sizeof(*pwrbt), GFP_KERNEL);
	if (!pwrbt)
		return -ENOMEM;
	
	pwrbt->input = input_allocate_device();
	if (!pwrbt->input) {
		dev_dbg(&pdev->dev, "Can't allocate power button\n");
		err = -ENOMEM;
		goto out;
	}

	err = request_irq(irq, powerbutton_irq,
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
			"twl4030_pwrbutton", pwrbt);
	if (err < 0) {
		dev_dbg(&pdev->dev, "Can't get IRQ for pwrbutton: %d\n", err);
		goto free_input_dev;
	}

	pwrbt->input->evbit[0] = BIT_MASK(EV_KEY);
	pwrbt->input->keybit[BIT_WORD(KEY_POWER)] = BIT_MASK(KEY_POWER);
	pwrbt->input->name = "twl4030_pwrbutton";
	pwrbt->input->phys = "twl4030_pwrbutton/input0";
	pwrbt->input->dev.parent = &pdev->dev;
	
	platform_set_drvdata(pdev, pwrbt);
	
	err = input_register_device(pwrbt->input);
	if (err) {
		dev_dbg(&pdev->dev, "Can't register power button: %d\n", err);
		goto free_irq_and_out;
	}

	return 0;

free_irq_and_out:
	free_irq(irq, NULL);
free_input_dev:
	input_free_device(pwrbt->input);
out:
	return err;
}

static int __devexit twl4030_pwrbutton_remove(struct platform_device *pdev)
{
	struct twl4030_pwrbutton *pwrbt = platform_get_drvdata(pdev);
	int irq = platform_get_irq(pdev, 0);

	free_irq(irq, pwrbt->input);
	input_unregister_device(pwrbt->input);

	kfree(pwrbt);

	return 0;
}

struct platform_driver twl4030_pwrbutton_driver = {
	.probe		= twl4030_pwrbutton_probe,
	.remove		= __devexit_p(twl4030_pwrbutton_remove),
	.driver		= {
		.name	= "twl4030_pwrbutton",
		.owner	= THIS_MODULE,
	},
};

static int __init twl4030_pwrbutton_init(void)
{
	return platform_driver_register(&twl4030_pwrbutton_driver);
}
module_init(twl4030_pwrbutton_init);

static void __exit twl4030_pwrbutton_exit(void)
{
	platform_driver_unregister(&twl4030_pwrbutton_driver);
}
module_exit(twl4030_pwrbutton_exit);

MODULE_ALIAS("platform:twl4030_pwrbutton");
MODULE_DESCRIPTION("Triton2 Power Button");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter De Schrijver <peter.de-schrijver@nokia.com>");
MODULE_AUTHOR("Felipe Balbi <felipe.balbi@nokia.com>");

