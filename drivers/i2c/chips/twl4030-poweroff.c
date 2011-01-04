/*
 * linux/drivers/i2c/chips/twl4030_poweroff.c
 *
 * Power off device
 *
 * Copyright (C) 2008 Nokia Corporation
 *
 * Written by Peter De Schrijver <peter.de-schrijver@nokia.com>
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
#include <linux/pm.h>
#include <linux/i2c/twl4030.h>
#ifdef CONFIG_MACH_ARCHOS
#include <mach/board-archos.h>
#endif

#define PWR_P1_SW_EVENTS	0x10
#define PWR_P2_SW_EVENTS	0x11
#define PWR_P3_SW_EVENTS	0x13
#define PWR_DEVOFF		(1<<0)
#define PWR_STOPON_POWERON 	(1<<6)
#define PWR_LVL_WAKEUP		(1<<3)

#define PWR_CFG_P1_TRANSITION	0x00
#define PWR_CFG_P2_TRANSITION	0x01
#define PWR_CFG_P3_TRANSITION	0x02
#define PWR_CFG_P123_TRANSITION	0x03

#define SEQ_OFFSYNC	(1<<0)
#define STARTON_PWRON		0x01
#define STARTON_VBUS		0x20

#define R_PROTECT_KEY		0x0E
#define KEY_1			0xFC
#define KEY_2 			0x96

#define R_VDD1_DEV_GRP		0x55
#define R_VDD2_DEV_GRP		0x63

static void twl_dump_power_regs(void)
{
	u8 i;
	
	for (i = 0; i < 0x25; i++) {
		u8 regval;
		twl4030_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &regval, i);
		printk("PM_MASTER reg 0x%02x => %02x\n", i, regval);
	}
}

static int unprotect_pm_master(void)
{
	int err;
	
	/* unlock registers for writing
	 * FIXME: should this sequence be protected with a spin lock?
	 */
	err = twl4030_i2c_write_u8(TWL4030_MODULE_PM_MASTER, KEY_1,
				R_PROTECT_KEY);
	err |= twl4030_i2c_write_u8(TWL4030_MODULE_PM_MASTER, KEY_2,
				R_PROTECT_KEY);
	if (err)
		pr_warning("TWL4030 Unable to unlock registers\n");

	return err;
}

static int protect_pm_master(void)
{
	int err;

	/* lock registers again */
	if ((err = twl4030_i2c_write_u8(TWL4030_MODULE_PM_MASTER, 0, R_PROTECT_KEY)) != 0)
		printk(KERN_ERR
			"TWL4030 Unable to relock registers\n");

	return err;
}

static int twl_set_bits(u8 mod_no, u8 val, u8 reg)
{
	int err;
	u8 uninitialized_var(reg_val);
	
	err = twl4030_i2c_read_u8(mod_no, &reg_val, reg);
	if (err < 0)
		return err;
	
	reg_val |= val;
	err = twl4030_i2c_write_u8(mod_no, reg_val, reg);
	
	return err;
}

static int twl_clear_bits(u8 mod_no, u8 val, u8 reg)
{
	int err;
	u8 uninitialized_var(reg_val);
	
	err = twl4030_i2c_read_u8(mod_no, &reg_val, reg);
	if (err < 0)
		return err;
	
	reg_val &= ~val;
	err = twl4030_i2c_write_u8(mod_no, reg_val, reg);
	
	return err;	
}

static void twl4030_poweroff(void)
{
	u8 uninitialized_var(val);
	int err;

	unprotect_pm_master();	
	/* Make sure SEQ_OFFSYNC is set so that all the res goes to wait-on */
	err = twl_set_bits(TWL4030_MODULE_PM_MASTER, SEQ_OFFSYNC, 
			PWR_CFG_P123_TRANSITION);	
	protect_pm_master();

	if (err < 0) {
		pr_warning("I2C error %d while setting TWL4030 PM_MASTER CFG_P123_TRANSITION\n", err);
		return;
	}

	err = twl_set_bits(TWL4030_MODULE_PM_MASTER, 
			PWR_STOPON_POWERON | PWR_DEVOFF, PWR_P1_SW_EVENTS);
	
	if (err < 0) {
		pr_warning("I2C error %d while writing TWL4030 PM_MASTER P1_SW_EVENTS\n", err);
	}

	return;
}

static int __init twl4030_poweroff_init(void)
{
	int err;
	u8 starton_flags;
	
	unprotect_pm_master();
	
	starton_flags = STARTON_PWRON;
	
	if (!machine_has_usbhost_plug())
		starton_flags |= STARTON_VBUS; 
	
	err = twl4030_i2c_write_u8(TWL4030_MODULE_PM_MASTER, starton_flags, 
			PWR_CFG_P1_TRANSITION);
	err |= twl4030_i2c_write_u8(TWL4030_MODULE_PM_MASTER, starton_flags, 
			PWR_CFG_P2_TRANSITION);
	err |= twl4030_i2c_write_u8(TWL4030_MODULE_PM_MASTER, starton_flags, 
			PWR_CFG_P3_TRANSITION);
	if (err)
		pr_warning("TWL4030 Unable to configure STARTON transition\n");
	
	protect_pm_master();

	err = twl_set_bits(TWL4030_MODULE_PM_MASTER, PWR_STOPON_POWERON, 
			PWR_P1_SW_EVENTS);
	err |= twl_set_bits(TWL4030_MODULE_PM_MASTER, PWR_STOPON_POWERON, 
			PWR_P2_SW_EVENTS);
	err |= twl_set_bits(TWL4030_MODULE_PM_MASTER, PWR_STOPON_POWERON,
			PWR_P3_SW_EVENTS);
	if (err) {
		printk(KERN_WARNING "I2C error %d while writing TWL4030"
					"PM_MASTER P1_SW_EVENTS\n", err);
	}

	pm_power_off = twl4030_poweroff;

	return 0;
}

static void __exit twl4030_poweroff_exit(void)
{
	pm_power_off = NULL;
}

module_init(twl4030_poweroff_init);
module_exit(twl4030_poweroff_exit);

MODULE_ALIAS("i2c:twl4030-poweroff");
MODULE_DESCRIPTION("Triton2 device power off");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter De Schrijver");
