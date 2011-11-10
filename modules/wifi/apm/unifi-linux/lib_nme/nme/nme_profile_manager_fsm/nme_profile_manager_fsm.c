/** @file nme_profile_manager_fsm.c
 *
 * Network Management Entity Profile Manager FSM Implementation
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
 *   NME Profile Manager FSM Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_profile_manager_fsm/nme_profile_manager_fsm.c#8 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_profile_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_cstl/csr_wifi_list.h"
#include "nme_top_level_fsm/nme_top_level_fsm.h"

/* MACROS *******************************************************************/

#define MAX_PROFILE_CONNECT_INTERVAL (15 * 60000) /* 15 Minutes */
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
   FSMSTATE_idle,
   FSMSTATE_scanning,
   FSMSTATE_monitoring,
   FSMSTATE_connecting,
   FSMSTATE_connected,
   FSMSTATE_MAX_STATE
} FsmState;

/**
 * @brief
 *   Profile list node
 *
 * @par Description
 *   Struct defining the Profile list node based on the generic csr_list
 */
typedef struct nmeProfileListNode_t
{
    csr_list_node node;
    unifi_Profile profile;
} nmeProfileListNode_t;

/**
 * @brief
 *   Scan list node
 *
 * @par Description
 *   Struct defining the Profile list node based on the generic csr_list
 */
typedef struct nmeScanListNode_t
{
    csr_list_node node;
    unifi_ProfileIdentity id;
    CsrUint32 failedConnectAttempts;
    CsrUint32 lastConnectAttempt;
} nmeScanListNode_t;



/**
 * @brief
 *   Profile order list
 *
 * @par Description
 *   Struct defining the Profile order list
 */
typedef struct nmeProfileOrderList_t
{
    CsrUint8 count;
    unifi_ProfileIdentity *pProfileIdentitys;
} nmeProfileOrderList_t;


/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData
{
    csr_list profileList;
    csr_list scanList;
    nmeProfileOrderList_t nmeProfileOrderList;

    CsrBool havePerformedScan;

    /* List of profiles to try and auto connect to
     * Managed separately to the actual profiles themselves to have a clear
     * split between the data owned by the user application (ie the profiles)
     * and that owned by the NME.
     */
    CsrUint8               autoConnectProfilesCount;
    CsrUint8               autoConnectProfilesIndex;
    unifi_ProfileIdentity* autoConnectProfiles;

} FsmData;

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief Sends an update cloaked SSID list to the SME
 *
 * @par Description:
 * See brief
 *
 * @param[in] FsmContext* : context
 *
 * @return
 *      None
 */
static void nme_profile_update_cloaked_ssids(FsmContext* pContext)
{
    unifi_AppValue value;

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_update_cloaked_ssids()"));

    value.id = unifi_CloakedSsidConfigValue;
    value.unifi_Value_union.cloakedSsids.cloakedSsidsCount = nme_profile_manager_get_cloaked_ssids(pContext, &value.unifi_Value_union.cloakedSsids.cloakedSsids);

    /* Not interested in the response */
    nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_SET_VALUE_CFM_ID, FSM_TERMINATE);
    call_unifi_nme_mgt_set_value_req(pContext, NME_APP_HANDLE(pContext), &value);

    if (value.unifi_Value_union.cloakedSsids.cloakedSsids)
    {
        CsrPfree(value.unifi_Value_union.cloakedSsids.cloakedSsids);
    }
}

/**
 * @brief Gets a required node from the scan list
 *
 * @par Description:
 * This function finds a cached scan from the list
 *
 * @param[in] csr_list * : scan List
 * @param[in] const unifi_MACAddress * : MAC address to search for
 *
 * @return
 *      nmeScanListNode_t* node in the list or NULL
 */
static nmeScanListNode_t* nme_profile_scan_node_get(csr_list *scanList, const unifi_MACAddress* bssid)
{
    nmeScanListNode_t* node;

    require(TR_NME_PMGR_FSM, NULL != scanList);
    require(TR_NME_PMGR_FSM, NULL != bssid);

    for (node = csr_list_gethead_t(nmeScanListNode_t *, scanList); NULL != node;
         node = csr_list_getnext_t(nmeScanListNode_t *, scanList, &(node->node)))
    {
        if(0 == CsrMemCmp(bssid->data, node->id.bssid.data, sizeof(bssid->data)))
        {
            return node;
        }
    }
    return NULL;
}

/**
 * @brief Reset the scan data for a fresh attempt
 *
 * @param[in] csr_list * : scan List
 *
 * @return
 *      void
 */
static void nme_profile_scan_list_reset_data(csr_list *scanList)
{
    nmeScanListNode_t* node;

    require(TR_NME_PMGR_FSM, NULL != scanList);

    for (node = csr_list_gethead_t(nmeScanListNode_t *, scanList); NULL != node;
         node = csr_list_getnext_t(nmeScanListNode_t *, scanList, &(node->node)))
    {
        node->failedConnectAttempts = 0;
    }
}

/**
 * @brief Determine whether there is another connection attempt to
 *        be made or not.
 *
 * @par Description:
 *     If the list of profiles to attempt to connect to have all been tried
 *     then see if a full scan is required, otherwise just wait for any further
 *     scan result indications.
 *     If there are still profiles to attempt to connect to then try the next one.
 *
 * @param[in] FsmContext * : context
 *
 * @return
 *      None
 */
