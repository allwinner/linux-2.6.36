/** @file nme_connection_manager_fsm.c
 *
 * Network Management Entity Connection FSM Implementation
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
 *   NME Connection Manager FSM Implementation only responsible for handling
 *   profile connect requests.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_connection_manager_fsm/nme_connection_manager_fsm.c#14 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_connection_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_cstl/csr_wifi_list.h"
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
    /* Always assumes that the SME is disconnected */
    FSMSTATE_disconnected,
    /* Need to wait for any initial security related actions
     * prior to actually requesting the SME to connect.
     */
    FSMSTATE_waitingforSecPreconnect,
    /* Waiting for the SME to connect */
    FSMSTATE_connecting,
    /* Waiting for the security to complete any authentication required */
    FSMSTATE_authenticating,
    FSMSTATE_connected,
    FSMSTATE_disconnecting,
    /* Ensure that the security has terminated before moving on to doing anything
     * else.
     */
    FSMSTATE_waitingforSecTerminate,
    FSMSTATE_terminating,
    FSMSTATE_MAX_STATE
} FsmState;

/**
 * @brief
 *   Connect Attempt list node
 *
 * @par Description
 *   Struct defining the connect attempt list node based on the generic csr_list
 */
typedef struct nmeConnectAttemptListNode_t
{
    csr_list_node node;
    unifi_ConnectAttempt connectAttempt;
} nmeConnectAttemptListNode_t;

/**
 * @brief
 *   Connect Attempt list
 *
 * @par Description
 *   Struct defining the connect attempt list based on the generic csr_list
 */
typedef struct nmeConnectAttemptsList_t
{
    csr_list connectAttemptList;
    CsrUint8 connectAttemptListCount;
} nmeConnectAttemptsList_t;


/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData
{
    /* Need to track when the NME is attempting a connection to handle the
     * routing of the various indications and confirmations that are produced
     * during the connection attempt.
     */
    unifi_Profile *pConnectionProfile;
    CsrBool sentConnectCfm;
    void*   profileConnectReqAppHandle;
    unifi_ConnectionConfig connectionConfig;

    /* Has a disconnection request been sent, this allows us to recognise when
     * a disconnection indication from below and a disconnection request cross.
     */
    CsrBool requestedDisconnection;

    /* Need to track when another NME FSM requests disconnection as we
     * need to respond back to the correct FSM.
     */
    CsrBool otherFsmRequestedDisconnection;
    CsrUint16 fsmThatRequestedDisconnection;

    /* If the NME triggered the disconnection then we might not need to send
     * any disconnect cfm upward, unless we subsequently receive a disconnect
     * req. So we need to track whether this is the case or not.
     */
    CsrBool sendDisconnectCfmToClient;

    /* Security Manager identifier, set at creation, used for passing
     * signals and termination.
     */
    CsrUint16 securityManagerInstance;

    /* Need to track whether we've already requested security manager to start
     * or not.
     */
    CsrBool alreadyStartedSecurityManager;

    /* Need a list to track the APs that we attempt to connect to during a
     * connection, so that if we fail we can provide the application with
     * the details.
     */
    nmeConnectAttemptsList_t *pConnectAttemptsList;

    /* Need to track the current BSS id that we are attempting to connect
     * to.
     */
    unifi_MACAddress associatedBssId;

    /* If the AP deauthenticates the SME then it may well roam back to the
     * same AP and theNME is unaware of this. So we need to track how many
     * times we've tried the same AP.
     */
    CsrUint8 attemptedBssIdTimes;

    /* During a connection attempt if security is failed then the AP is blacklisted.
     * This will trigger the SME to attempt to roam, if it can find a suitable AP.
     * This allows the NME to try and connect to all suitable APs not just try the
     * first one that the SME finds and fail.
     * Have to keep a seperate list to the connection attempt list because the application
     * might request blacklist changes that also effect this list. As we don't want to
     * remove a blacklist that has also been set by the application.
     */
    unifi_AddressList blacklistedAddressList;

} FsmData;


/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Blacklists the supplied address if not already present
 *   in the blacklisted list.
 *
 * @par Description
 *   See brief
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const unifi_MACAddress* : address
 *
 * @return
 *    None
 */
static void nme_connection_mgr_blacklist_address(
        FsmContext* pContext,
        const unifi_MACAddress* pNewAddress)
{
    FsmData *pFsmData = FSMDATA;
    unifi_MACAddress* pNewList;
    CsrUint8 i;
    CsrBool found = FALSE;

    require(TR_NME_CMGR_FSM, NULL != pNewAddress);

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_blacklist_address - addressCount: %d", pFsmData->blacklistedAddressList.addressesCount));

    /* Even though there is a NULL pointer check in the above require
     * we still need to check for a NULL pointer as the require will not be there
     * in a release build. We also need to keep lint happy.
     */
    if (NULL == pNewAddress)
    {
        return;
    }

    /* Only add address if not already in the list */
    for (i = 0; i < pFsmData->blacklistedAddressList.addressesCount && found == FALSE; ++i)
    {
        if (CsrMemCmp((const void *)pNewAddress,
                      (const void *)&(pFsmData->blacklistedAddressList.addresses[i]),
                      sizeof(pFsmData->blacklistedAddressList.addresses[i].data)) == 0)
        {
            found = TRUE;
        }
    }
    if (!found)
    {
        pNewList = (unifi_MACAddress*)CsrPmalloc(sizeof(unifi_MACAddress) * (pFsmData->blacklistedAddressList.addressesCount + 1));
        if (pFsmData->blacklistedAddressList.addressesCount)
        {
            CsrMemCpy(pNewList, pFsmData->blacklistedAddressList.addresses, (sizeof(unifi_MACAddress) * (pFsmData->blacklistedAddressList.addressesCount + 1)));
        }
        CsrMemCpy(&pNewList[pFsmData->blacklistedAddressList.addressesCount], pNewAddress, sizeof(unifi_MACAddress));
        pFsmData->blacklistedAddressList.addressesCount++;
        CsrPfree(pFsmData->blacklistedAddressList.addresses);
        pFsmData->blacklistedAddressList.addresses = pNewList;

        /* Not really interested in the CFM but it needs to be absorbed by the NME rather
         * than passed out to the application
         */
        nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_MGT_BLACKLIST_CFM_ID, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        call_unifi_nme_mgt_blacklist_req(
                pContext,
                NME_APP_HANDLE(pContext),
                unifi_ListActionAdd,
                1,
                pNewAddress);
    }
}


/**
 * @brief
 *   Removes any matching addresses from the blacklist setup during
 *   connection attempts
 *
 * @par Description
 *   Just removes the address without sending any blacklist changes
 *   to the SME, as the application blacklist req will be passed down
 *   to the SME.
 *
 *   So for example if the NME has blacklist an address during the
 *   connection attempts and then the application sends a request to
 *   blacklist the same address. The address will be removed from the
 *   NME list. Thus when the NME removes it's blacklists the blacklist
 *   will remain in effect.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const unifi_AddressList* : addresses to remove
 *
 * @return
 *    None
 */
static void nme_connection_mgr_remove_addresses_from_blacklist(
        FsmContext* pContext,
        const unifi_AddressList* pRemoveAddresses)
{
    FsmData *pFsmData = FSMDATA;
    CsrUint8 i, j;

    require(TR_NME_CMGR_FSM, NULL != pRemoveAddresses);

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_remove_addresses_from_blacklist - addressCount: %d", pFsmData->blacklistedAddressList.addressesCount));

    /* Even though there is a NULL pointer check in the above require
     * we still need to check for a NULL pointer as the require will not be there
     * in a release build. We also need to keep lint happy.
     */
    if (NULL == pRemoveAddresses)
    {
        return;
    }

    for (j = 0; j < pRemoveAddresses->addressesCount; ++j)
    {
        for (i = 0; i < pFsmData->blacklistedAddressList.addressesCount; ++i)
        {
            if (CsrMemCmp(&pFsmData->blacklistedAddressList.addresses[i], &(pRemoveAddresses->addresses[j]), sizeof(pRemoveAddresses->addresses[j].data)) == 0)
            {
                /* Move the rest of the list Up 1  */
                CsrMemMove(&pFsmData->blacklistedAddressList.addresses[i],
                           &pFsmData->blacklistedAddressList.addresses[i+1],
                           ((pFsmData->blacklistedAddressList.addressesCount - i) - 1)*(sizeof(unifi_MACAddress)));
                pFsmData->blacklistedAddressList.addressesCount--;
                break;
            }
        }
    }
}


