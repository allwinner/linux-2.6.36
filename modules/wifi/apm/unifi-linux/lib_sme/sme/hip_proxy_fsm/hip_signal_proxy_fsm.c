/** @file hip_signal_proxy_fsm.c
 *
 * Hip Signal Proxy FSM Implementation
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
 *   Hip Signal Proxy Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/hip_signal_proxy_fsm.c#3 $
 *
 ****************************************************************************/

/** @{
 * @ingroup hip_proxy
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "hip_proxy_fsm/hip_signal_proxy_fsm.h"
#include "sys_sap/sys_sap_from_sme_interface.h"
#include "sys_sap/sme_interface_hip_signal_to_sys_sap.h"

#include "mgt_sap/mgt_sap_from_sme_interface.h"

#ifdef CCX_VARIANT
#include "ccx_iapp_msg_handler/ccx_iapp_msg_handler.h"
#endif

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Processes Custom data
 *
 * @par Description
 *   see brief
 */
#define PROXYDATA (fsm_get_params(context, ProxyData))

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/
/**
 * @brief
 *   FSM States
 *
 * @par Description
 *   Enum defining the FSM States for this FSM process
 */
typedef enum FsmState
{
   FSMSTATE_idle,
   FSMSTATE_MAX_STATE
} FsmState;

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct ProxyData
{
    /** storage for the current sender ID */
    CsrUint16 req_sender;

    /** Registered Security Manager */
    CsrUint16 securityMgrInstance;

    /** Registered Connection Manager */
    CsrUint16 connectionMgrInstance;

    /** Registered Security Manager's Mac Address */
    unifi_MACAddress macAddress;

} ProxyData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Security Manager Process FSM Entry Function
 *
 * @par Description
 *   Called on Security Manager Process startup to initialise
 *   the security manager data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void hipProxyInit(FsmContext* context)
{
    sme_trace_entry((TR_HIP_PROXY, "hipProxyInit()"));
    fsm_create_params(context, ProxyData);

    PROXYDATA->securityMgrInstance = FSM_TERMINATE;
    PROXYDATA->connectionMgrInstance = FSM_TERMINATE;
    PROXYDATA->req_sender = FSM_TERMINATE;

    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   Handle security events
 *
 * @par Description
 *   Forward the security events to the correct security manager instance
 *
 * @param[in]    context   : FSM context
 * @param[in]    evt       : event
 *
 * @return
 *   void
 */
static void forward_to_security_mgr(FsmContext* context, const FsmEvent* pEvt)
{
    sme_trace_entry((TR_HIP_PROXY, "hipProxy::*::forward_to_security_mgr(%d)",PROXYDATA->securityMgrInstance));

    if (PROXYDATA->securityMgrInstance != FSM_TERMINATE)
    {
        fsm_forward_event(context, PROXYDATA->securityMgrInstance, pEvt);
    }
    else
    {
        /* call ignore to auto clean up leaky events */
        fsm_ignore_event(context, pEvt);
    }
}

/**
 * @brief
 *   Handle connection events
 *
 * @par Description
 *   Forward the connetion events to the correct connection manager instance
 *
 * @param[in]    context   : FSM context
 * @param[in]    evt       : event
 *
 * @return
 *   void
 */
static void forward_to_connection_mgr(FsmContext* context, const FsmEvent* pEvt)
{
    sme_trace_entry((TR_HIP_PROXY, "hipProxy::*::forward_to_connection_mgr(0x%.4X)", PROXYDATA->connectionMgrInstance));

    if (PROXYDATA->connectionMgrInstance != FSM_TERMINATE)
    {
        fsm_forward_event(context, PROXYDATA->connectionMgrInstance, pEvt);
    }
    else if (pEvt->id == MLME_CONNECTED_IND_ID)
    {
        /* Forward to unifi_driver_fsm.  This is needed as that FSM will
         * wait during startup for the first connected-ind to arrive,
         * signifying that the UniFi firmware is ready to use.  Any stray
         * connected-ind after that are ignored by unifi_manager_fsm, as
         * they would be here.
         */
        fsm_forward_event(context, getSmeContext(context)->unifiDriverInstance, pEvt);
    }
#ifdef CSR_AMP_ENABLE
    else if (pEvt->id == MLME_DEAUTHENTICATE_IND_ID)
    {
        /* Forward to the AmpPal Subsystem */
        fsm_forward_event(context, getSmeContext(context)->palMgrFsmInstance, pEvt);
    }
    else if (pEvt->id == UNIFI_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID ||
             pEvt->id == UNIFI_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID)
    {
        /* Forward to the AmpPal Subsystem */
        fsm_forward_event(context, getSmeContext(context)->palDmFsmInstance, pEvt);
    }
    else if (pEvt->id == UNIFI_SYS_MA_UNITDATA_IND_ID ||
             pEvt->id == UNIFI_SYS_EAPOL_CFM_ID ||
             pEvt->id == UNIFI_SYS_MA_UNITDATA_CFM_ID)
    {
        /* Forward to the AmpPal Subsystem */
        fsm_forward_event(context, getSmeContext(context)->palMgrFsmInstance, pEvt);
    }
#endif
    else
    {
        /* call ignore to auto clean up leaky events */
        fsm_ignore_event(context, pEvt);
    }
}

/**
 * @brief
 *   Handle security events
 *
 * @par Description
 *   Forward the security events to the correct security manager instance
 *
 * @param[in]    context   : FSM context
 * @param[in]    evt       : event
 *
 * @return
 *   void
 */
static void decode_and_forward(FsmContext* context, const MaUnitdataInd_Evt* ind)
{
    PldHdl hnd = (PldHdl)ind->data.slotNumber;
    CsrUint8 *pBuf;
    CsrUint16 length;

    sme_trace_entry((TR_HIP_PROXY, "hipProxy::*::decode_and_forward(event = 0x%.4X)",ind->common.id ));

    pld_access(getPldContext(context), hnd, (void**)&pBuf, &length);

#ifdef CCX_VARIANT
    if (ccx_iapp_identify_iapp_packet(context, pBuf, length))
    {
        CsrUint16 instanceId;
        if (ccx_iapp_process_iapp_destination(context, pBuf, &instanceId))
        {
            sme_trace_entry((TR_HIP_PROXY, "hipProxy::fsm_forward_event" ));
            fsm_forward_event(context, instanceId, (FsmEvent*)ind);
        }
    }
#endif
#ifdef CCX_VARIANT
#ifdef CSR_AMP_ENABLE
    else
#endif
#endif
#ifdef CSR_AMP_ENABLE
    {
        fsm_forward_event(context, getSmeContext(context)->palMgrFsmInstance, (FsmEvent*)ind);
    }
#endif
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in hip_proxy_fsm/hip_signal_proxy_fsm.h
 */
void hip_proxy_register_security_manager(FsmContext* context, CsrUint16 securityMgrInstance, unifi_MACAddress macAddress)
{
    ProxyData* fsmData;
    sme_trace_entry((TR_HIP_PROXY, "hip_proxy_register_security_manager(0x%.4X)", securityMgrInstance));
    require(TR_SECURITY, securityMgrInstance < context->maxProcesses);
    require(TR_SECURITY, context->instanceArray[securityMgrInstance].fsmInfo->processId == SECURITY_MANAGER_PROCESS);

    fsmData = fsm_get_params_by_id(context, getSmeContext(context)->hipProxyInstance, ProxyData);

    fsmData->securityMgrInstance = securityMgrInstance;
    fsmData->macAddress = macAddress;
}

/**
 * See description in hip_proxy_fsm/hip_signal_proxy_fsm.h
 */
CsrUint16 hip_proxy_get_security_manager(FsmContext* context)
{
    sme_trace_entry((TR_HIP_PROXY, "hip_proxy_get_security_manager()"));

    return (fsm_get_params_by_id(context, getSmeContext(context)->hipProxyInstance, ProxyData))->securityMgrInstance;
}

/**
 * See description in hip_proxy_fsm/hip_signal_proxy_fsm.h
 */
void hip_proxy_deregister_security_manager(FsmContext* context, CsrUint16 securityMgrInstance)
{
    ProxyData* fsmData = (fsm_get_params_by_id(context, getSmeContext(context)->hipProxyInstance, ProxyData));
    sme_trace_entry((TR_HIP_PROXY, "hip_proxy_deregister_security_manager(0x%.4X)", securityMgrInstance));
    require(TR_SECURITY, securityMgrInstance < context->maxProcesses);
    require(TR_SECURITY, context->instanceArray[securityMgrInstance].fsmInfo->processId == SECURITY_MANAGER_PROCESS);

    /* Ignore deregister if it if not from the registered security instance */
    if (fsmData->securityMgrInstance != securityMgrInstance) return;

    fsm_flush_saved_events_from(context, getSmeContext(context)->hipProxyInstance, securityMgrInstance);

    if (fsmData->req_sender == securityMgrInstance)
    {
        fsmData->req_sender = FSM_TERMINATE;
    }
    fsmData->securityMgrInstance = FSM_TERMINATE;
}


/**
 * See description in hip_proxy_fsm/hip_signal_proxy_fsm.h
 */
void hip_proxy_register_connection_manager(FsmContext* context, CsrUint16 connectionMgrInstanceId)
{
    sme_trace_entry((TR_HIP_PROXY, "hip_proxy_register_connection_manager(0x%.4X)", connectionMgrInstanceId));

    require(TR_SECURITY, connectionMgrInstanceId < context->maxProcesses);

    (fsm_get_params_by_id(context, getSmeContext(context)->hipProxyInstance, ProxyData))->connectionMgrInstance = connectionMgrInstanceId;
}

/**
 * See description in hip_proxy_fsm/hip_signal_proxy_fsm.h
 */
void hip_proxy_deregister_connection_manager(FsmContext* context, CsrUint16 connectionMgrInstanceId)
{
    ProxyData* fsmData = (fsm_get_params_by_id(context, getSmeContext(context)->hipProxyInstance, ProxyData));
    sme_trace_entry((TR_HIP_PROXY, "hip_proxy_deregister_connection_manager(0x%.4X)", connectionMgrInstanceId));

    require(TR_SECURITY, connectionMgrInstanceId < context->maxProcesses);

    /* Ignore deregister if it if not from the registered connection instance */
    if (fsmData->connectionMgrInstance != connectionMgrInstanceId) return;

    fsm_flush_saved_events_from(context, getSmeContext(context)->hipProxyInstance, connectionMgrInstanceId);

    if (fsmData->req_sender == connectionMgrInstanceId)
    {
        fsmData->req_sender = FSM_TERMINATE;
    }
    fsmData->connectionMgrInstance = FSM_TERMINATE;
}

/* FSM DEFINITION **********************************************/

static const FsmEventEntry idleTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(MA_UNITDATA_IND_ID,                           decode_and_forward ),
    fsm_event_table_entry(MLME_PROTECTEDFRAMEDROPPED_IND_ID,            forward_to_security_mgr ),
    fsm_event_table_entry(MLME_MICHAELMICFAILURE_IND_ID,                forward_to_security_mgr ),
    fsm_event_table_entry(UNIFI_SYS_M4_TRANSMITTED_IND_ID,              forward_to_security_mgr ),
    fsm_event_table_entry(UNIFI_SYS_PORT_CONFIGURE_CFM_ID,              forward_to_security_mgr ),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                        forward_to_connection_mgr),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,                   forward_to_connection_mgr),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,                     forward_to_connection_mgr),
    fsm_event_table_entry(UNIFI_SYS_QOS_CONTROL_CFM_ID,                 forward_to_connection_mgr),

#ifdef CSR_AMP_ENABLE
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,       forward_to_connection_mgr),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID,     forward_to_connection_mgr),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_IND_ID,                 forward_to_connection_mgr),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_CFM_ID,                 forward_to_connection_mgr),
    fsm_event_table_entry(UNIFI_SYS_EAPOL_CFM_ID,                       forward_to_connection_mgr),
#endif
};

/** Hip Signal Proxy state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                         State                             State                                      Save     */
   /*                         Name                              Transitions                                 *       */
   fsm_state_table_entry(FSMSTATE_idle,                         idleTransitions,                          FALSE),
};

const FsmProcessStateMachine hip_signal_proxy_fsm = {
#ifdef FSM_DEBUG
       "Hip Proxy",                                                             /* SM Process Name       */
#endif
       HIP_PROXY_PROCESS,                                                       /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                                        /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),                  /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                   /* ignore event handers */
       hipProxyInit,                                                            /* Entry Function        */
       NULL,                                                                    /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                                     /* Trace Dump Function   */
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
