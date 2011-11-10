/** @file nme_network_selector_fsm.c
 *
 * NME Network Selector FSM Implementation
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
 *   NME Network Selector FSM Implementation
 *
 *   Static FSM that is only active when Wifi is on.
 *
 *   Acts as the single coordination point between WPS attempts and other
 *   network connection requests, as well as the autonomous network selection.
 *
 *   When coordinating WPS and other connection requests the latest
 *   request always superceeds the current active request.
 *
 *   Will rely on the SME behaviour as much as possible to avoid
 *   duplicating code here. So if the SME will perform a check on
 *   a connection request for us then there is no need for that
 *   check to be repeated here.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_network_selector_fsm/nme_network_selector_fsm.c#8 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_network_selector
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_debug.h"
#include "fsm/fsm_internal.h"

#include "nme_top_level_fsm/nme_top_level_fsm.h"


/* MACROS *******************************************************************/

/**
 * @brief
 *   Simple accessor for this Processes Custom data
 *
 * @par Description
 *   see brief
 */
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
    FSMSTATE_stopped,
    FSMSTATE_disconnected,
    FSMSTATE_connecting,
    FSMSTATE_connected,
    FSMSTATE_disconnecting,
    FSMSTATE_terminatingActiveFsm,
    FSMSTATE_wps,
    FSMSTATE_waitingForSmeDisconnected,
    FSMSTATE_cancellingConnectionSetup,
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
    /* Reference to any associated connection manager FSM instance. */
    CsrUint16 connectionManagerInstance;

    /* Reference to any associated WPS FSM instance. */
    CsrUint16 wpsInstance;

    /* If the current connection was initiated from an
     * internal fsm this will != coreFsm */
    CsrUint16 currentConnectionSource;

    /* During the roaming process we need to discard any MA UNITDATA INDs */
    CsrBool isRoaming;

    /* If connected/hosting an adhoc network MEDIA STATUS IND disconnected
     * is not treated as disconnecting from the network. For an Adhoc network
     * we have to explicitly disconnect.
     */
    CsrBool isAdhocNetwork;

    /* Need to track when we've been explicitly told to disconnect so that
     * we wait for the confirmation before moving to the next state.
     */
    CsrBool appRequestedDisconnect;

} FsmData;

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   returns true if the network selector is connected
 *
 * @par Description
 *   Returns true of the network selector is connected
 *
 * @param[in] FsmContext* : FSM context
 *
 * @return
 *   CsrBool
 */
CsrBool nme_ntw_selector_is_connected(FsmContext* pContext)
{
    FsmState state = (FsmState)fsm_get_context_by_id(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance)->state;

    if (state == FSMSTATE_connected)
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief
 *   A core start request
 *
 * @par Description
 *   Sets the network selector to the disconnected state
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    NmeCoreStartReq_Evt* : start request event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_core_start_req(
        FsmContext* pContext,
        const NmeCoreStartReq_Evt* pReq)
{
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_core_start_req()"));
    fsm_next_state(pContext, FSMSTATE_disconnected);
}

/**
 * @brief
 *   A core stop request
 *
 * @par Description
 *   Returns the network selector to the stopped state
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    NmeCoreStopReq_Evt* : start request event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_core_stop_req(
        FsmContext* pContext,
        const NmeCoreStopReq_Evt* pReq)
{
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_core_stop_req()"));
    fsm_next_state(pContext, FSMSTATE_stopped);
}


/**
 * @brief
 *   Handle a received MGT DISCONNECT CFM
 *
 * @par Description
 *   Sets network selector to the disconnected state
 *
 * @param[in]  FsmContext* : FSM context
 * @param[in]  const FsmEvent* : disconnect cfm event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_disconnect_cfm(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_disconnect_cfm()"));
    send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
    fsm_next_state(pContext, FSMSTATE_disconnected);
}

/**
 * @brief
 *   Handles a NME PROFILE CONNECT REQUEST
 *
 * @par Description
 *   If in the disconnected state then we can just start
 *   the connection manager to handle the profile connection.
 *   If not in the disconnected state then we have to take
 *   actions to return to that state.
 *
 * @param[in]  FsmContext* : FSM context
 * @param[in]  FsmEvent* : request event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_start_new_profile_connection(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    FsmData *pFsmData = FSMDATA;
    require(TR_NME_NS_FSM, FSM_TERMINATE == pFsmData->connectionManagerInstance);
    require(TR_NME_NS_FSM, FSM_TERMINATE == pFsmData->wpsInstance);
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_start_new_profile_connection()"));

    if (!nme_ntw_selector_is_disconnected(pContext))
    {
        if (FSM_TERMINATE != pFsmData->connectionManagerInstance)
        {
            send_nme_conn_mgr_terminate_req(pContext, pFsmData->connectionManagerInstance);
            fsm_next_state(pContext, FSMSTATE_terminatingActiveFsm);
        }
        else
        {
            /* Ensure that the SME is disconnected before starting the connection attempt */
            call_unifi_nme_mgt_disconnect_req(pContext, NME_APP_HANDLE(pContext));
            fsm_next_state(pContext, FSMSTATE_waitingForSmeDisconnected);
        }
        fsm_saved_event(pContext, (const FsmEvent*)pReq);
    }
    else
    {
        /* Save the Sender to use when routing the response*/
        pFsmData->currentConnectionSource = pReq->sender_;
        pFsmData->connectionManagerInstance = fsm_add_instance(pContext, &nme_connection_manager_fsm, TRUE);
        fsm_forward_event(pContext, pFsmData->connectionManagerInstance, pReq);
        fsm_next_state(pContext, FSMSTATE_connecting);
    }
}

