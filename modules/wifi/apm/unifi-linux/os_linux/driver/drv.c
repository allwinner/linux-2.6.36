/*
 * ---------------------------------------------------------------------------
 *  FILE:     drv.c
 *
 *  PURPOSE:
 *      Conventional device interface for debugging/monitoring of the
 *      driver and h/w using unicli. This interface is also being used
 *      by the SME linux implementation and the helper apps.
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */



/*
 * Porting Notes:
 * Part of this file contains an example for how to glue the OS layer
 * with the HIP core lib, the SDIO glue layer, and the SME.
 *
 * When the unifi_sdio.ko modules loads, the linux kernel calls unifi_load().
 * unifi_load() calls uf_sdio_load() which is exported by the SDIO glue
 * layer. uf_sdio_load() registers this driver with the underlying SDIO driver.
 * When a card is detected, the SDIO glue layer calls register_unifi_sdio()
 * to pass the SDIO function context and ask the OS layer to initialise
 * the card. register_unifi_sdio() allocates all the private data of the OS
 * layer and calls uf_run_unifihelper() to start the SME. The SME calls
 * unifi_sys_wifi_on_req() which uses the HIP core lib to initialise the card.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <linux/jiffies.h>

#include <driver/unifiversion.h>
#include "unifi_priv.h"
#include <driver/conversions.h>

#include "event_pack_unpack/event_pack_unpack.h"

/* Module parameter variables */
int buswidth = 0;               /* 0 means use default, values 1,4 */
int sdio_clock = 50000;         /* kHz */
int unifi_debug = 0;
/*
 * fw_init prevents f/w initialisation on error.
 * Unless necessary, avoid usage in the CSR_SME_EMB build because it prevents
 * UniFi initialisation after getting out of suspend and also leaves
 * UniFi powered when the module unloads.
 */
int fw_init[MAX_UNIFI_DEVS] = {-1, -1};
int use_5g = 0;
int led_mask = 0;               /* 0x0c00 for dev-pc-1503c, dev-pc-1528a */
int disable_hw_reset = 0;
int disable_power_control = 0;
int enable_wol = UNIFI_WOL_OFF; /* 0 for none, 1 for SDIO IRQ, 2 for PIO */
#ifdef CSR_SME_EMB
int sme_debug = 0;
#endif
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
int tl_80211d = (int)unifi_TrustMIB;
#endif
int sdio_block_size = -1;      /* Override SDIO block size */
int sdio_byte_mode = 0;        /* 0 for block mode + padding, 1 for byte mode */
int coredump_max = HIP_NUM_COREDUMP_BUFFERS;
int run_bh_once = -1;          /* Set for scheduled interrupt mode, -1 = default */

MODULE_DESCRIPTION("CSR UniFi (SDIO)");

module_param(buswidth,    int, S_IRUGO|S_IWUSR);
module_param(sdio_clock,  int, S_IRUGO|S_IWUSR);
module_param(unifi_debug, int, S_IRUGO|S_IWUSR);
module_param_array(fw_init, int, NULL, S_IRUGO|S_IWUSR);
module_param(use_5g,      int, S_IRUGO|S_IWUSR);
module_param(led_mask,    int, S_IRUGO|S_IWUSR);
#ifdef CSR_SME_EMB
module_param(sme_debug,   int, S_IRUGO|S_IWUSR);
#endif /* CSR_SME_EMB */
module_param(disable_hw_reset,  int, S_IRUGO|S_IWUSR);
module_param(disable_power_control,  int, S_IRUGO|S_IWUSR);
module_param(enable_wol,  int, S_IRUGO|S_IWUSR);
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
module_param(tl_80211d,   int, S_IRUGO|S_IWUSR);
#endif
module_param(sdio_block_size, int, S_IRUGO|S_IWUSR);
module_param(sdio_byte_mode, int, S_IRUGO|S_IWUSR);
module_param(coredump_max, int, S_IRUGO|S_IWUSR);
module_param(run_bh_once, int, S_IRUGO|S_IWUSR);

MODULE_PARM_DESC(buswidth, "SDIO bus width (0=default), set 1 for 1-bit or 4 for 4-bit mode");
MODULE_PARM_DESC(sdio_clock, "SDIO bus frequency in kHz, (default = 50 MHz)");
MODULE_PARM_DESC(unifi_debug, "Diagnostic reporting level");
MODULE_PARM_DESC(fw_init, "Set to 0 to prevent f/w initialization on error");
MODULE_PARM_DESC(use_5g, "Use the 5G (802.11a) radio band");
MODULE_PARM_DESC(led_mask, "LED mask flags");
#ifdef CSR_SME_EMB
MODULE_PARM_DESC(sme_debug, "SME diagnostic reporting level");
#endif
MODULE_PARM_DESC(disable_hw_reset, "Set to 1 to disable hardware reset");
MODULE_PARM_DESC(disable_power_control, "Set to 1 to disable SDIO power control");
MODULE_PARM_DESC(enable_wol, "Enable wake-on-wlan function 0=off, 1=SDIO, 2=PIO");
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
MODULE_PARM_DESC(tl_80211d, "802.11d Trust Level (1-6, default = 5)");
#endif
MODULE_PARM_DESC(sdio_block_size, "Set to override SDIO block size");
MODULE_PARM_DESC(sdio_byte_mode, "Set to 1 for byte mode SDIO");
MODULE_PARM_DESC(coredump_max, "Number of chip coredump buffers to allocate");
MODULE_PARM_DESC(run_bh_once, "Run BH only when firmware interrupts");

/* Callback for event logging to UDI clients */
static void udi_log_event(ul_client_t *client,
                          const u8 *signal, int signal_len,
                          const bulk_data_param_t *bulkdata,
                          int dir);

static void udi_set_log_filter(ul_client_t *pcli,
                               unifiio_filter_t *udi_filter);


/* Mutex to protect access to  priv->sme_cli */
DECLARE_MUTEX(udi_mutex);

static const char*
trace_putest_cmdid(unifi_putest_command_t putest_cmd)
{
    switch (putest_cmd)
    {
        case UNIFI_PUTEST_START:
            return "START";
        case UNIFI_PUTEST_STOP:
            return "STOP";
        case UNIFI_PUTEST_SET_SDIO_CLOCK:
            return "SET CLOCK";
        case UNIFI_PUTEST_CMD52_READ:
            return "CMD52R";
        case UNIFI_PUTEST_CMD52_WRITE:
            return "CMD52W";
        case UNIFI_PUTEST_DL_FW:
            return "D/L FW";
        case UNIFI_PUTEST_DL_FW_BUFF:
            return "D/L FW BUFFER";
        default:
            return "ERROR: unrecognised command";
    }
 }



/*
 * ---------------------------------------------------------------------------
 *  unifi_open
 *  unifi_release
 *
 *      Open and release entry points for the UniFi debug driver.
 *
 *  Arguments:
 *      Normal linux driver args.
 *
 *  Returns:
 *      Linux error code.
 * ---------------------------------------------------------------------------
 */
