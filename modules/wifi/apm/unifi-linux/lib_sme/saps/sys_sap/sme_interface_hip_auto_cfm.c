/** @file sme_interface_hip_auto_cfm.c
 *
 * Stores cfm's for use if comms with the driver is lost
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
 *   Stores auto cfm's for outstanding hip requests that are used to send to the
 *   sme in the event of a loss of comms with the driver / firmware
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/saps/sys_sap/sme_interface_hip_auto_cfm.c#1 $
 *
 ***************************************************************************/

/* STANDARD INCLUDES ********************************************************/


/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "fsm/fsm_types.h"
#include "fsm/fsm_private.h"

#include "sys_sap/sme_interface_hip_auto_cfm.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

typedef struct
{
    csr_list_node listNode;

    CsrUint32 messageTimeStamp;

    FsmEvent* message;

} message_list_node;

/**
 * type definition of the empty autocfm queue callback function
 */
typedef void (*zeroCfmfuncPtr)(FsmContext* context);

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/
struct HipAutoCfm
{
    FsmContext* fsmContext;

    /** List of events */
    csr_list cfmEvents;

    /** Indicates if auto cfm's should be used */
    CsrBool sendCfmEvents;

    /* zero cfms call back function */
    CsrBool SingleShot;
    zeroCfmfuncPtr zeroCfmfuncPtr;
};

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in dd_sap/sme_interface_hip_auto_cfm.h
 */
HipAutoCfm* hip_auto_cfm_init(FsmContext* fsmContext)
{
    HipAutoCfm* context = (HipAutoCfm*)CsrPmalloc(sizeof(HipAutoCfm));

    sme_trace_entry((TR_SYS_SAP, "hip_auto_cfm_init()"));
    require(TR_SYS_SAP, fsmContext != NULL);

    context->fsmContext = fsmContext;
    context->sendCfmEvents = TRUE;
    context->SingleShot= TRUE;
    context->zeroCfmfuncPtr = NULL;
    csr_list_init(&context->cfmEvents);
    return context;
}

/**
 * See description in dd_sap/sme_interface_hip_auto_cfm.h
 */
void hip_auto_cfm_shutdown(HipAutoCfm* context)
{
    message_list_node* listNode;
    require(TR_SYS_SAP, context != NULL);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->fsmContext->externalEventQueueLock);
#endif
    sme_trace_entry((TR_SYS_SAP, "hip_auto_cfm_shutdown()"));

    listNode = csr_list_gethead_t(message_list_node*, &context->cfmEvents);
    while (NULL != listNode)
    {
        CsrPfree(listNode->message);
        csr_list_removehead(&context->cfmEvents);
        listNode = csr_list_gethead_t(message_list_node *, &context->cfmEvents);
    }
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif

    CsrPfree(context);
}

/**
 * See description in dd_sap/sme_interface_hip_auto_cfm.h
 */
