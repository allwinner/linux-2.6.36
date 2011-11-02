/** @file nme_wps_fsm.c
 *
 * Network Management Entity Core FSM Implementation
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009-2010. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   NME Wi-FI Protected Setup FSM Implementation
 *
 *   This FSM handles finding the suitable WPS candidates and then
 *   attempting to connect and then create a profile by attempting the
 *   required security actions.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_wps_fsm/nme_wps_fsm.c#5 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_wps
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "nme_top_level_fsm/nme_top_level_fsm.h"
#include "csr_security.h"
#include "ie_utils/ie_utils.h"
#include "lib_info_element.h"
#include "ie_access/ie_access_rsn.h"

/* MACROS *******************************************************************/
#define FSMDATA (fsm_get_params(pContext, FsmData))

/* limit the number of APs that we'll possible try */
#define NME_WPS_MAX_CANDIDATES 6

/* Maximum time to attempt to find a suitable AP and establish the security
 * parameters with.
 */
#define NME_SECOND (1000)
#define NME_MINUTE (60000)
#define NME_WPS_WALK_TIME (2 * NME_MINUTE)

/* Delay between scans whilst attempt to find suitable APs */
#define NME_DELAY_SCAN_TIME 4000 /* 4 seconds */
#define NME_DELAY_SCAN_ACCURACY 1000 /* 1 second */

/* WPS IE element identities */
#define WPS_DEVICE_PASSWORD_ID 0x1012

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* A pin of all zeros indicates the use of push button mode
 * rather than a pin.
 */
const CsrUint8 NmeWpsPushButtonPin[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* PRIVATE TYPES DEFINITIONS ************************************************/

typedef enum CsrWifiNmeWpsActivedMode
{
    CSR_WIFI_NME_WPS_ACTIVED_MODE_NOT,
    CSR_WIFI_NME_WPS_ACTIVED_MODE_PUSH_BUTTON,
    CSR_WIFI_NME_WPS_ACTIVED_MODE_PIN
} CsrWifiNmeWpsActivedMode;

/* For each possible WPS candidate we need to record the security particulars
 * that we are able to extract from the beacons, so that we know how to configure
 * security.
 */
typedef struct
{
    unifi_MACAddress bssid;
    unifi_SSID ssid;
    CsrUint16 authModeMask;
    CsrUint16 encryptionModeMask;
    CsrUint8 groupCipherSuite;
    CsrUint8 pairwiseCipherSuite;
    unifi_80211PrivacyMode privacyMode;
    CsrBool wpsEnabled;
    CsrWifiNmeWpsActivedMode wpsActivated;
} Network;


/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData
{
    /* Need to retain the appHandle of the request so that it can be sent back in
     * the response
     */
    void* wpsReqAppHandle;

    /* Need to track whether the WPS request has been responded to or not
     * so that in the case of termination the FSM can determine whether
     * it needs to inform the requester that the request was cancelled.
     */
    CsrBool wpsConfirmSent;

    /* Need to take a copy of the pin that is received in the request. */
    CsrUint8 pin[8];

    /* Need to determine whether the request is push button or pin mode. */
    CsrBool isPushButton;

    /* Need to record the time that the WPS process started as the process
     * of attempting to connect is allowed continue until NME_WPS_WALK_TIME
     * has expired.
     */
    CsrUint32 startTime;

    /* Need to rework the scan results into a candidate list of APs to try and connect to
     * Assuming for the moment that a short array is ok, if not it could be reworked into
     * a list in the future.
     */
    CsrUint8 candidateCount;
    Network candidateList[NME_WPS_MAX_CANDIDATES];

    /* Track the candidate index that we are attempting to connect to */
    CsrUint8 attemptingCandidateIndex;

    /* If we get the scan results and don't find any suitable candidates
     * don't trigger another scan straight away. This is the timer for
     * implementing the delay between scans.
     */
    FsmTimerId delayScanRequestTimer;

    /* Security Manager identifier, set at creation, used for passing
     * signals and termination.
     */
    CsrUint16 securityManagerInstance;

    /* Need to track the result of the last WPS attempt, as at the time
     * of the failure we don't know if we'll find anything else to try.
     * So when reporting the status if the whole WPS procedure fails
     * we'll assume that this is the result to report rather than just
     * not found.
     */
    unifi_Status lastWpsAttemptStatus;

    /* Need to track whether we are associated with an AP or not */
    CsrBool associated;

    /* Need to track whether we have been asked to cancel the req*/
    void* userCancelReqAppHandle;

    /* We could get multi MEDIA STATUS IND reporting connected but
     * we only want to try the WPS after the first one, otherwise
     * we can end up stuck looping.
     */
    CsrBool startedSecurity;

    /* The goal is to generate and return a profile, look after it here
     */
    unifi_Profile *wpsProfile;

    /* Track the FSM that requested termination so that the confirm is
     * sent back to the correct FSM. Assuming here that the FSM will
     * still remain present.
     */
    CsrUint16 requestedTerminationFsm;

} FsmData;


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
    /* Attempting to find suitable APs */
    FSMSTATE_scanning,
    /* Attempting the WPS procedure with a suitable AP */
    FSMSTATE_attemptingWps,
    /* Waiting for security to terminate before either
     * 1. trying another AP that we've already found
     * 2. trying to find suitable APs
     * 3. reporting the final result
     */
    FSMSTATE_waitingforSecTerminate,
    /* Wait for the disconnection of the currently
     * associated AP to finish
     */
    FSMSTATE_waitingforDisconnected,
    /* terminating the WPS FSM */
    FSMSTATE_terminating,
    FSMSTATE_MAX_STATE
} FsmState;


/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/
/**
 * @brief
 *   Stops the delayed scan timer if it is running
 *
 * @par Description
 *   See brief.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void nme_wps_stop_delayed_scan_timer(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_stop_delayed_scan_timer()"));
    if (pFsmData->delayScanRequestTimer.uniqueid)
    {
        fsm_remove_timer(pContext, pFsmData->delayScanRequestTimer);
        pFsmData->delayScanRequestTimer.uniqueid = 0;
    }
}

/**
 * @brief
 *   Requests the SME to perform a scan
 *
 * @par Description
 *   Requests the SME to perform a scan and records the primitive
 *   routing details to ensure that the cfm gets back to this FSM.
 *
 * @param[in] context : FSM context
 *
 * @return
 *   void
 */
static void nme_wps_request_scan(FsmContext* pContext)
{
    /* Have to route any response via the network selector FSM as this FSM is
     * dynamic and might not exist when the response is received.
     */
    nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_MGT_SCAN_FULL_CFM_ID, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
    call_unifi_nme_mgt_scan_full_req(
            pContext,
            NME_APP_HANDLE(pContext),
            0, /* ssidCount */
            NULL,
            &NmeBroadcastBssid,
            TRUE, /* Force the scan */
            unifi_Infrastructure, /* assuming infrastructure only for WPS */
            unifi_ScanActive,
            0, /* channelListCount */
            NULL, /* channelList */
            0, /* probeIeLength */
            NULL /* probeIe */);
}

