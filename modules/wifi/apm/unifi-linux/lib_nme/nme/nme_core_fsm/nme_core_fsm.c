/** @file nme_core_fsm.c
 *
 * Network Management Entity Core FSM Implementation
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
 *   NME Core FSM Implementation
 *   Responsible for starting and stopping other "static" FSMs that should
 *   only be active when wifi is on. Also handles initial checking of requests
 *   against the restricted access configuration and wifi status.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_core_fsm/nme_core_fsm.c#2 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "nme_top_level_fsm/nme_top_level_fsm.h"

/* MACROS *******************************************************************/
#define FSMDATA (fsm_get_params(pContext, FsmData))

#define CONFIG_SET_FLAG(fsmData, value) (fsmData->configFlags |= value)
#define CONFIG_CLEAR_FLAG(fsmData, value) (fsmData->configFlags &= ~value)
#define CONFIG_IS_SET(fsmData, value) ((fsmData->configFlags & value) != 0)

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
    /* Obtain any configuration details that are required from the SME
     * before handling any other events.
     */
   FSMSTATE_initialising,
   FSMSTATE_idle,
   /* Obtain any configuration that we need now that wifi is on */
   FSMSTATE_gettingWifiOnConfig,
   FSMSTATE_MAX_STATE
} FsmState;

/**
 * @brief
 *   Config Flags bit indexes
 *
 * @par Description
 *   Enum defining the indexes into configFlags
 *   used to determine the status of the various config items
 *   held by core
 */
typedef enum ConfigFlags
{
    ConfigFlag_receivedSmeConfig   = 0x0001,
    ConfigFlag_receivedMacAddress  = 0x0002
} ConfigFlags;

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData
{
    /* See the nme_nore_fsm/nme_core_fsm.h header file for details */
    NmeConfigData nmeConfig;

    /* Track whether wifi is on or not as the responses to certain requests
     * have to be determined differently depending on whether wifi is on or not.
     */
    CsrBool wifiIsOn;

    /* We need to track the whether we've received various config items
     * from the SME before changing states.
     */
    CsrUint16 configFlags;

} FsmData;

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Process a WIFI_ON_CFM.
 *
 * @par Description
 *   If wifi is now on then we need to record that it is so that we
 *   can allow through requests that are only valid when wifi is on.
 *   We need to obtain any configuration that might now be valid that
 *   we need to make use of such as the station MAC address.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_core_wifi_on_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    const UnifiNmeWifiOnCfm_Evt* pWifiOnCfm = (UnifiNmeWifiOnCfm_Evt*)pEvt;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_wifi_on_cfm()"));

    /* Avoid starting in the case that WIFI was already on */
    if(unifi_Success == pWifiOnCfm->status &&
       !pFsmData->wifiIsOn)
    {
        pFsmData->wifiIsOn = TRUE;
        pFsmData->nmeConfig.reportedConnected = FALSE;
        /* Need to get the MAC address */
        nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_MGT_GET_VALUE_CFM_ID, getNmeContext(pContext)->nmeCoreFsmInstance);
        call_unifi_nme_mgt_get_value_req(pContext, NME_APP_HANDLE(pContext), unifi_StationMACAddressValue);
        /* Start any FSMs that are only valid if wifi is on */
        send_nme_core_start_req(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        fsm_next_state(pContext, FSMSTATE_gettingWifiOnConfig);
    }
}


/**
 * @brief
 *   Process a WIFI_OFF_IND/CFM.
 *
 * @par Description
 *   Send CORE_STOP_REQ to the FSMs that need to know
 *   Update the internal configuration
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_core_wifi_off(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_wifi_off_cfm/ind()"));
    if (pFsmData->wifiIsOn)
    {
        pFsmData->wifiIsOn = FALSE;
        /* Stop any FSMs that are no longer valid as wifi is off */
        send_nme_core_stop_req(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
        send_nme_core_stop_req(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        CONFIG_CLEAR_FLAG(pFsmData, ConfigFlag_receivedMacAddress);
        fsm_next_state(pContext, FSMSTATE_idle);
    }
}