/**
 * @brief
 *   Removes any blacklist that was put in place during the connection
 *
 * @par Description
 *   See brief
 *
 * @param[in] FsmContext* : FSM context
 *
 * @return
 *    None
 */
static void nme_connection_mgr_remove_blacklist(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_remove_blacklist - addressCount: %d", pFsmData->blacklistedAddressList.addressesCount));

    if (pFsmData->blacklistedAddressList.addressesCount)
    {
        /* Not really interested in the CFM but it needs to be absorbed by the NME rather
         * than passed out to the application
         */
        nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_MGT_BLACKLIST_CFM_ID, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        call_unifi_nme_mgt_blacklist_req(
                pContext,
                NME_APP_HANDLE(pContext),
                unifi_ListActionRemove,
                pFsmData->blacklistedAddressList.addressesCount,
                pFsmData->blacklistedAddressList.addresses);

        CsrPfree(pFsmData->blacklistedAddressList.addresses);
        pFsmData->blacklistedAddressList.addressesCount = 0;
        pFsmData->blacklistedAddressList.addresses = NULL;
    }
    verify(TR_NME_CMGR_FSM,
           0 == pFsmData->blacklistedAddressList.addressesCount &&
           NULL ==  pFsmData->blacklistedAddressList.addresses);
}

/**
 * @brief
 *   Initialises a connection attempt list
 *
 * @par Description
 *   Initialises a connection attempt list
 *
 * @param[in] None
 *
 * @return
 *    nmeConnectAttemptsList_t* : initialised connection attempt list
 */
static nmeConnectAttemptsList_t* nme_connection_mgr_connect_attempt_list_init(void)
{
    nmeConnectAttemptsList_t *pList = (nmeConnectAttemptsList_t*)CsrPmalloc(sizeof(*pList));

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_connect_attempt_list_init(%p)", pList));

    pList->connectAttemptListCount=0;
    csr_list_init(&(pList->connectAttemptList));
    return pList;
}

/**
 * @brief
 *   Clears the supplied connection attempt list
 *
 * @par Description
 *   Clears the supplied connection attempt list
 *
 * @param[in] nmeConnectAttemptsList_t * : list to clear
 *
 * @return
 *    None
 */
static void nme_connection_mgr_connect_attempt_list_clear(nmeConnectAttemptsList_t *pList)
{
    require(TR_NME_CMGR_FSM, NULL != pList);

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_connect_attempt_list_clear(%p)", pList));

    while (!csr_list_isempty(&(pList->connectAttemptList)))
    {
        csr_list_removehead(&(pList->connectAttemptList));
    }
    pList->connectAttemptListCount=0;
}

/**
 * @brief Adds the supplied connection attempt details to the list
 *
 * @par Description:
 * See brief
 *
 * @param[in] nmeConnectAttemptsList_t * : List
 * @param[in] unifi_MACAddress : mac address
 * @param[in] unifi_Status : status
 * @param[in] unifi_SecurityError : security error
 *
 * @return
 *      None
 */
void nme_connection_mgr_connect_attempt_node_set(
        nmeConnectAttemptsList_t* pList,
        unifi_MACAddress bssId,
        unifi_Status status,
        unifi_SecError securityError)
{
    nmeConnectAttemptListNode_t *pNode = NULL;

    require(TR_NME_CMGR_FSM, NULL != pList);

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_connect_attempt_node_set"));

    pNode = CsrPmalloc(sizeof(*pNode));
    pNode->connectAttempt.bssid = bssId;
    pNode->connectAttempt.status = status;
    pNode->connectAttempt.securityError = securityError;

    pList->connectAttemptListCount++;
    csr_list_insert_tail(&(pList->connectAttemptList), list_owns_value, &(pNode->node), pNode);
}

/**
 * @brief
 *   Generates the connection attempts suitable for placing
 *   in a NME PROFILE CONNECT CFM or NME PROFILE DISCONNECT
 *   IND
 *
 * @par Description
 *   See brief
 *
 * @param[in] nmeConnectAttemptsList_t* : connect attempt list
 * @param[in] CsrUint8 * : count
 * @param[in] unifi_ConnectAttempt ** : point that will be updated to
 *                                     list of connection attempts
 *
 * @return
 *   void
 */
void nme_connection_mgr_get_connection_attempts(
        nmeConnectAttemptsList_t* pList,
        CsrUint8* pCount,
        unifi_ConnectAttempt** ppConnectAttempts)
{
    nmeConnectAttemptListNode_t *pNode;
    unifi_ConnectAttempt* pConnectAttempt = NULL;

    require(TR_NME_CMGR_FSM, NULL != pList);

    sme_trace_entry((TR_NME_CMGR_FSM, "connectAttemptList(%p) count %d", pList, csr_list_size(&(pList->connectAttemptList))));

    *pCount = pList->connectAttemptListCount;
    *ppConnectAttempts = NULL;
    if (pList->connectAttemptListCount)
    {
        *ppConnectAttempts = CsrPmalloc(pList->connectAttemptListCount * sizeof(unifi_ConnectAttempt));
        pConnectAttempt = *ppConnectAttempts;
        for (pNode = csr_list_gethead_t(nmeConnectAttemptListNode_t *, (csr_list *)&(pList->connectAttemptList)); NULL != pNode;
             pNode = csr_list_getnext_t(nmeConnectAttemptListNode_t *, (csr_list *)&(pList->connectAttemptList), &(pNode->node)))
        {
            CsrMemCpy(pConnectAttempt, &(pNode->connectAttempt), sizeof(*pConnectAttempt));
            pConnectAttempt++;
        }
    }
}

/**
 * @brief
 *   generates a NME PROFILE CONNECT CFM
 *
 * @par Description
 *   Will populate the cfm with the connect attempts stored
 *   in the list and clear the list.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    unifi_Status : status
 *
 * @return
 *   void
 */
static void nme_connection_mgr_send_profile_connect_cfm(
        FsmContext* pContext,
        unifi_Status status)
{
    FsmData *pFsmData = FSMDATA;

    require(TR_NME_CMGR_FSM, NULL != pFsmData->pConnectionProfile);
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_send_profile_connect_cfm alreadySent %d", pFsmData->sentConnectCfm));

    /* Only want to send the NME CONNECT PROFILE CFM once */
    if (!pFsmData->sentConnectCfm)
    {
        CsrUint8 count = 0;
        unifi_ConnectAttempt* pConnectAttempts = NULL;

        nme_connection_mgr_get_connection_attempts(pFsmData->pConnectAttemptsList, &count, &pConnectAttempts);
        send_unifi_nme_profile_connect_cfm(pContext,
                                           getNmeContext(pContext)->nmeNetworkSelectorFsmInstance,
                                           pFsmData->profileConnectReqAppHandle,
                                           status,
                                           count,
                                           pConnectAttempts);
        pFsmData->sentConnectCfm = TRUE;
    }
    nme_connection_mgr_connect_attempt_list_clear(pFsmData->pConnectAttemptsList);
    nme_connection_mgr_remove_blacklist(pContext);
}

/**
 * @brief
 *   generates a NME PROFILE DISCONNECT IND
 *
 * @par Description
 *   Will populate the ind with the connect attempts stored
 *   in the list and clear the list.
 *
 *   NOTE: The ind is only generated if there are connection attempts
 *   to report otherwise the fact that we are disconnected is indicated
 *   by the MEDIA STATUS IND
 *
 * @param[in]    FsmContext* : FSM context
 *
 * @return
 *   void
 */
