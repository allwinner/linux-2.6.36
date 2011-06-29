/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : cpu-freq.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-6-18 18:13
* Descript: cpufreq driver on allwinner chips;
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/err.h>
#include "cpu-freq.h"

static struct sun4i_cpu_freq_t  cpu_cur;    /* current cpu frequency configuration  */
static unsigned int last_target = ~0;       /* backup last target frequency         */

static struct clk *clk_pll; /* pll clock handler */
static struct clk *clk_cpu; /* cpu clock handler */
static struct clk *clk_axi; /* axi clock handler */
static struct clk *clk_ahb; /* ahb clock handler */
static struct clk *clk_apb; /* apb clock handler */


/*
*********************************************************************************************************
*                           sun4i_cpufreq_verify
*
*Description: check if the cpu frequency policy is valid;
*
*Arguments  : policy    cpu frequency policy;
*
*Return     : result, return if verify ok, else return -EINVAL;
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_verify(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return -EINVAL;

	return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_show
*
*Description: show cpu frequency information;
*
*Arguments  : pfx   name;
*
*
*Return     :
*
*Notes      :
*
*********************************************************************************************************
*/
static void sun4i_cpufreq_show(const char *pfx, struct sun4i_cpu_freq_t *cfg)
{
	CPUFREQ_DBG("%s: pll=%u, cpudiv=%u, axidiv=%u, ahbdiv=%u, apb=%u\n",
        pfx, cfg->pll, cfg->div.cpu_div, cfg->div.axi_div, cfg->div.ahb_div, cfg->div.apb_div);
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_settarget
*
*Description: adjust cpu frequency;
*
*Arguments  : policy    cpu frequency policy, to mark if need notify;
*             cpu_freq  new cpu frequency configuration;
*
*Return     : return 0 if set successed, otherwise, return -EINVAL
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_settarget(struct cpufreq_policy *policy, struct sun4i_cpu_freq_t *cpu_freq)
{
    int             ret;
    unsigned int    frequency;
    struct cpufreq_freqs    freqs;
    struct sun4i_cpu_freq_t cpu_new;

    /* show current cpu frequency configuration, just for debug */
	sun4i_cpufreq_show("cur", &cpu_cur);

    /* get new cpu frequency configuration */
	cpu_new = *cpu_freq;
	sun4i_cpufreq_show("new", &cpu_new);

    /* notify that cpu clock will be adjust if needed */
	if (policy) {
	    freqs.cpu = 0;
	    freqs.old = cpu_cur.pll / 1000;
	    freqs.new = cpu_new.pll / 1000;
		cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	}

    /* try to set div for axi/ahb/apb/first */
    frequency = cpu_cur.pll / cpu_cur.div.cpu_div;
    frequency /= 4;
    clk_set_rate(clk_axi, frequency);
    frequency /= 2;
    clk_set_rate(clk_ahb, frequency);
    frequency /= 2;
    clk_set_rate(clk_apb, frequency);

    /* try to adjust pll frequency */
    ret = clk_set_rate(clk_pll, cpu_new.pll);
    /* try to adjust cpu frequency */
    frequency = cpu_new.pll / cpu_new.div.cpu_div;
    ret |= clk_set_rate(clk_cpu, frequency);
    /* try to adjuxt axi frequency */
    frequency /= cpu_new.div.axi_div;
    ret |= clk_set_rate(clk_axi, frequency);
    /* try to adjust ahb frequency */
    frequency /= cpu_new.div.ahb_div;
    ret |= clk_set_rate(clk_ahb, frequency);
    /* try to adjust apb frequency */
    frequency /= cpu_new.div.apb_div;
    ret |= clk_set_rate(clk_apb, frequency);

    if(ret) {
        CPUFREQ_ERR("%s try to set cpu frequency failed!\n", __func__);
        clk_set_rate(clk_pll, cpu_cur.pll);
        frequency = cpu_cur.pll / cpu_cur.div.cpu_div;
        clk_set_rate(clk_cpu, frequency);
        frequency /= cpu_cur.div.axi_div;
        clk_set_rate(clk_axi, frequency);
        frequency /= cpu_cur.div.ahb_div;
        clk_set_rate(clk_ahb, frequency);
        frequency /= cpu_cur.div.apb_div;
        clk_set_rate(clk_apb, frequency);
        sun4i_cpufreq_show("cur", &cpu_cur);

        /* notify everyone that clock transition finish */
    	if (policy) {
	        freqs.cpu = 0;
	        freqs.old = freqs.new = cpu_cur.pll / 1000;
		    cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	    }

        CPUFREQ_ERR(KERN_ERR "no compatible settings cpu freq for %d\n", cpu_cur.pll);
        return -EINVAL;
    }

	/* update our current settings */
	cpu_cur = cpu_new;

	/* notify everyone we've done this */
	if (policy) {
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	}

	CPUFREQ_DBG("%s: finished\n", __func__);
	return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_target
*
*Description: adjust the frequency that cpu is currently running;
*
*Arguments  : policy    cpu frequency policy;
*             freq      target frequency to be set, based on khz;
*             relation  method for selecting the target requency;
*
*Return     : result, return 0 if set target frequency successed,
*                     else, return -EINVAL;
*
*Notes      : this function is called by the cpufreq core;
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_target(struct cpufreq_policy *policy, __u32 freq, __u32 relation)
{
    unsigned int            index;
    struct sun4i_cpu_freq_t freq_cfg;

	/* avoid repeated calls which cause a needless amout of duplicated
	 * logging output (and CPU time as the calculation process is
	 * done) */
	if (freq == last_target) {
		return 0;
	}
	last_target = freq;

    CPUFREQ_DBG("%s: policy %p, target %u, relation %u\n", __func__, policy, freq, relation);

    /* try to look for a valid frequency value from cpu frequency table */
    if (cpufreq_frequency_table_target(policy, sun4i_freq_tbl, freq, relation, &index)) {
        CPUFREQ_ERR("%s: try to look for a valid frequency for %u failed!\n", __func__, freq);
		return -EINVAL;
	}

    /* update the target frequency */
    freq_cfg.pll = sun4i_freq_tbl[index].frequency * 1000;
    freq_cfg.div = *(struct sun4i_clk_div_t *)&sun4i_freq_tbl[index].index;
    CPUFREQ_DBG("%s: target frequency find is %u, entry %u\n", __func__, freq_cfg.pll, index);

    /* try to set target frequency */
    return sun4i_cpufreq_settarget(policy, &freq_cfg);
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_get
*
*Description: get the frequency that cpu currently is running;
*
*Arguments  : cpu   cpu number;
*
*Return     : cpu frequency, based on khz;
*
*Notes      :
*
*********************************************************************************************************
*/
static unsigned int sun4i_cpufreq_get(unsigned int cpu)
{
	return clk_get_rate(clk_cpu) / 1000;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_init
*
*Description: cpu frequency initialise a policy;
*
*Arguments  : policy    cpu frequency policy;
*
*Return     : result, return 0 if init ok, else, return -EINVAL;
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_init(struct cpufreq_policy *policy)
{
	CPUFREQ_DBG(KERN_INFO "%s: initialising policy %p\n", __func__, policy);

	if (policy->cpu != 0)
		return -EINVAL;

	policy->cur = sun4i_cpufreq_get(0);
	policy->min = policy->cpuinfo.min_freq = SUN4I_CPUFREQ_MIN / 1000;
	policy->max = policy->cpuinfo.max_freq = SUN4I_CPUFREQ_MAX / 1000;
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	/* feed the latency information from the cpu driver */
	policy->cpuinfo.transition_latency = SUN4I_FREQTRANS_LATENCY;

	return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_getcur
*
*Description: get current cpu frequency configuration;
*
*Arguments  : cfg   cpu frequency cofniguration;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_getcur(struct sun4i_cpu_freq_t *cfg)
{
    unsigned int    freq, freq0;

    if(!cfg) {
        return -EINVAL;
    }

	cfg->pll = clk_get_rate(clk_pll);
    freq = clk_get_rate(clk_cpu);
    cfg->div.cpu_div = cfg->pll / freq;
    freq0 = clk_get_rate(clk_axi);
    cfg->div.axi_div = freq / freq0;
    freq = clk_get_rate(clk_ahb);
    cfg->div.ahb_div = freq0 / freq;
    freq0 = clk_get_rate(clk_apb);
    cfg->div.apb_div = freq /freq0;

	return 0;
}



#ifdef CONFIG_PM

/* variable for backup cpu frequency configuration */
static struct sun4i_cpu_freq_t suspend_freq;

/*
*********************************************************************************************************
*                           sun4i_cpufreq_suspend
*
*Description: back up cpu frequency configuration for suspend;
*
*Arguments  : policy    cpu frequency policy;
*             pmsg      power management message;
*
*Return     : return 0,
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_suspend(struct cpufreq_policy *policy, pm_message_t pmsg)
{
    struct sun4i_cpu_freq_t suspend;

    CPUFREQ_DBG("%s, set cpu frequency to 60Mhz to prepare enter standby\n", __func__);

    sun4i_cpufreq_getcur(&suspend_freq);

    /* set cpu frequency to 60M hz for standby */
    suspend.pll = 30000000;
    suspend.div.cpu_div = 1;
    suspend.div.axi_div = 1;
    suspend.div.ahb_div = 2;
    suspend.div.apb_div = 2;
    sun4i_cpufreq_settarget(NULL, &suspend);

    return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_resume
*
*Description: cpu frequency configuration resume;
*
*Arguments  : policy    cpu frequency policy;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_resume(struct cpufreq_policy *policy)
{
	int ret;

    /* invalidate last_target setting */
	last_target = ~0;

	CPUFREQ_DBG("%s: resuming with policy %p\n", __func__, policy);

    /* restore cpu frequency configuration */
	ret = sun4i_cpufreq_settarget(NULL, &suspend_freq);
	if (ret) {
		CPUFREQ_ERR(KERN_ERR "%s: failed to reset pll/freq\n", __func__);
		return ret;
	}

	return 0;
}


#else   /* #ifdef CONFIG_PM */

#define sun4i_cpufreq_suspend   NULL
#define sun4i_cpufreq_resume    NULL

#endif  /* #ifdef CONFIG_PM */


static struct cpufreq_driver sun4i_cpufreq_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= sun4i_cpufreq_verify,
	.target		= sun4i_cpufreq_target,
	.get		= sun4i_cpufreq_get,
	.init		= sun4i_cpufreq_init,
	.suspend	= sun4i_cpufreq_suspend,
	.resume		= sun4i_cpufreq_resume,
	.name		= "sun4i",
};


/*
*********************************************************************************************************
*                           sun4i_cpufreq_initclks
*
*Description: init cpu frequency clock resource;
*
*Arguments  : none
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static __init int sun4i_cpufreq_initclks(void)
{
    clk_pll = clk_get(NULL, "core_pll");
    clk_cpu = clk_get(NULL, "cpu");
    clk_axi = clk_get(NULL, "axi");
    clk_ahb = clk_get(NULL, "ahb");
    clk_apb = clk_get(NULL, "apb");

	if (IS_ERR(clk_pll) || IS_ERR(clk_cpu) || IS_ERR(clk_axi) ||
	    IS_ERR(clk_ahb) || IS_ERR(clk_apb)) {
		printk(KERN_ERR "%s: could not get clock(s)\n", __func__);
		return -ENOENT;
	}

	printk(KERN_INFO "%s: clocks pll=%lu,cpu=%lu,axi=%lu,ahp=%lu,apb=%lu\n", __func__,
	       clk_get_rate(clk_pll), clk_get_rate(clk_cpu), clk_get_rate(clk_axi),
	       clk_get_rate(clk_ahb), clk_get_rate(clk_apb));

	return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_initcall
*
*Description: cpu frequency driver initcall
*
*Arguments  : none
*
*Return     : result
*
*Notes      :
*
*********************************************************************************************************
*/
static int __init sun4i_cpufreq_initcall(void)
{
	int ret = 0;

    /* initialise some clock resource */
    ret = sun4i_cpufreq_initclks();
    if(ret) {
        return ret;
    }

    /* initialise current frequency configuration */
	sun4i_cpufreq_getcur(&cpu_cur);
	sun4i_cpufreq_show("cur", &cpu_cur);

    /* register cpu frequency driver */
    ret = cpufreq_register_driver(&sun4i_cpufreq_driver);
    /* register cpu frequency table to cpufreq core */
    cpufreq_frequency_table_get_attr(sun4i_freq_tbl, 0);

	return ret;
}
late_initcall(sun4i_cpufreq_initcall);

