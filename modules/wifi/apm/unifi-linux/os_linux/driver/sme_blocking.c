/*
 * ---------------------------------------------------------------------------
 * FILE:     sme_mgt_blocking.c
 *
 * PURPOSE:
 *      This file contains the driver specific implementation of
 *      the WEXT <==> SME MGT interface for all SME builds that support WEXT.
 *
 * Copyright (C) 2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */

#include "unifi_priv.h"


/*
 * This file also contains the implementation of the asyncronous
 * requests to the SME.
 *
 * Before calling an asyncronous SME function, we call sme_init_request()
 * which gets hold of the SME semaphore and updates the request status.
 * The semaphore makes sure that there is only one pending request to
 * the SME at a time.
 *
 * Now we are ready to call the SME function, but only if
 * sme_init_request() has returned 0.
 *
 * When the SME function returns, we need to wait
 * for the reply. This is done in sme_wait_for_reply().
 * If the request times-out, the request status is set to SME_REQUEST_TIMEDOUT
 * and the sme_wait_for_reply() returns.
 *
 * If the SME replies in time, we call sme_complete_request().
 * There we change the request status to SME_REQUEST_RECEIVED. This will
 * wake up the process waiting on sme_wait_for_reply().
 * It is important that we copy the reply data in priv->sme_reply
 * before calling sme_complete_request().
 *
 * Handling the wext requests, we need to block
 * until the SME sends the response to our request.
 * We use the sme_init_request() and sme_wait_for_reply()
 * to implement this behavior in the following functions:
 * sme_mgt_wifi_on()
 * sme_mgt_wifi_off()
 * sme_mgt_scan_full()
 * sme_mgt_scan_results_get_async()
 * sme_mgt_connect()
 * unifi_mgt_media_status_ind()
 * sme_mgt_disconnect()
 * sme_mgt_pmkid()
 * sme_mgt_key()
 * sme_mgt_mib_get()
 * sme_mgt_mib_set()
 * sme_mgt_get_versions()
 * sme_mgt_set_value()
 * sme_mgt_get_value()
 * sme_mgt_set_value_async()
 * sme_mgt_get_value_async()
 * sme_mgt_packet_filter_set()
 * sme_mgt_tspec()
 */


/*
 * Handling the suspend and resume system events, we need to block
 * until the SME sends the response to our indication.
 * We use the sme_init_request() and sme_wait_for_reply()
 * to implement this behavior in the following functions:
 * sme_sys_suspend()
 * sme_sys_resume()
 */

#define UNIFI_SME_MGT_SHORT_TIMEOUT    10000
#define UNIFI_SME_MGT_LONG_TIMEOUT     19000
#define UNIFI_SME_SYS_LONG_TIMEOUT     10000

int
sme_init_request(unifi_priv_t *priv)
{
    if (priv == NULL) {
        unifi_error(priv, "sme_init_request: Invalid priv\n");
        return -EIO;
    }

    /* Grab the SME semaphore until the reply comes, or timeout */
    if (down_interruptible(&priv->sme_sem)) {
        unifi_error(priv, "sme_init_request: Failed to get SME semaphore\n");
        return -EIO;
    }
    priv->sme_reply.request_status = SME_REQUEST_PENDING;

    return 0;

} /* sme_init_request() */


void
sme_complete_request(unifi_priv_t *priv, unifi_Status reply_status)
{
    if (priv == NULL) {
        unifi_error(priv, "sme_complete_request: Invalid priv\n");
        return;
    }

    if (priv->sme_reply.request_status != SME_REQUEST_PENDING) {
        unifi_trace(priv, UDBG2,
                    "sme_complete_request: request not pending (s:%d)\n",
                    priv->sme_reply.request_status);
        return;
    }
    priv->sme_reply.request_status = SME_REQUEST_RECEIVED;
    priv->sme_reply.reply_status = reply_status;

    wake_up_interruptible(&priv->sme_request_wq);

    return;
}


