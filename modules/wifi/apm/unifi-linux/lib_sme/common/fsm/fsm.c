/** @file fsm.c
 *
 * Public FSM code
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
 *   FSM implementation for the public FSM code
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/fsm/fsm.c#3 $
 *
 ****************************************************************************/

/** @{
 * @ingroup fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "fsm/fsm_types.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_internal.h"
#include "fsm/fsm_private.h"
#include "fsm/fsm_debug.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/
#ifdef FSM_DEBUG
#define process_name(instance) ((instance)?instance->fsmInfo->processName:"NULL")
#define current_state_name(instance) ((instance)?(instance->state!=FSM_TERMINATE?instance->fsmInfo->transitionTable.aStateEventMatrix[instance->state].stateName:"FSM_TERMINATE"):"NULL")
#else
#define process_name(instance) ((instance)?"UNKNOWN":"NULL")
#define current_state_name(instance) ((instance)?(instance->state!=FSM_TERMINATE?"UNKNOWN":"FSM_TERMINATE"):"NULL")
#endif

#define process_name_by_id(context,id) ((id == FSM_ENV)?"ENV":process_name((&context->instanceArray[id])))


static void cleanup_fsm(FsmContext* context, FsmInstanceEntry* pInstance)
{
    FsmEvent* queuedEvent;

    sme_trace_info((TR_FSM, "cleanup_fsm() :: Cleaning up %s", process_name(pInstance)));

    /* Clean up SubFsm's (Recursive) */
    if (pInstance->subFsm != NULL)
    {
        cleanup_fsm(context, (FsmInstanceEntry*)pInstance->subFsm);
        CsrPfree(pInstance->subFsm);
    }

    /* Clean up the Saved Event Queue */
    queuedEvent = fsm_pop_event(&pInstance->savedEventQueue);
    while (queuedEvent != NULL)
    {
        sme_trace_warn((TR_FSM, "cleanup_fsm() :: Cleaning up %s :: ignoring saved 0x%.4X", process_name(pInstance), queuedEvent->id));
        fsm_ignore_event(context, queuedEvent);
        CsrPfree(queuedEvent);
        queuedEvent = fsm_pop_event(&pInstance->savedEventQueue);
    }

    /* Clean up the queues only if this is not a sub FSM*/
    if (!context->ownerInstance)
    {
        /* Clean up the Internal Event Queue */
        queuedEvent = fsm_pop_event_for_destination(&context->eventQueue, pInstance->instanceId);
        while (queuedEvent != NULL)
        {
            sme_trace_warn((TR_FSM, "cleanup_fsm() :: Cleaning up %s :: ignoring internal 0x%.4X", process_name(pInstance), queuedEvent->id));
            fsm_ignore_event(context, queuedEvent);
            CsrPfree(queuedEvent);
            queuedEvent = fsm_pop_event_for_destination(&context->eventQueue, pInstance->instanceId);
        }

        /* Clean up the External Event Queue */
        queuedEvent = fsm_pop_event_for_destination_thread_safe(
#ifdef FSM_MUTEX_ENABLE
                                                                context->externalEventQueueLock,
#endif
                                                                &context->externalEventQueue, pInstance->instanceId);
        while (queuedEvent != NULL)
        {
            sme_trace_warn((TR_FSM, "cleanup_fsm() :: Cleaning up %s :: ignoring external 0x%.4X", process_name(pInstance), queuedEvent->id));
            fsm_ignore_event(context, queuedEvent);
            CsrPfree(queuedEvent);
            queuedEvent = fsm_pop_event_for_destination_thread_safe(
#ifdef FSM_MUTEX_ENABLE
                                                                     context->externalEventQueueLock,
#endif
                                                                     &context->externalEventQueue, pInstance->instanceId);
        }
    }

    /* Clean up any process data */
    CsrPfree(pInstance->params);
    pInstance->params = NULL;
}


/**
 * @brief
 *   looks up and calls the transtion function for a event
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context : FSM context
 * @param[in]    event   : event to process
 *
 * @return
 *   CsrBool State Transition occured?
 */
