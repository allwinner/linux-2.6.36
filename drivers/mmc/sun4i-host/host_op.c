 /*
*********************************************************************************************************
*                                                    eBase
*                                           the Abstract of Hardware
*
*                                   (c) Copyright 2006-2010, holigun China
*                                                All Rights Reserved
*
* File        :  host_op.c
* Date        :  2012-11-25 14:24:12
* Author      :  Aaron.Maoye
* Version     :  v1.00
* Description:
*                                   Operation for SD Controller module, aw1620
* History     :
*      <author>         <time>                  <version >      <desc>
*       Maoye           2012-11-25 14:25:25     1.0             create this file
*
*********************************************************************************************************
*/

#include "smc_syscall.h"
#include "host_op.h"
#include "sdxc.h"
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <mach/clock.h>

static char* awsmc_hclk_name[4] = {"ahb_sdc0", "ahb_sdc1", "ahb_sdc2", "ahb_sdc3"};
static char* awsmc_mclk_name[4] = {"sdc0", "sdc1", "sdc2", "sdc3"};
struct awsmc_host* sw_host[4] = {NULL, NULL, NULL, NULL};
EXPORT_SYMBOL_GPL(sw_host);

/* Module parameters */
/* debug control */
unsigned int smc_debug = 2;
EXPORT_SYMBOL_GPL(smc_debug);
module_param_named(awsmc_debug, smc_debug, int, 0);

static unsigned int smc_mclk_source = SMC_MCLK_SRC_DRAMPLL;
module_param_named(mclk_source, smc_mclk_source, int, 0);

static unsigned int smc_io_clock = SMC_MAX_IO_CLOCK;
module_param_named(io_clock, smc_io_clock, int, 0);

static unsigned int smc_mod_clock = SMC_MAX_MOD_CLOCK;
module_param_named(mod_clock, smc_mod_clock, int, 0);

static int sdc_used[4] = {0};
#ifdef CONFIG_SW_MMC_POWER_CONTROL
extern int mmc_pm_get_mod_type(void);
#else
static __inline int mmc_pm_get_mod_type(void){return 0;}
#endif
void awsmc_dumpreg(struct awsmc_host* smc_host)
{
    __u32 i;

    printk("SDC %d registers:", smc_host->pdev->id);
    for (i=0; i<0xA0; i+=4)
    {
        if (!(i&0xf))
            printk("\n0x%08x : ", i);
        printk("0x%08x ", sdc_read(smc_host->smc_base + i));
    }
    printk("\n");
}

s32 awsmc_init_controller(struct awsmc_host* smc_host)
{
    u32 smc_no = smc_host->pdev->id;

    awsmc_msg("MMC Driver init host %d\n", smc_no);

    sdxc_init(smc_host);

    return 0;
}

/* static s32 awsmc_set_src_clk(struct awsmc_host* smc_host)
 * 设置SD卡控制器源时钟频率, 目标为100MHz，clock源有smc_host的clk_source决定
 * clk_source: 0-video PLL, 2-dram PLL, 3-core pll
 */
static int awsmc_set_src_clk(struct awsmc_host* smc_host)
{
    struct clk *source_clock = NULL;
    char* name = NULL;
    int ret;

    switch (smc_host->clk_source)
    {
        case 0:
        case 3:
            source_clock = clk_get(&smc_host->pdev->dev, "hosc");
            name = "hosc";
            break;
        case 1:
            source_clock = clk_get(&smc_host->pdev->dev, "sata_pll_2");
            name = "sata_pll_2";
            break;
        case 2:
            source_clock = clk_get(&smc_host->pdev->dev, "sdram_pll_p");
            name = "sdram_pll_p";
            break;
    }
    if (IS_ERR(source_clock))
    {
    	ret = PTR_ERR(source_clock);
    	awsmc_dbg_err("Error to get source clock %s\n", name);
    	return ret;
    }

    clk_set_parent(smc_host->mclk, source_clock);
    clk_set_rate(smc_host->mclk, smc_host->mod_clk);
    clk_enable(smc_host->mclk);
    #ifdef AW1623_FPGA
    smc_host->mod_clk = 24000000;//fpga
    #else
    smc_host->mod_clk = clk_get_rate(smc_host->mclk);
    #endif
    clk_enable(smc_host->hclk);

    awsmc_msg("smc %d, source = %s, src_clk = %u, mclk %u, \n", smc_host->pdev->id, name, (unsigned)clk_get_rate(source_clock), smc_host->mod_clk);
    clk_put(source_clock);

    return 0;
}

static int awsmc_resource_request(struct awsmc_host *smc_host)
{
    struct platform_device *pdev = smc_host->pdev;
    u32 smc_no = pdev->id;
    char* pio_para[] = {"mmc0_para", "mmc1_para", "mmc2_para", "mmc3_para"};
    u32 pio_hdle = 0;
    s32 ret = 0;

    pio_hdle = gpio_request_ex(pio_para[smc_no], NULL);
    if (!pio_hdle)
    {
        awsmc_dbg_err("sdc %d request pio parameter failed\n", smc_no);
        #ifndef AW1623_FPGA
        goto out;//fpga
        #endif
    }
    smc_host->pio_hdle = pio_hdle;

    //iomap
    smc_host->smc_base_res  = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!smc_host->smc_base_res)
    {
    	awsmc_dbg_err("Failed to get io memory region resouce.\n");

    	ret = -ENOENT;
    	goto release_pin;
    }
    /* smc address remap */
    smc_host->smc_base_res = request_mem_region(smc_host->smc_base_res->start, RESSIZE(smc_host->smc_base_res), pdev->name);
    if (!smc_host->smc_base_res)
    {
    	awsmc_dbg_err("Failed to request io memory region.\n");
    	ret = -ENOENT;
    	goto release_pin;
    }
    smc_host->smc_base = ioremap(smc_host->smc_base_res->start, RESSIZE(smc_host->smc_base_res));
    if (!smc_host->smc_base)
    {
    	awsmc_dbg_err("Failed to ioremap() io memory region.\n");
    	ret = -EINVAL;
    	goto free_mem_region;
    }

    //get hclock
    smc_host->hclk = clk_get(&pdev->dev, awsmc_hclk_name[smc_no]);
    if (IS_ERR(smc_host->hclk))
    {
    	ret = PTR_ERR(smc_host->hclk);
    	awsmc_dbg_err("Error to get ahb clk for %s\n", awsmc_hclk_name[smc_no]);
    	goto iounmap;
    }

    smc_host->mclk = clk_get(&pdev->dev, awsmc_mclk_name[smc_no]);
    if (IS_ERR(smc_host->mclk))
    {
    	ret = PTR_ERR(smc_host->mclk);
    	awsmc_dbg_err("Error to get clk for mux_mmc\n");
    	goto free_hclk;
    }

    goto out;

