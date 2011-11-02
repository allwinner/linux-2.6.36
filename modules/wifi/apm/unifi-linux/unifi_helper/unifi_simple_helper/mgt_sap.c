/** @file mgt_sap.c
 *
 * Linux Example mgt sap implementation
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   This file is a simple example of the mgt sap
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/unifi_simple_helper/mgt_sap.c#1 $
 *
 ****************************************************************************/

#include <stdio.h>
#include <errno.h>

#include "linuxexample.h"
#include "sme_top_level_fsm/sme.h"


void save_calibration_data(LinuxExampleContext* context)
{
    unifi_AppValue value;
    unifi_DataBlock* datablock = &value.unifi_Value_union.calibrationData;

    if(context->calibrationDataFile == NULL)
    {
        return;
    }

    printf("save_calibration_data()\n");

    unifi_mgt_claim_sync_access(context->fsmContext);
    value.id = unifi_CalibrationDataValue;
    if (unifi_mgt_get_value(context->fsmContext, &value) != unifi_Success)
    {
        printf("unifi_mgt_get_value() get Calibration Data Failed\n");
        exit(-1);
    }
    if (datablock->length)
    {
        int writeSize;
        FILE *fp = fopen(context->calibrationDataFile, "w");
        if (!fp)
        {
            printf("save_calibration_data() : file -> %s fopen failed : %s\n", context->calibrationDataFile, strerror(errno));
            return;
        }

        writeSize = fwrite(datablock->data, 1, (unsigned int)datablock->length, fp);
        if (writeSize != datablock->length)
        {
            printf("save_calibration_data() : file -> %s fwrite failed returned %d : %s\n", context->calibrationDataFile, writeSize, strerror(errno));
            fclose(fp);
            return;
        }

        fclose(fp);
    }
    unifi_mgt_release_sync_access(context->fsmContext);
}

void unifi_mgt_wifi_on_cfm(void* context, void* appHandle, unifi_Status status)
{
    LinuxExampleContext* con = (LinuxExampleContext*)context;
    int i;
    unifi_Key key;
    key.authenticator = FALSE;
    key.keyType = unifi_GroupKey;
    memset(&key.address, 0xFF, sizeof(key.address));
    memset(&key.keyRsc, 0x00, sizeof(key.keyRsc));

    printf("unifi_mgt_wifi_on_cfm(%d)\n", status);

    if (status != unifi_Success)
    {
        printf("unifi_mgt_wifi_on_cfm() Wifi on Failed\n");
        exit(status);
    }

    /* Install the Wep Keys */
    for (i = 0; i < 4; ++i)
    {
        if (con->keys[i].keysize)
        {
            key.wepTxKey = (con->txkey == i);
            key.keyIndex = i;
            key.keyLength = con->keys[i].keysize;
            memcpy(key.key, con->keys[i].key, con->keys[i].keysize);
            unifi_mgt_key_req(con->fsmContext, NULL, unifi_ListActionAdd, &key);
        }
    }

    if (con->scan)
    {
        printf("unifi_mgt_scan_full_req()\n");
        unifi_mgt_scan_full_req(context,
                                NULL,
                                0,              /* ssidCount    */
                                NULL,           /* ssid         */
                                NULL,           /* bssid        */
                                FALSE,          /* forceScan    */
                                unifi_AnyBss,   /* bssType      */
                                unifi_ScanAll,  /* scanType     */
                                0, NULL,        /* channelList  */
                                0, NULL);       /* probeIe      */

    }
    else
    {
        unifi_ConnectionConfig connectionConfig;
        connectionConfig.ssid = con->ssid;
        connectionConfig.bssid = con->bssid;
        connectionConfig.bssType = con->adhoc?unifi_Adhoc:unifi_Infrastructure;
        connectionConfig.authModeMask = con->authMode;
        connectionConfig.privacyMode = con->privacy;
        connectionConfig.encryptionModeMask = 0x0000; /* No Wpa Encryption needed */
        connectionConfig.mlmeAssociateReqInformationElementsLength = 0;
        connectionConfig.mlmeAssociateReqInformationElements = NULL;
        connectionConfig.adhocChannel = 0;
        connectionConfig.adhocJoinOnly = FALSE;
        connectionConfig.ifIndex = unifi_GHZ_Both;

        printf("unifi_mgt_connect_req(%.*s)\n", con->ssid.length, con->ssid.ssid);
        (void)fflush(stdout);
        unifi_mgt_connect_req(con->fsmContext, NULL, &connectionConfig);
    }
}

