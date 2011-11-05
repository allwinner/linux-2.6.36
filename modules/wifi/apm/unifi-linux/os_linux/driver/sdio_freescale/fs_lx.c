/*
 * Freescale SDIO glue modules.
 *
 * Copyright (C) 2008 Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 *  Note:
 *      The Freescale MMC/SDIO driver is a project under development so the
 *      code that interfaces their driver is likely to have changed between
 *      releases. This code is tested only with the SDIO/MMC driver released
 *      by Freescale for the imx31ads board using Linux Kernel 2.6.16.
 *
 *      Also, the code in the probe that sets the pull-ups is platform
 *      specific and should really be part of the controller's initialisation.
 *      In the aforesaid Freescale release this code was missing but may
 *      be in place in the future releases.
 *
 *  Important:
 *      This module does not support more than one device driver instances.
 *
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <linux/module.h>
#include <linux/init.h>

#include <linux/scatterlist.h>
#include <linux/mmc/card.h>
#include <linux/mmc/protocol.h>
#include <linux/mmc/host.h>

#ifdef CONFIG_MX31_3STACK
/* The 3STACK board allows control of the power supply and clock to
 * the APM module
 */
#include <linux/clk.h>
#include <asm/arch/pmic_power.h>
#include <asm/arch/mmc.h>
#include <asm/arch/gpio.h>
#endif

#include "fs_sdio_api.h"

#define CMD_RETRIES	3

#define SDIO_FBR_REG(f, r)      (0x100*(f) + (r))
#define SDIO_FBR_BLK_SIZE(f)    SDIO_FBR_REG(f, 0x10)

#define SDIO_CCCR_IO_EN          0x02
#define SDIO_CCCR_IO_READY       0x03
#define SDIO_CCCR_INT_EN         0x04
#define SDIO_CCCR_INT_PENDING    0x05
#define SDIO_CCCR_IO_ABORT       0x06
#define SDIO_CCCR_BUS_IFACE_CNTL 0x07
#define SDIO_CCCR_CIS_PTR        0x09
#define SDIO_CCCR_FN0_BLK_SZ0	 0x10
#define SDIO_CCCR_FN0_BLK_SZ1	 0x11

#define SDIO_FBR_CIS_PTR         0x109

#define CISTPL_MANFID       0x20
#define CISTPL_MANFID_SIZE  0x04
#define CISTPL_FUNCE        0x22
#define CISTPL_FUNCE_01_SIZE 0x2a
#define CISTPL_END          0xff

static int fs_sdio_probe(struct mmc_card *card);
static void fs_sdio_remove(struct mmc_card *card);
static irqreturn_t fs_sdio_irq(int irq, void *devid);
static int fs_sdio_suspend(struct mmc_card *card, pm_message_t state);
static int fs_sdio_resume(struct mmc_card *card);


EXPORT_SYMBOL(fs_sdio_register_driver);
EXPORT_SYMBOL(fs_sdio_unregister_driver);

EXPORT_SYMBOL(fs_sdio_readb);
EXPORT_SYMBOL(fs_sdio_writeb);
EXPORT_SYMBOL(fs_sdio_block_rw);
EXPORT_SYMBOL(fs_sdio_set_block_size);
EXPORT_SYMBOL(fs_sdio_set_max_clock_speed);
EXPORT_SYMBOL(fs_sdio_enable);
EXPORT_SYMBOL(fs_sdio_enable_interrupt);
EXPORT_SYMBOL(fs_sdio_hard_reset);

/* Globals to store the context to this module and the device driver */
static struct sdio_dev *available_sdio_dev = NULL;
static struct fs_driver *available_driver = NULL;



enum sdio_cmd_direction {
    CMD_READ, CMD_WRITE,
};


static int sdio_cmd52(struct mmc_card *card, int func, uint32_t addr, uint8_t *data,
                      enum sdio_cmd_direction dir)
{
    struct mmc_command cmd;
    int err;
    int rw, raw;

    if (dir == CMD_READ) {
        rw = SDIO_RW_READ;
        raw = 0;
    } else {
        rw = SDIO_RW_WRITE;
        raw = 1;
    }