/**
 * @brief
 *   Process a WIFI_OFF_IND.
 *
 * @par Description
 *   Calls nme_core_wifi_off and also needs to free the
 *   contents of the message
 *
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_core_wifi_off_ind(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    const UnifiMgtWifiOffInd_Evt* pWifiOffInd = (UnifiMgtWifiOffInd_Evt*)pEvt;

    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_wifi_off_ind()"));
    nme_core_wifi_off(pContext, pEvt);
    free_unifi_mgt_wifi_off_ind_contents(pWifiOffInd);
}


/**
 * @brief
 *   Process a WIFI_OFF_CFM.
 *
 * @par Description
 *   Calls nme_core_wifi_off if the result is success
 *
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_core_wifi_off_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    const UnifiMgtWifiOffCfm_Evt* pWifiOffCfm = (UnifiMgtWifiOffCfm_Evt*)pEvt;

    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_wifi_off_cfm()"));
    if (unifi_Success == pWifiOffCfm->status)
    {
        nme_core_wifi_off(pContext, pEvt);
    }
}

/**
 * @brief
 *   Process a UNIFI_NME_CONNECTION_STATUS_GET_REQ
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeConnectionStatusGetReq_Evt* : UNIFI NME CONNECTION STATUS GET REQ event
 *
 * @return
 *   void
 */
static void nme_core_connection_status_get_req(
        FsmContext* pContext,
        const UnifiNmeConnectionStatusGetReq_Evt* pConnectionStatusGetReq)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_connection_status_get_req()"));

    if (pFsmData->wifiIsOn)
    {
        unifi_ConnectionStatus connectionStatus = nme_connection_manager_connection_status_get(pContext);
        call_unifi_nme_connection_status_get_cfm(
                pContext,
                pConnectionStatusGetReq->appHandle,
                unifi_Success,
                connectionStatus);
    }
    else
    {
        call_unifi_nme_connection_status_get_cfm(
                pContext,
                pConnectionStatusGetReq->appHandle,
                unifi_WifiOff,
                unifi_ConnectionStatusDisconnected);
    }
}


/**
 * @brief
 *   Process a UNIFI_NME_MGT_GET_VALUE_CFM
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeMgtGetValueCfm_Evt* : event
 *
 * @return
 *   void
 */
static void nme_core_initialising_mgt_get_value_cfm(
        FsmContext* pContext,
        const UnifiNmeMgtGetValueCfm_Evt* pGetValueCfm)
{
    FsmData *pFsmData = FSMDATA;
    NmeConfigData* cfg = nme_core_get_nme_config(pContext);

    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_initialising_mgt_get_value_cfm()"));

    /* These are the values that were requested from the SME that need
     * to be stored in the NME configuration
     */
    if (unifi_SmeConfigValue == pGetValueCfm->appValue.id)
    {
        sme_trace_debug((TR_NME_CORE_FSM, "nme_core_mgt_get_value_cfm(), enableRestrictedAccess %d", pGetValueCfm->appValue.unifi_Value_union.smeConfig.enableRestrictedAccess));
        CONFIG_SET_FLAG(pFsmData, ConfigFlag_receivedSmeConfig);
        cfg->enableRestrictedAccess = pGetValueCfm->appValue.unifi_Value_union.smeConfig.enableRestrictedAccess;
        /* Got everything we need */
        fsm_next_state(pContext, FSMSTATE_idle);
    }
    else
    {
        sme_trace_error((TR_NME_CORE_FSM, "nme_core_initialising_mgt_get_value_cfm(), unexpected appvalue %s", trace_unifi_AppValueId(pGetValueCfm->appValue.id)));
    }
}


/**
 * @brief
 *   Process a UNIFI_NME_MGT_GET_VALUE_CFM in wifi on config state
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeMgtGetValueCfm_Evt* : event
 *
 * @return
 *   void
 */
