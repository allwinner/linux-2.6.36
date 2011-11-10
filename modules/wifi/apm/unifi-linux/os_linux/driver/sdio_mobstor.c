/*
 * ---------------------------------------------------------------------------
 *
 * FILE: sdio_3dlabs.c
 *
 * PURPOSE: Driver instantiation and deletion for SDIO on 3dlabs DMS-05.
 *
 *      This file brings together the SDIO bus interface and the UniFi
 *      driver core.
 *
 * Copyright (C) 2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * 
 * NOTES:
 *
 * This file was written to support the 3D Labs DMS-05 EVM board.
 * 
 * The SDIO/SD/MMC slots are:
 *   0  4G Flash memory
 *   1  SDIO connector
 *   2  Marvell WiFi
 * 
 * The Marvell WiFi registers itself as network device "wifi0" and
 * works with iwconfig and iwlist.
 *
 *
 * CHANGES TO PLATFORM:
 * Modified modprobe.conf to disable sd8xxx (Marvell driver) and change
 * wifi to alias unifi_sdio.
 * 
 * To start UniFi:
 *   mmcconfig remove 2
 *   modprobe unifi_sdio
 *   mmcconfig remove 1
 *   mmcconfig add 1
 * (could modify /etc/init.d/rcS to do this)
 *
 * Note: the unifi_helper and unififw script must be installed in /usr/sbin.
 * ---------------------------------------------------------------------------
 */
/* The code here was based on Jens sdio_mobs.c file */

#include <linux/kmod.h>
#include <linux/init.h>
#include "mobstor_sdio_bus.h"

#include "driver/unifi.h"
#include "unifi_priv.h"

#define USE_POLLING


static int Interrupt_enabled = 0;
#define BOUNCE_BUFF_SIZE 1600
static u8 *Bounce_buff = NULL;


static int
convert_sdio_error(unsigned int r)
{
    int err = 0;

    if (r == 0) return 0;

    switch (r) {

      case ERRCMDINPROGRESS:
      case ERRCRC:
      case ERRECCFAILED:
      case ERRCCERR:
      case ERRUNDERRUN:
      case ERROVERRUN:
      case ERRUNDERWRITE:
      case ERROVERREAD:
      case ERRENDBITERR:
      case ERRDCRC:
      case ERRSTARTBIT:
        err = -EIO;
        break;

      case ERRRESPTIMEOUT:
      case ERRCMDRETRIESOVER:
      case ERRDATATIMEOUT:
      case ERRTIMEROUT:
        err = -ETIMEDOUT;
        break;

      case ERRCARDNOTFOUND:
      case ERRCARDNOTCONN:
        err = -ENODEV;
        break;

      default:
      case ERRCMDNOTSUPP:
      case ERRINVALIDCARDNUM:
      case ERRNOTSUPPORTED:
      case ERRRESPRECEP:
      case ERRENUMERATE:
      case ERRHARDWARE:
      case ERRNOMEMORY:
      case ERRFSMSTATE:
      case ERRADDRESSRANGE:
      case ERRADDRESSMISALIGN:
      case ERRBLOCKLEN:
      case ERRERASESEQERR:
      case ERRERASEPARAM:
      case ERRPROT:
      case ERRCARDLOCKED:
      case ERRILLEGALCOMMAND:
      case ERRUNKNOWN:
      case ERRCSDOVERWRITE:
      case ERRERASERESET:
      case ERRCARDNOTREADY:
      case ERRBADFUNC:
      case ERRPARAM:
      case ERRNOTFOUND:
        err = -EINVAL;
        break;
    }

    return err;
} /* convert_sdio_error() */


#define CISTPL_NULL     0x00
#define CISTPL_CHECKSUM 0x10
#define CISTPL_VERS_1   0x15
#define CISTPL_ALTSTR   0x16
#define CISTPL_MANFID   0x20
#  define CISTPL_MANFID_SIZE 0x04
#define CISTPL_FUNCID   0x21
#define CISTPL_FUNCE    0x22
#define CISTPL_SDIO_STD 0x91
#define CISTPL_SDIO_EXT 0x92
#define CISTPL_END      0xff
#define CISTPL_FUNCE  0x22
#  define CISTPL_FUNCE_00_SIZE 0x04
#  define CISTPL_FUNCE_01_SIZE 0x2a