free_hclk:
    clk_put(smc_host->hclk);

iounmap:
    iounmap(smc_host->smc_base);

free_mem_region:
    release_mem_region(smc_host->smc_base_res->start, RESSIZE(smc_host->smc_base_res));

release_pin:
    gpio_release(smc_host->pio_hdle, 1);

out:
    return ret;
}


static int awsmc_resource_release(struct awsmc_host *smc_host)
{
    //close clock resource
    clk_disable(smc_host->hclk);
    clk_put(smc_host->hclk);

    clk_disable(smc_host->mclk);
    clk_put(smc_host->mclk);

    //free memory region
    iounmap(smc_host->smc_base);
    release_mem_region(smc_host->smc_base_res->start, RESSIZE(smc_host->smc_base_res));

    gpio_release(smc_host->pio_hdle, 1);
    return 0;
}

static inline void mmc_suspend_pins(struct awsmc_host* smc_host)
{
    int ret;
    user_gpio_set_t suspend_gpio_set = {"suspend_pins_sdio", 0, 0, 0, 2, 1, 0};     //for sdio
    user_gpio_set_t suspend_gpio_set_card = {"suspend_pins_mmc", 0, 0, 0, 0, 1, 0};    //for mmc card
    u32 i;
    
    awsmc_msg("mmc %d suspend pins\n", smc_host->pdev->id);
    /* backup gpios' current config */
    ret = gpio_get_all_pin_status(smc_host->pio_hdle, smc_host->bak_gpios, 6, 1);
    if (ret)
    {
        awsmc_dbg_err("fail to fetch current gpio cofiguration\n");
        return;
    }
    
//    {
//        awsmc_msg("printk backup gpio configuration: \n");
//        for (i=0; i<6; i++)
//        {
//            awsmc_msg("gpio[%d]: name %s, port %c[%d], cfg %d, pull %d, drvl %d, data %d\n",
//                         i, smc_host->bak_gpios[i].gpio_name, 
//                            smc_host->bak_gpios[i].port + 'A' - 1, 
//                            smc_host->bak_gpios[i].port_num, 
//                            smc_host->bak_gpios[i].mul_sel,
//                            smc_host->bak_gpios[i].pull,
//                            smc_host->bak_gpios[i].drv_level,
//                            smc_host->bak_gpios[i].data);
//        }
//    }
    
    switch(smc_host->pdev->id)
    {
        case 0:
        case 1:
        case 2:
            /* setup all pins to input and no pull to save power */
            for (i=0; i<6; i++)
            {
                ret = gpio_set_one_pin_status(smc_host->pio_hdle, &suspend_gpio_set_card, smc_host->bak_gpios[i].gpio_name, 1);
                if (ret)
                {
                    awsmc_dbg_err("fail to set IO(%s) into suspend status\n", smc_host->bak_gpios[i].gpio_name);
                }
            }
            break;
        case 3:
            /* setup all pins to input and pulldown to save power */
            for (i=0; i<6; i++)
            {
                ret = gpio_set_one_pin_status(smc_host->pio_hdle, &suspend_gpio_set, smc_host->bak_gpios[i].gpio_name, 1);
                if (ret)
                {
                    awsmc_dbg_err("fail to set IO(%s) into suspend status\n", smc_host->bak_gpios[i].gpio_name);
                }
            }
            break;
    }
    
//    {
//        user_gpio_set_t post_cfg[6];
//        
//        gpio_get_all_pin_status(smc_host->pio_hdle, post_cfg, 6, 1);
//        for (i=0; i<6; i++)
//        {
//            awsmc_msg("post suspend, gpio[%d]: name %s, port %c[%d], cfg %d, pull %d, drvl %d, data %d\n",
//                         i, post_cfg[i].gpio_name, 
//                            post_cfg[i].port + 'A' - 1, 
//                            post_cfg[i].port_num, 
//                            post_cfg[i].mul_sel,
//                            post_cfg[i].pull,
//                            post_cfg[i].drv_level,
//                            post_cfg[i].data);
//        }
//    }
    
    smc_host->gpio_suspend_ok = 1;
    return;
}

static inline void mmc_resume_pins(struct awsmc_host* smc_host)
{
    int ret;
    u32 i;
    
    awsmc_msg("mmc %d resume pins\n", smc_host->pdev->id);
    switch(smc_host->pdev->id)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            /* restore gpios' backup configuration */
            if (smc_host->gpio_suspend_ok)
            {
                smc_host->gpio_suspend_ok = 0;
                for (i=0; i<6; i++)
                {
                    ret = gpio_set_one_pin_status(smc_host->pio_hdle, &smc_host->bak_gpios[i], smc_host->bak_gpios[i].gpio_name, 1);
                    if (ret)
                    {
                        awsmc_dbg_err("fail to restore IO(%s) to resume status\n", smc_host->bak_gpios[i].gpio_name);
                    }
                }
            }
            
            break;
    }
}