static void nme_core_wifi_on_mgt_get_value_cfm(
        FsmContext* pContext,
        const UnifiNmeMgtGetValueCfm_Evt* pGetValueCfm)
{
    FsmData *pFsmData = FSMDATA;
    NmeConfigData* cfg = nme_core_get_nme_config(pContext);

    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_wifi_on_mgt_get_value_cfm()"));

    /* These are the values that were requested from the SME that need
     * to be stored in the NME configuration
     */
    if(unifi_StationMACAddressValue == pGetValueCfm->appValue.id)
    {
        CONFIG_SET_FLAG(pFsmData, ConfigFlag_receivedMacAddress);
        cfg->stationMacAddress = pGetValueCfm->appValue.unifi_Value_union.stationMacAddress;
        /* The only configuration that we get from the SME when wifi is
         * turned on. */
        send_nme_core_start_req(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
        fsm_next_state(pContext, FSMSTATE_idle);
    }
    else
    {
        sme_trace_error((TR_NME_CORE_FSM, "nme_core_wifi_on_mgt_get_value_cfm(), unexpected appvalue %s", trace_unifi_AppValueId(pGetValueCfm->appValue.id)));
    }
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
static void nme_core_init_data(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_init_data()"));

    pFsmData->nmeConfig.enableRestrictedAccess = FALSE;
    pFsmData->nmeConfig.restrictedAccessActivated = FALSE;
    pFsmData->nmeConfig.restrictedAccessAppHandle = NULL;
    pFsmData->nmeConfig.reportedConnected = FALSE;
    pFsmData->wifiIsOn = FALSE;
    pFsmData->configFlags = 0;

    csr_list_clear(&pFsmData->nmeConfig.pmkCache);
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
static void nme_core_init_fsm(FsmContext* pContext)
{
    FsmData* pFsmData;
    sme_trace_entry((TR_NME_CORE_FSM, "init_nme_core_fsm()"));
    fsm_create_params(pContext, FsmData);

    pFsmData = FSMDATA;
    csr_list_init(&pFsmData->nmeConfig.pmkCache);
    pFsmData->nmeConfig.cloakedSsidsCount = 0;
    pFsmData->nmeConfig.cloakedSsids = NULL;

    nme_core_init_data(pContext);

    /* Obtain any default configuration that we want to keep in line with the SME settings */
    nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_MGT_GET_VALUE_CFM_ID, getNmeContext(pContext)->nmeCoreFsmInstance);
    call_unifi_nme_mgt_get_value_req(pContext, NME_APP_HANDLE(pContext), unifi_SmeConfigValue);

    fsm_next_state(pContext, FSMSTATE_initialising);
}


/**
 * @brief
 *   FSM data reset
 *
 * @par Description
 *   Cleanup function (Release all memory)
 *
 * @param[in]  FsmContext*   : FSM context
 *
 * @return
 *   void
 */
static void nme_core_reset_fsm(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_reset_fsm()"));

    nme_core_init_data(pContext);
    if (pFsmData->nmeConfig.cloakedSsids)
    {
        CsrPfree(pFsmData->nmeConfig.cloakedSsids);
    }
}


/**
 * @brief
 *   Handle Activate Access Restrictions request
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const UnifiNmeActivateReq_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_core_restricted_access_enable_req(
        FsmContext* pContext,
        const UnifiNmeRestrictedAccessEnableReq_Evt* pReq)
{
    NmeConfigData* cfg = nme_core_get_nme_config(pContext);
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_restricted_access_enable_req(%p)", pReq->appHandle));

    if ( !cfg->enableRestrictedAccess ||
        (cfg->enableRestrictedAccess && !cfg->restrictedAccessActivated) ||
        (cfg->enableRestrictedAccess && cfg->restrictedAccessActivated && cfg->restrictedAccessAppHandle == pReq->appHandle))
    {
        /* Implicit enabling of access restrictions */
        cfg->enableRestrictedAccess = TRUE;

        cfg->restrictedAccessActivated = TRUE;
        cfg->restrictedAccessAppHandle = pReq->appHandle;
        /* Need to forward the request to the SME and wait for the response before
         * responding to the requester. NOTE: might need to alter the values above
         * based on the response from the SME.
         */
        call_unifi_nme_mgt_restricted_access_enable_req(pContext, NME_APP_HANDLE(pContext));
    }
    else
    {
        /* The activation request is either not supported or another app handle
         * has already enabled restricted access. So a different app handle can
         * not also request it.
         */
        call_unifi_nme_restricted_access_enable_cfm(pContext, pReq->appHandle, unifi_Restricted);
    }
}