static void nme_profile_attempt_next_autoconnect(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_attempt_next_autoconnect()"));

    /* If we've tried all the possible profiles at the moment then either
     * perform a scan or just monitor the incoming scan result indications.
     */
    if (pFsmData->autoConnectProfilesIndex == pFsmData->autoConnectProfilesCount)
    {
        sme_trace_info((TR_NME_PMGR_FSM, "nme_profile_attempt_next_autoconnect() No more profiles to try"));
        if (!pFsmData->havePerformedScan)
        {
            sme_trace_info((TR_NME_PMGR_FSM, "nme_profile_attempt_next_autoconnect() Trying a scan"));
            pFsmData->havePerformedScan = TRUE;
            nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_MGT_SCAN_FULL_CFM_ID, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
            call_unifi_nme_mgt_scan_full_req(pContext,
                                             NME_APP_HANDLE(pContext),
                                             0,           /* ssidCount */
                                             NULL,        /* ssid's    */
                                             &NmeBroadcastBssid,
                                             FALSE,       /* Do not force the scan */
                                             unifi_AnyBss,
                                             unifi_ScanAll,
                                             0,           /* channelListCount */
                                             NULL,        /* channelList */
                                             0,           /* probeIeLength */
                                             NULL         /* probeIe */);

            fsm_next_state(pContext, FSMSTATE_scanning);
        }
        else
        {
            fsm_next_state(pContext, FSMSTATE_monitoring);
        }
        return;
    }

    sme_trace_debug((TR_NME_PMGR_FSM, "nme_profile_attempt_next_autoconnect(id: %s, %s)",
                     trace_unifi_SSID(&(pFsmData->autoConnectProfiles[pFsmData->autoConnectProfilesIndex].ssid), getNMESSIDBuffer(pContext)),
                     trace_unifi_MACAddress(pFsmData->autoConnectProfiles[pFsmData->autoConnectProfilesIndex].bssid, getNMEMacAddressBuffer(pContext))));
    send_unifi_nme_profile_connect_req(pContext,
                                       getNmeContext(pContext)->nmeNetworkSelectorFsmInstance,
                                       NULL,
                                       pFsmData->autoConnectProfiles[pFsmData->autoConnectProfilesIndex]);
    pFsmData->autoConnectProfilesIndex++;
    fsm_next_state(pContext, FSMSTATE_connecting);
}


/**
 * @brief Build the list of profiles to attempt to auto connect to.
 *
 * @par Description:
 *     The list is generated based on the profile identities present
 *     in the profile order list and the networks present in the
 *     scan list.
 *     To avoid retrying the same network within a short period the
 *     scan list tracks connection attempts since the last full scan
 *     and a network will not be re-attempted within a defined period.
 *
 * @param[in] FsmContext * : context
 *
 * @return
 *      None
 */
static void nme_profile_build_auto_connect_profiles(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    unifi_ProfileIdentity *pProfileIdentity = pFsmData->nmeProfileOrderList.pProfileIdentitys;
    CsrUint8 i = 0;
    CsrUint32 now = fsm_get_time_of_day_ms(pContext);

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_build_auto_connect_profiles()"));

    sme_trace_debug_code(
    {
        unifi_ProfileIdentity* dbgProfileIdentity = pFsmData->nmeProfileOrderList.pProfileIdentitys;
        for (i = 0; i < pFsmData->nmeProfileOrderList.count; i++)
        {
            sme_trace_debug((TR_NME_PMGR_FSM, "nme_profile_build_auto_connect_profiles() prefered: %s, %s",
                                 trace_unifi_MACAddress(dbgProfileIdentity->bssid, getNMEMacAddressBuffer(pContext)),
                                 trace_unifi_SSID(&(dbgProfileIdentity->ssid), getNMESSIDBuffer(pContext))));
            dbgProfileIdentity++;
        }

        {
        nmeScanListNode_t* node;

        for (node = csr_list_gethead_t(nmeScanListNode_t *, &pFsmData->scanList); NULL != node;
             node = csr_list_getnext_t(nmeScanListNode_t *, &pFsmData->scanList, &(node->node)))
        {
            sme_trace_debug((TR_NME_PMGR_FSM, "nme_profile_build_auto_connect_profiles() node: %s, %s",
                                trace_unifi_MACAddress(node->id.bssid, getNMEMacAddressBuffer(pContext)),
                                trace_unifi_SSID(&(node->id.ssid), getNMESSIDBuffer(pContext))));
        }
        }
    }
    );

    /* Reset the autoConnect list */
    CsrPfree(pFsmData->autoConnectProfiles);
    pFsmData->autoConnectProfilesIndex = 0;
    pFsmData->autoConnectProfilesCount = 0;
    pFsmData->autoConnectProfiles = CsrPmalloc(sizeof(unifi_ProfileIdentity) * pFsmData->nmeProfileOrderList.count);

    /* Build connection list Any profile Id in the ordered list that
     * matches a scan result is added to the autoconnect list*/
    for (i = 0; i < pFsmData->nmeProfileOrderList.count; i++)
    {
        nmeScanListNode_t* node = NULL;
        for (node = csr_list_gethead_t(nmeScanListNode_t *, &pFsmData->scanList); NULL != node;
             node = csr_list_getnext_t(nmeScanListNode_t *, &pFsmData->scanList, &(node->node)))
        {
            CsrBool match = FALSE;

            if ((0 == node->id.ssid.length ||
                 0 == CsrMemCmp(node->id.ssid.ssid, pProfileIdentity->ssid.ssid, pProfileIdentity->ssid.length)) &&
                (0 == CsrMemCmp(node->id.bssid.data, pProfileIdentity->bssid.data, sizeof(pProfileIdentity->bssid.data)) ||
                 0 == CsrMemCmp(NmeBroadcastBssid.data, pProfileIdentity->bssid.data, sizeof(pProfileIdentity->bssid.data))))
            {
                match = TRUE;
            }

            if (match)
            {
                CsrUint32 nextConnectTime = CsrTimeAdd(node->lastConnectAttempt, 10000 * node->failedConnectAttempts);
                if ((10000 * node->failedConnectAttempts) > MAX_PROFILE_CONNECT_INTERVAL)
                {
                    nextConnectTime = CsrTimeAdd(node->lastConnectAttempt, MAX_PROFILE_CONNECT_INTERVAL);
                }

                /* Check the last connect attempt time AND failure count */
                if(node->failedConnectAttempts &&
                   CsrTimeGt(nextConnectTime, now))
                {
                    sme_trace_debug((TR_NME_PMGR_FSM, "nme_profile_build_auto_connect_profiles() filter: %s, %s Attempts = %d Interval=%d Last %d ms ago : Too soon! Do not connect",
                                            trace_unifi_MACAddress(node->id.bssid, getNMEMacAddressBuffer(pContext)),
                                            trace_unifi_SSID(&(node->id.ssid), getNMESSIDBuffer(pContext)),
                                            node->failedConnectAttempts,
                                            10000 * node->failedConnectAttempts,
                                            CsrTimeSub(now, node->lastConnectAttempt)));
                    match = FALSE;
                }
            }
            if (match)
            {
                sme_trace_debug((TR_NME_PMGR_FSM, "nme_profile_build_auto_connect_profiles() match: %s, %s matched",
                                                  trace_unifi_SSID(&(node->id.ssid), getNMESSIDBuffer(pContext)),
                                                  trace_unifi_MACAddress(node->id.bssid, getNMEMacAddressBuffer(pContext))));

                node->lastConnectAttempt = now;
                node->failedConnectAttempts++;

                pFsmData->autoConnectProfiles[pFsmData->autoConnectProfilesCount].bssid = pProfileIdentity->bssid;
                pFsmData->autoConnectProfiles[pFsmData->autoConnectProfilesCount].ssid  = pProfileIdentity->ssid;
                pFsmData->autoConnectProfilesCount++;
                break;
            }
        }
        pProfileIdentity++;
    }

    if (pFsmData->autoConnectProfilesCount == 0)
    {
        CsrPfree(pFsmData->autoConnectProfiles);
        pFsmData->autoConnectProfiles = NULL;
    }
}