/**
 * @brief
 *   Completes the termination of the FSM
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 *
 * @return
 *   void
 */
static void nme_wps_complete_termination(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_complete_termination()"));
    send_nme_wps_terminate_cfm(pContext, pFsmData->requestedTerminationFsm);
    fsm_next_state(pContext, FSM_TERMINATE);
}

/**
 * @brief
 *   Checks the supplied IE buffer to look for WPA/WPA2 information
 *
 * @par Description
 *   Checks the supplied IE buffer to look for WPA/WPA2 information
 *   and will update the supplied network record with the details.
 *
 * @param[in] CsrUint8* : IE buffer
 * @param[in] CsrUint32 : buffer length
 * @param[in/out] Network *: network record to update
 *
 * @return
 *   void
 */
static void nme_wps_wpa_or_wpa2_enabled_check(
        CsrUint8* pBuffer,
        CsrUint16 bufferLen,
        Network *pNetwork)
{
    srs_scan_data sRelement;
    CsrUint16 groupCiphers = 0;
    CsrUint16 pairwiseCiphers = 0;

    if (0 == bufferLen)
    {
        return;
    }

    require(TR_NME_WPS_FSM, NULL != pBuffer);
    require(TR_NME_WPS_FSM, NULL != pNetwork);

    /* Use the same function as the SME to extract the auth modes and
     * cipher suites supported and then select the most secure.
     */
    CsrMemSet(&sRelement, 0, sizeof(srs_scan_data));
    ie_rsn_update_ap_capabilities(&sRelement, pBuffer, bufferLen,
            FALSE /* assume that this doesn't matter for the moment - allowUnicastUseGroupCipher */);

    if(sRelement.securityData.authMode_CapabilityFlags & (unifi_8021xAuthWPA  | unifi_8021xAuthWPAPSK ))
    {
        pNetwork->authModeMask = unifi_8021xAuthWPAPSK | unifi_80211AuthOpen;
        groupCiphers = sRelement.securityData.wpaCipherCapabilityFlags & GROUP_CIPHER_MASK;
        pairwiseCiphers = sRelement.securityData.wpaCipherCapabilityFlags & PAIRWISE_CIPHER_MASK;

        sme_trace_debug((TR_NME_WPS_FSM,
                         "cipher suites WPA grp 0x%x pw 0x%x",
                         groupCiphers,pairwiseCiphers));
    }
    if(sRelement.securityData.authMode_CapabilityFlags & (unifi_8021xAuthWPA2 | unifi_8021xAuthWPA2PSK ))
    {
        /* Select WPA 2 over WPA */
        pNetwork->authModeMask = unifi_8021xAuthWPA2PSK | unifi_80211AuthOpen;
        groupCiphers = sRelement.securityData.wpa2CipherCapabilityFlags & GROUP_CIPHER_MASK;
        pairwiseCiphers = sRelement.securityData.wpa2CipherCapabilityFlags & PAIRWISE_CIPHER_MASK;

        sme_trace_debug((TR_NME_WPS_FSM,
                         "cipher suites WPA2 grp 0x%x pw 0x%x",
                         groupCiphers, pairwiseCiphers));
    }

    pNetwork->encryptionModeMask = groupCiphers;

    /* Use the group cipher suite if there is no pairwise defined */
    if (0 == pairwiseCiphers)
    {
        pNetwork->pairwiseCipherSuite = ((groupCiphers & ~unifi_EncryptionCipherGroupSms4) >> 8) ;
    }
    else
    {
        /* use the strongest pairwise cipher defined */
        if (pairwiseCiphers & unifi_EncryptionCipherPairwiseWep40)
        {
            pNetwork->pairwiseCipherSuite = unifi_EncryptionCipherPairwiseWep40;
        }
        if (pairwiseCiphers & unifi_EncryptionCipherPairwiseWep104)
        {
            pNetwork->pairwiseCipherSuite = unifi_EncryptionCipherPairwiseWep104;
        }
        if (pairwiseCiphers & unifi_EncryptionCipherPairwiseTkip)
        {
            pNetwork->pairwiseCipherSuite = unifi_EncryptionCipherPairwiseTkip;
        }
        if (pairwiseCiphers & unifi_EncryptionCipherPairwiseCcmp)
        {
            pNetwork->pairwiseCipherSuite = unifi_EncryptionCipherPairwiseCcmp;
        }
    }
    pNetwork->encryptionModeMask |= pNetwork->pairwiseCipherSuite;

    sme_trace_debug((TR_NME_WPS_FSM,
                    "FINAL cipher suites grp 0x%x, pw 0x%x mask 0x%x",
                    groupCiphers, pNetwork->pairwiseCipherSuite, pNetwork->encryptionModeMask));
}

/**
 * @brief
 *   Checks the supplied IE buffer to look for WPS information
 *
 * @par Description
 *   Checks the supplied IE buffer to look for WPS information
 *   and will update the supplied network record with the details.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] CsrUint8* : IE buffer
 * @param[in] CsrUint32 : buffer length
 * @param[in/out] Network *: network record to update
 *
 * @return
 *   void
 */