static void nme_connection_mgr_send_profile_disconnect_ind(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_send_profile_disconnect_ind"));

    /* Are there any failed connection attempts to report as part of this disconnection? */
    if (pFsmData->sentConnectCfm &&
        pFsmData->pConnectAttemptsList->connectAttemptListCount)
    {
        void* appHandles;
        CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndProfileDisconnect, &appHandles);
        if (appHandleCount)
        {
            CsrUint8 count = 0;
            unifi_ConnectAttempt* pConnectAttempts = NULL;

            nme_connection_mgr_get_connection_attempts(pFsmData->pConnectAttemptsList, &count, &pConnectAttempts);
            call_unifi_nme_profile_disconnect_ind(pContext, appHandleCount, appHandles, count, pConnectAttempts);
            CsrPfree(pConnectAttempts);
        }
        nme_connection_mgr_connect_attempt_list_clear(pFsmData->pConnectAttemptsList);
    }
}


/**
 * @brief
 *   Initialises the FSM data for the disconnected state
 *
 * @par Description
 *   See brief
 *
 * @param[in]    FsmContext* : FSM context
 *
 * @return
 *   void
 */
static void nme_connection_mgr_init_data_disconnected(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_init_data_disconnected()"));

    /* Data is set appropriately for the disconnected state */
    CsrPfree(pFsmData->pConnectionProfile);
    pFsmData->pConnectionProfile = NULL;
    pFsmData->profileConnectReqAppHandle = NULL;
    pFsmData->sentConnectCfm = FALSE;
    pFsmData->requestedDisconnection = FALSE;
    pFsmData->sendDisconnectCfmToClient = FALSE;
    pFsmData->otherFsmRequestedDisconnection = FALSE;
    pFsmData->fsmThatRequestedDisconnection = 0;
    pFsmData->associatedBssId = NmeNullBssid;
    pFsmData->attemptedBssIdTimes = 0;
    nme_connection_mgr_connect_attempt_list_clear(pFsmData->pConnectAttemptsList);
    CsrPfree(pFsmData->blacklistedAddressList.addresses);
    pFsmData->blacklistedAddressList.addresses = NULL;
    pFsmData->blacklistedAddressList.addressesCount = 0;
    CsrMemSet(&pFsmData->connectionConfig, 0, sizeof(pFsmData->connectionConfig));
}


/**
 * @brief
 *   Reset the FSM
 *
 * @par Description
 *   Has to ensure that any resources used by the FSM
 *   are freed and the FSM is placed in the TERMINATE
 *   state so that it is cleared.
 *
 * @param[in]    FsmContext* : FSM context
 *
 * @return
 *   void
 */
static void nme_connection_mgr_reset(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_reset()"));
    nme_connection_mgr_init_data_disconnected(pContext);
    CsrPfree(pFsmData->pConnectAttemptsList);
    fsm_next_state(pContext, FSM_TERMINATE);
}


/**
 * @brief
 *   Sets the connection manager to the disconnected state
 *
 * @par Description
 *   Sets the connection manager to the disconnected state and initialises
 *   the FSM data. This function only performs actions that are common to
 *   all states moving into disconnected. Any specific state dependent behaviour
 *   should be put elsewhere.
 *
 * @param[in]    FsmContext* : FSM context
 *
 * @return
 *   void
 */
static void nme_connection_mgr_disconnected(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_disconnected()"));

    nme_connection_mgr_remove_blacklist(pContext);
    nme_connection_mgr_init_data_disconnected(pContext);

    if (unifi_ConnectionStatusDisconnected != nme_connection_manager_connection_status_get(pContext))
    {
        send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
    }
    fsm_next_state(pContext, FSMSTATE_disconnected);

    if (pFsmData->securityManagerInstance != FSM_TERMINATE)
    {
        /* clean up the security manager */
        send_nme_security_terminate_req(pContext, pFsmData->securityManagerInstance);
        fsm_next_state(pContext, FSMSTATE_waitingforSecTerminate);
    }
}


/**
 * @brief
 *   Handles a received UNIFI_NME_PROFILE_CONNECT_REQ_ID
 *
 * @par Description
 *   Handles a received UNIFI_NME_PROFILE_CONNECT_REQ_ID
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_profile_connect_req(FsmContext* pContext, const FsmEvent* pReq)
{
    FsmData *pFsmData = FSMDATA;
    unifi_Status status = unifi_NotFound;
    unifi_Profile *pProfile = NULL;
    const UnifiNmeProfileConnectReq_Evt *pProfileConnectReq = (UnifiNmeProfileConnectReq_Evt *)pReq;

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_profile_connect_req(id: %s, %s)",
            trace_unifi_SSID(&(pProfileConnectReq->profileIdentity.ssid), getNMESSIDBuffer(pContext)),
            trace_unifi_MACAddress(pProfileConnectReq->profileIdentity.bssid, getNMEMacAddressBuffer(pContext))));

    /* Ensure that the profile identity is sensible */
    if (pProfileConnectReq->profileIdentity.ssid.length > sizeof(pProfileConnectReq->profileIdentity.ssid.ssid))
    {
        sme_trace_warn((TR_NME_CMGR_FSM, "specified ssid  lenght %d > support %d",
                pProfileConnectReq->profileIdentity.ssid.length,
                sizeof(pProfileConnectReq->profileIdentity.ssid.ssid)));

        send_unifi_nme_profile_connect_cfm(pContext,
                                           getNmeContext(pContext)->nmeNetworkSelectorFsmInstance,
                                           pProfileConnectReq->appHandle,
                                           unifi_InvalidParameter,
                                           0, /* Not attempted to connect to any APs yet */
                                           NULL);
        send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        nme_connection_mgr_disconnected(pContext);
        return;
    }

    pProfile = nme_profile_manager_get_profile(pContext, &(pProfileConnectReq->profileIdentity));
    if (NULL != pProfile)
    {
        NmeConfigData* pNmeConfig = nme_core_get_nme_config(pContext);

        nme_trace_credentials((const unifi_Credentials *)&(pProfile->credentials));

        /* store the Profile Connect Req appHandle */
        pFsmData->profileConnectReqAppHandle = pProfileConnectReq->appHandle;

        /* Found the profile now need to try and connect to it by generating an
         * appropriate connect request to the SME based on information contained
         * in the profile.
         */
        CsrMemCpy(&(pFsmData->connectionConfig.ssid),
                  &(pProfile->profileIdentity.ssid),
                  sizeof(pProfile->profileIdentity.ssid));
        CsrMemCpy(&(pFsmData->connectionConfig.bssid),
                  &(pProfile->profileIdentity.bssid),
                  sizeof(pProfile->profileIdentity.bssid));

        /* Assume that the SME will use the appropriate one based on scan results */
        pFsmData->connectionConfig.ifIndex = unifi_GHZ_Both;

        pFsmData->connectionConfig.mlmeAssociateReqInformationElementsLength = 0;
        pFsmData->connectionConfig.mlmeAssociateReqInformationElements = NULL;
        pFsmData->connectionConfig.wmmQosInfo = pProfile->wmmQosCapabilitiesMask;

        /* Don't restrict to joining only for the moment, although we might want to add it as
         * an option on the profile in the future.
         */
        pFsmData->connectionConfig.adhocJoinOnly = FALSE;

        /* Default for the moment only set if the network type specified is adhoc */
        pFsmData->connectionConfig.adhocChannel = 0;
        pFsmData->connectionConfig.bssType = pProfile->bssType;

        if (unifi_Adhoc == pProfile->bssType)
        {
            pFsmData->connectionConfig.adhocChannel = pProfile->channelNo;
        }

        /* Retain the copy of the profile as we'll need to refer back to it. */
        if (NULL != pFsmData->pConnectionProfile)
        {
            CsrPfree(pFsmData->pConnectionProfile);
        }
        pFsmData->pConnectionProfile = pProfile;

        pFsmData->securityManagerInstance = fsm_add_instance(pContext, &nme_security_manager_fsm, TRUE);

        send_nme_security_preconnect_req(pContext,
                pFsmData->securityManagerInstance,
                pFsmData->connectionConfig.ssid,
                pNmeConfig->stationMacAddress,
                pFsmData->pConnectionProfile->credentials);

        fsm_next_state(pContext, FSMSTATE_waitingforSecPreconnect);
    }
    else
    {
        sme_trace_warn((TR_NME_CMGR_FSM, "nme_profile_connect_req no matching profile found"));
        send_unifi_nme_profile_connect_cfm(pContext,
                                           getNmeContext(pContext)->nmeNetworkSelectorFsmInstance,
                                           pProfileConnectReq->appHandle,
                                           status,
                                           0, /* Not attempted to connect to any APs yet */
                                           NULL);
        send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        nme_connection_mgr_disconnected(pContext);
    }
}


