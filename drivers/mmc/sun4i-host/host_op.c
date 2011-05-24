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
#include <asm/uaccess.h>
#include <mach/clock.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

static char* awsmc_hclk_name[4] = {"ahb_sdc0", "ahb_sdc1", "ahb_sdc2", "ahb_sdc3"};
static char* awsmc_mclk_name[4] = {"sdc0", "sdc1", "sdc2", "sdc3"};

/* Module parameters */
/* debug control */
unsigned int smc_debug = 2;
EXPORT_SYMBOL_GPL(smc_debug);
module_param_named(awsmc_debug, smc_debug, int, 0);

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
    char* name;
    int ret;
    
    switch (smc_host->clk_source)
    {
        case 0:
        case 1:
            source_clock = clk_get(&smc_host->pdev->dev, "video_pll0");
            name = "video_pll0";
            break;
        case 2:
            source_clock = clk_get(&smc_host->pdev->dev, "sdram_pll");
            name = "sdram_pll";
            break;
        case 3:
            source_clock = clk_get(&smc_host->pdev->dev, "core_pll");
            name = "core_pll";
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
	smc_syscall_ioremap();
	
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
    return 0;
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
	    awsmc_dbg("card detect change, present %d\n", present);
        smc_host->present = present;
        mmc_detect_change(smc_host->mmc, msecs_to_jiffies(300));
    } else {
//        awsmc_dbg("card detect no change\n");
    }
    
    mod_timer(&smc_host->cd_timer, jiffies + 100);
	return;
}

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
		awsmc_dbg("- sdio int -\n");
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
    		break;
    	case MMC_POWER_OFF:
    	default:
    		break;
	}
	
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

static void awsmc_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
    struct awsmc_host *smc_host = mmc_priv(mmc);
	unsigned long flags;

	spin_lock_irqsave(&smc_host->lock, flags);	
    sdxc_enable_sdio_irq(smc_host, enable);
	spin_unlock_irqrestore(&smc_host->lock, flags);
}

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

#ifdef CONFIG_DEBUG_FS

#define AWSMC_DRV_VERSION "0.03"

/* driver version show */
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

/* driver control info. show */
static int awsmc_hostinfo_show(struct seq_file * seq, void *data)
{
	struct awsmc_host *smc_host = (struct awsmc_host *)seq->private;
	struct device* dev = &smc_host->pdev->dev;
	char* clksrc[] = {"videopll0", "videopll0", "sdrampll", "corepll"};
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

/* register show */
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
	unsigned long iflags;
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
	
    spin_lock_irqsave(&smc_host->lock, iflags);
    
    if (awsmc_resource_request(smc_host))
    {
        awsmc_msg("%s: Failed to get resouce.\n", dev_name(&pdev->dev));
        goto probe_free_host;
    }
	
    smc_host->cclk  = 400000;
    smc_host->mod_clk   = 95000000;
    smc_host->clk_source = 0;
    smc_host->ops.send_request = sdxc_request;
	smc_host->ops.finalize_requset = sdxc_request_done;
	smc_host->ops.check_status = sdxc_check_status;
    
    if (awsmc_set_src_clk(smc_host))
    {
        goto probe_free_host;
    }
    awsmc_init_controller(smc_host);
    
	spin_unlock_irqrestore(&smc_host->lock, iflags);
	
	mmc->ops        = &awsmc_ops;
	mmc->ocr_avail	= MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->caps	    = MMC_CAP_4_BIT_DATA|MMC_CAP_MMC_HIGHSPEED|MMC_CAP_SD_HIGHSPEED|MMC_CAP_SDIO_IRQ;
	mmc->f_min 	    = 200000;
	mmc->f_max 	    = 50000000;
	
	mmc->max_blk_count	= 0xffff;
	mmc->max_blk_size	= 0xffff;
	mmc->max_req_size	= 0x800000;              //32bit byte counter = 2^32 - 1
	mmc->max_seg_size	= mmc->max_req_size;
	
	mmc->max_phys_segs	= 128;
	mmc->max_hw_segs	= 128;
	
    /* add host */
	ret = mmc_add_host(mmc);
	if (ret) 
	{
		dev_err(&pdev->dev, "Failed to add mmc host.\n");
		goto probe_free_irq;
	}

	awsmc_debugfs_attach(smc_host);
	
	platform_set_drvdata(pdev, mmc);
	
    /* irq */
	smc_host->irq = platform_get_irq(pdev, 0);
	if (smc_host->irq == 0) 
	{
		dev_err(&pdev->dev, "Failed to get interrupt resouce.\n");
		ret = -EINVAL;
		goto probe_free_resource;
	}

	if (request_irq(smc_host->irq, awsmc_irq, IRQF_SHARED, DRIVER_NAME, smc_host)) 
	{
		dev_err(&pdev->dev, "Failed to request smc card interrupt.\n");
		ret = -ENOENT;
		goto probe_free_irq;
	}
	disable_irq(smc_host->irq);
	
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
    struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct awsmc_host *smc_host = mmc_priv(mmc);
    
    awsmc_msg("%s: ShutDown.\n", dev_name(&pdev->dev));   
    
	awsmc_debugfs_remove(smc_host);
//	awsmc_cpufreq_deregister(smc_host);
	mmc_remove_host(mmc);
}