    mmc_card_claim_host(card);
    cmd.opcode = SD_IO_RW_DIRECT;
    cmd.arg = IO_RW_DIRECT_ARG(rw, raw, func, addr, (rw == SDIO_RW_WRITE ? (*data) : 0));
    cmd.flags = MMC_RSP_R5 | MMC_KEEP_CLK_RUN;
    err = mmc_wait_for_cmd(card->host, &cmd, CMD_RETRIES);
    mmc_card_release_host(card);

    if (err) {
        return -EINVAL;
    }

    if (rw == SDIO_RW_READ) {
        *data = (cmd.resp[0] & 0xff);
    }

    return 0;
}


static int sdio_cmd53(struct sdio_dev *fdev, int func, uint32_t addr, uint8_t *data,
                      size_t len, enum sdio_cmd_direction dir)
{
    struct mmc_card *card = fdev->card;
    struct mmc_request mmc_req;
    struct mmc_command cmd;
    struct mmc_data mdata;
    struct scatterlist sg;

    int err;
    int rw;
    int count;

    /* Read or Write ? */
    if (dir == CMD_WRITE) {
        rw = SDIO_RW_WRITE;
    } else {
        rw = SDIO_RW_READ;
    }

    /* Calculate the request length */
    if (len >= fdev->max_blocksize) {
        count = len / fdev->max_blocksize;
    } else {
        count = len;
    }

    mmc_card_claim_host(card);
    cmd.opcode = SD_IO_RW_EXTENDED;     //CM53
    cmd.arg = IO_RW_EXTENDED_ARG(rw,
                                 len >= fdev->max_blocksize ? 1 : 0,
                                 0,
                                 func, addr, count);
    cmd.flags = MMC_RSP_R5 | MMC_RSP_BUSY | MMC_KEEP_CLK_RUN;

    cmd.retries = 0;

    mmc_req.cmd = &cmd;

    mdata.timeout_ns = 80000000;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16)
    mdata.blksz_bits = (len >= fdev->max_blocksize ? fdev->max_blocksize*8 : len*8);
#endif
    mdata.blksz = (len >= fdev->max_blocksize ? fdev->max_blocksize : len);
    mdata.blocks = (len >= fdev->max_blocksize ? count : 1);

    if (rw == SDIO_RW_WRITE) {
        mdata.flags = MMC_DATA_WRITE;
    } else {
        mdata.flags = MMC_DATA_READ;
    }

    if ((len >= fdev->max_blocksize) && (count > 1)) {
        mdata.flags |= MMC_DATA_MULTI;
    }

    /* Make a scatterlist from the buffer. */
    sg_init_one(&sg, data, len);
    mdata.sg = &sg;
    mdata.sg_len = 1;

    mdata.mrq = &mmc_req;
    mdata.stop = NULL;
    cmd.data = &mdata;
    cmd.mrq = &mmc_req;

    mmc_req.data = &mdata;
    mmc_req.stop = NULL;

    err = mmc_wait_for_req(card->host, &mmc_req);
    mmc_card_release_host(card);

    if (err || mdata.error) {
        return -EINVAL;
    }

    return 0;
}



int
fs_sdio_readb(struct sdio_dev *fdev, int funcnum, unsigned long addr,
              unsigned char *pdata)
{
    struct mmc_card *card = fdev->card;

    return sdio_cmd52(card, funcnum, addr, pdata, CMD_READ);
}


int
fs_sdio_writeb(struct sdio_dev *fdev, int funcnum, unsigned long addr,
               unsigned char data)
{
    struct mmc_card *card = fdev->card;

    return sdio_cmd52(card, funcnum, addr, &data, CMD_WRITE);
}


int
fs_sdio_block_rw(struct sdio_dev *fdev, int funcnum, unsigned long addr,
                 unsigned char *pdata, unsigned int count, int direction)
{
    int ret, remainder = 0;

    if (count > fdev->max_blocksize) {
        remainder = count % fdev->max_blocksize;
        count -= remainder;
    }

    ret = sdio_cmd53(fdev, funcnum, addr, (uint8_t *)pdata, count, direction);
    if (!ret && remainder) {
        ret = sdio_cmd53(fdev, funcnum, addr, (uint8_t *)pdata + count, remainder, direction);
    }

    return ret;
}


