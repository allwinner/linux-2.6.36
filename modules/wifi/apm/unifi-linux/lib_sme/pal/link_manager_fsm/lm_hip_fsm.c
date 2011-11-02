/** @file lm_hip_fsm.c
 *
 * PAL Link Manager HIP FSM
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
 *  This FSM is tasked assisting Link FSM to connect to peer AMP device including
 *  seucurity handshake and activity reports. Basically all interactions with HIP IO
 *  is dealt by this FSM.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/lm_hip_fsm.c#5 $
 *
 ****************************************************************************/

/** @{
 * @ingroup link_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "link_manager_fsm/lm_hip_fsm.h"
#include "link_manager_fsm/hip_interface.h"
#include "event_pack_unpack/event_pack_unpack.h"
#include "payload_manager/payload_manager.h"
#include "pal_manager/pal_manager.h"
#include "regulatory_domain/regulatory_domain.h"
#include "scan_manager_fsm/scan_results_storage.h"
#include "hip_proxy_fsm/mib_encoding.h"
#include "hip_proxy_fsm/mib_action_sub_fsm.h"
#include "link_manager_fsm/lm_link_fsm.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, HipFsmData))

#define CSR_WPA_RETRANSMISSION_TIMEOUT (100)
#define CSR_WPA_RETRANSMISSION_COUNT (3)

#define PAL_UNITDATA_MAC_ADDR_SIZE (12) /* size of mac addresses in unitdata frame */
#define PAL_GET_SNAP_PTR(frame) (frame+PAL_UNITDATA_MAC_ADDR_SIZE)
#define PAL_GET_AR_PAYLOAD(frame) (frame+PAL_UNITDATA_MAC_ADDR_SIZE+PAL_HIP_SNAP_HEADER_SIZE)
#define PAL_MIN_FRAME_SIZE (PAL_UNITDATA_MAC_ADDR_SIZE+PAL_HIP_SNAP_HEADER_SIZE) /* mac addresses (12bytes) + snap(8bytes) */
#define PAL_MIN_AR_SIZE (PAL_UNITDATA_MAC_ADDR_SIZE+PAL_HIP_SNAP_HEADER_SIZE+PAL_MIN_ACTIVITY_REPORT_LEN)

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
    /** Waiting for connect req */
    FSMSTATE_disconnected,

    /** wait for MLME Probe response */
    FSMSTATE_scanning,

    /** wait for MLME Association */
    FSMSTATE_associating,

    /** wait for MLME Authentication */
    FSMSTATE_authenticating,

    /** wait for MLME Deauthentication */
    FSMSTATE_deauthenticating,

    /** wait for handshake to complete */
    FSMSTATE_security_handshake,

    /** installing keys */
    FSMSTATE_installing_keys,

    /** wait for MLME Association */
    FSMSTATE_connected,

    /** Last enum in the list */
    FSMSTATE_MAX_STATE
} FsmState;

typedef enum HipReasonForDisconnect
{
    DISCONNECT_DUE_TO_KEY_FAILURE,
    DISCONNECT_DUE_TO_PEER_DEAUTHENTICATION,
    DISCONNECT_DUE_TO_SECURITY_HANDSHAKE_FAILURE
}HipReasonForDisconnect;

#ifdef SME_TRACE_ENABLE
typedef struct HipFsmStatistics
{
    /* num HIP fsm instances created */
    CsrInt32 numInstancesCreated;
    CsrInt32 numConnectionsSucceeded;
    CsrInt32 numConnectionsFailed;
    CsrInt32 numPreHandshakeSucceeded;
    CsrInt32 numHandshakeFailed;

    /* probe retries stats*/
    CsrInt32 totalRetryCount;
    CsrInt32 retryFailedCount;
    CsrInt32 retryPassedCount;

    CsrInt32 deauthFailedCount; /* Count of mlme_deauthentication_ind from peer */
    CsrInt32 disconnectRequestedDuringHandshake; /* primarily to capture TX failures of handshake messages */

    CsrInt32 m1ReceivedCount; /* For Acceptor */
    CsrInt32 m2ReceivedCount; /* For Initiator */
    CsrInt32 connectedIndCount;
    CsrInt32 connectedIndCount_disconnected;
    CsrInt32 connectedIndCount_connected;
    CsrUint8 connectionResult[200];
}HipFsmStatistics;
#endif

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct HipFsmData
{
    CsrUint8 physicalLinkHandle;
    PalRole role;
    HipReasonForDisconnect reasonForDisconnect;
    CsrWifiSecurityContext* securityContext;

    unifi_DataBlock remoteRsnIE;
    FsmTimerId securityHandshakeTimer;

    CsrBool disconnectPending;
#ifdef SME_TRACE_ENABLE
     CsrBool firstHandshakeRecvd;
#endif
} HipFsmData;


/* PRIVATE CONSTANT DEFINITIONS *********************************************/
#define PAL_START_TIMER(timer,duration) send_pal_security_handshake_timer(context,timer, duration, 50)
#define PAL_STOP_TIMER(timer) {if (timer.id) fsm_remove_timer(context, timer); timer.id=0;}

/* PRIVATE VARIABLE DEFINITIONS *********************************************/
#ifdef SME_TRACE_ENABLE
static HipFsmStatistics stats;
#endif

/* PRIVATE FUNCTION PROTOTYPES **********************************************/


/* PRIVATE FUNCTION DEFINITIONS *********************************************/


/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   Resets the FSM data back to the initial state
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void init_pal_lm_hip_data(FsmContext* context)
{
    HipFsmData *fsmData = FSMDATA;
    sme_trace_entry((TR_PAL_LM_HIP_FSM, "init_pal_lm_hip_data()"));

    CsrMemSet(fsmData,0,sizeof(HipFsmData));
}

/**
 * @brief
 *   PAL_LM_Hip FSM Entry Function
 *
 * @par Description
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void pal_lm_hip_init(FsmContext* context)
{
    sme_trace_entry((TR_PAL_LM_LINK_FSM, "pal_lm_hip_init()"));

    fsm_create_params(context, HipFsmData);
    init_pal_lm_hip_data(context);
    fsm_next_state(context, FSMSTATE_disconnected);
}

#ifdef FSM_DEBUG_DUMP
static void lm_hip_fsm_dump(FsmContext* context, const CsrUint16 id);
#endif

/**
 * @brief
 *   Saves the remote RSN IE
 *
 * @par Description
 *   If rsnIE is NULL then the saved RSN IE if any is deleted.
 *
 * @param[in]    fsmData   : FSM context
 * @param[in]    rsnIE   : remote RSN IE. Set this to NULL if you want to delete the saved RSN IE
 * @param[in]    rsnIELen   : length of the received RSN IE
 *
 * @return
 *   void
 */
static void pal_set_remote_rsn_ie(HipFsmData *fsmData, CsrUint8 *rsnIE, CsrUint8 rsnIELen)
{
    if (rsnIE)
    {
        if (fsmData->remoteRsnIE.data==NULL)
        {
            sme_trace_info((TR_PAL_LM_HIP_FSM,"pal_set_remote_rsn_ie:remote RSN IE of length %d",rsnIELen));
            sme_trace_hex((TR_PAL_LM_HIP_FSM, TR_LVL_DEBUG, "pal_set_remote_rsn_ie:Remote RSNIE:", rsnIE, rsnIELen));
            fsmData->remoteRsnIE.data = CsrPmalloc(rsnIELen);
            CsrMemCpy(fsmData->remoteRsnIE.data,rsnIE, rsnIELen);
            fsmData->remoteRsnIE.length = rsnIELen;
        }
        else
        {
            sme_trace_crit((TR_PAL_LM_HIP_FSM,"pal_set_remote_rsn_ie:remote RSN IE of length %d exits",fsmData->remoteRsnIE.length));
            sme_trace_hex((TR_PAL_LM_HIP_FSM, TR_LVL_DEBUG, "pal_set_remote_rsn_ie:Remote RSNIE:", fsmData->remoteRsnIE.data, fsmData->remoteRsnIE.length));
        }
    }
    else if (fsmData->remoteRsnIE.data)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"pal_set_remote_rsn_ie:freeing remote RSN IE"));
        CsrPfree(fsmData->remoteRsnIE.data);
        CsrMemSet(&fsmData->remoteRsnIE, 0, sizeof(fsmData->remoteRsnIE));
    }
}

/**
 * @brief
 *   Cleanup function
 *
 * @par Description
 *   function called before terminating the FSM to free up resources.
 *
 * @param[in]    fsmData   : FSM context
 *
 * @return
 *   void
 */
static void hip_fsm_cleanup(FsmContext *context)
{
    HipFsmData *fsmData = FSMDATA;

    if (fsmData->securityContext)
    {
        CsrWifiSecurityDeInit(fsmData->securityContext);
        fsmData->securityContext = NULL;
        pal_set_remote_rsn_ie(fsmData, NULL,0);
    }

    fsm_next_state(context, FSMSTATE_disconnected);
}