#define WPA2_RSN_ID 0x30
#define WPA_ID      0xDD
#define WPA_OUI1    0x00
#define WPA_OUI2    0x50
#define WPA_OUI3    0xF2
#define WPA_OUI4    0x01

static char* getSecurityType(unifi_ScanResult* scan)
{
    int current = 0;
    if ((scan->capabilityInformation & 0x0010) == 0)
    {
        return "Open";
    }

    /* Can the info elements for WPA/WPA2 data */
    while(current < scan->informationElementsLength)
    {
        /* Check for the WPA2 Info Element */
        if (scan->informationElements[current] == WPA2_RSN_ID)
        {
            return "WPA2";
        }

        /* Check for the WPA Info Element */
        if (scan->informationElements[current]     == WPA_ID &&
            scan->informationElements[current + 2] == WPA_OUI1 &&
            scan->informationElements[current + 3] == WPA_OUI2 &&
            scan->informationElements[current + 4] == WPA_OUI3 &&
            scan->informationElements[current + 5] == WPA_OUI4)
        {
            return "WPA ";
        }

        current += scan->informationElements[current + 1] + 2;
    }

    return "Wep ";
}

void unifi_mgt_scan_full_cfm(void* context, void* appHandle, unifi_Status status)
{
    LinuxExampleContext* con = (LinuxExampleContext*)context;
    unifi_ScanResultList results = {0, NULL};
    unifi_ConnectionConfig connectionConfig;
    int i;
    printf("unifi_mgt_scan_full_cfm(%d)\n", status);

    if (status != unifi_Success)
    {
        printf("unifi_mgt_scan_full_cfm() Scan Failed\n");
        exit(status);
    }

    unifi_mgt_claim_sync_access(con->fsmContext);
    if (unifi_mgt_scan_results_get(con->fsmContext, &results) != unifi_Success)
    {
        printf("unifi_mgt_scan_results_get() Failed\n");
        exit(-1);
    }

    /* Print out the scan results */
    printf("-------------------------- %2d Scan Results -------------------------------\n", results.numElements);
    printf(" type | Chnl | RSSI | SNR | Sec  |       BSSID       | SSID\n");
    printf("--------------------------------------------------------------------------\n");
    for (i = 0; i < results.numElements; ++i)
    {
        unifi_ScanResult* scan = &results.results[i];
        printf("%s |  %2d  | %3d  | %3d | %s | %.2X:%.2X:%.2X:%.2X:%.2X:%.2X | %.*s\n",
                        scan->bssType==unifi_Adhoc?"Adhoc":"Infra",
                        scan->channelNumber,
                        scan->rssi,
                        scan->snr,
                        getSecurityType(scan),
                        scan->bssid.data[0],
                        scan->bssid.data[1],
                        scan->bssid.data[2],
                        scan->bssid.data[3],
                        scan->bssid.data[4],
                        scan->bssid.data[5],
                        scan->ssid.length, scan->ssid.ssid);
    }
    printf("--------------------------------------------------------------------------\n");

    unifi_mgt_release_sync_access(con->fsmContext);

    /* Finished Scanning so connect */
    connectionConfig.ssid = con->ssid;
    connectionConfig.bssid = con->bssid;
    connectionConfig.bssType = con->adhoc?unifi_Adhoc:unifi_Infrastructure;
    connectionConfig.authModeMask = con->authMode;
    connectionConfig.privacyMode = con->privacy;
    connectionConfig.encryptionModeMask = 0x0000; /* No Encryption needed */
    connectionConfig.mlmeAssociateReqInformationElementsLength = 0;
    connectionConfig.mlmeAssociateReqInformationElements = NULL;
    connectionConfig.adhocChannel = 0;
    connectionConfig.adhocJoinOnly = FALSE;
    connectionConfig.ifIndex = unifi_GHZ_Both;

    printf("unifi_mgt_connect_req(%.*s)\n", con->ssid.length, con->ssid.ssid);
    (void)fflush(stdout);
    unifi_mgt_connect_req(con->fsmContext, NULL, &connectionConfig);
}