/**
 * @brief
 *   Handles a received preconnect cfm response from security
 *
 * @par Description
 *   Handles the preconnect cfm from the security FSM, which if successful
 *   means that the connection attempt may proceed. Otherwise the connection
 *   attempt is failed.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const NmeSecurityPreconnectCfm_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_security_preconnect_cfm(FsmContext* pContext, const NmeSecurityPreconnectCfm_Evt* pCfm)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_security_preconnect_cfm() SecurityResultCode:%d", pCfm->status ));

    if (nme_SecurityResultCodeSuccess == pCfm->status)
    {
        sme_trace_entry((TR_NME_CMGR_FSM, "nme_security_preconnect_cfm() privacyMode: %s", trace_unifi_80211PrivacyMode(pCfm->privacyMode) ));
        sme_trace_entry((TR_NME_CMGR_FSM, "nme_security_preconnect_cfm() authModeMask: 0x%x", pCfm->authModeMask ));
        sme_trace_entry((TR_NME_CMGR_FSM, "nme_security_preconnect_cfm() encryptionModeMask: 0x%x", pCfm->encryptionModeMask ));

        pFsmData->connectionConfig.privacyMode = pCfm->privacyMode;
        pFsmData->connectionConfig.authModeMask = pCfm->authModeMask;
        pFsmData->connectionConfig.encryptionModeMask = pCfm->encryptionModeMask;

        call_unifi_nme_mgt_connect_req(pContext,
                                       pFsmData->profileConnectReqAppHandle,
                                       (const unifi_ConnectionConfig *)&pFsmData->connectionConfig);

        fsm_next_state(pContext, FSMSTATE_connecting);
    }
    else
    {
        if (nme_SecurityResultCodeUnknownCredentialType == pCfm->status ||
            nme_SecurityResultCodeUnsupportedCredentialType == pCfm->status)
        {
            nme_connection_mgr_send_profile_connect_cfm(pContext, unifi_Unsupported);
        }
        else if (nme_SecurityResultCodeNoPacSupplied == pCfm->status)
        {
            nme_connection_mgr_send_profile_connect_cfm(pContext, unifi_SecurityError);
        }
        else
        {
            nme_connection_mgr_send_profile_connect_cfm(pContext, unifi_Error);
        }

        send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        nme_connection_mgr_disconnected(pContext);
    }
}


/**
 * @brief
 *   Handles a received UNIFI_NME_DISCONNECT_REQ_ID
 *
 * @par Description
 *   See Brief
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_disconnect_req(FsmContext* pContext, const FsmEvent* pReq)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_disconnect_req()"));

    /* In the authenticating state we've received the mgt connect cfm but haven't
     * yet completed the profile connect, so we need to inform the connection
     * requestor that it has been cancelled.
     */
    if (unifi_ConnectionStatusAuthenticating == nme_connection_manager_connection_status_get(pContext))
    {
        nme_connection_mgr_send_profile_connect_cfm(pContext, unifi_Cancelled);
    }
    nme_forward_to_unifi_mgt_sap(pContext, pReq);
    pFsmData->sendDisconnectCfmToClient = TRUE;
    pFsmData->requestedDisconnection = TRUE;

    fsm_next_state(pContext, FSMSTATE_disconnecting);
}
/**
 * @brief
 *   Handles a received UNIFI_NME_DISCONNECT_REQ_ID when disconnected
 *
 * @par Description
 *   See Brief
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeDisconnectReq_Evt* : event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_disconnected_disconnect_req(FsmContext* pContext, const UnifiNmeDisconnectReq_Evt* pReq)
{
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_disconnected_disconnect_req()"));

    call_unifi_nme_disconnect_cfm(pContext, pReq->appHandle, unifi_Success);
}

/**
 * @brief
 *   Handles a received UNIFI_NME_CONN_MGR_DISCONNECT_REQ_ID
 *
 * @par Description
 *   See Brief
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const NmeConnMgrDisconnectReq_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_int_disconnect_req(FsmContext* pContext, const NmeConnMgrDisconnectReq_Evt* pReq)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_int_disconnect_req()"));

    /* In the authenticating state we've received the mgt connect cfm but haven't
     * yet completed the profile connect, so we need to inform the connection
     * requestor that it has been cancelled. The only reason for this happening at
     * the moment is if the security fails, so return error.
     */
    if (unifi_ConnMgrDisconnectReasonSecurityAborted == pReq->reason &&
        unifi_ConnectionStatusAuthenticating == nme_connection_manager_connection_status_get(pContext))
    {
        nme_connection_mgr_connect_attempt_node_set(
                pFsmData->pConnectAttemptsList,
                pFsmData->associatedBssId,
                unifi_SecurityError,
                unifi_SecErrorUnknown);

        /* Blacklisting the currently associated AP should trigger the SME
         * to attempt to roaming to a alternative AP (if a suitable one is
         * available).
         */
        nme_connection_mgr_blacklist_address(pContext, (const unifi_MACAddress *)&(pFsmData->associatedBssId));
        fsm_next_state(pContext, FSMSTATE_connecting);
    }
    else
    {
        /* If we've not already indicated the result of the connection attempt
         * to the application, indicate that it has been cancelled.
         */
        nme_connection_mgr_send_profile_connect_cfm(pContext, unifi_Cancelled);
        call_unifi_nme_mgt_disconnect_req(pContext, NME_APP_HANDLE(pContext));
        pFsmData->sendDisconnectCfmToClient = FALSE;
        pFsmData->requestedDisconnection = TRUE;

        if (pReq->confirmationRequired)
        {
            pFsmData->otherFsmRequestedDisconnection = TRUE;
            pFsmData->fsmThatRequestedDisconnection = pReq->common.sender_;
        }

        fsm_next_state(pContext, FSMSTATE_disconnecting);
    }
}

/**
 * @brief
 *   Handles a received UNIFI_NME_CONN_MGR_DISCONNECT_REQ_ID
 *
 * @par Description
 *   In the disconnected state
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const NmeConnMgrDisconnectReq_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_disconnected_int_disconnect_req(FsmContext* pContext, const NmeConnMgrDisconnectReq_Evt* pReq)
{
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_disconnected_int_disconnect_req()"));
    if (pReq->confirmationRequired)
    {
        send_nme_conn_mgr_disconnect_cfm(pContext, pReq->common.sender_);
    }
}


/**
 * @brief
 *   Handles a received UNIFI_NME_MGT_CONNECT_CFM_ID
 *
 * @par Description
 *   See Brief
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_connect_cfm(FsmContext* pContext, const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    const UnifiNmeMgtConnectCfm_Evt* pConnectCfm = (UnifiNmeMgtConnectCfm_Evt*)pEvt;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_connect_cfm()"));

    if (unifi_Success == pConnectCfm->status)
    {
        if (unifi_Adhoc == pFsmData->pConnectionProfile->bssType &&
            unifi_ConnectionStatusConnecting == nme_connection_manager_connection_status_get(pContext))
        {
            /* For an Adhoc network we will not get the media status ind if we are hosting
             * the network until someone actually joins, so we can consider the connect
             * complete at this point.
             */
            send_nme_ns_connect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
            nme_connection_mgr_send_profile_connect_cfm(pContext, pConnectCfm->status);
            fsm_next_state(pContext, FSMSTATE_connected);
        }
    }
    else
    {
        /* We've failed to connect */
        if (pFsmData->requestedDisconnection)
        {
            /* The status code from the SME will be unifi_Error rather
             * than cancelled, so as we know that we've cancelled it
             * return that instead.
             */
            nme_connection_mgr_send_profile_connect_cfm(pContext, unifi_Cancelled);
        }
        else
        {
            nme_connection_mgr_send_profile_connect_cfm(pContext, pConnectCfm->status);
        }
        /* If we cancelled the connection then we need to wait for the disconnect confirmation
         * otherwise we can inform the network selector, which will then terminate this connection
         * instance.
         */
        if (!pFsmData->requestedDisconnection)
        {
            send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
            nme_connection_mgr_disconnected(pContext);
        }
    }
}


