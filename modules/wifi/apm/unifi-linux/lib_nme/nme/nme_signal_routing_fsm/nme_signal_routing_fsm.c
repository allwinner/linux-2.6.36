/** @file nme_signal_routing_fsm.c
 *
 * Network Management Entity Signal Routing FSM Implementation
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009-2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   NME Signal Routing FSM Implementation
 *   Some NME-SAP functions map straight on to an equivalent SME MGT-SAP
 *   function, whilst others require specific functionality in the NME.
 *   For the moment those that map on to the SME MGT-SAP will be "forwarded"
 *   by this FSM. Some primitives also have to be "forwarded" in the opposite
 *   direction, these will also be handled in this FSM (for the moment).
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_signal_routing_fsm/nme_signal_routing_fsm.c#1 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_signal_routing
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "nme_top_level_fsm/nme_top_level_fsm.h"

/* MACROS *******************************************************************/
#define FSMDATA (fsm_get_params(pContext, FsmData))

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
typedef struct FsmData
{

    /* control block of structures for routing primitives both internal and external */
    RoutingCtrlblk routingCtrlblk;

    /* As we are sending events outside of the NME from dynamic FSMs
     * that we need to determine whether the response is been received
     * by the same FSM or a new instance of the FSM, we need to assign
     * a unique reference to the dynamic FSM instances to use as a
     * message identifier.
     */
    CsrUint8 dynamicFsmMsgId[NME_NUM_OF_DYN_FSM];
} FsmData;

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/
/**
 * @brief
 *   Forwards the received CSR_WIFI_NME request to the UNIFI_MGT SAP
 *
 * @par Description
 *   Forwards the received CSR_WIFI_NME request to the UNIFI_MGT SAP
 *
 *   Actually just a local wrapper function that needs to be used to
 *   keep the FSM State diagram utils happy as they expect each
 *   state transition to be a function in this file.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_signal_routing_forward_to_unifi_mgt_sap(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    nme_forward_to_unifi_mgt_sap(pContext, pReq);
}

/**
 * @brief
 *   Forwards the received UNIFI_MGT SAP event to the CSR_WIFI_NME SAP
 *
 * @par Description
 *   Forwards the received UNIFI_MGT event to the CSR_WIFI_NME SAP.
 *
 *   Actually just a local wrapper function that needs to be used to
 *   keep the FSM State diagram utils happy as they expect each
 *   state transition to be a function in this file.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_signal_routing_forward_to_csr_wifi_nme_sap(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    nme_forward_to_csr_wifi_nme_sap(pContext, pEvt);
}


/**
 * @brief
 *   Process a SCAN_RESULTS_GET_CFM.
 *
 * @par Description
 *   Forward the SCAN_RESULTS_GET_CFM event to the CSR_WIFI_NME SAP.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_signal_routing_scan_results_get_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
#ifdef CCX_VARIANT
    const UnifiNmeScanResultsGetCfm_Evt* pCfm = (UnifiNmeScanResultsGetCfm_Evt*)pEvt;
#endif
    sme_trace_entry((TR_NME_SIG_FSM, "nme_core_scan_results_get_cfm()"));

    /* place immediately in external event queue */
    nme_signal_routing_forward_to_csr_wifi_nme_sap(pContext, pEvt);

#ifdef CCX_VARIANT
    /* check for ssidl */
    nme_ccx_process_scanresults_for_ssidl(pContext, pCfm->scanResults, pCfm->scanResultsCount);
#endif

}

/**
 * @brief
 *   Process a SCAN_RESULTS_GET_CFM.
 *
 * @par Description
 *   Forward the SCAN_RESULTS_GET_CFM event to the CSR_WIFI_NME SAP.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_signal_routing_scan_result_ind(FsmContext* pContext, const UnifiNmeScanResultInd_Evt* ind)
{
    sme_trace_entry((TR_NME_SIG_FSM, "nme_core_scan_result_ind_cfm()"));

    nme_forward_to_csr_wifi_nme_sap(pContext, (const FsmEvent*)ind);

    fsm_forward_event(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance, (const FsmEvent*)ind);

#ifdef CCX_VARIANT
    /* check for ssidl */