/**
 * @brief
 *   Handles a new NME CONNECT REQUEST.
 *
 * @par Description
 *   Handles all NME CONNECT REQUESTS except when there is an active
 *   WPS FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : request event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_start_new_mgt_sap_connection(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    FsmData *pFsmData = FSMDATA;
    require(TR_NME_NS_FSM, FSM_TERMINATE == pFsmData->wpsInstance);
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_start_new_mgt_sap_connection()"));

    /* Trigger the termination of any active profile connection FSM */
    if (FSM_TERMINATE != pFsmData->connectionManagerInstance)
    {
        send_nme_conn_mgr_terminate_req(pContext, pFsmData->connectionManagerInstance);
        fsm_next_state(pContext, FSMSTATE_terminatingActiveFsm);
        fsm_saved_event(pContext, (const FsmEvent*)pReq);
    }
    else
    {
        const UnifiNmeConnectReq_Evt *pConnectReq = (UnifiNmeConnectReq_Evt *)pReq;

        /* There is no profile connection active so just forward the request
         * to the SME and let it sort it out.
         */

        /* Save the Sender to use when routing the response*/
        pFsmData->currentConnectionSource = pReq->sender_;

        if (unifi_Adhoc == pConnectReq->connectionConfig.bssType)
        {
            pFsmData->isAdhocNetwork = TRUE;
        }
        else
        {
            pFsmData->isAdhocNetwork = FALSE;
        }

        /* No need to check for restricted access as already done in core fsm */
        nme_forward_to_unifi_mgt_sap(pContext, pReq);
        if (!nme_ntw_selector_is_disconnected(pContext) && !nme_ntw_selector_is_connected(pContext))
        {
            /* Must be a non profile connection, so we need to wait for any outstanding
             * signaling relating to that connection, before starting this one, otherwise
             * things are going to get very confusing in the FSM trying to determine which
             * signaling relates to which connection request.
             */
            fsm_next_state(pContext, FSMSTATE_cancellingConnectionSetup);
        }
        else
        {
            fsm_next_state(pContext, FSMSTATE_connecting);
        }
    }
}

/**
 * @brief
 *   A NME WPS REQUEST
 *
 * @par Description
 *   Handles the WPS request when in the disconnected state by
 *   passing the request to the WPS FSM.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME WPS REQUEST event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_disconnected_wps_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    FsmData *pFsmData = FSMDATA;
    require(TR_NME_NS_FSM, FSM_TERMINATE == FSMDATA->connectionManagerInstance);
    require(TR_NME_NS_FSM, FSM_TERMINATE == FSMDATA->wpsInstance);
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_disconnected_wps_req()"));
    /* No need to check for restricted access as already done in core fsm */
    pFsmData->wpsInstance = fsm_add_instance(pContext, &nme_wps_fsm, TRUE);
    fsm_forward_event(pContext, pFsmData->wpsInstance, pReq);
    fsm_next_state(pContext, FSMSTATE_wps);
}

/**
 * @brief
 *   Disconnect the existing connection
 *
 * @par Description
 *   See brief
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_disconnect_active_connection(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    FsmData *pFsmData = FSMDATA;
    require(TR_NME_NS_FSM, (FSM_TERMINATE != pFsmData->connectionManagerInstance) || (FSM_TERMINATE == pFsmData->wpsInstance));
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_terminate_active_connection()"));

    if (FSM_TERMINATE != pFsmData->connectionManagerInstance)
    {
        send_nme_conn_mgr_disconnect_req(
                pContext,
                pFsmData->connectionManagerInstance,
                FALSE, /* trigger termination of the connection mgr fsm on NS DISCONNECT IND */
                unifi_ConnMgrDisconnectReasonCancelled);
        fsm_next_state(pContext, FSMSTATE_disconnecting);
    }
    else
    {
        /* Ensure that the SME is disconnected before starting the connection attempt */
        call_unifi_nme_mgt_disconnect_req(pContext, NME_APP_HANDLE(pContext));
        fsm_next_state(pContext, FSMSTATE_waitingForSmeDisconnected);
    }
    fsm_saved_event(pContext, (const FsmEvent*)pReq);
}