static int
unifi_open(struct inode *inode, struct file *file)
{
    int devno;
    unifi_priv_t *priv;
    ul_client_t *udi_cli;

    func_enter();

    devno = MINOR(inode->i_rdev) >> 1;

    /*
     * Increase the ref_count for the char device clients.
     * Make sure you call uf_put_instance() to decreace it if
     * unifi_open returns an error.
     */
    priv = uf_get_instance(devno);
    if (priv == NULL) {
        unifi_error(NULL, "unifi_open: No device present\n");
        func_exit();
        return -ENODEV;
    }

    /* Register this instance in the client's list. */
    /* The minor number determines the nature of the client (Unicli or SME). */
    if (MINOR(inode->i_rdev) & 0x1) {
        udi_cli = ul_register_client(priv, CLI_USING_WIRE_FORMAT, udi_log_event);
        if (udi_cli == NULL) {
            /* Too many clients already using this device */
            unifi_error(priv, "Too many clients already open\n");
            uf_put_instance(devno);
            func_exit();
            return -ENOSPC;
        }
        unifi_trace(priv, UDBG1, "Client is registered to /dev/unifiudi%d\n", devno);
    } else {
        /*
         * Even-numbered device nodes are the control application.
         * This is the userspace helper containing SME or
         * unifi_manager.
         */

        down(&udi_mutex);

#ifdef CSR_SME_USERSPACE
        /* Check if a config client is already attached */
        if (priv->sme_cli) {
            up(&udi_mutex);
            uf_put_instance(devno);

            unifi_info(priv, "There is already a configuration client using the character device\n");
            func_exit();
            return -EBUSY;
        }
#endif /* CSR_SME_USERSPACE */

#ifdef CSR_SUPPORT_SME
        udi_cli = ul_register_client(priv,
                                     CLI_USING_WIRE_FORMAT | CLI_SME_USERSPACE,
                                     sme_log_event);
#else
        /* Config client for native driver */
        udi_cli = ul_register_client(priv,
                                     0,
                                     sme_native_log_event);
#endif
        if (udi_cli == NULL) {
            /* Too many clients already using this device */
            up(&udi_mutex);
            uf_put_instance(devno);

            unifi_error(priv, "Too many clients already open\n");
            func_exit();
            return -ENOSPC;
        }

#ifndef CSR_SME_EMB
        /*
         * Fill-in the pointer to the configuration client.
         * This is the SME userspace helper or unifi_manager.
         * Not used in the SME embedded version.
         */
        unifi_trace(priv, UDBG1, "SME client (id:%d s:0x%X) is registered\n",
                    udi_cli->client_id, udi_cli->sender_id);
        /* Store the SME UniFi Linux Client */
        if (priv->sme_cli == NULL) {
            priv->sme_cli = udi_cli;
        }
#endif

        up(&udi_mutex);
    }


    /*
     * Store the pointer to the client.
     * All char driver's entry points will pass this pointer.
     */
    file->private_data = udi_cli;

    func_exit();
    return 0;
} /* unifi_open() */



static int
unifi_release(struct inode *inode, struct file *filp)
{
    ul_client_t *udi_cli = (void*)filp->private_data;
    int devno;
    unifi_priv_t *priv;

    func_enter();

    priv = uf_find_instance(udi_cli->instance);
    if (!priv) {
        unifi_error(priv, "unifi_close: instance for device not found\n");
        return -ENODEV;
    }

    devno = MINOR(inode->i_rdev) >> 1;

    /* Even device nodes are the config client (i.e. SME or unifi_manager) */
    if ((MINOR(inode->i_rdev) & 0x1) == 0) {

#ifndef CSR_SME_EMB
        if (priv->sme_cli != udi_cli) {
            unifi_notice(priv, "Surprise closing config device: not the sme client\n");
        }
        unifi_trace(priv, UDBG2, "SME client close (unifi%d)\n", devno);

        /*
         * Clear sme_cli before calling unifi_sys_... so it doesn't try to
         * queue a reply to the (now gone) SME.
         */
        down(&udi_mutex);
        priv->sme_cli = NULL;
        up(&udi_mutex);
#endif

#ifdef CSR_SME_USERSPACE
        /* Power-down when config client closes */
        unifi_sys_wifi_off_req(priv, NULL);

        uf_sme_deinit(priv);
#endif /* CSR_SME_USERSPACE */
    } else {

        unifi_trace(priv, UDBG2, "UDI client close (unifiudi%d)\n", devno);

        /* If the pointer matches the logging client, stop logging. */
        down(&priv->udi_logging_mutex);
        if (udi_cli == priv->logging_client) {
            priv->logging_client = NULL;
        }
        up(&priv->udi_logging_mutex);

        if (udi_cli == priv->amp_client) {
            priv->amp_client = NULL;
        }
    }

    /* Deregister this instance from the client's list. */
    ul_deregister_client(udi_cli);

    uf_put_instance(devno);

    return 0;
} /* unifi_release() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_read
 *
 *      The read() driver entry point.
 *
 *  Arguments:
 *      filp        The file descriptor returned by unifi_open()
 *      p           The user space buffer to copy the read data
 *      len         The size of the p buffer
 *      poff
 *
 *  Returns:
 *      number of bytes read or an error code on failure
 * ---------------------------------------------------------------------------
 */
static ssize_t
unifi_read(struct file *filp, char *p, size_t len, loff_t *poff)
{
    ul_client_t *pcli = (void*)filp->private_data;
    unifi_priv_t *priv;
    udi_log_t *logptr = NULL;
    udi_msg_t *msgptr;
    struct list_head *l;
    int msglen;

    func_enter();

    priv = uf_find_instance(pcli->instance);
    if (!priv) {
        unifi_error(priv, "invalid priv\n");
        return -ENODEV;
    }

    if (!pcli->udi_enabled) {
        unifi_error(priv, "unifi_read: unknown client.");
        return -EINVAL;
    }

    if (list_empty(&pcli->udi_log)) {
        if (filp->f_flags & O_NONBLOCK) {
            /* Non-blocking - just return if the udi_log is empty */
            return 0;
        } else {
            /* Blocking - wait on the UDI wait queue */
            if (wait_event_interruptible(pcli->udi_wq,
                !list_empty(&pcli->udi_log)))
            {
                unifi_error(priv, "unifi_read: wait_event_interruptible failed.");
                return -ERESTARTSYS;
            }
        }
    }

    /* Read entry from list head and remove it from the list */
    if (down_interruptible(&pcli->udi_sem)) {
        return -ERESTARTSYS;
    }
    l = pcli->udi_log.next;
    list_del(l);
    up(&pcli->udi_sem);

    /* Get a pointer to whole struct */
    logptr = list_entry(l, udi_log_t, q);
    if (logptr == NULL) {
        unifi_error(priv, "unifi_read: failed to get event.\n");
        return -EINVAL;
    }

    /* Get the real message */
    msgptr = &logptr->msg;
    msglen = msgptr->length;
    if (msglen > len) {
        printk(KERN_WARNING "truncated read to %d actual msg len is %d\n", msglen, len);
        msglen = len;
    }

    /* and pass it to the client (SME or Unicli). */
    if (copy_to_user(p, msgptr, msglen))
    {
        printk(KERN_ERR "Failed to copy UDI log to user\n");
        kfree(logptr);
        return -EFAULT;
    }

    /* It is our resposibility to free the message buffer. */
    kfree(logptr);

    func_exit_r(msglen);
    return msglen;

} /* unifi_read() */



/*
 * ---------------------------------------------------------------------------
 * udi_send_signal_unpacked
 *
 *      Sends an unpacked signal to UniFi.
 *
 * Arguments:
 *      priv            Pointer to private context struct
 *      data            Pointer to request structure and data to send
 *      data_len        Length of data in data pointer.
 *
 * Returns:
 *      Number of bytes written, error otherwise.
 *
 * Notes:
 *      All clients that use this function to send a signal to the unifi
 *      must use the host formatted structures.
 * ---------------------------------------------------------------------------
 */
static int
udi_send_signal_unpacked(unifi_priv_t *priv, unsigned char* data, uint data_len)
{
    CSR_SIGNAL *sigptr = (CSR_SIGNAL*)data;
    CSR_DATAREF *datarefptr;
    bulk_data_param_t bulk_data;
    uint signal_size, i;
    uint bulk_data_offset = 0;
    int bytecount, r;

    /* Number of bytes in the signal */
    signal_size = SigGetSize(sigptr);
    if (!signal_size || (signal_size > data_len)) {
        unifi_error(priv, "unifi_sme_mlme_req - Invalid signal 0x%x size should be %d bytes\n",
                    sigptr->SignalPrimitiveHeader.SignalId,
                    signal_size);
        return -EINVAL;
    }
    bytecount = signal_size;

    /* Get a pointer to the information of the first data reference */
    datarefptr = (CSR_DATAREF*)&sigptr->u;

    /* Initialize the offset in the data buffer, bulk data is right after the signal. */
    bulk_data_offset = signal_size;

    /* store the references and the size of the bulk data to the bulkdata structure */
    for (i = 0; i < UNIFI_MAX_DATA_REFERENCES; i++) {
        /* the length of the bulk data is in the signal */
        if ((datarefptr+i)->DataLength) {
            void *dest;

            r = unifi_net_data_malloc(priv, &bulk_data.d[i], (datarefptr+i)->DataLength);
            if (r != 0) {
                unifi_error(priv, "udi_send_signal_unpacked: failed to allocate request_data.\n");
                return -EIO;
            }

            dest = (void*)bulk_data.d[i].os_data_ptr;
            memcpy(dest, data + bulk_data_offset, bulk_data.d[i].data_length);
        } else {
            bulk_data.d[i].data_length = 0;
        }

        bytecount += bulk_data.d[i].data_length;
        /* advance the offset, to point the next bulk data */
        bulk_data_offset += bulk_data.d[i].data_length;
    }


    unifi_trace(priv, UDBG3, "SME Send: signal %s\n",
                lookup_signal_name(sigptr->SignalPrimitiveHeader.SignalId));

    /* Send the signal. */
    r = ul_send_signal_unpacked(priv, sigptr, &bulk_data);
    if (r < 0) {
        unifi_error(priv, "udi_send_signal_unpacked: send failed (%d)\n", r);
        func_exit();
        return -EIO;
    }

    return bytecount;
} /* udi_send_signal_unpacked() */