/**
 * @brief
 *   Handles a received NME_SECURITY_COMPLETE_IND
 *
 * @par Description
 *   If security has completed successfully then:
 *
 *   Requests a connectionInfo from the SME to generate a
 *   NME MEDIA STATUS IND before sending the NME PROFILE
 *   CONNECT CFM.
 *
 *   If security failed; blacklist the current AP which
 *   will trigger roaming.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const NmeSecurityCompleteInd_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_security_complete_ind(
        FsmContext* pContext,
        const NmeSecurityCompleteInd_Evt* pSecCompleteInd)
{
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_security_complete_ind(%d)", pSecCompleteInd->result));

    if (nme_SecurityResultCodeSuccess == pSecCompleteInd->result)
    {
        nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_MGT_GET_VALUE_CFM_ID, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        call_unifi_nme_mgt_get_value_req(pContext, NME_APP_HANDLE(pContext), unifi_ConnectionInfoValue);
        sme_trace_debug((TR_NME_CMGR_FSM, "Requesting connectionInfo"));
    }
    else
    {
        FsmData *pFsmData = FSMDATA;
        sme_trace_debug((TR_NME_CMGR_FSM, "blacklisted current ap - attempting to roam"));
        nme_connection_mgr_connect_attempt_node_set(
                pFsmData->pConnectAttemptsList,
                pFsmData->associatedBssId,
                unifi_SecurityError,
                unifi_SecErrorUnknown);

        /* If the connection has failed due to a MIC failure then the SME will blacklist
         * the AP, without the NME needing to. Disassociation will be triggered from the
         * AP.
         */
        if (nme_SecurityResultCodeSecondMicFailure != pSecCompleteInd->result)
        {
            /* Blacklisting the currently associated AP should trigger the SME
             * to attempt to roaming to a alternative AP (if a suitable one is
             * available).
             */
            nme_connection_mgr_blacklist_address(pContext, (const unifi_MACAddress *)&(pFsmData->associatedBssId));
        }
        fsm_next_state(pContext, FSMSTATE_connecting);
    }
}

/**
 * @brief
 *   Handles a received NME_SECURITY_ABORT_IND
 *
 * @par Description
 *   If security failed; blacklist the current AP which
 *   will trigger roaming.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const NmeSecurityAbortInd_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_security_abort_ind(
        FsmContext* pContext,
        const NmeSecurityAbortInd_Evt* pSecAbortInd)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_security_abort_ind(%d)", pSecAbortInd->result));

    /* If the connection has failed due to a MIC failure then the SME will blacklist
     * the AP, without the NME needing to. Disassociation will be triggered from the
     * AP.
     */
    if (nme_SecurityResultCodeSecondMicFailure != pSecAbortInd->result)
    {
        /* Blacklisting the currently associated AP should trigger the SME
         * to attempt to roaming to a alternative AP (if a suitable one is
         * available).
         */
        nme_connection_mgr_blacklist_address(pContext, (const unifi_MACAddress *)&(pFsmData->associatedBssId));
    }
    fsm_next_state(pContext, FSMSTATE_connecting);
}

/**
 * @brief
 *   Handles a received MGT GET VALUE CFM
 *
 * @par Description
 *   Generate the NME MEDIA STATUS IND and if we haven't sent
 *   it yet the NME PROFILE CONNECT CFM as well.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const UnifiNmeMgtGetValueCfm_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_get_value_cfm(
        FsmContext* pContext,
        const UnifiNmeMgtGetValueCfm_Evt* pGetValueCfm)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_get_value_cfm(id: %d, status: %d)", pGetValueCfm->appValue.id, pGetValueCfm->status));

    if (unifi_ConnectionInfoValue == pGetValueCfm->appValue.id)
    {
        if (unifi_Success == pGetValueCfm->status)
        {
            void* appHandles;
            NmeConfigData* pNmeConfig = nme_core_get_nme_config(pContext);
            CsrUint16 appHandleCount = nme_routing_get_ind_apphandles(pContext, unifi_IndMediaStatus, &appHandles);

            /* Record the fact that we are reporting that we've connected */
            pNmeConfig->reportedConnected = TRUE;

            if (appHandleCount)
            {
                call_unifi_nme_media_status_ind(
                        pContext, appHandleCount, appHandles,
                        unifi_MediaConnected,
                        &(pGetValueCfm->appValue.unifi_Value_union.connectionInfo),
                        unifi_IEEE80211ReasonSuccess,
                        unifi_IEEE80211ReasonSuccess);
            }

            if (unifi_ConnectionStatusAuthenticating == nme_connection_manager_connection_status_get(pContext))
            {
                send_nme_ns_connect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
                nme_connection_mgr_connect_attempt_node_set(
                        pFsmData->pConnectAttemptsList,
                        pFsmData->associatedBssId,
                        unifi_Success,
                        unifi_SecErrorUnknown);
                nme_connection_mgr_send_profile_connect_cfm(pContext, unifi_Success);
                fsm_next_state(pContext, FSMSTATE_connected);
            }
        }
        else
        {
            /* Assume that the reason for failure will also trigger the disconnection
             * of the connection. So do nothing.
             */
            sme_trace_warn((TR_NME_CMGR_FSM, "failed to read connectionInfo!"));
        }
    }
    else
    {
        /* This should never happen as the SME should return cfms in the same order as requests.
         * The NME assumes this order to make it's message routing work.
         */
        sme_trace_crit((TR_NME_CMGR_FSM, "Incorrect value id received instead of connectionInfo!"));
    }
    free_unifi_mgt_get_value_cfm_contents((const UnifiMgtGetValueCfm_Evt*)pGetValueCfm);
}


/**
 * @brief
 *   Will forward UNIFI_NME_MGT_MEDIA_STATUS_IND
 *
 * @par Description
 *   See brief
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_forward_media_status_ind(FsmContext* pContext, const FsmEvent* pEvt)
{
    sme_trace_debug_code(
    const UnifiNmeMgtMediaStatusInd_Evt* pMediaStatusInd = (UnifiNmeMgtMediaStatusInd_Evt*)pEvt;

    sme_trace_entry((TR_NME_CMGR_FSM,
                     "nme_connection_mgr_forward_media_status_ind(status : 0x%x)",
                     pMediaStatusInd->mediaStatus));
    ); /* sme_trace_debug_code */

    nme_forward_to_csr_wifi_nme_sap(pContext, pEvt);
}


/**
 * @brief
 *   Handles a received UNIFI_NME_MGT_MEDIA_STATUS_IND
 *
 * @par Description
 *   Handles a received UNIFI_NME_MGT_MEDIA_STATUS_IND in a number of states
 *   as the bulk of the processing is similar. State specific code is in the
 *   transition that invokes this function and always has to be called before
 *   this function as this one frees the message contents.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */

