/*
 * ---------------------------------------------------------------------------
 * FILE:     sme_mgt.c
 *
 * PURPOSE:
 *      This file contains the driver specific implementation of
 *      the SME MGT SAP.
 *      It is part of the porting exercise.
 *
 * Copyright (C) 2008-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */

#include "driver/unifiversion.h"
#include "unifi_priv.h"
#include "driver/conversions.h"

/*
 * This file implements the SME MGT API. It contains the following functions:
 * unifi_mgt_wifi_flightmode_cfm()
 * unifi_mgt_wifi_on_cfm()
 * unifi_mgt_wifi_off_cfm()
 * unifi_mgt_wifi_off_ind()
 * unifi_mgt_scan_full_cfm()
 * unifi_mgt_scan_results_get_cfm()
 * unifi_mgt_scan_result_ind()
 * unifi_mgt_scan_results_flush_cfm()
 * unifi_mgt_connect_cfm()
 * unifi_mgt_media_status_ind()
 * unifi_mgt_disconnect_cfm()
 * unifi_mgt_key_cfm()
 * unifi_mgt_multicast_address_cfm()
 * unifi_mgt_set_value_cfm()
 * unifi_mgt_get_value_cfm()
 * unifi_mgt_mic_failure_ind()
 * unifi_mgt_pmkid_cfm()
 * unifi_mgt_pmkid_candidate_list_ind()
 * unifi_mgt_mib_set_cfm()
 * unifi_mgt_mib_get_cfm()
 * unifi_mgt_mib_get_next_cfm()
 * unifi_mgt_connection_quality_ind()
 * unifi_mgt_packet_filter_set_cfm()
 * unifi_mgt_tspec_cfm()
 * unifi_mgt_tspec_ind()
 * unifi_mgt_blacklist_cfm()
 * unifi_mgt_event_mask_set_cfm()
 * unifi_mgt_roam_start_ind()
 * unifi_mgt_roam_complete_ind()
 * unifi_mgt_association_start_ind()
 * unifi_mgt_association_complete_ind()
 * unifi_mgt_ibss_station_ind()
 */


void unifi_mgt_mic_failure_ind(void *drvpriv,
                               CsrUint16 appHandlesCount, void* *appHandles,
                               CsrBool secondFailure,
                               CsrUint16 count, const unifi_MACAddress* address,
                               unifi_KeyType keyType, CsrUint16 keyId,
                               const CsrUint16* tSC)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    CSR_MLME_MICHAELMICFAILURE_INDICATION mic_ind;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_mic_failure_ind: invalid priv\n");
        return;
    }

    unifi_trace(priv, UDBG1,
                "unifi_mgt_mic_failure_ind: count=%d, KeyType=%d, KeyId=%d\n",
                count, keyType, keyId);

    mic_ind.Count = count;
    memcpy(mic_ind.Address.x, address->data, 6);
    mic_ind.KeyType = keyType;
    mic_ind.KeyId = keyId;
    memcpy(mic_ind.Tsc, tSC, sizeof(CsrUint16) * 4);

    wext_send_michaelmicfailure_event(priv, &mic_ind);
}


void unifi_mgt_pmkid_cfm(void *drvpriv, void* appHandle, unifi_Status status,
                         unifi_ListAction action,
                         CsrUint8 getPmkidsCount,
                         const unifi_Pmkid *getPmkids)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_pmkid_cfm: Invalid ospriv.\n");
        return;
    }

    /*
     * WEXT never does a GET operation the PMKIDs, so we don't need
     * handle data returned in pmkids.
     */

    sme_complete_request(priv, status);
}


void unifi_mgt_pmkid_candidate_list_ind(void *drvpriv,
                                        CsrUint16 appHandlesCount, void* *appHandles,
                                        CsrUint8 pmkidCandidatesCount,
                                        const unifi_PmkidCandidate *pmkidCandidates)
{
}

void unifi_mgt_scan_results_flush_cfm(void *drvpriv, void* appHandle, unifi_Status status)
{
}