static int __devexit awsmc_remove(struct platform_device *pdev)
{
    struct mmc_host		*mmc  = platform_get_drvdata(pdev);
	struct awsmc_host	*smc_host = mmc_priv(mmc);

    awsmc_msg("%s: Remove.\n", dev_name(&pdev->dev)); 
    
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
	return 0;
}

#ifdef CONFIG_PM
static int awsmc_suspend(struct device *dev)
{
	struct mmc_host *mmc = platform_get_drvdata(to_platform_device(dev));

    if (mmc) 
    {
	    return  mmc_suspend_host(mmc);
	}
	return 0;
}

static int awsmc_resume(struct device *dev)
{
	struct mmc_host *mmc = platform_get_drvdata(to_platform_device(dev));

    if (mmc) 
    {
	    return mmc_resume_host(mmc);
	}
	return 0;
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
	.shutdown       = awsmc_shutdown,
};

static int __init awsmc_init(void)
{
	int sdc_used[4] = {0};
	int ret;
	
	awsmc_msg("awsmc_init\n");
	
	ret = script_parser_fetch("mmc0_para","sdc_used", &sdc_used[0], sizeof(int));
	if (ret)
	{
		printk("awsmc_init fetch mmc0 using configuration failed\n");
		#ifndef AW1623_FPGA
		return -1;//fpga
		#endif
	}	
	ret = script_parser_fetch("mmc1_para","sdc_used", &sdc_used[1], sizeof(int));
	if (ret)
    {
        printk("awsmc_init fetch mmc1 using onfiguration failed\n");
		#ifndef AW1623_FPGA
		return -1;//fpga
		#endif
    }       
	ret = script_parser_fetch("mmc2_para","sdc_used", &sdc_used[2], sizeof(int));
	if (ret)
    {
        printk("awsmc_init fetch mmc2 using configuration failed\n");
		#ifndef AW1623_FPGA
		return -1;//fpga
		#endif
    }       
	ret = script_parser_fetch("mmc3_para","sdc_used", &sdc_used[3], sizeof(int));
	if (ret)
    {
        printk("awsmc_init fetch mmc3 using configuration failed\n");
		#ifndef AW1623_FPGA
		return -1;//fpga
		#endif
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
    
	return platform_driver_register(&awsmc_driver);
}

static void __exit awsmc_exit(void)
{
	platform_driver_unregister(&awsmc_driver);
}


module_init(awsmc_init);
module_exit(awsmc_exit);

MODULE_DESCRIPTION("SUN4I SD/MMC Card Controller Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aaron.maoye<leafy.myeh@allwinnertech.com>");
MODULE_ALIAS("platform:aw-smc");