/*    nme_ccx_process_scanresults_for_ssidl(pContext, (unifi_ScanResult*)&pInd->result, 1);
 */
#endif

}



/**
 * @brief
 *   Process a UNIFI_NME_EVENT_MASK_SET_REQ
 *
 * @par Description
 *   The NME has already registered to receive all indications
 *   from the SME, so this request is not forwarded to the SME.
 *   Rather the NME records which indications are registered against
 *   a particular app handle and then on getting an indication from
 *   the SME will forward it to all those app handles that registered
 *   to receive it.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeEventMaskSetReq_Evt* : UNIFI NME EVENT MASK SET REQ event
 *
 * @return
 *   void
 */
static void nme_signal_routing_event_mask_set_req(
        FsmContext* pContext,
        const UnifiNmeEventMaskSetReq_Evt* pEventMaskSetReq)
{
    FsmData *pFsmData = FSMDATA;
    RoutingCtrlblk *pRoutingCtrlblk = &(pFsmData->routingCtrlblk);
    CsrBool foundExisting = FALSE;
    CsrUint8 i;

    sme_trace_entry((TR_NME_SIG_FSM, "event_mask_set_req(%p, 0x%.4X) Size = %d Count = %d",
            pEventMaskSetReq->appHandle,
            pEventMaskSetReq->indMask,
            pRoutingCtrlblk->indRegistrationSize,
            pRoutingCtrlblk->indRegistrationCount));

    for (i = 0; i < pRoutingCtrlblk->indRegistrationCount; ++i)
    {
        if (pEventMaskSetReq->appHandle == pRoutingCtrlblk->indRegistrations[i].appHandle)
        {
            if (unifi_IndNone == pEventMaskSetReq->indMask)
            {
                sme_trace_debug((TR_NME_SIG_FSM, "Existing entry - Removed"));
                /* Move the rest of the list Up 1  */
                CsrMemMove(&pRoutingCtrlblk->indRegistrations[i],
                            &pRoutingCtrlblk->indRegistrations[i+1],
                            ((pRoutingCtrlblk->indRegistrationCount  - i) - 1)*(sizeof(NmeIndRegistration)));
                pRoutingCtrlblk->indRegistrationCount--;
            }
            else
            {
                sme_trace_debug((TR_NME_SIG_FSM, "Existing entry - Updated"));
                pRoutingCtrlblk->indRegistrations[i].indMask = pEventMaskSetReq->indMask;
            }
            foundExisting = TRUE;
            break;
        }
    }

    if (!foundExisting && unifi_IndNone != pEventMaskSetReq->indMask)
    {
        sme_trace_debug((TR_NME_SIG_FSM, "New entry - Added"));
        /* Add new entry */
        if ((pRoutingCtrlblk->indRegistrationCount * 4) == pRoutingCtrlblk->indRegistrationSize)
        {
            /* Need to allocate more memory */
            NmeIndRegistration* newIndRegistrations = CsrPmalloc((pRoutingCtrlblk->indRegistrationSize + 4) * sizeof(NmeIndRegistration));
            pRoutingCtrlblk->indRegistrationSize += 4;

            if (pRoutingCtrlblk->indRegistrationCount > 0)
            {
                /* Copy existing data */
                CsrMemCpy(newIndRegistrations, pRoutingCtrlblk->indRegistrations, pRoutingCtrlblk->indRegistrationCount * sizeof(NmeIndRegistration));
                CsrPfree(pRoutingCtrlblk->indRegistrations);
            }
            pRoutingCtrlblk->indRegistrations = newIndRegistrations;
        }
        pRoutingCtrlblk->indRegistrations[pRoutingCtrlblk->indRegistrationCount].appHandle = pEventMaskSetReq->appHandle;
        pRoutingCtrlblk->indRegistrations[pRoutingCtrlblk->indRegistrationCount].indMask   = pEventMaskSetReq->indMask;
        pRoutingCtrlblk->indRegistrationCount++;
    }
    call_unifi_nme_event_mask_set_cfm(pContext, pEventMaskSetReq->appHandle, unifi_Success);
}