static void nme_wps_activated_check(
        FsmContext* pContext,
        CsrUint8 *pBuffer,
        CsrUint8 bufferSize,
        Network *pNetwork)
{
    CsrUint8 *pBufferEnd;
    CsrUint8 *pIndex;

    if (0 == bufferSize)
    {
        return;
    }

    require(TR_NME_WPS_FSM, NULL != pBuffer);
    require(TR_NME_WPS_FSM, NULL != pNetwork);

    pNetwork->wpsActivated = CSR_WIFI_NME_WPS_ACTIVED_MODE_NOT;

    /* Need to iterate over the TLV IEs in the buffer and see if the
     * record it's current settings. Due to the problem of some APs not
     * correctly populating the WPS IE and indicating that they are active
     * we can not just ignore WPS capable PAs that are not indicating they
     * are active for registration.
     */
    pBufferEnd = pBuffer + bufferSize;
    pIndex = pBuffer;
    while(pIndex < pBufferEnd)
    {
        CsrUint32 type;
        CsrUint32 len;
        type = *pIndex++ << 8; /*lint !e661 !e662 as data[1] in ie_wps.h */
        if( pIndex > pBufferEnd ) break;
        type |= *pIndex++; /*lint !e661 !e662 as data[1] in ie_wps.h */
        if( pIndex > pBufferEnd ) break;
        len = *pIndex++ << 8; /*lint !e661 !e662 as data[1] in ie_wps.h */
        if( pIndex > pBufferEnd ) break;
        len |= *pIndex++; /*lint !e661 !e662 as data[1] in ie_wps.h */
        if( pIndex > pBufferEnd ) break;

        if (WPS_DEVICE_PASSWORD_ID == type)
        {
            CsrUint32 attribute = *pIndex << 8;
            attribute |= *(pIndex + 1 ); /*lint !e661 !e662 as data[1] in ie_wps.h */
            /* attribute will be 0x0000 for PIN mode and 0x0004 for pushbutton mode */
            if (0x0000 == attribute)
            {
                sme_trace_info((TR_NME_WPS_FSM, "PIN mode activated"));
                pNetwork->wpsActivated = CSR_WIFI_NME_WPS_ACTIVED_MODE_PIN;
                break;
            }
            if (0x0004 & attribute)
            {
                sme_trace_info((TR_NME_WPS_FSM, "Pushbutton mode activated"));
                pNetwork->wpsActivated = CSR_WIFI_NME_WPS_ACTIVED_MODE_PUSH_BUTTON;
                break;
            }
        }
        pIndex += len; /*lint !e661 !e662 as data[1] in ie_wps.h */
    }
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
static void nme_wps_forward_to_security(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_forward_to_security()"));
    if (FSM_TERMINATE != pFsmData->securityManagerInstance)
    {
        fsm_forward_event(pContext, pFsmData->securityManagerInstance, pEvt);
    }
    else
    {
        sme_trace_debug((TR_NME_WPS_FSM, "discarding event id: %x", pEvt->id));
        nme_free_event(pEvt);
    }
}

/**
 * @brief
 *   reports failure of the WPS to the requester
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] unifi_Status : status
 *
 * @return
 *   void
 */
static void nme_wps_failed(
        FsmContext* pContext,
        unifi_Status status)
{
    FsmData *pFsmData = FSMDATA;
    unifi_Profile *pProfile = nme_profile_manager_get_null_profile();

    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_failed(status: %d", status));

    nme_wps_stop_delayed_scan_timer(pContext);
    if (unifi_NotFound == status)
    {
        /* Might have found a suitable candidate network and had the wps fail
         * on it so report that status rather than not found. Responses are
         * routed through network selector so that it can then terminate
         * this FSM
         */
        send_unifi_nme_wps_cfm(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, pFsmData->wpsReqAppHandle, pFsmData->lastWpsAttemptStatus, *pProfile);
    }
    else
    {
        send_unifi_nme_wps_cfm(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, pFsmData->wpsReqAppHandle, status, *pProfile);
    }
    pFsmData->wpsConfirmSent = TRUE;
    CsrPfree(pProfile);

    /* Termination of the WPS FSM is triggered from the
     * network selector FSM not from here.
     */
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
static void nme_wps_init_data(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_init_data()"));

    pFsmData->wpsReqAppHandle = NULL;
    pFsmData->wpsConfirmSent = FALSE;
    pFsmData->candidateCount = 0;
    pFsmData->attemptingCandidateIndex = 0;
    pFsmData->delayScanRequestTimer.uniqueid = 0;
    pFsmData->securityManagerInstance = FSM_TERMINATE;
    pFsmData->lastWpsAttemptStatus = unifi_NotFound;
    pFsmData->associated = FALSE;
    pFsmData->userCancelReqAppHandle = NULL;
    pFsmData->startedSecurity = FALSE;
    pFsmData->wpsProfile = NULL;
}

/**
 * @brief
 *   FSM initialisation
 *
 * @par Description
 *   Initial FSM Entry function
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void nme_wps_init_fsm(FsmContext* pContext)
{
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_init_fsm()"));
    fsm_create_params(pContext, FsmData);
    nme_wps_init_data(pContext);
    fsm_next_state(pContext, FSMSTATE_idle);
}

/**
 * @brief
 *   Delay sending the scan request
 *
 * @par Description
 *   If there is still enough of the walktime remaining then
 *   time Starts a timer that on expiry will trigger the sending
 *   of the scan request to the SME.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   CsrBool: indicates whether the timer as started or not
 */
static CsrBool nme_wps_delay_scan_request(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    CsrBool startedTimer = FALSE;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_delay_scan_request()"));
    if (CsrTimeLe(fsm_get_time_of_day_ms(pContext),pFsmData->startTime + NME_WPS_WALK_TIME))
    {
        nme_wps_stop_delayed_scan_timer(pContext);
        send_nme_wps_delay_scan_expiry_timer(pContext,
                pFsmData->delayScanRequestTimer,
                NME_DELAY_SCAN_TIME,
                NME_DELAY_SCAN_ACCURACY);
        startedTimer = TRUE;
        fsm_next_state(pContext, FSMSTATE_scanning);
    }
    else
    {
        sme_trace_entry((TR_NME_WPS_FSM, "exceeded walktime - not starting scan"));
    }

    return(startedTimer);
}

/**
 * @brief
 *   Checks if there is a cancel request in progress that
 *   needs to be completed.
 *
 * @par Description
 *   See brief
 *
 * @param[in]  FsmContext*   : FSM context
 *
 * @return
 *   CsrBool: TRUE if an outstanding cancel request was completed
 */
static CsrBool nme_wps_completed_cancel_req(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    CsrBool completedCancel = FALSE;

    if (NULL != pFsmData->userCancelReqAppHandle)
    {
        if (!pFsmData->wpsConfirmSent)
        {
            nme_wps_failed(pContext, unifi_Cancelled);
        }
        send_unifi_nme_wps_cancel_cfm(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, pFsmData->userCancelReqAppHandle, unifi_Success);
        nme_wps_init_data(pContext);
        fsm_next_state(pContext, FSMSTATE_idle);
        completedCancel = TRUE;
    }
    return(completedCancel);
}

/**
 * @brief
 *   Handles a WPS TERMINATE REQ
 *
 * @par Description
 *   Terminates the WPS FSM if there is no security FSM
 *   instance to terminate first, if there is then the
 *   termination of the security FSM is triggered.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    const NmeWpsTerminateReq_Evt* : terminate request event
 *
 * @return
 *   void
 */
static void nme_wps_terminate_req(
        FsmContext* pContext,
        const NmeWpsTerminateReq_Evt* pReq)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_terminate_req()"));

    nme_wps_stop_delayed_scan_timer(pContext);
    pFsmData->requestedTerminationFsm = pReq->common.sender_;

    /* If the initiator of the WPS is not aware of the result then inform them
     * the WPS has been cancelled.
     */
    if (!pFsmData->wpsConfirmSent)
    {
        nme_wps_failed(pContext, unifi_Cancelled);
    }

    if (pFsmData->securityManagerInstance != FSM_TERMINATE)
    {
        /* clean up the security manager */
        send_nme_security_stop_req(pContext, pFsmData->securityManagerInstance);
        /* Need to wait for security to terminate beforFe signalling completion of the termination */
        sme_trace_debug((TR_NME_WPS_FSM, "waiting for security to terminate"));
        fsm_next_state(pContext, FSMSTATE_terminating);
    }
    else if (pFsmData->associated)
    {
        call_unifi_nme_mgt_disconnect_req(pContext, NME_APP_HANDLE(pContext));
        fsm_next_state(pContext, FSMSTATE_terminating);
    }
    else
    {
        nme_wps_complete_termination(pContext);
    }
}


/**
 * @brief
 *   Handles a NME WPS REQ whilst the FSM is idle
 *
 * @par Description
 *   Records the start time for tracking the 2 minute walktime of
 *   the WPS attempt; the appHandle for the cfm and method - push
 *   button or pin. Then requests full scan.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeWpsReq_Evt* : MGT SCAN FULL CFM event
 *
 * @return
 *   void
 */
static void nme_wps_req(
        FsmContext* pContext,
        const UnifiNmeWpsReq_Evt* pWpsReq)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_req()"));

    pFsmData->wpsReqAppHandle = pWpsReq->appHandle;

#if defined(CSR_WIFI_SECURITY_WPS_ENABLE)
    if(strcmp((const char*)pWpsReq->pin, WPS_PUSHBUTTON_PIN))
    {
        sme_trace_info((TR_NME_WPS_FSM, "PIN mode activated" ));
        pFsmData->isPushButton = FALSE;
    }
    else
    {
        sme_trace_info((TR_NME_WPS_FSM, "PUSH BUTTON mode activated" ));
        pFsmData->isPushButton = TRUE;
    }

    CsrMemCpy(pFsmData->pin, pWpsReq->pin, sizeof(pWpsReq->pin));

    /* Now request to scan. The results will be used to generate the candidate list. */
    nme_wps_request_scan(pContext);

    /* Record the start time as we've got a 2  minute (walktime) defined to keep
     * trying the WPS in, until we give up.
     */
    pFsmData->startTime = fsm_get_time_of_day_ms(pContext);
    fsm_next_state(pContext, FSMSTATE_scanning);
#else
    nme_wps_failed(pContext, unifi_Unsupported);
#endif
}


/**
 * @brief
 *   Handles a MGT SCAN FULL CFM whilst the FSM is active
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
static void nme_wps_mgt_scan_full_cfm(
        FsmContext* pContext,
        const UnifiNmeMgtScanFullCfm_Evt* pScanFullCfm)
{
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_mgt_scan_full_cfm(status: %d), pScanFullCfm->status"));

    if (unifi_Success == pScanFullCfm->status)
    {
        nme_routing_store_cfm_prim_internal(pContext, UNIFI_NME_MGT_SCAN_RESULTS_GET_CFM_ID, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance);
        call_unifi_nme_mgt_scan_results_get_req(pContext, NME_APP_HANDLE(pContext));
    }
    else
    {
        if (!nme_wps_delay_scan_request(pContext))
        {
            /* Failed to start the timer so the WPS procedure has failed */
            nme_wps_failed(pContext, unifi_NotFound);
        }
    }
}