/**
 * @brief
 *   Encode activity report
 *
 * @par Description
 *   If rsnIE is NULL then the saved RSN IE if any is deleted.
 *
 * @param[in]    activityReport   : Activity report structure to encode
 * @param[out]    resultBuf   : buffer where encoded activity report will be written
 *
 * Notes: Function assumed resultBuf holds enough memory to write the encoded data.
 *
 * @return: length of the encoded data.
 *
 */
static CsrUint16 encode_activity_report(const PalActivityReport* activityReport, CsrUint8 *resultBuf)
{
    CsrUint8 *buf = resultBuf;
    CsrInt32 i;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"encode_activity_report()"));
    (void)event_pack_CsrUint8(&buf, (CsrUint8)activityReport->scheduleKnown);
    (void)event_pack_CsrUint8(&buf, activityReport->numReports);

    for (i=0; i<activityReport->numReports; i++)
    {
        (void)event_pack_CsrUint32(&buf, activityReport->arTriplet[i].startTime);
        (void)event_pack_CsrUint32(&buf, activityReport->arTriplet[i].duration);
        (void)event_pack_CsrUint32(&buf, activityReport->arTriplet[i].periodicity);
    }
    return (CsrUint16)(buf-resultBuf);
}

/**
 * @brief
 *  Security handshake timer callback. called by the security library to start a timer
 *
 * @par Description
 *
 * @param[in]    clientCtx   : Client context
 * @param[out]    duration   : duration of the timer
 *
 * @return
 *   void
 */
static void start_security_timer_callback(void *clientCtx, const CsrUint32 duration, const CsrWifiSecurityTimeout timeoutId)
{
    FsmContext *context = (FsmContext *)clientCtx;
    HipFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"start_security_timer_callback(): Start WPA timer of duration %d milliseconds for timeoutId-%d",duration, timeoutId));

    /* PAL needs only the message retry timer. Ignore other timer requests */
    if (timeoutId != CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"start_security_timer_callback(): unsupported timer id. Ignore request without starting timer"));
        return;
    }

    if (!fsmData->securityHandshakeTimer.id)
    {
        PAL_START_TIMER(fsmData->securityHandshakeTimer,duration);
    }
    else
    {
        sme_trace_warn((TR_PAL_LM_HIP_FSM,"start_security_timer_callback():security retry timer already running"));
    }
}

/**
 * @brief
 *  Security handshake timer callback. called by the security library to stop a timer
 *
 * @par Description
 *
 * @param[in]    clientCtx   : Client context
 *
 * @return
 *   void
 */
static void stop_security_timer_callback(void *clientCtx, CsrWifiSecurityTimeout timeoutId)
{
    FsmContext *context = (FsmContext *)clientCtx;
    HipFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"stop_security_timer_callback(): timeoutId-%d",timeoutId));

    /* PAL needs only the message retry timer. Ignore other timer requests */
    if (timeoutId != CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"stop_security_timer_callback(): unsupported timer id. Ignore request without stopping timer"));
        return;
    }

    if (fsmData->securityHandshakeTimer.id)
    {
        PAL_STOP_TIMER(fsmData->securityHandshakeTimer);
    }
    else
    {
        sme_trace_warn((TR_PAL_LM_HIP_FSM,"stop_security_timer_callback():security retry timer NOT running"));
    }
}

/**
 * @brief
 *  Security handshake callback. called by the security library to abort the handshake due to a failure
 *
 * @par Description
 * FIXME: A reason for aborting may be a good idea so that clients can take different actions based
 * on the reason for abort. Raise with Jim. Espcially where we may not want to deauthenticate
 * if there is a MIC error as it could be a spoofed message. So it may be good idea to ignore
 * MIC Errors.
 *
 * @param[in]    clientCtx   : Client context
 *
 * @return
 *   void
 */
static void abort_security_handshake_callback(void *clientCtx )
{
    FsmContext *context = (FsmContext *)clientCtx;
    HipFsmData *fsmData = FSMDATA;
    unifi_MACAddress *address = pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"abort_security_handshake_callback(): Aborting security handshake"));

#ifdef SME_TRACE_ENABLE
    stats.deauthFailedCount++;
#endif

    verify(TR_PAL_LM_HIP_FSM, address!=NULL);
    PAL_STOP_TIMER(fsmData->securityHandshakeTimer);

    if (PalRole_PalRoleInitiator == fsmData->role)
    {
        sme_trace_entry((TR_PAL_LM_HIP_FSM,"abort_security_handshake_callback(): Initiator. deauthenticate peer"));
        pal_hip_send_mlme_deauthenticate_req(context, (unifi_MACAddress *)address);
        fsmData->reasonForDisconnect = DISCONNECT_DUE_TO_SECURITY_HANDSHAKE_FAILURE;
        fsm_next_state(context, FSMSTATE_deauthenticating);
    }
    else
    {
        sme_trace_entry((TR_PAL_LM_HIP_FSM,"abort_security_handshake_callback(): . acceptor: simply go away"));
#ifdef SME_TRACE_ENABLE
        stats.numConnectionsFailed++;
        stats.numHandshakeFailed++;
#endif
        send_pal_link_connect_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                  unifi_Error, FALSE);
        hip_fsm_cleanup(context);
    }
}

/**
 * @brief
 *  Security handshake callback. called by the security library to send the security message to peer.
 *
 * @par Description
 *
 * @param[in]    clientCtx   : Client context
 * @param[in]    buffer   : buffer including the SNAP to send
 * @param[in]    bufLen   : Length of the buffer
 * @param[in]    localMac   : local MAC Address
 * @param[in]    peerMac   : peer MAC Address
 *
 * @return
 *   void
 */
static void send_eapol_message_callback(void *clientCtx,
                                               const CsrUint8* buffer,
                                               const CsrUint32 bufLen,
                                               const CsrUint8* localMac,
                                               const CsrUint8* peerMac)
{
    FsmContext *context = (FsmContext *)clientCtx;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"send_eapol_message_callback(): Send EAPOL-KEY"));
    sme_trace_hex((TR_PAL_LM_HIP_FSM, TR_LVL_DEBUG, "send_eapol_message_callback():",peerMac, CSR_WIFI_SECURITY_MACADDRESS_LENGTH));

    /* Check for EAPOL-KEY */
    if( 0x03 == buffer[PAL_HIP_SNAP_HEADER_SIZE+1] )
    {
        HipFsmData *fsmData = FSMDATA;

        CsrUint32 hostTag = PAL_HIP_FSM_HOST_TAG_ID | (context->currentInstance->instanceId << 16);
        pal_hip_send_ma_unitdata_req(context,
                                     (unifi_MACAddress *)peerMac,
                                     pal_get_link_qos_support(context, fsmData->physicalLinkHandle),
                                     buffer+PAL_HIP_SNAP_HEADER_SIZE,
                                     (CsrUint16)(bufLen-PAL_HIP_SNAP_HEADER_SIZE),
                                     PAL_DATA_PROTO_SECURITY_FRAMES_ID,
                                     hostTag );
    }
    else
    {
        sme_trace_crit((TR_PAL_LM_HIP_FSM,"send_eapol_message_callback():EAPOL incorrect"));
        sme_trace_hex((TR_PAL_LM_HIP_FSM, TR_LVL_DEBUG, "send_eapol_message_callback():",buffer,bufLen));
        verify(TR_PAL_LM_HIP_FSM,NULL==buffer); /* Something that will definitely fail and should not generate lint warning */
    }
}


/**
 * @brief
 *  Security handshake callback. called by the security library to install the Pairwise keys
 * once handhsake is succesfully completed
 *
 * @par Description
 *
 * @param[in]    clientCtx   : Client context
 * @param[in]    key   : key to install
 * @param[in]    keyLength   : length of the key
 *
 * @return
 *   void
 */
static void install_pairwise_key_callback(void* clientCtx,
        CsrWifiSecurityKeyType secKeyType, const CsrUint8* key, const CsrUint32 keyLength, const CsrUint8* rsc, const CsrUint8 keyIndex)
{
    FsmContext *context = (FsmContext *)clientCtx;
    HipFsmData *fsmData = FSMDATA;
    CsrBool authenticator_or_supplicant = (fsmData->role==PalRole_PalRoleInitiator)?TRUE:FALSE; /* Initiatory is authenticator */
    CsrUint16 receiveSequenceCount[8];
    CsrUint16 keyId;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"install_pairwise_key_callback():Install Keys"));
    sme_trace_hex((TR_PAL_LM_HIP_FSM, TR_LVL_DEBUG, "install_pairwise_key_callback()",key, keyLength));

    /* Stop the timer just in case */
    PAL_STOP_TIMER(fsmData->securityHandshakeTimer);

    /* clause 8.5.3.7 IEEE 2007 says:
     * "The PTK and GTK are installed by using MLME-SETKEYS.request primitive
     *  after Message 4 is sent. The PTK is installed before the GTK."
     */
    CsrMemSet(receiveSequenceCount, 0, sizeof(CsrUint16) * 8);
    keyId = 0;
    pal_hip_send_mlme_setkeys_req(context,
                                  KeyType_Pairwise,
                                  key,
                                  keyLength,
                                  keyId,
                                  receiveSequenceCount,
                                  pal_get_remote_mac_address(context, fsmData->physicalLinkHandle),
                                  authenticator_or_supplicant);

    fsm_next_state(context, FSMSTATE_installing_keys);
}