static void finalize_request(struct awsmc_host *smc_host)
{
    struct mmc_request* mrq = smc_host->mrq;

    if (smc_host->wait != SDC_WAIT_FINALIZE)
    {
	    printk("nothing finalize\n");
        return;
	}

    awsmc_dbg("request finalize !!\n");
    smc_host->ops.finalize_requset(smc_host);

    if (smc_host->error)
    {
        mrq->cmd->error = ETIMEDOUT;
        if (mrq->data)
        {
        	mrq->data->error = ETIMEDOUT;
        }
    }
    else
    {
    	if (mrq->data)
        {
    		if (mrq->data)
    		{
    	        mrq->data->bytes_xfered = (mrq->data->blocks * mrq->data->blksz);
    		}
        }
    }

    smc_host->wait = SDC_WAIT_NONE;
    smc_host->mrq = NULL;
    smc_host->error = 0;
    smc_host->todma = 0;
    mmc_request_done(smc_host->mmc, mrq);

    return;
}

static s32 awsmc_get_ro(struct mmc_host *mmc)
{
    struct awsmc_host *smc_host = mmc_priv(mmc);
    char* mmc_para[4] = {"mmc0_para", "mmc1_para", "mmc2_para", "mmc3_para"};
    int card_wp = 0;
    int ret;
    u32 gpio_val;

    ret = script_parser_fetch(mmc_para[smc_host->pdev->id], "sdc_use_wp", &card_wp, sizeof(int));
    if (ret)
    {
    	awsmc_dbg_err("sdc fetch card write protect mode failed\n");
    }
    if (card_wp)
    {
        gpio_val = gpio_read_one_pin_value(smc_host->pio_hdle, "sdc_wp");
        awsmc_dbg("sdc fetch card write protect pin status val = %d \n", gpio_val);
        if (!gpio_val)
        {
            smc_host->read_only = 0;
            return 0;
        }
        else
        {
            awsmc_msg("Card is write-protected\n");
            smc_host->read_only = 1;
            return 1;
        }
    }
    else
    {
        smc_host->read_only = 0;
        return 0;
    }
}

static void awsmc_cd_timer(unsigned long data)
{
    struct awsmc_host *smc_host = (struct awsmc_host *)data;
    u32 gpio_val;
    u32 present;

    gpio_val = gpio_read_one_pin_value(smc_host->pio_hdle, "sdc_det");
    if (gpio_val)
        present = 0;
    else
        present = 1;

//    awsmc_dbg("cd %d, host present %d, cur present %d\n", gpio_val, smc_host->present, present);

    if (smc_host->present ^ present) {
        awsmc_msg("mmc %d detect change, present %d\n", smc_host->pdev->id, present);
        smc_host->present = present;
        mmc_detect_change(smc_host->mmc, msecs_to_jiffies(100));
    } else {
//        awsmc_dbg("card detect no change\n");
    }

    mod_timer(&smc_host->cd_timer, jiffies + 30);
    return;
}

void sw_mmc_rescan_card(unsigned id, unsigned insert)
{
    struct awsmc_host *smc_host = NULL;
    
    BUG_ON(id > 3);
    BUG_ON(sw_host[id] == NULL);
    smc_host = sw_host[id];
    
    smc_host->present = insert ? 1 : 0;
    mmc_detect_change(smc_host->mmc, 0);
    return;
}
EXPORT_SYMBOL_GPL(sw_mmc_rescan_card);

static int awsmc_card_present(struct mmc_host *mmc)
{
    struct awsmc_host *smc_host = mmc_priv(mmc);

    if (smc_host->cd_mode == CARD_ALWAYS_PRESENT)
        return 1;
    else
        return smc_host->present;
}

static irqreturn_t awsmc_irq(int irq, void *dev_id)
{
    struct awsmc_host *smc_host = dev_id;
    sdc_req_ops* ops = &smc_host->ops;
    unsigned long iflags;

    spin_lock_irqsave(&smc_host->lock, iflags);

    smc_host->sdio_int = 0;
    if (smc_host->cd_mode == CARD_DETECT_BY_DATA3)
    {
        smc_host->change = 0;
    }

    ops->check_status(smc_host);

    if (smc_host->wait == SDC_WAIT_FINALIZE)
    {
        tasklet_schedule(&smc_host->tasklet);
    }

    spin_unlock_irqrestore(&smc_host->lock, iflags);

    /* sdio interrupt call */
    if (smc_host->sdio_int)
    {
        mmc_signal_sdio_irq(smc_host->mmc);
    	//awsmc_dbg("- sdio int -\n");
    }

    /* card detect change */
    if (smc_host->cd_mode == CARD_DETECT_BY_DATA3)
    {
        if (smc_host->change)
        {
            mmc_detect_change(smc_host->mmc, msecs_to_jiffies(500));
        }
    }

    return IRQ_HANDLED;
}

static void awsmc_tasklet(unsigned long data)
{
    struct awsmc_host *smc_host = (struct awsmc_host *) data;

    if (smc_host->wait == SDC_WAIT_FINALIZE)
    {
        finalize_request(smc_host);
    }
}

static void awsmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
    struct awsmc_host *smc_host = mmc_priv(mmc);
    
    /* Set the power state */
    switch (ios->power_mode)
    {
        case MMC_POWER_ON:
        case MMC_POWER_UP:
            if (!smc_host->power_on)
            {
                awsmc_dbg("MMC_POWER_ON\n");
                /* resume pins to correct status */
                mmc_resume_pins(smc_host);
            	/* enable mmc hclk */
            	clk_enable(smc_host->hclk);
            	/* enable mmc mclk */
            	clk_enable(smc_host->mclk);
                /* restore registers */
                sdxc_regs_restore(smc_host);
                sdxc_program_clk(smc_host);
                /* enable irq */
                enable_irq(smc_host->irq);
                smc_host->power_on = 1;
            }
        	break;
        case MMC_POWER_OFF:
            if (smc_host->power_on)
            {
                awsmc_dbg("MMC_POWER_OFF\n");
                /* disable irq */
                disable_irq(smc_host->irq);
                /* backup registers */
                sdxc_regs_save(smc_host);
            	/* disable mmc mclk */
            	clk_disable(smc_host->mclk);
            	/* disable mmc hclk */
            	clk_disable(smc_host->hclk);
                /* suspend pins to save power */
                mmc_suspend_pins(smc_host);
                smc_host->power_on = 0;
            }
            
        default:
        	break;
    }
    
    if (smc_host->power_on) 
    {
        /* set clock */
        if (ios->clock)
        {
            smc_host->cclk = ios->clock;
            sdxc_update_clk(smc_host, smc_host->mod_clk, smc_host->cclk);
    
            if ((ios->power_mode == MMC_POWER_ON) || (ios->power_mode == MMC_POWER_UP))
            {
            	awsmc_dbg("running at %dkHz (requested: %dkHz).\n", smc_host->real_cclk/1000, ios->clock/1000);
            }
            else
            {
            	awsmc_dbg("powered down.\n");
            }
        }
    
        /* set bus width */
        if (smc_host->bus_width != (1<<ios->bus_width))
        {
            sdxc_set_buswidth(smc_host, 1<<ios->bus_width);
            smc_host->bus_width = 1<<ios->bus_width;
        }
    }
}