/**
 * @brief
 *   Handles a NME WPS DELAY SCAN EXPIRY TIMER whilst the FSM is active
 *
 * @par Description
 *   Requests a scan from the SME. There is no need to check if there
 *   is still time remaining as the timer should only ever be started
 *   if there is. So just trigger the scan.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeWpsDelayScanExpiryTimer_Evt* : TIMER EXPIRY event
 *
 * @return
 *   void
 */
static void nme_wps_delay_scan_expiry(
        FsmContext* pContext,
        const NmeWpsDelayScanExpiryTimer_Evt* pTimer)
{
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_delay_scan_expiry()"));
    nme_wps_request_scan(pContext);
}


/**
 * @brief
 *   Triggers the WPS attempt for the Network referenced via
 *   attemptingCandidateIndex
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 *
 * @return
 *   void
 */
static void nme_wps_start_wps_attempt(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;
    unifi_ConnectionConfig connectionConfig;
    NmeConfigData* pNmeConfig = nme_core_get_nme_config(pContext);
    /* Have to send this fixed IE contents in the Association */
    CsrUint8 wpsIe[] = { 0xdd, 0x0e, 0x00, 0x50, 0xf2, 0x04, 0x10, 0x4a, 0x00, 0x01, 0x10, 0x10, 0x3a, 0x00, 0x01, 0x01 };

    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_start_wps_attempt()"));

    pFsmData->securityManagerInstance = fsm_add_instance(pContext, &nme_security_manager_fsm, TRUE);
    send_nme_security_prewps_req(pContext,
            pFsmData->securityManagerInstance,
            pFsmData->candidateList[pFsmData->attemptingCandidateIndex].ssid,
            pNmeConfig->stationMacAddress,
            pFsmData->pin);

    CsrMemSet(&connectionConfig, 0, sizeof(unifi_ConnectionConfig));

    CsrMemCpy(&(connectionConfig.ssid),
              &(pFsmData->candidateList[pFsmData->attemptingCandidateIndex].ssid),
              sizeof(connectionConfig.ssid));
    CsrMemCpy(&(connectionConfig.bssid),
              &(pFsmData->candidateList[pFsmData->attemptingCandidateIndex].bssid),
              sizeof(connectionConfig.bssid));

    connectionConfig.ifIndex = unifi_GHZ_Both;
    connectionConfig.mlmeAssociateReqInformationElementsLength = sizeof(wpsIe);
    connectionConfig.mlmeAssociateReqInformationElements = (CsrUint8*) CsrPmalloc(sizeof(wpsIe));
    CsrMemCpy(connectionConfig.mlmeAssociateReqInformationElements, wpsIe, sizeof(wpsIe));
    connectionConfig.wmmQosInfo = 0;
    connectionConfig.adhocJoinOnly = FALSE;
    connectionConfig.adhocChannel = 0;
    connectionConfig.bssType = unifi_Infrastructure;
    connectionConfig.privacyMode = pFsmData->candidateList[pFsmData->attemptingCandidateIndex].privacyMode;
    connectionConfig.authModeMask = pFsmData->candidateList[pFsmData->attemptingCandidateIndex].authModeMask;
    connectionConfig.encryptionModeMask = pFsmData->candidateList[pFsmData->attemptingCandidateIndex].encryptionModeMask;
    call_unifi_nme_mgt_connect_req(pContext,
                                   pFsmData->wpsReqAppHandle,
                                   (const unifi_ConnectionConfig *)&connectionConfig);

    CsrPfree(connectionConfig.mlmeAssociateReqInformationElements);
}

/**
 * @brief
 *   Determines whether there is another suitable network candidate
 *   to try, or whether another scan to find suitable networks
 *   should be scheduled.
 *
 * @par Description
 *   If there are still potential candidates left in the list then
 *   the next one will be tried.
 *   Alternatively if there is still enough time remaining then
 *   continue looking for possible candidate networks.
 *
 * @param[in] FsmContext* : FSM context
 *
 * @return
 *   void
 */
static void nme_wps_select_candidate_to_attempt_wps(FsmContext* pContext)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_select_candidate_to_attempt_wps"));

    fsm_next_state(pContext, FSMSTATE_attemptingWps);
    if (pFsmData->attemptingCandidateIndex < (pFsmData->candidateCount - 1))
    {
        /* Try the next candidate */
        pFsmData->attemptingCandidateIndex++;
        nme_wps_start_wps_attempt(pContext);
    }
    else
    {
        /* If we are still within the 2mins that is allowed for the WPS to take place
         * and have enough time to perform a delayed scan again then start the timer.
         * Otherwise there isn't enough time so the WPS has failed.
         */
        if (!nme_wps_delay_scan_request(pContext))
        {
            nme_wps_failed(pContext, unifi_NotFound);
        }
    }
}