/**
 * @brief
 *   Handle Access Restrictions Activation Confirmation
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const UnifiNmeMgtActivateCfm_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_core_mgt_restricted_access_enable_cfm(
        FsmContext* pContext,
        const UnifiNmeMgtRestrictedAccessEnableCfm_Evt* pCfm)
{
    NmeConfigData* cfg = nme_core_get_nme_config(pContext);
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_mgt_restricted_access_enable_cfm()"));

    call_unifi_nme_restricted_access_enable_cfm(pContext, cfg->restrictedAccessAppHandle, pCfm->status);

    /* Should only have passed down a request when we thought it
     * was going to be accepted, but if the SME has failed the request then
     * we need to update the info.
     */
    if (unifi_Success != pCfm->status)
    {
        cfg->restrictedAccessActivated = FALSE;
        cfg->restrictedAccessAppHandle = NULL;
    }
}

/**
 * @brief
 *   Handle Deactivate Access Restrictions request
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const UnifiNmeDeactivateReq_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_core_restricted_access_disable_req(
        FsmContext* pContext,
        const UnifiNmeRestrictedAccessDisableReq_Evt* pReq)
{
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_restricted_access_disable_req(%p)", pReq->appHandle));
    if (!nme_core_is_access_restricted(pContext, pReq->appHandle))
    {
        /* Forward the request to the SME, updating the info is performed
         * when the confirmation is received.
         */
        call_unifi_nme_mgt_restricted_access_disable_req(pContext, NME_APP_HANDLE(pContext));
    }
    else
    {
        call_unifi_nme_restricted_access_disable_cfm(pContext, pReq->appHandle, unifi_Restricted);
    }
}

/**
 * @brief
 *   Handle Deactivate Access Restrictions confirm
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const UnifiNmeMgtDeactivateCfm_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_core_mgt_restricted_access_disable_cfm(
        FsmContext* pContext,
        const UnifiNmeMgtRestrictedAccessDisableCfm_Evt* pCfm)
{
    NmeConfigData* cfg = nme_core_get_nme_config(pContext);
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_mgt_restricted_access_disable_cfm()"));

    call_unifi_nme_restricted_access_disable_cfm(pContext, cfg->restrictedAccessAppHandle, pCfm->status);
    if (unifi_Success == pCfm->status)
    {
        cfg->restrictedAccessActivated = FALSE;
        cfg->restrictedAccessAppHandle = NULL;
    }
}

/**
 * @brief
 *   Handles a NME PROFILE SET REQ which as long as not access restricted
 *   can be forwarded to the Profile Manager FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME PROFILE SET REQ
 *
 * @return
 *   void
 */
static void nme_core_profile_set_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    const UnifiNmeProfileSetReq_Evt* pProfileSetReq = (UnifiNmeProfileSetReq_Evt*)pReq;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_profile_set_req()"));

    if (!nme_core_is_access_restricted(pContext, pProfileSetReq->appHandle))
    {
        fsm_forward_event(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance, pReq);
    }
    else
    {
        call_unifi_nme_profile_set_cfm(pContext, pProfileSetReq->appHandle, unifi_Restricted);
    }
}

/**
 * @brief
 *   Handles a NME PROFILE DELETE  REQ which as long as not access restricted
 *   can be forwarded to the Profile Manager FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME PROFILE DELETE  REQ
 *
 * @return
 *   void
 */
static void nme_core_profile_delete_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    const UnifiNmeProfileDeleteReq_Evt* pProfileDeleteReq = (UnifiNmeProfileDeleteReq_Evt*)pReq;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_profile_delete_req()"));

    if (!nme_core_is_access_restricted(pContext, pProfileDeleteReq->appHandle))
    {
        fsm_forward_event(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance, pReq);
    }
    else
    {
        call_unifi_nme_profile_delete_cfm(pContext, pProfileDeleteReq->appHandle, unifi_Restricted);
    }
}

/**
 * @brief
 *   Handles a NME PROFILE DELETE ALL REQ which as long as not access restricted
 *   can be forwarded to the Profile Manager FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME PROFILE DELETE ALL REQ
 *
 * @return
 *   void
 */
