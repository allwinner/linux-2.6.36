/*
 * ***************************************************************************
 *
 *  FILE:     sme_native.c
 * 
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */
#include "unifi_priv.h"
#include "driver/conversions.h"


int
uf_sme_init(unifi_priv_t *priv)
{
    int r;

    init_MUTEX(&priv->mlme_blocking_mutex);

    r = uf_init_wext_interface(priv);
    if (r != 0) {
        return r;
    }

    /* Need a separate client for the background multicast process */
    priv->multicast_client = ul_register_client(priv, 0, sme_native_mlme_event_handler);
    if (priv->multicast_client == NULL) {
        unifi_error(priv, "Failed to register WEXT as a unifi client\n");
        return -1;
    }

    priv->mc_list_count_stored = 0;

    return 0;
} /* uf_sme_init() */


void
uf_sme_deinit(unifi_priv_t *priv)
{
    /* Free memory allocated for the scan table */
    unifi_clear_scan_table(priv);

    /* Unregister WEXT as a client. */
    if (priv->multicast_client) {
        ul_deregister_client(priv->multicast_client);
        priv->multicast_client = NULL;
    }

    /* Cancel any pending workqueue tasks */
    flush_workqueue(priv->unifi_workqueue);

    uf_deinit_wext_interface(priv);

} /* uf_sme_deinit() */


int sme_mgt_wifi_on(unifi_priv_t *priv)
{
    int r;
    CsrInt32 csr_r;

    if (priv == NULL) {
        return -EINVAL;
    }

    r = uf_request_firmware_files(priv, UNIFI_FW_STA);
    if (r) {
        unifi_error(priv, "sme_mgt_wifi_on: Failed to get f/w\n");
        return r;
    }

    /*
     * The request to initialise UniFi might come while UniFi is running.
     * We need to block all I/O activity until the reset completes, otherwise
     * an SDIO error might occur resulting an indication to the SME which 
     * makes it think that the initialisation has failed.
     */
    priv->bh_thread.block_thread = 1;

    /* Power on UniFi */
    csr_r = CsrSdioPowerOn(priv->sdio);
    if (csr_r < 0) {
        return convert_csr_error(csr_r);
    }

    if (csr_r == CSR_SDIO_RESULT_DEVICE_WAS_RESET) {
        /* Initialise UniFi hardware */
        r = uf_init_hw(priv);
        if (r) {
            return r;
        }
    }

    /* Re-enable the I/O thread */
    priv->bh_thread.block_thread = 0;

    /* Disable deep sleep signalling during the firmware initialisation, to
     * prevent the wakeup mechanism raising the SDIO clock beyond INIT before
     * the first MLME-RESET.ind. It gets re-enabled at the CONNECTED.ind,
     * immediately after the MLME-RESET.ind
     */
    csr_r = unifi_configure_low_power_mode(priv->card,
                                           UNIFI_LOW_POWER_DISABLED,
                                           UNIFI_PERIODIC_WAKE_HOST_DISABLED);
    if (csr_r) {
        unifi_warning(priv,
                      "sme_mgt_wifi_on: unifi_configure_low_power_mode() returned an error\n");
    }


    /* Start the I/O thread */
    r = uf_init_bh(priv);
    if (r) {
        CsrSdioPowerOff(priv->sdio);
        return r;
    }

    priv->init_progress = UNIFI_INIT_FW_DOWNLOADED;

    return 0;
}

int
sme_sys_suspend(unifi_priv_t *priv)
{
    CsrInt32 csr_r;

    /* Abort any pending requests. */
    uf_abort_mlme(priv);

    /* Allow our mlme request to go through. */
    priv->io_aborted = 0;

    /* Send MLME-RESET.req to UniFi. */
    unifi_reset_state(priv, priv->netdev->dev_addr, 0);

    /* Stop the network traffic */
    netif_carrier_off(priv->netdev);

    /* Put UniFi to deep sleep */
    csr_r = unifi_force_low_power_mode(priv->card);

    return 0;
} /* sme_sys_suspend() */


int
sme_sys_resume(unifi_priv_t *priv)
{
    /* Send disconnect event so clients will re-initialise connection. */
    memset(priv->wext_conf.current_ssid, 0, UNIFI_MAX_SSID_LEN);
    memset((void*)priv->wext_conf.current_bssid, 0, ETH_ALEN);
    priv->wext_conf.capability = 0;
    wext_send_disassoc_event(priv);

    return 0;
} /* sme_sys_resume() */