static u32
get_tuple(MobsHostInfo_t * mobs_unit, u32 slot, u32 func, u8 ident, u32 len,
          u8 * output_buffer)
{
    u32 retval;
    u8 current_tuple = 0;
    u8 length;
    u8 buf[3];
    u32 cis;
    u32 saved_cis = 0;


    if ((0 == ident) || (0xff == ident)) {
        printk("Bad Ident : 0x%02x\n", ident);
        return ERRPARAM;
    }


    if((retval = bus_io_rw_52(mobs_unit, slot, 0, (func << 8) | 0x09, &buf[0], 0, 0)))
    {
        printk("Unable to read CIS pointer : %d\n", retval);
    }
    if((retval = bus_io_rw_52(mobs_unit, slot, 0, (func << 8) | 0x0a, &buf[1], 0, 0)))
    {
        printk("Unable to read CIS pointer : %d\n", retval);
    }
    if((retval = bus_io_rw_52(mobs_unit, slot, 0, (func << 8) | 0x0b, &buf[2], 0, 0)))
    {
        printk("Unable to read CIS pointer : %d\n", retval);
    }


    /* Now we have the cis pointer */
    saved_cis = cis = (buf[2] << 16) | (buf[1] << 8) | buf[0];;


    if ((cis < 0x0001000) || (cis > 0x017fff)) {
        printk("CIS pointer out of range : 0x%08x\n", cis);
        return ERRHARDWARE;
    }

    while ((current_tuple != 0xff) && (current_tuple != ident)) {
        if ((retval =
             bus_io_rw_52(mobs_unit, slot, 0, cis, &current_tuple, 0,
                          0))) {
            printk("Unable to read tuple ident @ 0x%08x\n", cis);
            return retval;
        }


        if ((retval =
             bus_io_rw_52(mobs_unit, slot, 0, cis + 1, &length, 0, 0))) {
            printk("Unable to read tuple length @ 0x%08x\n", cis + 1);
            return retval;
        }
        saved_cis = cis;
        cis += (length + 2);
    }

    if (current_tuple == ident) {
        int i;

        if (length < len) {
            /* buffer is smaller than tuple */
            len = length;
        }

        for (i = 0; i < len; i++) {
            retval =
                bus_io_rw_52(mobs_unit, slot, 0, saved_cis + 2 + i,
                             &output_buffer[i], 0, 0);
            if (retval) {
                break;
            }
        }
#if 0
        if (!retval) {
            int c;
            printk("\n");
            for (c = 0; c < len; c++) {
                printk("%d ", output_buffer[c]);
            }
            printk("\n");
        }
#endif

    } else {
        retval = ERRNOTFOUND;
    }

    return retval;
}


static int
get_device_ids(struct sdio_device *the_device, u16 *vendor_id, u16 *device_id)
{
    u8 idbuf[CISTPL_MANFID_SIZE];

    /* read manfid from CIS */
    memset(idbuf, 0, CISTPL_MANFID_SIZE);

    BUG_ON(the_device == NULL);
    BUG_ON(the_device->host == NULL);

    plat_get_device(the_device->host);
    if (get_tuple(the_device->host,
                           the_device->slot_num,
                           0 /* the_device->function_number */,
                           CISTPL_MANFID, CISTPL_MANFID_SIZE,
                           idbuf))
    {
        printk("Could not get tuple !\n");
        plat_relinquish_device(the_device->host);
        return -EIO;
    }
    plat_relinquish_device(the_device->host);

    *vendor_id = idbuf[0] | (idbuf[1] << 8);
    *device_id = idbuf[2] | (idbuf[3] << 8);

    return 0;
}


static int
print_reg(struct sdio_device *sdio_dev, int addr, const char *reg_name)
{
    int r = 0;
    u8 val;

    r = sdio_dev->do_single_register_io(sdio_dev, addr, &val,
                                        DOREAD, FUNC0);
    if (r) {
        printk("Error %d reading %s\n", r, reg_name);
    } else {
        printk("%s=0x%X\n", reg_name, val);
    }
    return r;
} /* print_reg() */