static void awsmc_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
    struct awsmc_host *smc_host = mmc_priv(mmc);
    unsigned long flags;

    spin_lock_irqsave(&smc_host->lock, flags);
    sdxc_enable_sdio_irq(smc_host, enable);
    spin_unlock_irqrestore(&smc_host->lock, flags);
}

int awsmc_check_r1_ready(struct mmc_host *mmc)
{
    struct awsmc_host *smc_host = mmc_priv(mmc);
    return sdxc_check_r1_ready(smc_host);
}
EXPORT_SYMBOL_GPL(awsmc_check_r1_ready);

static void awsmc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
    struct awsmc_host *smc_host = mmc_priv(mmc);
    struct mmc_command *cmd = mrq->cmd;

    if (cmd->opcode == MMC_STOP_TRANSMISSION)
    {
        awsmc_msg("Request to send stop command, do not care !!\n");
        //mmc_request_done(mmc, mrq);
        return;
    }

    smc_host->mrq = mrq;

    if (awsmc_card_present(mmc) == 0)
    {
    	awsmc_dbg("no medium present\n");
    	smc_host->mrq->cmd->error = -ENOMEDIUM;
    	mmc_request_done(mmc, mrq);
    }
    else
    {
        smc_host->ops.send_request(smc_host, mrq);
    }
}

/*
#ifdef CONFIG_DEBUG_FS

#define AWSMC_DRV_VERSION "0.03"

/ * driver version show * /
static int awsmc_version_show(struct seq_file * seq, void *data)
{
    seq_printf(seq, "MMC driver version: %s\n", AWSMC_DRV_VERSION);

    return 0;
}

static int awsmc_version_open(struct inode *inode, struct file *file)
{
    return single_open(file, awsmc_version_show, inode->i_private);
}

static const struct file_operations awsmc_fops_version = {
    .owner		= THIS_MODULE,
    .open		= awsmc_version_open,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};

/ * driver control info. show * /
static int awsmc_hostinfo_show(struct seq_file * seq, void *data)
{
    struct awsmc_host *smc_host = (struct awsmc_host *)seq->private;
    struct device* dev = &smc_host->pdev->dev;
    char* clksrc[] = {"hosc", "satapll", "sdrampll_p", "hosc"};
    char* cd_mode[] = {"none", "gpio mode", "data3 mode", "always in"};

     seq_printf(seq, "%s controller information:\n", dev_name(dev));
    seq_printf(seq, "reg base\t : %p\n", smc_host->smc_base);
    seq_printf(seq, "clock source\t : %s\n", clksrc[smc_host->clk_source]);
    seq_printf(seq, "mod clock\t : %d\n", smc_host->mod_clk);
    seq_printf(seq, "card clock\t : %d\n", smc_host->real_cclk);
    seq_printf(seq, "bus width\t : %d\n", smc_host->bus_width);
    seq_printf(seq, "present\t : %d\n", smc_host->present);
    seq_printf(seq, "cd gpio\t : %s\n", cd_mode[smc_host->cd_mode]);

    return 0;
}

static int awsmc_hostinfo_open(struct inode *inode, struct file *file)
{
    return single_open(file, awsmc_hostinfo_show, inode->i_private);
}

static const struct file_operations awsmc_fops_hostinfo = {
    .owner		= THIS_MODULE,
    .open		= awsmc_hostinfo_open,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};

/ * register show * /
static int awsmc_regs_show(struct seq_file * seq, void *data)
{
    struct awsmc_host *smc_host = (struct awsmc_host *)seq->private;
    u32 i;

    seq_printf(seq, "Dump smc regs:\n");

    for (i=0; i<0x100; i+=4)
    {
        if (!(i&0xf))
            seq_printf(seq, "\n0x%08x : ", i);
        seq_printf(seq, "%08x ", sdc_read(smc_host->smc_base + i));
    }
    seq_printf(seq, "\n");

    return 0;
}

static int awsmc_regs_open(struct inode *inode, struct file *file)
{
    return single_open(file, awsmc_regs_show, inode->i_private);
}

static const struct file_operations awsmc_fops_regs = {
    .owner		= THIS_MODULE,
    .open		= awsmc_regs_open,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};

static inline void awsmc_debugfs_attach(struct awsmc_host *smc_host)
{
    struct device *dev = &smc_host->pdev->dev;
    char* dbg_reg_name[] = {"awsmc.0-reg", "awsmc.1-reg", "awsmc.2-reg", "awsmc.3-reg"};

    smc_host->debug_root = debugfs_create_dir(dev_name(dev), NULL);
    if (IS_ERR(smc_host->debug_root)) {
        awsmc_msg("%s: failed to create debugfs root.\n", dev_name(dev));
    	return;
    }

    smc_host->driver_version = debugfs_create_file("version", 0444, smc_host->debug_root, smc_host, &awsmc_fops_version);
    if (IS_ERR(smc_host->driver_version))
    {
        awsmc_msg("%s: failed to create version file.\n", dev_name(dev));
    }

    smc_host->host_info = debugfs_create_file("hostinfo", 0444, smc_host->debug_root, smc_host, &awsmc_fops_hostinfo);
    if (IS_ERR(smc_host->host_info))
    {
        awsmc_msg("%s: failed to create hostinfo file.\n", dev_name(dev));
    }

    smc_host->debug_level = debugfs_create_u32("dbglevel", 0644, smc_host->debug_root, (u32*)&smc_debug);
    if (IS_ERR(smc_host->debug_level))
    {
        awsmc_msg("%s: failed to create dbglevel file.\n", dev_name(dev));
    }

    smc_host->debug_regs = debugfs_create_file(dbg_reg_name[smc_host->pdev->id], 0444, smc_host->debug_root, smc_host, &awsmc_fops_regs);
    if (IS_ERR(smc_host->debug_regs))
    {
        awsmc_msg("%s: failed to create regs file.\n", dev_name(dev));
    }
}

static inline void awsmc_debugfs_remove(struct awsmc_host *smc_host)
{
    debugfs_remove(smc_host->debug_regs);
    debugfs_remove(smc_host->debug_level);
    debugfs_remove(smc_host->host_info);
    debugfs_remove(smc_host->driver_version);
    debugfs_remove(smc_host->debug_root);
}

#else

static inline void awsmc_debugfs_attach(struct awsmc_host *smc_host) { }
static inline void awsmc_debugfs_remove(struct awsmc_host *smc_host) { }

#endif
*/