/**
 * @brief Determines that appropriate auto connect behaviour at this
 *        time.
 *
 * @par Description:
 *     See brief
 *
 * @param[in] FsmContext * : context
 *
 * @return
 *      None
 */
static void nme_profile_start_autoconnect(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_start_autoconnect()"));

    nme_profile_scan_list_reset_data(&pFsmData->scanList);

    /* There is already a "directed" connection present so don't start the
     * auto connect.
     */
    if (!nme_ntw_selector_is_disconnected(pContext))
    {
        sme_trace_debug((TR_NME_PMGR_FSM, "nme_profile_start_autoconnect() Network Selector is busy"));
        fsm_next_state(pContext, FSMSTATE_idle);
        return;
    }

    /* There is no profile order list so we don't know the profiles that
     * we could attempt to auto connect to, so don't start the auto connect.
     */
    if (!pFsmData->nmeProfileOrderList.count)
    {
        sme_trace_info((TR_NME_PMGR_FSM, "nme_profile_start_autoconnect() No preferred profiles"));
        fsm_next_state(pContext, FSMSTATE_idle);
        return;
    }

    /* Build a possible list of profiles to attempt to connect to
     * based on the profiles in the order list and scan results.
     */
    nme_profile_build_auto_connect_profiles(pContext);

    /* If we have anything to connect to try it */
    if (pFsmData->autoConnectProfilesCount)
    {
        nme_profile_attempt_next_autoconnect(pContext);
    }
    else if (!pFsmData->havePerformedScan)
    {
        pFsmData->havePerformedScan = TRUE;
        nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_MGT_SCAN_FULL_CFM_ID, getNmeContext(pContext)->nmeProfileMgrFsmInstance);
        call_unifi_nme_mgt_scan_full_req(pContext,
                                         NME_APP_HANDLE(pContext),
                                         0,           /* ssidCount */
                                         NULL,        /* ssid's    */
                                         &NmeBroadcastBssid,
                                         FALSE,       /* Do not force the scan */
                                         unifi_AnyBss,
                                         unifi_ScanAll,
                                         0,           /* channelListCount */
                                         NULL,        /* channelList */
                                         0,           /* probeIeLength */
                                         NULL         /* probeIe */);

        fsm_next_state(pContext, FSMSTATE_scanning);
    }
    else
    {
        /* Just wait for any changes in the scan list that might trigger a connection attempt. */
        fsm_next_state(pContext, FSMSTATE_monitoring);
    }
}

/**
 * @brief Gets a required node from the profile list
 *
 * @par Description:
 * This function finds a cached profile from the list
 *
 * @ingroup nme_profile_manager
 *
 * @param[in] csr_list * : Profile List
 * @param[in] const unifi_ProfileIdentity * : possible MAC address and/or SSID to search for
 *
 * @return
 *      nmeProfileListNode_t* node in the list or NULL
 */
static nmeProfileListNode_t* nme_profile_node_get(csr_list *pProfileList,
                                                  const unifi_ProfileIdentity *pProfileIdentity)
{
    nmeProfileListNode_t *pNode;
    sme_trace_debug_code(int nodeIndex = 1);

    require(TR_NME_PMGR_FSM, NULL != pProfileList && NULL != pProfileIdentity);

    sme_trace_entry((TR_NME_PMGR_FSM, "profileList(%p) count %d", pProfileList, csr_list_size(pProfileList)));

    for (pNode = csr_list_gethead_t(nmeProfileListNode_t *, pProfileList); NULL != pNode;
         pNode = csr_list_getnext_t(nmeProfileListNode_t *, pProfileList, &(pNode->node)))
    {
        CsrUint8 ssidLen = sizeof(pProfileIdentity->ssid.ssid);
        if (ssidLen > pProfileIdentity->ssid.length)
        {
            ssidLen = pProfileIdentity->ssid.length;
        }
        /* The profile has to match both the BSS id (undefined if we don't care) and the SS id */
        if(0 == CsrMemCmp(pProfileIdentity->bssid.data, pNode->profile.profileIdentity.bssid.data, sizeof(pProfileIdentity->bssid.data)) &&
           pProfileIdentity->ssid.length == pNode->profile.profileIdentity.ssid.length &&
           0 == CsrMemCmp(pProfileIdentity->ssid.ssid, pNode->profile.profileIdentity.ssid.ssid, ssidLen))
        {
            sme_trace_debug((TR_NME_PMGR_FSM, " found matching node %p at index %d", pNode, nodeIndex));
            return pNode;
        }
        sme_trace_debug_code(nodeIndex++);
    }
    sme_trace_debug((TR_NME_PMGR_FSM, " NO matching node"));
    return NULL;
}

/**
 * @brief Deletes the supplied node from the profile list
 *
 * @par Description:
 * See brief
 *
 * @ingroup nme_profile_manager
 *
 * @param[in] csr_list * : Profile List
 * @param[in] nmeProfileListNode_t * : profile node
 *
 * @return
 *      None
 */
