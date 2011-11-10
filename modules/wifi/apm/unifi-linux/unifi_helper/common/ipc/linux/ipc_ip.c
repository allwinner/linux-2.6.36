/** @file ipc_ip.c
 *
 *  Ip ipc implementation
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
 *   This file provides socket-related OS functionality expected by the IPC
 *   abstraction layer.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/ipc/linux/ipc_ip.c#2 $
 *
 ****************************************************************************/

/** @ingroup abstractions
 * @{
 */

/* STANDARD INCLUDES ********************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <strings.h>


/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"
#include "csr_pmalloc.h"
#include "csr_util.h"
#include "sme_trace/sme_trace.h"
#include "ipc/ipc_private.h"
#include "ipc/linux/ipc_ip.h"

#ifdef IPC_TX_THREAD
#include "ipc/linux/ipc_tx_thread.h"
#endif


#ifdef IPC_IP

/* MACROS *******************************************************************/
#define MAGIC_SIGNAL_START 0x13572468

#define MAX_SUPPORTED_MESSAGE_LENGTH (10 * 1024)

/**
 * This macro is used internally to the private serviceDataSocket function.
 * All signals sent over IPC have a header that contains a magic marker and
 * a length. This macro gives the size of that header.
 */
#define SIZEOF_MAGIC_AND_LENGTH (sizeof(CsrUint32) * 2)

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/
typedef struct ipc_header
{
    CsrUint32 magic;           /** filled in on transmit */
    CsrUint32 payload_len;     /** length of payload in bytes */
} ipc_header;

struct ipcConnectionImpl
{

    int     listeningFd;
    int     dataFd;
#ifdef IPC_TX_THREAD
    IpcThreadContext* thread;
#endif
};

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/
static int     ipc_ip_accept_connection(int fd);
static void    setFdCloseOnExec(int fd);

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Accepts a remote connection request, creates data path.
 *
 * @param[in]     fd : the descriptor for the listening socket
 *
 * @return
 *      int: the descriptor for the data socket created by the accept
 */
static int ipc_ip_accept_connection(int fd)
{
    int                 dataFd;
    struct sockaddr_in  clientAddress;
    socklen_t           clientAddressLength = sizeof(struct sockaddr_in);
    int                 on = 1;

    dataFd = accept(fd, (struct sockaddr *) &clientAddress, &clientAddressLength);
    if (dataFd < 0)
    {
        sme_trace_error((TR_IPC, "%s: accept(fd:%d) failed: %s", __FUNCTION__, fd, strerror(errno)));
        return IPC_INVALID_FD;
    }

    sme_trace_info((TR_IPC, "%s: accepted connection from : %d.%d.%d.%d",
            __FUNCTION__,
            (clientAddress.sin_addr.s_addr      ) & 0xff,
            (clientAddress.sin_addr.s_addr >>  8) & 0xff,
            (clientAddress.sin_addr.s_addr >> 16) & 0xff,
            (clientAddress.sin_addr.s_addr >> 24) & 0xff
            ));

    if (setsockopt(dataFd, IPPROTO_TCP,  TCP_NODELAY, (char *)&on, sizeof(on)) < 0)
    {
        sme_trace_error((TR_IPC, "%s: setsockopt(TCP_NODELAY) failed: %s", __FUNCTION__, strerror(errno)));
    }
    return dataFd;
}

/**
 * @brief
 *   Sets close-on-exec flag to hide a socket from any spawned child process
 *
 * @par Description:
 *   In order to protect the sockets formed during connection or queue-
 *   creation, a mechanism for enforcing a 'close-on-exec' policy is
 *   provided by this function.
 *
 * @par
 *   By closing sockets across any exec calls, the child process is denied
 *   access to them.
 *
 * @param[in]     fd : the descriptor to close across any exec calls
 *
 * @return
 *   void
 */
static void setFdCloseOnExec(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
    {
        sme_trace_error((TR_IPC, "%s: fcntl(fd %d, F_GETFD) failed: %s", __FUNCTION__, fd, strerror(errno)));
        return;
    }

    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) == -1)
    {
        sme_trace_error((TR_IPC, "%s: fcntl(fd %d, F_SETFD) failed: %s", __FUNCTION__, fd, strerror(errno)));
        return;
    }
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