/*
 * ---------------------------------------------------------------------------
 * udi_send_signal_raw
 *
 *      Sends a packed signal to UniFi.
 *
 * Arguments:
 *      priv            Pointer to private context struct
 *      buf             Pointer to request structure and data to send
 *      buflen          Length of data in data pointer.
 *
 * Returns:
 *      Number of bytes written, error otherwise.
 *
 * Notes:
 *      All clients that use this function to send a signal to the unifi
 *      must use the wire formatted structures.
 * ---------------------------------------------------------------------------
 */
static int
udi_send_signal_raw(unifi_priv_t *priv, const unsigned char *buf, int buflen)
{
    int signal_size;
    int sig_id;
    bulk_data_param_t data_ptrs;
    int i, r;
    unsigned int num_data_refs;
    int bytecount;
#ifdef CSR_NATIVE_LINUX
    CsrInt32 csr_r;
#endif

    func_enter();

    /*
     * The signal is the first thing in buf, the signal id is the
     * first 16 bits of the signal, so we can just pass the buffer to
     * get_packed_struct_size() as if it were a CSR_SIGNAL.
     */
    /* Number of bytes in the signal */
    sig_id = GET_SIGNAL_ID(buf);
    signal_size = get_packed_struct_size(buf);
    if (signal_size <= 0) {
        unifi_error(priv, "udi_send_signal_raw - Couldn't find length of signal 0x%x\n",
                    sig_id);
        func_exit();
        return -EINVAL;
    }

    unifi_trace(priv, UDBG2, "udi_send_signal_raw: signal %s len:%d\n",
                lookup_signal_name(sig_id), signal_size);
    /* Zero the data ref arrays */
    memset(&data_ptrs, 0, sizeof(data_ptrs));

    /*
     * Find the number of associated bulk data packets.  Scan through
     * the data refs to check that we have enough data and pick out
     * pointers to appended bulk data.
     */
    num_data_refs = 0;
    bytecount = signal_size;

    for (i = 0; i < UNIFI_MAX_DATA_REFERENCES; ++i)
    {
        unsigned int len = GET_PACKED_DATAREF_LEN(buf, i);
        unifi_trace(priv, UDBG3, "udi_send_signal_raw: data_ref length = %d\n", len);

        if (len != 0) {
            void *dest;

            r = unifi_net_data_malloc(priv, &data_ptrs.d[i], len);
            if (r != 0) {
                unifi_error(priv, "udi_send_signal_raw: failed to allocate request_data.\n");
                return -EIO;
            }

            dest = (void*)data_ptrs.d[i].os_data_ptr;
            memcpy(dest, buf + bytecount, len);

            bytecount += len;
            num_data_refs++;
        }
        data_ptrs.d[i].data_length = len;
    }

    unifi_trace(priv, UDBG3, "Queueing signal 0x%X %s from UDI with %u data refs\n",
          sig_id,
          lookup_signal_name(sig_id),
          num_data_refs);

    if (bytecount > buflen) {
        unifi_error(priv, "udi_send_signal_raw: Not enough data (%d instead of %d)\n", buflen, bytecount);
        func_exit();
        return -EINVAL;
    }

    /* Send the signal calling the function that uses the wire-formatted signals. */
    r = ul_send_signal_raw(priv, buf, signal_size, &data_ptrs);
    if (r < 0) {
        unifi_error(priv, "udi_send_signal_raw: send failed (%d)\n", r);
        func_exit();
        return -EIO;
    }

#ifdef CSR_NATIVE_LINUX
    if (sig_id == CSR_MLME_POWERMGT_REQUEST_ID) {
        /* Overide the wext power mode to the new value */
        priv->wext_conf.power_mode = COAL_GET_UINT16_FROM_LITTLE_ENDIAN((buf +
                                              SIZEOF_SIGNAL_HEADER + (UNIFI_MAX_DATA_REFERENCES*SIZEOF_DATAREF)));

        /* Configure deep sleep signaling */
        if (priv->wext_conf.power_mode || (priv->connected == UnifiNotConnected)) {
            csr_r = unifi_configure_low_power_mode(priv->card,
                                                   UNIFI_LOW_POWER_ENABLED,
                                                   UNIFI_PERIODIC_WAKE_HOST_DISABLED);
        } else {
            csr_r = unifi_configure_low_power_mode(priv->card,
                                                   UNIFI_LOW_POWER_DISABLED,
                                                   UNIFI_PERIODIC_WAKE_HOST_DISABLED);
        }
    }
#endif

    func_exit_r(bytecount);

    return bytecount;
} /* udi_send_signal_raw */

/*
 * ---------------------------------------------------------------------------
 *  unifi_write
 *
 *      The write() driver entry point.
 *      A UniFi Debug Interface client such as unicli can write a signal
 *      plus bulk data to the driver for sending to the UniFi chip.
 *
 *      Only one signal may be sent per write operation.
 *
 *  Arguments:
 *      filp        The file descriptor returned by unifi_open()
 *      p           The user space buffer to get the data from
 *      len         The size of the p buffer
 *      poff
 *
 *  Returns:
 *      number of bytes written or an error code on failure
 * ---------------------------------------------------------------------------
 */
