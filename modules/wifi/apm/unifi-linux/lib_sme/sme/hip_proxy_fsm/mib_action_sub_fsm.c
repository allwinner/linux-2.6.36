/** @file hip_proxy_fsm.c
 *
 * Mib Actions Subfsm Implementation
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2007-2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   Mib Actions Subfsm Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mib_action_sub_fsm.c#1 $
 *
 ****************************************************************************/

/** @{
 * @ingroup debug_test
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "hip_proxy_fsm/mib_action_sub_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"


/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process's Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, FsmData))

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/*lint -e749 */
/**
 * @brief
 *   FSM States
 *
 * @par Description
 *   Enum defining the FSM States for this FSM process
 */
typedef enum
{
    FSMSTATE_actioning,
    FSMSTATE_MAX_STATE
} FsmState;

typedef void (*mibCompleteFnPtr)(FsmContext* context, const FsmEvent* evt);

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData
{
    mibCompleteFnPtr completeFn;
} FsmData;

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
static void mib_action_sub_fsm_init(FsmContext* context)
{
    sme_trace_entry((TR_MIB_ACCESS_FSM, "mib_action_sub_fsm_init"));
    fsm_create_params(context, FsmData);
}

/**
 * @brief
 *   mib get
 *
 * @param[in]    context : FSM context
 * @param[in]    cfm     : event
 *
 * @return
 *   void
 */
static void mib_get_cfm(FsmContext* context, const MlmeGetCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "mib_get_cfm()"));

    /* Call complete Function and Exit */
    if (fsmData->completeFn)
    {
        mibGetCompleteFnPtr fnCall = (mibGetCompleteFnPtr)fsmData->completeFn;
        fsm_call_sub_instance_complete(context, ((*fnCall)(context, cfm)));
    }

    fsm_next_state(context, FSM_TERMINATE);
}

/**
 * @brief
 *   mib get next
 *
 * @param[in]    context : FSM context
 * @param[in]    cfm     : event
 *
 * @return
 *   void
 */
static void mib_get_next_cfm(FsmContext* context, const MlmeGetNextCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "mib_get_next_cfm()"));

    /* Call complete Function and Exit */
    if (fsmData->completeFn)
    {
        mibGetNextCompleteFnPtr fnCall = (mibGetNextCompleteFnPtr)fsmData->completeFn;
        fsm_call_sub_instance_complete(context, ((*fnCall)(context, cfm)));
    }

    fsm_next_state(context, FSM_TERMINATE);
}


/**
 * @brief
 *   cdl mib settings set
 *
 * @param[in]    context : FSM context
 * @param[in]    cfm     : event
 *
 * @return
 *   void
 */
static void mib_set_cfm(FsmContext* context, const MlmeSetCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "mib_set_cfm()"));

    /* Call complete Function and Exit */
    if (fsmData->completeFn)
    {
        mibSetCompleteFnPtr fnCall = (mibSetCompleteFnPtr)fsmData->completeFn;
        fsm_call_sub_instance_complete(context, ((*fnCall)(context, cfm)));
    }

    fsm_next_state(context, FSM_TERMINATE);
}

/* FSM DEFINITION **********************************************/

static const FsmEventEntry cfmTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(MLME_GET_CFM_ID,                              mib_get_cfm),
    fsm_event_table_entry(MLME_GET_NEXT_CFM_ID,                         mib_get_next_cfm),
    fsm_event_table_entry(MLME_SET_CFM_ID,                              mib_set_cfm),
};

/** Debug Test state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
    /*                    State                        State                         Save    */
    /*                    Name                         Transitions                    *      */
        fsm_state_table_entry(FSMSTATE_actioning,      cfmTransitions,              FALSE ),
};

/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the state machine data
 */
const FsmProcessStateMachine mib_action_sub_fsm = {
#ifdef FSM_DEBUG
       "MIB_ACTION",                                                             /* SM Process Name       */
#endif
       MIB_ACTION_PROCESS,                                                       /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                                         /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),                   /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),                   /* ignore event handers  */
       mib_action_sub_fsm_init,                                                  /* Entry Function        */
       NULL,                                                                     /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                                      /* Trace Dump Function   */
#endif
};

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in hip_proxy_fsm/mib_action_sub_fsm.h
 */
void mib_set_sub_fsm_start(FsmContext* context, mibSetCompleteFnPtr completeFn, const DataReference* dataRef, CsrBool sendToHipSap)
{
    FsmData* fsmData;

    /* Create this sub fsm */
    FsmInstanceEntry* subFsmInstance = fsm_add_sub_instance(context, &mib_action_sub_fsm);
    FsmInstanceEntry* savedFsmInstance  = fsm_save_current_instance(context, subFsmInstance);

    sme_trace_entry((TR_MIB_ACCESS_FSM, "mib_set_sub_fsm_start()"));

    fsmData = FSMDATA;
    fsmData->completeFn = (mibCompleteFnPtr)completeFn;

    if (sendToHipSap)
    {
        send_mlme_set_req(context, *dataRef);
    }
    else
    {
        /* Send Mib Event */
        send_mlme_set_req_internal(context, getSmeContext(context)->mibAccessInstance, *dataRef);
    }

    fsm_restore_current_instance(context, savedFsmInstance);
}


/**
 * See description in hip_proxy_fsm/mib_action_sub_fsm.h
 */
void mib_get_sub_fsm_start(FsmContext* context, mibGetCompleteFnPtr completeFn, const DataReference* dataRef, CsrBool sendToHipSap)
{
    FsmData* fsmData;

    /* Create this sub fsm */
    FsmInstanceEntry* subFsmInstance = fsm_add_sub_instance(context, &mib_action_sub_fsm);
    FsmInstanceEntry* savedFsmInstance  = fsm_save_current_instance(context, subFsmInstance);

    sme_trace_entry((TR_MIB_ACCESS_FSM, "mib_get_sub_fsm_start()"));

    fsmData = FSMDATA;
    fsmData->completeFn = (mibCompleteFnPtr)completeFn;

    if (sendToHipSap)
    {
        send_mlme_get_req(context, *dataRef);
    }
    else
    {
        /* Send Mib Event */
        send_mlme_get_req_internal(context, getSmeContext(context)->mibAccessInstance, *dataRef);
    }

    fsm_restore_current_instance(context, savedFsmInstance);
}

/**
 * See description in hip_proxy_fsm/mib_action_sub_fsm.h
 */
void mib_getnext_sub_fsm_start(FsmContext* context, mibGetNextCompleteFnPtr completeFn, const DataReference* dataRef, CsrBool sendToHipSap)
{
    FsmData* fsmData;

    /* Create this sub fsm */
    FsmInstanceEntry* subFsmInstance    = fsm_add_sub_instance(context, &mib_action_sub_fsm);
    FsmInstanceEntry* savedFsmInstance  = fsm_save_current_instance(context, subFsmInstance);

    sme_trace_entry((TR_MIB_ACCESS_FSM, "mib_getnext_sub_fsm_start()"));

    fsmData = FSMDATA;
    fsmData->completeFn = (mibCompleteFnPtr)completeFn;

    if (sendToHipSap)
    {
        send_mlme_get_next_req(context, *dataRef);
    }
    else
    {
        /* Send Mib Event */
        send_mlme_get_next_req_internal(context, getSmeContext(context)->mibAccessInstance, *dataRef);
    }

    fsm_restore_current_instance(context, savedFsmInstance);
}

/*
 * FSM Scripts Config for this FSM
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/** @}
 */