static CsrBool fsm_process_event(FsmContext* context, FsmEvent *event)
{
    CsrBool result = FALSE;
    int i;
    FsmInstanceEntry* pInstance;

    CsrUint32 previous_state;
    const FsmTransitionFunctionTable* ptTable = NULL;
    const FsmEventEntry* eventEntryArray = NULL;
    FsmTransitionFnPtr  pfTrans = NULL;

#ifdef FSM_DEBUG
#ifdef FSM_DEBUG_DUMP
    FsmEvent eventcopy = *event;
    const char* transitionName = NULL;
#endif
#endif

#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexLock(context->transitionLock);
#endif
#endif

    sme_trace_entry((TR_FSM, "fsm_process_event()"));


    require(TR_FSM, context != NULL);
    require(TR_FSM, event != NULL);
    require(TR_FSM, context->instanceArray != NULL);
    require_trace(TR_FSM, context->maxProcesses > event->destination,
                  (TR_FSM, "maxProcesses %d destination %d", context->maxProcesses, event->destination));
    require_trace(TR_FSM, context->maxProcesses > event->sender_ || event->sender_ == FSM_ENV,
                  (TR_FSM, "maxProcesses %d sender_ %d", context->maxProcesses, event->sender_));

    pInstance = &context->instanceArray[event->destination];

    context->ownerInstance = NULL;
    context->currentInstance = pInstance;

    /* Safety first. Check Error conditions */
    if (context->maxProcesses <= event->destination || pInstance->state == FSM_TERMINATE)
    {
        pfTrans = fsm_invalid_event;
    }
    else
    {
        /* Set up the owner and current instance if using SubFsm's */
        while (pInstance->subFsm)
        {
            context->ownerInstance = pInstance;
            pInstance = (FsmInstanceEntry*)pInstance->subFsm;
        }
        context->currentInstance = pInstance;

        verify(TR_FSM, pInstance->fsmInfo != NULL);
        ptTable = &(pInstance->fsmInfo->transitionTable);

        verify(TR_FSM, ptTable->aStateEventMatrix != NULL);
        verify(TR_FSM, ptTable->numStates > pInstance->state);

        eventEntryArray = ptTable->aStateEventMatrix[pInstance->state].eventEntryArray;
        verify(TR_FSM, eventEntryArray != NULL);
    }
    /* A bit less efficient that the simple ID look up
     * but unless the FSM has LOTS of events this search
     * will not be unreasonable in performance
     */
    for (i = 0; pfTrans == NULL && i < ptTable->aStateEventMatrix[pInstance->state].numEntries; i++)
    {
        if (event->id == eventEntryArray[i].eventid)
        {
#ifdef FSM_DEBUG
            sme_trace_info((TR_FSM, "fsm_transition(%s) State(%s) Event(%s) From(%s)",
                         process_name(pInstance),
                         current_state_name(pInstance),
                         eventEntryArray[i].transitionName,
                         process_name_by_id(context,event->sender_)));
#endif
            pfTrans = eventEntryArray[i].transition;

#ifdef FSM_DEBUG
#ifdef FSM_DEBUG_DUMP
            transitionName = eventEntryArray[i].transitionName;
#endif
            if ( (context->onTransition != NULL)
               &&(pfTrans != fsm_invalid_event)
               &&(pfTrans != fsm_saved_event)
               &&(pfTrans != fsm_ignore_event))
            {
                (*context->onTransition)(context->externalContext, &eventEntryArray[i], event);
            }
#endif
        }
    }

    /* Try the unhandled Transitions */
    if (pfTrans == NULL)
    {
        eventEntryArray = pInstance->fsmInfo->unhandledTransitions.eventEntryArray;
        for (i = 0; i < pInstance->fsmInfo->unhandledTransitions.numEntries && pfTrans == NULL; i++)
        {
            if (event->id == eventEntryArray[i].eventid)
            {
#ifdef FSM_DEBUG
                sme_trace_info((TR_FSM, "fsm_transition(%s) State(%s) Event(%s) From(%s)",
                         process_name(pInstance),
                         current_state_name(context->currentInstance),
                         eventEntryArray[i].transitionName,
                         process_name_by_id(context,event->sender_)));
#endif
                pfTrans = eventEntryArray[i].transition;

#ifdef FSM_DEBUG
#ifdef FSM_DEBUG_DUMP
                transitionName = eventEntryArray[i].transitionName;
#endif
                if( ((context)->onUnhandedCallback != NULL)
                   &&(pfTrans != fsm_invalid_event)
                   &&(pfTrans != fsm_saved_event)
                   &&(pfTrans != fsm_ignore_event))
                {
                    (*context->onUnhandedCallback)(context->externalContext, &eventEntryArray[i], event);
                }
#endif
                break;
            }
        }
    }

    /* Save Event if Save* enabled */
    if (pfTrans == NULL && ptTable->aStateEventMatrix[pInstance->state].saveAll)
    {
        pfTrans = fsm_saved_event;
    }

    /* Default unhandled behavior */
    if (pfTrans == NULL)
    {
        /* Handle Saving to owner when using SubFsm */
        if (context->ownerInstance)
        {
            /* Set currentInstance -> Save the event -> Restore currentInstance */
            context->currentInstance = context->ownerInstance;
            pfTrans = fsm_saved_event;
        }
        else
        {
            pfTrans = fsm_invalid_event;
        }
    }

    /* store current state before calling transition function */
    previous_state = pInstance->state;

    /* Call the transition function */
    (*(pfTrans))(context, event);

    /* Restore the currentInstance when saving to SubFsm Owner */
    if (context->ownerInstance == context->currentInstance)
    {
        context->currentInstance = pInstance;
    }

    verify(TR_FSM, context->eventForwardedOrSaved == NULL || context->eventForwardedOrSaved == event);

#ifdef FSM_DEBUG_DUMP
    {
        FsmInstanceEntry* pDbgInstance = pInstance;
        FsmTransitionRecord* lastrecord = &pDbgInstance->transitionRecords.records[(pDbgInstance->transitionRecords.numTransitions-1) % FSM_MAX_TRANSITION_HISTORY];

        if (pDbgInstance->transitionRecords.numTransitions != 0 &&
            lastrecord->event.id      == eventcopy.id        &&
            lastrecord->event.sender_ == eventcopy.sender_   &&
            lastrecord->fromState     == previous_state      &&
            lastrecord->toState       == pDbgInstance->state)
        {
            /* Same event -> Transition So just count */
            lastrecord->transitionNumber = context->masterTransitionNumber++;
            lastrecord->transitionCount++;
        }
        else
        {
            /* Store a copy of the transition */
            FsmTransitionRecord* record = &pDbgInstance->transitionRecords.records[pDbgInstance->transitionRecords.numTransitions % FSM_MAX_TRANSITION_HISTORY];
            pDbgInstance->transitionRecords.numTransitions++;
            record->transitionNumber = context->masterTransitionNumber++;
            record->transitionCount = 1;
            record->event = eventcopy;
            record->fromState = (CsrUint16)previous_state;
            record->toState = pDbgInstance->state;
            record->transitionFn = pfTrans;

#ifdef FSM_DEBUG
            record->transitionName = transitionName;
#endif
        }
}
#endif

    /* Delete signal unless saved or forwarded */
    if (context->eventForwardedOrSaved == NULL)
    {
        CsrPfree(event);
    }
    context->eventForwardedOrSaved = NULL;

    if ((previous_state != pInstance->state))
    {
        result = TRUE;
    }

    if (pInstance->state == FSM_TERMINATE)
    {
        if (context->ownerInstance)
        {
            /* Termination of a SubFsm */
            /* Fix the pointer IF the complete function added a SubFsm */
            context->ownerInstance->subFsm = pInstance->subFsm;
            pInstance->subFsm = NULL;
        }

        cleanup_fsm(context, pInstance);

        if (context->ownerInstance)
        {
            /* Termination of a SubFsm */
            CsrPfree(pInstance); /*lint !e424*/ /* lint is being over cautious, this is being covered by the if statement */
            pInstance = NULL;

            /* Restore the owner to the current instance */
            context->currentInstance = context->ownerInstance;
            result = TRUE;
        }
        else
        {
            context->numProcesses--;
            result = FALSE;
        }
    }

#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexUnlock(context->transitionLock);
#endif
#endif

    return result;
}

