/*
 * ---------------------------------------------------------------------------
 * FILE:     bh.c
 *
 * PURPOSE:
 *      Provides an implementation for the driver bottom-half.
 *      It is part of the porting exercise in Linux.
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include "driver/unifi.h"
#include "unifi_priv.h"


/*
 * ---------------------------------------------------------------------------
 *  uf_start_thread
 *
 *      Helper function to start a new thread.
 *
 *  Arguments:
 *      priv            Pointer to OS driver structure for the device.
 *      thread          Pointer to the thread object
 *      func            The thread function
 *
 *  Returns:
 *      0 on success or else a Linux error code.
 * ---------------------------------------------------------------------------
 */
int
uf_start_thread(unifi_priv_t *priv, struct uf_thread *thread, int (*func)(void *))
{
    if (thread->thread_task != NULL) {
        unifi_error(priv, "%s thread already started\n", thread->name);
        return 0;
    }

    /* New thread: system startup, so can't be requesting coredump from any
     * previous UniFi failure. Ensure no such requests are pending.
     */
    unifi_coredump_request_at_next_reset(priv->card, 0);

    /* Start the kernel thread that handles all h/w accesses. */    
    thread->thread_task = kthread_run(func, priv, "%s", thread->name);
    if (IS_ERR(thread->thread_task)) {
        return PTR_ERR(thread->thread_task);
    }

    unifi_trace(priv, UDBG2, "Started %s thread\n", thread->name);

    return 0;
} /* uf_start_thread() */


/*
 * ---------------------------------------------------------------------------
 *  uf_stop_thread
 *
 *      Helper function to stop a thread.
 *
 *  Arguments:
 *      priv            Pointer to OS driver structure for the device.
 *      thread          Pointer to the thread object
 *
 *  Returns:
 *
 * ---------------------------------------------------------------------------
 */
void
uf_stop_thread(unifi_priv_t *priv, struct uf_thread *thread)
{
    if (!thread->thread_task) {
        unifi_notice(priv, "%s thread is already stopped\n", thread->name);
        return;
    }

    unifi_trace(priv, UDBG2, "Stopping %s thread\n", thread->name);

    kthread_stop(thread->thread_task);
    thread->thread_task = NULL;

} /* uf_stop_thread() */



/*
 * ---------------------------------------------------------------------------
 *  uf_wait_for_thread_to_stop
 *
 *      Helper function to wait until a thread is stopped.
 *
 *  Arguments:
 *      priv            Pointer to OS driver structure for the device.
 *
 *  Returns:
 *
 * ---------------------------------------------------------------------------
 */
void
uf_wait_for_thread_to_stop(unifi_priv_t *priv, struct uf_thread *thread)
{
    /*
     * kthread_stop() cannot handle the thread exiting while
     * kthread_should_stop() is false, so sleep until kthread_stop()
     * wakes us up.
     */
    unifi_trace(priv, UDBG2, "%s waiting for the stop signal.\n", thread->name);
    set_current_state(TASK_INTERRUPTIBLE);
    if (!kthread_should_stop()) {
        unifi_trace(priv, UDBG2, "%s schedule....\n", thread->name);
        schedule();
    }

    thread->thread_task = NULL;
    unifi_trace(priv, UDBG2, "%s exiting....\n", thread->name);
} /* uf_wait_for_thread_to_stop() */


/*
 * ---------------------------------------------------------------------------
 *  handle_bh_error
 *
 *      This function reports an error returned from the HIP core bottom-half.
 *      Normally, implemented during the porting exercise, passing the error
 *      to the SME using unifi_sys_wifi_off_ind().
 *      The SME will try to reset the device and go through
 *      the initialisation of the UniFi.
 *
 *  Arguments:
 *      priv            Pointer to OS driver structure for the device.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
handle_bh_error(unifi_priv_t *priv)
{
    u8 conf_param = CONFIG_IND_ERROR;


    /* Block unifi_run_bh() until the error has been handled. */
    priv->bh_thread.block_thread = 1;

    /* As the driver is resetting due to detecting a previous SDIO failure,
     * request a mini-coredump from the chip when the reset completes,
     * in case the SDIO failure was due to a chip watchdog-reset or f/w crash.
     */
    unifi_coredump_request_at_next_reset(priv->card, 1);
    
    /* Consider UniFi to be uninitialised */
    priv->init_progress = UNIFI_INIT_NONE;

    /* Stop the network traffic */
    if (priv->netdev_registered == 1) {
        netif_carrier_off(priv->netdev);
    }

#ifdef CSR_NATIVE_LINUX
    /* Force any client waiting on an mlme_wait_for_reply() to abort. */
    uf_abort_mlme(priv);

    /* Cancel any pending workqueue tasks */
    flush_workqueue(priv->unifi_workqueue);

#endif /* CSR_NATIVE_LINUX */

    unifi_error(priv, "handle_bh_error: fatal error is reported to the SME.\n");
    /* Notify the clients (SME or unifi_manager) for the error. */
    ul_log_config_ind(priv, &conf_param, sizeof(u8));

} /* handle_bh_error() */



