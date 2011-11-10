/** @file ipc_chardevice.c
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
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/ipc/linux/ipc_chardevice.c#1 $
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "csr_types.h"
#include "csr_pmalloc.h"
#include "csr_util.h"
#include "ipc/ipc_private.h"
#include "ipc/linux/ipc_chardevice.h"
#include "sme_trace/sme_trace.h"

#ifdef IPC_TX_THREAD
#include "ipc/linux/ipc_tx_thread.h"
#endif


/* ---------------------------------- */
/* So we can correctly size udi_msg_t */
/* ---------------------------------- */
#include "unifiio.h"
/* ---------------------------------- */

#ifdef IPC_CHARDEVICE

struct ipcConnectionImpl
{
    int fd;
#ifdef IPC_TX_THREAD
    IpcThreadContext* thread;
#endif

};

static void setFdCloseOnExec(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags == -1)
    {
        sme_trace_error((TR_IPC, "setFdCloseOnExec(): fcntl(fd %d, F_GETFD) failed: %s", fd, strerror(errno)));
        return;
    }

    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) == -1)
    {
        sme_trace_error((TR_IPC, "setFdCloseOnExec(): fcntl(fd %d, F_SETFD) failed: %s", fd, strerror(errno)));
        return;
    }
}


static void ipc_chardevice_disconnect(ipcConnection* con)
{
    sme_trace_entry((TR_IPC, "ipc_disconnect()"));
    if (con->impl == NULL) return;

    close(con->impl->fd);

#ifdef IPC_TX_THREAD
    ipc_destroy_thread(con->impl->thread);
#endif

    CsrPfree(con->impl);
    CsrPfree(con);
}

#ifndef IPC_TX_THREAD
static CsrBool ipc_chardevice_message_send_3(ipcConnection* con, const CsrUint8 *message, CsrUint32 length,
                                                                 const CsrUint8 *data1, CsrUint32 data1length,
                                                                 const CsrUint8 *data2, CsrUint32 data2length)
{
    CsrUint32 bufSize = length + data1length + data2length;
    CsrUint8* buf;
    sme_trace_entry((TR_IPC, "ipc_chardevice_message_send_3()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_chardevice_message_send_3(): Invalid connection"));
        return FALSE;
    }

    if ((message == NULL) || (length == 0))
    {
        sme_trace_debug((TR_IPC, "ipc_chardevice_message_send_3(): will not send empty message at %p, length %d", message, length));
        return FALSE;
    }

    buf = (CsrUint8*)CsrPmalloc(bufSize);

    CsrMemCpy(buf, message, length);
    CsrMemCpy(&buf[length], data1, data1length);
    CsrMemCpy(&buf[length + data1length], data2, data2length);
    sme_trace_hex((TR_IPC, TR_LVL_DEBUG, "ipc_chardevice_message_send_3():", buf, bufSize));

    if (write(con->impl->fd, buf, bufSize) <= 0)
    {
        sme_trace_error((TR_IPC, "ipc_chardevice_message_send_3(): write Failed %d, %s\n", errno, strerror(errno)));

        CsrPfree(buf);
        return FALSE;
    }

    CsrPfree(buf);
    return TRUE;
}
#endif

static CsrBool ipc_chardevice_message_send(ipcConnection* con, const CsrUint8 *message, CsrUint32 length)
{
    sme_trace_entry((TR_IPC, "ipc_chardevice_message_send()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_chardevice_message_send(): Invalid connection"));
        return FALSE;
    }

    if ((message == NULL) || (length == 0))
    {
        sme_trace_debug((TR_IPC, "ipc_chardevice_message_send(): will not send empty message at %p, length %d", message, length));
        return FALSE;
    }

    sme_trace_hex((TR_IPC, TR_LVL_DEBUG, "ipc_chardevice_message_send():", message, length));
    if (write(con->impl->fd, message, length) <= 0)
    {
        sme_trace_error((TR_IPC, "ipc_chardevice_message_send_3(): write Failed %d, %s\n", errno, strerror(errno)));
        return FALSE;
    }
    return TRUE;
}