int
sme_wait_for_reply(unifi_priv_t *priv,
                   unsigned long timeout)
{
    long r;

    unifi_trace(priv, UDBG5, "sme_wait_for_reply: sleep\n");
    r = wait_event_interruptible_timeout(priv->sme_request_wq,
                                            (priv->sme_reply.request_status != SME_REQUEST_PENDING),
                                            msecs_to_jiffies(timeout));
    unifi_trace(priv, UDBG5, "sme_wait_for_reply: awake\n");

    if (r == -ERESTARTSYS) {
        /* The thread was killed */
        up(&priv->sme_sem);
        return r;
    }
    if ((r == 0) && (priv->sme_reply.request_status != SME_REQUEST_RECEIVED)) {
        unifi_notice(priv, "Timeout waiting for SME to reply. (s:%d, t:%d)\n",
        priv->sme_reply.request_status, timeout);

#if (defined UNIFI_DEBUG) && (defined CSR_SME_EMB)
        /* Get the SME to dump a full state dump */
        if (priv->smepriv != NULL) {
            fsm_debug_dump(priv->smepriv);
        }
#endif /* CSR_SME_EMB */

        priv->sme_reply.request_status = SME_REQUEST_TIMEDOUT;

        /* Release the SME semaphore that was downed in sme_init_request() */
        up(&priv->sme_sem);

        return -ETIMEDOUT;
    }

    /* Release the SME semaphore that was downed in sme_init_request() */
    up(&priv->sme_sem);

    return 0;
} /* sme_wait_for_reply() */




#ifdef CSR_SUPPORT_WEXT
int sme_mgt_wifi_on(unifi_priv_t *priv)
{
    unifi_DataBlockList mib_data_list;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_wifi_on: invalid smepriv\n");
        return -EIO;
    }

    if (priv->mib_data.length) {
        mib_data_list.numElements = 1;
        mib_data_list.dataList = &priv->mib_data;
    } else {
        mib_data_list.numElements = 0;
        mib_data_list.dataList = NULL;
    }
    /* Start the SME */
    unifi_mgt_wifi_on_req(priv->smepriv, NULL,
                          &priv->sta_mac_address,
                          mib_data_list.numElements,
                          mib_data_list.dataList);
    return 0;
} /* sme_mgt_wifi_on() */


int sme_mgt_wifi_off(unifi_priv_t *priv)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_wifi_off: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    /* Stop the SME */
    unifi_mgt_wifi_off_req(priv->smepriv, NULL);

    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_LONG_TIMEOUT);
    if (r) {
        return r;
    }

    unifi_trace(priv, UDBG4,
                "sme_mgt_wifi_off: unifi_mgt_wifi_off_req <-- (r=%d, status=%d)\n",
                r, priv->sme_reply.reply_status);
    return convert_sme_error(priv->sme_reply.reply_status);

} /* sme_mgt_wifi_off */


int sme_mgt_set_value_async(unifi_priv_t *priv, unifi_AppValue *app_value)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_set_value_async: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_mgt_set_value_req(priv->smepriv, NULL, app_value);

    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_SHORT_TIMEOUT);
    if (r) {
        return r;
    }

    unifi_trace(priv, UDBG4,
                "sme_mgt_set_value_async: unifi_mgt_set_value_req <-- (r=%d status=%d)\n",
                r, priv->sme_reply.reply_status);
    return convert_sme_error(priv->sme_reply.reply_status);
} /* sme_mgt_set_value_async() */


int sme_mgt_get_value_async(unifi_priv_t *priv, unifi_AppValue *app_value)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_app_get_value: invalid smepriv\n");
        return -EIO;
    }

    unifi_trace(priv, UDBG4, "sme_app_get_value: unifi_mgt_get_value_req -->\n");
    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_mgt_get_value_req(priv->smepriv, NULL, app_value->id);

    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_SHORT_TIMEOUT);
    if (r) {
        return r;
    }

    /* store the reply */
    if (app_value != NULL) {
        memcpy((unsigned char*)app_value,
               (unsigned char*)&priv->sme_reply.reply_app_value,
               sizeof(unifi_AppValue));
    }

    unifi_trace(priv, UDBG4,
                "sme_app_get_value: unifi_mgt_get_value_req <-- (r=%d status=%d)\n",
                r, priv->sme_reply.reply_status);

    return convert_sme_error(priv->sme_reply.reply_status);
} /* sme_mgt_get_value_async() */


int sme_mgt_key(unifi_priv_t *priv, unifi_Key *sme_key,
                enum unifi_ListAction action)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_key: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_mgt_key_req(priv->smepriv, NULL, action, sme_key);

    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_SHORT_TIMEOUT);
    if (r) {
        return r;
    }

    return convert_sme_error(priv->sme_reply.reply_status);
}


