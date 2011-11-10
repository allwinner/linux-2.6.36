/** @file fsm_private.c
 *
 * Private FSM code
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
 *   FSM implementation for the non public FSM code
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/fsm/fsm_private.c#4 $
 *
 ****************************************************************************/

/** @{
 * @ingroup fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"

#include "fsm/fsm_types.h"
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_internal.h"
#include "fsm/fsm_private.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in fsm/fsm_private.h
 */
void fsm_add_event(FsmEventList *eventList, FsmEvent *obj)
{
    require(TR_FSM, eventList != NULL);
    require(TR_FSM, obj != NULL);

    obj->next = NULL;

    if(eventList->first == NULL)
    {
        eventList->first = obj;
    }
    else
    {
        eventList->last->next = obj;
    }

    eventList->last = obj;
}

/**
 * See description in fsm/fsm_private.h
 */
FsmEvent* fsm_find_event_by_id(FsmEventList *eventList, CsrUint16 id)
{
    FsmEvent* currentEvent;

    require(TR_FSM, eventList != NULL);

    if (eventList->first == NULL)
    {
        return NULL;
    }

    if (eventList->first->id == id)
    {
        return eventList->first;
    }

    currentEvent = eventList->first;

    while (currentEvent->next != NULL)
    {
        if (currentEvent->next->id == id)
        {
            return currentEvent->next;
        }
        currentEvent = currentEvent->next;
    }
    return NULL;
}

/**
 * See description in fsm/fsm_private.h
 */
void fsm_remove_event(FsmEventList *eventList, FsmEvent *event)
{
    FsmEvent* currentEvent;

    require(TR_FSM, eventList != NULL);
    require(TR_FSM, event != NULL);

    if (eventList->first == NULL)
    {
        return;
    }

    if (eventList->first == event)
    {
        (void)fsm_pop_event(eventList);
        return;
    }

    currentEvent = eventList->first;

    while (currentEvent->next != NULL)
    {
        if (currentEvent->next == event)
        {
            currentEvent->next = currentEvent->next->next;
            if (eventList->last == event)
            {
                eventList->last = currentEvent;
            }
            return;
        }
        currentEvent = currentEvent->next;
    }
}

/**
 * See description in fsm/fsm_private.h
 */
FsmEvent* fsm_pop_event(FsmEventList *eventList)
{
    FsmEvent * result = NULL;

    require(TR_FSM, eventList != NULL);
    if (eventList->first != NULL)
    {
        result = eventList->first;
        eventList->first = result->next;
    }
    return result;
}

/**
 * See description in fsm/fsm_private.h
 */
FsmEvent *fsm_pop_event_for_destination(FsmEventList *eventList, CsrUint16 destination)
{
    FsmEvent* currentEvent;
    FsmEvent* returnEvent;

    require(TR_FSM, eventList != NULL);

    if (eventList->first == NULL)
    {
        return NULL;
    }

    if (eventList->first->destination == destination)
    {
        return fsm_pop_event(eventList);
    }

    currentEvent = eventList->first;

    while (currentEvent->next != NULL)
    {
        if (currentEvent->next->destination == destination)
        {
            returnEvent = currentEvent->next;
            currentEvent->next = currentEvent->next->next;
            if (eventList->last == returnEvent)
            {
                eventList->last = currentEvent;
            }
            return returnEvent;
        }
        currentEvent = currentEvent->next;
    }
    return NULL;
}

/**
 * See description in fsm/fsm_private.h
 */
FsmEvent *fsm_pop_event_from_source(FsmEventList *eventList, CsrUint16 source)
{
    FsmEvent* currentEvent;
    FsmEvent* returnEvent;

    require(TR_FSM, eventList != NULL);

    if (eventList->first == NULL)
    {
        return NULL;
    }

    if (eventList->first->sender_ == source)
    {
        return fsm_pop_event(eventList);
    }

    currentEvent = eventList->first;

    while (currentEvent->next != NULL)
    {
        if (currentEvent->next->sender_ == source)
        {
            returnEvent = currentEvent->next;
            currentEvent->next = currentEvent->next->next;
            if (eventList->last == returnEvent)
            {
                eventList->last = currentEvent;
            }
            return returnEvent;
        }
        currentEvent = currentEvent->next;
    }
    return NULL;

}