/**
 * @brief
 *  Security handshake callback. called by the security library to install the Group keys
 * once handhsake is succesfully completed
 *
 * @par Description
 *    AMP does not use GTK
 *
 * @param[in]    clientCtx   : Client context
 * @param[in]    key   : key to install
 * @param[in]    keyLength   : length of the key
 *
 * @return
 *   void
 */
static void install_group_key_callback(void* clientCtx, CsrWifiSecurityKeyType secKeyType,
                                       const CsrUint8* key, const CsrUint32 keyLength,
                                       const CsrUint8* rsc, const CsrUint8 keyIndex)
{
    /* Do Nothing */
    sme_trace_hex((TR_PAL_LM_HIP_FSM, TR_LVL_DEBUG, "install_group_key_callback()",key, keyLength));
}

/**
 * @brief
 *  Security library callback. called by the security library after WPS
 *
 * @par Description
 *    AMP does not use WPS
 *
 * @return
 *   void
 */
static void wpsDone(void* myCtx, const CsrUint8 result, const CsrUint32 authenticationType,
                    const CsrUint32 encryptionType, const CsrUint8 *networkKey, const CsrUint16 networkKeyLength,
                    const CsrUint8 networkKeyIndex, const CsrUint8 *ssid)
{
    sme_trace_info((TR_SME_TRACE, "WPS Done" ));
}

/**
 * @brief
 *  Security library callback. called by the security library for session resumption
 *
 * @par Description
 *    AMP does not use session resumption
 *
 * @return
 *   void
 */
static void install_session_callback(void *myCtx, const CsrUint8 *session, const CsrUint32 sessionLength)
{
    sme_trace_info((TR_SME_TRACE, "install_session_callback()"));
}

/**
 * @brief
 *  Security library callback. called by the security library to install the PAC
 *
 * @par Description
 *    AMP does not use PAC
 *
 * @param[in]    clientCtx   : Client context
 * @param[in]    pac         : PAC to install
 * @param[in]    pacLength   : length of the PAC
 *
 * @return
 *   void
 */
static void install_pac_callback(void* clientCtx, const CsrUint8* pac, const CsrUint32 pacLength)
{
    /* Do Nothing */
    sme_trace_hex((TR_PAL_LM_HIP_FSM, TR_LVL_DEBUG, "install_pac_callback()",pac, pacLength));
}

/**
 * @brief
 *  Free a processed packet
 *
 * @par Description
 *  Frees a packet
 *
 * @param
 *  void*   : Global client context
 *
 * @return
 *  void
 */
static void packet_processing_done(void *clientCtx, CsrUint32 appCookie)
{
    sme_trace_entry((TR_SME_TRACE, "packet_processing_done: cookie - %d",appCookie));
    CsrPfree((CsrUint8 *)appCookie);
}

static void pmkid_store(void* myCtx, const CsrUint8 bssid[CSR_WIFI_SECURITY_MACADDRESS_LENGTH], const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH], const CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH])
{
}

static CsrBool pmk_get(void* myCtx, const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH], CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH])
{
    return FALSE;
}

static const CsrWifiSecuritycallbacks secCallbacks = {
    send_eapol_message_callback,
    install_pairwise_key_callback,
    install_group_key_callback,
    install_pac_callback,
    install_session_callback,
    wpsDone,
    start_security_timer_callback,
    stop_security_timer_callback,
    abort_security_handshake_callback,
    packet_processing_done,
    pmkid_store,
    pmk_get
};

/* this is event only the PAL is authenticator. This is to allow mlme to complete -mlme-associate-rsp handling. */
/**
 * @brief
 *  function handler to handle mlme-get-cfm from the sub-fsm
 *
 * @par Description
 *    This is to allow mlme to complete -mlme-associate-rsp handling when PAL is configured as authenticator.
 * Hopefully it will deal with raise condition in firmware where it discards the ma-unitdata-req (message 1) because
 * mlme has not finished its configuration.
 *
 * @param[in]    context   : FSM Context
 * @param[in]    cfm   : pointer to mlme-get-cfm
 *
 * @return
 *   void
 */
static void mlme_syncing__mlme_get_cfm(FsmContext *context, const MlmeGetCfm_Evt *cfm)
{
    HipFsmData *fsmData = FSMDATA;
    CsrWifiSecurityStart(fsmData->securityContext);

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }
    verify(TR_PAL_LM_HIP_FSM,cfm->dummyDataRef2.dataLength==0);

    fsm_next_state(context, FSMSTATE_security_handshake);
}

/**
 * @brief
 *  function start the security handshake procedure
 *
 * @par Description
 *    Initialises the security library and kick starts the procedure
 *
 * @param[in]    context   : FSM Context
 *
 * @return
 *   void
 */
static void start_security_handshake(FsmContext *context)
{
    HipFsmData *fsmData = FSMDATA;
    CsrWifiSecuritySetup secSetup;

    /* Security Setup */
    CsrMemSet(&secSetup, 0x00, sizeof(secSetup));
    CsrMemCpy(secSetup.localMacAddress, pal_get_local_mac_address(context)->data, CSR_WIFI_SECURITY_MACADDRESS_LENGTH);
    CsrMemCpy(secSetup.peerMacAddress, pal_get_remote_mac_address(context, fsmData->physicalLinkHandle)->data, CSR_WIFI_SECURITY_MACADDRESS_LENGTH);
    CsrMemCpy(secSetup.pmk, fsm_get_params_by_id(context, pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle), physicalLinkAttrib)->linkKey, CSR_WIFI_SECURITY_PMK_LENGTH);
    secSetup.pmkValid = TRUE;
    secSetup.secIe = CsrPmalloc(PAL_HIP_IE_RSN_SIZE);
    secSetup.secIeLen = pal_hip_build_rsn_element_ie(context, secSetup.secIe);
    secSetup.protocolSnapValid = TRUE;
    secSetup.protocolSnap[0] = 0xAA;
    secSetup.protocolSnap[1] = 0xAA;
    secSetup.protocolSnap[2] = 0x03;
    secSetup.protocolSnap[3] = 0x00;
    secSetup.protocolSnap[4] = 0x19;
    secSetup.protocolSnap[5] = 0x58;
    secSetup.protocolSnap[6] = 0x00;
    secSetup.protocolSnap[7] = 0x03;

    /* Ignore SNAP for now */
    secSetup.responseTimeout = CSR_WPA_RETRANSMISSION_TIMEOUT;
    secSetup.retransmissionAttempts = CSR_WPA_RETRANSMISSION_COUNT;

    sme_trace_info((TR_PAL_LM_HIP_FSM,"wait_for_authenticate__pal_link_authenticate_req():security handshake started"));
    if (PalRole_PalRoleInitiator == fsmData->role)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"wait_for_authenticate__pal_link_authenticate_req():Authenticator"));
        secSetup.mode = CSR_WIFI_SECURITY_WPAPSK_MODE_AUTHENTICATOR;
        secSetup.securityType = CSR_WIFI_SECURITY_TYPE_WPAPSK_AUTHENTICATOR;
    }
    else
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"wait_for_authenticate__pal_link_authenticate_req():Supplicant"));
        secSetup.mode = CSR_WIFI_SECURITY_WPAPSK_MODE_SUPPLICANT;
        secSetup.securityType = CSR_WIFI_SECURITY_TYPE_WPAPSK_SUPPLICANT;
    }

    fsmData->securityContext = CsrWifiSecurityInit(context, getSmeContext(context)->cryptoContext, &secSetup, &secCallbacks);

    CsrPfree(secSetup.secIe);

    /* authenticator will start with the message 1 */
    if (CSR_WIFI_SECURITY_TYPE_WPAPSK_AUTHENTICATOR == secSetup.securityType)
    {
        /* Perform a dummy mib read to ensure that association procedure is complete. This is to tackle
         * the raise condition where M1 gets into unifi before association_rsp handling is complete in mlme
         */
        DataReference mibGetDataRef = mib_encode_create_get(context, 1);
        sme_trace_info((TR_PAL_LM_HIP_FSM,"wait_for_authenticate__pal_link_authenticate_req():authenticator: start security handshake"));

        (void)mib_encode_add_get(context, &mibGetDataRef, dot11RTSThreshold, unifi_GHZ_2_4,0);
        mib_get_sub_fsm_start(context, mlme_syncing__mlme_get_cfm, &mibGetDataRef, FALSE);
    }
    else
    {
        fsm_next_state(context, FSMSTATE_security_handshake);
    }
}


/************************************** TRANSITION FUNCTIONS *******************************************/