/**
 * @brief
 *   Trigger the termination any existing profile connection
 *
 * @par Description
 *   See brief
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_terminate_active_connection(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_terminate_active_connection()"));

    if (FSM_TERMINATE != pFsmData->connectionManagerInstance)
    {
        send_nme_conn_mgr_terminate_req(pContext, pFsmData->connectionManagerInstance);
        fsm_next_state(pContext, FSMSTATE_terminatingActiveFsm);
    }
    else
    {
        send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
        fsm_next_state(pContext, FSMSTATE_disconnected);
    }
    fsm_saved_event(pContext, (const FsmEvent*)pReq);
}

/**
 * @brief
 *   Terminate the existing WPS FSM instance
 *
 * @par Description
 *   See brief
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_terminate_wps(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    FsmData *pFsmData = FSMDATA;
    require(TR_NME_NS_FSM, FSM_TERMINATE != pFsmData->wpsInstance);
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_terminate_wps()"));

    /* Save the request for handling once we get back to disconnected state */
    fsm_saved_event(pContext, (const FsmEvent*)pReq);

    /* As requires are removed for a release build, still have to ensure that
     * the code is safe.
     */
    if (FSM_TERMINATE != pFsmData->wpsInstance)
    {
        send_nme_wps_terminate_req(pContext, pFsmData->wpsInstance);
        fsm_next_state(pContext, FSMSTATE_terminatingActiveFsm);
    }
    else
    {
        send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
        fsm_next_state(pContext, FSMSTATE_disconnected);
    }
}


/**
 * @brief
 *   Handles NME DISCONNECT REQUEST
 *
 * @par Description
 *   Handles the disconnect request
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME DISCONNECT REQUEST event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_disconnect_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    FsmData *pFsmData = FSMDATA;
    const UnifiNmeDisconnectReq_Evt* pDisconnectReq = (UnifiNmeDisconnectReq_Evt*)pReq;

    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_disconnect_req()"));
    /* No need to check for restricted access as already done in core fsm */
    if (FSM_TERMINATE != pFsmData->connectionManagerInstance)
    {
        pFsmData->appRequestedDisconnect = TRUE;
        fsm_forward_event(pContext, pFsmData->connectionManagerInstance, pReq);
        fsm_next_state(pContext, FSMSTATE_disconnecting);
    }
    else if (FSM_TERMINATE != pFsmData->wpsInstance)
    {
        /* We don't want to send a disconnect request to the SME during a WPS procedure
         * as it will interfere with it
         */
        call_unifi_nme_disconnect_cfm(pContext, pDisconnectReq->appHandle, unifi_Success);
    }
    else
    {
        /* Just forward to the SME and it will respond appropriately. */
        pFsmData->appRequestedDisconnect = TRUE;
        nme_forward_to_unifi_mgt_sap(pContext, pReq);
        fsm_next_state(pContext, FSMSTATE_disconnecting);
    }
}

