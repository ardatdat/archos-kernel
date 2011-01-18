#ifndef __OMAP3_OPP_H_
#define __OMAP3_OPP_H_

#include <mach/omap-pm.h>

static struct omap_opp omap3630_mpu_rate_table[] = {
	{0, 0, 0},
	{S300M,  VDD1_OPP1, 0x1f, 0x0, 0x0},
	{S600M,  VDD1_OPP2, 0x2d, 0x0, 0x0},
	{S1000M, VDD1_OPP3, 0x3c, 0x0, 0x0},
	{S1100M, VDD1_OPP4, 0x3d, 0x0, 0x0},
	{S1280M, VDD1_OPP5, 0x40, 0x0, 0x0},
};

static struct omap_opp omap3630_dsp_rate_table[] = {
	{0, 0, 0},
	{S260M, VDD1_OPP1, 0x1f, 0x0, 0x0},
	{S520M, VDD1_OPP2, 0x2d, 0x0, 0x0},
	{S400M, VDD1_OPP3, 0x3c, 0x0, 0x0},
	{S90M,  VDD1_OPP4, 0x3d, 0x0, 0x0},
	{S90M,  VDD1_OPP5, 0x40, 0x0, 0x0},
};

static struct omap_opp omap3630_l3_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP50) - 0.97V*/
	{S100M, VDD2_OPP1, 0x1e, 0x0, 0x0},
	/*OPP2 (OPP100) - 1.1625V*/
	{S200M, VDD2_OPP2, 0x2d, 0x0, 0x0},
};

static struct omap_opp omap3611_l3_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP50) - 0.97V*/
	{S83M, VDD2_OPP1, 0x1e, 0x0, 0x0},
	/*OPP2 (OPP100) - 1.1625V*/
	{S166M, VDD2_OPP2, 0x2d, 0x0, 0x0},
};


#endif