/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   FSM data function
 *
 * @param[in] FsmContext*  : FSM context
 *
 * @return
 *   void
 */
static void nme_signal_routing_init_data(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    CsrUint8 i;

    sme_trace_entry((TR_NME_SIG_FSM, "nme_signal_routing_init_data()"));
    nme_routing_context_initialize(pContext, &(pFsmData->routingCtrlblk));

    for (i = 0; i< NME_NUM_OF_DYN_FSM; i++)
    {
        pFsmData->dynamicFsmMsgId[i] = 0;
    }
}

/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   Initial FSM Entry function
 *
 * @param[in]  FsmContext*   : FSM context
 *
 * @return
 *   void
 */
static void nme_signal_routing_init_fsm(FsmContext* pContext)
{
    sme_trace_entry((TR_NME_SIG_FSM, "nme_signal_routing_init_fsm()"));
    fsm_create_params(pContext, FsmData);
    nme_signal_routing_init_data(pContext);

    /* Request that the NME receives all indications */
    /* Not defining routing info as this FSM will receive the event rather than routing via the forward functions. */
    call_unifi_nme_mgt_event_mask_set_req(pContext, NME_APP_HANDLE(pContext), unifi_IndAll);

    fsm_next_state(pContext, FSMSTATE_idle);
}

/**
 * @brief
 *   FSM data reset
 *
 * @par Description
 *   reset FSM function
 *
 * @param[in]  FsmContext*  : FSM context
 *
 * @return
 *   void
 */
static void nme_signal_routing_reset_fsm(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_SIG_FSM, "nme_signal_routing_reset_fsm()"));
    nme_routing_context_reset(pContext, &(pFsmData->routingCtrlblk));
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * See description in nme_signal_routing_fsm/nme_signal_routing_fsm.h
 */
RoutingCtrlblk* nme_signal_routing_get_ctrlblk_context(FsmContext* pContext)
{
    FsmData *fsmData = (fsm_get_params_by_id(pContext, getNmeContext(pContext)->nmeSignalRoutingFsmInstance, FsmData));
    sme_trace_entry((TR_NME_SIG_FSM, "nme_core_get_routing_ctrlblk_context() "));
    return &(fsmData->routingCtrlblk);
}

/*
 * See description in nme_signal_routing_fsm/nme_signal_routing_fsm.h
 */
CsrUint8 nme_signal_routing_get_dyn_fsm_msg_id(
        FsmContext* pContext,
        const CsrUint8 dynFsm)
{
    FsmData *fsmData = (fsm_get_params_by_id(pContext, getNmeContext(pContext)->nmeSignalRoutingFsmInstance, FsmData));
    ++fsmData->dynamicFsmMsgId[dynFsm];
    /* Treat 0 as an invalid instance msg id */
    if (0 == fsmData->dynamicFsmMsgId[dynFsm])
    {
        fsmData->dynamicFsmMsgId[dynFsm] = 1;
    }
    return(fsmData->dynamicFsmMsgId[dynFsm]);
}
/* FSM DEFINITION **********************************************/