static ssize_t
unifi_write(struct file *filp, const char *p, size_t len, loff_t *poff)
{
    ul_client_t *pcli = (ul_client_t*)filp->private_data;
    unifi_priv_t *priv;
    unsigned char *buf;
    unsigned char *bufptr;
    int remaining;
    int bytes_written;
    int r;
    bulk_data_param_t bulkdata;

    func_enter();

    priv = uf_find_instance(pcli->instance);
    if (!priv) {
        unifi_error(priv, "invalid priv\n");
        return -ENODEV;
    }

    unifi_trace(priv, UDBG5, "unifi_write: len = %d\n", len);

    if (!pcli->udi_enabled) {
        unifi_error(priv, "udi disabled\n");
        return -EINVAL;
    }

    /*
     * AMP client sends only one signal at a time, so we can use
     * unifi_net_data_malloc to save the extra copy.
     */
    if (pcli == priv->amp_client) {
        int signal_size;
        int sig_id;
        unsigned char *signal_buf;
        char *user_data_buf;

        r = unifi_net_data_malloc(priv, &bulkdata.d[0], len);
        if (r) {
            unifi_error(priv, "unifi_write: failed to allocate request_data.\n");
            func_exit();
            return -ENOMEM;
        }

        user_data_buf = (char*)bulkdata.d[0].os_data_ptr;

        /* Get the data from the AMP client. */
        if (copy_from_user((void*)user_data_buf, p, len)) {
            unifi_error(priv, "unifi_write: copy from user failed\n");
            unifi_net_data_free(priv, &bulkdata.d[0]);
            func_exit();
            return -EFAULT;
        }

        bulkdata.d[1].data_length = 0;

        /* Number of bytes in the signal */
        sig_id = GET_SIGNAL_ID(bulkdata.d[0].os_data_ptr);
        signal_size = get_packed_struct_size(bulkdata.d[0].os_data_ptr);
        if ((signal_size <= 0) || (signal_size > len)) {
            unifi_error(priv, "unifi_write - Couldn't find length of signal 0x%x\n",
                        sig_id);
            unifi_net_data_free(priv, &bulkdata.d[0]);
            func_exit();
            return -EINVAL;
        }

        unifi_trace(priv, UDBG2, "unifi_write: signal %s len:%d\n",
                    lookup_signal_name(sig_id), signal_size);

        /* Allocate a buffer for the signal */
        signal_buf = kmalloc(signal_size, GFP_KERNEL);
        if (!signal_buf) {
            unifi_net_data_free(priv, &bulkdata.d[0]);
            func_exit();
            return -ENOMEM;
        }

        /* Get the signal from the os_data_ptr */
        memcpy(signal_buf, bulkdata.d[0].os_data_ptr, signal_size);
        signal_buf[5] = (pcli->sender_id >> 8) & 0xff;

        if (signal_size < len) {
            /* Remove the signal from the os_data_ptr */
            bulkdata.d[0].data_length -= signal_size;
            bulkdata.d[0].os_data_ptr += signal_size;
        } else {
            bulkdata.d[0].data_length = 0;
            bulkdata.d[0].os_data_ptr = NULL;
        }

        /* Send the signal calling the function that uses the wire-formatted signals. */
        r = ul_send_signal_raw(priv, signal_buf, signal_size, &bulkdata);
        if (r < 0) {
            unifi_error(priv, "unifi_write: send failed (%d)\n", r);
            if (bulkdata.d[0].os_data_ptr != NULL) {
                unifi_net_data_free(priv, &bulkdata.d[0]);
            }
        }

        /* Free the signal buffer and return */
        kfree(signal_buf);
        return len;
    }

    buf = kmalloc(len, GFP_KERNEL);
    if (!buf) {
        return -ENOMEM;
    }

    /* Get the data from the client (SME or Unicli). */
    if (copy_from_user((void*)buf, p, len)) {
        unifi_error(priv, "copy from user failed\n");
        kfree(buf);
        return -EFAULT;
    }

    /*
     * In SME userspace build read() contains a SYS or MGT message.
     * Note that even though the SME sends one signal at a time, we can not
     * use unifi_net_data_malloc because in the early stages, before having
     * initialised the core, it will fail since the I/O block size is unknown.
     */
#ifdef CSR_SME_USERSPACE
    if (pcli->configuration & CLI_SME_USERSPACE) {
        /*
         * Should change this to check for HIP signal id and fall-through
         * if it's a HIP signal.
         */
        r = receive_remote_sys_hip_req(priv->smepriv, buf, len); /* Hip Messages */
        if (!r) {
            r = remote_sys_signal_receive(priv->smepriv, buf, len);
            if (!r) {
                r = remote_mgt_signal_receive(priv->smepriv, buf, len);
                if (!r) {
                    unifi_error(priv, "SME can not process the message\n");
                    /* FIXME: For debug only, remove it later */
                    dump(buf, len);
                }
            }
        }

        kfree(buf);
        return len;
    }
#endif

    /* ul_send_signal_raw will  do a sanity check of len against signal content */

    /*
     * udi_send_signal_raw() and udi_send_signal_unpacked() return the number of bytes consumed.
     * A write call can pass multiple signal concatenated together.
     */
    bytes_written = 0;
    remaining = len;
    bufptr = buf;
    while (remaining > 0)
    {
        int r;

        /*
         * Set the SenderProcessId.
         * The SignalPrimitiveHeader is the first 3 16-bit words of the signal,
         * the SenderProcessId is bytes 4,5.
         * The MSB of the sender ID needs to be set to the client ID.
         * The LSB is controlled by the SME.
         */
        bufptr[5] = (pcli->sender_id >> 8) & 0xff;

        /* use the appropriate interface, depending on the clients' configuration */
        if (pcli->configuration & CLI_USING_WIRE_FORMAT) {
            unifi_trace(priv, UDBG1, "unifi_write: call udi_send_signal().\n");
            r = udi_send_signal_raw(priv, bufptr, remaining);
        } else {
            r = udi_send_signal_unpacked(priv, bufptr, remaining);
        }
        if (r < 0) {
            /* Set the return value to the error code */
            unifi_error(priv, "unifi_write: (udi or sme)_send_signal() returns %d\n", r);
            bytes_written = r;
            break;
        }
        bufptr += r;
        remaining -= r;
        bytes_written += r;
    }

    kfree(buf);

    func_exit_r(bytes_written);

    return bytes_written;
} /* unifi_write() */





/*
 * ----------------------------------------------------------------
 *  unifi_ioctl
 *
 *      Ioctl handler for unifi driver.
 *
 * Arguments:
 *	inodep          Pointer to inode structure.
 *	filp            Pointer to file structure.
 *	cmd             Ioctl cmd passed by user.
 *	arg             Ioctl arg passed by user.
 *
 * Returns:
 *      0 on success, -ve error code on error.
 * ----------------------------------------------------------------
 */
#if 0
static int
unifi_ioctl(struct inode *inodep, struct file *filp,
            unsigned int cmd, unsigned long arg)
#else
static int
unifi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
    ul_client_t *pcli = (ul_client_t*)filp->private_data;
    unifi_priv_t *priv;
    struct net_device *dev;
    int rc = 0;
    int int_param, i;
    u8* buf;
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
    unifi_cfg_command_t cfg_cmd;
    unifi_CoexConfig coex_config;
    unifi_AppValue sme_app_value;
    unsigned char uchar_param;
    unsigned char varbind[MAX_VARBIND_LENGTH];
    int vblen;
