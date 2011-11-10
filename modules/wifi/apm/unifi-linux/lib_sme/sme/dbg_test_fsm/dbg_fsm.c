/** @file dbg_fsm.c
 *
 * Debug FSM Implementation
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (c) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Debug Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/dbg_test_fsm/dbg_fsm.c#2 $
 *
 ****************************************************************************/

/** @{
 * @ingroup debug_test
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_debug.h"
#include "fsm/fsm_internal.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "dbg_test_fsm/dbg_fsm.h"
#include "dbg_sap/dbg_sap_from_sme_interface.h"


#ifdef FAULT_DECODE
const CsrChar* find_fault(CsrUint16 faultNumber);
#endif

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/
/**
 * @brief
 *   FSM States
 *
 * @par Description
 *   Enum defining the FSM States for this FSM process
 */
typedef enum FSMSTATE_idle
{
   FSMSTATE_idle,
   FSMSTATE_MAX_STATE
} FsmState;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/


/**
 * @brief
 *   FSM Entry Function
 *
 * @par Description
 *   Called on startup to initialise the process
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void dbg_init(FsmContext* context)
{
    sme_trace_entry((TR_DBG, "dbg::dbg_init()"));
    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   Handles the debug events
 *
 * @par Description
 *   Use 1 function to handle all the events because the code is simple and the switch
 *   is more size efficient than lots of functions
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void idle_any_event(FsmContext* context, const FsmEvent* req)
{
    sme_trace_entry((TR_DBG, "dbg::idle_any_event()"));

    switch(req->id)
    {
        case DEBUG_STRING_IND_ID:
        {
            DebugStringInd_Evt* castreq = (DebugStringInd_Evt*)req;
            sme_trace_error((TR_DBG, "DebugString_indication :: Should never be used"));
            pld_release(getPldContext(context), castreq->debugMessage.slotNumber);
            break;
        }
        case DEBUG_WORD16_IND_ID:
        {
            sme_trace_warn_code( DebugWord16Ind_Evt* castreq = (DebugWord16Ind_Evt*)req; )
            sme_trace_warn_code( CsrUint32 ts = (castreq->debugWords[6] << 16) | castreq->debugWords[5]; )

#ifdef FAULT_DECODE
            sme_trace_warn((TR_DBG, "MLME FAULT REPORT:  %10lu: %s fault %04x, arg %04x (x%d): '%s'\n",
                           ts,
                           castreq->debugWords[3] == 0x8000 ? "MAC" :
                           castreq->debugWords[3] == 0x4000 ? "PHY" :
                           "???",
                           castreq->debugWords[1],
                           castreq->debugWords[2],
                           castreq->debugWords[4],
                           find_fault(castreq->debugWords[1])));
#else
            sme_trace_warn((TR_DBG, "MLME FAULT REPORT:  %10lu: %s fault %04x, arg %04x (x%d)\n",
                           ts,
                           castreq->debugWords[3] == 0x8000 ? "MAC" :
                           castreq->debugWords[3] == 0x4000 ? "PHY" :
                           "???",
                           castreq->debugWords[1],
                           castreq->debugWords[2],
                           castreq->debugWords[4]));
#endif

            break;
        }
        default:
            break;
    }
}


/**
 * @brief
 *   Handles the debug events
 *
 * @par Description
 *   Use 1 function to handle all the events because the code is simple and the switch
 *   is more size efficient than lots of functions
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void idle_dbg_cmd_req(FsmContext* context, const UnifiDbgCmdReq_Evt* req)
{
    sme_trace_entry((TR_DBG, "dbg::idle_dbg_cmd_req(%s)", req->command));

#ifdef CSR_AMP_ENABLE
    if (CsrStrNCmp(req->command, "palselectchannel", 15)== 0)
    {
        CsrUint8 channel=0;
        char *str = &req->command[17];

        while (*str && *str != '\0' && *str >= '0' && *str <= '9')
        {
            channel *= 10;
            channel += (CsrUint8)((*str) - '0'); /*lint !e571 */
            str++;
        }

        pal_fix_channel(context,channel);
        CsrPfree(req->command);
        return;
    }

    if (CsrStrNCmp(req->command, "paldisableqos", 13)== 0)
    {
        pal_set_qos_support(context,FALSE);
        CsrPfree(req->command);
        return;
    }

    if (CsrStrNCmp(req->command, "palenableqos", 12)== 0)
    {
        pal_set_qos_support(context,TRUE);
        CsrPfree(req->command);
        return;
    }

    if (CsrStrNCmp(req->command, "paldisablesecurity", 18)== 0)
    {
        pal_set_security_support(context,FALSE);
        CsrPfree(req->command);
        return;
    }

    if (CsrStrNCmp(req->command, "palgeneratear", 13)== 0)
    {
        CsrUint32 scheduleKnown=FALSE;
        CsrUint32 startTime=0, duration=0, periodicity=0;
        char *str = &req->command[14];

        while (*str && *str != ',' && *str >= '0' && *str <= '9')
        {
            scheduleKnown *= 10;
            scheduleKnown += (CsrUint32)((*str) - '0'); /*lint !e571 */
            str++;
        }

        if (scheduleKnown)
        {
            str++; /* Skip ',' */
            while (*str && *str != ',' && *str >= '0' && *str <= '9')
            {
                startTime *= 10;
                startTime += (CsrUint32)((*str) - '0'); /*lint !e571 */
                str++;
            }
            str++; /* Skip ',' */

            while (*str && *str != ',' && *str >= '0' && *str <= '9')
            {
                duration *= 10;
                duration += (CsrUint32)((*str) - '0'); /*lint !e571 */
                str++;
            }
            str++; /* Skip ',' */

            while (*str && *str >= '0' && *str <= '9')
            {
                periodicity *= 10;
                periodicity += (CsrUint32)((*str) - '0'); /*lint !e571 */
                str++;
            }
        }

        pal_generate_ar(context,(CsrBool)scheduleKnown, startTime, duration, periodicity);
        CsrPfree(req->command);
        return;
    }