int sme_mgt_scan_full(unifi_priv_t *priv,
                      unifi_SSID *specific_ssid,
                      int num_channels,
                      unsigned char *channel_list)
{
    unifi_MACAddress bcastAddress = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }};
    CsrBool is_active = (num_channels > 0) ? TRUE : FALSE;
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_scan_full: invalid smepriv\n");
        return -EIO;
    }

    unifi_trace(priv, UDBG4, "sme_mgt_scan_full: -->\n");

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    /* If a channel list is provided, do an active scan */
    if (is_active) {
        unifi_trace(priv, UDBG1,
                    "channel list - num_channels: %d, active scan\n",
                    num_channels);
    }

    unifi_mgt_scan_full_req(priv->smepriv, NULL,
                            specific_ssid->length?1:0, /* 0 or 1 SSIDS */
                            specific_ssid,
                            &bcastAddress,
                            is_active,
                            unifi_AnyBss,
                            unifi_ScanAll,
                            (CsrUint16)num_channels, channel_list,
                            0, NULL);

    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_LONG_TIMEOUT);
    if (r) {
        return r;
    }

    unifi_trace(priv, UDBG4, "sme_mgt_scan_full: <-- (status=%d)\n", priv->sme_reply.reply_status);
    if (priv->sme_reply.reply_status == unifi_Unavailable) {
        return 0; /* initial scan already underway */
    } else {
        return convert_sme_error(priv->sme_reply.reply_status);
    }
}


int sme_mgt_scan_results_get_async(unifi_priv_t *priv,
                                   struct iw_request_info *info,
                                   char *scan_results,
                                   long scan_results_len)
{
    CsrUint16 scan_result_list_count;
    unifi_ScanResult *scan_result_list;
    unifi_ScanResult *scan_result;
    int r;
    int i;
    char *current_ev = scan_results;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_scan_results_get_async: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_mgt_scan_results_get_req(priv->smepriv, NULL);
    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_LONG_TIMEOUT);
    if (r) {
        return r;
    }

    scan_result_list_count = priv->sme_reply.reply_scan_results_count;
    scan_result_list = priv->sme_reply.reply_scan_results;
    unifi_trace(priv, UDBG2,
                "scan_results: Scan returned %d, numElements=%d\n",
                r, scan_result_list_count);

    /* OK, now we have the scan results */
    for (i = 0; i < scan_result_list_count; ++i) {
        scan_result = &scan_result_list[i];

        unifi_trace(priv, UDBG2, "Scan Result: %.*s\n",
                    scan_result->ssid.length,
                    scan_result->ssid.ssid);

        r = unifi_translate_scan(priv->netdev, info,
                                 current_ev,
                                 scan_results + scan_results_len,
                                 scan_result, i+1);
        if (r < 0) {
            CsrPfree(scan_result_list);
            priv->sme_reply.reply_scan_results_count = 0;
            priv->sme_reply.reply_scan_results = NULL;
            return r;
        }

        current_ev += r;
    }

    /*
     * Free the scan results allocated in unifi_mgt_scan_results_get_cfm()
     * and invalidate the reply_scan_results to avoid re-using
     * the freed pointers.
     */
    CsrPfree(scan_result_list);
    priv->sme_reply.reply_scan_results_count = 0;
    priv->sme_reply.reply_scan_results = NULL;

    unifi_trace(priv, UDBG2,
                "scan_results: Scan translated to %d bytes\n",
                current_ev - scan_results);
    return (current_ev - scan_results);
}


int sme_mgt_connect(unifi_priv_t *priv)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_connect: invalid smepriv\n");
        return -EIO;
    }

    unifi_trace(priv, UDBG2, "sme_mgt_connect: %.*s\n",
            priv->connection_config.ssid.length,
            priv->connection_config.ssid.ssid);

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_mgt_connect_req(priv->smepriv, NULL, &priv->connection_config);
    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_SHORT_TIMEOUT);
    if (r) {
        return r;
    }

    if (priv->sme_reply.reply_status) {
        unifi_trace(priv, UDBG1, "sme_mgt_connect: failed with SME status %d\n",
                    priv->sme_reply.reply_status);
    }

    return convert_sme_error(priv->sme_reply.reply_status);
}


int sme_mgt_disconnect(unifi_priv_t *priv)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_disconnect: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_mgt_disconnect_req(priv->smepriv, NULL);
    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_SHORT_TIMEOUT);
    if (r) {
        return r;
    }

    unifi_trace(priv, UDBG4, "sme_mgt_disconnect: <-- (status=%d)\n", priv->sme_reply.reply_status);
    return convert_sme_error(priv->sme_reply.reply_status);
}