static int
set_block_size(struct sdio_device *sdio_dev, int blocksize)
{
    int r;
    u8 val;
#define SDIO_FUNC1_BLOCKSIZE 0x110

    val = blocksize & 0xFF;
    printk("writing 0x%x to 0x%X\n", val, SDIO_FUNC1_BLOCKSIZE);
    r = sdio_dev->do_single_register_io(sdio_dev, SDIO_FUNC1_BLOCKSIZE, &val,
                                        DOWRITE, FUNC0);
    if (r) {
        printk("Error %d writing 0x%02X to SDIO reg 0x%X\n",
               r, val, SDIO_FUNC1_BLOCKSIZE);
//        return r;
    }

    val = (blocksize >> 8) & 0xFF;
    printk("writing 0x%x to 0x%X\n", val, SDIO_FUNC1_BLOCKSIZE+1);
    r = sdio_dev->do_single_register_io(sdio_dev, SDIO_FUNC1_BLOCKSIZE+1, &val,
                                        DOWRITE, FUNC0);
    if (r) {
        printk("Error %d writing 0x%02X to SDIO reg 0x%X\n",
               r, val, SDIO_FUNC1_BLOCKSIZE+1);
//        return r;
    }

    print_reg(sdio_dev, 0x110, "F1_BLOCKSIZE_LO");
    print_reg(sdio_dev, 0x111, "F1_BLOCKSIZE_HI");

    return r;
} /* set_block_size() */




/*
 * ---------------------------------------------------------------------------
 *  unifi_sdio_readb
 *  unifi_sdio_writeb
 *  unifi_sdio_block_rw
 *
 *      Basic SDIO read/write functions
 * 
 *  Arguments:
 *      
 *
 *  Returns:
 *      0
 *      -EINVAL
 *      -EIO
 *      -ETIMEDOUT
 *      -ENODEV
 * ---------------------------------------------------------------------------
 */
int
unifi_sdio_readb(void *sdio, int funcnum, unsigned long addr,
                 unsigned char *pdata)
{
    struct sdio_device *sdio_dev = sdio;
    unsigned int err;

    err = sdio_dev->do_single_register_io(sdio_dev, addr, pdata,
                                          DOREAD,
                                          (funcnum == 0) ? FUNC0 : OWNFUNC);
    return convert_sdio_error(err);
} /* unifi_sdio_readb() */

int
unifi_sdio_writeb(void *sdio, int funcnum, unsigned long addr,
                  unsigned char data)
{
    struct sdio_device *sdio_dev = sdio;
    unsigned int err;

    err = sdio_dev->do_single_register_io(sdio_dev, addr, &data,
                                          DOWRITE,
                                          (funcnum == 0) ? FUNC0 : OWNFUNC);
    return convert_sdio_error(err);
} /* unifi_sdio_writeb() */


