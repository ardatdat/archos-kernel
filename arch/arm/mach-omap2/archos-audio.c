#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <mach/gpio.h>
#include <mach/mux.h>
#include <mach/archos-audio.h>
#include <mach/archos-gpio.h>
#include <asm/mach-types.h>
#include <mach/board.h>
#include <mach/display.h>
#include <mach/board-archos.h>
#include <mach/mcbsp.h>
#include <linux/delay.h>

#include "clock.h"

static struct archos_audio_conf audio_gpio;

static char *sysclock_name = NULL;
static char sys_clkout1_name[] = "sys_clkout1";
static char sys_clkout2_name[] = "sys_clkout2";
extern int archos_get_highcharge_level(void);

/* WORKAROUND: There is no way to keep CM_96M_FCLK (the sys_clkout2 source)
 * alive when every module which depends on it is idle, So we have to keep
 * the McBSP1 fclk ON.
 */
int use_mcbsp1_fclk = 0;

static void _set_ampli(int onoff)
{
	if (GPIO_PIN( audio_gpio.spdif ) < 0) {
		printk(KERN_ERR "No SPDIF in this device !\n");
		return;
	}

	if ( onoff ==  PLAT_ON)
		gpio_set_value( GPIO_PIN( audio_gpio.spdif), 0);
	else
		gpio_set_value( GPIO_PIN( audio_gpio.spdif), 1);
}

static void _set_vamp(int onoff)
{
	if (GPIO_PIN( audio_gpio.vamp_vbat ) < 0 || GPIO_PIN( audio_gpio.vamp_dc ) < 0) {
		printk(KERN_ERR "No Vamp config in this device !\n");
		return;
	}
	
	if ( onoff ==  PLAT_ON){
		if (archos_get_highcharge_level()) {
			gpio_set_value( GPIO_PIN( audio_gpio.vamp_dc), 1);
			gpio_set_value( GPIO_PIN( audio_gpio.vamp_vbat), 0);
		} else {
			gpio_set_value( GPIO_PIN( audio_gpio.vamp_vbat), 1);
			gpio_set_value( GPIO_PIN( audio_gpio.vamp_dc), 0);
		}
		msleep(300);	// fix me
	} else {
		msleep(10);
		gpio_set_value( GPIO_PIN( audio_gpio.vamp_vbat), 0);
		gpio_set_value( GPIO_PIN( audio_gpio.vamp_dc), 0);
	}
}

static void _set_hp(int onoff)
{
	if (GPIO_PIN( audio_gpio.hp_on ) < 0) {
		printk(KERN_ERR "No Speaker in this device !\n");
		return;
	}

	if ( onoff ==  PLAT_ON) {
		_set_vamp(onoff);
		gpio_set_value( GPIO_PIN( audio_gpio.hp_on), 1);
	} else {
		gpio_set_value( GPIO_PIN( audio_gpio.hp_on), 0);
		_set_vamp(onoff);
	}
}

static int _get_headphone_plugged(void)
{
	if (GPIO_PIN( audio_gpio.headphone_plugged ) < 0) {
		printk(KERN_ERR "No Headphone detection in this device !\n");
		return -1;
	}

	return gpio_get_value( GPIO_PIN( audio_gpio.headphone_plugged) );
}

static int _get_headphone_irq(void)
{
	if (GPIO_PIN( audio_gpio.headphone_plugged ) < 0) {
		printk(KERN_ERR "No Headphone detection in this device !\n");
		return -1;
	}

	return gpio_to_irq(GPIO_PIN( audio_gpio.headphone_plugged));
}

static void _sys_clkout_en(int en)
{
	struct clk *sys_clkout;

	sys_clkout = clk_get(NULL, sysclock_name);
	if (!IS_ERR(sys_clkout)) {
		if (en == PLAT_ON) {
			if (use_mcbsp1_fclk) {
				omap_mcbsp_enable_fclk(0);
			}
			if (clk_enable(sys_clkout) != 0) {
				printk(KERN_ERR "failed to enable %s\n", sysclock_name);
			}
		} else {
			clk_disable(sys_clkout);
			if (use_mcbsp1_fclk) {
				omap_mcbsp_disable_fclk(0);
			}
		}

		clk_put(sys_clkout);
	}
}

static int _get_clkout_rate(void)
{
	struct clk *sys_clkout;
	int rate;

	sys_clkout = clk_get(NULL, sysclock_name);
	if (IS_ERR(sys_clkout)) {
		printk(KERN_ERR "failed to get %s\n", sysclock_name);
	}
	rate = clk_get_rate(sys_clkout);
	clk_put(sys_clkout);

	return rate;
}

static void _suspend(void)
{
	if (GPIO_PIN( audio_gpio.headphone_plugged ) != -1)
		omap_set_gpio_debounce(GPIO_PIN( audio_gpio.headphone_plugged ),0);	
}

static void _resume(void)
{
	if (GPIO_PIN( audio_gpio.headphone_plugged ) != -1)
		omap_set_gpio_debounce(GPIO_PIN( audio_gpio.headphone_plugged ),1);	
}

