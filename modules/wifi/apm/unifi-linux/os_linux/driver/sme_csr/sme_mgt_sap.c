/*
 * ---------------------------------------------------------------------------
 *
 * FILE: sme_mgt_sap.c
 *
 * PURPOSE:
 * This file provides the SME MGT API implementation.
 *
 * Copyright (C) 2008 Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */

#include "unifi_priv.h"
#include "sme_top_level_fsm/sme.h"
#include "mgt_sap/mgt_sap_serialise.h"


void unifi_mgt_wifi_on_req(FsmContext* context, void* appHandle, const unifi_MACAddress* address, CsrUint16 mibFilesCount, const unifi_DataBlock *mibFiles)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_wifi_on_req()\n");

    buflen = serialise_unifi_mgt_wifi_on_req(&buf, appHandle, address, mibFilesCount, mibFiles);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_wifi_off_req(FsmContext* context, void* appHandle)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_wifi_off_req\n");

    buflen = serialise_unifi_mgt_wifi_off_req(&buf, appHandle);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_set_value_req(FsmContext* context, void* appHandle,
                             const unifi_AppValue* appValue)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_set_value_req(%d)\n", appValue->id);

    buflen = serialise_unifi_mgt_set_value_req(&buf, appHandle, appValue);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_get_value_req(FsmContext* context, void* appHandle, unifi_AppValueId appValueId)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_get_value_req(%d)\n", appValueId);

    buflen = serialise_unifi_mgt_get_value_req(&buf, appHandle, appValueId);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_mib_get_req(FsmContext* context, void* appHandle, CsrUint16 mibAttributeLength, const CsrUint8 *mibAttribute)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_mib_get_req\n");

    buflen = serialise_unifi_mgt_mib_get_req(&buf, appHandle, mibAttributeLength, mibAttribute);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_mib_set_req(FsmContext* context, void* appHandle, CsrUint16 mibAttributeLength, const CsrUint8 *mibAttribute)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_mib_set_req\n");

    buflen = serialise_unifi_mgt_mib_set_req(&buf, appHandle, mibAttributeLength, mibAttribute);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_mib_get_next_req(FsmContext* context, void* appHandle, CsrUint16 mibAttributeLength, const CsrUint8 *mibAttribute)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_mib_get_next_req\n");

    buflen = serialise_unifi_mgt_mib_get_next_req(&buf, appHandle, mibAttributeLength, mibAttribute);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_scan_full_req(FsmContext* context, void* appHandle,
                             CsrUint8 ssidCount,
                             const unifi_SSID *ssid,
                             const unifi_MACAddress *bssid,
                             CsrBool forceScan,
                             unifi_BSSType bssType,
                             unifi_ScanType scanType,
                             CsrUint16 channelListCount,
                             const CsrUint8 *channelList,
                             CsrUint16 probeIeLength,
                             const CsrUint8 *probeIe)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_scan_full_req\n");

    buflen = serialise_unifi_mgt_scan_full_req(&buf, appHandle, ssidCount, ssid, bssid, forceScan,
                                               bssType, scanType, channelListCount, channelList,
                                               probeIeLength, probeIe);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_scan_results_get_req(FsmContext* context, void* appHandle)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_scan_results_get_req\n");

    buflen = serialise_unifi_mgt_scan_results_get_req(&buf, appHandle);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_scan_results_flush_req(FsmContext* context, void* appHandle)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_scan_results_flush_req\n");

    buflen = serialise_unifi_mgt_scan_results_flush_req(&buf, appHandle);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_mgt_connect_req(FsmContext* context, void* appHandle,
                           const unifi_ConnectionConfig* connectionConfig)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_connect_req\n");

    buflen = serialise_unifi_mgt_connect_req(&buf, appHandle, connectionConfig);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_disconnect_req(FsmContext* context, void* appHandle)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_disconnect_req\n");

    buflen = serialise_unifi_mgt_disconnect_req(&buf, appHandle);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_multicast_address_req(FsmContext* context, void* appHandle,
                                     unifi_ListAction action,
                                     CsrUint8 setAddressesCount,
                                     const unifi_MACAddress *setAddresses)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_multicast_address_req\n");

    buflen = serialise_unifi_mgt_multicast_address_req(&buf, appHandle, action,
                                                        setAddressesCount,
                                                        setAddresses);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_pmkid_req(FsmContext* context, void* appHandle, unifi_ListAction action,
                         CsrUint8 setPmkidsCount, const unifi_Pmkid *setPmkids)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_pmkid_req\n");

    buflen = serialise_unifi_mgt_pmkid_req(&buf, appHandle, action, setPmkidsCount, setPmkids);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_key_req(FsmContext* context, void* appHandle, unifi_ListAction action,
                       const unifi_Key* key)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_key_req\n");

    buflen = serialise_unifi_mgt_key_req(&buf, appHandle, action, key);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_packet_filter_set_req(FsmContext* context, void* appHandle,
                                     CsrUint16 filterLength,
                                     const CsrUint8 *filter,
                                     unifi_PacketFilterMode mode,
                                     const unifi_IPV4Address *ipV4Address)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_packet_filter_set_req\n");

    buflen = serialise_unifi_mgt_packet_filter_set_req(&buf, appHandle, filterLength, filter,
                                                          mode, ipV4Address);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_tspec_req(FsmContext* context, void* appHandle,
                         unifi_ListAction action,
                         CsrUint32 transactionId,
                         CsrBool strict,
                         CsrUint8 ctrlMask,
                         CsrUint16 tspecLength,
                         const CsrUint8 *tspec,
                         CsrUint16 tclasLength,
                         const CsrUint8 *tclas)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_tspec_req\n");

    buflen = serialise_unifi_mgt_tspec_req(&buf, appHandle, action, transactionId,
                                           strict, ctrlMask, tspecLength,
                                           tspec, tclasLength, tclas);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_mgt_event_mask_set_req(FsmContext* context,
                                  void* appHandle,
                                  CsrUint32 indMask)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_mgt_event_mask_set_req(%d, 0x%.8X)\n", appHandle, indMask);

    buflen = serialise_unifi_mgt_event_mask_set_req(&buf, appHandle, indMask);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}