/**
 * @brief
 *   Handles the failure of the WPS attempt for the Network
 *   referenced via attemptingCandidateIndex.
 *
 * @par Description
 *   Handles the failure of the WPS attempt for the Network
 *   referenced via attemptingCandidateIndex.
 *   If there are still potential candidates left in the list then
 *   the next one will be tried.
 *   Alternatively if there is still enough time remaining then
 *   continue looking for possible candidate networks.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] unifi_Status : status
 *
 * @return
 *   void
 */
static void nme_wps_failed_wps_attempt(
        FsmContext* pContext,
        unifi_Status status)
{
    FsmData *pFsmData = FSMDATA;

    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_failed_wps_attempt(status: %d)", status));

    pFsmData->lastWpsAttemptStatus = status;
    if (pFsmData->securityManagerInstance != FSM_TERMINATE)
    {
        /* clean up the security manager */
        send_nme_security_stop_req(pContext, pFsmData->securityManagerInstance);
        fsm_next_state(pContext, FSMSTATE_waitingforSecTerminate);
    }
    else
    {
        nme_wps_select_candidate_to_attempt_wps(pContext);
    }
}


/**
 * @brief
 *   Handles a MGT SCAN RESULTS GET CFM whilst the FSM is active
 *
 * @par Description
 *   Needs to parse the scan results to determine if there is a
 *   suitable candidate. If not and we are still in the walktime
 *   then another scan will be performed. If the walktime has
 *   expired then the WPS attempt has failed.
 *   If the scan results contain some candidates we can try connecting
 *   to them.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeMgtScanResultsGetCfm_Evt* : MGT SCAN RESULTS GET CFM event
 *
 * @return
 *   void
 */
static void nme_wps_mgt_scan_results_get_cfm(
        FsmContext* pContext,
        const UnifiNmeMgtScanResultsGetCfm_Evt* pScanResultsGetCfm)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_mgt_scan_results_get_cfm()"));

    /* Need to generate a candidate AP list from the scan results
     * and attempt to connect to each one in turn until either:
     * 1. Successful connect - the WPS is complete
     * 2. the list is finished
     *    a. if there is still time then scan again and repeat the process
     *    b. no time remaining - failed.
     */
    if (unifi_Success != pScanResultsGetCfm->status ||
        0 == pScanResultsGetCfm->scanResultsCount)
    {
        free_unifi_mgt_scan_results_get_cfm_contents((UnifiMgtScanResultsGetCfm_Evt*)pScanResultsGetCfm);
        /* The scan failed or there are no actual results
         *  so delay the next scan request
         */
        if (!nme_wps_delay_scan_request(pContext))
        {
            /* handle failure of the complete WPS procedure */
            nme_wps_failed(pContext, unifi_NotFound);
        }
    }
    else
    {
        unifi_ScanResult* pCurrentScanResult = pScanResultsGetCfm->scanResults;
        CsrUint8 i = 0;
        pFsmData->candidateCount = 0;
        for (i=0; i< pScanResultsGetCfm->scanResultsCount; i++)
        {
            ie_wps* pWpsIe;
            ie_result getIeResult;

            getIeResult = ie_get_wps(pCurrentScanResult->informationElements,
                                     pCurrentScanResult->informationElementsLength,
                                     pWpsIe);
            if (ie_success == getIeResult)
            {
                /* This AP is at least WPS capable, so add it to our candidate list */
                pFsmData->candidateList[pFsmData->candidateCount].bssid = pCurrentScanResult->bssid;
                pFsmData->candidateList[pFsmData->candidateCount].ssid = pCurrentScanResult->ssid;

                sme_trace_debug((TR_NME_WPS_FSM, "WPS IE present for %s, %s",
                                 trace_unifi_SSID(&(pCurrentScanResult->ssid), getNMESSIDBuffer(pContext)),
                                 trace_unifi_MACAddress(pCurrentScanResult->bssid, getNMEMacAddressBuffer(pContext))));

                pFsmData->candidateList[pFsmData->candidateCount].wpsEnabled = TRUE;
                pFsmData->candidateList[pFsmData->candidateCount].wpsActivated = FALSE;
                nme_wps_activated_check(
                         pContext,
                         &(pWpsIe->data[0]),
                         pWpsIe->length - 4, /* Remove the length of the oui */
                         &(pFsmData->candidateList[pFsmData->candidateCount]));

                if((pCurrentScanResult->capabilityInformation & 0x0010) != 0)
                {
                    pFsmData->candidateList[pFsmData->candidateCount].privacyMode = unifi_80211PrivacyEnabled;
                }
                else
                {
                    pFsmData->candidateList[pFsmData->candidateCount].privacyMode = unifi_80211PrivacyDisabled;
                }
                pFsmData->candidateList[pFsmData->candidateCount].authModeMask = unifi_80211AuthOpen;
                pFsmData->candidateList[pFsmData->candidateCount].groupCipherSuite = CSR_WIFI_SECURITY_WPS_CIPHER_SUITE_UNKNOWN;
                pFsmData->candidateList[pFsmData->candidateCount].pairwiseCipherSuite = CSR_WIFI_SECURITY_WPS_CIPHER_SUITE_UNKNOWN;
                pFsmData->candidateList[pFsmData->candidateCount].encryptionModeMask = 0;

                nme_wps_wpa_or_wpa2_enabled_check(pCurrentScanResult->informationElements,
                                                   pCurrentScanResult->informationElementsLength,
                                                   &(pFsmData->candidateList[pFsmData->candidateCount]));
                pFsmData->candidateCount++;

                /* If the request was for pin mode then we can try multiple APs as
                 * the pin should ensure that the credentials are obtained from the
                 * correct AP. However for push button mode we should only attempt to
                 * connect if a single AP is found. If more than a single AP is found
                 * then no connection should be attempted and the WPS attempt should
                 * be aborted.
                 */
                if (!pFsmData->isPushButton)
                {
                    /* Optimise the number of connections that we attempt based on how
                     * long we've already been trying and the number of networks that
                     * we attempt.
                     */
                    if (CSR_WIFI_NME_WPS_ACTIVED_MODE_PUSH_BUTTON == pFsmData->candidateList[pFsmData->candidateCount - 1].wpsActivated)
                    {
                        /* Not in correct mode so remove */
                        sme_trace_debug((TR_NME_WPS_FSM, "Ignoring active WPS AP in push button mode as pin required"));
                        pFsmData->candidateCount--;
                    }
                    else if (CsrTimeGt(((30 * NME_SECOND) + pFsmData->startTime), fsm_get_time_of_day_ms(pContext)))
                    {
                        if (CSR_WIFI_NME_WPS_ACTIVED_MODE_PIN == pFsmData->candidateList[pFsmData->candidateCount - 1].wpsActivated)
                        {
                            /* Restrict the count to the first active WPS AP */
                            sme_trace_debug((TR_NME_WPS_FSM, "active WPS AP found in 30s"));
                            break;
                        }
                        else
                        {
                            /* Only look at active WPS APs for the first 30 seconds */
                            sme_trace_debug((TR_NME_WPS_FSM, "Ignoring enabled but inactive WPS AP within first 30s"));
                            pFsmData->candidateCount--;
                        }
                    }
                    else if (CsrTimeGt(((50 * NME_SECOND) + pFsmData->startTime), fsm_get_time_of_day_ms(pContext)))
                    {
                        /* After the first 30 seconds expand the results to include
                         * WPS enabled APs, but still restrict it to the first AP
                         * that is seen.
                         */
                        sme_trace_debug((TR_NME_WPS_FSM, "Found single WPS enabled/active AP in 50s"));
                        break;
                    }
                    else if (CsrTimeGt(((70 * NME_SECOND) + pFsmData->startTime), fsm_get_time_of_day_ms(pContext)) &&
                             2 == pFsmData->candidateCount)
                    {
                        /* From 50 to 70 seconds try the first 2 APs in the candidate list */
                        sme_trace_debug((TR_NME_WPS_FSM, "Found 2 WPS enabled/active AP in 50s"));
                        break;
                    }
                    else if (CsrTimeGt(((90 * NME_SECOND) + pFsmData->startTime), fsm_get_time_of_day_ms(pContext)) &&
                             4 == pFsmData->candidateCount)
                    {
                        /* From 70 to 90 seconds try up to the first 4 APs in the list */
                        sme_trace_debug((TR_NME_WPS_FSM, "Found 4 WPS enabled/active AP in 90s"));
                        break;
                    }
                    else if (NME_WPS_MAX_CANDIDATES == pFsmData->candidateCount)
                    {
                        /* After 90 seconds try a maximum of 6 APs */
                        break;
                    }
                }
                else
                {
                    if (CSR_WIFI_NME_WPS_ACTIVED_MODE_PUSH_BUTTON != pFsmData->candidateList[pFsmData->candidateCount - 1].wpsActivated)
                    {
                        /* Only look at active WPS APs for the push button mode */
                        sme_trace_debug((TR_NME_WPS_FSM, "Ignoring enabled but inactive WPS AP for push button mode"));
                        pFsmData->candidateCount--;
                    }
                }
            }
            pCurrentScanResult++;
        }
        free_unifi_mgt_scan_results_get_cfm_contents((UnifiMgtScanResultsGetCfm_Evt*)pScanResultsGetCfm);

        if (pFsmData->isPushButton &&
            1 < pFsmData->candidateCount)
        {
            sme_trace_debug((TR_NME_WPS_FSM, "push button mode overlap error"));
            nme_wps_failed(pContext, unifi_SecurityError);
        }
        if (0 < pFsmData->candidateCount)
        {
            /* If we've managed to find something we can now try to connect to it */
            pFsmData->attemptingCandidateIndex = 0;
            nme_wps_start_wps_attempt(pContext);
            fsm_next_state(pContext, FSMSTATE_attemptingWps);
        }
        else
        {
            sme_trace_debug((TR_NME_WPS_FSM, "No WPS candidates found in scan results"));
            /* If we are still within the 2mins that is allowed for the WPS to take place
             * and have enough time to perform a delayed scan again then start the timer.
             * Otherwise there isn't enough time so the WPS has failed.
             */
            if (!nme_wps_delay_scan_request(pContext))
            {
                sme_trace_entry((TR_NME_WPS_FSM, "walktime exceeded"));
                nme_wps_failed(pContext, unifi_NotFound);
            }
        }
    }
}


