/*
 * fixed-switchable.h
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#ifndef __REGULATOR_FIXED_SWITCHABLE_H
#define __REGULATOR_FIXED_SWITCHABLE_H

struct fixed_switchable_voltage_config {
	const char *supply_name;
	int microvolts;
	int gpio;
};

#endif