/**
 * @brief
 *   Handles the NME BLACKLIST REQUEST event
 *
 * @par Description
 *   If there is a profile connection manager active
 *   that is not terminating then it needs to know about
 *   the blacklist request. Otherwise this request can
 *   just be ignored.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* :  event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_blacklist_req(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_blacklist_req()"));
    if (FSM_TERMINATE != pFsmData->connectionManagerInstance)
    {
        fsm_forward_event(pContext, pFsmData->connectionManagerInstance, pEvt);
    }
    else
    {
        free_unifi_mgt_blacklist_req_contents((UnifiMgtBlacklistReq_Evt*) pEvt);
    }
}

/**
 * @brief
 *   Forwards the received event to the active FSM
 *
 * @par Description
 *   It is possible that there might not be an active FSM
 *   to forward the event to, in which case it is ignored.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* :  event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_forward_to_active_fsm_only(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_forward_to_active_fsm()"));

    if (UNIFI_NME_MGT_MEDIA_STATUS_IND_ID == pEvt->id)
    {
        pFsmData->isRoaming = FALSE;
    }
    if (FSM_TERMINATE != pFsmData->connectionManagerInstance)
    {
        fsm_forward_event(pContext, pFsmData->connectionManagerInstance, pEvt);
    }
    else if (FSM_TERMINATE != pFsmData->wpsInstance)
    {
        fsm_forward_event(pContext, pFsmData->wpsInstance, pEvt);
    }
    else
    {
        nme_free_event(pEvt);
    }
}

/**
 * @brief
 *   Forwards the MA UNITDATA IND to the active FSM, but only if
 *   we are not roaming.
 *
 * @par Description
 *   See brief
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* :  event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_forward_ma_unitdata_ind(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_forward_ma_unitdata_ind() roaming = %d", pFsmData->isRoaming));
    if (!pFsmData->isRoaming)
    {
        nme_ntw_selector_forward_to_active_fsm_only(pContext, pEvt);
    }
    else
    {
        nme_free_event(pEvt);
    }
}


/**
 * @brief
 *   Forwards the received event to the active FSM or the NME-SAP
 *
 * @par Description
 *   See brief.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* :  event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_forward_to_active_fsm_or_nme_sap(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_forward_to_active_fsm_or_nme_sap()"));

    if (UNIFI_NME_MGT_MEDIA_STATUS_IND_ID == pEvt->id)
    {
        pFsmData->isRoaming = FALSE;
    }
    else if (UNIFI_NME_MGT_DISCONNECT_CFM_ID == pEvt->id)
    {
        pFsmData->isAdhocNetwork = FALSE;
        pFsmData->appRequestedDisconnect = FALSE;
    }

    if (FSM_TERMINATE != pFsmData->connectionManagerInstance)
    {
        fsm_forward_event(pContext, pFsmData->connectionManagerInstance, pEvt);
    }
    else if (FSM_TERMINATE != pFsmData->wpsInstance)
    {
        fsm_forward_event(pContext, pFsmData->wpsInstance, pEvt);
    }
    else
    {
        /* Need to track the connection status for a connection triggered
         * via the MGT-SAP primitives rather than a profile connection.
         */
        if (UNIFI_NME_MGT_CONNECT_CFM_ID == pEvt->id)
        {
            const UnifiNmeMgtConnectCfm_Evt* pConnectCfm = (UnifiNmeMgtConnectCfm_Evt*)pEvt;

            if (unifi_Success == pConnectCfm->status)
            {
                fsm_next_state(pContext, FSMSTATE_connected);
            }
            else
            {
                pFsmData->isAdhocNetwork = FALSE;
                /* If the application requested disconnect wait for the confirmation
                 * before changing state.
                 */
                if (!pFsmData->appRequestedDisconnect)
                {
                    send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
                    fsm_next_state(pContext, FSMSTATE_disconnected);
                }
            }
        }
        else if (!pFsmData->isAdhocNetwork &&
                 UNIFI_NME_MGT_MEDIA_STATUS_IND_ID == pEvt->id)
        {
            const UnifiNmeMgtMediaStatusInd_Evt* pMediaStatusInd = (UnifiNmeMgtMediaStatusInd_Evt*)pEvt;
            if (unifi_MediaConnected == pMediaStatusInd->mediaStatus)
            {
                fsm_next_state(pContext, FSMSTATE_connected);
            }
            else
            {
                /* If the application requested disconnect wait for the confirmation
                 * before changing state.
                 */
                if (!pFsmData->appRequestedDisconnect)
                {
                    send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
                    fsm_next_state(pContext, FSMSTATE_disconnected);
                }
            }
        }
        if (UNIFI_NME_MGT_DISCONNECT_CFM_ID == pEvt->id)
        {
            /* Assume that the disconnect is always successful */
            send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
            fsm_next_state(pContext, FSMSTATE_disconnected);
        }
        nme_forward_to_csr_wifi_nme_sap(pContext, pEvt);
    }
}

/**
 * @brief
 *   Handles the CONNECT CFM that signaled the completion of
 *   canceling the previous connection setup
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const FsmEvent* pEvt
 *
 * @return
 *   void
 */
static void nme_ntw_selector_cancelled_connection_setup_complete(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_cancelled_connection_setup_complete()"));

    /* Assume that we are now proceeding with the connection setup that triggered
     * the cancellation of the previous connection attempt.
     */
    fsm_next_state(pContext, FSMSTATE_connecting);
    nme_forward_to_csr_wifi_nme_sap(pContext, pEvt);
}

/**
 * @brief
 *   Forwards the received event to the NME-SAP
 *   without changing the state of the FSM
 *
 * @par Description
 *   See brief.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* :  event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_forward_to_nme_sap_only(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    if (UNIFI_NME_MGT_MEDIA_STATUS_IND_ID == pEvt->id)
    {
        pFsmData->isRoaming = FALSE;
    }

    nme_forward_to_csr_wifi_nme_sap(pContext, pEvt);
}


/**
 * @brief
 *   Forwards the received event to the WPS FSM
 *
 * @par Description
 *   It is possible that there might not be an active wps FSM
 *   to forward the event to.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* :  event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_forward_to_wps(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_forward_to_wps()"));

    if (UNIFI_NME_MGT_MEDIA_STATUS_IND_ID == pEvt->id)
    {
        pFsmData->isRoaming = FALSE;
    }
    if (FSM_TERMINATE != pFsmData->wpsInstance)
    {
        fsm_forward_event(pContext, pFsmData->wpsInstance, pEvt);
    }
    else
    {
        nme_free_event(pEvt);
    }
}


/**
 * @brief
 *   Handles a NME NS CONNECT IND
 *
 * @par Description
 *   The requested connection has been established
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeNsConnectInd_Evt* : event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_connect_ind(
        FsmContext* pContext,
        const NmeNsConnectInd_Evt* pConnectInd)
{
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_connect_ind()"));
    fsm_next_state(pContext, FSMSTATE_connected);
}


/**
 * @brief
 *   Route the profile connect cfm to the correct place.
 *   Connection Manager is responsible for when to send it
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeProfileConnectCfm_Evt* : evnet
 *
 * @return
 *   void
 */
