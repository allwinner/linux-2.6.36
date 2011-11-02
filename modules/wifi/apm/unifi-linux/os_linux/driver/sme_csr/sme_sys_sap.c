/*
 * ---------------------------------------------------------------------------
 *
 * FILE: sme_sys_sap.c
 *
 * PURPOSE:
 * This file provides the SME SYS API implementation.
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
#include "sys_sap/sys_sap_serialise.h"


void unifi_sys_wifi_on_ind(FsmContext* context, unifi_Status status,
                           const unifi_DriverVersions* versions)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_wifi_on_ind(%d)\n", status);

    buflen = serialise_unifi_sys_wifi_on_ind(&buf, status, versions);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_wifi_on_cfm(FsmContext* context, unifi_Status status)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_wifi_on_cfm(%d)\n", status);

    buflen = serialise_unifi_sys_wifi_on_cfm(&buf, status);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_wifi_off_ind(FsmContext* context,
                            unifi_ControlIndication controlIndication)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_wifi_off_ind\n");

    buflen = serialise_unifi_sys_wifi_off_ind(&buf, controlIndication);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_wifi_off_cfm(FsmContext* context)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_wifi_off_cfm\n");

    buflen = serialise_unifi_sys_wifi_off_cfm(&buf);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_suspend_ind(FsmContext* context, CsrBool hardSuspend, CsrBool d3Suspend)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_suspend_ind(%d, %d)\n", hardSuspend, d3Suspend);

    buflen = serialise_unifi_sys_suspend_ind(&buf, hardSuspend, d3Suspend);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_resume_ind(FsmContext* context, CsrBool resumePowerMaintained)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2,
                "unifi_sys_resume_ind(%d)\n", resumePowerMaintained);

    buflen = serialise_unifi_sys_resume_ind(&buf, resumePowerMaintained);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_hip_ind(FsmContext* context,
                              CsrUint16 mlmeCommandLength,
                              const CsrUint8 *mlmeCommand,
                              CsrUint16 dataRef1Length,
                              const CsrUint8 *dataRef1,
                              CsrUint16 dataRef2Length,
                              const CsrUint8 *dataRef2)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_hip_ind\n");

    buflen = serialise_unifi_sys_hip_ind(&buf, mlmeCommandLength, mlmeCommand,
                                               dataRef1Length, dataRef1,
                                               dataRef2Length, dataRef2);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_qos_control_cfm(FsmContext* context, unifi_Status status)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_qos_control_cfm(%d)\n", status);

    buflen = serialise_unifi_sys_qos_control_cfm(&buf, status);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_port_configure_cfm(FsmContext* context, unifi_Status status,
                                  const unifi_MACAddress* macAddress)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_port_configure_cfm(%d)\n", status);

    buflen = serialise_unifi_sys_port_configure_cfm(&buf, status,
                                                       macAddress);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_sys_traffic_protocol_ind(FsmContext* context,
                                    unifi_TrafficPacketType packetType,
                                    unifi_ProtocolDirection direction,
                                    const unifi_MACAddress* srcAddress)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_traffic_protocol_ind\n");

    buflen = serialise_unifi_sys_traffic_protocol_ind(&buf,
                                                         packetType,
                                                         direction,
                                                         srcAddress);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_sys_traffic_sample_ind(FsmContext* context, const unifi_TrafficStats* stats)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_traffic_protocol_ind\n");

    buflen = serialise_unifi_sys_traffic_sample_ind(&buf,stats);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_ip_configured_ind(FsmContext* context, CsrBool ipConfigured)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2,
                "unifi_sys_ip_configured_ind(%d)\n", ipConfigured);

    buflen = serialise_unifi_sys_ip_configured_ind(&buf, ipConfigured);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_multicast_address_ind(FsmContext* context,
                                     unifi_ListAction action,
                                     CsrUint8 setAddressesCount,
                                     const unifi_MACAddress *setAddresses)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_multicast_address_ind\n");

    buflen = serialise_unifi_sys_multicast_address_ind(&buf,
                                                       action,
                                                       setAddressesCount,
                                                       setAddresses);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_tclas_add_cfm(FsmContext* context, unifi_Status status)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_tclas_add_cfm(%d)\n", status);

    buflen = serialise_unifi_sys_tclas_add_cfm(&buf, status);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}


void unifi_sys_tclas_del_cfm(FsmContext* context, unifi_Status status)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_tclas_del_cfm(%d)\n", status);

    buflen = serialise_unifi_sys_tclas_del_cfm(&buf, status);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_sys_ma_unitdata_subscribe_cfm(FsmContext* context, void* appHandle, CsrUint8 subscriptionHandle,
                                         unifi_SubscriptionResult status, CsrUint16 allocOffset)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_ma_unitdata_subscribe_cfm(%p, %d, %d, %d)\n", appHandle, subscriptionHandle, status, allocOffset);

    buflen = serialise_unifi_sys_ma_unitdata_subscribe_cfm(&buf, appHandle, subscriptionHandle, status, allocOffset);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_sys_ma_unitdata_unsubscribe_cfm(FsmContext* context, void* appHandle, unifi_SubscriptionResult status)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_ma_unitdata_unsubscribe_cfm(%p, %d)\n", appHandle, status);

    buflen = serialise_unifi_sys_ma_unitdata_unsubscribe_cfm(&buf, appHandle, status);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_sys_capabilities_cfm(FsmContext* context, void* appHandle, CsrUint16 commandQueueSize, CsrUint16 trafficQueueSize)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_capabilities_cfm(%p, %d, %d)\n", appHandle, commandQueueSize, trafficQueueSize);

    buflen = serialise_unifi_sys_capabilities_cfm(&buf, appHandle, commandQueueSize, trafficQueueSize);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_sys_ma_unitdata_ind(FsmContext* context,
                               void* appHandle,
                               CsrUint8 subscriptionHandle,
                               CsrUint16 frameLength,
                               const CsrUint8 *frame,
                               unifi_FrameFreeFunction freeFunction,
                               unifi_ReceptionStatus receptionStatus,
                               unifi_Priority priority,
                               unifi_ServiceClass serviceClass)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_ma_unitdata_ind(%d, %d)\n", subscriptionHandle, frameLength);

    buflen = serialise_unifi_sys_ma_unitdata_ind(&buf, appHandle, subscriptionHandle, frameLength, frame, freeFunction, receptionStatus, priority,serviceClass);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_sys_ma_unitdata_cfm(FsmContext* context,
                               void* appHandle,
                               unifi_Status result,
                               unifi_TransmissionStatus transmissionStatus,
                               unifi_Priority providedPriority,
                               unifi_ServiceClass providedServiceClass,
                               CsrUint32 reqIdentifier)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_ma_unitdata_cfm(%d)\n", result);
    buflen = serialise_unifi_sys_ma_unitdata_cfm(&buf, appHandle,
                                                 result,
                                                 transmissionStatus,
                                                 providedPriority,
                                                 providedServiceClass,
                                                 reqIdentifier);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_sys_eapol_cfm(FsmContext* context, void* appHandle,
                         unifi_EapolRc result)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_eapol_cfm(%d)\n", result);

    buflen = serialise_unifi_sys_eapol_cfm(&buf, appHandle, result);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}

void unifi_sys_m4_transmitted_ind(FsmContext* context)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_sys_m4_transmitted_ind()\n");

    buflen = serialise_unifi_sys_m4_transmitted_ind(&buf);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}