static void disconnected__pal_link_stop_req(FsmContext *context, const PalLinkStopReq_Evt *req)
{
    fsm_next_state(context, FSM_TERMINATE);
}

static void disconnected__pal_link_connect_req(FsmContext *context, const PalLinkConnectReq_Evt *req)
{
    HipFsmData *fsmData = FSMDATA;

    fsmData->role = req->role;
    fsmData->physicalLinkHandle = fsm_get_params_by_id(context, req->common.sender_, physicalLinkAttrib)->handle;

    if (PalRole_PalRoleResponder== req->role)
    {
        CsrUint32 crntTime = fsm_get_time_of_day_ms(context);

        /* cancel any other Scans as this takes precedence */
        send_sm_scan_cancel_ind(context, getSmeContext(context)->scanManagerInstance, crntTime, TRUE);
        send_sm_scan_req(context,
                         getSmeContext(context)->scanManagerInstance,
                         crntTime,
                         pal_get_selected_channel_no(context),
                         0,
                         NULL,
                         *pal_get_remote_mac_address(context, fsmData->physicalLinkHandle),
                         TRUE,
                         TRUE,
                         FALSE,
                         unifi_ScanStopAllResults,
                         BssType_Infrastructure,
                         0);

        fsm_next_state(context, FSMSTATE_scanning);
    }
    else
    {
        fsm_next_state(context, FSMSTATE_authenticating);
    }
}

static void disconnected__pal_link_disconnect_req(FsmContext *context, const PalLinkDisconnectReq_Evt *req)
{
    HipFsmData *fsmData = FSMDATA;
    send_pal_link_disconnect_cfm(context,
                                 pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                 unifi_Success);

    hip_fsm_cleanup(context);
}

static void scanning__mlme_scan_cfm(FsmContext *context, const SmScanCfm_Evt *cfm)
{
    HipFsmData *fsmData = FSMDATA;

    if (fsmData->disconnectPending)
    {
        fsmData->disconnectPending = FALSE;
        send_pal_link_connect_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                  unifi_Error, FALSE);

        send_pal_link_disconnect_cfm(context,
                                     pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                     unifi_Success);
        hip_fsm_cleanup(context);
    }
    else
    {
         unifi_MACAddress *address = pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);

        sme_trace_entry((TR_PAL_LM_HIP_FSM,"scanning__mlme_scan_cfm: role-%d,result-%d",fsmData->role,cfm->scanResult));
        verify(TR_PAL_LM_HIP_FSM, address!=NULL);

        if (TRUE == cfm->joinScanResult)
        {
            if (pal_security_enabled(context))
            {
                srs_scan_data* pJoinScanParameters = srs_get_join_scan_parameters(context);
                CsrUint8 *result = pal_hip_ie_parameter_check(context, &pJoinScanParameters->informationElements,IE_ID_RSN);

                if (pJoinScanParameters->securityData.wpa2CipherCapabilityFlags)
                {
                    sme_trace_info((TR_PAL_LM_HIP_FSM,"remote RSN IE present of length %d",result[1]));
                    /* FIXME: Should we validate the RSN IE . May be the security handshake lib can do it ??*/
                    pal_set_remote_rsn_ie(fsmData, result, result[1]+2);

                    pal_hip_send_mlme_authenticate_req(context, (unifi_MACAddress *)address);
                    fsm_next_state(context, FSMSTATE_authenticating);
                }
                else
                {
                    sme_trace_info((TR_PAL_LM_HIP_FSM,"remote RSN IE NOT present. Link will be disconnected!!"));
                    pal_set_remote_rsn_ie(fsmData, NULL,0);

                    send_pal_link_connect_cfm(context,
                                              pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                              unifi_Error, FALSE);
                    hip_fsm_cleanup(context);
                }
            }
            else
            {
                pal_hip_send_mlme_authenticate_req(context, (unifi_MACAddress *)address);
                fsm_next_state(context, FSMSTATE_authenticating);
            }
        }
        else
        {
            sme_trace_info((TR_PAL_LM_HIP_FSM,"scanning__mlme_scan_cfm: failed"));

    #ifdef SME_TRACE_ENABLE
            stats.numConnectionsFailed++;
    #endif

            send_pal_link_connect_cfm(context,
                                      pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                      unifi_Error, FALSE);
            hip_fsm_cleanup(context);
        }
    }
}

static void scanning__pal_link_disconnect_req(FsmContext *context, const PalLinkDisconnectReq_Evt *req)
{
    HipFsmData *fsmData = FSMDATA;

    fsmData->disconnectPending = TRUE;
}

static void authenticating__mlme_authenticate_cfm(FsmContext *context, const MlmeAuthenticateCfm_Evt *cfm)
{
    HipFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"authenticating__mlme_authenticate_cfm: role-%d,result-%d",fsmData->role,cfm->resultCode));

    if (fsmData->disconnectPending)
    {
        fsmData->disconnectPending = FALSE;
        send_pal_link_connect_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                  unifi_Error, FALSE);
        send_pal_link_disconnect_cfm(context,
                                     pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                     unifi_Success);
        hip_fsm_cleanup(context);
    }
    else
    {
        if (PalRole_PalRoleResponder == fsmData->role)
        {
            if (ResultCode_Success == cfm->resultCode)
            {
                unifi_MACAddress *address = pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);
                verify(TR_PAL_LM_HIP_FSM, address!=NULL);
                /* ignore message if its not from the device we are expecting*/
                if (!CsrMemCmp(cfm->peerStaAddress.data, address->data, sizeof(cfm->peerStaAddress.data)))
                {
                    pal_hip_send_mlme_associate_req(context,(unifi_MACAddress *)address);
                    fsm_next_state(context, FSMSTATE_associating);
                }
            }
            else
            {
    #ifdef SME_TRACE_ENABLE
                stats.numConnectionsFailed++;
    #endif
                send_pal_link_connect_cfm(context,
                                          pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                          unifi_Error, FALSE);
                hip_fsm_cleanup(context);
            }
        }
        else
        {
            sme_trace_warn((TR_PAL_LM_HIP_FSM,"Unexpected mlme_authenticate_cfm while PAL is a initiator"));
        }
    }

    if (cfm->informationElements.dataLength)
    {
        pld_release(getPldContext(context), cfm->informationElements.slotNumber);
    }
    verify(TR_PAL_LM_HIP_FSM,cfm->dummyDataRef2.dataLength==0);
}

static void authenticating__mlme_authenticate_ind(FsmContext *context, const MlmeAuthenticateInd_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;
    unifi_MACAddress *address = pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"authenticating__mlme_authenticate_ind: role-%d,authtype-%d",fsmData->role,ind->authenticationType));
    verify(TR_PAL_LM_HIP_FSM, address!=NULL);
    /* ignore message if its not from the device we are expecting*/
    if (!CsrMemCmp(ind->peerStaAddress.data, address->data, sizeof(ind->peerStaAddress.data)) &&
        PalRole_PalRoleInitiator == fsmData->role)
    {
        if (AuthenticationType_OpenSystem == ind->authenticationType)
        {
            pal_hip_send_mlme_authenticate_rsp(context, ResultCode_Success, (unifi_MACAddress *)address);
            /* Wait for associate-ind */
            fsm_next_state(context, FSMSTATE_associating);
        }
        else
        {
            pal_hip_send_mlme_authenticate_rsp(context, ResultCode_RefusedNotAuthenticated, (unifi_MACAddress *)address);
            send_pal_link_connect_cfm(context,
                                      pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                      unifi_Error, FALSE);
#ifdef SME_TRACE_ENABLE
            stats.numConnectionsFailed++;
#endif

            hip_fsm_cleanup(context);
        }
    }
    else
    {
        sme_trace_warn((TR_PAL_LM_HIP_FSM,"Unexpected mlme_authenticate_ind: wrong remote addr and/or PAL is Responder"));
    }

    if (ind->informationElements.dataLength)
    {
        pld_release(getPldContext(context), ind->informationElements.slotNumber);
    }
    verify(TR_PAL_LM_HIP_FSM,ind->dummyDataRef2.dataLength==0);
}

static void authenticating__mlme_deauthenticate_ind(FsmContext *context, const MlmeDeauthenticateInd_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;

#ifdef SME_TRACE_ENABLE
    stats.deauthFailedCount++;
    stats.numConnectionsFailed++;
#endif

    send_pal_link_connect_cfm(context,
                              pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                              unifi_Error, FALSE);

    if (fsmData->disconnectPending)
    {
        fsmData->disconnectPending = FALSE;
        send_pal_link_disconnect_cfm(context,
                                     pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                     unifi_Success);
    }

    if (ind->informationElements.dataLength)
    {
        pld_release(getPldContext(context), ind->informationElements.slotNumber);
    }
    verify(TR_PAL_LM_HIP_FSM,ind->dummyDataRef2.dataLength==0);
    hip_fsm_cleanup(context);
}