#endif
    unifi_putest_command_t putest_cmd;

    priv = uf_find_instance(pcli->instance);
    if (!priv) {
        unifi_error(priv, "ioctl error: unknown instance=%d\n", pcli->instance);
        return -ENODEV;
    }
    unifi_trace(priv, UDBG5, "unifi_ioctl: cmd=0x%X, arg=0x%lX\n", cmd, arg);

    dev = priv->netdev;

    switch (cmd) {

      case UNIFI_GET_UDI_ENABLE:
        unifi_trace(priv, UDBG4, "UniFi Get UDI Enable\n");

        down(&priv->udi_logging_mutex);
        int_param = (priv->logging_client == NULL) ? 0 : 1;
        up(&priv->udi_logging_mutex);

        if (put_user(int_param, (int*)arg))
        {
            unifi_error(priv, "UNIFI_GET_UDI_ENABLE: Failed to copy to user\n");
            return -EFAULT;
        }
        break;

      case UNIFI_SET_UDI_ENABLE:
        unifi_trace(priv, UDBG4, "UniFi Set UDI Enable\n");
        if (get_user(int_param, (int*)arg))
        {
            unifi_error(priv, "UNIFI_SET_UDI_ENABLE: Failed to copy from user\n");
            return -EFAULT;
        }

        down(&priv->udi_logging_mutex);
        if (int_param) {
            pcli->event_hook = udi_log_event;
            unifi_set_udi_hook(priv->card, logging_handler);
            /* Log all signals by default */
            for (i = 0; i < SIG_FILTER_SIZE; i++) {
                pcli->signal_filter[i] = 0xFFFF;
            }
            priv->logging_client = pcli;

        } else {
            priv->logging_client = NULL;
            pcli->event_hook = NULL;
        }
        up(&priv->udi_logging_mutex);

        break;

      case UNIFI_SET_MIB:
        unifi_trace(priv, UDBG4, "UniFi Set MIB\n");
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
        /* Read first 2 bytes and check length */
        if (copy_from_user((void*)varbind, (void*)arg, 2)) {
            unifi_error(priv,
                        "UNIFI_SET_MIB: Failed to copy in varbind header\n");
            return -EFAULT;
        }
        vblen = varbind[1];
        if ((vblen + 2) > MAX_VARBIND_LENGTH) {
            unifi_error(priv,
                        "UNIFI_SET_MIB: Varbind too long (%d, limit %d)\n",
                        (vblen+2), MAX_VARBIND_LENGTH);
            return -EINVAL;
        }
        /* Read rest of varbind */
        if (copy_from_user((void*)(varbind+2), (void*)(arg+2), vblen)) {
            unifi_error(priv, "UNIFI_SET_MIB: Failed to copy in varbind\n");
            return -EFAULT;
        }

        /* send to SME */
        vblen += 2;
        rc = sme_mgt_mib_set(priv, varbind, vblen);
        if (rc) {
            return rc;
        }
#else
        unifi_notice(priv, "UNIFI_SET_MIB: Unsupported.\n");
#endif /* CSR_SUPPORT_WEXT */
        break;

      case UNIFI_GET_MIB:
        unifi_trace(priv, UDBG4, "UniFi Get MIB\n");
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
        /* Read first 2 bytes and check length */
        if (copy_from_user((void*)varbind, (void*)arg, 2)) {
            unifi_error(priv, "UNIFI_GET_MIB: Failed to copy in varbind header\n");
            return -EFAULT;
        }
        vblen = varbind[1];
        if ((vblen+2) > MAX_VARBIND_LENGTH) {
            unifi_error(priv, "UNIFI_GET_MIB: Varbind too long (%d, limit %d)\n",
                        (vblen+2), MAX_VARBIND_LENGTH);
            return -EINVAL;
        }
        /* Read rest of varbind */
        if (copy_from_user((void*)(varbind+2), (void*)(arg+2), vblen)) {
            unifi_error(priv, "UNIFI_GET_MIB: Failed to copy in varbind\n");
            return -EFAULT;
        }

        vblen += 2;
        rc = sme_mgt_mib_get(priv, varbind, &vblen);
        if (rc) {
            return rc;
        }
        /* copy out varbind */
        if (vblen > MAX_VARBIND_LENGTH) {
            unifi_error(priv,
                        "UNIFI_GET_MIB: Varbind result too long (%d, limit %d)\n",
                        vblen, MAX_VARBIND_LENGTH);
            return -EINVAL;
        }
        if (copy_to_user((void*)arg, varbind, vblen)) {
            return -EFAULT;
        }
#else
        unifi_notice(priv, "UNIFI_GET_MIB: Unsupported.\n");
#endif /* CSR_SUPPORT_WEXT */
        break;

      case UNIFI_CFG:
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
        if (get_user(cfg_cmd, (unifi_cfg_command_t*)arg))
        {
            unifi_error(priv, "UNIFI_CFG: Failed to get the command\n");
            return -EFAULT;
        }

        unifi_trace(priv, UDBG1, "UNIFI_CFG: Command is %d (t=%u) sz=%d\n",
                    cfg_cmd, jiffies_to_msecs(jiffies), sizeof(unifi_cfg_command_t));
        switch (cfg_cmd) {
          case UNIFI_CFG_POWER:
            rc = unifi_cfg_power(priv, (unsigned char*)arg);
            break;
          case UNIFI_CFG_POWERSAVE:
            rc = unifi_cfg_power_save(priv, (unsigned char*)arg);
            break;
          case UNIFI_CFG_POWERSUPPLY:
            rc = unifi_cfg_power_supply(priv, (unsigned char*)arg);
            break;
          case UNIFI_CFG_FILTER:
            rc = unifi_cfg_packet_filters(priv, (unsigned char*)arg);
            break;
          case UNIFI_CFG_GET:
            rc = unifi_cfg_get_info(priv, (unsigned char*)arg);
            break;
          case UNIFI_CFG_WMM_QOSINFO:
            rc = unifi_cfg_wmm_qos_info(priv, (unsigned char*)arg);
            break;
          case UNIFI_CFG_WMM_ADDTS:
            rc = unifi_cfg_wmm_addts(priv, (unsigned char*)arg);
            break;
          case UNIFI_CFG_WMM_DELTS:
            rc = unifi_cfg_wmm_delts(priv, (unsigned char*)arg);
            break;
          case UNIFI_CFG_STRICT_DRAFT_N:
            rc = unifi_cfg_strict_draft_n(priv, (unsigned char*)arg);
            break;
          case UNIFI_CFG_ENABLE_OKC:
            rc = unifi_cfg_enable_okc(priv, (unsigned char*)arg);
            break;
          default:
            unifi_error(priv, "UNIFI_CFG: Unknown Command (%d)\n", cfg_cmd);
            return -EINVAL;
        }
#endif

        break;

      case UNIFI_PUTEST:
        if (get_user(putest_cmd, (unifi_putest_command_t*)arg))
        {
            unifi_error(priv, "UNIFI_PUTEST: Failed to get the command\n");
            return -EFAULT;
        }

        unifi_trace(priv, UDBG1, "UNIFI_PUTEST: Command is %s\n",
                    trace_putest_cmdid(putest_cmd));
        switch (putest_cmd) {
          case UNIFI_PUTEST_START:
            rc = unifi_putest_start(priv, (unsigned char*)arg);
            break;
          case UNIFI_PUTEST_STOP:
            rc = unifi_putest_stop(priv, (unsigned char*)arg);
            break;
          case UNIFI_PUTEST_SET_SDIO_CLOCK:
            rc = unifi_putest_set_sdio_clock(priv, (unsigned char*)arg);
            break;
          case UNIFI_PUTEST_CMD52_READ:
            rc = unifi_putest_cmd52_read(priv, (unsigned char*)arg);
            break;
          case UNIFI_PUTEST_CMD52_WRITE:
            rc = unifi_putest_cmd52_write(priv, (unsigned char*)arg);
            break;
          case UNIFI_PUTEST_DL_FW:
            rc = unifi_putest_dl_fw(priv, (unsigned char*)arg);
            break;
          case UNIFI_PUTEST_DL_FW_BUFF:
            rc = unifi_putest_dl_fw_buff(priv, (unsigned char*)arg);
            break;
          default:
            unifi_error(priv, "UNIFI_PUTEST: Unknown Command (%d)\n", putest_cmd);
            return -EINVAL;
        }

        break;

      case UNIFI_INIT_HW:
        unifi_trace(priv, UDBG2, "UNIFI_INIT_HW.\n");
        priv->init_progress = UNIFI_INIT_NONE;

#ifdef CSR_SUPPORT_WEXT
        /* At this point we are ready to start the SME. */
        rc = sme_mgt_wifi_on(priv);
        if (rc) {
            return rc;
        }
#endif

        break;

      case UNIFI_INIT_NETDEV:
        unifi_trace(priv, UDBG2, "UNIFI_INIT_NETDEV.\n");
        if (copy_from_user((void*)dev->dev_addr, (void*)arg, 6)) {
            return -EFAULT;
        }

        /* Attach the network device to the stack */
        if (!priv->netdev_registered)
        {
            rc = uf_register_netdev(priv);
            if (rc) {
                unifi_error(priv, "Failed to register the network device.\n");
                return rc;
            }
        }

        /* Apply scheduled interrupt mode, if requested by module param */
        if (run_bh_once != -1) {
            unifi_set_interrupt_mode(priv->card, (CsrUint32)run_bh_once);
        }

        priv->init_progress = UNIFI_INIT_COMPLETED;

        /* Firmware initialisation is complete, so let the SDIO bus
         * clock be raised when convienent to the core.
         */
        unifi_request_max_sdio_clock(priv->card);

#ifdef CSR_SUPPORT_WEXT
        /* Notify the Android wpa_supplicant that we are ready */
        wext_send_started_event(priv);
#endif

        unifi_info(priv, "UniFi ready\n");
        break;

      case UNIFI_GET_INIT_STATUS:
        unifi_trace(priv, UDBG2, "UNIFI_GET_INIT_STATUS.\n");
        if (put_user(priv->init_progress, (int*)arg))
        {
            printk(KERN_ERR "UNIFI_GET_INIT_STATUS: Failed to copy to user\n");
            return -EFAULT;
        }
        break;

      case UNIFI_KICK:
        unifi_trace(priv, UDBG4, "Kick UniFi\n");
        unifi_sdio_interrupt_handler(priv->card);
        break;

      case UNIFI_SET_DEBUG:
        unifi_debug = arg;
        unifi_trace(priv, UDBG4, "unifi_debug set to %d\n", unifi_debug);
        break;

      case UNIFI_SET_TRACE:
        /* no longer supported */
        rc = -EINVAL;
        break;

      case UNIFI_SET_UDI_LOG_MASK:
        {
            unifiio_filter_t udi_filter;
            uint16_t *sig_ids_addr;
#define UF_MAX_SIG_IDS  128     /* Impose a sensible limit */

            if (copy_from_user((void*)(&udi_filter), (void*)arg, sizeof(udi_filter))) {
                return -EFAULT;
            }
            if ((udi_filter.action < UfSigFil_AllOn) ||
                (udi_filter.action > UfSigFil_SelectOff))
            {
                printk(KERN_WARNING
                       "UNIFI_SET_UDI_LOG_MASK: Bad action value: %d\n",
                       udi_filter.action);
                return -EINVAL;
            }
            /* No signal list for "All" actions */
            if ((udi_filter.action == UfSigFil_AllOn) ||
                (udi_filter.action == UfSigFil_AllOff))
            {
                udi_filter.num_sig_ids = 0;
            }

            if (udi_filter.num_sig_ids > UF_MAX_SIG_IDS) {
                printk(KERN_WARNING
                       "UNIFI_SET_UDI_LOG_MASK: too many signal ids (%d, max %d)\n",
                       udi_filter.num_sig_ids, UF_MAX_SIG_IDS);
                return -EINVAL;
            }

            /* Copy in signal id list if given */
            if (udi_filter.num_sig_ids > 0) {
                /* Preserve userspace address of sig_ids array */
                sig_ids_addr = udi_filter.sig_ids;
                /* Allocate kernel memory for sig_ids and copy to it */
                udi_filter.sig_ids =
                    kmalloc(udi_filter.num_sig_ids * sizeof(uint16_t), GFP_KERNEL);
                if (!udi_filter.sig_ids) {
                    return -ENOMEM;
                }
                if (copy_from_user((void*)udi_filter.sig_ids,
                                   (void*)sig_ids_addr,
                                   udi_filter.num_sig_ids * sizeof(uint16_t)))
                {
                    kfree(udi_filter.sig_ids);
                    return -EFAULT;
                }
            }

            udi_set_log_filter(pcli, &udi_filter);

            if (udi_filter.num_sig_ids > 0) {
                kfree(udi_filter.sig_ids);
            }
        }
        break;

      case UNIFI_SET_AMP_ENABLE:
        unifi_trace(priv, UDBG4, "UniFi Set AMP Enable\n");
        if (get_user(int_param, (int*)arg))
        {
            unifi_error(priv, "UNIFI_SET_AMP_ENABLE: Failed to copy from user\n");
            return -EFAULT;
        }

        if (int_param) {
            priv->amp_client = pcli;
        } else {
            priv->amp_client = NULL;
        }

        int_param = 0;
        buf = (u8*)&int_param;
        buf[0] = UNIFI_SOFT_COMMAND_Q_LENGTH - 1;
        buf[1] = UNIFI_SOFT_TRAFFIC_Q_LENGTH - 1;
        if (copy_to_user((void*)arg, &int_param, sizeof(int))) {
            return -EFAULT;
        }
        break;

      case UNIFI_SET_UDI_SNAP_MASK:
        {
            unifiio_snap_filter_t snap_filter;

            if (copy_from_user((void*)(&snap_filter), (void*)arg, sizeof(snap_filter))) {
                return -EFAULT;
            }

            if (pcli->snap_filter.count) {
                pcli->snap_filter.count = 0;
                CsrPfree(pcli->snap_filter.protocols);
            }

            if (snap_filter.count == 0) {
                break;
            }

            pcli->snap_filter.protocols = CsrPmalloc(snap_filter.count * sizeof(CsrUint16));
            if (!pcli->snap_filter.protocols) {
                return -ENOMEM;
            }
            if (copy_from_user((void*)pcli->snap_filter.protocols,
                               (void*)snap_filter.protocols,
                               snap_filter.count * sizeof(CsrUint16)))
            {
                CsrPfree(pcli->snap_filter.protocols);
                return -EFAULT;
            }

            pcli->snap_filter.count = snap_filter.count;

        }
        break;

      case UNIFI_SME_PRESENT:
        {
            u8 ind;
            unifi_trace(priv, UDBG4, "UniFi SME Present IOCTL.\n");
            if (copy_from_user((void*)(&int_param), (void*)arg, sizeof(int)))
            {
                printk(KERN_ERR "UNIFI_SME_PRESENT: Failed to copy from user\n");
                return -EFAULT;
            }

            priv->sme_is_present = int_param;
            if (priv->sme_is_present == 1) {
                ind = CONFIG_SME_PRESENT;
            } else {
                ind = CONFIG_SME_NOT_PRESENT;
            }
            /* Send an indication to the helper app. */
            ul_log_config_ind(priv, &ind, sizeof(u8));
        }
        break;

      case UNIFI_CFG_PERIOD_TRAFFIC:
        unifi_trace(priv, UDBG4, "UniFi Configure Periodic Traffic.\n");
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
        if (copy_from_user((void*)(&uchar_param), (void*)arg, sizeof(unsigned char))) {
            unifi_error(priv, "UNIFI_CFG_PERIOD_TRAFFIC: Failed to copy from user\n");
            return -EFAULT;
        }

        if (uchar_param == 0) {
            sme_app_value.id = unifi_CoexConfigValue;
            rc = sme_mgt_get_value(priv, &sme_app_value);
            if (rc) {
                unifi_error(priv, "UNIFI_CFG_PERIOD_TRAFFIC: Get unifi_CoexInfoValue failed.\n");
                return rc;
            }
            if (copy_to_user((void*)(arg + 1),
                             (void*)&sme_app_value.unifi_Value_union.coexConfig,
                             sizeof(unifi_CoexConfig))) {
                return -EFAULT;
            }
            return 0;
        }

        if (copy_from_user((void*)(&coex_config), (void*)(arg + 1), sizeof(unifi_CoexConfig)))
        {
            unifi_error(priv, "UNIFI_CFG_PERIOD_TRAFFIC: Failed to copy from user\n");
            return -EFAULT;
        }

        sme_app_value.unifi_Value_union.coexConfig = coex_config;
        sme_app_value.id = unifi_CoexConfigValue;
        rc = sme_mgt_set_value(priv, &sme_app_value);
        if (rc) {
            unifi_error(priv, "UNIFI_CFG_PERIOD_TRAFFIC: Set unifi_CoexInfoValue failed.\n");
            return rc;
        }

#endif /* CSR_SUPPORT_SME && CSR_SUPPORT_WEXT */
        break;

      case UNIFI_CFG_UAPSD_TRAFFIC:
        unifi_trace(priv, UDBG4, "UniFi Configure U-APSD Mask.\n");
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
        if (copy_from_user((void*)(&uchar_param), (void*)arg, sizeof(unsigned char))) {
            unifi_error(priv, "UNIFI_CFG_UAPSD_TRAFFIC: Failed to copy from user\n");
            return -EFAULT;
        }
        unifi_trace(priv, UDBG4, "New U-APSD Mask: 0x%x\n", uchar_param);
#endif /* CSR_SUPPORT_SME && CSR_SUPPORT_WEXT */
        break;

#ifndef UNIFI_DISABLE_COREDUMP
      case UNIFI_COREDUMP_GET_REG:
        unifi_trace(priv, UDBG4, "UniFi Coredump request\n");
        {
            unifiio_coredump_req_t dump_req;    /* Public */
            unifi_coredump_req_t priv_req;      /* Private */

            if (copy_from_user((void*)(&dump_req), (void*)arg, sizeof(dump_req))) {
                return -EFAULT;
            }
            memset(&priv_req, 0, sizeof(priv_req));
            priv_req.index = dump_req.index;
            priv_req.offset = dump_req.offset;
            priv_req.space = dump_req.space;

            if (priv_req.space == UNIFI_COREDUMP_TRIGGER_MAGIC) {
                /* Force a coredump grab now */
                unifi_trace(priv, UDBG2, "UNIFI_COREDUMP_GET_REG: Force capture\n");
                rc = unifi_coredump_capture(priv->card, &priv_req);
                unifi_trace(priv, UDBG5, "UNIFI_COREDUMP_GET_REG: status %d\n", rc);
            } else {
                /* Retreive the appropriate register entry */
                unifi_trace(priv, UDBG4, "Get %d, 0x%x:%x\n",
                            priv_req.index, priv_req.space, priv_req.offset);
                rc =  unifi_coredump_get_value(priv->card, &priv_req);
                if (rc) {
                    unifi_trace(priv, UDBG5, "UNIFI_COREDUMP_GET_REG: Status %d\n", rc);
                    return rc;
                }
                dump_req.value = priv_req.value;
                /* The priv_req.timestamp is not filled by the HIP lib */
                dump_req.timestamp = (unsigned int)jiffies_to_msecs(jiffies);
                dump_req.requestor = priv_req.requestor;
                dump_req.serial = priv_req.serial;
                dump_req.chip_ver = priv_req.chip_ver;
                dump_req.fw_ver = priv_req.fw_ver;
                dump_req.drv_build = UNIFI_DRIVER_BUILD_ID;

                unifi_trace(priv, UDBG6,
                            "Dump: %d (seq %d): V:0x%04x (%d) @0x%02x:%04x = 0x%04x\n",
                            dump_req.index, dump_req.serial,
                            dump_req.chip_ver, dump_req.drv_build,
                            dump_req.space, dump_req.offset, dump_req.value);
            }
            if (copy_to_user((void*)arg, (void*)&dump_req, sizeof(dump_req))) {
                return -EFAULT;
            }
        }
        break;
#endif
      default:
        rc = -EINVAL;
    }

    return rc;
} /* unifi_ioctl() */



