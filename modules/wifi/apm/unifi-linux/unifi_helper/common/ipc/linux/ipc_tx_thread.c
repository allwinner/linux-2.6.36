/** @file ipc_tx_thread.c
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
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/ipc/linux/ipc_tx_thread.c#5 $
 *
 ****************************************************************************/

#include <pthread.h>

#include "csr_pmalloc.h"
#include "csr_util.h"

#include "ipc/linux/ipc_tx_thread.h"

#include "ipc/ipc_private.h"
#include "sme_trace/sme_trace.h"
#include "csr_cstl/csr_wifi_list.h"



#ifdef IPC_TX_THREAD

typedef struct IpcMessage {
    csr_list_node listNode;

    CsrUint8 *message;
    CsrUint32 length;
} IpcMessage;

struct IpcThreadContext {
    pthread_t       messageThread;
    pthread_mutex_t messageMutex;
    pthread_cond_t  messageCond;
    pthread_cond_t  threadCompleteCond;

    pthread_mutex_t listMutex;


    ipcConnection* con;
    ipc_message_send_fn sendFn;
    csr_list messageList;

    CsrBool done;
};

static void* ipc_thread_main(IpcThreadContext* context);

IpcThreadContext* ipc_create_thread(ipcConnection* con, ipc_message_send_fn sendFn)
{
    IpcThreadContext* t = (IpcThreadContext*) CsrPmalloc(sizeof(IpcThreadContext));
    pthread_mutex_init( &t->listMutex, NULL);
    pthread_mutex_init( &t->messageMutex, NULL);
    pthread_cond_init( &t->messageCond, NULL);
    pthread_cond_init( &t->threadCompleteCond, NULL);

    t->con = con;
    t->sendFn = sendFn;

    csr_list_init(&t->messageList);
    t->done = FALSE;

    pthread_create(&(t->messageThread), NULL, (void*)&ipc_thread_main, (void*)t);

    return t;
}

void ipc_destroy_thread(IpcThreadContext* context)
{
    context->done = TRUE;

    /* Signal the Thread And wait for it to exit */
    pthread_mutex_lock(&context->messageMutex);
    pthread_cond_signal(&context->messageCond);
    pthread_cond_wait(&context->threadCompleteCond, &context->messageMutex);
    pthread_mutex_unlock(&context->messageMutex);

    pthread_cond_destroy(&context->messageCond);
    pthread_cond_destroy(&context->threadCompleteCond);
    pthread_mutex_destroy(&context->messageMutex);
    pthread_mutex_destroy(&context->listMutex);
    CsrPfree(context);
}


CsrBool ipc_thread_message_send_3(IpcThreadContext* context,
                                  const CsrUint8 *message, CsrUint32 length,
                                  const CsrUint8 *data1, CsrUint32 data1length,
                                  const CsrUint8 *data2, CsrUint32 data2length)
{
    IpcMessage* msg = (IpcMessage*)CsrPmalloc(sizeof(IpcMessage));

    msg->length = length + data1length + data2length;

    msg->message = (CsrUint8*)CsrPmalloc(msg->length);
    CsrMemCpy(msg->message, (CsrUint8*)message, length);
    CsrMemCpy(&msg->message[length], data1, data1length);
    CsrMemCpy(&msg->message[length + data1length], data2, data2length);

    /* Insert into List */
    pthread_mutex_lock(&context->listMutex);
    csr_list_insert_tail(&context->messageList, list_owns_value, &msg->listNode, msg);
    pthread_mutex_unlock(&context->listMutex);

    /* Signal the Thread */
    pthread_mutex_lock(&context->messageMutex);
    pthread_cond_signal(&context->messageCond);
    pthread_mutex_unlock(&context->messageMutex);

    return TRUE;
}

CsrBool ipc_thread_message_send(IpcThreadContext* context, const CsrUint8 *message, CsrUint32 length)
{
    return ipc_thread_message_send_3(context, message, length, NULL, 0 , NULL, 0);
}

static void* ipc_thread_main(IpcThreadContext* context)
{
    IpcMessage* msg;
    CsrUint8 *message;
    CsrUint32 length;

    while(!context->done)
    {
        /* Send all messages in the */
        pthread_mutex_lock(&context->listMutex);
        msg = csr_list_gethead_t(IpcMessage*, &context->messageList);

        while(msg != NULL)
        {
            length = msg->length;
            message = msg->message;
            csr_list_removehead(&context->messageList);
            pthread_mutex_unlock(&context->listMutex);

            if (!context->done)
            {
                /* send the message */
                context->sendFn(context->con, message, length);
            }
            CsrPfree(message);

            pthread_mutex_lock(&context->listMutex);
            msg = csr_list_gethead_t(IpcMessage*, &context->messageList);
        }

        /* wait for a message to send. */
        pthread_mutex_lock(&context->messageMutex);
        pthread_mutex_unlock(&context->listMutex); /* Ensure we hold the list mutex untill we have got hold of the message mutex, 
                                                      to avoid missing the messageCond signal. */
        if(!context->done) /* Exit if the done-flag was set while sending a message, or while the done-flag was evaluated*/
        {
            pthread_cond_wait(&context->messageCond, &context->messageMutex);
        }
        pthread_mutex_unlock(&context->messageMutex);
    }
    pthread_mutex_lock(&context->messageMutex);
    pthread_cond_signal(&context->threadCompleteCond);
    pthread_mutex_unlock(&context->messageMutex);

    return NULL;
}

#endif