static void authenticating__pal_link_disconnect_req(FsmContext *context, const PalLinkDisconnectReq_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;

    /* Save disconnect request if FSM is the responder (Waiting for an mlme-authenticate-cfm).
     * Otherwise finish it off
     */
    if (PalRole_PalRoleInitiator == fsmData->role)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"authenticating__pal_link_disconnect_req(): waiting for mlme-authenticate-ind. can be disconnected straight away"));

        send_pal_link_connect_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                  unifi_Error, FALSE);

        send_pal_link_disconnect_cfm(context,
                                     pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                     unifi_Success);
        hip_fsm_cleanup(context);
    }
    else
    {
        fsmData->disconnectPending = TRUE;
    }
}

static void deauthenticating__mlme_deauthenticate_cfm(FsmContext *context, const MlmeDeauthenticateCfm_Evt *cfm)
{
    HipFsmData *fsmData = FSMDATA;

    if (DISCONNECT_DUE_TO_KEY_FAILURE==fsmData->reasonForDisconnect ||
        DISCONNECT_DUE_TO_SECURITY_HANDSHAKE_FAILURE==fsmData->reasonForDisconnect)
    {
#ifdef SME_TRACE_ENABLE
        stats.numConnectionsFailed++;
        stats.numHandshakeFailed++;
#endif
        send_pal_link_connect_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                  unifi_Error, FALSE);
    }

    if (fsmData->disconnectPending)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"deauthenticating__mlme_deauthenticate_cfm(): disconnection pending. so complete it"));
        fsmData->disconnectPending = FALSE;

        send_pal_link_disconnect_cfm(context,
                                     pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                     unifi_Success);
    }

    if (cfm)
    {
        verify(TR_PAL_LM_HIP_FSM,cfm->dummyDataRef2.dataLength==0 && cfm->dummyDataRef1.dataLength==0);
    }
    hip_fsm_cleanup(context);
}

static void deauthenticating__mlme_deauthenticate_ind(FsmContext *context, const MlmeDeauthenticateInd_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;

#ifdef SME_TRACE_ENABLE
    stats.deauthFailedCount++;
    stats.numConnectionsFailed++;
#endif

    send_pal_link_connect_cfm(context,
                              pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                              unifi_Error, FALSE);
    if (ind->informationElements.dataLength)
    {
        pld_release(getPldContext(context), ind->informationElements.slotNumber);
    }
    verify(TR_PAL_LM_HIP_FSM,ind->dummyDataRef2.dataLength==0);
    hip_fsm_cleanup(context);
}

static void deauthenticating__mlme_deletekeys_cfm(FsmContext *context, const MlmeDeletekeysCfm_Evt *cfm)
{
    HipFsmData *fsmData = FSMDATA;

    switch (fsmData->reasonForDisconnect)
    {
        case DISCONNECT_DUE_TO_PEER_DEAUTHENTICATION:
            sme_trace_info((TR_PAL_LM_HIP_FSM,"deauthenticating__mlme_deletekeys_cfm(): Peer already deauthenticated.So disconnect locally"));
            send_pal_link_disconnect_ind(context,
                                         pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                         unifi_Success);
            break;

        case DISCONNECT_DUE_TO_KEY_FAILURE:
        {
            sme_trace_info((TR_PAL_LM_HIP_FSM,"deauthenticating__mlme_deletekeys_cfm(): disconnect reason - DISCONNECT_DUE_TO_KEY_FAILURE"));
        }
        break;

        default:
            sme_trace_crit((TR_PAL_LM_HIP_FSM,"deauthenticating__mlme_deletekeys_cfm(): no action on reason -%d",fsmData->reasonForDisconnect));
            verify(TR_PAL_LM_HIP_FSM, FALSE); /*lint !e774*/
            break;
    }

    if (fsmData->disconnectPending)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"deauthenticating__mlme_deletekeys_cfm(): disconnection pending. so complete it"));
        fsmData->disconnectPending = FALSE;

        send_pal_link_disconnect_cfm(context,
                                     pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                     unifi_Success);
    }
   hip_fsm_cleanup(context);
}

static void deauthenticating__pal_link_disconnect_req(FsmContext *context, const PalLinkDisconnectReq_Evt *req)
{
    HipFsmData *fsmData = FSMDATA;
    fsmData->disconnectPending = TRUE;
}

static void connected__mlme_deauthenticate_ind(FsmContext *context, const MlmeDeauthenticateInd_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;

#ifdef SME_TRACE_ENABLE
    stats.deauthFailedCount++;
#endif

    if (pal_security_enabled(context))
    {
        unifi_MACAddress remoteMacAddr = *pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);

        send_mlme_deletekeys_req(context, 0, KeyType_Pairwise,remoteMacAddr);
        fsmData->reasonForDisconnect = DISCONNECT_DUE_TO_PEER_DEAUTHENTICATION;
        fsm_next_state(context, FSMSTATE_deauthenticating);
    }
    else
    {
        send_pal_link_disconnect_ind(context,
                                     pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                     unifi_Success);
        hip_fsm_cleanup(context);
    }

    if (ind->informationElements.dataLength)
    {
        pld_release(getPldContext(context), ind->informationElements.slotNumber);
    }
    verify(TR_PAL_LM_HIP_FSM,ind->dummyDataRef2.dataLength==0);

}

static void connected__pal_link_disconnect_req(FsmContext *context, const PalLinkDisconnectReq_Evt *req)
{
    HipFsmData *fsmData = FSMDATA;

    if (pal_security_enabled(context))
    {
        unifi_MACAddress remoteMacAddr = *pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);

        fsmData->disconnectPending = TRUE;
        send_mlme_deletekeys_req(context, 0, KeyType_Pairwise, remoteMacAddr);
        fsm_next_state(context, FSMSTATE_deauthenticating);
    }
    else
    {
        send_pal_link_disconnect_cfm(context,
                             pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                             unifi_Success);
        hip_fsm_cleanup(context);
    }
}

static void connected__pal_coex_local_activity_report_ind(FsmContext *context, const PalCoexLocalActivityReportInd_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;
    unifi_MACAddress *peerStaAddress = pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);
    CsrUint16 dataLength;
    CsrUint8 *data;
    CsrUint32 hostTag = PAL_HIP_FSM_HOST_TAG_ID | (context->currentInstance->instanceId << 16);

    dataLength = PAL_MIN_ACTIVITY_REPORT_LEN+ind->activityReport.numReports*PAL_ACTIVITY_REPORT_TRIPLET_LEN;

    sme_trace_info((TR_PAL_LM_HIP_FSM,"connected__pal_coex_local_activity_report_ind() data len-%d",dataLength));

    data = CsrPmalloc(dataLength);
    if (encode_activity_report(&ind->activityReport, data) != dataLength)
    {
        verify(TR_PAL_LM_HIP_FSM, FALSE); /*lint !e774*/
    }

    pal_hip_send_ma_unitdata_req(context,
                                 (unifi_MACAddress *)peerStaAddress,
                                 pal_get_link_qos_support(context, fsmData->physicalLinkHandle),
                                 data,
                                 dataLength,
                                 PAL_DATA_PROTO_ACTIVITY_REPORT_ID,
                                 hostTag);
    if (ind->activityReport.numReports)
    {
        CsrPfree(ind->activityReport.arTriplet);
    }
    CsrPfree(data);
}

static void connected__pal_link_link_supervision_request_req(FsmContext *context, const PalLinkLinkSupervisionRequestReq_Evt *req)
{
    HipFsmData *fsmData = FSMDATA;
    unifi_MACAddress *peerStaAddress = pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);
    CsrUint32 hostTag = PAL_HIP_FSM_HOST_TAG_ID | (context->currentInstance->instanceId << 16);


    sme_trace_info((TR_PAL_LM_HIP_FSM,"connected__pal_link_link_supervision_request_req() "));
    pal_hip_send_ma_unitdata_req(context,
                                 (unifi_MACAddress *)peerStaAddress,
                                 pal_get_link_qos_support(context, fsmData->physicalLinkHandle),
                                 NULL,
                                 0,
                                 PAL_DATA_PROTO_LINK_SUPERVISION_REQUEST_ID,
                                 hostTag);
}