#define ROWSTATUS_ACTIVE 1
#define ROWSTATUS_NOT_IN_SERVICE 2

#define dot11Address_oid              "1.2.840.10036.2.3.1.2"
#define dot11GroupAddressesStatus_oid "1.2.840.10036.2.3.1.3"

void
uf_multicast_list_wq(struct work_struct *work)
{
    unifi_priv_t *priv = container_of(work, unifi_priv_t,
                                      multicast_list_task);

    char oidstr[128];
    u8 *mc_list = priv->mc_list;
    int mc_list_count_to_store, r, i;

    /* priv->mc_list_count is not persistent. use a local variable. */
    mc_list_count_to_store = priv->mc_list_count;
    /* Get pointer to the addresses list. */
    mc_list = priv->mc_list;
    /* Set list of valid addresses, if any. */
    for (i = 1; i <= mc_list_count_to_store; i++)
    {
        snprintf(oidstr, 128, "%s.%d.%d", dot11Address_oid, priv->if_index, i);
        r = unifi_set_mib_string(priv, priv->multicast_client, oidstr, mc_list, ETH_ALEN);
        if (r < 0) {
            continue;
        }

        snprintf(oidstr, 128, "%s.%d.%d", dot11GroupAddressesStatus_oid, priv->if_index, i);
        r = unifi_set_mib_int(priv, priv->multicast_client,
                              oidstr, ROWSTATUS_ACTIVE);
        if (r < 0) {
            continue;
        }

        /* Advance pointer to next address. */
        mc_list += ETH_ALEN;
    }

    /* Invalidate the rest of the address already stored in the UniFi's MIB. */
    for (i = mc_list_count_to_store + 1; i <= priv->mc_list_count_stored; i++)
    {
        snprintf(oidstr, 128, "%s.%d.%d", dot11GroupAddressesStatus_oid, priv->if_index, i);
        r = unifi_set_mib_int(priv, priv->multicast_client,
                              oidstr, ROWSTATUS_NOT_IN_SERVICE);
        if (r < 0) {
            continue;
        }
    }

    /* Update our state. */
    priv->mc_list_count_stored = priv->mc_list_count;

}


