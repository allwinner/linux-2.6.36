/** @file ipc.c
 *
 * BT interface
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
 *   This file implements the BT sap for unicore functionality
 * 
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/ipc/ipc.c#1 $
 * 
 ****************************************************************************/

#include "ipc/ipc_private.h"

void ipc_disconnect(ipcConnection* con)
{
    if (con && con->disconnectFn)
    {
        (*con->disconnectFn)(con);
    }
}

CsrBool ipc_message_send(ipcConnection* con, const CsrUint8 *message, CsrUint32 length)
{
    if (con && con->messageSendFn)
    {
        return (*con->messageSendFn)(con, message, length);
    }
    return FALSE;
}

CsrBool ipc_message_send_3(ipcConnection* con, const CsrUint8 *message, CsrUint32 length,
                                               const CsrUint8 *data1, CsrUint32 data1length,
                                               const CsrUint8 *data2, CsrUint32 data2length)
{
    if (con && con->messageSend3Fn)
    {
        return (*con->messageSend3Fn)(con, message, length, data1, data1length, data2, data2length);
    }
    return FALSE;
}

CsrBool ipc_message_recv(ipcConnection* con, CsrUint32 timeout_ms, CsrUint8 **message, CsrUint32 *length)
{
    if (con && con->messageRecvFn)
    {
        return (*con->messageRecvFn)(con, timeout_ms, message, length);
    }
    return FALSE;
}

void ipc_wakeup(ipcConnection* con)
{
    if (con && con->wakeupFn)
    {
        (*con->wakeupFn)(con);
    }
}

void ipc_message_free(ipcConnection* con, CsrUint8 *message)
{
    if (con && con->messageFreeFn)
    {
        (*con->messageFreeFn)(con, message);
    }
}

CsrInt32 ipc_getFd(ipcConnection* con)
{
    if (con && con->getFd)
    {
        return (*con->getFd)(con);
    }
    return IPC_INVALID_FD;
}