/**
 * @brief
 *   Processes ALL saved events on a process
 *
 * @par Description
 *   Called on a state change to give the new state a chance to consume
 *   any saved events.
 *   Sets tempSaveList to save any saves so that on a new transition
 *   the saved event ordering is perserved AND so that an infinite loop is avoided.
 *
 * @param[in]    context : FSM context
 *
 * @return
 *   void
 */
static void fsm_process_saved_events(FsmContext* context)
{
    FsmInstanceEntry *pInstance;
    FsmEvent *savedEvent;

    require(TR_FSM, context != NULL);
    require(TR_FSM, context->currentInstance != NULL);

    pInstance = context->currentInstance;
    savedEvent = fsm_pop_event(&pInstance->savedEventQueue);

    context->useTempSaveList = TRUE;
    fsm_init_signal_list(&context->tempSaveList);

    while (savedEvent != NULL)
    {
        CsrBool checkSavedEvents = fsm_process_event(context, savedEvent);

        /* On a State Transition need to remerge the tempSaveList so
         * that the new state gets the saved events in the correct order
         */
        if (checkSavedEvents == TRUE)
        {
            savedEvent = fsm_pop_event(&pInstance->savedEventQueue);
            while (savedEvent != NULL)
            {
                fsm_add_event(&context->tempSaveList, (FsmEvent*)savedEvent);
                savedEvent = fsm_pop_event(&pInstance->savedEventQueue);
            }
            pInstance->savedEventQueue = context->tempSaveList;
            fsm_init_signal_list(&context->tempSaveList);
        }

        savedEvent = fsm_pop_event(&pInstance->savedEventQueue);
    }

    pInstance->savedEventQueue = context->tempSaveList;
    context->useTempSaveList = FALSE;
}

/**
 * @brief
 *   Processes ALL internal FSM events
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context : FSM context
 *
 * @return
 *   void
 */
static CsrUint32 fsm_process_events(FsmContext* context)
{
    CsrUint32 eventsprocessed = 0;
    FsmEvent* event;

    require(TR_FSM, context != NULL);

    event = fsm_pop_event(&context->eventQueue);
    while (event != NULL)
    {
        CsrBool checkSavedEvents = fsm_process_event(context, event);
        eventsprocessed++;

        if (checkSavedEvents == TRUE)
        {
            fsm_process_saved_events(context);
        }
        event = fsm_pop_event(&context->eventQueue);
    }
    return eventsprocessed;
}

/**
 * @brief
 *   Processes 1 external FSM event
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context : FSM context
 *
 * @return
 *   void
 */
static CsrUint32 fsm_process_external_event(FsmContext* context)
{
    CsrBool checkSavedEvents;
    FsmEvent* event;

    require(TR_FSM, context != NULL);

    event = fsm_pop_event_thread_safe(
#ifdef FSM_MUTEX_ENABLE
                                      context->externalEventQueueLock,
#endif
                                      &context->externalEventQueue);
    if (event == NULL)
    {
        return 0;
    }

    checkSavedEvents = fsm_process_event(context, event);

    if (checkSavedEvents == TRUE)
    {
        fsm_process_saved_events(context);
    }

    return 1;
}

/**
 * @brief
 *   Processes ALL expired FSM timers
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context : FSM context
 *
 * @return
 *   void
 */