static void nme_connection_mgr_media_status_ind(FsmContext* pContext, const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    const UnifiNmeMgtMediaStatusInd_Evt* pMediaStatusInd = (UnifiNmeMgtMediaStatusInd_Evt*)pEvt;

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_media_status_ind(status : 0x%x)", pMediaStatusInd->mediaStatus));

    /* Should only ever be called if the status is connected */
    ensure(TR_NME_CMGR_FSM, unifi_MediaConnected == pMediaStatusInd->mediaStatus);

    if (unifi_MediaConnected == pMediaStatusInd->mediaStatus)
    {
        CsrUint8 *pIe = NULL;
        CsrBool haveBlacklistBssId = FALSE;

        if (0 < pMediaStatusInd->connectionInfo.assocReqInfoElementsLength)
        {
            pIe = CsrPmalloc(pMediaStatusInd->connectionInfo.assocReqInfoElementsLength);
            CsrMemCpy(pIe, pMediaStatusInd->connectionInfo.assocReqInfoElements, pMediaStatusInd->connectionInfo.assocReqInfoElementsLength);
        }

        if(CsrMemCmp(pMediaStatusInd->connectionInfo.bssid.data, pFsmData->associatedBssId.data, sizeof(pFsmData->associatedBssId.data)))
        {
            pFsmData->associatedBssId = pMediaStatusInd->connectionInfo.bssid;
            pFsmData->attemptedBssIdTimes = 1;
        }
        else
        {
            /* Make sure that we don't loop just trying the same BSS id */
            if (4 == pFsmData->attemptedBssIdTimes)
            {
                /* Blacklisting the currently associated AP should trigger the SME
                 * to attempt to roaming to an alternative AP (if a suitable one is
                 * available).
                 */
                nme_connection_mgr_blacklist_address(pContext, (const unifi_MACAddress *)&(pFsmData->associatedBssId));
                haveBlacklistBssId = TRUE;
                send_nme_security_stop_req(pContext, pFsmData->securityManagerInstance);
                fsm_next_state(pContext, FSMSTATE_connecting);
            }
            pFsmData->attemptedBssIdTimes++;
        }

        /* if we've blacklisted the current BSS id then we need to wait for the
         * SME to either find another suitable BSS ID or for it to report that
         * we're disconnected.
         */
        if (!haveBlacklistBssId)
        {
            if (!pFsmData->alreadyStartedSecurityManager)
            {
                /* Always send the start request to the security FSM regardless of the
                 * credential type of the profile. Let the security manager FSM decide
                 * what needs to be done.
                 */
                send_nme_security_start_req(pContext,
                        pFsmData->securityManagerInstance,
                        pMediaStatusInd->connectionInfo.assocReqInfoElementsLength,
                        pIe,
                        pMediaStatusInd->connectionInfo.ssid,
                        pMediaStatusInd->connectionInfo.bssid);
                pFsmData->alreadyStartedSecurityManager = TRUE;
            }
            else
            {
                send_nme_security_restart_req(
                        pContext,
                        pFsmData->securityManagerInstance,
                        pMediaStatusInd->connectionInfo.assocReqInfoElementsLength,
                        pIe,
                        pMediaStatusInd->connectionInfo.ssid,
                        pMediaStatusInd->connectionInfo.bssid);
            }
            fsm_next_state(pContext, FSMSTATE_authenticating);
        }
        else if (NULL != pIe)
        {
            /* Didn't pass the IE across to security so need to free it */
            CsrPfree(pIe);
        }
    }
    /* Don't forward the event until after security is successful, as the event
     * indicates to the application that data may flow. But that isn't the case
     * as any security actions have not yet been performed.
     * If security is successful then we'll request the connectionInfo and
     * send up a new NME MEDIA STATUS IND.
     */
    free_unifi_mgt_media_status_ind_contents((const UnifiMgtMediaStatusInd_Evt*)pMediaStatusInd);
}


/**
 * @brief
 *   Handles a received UNIFI_NME_MGT_MEDIA_STATUS_IND
 *
 * @par Description
 *   Handles a received UNIFI_NME_MGT_MEDIA_STATUS_IND in the connected
 *   state as this is the indication that we get that the connection
 *   is no longer connected.
 *
 *   Or we could have roamed to a new BSS ID in the same Network.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_media_status_ind_whilst_connected(FsmContext* pContext, const FsmEvent* pEvt)
{
    const UnifiNmeMgtMediaStatusInd_Evt* pMediaStatusInd = (UnifiNmeMgtMediaStatusInd_Evt*)pEvt;

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_media_status_ind_whilst_connected(status : 0x%x)", pMediaStatusInd->mediaStatus));

    if (unifi_MediaDisconnected == pMediaStatusInd->mediaStatus)
    {
        FsmData *pFsmData = FSMDATA;
        /* Always forward disconnections */
        nme_connection_mgr_forward_media_status_ind(pContext, pEvt);
        /* If Adhoc then remains connected regardless of the MEDIA STATUS IND */
        if (unifi_Infrastructure == pFsmData->pConnectionProfile->bssType)
        {
            nme_connection_mgr_send_profile_disconnect_ind(pContext);
            send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
            nme_connection_mgr_disconnected(pContext);
        }
    }
    else
    {
        nme_connection_mgr_media_status_ind(pContext, pEvt);
    }
}

/**
 * @brief
 *   Handles a received UNIFI_NME_MGT_MEDIA_STATUS_IND
 *
 * @par Description
 *   Handles a received UNIFI_NME_MGT_MEDIA_STATUS_IND in the authenticating
 *   or connecting state
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_media_status_ind_whilst_connecting(FsmContext* pContext, const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    const UnifiNmeMgtMediaStatusInd_Evt* pMediaStatusInd = (UnifiNmeMgtMediaStatusInd_Evt*)pEvt;

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_media_status_ind_whilst_connecting(status : 0x%x)", pMediaStatusInd->mediaStatus));

    if (unifi_MediaDisconnected == pMediaStatusInd->mediaStatus)
    {
        if (pFsmData->sentConnectCfm)
        {
            nme_connection_mgr_send_profile_disconnect_ind(pContext);
            nme_connection_mgr_forward_media_status_ind(pContext, pEvt);
        }
        else
        {
            nme_connection_mgr_send_profile_connect_cfm(pContext, unifi_SecurityError);
            /* Don't send indication that we've disconnected if we've not yet
             * informed the user that we were connected.
             */
            free_unifi_mgt_media_status_ind_contents((const UnifiMgtMediaStatusInd_Evt*)pMediaStatusInd);
        }
        send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        nme_connection_mgr_disconnected(pContext);
    }
    else
    {
        nme_connection_mgr_media_status_ind(pContext, pEvt);
    }
}


/**
 * @brief
 *   Handles a received UNIFI_NME_MGT_DISCONNECT_CFM_ID
 *
 * @par Description
 *   See Brief
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_disconnect_cfm(FsmContext* pContext, const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_disconnect_cfm()"));
    if (pFsmData->sendDisconnectCfmToClient)
    {
        nme_forward_to_csr_wifi_nme_sap(pContext, pEvt);
    }
    if (pFsmData->otherFsmRequestedDisconnection)
    {
        send_nme_conn_mgr_disconnect_cfm(pContext, pFsmData->fsmThatRequestedDisconnection);
    }
    nme_connection_mgr_disconnected(pContext);
}


/**
 * @brief
 *   Updates the connection manager blacklist used for
 *   blacklisting APs that have failed security whilst
 *   trying to connect.
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
static void nme_connection_mgr_blacklist_req(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    const UnifiNmeBlacklistReq_Evt* pBlacklistReq = (UnifiNmeBlacklistReq_Evt*)pEvt;

    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_blacklist_req()"));

    if (unifi_ListActionGet != pBlacklistReq->action &&
        pFsmData->blacklistedAddressList.addressesCount)
    {
        if (unifi_ListActionFlush == pBlacklistReq->action)
        {
            /* All blacklists will be removed in the SME so empty our own list
             * the only downside to this is that the SME may try APs.
             */
            CsrPfree(pFsmData->blacklistedAddressList.addresses);
            pFsmData->blacklistedAddressList.addressesCount = 0;
            pFsmData->blacklistedAddressList.addresses = NULL;
        }
        else
        {
            unifi_AddressList addressList;
            addressList.addresses = pBlacklistReq->setAddresses;
            addressList.addressesCount = pBlacklistReq->setAddressCount;
            /* Regardless of whether the blacklist req is added or removing the
             * address we always remove them from the local list. This ensures
             * that we don't end up removing an application set blacklist when
             * the connection attempt is finished.
             */
            nme_connection_mgr_remove_addresses_from_blacklist(pContext, &addressList);
        }
    }
    free_unifi_mgt_blacklist_req_contents((UnifiMgtBlacklistReq_Evt*)pBlacklistReq);
}


/**
 * @brief
 *   Handles a received NME_SECURITY_TERMINATE_CFM_ID
 *
 * @par Description
 *   See Brief
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_security_terminate_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_security_terminate_cfm()"));

    send_nme_ns_disconnect_ind(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);

    /* the dynamic Security FSM has terminated */
    pFsmData->securityManagerInstance = FSM_TERMINATE;
    fsm_next_state(pContext, FSMSTATE_disconnected);
}


