/** @file ipc.h
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
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/ipc/ipc.h#1 $
 *
 ****************************************************************************/

#ifndef IPC_H
#define IPC_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/
#include "csr_types.h"

/* PROJECT INCLUDES *********************************************************/

/* PUBLIC MACROS ************************************************************/
#define IPC_INVALID_FD          (-1)

/* PUBLIC TYPES DEFINITIONS *************************************************/
/** Forward declaration of IPC  */
typedef struct ipcConnection ipcConnection;

typedef void (*ipc_on_connect_fn) (void* externalContext);
typedef void (*ipc_on_receive_fn) (void* externalContext, CsrUint8* message, CsrUint16 length);
typedef void (*ipc_on_disconnect_fn) (void* externalContext);

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Disconnect from an IPC channel.
 *
 * @par Description
 * Disconnect from an IPC channel.
 *
 * @param[in] con : connection to disconnect.
 *
 * @return
 *     void
 */
extern void ipc_disconnect(ipcConnection* con);

/**
 * @brief Send a message through IPC.
 *
 * @par Description
 * Send a message through IPC.
 *
 * @param[in]  con     : connection to use
 * @param[in]  message : message to send
 * @param[in]  length  : further data to send contiguously, or NULL for none
 *
 * @return
 *     CsrBool : successfull send
 */
extern CsrBool ipc_message_send(ipcConnection* con, const CsrUint8 *message, CsrUint32 length);

/**
 * @brief Send a message through IPC.
 *
 * @par Description
 * Send a message through IPC.
 *
 * @param[in]  con     : connection to use
 * @param[in]  message : message to send
 * @param[in]  length  : Number of message bytes
 * @param[in]  data1   : message to send
 * @param[in]  data1length  : Number of data1 bytes
 * @param[in]  data2   : message to send
 * @param[in]  data2length  : Number of data2 bytes
 *
 * @return
 *     CsrBool : successfull send
 */
extern CsrBool ipc_message_send_3(ipcConnection* con, const CsrUint8 *message, CsrUint32 length,
                                                      const CsrUint8 *data1, CsrUint32 data1length,
                                                      const CsrUint8 *data2, CsrUint32 data2length);

/**
 * @brief Receive an IPC message.
 *
 * @par Description
 * The callee allocates the data buffer.  Received messages must be freed with
 * ipc_message_free().
 *
 * @param[in] conn       : handle of the connection to use
 * @param[in] timeout_ms : time in ms to wait for a message.  0 indicates an
 *                         immediate return, and (-1) or 0xffffffff indicates
 *                         WAIT_FOREVER.
 * @param[out] message   : used to return the received message,
 *                         or set to NULL on a timeout or error.
 * @param[out] length    :Set to the length of the message being returned, or
 *                        0 for timeout or error.
 *
 * @return
 *     CsrBool : successfull rx or timeout
 */
extern CsrBool ipc_message_recv(ipcConnection* con, CsrUint32 timeout_ms, CsrUint8 **message, CsrUint32 *length);

/**
 * @brief Wake up the ipc recieve
 *
 * @par Description
 * Unblocks the IPC call to ipc_message_recv
 *
 * @param[in]  con    : connection to use
 *
 * @return
 */
extern void ipc_wakeup(ipcConnection* con);

/**
 * @brief Gets the file discriptor of the connection
 *
 * @par Description
 *  Gets the file discriptor of the connection
 *
 * @param[in]  con    : connection to use
 *
 * @return
 *  CsrInt32 fileDiscriptor
 */
extern CsrInt32 ipc_getFd(ipcConnection* con);

/**
 * @brief  Free an IPC message
 *
 * @par Description
 * Free an IPC message.  This frees both the ipc_message
 * structure and any associated data.
 *
 * @param[in] con     : IPC Connection.
 * @param[in] message : message to free.
 *
 * @return
 *     void
 */
extern void ipc_message_free(ipcConnection* con, CsrUint8 *message);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* IPC_H */