int
unifi_sdio_block_rw(void *sdio, int funcnum, unsigned long addr,
                    unsigned char *pdata, unsigned int count, int direction)
{
    struct sdio_device *sdio_dev = sdio;
    unsigned int err;
    int nblocks;

    {
        int ofs;
        ofs = (pdata - (unsigned char *)NULL) & 15;
        if (ofs) {
            //printk("%s: unaligned buffer: pdata=%p\n", __FUNCTION__, pdata);

            if (count > BOUNCE_BUFF_SIZE) {
                printk("%s: count (%d) too big for bounce buffer (%d)\n",
                       __FUNCTION__, count, BOUNCE_BUFF_SIZE);
                return -EINVAL;
            }
            memcpy(Bounce_buff, pdata, count);
            pdata = Bounce_buff;
        }
    }

    if ((count % sdio_dev->max_block_size) != 0) {
        printk("Invalid CMD53 byte count, whole blocks only, count=%d, blksiz=%d\n",
               count, sdio_dev->max_block_size);
        return -EINVAL;
    }
    nblocks = count / sdio_dev->max_block_size;

#if 0
    unifi_trace(NULL, UDBG1,
                "block_rw: %c pdata=%p addr=0x%X, count=0x%X, blksiz=0x%X, nblks=0x%X\n",
                direction ? 'W' : 'R',
                pdata,
                addr,
                count,
                sdio_dev->max_block_size,
                nblocks);
#endif

    err = sdio_dev->do_multiple_register_io_block(
        sdio_dev,
        addr,
        pdata,
        nblocks,
        sdio_dev->max_block_size,
        direction,
        OWNFUNC,
        1 /* FIFO mode, don't inc addr */);

    return convert_sdio_error(err);
} /* unifi_sdio_block_rw() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_sdio_set_max_clock_speed
 *
 *      Set the maximum SDIO bus clock speed to use.
 *
 *  Arguments:
 *      sdio            SDIO context pointer
 *      max_khz         maximum clock speed in kHz
 *
 *  Returns:
 *      Set clock speed in kHz; or a UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
int
unifi_sdio_set_max_clock_speed(void *sdio, int max_khz)
{
#define MOBS_CLK 45000
    struct sdio_device *sdio_dev = sdio;
    u32 card_divider;
    int actual_khz;

//    printk("%s: maxkhz=%d\n", __FUNCTION__, max_khz);

    /* Apply module parameter override */
    if (max_khz <= 0 || max_khz > sdio_clock) {
        max_khz = sdio_clock;
    }

    card_divider = MOBS_CLK / max_khz;

    /* 
     * Would be good to call bus_set_clk_freq(() here,
     * but it is not an exported symbol.
     */
    sdio_dev->host->cards[0].divider_val = card_divider;

    actual_khz = MOBS_CLK / (card_divider+1);
//    printk("actual freq %d\n", actual_khz);

    return actual_khz;
} /* unifi_sdio_set_max_clock_speed() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_sdio_enable_interrupt
 *
 *      Enable or disable the SDIO interrupt.
 *      The driver can be made more efficient by disabling the SDIO interrupt
 *      during processing.
 *      The SDIO interrupt can be disabled by modifying the SDIO_INT_ENABLE
 *      register in the Card Common Control Register block, but this requires
 *      two CMD52 operations. A better solution is to mask the interrupt at
 *      the host controller.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 *      enable          If zero disable (or mask) the interrupt, otherwise
 *                      enable (or unmask) it.
 *
 *  Returns:
 *      Zero on success or a UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
int
unifi_sdio_enable_interrupt(void *sdio, int enable)
{
#ifdef USE_POLLING
//    printk("unifi_sdio_enable_interrupt: enable=%d\n", enable);

#else
    struct sdio_device *sdio_dev = sdio;

//    printk("unifi_sdio_enable_interrupt: enable=%d\n", enable);

    if (enable == Interrupt_enabled) {
        return 0;
    }

    if (enable) {
#if 0
        print_reg(sdio_dev, SDIO_REG_IOENABLE, "IO_EN");
        print_reg(sdio_dev, SDIO_REG_INTRENABLE, "INT_EN");
        print_reg(sdio_dev, SDIO_REG_INTRPENDING, "INT_PEND");
#endif
        sdio_dev->enable_device_interrupt(sdio_dev);
    } else {
        sdio_dev->disable_device_interrupt(sdio_dev);
    }
#endif /* USE_POLLING */

    Interrupt_enabled = enable;

    return 0;
} /* unifi_sdio_enable_interrupt() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_sdio_enable
 *
 *      Enable i/o on function 1.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 *
 * Returns:
 *      UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
int
unifi_sdio_enable(void *sdio)
{
    struct sdio_device *sdio_dev = sdio;
    int count;
    u8 ioready;
    int r;

    /* Enable UniFi function 1 (the 802.11 part). */
    r = unifi_sdio_writeb(sdio, 0, SDIO_REG_IOENABLE, 2);
    if (r) {
        printk(KERN_ERR "Failed to enable func 1, sdio err=%d\n", r);
        return convert_sdio_error(r);
    }

    /* Wait for IOREADY */
    ioready = 0;
    for (count = 0; count < 10; count++) {
        r = unifi_sdio_readb(sdio_dev, 0, SDIO_REG_IOREADY, &ioready);
        if (r) {
            printk(KERN_ERR "Failed to read IOREADY, sdio err=%d\n", r);
            return convert_sdio_error(r);
        }
        if (ioready & 0x2) {
            break;
        }
        msleep(1);
    }
    if ((ioready & 0x2) == 0) {
        printk(KERN_ERR "IOREADY still 0 after 100 tries\n");
        return -ETIMEDOUT;
    }

    return r;
} /* unifi_sdio_enable() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_sdio_active
 *
 *      No-op as the bus goes to an active state at the start of every
 *      command.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 * ---------------------------------------------------------------------------
 */
