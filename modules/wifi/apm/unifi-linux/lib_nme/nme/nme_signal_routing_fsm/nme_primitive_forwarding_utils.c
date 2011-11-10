/** @file nme_primitive_forwarding_utisl.c
 *
 * Network Management Entity Primitive Forwarding utils
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   A large portion of the NME involves forwarding of primitives between
 *   the NME SAP and the SME MGT SAP. As the handling of the primitives is
 *   spread across several FSMs but they all need to forward various
 *   primitives.
 *
 *   We've decided to have common forwarding utils. This also makes it
 *   easier to check that we're handling all the primitives, which would
 *   be more difficult if the forwarding utils were spread across several
 *   files.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_signal_routing_fsm/nme_primitive_forwarding_utils.c#3 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "nme_top_level_fsm/nme_top_level_fsm.h"


/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/
/**
 * @brief
 *   If the specified fsm instance is different to the calling context
 *   then the event is forwarded to the other FSM
 *
 * @par Description
 *   see brief
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] destination FSM instance
 * @param[in] FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_signal_routing_forward_to_another_nme_fsm(
        FsmContext* pContext,
        CsrUint16 destinationFsmInstance,
        const FsmEvent* pEvt
)
{
    /* only forward is if the recipient is different to the calling context */
    if(pContext->currentInstance->instanceId != destinationFsmInstance)
    {
        sme_trace_debug((TR_NME_CORE_FSM, "nme_signal_routing_forward_to_another_nme_fsm() forwarding evt to %s ", fsm_process_name_by_id(pContext, destinationFsmInstance) ));
        fsm_forward_event(pContext, destinationFsmInstance, pEvt);
    }
    else
    {
        sme_trace_debug((TR_NME_CORE_FSM, "nme_signal_routing_forward_to_another_nme_fsm() evt was for %s", fsm_process_name_by_id(pContext, destinationFsmInstance) ));
        nme_free_event(pEvt);
    }
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * @brief
 *   Forwards the received CSR_WIFI_NME event to the UNIFI_MGT SAP
 *
 * @par Description
 *   Forwards the received CSR_WIFI_NME event to the UNIFI_MGT SAP
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
void nme_forward_to_unifi_mgt_sap(
        FsmContext* pContext,
        const FsmEvent* pReq)
{
    sme_trace_entry((TR_NME_CORE_FSM, "nme_core_forward_to_unifi_mgt_sap(ReqID: %x)",pReq->id));

    switch(pReq->id)
    {
        case UNIFI_NME_WIFI_ON_REQ_ID:
        {
            const UnifiNmeWifiOnReq_Evt* pWifiOnReq = (UnifiNmeWifiOnReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, pWifiOnReq->appHandle))
            {
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_WIFI_ON_CFM_ID, pWifiOnReq->appHandle);
                call_unifi_nme_mgt_wifi_on_req(pContext,
                                               NME_APP_HANDLE(pContext),
                                               &pWifiOnReq->address,
                                               pWifiOnReq->mibFilesCount,
                                               pWifiOnReq->mibFiles);
            }
            else
            {
                call_unifi_nme_wifi_on_cfm(pContext, pWifiOnReq->appHandle, unifi_Restricted);
            }
            free_unifi_mgt_wifi_on_req_contents((UnifiMgtWifiOnReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_WIFI_OFF_REQ_ID:
        {
            const UnifiNmeWifiOffReq_Evt* pWifiOffReq = (UnifiNmeWifiOffReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, pWifiOffReq->appHandle))
            {
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_WIFI_OFF_CFM_ID, pWifiOffReq->appHandle);
                call_unifi_nme_mgt_wifi_off_req(pContext, NME_APP_HANDLE(pContext));
            }
            else
            {
                call_unifi_nme_wifi_off_cfm(pContext, pWifiOffReq->appHandle, unifi_Restricted);
            }
            break;
        }
        case UNIFI_NME_WIFI_FLIGHTMODE_REQ_ID:
        {
            const UnifiNmeWifiFlightmodeReq_Evt* pWifiFlightModeReq = (UnifiNmeWifiFlightmodeReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, pWifiFlightModeReq->appHandle))
            {
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_WIFI_FLIGHTMODE_CFM_ID, pWifiFlightModeReq->appHandle);
                call_unifi_nme_mgt_wifi_flightmode_req(pContext,
                                                       NME_APP_HANDLE(pContext),
                                                       &pWifiFlightModeReq->address,
                                                       pWifiFlightModeReq->mibFilesCount,
                                                       pWifiFlightModeReq->mibFiles);
            }
            else
            {
                call_unifi_nme_wifi_flightmode_cfm(pContext, pWifiFlightModeReq->appHandle, unifi_Restricted);
            }
            free_unifi_mgt_wifi_flightmode_req_contents((UnifiMgtWifiFlightmodeReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_SET_VALUE_REQ_ID:
        {
            const UnifiNmeSetValueReq_Evt* req = (UnifiNmeSetValueReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, req->appHandle))
            {
                NmeConfigData* cfg = nme_core_get_nme_config(pContext);
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_SET_VALUE_CFM_ID, req->appHandle);

                /* Add any cloaked profiles to the list */
                if (unifi_CloakedSsidConfigValue == req->appValue.id)
                {
                    unifi_AppValue value;
                    value.id = unifi_CloakedSsidConfigValue;

                    if (cfg->cloakedSsids)
                    {
                        CsrPfree(cfg->cloakedSsids);
                    }
                    cfg->cloakedSsidsCount = req->appValue.unifi_Value_union.cloakedSsids.cloakedSsidsCount;
                    cfg->cloakedSsids      = req->appValue.unifi_Value_union.cloakedSsids.cloakedSsids;

                    value.unifi_Value_union.cloakedSsids.cloakedSsidsCount = nme_profile_manager_get_cloaked_ssids(pContext, &value.unifi_Value_union.cloakedSsids.cloakedSsids);

                    call_unifi_nme_mgt_set_value_req(pContext, NME_APP_HANDLE(pContext), &value);

                    if (value.unifi_Value_union.cloakedSsids.cloakedSsids)
                    {
                        CsrPfree(value.unifi_Value_union.cloakedSsids.cloakedSsids);
                    }

                    /* Do not call the free function as we want to keep the ssid data */
                    break;
                }
                else
                {
                    call_unifi_nme_mgt_set_value_req(pContext, NME_APP_HANDLE(pContext), &req->appValue);
                }
                /* Sniff any SME configuration changes that we're interested in
                 * and are sure that the SME will not reject. As we have to update
                 * the value based on the request as the confirm doesn't contain
                 * the value that was set only the result.
                 */
                if (unifi_SmeConfigValue == req->appValue.id)
                {
                    cfg->enableRestrictedAccess = req->appValue.unifi_Value_union.smeConfig.enableRestrictedAccess;
                }
            }
            else
            {
                call_unifi_nme_set_value_cfm(pContext, req->appHandle, unifi_Restricted, req->appValue.id);
            }
            free_unifi_mgt_set_value_req_contents((UnifiMgtSetValueReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_GET_VALUE_REQ_ID:
        {
            const UnifiNmeGetValueReq_Evt* pGetValueReq = (UnifiNmeGetValueReq_Evt*)pReq;
            nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_GET_VALUE_CFM_ID, pGetValueReq->appHandle);
            call_unifi_nme_mgt_get_value_req(pContext, NME_APP_HANDLE(pContext), pGetValueReq->appValueId);
            break;
        }
        case UNIFI_NME_MIB_SET_REQ_ID:
        {
            const UnifiNmeMibSetReq_Evt* pMibSetReq = (UnifiNmeMibSetReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, pMibSetReq->appHandle))
            {
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_MIB_SET_CFM_ID, pMibSetReq->appHandle);
                call_unifi_nme_mgt_mib_set_req(pContext, NME_APP_HANDLE(pContext), pMibSetReq->mibAttributeLength, pMibSetReq->mibAttribute);
            }
            else
            {
                call_unifi_nme_mib_set_cfm(pContext, pMibSetReq->appHandle, unifi_Restricted);
            }
            free_unifi_mgt_mib_set_req_contents((UnifiMgtMibSetReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_MIB_GET_REQ_ID:
        {
            const UnifiNmeMibGetReq_Evt* pMibGetReq = (UnifiNmeMibGetReq_Evt*)pReq;
            nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_MIB_GET_CFM_ID, pMibGetReq->appHandle);
            call_unifi_nme_mgt_mib_get_req(pContext, NME_APP_HANDLE(pContext), pMibGetReq->mibAttributeLength, pMibGetReq->mibAttribute);
            free_unifi_mgt_mib_get_req_contents((UnifiMgtMibGetReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_MIB_GET_NEXT_REQ_ID:
        {
            const UnifiNmeMibGetNextReq_Evt* pMibGetNextReq = (UnifiNmeMibGetNextReq_Evt*)pReq;
            nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_MIB_GET_NEXT_CFM_ID, pMibGetNextReq->appHandle);
            call_unifi_nme_mgt_mib_get_next_req(pContext, NME_APP_HANDLE(pContext), pMibGetNextReq->mibAttributeLength, pMibGetNextReq->mibAttribute);
            free_unifi_mgt_mib_get_next_req_contents((UnifiMgtMibGetNextReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_SCAN_FULL_REQ_ID:
        {
            const UnifiNmeScanFullReq_Evt* pScanFullReq = (UnifiNmeScanFullReq_Evt*)pReq;
            nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_SCAN_FULL_CFM_ID, pScanFullReq->appHandle);
            call_unifi_nme_mgt_scan_full_req(pContext,
                                             NME_APP_HANDLE(pContext),
                                             pScanFullReq->ssidCount,
                                             pScanFullReq->ssid,
                                             &pScanFullReq->bssid,
                                             pScanFullReq->forceScan,
                                             pScanFullReq->bssType,
                                             pScanFullReq->scanType,
                                             pScanFullReq->channelListCount,
                                             pScanFullReq->channelList,
                                             pScanFullReq->probeIeLength,
                                             pScanFullReq->probeIe);
            free_unifi_mgt_scan_full_req_contents((UnifiMgtScanFullReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_SCAN_RESULTS_GET_REQ_ID:
        {
            const UnifiNmeScanResultsGetReq_Evt* pScanResultsGetReq = (UnifiNmeScanResultsGetReq_Evt*)pReq;
            nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_SCAN_RESULTS_GET_CFM_ID, pScanResultsGetReq->appHandle);
            call_unifi_nme_mgt_scan_results_get_req(pContext, NME_APP_HANDLE(pContext));
            break;
        }
        case UNIFI_NME_SCAN_RESULTS_FLUSH_REQ_ID:
        {
            const UnifiNmeScanResultsFlushReq_Evt* pScanResultsFlushReq = (UnifiNmeScanResultsFlushReq_Evt*)pReq;
            nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_SCAN_RESULTS_FLUSH_CFM_ID, pScanResultsFlushReq->appHandle);
            call_unifi_nme_mgt_scan_results_flush_req(pContext, NME_APP_HANDLE(pContext));
            break;
        }
        case UNIFI_NME_CONNECT_REQ_ID:
        {
            const UnifiNmeConnectReq_Evt* pConnectReq = (UnifiNmeConnectReq_Evt*)pReq;
            nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_CONNECT_CFM_ID, pConnectReq->appHandle);
            call_unifi_nme_mgt_connect_req(pContext, NME_APP_HANDLE(pContext), &pConnectReq->connectionConfig);
            free_unifi_mgt_connect_req_contents((UnifiMgtConnectReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_DISCONNECT_REQ_ID:
        {
            const UnifiNmeDisconnectReq_Evt* pDisconnectReq = (UnifiNmeDisconnectReq_Evt*)pReq;
            nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_DISCONNECT_CFM_ID, pDisconnectReq->appHandle);
            call_unifi_nme_mgt_disconnect_req(pContext, NME_APP_HANDLE(pContext));
            break;
        }
        case UNIFI_NME_MULTICAST_ADDRESS_REQ_ID:
        {
            const UnifiNmeMulticastAddressReq_Evt* pMulticastAddressReq = (UnifiNmeMulticastAddressReq_Evt*)pReq;
            nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_MULTICAST_ADDRESS_CFM_ID, pMulticastAddressReq->appHandle);
            call_unifi_nme_mgt_multicast_address_req(pContext,
                                                    NME_APP_HANDLE(pContext),
                                                     pMulticastAddressReq->action,
                                                     pMulticastAddressReq->setAddressesCount,
                                                     pMulticastAddressReq->setAddresses);
            free_unifi_mgt_multicast_address_req_contents((UnifiMgtMulticastAddressReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_PMKID_REQ_ID:
        {
            const UnifiNmePmkidReq_Evt* pPmkidReq = (UnifiNmePmkidReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, pPmkidReq->appHandle))
            {
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_PMKID_CFM_ID, pPmkidReq->appHandle);
                call_unifi_nme_mgt_pmkid_req(pContext,
                                             NME_APP_HANDLE(pContext),
                                             pPmkidReq->action,
                                             pPmkidReq->setPmkidsCount,
                                             pPmkidReq->setPmkids);
            }
            else
            {
                call_unifi_nme_pmkid_cfm(pContext, pPmkidReq->appHandle, unifi_Restricted, pPmkidReq->action, pPmkidReq->setPmkidsCount, pPmkidReq->setPmkids);
            }
            free_unifi_mgt_pmkid_req_contents((UnifiMgtPmkidReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_KEY_REQ_ID:
        {
            const UnifiNmeKeyReq_Evt* pKeyReq = (UnifiNmeKeyReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, pKeyReq->appHandle))
            {
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_KEY_CFM_ID, pKeyReq->appHandle);
                call_unifi_nme_mgt_key_req(pContext,
                                          NME_APP_HANDLE(pContext),
                                           pKeyReq->action,
                                           (const unifi_Key *)&(pKeyReq->key));
            }
            else
            {
                call_unifi_nme_key_cfm(pContext, pKeyReq->appHandle, unifi_Restricted, pKeyReq->action);
            }
            break;
        }
        case UNIFI_NME_PACKET_FILTER_SET_REQ_ID:
        {
            const UnifiNmePacketFilterSetReq_Evt* pPacketFilterSetReq = (UnifiNmePacketFilterSetReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, pPacketFilterSetReq->appHandle))
            {
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_PACKET_FILTER_SET_CFM_ID, pPacketFilterSetReq->appHandle);
                call_unifi_nme_mgt_packet_filter_set_req(pContext,
                                                         NME_APP_HANDLE(pContext),
                                                         pPacketFilterSetReq->filterLength,
                                                         pPacketFilterSetReq->filter,
                                                         pPacketFilterSetReq->mode,
                                                         &pPacketFilterSetReq->arpFilterAddress);
            }
            else
            {
                call_unifi_nme_packet_filter_set_cfm(pContext, pPacketFilterSetReq->appHandle, unifi_Restricted);
            }
            free_unifi_mgt_packet_filter_set_req_contents((UnifiMgtPacketFilterSetReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_TSPEC_REQ_ID:
        {
            const UnifiNmeTspecReq_Evt* pTspecReq = (UnifiNmeTspecReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, pTspecReq->appHandle))
            {
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_TSPEC_CFM_ID, pTspecReq->appHandle);
                call_unifi_nme_mgt_tspec_req(pContext,
                                             NME_APP_HANDLE(pContext),
                                             pTspecReq->action,
                                             pTspecReq->transactionId,
                                             pTspecReq->strict,
                                             pTspecReq->ctrlMask,
                                             pTspecReq->tspecLength,
                                             pTspecReq->tspec,
                                             pTspecReq->tclasLength,
                                             pTspecReq->tclas);
            }
            else
            {
                call_unifi_nme_tspec_cfm(pContext, pTspecReq->appHandle, unifi_Restricted, pTspecReq->transactionId, unifi_TspecResultUnspecifiedFailure, pTspecReq->tspecLength, pTspecReq->tspec);
            }
            free_unifi_mgt_tspec_req_contents((UnifiMgtTspecReq_Evt*)pReq);
            break;
        }
        case UNIFI_NME_EVENT_MASK_SET_REQ_ID:
        {
            /* Should be handled within the NME, as the NME has to receive all events
             * and then pass upwards to the applications only if the application requested
             * it.
             */
            sme_trace_crit((TR_NME_CORE_FSM, "nme_core_forward_to_unifi_mgt_sap(UNIFI_NME_EVENT_MASK_SET_REQ_ID) should NOT be received here"));
            break;
        }
        case UNIFI_NME_BLACKLIST_REQ_ID:
        {
            const UnifiNmeBlacklistReq_Evt* pBlacklistReq = (UnifiNmeBlacklistReq_Evt*)pReq;
            if (!nme_core_is_access_restricted(pContext, pBlacklistReq->appHandle))
            {
                nme_routing_store_cfm_prim_external(pContext, UNIFI_NME_BLACKLIST_CFM_ID, pBlacklistReq->appHandle);
                call_unifi_nme_mgt_blacklist_req(pContext,
                                                 NME_APP_HANDLE(pContext),
                                                 pBlacklistReq->action,
                                                 pBlacklistReq->setAddressCount,
                                                 pBlacklistReq->setAddresses);

                /* Also need to forward to network selector and from there to the connection manager
                 * if present as the blacklist req may have to be combined for any blacklist that the
                 * NME is managing in relation to any current connection attempt.
                 */
                fsm_forward_event(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, pReq);
            }
            else
            {
                call_unifi_nme_blacklist_cfm(pContext, pBlacklistReq->appHandle, unifi_Restricted, pBlacklistReq->action, pBlacklistReq->setAddressCount, pBlacklistReq->setAddresses);
                free_unifi_mgt_blacklist_req_contents((UnifiMgtBlacklistReq_Evt*)pReq);
            }
            break;
        }
        case UNIFI_NME_RESTRICTED_ACCESS_ENABLE_REQ_ID:
        {
            /* Should be handled in the NME, so this function should never be called to handle it */
            sme_trace_crit((TR_NME_CORE_FSM, "nme_core_forward_to_unifi_mgt_sap(UNIFI_NME_RESTRICTED_ACCESS_ENABLE_REQ_ID) should NOT be received here"));
            break;
        }
        case UNIFI_NME_RESTRICTED_ACCESS_DISABLE_REQ_ID:
        {
            /* Should be handled in the NME, so this function should never be called to handle it */
            sme_trace_crit((TR_NME_CORE_FSM, "nme_core_forward_to_unifi_mgt_sap(UNIFI_NME_RESTRICTED_ACCESS_DISABLE_REQ_ID) should NOT be received here"));
            break;
        }
        default:
        {
            /* Assert! Not handled an event that was sent in from the FSM */
            sme_trace_crit((TR_NME_CORE_FSM, "nme_core_forward_to_unifi_mgt_sap(ReqID: %d) UNHANDLED !!! ",pReq->id));
        }
    }
}

/**
 * @brief
 *   Forwards the received UNIFI_MGT SAP event to the CSR_WIFI_NME SAP
 *
 * @par Description
 *   Forwards the received UNIFI_MGT based on the following rules:
 *   1. Indications are always forwarded to the NME-SAP for all appHandles
 *      that have registered to receive them.
 *   2. Confirmations are checked against the list of outstanding confirms
 *      to determine whether they should be sent to the NME-SAP or directed
 *      to an internal FSM. As such all primitives on the NME MGT SAP must
 *      be received by the core fsm first.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
void nme_forward_to_csr_wifi_nme_sap(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    CsrBool isExternal;
    CsrUint16 fsmInstance;
    void *pCfmAppHandle = NULL;
    sme_trace_entry((TR_NME_CORE_FSM, "nme_forward_to_csr_wifi_nme_sap(EvtID: %x)",pEvt->id));

    switch(pEvt->id)
    {
    case UNIFI_NME_MGT_WIFI_OFF_IND_ID:
    case UNIFI_NME_MGT_SCAN_RESULT_IND_ID:
    case UNIFI_NME_MGT_MEDIA_STATUS_IND_ID:
    case UNIFI_NME_MGT_CONNECTION_QUALITY_IND_ID:
    case UNIFI_NME_MGT_MIC_FAILURE_IND_ID:
    case UNIFI_NME_MGT_PMKID_CANDIDATE_LIST_IND_ID:
    case UNIFI_NME_MGT_ROAM_START_IND_ID:
    case UNIFI_NME_MGT_ROAM_COMPLETE_IND_ID:
    case UNIFI_NME_MGT_ASSOCIATION_START_IND_ID:
    case UNIFI_NME_MGT_ASSOCIATION_COMPLETE_IND_ID:
    case UNIFI_NME_MGT_IBSS_STATION_IND_ID:
    case UNIFI_NME_MGT_TSPEC_IND_ID:
        /* Do not process  */
        break;
    default:
    {
        CsrBool foundCfmInfo = nme_routing_get_cfm_apphandle(pContext, pEvt->id, &pCfmAppHandle, &isExternal, &fsmInstance);
        /* There MUST be a record of the Signal */
        if (!foundCfmInfo)
        {
            sme_trace_crit((TR_NME_CORE_FSM, "nme_forward_to_csr_wifi_nme_sap(EvtID: %x) - no routing info", pEvt->id));
            nme_free_event(pEvt);
            return;
        }
        /* Internal Message... Just forward to the correct FSM */
        if (!isExternal)
        {
            /* FSM_TERMINATE == We do not care... Just throw away */
            if (fsmInstance != FSM_TERMINATE)
            {
                nme_signal_routing_forward_to_another_nme_fsm(pContext, fsmInstance, pEvt);
            }
            else
            {
                sme_trace_debug((TR_NME_CORE_FSM, "Sending FSM not interested in response (EvtID: %x)", pEvt->id));
                nme_free_event(pEvt);
            }
            return;
        }
        break;
    }
    }

    switch(pEvt->id)
    {
        case UNIFI_NME_MGT_WIFI_ON_CFM_ID:
        {
            const UnifiNmeMgtWifiOnCfm_Evt* pWifiOnCfm = (UnifiNmeMgtWifiOnCfm_Evt*)pEvt;
            call_unifi_nme_wifi_on_cfm(pContext, pCfmAppHandle, pWifiOnCfm->status);
            /* The core FSM needs to know that the Wifi status may have changed */
            nme_signal_routing_forward_to_another_nme_fsm(pContext, getNmeContext(pContext)->nmeCoreFsmInstance, pEvt);
            break;
        }
        case UNIFI_NME_MGT_WIFI_OFF_CFM_ID:
        {
            const UnifiNmeMgtWifiOffCfm_Evt* pWifiOffCfm = (UnifiNmeMgtWifiOffCfm_Evt*)pEvt;
            call_unifi_nme_wifi_off_cfm(pContext, pCfmAppHandle, pWifiOffCfm->status);
            /* The core FSM needs to know that the Wifi status may have changed */
            nme_signal_routing_forward_to_another_nme_fsm(pContext, getNmeContext(pContext)->nmeCoreFsmInstance, pEvt);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_WIFI_FLIGHTMODE_CFM_ID:
        {
            const UnifiNmeMgtWifiFlightmodeCfm_Evt* pFlightmodeCfm = (UnifiNmeMgtWifiFlightmodeCfm_Evt*)pEvt;
            call_unifi_nme_wifi_flightmode_cfm(pContext, pCfmAppHandle, pFlightmodeCfm->status);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_SET_VALUE_CFM_ID:
        {
            const UnifiNmeMgtSetValueCfm_Evt* pSetValueCfm = (UnifiNmeMgtSetValueCfm_Evt*)pEvt;
            call_unifi_nme_set_value_cfm(pContext, pCfmAppHandle, pSetValueCfm->status, pSetValueCfm->appValueId);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_GET_VALUE_CFM_ID:
        {
            const UnifiNmeMgtGetValueCfm_Evt* pGetValueCfm = (UnifiNmeMgtGetValueCfm_Evt*)pEvt;
            call_unifi_nme_get_value_cfm(pContext,
                                         pCfmAppHandle,
                                         pGetValueCfm->status,
                                         (const unifi_AppValue *)&(pGetValueCfm->appValue));
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_MIB_SET_CFM_ID:
        {
            const UnifiNmeMgtMibSetCfm_Evt* pMibSetCfm = (UnifiNmeMgtMibSetCfm_Evt*)pEvt;
            call_unifi_nme_mib_set_cfm(pContext, pCfmAppHandle, pMibSetCfm->status);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_MIB_GET_CFM_ID:
        {
            const UnifiNmeMgtMibGetCfm_Evt* pMibGetCfm = (UnifiNmeMgtMibGetCfm_Evt*)pEvt;
            call_unifi_nme_mib_get_cfm(pContext,
                                       pCfmAppHandle,
                                       pMibGetCfm->status,
                                       pMibGetCfm->mibAttributeLength,
                                       pMibGetCfm->mibAttribute);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_MIB_GET_NEXT_CFM_ID:
        {
            const UnifiNmeMgtMibGetNextCfm_Evt* pMibGetNextCfm = (UnifiNmeMgtMibGetNextCfm_Evt*)pEvt;
            call_unifi_nme_mib_get_next_cfm(pContext,
                                            pCfmAppHandle,
                                            pMibGetNextCfm->status,
                                            pMibGetNextCfm->mibAttributeLength,
                                            pMibGetNextCfm->mibAttribute);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_SCAN_FULL_CFM_ID:
        {
            const UnifiNmeMgtScanFullCfm_Evt* pScanFullCfm = (UnifiNmeMgtScanFullCfm_Evt*)pEvt;
            call_unifi_nme_scan_full_cfm(pContext, pCfmAppHandle, pScanFullCfm->status);
            break;
        }
        case UNIFI_NME_MGT_SCAN_RESULTS_GET_CFM_ID:
        {
            const UnifiNmeMgtScanResultsGetCfm_Evt* pScanResultsGetCfm = (UnifiNmeMgtScanResultsGetCfm_Evt*)pEvt;
            call_unifi_nme_scan_results_get_cfm(pContext,
                                                pCfmAppHandle,
                                                pScanResultsGetCfm->status,
                                                pScanResultsGetCfm->scanResultsCount,
                                                (const unifi_ScanResult *)(pScanResultsGetCfm->scanResults));
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_CONNECT_CFM_ID:
        {
            const UnifiNmeMgtConnectCfm_Evt* pConnectCfm = (UnifiNmeMgtConnectCfm_Evt*)pEvt;
            call_unifi_nme_connect_cfm(pContext, pCfmAppHandle, pConnectCfm->status);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_DISCONNECT_CFM_ID:
        {
            const UnifiNmeMgtDisconnectCfm_Evt* pDisconnectCfm = (UnifiNmeMgtDisconnectCfm_Evt*)pEvt;
            call_unifi_nme_disconnect_cfm(pContext, pCfmAppHandle, pDisconnectCfm->status);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_MULTICAST_ADDRESS_CFM_ID:
        {
            const UnifiNmeMgtMulticastAddressCfm_Evt* pMulticastAddressCfm = (UnifiNmeMgtMulticastAddressCfm_Evt*)pEvt;
            call_unifi_nme_multicast_address_cfm(pContext,
                                                 pCfmAppHandle,
                                                 pMulticastAddressCfm->status,
                                                 pMulticastAddressCfm->action,
                                                 pMulticastAddressCfm->getAddressesCount,
                                                 pMulticastAddressCfm->getAddresses);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_MIC_FAILURE_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndMicFailure, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtMicFailureInd_Evt* pMicFailureInd = (UnifiNmeMgtMicFailureInd_Evt*)pEvt;
                call_unifi_nme_mic_failure_ind(pContext,
                                               appHandleCount,
                                               appHandles,
                                               pMicFailureInd->secondFailure,
                                               pMicFailureInd->count,
                                               (const unifi_MACAddress *)&(pMicFailureInd->address),
                                               pMicFailureInd->keyType,
                                               pMicFailureInd->keyId,
                                               pMicFailureInd->tsc);
            }
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_PMKID_CANDIDATE_LIST_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndPmkidCandidateList, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtPmkidCandidateListInd_Evt* pPmkidCandidateListInd = (UnifiNmeMgtPmkidCandidateListInd_Evt*)pEvt;
                call_unifi_nme_pmkid_candidate_list_ind(pContext,
                                                        appHandleCount,
                                                        appHandles,
                                                        pPmkidCandidateListInd->pmkidCandidatesCount,
                                                        pPmkidCandidateListInd->pmkidCandidates);

            }
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_PMKID_CFM_ID:
        {
            const UnifiNmeMgtPmkidCfm_Evt* pPmkidCfm = (UnifiNmeMgtPmkidCfm_Evt*)pEvt;
            call_unifi_nme_pmkid_cfm(pContext,
                                     pCfmAppHandle,
                                     pPmkidCfm->status,
                                     pPmkidCfm->action,
                                     pPmkidCfm->getPmkidsCount,
                                     pPmkidCfm->getPmkids);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_KEY_CFM_ID:
        {
            const UnifiNmeMgtKeyCfm_Evt* pKeyCfm = (UnifiNmeMgtKeyCfm_Evt*)pEvt;
            call_unifi_nme_key_cfm(pContext, pCfmAppHandle, pKeyCfm->status, pKeyCfm->action);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_PACKET_FILTER_SET_CFM_ID:
        {
            const UnifiNmeMgtPacketFilterSetCfm_Evt* pPacketFilterSetCfm = (UnifiNmeMgtPacketFilterSetCfm_Evt*)pEvt;
            call_unifi_nme_packet_filter_set_cfm(pContext, pCfmAppHandle, pPacketFilterSetCfm->status);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_TSPEC_CFM_ID:
        {
            const UnifiNmeMgtTspecCfm_Evt* pTspecCfm = (UnifiNmeMgtTspecCfm_Evt*)pEvt;
            call_unifi_nme_tspec_cfm(pContext,
                                     pCfmAppHandle,
                                     pTspecCfm->status,
                                     pTspecCfm->transactionId,
                                     pTspecCfm->tspecResultCode,
                                     pTspecCfm->tspecLength,
                                     pTspecCfm->tspec);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_TSPEC_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndTspec, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtTspecInd_Evt* pTspecInd = (UnifiNmeMgtTspecInd_Evt*)pEvt;
                call_unifi_nme_tspec_ind(pContext,
                                         appHandleCount,
                                         appHandles,
                                         pTspecInd->transactionId,
                                         pTspecInd->tspecResultCode,
                                         pTspecInd->tspecLength,
                                         pTspecInd->tspec);
            }
            nme_free_event(pEvt);
           break;
        }
        case UNIFI_NME_MGT_SCAN_RESULTS_FLUSH_CFM_ID:
        {
            const UnifiNmeMgtScanResultsFlushCfm_Evt* pScanResultsFlushCfm = (UnifiNmeMgtScanResultsFlushCfm_Evt*)pEvt;
            call_unifi_nme_scan_results_flush_cfm(pContext, pCfmAppHandle, pScanResultsFlushCfm->status);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_BLACKLIST_CFM_ID:
        {
            const UnifiNmeMgtBlacklistCfm_Evt* pBlacklistCfm = (UnifiNmeMgtBlacklistCfm_Evt*)pEvt;
            call_unifi_nme_blacklist_cfm(pContext,
                                         pCfmAppHandle,
                                         pBlacklistCfm->status,
                                         pBlacklistCfm->action,
                                         pBlacklistCfm->getAddressCount,
                                         (const unifi_MACAddress *)(pBlacklistCfm->getAddresses));
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_RESTRICTED_ACCESS_ENABLE_CFM_ID:
        {
            const UnifiNmeMgtRestrictedAccessEnableCfm_Evt* pActivateCfm = (UnifiNmeMgtRestrictedAccessEnableCfm_Evt*)pEvt;
            call_unifi_nme_restricted_access_enable_cfm(pContext,
                                                        pCfmAppHandle,
                                                        pActivateCfm->status);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_RESTRICTED_ACCESS_DISABLE_CFM_ID:
        {
            const UnifiNmeMgtRestrictedAccessDisableCfm_Evt* pDeactivateCfm = (UnifiNmeMgtRestrictedAccessDisableCfm_Evt*)pEvt;
            call_unifi_nme_restricted_access_disable_cfm(pContext,
                                                         pCfmAppHandle,
                                                         pDeactivateCfm->status);
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_ROAM_START_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndRoamStart, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtRoamStartInd_Evt* pRoamStartInd = (UnifiNmeMgtRoamStartInd_Evt*)pEvt;
                call_unifi_nme_roam_start_ind(pContext,
                                              appHandleCount,
                                              appHandles,
                                              pRoamStartInd->roamReason,
                                              pRoamStartInd->reason80211);
            }
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_ROAM_COMPLETE_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndRoamComplete, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtRoamCompleteInd_Evt* pRoamCompleteInd = (UnifiNmeMgtRoamCompleteInd_Evt*)pEvt;
                call_unifi_nme_roam_complete_ind(pContext,
                                                 appHandleCount,
                                                 appHandles,
                                                 pRoamCompleteInd->status);
            }
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_ASSOCIATION_START_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndAssociationStart, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtAssociationStartInd_Evt* pAssociationStartInd = (UnifiNmeMgtAssociationStartInd_Evt*)pEvt;
                call_unifi_nme_association_start_ind(pContext,
                                                     appHandleCount,
                                                     appHandles,
                                                     (const unifi_MACAddress *)&(pAssociationStartInd->address),
                                                     (const unifi_SSID *)&(pAssociationStartInd->ssid));

            }
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_ASSOCIATION_COMPLETE_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndAssociationComplete, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtAssociationCompleteInd_Evt* pAssociationCompleteInd = (UnifiNmeMgtAssociationCompleteInd_Evt*)pEvt;
                call_unifi_nme_association_complete_ind(pContext,
                                                        appHandleCount,
                                                        appHandles,
                                                        pAssociationCompleteInd->status,
                                                        (const unifi_ConnectionInfo *)&(pAssociationCompleteInd->connectionInfo),
                                                        pAssociationCompleteInd->deauthReason);

            }
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_EVENT_MASK_SET_CFM_ID:
        {
            /* Should be handled within the NME, as the NME has to receive all events
             * and then pass upwards to the applications.
             */
            sme_trace_crit((TR_NME_CORE_FSM, "nme_forward_to_csr_wifi_nme_sap(UNIFI_NME_MGT_EVENT_MASK_SET_CFM_ID) should NOT be received here"));
            break;
        }
        case UNIFI_NME_MGT_WIFI_OFF_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndWifiOff, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtWifiOffInd_Evt* pWifiOffInd = (UnifiNmeMgtWifiOffInd_Evt*)pEvt;
                call_unifi_nme_wifi_off_ind(pContext,
                                            appHandleCount,
                                            appHandles,
                                            pWifiOffInd->controlIndication);
                /* The core FSM needs to know that the Wifi status may have changed */
                nme_signal_routing_forward_to_another_nme_fsm(pContext, getNmeContext(pContext)->nmeCoreFsmInstance, pEvt);
            }
            break;
        }
        case UNIFI_NME_MGT_SCAN_RESULT_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndScanResult, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtScanResultInd_Evt* pScanResultInd = (UnifiNmeMgtScanResultInd_Evt*)pEvt;
                call_unifi_nme_scan_result_ind(pContext,
                                               appHandleCount,
                                               appHandles,
                                               (const unifi_ScanResult *)&(pScanResultInd->result));
            }
            /* Do not free as this is forwarded to the profile manager */
            break;
        }
        case UNIFI_NME_MGT_MEDIA_STATUS_IND_ID:
        {
            const UnifiNmeMgtMediaStatusInd_Evt* pMediaStatusInd = (UnifiNmeMgtMediaStatusInd_Evt*)pEvt;
            NmeConfigData* pNmeConfig = nme_core_get_nme_config(pContext);

            /* Pass up any connected indications but we only pass up disconnect
             * if connected was the previously reported indication.
             */
            if (unifi_MediaConnected == pMediaStatusInd->mediaStatus ||
                pNmeConfig->reportedConnected)
            {
                void* appHandles;
                CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndMediaStatus, &appHandles);

                /* Record what we've reported. */
                if (unifi_MediaDisconnected == pMediaStatusInd->mediaStatus)
                {
                    pNmeConfig->reportedConnected = FALSE;
                }
                else
                {
                    pNmeConfig->reportedConnected = TRUE;
                }

                if (appHandleCount)
                {
                    call_unifi_nme_media_status_ind(pContext,
                                                    appHandleCount,
                                                    appHandles,
                                                    pMediaStatusInd->mediaStatus,
                                                    (const unifi_ConnectionInfo *)&(pMediaStatusInd->connectionInfo),
                                                    pMediaStatusInd->disassocReason,
                                                    pMediaStatusInd->deauthReason);
                }

            }
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_CONNECTION_QUALITY_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndConnectionQuality, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtConnectionQualityInd_Evt* pConnectionQualityInd = (UnifiNmeMgtConnectionQualityInd_Evt*)pEvt;
                call_unifi_nme_connection_quality_ind(pContext,
                                                      appHandleCount,
                                                      appHandles,
                                                      (const unifi_LinkQuality *)&(pConnectionQualityInd->linkQuality));
            }
            nme_free_event(pEvt);
            break;
        }
        case UNIFI_NME_MGT_IBSS_STATION_IND_ID:
        {
            void* appHandles;
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndIbssStation, &appHandles);
            if (appHandleCount)
            {
                const UnifiNmeMgtIbssStationInd_Evt* pIbssStationInd = (UnifiNmeMgtIbssStationInd_Evt*)pEvt;
                call_unifi_nme_ibss_station_ind(
                        pContext,
                        appHandleCount,
                        appHandles,
                        (const unifi_MACAddress *)&(pIbssStationInd->address),
                        pIbssStationInd->isconnected);

            }
            nme_free_event(pEvt);
            break;
        }
        default:
        {
            /* Assert! Not handled an event that was sent in from the FSM */
            sme_trace_crit((TR_NME_CORE_FSM, "nme_core_forward_to_csr_wifi_nme_sap(EvtID: %d) - UNHANDLED!!!",pEvt->id));
            nme_free_event(pEvt);
        }
    }
}