static struct audio_device_config audio_device_io = {
	.set_spdif = &_set_ampli,
	.get_headphone_plugged =&_get_headphone_plugged,
	.get_headphone_irq =&_get_headphone_irq,
	.set_speaker_state = &_set_hp,
	.set_codec_master_clk_state = &_sys_clkout_en,
	.get_master_clock_rate = &_get_clkout_rate,
	.suspend = &_suspend,
	.resume = &_resume,
};

struct audio_device_config *archos_audio_get_io(void) {
		return &audio_device_io;
}

int __init archos_audio_gpio_init(void)
{
	const struct archos_audio_config *audio_cfg;
	struct clk *clkout2_src_ck;
	struct clk *sys_clkout2;
	struct clk *core_ck;

	/* audio  */
	audio_cfg = omap_get_config( ARCHOS_TAG_AUDIO, struct archos_audio_config );
	if (audio_cfg == NULL) {
		printk(KERN_DEBUG "archos_audio_gpio_init: no board configuration found\n");
		return -ENODEV;
	}
	if ( hardware_rev >= audio_cfg->nrev ) {
		printk(KERN_DEBUG "archos_audio_gpio_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, audio_cfg->nrev);
		return -ENODEV;
	}

	audio_gpio = audio_cfg->rev[hardware_rev];

	if (audio_gpio.clk_mux != -1)
		omap_cfg_reg( audio_gpio.clk_mux );

	if (hardware_rev >= 1 || !(machine_is_archos_a32() || machine_is_archos_a43())) {
		core_ck = clk_get(NULL, "cm_96m_fck");
		if (IS_ERR(core_ck)) {
			printk(KERN_ERR "failed to get core_ck\n");
		}
	
		clkout2_src_ck = clk_get(NULL, "clkout2_src_ck");
		if (IS_ERR(clkout2_src_ck)) {
			printk(KERN_ERR "failed to get clkout2_src_ck\n");
		}
	
		sys_clkout2 = clk_get(NULL, "sys_clkout2");
		if (IS_ERR(sys_clkout2)) {
			printk(KERN_ERR "failed to get sys_clkout2\n");
		}
	
		if ( clk_set_parent(clkout2_src_ck, core_ck) != 0) {
			printk(KERN_ERR "failed to set sys_clkout2 parent to clkout2\n");
		}
	
		/* Set the clock to 12 Mhz */
		omap2_clksel_set_rate(sys_clkout2, 12000000);

		clk_put(sys_clkout2);
		clk_put(clkout2_src_ck);
		clk_put(core_ck);

		sysclock_name = sys_clkout2_name;
		use_mcbsp1_fclk = 1;
	} else {
		sysclock_name = sys_clkout1_name;
	}

	if (GPIO_PIN( audio_gpio.spdif ) != -1)
		archos_gpio_init_output( &audio_gpio.spdif, "spdif" );

	if (GPIO_PIN( audio_gpio.hp_on ) != -1)
		archos_gpio_init_output( &audio_gpio.hp_on, "hp_on" );

	if (GPIO_PIN( audio_gpio.headphone_plugged ) != -1)
		archos_gpio_init_input( &audio_gpio.headphone_plugged, "hp_detect" );

	if (GPIO_PIN( audio_gpio.vamp_vbat ) != -1)
		archos_gpio_init_output( &audio_gpio.vamp_vbat, "vamp_vbat" );
	if (GPIO_PIN( audio_gpio.vamp_dc ) != -1)
		archos_gpio_init_output( &audio_gpio.vamp_dc, "vamp_dc" );

	omap_cfg_reg(P21_OMAP34XX_MCBSP2_FSX);
	omap_cfg_reg(N21_OMAP34XX_MCBSP2_CLKX);
	omap_cfg_reg(R21_OMAP34XX_MCBSP2_DR);
	omap_cfg_reg(M21_OMAP34XX_MCBSP2_DX);

	if (!(machine_is_archos_a28() || machine_is_archos_a35())) {
		omap_cfg_reg(AF6_OMAP34XX_MCBSP3_DX);
		omap_cfg_reg(AE6_OMAP34XX_MCBSP3_DR);
		omap_cfg_reg(AF5_OMAP34XX_MCBSP3_CLX);
		omap_cfg_reg(AE5_OMAP34XX_MCBSP3_FSX);
		omap_cfg_reg(AD2_OMAP34XX_MCBSP4_DX);
		omap_cfg_reg(AD1_OMAP34XX_MCBSP4_DR);
		omap_cfg_reg(AE1_OMAP34XX_MCBSP4_CLX);
		omap_cfg_reg(AC1_OMAP34XX_MCBSP4_FSX);
	}
	
	// XXX maybe prevents OFF mode?
	if (GPIO_PIN( audio_gpio.headphone_plugged ) != -1)
		omap_set_gpio_debounce(GPIO_PIN( audio_gpio.headphone_plugged ),1);

	printk("Audio GPIO init done\n");
	return 0;
}

EXPORT_SYMBOL(archos_audio_get_io);