#endif

    if (CsrStrNCmp(req->command, "fsm_dump_lite", 13)== 0)
    {
        fsm_debug_dump_lite(context);
        CsrPfree(req->command);
        return;
    }
    if (CsrStrNCmp(req->command, "fsm_dump", 8)== 0)
    {
        fsm_debug_dump(context);
        CsrPfree(req->command);
        return;
    }

#ifdef FSM_TEST_SUPPORT
    if (CsrStrNCmp(req->command, "osa_fast_forward:", 17)== 0)
    {
        char* str = &req->command[17];
        CsrUint32 ms  = 0;

        while (*str && *str >= '0' && *str <= '9')
        {
            ms *= 10;
            ms += (CsrUint32)((*str) - '0'); /*lint !e571 */
            str++;
        }
        ms &= 0x7FFFFFFF;   /* cannot reliably forward time by more than (2^31 - 1) */

        /* if zero time, advance time to whenever next timer would expire */
        if (ms == 0)
        {
            FsmTimerData timeoutData;

            timeoutData = fsm_get_next_timeout(context);
            ms = timeoutData.timeoutTimeMs;
        }
        sme_trace_warn((TR_DBG, "cmd: %s, Calling osa_advance_time(%d)", req->command, ms));
        fsm_test_advance_time(context, ms);
        CsrPfree(req->command);
        return;
    }

    if (CsrStrNCmp(req->command, "fsm_fast_forward:", 17)== 0)
    {
        char* str = &req->command[17];
        CsrUint32 ms  = 0;

        while (*str && *str >= '0' && *str <= '9')
        {
            ms *= 10;
            ms += (CsrUint32)((*str) - '0'); /*lint !e571 */
            str++;
        }
        ms &= 0x7FFFFFFF;   /* cannot reliably forward time by more than (2^31 - 1) */

        /* if zero time, advance time to whenever next timer would expire */
        if (ms == 0)
        {
            FsmTimerData timeoutData;

            timeoutData = fsm_get_next_timeout(context);
            ms = timeoutData.timeoutTimeMs;
        }

        if (ms > 0x0000FFFF)
        {
            sme_trace_warn((TR_DBG, "cmd: %s, FSM time advance is limited to a 16-bit quantity", req->command));
        }
        sme_trace_warn((TR_DBG, "cmd: %s, Calling fsm_fast_forward(%d)", req->command, (CsrUint16)ms));
        fsm_fast_forward(context, (CsrUint16)ms);
        CsrPfree(req->command);
        return;
    }
#endif

    sme_trace_warn((TR_DBG, "unhandled command (%s)", req->command));
    CsrPfree(req->command);
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/* FSM DEFINITION **********************************************/

/** State idle transitions */
static const FsmEventEntry idleTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(DEBUG_STRING_IND_ID,                          idle_any_event ),
    fsm_event_table_entry(DEBUG_WORD16_IND_ID,                          idle_any_event ),
    fsm_event_table_entry(UNIFI_DBG_CMD_REQ_ID,                         idle_dbg_cmd_req ),
    fsm_event_table_entry(DEBUG_GENERIC_IND_ID,                         fsm_ignore_event ),
    fsm_event_table_entry(DEBUG_GENERIC_CFM_ID,                         fsm_ignore_event ),
};

/** Debug Test state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                    State                        State                         Save    */
   /*                    Name                         Transitions                    *      */
   fsm_state_table_entry(FSMSTATE_idle,               idleTransitions,              FALSE),
};

const FsmProcessStateMachine dbg_fsm = {
#ifdef FSM_DEBUG
       "DBG",                                     /* SM Process Name       */
#endif
       DBG_PROCESS,                               /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},          /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),    /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),    /* ignore event handers */
       dbg_init,                                 /* Entry Function        */
       NULL,                                                     /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                      /* Trace Dump Function   */
#endif
};

/*
 * FSM Scripts Config for this FSM
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/** @}
 */
