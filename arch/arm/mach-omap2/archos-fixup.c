/*
 * linux/arch/arm/mach-omap2/archos-fixup.c
 *
 * Copyright (C) Archos S.A.,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>

static char command_line[][COMMAND_LINE_SIZE] __initdata = {
	[0] = CONFIG_CMDLINE0,
	[1] = CONFIG_CMDLINE1,
	[2] = CONFIG_CMDLINE2,
	[3] = CONFIG_CMDLINE3,
	[4] = CONFIG_CMDLINE4,
	[5] = CONFIG_CMDLINE5,
};

void __init fixup_archos(struct machine_desc *desc,
		struct tag *tags, char **cmdline, struct meminfo *mi)
{
	if ( machine_is_archos_a43() || machine_is_archos_a70s()) {
		// regular boards
		*cmdline = command_line[0];
	} else if (machine_is_archos_a70h()) {
		// hdd boards
		*cmdline = command_line[1];
	} else if (machine_is_archos_a28()) {
		// 3611 boards
		*cmdline = command_line[2];
	} else if (machine_is_archos_a35()) {
		// 3611 boards with bigger screen...
		*cmdline = command_line[3];
	} else if (machine_is_archos_a101it()) {
		// Bigger Screen
		*cmdline = command_line[4];
	} else if (machine_is_archos_a32() || machine_is_archos_a32sd() || machine_is_archos_a35dm()) {
		// Small screen and only TV out
		*cmdline = command_line[5];
	}else {
		printk("%s : NO COMMAND LINE FOUND!", __FUNCTION__);
		return;
	}

	printk("fixup_archos: [%s]\n", *cmdline);
}