#ifdef CONFIG_PROC_FS
static const char awsmc_drv_version[] = "AWSMC Driver Version : 0.04 (move debugfs to procfs)";

static int awsmc_proc_read_drvversion(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    char *p = page;

    p += sprintf(p, "%s\n", awsmc_drv_version);
    return p - page;
}

static int awsmc_proc_read_hostinfo(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    char *p = page;
    struct awsmc_host *smc_host = (struct awsmc_host *)data;
    struct device* dev = &smc_host->pdev->dev;
    char* clksrc[] = {"hosc", "satapll_2", "sdrampll_p", "hosc"};
    char* cd_mode[] = {"none", "gpio mode", "data3 mode", "always in", "manual"};

    p += sprintf(p, "%s controller information:\n", dev_name(dev));
    p += sprintf(p, "reg base \t : %p\n", smc_host->smc_base);
    p += sprintf(p, "clock source\t : %s\n", clksrc[smc_host->clk_source]);
    p += sprintf(p, "mod clock\t : %d\n", smc_host->mod_clk);
    p += sprintf(p, "card clock\t : %d\n", smc_host->real_cclk);
    p += sprintf(p, "bus width\t : %d\n", smc_host->bus_width);
    p += sprintf(p, "present  \t : %d\n", smc_host->present);
    p += sprintf(p, "cd mode  \t : %s\n", cd_mode[smc_host->cd_mode]);
    p += sprintf(p, "read only\t : %d\n", smc_host->read_only);

    return p - page;
}


static int awsmc_proc_read_regs(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    char *p = page;
    struct awsmc_host *smc_host = (struct awsmc_host *)data;
    u32 i;

    p += sprintf(p, "Dump smc regs:\n");

    for (i=0; i<0x100; i+=4)
    {
        if (!(i&0xf))
            p += sprintf(p, "\n0x%08x : ", i);
        p += sprintf(p, "%08x ", sdc_read(smc_host->smc_base + i));
    }
    p += sprintf(p, "\n");
    
    p += sprintf(p, "Dump ccmu regs:\n");
    for (i=0; i<0x200; i+=4)
    {
        if (!(i&0xf))
            p += sprintf(p, "\n0x%08x : ", i);
        p += sprintf(p, "%08x ", sdc_read(SW_VA_CCM_IO_BASE + i)); 
    }
    p += sprintf(p, "\n");

    p += sprintf(p, "Dump gpio regs:\n");
    for (i=0; i<0x200; i+=4)
    {
        if (!(i&0xf))
            p += sprintf(p, "\n0x%08x : ", i);
        p += sprintf(p, "%08x ", sdc_read(SW_VA_PORTC_IO_BASE+ i));
    }
    p += sprintf(p, "\n");


    return p - page;
}

static int awsmc_proc_read_dbglevel(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    char *p = page;

    p += sprintf(p, "debug-level : %d\n", smc_debug);
    return p - page;
}

static int awsmc_proc_write_dbglevel(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
    smc_debug = simple_strtoul(buffer, NULL, 10);

    return sizeof(smc_debug);
}

static int awsmc_proc_read_insert_status(char *page, char **start, off_t off, int coutn, int *eof, void *data)
{
	char *p = page; 
    struct awsmc_host *smc_host = (struct awsmc_host *)data;

	p += sprintf(p, "Usage: \"echo 1 > insert\" to scan card and \"echo 0 > insert\" to remove card\n");
	if (smc_host->cd_mode != CARD_DETECT_BY_FS)
	{
		p += sprintf(p, "Sorry, this node if only for manual attach mode(cd mode 4)\n");
	}

	p += sprintf(p, "card attach status: %s\n", smc_host->present ? "inserted" : "removed");


	return p - page;
}

static int awsmc_proc_card_insert_ctrl(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	u32 insert = simple_strtoul(buffer, NULL, 10);
    struct awsmc_host *smc_host = (struct awsmc_host *)data;
	u32 present = insert ? 1 : 0;

	if (smc_host->present ^ present)
	{
		smc_host->present = present;
		mmc_detect_change(smc_host->mmc, msecs_to_jiffies(300));
	}

	return sizeof(insert);
}

