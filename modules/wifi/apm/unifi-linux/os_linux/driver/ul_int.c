/*
 * ***************************************************************************
 *  FILE:     ul_int.c
 *
 *  PURPOSE:
 *      Manage list of client applications using UniFi.
 *
 * Copyright (C) 2006-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */
#include "driver/unifi.h"
#include "driver/conversions.h"
#include "unifi_priv.h"
#include "unifiio.h"

static void free_bulkdata_buffers(unifi_priv_t *priv, bulk_data_param_t *bulkdata);
static void reset_driver_status(unifi_priv_t *priv);

/*
 * ---------------------------------------------------------------------------
 *  ul_init_clients
 *
 *      Initialise the clients array to empty.
 *
 *  Arguments:
 *      priv            Pointer to device private context struct
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      This function needs to be called before priv is stored in
 *      Unifi_instances[].
 * ---------------------------------------------------------------------------
 */
void
ul_init_clients(unifi_priv_t *priv)
{
    int id;
    ul_client_t *ul_clients;

    init_MUTEX(&priv->udi_logging_mutex);
    priv->logging_client = NULL;

    ul_clients = priv->ul_clients;

    for (id = 0; id < MAX_UDI_CLIENTS; id++) {
        memset(&ul_clients[id], 0, sizeof(ul_client_t));

        ul_clients[id].client_id = id;
        ul_clients[id].sender_id = UDI_SENDER_ID_BASE + (id << UDI_SENDER_ID_SHIFT);
        ul_clients[id].instance = -1;
        ul_clients[id].event_hook = NULL;

        INIT_LIST_HEAD(&ul_clients[id].udi_log);
        init_waitqueue_head(&ul_clients[id].udi_wq);
        sema_init(&ul_clients[id].udi_sem, 1);

        ul_clients[id].wake_up_wq_id = 0;
        ul_clients[id].seq_no = 0;
        ul_clients[id].wake_seq_no = 0;
        ul_clients[id].snap_filter.count = 0;
    }
} /* ul_init_clients() */


/*
 * ---------------------------------------------------------------------------
 *  ul_register_client
 *
 *      This function registers a new ul client.
 *
 *  Arguments:
 *      priv            Pointer to device private context struct
 *      configuration   Special configuration for the client.
 *      udi_event_clbk  Callback for receiving event from unifi.
 *
 *  Returns:
 *      0 if a new clients is registered, -1 otherwise.
 * ---------------------------------------------------------------------------
 */
ul_client_t *
ul_register_client(unifi_priv_t *priv, unsigned int configuration,
                   udi_event_t udi_event_clbk)
{
    unsigned char id, ref;
    ul_client_t *ul_clients;

    ul_clients = priv->ul_clients;

    /* check for an unused entry */
    for (id = 0; id < MAX_UDI_CLIENTS; id++) {
        if (ul_clients[id].udi_enabled == 0) {
            ul_clients[id].instance = priv->instance;
            ul_clients[id].udi_enabled = 1;
            ul_clients[id].configuration = configuration;

            /* Allocate memory for the reply signal.. */
            ul_clients[id].reply_signal = (CSR_SIGNAL*) CsrPmalloc(sizeof(CSR_SIGNAL));
            if (ul_clients[id].reply_signal == NULL) {
                unifi_error(priv, "Failed to allocate reply signal for client.\n");
                return NULL;
            }
            /* .. and the bulk data of the reply signal. */
            for (ref = 0; ref < UNIFI_MAX_DATA_REFERENCES; ref ++) {
                ul_clients[id].reply_bulkdata[ref] =
                        (bulk_data_t*) CsrPmalloc(sizeof(bulk_data_t));
                /* If allocation fails, free allocated memory. */
                if (ul_clients[id].reply_bulkdata[ref] == NULL) {
                    for (; ref > 0; ref --) {
                        CsrPfree(ul_clients[id].reply_bulkdata[ref - 1]);
                    }
                    CsrPfree(ul_clients[id].reply_signal);
                    unifi_error(priv, "Failed to allocate bulk data buffers for client.\n");
                    return NULL;
                }
            }

            /* Set the event callback. */
            ul_clients[id].event_hook = udi_event_clbk;

            unifi_trace(priv, UDBG2, "UDI %d (0x%x) registered. configuration = 0x%x\n",
                        id, &ul_clients[id], configuration);
            return &ul_clients[id];
        }
    }
    return NULL;
} /* ul_register_client() */


/*
 * ---------------------------------------------------------------------------
 *  ul_deregister_client
 *
 *      This function deregisters a blocking UDI client.
 *
 *  Arguments:
 *      client      Pointer to the client we deregister.
 *
 *  Returns:
 *      0 if a new clients is deregistered.
 * ---------------------------------------------------------------------------
 */
