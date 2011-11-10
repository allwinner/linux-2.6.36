/** @file ipc_chardevice.h
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
 *   Header for the public IPC API using a character device
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/ipc/linux/ipc_chardevice.h#1 $
 *
 ****************************************************************************/

#ifndef IPC_CHARDEVICE_H
#define IPC_CHARDEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/
#include "csr_types.h"

/* PROJECT INCLUDES *********************************************************/

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/
#ifdef IPC_CHARDEVICE

/**
 * @brief Connect to an IPC channel.
 *
 * @par Description
 * Connect to an IPC Channel.
 *
 * If the channel is invalid, errno is set to EINVAL and NULL is
 * returned.
 *
 * @param[in] devNode : device Node to connect to.
 * @param[in] externalContext : external context to pass to callbacks
 * @param[in] rxCallback : callback on receiving data
 * @param[in] disconCallback : callback on ipc disconnecting
 *
 * @return
 * ipcConnection : handle to the opened connection or NULL on error.
 */
extern ipcConnection* ipc_chardevice_connect(const char* devNode,
                                             void* externalContext,
                                             ipc_on_connect_fn connectCallback,
                                             ipc_on_receive_fn rxCallback,
                                             ipc_on_disconnect_fn disconCallback);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* IPC_H */