static inline void awsmc_procfs_attach(struct awsmc_host *smc_host)
{
    struct device *dev = &smc_host->pdev->dev;
    char awsmc_proc_rootname[16] = {0};

    //make mmc dir in proc fs path
    sprintf(awsmc_proc_rootname, "driver/%s", dev_name(dev));
    smc_host->proc_root = proc_mkdir(awsmc_proc_rootname, NULL);
    if (IS_ERR(smc_host->proc_root))
    {
        awsmc_msg("%s: failed to create procfs \"driver/mmc\".\n", dev_name(dev));
    }

    smc_host->proc_drvver = create_proc_read_entry("drv-version", 0444, smc_host->proc_root, awsmc_proc_read_drvversion, NULL);
    if (IS_ERR(smc_host->proc_root))
    {
        awsmc_msg("%s: failed to create procfs \"drv-version\".\n", dev_name(dev));
    }

    smc_host->proc_hostinfo = create_proc_read_entry("hostinfo", 0444, smc_host->proc_root, awsmc_proc_read_hostinfo, smc_host);
    if (IS_ERR(smc_host->proc_hostinfo))
    {
        awsmc_msg("%s: failed to create procfs \"hostinfo\".\n", dev_name(dev));
    }

    smc_host->proc_regs = create_proc_read_entry("register", 0444, smc_host->proc_root, awsmc_proc_read_regs, smc_host);
    if (IS_ERR(smc_host->proc_regs))
    {
        awsmc_msg("%s: failed to create procfs \"hostinfo\".\n", dev_name(dev));
    }

    smc_host->proc_dbglevel = create_proc_entry("debug-level", 0644, smc_host->proc_root);
    if (IS_ERR(smc_host->proc_dbglevel))
    {
        awsmc_msg("%s: failed to create procfs \"debug-level\".\n", dev_name(dev));
    }
    smc_host->proc_dbglevel->data = smc_host;
    smc_host->proc_dbglevel->read_proc = awsmc_proc_read_dbglevel;
    smc_host->proc_dbglevel->write_proc = awsmc_proc_write_dbglevel;

	smc_host->proc_insert = create_proc_entry("insert", 0644, smc_host->proc_root);
	if (IS_ERR(smc_host->proc_insert))
	{
		awsmc_msg("%s: failed to create procfs \"insert\".\n", dev_name(dev));
	}
	smc_host->proc_insert->data = smc_host;
	smc_host->proc_insert->read_proc = awsmc_proc_read_insert_status;
	smc_host->proc_insert->write_proc = awsmc_proc_card_insert_ctrl;

}

static inline void awsmc_procfs_remove(struct awsmc_host *smc_host)
{
    struct device *dev = &smc_host->pdev->dev;
    char awsmc_proc_rootname[16] = {0};
    sprintf(awsmc_proc_rootname, "driver/%s", dev_name(dev));

    remove_proc_entry("insert", smc_host->proc_root);
    remove_proc_entry("debug-level", smc_host->proc_root);
    remove_proc_entry("register", smc_host->proc_root);
    remove_proc_entry("hostinfo", smc_host->proc_root);
    remove_proc_entry("drv-version", smc_host->proc_root);
    remove_proc_entry(awsmc_proc_rootname, NULL);
}

#else

static inline void awsmc_procfs_attach(struct awsmc_host *smc_host) { }
static inline void awsmc_procfs_remove(struct awsmc_host *smc_host) { }

#endif

static struct mmc_host_ops awsmc_ops = {
    .request	     = awsmc_request,
    .set_ios	     = awsmc_set_ios,
    .get_ro		     = awsmc_get_ro,
    .get_cd		     = awsmc_card_present,
    .enable_sdio_irq = awsmc_enable_sdio_irq
};

static int __devinit awsmc_probe(struct platform_device *pdev)
{
    struct awsmc_host *smc_host = NULL;
    struct mmc_host	*mmc = NULL;
    int ret = 0;
    char* mmc_para[4] = {"mmc0_para", "mmc1_para", "mmc2_para", "mmc3_para"};
    int card_detmode = 0;

    awsmc_msg("%s: pdev->name: %s, pdev->id: %08x\n", dev_name(&pdev->dev), pdev->name, pdev->id);
    mmc = mmc_alloc_host(sizeof(struct awsmc_host), &pdev->dev);
    if (!mmc)
    {
        awsmc_msg("mmc alloc host failed\n");
    	ret = -ENOMEM;
    	goto probe_out;
    }

    smc_host = mmc_priv(mmc);
    memset((void*)smc_host, 0, sizeof(smc_host));
    smc_host->mmc = mmc;
    smc_host->pdev = pdev;

    spin_lock_init(&smc_host->lock);
    tasklet_init(&smc_host->tasklet, awsmc_tasklet, (unsigned long) smc_host);

    smc_host->cclk  = 400000;
    smc_host->mod_clk   = pdev->id == 3 ? SMC_3_MAX_MOD_CLOCK : smc_mod_clock;
    smc_host->clk_source = smc_mclk_source;
    smc_host->ops.send_request = sdxc_request;
    smc_host->ops.finalize_requset = sdxc_request_done;
    smc_host->ops.check_status = sdxc_check_status;
    
    mmc->ops        = &awsmc_ops;
    mmc->ocr_avail	= MMC_VDD_32_33 | MMC_VDD_33_34;
    mmc->caps	    = MMC_CAP_4_BIT_DATA|MMC_CAP_MMC_HIGHSPEED|MMC_CAP_SD_HIGHSPEED|MMC_CAP_SDIO_IRQ;
    mmc->f_min 	    = 200000;
    mmc->f_max 	    = pdev->id == 3 ? SMC_3_MAX_IO_CLOCK :  smc_io_clock;
    if (pdev->id==3 && (mmc_pm_get_mod_type()==2 || mmc_pm_get_mod_type()==5))
        mmc->pm_flags   = MMC_PM_IGNORE_PM_NOTIFY;

    mmc->max_blk_count	= 0xffff;
    mmc->max_blk_size	= 0xffff;
    mmc->max_req_size	= 0x800000;              //32bit byte counter = 2^32 - 1
    mmc->max_seg_size	= mmc->max_req_size;

    mmc->max_phys_segs	= 256;
    mmc->max_hw_segs	= 256;

    if (awsmc_resource_request(smc_host))
    {
        awsmc_msg("%s: Failed to get resouce.\n", dev_name(&pdev->dev));
        goto probe_free_host;
    }
    if (awsmc_set_src_clk(smc_host))
    {
        goto probe_free_resource;
    }
    awsmc_init_controller(smc_host);
    smc_host->power_on = 1;
    
//    awsmc_debugfs_attach(smc_host);
    awsmc_procfs_attach(smc_host);

    platform_set_drvdata(pdev, mmc);

    /* irq */
    smc_host->irq = platform_get_irq(pdev, 0);
    if (smc_host->irq == 0)
    {
    	dev_err(&pdev->dev, "Failed to get interrupt resouce.\n");
    	ret = -EINVAL;
    	goto probe_free_resource;
    }

    if (request_irq(smc_host->irq, awsmc_irq, 0, DRIVER_NAME, smc_host))
    {
    	dev_err(&pdev->dev, "Failed to request smc card interrupt.\n");
    	ret = -ENOENT;
    	goto probe_free_resource;
    }
    disable_irq(smc_host->irq);

    /* add host */
    ret = mmc_add_host(mmc);
    if (ret)
    {
    	dev_err(&pdev->dev, "Failed to add mmc host.\n");
    	goto probe_free_irq;
    }

    awsmc_dbg("SMC Probe: mapped smc_base:%p irq:%u dma:%u.\n", smc_host->smc_base, smc_host->irq, smc_host->dma_no);

    //fetch card detecetd mode
    ret = script_parser_fetch(mmc_para[pdev->id], "sdc_detmode", &card_detmode, sizeof(int));
    if (ret)
    {
    	awsmc_msg("sdc fetch card detect mode failed\n");
    }
    #ifdef AW1623_FPGA
    ret = 0;//fpga
    #endif

    //config card gpio to input
    smc_host->cd_mode = card_detmode;
    if (smc_host->cd_mode == CARD_DETECT_BY_GPIO)
    {
        //initial card detect timer
        init_timer(&smc_host->cd_timer);
        smc_host->cd_timer.expires = jiffies + 1*HZ;
        smc_host->cd_timer.function = &awsmc_cd_timer;
        smc_host->cd_timer.data = (unsigned long)smc_host;
        add_timer(&smc_host->cd_timer);
        smc_host->present = 0;
    }

    awsmc_msg("%s: Initialisation Done. ret %d\n", dev_name(&pdev->dev), ret);

    enable_irq(smc_host->irq);

    #ifdef AW1623_FPGA
    smc_host->cd_mode = CARD_ALWAYS_PRESENT;//fpga
    #endif
    if (smc_host->cd_mode == CARD_ALWAYS_PRESENT)
    {
        mmc_detect_change(smc_host->mmc, msecs_to_jiffies(300));
    }
    sw_host[pdev->id] = smc_host;

    goto probe_out;

probe_free_irq:
    if (smc_host->irq)
    {
        free_irq(smc_host->irq, smc_host);
    }

probe_free_resource:
    awsmc_resource_release(smc_host);

probe_free_host:
    mmc_free_host(mmc);

probe_out:
    return ret;
}