void
unifi_sdio_active(void *sdio)
{
}

/*
 * ---------------------------------------------------------------------------
 *  unifi_sdio_idle
 *
 *      Set the function as idle.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 * ---------------------------------------------------------------------------
 */
void
unifi_sdio_idle(void *sdio)
{
} /* unifi_sdio_idle() */


int unifi_sdio_power_on(void *sdio)
{
    return 1;
}

void unifi_sdio_power_off(void *sdio)
{
}


/*
 * ---------------------------------------------------------------------------
 *  unifi_sdio_hard_reset
 *
 *      Hard Resets UniFi is possible.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 *
 * Returns:
 * 1 if the SDIO driver is not capable of doing a hard reset.
 * 0 if a hard reset was successfully performed.
 * -EIO if an I/O error occured while re-initializing the
 * card.  This is a fatal, non-recoverable error.
 * -ENODEV if the card is no longer present.
 * ---------------------------------------------------------------------------
 */
int unifi_sdio_hard_reset(void *sdio)
{
    /* Can't do it */
    return 1;
} /* unifi_sdio_hard_reset() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_sdio_get_info
 *
 *      Return UniFi information read by the SDIO driver.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 *      param_type      The enum value for the required information
 *      param_value     Pointer to store the returned information
 *
 * Returns:
 *      UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
int
unifi_sdio_get_info(void *sdio, enum unifi_sdio_request param_type, unsigned int *param_value)
{
    struct sdio_device *sdio_dev = sdio;

    switch (param_type) {
      case UNIFI_SDIO_IO_BLOCK_SIZE:
        *param_value = sdio_dev->max_block_size;
        break;
      case UNIFI_SDIO_VENDOR_ID:
//        *param_value = fdev->vendor_id;
        *param_value = 0x032A;  /* CSR */
        break;
      case UNIFI_SDIO_DEVICE_ID:
//        *param_value = fdev->device_id;
        *param_value = 0x0001;  /* UniFi-1 */
        break;
      case UNIFI_SDIO_FUNCTION_NUM:
        *param_value = sdio_dev->function_number;
        break;
      default:
        return -EINVAL;
    }

    return 0;

} /* unifi_sdio_get_info() */


/*
 * ---------------------------------------------------------------------------
 *  uf_sdio_int_handler
 *
 *      Interrupt callback function for SDIO interrupts.
 *      This is called in kernel context (i.e. not interrupt context).
 *      We retrieve the unifi context pointer and call the main UniFi
 *      interrupt handler.
 *
 *  Arguments:
 *      fdev      SDIO context pointer
 *
 *  Returns:
 *      Linux error code, i.e. 0 on success -E??? on error
 * ---------------------------------------------------------------------------
 */
static int
uf_sdio_int_handler(struct sdio_device *sdio_dev)
{
    unifi_priv_t *priv = sdio_dev->priv_data;

    /* mobstor calls this with interrupts disabled */
//    Interrupt_enabled = 0;

//    printk("INTERRUPT!\n");
    unifi_sdio_interrupt_handler(priv->card);

    return 0;
} /* uf_sdio_int_handler() */








/*
 * ---------------------------------------------------------------------------
 * Polling support
 *
 */
#ifdef USE_POLLING
static long poll_thread_pid;
static int poll_running;