static void connected__ma_unitdata_ind(FsmContext *context, const UnifiSysMaUnitdataInd_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"connected__ma_unitdata_ind() len-%d, freeFunction-0x%x",ind->frameLength, ind->freeFunction));

    /* note: remote mac address is already verified in PAL manager for routing */
    if (0 == CsrMemCmp(pal_get_local_mac_address(context)->data, ind->frame, 6) &&
        ind->frameLength >= PAL_MIN_FRAME_SIZE)
    {
        pal_hip_llc_snap_hdr_t snap;

        pal_hip_decode_snap(PAL_GET_SNAP_PTR(ind->frame),&snap);
        if (PAL_DATA_PROTO_ACTIVITY_REPORT_ID == snap.protocol &&
            ind->frameLength >= PAL_MIN_AR_SIZE)
        {
            PalActivityReport activityReport;
            CsrUint8 *buf = PAL_GET_AR_PAYLOAD(ind->frame);

            CsrMemSet(&activityReport, 0, sizeof(PalActivityReport));
            activityReport.scheduleKnown = event_unpack_CsrUint8(&buf);
            activityReport.numReports = event_unpack_CsrUint8(&buf);
            if (activityReport.numReports)
            {
                /* this memory is freed at the receiver side when its finished with the processing */
                activityReport.arTriplet = CsrPmalloc(sizeof(PalActivityReportTriplet)*activityReport.numReports);
            }
            /* Discard any extra length. ensuring length atleast as big as numReports */
            if (ind->frameLength >= (PAL_MIN_AR_SIZE+(activityReport.numReports*PAL_ACTIVITY_REPORT_TRIPLET_LEN)))
            {
                int i=0;

                sme_trace_info((TR_PAL_LM_HIP_FSM,"connected__ma_unitdata_ind() number of activity reports %d",activityReport.numReports));

                for (i=0; i<activityReport.numReports;i++)
                {
                    activityReport.arTriplet[i].startTime=event_unpack_CsrUint32(&buf);
                    activityReport.arTriplet[i].duration=event_unpack_CsrUint32(&buf);
                    activityReport.arTriplet[i].periodicity=event_unpack_CsrUint32(&buf);

                    sme_trace_info((TR_PAL_LM_HIP_FSM,"connected__ma_unitdata_ind() Populating triplet in position-%d,startTime-%d,Duration-%d,Periodicity-%d",i,
                                    activityReport.arTriplet[i].startTime,
                                    activityReport.arTriplet[i].duration,
                                    activityReport.arTriplet[i].periodicity));

                }

                send_pal_coex_remote_activity_report_req(context,
                                                         getSmeContext(context)->palCoexFsmInstance,
                                                         fsmData->physicalLinkHandle,
                                                         activityReport);

            }
            else
            {
                sme_trace_warn((TR_PAL_LM_HIP_FSM,"connected__ma_unitdata_ind() invalid length for activity report triplet ar len-%d, numReports-%d",ind->frameLength-PAL_MIN_FRAME_SIZE, activityReport.numReports));
            }

        }
        else if (PAL_DATA_PROTO_LINK_SUPERVISION_RESPONSE_ID == snap.protocol)
        {
            sme_trace_info((TR_PAL_LM_HIP_FSM,"connected__ma_unitdata_ind() Link Supervision Response received"));
         }
        else if (PAL_DATA_PROTO_LINK_SUPERVISION_REQUEST_ID == snap.protocol)
        {
            unifi_MACAddress *peerStaAddress = pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);
            CsrUint32 hostTag = PAL_HIP_FSM_HOST_TAG_ID | (context->currentInstance->instanceId << 16);

            sme_trace_info((TR_PAL_LM_HIP_FSM,"connected__ma_unitdata_ind() Link Supervision Request received: Sending response"));
            pal_hip_send_ma_unitdata_req(context,
                                         (unifi_MACAddress *)peerStaAddress,
                                         pal_get_link_qos_support(context, fsmData->physicalLinkHandle),
                                         NULL,
                                         0,
                                         PAL_DATA_PROTO_LINK_SUPERVISION_RESPONSE_ID,
                                         hostTag);
        }

        /*consider any data packet on the control side to be indication of active link */
        send_pal_link_link_supervision_response_ind(context,
                        pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle));
    }

    if (ind->frame)
    {
        if (ind->freeFunction)
        {
            (*ind->freeFunction)(ind->frame);
        }
        else
        {
            CsrPfree(ind->frame);
        }
    }
}

static void security_handshake__ma_unitdata_ind(FsmContext *context, const UnifiSysMaUnitdataInd_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;
    unifi_MACAddress *remoteMacAddress = pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"security_handshake__ma_unitdata_ind() len-%d",ind->frameLength));

    if (0 == CsrMemCmp(remoteMacAddress->data, (ind->frame+6), 6) &&
        0 == CsrMemCmp(pal_get_local_mac_address(context)->data, ind->frame, 6) &&
        ind->frameLength > (PAL_HIP_SNAP_HEADER_SIZE+PAL_UNITDATA_MAC_ADDR_SIZE))
    {
        pal_hip_llc_snap_hdr_t snap;

        sme_trace_info((TR_PAL_LM_HIP_FSM,"security_handshake__ma_unitdata_ind(): Packet is meant for this device"));
        pal_hip_decode_snap(PAL_GET_SNAP_PTR(ind->frame), &snap);

        if (PAL_DATA_PROTO_SECURITY_FRAMES_ID == snap.protocol)
        {
            CsrUint8 *packet;
            sme_trace_info((TR_PAL_LM_HIP_FSM,"security_handshake__ma_unitdata_ind(): Valid Procotol-security: payldLen-%d",ind->frameLength));
            sme_trace_hex((TR_PAL_LM_HIP_FSM, TR_LVL_DEBUG, "security_handshake__ma_unitdata_ind()-message:",ind->frame, ind->frameLength ));

            packet = CsrPmalloc(ind->frameLength-PAL_UNITDATA_MAC_ADDR_SIZE);
            CsrMemCpy(packet, PAL_GET_SNAP_PTR(ind->frame), ind->frameLength-PAL_UNITDATA_MAC_ADDR_SIZE);
            CsrWifiSecurityProcessPacket(fsmData->securityContext,
                                       (CsrUint32)packet,
                                       packet,
                                       (CsrUint32)(ind->frameLength-PAL_UNITDATA_MAC_ADDR_SIZE));

#ifdef SME_TRACE_ENABLE
            if (FALSE==fsmData->firstHandshakeRecvd)
            {
                  fsmData->firstHandshakeRecvd = TRUE;
                  if (PalRole_PalRoleInitiator == fsmData->role)
                  {
                      stats.m2ReceivedCount++;
                  }
                  else
                  {
                      stats.m1ReceivedCount++;
                  }
            }
#endif

        }
    }

    if (ind->frame)
    {
        if (ind->freeFunction)
        {
            (*ind->freeFunction)((CsrUint8 *)ind->frame);
        }
        else
        {
            CsrPfree(ind->frame);
        }
    }
}

static void installing_keys__mlme_setkeys_cfm(FsmContext *context, const MlmeSetkeysCfm_Evt *cfm)
{
    HipFsmData *fsmData = FSMDATA;
    unifi_MACAddress remoteMacAddress = *pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);

    if (ResultCode_Success==cfm->resultCode)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"wait_for_setkeys__mlme_setkeys_cfm(): PTK setkey success: Set protection now"));

        if (fsmData->disconnectPending)
        {
            unifi_MACAddress remoteMacAddr = *pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);
            sme_trace_info((TR_PAL_LM_HIP_FSM,"wait_for_setkeys__mlme_setkeys_cfm(): disconnection pending. so trigger it"));

            send_pal_link_connect_cfm(context,
                                      pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                      unifi_Error, FALSE);

            send_mlme_deletekeys_req(context, 0, KeyType_Pairwise, remoteMacAddr);
            fsm_next_state(context, FSMSTATE_deauthenticating);
        }
        else
        {
            send_mlme_setprotection_req(context, remoteMacAddress, ProtectType_RxTx, KeyType_Pairwise);
        }
    }
    else
    {
        send_pal_link_connect_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                  unifi_Error, FALSE);
        if (fsmData->disconnectPending)
        {
            sme_trace_info((TR_PAL_LM_HIP_FSM,"wait_for_setkeys__mlme_setkeys_cfm(): disconnection pending. so trigger it"));
            fsmData->disconnectPending = FALSE;

            send_pal_link_disconnect_cfm(context,
                                         pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                         unifi_Success);
        }
        hip_fsm_cleanup(context);
    }
}

static void installing_keys__mlme_setprotection_cfm(FsmContext *context, const MlmeSetprotectionCfm_Evt *cfm)
{
    HipFsmData *fsmData = FSMDATA;

    if (ResultCode_Success==cfm->resultCode)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"wait_for_setkeys__mlme_setprotection_cfm(): PTK setprotection success: Security established"));

        if (fsmData->disconnectPending)
        {
            unifi_MACAddress remoteMacAddr = *pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);
            sme_trace_info((TR_PAL_LM_HIP_FSM,"wait_for_setkeys__mlme_setprotection_cfm(): disconnection pending. so trigger it"));

            send_pal_link_connect_cfm(context,
                                      pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                      unifi_Error, FALSE);

            send_mlme_deletekeys_req(context, 0, KeyType_Pairwise, remoteMacAddr);
            fsm_next_state(context, FSMSTATE_deauthenticating);
        }
        else
        {
#ifdef SME_TRACE_ENABLE
            stats.numConnectionsSucceeded++;
            /* overwrite results if exceeds the array size */
            stats.connectionResult[((CsrUint32)stats.numInstancesCreated%sizeof(stats.connectionResult))]=(CsrUint8)1;
#endif
            send_pal_link_connect_cfm(context,
                                      pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                      unifi_Success, FALSE);
            fsm_next_state(context, FSMSTATE_connected);
        }
    }
    else
    {
        unifi_MACAddress remoteMacAddr = *pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);

        send_mlme_deletekeys_req(context, 0, KeyType_Pairwise, remoteMacAddr);
        fsmData->reasonForDisconnect = DISCONNECT_DUE_TO_KEY_FAILURE;
        fsm_next_state(context, FSMSTATE_deauthenticating);
    }
}