static CsrUint32 fsm_process_timers(FsmContext* context)
{
    CsrUint32 eventsprocessed = 0;
    FsmEvent* event;

    require(TR_FSM, context != NULL);

    event = (FsmEvent*) fsm_pop_expired_timer(context, &context->timerQueue);
    while (event != NULL)
    {
        CsrBool checkSavedEvents = fsm_process_event(context, event);
        eventsprocessed++;

        if (checkSavedEvents == TRUE)
        {
            fsm_process_saved_events(context);
        }

        /* After a single timer is processed process ALL
         * Internal events before the next timer is processed */
        eventsprocessed += fsm_process_events(context);

        event = (FsmEvent*) fsm_pop_expired_timer(context, &context->timerQueue);
    }
    return eventsprocessed;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_call_entry_functions(FsmContext* context)
{
    CsrUint32 instanceID;

#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexLock(context->transitionLock);
#endif
#endif

    require(TR_FSM, context != NULL);
    sme_trace_entry((TR_FSM, "fsm_call_entry_functions()"));

    for (instanceID = 0; instanceID < context->maxProcesses; instanceID++)
    {
        FsmInstanceEntry* pInstance = &context->instanceArray[instanceID];
        if (pInstance->state == FSM_TERMINATE)
        {
            continue;
        }

        verify(TR_FSM, pInstance != NULL);  /*lint !e774*/
        if (*(pInstance->fsmInfo->entryFn) != NULL)
        {
             sme_trace_debug((TR_FSM, "fsm_call_entry_functions() Process(%s :: 0x%.4X) ",
                             process_name(pInstance),
                             pInstance->instanceId));
            context->currentInstance = pInstance;
            (*pInstance->fsmInfo->entryFn)(context);
        }
    }
#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexUnlock(context->transitionLock);
#endif
#endif
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_ignore_event(FsmContext* context, const FsmEvent* ignoreEvent)
{
    FsmInstanceEntry* pInstance;
    int i;
    require(TR_FSM, context != NULL);
    require(TR_FSM, ignoreEvent != NULL);
    require_trace(TR_FSM, context->maxProcesses > ignoreEvent->destination,
                  (TR_FSM, "maxProcesses %d destination %d", context->maxProcesses, ignoreEvent->destination));

    sme_trace_warn((TR_FSM, "Ignore event transition Process(%s) State(%s) Event(0x%.4X) From(%s)",
                             process_name(context->currentInstance),
                             current_state_name(context->currentInstance),
                             ignoreEvent->id,
                             process_name_by_id(context,ignoreEvent->sender_)));

    /* Look up destination FSM's ignore functions */
    pInstance = &context->instanceArray[ignoreEvent->destination];
    for (i = 0; i < pInstance->fsmInfo->ignoreFunctions.numEntries; i++)
    {
        if (ignoreEvent->id == pInstance->fsmInfo->ignoreFunctions.eventEntryArray[i].eventid)
        {
            pInstance->fsmInfo->ignoreFunctions.eventEntryArray[i].transition(context, ignoreEvent);
            return;
        }
    }

#ifdef FSM_DEBUG
    if ((context)->onIgnoreCallback != NULL)
    {
        (*(context)->onIgnoreCallback)(context->externalContext, ignoreEvent);
    }
#endif

    /* special ignore to allow the app to cleanup leaky signals */
    if ((context)->appIgnoreCallback != NULL)
    {
        (*(context)->appIgnoreCallback)(context->applicationContext, ignoreEvent);
    }
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_saved_event(FsmContext* context, const FsmEvent* saveEvent)
{
    FsmEvent* nonconstevent = (FsmEvent*)saveEvent;
    require(TR_FSM, context != NULL);
    require(TR_FSM, saveEvent != NULL);
    require_trace(TR_FSM, context->maxProcesses > saveEvent->destination,
                  (TR_FSM, "maxProcesses %d destination %d", context->maxProcesses, saveEvent->destination));
    require_trace(TR_FSM, context->instanceArray[saveEvent->destination].state != FSM_TERMINATE,
                  (TR_FSM, "Saved event transition Process(%s) State(%s) Event(0x%.4X) From(%s)",
                   process_name(context->currentInstance),
                   current_state_name(context->currentInstance),
                   saveEvent->id,
                   process_name_by_id(context,saveEvent->sender_)));

    sme_trace_info((TR_FSM, "Saved event transition Process(%s) State(%s) Event(0x%.4X) From(%s)",
                             process_name(context->currentInstance),
                             current_state_name(context->currentInstance),
                             saveEvent->id,
                             process_name_by_id(context,saveEvent->sender_)));

    context->eventForwardedOrSaved = nonconstevent;

    if (context->useTempSaveList)
    {
        fsm_add_event(&context->tempSaveList, (FsmEvent*)saveEvent);
    }
    else
    {
        fsm_add_event(&context->currentInstance->savedEventQueue, (FsmEvent*)saveEvent);
    }


#ifdef FSM_DEBUG
    if ((context)->onSaveCallback != NULL)
    {
        (*(context)->onSaveCallback)(context->externalContext, saveEvent);
    }
#endif

}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_error_event(FsmContext* context, const FsmEvent* errorEvent)
{
    require(TR_FSM, context != NULL);
    require(TR_FSM, errorEvent != NULL);
    require_trace(TR_FSM, context->maxProcesses > errorEvent->destination,
                  (TR_FSM, "maxProcesses %d destination %d", context->maxProcesses, errorEvent->destination));
    require_trace(TR_FSM, context->instanceArray[errorEvent->destination].state != FSM_TERMINATE,
                  (TR_FSM, "Error event transition Process(%s) State(%s) Event(0x%.4X) From(%s)",
                   process_name(context->currentInstance),
                   current_state_name(context->currentInstance),
                   errorEvent->id,
                   process_name_by_id(context,errorEvent->sender_)));

    sme_trace_error((TR_FSM, "Error event transition Process(%s) State(%s) Event(0x%.4X) From(%s)",
                             process_name(context->currentInstance),
                             current_state_name(context->currentInstance),
                             errorEvent->id,
                             process_name_by_id(context,errorEvent->sender_)));

    fsm_debug_dump(context);

#ifdef FSM_DEBUG
    if ((context)->onErrorCallback != NULL)
    {
        (*(context)->onErrorCallback)(context->externalContext, errorEvent);
    }
#endif

    verify(TR_FSM,0);  /*lint !e774*/

    /* Call ignore to cleanup the event */
    fsm_ignore_event(context, errorEvent);
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_invalid_event(FsmContext*  context, const FsmEvent* invalidEvent)
{
    require(TR_FSM, context != NULL);
    require(TR_FSM, invalidEvent != NULL);
    require_trace(TR_FSM, context->maxProcesses > invalidEvent->destination,
                  (TR_FSM, "maxProcesses %d destination %d", context->maxProcesses, invalidEvent->destination));
    require_trace(TR_FSM, context->instanceArray[invalidEvent->destination].state != FSM_TERMINATE,
                  (TR_FSM, "Invalid event transition Process(%s) State(%s) Event(0x%.4X) From(%s)",
                   process_name(context->currentInstance),
                   current_state_name(context->currentInstance),
                   invalidEvent->id,
                   process_name_by_id(context,invalidEvent->sender_)));

    fsm_debug_dump(context);

    sme_trace_crit((TR_FSM, "Invalid event transition Process(%s) State(%s) Event(0x%.4X) From(%s)",
                             process_name(context->currentInstance),
                             current_state_name(context->currentInstance),
                             invalidEvent->id,
                             process_name_by_id(context,invalidEvent->sender_)));

#ifdef FSM_DEBUG
    if ((context)->onInvalidCallback != NULL)
    {
        (*(context)->onInvalidCallback)(context->externalContext, invalidEvent);
    }
#endif

    verify(TR_FSM,0);  /*lint !e774*/

    /* Call ignore to cleanup the event */
    fsm_ignore_event(context, invalidEvent);
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_send_event(FsmContext *context, FsmEvent* event, CsrUint16 destination, CsrUint16 id)
{
    require(TR_FSM, context != NULL);
    require_trace(TR_FSM, context->maxProcesses > destination,
                  (TR_FSM, "maxProcesses %d destination %d", context->maxProcesses, destination));
    require_trace(TR_FSM, context->instanceArray[destination].state != FSM_TERMINATE,
                  (TR_FSM, "fsm_send_event Process(%s) State(%s) Event(0x%.4X) To(%s)",
                   process_name(context->currentInstance),
                   current_state_name(context->currentInstance),
                   id,
                   process_name_by_id(context,destination)));

    sme_trace_entry((TR_FSM, "fsm_send_event Process(%s) State(%s) Event(0x%.4X) To(%s)",
                             process_name(context->currentInstance),
                             current_state_name(context->currentInstance),
                             id,
                             process_name_by_id(context,destination)));

    /* if the event is null then no params are needed */
    /* this allows a small code space optimisation    */
    if (event == NULL)
    {
        event = (FsmEvent*) CsrPmalloc(sizeof(FsmEvent));
    }
    event->id = id;
    event->sender_ = context->currentInstance->instanceId;
    event->destination = destination;

    fsm_add_event(&context->eventQueue, event);
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_forward_event(FsmContext *context, CsrUint16 destination, const FsmEvent* event)
{
    FsmEvent* nonconstevent = (FsmEvent*)event;
    require(TR_FSM, context != NULL);
    require(TR_FSM, nonconstevent != NULL);
    require_trace(TR_FSM, context->maxProcesses > destination,
                  (TR_FSM, "maxProcesses %d destination %d", context->maxProcesses, destination));
    require_trace(TR_FSM, context->instanceArray[destination].state != FSM_TERMINATE,
                  (TR_FSM, "fsm_send_event Process(%s) State(%s) Event(0x%.4X) To(%s)",
                   process_name(context->currentInstance),
                   current_state_name(context->currentInstance),
                   event->id,
                   process_name_by_id(context,destination)));

    sme_trace_entry((TR_FSM, "fsm_forward_event Process(%s) State(%s) Event(0x%.4X) To(%s)",
                             process_name(context->currentInstance),
                             current_state_name(context->currentInstance),
                             nonconstevent->id,
                             process_name_by_id(context,destination)));

    context->eventForwardedOrSaved = nonconstevent;
    nonconstevent->sender_ = context->currentInstance->instanceId;
    nonconstevent->destination = destination;

    fsm_add_event(&context->eventQueue, nonconstevent);
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_send_event_external(FsmContext *context, FsmEvent* event, CsrUint16 destination, CsrUint16 id)
{
    require(TR_FSM, context != NULL);
    require_trace(TR_FSM, context->maxProcesses > destination,
                  (TR_FSM, "maxProcesses %d destination %d", context->maxProcesses, destination));
    require_trace(TR_FSM, context->instanceArray[destination].state != FSM_TERMINATE,
                  (TR_FSM, "fsm_send_event_external Process(N/A) State(N/A) Event(0x%.4X) To(%s)",
                   id,
                   process_name_by_id(context,destination)));

    sme_trace_entry((TR_FSM, "fsm_send_event_external Process(N/A) State(N/A) Event(0x%.4X) To(%s)",
                     id,
                     process_name_by_id(context,destination)));

    /* if the event is null then no params are needed */
    /* this allows a small code space optimisation    */
    if (event == NULL)
    {
        event = (FsmEvent*) CsrPmalloc(sizeof(FsmEvent));
    }
    event->id = id;
    event->sender_ = FSM_ENV;
    event->destination = destination;

    fsm_add_event_thread_safe(
#ifdef FSM_MUTEX_ENABLE
                              context->externalEventQueueLock,
#endif
                              &context->externalEventQueue, event);

    /* Call installed wakup on an external event into the FSM */
    if (context->externalEventFn != NULL)
    {
        (*context->externalEventFn)(context->externalContext);
    }
    sme_trace_entry((TR_FSM, "fsm_send_event_external complete"));
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
FsmTimerId fsm_set_timer(FsmContext *context, FsmTimer* pTimer, CsrUint32 timeInMs, CsrUint16 timeExtraMs, CsrUint16 id)
{
    require(TR_FSM, context != NULL);

    /* if the event is null then no params are needed */
    /* this allows a small code space optimisation    */
    if (pTimer == NULL)
    {
        pTimer = (FsmTimer*) CsrPmalloc(sizeof(FsmTimer));
    }

    pTimer->id = id;
    pTimer->sender_ = context->currentInstance->instanceId;
    pTimer->destination = context->currentInstance->instanceId;

    pTimer->timerid.id = pTimer->id;
    pTimer->timerid.destination = pTimer->destination;
    pTimer->timerid.uniqueid = context->timerQueue.nexttimerid;

    pTimer->timeoutData.timeoutTimeMs = fsm_get_time_of_day_ms(context) + timeInMs;
    pTimer->timeoutData.timeoutTimeExtraMs = timeExtraMs;
    context->timerQueue.nexttimerid++;
    if (context->timerQueue.nexttimerid == 0)
    {
        context->timerQueue.nexttimerid++;
    }

    fsm_add_timer(&context->timerQueue, pTimer);
    return pTimer->timerid;
}

/**
 * See description in fsm/fsm_internal.h
 */
const FsmEvent* fsm_sniff_saved_event(FsmContext *context, CsrUint16 eventid)
{
    require(TR_FSM, context != NULL);

    return fsm_find_event_by_id(&context->currentInstance->savedEventQueue, eventid);
}

/**
 * See description in fsm/fsm_internal.h
 */
void fsm_remove_saved_event(FsmContext *context, FsmEvent *event)
{
    require(TR_FSM, context != NULL);
    require(TR_FSM, event != NULL);

    fsm_remove_event(&context->currentInstance->savedEventQueue, event);
}


/**
 * See description in fsm/fsm_internal.h
 */
void fsm_flush_saved_events_from(FsmContext *context, CsrUint16 fsmid, CsrUint16 srcid)
{
    FsmEvent* event;
    FsmEventList* savedEventQueue;

    require(TR_FSM, context != NULL);
    require_trace(TR_FSM, context->maxProcesses > fsmid,
                  (TR_FSM, "maxProcesses %d fsmid %d", context->maxProcesses, fsmid));
    require_trace(TR_FSM, context->maxProcesses > srcid,
                  (TR_FSM, "maxProcesses %d srcid %d", context->maxProcesses, srcid));
    require_trace(TR_FSM, context->instanceArray[fsmid].state != FSM_TERMINATE,
                  (TR_FSM, "fsm_flush_saved_events_from  fsmid %d fsm(%s)",
                   fsmid,
                   process_name_by_id(context,fsmid)));

    sme_trace_entry((TR_FSM, "fsm_flush_saved_events_from() :: flush events from %s", fsm_process_name_by_id(context, srcid)));

    savedEventQueue = &context->instanceArray[fsmid].savedEventQueue;

    /* Clean up the Internal Event Queue */
    event = fsm_pop_event_from_source(savedEventQueue, srcid);
    while (event != NULL)
    {
        sme_trace_error((TR_FSM, "fsm_flush_saved_events_from() :: ignoring event 0x%.4X from %s", event->id, fsm_process_name_by_id(context, srcid)));
        fsm_ignore_event(context, event);
        CsrPfree(event);
        event = fsm_pop_event_from_source(savedEventQueue, srcid);
    }
}

/*
 * #define fsm_get_next_timeout(context)  ((context)->timerQueue.first != NULL? (context)->timerQueue.first->timeoutData.timeoutTimeMs:0xFFFFFFFF)
 *
 */
/**
 * See description in fsm/csr_wifi_fsm.h
 */
FsmTimerData fsm_get_next_timeout(FsmContext *context)
{
    FsmTimerData result = {0xFFFFFFFF, 0};
    if (context->timerQueue.first != NULL)
    {
        FsmTimer *currenttimer = context->timerQueue.first->next;
        CsrUint32 window = context->timerQueue.first->timeoutData.timeoutTimeMs +
                        context->timerQueue.first->timeoutData.timeoutTimeExtraMs;

        result = context->timerQueue.first->timeoutData;

        /* Make sure the timeout window returned is the smallest allowed for all the timers */
        while(currenttimer != NULL && CsrTimeLe(currenttimer->timeoutData.timeoutTimeMs, window))
        {
            CsrUint32 currentwindow = currenttimer->timeoutData.timeoutTimeMs + currenttimer->timeoutData.timeoutTimeExtraMs;
            if (CsrTimeLe(currentwindow, window))
            {
                result.timeoutTimeExtraMs = (CsrUint16)CsrTimeSub(currentwindow, result.timeoutTimeMs);
            }
            currenttimer = currenttimer->next;
        }
        result.timeoutTimeMs = (CsrUint32)CsrTimeSub(result.timeoutTimeMs, fsm_get_time_of_day_ms(context));

        /* The timer has passed it expiry time, reset it to 0 */
        if ((int)result.timeoutTimeMs < 0)
        {
                result.timeoutTimeMs = 0;
        }
    }

    return result;
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
CsrUint16 fsm_add_instance(FsmContext* context, const FsmProcessStateMachine* sminfo, CsrBool callEntry)
{
    FsmInstanceEntry* pThisInstance = NULL;
    CsrUint16 i;

#ifdef FSM_DEBUG
    sme_trace_entry((TR_FSM, "fsm_add_instance(%s)", sminfo->processName));
#else
    sme_trace_entry((TR_FSM, "fsm_add_instance()"));
#endif

    require(TR_FSM, context != NULL) ;
    require_trace(TR_FSM, context->numProcesses < context->maxProcesses,
                  (TR_FSM, "numProcesses %d maxProcesses %d", context->numProcesses, context->maxProcesses));

    /* First unused instance in the instance Array */
    for (i=0; i<context->maxProcesses; i++)
    {
        if (context->instanceArray[i].state == FSM_TERMINATE)
        {
            pThisInstance = &context->instanceArray[i];
            pThisInstance->instanceId = i;
            pThisInstance->fsmInfo = sminfo;
            pThisInstance->state = FSM_START;
            pThisInstance->params = NULL;
            pThisInstance->subFsm = NULL;
            fsm_init_signal_list(&pThisInstance->savedEventQueue);

            /* ---------------------------------------------------- */
            /* Validate the State numbers against the needed values */
            /* This ensures that the State enum matches the correct */
            /* row in the state table                               */
            /* ---------------------------------------------------- */
#ifdef FSM_DEBUG
            {
                int loop;
                for (loop = 0; loop < sminfo->transitionTable.numStates; loop++)
                {
                    if (sminfo->transitionTable.aStateEventMatrix[loop].stateNumber != loop)
                    {
                        sme_trace_crit((TR_FSM, "fsm_add_instance() state transition %s of process %s state number should be %d but is %d",
                                                 sminfo->transitionTable.aStateEventMatrix[loop].stateName,
                                                 sminfo->processName,
                                                 loop,
                                                 sminfo->transitionTable.aStateEventMatrix[loop].stateNumber));
                        CsrPanic(CSR_TECH_WIFI, CSR_PANIC_FW_UNEXPECTED_VALUE, "fsm_add_instance() State -> Enum mismatch");
                    }
                }
            }
#endif
            /* ---------------------------------------------------- */
#ifdef FSM_DEBUG_DUMP
            pThisInstance->transitionRecords.numTransitions = 0;
#endif

            context->numProcesses++;

#ifdef FSM_DEBUG
            if ((context)->onCreate != NULL)
            {
                (*context->onCreate)(context, pThisInstance);
            }
#endif

            if (callEntry && *(pThisInstance->fsmInfo->entryFn) != NULL)
            {
                 FsmInstanceEntry* currentInstance = context->currentInstance;
                 sme_trace_debug((TR_FSM, "fsm_add_instance() Calling entry function on Process(%s :: 0x%.4X) ",
                                 process_name(pThisInstance),
                                 pThisInstance->instanceId));
                context->currentInstance = pThisInstance;
                (*pThisInstance->fsmInfo->entryFn)(context);
                context->currentInstance = currentInstance;
            }


            return pThisInstance->instanceId;
        }
    }

    CsrPanic(CSR_TECH_WIFI, CSR_PANIC_FW_UNEXPECTED_VALUE, "fsm_add_instance() No free fsm instances for new fsm");
    return (CsrUint16)-1;
}

FsmInstanceEntry* fsm_add_sub_instance(FsmContext* context, const FsmProcessStateMachine* sminfo)
{
    FsmInstanceEntry* ownerInstance = context->currentInstance;
    FsmInstanceEntry* newInstance = NULL;

#ifdef FSM_DEBUG
    sme_trace_entry((TR_FSM, "fsm_add_sub_instance(%s)", sminfo->processName));
#else
    sme_trace_entry((TR_FSM, "fsm_add_sub_instance()"));
#endif

    require(TR_FSM, context != NULL) ;
    require(TR_FSM, context->currentInstance != NULL) ;

    /* Make sure we add this Sub FSM to the end of the list */
    while(ownerInstance->subFsm != NULL)
    {
        ownerInstance = (FsmInstanceEntry*)ownerInstance->subFsm;
    }

    newInstance = (FsmInstanceEntry*)CsrPmalloc(sizeof(FsmInstanceEntry));
    ownerInstance->subFsm = (struct FsmInstanceEntry*)newInstance;

    newInstance->instanceId = ownerInstance->instanceId;
    newInstance->fsmInfo = sminfo;
    newInstance->state = FSM_START;
    newInstance->params = NULL;
    newInstance->subFsm = NULL;
    fsm_init_signal_list(&newInstance->savedEventQueue);

#ifdef FSM_DEBUG_DUMP
    newInstance->transitionRecords.numTransitions = 0;
#endif

    #ifdef FSM_DEBUG
    if ((context)->onCreate != NULL)
    {
        (*context->onCreate)(context, newInstance);
    }
#endif

    if (*(newInstance->fsmInfo->entryFn) != NULL)
    {
        FsmInstanceEntry* currentInstance = context->currentInstance;
        sme_trace_debug((TR_FSM, "fsm_add_instance() Calling entry function on Sub Process(%s :: 0x%.4X) ",
                                                       process_name(newInstance), newInstance->instanceId));
        context->currentInstance = newInstance;
        (*newInstance->fsmInfo->entryFn)(context);
        context->currentInstance = currentInstance;
    }

    return newInstance;
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
FsmContext* fsm_init_context(void* applicationContext, void* externalContext, CsrUint16 maxProcesses)
{
    int i;
    FsmContext* context =  (FsmContext*) CsrPmalloc(sizeof(FsmContext));

    context->currentInstance = NULL;
    context->ownerInstance = NULL;
    context->applicationContext = applicationContext;
    context->externalContext = externalContext;

    fsm_init_signal_list(&context->eventQueue);
    fsm_init_signal_list(&context->externalEventQueue);
    context->useTempSaveList = FALSE;
    context->eventForwardedOrSaved = NULL;
    fsm_init_signal_list(&context->tempSaveList);
    fsm_init_timer_list(&context->timerQueue);
    context->maxProcesses = maxProcesses;
    context->numProcesses = 0;
    context->instanceArray = (FsmInstanceEntry*)CsrPmalloc(sizeof(FsmInstanceEntry) * maxProcesses);
    for (i=0; i<context->maxProcesses; i++)
    {
        context->instanceArray[i].state = FSM_TERMINATE;
        context->instanceArray[i].params = NULL;
    }
    context->externalEventFn = NULL;
    context->appIgnoreCallback = NULL;

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexCreate(&context->externalEventQueueLock);
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexCreate(&context->transitionLock);
#endif
#endif

#ifdef FSM_DEBUG
    context->onCreate = NULL;
    context->onTransition = NULL;
    context->onUnhandedCallback = NULL;
    context->onIgnoreCallback = NULL;
    context->onStateChange = NULL;
    context->onSaveCallback = NULL;
    context->onErrorCallback = NULL;
    context->onInvalidCallback = NULL;
#endif

#ifdef FSM_DEBUG_DUMP
    context->masterTransitionNumber = 0;
#endif
    return context;
}

/**
 * @brief
 *   Cleanup the fsm back to a pre started state.
 *
 * @param[in]    context : FSM context
 *
 * @return
 *   void
 */
static void fsm_flush(FsmContext* context)
{
    int i;
    FsmEvent* queuedEvent;
    FsmTimer* queuedTimer;

    require(TR_FSM, context != NULL);

    /* Clean up the Internal Event Queue */
    queuedEvent = fsm_pop_event(&context->eventQueue);
    while (queuedEvent != NULL)
    {
        fsm_ignore_event(context, queuedEvent);
        CsrPfree(queuedEvent);
        queuedEvent = fsm_pop_event(&context->eventQueue);
    }

    /* Clean up the Timer Event Queue */
    queuedTimer = context->timerQueue.first;
    while (queuedTimer != NULL)
    {
        context->timerQueue.first = queuedTimer->next;
        fsm_ignore_event(context, (FsmEvent*)queuedTimer);
        CsrPfree(queuedTimer);
        queuedTimer = context->timerQueue.first;
    }

    /* Clean up the External Event Queue */
    queuedEvent = fsm_pop_event_thread_safe(
#ifdef FSM_MUTEX_ENABLE
                                            context->externalEventQueueLock,
#endif
                                            &context->externalEventQueue);
    while (queuedEvent != NULL)
    {
        fsm_ignore_event(context, queuedEvent);
        CsrPfree(queuedEvent);
        queuedEvent = fsm_pop_event_thread_safe(
#ifdef FSM_MUTEX_ENABLE
                                                context->externalEventQueueLock,
#endif
                                                &context->externalEventQueue);
    }

    for (i = 0; i < context->maxProcesses; ++i)
    {
        if (context->instanceArray[i].state == FSM_TERMINATE) continue;

        sme_trace_info((TR_FSM, "Reset on Process(%s :: 0x%.4X) ", process_name((&context->instanceArray[i])), context->instanceArray[i].instanceId));

        /* Set the current instance pointer */
        context->currentInstance = &context->instanceArray[i];
        if (context->currentInstance->fsmInfo->resetFn != NULL)
        {
            (*context->currentInstance->fsmInfo->resetFn)(context);
        }

        cleanup_fsm(context, context->currentInstance);
        if (context->currentInstance->state == FSM_TERMINATE)
        {
            context->numProcesses--;
        }
    }
    context->currentInstance = NULL;
}


/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_reset(FsmContext* context)
{
    require(TR_FSM, context != NULL);
    fsm_flush(context);

    /* Recall the entry functions */
    fsm_call_entry_functions(context);
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_shutdown(FsmContext* context)
{
    require(TR_FSM, context != NULL);

    fsm_flush(context);

#ifdef FSM_MUTEX_ENABLE
    CsrMutexDestroy(context->externalEventQueueLock);
#ifdef FSM_TRANSITION_LOCK
    CsrMutexDestroy(context->transitionLock);
#endif
#endif

    CsrPfree(context->instanceArray);
    CsrPfree(context);
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
FsmTimerData fsm_execute(FsmContext* context)
{
    CsrUint32 eventsProcessed;

    require(TR_FSM, context != NULL);

    do {
        eventsProcessed = 0;

        eventsProcessed += fsm_process_events(context);

        /* Note: fsm_process_timers() calls fsm_process_events(context)
         * after each timer fires so that ALL internal events are
         * processed after EACH timer.
         * This means the behaviour of the FSM should be more
         * deterministic
         */
        eventsProcessed += fsm_process_timers(context);

        /* Note: This only processes 1 external event before going
         * around the loop again to consume ALL internal events.
         * This means the behaviour of the FSM should be more
         * deterministic
         */
        eventsProcessed += fsm_process_external_event(context);
    } while (eventsProcessed != 0);

    context->currentInstance = NULL;
    return fsm_get_next_timeout(context);
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
CsrBool fsm_has_events(FsmContext* context)
{
    return context->eventQueue.first != NULL || context->externalEventQueue.first != NULL;
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_install_wakeup_callback(FsmContext* context, FsmExternalWakupCallbackPtr callback)
{
    context->externalEventFn = callback;
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
#ifdef FSM_DEBUG
void fsm_install_on_create_callback(FsmContext* context, FsmOnCreateFnPtr callback)
{
    context->onCreate = callback;
}
#endif

/**
 * See description in fsm/csr_wifi_fsm.h
 */
#ifdef FSM_DEBUG
void fsm_install_on_transition_callback(FsmContext* context, FsmOnTransitionFnPtr callback)
{
    context->onTransition = callback;
}
#endif

/**
 * See description in fsm/csr_wifi_fsm.h
 */
#ifdef FSM_DEBUG
void fsm_install_unhandled_event_callback(FsmContext* context, FsmOnTransitionFnPtr callback)
{
    context->onUnhandedCallback = callback;
}
#endif

/**
 * See description in fsm/csr_wifi_fsm.h
 */
#ifdef FSM_DEBUG
void fsm_install_on_state_change_callback(FsmContext* context, FsmOnStateChangeFnPtr callback)
{
    context->onStateChange = callback;
}
#endif

/**
 * See description in fsm/csr_wifi_fsm.h
 */
#ifdef FSM_DEBUG
void fsm_install_ignore_event_callback(FsmContext* context, FsmOnEventFnPtr callback)
{
    context->onIgnoreCallback = callback;
}
#endif

/**
 * See description in fsm/csr_wifi_fsm.h
 */
#ifdef FSM_DEBUG
void fsm_install_save_event_callback(FsmContext* context, FsmOnEventFnPtr callback)
{
    context->onSaveCallback = callback;
}
#endif

/**
 * See description in fsm/csr_wifi_fsm.h
 */
#ifdef FSM_DEBUG
void fsm_install_error_event_callback(FsmContext* context, FsmOnEventFnPtr callback)
{
    context->onErrorCallback = callback;
}
#endif

/**
 * See description in fsm/csr_wifi_fsm.h
 */
#ifdef FSM_DEBUG
void fsm_install_invalid_event_callback(FsmContext* context, FsmOnEventFnPtr callback)
{
    context->onInvalidCallback = callback;
}
#endif


/** @}
 */