static int
poll_thread_function(void *arg)
{
    struct sdio_device *sdio_dev = arg;
#define POLL_INTERVAL_MS 50

    daemonize("3dlabs_sdio_poll");
    /*
     * daemonize() disables all signals. Allow SIGTERM to wake an
     * interruptible_sleep_on() call.
     */
    allow_signal(SIGTERM);

    while (1) {
        int r;
        u8 intpend;

        msleep_interruptible(POLL_INTERVAL_MS);
        if (!poll_running) {
            break;
        }

        if (Interrupt_enabled) {
            r = unifi_sdio_readb(sdio_dev, 0, SDIO_REG_INTRPENDING, &intpend);
            if (r) {
                printk("Error reading INTPEND register: %d\n", r);
                break;
            }
            //printk("INTPEND=0x%X\n", intpend);
            //print_reg(sdio_dev, SDIO_REG_INTRENABLE, "INT_EN");
            if (intpend & 2) {
                uf_sdio_int_handler(sdio_dev);
            }
        }
    }

    printk("Polling thread exit\n");

    return 0;
} /* poll_thread_function() */


static void
init_polling(struct sdio_device *sdio_dev)
{
    poll_running = 1;
    poll_thread_pid = kernel_thread(poll_thread_function, sdio_dev, 0);
}
#endif /* USE_POLLING */




/*
 * ---------------------------------------------------------------------------
 *  uf_sdio_probe
 *
 *      Card insert callback.
 *
 * Arguments:
 *      dev             Linux device struct
 *
 * Returns:
 *      UniFi driver error code.
 *
 * Notes:
 *      The mobstor SDIO driver has not set up the sdio_device struct when
 *      this is called. The only valid fields are:
 *              dev->bus_id
 *
 * That means UniFi is not ready and we can't do any SDIO at this time.
 *   
 * ---------------------------------------------------------------------------
 */
static int
uf_sdio_probe(struct device *dev)
{
    struct sdio_device *sdio_dev = to_sdio_device(dev);

    unifi_info(NULL, "uf_sdio_probe: card inserted\n");

#if 0
    printk("dev=%p, dev->bus_id \"%s\"\n", dev, dev->bus_id);
    printk("sdio_dev=0x%p: slot %d, name 0x%p \"%s\"\n",
           sdio_dev,
           sdio_dev->slot_num,
           sdio_dev->pretty_name, sdio_dev->pretty_name);
    printk("         d_f %d, f_n %d, max_block_size %d\n",
           sdio_dev->device_function,
           sdio_dev->function_number,
           sdio_dev->max_block_size);
    printk("         host %p\n",
           sdio_dev->host);

    printk("divider_val = %d\n", sdio_dev->host->cards[0].divider_val);

    sdio_dev->host->cards[0].divider_val = 10;
    printk("divider_val now %d\n", sdio_dev->host->cards[0].divider_val);

    print_reg(sdio_dev, SDIO_REG_IOABORT, "IO_ABORT");
    print_reg(sdio_dev, SDIO_REG_IOENABLE, "IO_EN");
    print_reg(sdio_dev, SDIO_REG_INTRENABLE, "INT_EN");
    print_reg(sdio_dev, 0x110, "F1_BLOCKSIZE_LO");
    print_reg(sdio_dev, 0x111, "F1_BLOCKSIZE_HI");
#endif


    Bounce_buff = (u8 *)kmalloc(BOUNCE_BUFF_SIZE, GFP_KERNEL);
    if (!Bounce_buff) {
        printk("Failed to allocate memory for bounce buffer\n");
        return -ENOMEM;
    }


    {
        int r;
        u16 vendor_id, device_id;

        r = get_device_ids(sdio_dev, &vendor_id, &device_id);
        if (r) {
            printk("Failed to get device ID from CIS: err %d\n", r);
        }
        else
        {
            unifi_info(NULL, "VID=%04X, PID=%04X\n", vendor_id, device_id);

            if ((vendor_id != 0x032A) || /* CSR */
                (device_id != 0x0001))   /* UniFi-1 */
            {
                /* Not a UniFi-1 */
                printk("Not a UniFi-1\n");
                return -EINVAL;
            }
        }
    }

    /* 
     * The mobstor seems to fail reading the CIS from UniFi, which means
     * we don't get a proper value of max_block_size.
     * So set it manually here.
     */
    sdio_dev->max_block_size = 64;

    set_block_size(sdio_dev, sdio_dev->max_block_size);


    /* Register this device with the main UniFi driver */
    /* Only support one device, so hard-code the unifi index to 0 */
    sdio_dev->priv_data = register_unifi_sdio(sdio_dev, 0);
//    printk("uf_sdio_probe: register_unifi_sdio returned %p\n", sdio_dev->priv_data);

#ifdef USE_POLLING
    printk("uf_sdio_probe: Polling mode\n");
    sdio_dev->disable_device_interrupt(sdio_dev);
    init_polling(sdio_dev);
#endif /* USE_POLLING */

    return sdio_dev->priv_data ? 0 : -ENOMEM;

} /* uf_sdio_probe() */