/**
 * @brief
 *   Handles a received NME CONNECTION MGR TERMINATE REQ
 *
 * @par Description
 *   Need to terminate the security manager and then this FSM.
 *   No attempt is made to disconnect any on going connection
 *   the assumption is that if the connection should have
 *   been disconnected then that would have been requested
 *   before terminating the FSM.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_terminate_req(FsmContext* pContext, const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_terminate_req()"));

    /* If we are authenticating then there will not be any more signalling
     * from the SME to trigger sending of the connect cfm back to the requestor.
     * So we need to generate a suitable response from here.
     */
    if (unifi_ConnectionStatusAuthenticating == nme_connection_manager_connection_status_get(pContext))
    {
        /* Assume that the reason for termination is a user request that
         * has resulted in the current connection attempt been stopped
         * eg. either a new connection attempt or wifi off.
         */
        nme_connection_mgr_send_profile_connect_cfm(pContext, unifi_Cancelled);
    }
    nme_connection_mgr_init_data_disconnected(pContext);

    if (pFsmData->securityManagerInstance != FSM_TERMINATE)
    {
        /* clean up the security manager */
        send_nme_security_terminate_req(pContext, pFsmData->securityManagerInstance);
        /* Need to wait for security to terminate before signalling completion of the termination */
        sme_trace_debug((TR_NME_CMGR_FSM, "waiting for security to terminate"));
        fsm_next_state(pContext, FSMSTATE_terminating);
    }
    else
    {
        /* Nothing else to wait for so signal that the termination is complete */
        nme_connection_mgr_disconnected(pContext);
        CsrPfree(pFsmData->pConnectAttemptsList);
        send_nme_conn_mgr_terminate_cfm(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        sme_trace_debug((TR_NME_CMGR_FSM, "FSM_TERMINATE"));
        fsm_next_state(pContext, FSM_TERMINATE);
    }
}


/**
 * @brief
 *   Handles a received SECURITY_TERMINATE_CFM_ID
 *
 * @par Description
 *   Terminates the connection manager FSM as the security
 *   FSM has terminated.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_terminating_security_terminate_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_terminating_security_terminate_cfm()"));

    /* the dynamic Security FSM has terminated */
    pFsmData->securityManagerInstance = FSM_TERMINATE;
    send_nme_conn_mgr_terminate_cfm(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
    nme_connection_mgr_disconnected(pContext);
    CsrPfree(pFsmData->pConnectAttemptsList);
    sme_trace_debug((TR_NME_CMGR_FSM, "FSM_TERMINATE"));
    fsm_next_state(pContext, FSM_TERMINATE);
}


/**
 * @brief
 *   Forwards an event to the security FSM
 *
 * @par Description
 *   Must only be called when the security manager FSM is active
 *   not when we are terminating it.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const FsmEvent* : event
 *
 * @return
 *   void
 */
static void nme_connection_mgr_forward_to_security(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    CsrUint16 securityInstance = nme_connection_manager_get_security_fsm_instance(pContext);
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_forward_to_security()"));
    if (FSM_TERMINATE != securityInstance)
    {
        fsm_forward_event(pContext, securityInstance, pEvt);
    }
    else
    {
        sme_trace_debug((TR_NME_CMGR_FSM, "discarding event id: 0x%x", pEvt->id));
        nme_free_event(pEvt);
    }
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
static void nme_connection_mgr_data_init(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_mgr_data_init()"));

    pFsmData->pConnectionProfile = NULL;
    pFsmData->securityManagerInstance = FSM_TERMINATE;
    pFsmData->alreadyStartedSecurityManager = FALSE;
    pFsmData->pConnectAttemptsList = nme_connection_mgr_connect_attempt_list_init();
    pFsmData->blacklistedAddressList.addresses = NULL;

    /* Data is set appropriately for the disconnected state */
    nme_connection_mgr_init_data_disconnected(pContext);
}

/**
 * @brief
 *   Inital FSM Entry function
 *
 * @par Description
 *   See Brief
 *
 * @param[in] context   : FSM context
 *
 * @return
 *   void
 */
static void nme_connection_manager_init_fsm(FsmContext* pContext)
{
    sme_trace_entry((TR_NME_CMGR_FSM, "nme_connection_manager_init_fsm()"));
    fsm_create_params(pContext, FsmData);
    nme_connection_mgr_data_init(pContext);
    fsm_next_state(pContext, FSMSTATE_disconnected);
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in nme_connection_manager_fsm/nme_connection_manager_fsm.h
 */
unifi_ConnectionStatus nme_connection_manager_connection_status_get(FsmContext* pContext)
{
    unifi_ConnectionStatus connectionStatus = unifi_ConnectionStatusDisconnected;
    CsrUint16 connectionMgrInstance = nme_ntw_selector_get_connection_mgr_fsm_instance(pContext);

    if (FSM_TERMINATE != connectionMgrInstance)
    {
        switch (fsm_get_context_by_id(pContext, connectionMgrInstance)->state)
        {
        case FSMSTATE_waitingforSecPreconnect:
        case FSMSTATE_connecting:
            connectionStatus = unifi_ConnectionStatusConnecting;
            break;
        case FSMSTATE_authenticating:
            connectionStatus = unifi_ConnectionStatusAuthenticating;
            break;
        case FSMSTATE_connected:
            connectionStatus = unifi_ConnectionStatusConnected;
            break;
        case FSMSTATE_disconnecting:
            connectionStatus = unifi_ConnectionStatusDisconnecting;
            break;
        default:
            /* Anything else is disconnected */
            break;
        }
    }

    sme_trace_debug((TR_NME_CMGR_FSM,
            "nme_connection_manager_connection_status_get() = %s", trace_unifi_ConnectionStatus(connectionStatus)));

    return(connectionStatus);
}


/**
 * See description in connection_manager_fsm/connection_manager_fsm.h
 */
CsrUint16 nme_connection_manager_get_security_fsm_instance(FsmContext* pContext)
{
    CsrUint16 secMgrInstance = FSM_TERMINATE;
    CsrUint16 connectionMgrInstance = nme_ntw_selector_get_connection_mgr_fsm_instance(pContext);

    if (FSM_TERMINATE != connectionMgrInstance)
    {
        secMgrInstance = fsm_get_params_by_id(pContext, connectionMgrInstance, FsmData)->securityManagerInstance;
    }
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_connection_manager_get_security_fsm_instance Sec Instance %d", secMgrInstance));
    return (secMgrInstance);
}


/**
 * See description in connection_manager_fsm/connection_manager_fsm.h
 */
unifi_ProfileIdentity nme_connection_manager_get_profile_identity(FsmContext* pContext)
{
    unifi_ProfileIdentity profileIdentity;
    CsrUint16 connectionMgrInstance = nme_ntw_selector_get_connection_mgr_fsm_instance(pContext);
    if (FSM_TERMINATE != connectionMgrInstance)
    {
        FsmData *pFsmData = fsm_get_params_by_id(pContext, connectionMgrInstance, FsmData);

        if (NULL != pFsmData->pConnectionProfile)
        {
            return(pFsmData->pConnectionProfile->profileIdentity);
        }
    }
    CsrMemSet(&profileIdentity, 0, sizeof(unifi_ProfileIdentity));
    return(profileIdentity);
}


/* FSM DEFINITION **********************************************/

static const FsmEventEntry disconnectedTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(UNIFI_NME_PROFILE_CONNECT_REQ_ID,              nme_connection_mgr_profile_connect_req),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   nme_connection_mgr_disconnected_disconnect_req),
    fsm_event_table_entry(NME_CONN_MGR_DISCONNECT_REQ_ID,                nme_connection_mgr_disconnected_int_disconnect_req),
};

/* State is only used during a profile connection, assume that something else
 * is handling the security aspects if NME CONNECT REQ is used to start the
 * connection.
 */
