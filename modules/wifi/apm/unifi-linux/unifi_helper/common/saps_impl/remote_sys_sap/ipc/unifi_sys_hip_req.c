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
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/saps_impl/remote_sys_sap/ipc/unifi_sys_hip_req.c#1 $
 *
 ****************************************************************************/

#include "sme_top_level_fsm/sme.h"
#include "ipc/ipc.h"

#ifdef CUSTOM_REMOTE_UNIFI_SYS_HIP_REQ
extern ipcConnection* get_remote_sys_ipc_connection(FsmContext* context);


void remote_unifi_sys_hip_req(void* context,
                       CsrUint16 mlmeCommandLength, const CsrUint8 *mlmeCommand,
                       CsrUint16 dataRef1Length, const CsrUint8 *dataRef1,
                       CsrUint16 dataRef2Length, const CsrUint8 *dataRef2)
{
    (void)ipc_message_send_3(get_remote_sys_ipc_connection(context),
                           mlmeCommand, mlmeCommandLength,
                           dataRef1, dataRef1Length,
                           dataRef2, dataRef2Length);
}
#endif