static void nme_profile_node_delete(csr_list* pProfileList, nmeProfileListNode_t *pNode)
{
    UnifiNmeProfileSetReq_Evt freeEvt;
    csr_list_result removeResult=csr_list_not_found;
    require(TR_NME_PMGR_FSM, NULL != pProfileList && NULL != pNode);

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_node_delete(%p)", pNode));

    /* Clean up the profile Memory. Reuse the autogenerated free_xxx_contents() */
    freeEvt.profile = pNode->profile;
    free_unifi_nme_profile_set_req_contents(&freeEvt);

    removeResult = csr_list_remove(pProfileList, &(pNode->node));
    if (csr_list_success != removeResult)
    {
        sme_trace_warn((TR_NME_PMGR_FSM, "nme_profile_node_delete failed to remove node"));
    }
}

/**
 * @brief
 *   Clears the supplied profile list
 *
 * @par Description
 *   Clears the supplied profile list
 *
 * @param[in] csr_list * : Profile list to clear
 *
 * @return
 *    None
 */
static void nme_profile_list_clear(csr_list* pProfileList)
{
    require(TR_NME_PMGR_FSM, NULL != pProfileList);

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_list_clear(%p)", pProfileList));

    while (!csr_list_isempty(pProfileList))
    {
        nme_profile_node_delete(pProfileList, csr_list_gethead_t(nmeProfileListNode_t *, pProfileList));
    }
}

/**
 * @brief Adds the supplied profile to the profile list
 *
 * @par Description:
 * See brief
 *
 * @ingroup nme_profile_manager
 *
 * @param[in] csr_list * : Profile List
 * @param[in] const unifi_Profile * : profile
 *
 * @return
 *      None
 */
static void nme_profile_node_set(FsmContext* pContext, csr_list* pProfileList, const unifi_Profile *pProfile)
{
    nmeProfileListNode_t *pNode = NULL;
    CsrBool updateCloaked = FALSE;

    require(TR_NME_PMGR_FSM, NULL != pProfileList && NULL != pProfile);
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_node_set - count: %d", csr_list_size(pProfileList)));

    /* Have we already got an existing profile that needs to be replaced */
    pNode = nme_profile_node_get(pProfileList, &(pProfile->profileIdentity));
    if (NULL != pNode)
    {
        /* Update if the profile is cloakedSsid has changed */
        updateCloaked = pProfile->cloakedSsid != pNode->profile.cloakedSsid;

        /* Update if cloaked and the ssid has changed */
        if ((pProfile->cloakedSsid != pNode->profile.cloakedSsid) ||
            (pProfile->cloakedSsid &&
             pProfile->profileIdentity.ssid.length &&
             0 != CsrMemCmp(pProfile->profileIdentity.ssid.ssid, pNode->profile.profileIdentity.ssid.ssid, pProfile->profileIdentity.ssid.length)))
        {
            updateCloaked = TRUE;
        }
        nme_profile_node_delete(pProfileList, pNode);
    }
    else
    {
        /* Update if the profile is cloaked and has an ssid set */
        updateCloaked = pProfile->cloakedSsid && pProfile->profileIdentity.ssid.length;
    }

    /* Assuming that there is no upper limit on the number of profiles that
     * the list can contain. As this would place a restriction on the number
     * of profiles that the user application/GUI could provide to the user.
     * So assume that any restrictions will be enforced by the application/GUI.
     */

    /* Create new Entry */
    pNode = CsrPmalloc(sizeof(*pNode));
    CsrMemCpy(&pNode->profile, pProfile, sizeof(*pProfile));

    csr_list_insert_tail(pProfileList, list_owns_value, &(pNode->node), pNode);
    sme_trace_debug((TR_NME_PMGR_FSM, "added node %p count: %d", pNode, csr_list_size(pProfileList)));

    if (updateCloaked)
    {
        nme_profile_update_cloaked_ssids(pContext);
    }
}

/**
 * @brief
 *   Generates the required NME PROFILE UPDATE IND to appHandles
 *   that have registered to receive it.
 *
 * @par Description
 *   See brief
 *
 * @param[in]    FsmContext*   : FSM context
 * @param[in]    unifi_Profile*   : profile to include in indication
 *
 * @return
 *   void
 */
void nme_profile_manager_send_profile_update_ind(
        FsmContext* pContext,
        unifi_Profile *pProfile)
{
    void* appHandles;
    CsrUint16 appHandleCount = 0;
    require(TR_NME_PMGR_FSM, NULL != pProfile);

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_manager_send_profile_update_ind()"));

    appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndProfileUpdate, &appHandles);
    if (appHandleCount)
    {
        call_unifi_nme_profile_update_ind(
                pContext,
                appHandleCount,
                appHandles,
                pProfile);
    }
    else
    {
        sme_trace_warn((TR_NME_PMGR_FSM, "No appHandles registered to receive profile_update_ind"));
    }
}

/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   Inital FSM Entry function
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void nme_profile_manager_reset_fsm(FsmContext* pContext)
{
    FsmData* pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_manager_reset_fsm()"));
    nme_profile_list_clear(&pFsmData->profileList);
    csr_list_clear(&pFsmData->scanList);
    if (0 < pFsmData->nmeProfileOrderList.count)
    {
        CsrPfree(pFsmData->nmeProfileOrderList.pProfileIdentitys);
    }

    if (pFsmData->autoConnectProfilesCount)
    {
        pFsmData->autoConnectProfilesIndex = 0;
        pFsmData->autoConnectProfilesCount = 0;
        CsrPfree(pFsmData->autoConnectProfiles);
        pFsmData->autoConnectProfiles = NULL;
    }

    fsm_next_state(pContext, FSMSTATE_stopped);
}