/**
 * @brief
 *   Handles a NME WPS CANCEL REQ when WPS is active
 *
 * @par Description
 *   Handles a NME WPS CANCEL REQ, the FSM is reset back to
 *   the idle state and the CFM sent.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeWpsCancelReq_Evt* : NME WPS CANCEL REQ event
 *
 * @return
 *   void
 */
static void nme_wps_cancel_req_wps_active(
        FsmContext* pContext,
        const UnifiNmeWpsCancelReq_Evt* pWpsCancelReq)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_cancel_req_wps_active()"));

    nme_wps_stop_delayed_scan_timer(pContext);
    pFsmData->userCancelReqAppHandle = pWpsCancelReq->appHandle;

    /* Don't response to the cancel request until security has terminated and we're disconnected */
    if (pFsmData->securityManagerInstance != FSM_TERMINATE)
    {
        /* clean up the security manager */
        send_nme_security_stop_req(pContext, pFsmData->securityManagerInstance);
        fsm_next_state(pContext, FSMSTATE_waitingforSecTerminate);
    }
    else if (pFsmData->associated)
    {
        call_unifi_nme_mgt_disconnect_req(pContext, NME_APP_HANDLE(pContext));
        fsm_next_state(pContext, FSMSTATE_waitingforDisconnected);
    }
    else
    {
        (void)nme_wps_completed_cancel_req(pContext);
    }
}


/**
 * @brief
 *   Handles a NME WPS CANCEL REQ when WPS is terminating
 *
 * @par Description
 *   Just sends back a cfm as the FSM is terminating.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeWpsCancelReq_Evt* : NME WPS CANCEL REQ event
 *
 * @return
 *   void
 */
static void nme_wps_terminating_cancel_req_wps(
        FsmContext* pContext,
        const UnifiNmeWpsCancelReq_Evt* pWpsCancelReq)
{
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_terminating_cancel_req_wps()"));

    /* Already terminating so no need to pass the response through network selector. */
    call_unifi_nme_wps_cancel_cfm(pContext, pWpsCancelReq->appHandle, unifi_Success);
}