static unsigned int
unifi_poll(struct file *filp, poll_table *wait)
{
    ul_client_t *pcli = (ul_client_t*)filp->private_data;
    unsigned int mask = 0;
    int ready;

    func_enter();

    ready = !list_empty(&pcli->udi_log);

    poll_wait(filp, &pcli->udi_wq, wait);

    if (ready) {
        mask |= POLLIN | POLLRDNORM;    /* readable */
    }

    func_exit();

    return mask;
} /* unifi_poll() */



/*
 * ---------------------------------------------------------------------------
 *  udi_set_log_filter
 *
 *      Configure the bit mask that determines which signal primitives are
 *      passed to the logging process.
 *
 *  Arguments:
 *      pcli            Pointer to the client to configure.
 *      udi_filter      Pointer to a unifiio_filter_t containing instructions.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      SigGetFilterPos() returns a 32-bit value that contains an index and a
 *      mask for accessing a signal_filter array. The top 16 bits specify an
 *      index into a signal_filter, the bottom 16 bits specify a mask to
 *      apply.
 * ---------------------------------------------------------------------------
 */
static void
udi_set_log_filter(ul_client_t *pcli, unifiio_filter_t *udi_filter)
{
    CsrUint32 filter_pos;
    int i;

    if (udi_filter->action == UfSigFil_AllOn)
    {
        for (i = 0; i < SIG_FILTER_SIZE; i++) {
            pcli->signal_filter[i] = 0xFFFF;
        }
    }
    else if (udi_filter->action == UfSigFil_AllOff)
    {
        for (i = 0; i < SIG_FILTER_SIZE; i++) {
            pcli->signal_filter[i] = 0;
        }
    }
    else if (udi_filter->action == UfSigFil_SelectOn)
    {
        for (i = 0; i < udi_filter->num_sig_ids; i++) {
            filter_pos = SigGetFilterPos(udi_filter->sig_ids[i]);
            if (filter_pos == 0xFFFFFFFF)
            {
                printk(KERN_WARNING
                       "Unrecognised signal id (0x%X) specifed in logging filter\n",
                       udi_filter->sig_ids[i]);
            } else {
                pcli->signal_filter[filter_pos >> 16] |= (filter_pos & 0xFFFF);
            }
        }
    }
    else if (udi_filter->action == UfSigFil_SelectOff)
    {
        for (i = 0; i < udi_filter->num_sig_ids; i++) {
            filter_pos = SigGetFilterPos(udi_filter->sig_ids[i]);
            if (filter_pos == 0xFFFFFFFF)
            {
                printk(KERN_WARNING
                       "Unrecognised signal id (0x%X) specifed in logging filter\n",
                       udi_filter->sig_ids[i]);
            } else {
                pcli->signal_filter[filter_pos >> 16] &= ~(filter_pos & 0xFFFF);
            }
        }
    }

} /* udi_set_log_filter() */