/**
 * @brief
 *   Handles a received UNIFI_NME_PROFILE_SET_REQ
 *
 * @par Description
 *   Handles a received UNIFI_NME_PROFILE_SET_REQ
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_profile_set_req(
        FsmContext* pContext,
        const UnifiNmeProfileSetReq_Evt* pProfileSetReq)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_set_req(id: %s, %s)",
                     trace_unifi_SSID(&(pProfileSetReq->profile.profileIdentity.ssid), getNMESSIDBuffer(pContext)),
                     trace_unifi_MACAddress(pProfileSetReq->profile.profileIdentity.bssid, getNMEMacAddressBuffer(pContext))));
    nme_trace_credentials((const unifi_Credentials *)&(pProfileSetReq->profile.credentials));

    /* Ensure that the profile is sensible */
    if (pProfileSetReq->profile.profileIdentity.ssid.length > sizeof(pProfileSetReq->profile.profileIdentity.ssid.ssid))
    {
        sme_trace_warn((TR_NME_PMGR_FSM, "specified ssid  lenght %d > support %d",
                pProfileSetReq->profile.profileIdentity.ssid.length,
                sizeof(pProfileSetReq->profile.profileIdentity.ssid.ssid)));
        call_unifi_nme_profile_set_cfm(pContext, pProfileSetReq->appHandle, unifi_InvalidParameter);
        return;
    }

    nme_profile_node_set(pContext, &pFsmData->profileList, &(pProfileSetReq->profile));
    call_unifi_nme_profile_set_cfm(pContext, pProfileSetReq->appHandle, unifi_Success);

    /* A profile has been updated. Reset the autoconnect */
    if (pFsmData->nmeProfileOrderList.count &&
        (fsm_current_state(pContext) == FSMSTATE_idle || fsm_current_state(pContext) == FSMSTATE_monitoring))
    {
        pFsmData->havePerformedScan = FALSE;
        nme_profile_start_autoconnect(pContext);
    }
}

/**
 * @brief
 *   Handles a received UNIFI_NME_PROFILE_DELETE_REQ
 *
 * @par Description
 *   Handles a received UNIFI_NME_PROFILE_DELETE_REQ
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_profile_delete_req(
        FsmContext* pContext,
        const UnifiNmeProfileDeleteReq_Evt* pProfileDeleteReq)
{
    FsmData *pFsmData = FSMDATA;
    nmeProfileListNode_t *pProfileNode = NULL;
    unifi_Status status = unifi_NotFound;
    CsrBool wasCloakedSsid = FALSE;

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_delete_req"));

    pProfileNode = nme_profile_node_get(&pFsmData->profileList, &(pProfileDeleteReq->profileIdentity));
    if (NULL != pProfileNode)
    {
        /* If profile was cloaked then update the cloaked list */
        wasCloakedSsid = pProfileNode->profile.cloakedSsid;

        /* @TBD what other actions are required than just removing it from the list? */
        nme_profile_node_delete(&pFsmData->profileList, pProfileNode);

        /* Has to be called after the profile is actually removed. */
        if (wasCloakedSsid)
        {
            nme_profile_update_cloaked_ssids(pContext);
        }

        status = unifi_Success;
    }
    call_unifi_nme_profile_delete_cfm(pContext, pProfileDeleteReq->appHandle, status);
}


/**
 * @brief
 *   Handles a received UNIFI_NME_PROFILE_DELETE_ALL_REQ
 *
 * @par Description
 *   Handles a received UNIFI_NME_PROFILE_DELETE_ALL_REQ
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_profile_delete_all_req(
        FsmContext* pContext,
        const UnifiNmeProfileDeleteAllReq_Evt* pProfileDeleteAllReq)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_delete_all_req"));

    nme_profile_list_clear(&pFsmData->profileList);

    nme_profile_update_cloaked_ssids(pContext);

    /* This is always successful whether we have any profiles or not. */
    call_unifi_nme_profile_delete_all_cfm(pContext, pProfileDeleteAllReq->appHandle, unifi_Success);
}

/**
 * @brief
 *   Handles a received UNIFI_NME_PROFILE_ORDER_SET_REQ
 *
 * @par Description
 *   Handles a received UNIFI_NME_PROFILE_ORDER_SET_REQ
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_profile_order_set_req(
        FsmContext* pContext,
        const UnifiNmeProfileOrderSetReq_Evt* req)
{
    FsmData *pFsmData = FSMDATA;
    unifi_Status status = unifi_Success;

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_order_set_req(%d)", req->profileIdentitysCount));

    /* Need to free the existing list */
    if (pFsmData->nmeProfileOrderList.count)
    {
        CsrPfree(pFsmData->nmeProfileOrderList.pProfileIdentitys);
    }

    /* No need to copy the data as we own it already */
    pFsmData->nmeProfileOrderList.count = req->profileIdentitysCount;
    pFsmData->nmeProfileOrderList.pProfileIdentitys = req->profileIdentitys;

    call_unifi_nme_profile_order_set_cfm(pContext, req->appHandle, status);

    /* The start the auto connect timer if we are idle */
    if (req->profileIdentitysCount &&
        (fsm_current_state(pContext) == FSMSTATE_idle || fsm_current_state(pContext) == FSMSTATE_monitoring))
    {
        pFsmData->havePerformedScan = FALSE;
        nme_profile_start_autoconnect(pContext);
    }
    else if (!req->profileIdentitysCount && fsm_current_state(pContext) == FSMSTATE_monitoring)
    {
        fsm_next_state(pContext, FSMSTATE_idle);
    }
}

/**
 * @brief
 *   A core start request
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    NmeCoreStartReq_Evt* : start request event
 *
 * @return
 *   void
 */
static void nme_profile_start(FsmContext* pContext, const NmeCoreStartReq_Evt* req)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_start() autoConnectProfilesCount = %d", pFsmData->autoConnectProfilesCount));

    send_nme_core_start_cfm(pContext, req->common.sender_, unifi_Success);

    csr_list_clear(&pFsmData->scanList);

    fsm_next_state(pContext, FSMSTATE_idle);

    /* If we have any preferred profiles Try and connect immediately */
    if (pFsmData->nmeProfileOrderList.count)
    {
        pFsmData->havePerformedScan = FALSE;
        nme_profile_start_autoconnect(pContext);
    }
}

/**
 * @brief
 *   A core stop request
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    NmeCoreStartReq_Evt* : start request event
 *
 * @return
 *   void
 */
static void nme_profile_stop(FsmContext* pContext, const NmeCoreStopReq_Evt* req)
{
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_stop()"));

    send_nme_core_stop_cfm(pContext, req->common.sender_, unifi_Success);

    fsm_next_state(pContext, FSMSTATE_stopped);
}


/**
 * @brief
 *   Handles a MGT SCAN FULL CFM
 *
 * @par Description
 *   Need to request the results of the scan from the SME
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeMgtScanFullCfm_Evt* : MGT SCAN FULL CFM event
 *
 * @return
 *   void
 */
