/** @file sme_interface_hip_signal_to_sys_sap.c
 *
 * Processes Hip signals from the sme and passes them to the driver
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
 *   Builds a buffer from the signal and and payloads in the Payload Manager
 *   then passes the buffer to the driver
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/saps_impl/sys_sap/ipc/unifi_sys_hip_req.c#2 $
 *
 ****************************************************************************/

#include "sme_top_level_fsm/sme.h"
#include "sys_sap_serialise.h"
#include "ipc/ipc.h"

extern ipcConnection* get_hip_ipc_connection(FsmContext* context);

#ifdef CUSTOM_UNIFI_SYS_HIP_REQ
void unifi_sys_hip_req(void* context,
                       CsrUint16 mlmeCommandLength, const CsrUint8 *mlmeCommand,
                       CsrUint16 dataRef1Length, const CsrUint8 *dataRef1,
                       CsrUint16 dataRef2Length, const CsrUint8 *dataRef2)
{
    (void)ipc_message_send_3(get_hip_ipc_connection(context),
                           mlmeCommand, mlmeCommandLength,
                           dataRef1, dataRef1Length,
                           dataRef2, dataRef2Length);
}
#endif

#ifdef CSR_AMP_ENABLE
#ifdef CUSTOM_UNIFI_SYS_MA_UNITDATA_REQ
void unifi_sys_ma_unitdata_req(void* context, void* appHandle, CsrUint8 subscriptionHandle, CsrUint16 frameLength, const CsrUint8 *frame, unifi_FrameFreeFunction freeFunction, unifi_Priority priority, unifi_ServiceClass serviceClass, CsrUint32 reqIdentifier)
{
    CsrUint8* evt;
    CsrUint16 packedLength = serialise_unifi_sys_ma_unitdata_req(&evt, appHandle, subscriptionHandle, frameLength, frame, freeFunction, priority, serviceClass, reqIdentifier);

    (void)ipc_message_send(get_hip_ipc_connection(context), evt, packedLength);
    CsrPfree(evt);

    if (freeFunction)
    {
        (*freeFunction)((void*)frame);
    }
    else
    {
        CsrPfree((CsrUint8 *)frame);
    }
}
#endif /* CUSTOM_UNIFI_SYS_MA_UNITDATA_REQ */
#endif