/*
 * ---------------------------------------------------------------------------
 *  udi_log_event
 *
 *      Callback function to be registered as the UDI hook callback.
 *      Copies the signal content into a new udi_log_t struct and adds
 *      it to the read queue for this UDI client.
 *
 *  Arguments:
 *      pcli            A pointer to the client instance.
 *      signal          Pointer to the received signal.
 *      signal_len      Size of the signal structure in bytes.
 *      bulkdata        Pointers to any associated bulk data.
 *      dir             Direction of the signal. Zero means from host,
 *                      non-zero means to host.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
udi_log_event(ul_client_t *pcli,
              const u8 *signal, int signal_len,
              const bulk_data_param_t *bulkdata,
              int dir)
{
    udi_log_t *logptr;
    u8 *p;
    int i;
    int total_len;
    udi_msg_t *msgptr;
    CsrUint32 filter_pos;

    func_enter();

    /* Just a sanity check */
    if ((signal == NULL) || (signal_len <= 0)) {
        return;
    }

#ifdef CSR_NATIVE_LINUX
    uf_native_process_udi_signal(pcli, signal, signal_len, bulkdata, dir);
#endif

    /*
     * Apply the logging filter - only report signals that have their
     * bit set in the filter mask.
     */
    filter_pos = SigGetFilterPos(GET_SIGNAL_ID(signal));

    if ((filter_pos != 0xFFFFFFFF) &&
        ((pcli->signal_filter[filter_pos >> 16] & (filter_pos & 0xFFFF)) == 0))
    {
        /* Signal is not wanted by client */
        return;
    }

    /* Calculate the buffer we need to store signal plus bulk data */
    total_len = signal_len;
    for (i = 0; i < UNIFI_MAX_DATA_REFERENCES; i++) {
        total_len += bulkdata->d[i].data_length;
    }

    /* Allocate log structure plus actual signal. */
    logptr = (udi_log_t *)kmalloc(sizeof(udi_log_t) + total_len, GFP_KERNEL);

    if (logptr == NULL) {
        printk(KERN_ERR
               "Failed to allocate %d bytes for a UDI log record\n",
               sizeof(udi_log_t) + total_len);
        return;
    }

    /* Fill in udi_log struct */
    INIT_LIST_HEAD(&logptr->q);
    msgptr = &logptr->msg;
    msgptr->length = sizeof(udi_msg_t) + total_len;
    msgptr->timestamp = jiffies_to_msecs(jiffies);
    msgptr->direction = dir;
    msgptr->signal_length = signal_len;

    /* Copy signal and bulk data to the log */
    p = (u8 *)(msgptr + 1);
    memcpy(p, signal, signal_len);
    p += signal_len;

    /* Append any bulk data */
    for (i = 0; i < UNIFI_MAX_DATA_REFERENCES; i++) {
        int len = bulkdata->d[i].data_length;

        /*
         * Len here might not be the same as the length in the bulk data slot.
         * The slot length will always be even, but len could be odd.
         */
        if (len > 0) {
            if (bulkdata->d[i].os_data_ptr) {
                memcpy(p, bulkdata->d[i].os_data_ptr, len);
            } else {
                memset(p, 0, len);
            }
            p += len;
        }
    }

    /* Add to tail of log queue */
    if (down_interruptible(&pcli->udi_sem)) {
        printk(KERN_WARNING "udi_log_event_q: Failed to get udi sem\n");
        kfree(logptr);
        func_exit();
        return;
    }
    list_add_tail(&logptr->q, &pcli->udi_log);
    up(&pcli->udi_sem);

    /* Wake any waiting user process */
    wake_up_interruptible(&pcli->udi_wq);

    func_exit();
} /* udi_log_event() */


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#define UF_DEVICE_CREATE(_class, _parent, _devno, _priv, _fmt, _args)       \
    device_create(_class, _parent, _devno, _priv, _fmt, _args)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#define UF_DEVICE_CREATE(_class, _parent, _devno, _priv, _fmt, _args)       \
    device_create_drvdata(_class, _parent, _devno, _priv, _fmt, _args)
#else
#define UF_DEVICE_CREATE(_class, _parent, _devno, _priv, _fmt, _args)       \
    device_create(_class, _parent, _devno, _fmt, _args)
#endif

/*
 ****************************************************************************
 *
 *      Driver instantiation
 *
 ****************************************************************************
 */