int
ul_deregister_client(ul_client_t *ul_client)
{
    struct list_head *pos, *n;
    udi_log_t *logptr;
    unifi_priv_t *priv = uf_find_instance(ul_client->instance);
    int ref;

    ul_client->instance = -1;
    ul_client->event_hook = NULL;
    ul_client->udi_enabled = 0;
    unifi_trace(priv, UDBG5, "UDI (0x%x) deregistered.\n", ul_client);

    /* Free memory allocated for the reply signal and its bulk data. */
    CsrPfree(ul_client->reply_signal);
    for (ref = 0; ref < UNIFI_MAX_DATA_REFERENCES; ref ++) {
        CsrPfree(ul_client->reply_bulkdata[ref]);
    }

    if (ul_client->snap_filter.count) {
        ul_client->snap_filter.count = 0;
        CsrPfree(ul_client->snap_filter.protocols);
    }

    /* Free anything pending on the udi_log list */
    down(&ul_client->udi_sem);
    list_for_each_safe(pos, n, &ul_client->udi_log)
    {
        logptr = list_entry(pos, udi_log_t, q);
        list_del(pos);
        kfree(logptr);
    }
    up(&ul_client->udi_sem);

    return 0;
} /* ul_deregister_client() */



/*
 * ---------------------------------------------------------------------------
 *  logging_handler
 *
 *      This function is registered with the driver core.
 *      It is called every time a UniFi HIP Signal is sent. It iterates over
 *      the list of processes interested in receiving log events and
 *      delivers the events to them.
 *
 *  Arguments:
 *      ospriv      Pointer to driver's private data.
 *      sigdata     Pointer to the packed signal buffer.
 *      signal_len  Length of the packed signal.
 *      bulkdata    Pointer to the signal's bulk data.
 *      dir         Direction of the signal
 *                  0 = from-host
 *                  1 = to-host
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
logging_handler(void *ospriv,
                CsrUint8 *sigdata, CsrUint32 signal_len,
                const bulk_data_param_t *bulkdata,
                enum udi_log_direction direction)
{
    unifi_priv_t *priv = (unifi_priv_t*)ospriv;
    ul_client_t *client;
    int dir;

    dir = (direction == UDI_LOG_FROM_HOST) ? UDI_FROM_HOST : UDI_TO_HOST;

    down(&priv->udi_logging_mutex);
    client = priv->logging_client;
    if (client != NULL) {
        client->event_hook(client, sigdata, signal_len,
                           bulkdata, dir);
    }
    up(&priv->udi_logging_mutex);

} /* logging_handler() */



/*
 * ---------------------------------------------------------------------------
 *  ul_log_config_ind
 *
 *      This function uses the client's register callback
 *      to indicate configuration information e.g core errors.
 *
 *  Arguments:
 *      priv        Pointer to driver's private data.
 *      conf_param  Pointer to the configuration data.
 *      len         Length of the configuration data.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
ul_log_config_ind(unifi_priv_t *priv, u8 *conf_param, int len)
{
#ifdef CSR_SUPPORT_SME
    if (priv->smepriv == NULL)
    {
        return;
    }
    if ((CONFIG_IND_ERROR == (*conf_param)) && (priv->wifi_on_state == wifi_on_in_progress)) {
        unifi_notice(priv, "ul_log_config_ind: wifi on in progress, suppress error\n");
    } else {
        /* wifi_off_ind (error or exit) */
        unifi_sys_wifi_off_ind(priv->smepriv, (unifi_ControlIndication)(*conf_param));
    }

#else
    bulk_data_param_t bulkdata;

    /*
     * If someone killed unifi_managed before the driver was unloaded
     * the g_drvpriv pointer is going to be NULL. In this case it is
     * safe to assume that there is no client to get the indication.
     */
    if (!priv) {
        unifi_notice(NULL, "uf_sme_event_ind: NULL priv\n");
        return;
    }

    /* Create a null bulkdata structure. */
    bulkdata.d[0].data_length = 0;
    bulkdata.d[1].data_length = 0;

    sme_native_log_event(priv->sme_cli, conf_param, sizeof(CsrUint8),
                         &bulkdata, UDI_CONFIG_IND);

#endif /* CSR_SUPPORT_SME */

} /* ul_log_config_ind */