int
fs_sdio_enable_interrupt(struct sdio_dev *fdev, int enable)
{
    struct mmc_card *card = fdev->card;
    unsigned flags;

    spin_lock_irqsave(&fdev->lock, flags);
    if (enable) {
        if (!fdev->int_enabled) {
            fdev->int_enabled = 1;
            enable_irq(card->host->sdio_irq);
        }
    } else {
        if (fdev->int_enabled) {
            disable_irq_nosync(card->host->sdio_irq);
            fdev->int_enabled = 0;
        }
    }
    spin_unlock_irqrestore(&fdev->lock, flags);

    return 0;
}





/**
 * Read a 24 bit CIS pointer register.
 */
static int
sdio_cis_read_ptr_reg(struct mmc_card *card, uint32_t addr, uint32_t *ptr)
{
    uint32_t cis_ptr = 0;
    int b;

    for (b = 0; b < 3; b++) {
        uint8_t p;
        int ret = sdio_cmd52(card, 0, addr + b, &p, CMD_READ);
        if (ret < 0)
            return ret;
        cis_ptr |= p << (b * 8);
    }
    *ptr = cis_ptr;
    return 0;
}

/**
 * Read a CIS tuple.
 */
static int
sdio_cis_get_tuple(struct mmc_card *card, uint32_t cis_ptr, uint8_t tuple,
                   void *buf, size_t len)
{
    uint8_t *bbuf = buf;
    uint8_t tpl, lnk;

    /* find tuple */
    for(;;) {
        int ret;

        if (cis_ptr >= 0x17000) {
            /* Valid CIS should have a CISTPL_END so this shouldn't happen. */
            return -ENXIO;
        }

        ret = sdio_cmd52(card, 0, cis_ptr++, &tpl, CMD_READ);
        if (ret < 0) {
            return ret;
        }
        ret = sdio_cmd52(card, 0, cis_ptr++, &lnk, CMD_READ);
        if (ret < 0) {
            return ret;
        }

        if (tpl == CISTPL_END) {
            return -ENXIO;
        }
        if (tpl == tuple) {
            break;
        }
        cis_ptr += lnk;
    }

    if (lnk > len) {
        return -EINVAL;
    }

    /* copy tuple data */
    for (; lnk > 0; lnk--) {
        int ret;

        ret = sdio_cmd52(card, 0, cis_ptr++, bbuf++, CMD_READ);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}



static int sdio_card_read_info(struct sdio_dev *fdev)
{
    struct mmc_card *card = fdev->card;
    uint32_t cis_ptr;
    uint16_t manfid[2];
    uint8_t funce_dat[CISTPL_FUNCE_01_SIZE];
    int ret;

    /* read func CIS ptr */
    if (sdio_cis_read_ptr_reg(card, SDIO_FBR_CIS_PTR, &cis_ptr) < 0) {
        return -1;
    }
    if (cis_ptr < 0x1000 || cis_ptr > 0x17000) {
        return -1;
    }

    if (sdio_cis_get_tuple(card, cis_ptr, CISTPL_FUNCE, funce_dat, CISTPL_FUNCE_01_SIZE) < 0) {
        return -1;
    }
    fdev->max_blocksize = (funce_dat[0x0c] & 0xff) | ((funce_dat[0x0d] & 0xff) << 8);

    /* Set the function 1 block size */
    ret = fs_sdio_writeb(fdev, 0, SDIO_FBR_BLK_SIZE(1), fdev->max_blocksize & 0xFF);
    if (ret) {
        return ret;
    }
    ret = fs_sdio_writeb(fdev, 0, SDIO_FBR_BLK_SIZE(1)+1, (fdev->max_blocksize >> 8) & 0xFF);
    if (ret) {
        return ret;
    }

    /* Set the block size read from the device to the MMC driver. */
    card->csd.read_blkbits = fdev->max_blocksize*8;
    card->csd.write_blkbits = fdev->max_blocksize*8;


    /* read common CIS ptr */
    if (sdio_cis_read_ptr_reg(card, SDIO_CCCR_CIS_PTR, &cis_ptr) < 0) {
        return -1;
    }
    if (cis_ptr < 0x1000 || cis_ptr > 0x17000) {
        return -1;
    }
    /* read manfid from CIS */
    if (sdio_cis_get_tuple(card, cis_ptr, CISTPL_MANFID, &manfid, CISTPL_MANFID_SIZE) < 0) {
        return -1;
    }
    fdev->vendor_id = le16_to_cpu(manfid[0]);

    /* read common CIS ptr */
    if (sdio_cis_read_ptr_reg(card, SDIO_CCCR_CIS_PTR, &cis_ptr) < 0) {
        return -1;
    }
    if (cis_ptr < 0x1000 || cis_ptr > 0x17000) {
        return -1;
    }
    /* read manfid from CIS */
    if (sdio_cis_get_tuple(card, cis_ptr, CISTPL_MANFID, &manfid, CISTPL_MANFID_SIZE) < 0) {
        return -1;
    }
    fdev->device_id = le16_to_cpu(manfid[1]);

    return 0;
}

static void fs_sdio_cmd0(struct mmc_card *card)
{
    struct mmc_command cmd;
    int err;

    mmc_card_claim_host(card);
    cmd.opcode = MMC_GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.flags = MMC_RSP_NONE | MMC_CMD_BC;
    err = mmc_wait_for_cmd(card->host, &cmd, 0);
    if( err )
    {
        printk("%s: error %d\n", __FUNCTION__, err );
    }
    mmc_card_release_host(card);
}

static void fs_sdio_cmd5(struct mmc_card *card, uint32_t arg)
{
    struct mmc_command cmd;
    int i, err = 0;

    mmc_card_claim_host(card);

    cmd.opcode = SD_IO_SEND_OP_COND;
    cmd.arg = arg;
    cmd.flags = MMC_RSP_R4 | MMC_CMD_BCR;

    for (i = 100; i; i--) {
        err = mmc_wait_for_cmd(card->host, &cmd, CMD_RETRIES);
        if (err != MMC_ERR_NONE)
        {
            printk("%s:%d: error %d\n", __FUNCTION__, __LINE__, err ); 
            break;
        }

        if (cmd.resp[0] & MMC_CARD_BUSY || arg == 0)
        {
            break;
        }
        
        err = MMC_ERR_TIMEOUT;

        mdelay(10);
    }

    mmc_card_release_host(card);
}

static void fs_sdio_cmd3(struct mmc_card *card)
{
    struct mmc_command cmd;
    int err;
    
    mmc_card_claim_host(card);
    cmd.opcode = MMC_SET_RELATIVE_ADDR;
    cmd.arg = 0;
    cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
    err = mmc_wait_for_cmd(card->host, &cmd, CMD_RETRIES);
    if( err )
    {
        printk("%s:%d: error %d\n", __FUNCTION__, __LINE__, err ); 
    }
    
    mmc_card_release_host(card);
}

static void fs_sdio_cmd7(struct mmc_card *card)
{
    struct mmc_command cmd;
    int err;
    
    mmc_card_claim_host(card);
    cmd.opcode = MMC_SELECT_CARD;
    cmd.arg = card->rca << 16;
    cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
    err = mmc_wait_for_cmd(card->host, &cmd, CMD_RETRIES);
    if( err )
    {
        printk("%s:%d: error %d\n", __FUNCTION__, __LINE__, err ); 
    }
    mmc_card_release_host(card);
}

#define IO_EN_TIMEOUT_MS 500

int
fs_sdio_enable(struct sdio_dev *fdev)
{
    uint8_t io_en, io_rdy;
    int timeout;
    int ret;
    
    /* The card may be active if not following a hard reset - abort it first */
    fs_sdio_writeb(fdev, 0, SDIO_CCCR_IO_ABORT, 0x8);

    /* We must go though complete card init, as we have just reset it */
    /* cmd0, then 2 x cmd5, cmd3 and finally 7 */
    
    /* Do a few cmd0 to settle down */
    for(timeout=0;timeout<2;timeout++)
    {
        fs_sdio_cmd0(fdev->card);
    }
    timeout = IO_EN_TIMEOUT_MS;
    
    fs_sdio_cmd5(fdev->card, 0);
    fs_sdio_cmd5(fdev->card, 0x80000);
    fs_sdio_cmd3(fdev->card);
    fs_sdio_cmd7(fdev->card);
    
    ret = fs_sdio_writeb(fdev, 0, SDIO_CCCR_BUS_IFACE_CNTL, 0x22);
    if (ret) {
        printk("%s: err %d line %d\n", __FUNCTION__, ret, __LINE__ );
        goto err;
    }

    ret = fs_sdio_writeb(fdev, 0, SDIO_CCCR_INT_EN, 3);
    if (ret) {
        printk("%s: err %d line %d\n", __FUNCTION__, ret, __LINE__ );
        goto err;
    }

    /* Read-Modify-Write the I/O Enable for function 1. */
    ret = fs_sdio_readb(fdev, 0, SDIO_CCCR_IO_EN, &io_en);
    if (ret) {
        printk("%s: err %d line %d\n", __FUNCTION__, ret, __LINE__ );
        goto err;
    }

    io_en |= (1 << 1);
    ret = fs_sdio_writeb(fdev, 0, SDIO_CCCR_IO_EN, io_en);
    if (ret) {
        printk("%s: err %d line %d\n", __FUNCTION__, ret, __LINE__ );
        goto err;
    }

    /* Wait until the function is enabled. */
    while (timeout) {
        ret = fs_sdio_readb(fdev, 0, SDIO_CCCR_IO_READY, &io_rdy);
        if (ret) {
            printk("%s: err %d line %d\n", __FUNCTION__, ret, __LINE__ );
            goto err;
        }
        if (io_rdy & (1 << 1)) {
            break;
        }

        udelay(1);
        timeout--;
    }

    if(! timeout )
    {
        ret = -ETIMEDOUT;
        printk("%s: err %d line %d\n", __FUNCTION__, ret, __LINE__ );
        goto err;
    }
    
    /* Set the function 1 block size */
    ret = fs_sdio_writeb(fdev, 0, SDIO_FBR_BLK_SIZE(1), fdev->max_blocksize & 0xFF);
    if (ret) {
        return ret;
    }
    ret = fs_sdio_writeb(fdev, 0, SDIO_FBR_BLK_SIZE(1)+1, (fdev->max_blocksize >> 8) & 0xFF);
    if (ret) {
        return ret;
    }
        
err:
    return ret;
}


int
fs_sdio_set_max_clock_speed(struct sdio_dev *fdev, int max_khz)
{
    struct mmc_card *card = fdev->card;

    /* Respect the host controller's min-max. */
    max_khz *= 1000;
    if (max_khz < card->host->f_min) {
        max_khz = card->host->f_min;
    }
    if (max_khz > card->host->f_max) {
        max_khz = card->host->f_max;
    }

    card->host->ios.clock = max_khz;
    card->host->ops->set_ios(card->host, &card->host->ios);

    return max_khz/1000;
}

int fs_sdio_set_block_size(struct sdio_dev *fdev, int blksz)
{
    return 0;
}

/* AW SUN4I */
#ifdef CONFIG_ARCH_SUN4I
extern int mmc_pm_get_mod_type(void);
extern int mmc_pm_gpio_ctrl(char* name, int level);
extern int mmc_pm_get_io_val(char* name);
extern void sw_mmc_rescan_card(unsigned id, unsigned insert);
/*
 * ---------------------------------------------------------------------------
 *
 * Turn on the power of WIFI card
 *
 * ---------------------------------------------------------------------------
 */
static void fs_unifi_power_on( int check_card )
{
    printk("unifi_sdio: power on\n");
    mmc_pm_gpio_ctrl("apm_6981_vcc_en", 1);
    msleep(1);
    mmc_pm_gpio_ctrl("apm_6981_vdd_en", 1);
    msleep(1);
    mmc_pm_gpio_ctrl("apm_6981_pwd_n", 1);
    mmc_pm_gpio_ctrl("apm_6981_rst_n", 0);
    msleep(100);
    mmc_pm_gpio_ctrl("apm_6981_rst_n", 1);
    msleep(300);
    if(check_card) {
        sw_mmc_rescan_card(3, 1);
        msleep(100);
    }
}

/*
 * ---------------------------------------------------------------------------
 *
 * Turn off the power of WIFI card
 *
 * ---------------------------------------------------------------------------
 */
static void fs_unifi_power_off( int check_card )
{
    printk("unifi_sdio: power off\n");
    
    mmc_pm_gpio_ctrl("apm_6981_rst_n", 0);
    if(check_card) {
        sw_mmc_rescan_card(3, 0);
        msleep(100);
    }
}

/* This should be made conditional on being slot 2 too - so we can
 * use a plug in card in slot 1
 */
int fs_sdio_hard_reset(struct sdio_dev *fdev)
{
    fs_unifi_power_off( 0 );
    mdelay(100);
    fs_unifi_power_on( 0 );
    mdelay(100);
    /* We did a hard reset, so return 0 */
    return 0;
}

#else /* #ifdef CONFIG_MX31_3STACK */

int fs_sdio_hard_reset(struct sdio_dev *fdev)
{
    printk("%s: called\n", __FUNCTION__ );

    /* We did not do a hard reset, so return 1 */
    return 1;
}

static void fs_unifi_power_on( int check_card )
{
    (void)check_card;
}

static void fs_unifi_power_off( int check_card )
{
    (void)check_card;
}

#endif /* ! CONFIG_MX31_3STACK */

static struct mmc_driver mmc_driver = {
    .drv		= {
        .name	= "fs_sdio",
    },
    .probe		= fs_sdio_probe,
    .remove		= fs_sdio_remove,
    .suspend	= fs_sdio_suspend,
    .resume		= fs_sdio_resume,
};



int fs_sdio_register_driver(struct fs_driver *driver)
{
    int ret;

    printk(KERN_INFO "fs_sdio_register_driver\n");
    
    /* Switch us on */
    fs_unifi_power_on(-1);
    
    /* Store the context to the device driver to the global */
    available_driver = driver;

    /*
     * If available_sdio_dev is not NULL, probe has been called,
     * so pass the probe to the registered driver
     */
    if (available_sdio_dev) {
        /* Store the context to the new device driver */
        available_sdio_dev->driver = driver;
        /* Do a bit of initialisation */
        sdio_card_read_info(available_sdio_dev);

        printk(KERN_INFO "fs_sdio_register_driver: Glue exists, add device driver and register IRQ\n");
        driver->probe(available_sdio_dev);

        /* Register the IRQ handler to the SDIO IRQ. */
        ret = request_irq(available_sdio_dev->card->host->sdio_irq,
                          fs_sdio_irq, 0, mmc_driver.drv.name, available_sdio_dev->card);
        if (ret) {
            return ret;
        }
    }

    return 0;
}


void fs_sdio_unregister_driver(struct fs_driver *driver)
{
    printk(KERN_INFO "fs_sdio_unregister_driver\n");

    /*
     * If available_sdio_dev is not NULL, probe has been called,
     * so pass the remove to the registered driver to clean up.
     */
    if (available_sdio_dev) {
        printk(KERN_INFO "fs_sdio_unregister_driver: Glue exists, unregister IRQ and remove device driver\n");

        /* Unregister the IRQ handler first. */
        free_irq(available_sdio_dev->card->host->sdio_irq, available_sdio_dev->card);

        driver->remove(available_sdio_dev);

        /* Invalidate the context to the device driver */
        available_sdio_dev->driver = NULL;
    }

    /* Power down the UniFi */
    fs_unifi_power_off( -1 );
    
    /* invalidate the context to the device driver to the global */
    available_driver = NULL;
}


static irqreturn_t fs_sdio_irq(int irq, void *devid)
{
    struct sdio_dev *fdev = (struct sdio_dev*) mmc_get_drvdata((struct mmc_card*)devid);
    struct mmc_card *card = fdev->card;

    if (fdev->driver) {
        if (fdev->driver->card_int_handler) {
            if (fdev->int_enabled) {
                disable_irq_nosync(card->host->sdio_irq);
                fdev->int_enabled = 0;
            }

            fdev->driver->card_int_handler(fdev);
        }
    }

    return IRQ_HANDLED;
}


#ifdef CONFIG_PM
static int fs_sdio_suspend(struct mmc_card *card, pm_message_t state)
{
    struct sdio_dev *fdev = (struct sdio_dev*)mmc_get_drvdata(card);

    /* Pass event to the registered driver. */
    if (fdev->driver) {
        if (fdev->driver->suspend) {
            fdev->driver->suspend(fdev, state);
        }
    }

    return 0;
}

static int fs_sdio_resume(struct mmc_card *card)
{
    struct sdio_dev *fdev = (struct sdio_dev*)mmc_get_drvdata(card);

    /* Pass event to the registered driver. */
    if (fdev->driver) {
        if (fdev->driver->resume) {
            fdev->driver->resume(fdev);
        }
    }

    return 0;
}
#else
#define fs_sdio_suspend NULL
#define fs_sdio_resume  NULL
#endif



static int fs_sdio_probe(struct mmc_card *card)
{
    struct sdio_dev *fdev;
    int ret = 0;

    /*
     * Set the pull-ups, this code should really be part of the MMC driver.
     * The IOMUXC_BASE_ADDR and the values set to the registers are platform
     * specific, so this code may not compile or do the right thing
     * on a different platform.
     */
#ifdef CONFIG_ARCH_MX3
    writel(0x1a569485, IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x168);
    writel(0x0a5295a5, IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x16C);
#endif

    /* Allocate our private context */
    fdev = kmalloc(sizeof(struct sdio_dev), GFP_KERNEL);
    memset(fdev, 0, sizeof(struct sdio_dev));
    fdev->int_enabled = 1;
    fdev->lock = SPIN_LOCK_UNLOCKED;
    fdev->card = card;
    /* Store our context to the global pointer */
    available_sdio_dev = fdev;

    /* Register the card context to the MMC driver. */
    card->scr.bus_widths = SD_SCR_BUS_WIDTH_4;
    card->csd.max_dtr = 1000000;
    /*
     * Set a default SDIO block size,
     * override it when we read the block size from the device.
     */
    card->csd.read_blkbits = 64*8;
    card->csd.write_blkbits = 64*8;
    card->csd.read_partial = 1;
    card->csd.write_partial = 1;

    /* Store our context in the MMC driver */
    printk(KERN_INFO "fs_sdio_probe: Add glue driver\n");
    mmc_set_drvdata(card, fdev);

    /* TODO: If a device driver is registered, call it's probe here */
    if (available_driver) {
        /* Store the context to the device driver */
        fdev->driver = available_driver;
        /* Do a bit of initialisation */
        sdio_card_read_info(fdev);

        printk(KERN_INFO "fs_sdio_probe: Add device driver and register IRQ\n");
        available_driver->probe(fdev);

        /* Register the IRQ handler to the SDIO IRQ. */
        ret = request_irq(card->host->sdio_irq, fs_sdio_irq, 0, mmc_driver.drv.name, card);
        if (ret) {
            return ret;
        }

    }

    return 0;
}

static void fs_sdio_remove(struct mmc_card *card)
{
    struct sdio_dev *fdev = (struct sdio_dev*)mmc_get_drvdata(card);

    /* If there is a registered device driver, pass on the remove */
    if (fdev->driver) {
        printk(KERN_INFO "fs_sdio_remove: Free IRQ and remove device driver\n");
        /* Unregister the IRQ handler first. */
        free_irq(card->host->sdio_irq, card);

        fdev->driver->remove(fdev);
    }

    printk(KERN_INFO "fs_sdio_remove: Remove glue driver\n");
    /* Unregister the card context from the MMC driver. */
    mmc_set_drvdata(card, NULL);

    /* Invalidate the global to our context. */
    available_sdio_dev = NULL;
    kfree(fdev);

    return;
}



/* Module init and exit, register and unregister to the SDIO/MMC driver */
static int __init fs_sdio_init(void)
{
    printk(KERN_INFO "Freescale: Register to MMC/SDIO driver\n");
    /* Sleep a bit - otherwise if the mmc subsystem has just started, it will
     * allow us to register, then immediatly remove us!
     */
    msleep(10);
    return mmc_register_driver(&mmc_driver);
}
module_init(fs_sdio_init);

static void __exit fs_sdio_exit(void)
{
    printk(KERN_INFO "Freescale: Unregister from MMC/SDIO driver\n");
    mmc_unregister_driver(&mmc_driver);
}
module_exit(fs_sdio_exit);


MODULE_DESCRIPTION("Freescale SDIO glue driver");
MODULE_AUTHOR("Cambridge Silicon Radio Ltd.");
MODULE_LICENSE("GPL");