static void awsmc_shutdown(struct platform_device *pdev)
{
    struct mmc_host    *mmc = platform_get_drvdata(pdev);
    struct awsmc_host *smc_host = mmc_priv(mmc);

    awsmc_msg("%s: ShutDown.\n", dev_name(&pdev->dev));

//    awsmc_debugfs_remove(smc_host);
    awsmc_procfs_remove(smc_host);
//    awsmc_cpufreq_deregister(smc_host);
    mmc_remove_host(mmc);
}

static int __devexit awsmc_remove(struct platform_device *pdev)
{
    struct mmc_host    	*mmc  = platform_get_drvdata(pdev);
    struct awsmc_host	*smc_host = mmc_priv(mmc);

    awsmc_msg("%s: Remove.\n", dev_name(&pdev->dev));
	
	sdxc_exit(smc_host);

	awsmc_shutdown(pdev);

    //dma
    tasklet_disable(&smc_host->tasklet);

    //irq
    free_irq(smc_host->irq, smc_host);

    if (smc_host->cd_mode == CARD_DETECT_BY_GPIO)
    {
        del_timer(&smc_host->cd_timer);
    }

    awsmc_resource_release(smc_host);
    
    mmc_free_host(mmc);
    sw_host[pdev->id] = NULL;

    return 0;
}

#ifdef CONFIG_PM
static int awsmc_suspend(struct device *dev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct mmc_host *mmc = platform_get_drvdata(pdev);
    int ret = 0;
    
    if (mmc)
    {
        struct awsmc_host *smc_host = mmc_priv(mmc);
        
        if (mmc->card && (mmc->card->type!=MMC_TYPE_SDIO || (mmc_pm_get_mod_type()!=2 && mmc_pm_get_mod_type()!=5)))
            ret = mmc_suspend_host(mmc);
            
        if (smc_host->power_on) {
            /* disable irq */
            disable_irq(smc_host->irq);
    
            /* backup registers */
            sdxc_regs_save(smc_host);
    
        	/* disable mmc mclk */
        	clk_disable(smc_host->mclk);
    
        	/* disable mmc hclk */
            if (mmc->card && mmc->card->type!=MMC_TYPE_SDIO)
        	    clk_disable(smc_host->hclk);
            
            /* suspend pins to save power */
            mmc_suspend_pins(smc_host);
        }
    }

    awsmc_msg("smc %d suspend\n", pdev->id);
    return ret;
}

static int awsmc_resume(struct device *dev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct mmc_host *mmc = platform_get_drvdata(pdev);
    int ret = 0;

    if (mmc)
    {
        struct awsmc_host *smc_host = mmc_priv(mmc);
        
        if (smc_host->power_on) {
            /* resume pins to correct status */
            mmc_resume_pins(smc_host);
            
        	/* enable mmc hclk */
            if (mmc->card && mmc->card->type!=MMC_TYPE_SDIO)
        	    clk_enable(smc_host->hclk);
    
        	/* enable mmc mclk */
        	clk_enable(smc_host->mclk);
    
            /* restore registers */
            if (mmc->card && mmc->card->type!=MMC_TYPE_SDIO)
                sdxc_regs_restore(smc_host);
            sdxc_program_clk(smc_host);
            
            /* enable irq */
            enable_irq(smc_host->irq);
        }
    
        if (mmc->card && (mmc->card->type!=MMC_TYPE_SDIO || mmc_pm_get_mod_type()!=2))
            ret = mmc_resume_host(mmc);
    }

    awsmc_msg("smc %d resume\n", pdev->id);
    return ret;
}

