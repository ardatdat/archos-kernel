#include <linux/kernel.h>
#include <linux/clk.h>
#include <mach/clock.h>
#include <linux/init.h>
#include <mach/board-archos.h>

static int archos_clocks_setup_sgx_clk(void)
{
	struct clk *core_ck, *sgx_fck, *omap_192m_alwon_ck;
	
	core_ck  = clk_get(NULL, "core_ck");
	omap_192m_alwon_ck = clk_get(NULL, "omap_192m_alwon_ck");
	sgx_fck = clk_get(NULL, "sgx_fck");

	if (clk_get_rate(core_ck)/2 < clk_get_rate(omap_192m_alwon_ck)) {
		clk_set_parent(sgx_fck, omap_192m_alwon_ck);
		clk_set_rate(sgx_fck, 192000000);
	} else {
		clk_set_parent(sgx_fck, core_ck);
		clk_set_rate(sgx_fck, 200000000);
	}
	
	printk("new sgx_fck clock: %lu\n", clk_get_rate(sgx_fck));

	clk_put(sgx_fck);
	clk_put(omap_192m_alwon_ck);
	clk_put(core_ck);
	return 0;
}

static int archos_clocks_setup_dpll4(struct archos_clocks_dpll4 *dpll4)
{	
	unsigned long current_rate;
	struct clk *dpll4_clk, *dpll4_m2_ck, *dpll4_m3_ck, *dpll4_m4_ck, *dpll4_m5_ck, *dpll4_m6_ck;
	struct clk *omap_96m_alwon_fck;
	
	dpll4_clk = clk_get(NULL, "dpll4_ck");
	dpll4_m2_ck = clk_get(NULL, "dpll4_m2_ck");
	dpll4_m3_ck = clk_get(NULL, "dpll4_m3_ck");
	dpll4_m4_ck = clk_get(NULL, "dpll4_m4_ck");
	dpll4_m5_ck = clk_get(NULL, "dpll4_m5_ck");
	dpll4_m6_ck = clk_get(NULL, "dpll4_m6_ck");
	omap_96m_alwon_fck = clk_get(NULL, "omap_96m_alwon_fck");
	
#ifdef CONFIG_CLOCKS_INIT_DEBUG
	printk("current dpll4 clock: %lu\n", clk_get_rate(dpll4_clk));
	printk("dpll4 m2 clock: %lu\n", clk_get_rate(dpll4_m2_ck));
	printk("dpll4 m3 clock: %lu\n", clk_get_rate(dpll4_m3_ck));
	printk("dpll4 m4 clock: %lu\n", clk_get_rate(dpll4_m4_ck));
	printk("dpll4 m5 clock: %lu\n", clk_get_rate(dpll4_m5_ck));
	printk("dpll4 m6 clock: %lu\n", clk_get_rate(dpll4_m6_ck));
	printk("omap_96m_alwon_fck clock: %lu\n", clk_get_rate(omap_96m_alwon_fck));
#endif	
	current_rate = clk_get_rate(dpll4_clk);
	
	if (current_rate < dpll4->rate) {
		clk_set_rate(dpll4_m2_ck, dpll4->m2_rate/2);
		clk_set_rate(dpll4_m3_ck, dpll4->m3_rate/2);
		clk_set_rate(dpll4_m4_ck, dpll4->m4_rate/2);
		clk_set_rate(dpll4_m5_ck, dpll4->m5_rate/2);
		clk_set_rate(dpll4_m6_ck, dpll4->m6_rate/2);
		clk_set_rate(omap_96m_alwon_fck, 96000000/2);
		
		clk_set_rate(dpll4_clk, dpll4->rate);
	} else {
		clk_set_rate(dpll4_clk, dpll4->rate);

		clk_set_rate(dpll4_m2_ck, dpll4->m2_rate);
		clk_set_rate(dpll4_m3_ck, dpll4->m3_rate);
		clk_set_rate(dpll4_m4_ck, dpll4->m4_rate);
		clk_set_rate(dpll4_m5_ck, dpll4->m5_rate);
		clk_set_rate(dpll4_m6_ck, dpll4->m6_rate);
		clk_set_rate(omap_96m_alwon_fck, 96000000);
	}
	
#ifdef CONFIG_CLOCKS_INIT_DEBUG
	printk("new dpll4 clock: %lu\n", clk_get_rate(dpll4_clk));
	printk("dpll4 m2 clock: %lu\n", clk_get_rate(dpll4_m2_ck));
	printk("dpll4 m3 clock: %lu\n", clk_get_rate(dpll4_m3_ck));
	printk("dpll4 m4 clock: %lu\n", clk_get_rate(dpll4_m4_ck));
	printk("dpll4 m5 clock: %lu\n", clk_get_rate(dpll4_m5_ck));
	printk("dpll4 m6 clock: %lu\n", clk_get_rate(dpll4_m6_ck));
	printk("omap_96m_alwon_fck clock: %lu\n", clk_get_rate(omap_96m_alwon_fck));
#endif

	clk_put(omap_96m_alwon_fck);	
	clk_put(dpll4_m6_ck);	
	clk_put(dpll4_m5_ck);	
	clk_put(dpll4_m4_ck);	
	clk_put(dpll4_m3_ck);	
	clk_put(dpll4_m2_ck);	
	clk_put(dpll4_clk);	

	return 0;
}

int __init archos_clocks_init(struct archos_clocks *clocks)
{
	int ret = archos_clocks_setup_dpll4(&clocks->dpll4);
	if (ret)
		return ret;
		
	return archos_clocks_setup_sgx_clk();
}