#ifdef FSM_MUTEX_ENABLE
/**
 * See description in fsm/fsm_private.h
 */
void fsm_add_event_thread_safe(CsrMutexHandle lock, FsmEventList *eventList, FsmEvent *obj)
{
    require(TR_FSM, eventList != NULL);
    require(TR_FSM, obj != NULL);

    sme_trace_entry((TR_FSM, "fsm_add_event_thread_safe: entered"));
    (void)CsrMutexLock(lock);
    fsm_add_event(eventList, obj);
    (void)CsrMutexUnlock(lock);
    sme_trace_entry((TR_FSM, "fsm_add_event_thread_safe: completed"));
}

/**
 * See description in fsm/fsm_private.h
 */
FsmEvent* fsm_find_event_by_id_thread_safe(CsrMutexHandle lock, FsmEventList *eventList, CsrUint16 id)
{
    FsmEvent* event;
    require(TR_FSM, eventList != NULL);

    (void)CsrMutexLock(lock);
    event = fsm_find_event_by_id(eventList, id);
    (void)CsrMutexUnlock(lock);
    return event;
}

/**
 * See description in fsm/fsm_private.h
 */
FsmEvent *fsm_pop_event_for_destination_thread_safe(CsrMutexHandle lock, FsmEventList *eventList, CsrUint16 destination)
{
    FsmEvent* event;
    require(TR_FSM, eventList != NULL);

    (void)CsrMutexLock(lock);
    event = fsm_pop_event_for_destination(eventList, destination);
    (void)CsrMutexUnlock(lock);
    return event;
}
/**
 * See description in fsm/fsm_private.h
 */
void fsm_remove_event_thread_safe(CsrMutexHandle lock, FsmEventList *eventList, FsmEvent *obj)
{
    require(TR_FSM, eventList != NULL);
    require(TR_FSM, obj != NULL);

    (void)CsrMutexLock(lock);
    fsm_remove_event(eventList, obj);
    (void)CsrMutexUnlock(lock);
}

/**
 * See description in fsm/fsm_private.h
 */
FsmEvent* fsm_pop_event_thread_safe(CsrMutexHandle lock, FsmEventList *eventList)
{
    FsmEvent * result = NULL;

    require(TR_FSM, eventList != NULL);

    (void)CsrMutexLock(lock);
    result = fsm_pop_event(eventList);
    (void)CsrMutexUnlock(lock);
    return result;
}
#endif

/**
 * See description in fsm/fsm_private.h
 */
void fsm_add_timer(FsmTimerList *timerList, FsmTimer *timer)
{
    FsmTimer *currenttimer = timerList->first;
    require(TR_FSM, timerList != NULL);
    require(TR_FSM, timer != NULL);

    /* Empty Timer List */
    if(currenttimer == NULL )
    {
        timerList->first = timer;
        timerList->last  = timer;
        timer->next = NULL;
        return;
    }

    /* Add to the head of the list */
    if (CsrTimeLe(timer->timeoutData.timeoutTimeMs, currenttimer->timeoutData.timeoutTimeMs))
    {
        timer->next = timerList->first;
        timerList->first = timer;
        return;
    }

    /* Look ahead 1 insert */
    while(currenttimer->next != NULL)
    {
        if (CsrTimeLe(timer->timeoutData.timeoutTimeMs, currenttimer->next->timeoutData.timeoutTimeMs))
        {
            timer->next = currenttimer->next;
            currenttimer->next  = timer;
            return;
        }
        currenttimer = currenttimer->next;
    }

    /* Add to the tail of the list */
    timerList->last->next  = timer;
    timerList->last  = timer;
    timer->next = NULL;


}

/**
 * See description in fsm/fsm_private.h
 */
FsmTimer *fsm_pop_expired_timer(FsmContext* context, FsmTimerList *timerList)
{
    FsmTimer * timer = NULL;
    require(TR_FSM, timerList != NULL);

    if (timerList->first != NULL)
    {
        if (CsrTimeLe(timerList->first->timeoutData.timeoutTimeMs, fsm_get_time_of_day_ms(context)))
        {
            timer = timerList->first;
            timerList->first = timer->next;
        }
    }
    return timer;
}