static void installing_keys__pal_link_disconnect_req(FsmContext *context, const PalLinkDisconnectReq_Evt *req)
{
    HipFsmData *fsmData = FSMDATA;

#ifdef SME_TRACE_ENABLE
    stats.disconnectRequestedDuringHandshake++;
#endif

    fsmData->disconnectPending = TRUE;
}

static void security_handshake__pal_link_disconnect_req(FsmContext *context, const PalLinkDisconnectReq_Evt *req)
{
    HipFsmData *fsmData = FSMDATA;

#ifdef SME_TRACE_ENABLE
    stats.disconnectRequestedDuringHandshake++;
#endif

    if (pal_security_enabled(context))
    {
        PAL_STOP_TIMER(fsmData->securityHandshakeTimer);
    }

    /* FIXME: we may have to save it if we are waiting for MA-Unitdata-Status-Ind whenever its done */
    send_pal_link_disconnect_cfm(context,
                                 pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                 unifi_Success);
    hip_fsm_cleanup(context);
}

static void security_handshake__mlme_deauthenticate_ind(FsmContext *context, const MlmeDeauthenticateInd_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"security_handshake__mlme_deauthenticate_ind:"));

#ifdef SME_TRACE_ENABLE
    stats.deauthFailedCount++;
#endif

    if (pal_security_enabled(context))
    {
        PAL_STOP_TIMER(fsmData->securityHandshakeTimer);
    }
    send_pal_link_connect_cfm(context,
                              pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                              unifi_Error, FALSE);
    hip_fsm_cleanup(context);

    if (ind->informationElements.dataLength)
    {
        pld_release(getPldContext(context), ind->informationElements.slotNumber);
    }
    verify(TR_PAL_LM_HIP_FSM,ind->dummyDataRef2.dataLength==0);

}

static void security_handshake__pal_security_handshake_timer(FsmContext *context, const PalSecurityHandshakeTimer_Evt *timer)
{
    HipFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"security_handshake__pal_security_handshake_timer:"));

    fsmData->securityHandshakeTimer.id=0;
    CsrWifiSecurityTimerExpired(fsmData->securityContext, CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);
}

static void associating__mlme_associate_cfm(FsmContext *context, const MlmeAssociateCfm_Evt *cfm)
{
    HipFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"associating__mlme_associate_cfm: role-%d,result-%d",fsmData->role,cfm->resultCode));

    if (fsmData->disconnectPending)
    {
        fsmData->disconnectPending = FALSE;
        send_pal_link_connect_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                  unifi_Error, FALSE);
        send_pal_link_disconnect_cfm(context,
                                     pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                     unifi_Success);
        hip_fsm_cleanup(context);
    }
    else
    {
        if (PalRole_PalRoleResponder == fsmData->role)
        {
            if (ResultCode_Success == cfm->resultCode)
            {
                CsrBool localQosSupport = pal_get_link_qos_support(context, fsmData->physicalLinkHandle);
                CsrUint8 *result = pal_hip_ie_parameter_check(context, &cfm->informationElements,IE_ID_EDCA_PARAMS);

    #ifdef SME_TRACE_ENABLE
                stats.numPreHandshakeSucceeded++;
    #endif
                /* If Qos is supported  locally then, link will support qos if peer supports it.
                 * If Qos is not supported locally, then link will not support Qos
                 */
                localQosSupport = (localQosSupport)?(result?TRUE:FALSE):localQosSupport;
                pal_set_link_qos_support(context, fsmData->physicalLinkHandle, localQosSupport);

                if (pal_security_enabled(context))
                {
                    start_security_handshake(context);
                }
                else
                {
#ifdef SME_TRACE_ENABLE
                    stats.numConnectionsSucceeded++;
                    /* overwrite results if exceeds the array size */
                    stats.connectionResult[((CsrUint32)stats.numInstancesCreated%sizeof(stats.connectionResult))]=(CsrUint8)1;
#endif
                    send_pal_link_connect_cfm(context,
                                              pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                              unifi_Success, (result?TRUE:FALSE));
                    fsm_next_state(context, FSMSTATE_connected);
                }
            }
            else
            {
                send_pal_link_connect_cfm(context,
                                          pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                          unifi_Error, FALSE);
                hip_fsm_cleanup(context);
            }
        }
        else
        {
            sme_trace_warn((TR_PAL_LM_HIP_FSM,"Unexpected mlme_associate_cfm while PAL is a initiator"));
        }
    }

    if (cfm->informationElements.dataLength)
    {
        pld_release(getPldContext(context), cfm->informationElements.slotNumber);
    }
    if (cfm->exchangedFrames.dataLength)
    {
        pld_release(getPldContext(context), cfm->exchangedFrames.slotNumber);
    }
}

static void associating__mlme_associate_ind(FsmContext *context,const MlmeAssociateInd_Evt *ind)
{
    HipFsmData *fsmData = FSMDATA;
    unifi_MACAddress *address = pal_get_remote_mac_address(context, fsmData->physicalLinkHandle);

    sme_trace_entry((TR_PAL_LM_HIP_FSM,"associating__mlme_associate_ind: role-%d",fsmData->role));

    verify(TR_PAL_LM_HIP_FSM, address!=NULL);
    /* ignore the message if PAL is not initiator or remote address is unexpected */
    if (PalRole_PalRoleInitiator == fsmData->role &&
        !CsrMemCmp(ind->peerStaAddress.data, address->data, sizeof(ind->peerStaAddress.data)))
    {
        CsrUint8 *result = pal_hip_ie_parameter_check(context, &ind->informationElements,IE_ID_QOS_CAPABILITY);
        CsrBool peerQosSupported = result?TRUE:FALSE;
        CsrBool localQosSupport = pal_get_link_qos_support(context, fsmData->physicalLinkHandle);

        sme_trace_info((TR_PAL_LM_HIP_FSM,"associating__mlme_associate_ind():Everything is fine.  peerQosSupport-%d",peerQosSupported));

        if (pal_security_enabled(context))
        {
            result = pal_hip_ie_parameter_check(context, &ind->informationElements,IE_ID_RSN);
            if (result)
            {
                sme_trace_info((TR_PAL_LM_HIP_FSM,"mlme-assoc-ind:remote RSN IE present of length %d",result[1]));
                /* FIXME: Should we validate the RSN IE . May be the security handshake lib can do it ??*/
                pal_set_remote_rsn_ie(fsmData, result, result[1]+2);
            }
            else
            {
                sme_trace_info((TR_PAL_LM_HIP_FSM,"mlme-assoc-ind:remote RSN IE NOT present. Link will be disconnected!!"));
                pal_set_remote_rsn_ie(fsmData, NULL,0);
            }
        }
        pal_hip_send_mlme_associate_rsp(context,
                                        ResultCode_Success,
                                        (unifi_MACAddress *)address,
                                        (AssociationId)fsmData->physicalLinkHandle);
#ifdef SME_TRACE_ENABLE
        stats.numPreHandshakeSucceeded++;
#endif

        /* If Qos is supported  locally then, link will support qos if peer supports it.
                * If Qos is not supported locally, then link will not support Qos
                */
        localQosSupport = (localQosSupport)?peerQosSupported:localQosSupport;
        pal_set_link_qos_support(context, fsmData->physicalLinkHandle, localQosSupport);
        sme_trace_info((TR_PAL_LM_HIP_FSM,"associating__mlme_associate_ind(): localQosSupport-%d,peerQosSupported-%d,qosSupport-%d,handle-%d",
                        localQosSupport,peerQosSupported,pal_get_link_qos_support(context, fsmData->physicalLinkHandle),
                        fsmData->physicalLinkHandle));

        if (pal_security_enabled(context))
        {
            start_security_handshake(context);
        }
        else
        {
#ifdef SME_TRACE_ENABLE
            stats.numConnectionsSucceeded++;
            /* overwrite results if exceeds the array size */
            stats.connectionResult[((CsrUint32)stats.numInstancesCreated%sizeof(stats.connectionResult))]=(CsrUint8)1;
#endif
            send_pal_link_connect_cfm(context,
                                      pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                      unifi_Success, peerQosSupported);
            fsm_next_state(context, FSMSTATE_connected);
        }
    }
    else
    {
        sme_trace_warn((TR_PAL_LM_HIP_FSM,"Unexpected mlme_associate_cfm - invalid remote addr or unexpected as Responder"));
    }

    if (ind->informationElements.dataLength)
    {
        pld_release(getPldContext(context), ind->informationElements.slotNumber);
    }
    verify(TR_PAL_LM_HIP_FSM,ind->dummyDataRef2.dataLength==0);
}