static void ipc_ip_create_listen_socket(int listeningPort, ipcConnectionImpl* cSetup)
{
    struct sockaddr_in serv_addr;
    int on = 1;
    int result;

    sme_trace_entry((TR_IPC, ">> %s: IPC listening port is %d", __FUNCTION__, listeningPort));

    if (!cSetup)
    {
        sme_trace_error((TR_IPC, "%s: no ipcConnection structure passed in", __FUNCTION__));
        return;
    }

    /* Create the listening socket */
    if ((cSetup->listeningFd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        sme_trace_error((TR_IPC, "%s: socket() failed: %s", __FUNCTION__, strerror(errno)));
        return;
    }

    /* Do not pass socket to exec'd subprocesses */
    setFdCloseOnExec(cSetup->listeningFd);

    /* Allow socket descriptor to be reuseable */
    result = setsockopt(cSetup->listeningFd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on));
    if (result < 0)
    {
        sme_trace_error((TR_IPC, "%s: setsockopt() failed: %s", __FUNCTION__, strerror(errno)));
        return;
    }

    /* Set socket to be non-blocking. All of the sockets for the incomming
     * connections will also be non-blocking since they will inherit that
     * state from the listening socket.
     */
    result = ioctl(cSetup->listeningFd, FIONBIO, (char *)&on);
    if (result < 0)
    {
        sme_trace_error((TR_IPC, "%s: ioctl(FIONBIO) on listeningFd failed: %s", __FUNCTION__, strerror(errno)));
        return;
    }
    CsrMemSet((char *) &serv_addr, 0, sizeof(serv_addr));

    /*
     * bind the listening socket to the listening port for all available
     * interfaces.
     *
     * NOTE: this is perhaps a little ill-advised as we really don't want
     * to accept any connections from the network over the air-interface
     */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((CsrUint16) listeningPort);

    if (bind(cSetup->listeningFd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
    {
        close(cSetup->listeningFd);
        cSetup->listeningFd = IPC_INVALID_FD;
        sme_trace_error((TR_IPC, "%s: bind() failed: %s", __FUNCTION__,
                         strerror(errno)));
        return;
    }

    /* Allow connections to be accepted via this socket */
    if (listen(cSetup->listeningFd, 1) < 0)
    {
        sme_trace_error((TR_IPC, "%s: listen() failed: %s", __FUNCTION__, strerror(errno)));
        return;
    }
}

static int ipc_ip_connect_data_socket(const char *hostname, int portno, ipcConnectionImpl* cSetup)
{
    struct hostent *hostInfo;
    struct sockaddr_in serv_addr;
    int result;
    int on = 1;

    sme_trace_info((TR_IPC, "ipc_ip_connect_data_socket(): looking up host '%s'...", hostname));
    if ((hostInfo = gethostbyname(hostname)) == NULL)
    {
        sme_trace_error((TR_IPC, "ipc_ip_connect_data_socket(): gethostbyname failed: %s.", hstrerror(h_errno)));
        return(-1);
    }

    cSetup->dataFd = socket(PF_INET, SOCK_STREAM, 0);

    if (cSetup->dataFd < 0)
    {
        sme_trace_error((TR_IPC, "ipc_ip_connect_data_socket(): socket() failed: (%d) %s", errno, strerror(errno))); /*lint !e564 */
        return(-1);
    }

    CsrMemSet((char *) &serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr =
            (hostInfo->h_addr_list[0][0] <<  0) |
            (hostInfo->h_addr_list[0][1] <<  8) |
            (hostInfo->h_addr_list[0][2] << 16) |
            (hostInfo->h_addr_list[0][3] << 24);
    serv_addr.sin_port = htons((CsrUint16) portno);

    sme_trace_info((TR_IPC, "ipc_ip_connect_data_socket(): connecting to %s (%#x) port %d...",
                    hostInfo->h_name, serv_addr.sin_addr.s_addr, portno));

    result = connect(cSetup->dataFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (result < 0)
    {
        if (errno != EINPROGRESS)
        {
            sme_trace_error((TR_IPC, "ipc_ip_connect_data_socket(): connect failed: %s", strerror(errno)));
            close(cSetup->dataFd);
            cSetup->dataFd = IPC_INVALID_FD;
            return(-1);
        }
    }

    if (setsockopt(cSetup->dataFd, IPPROTO_TCP,  TCP_NODELAY, (char *)&on, sizeof(on)) < 0)
    {
        sme_trace_error((TR_IPC, "%s: setsockopt(TCP_NODELAY) failed: %s", __FUNCTION__, strerror(errno)));
    }

    return(cSetup->dataFd);
}

/* --------------------------------------------------------------------------
 * Description:
 *   See description in abstractions/linux/sdl_comms.h
 *
 *   Attempts to read the specified amount of data from the specified socket
 */
int ipc_ip_read(int dataFd, CsrUint8 *buffer, CsrUint32 length)
{
    CsrUint32  totalRead = 0;
    int     thisReadLength;
    int     result;

    sme_trace_entry((TR_IPC, ">> ipc_ip_read(): on descriptor %d, %d bytes, buffer at %p", dataFd, length, buffer));

    if (dataFd < 0)
    {
        errno = ENOTCONN;
        sme_trace_error((TR_IPC, "ipc_ip_read(): bad descriptor (%d)",dataFd));
        return(-1);
    }

    /* recv may return with less than the amount of data we requested so
     * keep trying until we get what we wanted (adjusting the request length
     * accordingly) */
    while (totalRead < length)
    {
        sme_trace_debug((TR_IPC, "ipc_ip_read(): attempting to read %d bytes on descriptor %d", length - totalRead, dataFd));
        thisReadLength = recv(dataFd, &buffer[totalRead], length - totalRead, 0);
        if (thisReadLength < 0)
        {
            sme_trace_error((TR_IPC, "ipc_ip_read(): read failed with %d/%d bytes read: %s (%d)",
                             totalRead, length, strerror(errno), errno)); /*lint !e564 */
            if (errno != EINTR)
            {
                return(thisReadLength);
            }
        }

        if (thisReadLength == 0)
        {
            sme_trace_error((TR_IPC, "ipc_ip_read(): zero byte read with %d/%d bytes read: %s (%d)",
                             totalRead, length, strerror(errno), errno)); /*lint !e564 */

            /* The socket has closed */
            result = shutdown(dataFd, SHUT_RDWR);
            if (result < 0)
            {
                sme_trace_error((TR_IPC, "ipc_ip_read(): shutdown failed: %s", strerror(errno)));
            }
            close(dataFd);
            sme_trace_debug((TR_IPC, "ipc_ip_read(): data socket fd %d closed", dataFd));
            errno = ENOTCONN;
            return(-1);
        }

        if (thisReadLength > 0)
        {
            totalRead += (unsigned long) thisReadLength;
        }
    }

    sme_trace_entry((TR_IPC, "<< ipc_ip_read(): totalRead indicates %d bytes (%d requested)", totalRead, length));

    return((int) totalRead);
}

static int ipc_message_tx(ipcConnectionImpl* cSetup, const CsrUint8 *buffer, CsrUint32 size)
{
    int  totalWritten = 0;
    int  thisWriteLength;
    fd_set  writeFdSet;

    sme_trace_entry((TR_IPC, "ipc_message_tx(): buffer at %p, size %ld.", buffer, size));

    if (cSetup->dataFd < 0)
    {
        sme_trace_info((TR_IPC, "ipc_message_tx(): Rejecting IPC output because the data socket is closed."));
        return(-1);
    }

    /* write to the socket, coping with a broken write */
    while (totalWritten < size) /*lint !e737 !e574*/
    {
        FD_ZERO(&writeFdSet);
        FD_SET((unsigned int) cSetup->dataFd, &writeFdSet);

        sme_trace_debug((TR_IPC, "ipc_message_tx(): attempting to write %d bytes on descriptor %d", size - totalWritten, cSetup->dataFd)); /*lint !e737*/
        thisWriteLength = send(cSetup->dataFd, &buffer[totalWritten], size - totalWritten, 0); /*lint !e737*/

        if (thisWriteLength < 0)
        {
            sme_trace_error((TR_IPC, "ipc_message_tx(): write() failed: %s", strerror(errno)));
            if ((errno != EAGAIN) && (errno != EINTR))
            {
                return(-1);
            }
        }
        else
        {
            totalWritten += thisWriteLength;
        }
    }
    return totalWritten;
}

static int ipc_message_rx(ipcConnectionImpl* cSetup, CsrUint8 *buf, CsrUint32 length)
{
    CsrUint8 *buffer=buf;

    sme_trace_entry((TR_IPC, "ipc_message_rx(): buffer at %p, size %ld.", buffer, length));

    if (cSetup->dataFd != IPC_INVALID_FD)
    {
        int result = result = ipc_ip_read(cSetup->dataFd, buffer, length);
        if (result < 0)
        {
            if (errno == ENOTCONN)
            {
                sme_trace_error((TR_IPC, "ipc_message_rx: ipc_ip_read failed with ENOTCONN; clearing dataFd %d", cSetup->dataFd));
                cSetup->dataFd = IPC_INVALID_FD;
            }
            else
            {
                sme_trace_error((TR_IPC, "ipc_message_rx: ipc_ip_read failed: %s", strerror(errno)));
            }
        }
        return result;
    }

    if (cSetup->listeningFd != IPC_INVALID_FD)
    {
        cSetup->dataFd = ipc_ip_accept_connection(cSetup->listeningFd);
        sme_trace_info((TR_IPC, "ipc_message_rx(): Connect"));
    }

    return 0;
}

/**
 *   See description in ipc/ipc.h
 */
static void ipc_ip_disconnect(ipcConnection* con)
{
    sme_trace_entry((TR_IPC, "ipc_disconnect()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_disconnect(): Invalid connection"));
        return;
    }

    sme_trace_warn((TR_IPC, "%s: closing dataFd %d and listeningFd %d if applicable",
                __FUNCTION__, con->impl->dataFd, con->impl->listeningFd));

    if (con->impl->dataFd != IPC_INVALID_FD)
    {
        (void) shutdown(con->impl->dataFd, SHUT_RDWR);
        (void) close(con->impl->dataFd);
        con->impl->dataFd = IPC_INVALID_FD;
    }

    if (con->impl->listeningFd != IPC_INVALID_FD)
    {
        (void) shutdown(con->impl->listeningFd, SHUT_RDWR);
        (void) close(con->impl->listeningFd);
        con->impl->listeningFd = IPC_INVALID_FD;
    }

#ifdef IPC_TX_THREAD
    ipc_destroy_thread(con->impl->thread);
#endif

    CsrPfree(con->impl);
    CsrPfree(con);
}


/**
 *   See description in ipc/ipc.h
 */
static CsrBool ipc_ip_message_send_3(ipcConnection* con, const CsrUint8 *message, CsrUint32 length,
                                            const CsrUint8 *data1, CsrUint32 data1length,
                                            const CsrUint8 *data2, CsrUint32 data2length)
{
    ipc_header ipcHeader;

    sme_trace_entry((TR_IPC, "ipc_message_send()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_message_send(): Invalid connection"));
        return FALSE;
    }

    if ((message == NULL) || (length == 0))
    {
        sme_trace_debug((TR_IPC, "ipc_message_send(): will not send empty message at %p, length %d", message, length));
        return FALSE;
    }

    /* Fill in field used to recognise start of signal synchronisation */
    ipcHeader.magic = MAGIC_SIGNAL_START;
    ipcHeader.payload_len = length + data1length + data2length;

    /* Write out the IPC header, followed by the payload */
    if ((ipc_message_tx(con->impl, (CsrUint8 *) &ipcHeader, sizeof(ipcHeader))) < 0)
    {
        return FALSE;
    }

    sme_trace_debug((TR_IPC, "ipc_message_send(): ipc_message_tx (payload) from %p, length %ld", message, length));
    sme_trace_hex((TR_IPC, TR_LVL_DEBUG, "sdl_ipc_message_send(): Signal", message, length));
    if ((ipc_message_tx(con->impl, message, length)) < 0)
    {
        return FALSE;
    }

    if (data1length != 0 && data1 != NULL)
    {
        sme_trace_hex((TR_IPC, TR_LVL_DEBUG, "sdl_ipc_message_send(): Data1", data1, data1length));
        if ((ipc_message_tx(con->impl, data1, data1length)) < 0)
        {
            return FALSE;
        }
    }

    if (data2length != 0 && data2 != NULL)
    {
        sme_trace_hex((TR_IPC, TR_LVL_DEBUG, "sdl_ipc_message_send(): data2", data2, data2length));
        if ((ipc_message_tx(con->impl, data2, data2length)) < 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static CsrBool ipc_ip_message_send(ipcConnection* con, const CsrUint8 *message, CsrUint32 length)
{
    return ipc_ip_message_send_3(con, message, length, NULL, 0, NULL, 0);
}

#ifdef IPC_TX_THREAD
static CsrBool ipc_ip_message_send_3_thread(ipcConnection* con,
                                            const CsrUint8 *message, CsrUint32 length,
                                            const CsrUint8 *data1, CsrUint32 data1length,
                                            const CsrUint8 *data2, CsrUint32 data2length)
{
    return ipc_thread_message_send_3(con->impl->thread, message, length, data1, data1length, data2, data2length);
}

static CsrBool ipc_ip_message_send_thread(ipcConnection* con, const CsrUint8 *message, CsrUint32 length)
{
    return ipc_thread_message_send(con->impl->thread, message, length);
}
#endif


/**
 *   See description in ipc/ipc.h
 */
static CsrBool ipc_ip_message_recv(ipcConnection* con, CsrUint32 timeout_ms,
                      CsrUint8 **message, CsrUint32 *length)
{
    int result;
    ipc_header ipcHeader;
    CsrUint8 *ipc_message;

    sme_trace_entry((TR_IPC, "ipc_message_recv()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_message_recv(): Invalid connection"));
        return FALSE;
    }

    /* Be pessimistic. */
    *message = NULL;
    *length = 0;

    /* select thinks there is a message to be received
     * Read the IPC header in first. */
    sme_trace_entry((TR_IPC, "ipc_message_recv(): about to read in header..."));
    if (ipc_message_rx(con->impl, (CsrUint8 *) &ipcHeader, sizeof(ipc_header))
                                                    < (int) sizeof(ipc_header))
    {
        sme_trace_error((TR_IPC, "ipc_message_recv(): failed while trying to retrieve IPC message header"));
        return FALSE;
    }

    sme_trace_entry((TR_IPC, "ipc_message_recv(): read in header."));
    if (ipcHeader.magic != MAGIC_SIGNAL_START)
    {
        sme_trace_error((TR_IPC, "ipc_message_recv(): IPC message header magic: expected %#x, read %#lx", MAGIC_SIGNAL_START, ipcHeader.magic));
        return FALSE;
    }

    if (ipcHeader.payload_len > MAX_SUPPORTED_MESSAGE_LENGTH)
    {
        sme_trace_error((TR_IPC, "ipc_message_recv(): IPC message header length: max %#x, read %#lx",
                                MAX_SUPPORTED_MESSAGE_LENGTH, ipcHeader.payload_len));
        return FALSE;
    }

    sme_trace_entry((TR_IPC, "ipc_message_recv(): magic %#x, payload_len %#x",
                             ipcHeader.magic, ipcHeader.payload_len));

    /* We are assuming the header is correct, so allocate a receive buffer
     * and read in the message.
     */
    ipc_message = (CsrUint8 *) CsrPmalloc(ipcHeader.payload_len);

    sme_trace_entry((TR_IPC, "ipc_message_recv(): before payload ipc_message_rx"));

    result = ipc_message_rx(con->impl, ipc_message, ipcHeader.payload_len);

    sme_trace_entry((TR_IPC, "ipc_message_recv(): after payload ipc_message_rx"));

    if (result < (int) ipcHeader.payload_len)
    {
        sme_trace_debug((TR_IPC, "ipc_message_recv(): retrieved only %d/%ld bytes of IPC message payload", result, ipcHeader.payload_len));
        CsrPfree(ipc_message);
        return FALSE;
    }

    *message = ipc_message;
    *length = ipcHeader.payload_len;

    sme_trace_entry((TR_IPC, "ipc_message_recv(): read message, length %d.",  ipcHeader.payload_len));

    sme_trace_hex((TR_IPC, TR_LVL_DEBUG, "ipc_message_recv(): payload", ipc_message, ipcHeader.payload_len));

    return TRUE;
}

/**
 *   See description in ipc/ipc.h
 */
static void ipc_ip_message_free(ipcConnection* con, CsrUint8 *message)
{
    sme_trace_entry((TR_IPC, "ipc_message_free()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_message_free(): Invalid connection"));
    }

    CsrPfree(message);
}

static CsrInt32 ipc_ip_getFd(ipcConnection* con)
{
    sme_trace_entry((TR_IPC, "ipc_ip_getFd()"));
    if (!con || !con->impl)
    {
        sme_trace_error((TR_IPC, "ipc_ip_getFd(): Invalid connection"));
        return IPC_INVALID_FD;
    }

    if (con->impl->dataFd != IPC_INVALID_FD)
    {
        return con->impl->dataFd;
    }
    return con->impl->listeningFd;
}

/**
 *   See description in ipc/ipc.h
 */
ipcConnection* ipc_ip_connect(const char *channel,
                              void* externalContext,
                              ipc_on_connect_fn connectCallback,
                              ipc_on_receive_fn rxCallback,
                              ipc_on_disconnect_fn disconCallback)
{
#define MAX_HOSTNAME_LENGTH 256
#define CONNECTION_TYPE_LENGTH 4

    ipcConnection* con = NULL;
    char channel_hostname[MAX_HOSTNAME_LENGTH];
    int portNumber;
    char *colon1;
    char *colon2;

    sme_trace_entry((TR_IPC, "ipc_connect(%s)", channel));

    errno = 0;

    if (CsrStrLen(channel) > MAX_HOSTNAME_LENGTH)
    {
        sme_trace_error((TR_IPC, "ipc_connect(): hostname length in '%s' too long for comfort", channel));
        errno = EINVAL;
        return NULL;
    }

    colon1 = index(channel, ':');
    colon2 = (char*)strrchr(channel, ':');
    if ((colon1 == NULL) || (colon2 == NULL))
    {
        sme_trace_error((TR_IPC, "ipc_connect(): failed to parse"));
        errno = EINVAL;
        return NULL;
    }

    if (CsrStrNCmp(channel, "tcp", (CsrUint32) colon1 - (CsrUint32) channel) != 0)
    {
        sme_trace_error_code(char connectionType[CONNECTION_TYPE_LENGTH]);
        sme_trace_error((TR_IPC, "ipc_connect(): unsupported connection type '%s'", connectionType));
        errno = EINVAL;
        return NULL;
    }

    CsrMemSet(channel_hostname, 0, sizeof(channel_hostname));
    CsrMemCpy(channel_hostname, &colon1[1], ((CsrUint32) colon2 - (CsrUint32) colon1) - 1);

    (void) sscanf(&colon2[1], "%d", &portNumber);

    sme_trace_info((TR_IPC, " -----------------------------------------------------------"));
    sme_trace_info((TR_IPC, "| Attempting to connect to %s port %d", channel_hostname, portNumber));
    sme_trace_info((TR_IPC, " -----------------------------------------------------------"));


    con = (ipcConnection*)CsrPmalloc(sizeof(ipcConnection));
    con->disconnectFn    = ipc_ip_disconnect;
    con->messageSendFn   = ipc_ip_message_send;
    con->messageSend3Fn  = ipc_ip_message_send_3;
    con->messageRecvFn   = ipc_ip_message_recv;
    con->wakeupFn        = NULL;
    con->messageFreeFn   = ipc_ip_message_free;
    con->getFd           = ipc_ip_getFd;
    con->connectCallback = connectCallback;
    con->rxCallback      = rxCallback;
    con->disconCallback  = disconCallback;

    con->impl = (ipcConnectionImpl*)CsrPmalloc(sizeof(ipcConnectionImpl));
    con->impl->listeningFd  = IPC_INVALID_FD;
    con->impl->dataFd       = IPC_INVALID_FD;
    if (ipc_ip_connect_data_socket(channel_hostname, portNumber, con->impl) < 0)
    {
        CsrPfree(con->impl);
        CsrPfree(con);
        con = NULL;
    }
    return con;
}

/**
 *   See description in ipc/ipc.h
 */
ipcConnection* ipc_ip_create(int portNumber,
                             void* externalContext,
                             ipc_on_connect_fn connectCallback,
                             ipc_on_receive_fn rxCallback,
                             ipc_on_disconnect_fn disconCallback)
{
    ipcConnection* con = (ipcConnection*)CsrPmalloc(sizeof(ipcConnection));

    sme_trace_entry((TR_IPC, "ipc_listen(%d)", portNumber));

    con->disconnectFn    = ipc_ip_disconnect;
#ifndef IPC_TX_THREAD
    con->messageSendFn   = ipc_ip_message_send;
    con->messageSend3Fn  = ipc_ip_message_send_3;
#else
    con->messageSendFn   = ipc_ip_message_send_thread;
    con->messageSend3Fn  = ipc_ip_message_send_3_thread;
#endif
    con->messageRecvFn   = ipc_ip_message_recv;
    con->wakeupFn        = NULL;
    con->messageFreeFn   = ipc_ip_message_free;
    con->getFd           = ipc_ip_getFd;
    con->connectCallback = connectCallback;
    con->rxCallback      = rxCallback;
    con->disconCallback  = disconCallback;
    con->impl = (ipcConnectionImpl*)CsrPmalloc(sizeof(ipcConnectionImpl));
    con->impl->listeningFd  = IPC_INVALID_FD;
    con->impl->dataFd       = IPC_INVALID_FD;
    ipc_ip_create_listen_socket(portNumber, con->impl);
    if (con->impl->listeningFd < 0)
    {
        CsrPfree(con->impl);
        CsrPfree(con);
        con = NULL;
    } else {
#ifdef IPC_TX_THREAD
    con->impl->thread = ipc_create_thread(con, ipc_ip_message_send);
#endif
    }
    return con;
}
#endif
/** @}
 */