static void nme_profile_scan_cfm(FsmContext* pContext, const UnifiNmeMgtScanFullCfm_Evt* cfm)
{
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_scan_cfm(%s)", trace_unifi_Status(cfm->status)));

    nme_profile_start_autoconnect(pContext);
}

/**
 * @brief
 *   Handles a NME PROFILE CONNECT CFM
 *
 * @par Description
 *   Handles the profile connect cfm depending on the result of
 *   connection request.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeProfileConnectCfm_Evt* : NME PROFILE CONNECT CFM event
 *
 * @return
 *   void
 */
static void nme_profile_connect_cfm(FsmContext* pContext, const UnifiNmeProfileConnectCfm_Evt* cfm)
{
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_connect_cfm(%s)", trace_unifi_Status(cfm->status)));

    if (cfm->status == unifi_Success)
    {
        fsm_next_state(pContext, FSMSTATE_connected);
    }
    else if (cfm->status == unifi_Cancelled)
    {
        /* Should only happen when a higher layer application request
         * triggers the termination of the auto connect request.
         * By returning the to idle state profile manager will check that
         * the network selector is disconnected, before initiating any new
         * connection requests.
         */
        fsm_next_state(pContext, FSMSTATE_idle);
    }
    /* Otherwise remain in the same state and Wait for the disconnected ind
     * before attempting any further connections
     */

    free_unifi_nme_profile_connect_cfm_contents(cfm);
}

/**
 * @brief
 *   Handles a disconnect ind's while connecting
 *
 * @par Description
 *   Try the next profile in preferred list
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] NmeNsDisconnectInd_Evt* : event
 *
 * @return
 *   void
 */
static void nme_profile_connecting_disconnected(FsmContext* pContext, const NmeNsDisconnectInd_Evt* ind)
{
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_connecting_disconnected()"));
    nme_profile_attempt_next_autoconnect(pContext);
}

/**
 * @brief
 *   Handles a disconect ind's
 *
 * @par Description
 *   We were disconnected.
 *   Start the autoconnect again (If allowed)
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] NmeNsDisconnectInd_Evt* : event
 *
 * @return
 *   void
 */
static void nme_profile_disconnected(FsmContext* pContext, const NmeNsDisconnectInd_Evt* ind)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_disconnected()"));

    if (pFsmData->nmeProfileOrderList.count && nme_ntw_selector_is_disconnected(pContext))
    {
        pFsmData->havePerformedScan = FALSE;
        nme_profile_start_autoconnect(pContext);
    }
    else
    {
        fsm_next_state(pContext, FSMSTATE_idle);
    }
}

/**
 * @brief
 *   Use the scan ind to update the contents of the scan list
 *   Doesn't actually trigger any other auto-connect behaviour.
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] UnifiNmeMgtScanResultInd_Evt* : event
 *
 * @return
 *   void
 */
static void nme_profile_scan_result_ind_update(FsmContext* pContext, const UnifiNmeMgtScanResultInd_Evt* ind)
{
    FsmData *pFsmData = FSMDATA;
    nmeScanListNode_t* node;

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_scan_result_ind_store(%s, %s) [%2d]",
                     trace_unifi_MACAddress(ind->result.bssid, getNMEMacAddressBuffer(pContext)),
                     trace_unifi_SSID(&(ind->result.ssid), getNMESSIDBuffer(pContext)),
                     ind->result.channelNumber));

    node = nme_profile_scan_node_get(&pFsmData->scanList, &ind->result.bssid);

    /* Check for Node Delete */
    if (0 == ind->result.channelNumber)
    {
        if (node)
        {
            (void)csr_list_remove(&pFsmData->scanList, &node->node);
        }
    }
    else if (NULL == node)
    {
        /* Check for Node Create */
        node = (nmeScanListNode_t*)CsrPmalloc(sizeof(nmeScanListNode_t));
        node->id.bssid = ind->result.bssid;
        node->id.ssid = ind->result.ssid;
        node->failedConnectAttempts = 0;
        csr_list_insert_tail(&pFsmData->scanList, list_owns_value, &(node->node), node);
    }
    else if (NULL != node)
    {
    	/* Overwrite with the new SSID */
        node->id.ssid = ind->result.ssid;
        node->failedConnectAttempts = 0;
    }
    /* Freeing of message contents performed in functions that call this one, as
     * in some cases further processing of the indication contents is performed.
     */
}


/**
 * @brief
 *   Handles a scan ind's whilst already attempting to connect
 *   so doesn't trigger any auto connect behaviour.
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] UnifiNmeMgtScanResultInd_Evt* : event
 *
 * @return
 *   void
 */
static void nme_profile_scan_result_ind_store(FsmContext* pContext, const UnifiNmeMgtScanResultInd_Evt* ind)
{
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_scan_result_ind_store(%s, %s)",
                     trace_unifi_MACAddress(ind->result.bssid, getNMEMacAddressBuffer(pContext)),
                     trace_unifi_SSID(&(ind->result.ssid), getNMESSIDBuffer(pContext))));

    nme_profile_scan_result_ind_update(pContext, ind);
    free_unifi_mgt_scan_result_ind_contents((const UnifiMgtScanResultInd_Evt*)ind);

}

/**
 * @brief
 *   Handles a scan ind's when not already attempting to connect
 *   so may trigger auto-connect behaviour if a new potential
 *   network to attempt to connect has become available.
 *
 * @par Description
 *   Use the scan ind's to trigger a connect attempt
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] UnifiNmeMgtScanResultInd_Evt* : event
 *
 * @return
 *   void
 */
