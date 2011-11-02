/** @file hip_proxy_fsm.c
 *
 * Qos Actions Subfsm Implementation
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2007-2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   QoS Actions Subfsm Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_action_sub_fsm.c#1 $
 *
 ****************************************************************************/

/** @{
 * @ingroup debug_test
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "qos_fsm/qos_action_sub_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "qos_fsm/qos_tspec.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "sys_sap/sys_sap_from_sme_interface.h"


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

typedef void (*qosCompleteFnPtr)(FsmContext* context, const FsmEvent* evt);

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData
{
    qosCompleteFnPtr completeFn;
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
static void qos_action_sub_fsm_init(FsmContext* context)
{
    sme_trace_entry((TR_QOS, "qos_action_sub_fsm_init"));
    fsm_create_params(context, FsmData);
}

/**
 * @brief
 *   qos add cfm
 *
 * @param[in]    context : FSM context
 * @param[in]    cfm     : event
 *
 * @return
 *   void
 */
static void qos_tspec_add_cfm(FsmContext* context, const MlmeAddtsCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_QOS, "qos_tspec_add_cfm()"));

    sme_trace_debug((TR_QOS, " WMM Connection, start the add process %p", fsmData->completeFn ));

    /* Call complete Function and Exit */
    if (fsmData->completeFn)
    {
        qosTspecAddCompleteFnPtr fnCall = (qosTspecAddCompleteFnPtr)fsmData->completeFn;
        fsm_call_sub_instance_complete(context, ((*fnCall)(context, cfm)));
    }

    fsm_next_state(context, FSM_TERMINATE);
}

/**
 * @brief
 *   qos del cfm
 *
 * @param[in]    context : FSM context
 * @param[in]    cfm     : event
 *
 * @return
 *   void
 */
static void qos_tspec_del_cfm(FsmContext* context, const MlmeDeltsCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_QOS, "qos_tspec_del_cfm()"));

    /* Call complete Function and Exit */
    if (fsmData->completeFn)
    {
        qosTspecDelCompleteFnPtr fnCall = (qosTspecDelCompleteFnPtr)fsmData->completeFn;
        fsm_call_sub_instance_complete(context, ((*fnCall)(context, cfm)));
    }

    fsm_next_state(context, FSM_TERMINATE);
}

/**
 * @brief
 *   qos add cfm
 *
 * @param[in]    context : FSM context
 * @param[in]    cfm     : event
 *
 * @return
 *   void
 */
static void qos_tclas_add_cfm(FsmContext* context, const UnifiSysTclasAddCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_QOS, "qos_tclas_add_cfm()"));

    /* Call complete Function and Exit */
    if (fsmData->completeFn)
    {
        qosTclasAddCompleteFnPtr fnCall = (qosTclasAddCompleteFnPtr)fsmData->completeFn;
        fsm_call_sub_instance_complete(context, ((*fnCall)(context, cfm)));
    }

    fsm_next_state(context, FSM_TERMINATE);
}

/**
 * @brief
 *   qos del cfm
 *
 * @param[in]    context : FSM context
 * @param[in]    cfm     : event
 *
 * @return
 *   void
 */
static void qos_tclas_del_cfm(FsmContext* context, const UnifiSysTclasDelCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_QOS, "qos_tclas_del_cfm()"));

    /* Call complete Function and Exit */
    if (fsmData->completeFn)
    {
        qosTclasDelCompleteFnPtr fnCall = (qosTclasDelCompleteFnPtr)fsmData->completeFn;
        fsm_call_sub_instance_complete(context, ((*fnCall)(context, cfm)));
    }

    fsm_next_state(context, FSM_TERMINATE);
}

/* FSM DEFINITION **********************************************/

static const FsmEventEntry cfmTransitions[] =
{
    /*                    Signal Id,                            Function */
    fsm_event_table_entry(MLME_ADDTS_CFM_ID,                    qos_tspec_add_cfm),
    fsm_event_table_entry(MLME_DELTS_CFM_ID,                    qos_tspec_del_cfm),
    fsm_event_table_entry(UNIFI_SYS_TCLAS_ADD_CFM_ID,           qos_tclas_add_cfm),
    fsm_event_table_entry(UNIFI_SYS_TCLAS_DEL_CFM_ID,           qos_tclas_del_cfm),
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
const FsmProcessStateMachine qos_action_sub_fsm = {
#ifdef FSM_DEBUG
       "QOS_ACTION",                                                             /* SM Process Name       */
#endif
       QOS_ACTION_PROCESS,                                                       /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                                         /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),                   /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),                   /* ignore event handers  */
       qos_action_sub_fsm_init,                                                  /* Entry Function        */
       NULL,                                                                     /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                                      /* Trace Dump Function   */