static void nme_core_profile_delete_all_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    const UnifiNmeProfileDeleteAllReq_Evt* pProfileDeleteAllReq = (UnifiNmeProfileDeleteAllReq_Evt*)pReq;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_profile_delete_all_req()"));

    if (!nme_core_is_access_restricted(pContext, pProfileDeleteAllReq->appHandle))
    {
        fsm_forward_event(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance, pReq);
    }
    else
    {
        call_unifi_nme_profile_delete_all_cfm(pContext, pProfileDeleteAllReq->appHandle, unifi_Restricted);
    }
}

/**
 * @brief
 *   Handles a NME PROFILE ORDER SET REQ which as long as not access restricted
 *   can be forwarded to the Profile Manager FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME PROFILE ORDER SET REQ
 *
 * @return
 *   void
 */
static void nme_core_profile_order_set_req(
        FsmContext* pContext,
        const UnifiNmeProfileOrderSetReq_Evt* req)
{
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_profile_order_set_req()"));

    if (!nme_core_is_access_restricted(pContext, req->appHandle))
    {
        fsm_forward_event(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance, (FsmEvent*)req);
    }
    else
    {
        call_unifi_nme_profile_order_set_cfm(pContext, req->appHandle, unifi_Restricted);
    }
}

/**
 * @brief
 *   Handles a NME PROFILE CONNECT REQ which as long as not access restricted
 *   and wifi is on can be forwarded to the Network Selector FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME PROFILE CONNECT REQ
 *
 * @return
 *   void
 */
static void nme_core_profile_connect_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    const UnifiNmeProfileConnectReq_Evt* pProfileConnectReq = (UnifiNmeProfileConnectReq_Evt*)pReq;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_profile_connect_req()"));

    if (!nme_core_is_access_restricted(pContext, pProfileConnectReq->appHandle))
    {
        FsmData *pFsmData = FSMDATA;
        if (pFsmData->wifiIsOn)
        {
            fsm_forward_event(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, pReq);
        }
        else
        {
            call_unifi_nme_profile_connect_cfm(
                    pContext,
                    pProfileConnectReq->appHandle,
                    unifi_WifiOff,
                    0, /* Not attempted to connect to any APs yet */
                    NULL);
        }
    }
    else
    {
        call_unifi_nme_profile_connect_cfm(
                pContext,
                pProfileConnectReq->appHandle,
                unifi_Restricted,
                0, /* Not attempted to connect to any APs yet */
                NULL);
    }
}

/**
 * @brief
 *   Handles a NME CONNECT REQ which as long as not access restricted
 *   and wifi is on can be forwarded to the Network Selector FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME CONNECT REQ
 *
 * @return
 *   void
 */
static void nme_core_connect_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    const UnifiNmeConnectReq_Evt* pConnectReq = (UnifiNmeConnectReq_Evt*)pReq;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_connect_req()"));

    if (!nme_core_is_access_restricted(pContext, pConnectReq->appHandle))
    {
        FsmData *pFsmData = FSMDATA;
        if (pFsmData->wifiIsOn)
        {
            fsm_forward_event(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, pReq);
        }
        else
        {
            call_unifi_nme_connect_cfm(pContext, pConnectReq->appHandle, unifi_WifiOff);
        }
    }
    else
    {
        call_unifi_nme_connect_cfm(pContext, pConnectReq->appHandle, unifi_Restricted);
    }
}

/**
 * @brief
 *   Handles a NME DISCONNECT REQ which as long as not access restricted
 *   and wifi is on can be forwarded to the Network Selector FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME DISCONNECT REQ
 *
 * @return
 *   void
 */
static void nme_core_disconnect_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{

    const UnifiNmeDisconnectReq_Evt* pDisconnectReq = (UnifiNmeDisconnectReq_Evt*)pReq;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_disconnect_req()"));

    if (!nme_core_is_access_restricted(pContext, pDisconnectReq->appHandle))
    {
        FsmData *pFsmData = FSMDATA;
        if (pFsmData->wifiIsOn)
        {
            fsm_forward_event(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, pReq);
        }
        else
        {
            call_unifi_nme_disconnect_cfm(pContext, pDisconnectReq->appHandle, unifi_WifiOff);
        }
    }
    else
    {
        call_unifi_nme_disconnect_cfm(pContext, pDisconnectReq->appHandle, unifi_Restricted);
    }
}