void unifi_mgt_connect_cfm(void* context, void* appHandle, unifi_Status status)
{
    LinuxExampleContext* con = (LinuxExampleContext*)context;
    unifi_AppValue value;
    unifi_ConnectionInfo* info = &value.unifi_Value_union.connectionInfo;
    printf("unifi_mgt_connect_cfm(%d)\n", status);
    if (status != unifi_Success)
    {
        printf("unifi_mgt_connect_cfm() Connect Failed\n");
        exit(status);
    }

    /* Save the calibration data
     * The SME will refresh the calibration data from the firmware
     * after connecting to an Access point OR after the first full scan */
    save_calibration_data(context);

    unifi_mgt_claim_sync_access(con->fsmContext);
    value.id = unifi_ConnectionInfoValue;
    if (unifi_mgt_get_value(con->fsmContext, &value) != unifi_Success)
    {
        printf("unifi_mgt_get_value() get Connection Info Failed\n");
        exit(-1);
    }

    printf("unifi_mgt_connect_cfm() Connected to %.2X:%.2X:%.2X:%.2X:%.2X:%.2X => %.*s \n",
                                info->bssid.data[0],
                                info->bssid.data[1],
                                info->bssid.data[2],
                                info->bssid.data[3],
                                info->bssid.data[4],
                                info->bssid.data[5],
                                info->ssid.length, info->ssid.ssid);
    printf("Use ifconfig or dhclient assign an ipaddress\n");
    (void)fflush(stdout);
    unifi_mgt_release_sync_access(con->fsmContext);

}

void unifi_mgt_key_cfm(void* context, void* appHandle, unifi_Status status, unifi_ListAction action)
{
    printf("unifi_mgt_key_cfm(%d)\n", status);
    if (status != unifi_Success)
    {
        printf("unifi_mgt_key_cfm() Set Wep Key Failed\n");
        exit(status);
    }
}

void unifi_mgt_wifi_off_cfm(void* context, void* appHandle, unifi_Status status)
{
    printf("unifi_mgt_wifi_off_cfm(%d)\n", status);
}

void unifi_mgt_wifi_off_ind(void* context, CsrUint16 appHandlesCount, void* *appHandles, unifi_ControlIndication controlIndication)
{
    printf("unifi_mgt_wifi_off_ind(%d)\n", controlIndication);
}

void unifi_mgt_set_value_cfm(void* context, void* appHandle, unifi_Status status, unifi_AppValueId appValueId)
{
    printf("unifi_mgt_set_value_cfm(%d)\n", status);
}

void unifi_mgt_get_value_cfm(void* context, void* appHandle, unifi_Status status, const unifi_AppValue* appValue)
{
    printf("unifi_mgt_get_value_cfm(%d)\n", status);
}

void unifi_mgt_mib_set_cfm(void* context, void* appHandle, unifi_Status status)
{
    printf("unifi_mgt_mib_set_cfm(%d)\n", status);
}

void unifi_mgt_mib_get_cfm(void* context, void* appHandle, unifi_Status status, CsrUint16 mibAttributeLength, const CsrUint8 *mibAttribute)
{
    printf("unifi_mgt_mib_get_cfm(%d)\n", status);
}

void unifi_mgt_mib_get_next_cfm(void* context, void* appHandle, unifi_Status status, CsrUint16 mibAttributeLength, const CsrUint8 *mibAttribute)
{
    printf("unifi_mgt_mib_get_next_cfm(%d)\n", status);
}

void unifi_mgt_scan_results_get_cfm(void* context, void* appHandle, unifi_Status status, CsrUint16 scanResultsCount, const unifi_ScanResult *scanResults)
{
    printf("unifi_mgt_scan_results_get_cfm(%d)\n", status);
}

void unifi_mgt_scan_result_ind(void* context, CsrUint16 appHandlesCount, void* *appHandles, const unifi_ScanResult* result)
{
    /*printf("unifi_mgt_scan_result_ind()"); Noisy*/
}

void unifi_mgt_media_status_ind(void* context,
                                CsrUint16 appHandlesCount, void* *appHandles,
                                unifi_MediaStatus mediaStatus, const unifi_ConnectionInfo* connectionInfo,
                                unifi_IEEE80211Reason disassocReason, unifi_IEEE80211Reason deauthReason)
{
    printf("unifi_mgt_media_status_ind(%s)\n", mediaStatus==unifi_MediaConnected?"Connected":"Disconnected");
}

void unifi_mgt_connection_quality_ind(void* context, CsrUint16 appHandlesCount, void* *appHandles, const unifi_LinkQuality *linkQuality)
{
    printf("unifi_mgt_connection_quality_ind()");
}

void unifi_mgt_disconnect_cfm(void* context, void* appHandle, unifi_Status status)
{
    printf("unifi_mgt_disconnect_cfm(%d)\n", status);
}

void unifi_mgt_multicast_address_cfm(void* context, void* appHandle, unifi_Status status, unifi_ListAction action, CsrUint8 getAddressesCount, const unifi_MACAddress* getAddresses)
{
    printf("unifi_mgt_multicast_address_cfm(%d)\n", status);
}