static void nme_ntw_selector_profile_connect_cfm(
        FsmContext* pContext,
        const UnifiNmeProfileConnectCfm_Evt* cfm)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_profile_connect_cfm(currentConnectionSource)", pFsmData->currentConnectionSource));

    if (pFsmData->currentConnectionSource == getNmeContext(pContext)->nmeCoreFsmInstance)
    {
        /* Send to the Environment */
        call_unifi_nme_profile_connect_cfm(pContext,
                                           cfm->appHandle,
                                           cfm->status,
                                           cfm->connectAttemptsCount,
                                           cfm->connectAttempts);
        free_unifi_nme_profile_connect_cfm_contents(cfm);
    }
    else
    {
        fsm_forward_event(pContext, pFsmData->currentConnectionSource, (const FsmEvent*)cfm);
    }
}

/**
 * @brief
 *   Handles a NME NS DISCONNECT IND
 *
 * @par Description
 *   The established connection has disconnected
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeNsDisconnectInd_Evt* : event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_disconnect_ind(
        FsmContext* pContext,
        const NmeNsDisconnectInd_Evt* pDisconnectInd)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_disconnect_ind()"));
    if (FSM_TERMINATE != pFsmData->connectionManagerInstance)
    {
        send_nme_conn_mgr_terminate_req(pContext, pFsmData->connectionManagerInstance);
        fsm_next_state(pContext, FSMSTATE_terminatingActiveFsm);
    }
    else
    {
        send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
        fsm_next_state(pContext, FSMSTATE_disconnected);
    }
}

/**
 * @brief
 *   Handles a NME CONN MGR TERMINATE CFM
 *
 * @par Description
 *   The connection manager FSM has completed it's termination
 *   as requested.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const FsmEvent* : event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_connection_mgr_terminate_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    require(TR_NME_NS_FSM, FSM_TERMINATE != pFsmData->connectionManagerInstance);
    require(TR_NME_NS_FSM, FSM_TERMINATE == pFsmData->wpsInstance);
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_connection_mgr_terminate_cfm()"));
    pFsmData->connectionManagerInstance = FSM_TERMINATE;
    send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
    fsm_next_state(pContext, FSMSTATE_disconnected);
}


/**
 * @brief
 *   Triggers termination of the WPS FSM when it has finished
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const FsmEvent* : event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_wps_finished(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_wps_finished(event id: 0x%x)", pEvt->id));

    if (UNIFI_NME_WPS_CFM_ID == pEvt->id)
    {
        UnifiNmeWpsCfm_Evt *pNmeWpsCfm = (UnifiNmeWpsCfm_Evt *)pEvt;
        call_unifi_nme_wps_cfm(pContext, pNmeWpsCfm->appHandle, pNmeWpsCfm->status, &(pNmeWpsCfm->profile));
    }
    else if (UNIFI_NME_WPS_CANCEL_CFM_ID == pEvt->id)
    {
        UnifiNmeWpsCancelCfm_Evt *pNmeWpsCancelCfm = (UnifiNmeWpsCancelCfm_Evt *)pEvt;
        call_unifi_nme_wps_cancel_cfm(pContext, pNmeWpsCancelCfm->appHandle, pNmeWpsCancelCfm->status);
    }
    else
    {
        sme_trace_crit((TR_NME_NS_FSM, "Unexpected event"));
    }

    if (FSM_TERMINATE != pFsmData->wpsInstance)
    {
        send_nme_wps_terminate_req(pContext, pFsmData->wpsInstance);
        fsm_next_state(pContext, FSMSTATE_terminatingActiveFsm);
    }
    else
    {
        sme_trace_warn((TR_NME_NS_FSM, "WPS FSM already TERMINATED"));
    }
}

/**
 * @brief
 *   Handles a NME WPS TERMINATE CFM
 *
 * @par Description
 *   The WPS FSM has completed it's termination
 *   as requested.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const FsmEvent* : event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_wps_terminate_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    require(TR_NME_NS_FSM, FSM_TERMINATE == pFsmData->connectionManagerInstance);
    require(TR_NME_NS_FSM, FSM_TERMINATE != pFsmData->wpsInstance);
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_wps_terminate_cfm()"));
    pFsmData->wpsInstance = FSM_TERMINATE;
    send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
    fsm_next_state(pContext, FSMSTATE_disconnected);
}

/**
 * @brief
 *   Handles a NME MGT ROAMING START IND
 *
 * @par Description
 *   Need to record the fact that we are roaming so that
 *   we can discard MA UNITDATA INDs.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const FsmEvent* : event
 *
 * @return
 *   void
 */
static void nme_ntw_selector_roam_start_ind(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_roam_start_ind()"));
    pFsmData->isRoaming = TRUE;
    nme_forward_to_csr_wifi_nme_sap(pContext, pEvt);
}


/**
 * @brief
 *   Initialises the FSM data at startup
 *
 * @par Description
 *   See Brief
 *
 * @param[in]    FsmContext* : FSM context
 *
 * @return
 *   void
 */