void hip_auto_cfm_add(HipAutoCfm* context, FsmEvent* cfm)
{
    sme_trace_entry((TR_SYS_SAP, "hip_auto_cfm_add(0x%.4X)", cfm->id));
    require(TR_SYS_SAP, context != NULL);
    require(TR_SYS_SAP, cfm != NULL);

    if (context->sendCfmEvents == FALSE)
    {
        message_list_node* listnode = CsrPmalloc(sizeof(message_list_node));
        listnode->message = cfm;

#ifdef FSM_MUTEX_ENABLE
        (void)CsrMutexLock(context->fsmContext->externalEventQueueLock);
#endif
        listnode->messageTimeStamp = fsm_get_time_of_day_ms(context->fsmContext);
        csr_list_insert_tail(&context->cfmEvents, list_owns_value, &listnode->listNode, listnode);
#ifdef FSM_MUTEX_ENABLE
        (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif
    }
    else
    {
        fsm_send_event_external(context->fsmContext, cfm, cfm->destination, cfm->id);
    }
}

/**
 * See description in dd_sap/sme_interface_hip_auto_cfm.h
 */
CsrBool hip_auto_cfm_remove_matching_cfm(HipAutoCfm* context, CsrUint16 id, CsrBool optional)
{
    message_list_node* listNode;

    sme_trace_entry((TR_SYS_SAP, "hip_auto_cfm_remove_matching_cfm(0x%.4X)", id));
    require(TR_SYS_SAP, context != NULL);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->fsmContext->externalEventQueueLock);
#endif
    listNode = csr_list_gethead_t(message_list_node *, &context->cfmEvents);
    while (listNode != NULL)
    {
        if (listNode->message->id == id)
        {
            CsrPfree(listNode->message);
            (void)csr_list_remove(&context->cfmEvents, &listNode->listNode);
#ifdef FSM_MUTEX_ENABLE
            (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif

            /* see if a function has bee registered */
            if(context->zeroCfmfuncPtr != NULL)
            {
                /* only if the list is now empty */
                if(csr_list_isempty(&context->cfmEvents))
                {
                    context->zeroCfmfuncPtr(context->fsmContext);

                    /* only one function call to be made? */
                    if(context->SingleShot)
                    {
                        context->zeroCfmfuncPtr = NULL;
                    }
                }
            }
            return TRUE;
        }
        listNode = csr_list_getnext_t(message_list_node *, &context->cfmEvents, &listNode->listNode);
    }

    if (!optional)
    {
        sme_trace_warn((TR_SYS_SAP, "hip_auto_cfm_remove_matching_cfm(0x%.4X) Unmatched message", id));
        verify(TR_SYS_SAP, context->sendCfmEvents == TRUE);
#ifdef FSM_MUTEX_ENABLE
        (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif
        return FALSE;
    }
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif
    return TRUE;
}

/**
 * See description in dd_sap/sme_interface_hip_auto_cfm.h
 */
CsrBool hip_auto_cfm_has_matching_cfm(HipAutoCfm* context, CsrUint16 id)
{
    message_list_node* listNode;

    sme_trace_entry((TR_SYS_SAP, "hip_auto_cfm_has_matching_cfm(0x%.4X)", id));
    require(TR_SYS_SAP, context != NULL);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->fsmContext->externalEventQueueLock);
#endif
    listNode = csr_list_gethead_t(message_list_node *, &context->cfmEvents);
    while (listNode != NULL)
    {
        if (listNode->message->id == id)
        {
#ifdef FSM_MUTEX_ENABLE
            (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif
            return TRUE;
        }
        listNode = csr_list_getnext_t(message_list_node *, &cfmEvents, &listNode->listNode);
    }

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif
    return FALSE;
}

/**
 * See description in dd_sap/sme_interface_hip_auto_cfm.h
 */
void hip_auto_cfm_send_events(HipAutoCfm* context)
{
    message_list_node* listNode;

    sme_trace_entry((TR_SYS_SAP, "hip_auto_cfm_send_events()"));
    require(TR_SYS_SAP, context != NULL);
    context->sendCfmEvents = TRUE;

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->fsmContext->externalEventQueueLock);
#endif
    listNode = csr_list_gethead_t(message_list_node*, &context->cfmEvents);
    while (NULL != listNode)
    {
        FsmEvent* event = listNode->message;
        sme_trace_info((TR_SYS_SAP, "hip_auto_cfm_send_events(Remove %p :: ID 0x%.4X)", listNode, listNode->message->id));
        csr_list_removehead(&context->cfmEvents);
#ifdef FSM_MUTEX_ENABLE
        (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif
        fsm_send_event_external(context->fsmContext, event, event->destination, event->id);

#ifdef FSM_MUTEX_ENABLE
        (void)CsrMutexLock(context->fsmContext->externalEventQueueLock);
#endif
        listNode = csr_list_gethead_t(message_list_node *, &context->cfmEvents);
    }
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif
}

/**
 * See description in dd_sap/sme_interface_hip_auto_cfm.h
 */
CsrBool hip_auto_cfm_message_timedout(HipAutoCfm* context, CsrUint16 timeoutMs)
{
    message_list_node* listNode;
    sme_trace_entry((TR_SYS_SAP, "hip_auto_cfm_message_timedout()"));
    require(TR_SYS_SAP, context != NULL);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->fsmContext->externalEventQueueLock);
#endif
    listNode = csr_list_gethead_t(message_list_node*, &context->cfmEvents);
    if (NULL != listNode && CsrTimeLe(listNode->messageTimeStamp, CsrTimeSub(fsm_get_time_of_day_ms(context->fsmContext), timeoutMs)))
    {
        sme_trace_crit((TR_SYS_SAP, "hip_auto_cfm_message_timedout() :: HIP Message(0x%.4X) timedout", listNode->message->id));
#ifdef FSM_MUTEX_ENABLE
        (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif
        return TRUE;
    }
#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->fsmContext->externalEventQueueLock);
#endif
    return FALSE;
}



/**
 * See description in dd_sap/sme_interface_hip_auto_cfm.h
 */
void hip_auto_cfm_store_events(HipAutoCfm* context)
{
    sme_trace_entry((TR_SYS_SAP, "hip_auto_cfm_store_events()"));
    require(TR_SYS_SAP, context != NULL);

    context->sendCfmEvents = FALSE;
}

/**
 * See description in dd_sap/sme_interface_hip_auto_cfm.h
 */
CsrBool hip_auto_cfm_is_hip_sap_open(HipAutoCfm* context)
{
    sme_trace_entry((TR_SYS_SAP, "hip_auto_cfm_is_hip_sap_open()"));
    require(TR_SYS_SAP, context != NULL);

    return !context->sendCfmEvents;
}


void hip_auto_cfm_register_empty_queue_callback(HipAutoCfm* context, CsrBool singleShot, void (*funcPtr)(FsmContext* context))
{
    context->SingleShot = singleShot;
    context->zeroCfmfuncPtr = funcPtr;
}
