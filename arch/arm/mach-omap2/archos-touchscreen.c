#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>
#include <mach/mcspi.h>
#include <mach/gpio.h>
#include <mach/mux.h>
#include <asm/mach-types.h>
#include <mach/archos-gpio.h>
#include <mach/display.h>
#include <mach/board-archos.h>
#include <linux/delay.h>

/* GPIO used for TSC2046 (touch screen)
 *
 * Also note that the tsc2046 is the same silicon as the ads7846, so
 * that driver is used for the touch screen. */
static struct archos_gpio ts_pwron = UNUSED_GPIO;
static struct archos_gpio ts_irq = UNUSED_GPIO;

/* This enable(1)/disable(0) the voltage for TS */
static int ads7846_vaux_control(int vaux_cntrl)
{
	int enable = 1;

	if (vaux_cntrl == VAUX_ENABLE) {
		gpio_set_value( GPIO_PIN( ts_pwron ), enable);
	} else if (vaux_cntrl == VAUX_DISABLE)
		gpio_set_value( GPIO_PIN( ts_pwron ), !enable);

	return 0;
}

static struct omap2_mcspi_device_config tsc2046_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel = 1,  /* 0: slave, 1: master */
	.mode			= OMAP2_MCSPI_MASTER,
	.dma_mode		= 0,
	.force_cs_mode		= 0,
	.fifo_depth		= 0,
};

#define NUM_DATA_IDX 2
#define FILTER_LEN   4

struct ads7846_filter_data {
	int accu[NUM_DATA_IDX];
	int cnt[NUM_DATA_IDX];
};

static int ads7846_filter_init(struct ads7846_platform_data *pdata, 
		void **filter_data)
{
	void *data = kzalloc(sizeof(struct ads7846_filter_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	
	*filter_data = data;
	return 0;
}

static void ads7846_filter_cleanup(void *filter_data)
{
	if (filter_data == NULL)
		return;
	
	kfree(filter_data);
}

static int ads7846_filter(void *filter_data, int data_idx, int *val)
{
	struct ads7846_filter_data *filt = filter_data;
	int filter_len = FILTER_LEN;
	int delta;
	int data;
	
	/* we only filter X/Y */
	if (data_idx >= NUM_DATA_IDX)
		return ADS7846_FILTER_OK;
	
	if (filt->cnt[data_idx] == 0) {
		// don't send first value if delta is high, should be oscillation for low pressure ts
		if (( abs(filt->accu[data_idx] - *val ) > 1000)){
			filt->accu[data_idx] = *val;
			return ADS7846_FILTER_IGNORE;

		} else 
			data = *val;
	} else
		data = filt->accu[data_idx];

	/* filter weighted by movement speed */
	delta = abs(data - (*val));

	if (delta > 800)	// high delta probably not valid filtering it 
		data = *val;
	else if (delta > 200)
		filter_len = 1;
	else if (delta > 100)
		filter_len = 2;
	
	data = (data * (filter_len-1) + *val) / (filter_len);
	
	pr_debug("ads7846_filter: %p %i %i -> %i\n", 
			filter_data, data_idx, *val, data);
	
	*val = data;
	filt->accu[data_idx] = data;
	filt->cnt[data_idx]++;
	
	return ADS7846_FILTER_OK;
}

static void ads7846_filter_flush(void *filter_data)
{
	struct ads7846_filter_data *filt = filter_data;
	filt->cnt[0]= 0;
	filt->cnt[1]= 0;
}

// fixme depending on a32 a43 etc... ts type
static struct ads7846_platform_data tsc2046_config = {
	.keep_vref_on	   = 0,
	.vaux_control	   = ads7846_vaux_control,
	.settle_delay_usecs = 100,
	.x_plate_ohms      = 765,
	.pressure_min	   = 100,
	.pressure_max	   = 750,
	.x_max		   = MAX_12BIT,	
	.y_max		   = MAX_12BIT,	
	.penirq_recheck_delay_usecs = 0,
 	.filter_init	   = ads7846_filter_init,
 	.filter_cleanup	   = ads7846_filter_cleanup,
	.filter		   = ads7846_filter,
 	.filter_flush	   = ads7846_filter_flush,
};

static struct spi_board_info ts_spi_board_info[] = {
	[0] = {
		.modalias	= "ads7846",
		.bus_num	= 1,
		.chip_select	= 0,
		.max_speed_hz   = 2000000,
		.controller_data= &tsc2046_mcspi_config,
		.platform_data  = &tsc2046_config,
	},
};

int __init ads7846_dev_init(void)
{
	const struct archos_tsp_config *tsp_cfg;
	tsp_cfg = omap_get_config( ARCHOS_TAG_TSP, struct archos_tsp_config );
	/* might be NULL */
	if (tsp_cfg == NULL) {
		printk(KERN_DEBUG "ads7846_dev_init: no board configuration found\n");
		return -ENODEV;
	}
	if ( hardware_rev >= tsp_cfg->nrev ) {
		printk(KERN_DEBUG "ads7846_dev_init: hardware_rev (%i) >= nrev (%i)\n",
			hardware_rev, tsp_cfg->nrev);
		return -ENODEV;
	}

	ts_irq = tsp_cfg->rev[hardware_rev].irq_gpio;
	ts_pwron = tsp_cfg->rev[hardware_rev].pwr_gpio;

	tsc2046_config.x_plate_ohms = tsp_cfg->rev[hardware_rev].x_plate_ohms;
	tsc2046_config.pressure_max = tsp_cfg->rev[hardware_rev].pressure_max;
	tsc2046_config.inversion_flags = tsp_cfg->rev[hardware_rev].inversion_flags;

	if (tsp_cfg->rev[hardware_rev].bus_num != 0)
		ts_spi_board_info[0].bus_num = tsp_cfg->rev[hardware_rev].bus_num;

	if (tsp_cfg->rev[hardware_rev].bus_num == 2) {
		omap_cfg_reg(AA3_3430_MCSPI2_CLK);
		omap_cfg_reg(Y2_3430_MCSPI2_SIMO);
		omap_cfg_reg(Y3_3430_MCSPI2_SOMI);
		omap_cfg_reg(Y4_3430_MCSPI2_CS0);
	}

	printk(KERN_DEBUG "ads7846_dev_init: irq_gpio %i, pwr_gpio %i\n",
			ts_irq.nb, ts_pwron.nb);

	archos_gpio_init_output(&ts_pwron, "ts_pwron");
	tsc2046_config.gpio_pendown = GPIO_PIN(ts_irq);
	ads7846_vaux_control( VAUX_DISABLE );

	/* fix spi irq gio nb */
	ts_spi_board_info[0].irq = OMAP_GPIO_IRQ(GPIO_PIN(ts_irq));
	spi_register_board_info(ts_spi_board_info, 1);

	return 0;
}