/**
 * @brief
 *   Handles a received UNIFI_NME_MGT_MEDIA_STATUS_IND
 *
 * @par Description
 *   If the received UNIFI_NME_MGT_MEDIA_STATUS_IND indicates that
 *   the connection was established to the AP the WPS security procedure
 *   will be started.
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_wps_media_status_ind(FsmContext* pContext, const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    const UnifiNmeMgtMediaStatusInd_Evt* pMediaStatusInd = (UnifiNmeMgtMediaStatusInd_Evt*)pEvt;

    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_media_status_ind(status : %x)", pMediaStatusInd->mediaStatus));

    if (unifi_MediaConnected == pMediaStatusInd->mediaStatus)
    {
        if (!pFsmData->startedSecurity)
        {
            CsrUint8 *pIe = NULL;

            /* Need to take a copy of any assoc req info element present in the connection
             * info so that it can be passed to security.
             */
            if (0 < pMediaStatusInd->connectionInfo.assocReqInfoElementsLength)
            {
                pIe = CsrPmalloc(pMediaStatusInd->connectionInfo.assocReqInfoElementsLength);
                CsrMemCpy(pIe, pMediaStatusInd->connectionInfo.assocReqInfoElements, pMediaStatusInd->connectionInfo.assocReqInfoElementsLength);
            }

            pFsmData->startedSecurity = TRUE;
            send_nme_security_start_req(pContext,
                    pFsmData->securityManagerInstance,
                    pMediaStatusInd->connectionInfo.assocReqInfoElementsLength,
                    pIe,
                    pMediaStatusInd->connectionInfo.ssid,
                    pMediaStatusInd->connectionInfo.bssid);
        }
        else
        {
            nme_wps_failed_wps_attempt(pContext, unifi_Error);
        }
    }
    else
    {
        /* handle indication of a disconnection. */
        pFsmData->associated = FALSE;
        nme_wps_failed_wps_attempt(pContext, unifi_Error);
    }
    /* Hiding the NME MEDIA STATUS IND (ie not forwarding to the NME SAP)
     * as part of the WPS handling. Also the connection will be disconnected
     * once the profile is created.
     */
    free_unifi_mgt_media_status_ind_contents((const UnifiMgtMediaStatusInd_Evt*)pMediaStatusInd);
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
static void nme_wps_connect_cfm(FsmContext* pContext, const FsmEvent* pEvt)
{
    const UnifiNmeMgtConnectCfm_Evt* pConnectCfm = (UnifiNmeMgtConnectCfm_Evt*)pEvt;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_connect_cfm(status: %d)", pConnectCfm->status));

    if (unifi_Success != pConnectCfm->status)
    {
        nme_wps_failed_wps_attempt(pContext, pConnectCfm->status);
    }
    else
    {
        FsmData *pFsmData = FSMDATA;
        pFsmData->associated = TRUE;
    }
}

/**
 * @brief
 *   Handles a received NME_SECURITY_PREWPS_CFM_ID
 *
 * @par Description
 *   If the security actions have failed then try the next
 *   AP.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityPrewpsCfm_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_wps_security_prewps_cfm(
        FsmContext* pContext,
        const NmeSecurityPrewpsCfm_Evt* pSecPreWpsCfm)
{
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_security_prewps_cfm(status: %d)", pSecPreWpsCfm->status));
    if (nme_SecurityResultCodeSuccess != pSecPreWpsCfm->status)
    {
        nme_wps_failed_wps_attempt(pContext, unifi_Error);
    }
}


/**
 * @brief
 *   Handles a received NME_SECURITY_COMPLETE_IND_ID
 *
 * @par Description
 *   If the security actions have failed then try the next
 *   AP.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityPrewpsCfm_Evt* : Event
 *
 * @return
 *   void
 */
static void nme_wps_security_complete_ind_active(
        FsmContext* pContext,
        const NmeSecurityCompleteInd_Evt* pSecCompleteInd)
{
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_security_complete_ind_active(status: %d)", pSecCompleteInd->result));
    if (nme_SecurityResultCodeSuccess == pSecCompleteInd->result)
    {
        FsmData *pFsmData = FSMDATA;
        unifi_Credentials *pCredentials = nme_sec_get_credentials(pContext);
        unifi_SSID wpsSsid = nme_sec_get_wps_ssid(pContext);

        if (NULL == pCredentials)
        {
            nme_wps_failed_wps_attempt(pContext, unifi_Error);
            return;
        }

        pFsmData->wpsProfile = nme_profile_manager_get_null_profile();

        CsrMemCpy(&(pFsmData->wpsProfile->credentials), pCredentials, sizeof(*pCredentials));
        CsrPfree(pCredentials);

        pFsmData->wpsProfile->bssType = unifi_Infrastructure;
        pFsmData->wpsProfile->ccxOptionsMask = 0;
        pFsmData->wpsProfile->channelNo = 0;
        pFsmData->wpsProfile->wmmQosCapabilitiesMask = 0;
        pFsmData->wpsProfile->cloakedSsid = FALSE;
        pFsmData->wpsProfile->profileIdentity.bssid = pFsmData->candidateList[pFsmData->attemptingCandidateIndex].bssid;
        pFsmData->wpsProfile->profileIdentity.ssid = wpsSsid;

        /* clean up the security manager */
        send_nme_security_terminate_req(pContext, pFsmData->securityManagerInstance);
        fsm_next_state(pContext, FSMSTATE_waitingforSecTerminate);
    }
    else
    {
        nme_wps_failed_wps_attempt(pContext, unifi_Error);
    }
}


/**
 * @brief
 *   Handles a received NME_SECURITY_TERMINATE_CFM_ID
 *
 * @par Description
 *   Handles a received NME_SECURITY_TERMINATE_CFM_ID whilst the WPS
 *   FSM is active and is waiting for the security FSM to terminate
 *   before trying another candidate network.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_wps_security_terminate_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_security_terminate_cfm()"));

    /* the dynamic Security FSM has terminated */
    pFsmData->securityManagerInstance = FSM_TERMINATE;
    pFsmData->startedSecurity = FALSE;

    if (pFsmData->associated)
    {
        call_unifi_nme_mgt_disconnect_req(pContext, NME_APP_HANDLE(pContext));
        fsm_next_state(pContext, FSMSTATE_waitingforDisconnected);
    }
    else if (!nme_wps_completed_cancel_req(pContext))
    {
        nme_wps_select_candidate_to_attempt_wps(pContext);
    }
}

/**
 * @brief
 *   Handles a received NME_MGT_DISCONNECT_CFM_ID
 *
 * @par Description
 *   Completed the disconnection from the associated AP
 *   before trying another candidate network.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_wps_disconnect_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_disconnect_cfm()"));
    pFsmData->associated = FALSE;

    if (NULL != pFsmData->wpsProfile)
    {
        send_unifi_nme_wps_cfm(pContext, getNmeContext(pContext)->nmeNetworkSelectorFsmInstance, pFsmData->wpsReqAppHandle, unifi_Success, *pFsmData->wpsProfile);
        pFsmData->wpsConfirmSent = TRUE;
        CsrPfree(pFsmData->wpsProfile);
        fsm_next_state(pContext, FSMSTATE_idle);
    }
    else if (!nme_wps_completed_cancel_req(pContext))
    {
        nme_wps_select_candidate_to_attempt_wps(pContext);
    }
}

/**
 * @brief
 *   Handles a received SECURITY_TERMINATE_CFM_ID
 *
 * @par Description
 *   Terminates the WPS FSM as the security
 *   FSM has terminated.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_wps_terminating_security_terminate_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_terminating_security_terminate_cfm()"));

    /* the dynamic Security FSM has terminated */
    pFsmData->securityManagerInstance = FSM_TERMINATE;
    if (pFsmData->associated)
    {
        call_unifi_nme_mgt_disconnect_req(pContext, NME_APP_HANDLE(pContext));
    }
    else
    {
        nme_wps_complete_termination(pContext);
    }
}

/**
 * @brief
 *   Handles a received MGT_DISCONNECT_CFM_ID
 *
 * @par Description
 *   Terminates the WPS FSM.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] FsmEvent* : Event
 *
 * @return
 *   void
 */