void unifi_mgt_scan_results_get_cfm(void *drvpriv, void* appHandle, unifi_Status status,
                                    CsrUint16 scanResultsCount, const unifi_ScanResult *scanResults)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    int bytesRequired = scanResultsCount * sizeof(unifi_ScanResult);
    int i;
    CsrUint8* current_buff;
    unifi_ScanResult* scanCopy;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_scan_results_get_cfm: Invalid ospriv.\n");
        return;
    }

    /* Calc the size of the buffer reuired */
    for (i = 0; i < scanResultsCount; ++i) {
        const unifi_ScanResult *scan_result = &scanResults[i];
        bytesRequired += scan_result->informationElementsLength;
    }

    /* Take a Copy of the scan Results :-) */
    scanCopy = CsrPmalloc(bytesRequired);
    memcpy(scanCopy, scanResults, sizeof(unifi_ScanResult) * scanResultsCount);

    /* Take a Copy of the Info Elements AND update the scan result pointers */
    current_buff = (CsrUint8*)&scanCopy[scanResultsCount];
    for (i = 0; i < scanResultsCount; ++i)
    {
        unifi_ScanResult *scan_result = &scanCopy[i];
        CsrMemCpy(current_buff, scan_result->informationElements, scan_result->informationElementsLength);
        scan_result->informationElements = current_buff;
        current_buff += scan_result->informationElementsLength;
    }

    priv->sme_reply.reply_scan_results_count = scanResultsCount;
    priv->sme_reply.reply_scan_results = scanCopy;

    sme_complete_request(priv, status);
}


void unifi_mgt_scan_full_cfm(void *drvpriv, void* appHandle, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_scan_full_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}


void unifi_mgt_scan_result_ind(void *drvpriv,
                               CsrUint16 appHandlesCount, void* *appHandles,
                               const unifi_ScanResult* result)
{

}


void unifi_mgt_connect_cfm(void *drvpriv, void* appHandle, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_connect_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}


void unifi_mgt_disconnect_cfm(void *drvpriv, void* appHandle, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_disconnect_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}


void unifi_mgt_key_cfm(void *drvpriv, void* appHandle, unifi_Status status,
                       unifi_ListAction action)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_key_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}


void unifi_mgt_multicast_address_cfm(void *drvpriv, void* appHandle, unifi_Status status,
                                     unifi_ListAction action,
                                     CsrUint8 getAddressesCount,
                                     const unifi_MACAddress *getAddresses)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_multicast_address_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}

void unifi_mgt_wifi_flightmode_cfm(void *drvpriv, void* appHandle, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_wifi_flightmode_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}

void unifi_mgt_wifi_on_cfm(void *drvpriv, void* appHandle, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_wifi_on_cfm: Invalid ospriv.\n");
        return;
    }

    unifi_trace(priv, UDBG4,
                "unifi_mgt_wifi_on_cfm: wake up status %d\n", status);

}

void unifi_mgt_wifi_off_cfm(void *drvpriv, void* appHandle, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_wifi_off_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}


void unifi_mgt_wifi_off_ind(void *drvpriv,
                            CsrUint16 appHandlesCount, void* *appHandles,
                            unifi_ControlIndication status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_sys_stopped_req: Invalid ospriv.\n");
        return;
    }

    if (priv->smepriv == NULL) {
        unifi_error(priv, "unifi_sys_stopped_req: invalid smepriv\n");
        return;
    }

    /*
     * If the status indicates an error, the SME is in a stopped state.
     * We need to start it again in order to reinitialise UniFi.
     */
    switch (status) {
        case unifi_ControlError:
          unifi_trace(priv, UDBG1,
                      "unifi_sys_stopped_req: Restarting SME (ind:%d)\n",
                      status);

          /* On error, restart the SME */
          sme_mgt_wifi_on(priv);
          break;
        case unifi_ControlExit:
          break;
        default:
          break;
    }

}


void unifi_mgt_set_value_cfm(void *drvpriv, void* appHandle, unifi_Status status,
                             unifi_AppValueId ValueId)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_set_value_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}