/*
 * ---------------------------------------------------------------------------
 *  free_bulkdata_buffers
 *
 *      Free the bulkdata buffers e.g. after a failed unifi_send_signal().
 * 
 *  Arguments:
 *      priv        Pointer to device private struct
 *      bulkdata    Pointer to bulkdata parameter table
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
free_bulkdata_buffers(unifi_priv_t *priv, bulk_data_param_t *bulkdata)
{
    int i;

    if (bulkdata) {
        for (i = 0; i < UNIFI_MAX_DATA_REFERENCES; ++i) {
            if (bulkdata->d[i].data_length != 0) {
                unifi_net_data_free(priv, (bulk_data_desc_t *)(&bulkdata->d[i]));
                /* data_length is now 0 */
            }
        }
    }
    
} /* free_bulkdata_buffers */

/*
 * ---------------------------------------------------------------------------
 *  ul_send_signal_unpacked
 *
 *      This function sends a host formatted signal to unifi.
 *
 *  Arguments:
 *      priv        Pointer to driver's private data.
 *      sigptr      Pointer to the signal.
 *      bulkdata    Pointer to the signal's bulk data.
 *
 *  Returns:
 *      O on success, error code otherwise.
 *
 *  Notes:
 *  The signals have to be sent in the format described in the host interface
 *  specification, i.e wire formatted. Certain clients use the host formatted
 *  structures. The write_pack() transforms the host formatted signal
 *  into the wired formatted signal. The code is in the core, since the signals
 *  are defined therefore binded to the host interface specification.
 * ---------------------------------------------------------------------------
 */
int
ul_send_signal_unpacked(unifi_priv_t *priv, const CSR_SIGNAL *sigptr,
                        const bulk_data_param_t *bulkdata)
{
    CsrUint8 sigbuf[UNIFI_PACKED_SIGBUF_SIZE];
    CsrUint16 packed_siglen;
    CsrInt32 csr_r;
    unsigned long lock_flags;

    csr_r = write_pack(sigptr, sigbuf, &packed_siglen);
    if (csr_r) {
        unifi_error(priv, "Malformed HIP signal in ul_send_signal_unpacked()\n");
        return convert_csr_error(csr_r);
    }

    spin_lock_irqsave(&priv->send_signal_lock, lock_flags);
    csr_r = unifi_send_signal(priv->card, sigbuf, packed_siglen, bulkdata);
    if (csr_r) {
        free_bulkdata_buffers(priv, (bulk_data_param_t *)bulkdata);
    }
    spin_unlock_irqrestore(&priv->send_signal_lock, lock_flags);

    return convert_csr_error(csr_r);
} /* ul_send_signal_unpacked() */


/*
 * ---------------------------------------------------------------------------
 *  reset_driver_status
 *
 *      This function is called from ul_send_signal_raw() when it detects
 *      that the SME has sent a MLME-RESET request.
 * 
 *  Arguments:
 *      priv        Pointer to device private struct
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
reset_driver_status(unifi_priv_t *priv)
{
    priv->sta_wmm_capabilities = 0;
#ifdef CSR_NATIVE_LINUX
    priv->mc_list_count_stored = 0;
    priv->wext_conf.flag_associated = 0;
    priv->wext_conf.block_controlled_port = unifi_8021xPortOpen;
    priv->wext_conf.bss_wmm_capabilities = 0;
    priv->wext_conf.disable_join_on_ssid_set = 0;
#endif
} /* reset_driver_status() */


/*
 * ---------------------------------------------------------------------------
 *  ul_send_signal_raw
 *
 *      This function sends a wire formatted data signal to unifi.
 *
 *  Arguments:
 *      priv        Pointer to driver's private data.
 *      sigptr      Pointer to the signal.
 *      siglen      Length of the signal.
 *      bulkdata    Pointer to the signal's bulk data.
 *
 *  Returns:
 *      O on success, error code otherwise.
 * ---------------------------------------------------------------------------
 */
int
ul_send_signal_raw(unifi_priv_t *priv, const unsigned char *sigptr, int siglen,
                   const bulk_data_param_t *bulkdata)
{
    CsrInt32 csr_r;
    unsigned long lock_flags;

    spin_lock_irqsave(&priv->send_signal_lock, lock_flags);
    csr_r = unifi_send_signal(priv->card, sigptr, siglen, bulkdata);
    if (csr_r) {
        free_bulkdata_buffers(priv, (bulk_data_param_t *)bulkdata);
    }
    spin_unlock_irqrestore(&priv->send_signal_lock, lock_flags);

    if (csr_r) {
        return convert_csr_error(csr_r);
    }

    /*
     * Since this is use by unicli, if we get an MLME reset request
     * we need to initialize a few status parameters
     * that the driver uses to make decisions.
     */
    if (GET_SIGNAL_ID(sigptr) == CSR_MLME_RESET_REQUEST_ID) {
        /* TODO: do not use this */
        reset_driver_status(priv);
    }

    return 0;
} /* ul_send_signal_raw() */