static void nme_ntw_selector_data_init(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_data_init()"));
    pFsmData->connectionManagerInstance = FSM_TERMINATE;
    pFsmData->wpsInstance = FSM_TERMINATE;
    pFsmData->isRoaming = FALSE;
    pFsmData->isAdhocNetwork = FALSE;
    pFsmData->appRequestedDisconnect = FALSE;
}

/**
 * @brief
 *   Inital FSM Entry function
 *
 * @par Description
 *   As the FSM is only active whilst wifi is on and some form of
 *   autonomous connection behaviour is required is. The FSM will
 *   immediately trigger a full scan and attempt to select a
 *   suitable network.
 *
 * @param[in] context   : FSM context
 *
 * @return
 *   void
 */
static void nme_ntw_selector_init_fsm(FsmContext* pContext)
{
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_init_fsm()"));
    fsm_create_params(pContext, FsmData);
    nme_ntw_selector_data_init(pContext);
    fsm_next_state(pContext, FSMSTATE_stopped);
}

/**
 * @brief
 *   FSM data reset
 *
 * @par Description
 *   reset FSM function
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void nme_ntw_selector_reset_fsm(FsmContext* pContext)
{
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_reset_fsm()"));
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in nme_network_selector_fsm/nme_network_selector_fsm.h
 */
CsrBool nme_ntw_selector_is_disconnected(FsmContext* pContext)
{
    FsmState state = fsm_get_context_by_id(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance)->state;

    if (state == FSMSTATE_disconnected)
    {
        return TRUE;
    }
    return FALSE;
}

/*
 * Description:
 * See description in nme_network_selector_fsm/nme_network_selector_fsm.h
 */
CsrUint16 nme_ntw_selector_get_connection_mgr_fsm_instance(FsmContext* pContext)
{
    CsrUint16 connectionMgrInstance = fsm_get_params_by_id(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, FsmData)->connectionManagerInstance;
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_get_connection_mgr_fsm_instance %d", connectionMgrInstance));
    return (connectionMgrInstance);
}

/*
 * Description:
 * See description in nme_network_selector_fsm/nme_network_selector_fsm.h
 */
CsrUint16 nme_ntw_selector_get_wps_fsm_instance(FsmContext* pContext)
{
    CsrUint16 wpsInstance = fsm_get_params_by_id(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, FsmData)->wpsInstance;
    sme_trace_entry((TR_NME_NS_FSM, "nme_ntw_selector_get_wps_fsm_instance %d", wpsInstance));
    return (wpsInstance);
}

/*
 * Description:
 * See description in nme_network_selector_fsm/nme_network_selector_fsm.h
 */
CsrUint16 nme_ntw_selector_get_security_fsm_instance(FsmContext* pContext)
{
    CsrUint16 secMgrInstance = nme_connection_manager_get_security_fsm_instance(pContext);

    if (FSM_TERMINATE == secMgrInstance)
    {
        secMgrInstance = nme_wps_get_security_fsm_instance(pContext);
    }
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_ntw_selector_get_security_fsm_instance Sec Instance %d", secMgrInstance));
    return (secMgrInstance);
}

/* FSM DEFINITION **********************************************/
static const FsmEventEntry stoppedTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_CORE_START_REQ_ID,                         nme_ntw_selector_core_start_req),
    fsm_event_table_entry(NME_CORE_STOP_REQ_ID,                          fsm_ignore_event),

    /* Only active between wifi-on and wifi-off, requests should be intercepted in the core fsm
     * before getting to here when wifi is off
     */

    /* Ignore these as they relate to a connection that we can not now handle */
    fsm_event_table_entry(UNIFI_NME_CERTIFICATE_VALIDATE_RSP_ID,         fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID,  fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_CFM_ID,              fsm_ignore_event),
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,                      fsm_ignore_event),
};


static const FsmEventEntry disconnectedTransitions[] =
{
                       /* Signal Id,                                     Function */
    /* Explicit connection/disconnection requests */
    fsm_event_table_entry(UNIFI_NME_CONNECT_REQ_ID,                      nme_ntw_selector_start_new_mgt_sap_connection),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   nme_ntw_selector_disconnect_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_REQ_ID,              nme_ntw_selector_start_new_profile_connection),
    fsm_event_table_entry(UNIFI_NME_WPS_REQ_ID,                          nme_ntw_selector_disconnected_wps_req),

    /* Ignore these as they relate to a previous connection that we are not now handle */
    fsm_event_table_entry(UNIFI_NME_CERTIFICATE_VALIDATE_RSP_ID,         fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID,  fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_CFM_ID,              fsm_ignore_event),
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,                      fsm_ignore_event),

    fsm_event_table_entry(NME_CORE_STOP_REQ_ID,                          nme_ntw_selector_core_stop_req),

};