void unifi_mgt_get_value_cfm(void *drvpriv, void* appHandle, unifi_Status status,
                             const unifi_AppValue* appValue)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_get_value_cfm: Invalid ospriv.\n");
        return;
    }

    memcpy((unsigned char*)&priv->sme_reply.reply_app_value,
           (unsigned char*)appValue,
           sizeof(unifi_AppValue));

    sme_complete_request(priv, status);
}

void unifi_mgt_mib_set_cfm(void *drvpriv, void* appHandle, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_mib_set_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}

void unifi_mgt_mib_get_cfm(void *drvpriv, void* appHandle, unifi_Status status,
                                          CsrUint16 mibAttributeLength,
                                          const CsrUint8 *mibAttribute)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_mib_get_cfm: Invalid ospriv.\n");
        return;
    }

    if (mibAttribute == NULL) {
        unifi_error(priv, "unifi_mgt_mib_get_cfm: Empty reply.\n");
        sme_complete_request(priv, status);
        return;
    }

    if ((priv->mib_cfm_buffer != NULL) &&
        (priv->mib_cfm_buffer_length >= mibAttributeLength)) {
        memcpy(priv->mib_cfm_buffer, mibAttribute, mibAttributeLength);
        priv->mib_cfm_buffer_length = mibAttributeLength;
    } else {
        unifi_error(priv,
                    "unifi_mgt_mib_get_cfm: No room to store MIB data (have=%d need=%d).\n",
                    priv->mib_cfm_buffer_length, mibAttributeLength);
    }

    sme_complete_request(priv, status);
}

void unifi_mgt_mib_get_next_cfm(void *drvpriv, void* appHandle, unifi_Status status,
                                CsrUint16 mibAttributeLength,
                                const CsrUint8 *mibAttribute)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_mib_get_next_cfm: Invalid ospriv.\n");
        return;
    }

    /* Need to copy MIB data */

    sme_complete_request(priv, status);
}

void unifi_mgt_connection_quality_ind(void *drvpriv,
                                      CsrUint16 appHandlesCount, void* *appHandles,
                                      const unifi_LinkQuality* linkQuality)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    int signal, noise, snr;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_connection_quality_ind: Invalid ospriv.\n");
        return;
    }

    /*
     * level and noise below are mapped into an unsigned 8 bit number,
     * ranging from [-192; 63]. The way this is achieved is simply to
     * add 0x100 onto the number if it is negative,
     * once clipped to the correct range.
     */
    signal = linkQuality->unifiRssi;
    /* Clip range of snr */
    snr    = (linkQuality->unifiSnr > 0) ? linkQuality->unifiSnr : 0; /* In dB relative, from 0 - 255 */
    snr    = (snr < 255) ? snr : 255;
    noise  = signal - snr;

    /* Clip range of signal */
    signal = (signal < 63) ? signal : 63;
    signal = (signal > -192) ? signal : -192;

    /* Clip range of noise */
    noise = (noise < 63) ? noise : 63;
    noise = (noise > -192) ? noise : -192;

    /* Make u8 */
    signal = ( signal < 0 ) ? signal + 0x100 : signal;
    noise = ( noise < 0 ) ? noise + 0x100 : noise;

    priv->wext_wireless_stats.qual.level   = (u8)signal; /* -192 : 63 */
    priv->wext_wireless_stats.qual.noise   = (u8)noise;  /* -192 : 63 */
    priv->wext_wireless_stats.qual.qual    = snr;         /* 0 : 255 */
    priv->wext_wireless_stats.qual.updated = 0;

#if WIRELESS_EXT > 16
    priv->wext_wireless_stats.qual.updated |= IW_QUAL_LEVEL_UPDATED |
                                              IW_QUAL_NOISE_UPDATED |
                                              IW_QUAL_QUAL_UPDATED;
#if WIRELESS_EXT > 18
    priv->wext_wireless_stats.qual.updated |= IW_QUAL_DBM;
#endif
#endif
}

void unifi_mgt_packet_filter_set_cfm(void *drvpriv, void* appHandle, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_packet_filter_set_cfm: Invalid ospriv.\n");
        return;
    }

    /* The packet filter set request does not block for a reply */
}