static struct file_operations unifi_fops = {
    .owner      = THIS_MODULE,
    .open       = unifi_open,
    .release    = unifi_release,
    .read       = unifi_read,
    .write      = unifi_write,
    //.ioctl      = unifi_ioctl,
    .unlocked_ioctl      = unifi_ioctl,
    .poll       = unifi_poll,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#define UF_DEVICE_CREATE(_class, _parent, _devno, _priv, _fmt, _args)       \
    device_create(_class, _parent, _devno, _priv, _fmt, _args)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#define UF_DEVICE_CREATE(_class, _parent, _devno, _priv, _fmt, _args)       \
    device_create_drvdata(_class, _parent, _devno, _priv, _fmt, _args)
#else
#define UF_DEVICE_CREATE(_class, _parent, _devno, _priv, _fmt, _args)       \
    device_create(_class, _parent, _devno, _fmt, _args)
#endif

static dev_t unifi_first_devno;
static struct class *unifi_class;


int uf_create_device_nodes(unifi_priv_t *priv, int bus_id)
{
    dev_t devno;
    int r;

    cdev_init(&priv->unifi_cdev, &unifi_fops);

    /* cdev_init() should set the cdev owner, but it does not */
    priv->unifi_cdev.owner = THIS_MODULE;

    devno = MKDEV(MAJOR(unifi_first_devno),
                  MINOR(unifi_first_devno) + (bus_id * MAX_UNIFI_DEVS));
    r = cdev_add(&priv->unifi_cdev, devno, 1);
    if (r) {
        return r;
    }

#ifdef SDIO_EXPORTS_STRUCT_DEVICE
    if (!UF_DEVICE_CREATE(unifi_class, priv->unifi_device,
                          devno, priv, "unifi%d", bus_id)) {
#else
    priv->unifi_device = UF_DEVICE_CREATE(unifi_class, NULL,
                                          devno, priv, "unifi%d", bus_id);
    if (priv->unifi_device == NULL) {
#endif /* SDIO_EXPORTS_STRUCT_DEVICE */

        cdev_del(&priv->unifi_cdev);
        return -EINVAL;
    }

    cdev_init(&priv->unifiudi_cdev, &unifi_fops);

    /* cdev_init() should set the cdev owner, but it does not */
    priv->unifiudi_cdev.owner = THIS_MODULE;

    devno = MKDEV(MAJOR(unifi_first_devno),
                  MINOR(unifi_first_devno) + (bus_id * MAX_UNIFI_DEVS) + 1);
    r = cdev_add(&priv->unifiudi_cdev, devno, 1);
    if (r) {
        device_destroy(unifi_class, priv->unifi_cdev.dev);
        cdev_del(&priv->unifi_cdev);
        return r;
    }

    if (!UF_DEVICE_CREATE(unifi_class,
#ifdef SDIO_EXPORTS_STRUCT_DEVICE
                          priv->unifi_device,
#else
                          NULL,
#endif /* SDIO_EXPORTS_STRUCT_DEVICE */
                          devno, priv, "unifiudi%d", bus_id)) {
        device_destroy(unifi_class, priv->unifi_cdev.dev);
        cdev_del(&priv->unifiudi_cdev);
        cdev_del(&priv->unifi_cdev);
        return -EINVAL;
    }

    return 0;
}


void uf_destroy_device_nodes(unifi_priv_t *priv)
{
    device_destroy(unifi_class, priv->unifiudi_cdev.dev);
    device_destroy(unifi_class, priv->unifi_cdev.dev);
    cdev_del(&priv->unifiudi_cdev);
    cdev_del(&priv->unifi_cdev);
}



/*
 * ----------------------------------------------------------------
 *  uf_create_debug_device
 *
 *      Allocates device numbers for unifi character device nodes
 *      and creates a unifi class in sysfs
 *
 * Arguments:
 *	fops          Pointer to the char device operations structure.
 *
 * Returns:
 *      0 on success, -ve error code on error.
 * ----------------------------------------------------------------
 */
static int
uf_create_debug_device(struct file_operations *fops)
{
    int ret;

    /* Allocate two device numbers for each device. */
    ret = alloc_chrdev_region(&unifi_first_devno, 0, MAX_UNIFI_DEVS*2, UNIFI_NAME);
    if (ret) {
        unifi_error(NULL, "Failed to add alloc dev numbers: %d\n", ret);
        return ret;
    }

    /* Create a UniFi class */
    unifi_class = class_create(THIS_MODULE, UNIFI_NAME);
    if (IS_ERR(unifi_class)) {
        unifi_error(NULL, "Failed to create UniFi class\n");

        /* Release device numbers */
        unregister_chrdev_region(unifi_first_devno, MAX_UNIFI_DEVS*2);
        unifi_first_devno = 0;
        return -EINVAL;
    }

    return 0;
} /* uf_create_debug_device() */


/*
 * ----------------------------------------------------------------
 *  uf_remove_debug_device
 *
 *      Destroys the unifi class and releases the allocated
 *      device numbers for unifi character device nodes.
 *
 * Arguments:
 *
 * Returns:
 * ----------------------------------------------------------------
 */
static void
uf_remove_debug_device(void)
{
    /* Destroy the UniFi class */
    class_destroy(unifi_class);

    /* Release device numbers */
    unregister_chrdev_region(unifi_first_devno, MAX_UNIFI_DEVS*2);
    unifi_first_devno = 0;

} /* uf_remove_debug_device() */


/*
 * ---------------------------------------------------------------------------
 *
 *      Module loading.
 *
 * ---------------------------------------------------------------------------
 */
#if defined CUSTOMER_ALLWINNER && defined CONFIG_SW_MMC_POWER_CONTROL
extern int mmc_pm_get_mod_type(void);
extern int mmc_pm_gpio_ctrl(char* name, int level);
extern int mmc_pm_get_io_val(char* name);
extern void sw_mmc_rescan_card(unsigned id, unsigned insert);
#endif
int __init
unifi_load(void)
{
    int r;
    
#if defined CUSTOMER_ALLWINNER && defined CONFIG_SW_MMC_POWER_CONTROL
    printk("unifi_load\n");
    mmc_pm_gpio_ctrl("apm_6981_vcc_en", 1);
    msleep(1);
    mmc_pm_gpio_ctrl("apm_6981_vdd_en", 1);
    msleep(1);
    mmc_pm_gpio_ctrl("apm_6981_pwd_n", 1);
    mmc_pm_gpio_ctrl("apm_6981_rst_n", 0);
    msleep(100);
    mmc_pm_gpio_ctrl("apm_6981_rst_n", 1);
    msleep(300);
    sw_mmc_rescan_card(3, 1);
    msleep(100);
#endif
    mdelay(100);
    printk("UniFi SDIO Driver: v%s (build:%d) %s %s\n",
           UNIFI_DRIVER_VERSION,
           UNIFI_DRIVER_BUILD_ID,
           __DATE__, __TIME__);

#ifdef CSR_SME_EMB
    printk("CSR Embedded SME\n");
#endif
#ifdef CSR_SME_USERSPACE
#ifdef CSR_SUPPORT_WEXT
    printk("CSR SME with WEXT support\n");
#else
    printk("CSR SME no WEXT support\n");
#endif /* CSR_SUPPORT_WEXT */
#endif /* CSR_SME_USERSPACE */
#ifdef CSR_NATIVE_LINUX
    printk("CSR native WEXT\n");
#endif

    /*
     * Instantiate the /dev/unifi* device nodes.
     * We must do this before registering with the SDIO driver because it
     * will immediately call the "insert" callback if the card is
     * already present.
     */
    r = uf_create_debug_device(&unifi_fops);
    if (r) {
        return r;
    }

    /* Now register with the SDIO driver */
    r = uf_sdio_load();
    if (r) {
        uf_remove_debug_device();
        return r;
    }

    if (sdio_block_size > -1) {
        unifi_info(NULL, "sdio_block_size %d\n", sdio_block_size);
    }

    if (sdio_byte_mode) {
        unifi_info(NULL, "sdio_byte_mode\n");
    }

    if (disable_power_control) {
        unifi_info(NULL, "disable_power_control\n");
    }

    if (disable_hw_reset) {
        unifi_info(NULL, "disable_hw_reset\n");
    }

    if (enable_wol) {
        unifi_info(NULL, "enable_wol %d\n", enable_wol);
    }

    if (run_bh_once != -1) {
        unifi_info(NULL, "run_bh_once %d\n", run_bh_once);
    }

    return 0;
} /* unifi_load() */


void __exit
unifi_unload(void)
{
    /* The SDIO remove hook will call unifi_disconnect(). */
    uf_sdio_unload();

    uf_remove_debug_device();
    
#if defined CUSTOMER_ALLWINNER && defined CONFIG_SW_MMC_POWER_CONTROL
    printk("unifi_sdio: power off\n");
    
    mmc_pm_gpio_ctrl("apm_6981_rst_n", 0);
    mmc_pm_gpio_ctrl("apm_6981_pwd_n", 0);
    sw_mmc_rescan_card(3, 0);
    msleep(100);
#endif
} /* unifi_unload() */

module_init(unifi_load);
module_exit(unifi_unload);

MODULE_DESCRIPTION("UniFi Device driver");
MODULE_AUTHOR("Cambridge Silicon Radio Ltd.");
MODULE_LICENSE("GPL and additional rights");