static const FsmEventEntry connectingTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_NS_CONNECT_IND_ID,                         nme_ntw_selector_connect_ind),
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,                      nme_ntw_selector_disconnect_ind),

    /* Explicit connection/disconnection requests */
    fsm_event_table_entry(UNIFI_NME_CONNECT_REQ_ID,                      nme_ntw_selector_start_new_mgt_sap_connection),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   nme_ntw_selector_disconnect_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_REQ_ID,              nme_ntw_selector_disconnect_active_connection),
    fsm_event_table_entry(UNIFI_NME_WPS_REQ_ID,                          nme_ntw_selector_disconnect_active_connection),

    fsm_event_table_entry(UNIFI_NME_MGT_GET_VALUE_CFM_ID,                nme_ntw_selector_forward_to_active_fsm_only),

    /* need to terminate any active connection before handling the stop req */
    fsm_event_table_entry(NME_CORE_STOP_REQ_ID,                          fsm_saved_event),
};


static const FsmEventEntry connectedTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_NS_CONNECT_IND_ID,                         fsm_ignore_event),
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,                      nme_ntw_selector_disconnect_ind),

    /* Explicit connection/disconnection requests */
    fsm_event_table_entry(UNIFI_NME_CONNECT_REQ_ID,                      nme_ntw_selector_start_new_mgt_sap_connection),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   nme_ntw_selector_disconnect_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_REQ_ID,              nme_ntw_selector_disconnect_active_connection),
    fsm_event_table_entry(UNIFI_NME_WPS_REQ_ID,                          nme_ntw_selector_disconnect_active_connection),

    fsm_event_table_entry(UNIFI_NME_MGT_GET_VALUE_CFM_ID,                nme_ntw_selector_forward_to_active_fsm_only),

    /* need to terminate any active connection before handling the stop req */
    fsm_event_table_entry(NME_CORE_STOP_REQ_ID,                          nme_ntw_selector_terminate_active_connection),
};


static const FsmEventEntry disconnectingTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,                      nme_ntw_selector_disconnect_ind),

    /* Already disconnecting so save the request until we are in a suitable state
     * to handle it as we have to ensure we generate a response for every request.
     */
    fsm_event_table_entry(UNIFI_NME_CONNECT_REQ_ID,                      fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_REQ_ID,              fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_WPS_REQ_ID,                          fsm_saved_event),
    fsm_event_table_entry(NME_CORE_STOP_REQ_ID,                          fsm_saved_event),

    /* Whilst waiting for the connection manager to disconnect we could get the following
     * which need to be ignored.
     */
    fsm_event_table_entry(NME_NS_CONNECT_IND_ID,                         fsm_ignore_event),
};


static const FsmEventEntry terminatingActiveFsmTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_CONN_MGR_TERMINATE_CFM_ID,                 nme_ntw_selector_connection_mgr_terminate_cfm),
    fsm_event_table_entry(NME_WPS_TERMINATE_CFM_ID,                      nme_ntw_selector_wps_terminate_cfm),

    /* Assume that these events from the SME will be handled by the terminating FSM */
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_ntw_selector_forward_to_active_fsm_only),
    fsm_event_table_entry(UNIFI_NME_MGT_DISCONNECT_CFM_ID,               nme_ntw_selector_forward_to_active_fsm_only),

    /* Whilst waiting for the connection manager to terminate we could get the following
     * which need to be ignored.
     */
    fsm_event_table_entry(NME_NS_CONNECT_IND_ID,                         fsm_ignore_event),
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,                      fsm_ignore_event),

    /* Need to save these events until termination is complete otherwise we
     * would send the WPS FSM another terminate event, which would be received
     * after the FSM has gone to TERMINATE
     */
    fsm_event_table_entry(UNIFI_NME_WPS_CFM_ID,                          fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_WPS_CANCEL_CFM_ID,                   fsm_saved_event),

    /* Wait until the termination of the current active FSM is complete */
    fsm_event_table_entry(UNIFI_NME_CONNECT_REQ_ID,                      fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_REQ_ID,              fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_WPS_REQ_ID,                          fsm_saved_event),
    fsm_event_table_entry(NME_CORE_STOP_REQ_ID,                          fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_BLACKLIST_REQ_ID,                    fsm_saved_event),
};


static const FsmEventEntry wpsTransitions[] =
{
                       /* Signal Id,                                     Function */
    /* Explicit connection/disconnection requests */
    fsm_event_table_entry(UNIFI_NME_CONNECT_REQ_ID,                      nme_ntw_selector_terminate_wps),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   nme_ntw_selector_disconnect_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_REQ_ID,              nme_ntw_selector_terminate_wps),
    fsm_event_table_entry(UNIFI_NME_WPS_REQ_ID,                          nme_ntw_selector_terminate_wps),

    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_ntw_selector_forward_to_wps),
    fsm_event_table_entry(UNIFI_NME_MGT_DISCONNECT_CFM_ID,               nme_ntw_selector_forward_to_wps),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_FULL_CFM_ID,                nme_ntw_selector_forward_to_wps),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULTS_GET_CFM_ID,         nme_ntw_selector_forward_to_wps),

    /* Need to terminate the WPS before handling the stop req */
    fsm_event_table_entry(NME_CORE_STOP_REQ_ID,                          nme_ntw_selector_terminate_wps),
};