void unifi_mgt_tspec_cfm(void* drvpriv, void* appHandle, unifi_Status status,
                         CsrUint32 transactionId,
                         unifi_TspecResultCode tspecResultCode,
                         CsrUint16 tspecLength,
                         const CsrUint8 *tspec)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_mgt_tspec_cfm: Invalid ospriv.\n");
        return;
    }

    sme_complete_request(priv, status);
}

void unifi_mgt_tspec_ind(void* drvpriv,
                         CsrUint16 appHandlesCount,
                         void* *appHandles,
                         CsrUint32 transactionId,
                         unifi_TspecResultCode tspecResultCode,
                         CsrUint16 tspecLength,
                         const CsrUint8 *tspec)
{
}

void unifi_mgt_blacklist_cfm(void* drvpriv,
                             void* appHandle,
                             unifi_Status status,
                             unifi_ListAction action,
                             CsrUint8 getAddressCount,
                             const unifi_MACAddress *getAddresses)
{
}

void unifi_mgt_event_mask_set_cfm(void* drvpriv, void* appHandle, unifi_Status status)
{
}


void unifi_mgt_roam_start_ind(void* drvpriv,
                              CsrUint16 appHandlesCount, void* *appHandles,
                              unifi_RoamReason reason, unifi_IEEE80211Reason reason80211)
{
}

void unifi_mgt_roam_complete_ind(void* drvpriv,
                                 CsrUint16 appHandlesCount, void* *appHandles,
                                 unifi_Status status)
{
}

void unifi_mgt_association_start_ind(void* drvpriv,
                                     CsrUint16 appHandlesCount, void* *appHandles,
                                     const unifi_MACAddress *address,
                                     const unifi_SSID *ssid)
{
}

void unifi_mgt_association_complete_ind(void* drvpriv,
                                        CsrUint16 appHandlesCount, void* *appHandles,
                                        unifi_Status status,
                                        const unifi_ConnectionInfo *connectionInfo,
                                        unifi_IEEE80211Reason deauthReason)
{
}

void unifi_mgt_ibss_station_ind(void* context,
                                CsrUint16 appHandlesCount, void* *appHandles,
                                const unifi_MACAddress *address,
                                CsrBool isconnected)
{
}

void unifi_mgt_restricted_access_enable_cfm(void* context, void* appHandle, unifi_Status status)
{
}

void unifi_mgt_restricted_access_disable_cfm(void* context, void* appHandle, unifi_Status status)
{
}

void unifi_mgt_media_status_ind(void *drvpriv,
                                CsrUint16 appHandlesCount, void* *appHandles,
                                unifi_MediaStatus mediaStatus,
                                const unifi_ConnectionInfo* connection_info,
                                unifi_IEEE80211Reason disassocReason,
                                unifi_IEEE80211Reason deauthReason)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "unifi_mgt_media_status_ind: invalid smepriv\n");
        return;
    }

    if (mediaStatus == unifi_MediaConnected) {
        /*
         * Send wireless-extension event up to userland to announce
         * connection.
         */
        wext_send_assoc_event(priv,
                              (unsigned char *)connection_info->bssid.data,
                              (unsigned char *)connection_info->assocReqInfoElements,
                              connection_info->assocReqInfoElementsLength,
                              (unsigned char *)connection_info->assocRspInfoElements,
                              connection_info->assocRspInfoElementsLength,
                              (unsigned char *)connection_info->assocScanInfoElements,
                              connection_info->assocScanInfoElementsLength);

        unifi_trace(priv, UDBG2,
                    "unifi_mgt_media_status_ind: IBSS=%02X:%02X:%02X:%02X:%02X:%02X\n",
                    connection_info->bssid.data[0],
                    connection_info->bssid.data[1],
                    connection_info->bssid.data[2],
                    connection_info->bssid.data[3],
                    connection_info->bssid.data[4],
                    connection_info->bssid.data[5]);

        sme_mgt_packet_filter_set(priv);

    } else  {
        /*
         * Send wireless-extension event up to userland to announce
         * connection lost to a BSS.
         */
        wext_send_disassoc_event(priv);
    }
}