/*
 * ---------------------------------------------------------------------------
 *  uf_sdio_remove
 *
 *      Card removal callback.
 *
 * Arguments:
 *      dev             Linux device struct
 *
 * Returns:
 *      UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
static int
uf_sdio_remove(struct device *dev)
{
    struct sdio_device *sdio_dev = to_sdio_device(dev);

    unifi_info(NULL, "card removed\n");

#ifdef USE_POLLING
    poll_running = 0;
#endif /* USE_POLLING */

    if (sdio_dev->disable_device_interrupt) {
        sdio_dev->disable_device_interrupt(sdio_dev);
    }

    /* Clean up the main UniFi driver */
    unregister_unifi_sdio(0);

    if (Bounce_buff) {
        kfree(Bounce_buff);
    }
    Bounce_buff = NULL;

    return 0;
} /* uf_sdio_remove */


/*
 * ---------------------------------------------------------------------------
 *  uf_sdio_suspend
 *
 *      System suspend callback.
 *
 * Arguments:
 *      dev             Linux device struct
 *
 * Returns:
 *
 * ---------------------------------------------------------------------------
 */
static int
uf_sdio_suspend(struct device *dev, pm_message_t state)
{
    struct sdio_device *sdio_dev = to_sdio_device(dev);
    unifi_priv_t *priv = sdio_dev->priv_data;

    unifi_trace(NULL, UDBG3, "Suspending...\n");
    /* Pass event to UniFi Driver. */
    unifi_suspend(priv);

    return 0;
} /* uf_sdio_suspend() */


/*
 * ---------------------------------------------------------------------------
 *  uf_sdio_resume
 *
 *      System resume callback.
 *
 * Arguments:
 *      dev             Linux device struct
 *
 * Returns:
 * 
 * ---------------------------------------------------------------------------
 */
static int
uf_sdio_resume(struct device *dev)
{
    struct sdio_device *sdio_dev = to_sdio_device(dev);
    unifi_priv_t *priv = sdio_dev->priv_data;

    unifi_trace(NULL, UDBG3, "Resuming...\n");
    /* Pass event to UniFi Driver. */
    unifi_resume(priv);

    return 0;
} /* uf_sdio_resume() */



 /* *MUST* claim to support SDIO type WLAN or mobstor won't match us */
static struct sdio_device_driver unifi_driver = {
    .pretty_name = "unifi_sdio_drv",
    .the_devids = { 1, { 7, } }, /* WLAN */
    .priv_data = NULL,
    .intr_handler = uf_sdio_int_handler,

    .drv = {
        .name = "unifi_sdio",
        .owner = THIS_MODULE,
        .probe  = uf_sdio_probe,
        .remove = uf_sdio_remove,
        .suspend  = uf_sdio_suspend,
        .resume = uf_sdio_resume,
    },
};


/*
 * ---------------------------------------------------------------------------
 *  uf_sdio_load
 *  uf_sdio_unload
 *
 *      These functions are called from the main module load and unload
 *      functions. They perform the appropriate operations for the monolithic
 *      driver.
 * 
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
int __init
uf_sdio_load(void)
{
    int r;

    printk("Unifi: Using Mobstor SDIO driver\n");

    r = sdio_driver_register(&unifi_driver);
    if (r) {
        unifi_error(NULL, "Failed to register UniFi driver with SDIO: %d\n", r);
        return r;
    }

    printk("uf_sdio_load: done\n");
    return 0;
} /* uf_sdio_load() */



void __exit
uf_sdio_unload(void)
{
    sdio_driver_unregister(&unifi_driver);
} /* uf_sdio_unload() */