static const FsmEventEntry waitingForSmeDisconnectedTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(UNIFI_NME_MGT_DISCONNECT_CFM_ID,               nme_ntw_selector_disconnect_cfm),
    fsm_event_table_entry(UNIFI_NME_MGT_CONNECT_CFM_ID,                  nme_ntw_selector_forward_to_nme_sap_only),
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_ntw_selector_forward_to_nme_sap_only),
};


static const FsmEventEntry cancellingConnectionSetupTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(UNIFI_NME_MGT_CONNECT_CFM_ID,                  nme_ntw_selector_cancelled_connection_setup_complete),
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_ntw_selector_forward_to_nme_sap_only),
};


/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry defaultHandlers[] =
{
                           /* Signal Id,                                 Function */
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_CFM_ID,              nme_ntw_selector_profile_connect_cfm),

    fsm_event_table_entry(UNIFI_NME_MGT_CONNECT_CFM_ID,                  nme_ntw_selector_forward_to_active_fsm_or_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_DISCONNECT_CFM_ID,               nme_ntw_selector_forward_to_active_fsm_or_nme_sap),
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_ntw_selector_forward_to_active_fsm_or_nme_sap),

    /* These events need to be routed through to the active connection FSM or ignored in specific states*/
    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,              nme_ntw_selector_forward_to_active_fsm_or_nme_sap),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,    nme_ntw_selector_forward_to_active_fsm_only),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID,  nme_ntw_selector_forward_to_active_fsm_only),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                    nme_ntw_selector_forward_to_active_fsm_only),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,              nme_ntw_selector_forward_ma_unitdata_ind),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_CFM_ID,              nme_ntw_selector_forward_to_active_fsm_only),
    fsm_event_table_entry(UNIFI_NME_CERTIFICATE_VALIDATE_RSP_ID,         nme_ntw_selector_forward_to_active_fsm_only),

    fsm_event_table_entry(UNIFI_NME_MGT_ROAM_START_IND_ID,               nme_ntw_selector_roam_start_ind),

    /* Only the WPS FSM should be performing scanning at the moment so ignore these
     * events at any other time (for the moment)
     */
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_FULL_CFM_ID,                fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULTS_GET_CFM_ID,         fsm_ignore_event),

    fsm_event_table_entry(UNIFI_NME_BLACKLIST_REQ_ID,                    nme_ntw_selector_blacklist_req),

    /* As part of the connection process the NME uses blacklisting to trigger roaming
     * attempts to any other suitable APs. Not really interested in the CFM to these
     * requests but they have to be absorbed by the NME rather than forwarded to the
     * application.
     */
    fsm_event_table_entry(UNIFI_NME_MGT_BLACKLIST_CFM_ID,                fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_MGT_GET_VALUE_CFM_ID,                fsm_ignore_event),

    /* Need to terminate the WPS FSM and forward the response */
    fsm_event_table_entry(UNIFI_NME_WPS_CFM_ID,                          nme_ntw_selector_wps_finished),
    fsm_event_table_entry(UNIFI_NME_WPS_CANCEL_CFM_ID,                   nme_ntw_selector_wps_finished),
};


static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                           State                         State                                  Save   */
   /*                           Name                          Transitions                            *      */
    fsm_state_table_entry(FSMSTATE_stopped,                   stoppedTransitions,                    FALSE),
    fsm_state_table_entry(FSMSTATE_disconnected,              disconnectedTransitions,               FALSE),
    fsm_state_table_entry(FSMSTATE_connecting,                connectingTransitions,                 FALSE),
    fsm_state_table_entry(FSMSTATE_connected,                 connectedTransitions,                  FALSE),
    fsm_state_table_entry(FSMSTATE_disconnecting,             disconnectingTransitions,              FALSE),
    fsm_state_table_entry(FSMSTATE_terminatingActiveFsm,      terminatingActiveFsmTransitions,       FALSE),
    fsm_state_table_entry(FSMSTATE_wps,                       wpsTransitions,                        FALSE),
    fsm_state_table_entry(FSMSTATE_waitingForSmeDisconnected, waitingForSmeDisconnectedTransitions,  TRUE),
    fsm_state_table_entry(FSMSTATE_cancellingConnectionSetup, cancellingConnectionSetupTransitions,  TRUE),
};


const FsmProcessStateMachine nme_network_selector_fsm =
{
#ifdef FSM_DEBUG
       "Nme Ntw Selector",                       /* SM Process Name       */
#endif
       NME_NETWORK_SELECTOR_PROCESS,            /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},        /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, defaultHandlers, FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),  /* Ignore Event handers */
       nme_ntw_selector_init_fsm,               /* Entry Function        */
       nme_ntw_selector_reset_fsm,              /* Reset Function        */
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