/*
 * ---------------------------------------------------------------------------
 *  bh_thread_function
 *
 *      All hardware access happens in this thread.
 *      This means there is no need for locks on the hardware and we don't need
 *      to worry about reentrancy with the SDIO library.
 *      Provides and example implementation on how to call unifi_bh(), which
 *      is part of the HIP core API.
 *
 *      It processes the events generated by unifi_run_bh() to serialise calls
 *      to unifi_bh(). It also demonstrates how the timeout parameter passed in
 *      and returned from unifi_bh() needs to be handled.
 *
 *  Arguments:
 *      arg             Pointer to OS driver structure for the device.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      When the bottom half of the driver needs to process signals, events,
 *      or simply the host status (i.e sleep mode), it invokes unifi_run_bh().
 *      Since we need all SDIO transaction to be in a single thread, the
 *      unifi_run_bh() will wake up this thread to process it.
 *
 * ---------------------------------------------------------------------------
 */
static int
bh_thread_function(void *arg)
{
    unifi_priv_t *priv = (unifi_priv_t*)arg;
    CsrInt32 csr_r;
    long ret;
    long timeout, t;
    struct uf_thread *this_thread;

    unifi_trace(priv, UDBG2, "bh_thread_function starting\n");

    this_thread = &priv->bh_thread;

    t = timeout = 0;
    while (!kthread_should_stop()) {
        /* wait until an error occurs, or we need to process something. */
        unifi_trace(priv, UDBG3, "bh_thread goes to sleep.\n");

        if (timeout > 0) {
            /* Convert t in ms to jiffies */
            t = msecs_to_jiffies(timeout);
            ret = wait_event_interruptible_timeout(this_thread->wakeup_q,
                                                   (this_thread->wakeup_flag && !this_thread->block_thread) ||
                                                   kthread_should_stop(),
                                                   t);
            timeout = (ret > 0) ? jiffies_to_msecs(ret) : 0;
        } else {
            ret = wait_event_interruptible(this_thread->wakeup_q,
                                           (this_thread->wakeup_flag && !this_thread->block_thread) ||
                                           kthread_should_stop());
        }

        if (kthread_should_stop()) {
            unifi_trace(priv, UDBG2, "bh_thread: signalled to exit\n");
            break;
        }

        if (ret < 0) {
            unifi_notice(priv,
                        "bh_thread: wait_event returned %d, thread will exit\n",
                        ret);
            uf_wait_for_thread_to_stop(priv, this_thread);
            break;
        }

        this_thread->wakeup_flag = 0;

        unifi_trace(priv, UDBG3, "bh_thread calls unifi_bh().\n");

        csr_r = unifi_bh(priv->card, &timeout);
        if (csr_r == -CSR_ENODEV) {
            uf_wait_for_thread_to_stop(priv, this_thread);
            break;
        }
        if (csr_r < 0) {
            /* Errors must be delivered to the error task */
            handle_bh_error(priv);
        }
    }

    /*
     * I would normally try to call csr_sdio_remove_irq() here to make sure
     * that we do not get any interrupts while this thread is not running.
     * However, the MMC/SDIO driver tries to kill its' interrupt thread.
     * The kernel threads implementation does not allow to kill threads
     * from a signalled to stop thread.
     * So, instead call csr_sdio_linux_remove_irq() always after calling
     * uf_stop_thread() to kill this thread.
     */

    unifi_trace(priv, UDBG2, "bh_thread exiting....\n");
    return 0;
} /* bh_thread_function() */


/*
 * ---------------------------------------------------------------------------
 *  uf_init_bh
 *
 *      Helper function to start the bottom half of the driver.
 *      All we need to do here is start the I/O bh thread.
 *
 *  Arguments:
 *      priv            Pointer to OS driver structure for the device.
 *
 *  Returns:
 *      0 on success or else a Linux error code.
 * ---------------------------------------------------------------------------
 */
int
uf_init_bh(unifi_priv_t *priv)
{
    int r;

    /* Enable mlme interface. */
    priv->io_aborted = 0;


    /* Start the BH thread */
    r = uf_start_thread(priv, &priv->bh_thread, bh_thread_function);
    if (r) {
        unifi_error(priv,
                    "uf_init_bh: failed to start the BH thread.\n");
        return r;
    }

    /* Allow interrupts */
    r = csr_sdio_linux_install_irq(priv->sdio);
    if (r) {
        unifi_error(priv,
                    "uf_init_bh: failed to install the IRQ.\n");

        uf_stop_thread(priv, &priv->bh_thread);
    }

    return r;
} /* uf_init_bh() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_run_bh
 *
 *      Part of the HIP core lib API, implemented in the porting exercise.
 *      The bottom half of the driver calls this function when
 *      it wants to process anything that requires access to unifi.
 *      We need to call unifi_bh() which in this implementation is done
 *      by waking up the I/O thread.
 *
 *  Arguments:
 *      ospriv          Pointer to OS driver structure for the device.
 *
 *  Returns:
 *      0 on success or else a Linux error code.
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
CsrInt32 unifi_run_bh(void *ospriv)
{
    unifi_priv_t *priv = ospriv;

    /*
     * If an error has occured, we discard silently all messages from the bh
     * until the error has been processed and the unifi has been reinitialised.
     */
    if (priv->bh_thread.block_thread == 1) {
        unifi_trace(priv, UDBG3, "unifi_run_bh: discard message.\n");
        /*
         * Do not try to acknowledge a pending interrupt here.
         * This function is called by unifi_send_signal() which in turn can be
         * running in an atomic or 'disabled irq' level if a signal is sent
         * from a workqueue task (i.e multicass addresses set).
         * We can not hold the SDIO lock because it might sleep.
         */
        return -CSR_EIO;
    }

    priv->bh_thread.wakeup_flag = 1;
    /* wake up I/O thread */
    wake_up_interruptible(&priv->bh_thread.wakeup_q);

    return 0;
} /* unifi_run_bh() */

