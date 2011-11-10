/** @file fsm_debug.c
 *
 * Debug FSM code
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/fsm/fsm_debug.c#2 $
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

#ifdef FSM_DEBUG_DUMP

/* MACROS *******************************************************************/
#ifdef FSM_DEBUG
const char* fsm_state_name_by_id(FsmContext *context, CsrUint16 id, CsrUint16 state)
{
    const char* name = context->instanceArray[id].fsmInfo->transitionTable.aStateEventMatrix[state].stateName;

    if (state >= context->instanceArray[id].fsmInfo->transitionTable.numStates)
    {
        static char statenumber[8];
        CsrSprintf(statenumber, "%d", state);
        return statenumber;
    }

    if (CsrStrNCmp(name, "FSMSTATE_", 9) == 0)
    {
        name += 9;
    }
    return name;
}

/*#define fsm_state_name_by_id(context, id, state) (context->instanceArray[id].fsmInfo->transitionTable.aStateEventMatrix[state].stateName)*/
#else
const char* fsm_state_name_by_id(FsmContext *context, CsrUint16 id, CsrUint16 state)
{
    static char statenumber[8];
    CsrSprintf(statenumber, "%d", state);
    return statenumber;
}
#endif

#define fsm_current_state_name_by_id(context,id) fsm_state_name_by_id(context,id, context->instanceArray[id].state)

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void fsm_report_events(FsmContext *context, const char* tagstr, FsmEventList *eventList)
{
    FsmEvent *savedEvent = eventList->first;

    sme_trace_crit((TR_FSM_DUMP, tagstr));

    while (savedEvent != NULL) {
        sme_trace_crit((TR_FSM_DUMP, "       :  %s -> 0x%.4X -> %s",  fsm_process_name_by_id(context, savedEvent->sender_), savedEvent->id, fsm_process_name_by_id(context, savedEvent->destination)));
        savedEvent = savedEvent->next;
    }
}

static void fsm_report_timers(FsmContext *context)
{
    FsmTimer* timer = context->timerQueue.first;
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    sme_trace_crit((TR_FSM_DUMP, "Timer Queue"));

    while (timer != NULL) {
        CsrUint32 remaining = (CsrUint32)CsrTimeSub(timer->timeoutData.timeoutTimeMs, now);
        sme_trace_crit((TR_FSM_DUMP, "   :  0x%.4X %4dms %s -> %s  ", timer->id, (int)remaining, fsm_process_name_by_id(context, timer->sender_), fsm_process_name_by_id(context, timer->destination)));
        timer = timer->next;
    }
}

static void fsm_report_fsm(FsmContext *context, CsrBool fullFatDump, CsrUint16 fsmid)
{
    sme_trace_crit((TR_FSM_DUMP, "%2d : %s -> %s ",  fsmid, fsm_process_name_by_id(context, fsmid), fsm_current_state_name_by_id(context, fsmid)));
    if (fullFatDump && context->instanceArray[fsmid].fsmInfo->dumpFn != NULL)
    {
        (*(context->instanceArray[fsmid].fsmInfo->dumpFn))(context, fsmid);
    }
    if (context->instanceArray[fsmid].savedEventQueue.first != NULL)
    {
        fsm_report_events(context, "   :  Saved Events", &context->instanceArray[fsmid].savedEventQueue);
    }

    /* Last X Transitions */
    if (fullFatDump)
    {
        int i;
        FsmTransitionRecords* records = &context->instanceArray[fsmid].transitionRecords;
        int offset = 0;
        int numrecords = records->numTransitions;
        if (records->numTransitions > FSM_MAX_TRANSITION_HISTORY)
        {
            offset = records->numTransitions % FSM_MAX_TRANSITION_HISTORY;
            numrecords = FSM_MAX_TRANSITION_HISTORY;
        }

        for (i = 0; i < numrecords; ++i)
        {
            FsmTransitionRecord* record = &records->records[(i + offset) % FSM_MAX_TRANSITION_HISTORY];
            const char* toState = fsm_state_name_by_id(context, record->event.destination, record->toState);
#ifdef FSM_DEBUG
            char transitionNameBuff[8];
            const char* transitionName = record->transitionName;
            if (transitionName == NULL)
            {
                transitionName = transitionNameBuff;
                CsrSprintf(transitionNameBuff, "0x%.4X", record->event.id);
            }
#else
            char transitionName[8];
            CsrSprintf(transitionName, "0x%.4X", record->event.id);
#endif

            if (record->transitionFn == fsm_saved_event)
            {
                toState = "Event Saved";
            }
            else if (record->transitionFn == fsm_invalid_event)
            {
                toState = "Invalid Event";
            }
            else if (record->transitionFn == fsm_ignore_event)
            {
                toState = "Event Ignored";
            }

            sme_trace_crit((TR_FSM_DUMP, "   :%6d %2d * %-14s -> %-16s : %-10s -> %-10s",
                                         record->transitionNumber,
                                         record->transitionCount,
                                         fsm_process_name_by_id(context, record->event.sender_),
                                         transitionName,
                                         fsm_state_name_by_id(context, record->event.destination, record->fromState),
                                         toState));
        }
    }
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/


/**
 * See description in fsm/fsm_debug.h
 */
void fsm_debug_dump_internal(FsmContext *context, CsrBool fullFatDump, const char* file, CsrUint32 line)
{
    CsrUint16 i;

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexLock(context->externalEventQueueLock);
#endif
    sme_trace_crit((TR_FSM_DUMP, "+++++++++++++++++++++++ FSM Dump +++++++++++++++++++++++"));
    sme_trace_crit((TR_FSM_DUMP, "Called from %s:%d",  file, (int)line));

    for (i = 0; i < context->maxProcesses; ++i) {
        if (context->instanceArray[i].state != FSM_TERMINATE) {
            fsm_report_fsm(context, fullFatDump, i);
        }
    }

    fsm_report_events(context, "Internal Event Queue", &context->eventQueue );
    fsm_report_events(context, "External Event Queue", &context->externalEventQueue );
    fsm_report_timers(context);

#ifdef FSM_MUTEX_ENABLE
    (void)CsrMutexUnlock(context->externalEventQueueLock);
#endif
}

#endif /* FSM_DEBUG_DUMP */

/**
 * See description in fsm/fsm_debug.h
 */
#ifdef FSM_TEST_SUPPORT
void fsm_fast_forward(FsmContext *context, CsrUint16 ms)
{
    FsmTimer* timer = context->timerQueue.first;
    sme_trace_crit((TR_FSM, "fsm_fast_forward(%d ms)", ms));
    while (timer != NULL) {
        timer->timeoutData.timeoutTimeMs -= ms;
        timer = timer->next;
    }
}
#endif /* FSM_DEBUG */

/** @}
 */