static const FsmEventEntry idleTransitions[] =
{
    /*                    Signal Id,                                 Function */

    /* -------------------------         Events from the NME SAP          ------------------------- */

    /* Requests that need to be forwarded to the SME MGT SAP */
    fsm_event_table_entry(UNIFI_NME_WIFI_ON_REQ_ID,                  nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_WIFI_OFF_REQ_ID,                 nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_WIFI_FLIGHTMODE_REQ_ID,          nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_SET_VALUE_REQ_ID,                nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_GET_VALUE_REQ_ID,                nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_MIB_SET_REQ_ID,                  nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_MIB_GET_REQ_ID,                  nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_MIB_GET_NEXT_REQ_ID,             nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_SCAN_FULL_REQ_ID,                nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_SCAN_RESULTS_GET_REQ_ID,         nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_MULTICAST_ADDRESS_REQ_ID,        nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_PMKID_REQ_ID,                    nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_KEY_REQ_ID,                      nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_PACKET_FILTER_SET_REQ_ID,        nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_TSPEC_REQ_ID,                    nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_SCAN_RESULTS_FLUSH_REQ_ID,       nme_signal_routing_forward_to_unifi_mgt_sap),
    fsm_event_table_entry(UNIFI_NME_BLACKLIST_REQ_ID,                nme_signal_routing_forward_to_unifi_mgt_sap),

    /* NME requires all indication events from the SME so application event mask set requests
     * are recorded in the NME and only the required indications are passed upwards.
     */
    fsm_event_table_entry(UNIFI_NME_EVENT_MASK_SET_REQ_ID,           nme_signal_routing_event_mask_set_req),

    /* -------------------------         Events from the MGT SAP          ------------------------- */

    fsm_event_table_entry(UNIFI_NME_MGT_WIFI_ON_CFM_ID,              nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_WIFI_OFF_CFM_ID,             nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_WIFI_OFF_IND_ID,             nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_WIFI_FLIGHTMODE_CFM_ID,      nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_SET_VALUE_CFM_ID,            nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_GET_VALUE_CFM_ID,            nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_MIB_SET_CFM_ID,              nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_MIB_GET_CFM_ID,              nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_MIB_GET_NEXT_CFM_ID,         nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_FULL_CFM_ID,            nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULTS_GET_CFM_ID,     nme_signal_routing_scan_results_get_cfm),
    fsm_event_table_entry(UNIFI_NME_MGT_MULTICAST_ADDRESS_CFM_ID,    nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_PMKID_CFM_ID,                nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_KEY_CFM_ID,                  nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_PACKET_FILTER_SET_CFM_ID,    nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_TSPEC_CFM_ID,                nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULTS_FLUSH_CFM_ID,   nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_BLACKLIST_CFM_ID,            nme_signal_routing_forward_to_csr_wifi_nme_sap),

    fsm_event_table_entry(UNIFI_NME_MGT_CONNECTION_QUALITY_IND_ID,   nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_TSPEC_IND_ID,                nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_PMKID_CANDIDATE_LIST_IND_ID, nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_IBSS_STATION_IND_ID,         nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULT_IND_ID,          nme_signal_routing_scan_result_ind),
    fsm_event_table_entry(UNIFI_NME_MGT_ROAM_START_IND_ID,           nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_ROAM_COMPLETE_IND_ID,        nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_ASSOCIATION_START_IND_ID,    nme_signal_routing_forward_to_csr_wifi_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_ASSOCIATION_COMPLETE_IND_ID, nme_signal_routing_forward_to_csr_wifi_nme_sap),

    /* The NME requests all indications from the SME regardless of what any higher application
     * might request. The NME assumes that the request is successful so ignores the confirm.
     */
    fsm_event_table_entry(UNIFI_NME_MGT_EVENT_MASK_SET_CFM_ID,       fsm_ignore_event),

};


/** NME Core state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                           State                       State                         Save     */
   /*                           Name                        Transitions                    *       */
   fsm_state_table_entry(FSMSTATE_idle,                  idleTransitions,                 FALSE),
};

const FsmProcessStateMachine nme_signal_routing_fsm =
{
#ifdef FSM_DEBUG
       "Nme Signal Routing",                    /* SM Process Name       */
#endif
       NME_SIGNAL_ROUTING_PROCESS,              /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},        /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),  /* Ignore Event handers */
       nme_signal_routing_init_fsm,             /* Entry Function        */
       nme_signal_routing_reset_fsm,            /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                     /* Trace Dump Function   */
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