static void nme_wps_terminating_disconnect_cfm(
        FsmContext* pContext,
        const FsmEvent* pEvt)
{
    FsmData *pFsmData = FSMDATA;
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_terminating_disconnect_cfm()"));

    /* Assume that the disconnect is successful */
    pFsmData->securityManagerInstance = FSM_TERMINATE;
    pFsmData->associated = FALSE;
    nme_wps_complete_termination(pContext);
}

/**
 * @brief
 *   FSM data reset
 *
 * @par Description
 *   Has to ensure that any resources used by the FSM
 *   are freed and the FSM is placed in the TERMINATE
 *   state so that it is cleared.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void nme_wps_reset_fsm(FsmContext* pContext)
{
    sme_trace_entry((TR_NME_WPS_FSM, "nme_wps_reset_fsm()"));

    nme_wps_stop_delayed_scan_timer(pContext);
    fsm_next_state(pContext, FSM_TERMINATE);
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * See description in nme_wps_fsm/nme_wps_fsm.h
 */
/*---------------------------------------------------------------------------*/
CsrUint16 nme_wps_get_security_fsm_instance(FsmContext* pContext)
{
    CsrUint16 secMgrInstance = FSM_TERMINATE;
    CsrUint16 wpsInstance = nme_ntw_selector_get_wps_fsm_instance(pContext);

    if (FSM_TERMINATE != wpsInstance)
    {
        secMgrInstance = fsm_get_params_by_id(pContext, wpsInstance, FsmData)->securityManagerInstance;
    }
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_wps_get_security_fsm_instance Sec Instance %d", secMgrInstance));
    return (secMgrInstance);
}

/* FSM DEFINITION **********************************************/

static const FsmEventEntry idleTransitions[] =
{
    /*                    Signal Id,                                     Function */
    /* As the FSM is dynamic this is the only valid event to receive */
    fsm_event_table_entry(UNIFI_NME_WPS_REQ_ID,                          nme_wps_req),
};

static const FsmEventEntry scanningTransitions[] =
{
    /*                    Signal Id,                                     Function */
    fsm_event_table_entry(UNIFI_NME_WPS_CANCEL_REQ_ID,                   nme_wps_cancel_req_wps_active),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_FULL_CFM_ID,                nme_wps_mgt_scan_full_cfm),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULTS_GET_CFM_ID,         nme_wps_mgt_scan_results_get_cfm),
    fsm_event_table_entry(NME_WPS_DELAY_SCAN_EXPIRY_TIMER_ID,            nme_wps_delay_scan_expiry),
};

static const FsmEventEntry attemptingWpsTransitions[] =
{
    /*                    Signal Id,                                     Function */
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             nme_wps_media_status_ind),
    fsm_event_table_entry(UNIFI_NME_MGT_CONNECT_CFM_ID,                  nme_wps_connect_cfm),
    fsm_event_table_entry(UNIFI_NME_WPS_CANCEL_REQ_ID,                   nme_wps_cancel_req_wps_active),
    fsm_event_table_entry(NME_SECURITY_PREWPS_CFM_ID,                    nme_wps_security_prewps_cfm),
    fsm_event_table_entry(NME_SECURITY_COMPLETE_IND_ID,                  nme_wps_security_complete_ind_active),
};

static const FsmEventEntry waitingforSecTerminateTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_SECURITY_TERMINATE_CFM_ID,                 nme_wps_security_terminate_cfm),
    fsm_event_table_entry(NME_WPS_TERMINATE_REQ_ID,                      fsm_saved_event),
    fsm_event_table_entry(UNIFI_NME_WPS_CANCEL_REQ_ID,                   fsm_saved_event),

    /* Security is terminating so don't pass these event into it */
    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_CFM_ID,              fsm_ignore_event),
};

static const FsmEventEntry waitingforDisconnectedTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(UNIFI_NME_MGT_DISCONNECT_CFM_ID,               nme_wps_disconnect_cfm),
    fsm_event_table_entry(UNIFI_NME_WPS_CANCEL_REQ_ID,                   fsm_saved_event),
    fsm_event_table_entry(NME_WPS_TERMINATE_REQ_ID,                      fsm_saved_event),
};

static const FsmEventEntry terminatingTransitions[] =
{
                       /* Signal Id,                                     Function */
    fsm_event_table_entry(NME_WPS_TERMINATE_REQ_ID,                      fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_WPS_CANCEL_REQ_ID,                   nme_wps_terminating_cancel_req_wps),
    fsm_event_table_entry(NME_SECURITY_TERMINATE_CFM_ID,                 nme_wps_terminating_security_terminate_cfm),
    fsm_event_table_entry(UNIFI_NME_MGT_DISCONNECT_CFM_ID,               nme_wps_terminating_disconnect_cfm),

    /* Security is terminating so don't pass these event into it */
    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_CFM_ID,              fsm_ignore_event),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry defaultHandlers[] =
{
                           /* Signal Id,                             Function */
    /* These events need to be forwarded to security */
    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,              nme_wps_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,    nme_wps_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID,  nme_wps_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                    nme_wps_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,              nme_wps_forward_to_security),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_CFM_ID,              nme_wps_forward_to_security),

    fsm_event_table_entry(NME_WPS_TERMINATE_REQ_ID,                      nme_wps_terminate_req),

    /* Media status ind for disconnections are ignored */
    fsm_event_table_entry(UNIFI_NME_MGT_MEDIA_STATUS_IND_ID,             fsm_ignore_event),

    /* Always comes back successful so ignore it. */
    fsm_event_table_entry(NME_SECURITY_START_CFM_ID,                     fsm_ignore_event),

    /* Could have had the request cancelled during scan so we'll need to ignore these results */
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_FULL_CFM_ID,                fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_MGT_SCAN_RESULTS_GET_CFM_ID,         fsm_ignore_event),
    fsm_event_table_entry(NME_SECURITY_PREWPS_CFM_ID,                    fsm_ignore_event),
};

/** NME Core state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                           State                     State                               Save     */
   /*                           Name                      Transitions                          *       */
   fsm_state_table_entry(FSMSTATE_idle,                   idleTransitions,                     FALSE),
   fsm_state_table_entry(FSMSTATE_scanning,               scanningTransitions,                 FALSE),
   fsm_state_table_entry(FSMSTATE_attemptingWps,          attemptingWpsTransitions,            FALSE),
   fsm_state_table_entry(FSMSTATE_waitingforSecTerminate, waitingforSecTerminateTransitions,   TRUE),
   fsm_state_table_entry(FSMSTATE_waitingforDisconnected, waitingforDisconnectedTransitions,   TRUE),
   fsm_state_table_entry(FSMSTATE_terminating,            terminatingTransitions,              TRUE),
};

const FsmProcessStateMachine nme_wps_fsm =
{
#ifdef FSM_DEBUG
       "Nme Wps",                               /* SM Process Name       */
#endif
       NME_WPS_PROCESS,                         /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},        /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, defaultHandlers, FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),  /* Ignore Event handers */
       nme_wps_init_fsm,                        /* Entry Function        */
       nme_wps_reset_fsm,                       /* Reset Function        */
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
