/** @file ipc_private.h
 *
 * remote access abstraction header file for the UniFi SME
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
 *   remote access abstraction header file for the UniFi SME
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/ipc/ipc_private.h#1 $
 *
 ****************************************************************************/

#ifndef IPC_PRIVATE_H
#define IPC_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "ipc/ipc.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/** Forward declaration of IPC information */
typedef struct ipcConnectionImpl ipcConnectionImpl;

typedef void    (*ipc_disconnect_fn)(ipcConnection* con);
typedef CsrBool (*ipc_message_send_fn)(ipcConnection* con, const CsrUint8 *message, CsrUint32 length);
typedef CsrBool (*ipc_message_send_3_fn)(ipcConnection* con, const CsrUint8 *message, CsrUint32 length,
                                                             const CsrUint8 *data1, CsrUint32 data1length,
                                                             const CsrUint8 *data2, CsrUint32 data2length);
typedef CsrBool (*ipc_message_recv_fn)(ipcConnection* con, CsrUint32 timeout_ms, CsrUint8 **message, CsrUint32 *length);
typedef void    (*ipc_wakeup_fn)(ipcConnection* con);
typedef void    (*ipc_message_free_fn)(ipcConnection* con, CsrUint8 *message);
typedef CsrInt32   (*ipc_getFd_fn) (ipcConnection* con);

/** IPC connection definition */
struct ipcConnection
{
    ipc_disconnect_fn       disconnectFn;
    ipc_message_send_fn     messageSendFn;
    ipc_message_send_3_fn   messageSend3Fn;
    ipc_message_recv_fn     messageRecvFn;
    ipc_wakeup_fn           wakeupFn;
    ipc_message_free_fn     messageFreeFn;
    ipc_getFd_fn            getFd;

    ipc_on_connect_fn       connectCallback;
    ipc_on_receive_fn       rxCallback;
    ipc_on_disconnect_fn    disconCallback;

    ipcConnectionImpl* impl;
};

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* IPC_PRIVATE_H */