static void associating__mlme_deauthenticate_ind(FsmContext *context, const MlmeDeauthenticateInd_Evt *ind)
{
    authenticating__mlme_deauthenticate_ind(context,ind);
}

static void associating__pal_link_disconnect_req(FsmContext *context, const PalLinkDisconnectReq_Evt *req)
{
    HipFsmData *fsmData = FSMDATA;

    /* Save disconnect request if FSM is the responder (Waiting for an mlme-associate-cfm).
     * Otherwise finish it off
     */
    if (PalRole_PalRoleInitiator == fsmData->role)
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM,"associating__pal_link_disconnect_req(): waiting for mlme-associate-ind. can be disconnected straight away"));

        send_pal_link_connect_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                  unifi_Error, FALSE);

        send_pal_link_disconnect_cfm(context,
                                     pal_get_link_fsm_pid_from_handle(context, fsmData->physicalLinkHandle),
                                     unifi_Success);
        hip_fsm_cleanup(context);
    }
    else
    {
        fsmData->disconnectPending = TRUE;
    }
}


#ifdef FSM_DEBUG_DUMP
static void lm_hip_fsm_dump(FsmContext* context, const CsrUint16 id)
{
    HipFsmData *fsmData = fsm_get_params_by_id(context, id, HipFsmData);
    int i;

    sme_trace_crit((TR_FSM_DUMP,"======LM HIP FSM (%s) STATISTICS DUMP START========",PalRole_PalRoleInitiator==fsmData->role?"Initiator":"Acceptor"));
    sme_trace_crit((TR_FSM_DUMP,"Num HIP FSM Instances created: %d", stats.numInstancesCreated));
    sme_trace_crit((TR_FSM_DUMP,"Connection outcome count: numTotalSuccess-%d, numPrehandshakeSuccess(authenticate&associate)-%d", stats.numConnectionsSucceeded,stats.numPreHandshakeSucceeded));
    sme_trace_crit((TR_FSM_DUMP,"Connection outcome count: numTotalFailures-%d, numFailureDuringHanshake-%d", stats.numConnectionsFailed,stats.numHandshakeFailed));
    sme_trace_crit((TR_FSM_DUMP,"Probe Retry Stats: Total-%d,Pass-%d,Fail-%d", stats.totalRetryCount, stats.retryFailedCount, stats.retryPassedCount));
    sme_trace_crit((TR_FSM_DUMP,"Deauthentication count from peer: %d", stats.deauthFailedCount));
    sme_trace_crit((TR_FSM_DUMP,"LM FSM Disconnect requested count during handshake: %d", stats.disconnectRequestedDuringHandshake));
    sme_trace_crit((TR_FSM_DUMP,"Security handshake first Message recieved count: Message1-%d, Message2-%d", stats.m1ReceivedCount,stats.m2ReceivedCount));
    sme_trace_crit((TR_FSM_DUMP,"MLME-Connected-Ind in CONNECTED state:total-%d, Connection Status as: disconnected count-%d, connected count-%d", stats.connectedIndCount, stats.connectedIndCount_disconnected, stats.connectedIndCount_connected));

    for (i=0;i<stats.numInstancesCreated; i++)
    {
         sme_trace_crit((TR_FSM_DUMP,"Run-%d => %s",i+1, stats.connectionResult[i]?"PASS":"FAIL"));
    }
    sme_trace_crit((TR_FSM_DUMP,"======LM HIP FSM STATISTICS DUMP END========"));

}
#endif

static void lm_hip_fsm_reset(FsmContext* context)
{
#ifdef FSM_DEBUG_DUMP
    lm_hip_fsm_dump(context,0);
#endif
}

/* FSM DEFINITION **********************************************/

/** State joining transitions */

/** State joining transitions */
static const FsmEventEntry disconnectedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PAL_LINK_STOP_REQ_ID,                     disconnected__pal_link_stop_req),
    fsm_event_table_entry(PAL_LINK_CONNECT_REQ_ID,                  disconnected__pal_link_connect_req),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_REQ_ID,               disconnected__pal_link_disconnect_req),
};

/** State idle transitions */
static const FsmEventEntry scanningTransitions[] =
{
    /* Signal Id,                                                 Function */

    fsm_event_table_entry(SM_SCAN_CFM_ID,                    scanning__mlme_scan_cfm),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_REQ_ID,        scanning__pal_link_disconnect_req),
};

/** State waitForAmpAssoc transitions */
static const FsmEventEntry associatingTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_ASSOCIATE_CFM_ID,                  associating__mlme_associate_cfm),
    fsm_event_table_entry(MLME_ASSOCIATE_IND_ID,                  associating__mlme_associate_ind),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,             associating__mlme_deauthenticate_ind),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_REQ_ID,             associating__pal_link_disconnect_req),
};

static const FsmEventEntry authenticatingTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_AUTHENTICATE_CFM_ID,                  authenticating__mlme_authenticate_cfm),
    fsm_event_table_entry(MLME_AUTHENTICATE_IND_ID,                  authenticating__mlme_authenticate_ind),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,                authenticating__mlme_deauthenticate_ind),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_REQ_ID,                authenticating__pal_link_disconnect_req),
};

static const FsmEventEntry deauthenticatingTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_DEAUTHENTICATE_CFM_ID,                  deauthenticating__mlme_deauthenticate_cfm),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,                  deauthenticating__mlme_deauthenticate_ind),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_REQ_ID,                  deauthenticating__pal_link_disconnect_req),
    fsm_event_table_entry(MLME_DELETEKEYS_CFM_ID,                      deauthenticating__mlme_deletekeys_cfm),
};

static const FsmEventEntry securityHandshakeTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_IND_ID,            security_handshake__ma_unitdata_ind),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_REQ_ID,              security_handshake__pal_link_disconnect_req),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,              security_handshake__mlme_deauthenticate_ind),
    fsm_event_table_entry(PAL_SECURITY_HANDSHAKE_TIMER_ID,         security_handshake__pal_security_handshake_timer),
};

static const FsmEventEntry installingKeysTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_SETKEYS_CFM_ID,                    installing_keys__mlme_setkeys_cfm),
    fsm_event_table_entry(MLME_SETPROTECTION_CFM_ID,              installing_keys__mlme_setprotection_cfm),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_REQ_ID,             installing_keys__pal_link_disconnect_req),
};

/** State connected transitions */
static const FsmEventEntry connectedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,                  connected__mlme_deauthenticate_ind),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_REQ_ID,                  connected__pal_link_disconnect_req),
    fsm_event_table_entry(PAL_COEX_LOCAL_ACTIVITY_REPORT_IND_ID,       connected__pal_coex_local_activity_report_ind),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_IND_ID,                connected__ma_unitdata_ind),
    fsm_event_table_entry(PAL_LINK_LINK_SUPERVISION_REQUEST_REQ_ID,    connected__pal_link_link_supervision_request_req),
};


/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_ASSOCIATE_IND_ID,                     fsm_ignore_event),
    fsm_event_table_entry(MLME_ASSOCIATE_CFM_ID,                     fsm_ignore_event),
    fsm_event_table_entry(MLME_AUTHENTICATE_IND_ID,                  fsm_ignore_event),
    fsm_event_table_entry(MLME_AUTHENTICATE_CFM_ID,                  fsm_ignore_event),
    fsm_event_table_entry(PAL_COEX_LOCAL_ACTIVITY_REPORT_IND_ID,     fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_CFM_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_EAPOL_CFM_ID,                    fsm_ignore_event),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                     fsm_ignore_event),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,                fsm_ignore_event),
};

/** Profile Storage state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                       State                   State                               Save     */
   /*                       Name                    Transitions                          *       */
   fsm_state_table_entry(FSMSTATE_disconnected, disconnectedTransitions,   FALSE),
   fsm_state_table_entry(FSMSTATE_scanning, scanningTransitions,   TRUE),
   fsm_state_table_entry(FSMSTATE_associating, associatingTransitions,   FALSE),
   fsm_state_table_entry(FSMSTATE_authenticating, authenticatingTransitions,   FALSE),
   fsm_state_table_entry(FSMSTATE_deauthenticating, deauthenticatingTransitions,   FALSE),
   fsm_state_table_entry(FSMSTATE_security_handshake, securityHandshakeTransitions,  FALSE),
   fsm_state_table_entry(FSMSTATE_installing_keys,  installingKeysTransitions,  TRUE),
   fsm_state_table_entry(FSMSTATE_connected,   connectedTransitions,                   FALSE),
};

const FsmProcessStateMachine pal_lm_hip_fsm =
{
#ifdef FSM_DEBUG
       "PAL LM Hip",                                  /* Process Name       */
#endif
       PAL_LM_HIP_PROCESS,                                /* Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                         /* Transition Tables  */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE),   /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                    /* ignore event handers */
       pal_lm_hip_init,                                    /* Entry Function     */
       lm_hip_fsm_reset,                                                               /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       lm_hip_fsm_dump                                                             /* Trace Dump Function   */
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