static const struct dev_pm_ops awsmc_pm = {
    .suspend	= awsmc_suspend,
    .resume		= awsmc_resume,
};
#define awsmc_pm_ops &awsmc_pm

#else /* CONFIG_PM */

#define awsmc_pm_ops NULL

#endif /* CONFIG_PM */

static struct resource awsmc0_resources[] = {
    [0] = {
    	.start	= SMC0_BASE,
    	.end	= SMC0_BASE+0x400,
    	.flags	= IORESOURCE_MEM,
    },
    [1] = {
    	.start	= INTC_IRQNO_SMC0,
    	.end	= INTC_IRQNO_SMC0,
    	.flags	= IORESOURCE_IRQ,
    },
};

static struct resource awsmc1_resources[] = {
    [0] = {
    	.start	= SMC1_BASE,
    	.end	= SMC1_BASE+0x400,
    	.flags	= IORESOURCE_MEM,
    },
    [1] = {
    	.start	= INTC_IRQNO_SMC1,
    	.end	= INTC_IRQNO_SMC1,
    	.flags	= IORESOURCE_IRQ,
    },
};

static struct resource awsmc2_resources[] = {
    [0] = {
    	.start	= SMC2_BASE,
    	.end	= SMC2_BASE+0x400,
    	.flags	= IORESOURCE_MEM,
    },
    [1] = {
    	.start	= INTC_IRQNO_SMC2,
    	.end	= INTC_IRQNO_SMC2,
    	.flags	= IORESOURCE_IRQ,
    },
};

static struct resource awsmc3_resources[] = {
    [0] = {
    	.start	= SMC3_BASE,
    	.end	= SMC3_BASE+0x400,
    	.flags	= IORESOURCE_MEM,
    },
    [1] = {
    	.start	= INTC_IRQNO_SMC3,
    	.end	= INTC_IRQNO_SMC3,
    	.flags	= IORESOURCE_IRQ,
    },
};

static struct platform_device axmmc_device[] = {
    [0] = {
        .name           = "awsmc",
        .id             = 0,
        .num_resources	= ARRAY_SIZE(awsmc0_resources),
        .resource       = awsmc0_resources,
        .dev            = {
        }
    },
    [1] = {
        .name           = "awsmc",
        .id             = 1,
        .num_resources	= ARRAY_SIZE(awsmc1_resources),
        .resource       = awsmc1_resources,
        .dev            = {
        }
    },
    [2] = {
        .name           = "awsmc",
        .id             = 2,
        .num_resources	= ARRAY_SIZE(awsmc2_resources),
        .resource       = awsmc2_resources,
        .dev            = {
        }
    },
    [3] = {
        .name           = "awsmc",
        .id             = 3,
        .num_resources	= ARRAY_SIZE(awsmc3_resources),
        .resource       = awsmc3_resources,
        .dev            = {
        }
    }
};

static struct platform_driver awsmc_driver = {
    .driver.name    = "awsmc",
    .driver.owner   = THIS_MODULE,
    .driver.pm	    = awsmc_pm_ops,
    .probe          = awsmc_probe,
    .remove         = __devexit_p(awsmc_remove),
//    .shutdown       = awsmc_shutdown,
};

static int __init awsmc_init(void)
{
    int ret;

    awsmc_msg("awsmc_init\n");
    
    memset((void*)sdc_used, 0, sizeof(sdc_used));
    ret = script_parser_fetch("mmc0_para","sdc_used", &sdc_used[0], sizeof(int));
    if (ret)
    {
    	printk("awsmc_init fetch mmc0 using configuration failed\n");
    }
    ret = script_parser_fetch("mmc1_para","sdc_used", &sdc_used[1], sizeof(int));
    if (ret)
    {
        printk("awsmc_init fetch mmc1 using onfiguration failed\n");
    }
    ret = script_parser_fetch("mmc2_para","sdc_used", &sdc_used[2], sizeof(int));
    if (ret)
    {
        printk("awsmc_init fetch mmc2 using configuration failed\n");
    }
    ret = script_parser_fetch("mmc3_para","sdc_used", &sdc_used[3], sizeof(int));
    if (ret)
    {
        printk("awsmc_init fetch mmc3 using configuration failed\n");
    }

    awsmc_msg("awsmc controller unsing config sdc0 %d, sdc1 %d, sdc2 %d, sdc3 %d\n", sdc_used[0], sdc_used[1], sdc_used[2], sdc_used[3]);
    #ifdef AW1623_FPGA
    sdc_used[0] = 1;//fpga
    #endif
    
    if (sdc_used[0])
    {
        platform_device_register(&axmmc_device[0]);
    }
    if (sdc_used[1])
    {
        platform_device_register(&axmmc_device[1]);
    }
    if (sdc_used[2])
    {
        platform_device_register(&axmmc_device[2]);
    }
    if (sdc_used[3])
    {
        platform_device_register(&axmmc_device[3]);
    }
    if (sdc_used[0] || sdc_used[1] || sdc_used[2] || sdc_used[3])
    {
        return platform_driver_register(&awsmc_driver);
    }
    else
    {
        awsmc_msg("cannot find any using configuration for controllers, return directly!\n");
        return 0;
    }
}

static void __exit awsmc_exit(void)
{
    if (sdc_used[0] || sdc_used[1] || sdc_used[2] || sdc_used[3])
    {
        sdc_used[0] = 0;
        sdc_used[1] = 0;
        sdc_used[2] = 0;
        sdc_used[3] = 0;
        platform_driver_unregister(&awsmc_driver);
    }
}


module_init(awsmc_init);
module_exit(awsmc_exit);

MODULE_DESCRIPTION("SUN4I SD/MMC Card Controller Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aaron.maoye<leafy.myeh@allwinnertech.com>");
MODULE_ALIAS("platform:aw-smc");