static void nme_profile_scan_result_ind(FsmContext* pContext, const UnifiNmeMgtScanResultInd_Evt* ind)
{
    FsmData *pFsmData = FSMDATA;
    CsrUint32 now = fsm_get_time_of_day_ms(pContext);

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_scan_result_ind(%s, %s)",
                     trace_unifi_MACAddress(ind->result.bssid, getNMEMacAddressBuffer(pContext)),
                     trace_unifi_SSID(&(ind->result.ssid), getNMESSIDBuffer(pContext))));

    /* If there is already a connection active then we'll not be able to
     * connect, so move to idle and we'll be kicked to look at scan again
     * when the connection is disconnected.
     */
    if (nme_ntw_selector_is_disconnected(pContext))
    {
        nme_profile_scan_result_ind_update(pContext, ind);

        /* Is there a new potential network to try and connect to? */
        if (ind->result.channelNumber != 0)
        {
            nme_profile_build_auto_connect_profiles(pContext);

            if (pFsmData->autoConnectProfilesCount)
            {
                nmeScanListNode_t* node;
                node = nme_profile_scan_node_get(&pFsmData->scanList, &ind->result.bssid);
                node->lastConnectAttempt = now;
                nme_profile_attempt_next_autoconnect(pContext);
            }
            else
            {
                fsm_next_state(pContext, FSMSTATE_monitoring);
            }
        }
    }
    else
    {
        fsm_next_state(pContext, FSMSTATE_idle);
    }

    free_unifi_mgt_scan_result_ind_contents((const UnifiMgtScanResultInd_Evt*)ind);
}

/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   FSM data function
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void nme_profile_manager_init_data(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_manager_init_data()"));
    csr_list_init(&pFsmData->profileList);
    csr_list_init(&pFsmData->scanList);
    pFsmData->nmeProfileOrderList.count = 0;

    pFsmData->autoConnectProfilesCount = 0;
    pFsmData->autoConnectProfiles = NULL;
 }

/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   Inital FSM Entry function
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void nme_profile_manager_init_fsm(FsmContext* pContext)
{
    sme_trace_entry((TR_NME_PMGR_FSM, "init_nme_profile_manager_fsm()"));
    fsm_create_params(pContext, FsmData);
    nme_profile_manager_init_data(pContext);
    fsm_next_state(pContext, FSMSTATE_stopped);
}


/**
 * @brief
 *   Updates the old session with the new session
 *
 * @par Description
 *   See brief.
 *
 * @param[in] CsrUint8 **: old Session,
 * @param[in] CsrUint32 *: old Length,
 * @param[in] const CsrUint8 *: new Session,
 * @param[in] const CsrUint32 : new session Length
 *
 * @return
 *   void
 */
static void nme_profile_update_session_data(
        CsrUint8 **ppSession,
        CsrUint32 *pSessionLength,
        const CsrUint8 *pNewSession,
        const CsrUint32 newsessionLength)
{
    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_update_session_data:session length[%d]", pSessionLength));
    if (pSessionLength)
    {
        sme_trace_debug((TR_NME_PMGR_FSM, "free old session buffer length[%d]", *pSessionLength));
        CsrPfree(*ppSession);
        *pSessionLength = 0;
    }
    if (0 < newsessionLength)
    {
        sme_trace_debug((TR_NME_PMGR_FSM, "new session"));
        *ppSession = CsrPmalloc(newsessionLength);
        CsrMemCpy(*ppSession, pNewSession, newsessionLength);
        *pSessionLength = newsessionLength;
    }
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

unifi_Profile *nme_profile_manager_get_profile(
        FsmContext* pContext,
        const unifi_ProfileIdentity *pProfileIdentity)
{
    nmeProfileListNode_t *pProfileNode = NULL;
    FsmData *pFsmData = (fsm_get_params_by_id(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance, FsmData));

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_manager_get_profile()"));

    pProfileNode = nme_profile_node_get(&pFsmData->profileList, pProfileIdentity);
    if (NULL != pProfileNode)
    {
        /* Return a pointer to a copy of the profile, this is done to ensure
         * that the user of the profile data has a stable copy and avoids
         * the problems of repeated accessing the data when it maybe changing
         * e.g by profile_set_requests.
         *
         * The alternative to returning a copy would be to return a pointer
         * but this would have to checked to ensure that it is still valid
         * before using it.
         *
         * Obviously if the profile is altered then some coordination will
         * be required.
         */
        unifi_Profile *pProfile;
        pProfile = CsrPmalloc(sizeof(*pProfile));
        CsrMemCpy(pProfile, &(pProfileNode->profile), sizeof(*pProfile));
        return(pProfile);
    }
    else
    {
        return(NULL);
    }
}

void nme_profile_manager_update_profile_pac(
        FsmContext* pContext,
        const unifi_ProfileIdentity *pProfileIdentity,
        const CsrUint8 *pPac,
        const CsrUint32 pacLength)
{
    nmeProfileListNode_t *pProfileNode = NULL;
    FsmData *pFsmData = (fsm_get_params_by_id(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance, FsmData));

    require(TR_NME_PMGR_FSM, (0 < pacLength && NULL != pPac) || (0 == pacLength && NULL == pPac));

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_manager_update_profile_pac()"));

    pProfileNode = nme_profile_node_get(&pFsmData->profileList, pProfileIdentity);
    if (NULL != pProfileNode)
    {
        if (unifi_CredentialType8021xFast == pProfileNode->profile.credentials.credentialType)
        {
            if (pProfileNode->profile.credentials.credential.fast.pacLength)
            {
                CsrPfree(pProfileNode->profile.credentials.credential.fast.pac);
                pProfileNode->profile.credentials.credential.fast.pacLength = 0;
            }
            if (0 < pacLength)
            {
                pProfileNode->profile.credentials.credential.fast.pac = CsrPmalloc(pacLength);
                CsrMemCpy(pProfileNode->profile.credentials.credential.fast.pac, pPac, pacLength);
                pProfileNode->profile.credentials.credential.fast.pacLength = pacLength;
            }
            nme_profile_manager_send_profile_update_ind(pContext, &(pProfileNode->profile));
        }
    }
    else
    {
        sme_trace_warn((TR_NME_PMGR_FSM, "no matching profile found!"));
    }
}

void nme_profile_manager_update_profile_session(
        FsmContext* pContext,
        const unifi_ProfileIdentity *pProfileIdentity,
        const CsrUint8 *pSession,
        const CsrUint32 sessionLength)
{
    nmeProfileListNode_t *pProfileNode = NULL;
    FsmData *pFsmData = (fsm_get_params_by_id(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance, FsmData));

    require(TR_NME_PMGR_FSM, (0 < sessionLength && NULL != pSession) || (0 == sessionLength && NULL == pSession));

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_manager_update_profile_session()"));

    pProfileNode = nme_profile_node_get(&pFsmData->profileList, pProfileIdentity);
    if (NULL != pProfileNode)
    {
        switch(pProfileNode->profile.credentials.credentialType)
        {
        case unifi_CredentialType8021xTls:
        {
            nme_profile_update_session_data(&pProfileNode->profile.credentials.credential.tls.session,
                                            &pProfileNode->profile.credentials.credential.tls.sessionLength,
                                            pSession, sessionLength);
            break;
        }
        case unifi_CredentialType8021xTtls:
        {
            nme_profile_update_session_data(&pProfileNode->profile.credentials.credential.ttls.session,
                                            &pProfileNode->profile.credentials.credential.ttls.sessionLength,
                                            pSession, sessionLength);
            break;
        }
        default:
        {
            sme_trace_warn((TR_NME_PMGR_FSM, "credential try not suitable for profile found!"));
            return;
        }
        }

        nme_profile_manager_send_profile_update_ind(pContext, &(pProfileNode->profile));
    }
    else
    {
        sme_trace_warn((TR_NME_PMGR_FSM, "no matching profile found!"));
    }
}


unifi_Profile *nme_profile_manager_get_null_profile(void)
{
    unifi_Profile *pProfile;
    pProfile = CsrZpmalloc(sizeof(*pProfile));
    return(pProfile);
}


CsrUint8 nme_profile_manager_get_cloaked_ssids(FsmContext* pContext, unifi_SSID** cloakedSsids)
{
    FsmData *pFsmData = (fsm_get_params_by_id(pContext, getNmeContext(pContext)->nmeProfileMgrFsmInstance, FsmData));
    NmeConfigData* cfg = nme_core_get_nme_config(pContext);
    nmeProfileListNode_t *node;
    CsrUint8 count = cfg->cloakedSsidsCount;

    sme_trace_entry((TR_NME_PMGR_FSM, "nme_profile_manager_get_cloaked_ssids()"));
    require(TR_NME_PMGR_FSM, NULL != cloakedSsids);

    if (count + csr_list_size(&pFsmData->profileList) == 0)
    {
        *cloakedSsids = NULL;
        return 0;
    }

    *cloakedSsids = (unifi_SSID*)CsrPmalloc(sizeof(unifi_SSID) * (count + csr_list_size(&pFsmData->profileList)));

    /* Copy the User values */
    CsrMemCpy(*cloakedSsids, cfg->cloakedSsids, sizeof(unifi_SSID) * count);

    /* Add the Cloaked Profiles */
    for (node = csr_list_gethead_t(nmeProfileListNode_t *, &pFsmData->profileList); NULL != node;
         node = csr_list_getnext_t(nmeProfileListNode_t *, &pFsmData->profileList, &(node->node)))
    {
        if (node->profile.cloakedSsid && node->profile.profileIdentity.ssid.length)
        {
            CsrMemCpy(&(*cloakedSsids)[count], &node->profile.profileIdentity.ssid, sizeof(unifi_SSID));
            count++;
        }
    }

    return count;
}

/* FSM DEFINITION **********************************************/

static const FsmEventEntry stoppedTransitions[] =
{
                         /* Signal Id,                            Function */
    fsm_event_table_entry(NME_CORE_START_REQ_ID,                  nme_profile_start),
    fsm_event_table_entry(NME_CORE_STOP_REQ_ID,                   fsm_ignore_event),

    /* Since we jump back to stopped from any state Ignore any stray messgaes */
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_FULL_CFM_ID,         fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULTS_GET_CFM_ID,  fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_CFM_ID,       fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,      fsm_ignore_event),
};