void unifi_mgt_mic_failure_ind(void* context, CsrUint16 appHandlesCount, void* *appHandles, CsrBool secondFailure, CsrUint16 count,
                     const unifi_MACAddress* address,
                     unifi_KeyType keyType,
                     CsrUint16 keyId,
                     const CsrUint16* tSC)
{
    printf("unifi_mgt_mic_failure_ind()\n");
}

void unifi_mgt_pmkid_candidate_list_ind(void* context, CsrUint16 appHandlesCount, void* *appHandles, CsrUint8 pmkidCandidatesCount, const unifi_PmkidCandidate* pmkidCandidates)
{
    printf("unifi_mgt_pmkid_candidate_list_ind(%d)\n", pmkidCandidatesCount);
}

void unifi_mgt_pmkid_cfm(void* context, void* appHandle, unifi_Status status, unifi_ListAction action, CsrUint8 getPmkidsCount, const unifi_Pmkid* getPmkids)
{
    printf("unifi_mgt_pmkid_cfm(%d)\n", status);
}

void unifi_mgt_packet_filter_set_cfm(void* context, void* appHandle, unifi_Status status)
{
    printf("unifi_mgt_packet_filter_set_cfm(%d)\n", status);
}

void unifi_mgt_wifi_flightmode_cfm(void* context, void* appHandle, unifi_Status status)
{
    printf("unifi_mgt_wifi_flightmode_cfm(%d)\n", status);
    exit(0);
}

void unifi_mgt_tspec_cfm(void* context, void* appHandle, unifi_Status status, CsrUint32 transactionId,
                     unifi_TspecResultCode tspecResultCode,
                     CsrUint16 tspecLength,
                     const CsrUint8 *tspec)
{
    printf("unifi_mgt_tspec_cfm(%d)\n", status);
}

void unifi_mgt_tspec_ind(void* context,
                         CsrUint16 appHandlesCount, void* *appHandles,
                         CsrUint32 transactionId,
                         unifi_TspecResultCode tspecResultCode,
                         CsrUint16 tspecLength,
                         const CsrUint8 *tspec)
{
    printf("unifi_mgt_tspec_ind(%d)\n", (int)transactionId);
}

void unifi_mgt_scan_results_flush_cfm(void* context, void* appHandle, unifi_Status status)
{
    printf("unifi_mgt_scan_results_flush_cfm(%d)\n", status);
}

void unifi_mgt_blacklist_cfm(void* context,
                             void* appHandle,
                             unifi_Status status,
                             unifi_ListAction action,
                             CsrUint8 getAddressCount,
                             const unifi_MACAddress *getAddresses)
{
    printf("unifi_mgt_blacklist_cfm(%d)\n", status);
}

void unifi_mgt_roam_start_ind(void* context, CsrUint16 appHandlesCount, void* *appHandles, unifi_RoamReason reason, unifi_IEEE80211Reason reason80211)
{
    printf("unifi_mgt_roam_start_ind(%d)\n", reason);
}

void unifi_mgt_roam_complete_ind(void* context, CsrUint16 appHandlesCount, void* *appHandles, unifi_Status status)
{
    printf("unifi_mgt_roam_complete_ind(%d)\n", status);
}

void unifi_mgt_association_start_ind(void* context,
                                     CsrUint16 appHandlesCount, void* *appHandles,
                                     const unifi_MACAddress *address,
                                     const unifi_SSID *ssid)
{
    printf("unifi_mgt_association_start_ind()\n");
}

void unifi_mgt_association_complete_ind(void* context,
                                        CsrUint16 appHandlesCount, void* *appHandles,
                                        unifi_Status status,
                                        const unifi_ConnectionInfo *connectionInfo,
                                        unifi_IEEE80211Reason deauthReason)
{
    printf("unifi_mgt_association_complete_ind(%d)\n", status);
}

void unifi_mgt_ibss_station_ind(void* context,
                                CsrUint16 appHandlesCount, void* *appHandles,
                                const unifi_MACAddress *address,
                                CsrBool isconnected)
{
    printf("unifi_mgt_ibss_station_ind()\n");
}

void unifi_mgt_event_mask_set_cfm(void* context,
                                  void* appHandle,
                                  unifi_Status status)
{
    printf("unifi_mgt_event_mask_set_cfm(%d)\n", status);
}

void unifi_mgt_restricted_access_enable_cfm(void* context, void* appHandle, unifi_Status status)
{
    printf("unifi_mgt_restricted_access_enable_cfm(%p, %d)\n", appHandle, status);
}

void unifi_mgt_restricted_access_disable_cfm(void* context, void* appHandle, unifi_Status status)
{
    printf("unifi_mgt_restricted_access_disable_cfm(%p, %d)\n", appHandle, status);
}
