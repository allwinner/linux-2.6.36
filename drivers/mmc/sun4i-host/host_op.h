/*
*********************************************************************************************************
*                                                    eBase
*                                           the Abstract of Hardware
*
*                                   (c) Copyright 2006-2010, holigun China
*                                                All Rights Reserved
*
* File        :  host_op.h
* Date        :  2012-11-25 14:24:12
* Author      :  Aaron.Maoye
* Version     :  v1.00
* Description:
*                            Register-based Operation for SD3.0 Controller module, aw1620
* History     :
*      <author>         <time>                  <version >      <desc>
*       Maoye           2012-11-25 14:25:25     1.0             create this file
*
*********************************************************************************************************
*/
#ifndef _SUN4I_HOST_OP_H_
#define _SUN4I_HOST_OP_H_ "host_op.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
//#include <linux/cpufreq.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>

#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>

#include <asm/cacheflush.h>
#include <mach/dma.h>
//#include <mach/clock.h>
//#include <mach/gpio.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

//#define AW1623_FPGA

#define DRIVER_NAME "sw-smc"

#define SMC_MAX_MOD_CLOCK		(42000000)
#define SMC_MAX_IO_CLOCK		(42000000)

enum mclk_src {
	SMC_MCLK_SRC_HOSC,
	SMC_MCLK_SRC_SATAPLL,
	SMC_MCLK_SRC_DRAMPLL
};
#define SMC_3_MAX_MOD_CLOCK     90000000
#define SMC_3_MAX_IO_CLOCK      45000000
#define SMC_3_MOD_CLK_SRC       SMC_MCLK_SRC_DRAMPLL

#define CARD_DETECT_BY_GPIO     (1)
#define CARD_DETECT_BY_DATA3    (2)        /* mmc detected by status of data3 */
#define CARD_ALWAYS_PRESENT     (3)        /* mmc always present, without detect pin */
#define CARD_DETECT_BY_FS		(4)		   /* mmc insert/remove by manual mode, from /proc/awsmc.x/insert node */
/* SDMMC Control registers definition */
#define  SMC0_BASE              0x01C0f000
#define  SMC1_BASE              0x01C10000
#define  SMC2_BASE              0x01C11000
#define  SMC3_BASE              0x01C12000

/* interrupt number */
#define  INTC_IRQNO_SMC0       32
#define  INTC_IRQNO_SMC1       33
#define  INTC_IRQNO_SMC2       34
#define  INTC_IRQNO_SMC3       35

/* register operation */
#define  sdc_write(addr, val)   writel(val, addr)
#define  sdc_read(addr)         readl(addr)

#ifdef CONFIG_SUN4I_MMC_SW_DBG         
#define SMC_DBG         (1 << 0)
#define SMC_DBG_ERR     (1 << 1)
#define SMC_DBG_INFO	(1 << 2)
#define awsmc_info(...)  do {                                        \
                            if(smc_debug&SMC_DBG_INFO) {                 \
                                printk("[mmc]: "__VA_ARGS__);  \
                            }                                       \
                        } while(0)
#define awsmc_dbg(...)  do {                                        \
                            if(smc_debug&SMC_DBG) {                 \
                                printk("[mmc]: "__VA_ARGS__);  \
                            }                                       \
                        } while(0)
#define awsmc_dbg_err(...)  do {                                                            \
                                if(smc_debug&SMC_DBG_ERR) {                                 \
                                    printk("[mmc]: %s(L%d): ", __FUNCTION__, __LINE__);  \
                                    printk(__VA_ARGS__);                                  \
                                }                                                           \
                            } while(0)
#define awsmc_msg(...)  do {printk("[mmc]: "__VA_ARGS__);} while(0)

#define ENTER()  do {if(smc_debug&SMC_DBG) printk("[mmc]:-Enter-: %s, %s:%d\n", __FUNCTION__, __FILE__, __LINE__);} while(0)
#define LEAVE()  do {if(smc_debug&SMC_DBG) printk("[mmc]:-Leave-: %s, %s:%d\n", __FUNCTION__, __FILE__, __LINE__);} while(0)

#else  //#ifdef AWSMC_DBG
#define awsmc_info(...)
#define awsmc_dbg(...)
#define awsmc_dbg_err(...) do {                                                        \
                                printk("[mmc]: %s(L%d): ", __FUNCTION__, __LINE__);  \
                                printk(__VA_ARGS__);                                  \
                            } while(0)
#define awsmc_msg(...) do {printk("[mmc]: "__VA_ARGS__);} while(0)
#define ENTER()
#define LEAVE()

#endif  //#ifdef AWSMC_DBG


struct awsmc_pdata {
	unsigned int	detect_invert : 1;   /* set => detect active high. */

	unsigned int	gpio_detect;
	unsigned long	ocr_avail;
	void            (*set_power)(unsigned char power_mode, unsigned short vdd);
};