/**
 * @brief
 *   Handles a NME WPS REQ which as long as not access restricted
 *   and wifi is on can be forwarded to the Network Selector FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME WPS REQ
 *
 * @return
 *   void
 */
static void nme_core_wps_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    const UnifiNmeWpsReq_Evt* pWpsReq = (UnifiNmeWpsReq_Evt*)pReq;
    unifi_Profile *pProfile = nme_profile_manager_get_null_profile();
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_wps_req()"));

#if defined(CSR_WIFI_SECURITY_WPS_ENABLE)

    if (!nme_core_is_access_restricted(pContext, pWpsReq->appHandle))
    {
        FsmData *pFsmData = FSMDATA;
        if (pFsmData->wifiIsOn)
        {
            fsm_forward_event(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, pReq);
        }
        else
        {
            call_unifi_nme_wps_cfm(pContext, pWpsReq->appHandle, unifi_WifiOff, pProfile);
        }
    }
    else
    {
        call_unifi_nme_wps_cfm(pContext, pWpsReq->appHandle, unifi_Restricted, pProfile);
    }
#else
    call_unifi_nme_wps_cfm(pContext, pWpsReq->appHandle, unifi_Unsupported, pProfile);
#endif
    CsrPfree(pProfile);
}

/**
 * @brief
 *   Handles a NME WPS CANCEL REQ which as long as not access restricted
 *   and wifi is on can be forwarded to the Network Selector FSM instance.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : NME WPS REQ
 *
 * @return
 *   void
 */
static void nme_core_wps_cancel_req(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    const UnifiNmeWpsCancelReq_Evt* pWpsCancelReq = (UnifiNmeWpsCancelReq_Evt*)pReq;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_wps_cancel_req()"));

#if defined(CSR_WIFI_SECURITY_WPS_ENABLE)

    if (!nme_core_is_access_restricted(pContext, pWpsCancelReq->appHandle))
    {
        FsmData *pFsmData = FSMDATA;
        if (pFsmData->wifiIsOn)
        {
            CsrUint16 wpsFsmInstance = nme_ntw_selector_get_wps_fsm_instance(pContext);
            if (FSM_TERMINATE != wpsFsmInstance)
            {
                fsm_forward_event(pContext, wpsFsmInstance, pReq);
            }
            else
            {
                call_unifi_nme_wps_cancel_cfm(pContext, pWpsCancelReq->appHandle, unifi_Success);
            }
        }
        else
        {
            call_unifi_nme_wps_cancel_cfm(pContext, pWpsCancelReq->appHandle, unifi_WifiOff);
        }
    }
    else
    {
        call_unifi_nme_wps_cancel_cfm(pContext, pWpsCancelReq->appHandle, unifi_Restricted);
    }
#else
    call_unifi_nme_wps_cancel_cfm(pContext, pWpsCancelReq->appHandle, unifi_Unsupported);
#endif
}



/* PUBLIC FUNCTION DEFINITIONS **********************************************/
/**
 * See description in nme_core_fsm/nme_core_fsm.h
 */
NmeConfigData* nme_core_get_nme_config(FsmContext* pContext)
{
    return &(fsm_get_params_by_id(pContext, getNmeContext(pContext)->nmeCoreFsmInstance, FsmData))->nmeConfig;
}


/*
 * See description in nme_core_fsm/nme_core_fsm.h
 */
CsrBool nme_core_is_access_restricted(
        FsmContext* pContext,
        void* appHandle)
{
    NmeConfigData* cfg = nme_core_get_nme_config(pContext);

    if (!cfg->enableRestrictedAccess)
    {
        return FALSE;
    }

    if (cfg->restrictedAccessActivated && cfg->restrictedAccessAppHandle == appHandle)
    {
        return FALSE;
    }
    return TRUE;
}

/* FSM DEFINITION **********************************************/

static const FsmEventEntry initialisingTransitions[] =
{
    /*                    Signal Id,                                      Function */

    /* All other events are saved until initialisation is complete */
    fsm_event_table_entry(UNIFI_NME_MGT_GET_VALUE_CFM_ID,                 nme_core_initialising_mgt_get_value_cfm),
};