int sme_mgt_pmkid(unifi_priv_t *priv,
                  unifi_ListAction action,
                  unifi_PmkidList *pmkid_list)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_pmkid: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_mgt_pmkid_req(priv->smepriv, NULL, action,
                        pmkid_list->numElements, pmkid_list->pmkids);
    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_SHORT_TIMEOUT);
    if (r) {
        return r;
    }

    unifi_trace(priv, UDBG4, "sme_mgt_pmkid: <-- (status=%d)\n", priv->sme_reply.reply_status);
    return convert_sme_error(priv->sme_reply.reply_status);
}


int sme_mgt_mib_get(unifi_priv_t *priv,
                    unsigned char *varbind, int *length)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_mib_get: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    priv->mib_cfm_buffer = varbind;
    priv->mib_cfm_buffer_length = MAX_VARBIND_LENGTH;

    unifi_mgt_mib_get_req(priv->smepriv, NULL, *length, varbind);
    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_SHORT_TIMEOUT);
    if (r) {
        priv->mib_cfm_buffer_length = 0;
        priv->mib_cfm_buffer = NULL;
        return r;
    }

    *length = priv->mib_cfm_buffer_length;

    priv->mib_cfm_buffer_length = 0;
    priv->mib_cfm_buffer = NULL;
    unifi_trace(priv, UDBG4, "sme_mgt_mib_get: <-- (status=%d)\n", priv->sme_reply.reply_status);
    return convert_sme_error(priv->sme_reply.reply_status);
}

int sme_mgt_mib_set(unifi_priv_t *priv,
                    unsigned char *varbind, int length)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_mib_get: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_mgt_mib_set_req(priv->smepriv, NULL, length, varbind);
    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_SHORT_TIMEOUT);
    if (r) {
        return r;
    }

    unifi_trace(priv, UDBG4, "sme_mgt_mib_set: <-- (status=%d)\n", priv->sme_reply.reply_status);
    return convert_sme_error(priv->sme_reply.reply_status);
}



int sme_mgt_get_versions(unifi_priv_t *priv, unifi_Versions *versions)
{
    unifi_AppValue sme_app_value;
    int r;

    sme_app_value.id = unifi_VersionsValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv,
                    "sme_mgt_get_versions: Failed to get unifi_VersionsValue.\n");
        return r;
    }

    memcpy(versions, &sme_app_value.unifi_Value_union.versions, sizeof(unifi_Versions));

    return 0;
}


int sme_mgt_get_value(unifi_priv_t *priv, unifi_AppValue *app_value)
{
    unifi_Status status;
    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_get_value: invalid smepriv\n");
        return -EIO;
    }

#ifdef CSR_SME_USERSPACE
    status = sme_mgt_get_value_async(priv->smepriv, app_value);
#else
    /* FIXME : This needs some rework as the
     *         buffers in app_value need to be copied */
    unifi_mgt_claim_sync_access(priv->smepriv);
    status = unifi_mgt_get_value(priv->smepriv, app_value);
    unifi_mgt_release_sync_access(priv->smepriv);
#endif
    return status;
} /* sme_mgt_get_value() */


int sme_mgt_set_value(unifi_priv_t *priv, unifi_AppValue *app_value)
{
    unifi_Status status;
    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_set_value: invalid smepriv\n");
        return -EIO;
    }
#ifdef CSR_SME_USERSPACE
    status = sme_mgt_set_value_async(priv->smepriv, app_value);
#else
    unifi_mgt_claim_sync_access(priv->smepriv);
    status = unifi_mgt_set_value(priv->smepriv, app_value);
    unifi_mgt_release_sync_access(priv->smepriv);
#endif
    return status;
} /* sme_mgt_set_value() */


int sme_mgt_packet_filter_set(unifi_priv_t *priv)
{
    unifi_IPV4Address ipAddress = {{0xFF, 0xFF, 0xFF, 0xFF }};
    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_packet_filter_set: invalid smepriv\n");
        return -EIO;
    }
    if (priv->packet_filters.arp_filter) {
        ipAddress.a[0] = (priv->sta_ip_address      ) & 0xFF;
        ipAddress.a[1] = (priv->sta_ip_address >>  8) & 0xFF;
        ipAddress.a[2] = (priv->sta_ip_address >> 16) & 0xFF;
        ipAddress.a[3] = (priv->sta_ip_address >> 24) & 0xFF;
    }

    unifi_trace(priv, UDBG5,
                "sme_mgt_packet_filter_set: IP address %d.%d.%d.%d\n",
                ipAddress.a[0], ipAddress.a[1],
                ipAddress.a[2], ipAddress.a[3]);

    /* Doesn't block for a confirm */
    unifi_mgt_packet_filter_set_req(priv->smepriv, NULL,
                                    priv->packet_filters.tclas_ies_length,
                                    priv->filter_tclas_ies,
                                    priv->packet_filters.filter_mode,
                                    &ipAddress);
    return 0;
}