struct awsmc_host;
struct mmc_request;
struct smc_idma_des;

typedef struct {
    void    (*send_request)(struct awsmc_host* host, struct mmc_request* request);
    s32     (*finalize_requset)(struct awsmc_host* host);
    void    (*check_status)(struct awsmc_host* host);
} sdc_req_ops;

struct awsmc_ctrl_regs {
	u32		gctrl;
	u32		clkc;
	u32		timeout;
	u32		buswid;
	u32		waterlvl;
	u32		funcsel;
	u32		debugc;
	u32		idmacc;
};

struct sw_sdio_res {
    u32     used;
    char    mname[32];
    u32     sdc_id;
    u32     pio_hdle;
    u32     poweron;
    u32     suspend;
};

struct awsmc_host {
    
    struct platform_device      *pdev;
    struct mmc_host             *mmc;
    
    void __iomem	            *smc_base;          /* sdc I/O base address  */       
     
    struct resource	            *smc_base_res;      /* resources found       */
    
    /* clock management */
    struct clk                  *hclk;              //
    struct clk                  *mclk;              //
    u32                         clk_source;         // clock, source, 0-video pll, 1-ac320 pll
    
    u32                         power_on;         // power save, 0-normal, 1-power save
    u32                         power_save;         // power save, 0-normal, 1-power save
    u32                         mod_clk;            // source clock of controller
    u32                         cclk;               // requested card clock frequence
    u32                         real_cclk;          // real card clock to output
    u32                         bus_width;
    
    /* irq */
    int                         irq;                // irq number
    volatile u32				irq_flag;
    volatile u32                sdio_int;
    volatile u32                int_sum;
    
    int                         dma_no;             //dma number
    volatile u32                dodma;              //transfer with dma mode
    volatile u32                todma;
    volatile u32                dma_done;           //dma complete
    volatile u32                ahb_done;           //dma complete
    volatile u32                dataover;           //dma complete
    struct smc_idma_des*        pdes;
    
    struct mmc_request	        *mrq;
    sdc_req_ops                 ops;
    
    volatile u32                with_autostop;
    volatile u32                wait;
#define SDC_WAIT_NONE           (1<<0)
#define SDC_WAIT_CMD_DONE       (1<<1)
#define SDC_WAIT_DATA_OVER      (1<<2)
#define SDC_WAIT_AUTOCMD_DONE   (1<<3)
#define SDC_WAIT_READ_DONE      (1<<4)
#define SDC_WAIT_DMA_ERR        (1<<5)
#define SDC_WAIT_ERROR          (1<<6)
#define SDC_WAIT_FINALIZE       (1<<7)

    volatile u32                error;
    spinlock_t		            lock;
	struct tasklet_struct       tasklet;
	
    volatile u32                present;
    volatile u32                change;
    
    struct timer_list           cd_timer;
    s32                         cd_gpio;
    s32                         cd_mode;
    u32                         pio_hdle;
    u32                         read_only;
    
#ifdef CONFIG_CPU_FREQ
	struct notifier_block       freq_transition;
#endif
/*
#ifdef CONFIG_DEBUG_FS
	struct dentry		        *debug_root;
	struct dentry		        *driver_version;
	struct dentry		        *host_info;
	struct dentry		        *debug_level;
	struct dentry		        *debug_regs;
#endif
*/
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry		*proc_root;
	struct proc_dir_entry		*proc_drvver;
	struct proc_dir_entry		*proc_hostinfo;
	struct proc_dir_entry		*proc_dbglevel;
	struct proc_dir_entry		*proc_regs;
	struct proc_dir_entry		*proc_insert;
#endif

	/* backup register structrue */
	struct awsmc_ctrl_regs		bak_regs;
	user_gpio_set_t             bak_gpios[6];
	u32                         gpio_suspend_ok;
};


static __inline void eLIBs_CleanFlushDCacheRegion(void *adr, __u32 bytes)
{
	__cpuc_flush_dcache_area(adr, bytes + (1 << 5) * 2 - 2);
}

#define MEM_ADDR_IN_SDRAM(addr) ((addr) >= 0x80000000)

#define RESSIZE(res)        (((res)->end - (res)->start)+1)

void awsmc_dumpreg(struct awsmc_host* smc_host);

#include <mach/platform.h>
#if 0 
static inline void uart_send_char(char c)
{
    while (!(sdc_read(SW_VA_UART0_IO_BASE + 0x7c) &  (1<<1)));
	sdc_write(SW_VA_UART0_IO_BASE, c);
}
#else
#define uart_send_char(c) 
#endif

int sw_get_sdio_resource(void);
int sw_put_sdio_resource(void);
int sw_sdio_powerup(char* mname);
int sw_sdio_poweroff(char* mname);

#endif