/**
 * @brief
 *   Removes a timer from the saved events list
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context   : FSM context
 * @param[in]    eventList : Event List to remove from
 * @param[in]    timerid   : timerid to remove
 *
 * @return
 *   void
 */
static void fsm_remove_saved_timer(FsmContext* context, FsmEventList *eventList, FsmTimerId timerid)
{
    FsmEvent* event;

    require(TR_FSM, context != NULL);
    require(TR_FSM, eventList != NULL);

    if (eventList->first == NULL)
    {
        return;
    }

    event = eventList->first;
    /* Handle start of list case */
    if (event->id == timerid.id &&
        event->destination == timerid.destination &&
        ((FsmTimer*)event)->timerid.uniqueid == timerid.uniqueid)
    {
        eventList->first = eventList->first->next;
        CsrPfree(event);
        return;
    }

    while (event->next != NULL)
    {
        if (event->next->id == timerid.id &&
            event->next->destination == timerid.destination &&
            ((FsmTimer*)event)->next->timerid.uniqueid == timerid.uniqueid)
        {
            FsmEvent* removeTimer = event->next;
            event->next = event->next->next;

            if (eventList->last == removeTimer)
            {
                eventList->last = event;
            }

            CsrPfree(removeTimer);
            return;
        }
        event = event->next;
    }
}

/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_remove_timer(FsmContext* context, FsmTimerId timerid)
{
    FsmTimerList* timerList;
    FsmTimer* timer;

    require(TR_FSM, context != NULL);

    /* Check reserved value */
    if (timerid.uniqueid == 0)
    {
        sme_trace_info((TR_FSM, "fsm_remove_timer() :: Attempted to remove a NULL timer"));
        return;
    }

    require(TR_FSM, context->numProcesses > timerid.destination);

    timerList = &context->timerQueue;
    if (timerList->first == NULL)
    {
        /* May have been fired and saved so try to remove
         * from the destinations saved events
         */
        fsm_remove_saved_timer(context, &context->instanceArray[timerid.destination].savedEventQueue, timerid);
        return;
    }

    timer = timerList->first;

    /* Handle start of list case */
    if (timer->timerid.uniqueid == timerid.uniqueid)
    {
        verify(TR_FSM, timer->timerid.id == timerid.id);
        verify(TR_FSM, timer->timerid.destination == timerid.destination);
        timerList->first = timerList->first->next;
        CsrPfree(timer);
        return;
    }

    while (timer->next != NULL)
    {
        if (timer->next->timerid.uniqueid == timerid.uniqueid)
        {
            FsmTimer* removeTimer = timer->next;
            timer->next = timer->next->next;

            if (timerList->last == removeTimer)
            {
                timerList->last = timer;
            }

            CsrPfree(removeTimer);
            return;
        }
        timer = timer->next;
    }
    /* May have been fired and saved so try to remove
     * from the destinations saved events
     */
    fsm_remove_saved_timer(context, &context->instanceArray[timerid.destination].savedEventQueue, timerid);
}

#ifdef FSM_TEST_SUPPORT
/* NOTE: This could be moved to the context in the future if needed */
static CsrUint32 fsmTimeOffset = 0;
#endif

/**
 * See description in fsm/csr_wifi_fsm.h
 */
CsrUint32 fsm_get_time_of_day_ms(FsmContext *context)
{
    CsrUint32 nowMs;
    csr_utctime tod;
    CsrSchedGetUtc(&tod, NULL);

    nowMs = (tod.sec * 1000) + tod.msec;

#ifdef FSM_TEST_SUPPORT
    nowMs += fsmTimeOffset;
#endif

    return nowMs;
}

#ifdef FSM_TEST_SUPPORT
/**
 * See description in fsm/csr_wifi_fsm.h
 */
void fsm_test_advance_time(FsmContext *context, CsrUint32 ms)
{
    fsmTimeOffset += ms;
}
#endif



/** @}
 */

