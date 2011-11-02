/** @file sys_sap.c
 *
 * Linux Example sys sap implementation
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
 *   This file implements the sys sap when talking to the linux kernel
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/unifi_simple_helper/sys_sap.c#3 $
 *
 ****************************************************************************/

#include <stdio.h>
#include <unistd.h>

#include "linuxexample.h"

#include "sme_top_level_fsm/sme.h"
#include "sys_sap/ipc/sys_sap_serialise.h"
#include "sme_trace/sme_trace.h"
#include "smeio/smeio_trace_types.h"

void unifi_sys_wifi_on_req(void* context, void* appHandle)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_wifi_on_req()" ));

    buflen = serialise_unifi_sys_wifi_on_req(&buf, appHandle);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_wifi_on_rsp(void* context, unifi_Status status, const unifi_MACAddress* stationMacAddress,
                           const unifi_SmeVersions *smeVersions,
                           CsrBool scheduledInterrupt)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_wifi_on_rsp(%s)", trace_unifi_Status(status)));

    buflen = serialise_unifi_sys_wifi_on_rsp(&buf, status, stationMacAddress, smeVersions, scheduledInterrupt);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_wifi_off_req(void* context, void* appHandle)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_wifi_off_req()"));

    buflen = serialise_unifi_sys_wifi_off_req(&buf, appHandle);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_wifi_off_rsp(void* context)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_wifi_off_rsp()"));

    buflen = serialise_unifi_sys_wifi_off_rsp(&buf);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_suspend_rsp(void* context, unifi_Status status)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_suspend_rsp(%s)", trace_unifi_Status(status)));

    buflen = serialise_unifi_sys_suspend_rsp(&buf, status);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_resume_rsp(void* context, unifi_Status status)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_resume_rsp(%s)", trace_unifi_Status(status)));

    buflen = serialise_unifi_sys_resume_rsp(&buf, status);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_hip_req(void* context,
                       CsrUint16 mlmeCommandLength, const CsrUint8 *mlmeCommand,
                       CsrUint16 dataRef1Length, const CsrUint8 *dataRef1,
                       CsrUint16 dataRef2Length, const CsrUint8 *dataRef2)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_hip_req()"));

    buflen = serialise_unifi_sys_hip_req(&buf, mlmeCommandLength, mlmeCommand,
                                               dataRef1Length, dataRef1,
                                               dataRef2Length, dataRef2);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_qos_control_req(void* context, unifi_QoSControl control)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_qos_control_req(%s)", trace_unifi_QoSControl(control)));

    buflen = serialise_unifi_sys_qos_control_req(&buf, control);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_port_configure_req(void* context, unifi_PortAction uncontrolledPortAction,
                                                 unifi_PortAction controlledPortAction,
                                                 const unifi_MACAddress* macAddress)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_port_configure_req(%s, %s)", trace_unifi_PortAction(uncontrolledPortAction),
                                                                         trace_unifi_PortAction(controlledPortAction)));

    buflen = serialise_unifi_sys_port_configure_req(&buf, uncontrolledPortAction, controlledPortAction, macAddress);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_configure_power_mode_req(void* context, unifi_LowPowerMode mode, CsrBool hostWakeup)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_configure_power_mode_req(%s, %s)", trace_unifi_LowPowerMode(mode), hostWakeup?"TRUE":"FALSE"));

    buflen = serialise_unifi_sys_configure_power_mode_req(&buf, mode, hostWakeup);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_media_status_req(void* context, unifi_MediaStatus mediaStatus, CsrUint32 mediaTypeMask)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_media_status_req(%s)", trace_unifi_MediaStatus(mediaStatus)));

    buflen = serialise_unifi_sys_media_status_req(&buf, mediaStatus, mediaTypeMask);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_multicast_address_rsp(void* context, unifi_Status status, unifi_ListAction action, CsrUint8 getAddressesCount, const unifi_MACAddress* getAddresses)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_multicast_address_rsp(%s)", trace_unifi_Status(status)));

    buflen = serialise_unifi_sys_multicast_address_rsp(&buf, status, action, getAddressesCount, getAddresses);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_traffic_config_req(void* context, unifi_TrafficConfigType type, const unifi_TrafficConfig* config)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_traffic_config_req(%s)", trace_unifi_TrafficConfigType(type)));

    buflen = serialise_unifi_sys_traffic_config_req(&buf, type, config);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}

void unifi_sys_traffic_classification_req(void* context, unifi_TrafficType type, CsrUint16 period)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_traffic_classification_req(%s)", trace_unifi_TrafficType(type)));

    buflen = serialise_unifi_sys_traffic_classification_req(&buf, type, period);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}


void unifi_sys_tclas_add_req(void* context,
                                    CsrUint16 tclasLength,
                                    const CsrUint8 *tclas)
{

}

void unifi_sys_tclas_del_req(void* context,
                                    CsrUint16 tclasLength,
                                    const CsrUint8 *tclas)
{

}

void unifi_sys_m4_transmit_req(void* context)
{
    LinuxExampleContext* con = ((LinuxExampleContext*)context);
    CsrUint8* buf;
    CsrUint16 buflen;
    sme_trace_entry((TR_SYS_SAP, "unifi_sys_m4_transmit_req(%s)"));

    buflen = serialise_unifi_sys_m4_transmit_req(&buf);
    write(con->sysSapFd, buf, buflen);
    CsrPfree(buf);
}