static CsrBool ipc_chardevice_message_recv(ipcConnection* con, CsrUint32 timeout_ms, CsrUint8 **message, CsrUint32 *length)
{
    int r;
    *length = 0;

    sme_trace_entry((TR_IPC, "ipc_message_recv()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_message_recv(): Invalid connection"));
        return FALSE;
    }

    *length = 2048;
    *message = (CsrUint8*)CsrPmalloc(*length);

    if ((r = read(con->impl->fd, *message, *length)) <= 0)
    {
        sme_trace_error((TR_IPC, "ipc_message_recv(): read Failed %d, %d, %s\n", r, errno, strerror(errno)));
        return FALSE;
    }
    *length = ((CsrUint32)r) - sizeof(udi_msg_t);
    CsrMemMove(*message, &(*message)[sizeof(udi_msg_t)], *length);

    sme_trace_debug_code(if (*length) { sme_trace_hex((TR_IPC, TR_LVL_DEBUG, "ipc_message_recv():", *message, *length)); })

    return TRUE;
}

static void ipc_chardevice_message_free(ipcConnection* con, CsrUint8 *message)
{
    sme_trace_entry((TR_IPC, "ipc_message_free()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_message_free(): Invalid connection"));
    }

    CsrPfree(message);
}

static CsrInt32 ipc_chardevice_getFd(ipcConnection* con)
{
    sme_trace_entry((TR_IPC, "ipc_chardevice_getFd()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_chardevice_getFd(): Invalid connection"));
        return IPC_INVALID_FD;
    }

    return con->impl->fd;
}

#ifdef IPC_TX_THREAD
static CsrBool ipc_chardevice_message_send_3_thread(ipcConnection* con,
                                                    const CsrUint8 *message, CsrUint32 length,
                                                    const CsrUint8 *data1, CsrUint32 data1length,
                                                    const CsrUint8 *data2, CsrUint32 data2length)
{
    return ipc_thread_message_send_3(con->impl->thread, message, length, data1, data1length, data2, data2length);
}

static CsrBool ipc_chardevice_message_send_thread(ipcConnection* con, const CsrUint8 *message, CsrUint32 length)
{
    return ipc_thread_message_send(con->impl->thread, message, length);
}
#endif


ipcConnection* ipc_chardevice_connect(const char *devNode,
                                     void* externalContext,
                                     ipc_on_connect_fn connectCallback,
                                     ipc_on_receive_fn rxCallback,
                                     ipc_on_disconnect_fn disconCallback)
{
    ipcConnection* con = NULL;
    int flags;
    int fd = open(devNode, O_RDWR);

    sme_trace_entry((TR_IPC, "ipc_connect(%s)", devNode));

    if (fd < 0)
    {
        sme_trace_error((TR_IPC, "ipc_connect: open %s Failed : %s", devNode, strerror(errno)));
        return NULL;
    }

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
    {
        sme_trace_error((TR_IPC, "ipc_connect: fcntl fcntl(F_GETFD) failed\n"));
        close(fd);
        return NULL;
    }

    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) == -1)
    {
        sme_trace_error((TR_IPC, "ipc_connect: fcntl fcntl(F_SETFD) failed\n"));
        close(fd);
        return con;
    }

    setFdCloseOnExec(fd);

    con = (ipcConnection*)CsrPmalloc(sizeof(ipcConnection));

    con->disconnectFn    = ipc_chardevice_disconnect;
#ifndef IPC_TX_THREAD
    con->messageSendFn   = ipc_chardevice_message_send;
    con->messageSend3Fn  = ipc_chardevice_message_send_3;
#else
    con->messageSendFn   = ipc_chardevice_message_send_thread;
    con->messageSend3Fn  = ipc_chardevice_message_send_3_thread;
#endif
    con->messageRecvFn   = ipc_chardevice_message_recv;
    con->wakeupFn        = NULL;
    con->messageFreeFn   = ipc_chardevice_message_free;
    con->getFd           = ipc_chardevice_getFd;
    con->connectCallback = connectCallback;
    con->rxCallback      = rxCallback;
    con->disconCallback  = disconCallback;
    con->impl = (ipcConnectionImpl*)CsrPmalloc(sizeof(ipcConnectionImpl));
    con->impl->fd = fd;

#ifdef IPC_TX_THREAD
    con->impl->thread = ipc_create_thread(con, ipc_chardevice_message_send);
#endif

    return con;
}
#endif