static const FsmEventEntry idleTransitions[] =
{
                         /* Signal Id,                            Function */
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,               nme_profile_disconnected),
};

static const FsmEventEntry scanningTransitions[] =
{
                         /* Signal Id,                            Function */
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_FULL_CFM_ID,         nme_profile_scan_cfm),
};

static const FsmEventEntry monitoringTransitions[] =
{
                         /* Signal Id,                            Function */
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULT_IND_ID,       nme_profile_scan_result_ind),
};


static const FsmEventEntry connectingTransitions[] =
{
                         /* Signal Id,                            Function */
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_CFM_ID,       nme_profile_connect_cfm),
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,               nme_profile_connecting_disconnected),
};

static const FsmEventEntry connectedTransitions[] =
{
                         /* Signal Id,                            Function */
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,               nme_profile_disconnected),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry defaultHandlers[] =
{
                         /* Signal Id,                            Function */
    fsm_event_table_entry(NME_CORE_STOP_REQ_ID,                   nme_profile_stop),
    fsm_event_table_entry(UNIFI_NME_PROFILE_SET_REQ_ID,           nme_profile_set_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_DELETE_REQ_ID,        nme_profile_delete_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_DELETE_ALL_REQ_ID,    nme_profile_delete_all_req),
    fsm_event_table_entry(UNIFI_NME_PROFILE_ORDER_SET_REQ_ID,     nme_profile_order_set_req),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULT_IND_ID,       nme_profile_scan_result_ind_store),
    fsm_event_table_entry(NME_NS_DISCONNECT_IND_ID,               fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULTS_GET_CFM_ID,  fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_CFM_ID,       fsm_ignore_event),
};

static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                           State            State                 Save     */
   /*                           Name             Transitions            *       */
    fsm_state_table_entry(FSMSTATE_stopped,     stoppedTransitions,    FALSE),
    fsm_state_table_entry(FSMSTATE_idle,        idleTransitions,       FALSE),
    fsm_state_table_entry(FSMSTATE_scanning,    scanningTransitions,   FALSE),
    fsm_state_table_entry(FSMSTATE_monitoring,  monitoringTransitions, FALSE),
    fsm_state_table_entry(FSMSTATE_connecting,  connectingTransitions, FALSE),
    fsm_state_table_entry(FSMSTATE_connected,   connectedTransitions,  FALSE),
};

const FsmProcessStateMachine nme_profile_manager_fsm =
{
#ifdef FSM_DEBUG
       "Nme Profile Manager",                   /* SM Process Name       */
#endif
       NME_PROFILE_MANAGER_PROCESS,             /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},        /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, defaultHandlers, FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE), /* Ignore Event handers */
       nme_profile_manager_init_fsm,            /* Entry Function        */
       nme_profile_manager_reset_fsm,           /* Reset Function        */
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
