#ifndef _ARCH_ARCHOS_GPIO_H_
#define _ARCH_ARCHOS_GPIO_H_

#include <mach/gpio.h>
#include <mach/mux.h>

struct archos_gpio {
	int nb;
	int mux_cfg;
};

#define GPIO_EXISTS(x) ( x.nb >= 0 )
#define GPIO_MUX(x) ( x.mux_cfg )
#define GPIO_PIN(x) ( x.nb )
#define UNUSED_GPIO (struct archos_gpio){ .nb = -1, .mux_cfg = -1 }
#define INITIALIZE_GPIO(pin, cfg) (struct archos_gpio){ .nb = pin, .mux_cfg = cfg }

static inline void archos_gpio_init_output(const struct archos_gpio *x, const char* label)
{
	int pin = x->nb;
	int cfg = x->mux_cfg;

	if (pin < 0)
		return;

	if (gpio_request(pin, label) < 0) {
		pr_debug("archos_gpio_init_output: cannot acquire GPIO%d \n", pin);
		return;
	}
	omap_cfg_reg( cfg ) ;

	gpio_direction_output(pin, 0);
}
#define GPIO_INIT_OUTPUT(x) archos_gpio_init_output(&x, NULL)

static inline void archos_gpio_init_input(const struct archos_gpio *x, const char* label)
{
	int pin = x->nb;
	int cfg = x->mux_cfg;

	if (pin < 0)
		return;

	if (gpio_request(pin, label) < 0) {
		pr_debug("archos_gpio_init_input: cannot acquire GPIO%d \n", pin);
		return;
	}
	omap_cfg_reg( cfg ) ;

	gpio_direction_input(pin);
}
#define GPIO_INIT_INPUT(x) archos_gpio_init_input(&x, NULL)

#endif