#endif
};

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in hip_proxy_fsm/qos_action_sub_fsm.h
 */
void qos_add_tspec_fsm_start(
        FsmContext* context,
        qosTspecAddCompleteFnPtr completeFn,
        const DataReference dataRef,
        DialogToken dialogToken)
{
    FsmData* fsmData;

    /* Create this sub fsm */
    FsmInstanceEntry* subFsmInstance = fsm_add_sub_instance(context, &qos_action_sub_fsm);
    FsmInstanceEntry* savedFsmInstance  = fsm_save_current_instance(context, subFsmInstance);

    fsmData = FSMDATA;
    fsmData->completeFn = (qosCompleteFnPtr)completeFn;

    sme_trace_debug((TR_QOS, " WMM Connection, start the add process %p", fsmData->completeFn ));

    sme_trace_debug((TR_QOS, "tspec sent"));

    send_mlme_addts_req(context,
                        dataRef,
                        dialogToken,
                        ADD_TSPEC_FAILURE_TIMEOUT);

    fsm_restore_current_instance(context, savedFsmInstance);
}

/**
 * See description in hip_proxy_fsm/qos_action_sub_fsm.h
 */
void qos_del_tspec_fsm_start(
        FsmContext* context,
        qosTspecDelCompleteFnPtr completeFn,
        const CsrUint32 tsinfo)
{
    FsmData* fsmData;
    SmeConfigData* cfg = get_sme_config(context);

    /* Create this sub fsm */
    FsmInstanceEntry* subFsmInstance = fsm_add_sub_instance(context, &qos_action_sub_fsm);
    FsmInstanceEntry* savedFsmInstance  = fsm_save_current_instance(context, subFsmInstance);

    sme_trace_entry((TR_QOS, "qos_del_tspec_fsm_start()"));

    fsmData = FSMDATA;
    fsmData->completeFn = (qosCompleteFnPtr)completeFn;

    /* need to negotiate its removal */
    send_mlme_delts_req(context,
                        cfg->connectionInfo.bssid,
                        tsinfo,
                        ReasonCode_QstaLeaving);

    fsm_restore_current_instance(context, savedFsmInstance);
}

/**
 * See description in hip_proxy_fsm/qos_action_sub_fsm.h
 */
void qos_add_tclas_fsm_start(
        FsmContext* context,
        qosTclasAddCompleteFnPtr completeFn,
        const CsrUint16 tclasLength,
        const CsrUint8 *tclas)
{
    FsmData* fsmData;

    /* Create this sub fsm */
    FsmInstanceEntry* subFsmInstance = fsm_add_sub_instance(context, &qos_action_sub_fsm);
    FsmInstanceEntry* savedFsmInstance  = fsm_save_current_instance(context, subFsmInstance);

    fsmData = FSMDATA;
    fsmData->completeFn = (qosCompleteFnPtr)completeFn;

    sme_trace_debug((TR_QOS, "tclas sent"));

    call_unifi_sys_tclas_add_req(context, tclasLength, tclas);

    fsm_restore_current_instance(context, savedFsmInstance);
}

/**
 * See description in hip_proxy_fsm/qos_action_sub_fsm.h
 */
void qos_del_tclass_fsm_start(
        FsmContext* context,
        qosTclasDelCompleteFnPtr completeFn,
        const CsrUint32 tsinfo)
{
    FsmData* fsmData;
    SmeConfigData* cfg = get_sme_config(context);

    /* Create this sub fsm */
    FsmInstanceEntry* subFsmInstance = fsm_add_sub_instance(context, &qos_action_sub_fsm);
    FsmInstanceEntry* savedFsmInstance  = fsm_save_current_instance(context, subFsmInstance);

    sme_trace_entry((TR_QOS, "qos_del_tspec_fsm_start()"));

    fsmData = FSMDATA;
    fsmData->completeFn = (qosCompleteFnPtr)completeFn;

    /* need to negotiate its removal */
    send_mlme_delts_req(context,
                        cfg->connectionInfo.bssid,
                        tsinfo,
                        ReasonCode_QstaLeaving);

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