static const FsmEventEntry idleTransitions[] =
{
    /*                    Signal Id,                                      Function */

    /* -------------------------         Events from the NME SAP          ------------------------- */
    fsm_event_table_entry(UNIFI_NME_RESTRICTED_ACCESS_ENABLE_REQ_ID,      nme_core_restricted_access_enable_req),
    fsm_event_table_entry(UNIFI_NME_RESTRICTED_ACCESS_DISABLE_REQ_ID,     nme_core_restricted_access_disable_req),

    /* Requests that need to be checked against restricted access and if allowed forwarded to another FSM */
    fsm_event_table_entry(UNIFI_NME_PROFILE_SET_REQ_ID,                   nme_core_profile_set_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_DELETE_REQ_ID,                nme_core_profile_delete_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_DELETE_ALL_REQ_ID,            nme_core_profile_delete_all_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_ORDER_SET_REQ_ID,             nme_core_profile_order_set_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_REQ_ID,               nme_core_profile_connect_req),
    fsm_event_table_entry(UNIFI_NME_WPS_REQ_ID,                           nme_core_wps_req),
    fsm_event_table_entry(UNIFI_NME_WPS_CANCEL_REQ_ID,                    nme_core_wps_cancel_req),
    fsm_event_table_entry(UNIFI_NME_CONNECT_REQ_ID,                       nme_core_connect_req),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                    nme_core_disconnect_req),

    /* -------------------------         Events from the MGT SAP          ------------------------- */
    fsm_event_table_entry(UNIFI_NME_MGT_WIFI_ON_CFM_ID,                   nme_core_wifi_on_cfm),
    fsm_event_table_entry(UNIFI_NME_MGT_WIFI_OFF_CFM_ID,                  nme_core_wifi_off_cfm),
    fsm_event_table_entry(UNIFI_NME_MGT_WIFI_OFF_IND_ID,                  nme_core_wifi_off_ind),
    fsm_event_table_entry(UNIFI_NME_MGT_RESTRICTED_ACCESS_ENABLE_CFM_ID,  nme_core_mgt_restricted_access_enable_cfm),
    fsm_event_table_entry(UNIFI_NME_MGT_RESTRICTED_ACCESS_DISABLE_CFM_ID, nme_core_mgt_restricted_access_disable_cfm),

    /* -------------------------         Internal Events          ------------------------- */
    /* Don't do anything with the confirms, only really there as all requests have
     * a matching confirm, so ignoring.
     */
    fsm_event_table_entry(NME_CORE_START_CFM_ID,                          fsm_ignore_event),
    fsm_event_table_entry(NME_CORE_STOP_CFM_ID,                           fsm_ignore_event),

};

static const FsmEventEntry gettingWifiOnConfigTransitions[] =
{
    /*                    Signal Id,                                      Function */

    /* All other events are saved until initialisation is complete */

    /* Handle the configuration values that were requested */
    fsm_event_table_entry(UNIFI_NME_MGT_GET_VALUE_CFM_ID,                 nme_core_wifi_on_mgt_get_value_cfm),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry defaultHandlers[] =
{
                           /* Signal Id,                                  Function */
    fsm_event_table_entry(UNIFI_NME_CONNECTION_STATUS_GET_REQ_ID,         nme_core_connection_status_get_req),
};

/** NME Core state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                           State                       State                         Save     */
   /*                           Name                        Transitions                    *       */
   fsm_state_table_entry(FSMSTATE_initialising,          initialisingTransitions,         TRUE),
   fsm_state_table_entry(FSMSTATE_idle,                  idleTransitions,                 FALSE),
   fsm_state_table_entry(FSMSTATE_gettingWifiOnConfig,   gettingWifiOnConfigTransitions,  TRUE),
};

const FsmProcessStateMachine nme_core_fsm =
{
#ifdef FSM_DEBUG
       "Nme Core",                              /* SM Process Name       */
#endif
       NME_CORE_PROCESS,                        /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},        /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, defaultHandlers, FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),  /* Ignore Event handers */
       nme_core_init_fsm,                       /* Entry Function        */
       nme_core_reset_fsm,                      /* Reset Function        */
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
