/** @file ipc_tx_thread.h
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/ipc/linux/ipc_tx_thread.h#1 $
 *
 ****************************************************************************/

#ifndef IPC_TX_THREAD_H
#define IPC_TX_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/
#include "csr_types.h"

/* PROJECT INCLUDES *********************************************************/
#include "ipc/ipc_private.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/
#ifdef IPC_TX_THREAD

typedef struct IpcThreadContext IpcThreadContext;

extern IpcThreadContext* ipc_create_thread(ipcConnection* con, ipc_message_send_fn sendFn);
extern void ipc_destroy_thread(IpcThreadContext* context);

extern CsrBool ipc_thread_message_send(IpcThreadContext* context, const CsrUint8 *message, CsrUint32 length);
extern CsrBool ipc_thread_message_send_3(IpcThreadContext* context,
                                         const CsrUint8 *message, CsrUint32 length,
                                         const CsrUint8 *data1, CsrUint32 data1length,
                                         const CsrUint8 *data2, CsrUint32 data2length);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* IPC_H */