/*
 * ---------------------------------------------------------------------------
 *  sme_native_log_event
 *
 *      Callback function to be registered as the SME event callback.
 *      Copies the signal content into a new udi_log_t struct and adds
 *      it to the read queue for the SME client.
 * 
 *  Arguments:
 *      arg             This is the value given to unifi_add_udi_hook, in
 *                      this case a pointer to the client instance.
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
sme_native_log_event(ul_client_t *pcli,
                     const u8 *sig_packed, int sig_len,
                     const bulk_data_param_t *bulkdata,
                     int dir)
{
    unifi_priv_t *priv;
    udi_log_t *logptr;
    u8 *p;
    int i, r;
    int signal_len;
    int total_len;
    udi_msg_t *msgptr;
    CSR_SIGNAL signal;
    ul_client_t *client = pcli;

    func_enter();

    if (client == NULL) {
        unifi_error(NULL, "sme_native_log_event: client has exited\n");
        return;
    }

    priv = uf_find_instance(client->instance);
    if (!priv) {
        unifi_error(priv, "invalid priv\n");
        return;
    }

    /* Just a sanity check */
    if ((sig_packed == NULL) || (sig_len <= 0)) {
        return;
    }

    /* Get the unpacked signal */
    r = read_unpack_signal(sig_packed, &signal);
    if (r == 0) {
        signal_len = SigGetSize(&signal);
    } else {
        /* The control indications are 1 byte, pass them to client */
        if (sig_len == 1) {
            unifi_trace(priv, UDBG5,
                        "Control indication (0x%x) for native SME.\n",
                        *sig_packed);

            *(u8*)&signal = *sig_packed;
            signal_len = sig_len;
        } else {
            unifi_error(priv, "Received unknown or corrupted signal.\n");
            return;
        }
    }

    unifi_trace(priv, UDBG3, "sme_native_log_event: signal %s for %d\n",
                lookup_signal_name(signal.SignalPrimitiveHeader.SignalId),
                client->client_id);

    /* Discard some ma-unidata.ind */
    if (signal.SignalPrimitiveHeader.SignalId == CSR_MA_UNITDATA_INDICATION_ID) {
        return;
    }
    /* Discard ma-unidata-status.ind */
    if (signal.SignalPrimitiveHeader.SignalId == CSR_MA_UNITDATA_CONFIRM_ID) {
        return;
    }

    total_len = signal_len;
    /* Calculate the buffer we need to store signal plus bulk data */
    for (i = 0; i < UNIFI_MAX_DATA_REFERENCES; i++) {
        total_len += bulkdata->d[i].data_length;
    }

    /* Allocate log structure plus actual signal. */
    logptr = (udi_log_t *)kmalloc(sizeof(udi_log_t) + total_len, GFP_KERNEL);

    if (logptr == NULL) {
        unifi_error(priv,
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
    memcpy(p, &signal, signal_len);
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
    down(&client->udi_sem);
    list_add_tail(&logptr->q, &client->udi_log);
    up(&client->udi_sem);

    /* Wake any waiting user process */
    wake_up_interruptible(&client->udi_wq);

    func_exit();

} /* sme_native_log_event() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_ta_indicate_protocol
 *
 *      Report that a packet of a particular type has been seen
 * 
 *  Arguments:
 *      drv_priv        The device context pointer passed to ta_init.
 *      protocol        The protocol type enum value.
 *      direction       Whether the packet was a tx or rx.
 *      src_addr        The source MAC address from the data packet.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      We defer the actual sending to a background workqueue,
 *      see uf_ta_ind_wq().
 * ---------------------------------------------------------------------------
 */
void
unifi_ta_indicate_protocol(void *ospriv, 
                           unifi_TrafficPacketType packet_type,
                           unifi_ProtocolDirection direction,
                           const unifi_MACAddress *src_addr)
{

} /* unifi_ta_indicate_protocol */

/*
 * ---------------------------------------------------------------------------
 * unifi_ta_indicate_sampling
 *
 *      Send the TA sampling information to the SME.
 * 
 *  Arguments:
 *      drv_priv        The device context pointer passed to ta_init.
 *      stats           The TA sampling data to send.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
unifi_ta_indicate_sampling(void *ospriv, unifi_TrafficStats *stats)
{

} /* unifi_ta_indicate_sampling() */


/*
 * ---------------------------------------------------------------------------
 * uf_native_process_udi_signal
 *
 *      Process interesting signals from the UDI interface.
 * 
 *  Arguments:
 *      pcli            A pointer to the client instance.
 *      signal          Pointer to the received signal.
 *      signal_len      Size of the signal structure in bytes.
 *      bulkdata        Pointers to any associated bulk data.
 *      dir             Direction of the signal. Zero means from host,
 *                      non-zero means to host.
 *
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
uf_native_process_udi_signal(ul_client_t *pcli,
                             const u8 *packed_signal, int packed_signal_len,
                             const bulk_data_param_t *bulkdata, int dir)
{
    unifi_priv_t *priv;
    CSR_SIGNAL signal;
    int r;
    unsigned int signal_id;
    unsigned int ie_cfm_len;

    priv = uf_find_instance(pcli->instance);
    if (!priv) {
        unifi_error(priv, "uf_native_process_udi_signal: invalid priv\n");
        return;
    }

    signal_id = (unsigned int) GET_SIGNAL_ID(packed_signal);
    if ((signal_id == CSR_MLME_ASSOCIATE_CONFIRM_ID) ||
        (signal_id == CSR_MLME_REASSOCIATE_CONFIRM_ID)) {

        r = read_unpack_signal(packed_signal, &signal);
        if (r != 0) {
            unifi_error(priv,
                        "uf_native_process_udi_signal: unknown signal (0x%04x)\n",
                        signal_id);
        }

        ie_cfm_len = signal.u.MlmeAssociateConfirm.InformationElements.DataLength;
        if ((ie_cfm_len > 0) && (ie_cfm_len < IE_VECTOR_MAXLEN)) {
            /* Need to get information about QOS from the assoc response */
            r = unifi_get_wmm_bss_capabilities(priv,
                                               (unsigned char *)bulkdata->d[0].os_data_ptr,
                                               ie_cfm_len,
                                               &priv->wext_conf.bss_wmm_capabilities);
            priv->sta_wmm_capabilities = (r == 1) ? priv->wext_conf.bss_wmm_capabilities : 0;
            unifi_trace(priv, UDBG1, "WMM mode %s\n",
                        (priv->sta_wmm_capabilities & QOS_CAPABILITY_WMM_ENABLED) ?
                        "enabled" : "disabled");
        }
    }

} /* uf_native_process_udi_signal() */
