/*
 * fixed-switchable.c
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This is useful for systems with mixed controllable and
 * non-controllable regulators, as well as for allowing testing on
 * systems with no controllable regulators.
 */

#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/fixed-switchable.h>
#include <linux/regulator/machine.h>
#include <linux/gpio.h>

struct fixed_switchable_voltage_data {
	struct regulator_desc desc;
	struct regulator_dev *dev;
	int microvolts;
	int gpio;
};

static int fixed_switchable_voltage_list_voltage(struct regulator_dev *dev, unsigned index)
{
	struct fixed_switchable_voltage_data *data = rdev_get_drvdata(dev);
	if (index > 0)
		return -EINVAL;

	return data->microvolts;
}

static int fixed_switchable_voltage_is_enabled(struct regulator_dev *dev)
{
	struct fixed_switchable_voltage_data *data = rdev_get_drvdata(dev);
	if (gpio_is_valid(data->gpio)) {
		return gpio_get_value_cansleep(data->gpio);
	}
	return 1;
}

static int fixed_switchable_voltage_enable(struct regulator_dev *dev)
{
	struct fixed_switchable_voltage_data *data = rdev_get_drvdata(dev);	
	if (gpio_is_valid(data->gpio)) {
		gpio_set_value_cansleep(data->gpio, 1);
	}
	
	return 0;
}

static int fixed_switchable_voltage_disable(struct regulator_dev *dev)
{
	struct fixed_switchable_voltage_data *data = rdev_get_drvdata(dev);
	if (gpio_is_valid(data->gpio)) {
		gpio_set_value_cansleep(data->gpio, 0);
	}
	
	return 0;
}

static int fixed_switchable_voltage_get_voltage(struct regulator_dev *dev)
{
	struct fixed_switchable_voltage_data *data = rdev_get_drvdata(dev);

	return data->microvolts;
}

static struct regulator_ops fixed_switchable_voltage_ops = {
	.is_enabled = fixed_switchable_voltage_is_enabled,
	.enable = fixed_switchable_voltage_enable,
	.disable = fixed_switchable_voltage_disable,
	.get_voltage = fixed_switchable_voltage_get_voltage,
	.list_voltage = fixed_switchable_voltage_list_voltage,
};

static int regulator_fixed_switchable_voltage_probe(struct platform_device *pdev)
{
	struct regulator_init_data *config = pdev->dev.platform_data;
	struct regulation_constraints *c = &config->constraints;
	struct fixed_switchable_voltage_data *drvdata;
	int ret;

	drvdata = kzalloc(sizeof(struct fixed_switchable_voltage_data), GFP_KERNEL);
	if (drvdata == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	if (c->name == NULL) {
		dev_err(&pdev->dev, "need name for regulation constraints\n");
		ret = -EINVAL;
		goto err;
	}

	if (c->min_uV != c->max_uV) {
		dev_err(&pdev->dev, "fixed regulators must have min_uV == max_uV\n");
		ret = -EINVAL;
		goto err;
	}

	drvdata->desc.name = kstrdup(c->name, GFP_KERNEL);
	if (drvdata->desc.name == NULL) {
		ret = -ENOMEM;
		goto err;
	}
	drvdata->desc.type = REGULATOR_VOLTAGE;
	drvdata->desc.owner = THIS_MODULE;
	drvdata->desc.ops = &fixed_switchable_voltage_ops,
	drvdata->desc.n_voltages = 1,

	drvdata->microvolts = c->min_uV;

	drvdata->gpio = (int)config->driver_data;
	if (gpio_is_valid(drvdata->gpio)) {
		gpio_request(drvdata->gpio, "reg_sw");
		gpio_direction_output(drvdata->gpio, 0);
	}
				
	drvdata->dev = regulator_register(&drvdata->desc, &pdev->dev, drvdata);
	if (IS_ERR(drvdata->dev)) {
		ret = PTR_ERR(drvdata->dev);
		goto err_name;
	}

	platform_set_drvdata(pdev, drvdata);

	dev_dbg(&pdev->dev, "%s supplying %duV\n", drvdata->desc.name,
		drvdata->microvolts);

	return 0;

err_name:
	kfree(drvdata->desc.name);
err:
	kfree(drvdata);
	return ret;
}

static int regulator_fixed_switchable_voltage_remove(struct platform_device *pdev)
{
	struct fixed_switchable_voltage_data *drvdata = platform_get_drvdata(pdev);

	regulator_unregister(drvdata->dev);
	kfree(drvdata->desc.name);
	kfree(drvdata);

	return 0;
}

static struct platform_driver regulator_fixed_switchable_voltage_driver = {
	.probe		= regulator_fixed_switchable_voltage_probe,
	.remove		= regulator_fixed_switchable_voltage_remove,
	.driver		= {
		.name		= "reg-fsw-voltage",
	},
};

static int __init regulator_fixed_switchable_voltage_init(void)
{
	return platform_driver_register(&regulator_fixed_switchable_voltage_driver);
}
subsys_initcall(regulator_fixed_switchable_voltage_init);

static void __exit regulator_fixed_switchable_voltage_exit(void)
{
	platform_driver_unregister(&regulator_fixed_switchable_voltage_driver);
}
module_exit(regulator_fixed_switchable_voltage_exit);

MODULE_AUTHOR("Niklas Schroeter <schroeter@archos.com>");
MODULE_DESCRIPTION("Fixed switchable voltage regulator");
MODULE_LICENSE("GPL");