int sme_mgt_tspec(unifi_priv_t *priv, unifi_ListAction action,
                  CsrUint32 tid, unifi_DataBlock *tspec, unifi_DataBlock *tclas)
{
    int r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_mgt_tspec: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_mgt_tspec_req(priv->smepriv, NULL, action, tid, TRUE, 0,
                        tspec->length, tspec->data,
                        tclas->length, tclas->data);
    r = sme_wait_for_reply(priv, UNIFI_SME_MGT_SHORT_TIMEOUT);
    if (r) {
        return r;
    }

    unifi_trace(priv, UDBG4, "sme_mgt_tspec: <-- (status=%d)\n", priv->sme_reply.reply_status);
    return convert_sme_error(priv->sme_reply.reply_status);
}
#endif /* CSR_SUPPORT_WEXT */



int sme_sys_suspend(unifi_priv_t *priv)
{
    int r;
    CsrInt32 csr_r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_sys_suspend: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    /* For powered suspend, tell the resume's wifi_on() not to reinit UniFi */
    priv->wol_suspend = (enable_wol == UNIFI_WOL_OFF) ? FALSE : TRUE;  
    
    /* Suspend the SME, which will cause it to power down UniFi */
    unifi_sys_suspend_ind(priv->smepriv, 0, priv->wol_suspend);
    r = sme_wait_for_reply(priv, UNIFI_SME_SYS_LONG_TIMEOUT);
    if (r) {
        /* No reply - forcibly power down in case the request wasn't processed */
        unifi_notice(priv,
                     "suspend: SME did not reply, power off UniFi anyway\n");

        /* Leave power on for production test, though */
        if (!priv->ptest_mode) {
            /* Put UniFi to deep sleep, in case we can not power it off */
            csr_r = unifi_force_low_power_mode(priv->card);

            /* For WOL, the UniFi must stay powered */
            if (!priv->wol_suspend) {
                unifi_trace(priv, UDBG1, "Power off\n");
                CsrSdioPowerOff(priv->sdio);
            }
        }
    }

    if (priv->wol_suspend) {
        unifi_trace(priv, UDBG1, "UniFi left powered for WOL\n");

        /* For PIO WOL, disable SDIO interrupt to enable PIO mode in the f/w */        
        if (enable_wol == UNIFI_WOL_PIO) {
            unifi_trace(priv, UDBG1, "Remove IRQ to enable PIO WOL\n");
            if (csr_sdio_linux_remove_irq(priv->sdio)) {
                unifi_notice(priv, "WOL csr_sdio_linux_remove_irq failed\n");
            }
        }        
    }
    
    /* Consider UniFi to be uninitialised */
    priv->init_progress = UNIFI_INIT_NONE;

    unifi_trace(priv, UDBG1, "sme_sys_suspend: <-- (r=%d status=%d)\n", r, priv->sme_reply.reply_status);
    return convert_sme_error(priv->sme_reply.reply_status);
}


int sme_sys_resume(unifi_priv_t *priv)
{
    int r;

    unifi_trace(priv, UDBG1, "sme_sys_resume %s\n", priv->wol_suspend ? "warm" : "");

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_sys_resume: invalid smepriv\n");
        return -EIO;
    }

    r = sme_init_request(priv);
    if (r) {
        return -EIO;
    }

    unifi_sys_resume_ind(priv->smepriv, priv->wol_suspend);

    if (priv->ptest_mode == 1) {
        r = sme_wait_for_reply(priv, UNIFI_SME_SYS_LONG_TIMEOUT);
        if (r) {
            /* No reply - forcibly power down in case the request wasn't processed */
            unifi_notice(priv,
                        "resume: SME did not reply, return success anyway\n");
        }
    } else {

        /*
         * We are not going to wait for the reply because the SME might be in
         * the userspace. In this case the event will reach it when the kernel
         * resumes. So, release now the SME semaphore that was downed in
         * sme_init_request().
         */
        up(&priv->sme_sem);
    }

    return 0;
}

