/*
 * SDRC register values for the ELPIDA EDK2132C2PD
 *
 * Copyright (C) 2008-2009 Texas Instruments, Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARCH_ARM_MACH_OMAP2_SDRAM_ELPIDA_EDK2132
#define ARCH_ARM_MACH_OMAP2_SDRAM_ELPIDA_EDK2132

#include <mach/sdrc.h>

static struct omap_sdrc_params edk2132c2pd_60_sdrc_params[] = {
	[0] = {
		.rate        = 166000000,
		.actim_ctrla = 0x829db486,
		.actim_ctrlb = 0x00011114,
		.rfr_ctrl    = 0x0004e201,
		.mr          = 0x00000032,
	},
	[1] = {
		.rate        = 100000000,
		.actim_ctrla = 0x61952484,
		.actim_ctrlb = 0x0002120e,
		.rfr_ctrl    = 0x0002da01,
		.mr          = 0x00000032,
	},
	[2] = {
		.rate        = 83000000,
		.actim_ctrla = 0x41512243,
		.actim_ctrlb = 0x0001110a,
		.rfr_ctrl    = 0x00025501,
		.mr          = 0x00000032,
	},
	[3] = {
		.rate        = 0
	},
};

static struct omap_sdrc_params edk2132c2pd_50_sdrc_params[] = {
	[0] = {
		.rate        = 200000000,
		.actim_ctrla = 0xa2e1b4c6,
		.actim_ctrlb = 0x00022118,
		.rfr_ctrl    = 0x0005e601,
		.mr          = 0x00000032,
	},
	[1] = {
		.rate        = 166000000,
		.actim_ctrla = 0x829db485,
		.actim_ctrlb = 0x00022114,
		.rfr_ctrl    = 0x0004e201,
		.mr          = 0x00000032,
	},
	[2] = {
		.rate        = 100000000,
		.actim_ctrla = 0x61912483,
		.actim_ctrlb = 0x0002210c,
		.rfr_ctrl    = 0x0002da01,
		.mr          = 0x00000032,
	},
	[3] = {
		.rate        = 83000000,
		.actim_ctrla = 0x41512243,
		.actim_ctrlb = 0x0002210a,
		.rfr_ctrl    = 0x00025501,
		.mr          = 0x00000032,
	},
	[4] = {
		.rate        = 0
	},
};

#endif