static const FsmEventEntry waitingforSecPreconnectTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_SECURITY_PRECONNECT_CFM_ID,                nme_security_preconnect_cfm),

    /* Having started the security initialisation it's more robust to wait for
     * that to complete before handling any other events. Even if they terminate
     * the connection that is setting up.
     */
    fsm_event_table_entry(NME_CONN_MGR_TERMINATE_REQ_ID,                 fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   fsm_saved_event),
    fsm_event_table_entry(NME_CONN_MGR_DISCONNECT_REQ_ID,                fsm_saved_event),
};

static const FsmEventEntry connectingTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(UNIFI_NME_MGT_CONNECT_CFM_ID,                  nme_connection_mgr_connect_cfm),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   nme_connection_mgr_disconnect_req),
    fsm_event_table_entry(NME_CONN_MGR_DISCONNECT_REQ_ID,                nme_connection_mgr_int_disconnect_req),
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_connection_mgr_media_status_ind_whilst_connecting),
    fsm_event_table_entry(UNIFI_NME_BLACKLIST_REQ_ID,                    nme_connection_mgr_blacklist_req),
};


static const FsmEventEntry authenticatingTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(UNIFI_NME_MGT_CONNECT_CFM_ID,                  nme_connection_mgr_connect_cfm),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   nme_connection_mgr_disconnect_req),
    fsm_event_table_entry(NME_CONN_MGR_DISCONNECT_REQ_ID,                nme_connection_mgr_int_disconnect_req),

    /* This is the only indication that an established connection has disconnected
     * or it could indicate that we've roamed. If we've roamed to a new BSS Id then
     * we need to restart security with the new parameters.
     */
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_connection_mgr_media_status_ind_whilst_connecting),
    fsm_event_table_entry(UNIFI_NME_BLACKLIST_REQ_ID,                    nme_connection_mgr_blacklist_req),

    fsm_event_table_entry(UNIFI_NME_CERTIFICATE_VALIDATE_RSP_ID,         nme_connection_mgr_forward_to_security),
    fsm_event_table_entry(NME_SECURITY_COMPLETE_IND_ID,                  nme_connection_mgr_security_complete_ind),

    fsm_event_table_entry(UNIFI_NME_MGT_GET_VALUE_CFM_ID,                nme_connection_mgr_get_value_cfm),
};

static const FsmEventEntry connectedTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(UNIFI_NME_MGT_CONNECT_CFM_ID,                  nme_connection_mgr_connect_cfm),
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   nme_connection_mgr_disconnect_req),
    fsm_event_table_entry(NME_CONN_MGR_DISCONNECT_REQ_ID,                nme_connection_mgr_int_disconnect_req),
    fsm_event_table_entry(UNIFI_NME_BLACKLIST_REQ_ID,                    nme_connection_mgr_blacklist_req),
    fsm_event_table_entry(UNIFI_NME_CERTIFICATE_VALIDATE_RSP_ID,         nme_connection_mgr_forward_to_security),

    fsm_event_table_entry(NME_SECURITY_ABORT_IND_ID,                     nme_connection_mgr_security_abort_ind),

    /* This is the only indication that an established connection has disconnected
     * or it could indicate that we've roamed. If we've roamed to a new BSS Id then
     * we need to restart security with the new parameters.
     */
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_connection_mgr_media_status_ind_whilst_connected),
    fsm_event_table_entry(UNIFI_NME_MGT_GET_VALUE_CFM_ID,                nme_connection_mgr_get_value_cfm),
};

static const FsmEventEntry disconnectingTransitions[] =
{
                       /* Signal Id,                            Function */
    fsm_event_table_entry(UNIFI_NME_DISCONNECT_REQ_ID,                   nme_connection_mgr_disconnect_req),
    fsm_event_table_entry(UNIFI_NME_MGT_CONNECT_CFM_ID,                  nme_connection_mgr_connect_cfm),
    fsm_event_table_entry(UNIFI_NME_MGT_DISCONNECT_CFM_ID,               nme_connection_mgr_disconnect_cfm),
    fsm_event_table_entry(UNIFI_NME_BLACKLIST_REQ_ID,                    nme_connection_mgr_blacklist_req),

    /* If we handle more than one of these at a time we'll overwrite the record
     * of which FSM the confirm has to be sent to, so just save it until we are
     * in the disconnected state.
     */
    fsm_event_table_entry(NME_CONN_MGR_DISCONNECT_REQ_ID,                fsm_saved_event),

    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_connection_mgr_forward_media_status_ind),

    /* Security may still be active as we don't terminate the FSM (if present) until after
     * we've disconnected.
     */
    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_CFM_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_CERTIFICATE_VALIDATE_RSP_ID,         fsm_ignore_event),
    fsm_event_table_entry(NME_SECURITY_COMPLETE_IND_ID,                  fsm_ignore_event),
};


static const FsmEventEntry waitingforSecTerminateTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_SECURITY_TERMINATE_CFM_ID,                 nme_connection_mgr_security_terminate_cfm),
    fsm_event_table_entry(NME_CONN_MGR_TERMINATE_REQ_ID,                 fsm_saved_event ),

    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_CFM_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_CERTIFICATE_VALIDATE_RSP_ID,         fsm_ignore_event),
    fsm_event_table_entry(NME_SECURITY_COMPLETE_IND_ID,                  fsm_ignore_event),
};


static const FsmEventEntry terminatingTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_CONN_MGR_TERMINATE_REQ_ID,                 fsm_ignore_event ),
    fsm_event_table_entry(NME_SECURITY_TERMINATE_CFM_ID,                 nme_connection_mgr_terminating_security_terminate_cfm),
    fsm_event_table_entry(NME_CONN_MGR_DISCONNECT_REQ_ID,                nme_connection_mgr_disconnected_int_disconnect_req ),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry defaultHandlers[] =
{
                           /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_CONN_MGR_TERMINATE_REQ_ID,                 nme_connection_mgr_terminate_req ),

    fsm_event_table_entry(NME_SECURITY_START_CFM_ID,                     fsm_ignore_event),
    fsm_event_table_entry(NME_SECURITY_RESTART_CFM_ID,                   fsm_ignore_event),
    fsm_event_table_entry(NME_SECURITY_KEYS_INSTALLED_IND_ID,            fsm_ignore_event),
    fsm_event_table_entry(NME_SECURITY_STOP_CFM_ID,                      fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_BLACKLIST_REQ_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_MGT_GET_VALUE_CFM_ID,                fsm_ignore_event),

    /* These events need to be forwarded to security */
    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,              nme_connection_mgr_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,    nme_connection_mgr_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID,  nme_connection_mgr_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                    nme_connection_mgr_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,              nme_connection_mgr_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_CFM_ID,              nme_connection_mgr_forward_to_security),
};


/** NME Core state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                           State                         State                                  Save     */
   /*                           Name                          Transitions                            *      */
    fsm_state_table_entry(FSMSTATE_disconnected,              disconnectedTransitions,               FALSE),
    fsm_state_table_entry(FSMSTATE_waitingforSecPreconnect,   waitingforSecPreconnectTransitions,    TRUE),
    fsm_state_table_entry(FSMSTATE_connecting,                connectingTransitions,                 FALSE),
    fsm_state_table_entry(FSMSTATE_authenticating,            authenticatingTransitions,             FALSE),
    fsm_state_table_entry(FSMSTATE_connected,                 connectedTransitions,                  FALSE),
    fsm_state_table_entry(FSMSTATE_disconnecting,             disconnectingTransitions,              FALSE),
    fsm_state_table_entry(FSMSTATE_waitingforSecTerminate,    waitingforSecTerminateTransitions,     TRUE),
    fsm_state_table_entry(FSMSTATE_terminating,               terminatingTransitions,                FALSE)

};

const FsmProcessStateMachine nme_connection_manager_fsm =
{
#ifdef FSM_DEBUG
       "Nme Connection Mgr",                    /* SM Process Name       */
#endif
       NME_CONNECTION_MANAGER_PROCESS,          /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},        /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, defaultHandlers, FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),  /* Ignore Event handers */
       nme_connection_manager_init_fsm,         /* Entry Function        */
       nme_connection_mgr_reset,                /* Reset Function        */
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
