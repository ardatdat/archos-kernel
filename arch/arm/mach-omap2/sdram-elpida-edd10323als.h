/*
 * SDRC register values for the ELPIDA EDD10323ALS
 *
 * Copyright (C) 2008-2009 Texas Instruments, Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARCH_ARM_MACH_OMAP2_SDRAM_ELPIDA_EDD1032
#define ARCH_ARM_MACH_OMAP2_SDRAM_ELPIDA_EDD1032

#include <mach/sdrc.h>

static struct omap_sdrc_params edd10323als_60_sdrc_params[] = {
	[0] = {
		.rate        = 166000000,
		.actim_ctrla = 0xba9dc486,
		.actim_ctrlb = 0x00022122,
		.rfr_ctrl    = 0x0004e201,
		.mr          = 0x00000032,
	},
	[1] = {
		.rate        = 100000000,
		.actim_ctrla = 0x71953484,
		.actim_ctrlb = 0x00022114,
		.rfr_ctrl    = 0x0002da01,
		.mr          = 0x00000032,
	},
	[2] = {
		.rate        = 83000000,
		.actim_ctrla = 0x61512483,
		.actim_ctrlb = 0x00022111,
		.rfr_ctrl    = 0x00025501,
		.mr          = 0x00000032,
	},
	[3] = {
		.rate        = 0
	},
};

#endif

