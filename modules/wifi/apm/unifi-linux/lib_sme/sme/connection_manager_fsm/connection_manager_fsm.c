/** @file connection_manager_fsm.c
 *
 * Connection Manager FSM Implementation
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
 *   The Connection Manager FSM is tasked with starting an IEEE 802.11
 *   wireless connection, monitoring it and shutting it down when it fails
 *   or is no longer required.
 *
 *   The Connection Manager FSM is dynamically created by the Network
 *   Selector FSM when it is required, and it is ultimately shut down and
 *   terminated by Network Selector.  Its lifetime does not extend beyond
 *   one layer 2 IEEE 802.11 connection.  Note that even an intra-ESS roam,
 *   organised by the Network Selector, will result in one Connection
 *   Manager terminating and a new one being started.
 *
 *   The Connection Manager FSM dynamically creates one Security Manager FSM
 *   to handle the security aspects of the connection, even if the network
 *   is open system.  The lifetime of the Security Manager FSM is shorter
 *   than that of the Connection Manager FSM, which terminates it.
 *
 *   The connection sequence is quite straightforward from the Connection
 *   Manager FSM's point of view.  Complications arise because unexpected
 *   messages can arrive from the Network Selector (detach_req), the UniFi MLME
 *   (deauth_ind, connection_ind) and the Security Manager FSM
 *   (security_disconnected_ind) at any time.  These are handled either
 *   immediately, or set a flag to abort procedings at the next convenient
 *   moment, or are ignored.
 *
 *   The Connection Manager FSM expects the UniFi to be in a sensible state
 *   when it starts up, and sends an MLME_RESET_REQ during the shutdown
 *   sequence to restore it to a known, low power state.
 *
 *   The Connection Manager FSM does not configure the Scan Manager FSM,
 *   Power Manager FSM or IP Connection Manager FSM, all of which are
 *   handled at a higher level in the Network Selector.
 *
 *   The Connection Manager FSM is intended to be a simple beast with little
 *   intelligence.  It has one task to do, and requires no areas of
 *   sophistication in order to be able to do it.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/connection_manager_fsm/connection_manager_fsm.c#11 $
 *
 ****************************************************************************/

/** @{
 * @ingroup sme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "sme_configuration/sme_configuration_fsm.h"
#include "connection_manager_fsm/connection_manager_fsm.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "ap_utils/ap_validation.h"
#include "security_manager_fsm/security_manager_fsm.h"
#include "hip_proxy_fsm/hip_signal_proxy_fsm.h"
#include "scan_manager_fsm/scan_manager_fsm.h"

#include "scan_manager_fsm/scan_results_storage.h"

#include "ie_access/ie_access.h"
#include "ie_message_handling/ie_access_associate_req.h"
/*#include "ie_message_handling/ie_access_associate_cfm.h"
 */
#include "qos_fsm/qos_fsm.h"

#include "sys_sap/sys_sap_from_sme_interface.h"
#include "mgt_sap/mgt_sap_from_sme_interface.h"

#include "smeio/smeio_trace_types.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, FsmData))

/**
 * @brief
 *   Time units to wait on a channel for adhoc connection where we have old scan data
 */
#define ADHOC_PROBE_SCAN_CHANNEL_TIME_TU 1000

/**
 * @brief
 *   Information Elements needed for starting an ad-hoc network
 */
#define ADHOC_BASIC_DATA_RATES           0x01, 0x08, 0x82, 0x84, 0x0b, 0x0c, 0x12, 0x16, 0x18, 0x24
#define ADHOC_EXTENDED_DATA_RATES        0x32, 0x04, 0x30, 0x48, 0x60, 0x6c
#define ADHOC_ATIM_WINDOW                0x06, 0x02, 0x00, 0x00


/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/**
 * @brief
 *   FSM States
 *
 * @par Description
 *   Enum declaring the FSM States for this FSM process
 */
typedef enum FsmState
{
    /** Started and waiting for the join request to start connection */
    FSMSTATE_waitingForNSJoin,
    /** Waiting for probe scan for the requested network */
    FSMSTATE_waitingForActiveScanProbe,
    /** Network found or Adhoc hosting; wait for security info */
    FSMSTATE_waitingForSecurityJoinStart,
    /** Waiting for MLME to send an mlme_join_cfm signal */
    FSMSTATE_waitingForMlmeJoin,
    /** Waiting for MLME to start an adhoc network */
    FSMSTATE_waitingForMlmeStart,
    /** Waiting for MLME to confirm it has accepted our group key */
    FSMSTATE_waitingForGroupMlmeSetProtection,
    /** Waiting for MLME to confirm it has accepted our pairwise key */
    FSMSTATE_waitingForPairwiseMlmeSetProtection,
    /** Waiting for MLME to confirm workaround authentication with the network being joined */
    FSMSTATE_waitingForJoinMlmeDeauthenticate,
    /** Waiting for MLME to confirm authentication with the network */
    FSMSTATE_waitingForMlmeAuthenticate,
    /** Waiting for MLME to confirm DD QoS Control */
    FSMSTATE_waitingForUnifiSysQoSControl,
    /** Waiting for MLME to confirm association with the network */
    FSMSTATE_waitingForMlmeAssociate,
    /** Waiting for Security Manager to perform layer 2 security (4-way h-shake) */
    FSMSTATE_waitingForSecurityConnectStart,
    /** 802.11 layer 2 connection established */
    FSMSTATE_connected,
    /** Tearing down a connection - waiting for Sec Mgr to cleanup & go idle */
    FSMSTATE_waitingForSecurityConnectStop,
    /** Waiting for MLME to confirm deauthentication from the network */
    FSMSTATE_waitingForMlmeDeauthenticate,
    /** Waiting for UniFi to confirm that it has executed a soft reset */
    FSMSTATE_waitingForUnifiReset,
    /** Waiting for a terminate signal from Network Selector */
    FSMSTATE_waitingForNsTerminate,
    /** Placeholder to mark end of state declarations */
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
    /** The currentlyAuthenticated flag ensures that we deauthenticate from
     * the network on shutdown if we got as far as authenticating to it. */
    CsrBool                 currentlyAuthenticated;

    /** The hosting flag changes the connection sequnence to host an adhoc network. */
    CsrBool                 hostAdhoc;

    /** Fast cleardown Avoid the mlme_deauth and mlme_reset if the connection fails. */
    CsrBool                 skipResetOnError;

    /** Use mlme_reassociate_req NOT mlme_associate_req. */
    CsrBool                 reassociate;

    /** DataRef containing IEs neeed for joining networks or starting IBSS */
    DataReference           joinOrStartIeDR;

    /** Beacon period reported by the network or to be used in hosting adhoc */
    CsrUint16                  beaconPeriodTu;

    /** Calculated Listen Interval in Beacons */
    CsrUint16                  listenIntervalBeacons;

    /** Security Manager identifier, set at creation, used for passing
     * signals and termination. */
    CsrUint16                  securityManagerInstance;

    /** Information Elements from Security Manager for use in MLME_ASSOCIATE_REQ */
    DataReference           securityIeDataRef;

    /** Counter to limit automatic join retries */
    CsrUint8                   joinRetry;
    /** Counter to limit automatic authentication retries */
    CsrUint8                   authRetry;
    /** Counter to limit automatic association retries */
    CsrUint8                   assocRetry;

    /** One NSJoinCfm is needed per connection */
    CsrBool                 outstandingNSJoinCfm;

    /** A connection may be the first join or due to a roam */
    CsrUint8                   connectionReason;

    /** A disconnection may be final or due to a roam */
    CsrUint8                   disconnectionReason;

    CsrBool activeProbeRsp_CountryIE_valid;

    /** authtype (initially from sme configuration) but then changes when we do shared followed by open */
    AuthenticationType         chosenAuthenticationType;

} FsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/** The reason the current connection is being closed */
#define DISCONNECTION_NONE                 (0x00)
#define DISCONNECTION_DETACH               (0x01)
#define DISCONNECTION_ROAM                 (0x02)
#define DISCONNECTION_ERROR                (0x04)

/* The instigator of the new connection */
#define CONNECTION_JOIN                    (0x01)
#define CONNECTION_ROAM                    (0x02)

#define AUTHENTICATION_FAILURE_TIMEOUT_MS   (200)
#define MINIMUM_SENSIBLE_BEACON_PERIOD_MS    (20)
#define MAXIMUM_SENSIBLE_BEACON_PERIOD_MS   (500)
#define HOSTING_ADHOC_PROBE_DELAY_US        (500)
#define JOIN_FAILURE_TIMEOUT_MS            (1000)
#define JOIN_PROBE_DELAY_US                 (500)
#define ASSOCIATE_FAILURE_TIMEOUT_MS        (200)

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/
static void     reinitialise_connection_data(FsmContext* context);
static void    initiate_joining_probe_scan(FsmContext* context);
static CsrBool initiate_host_adhoc(FsmContext* context);

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Connection Manager FSM initialisation
 *
 * @par Description
 *   This is the entry function for this FSM.  It initialises FSM data.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void init_connection_mgr_Fsm(FsmContext* context)
{
    sme_trace_entry((TR_CXN_MGR, "init_connection_mgr_Fsm()"));

    /* Register this instance of the connection manager with the hip proxy */
    hip_proxy_register_connection_manager(context, context->currentInstance->instanceId);

    fsm_create_params(context, FsmData);

    /* initialise this first time round */
    FSMDATA->securityIeDataRef.dataLength = 0;
    FSMDATA->joinOrStartIeDR.dataLength = 0;

    reinitialise_connection_data(context);

    fsm_next_state(context, FSMSTATE_waitingForNSJoin);
}

/**
 * @brief
 *   Connection Manager FSM cleardown prior to roam
 *
 * @par Description
 *   This function resets some data between the closedown of the previous
 *   access point and connection to the next one, as part of an intra-ESS
 *   roam.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void reinitialise_connection_data(FsmContext* context)
{
    SmeConfigData* cfg = get_sme_config(context);
    FsmData* fsmData;

    sme_trace_entry((TR_CXN_MGR, "reinitialise_connection_data"));

    fsmData = FSMDATA;

    fsmData->securityManagerInstance      = FSM_TERMINATE;
    fsmData->joinRetry                    = 0;
    fsmData->authRetry                    = 0;
    fsmData->assocRetry                   = 0;

    /* Prime for one NSJoinCfm needed per connection */
    fsmData->outstandingNSJoinCfm         = TRUE;
    fsmData->disconnectionReason          = DISCONNECTION_NONE;
    fsmData->currentlyAuthenticated       = FALSE;
    fsmData->hostAdhoc                    = FALSE;
    fsmData->beaconPeriodTu               = 100;
    fsmData->activeProbeRsp_CountryIE_valid = FALSE;

    cfg->disassocReason = unifi_IEEE80211ReasonSuccess;
    cfg->deauthReason = unifi_IEEE80211ReasonSuccess;

    /* we no longer have ownership */
    /* Clean up the previous Associations info */
    if (fsmData->securityIeDataRef.dataLength != 0)
    {
        pld_release(getPldContext(context), fsmData->securityIeDataRef.slotNumber);
        fsmData->securityIeDataRef.dataLength = 0;
    }

    if (cfg->assocReqIeDataRef.dataLength != 0)
    {
        pld_release(getPldContext(context), cfg->assocReqIeDataRef.slotNumber);
        cfg->assocReqIeDataRef.dataLength = 0;
    }

    /* Clean up the previous Associations info */
    if (cfg->connectionBeaconFrameDataRef.dataLength != 0)
    {
        pld_release(getPldContext(context), cfg->connectionBeaconFrameDataRef.slotNumber);
    }
    cfg->connectionBeaconFrameDataRef.dataLength = 0;
    cfg->connectionInfo.beaconFrameLength = 0;
    cfg->connectionInfo.beaconFrame = NULL;
    cfg->connectionInfo.assocScanInfoElementsLength = 0;
    cfg->connectionInfo.assocScanInfoElements = NULL;

    if (cfg->connectionExchangedFramesDataRef.dataLength != 0)
    {
        pld_release(getPldContext(context), cfg->connectionExchangedFramesDataRef.slotNumber);
    }
    cfg->connectionExchangedFramesDataRef.dataLength = 0;
    cfg->connectionInfo.associationReqFrameLength = 0;
    cfg->connectionInfo.associationReqFrame = NULL;
    cfg->connectionInfo.associationRspFrameLength = 0;
    cfg->connectionInfo.associationRspFrame = NULL;

    cfg->connectionInfo.assocReqInfoElementsLength = 0;
    cfg->connectionInfo.assocReqInfoElements = NULL;
    cfg->connectionInfo.assocRspInfoElementsLength = 0;
    cfg->connectionInfo.assocRspInfoElements = NULL;

    if (fsmData->joinOrStartIeDR.dataLength != 0)
    {
        pld_release(getPldContext(context), fsmData->joinOrStartIeDR.slotNumber);
        fsmData->joinOrStartIeDR.dataLength = 0;
    }
}

/* ====================[ RUNTIME UTITILITY FUNCTIONS ]==================== */

/**
 * @brief
 *   Send a MlmeAuthenticate request to UniFi
 *
 * @par Description
 *     The scan result received during the initial probe scan will have
 *     informed the security manager of the security used by the network
 *     we're attempting to join.  If needed, Group and Pairwise protection
 *     will already have been set earlier in this connection sequence.  All
 *     that remains now is to send an MLME_AUTHENTICATE_REQ to UniFi.
 *
 * @param[in]    context           : FSM context
 * @param[in]    authType          : AuthenticationType
 *
 * @return
 *   void
 */
static void do_auth_req(FsmContext* context, AuthenticationType authType )
{
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) AUTHENTICATING (type 0x%.4X) ",
        trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
        cfg->connectionConfig.authModeMask));

    send_mlme_authenticate_req(context,
                               NullDataReference,
                               cfg->connectionInfo.bssid,
                               authType,
                               AUTHENTICATION_FAILURE_TIMEOUT_MS);
}

/**
 * @brief
 *   Send a MlmeDeauthenticate request to UniFi
 *
 * @par Description
 *   We are currently authenticated, and want to shut down, so need to
 *   formally deauthenticate so that the access point / network can
 *   immediately release resources.
 *
 * @param[in]    context           : FSM context
 *
 * @return
 *   void
 */
static void do_deauth_req(FsmContext* context)
{
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) DEAUTHENTICATING",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    send_mlme_deauthenticate_req(context, NullDataReference,
                                cfg->connectionInfo.bssid,
                                ReasonCode_UnspecifiedReason );
    fsm_next_state(context, FSMSTATE_waitingForMlmeDeauthenticate);
}

/**
 * @brief
 *   Send a MlmeAssociate or MlmeReassociate request to UniFi
 *
 * @param[in]    context           : FSM context
 *
 * @return
 *   void
 */
static void do_assoc_req(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_CXN_MGR, "do_assoc_req(%s : %s)",
            trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
            trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    /* If there is data in the assocReqIeDataRef, increment the reference
     * count so that both this process and the MLME have to finish with it
     * before it disappears.  We cannot just hand it over as we may need it
     * again for a retry.
     */
    if (cfg->assocReqIeDataRef.dataLength != 0)
    {
        pld_add_ref(getPldContext(context), cfg->assocReqIeDataRef.slotNumber);
    }

    /* this is a new connection clear the WMM Flag */
    cfg->WMMAssociation = FALSE;

    if (!fsmData->reassociate)
    {
        cfg->connectionInfo.reassociation = FALSE;
        send_mlme_associate_req(context, cfg->assocReqIeDataRef,
                                cfg->connectionInfo.bssid,
                                ASSOCIATE_FAILURE_TIMEOUT_MS,
                                cfg->connectionInfo.assocReqCapabilities,
                                fsmData->listenIntervalBeacons);
    }
    else
    {
        cfg->connectionInfo.reassociation = TRUE;
        send_mlme_reassociate_req(context, cfg->assocReqIeDataRef,
                                cfg->connectionInfo.bssid,
                                ASSOCIATE_FAILURE_TIMEOUT_MS,
                                cfg->connectionInfo.assocReqCapabilities,
                                fsmData->listenIntervalBeacons);
    }
}

/**
 * @brief
 *   Reset UniFi so that it is in a known state again
 *
 * @par Description
 *   Regardless of what has happened earlier, the quickest way to get UniFi
 *   into a known state is to ask for it to be soft-reset.
 *
 * @param[in] context : FSM context
 *
 * @return
 *  Void
 */
static void reset_unifi(FsmContext *context)
{
    sme_trace_entry((TR_CXN_MGR, "reset_unifi"));

    /* Soft reset the UniFi so it is in a known state */
    send_unifi_reset_req(context, getSmeContext(context)->unifiDriverInstance);

    /* Wait for the reset request to respond */
    fsm_next_state(context, FSMSTATE_waitingForUnifiReset);
}

/**
 * @brief
 *   UniFi Reset Confirm has arrived
 *
 * @par Description
 *   UniFi has responded to the RESET_REQ sent earlier, so the shutdown can
 *   continue as appropriate.
 * @par
 *   The connection manager will need to issue a confirmation signal to the network
 *   selector to tell it the connection is shut down.  In the case of a
 *   setup error it will be a join_cfm with failure status.  For any other
 *   failure (or no failure), the Network Selector would have issued a
 *   detach request so a detach_cfm must be sent in reply.
 *
 * @param[in] context : FSM context
 *
 * @return
 *  Void
 */
static void waiting_for_unifi_reset__unifi_reset_cfm(FsmContext *context, const UnifiResetCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    unifi_ScanConfigData* roamScanData;

    sme_trace_entry((TR_CXN_MGR, "waiting_for_unifi_reset__unifi_reset_cfm"));

    if (fsmData->outstandingNSJoinCfm == TRUE)
    {
        /* We never reached the connected state so send a join_cfm with the
         * failure status.
         */
        sme_trace_debug((TR_CXN_MGR, "complete_wait_for_terminate() :: never connected: %s", trace_unifi_Status(unifi_Error)));

        if (fsmData->connectionReason == CONNECTION_JOIN)
        {
            send_network_selector_join_cfm(context, getSmeContext(context)->networkSelectorInstance, unifi_Error);
        }
        else
        {
            send_network_selector_roam_cfm(context, getSmeContext(context)->networkSelectorInstance, unifi_Error);
        }

        fsmData->outstandingNSJoinCfm = FALSE;
    }

    if (fsmData->disconnectionReason == DISCONNECTION_DETACH)
    {
        sme_trace_debug((TR_CXN_MGR, "complete_wait_for_terminate() :: sending detach confirm"));

        send_network_selector_detach_cfm(context, getSmeContext(context)->networkSelectorInstance);
        fsmData->disconnectionReason = DISCONNECTION_NONE;
    }
    /* The scan result for the connected AP may be expired check and expire if needed */
    srs_check_for_expiry(context, &cfg->connectionInfo.bssid, get_current_scan_data(context, &roamScanData)->intervalSeconds);

    /* Wait to be terminated */
    fsm_next_state(context, FSMSTATE_waitingForNsTerminate);
}

/**
 * @brief Kicks off security manager tear down
 *
 * @param[in]    context           : FSM context
 *
 * @return
 *   void
 */
static void shutdown_security_manager(FsmContext* context)
{
    sme_trace_info_code(SmeConfigData* cfg = get_sme_config(context);)
    sme_trace_debug((TR_CXN_MGR, "%-25s: shutdown_security_manager()", fsm_current_state_name(context)));

    /* Close the control port as we are shutting down the connection */
    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) CONTROLLED PORT CLOSED",
            trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
            trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));
    (void)ns_port_configure(context, unifi_8021xPortClosedDiscard, unifi_8021xPortClosedDiscard);

    /* The Security Manager instance needs to be shut down.*/
    send_security_connect_stop_req(context, FSMDATA->securityManagerInstance);
    fsm_next_state(context, FSMSTATE_waitingForSecurityConnectStop);
}

/**
 * @brief
 *   Checks the status and reason codes for an error and
 *   initiates the security shutdown if either fails
 *
 * @param[in]    context           : FSM context
 * @param[in]    status            : Status code
 * @param[in]    reason            : Reason Code
 *
 * @return
 *   void
 */
static CsrBool there_were_errors_or_a_ns_detach_req(FsmContext* context, unifi_Status status, ResultCode result)
{
    FsmData* fsmData = FSMDATA;
    if ((status != unifi_Success) ||
        (result != ResultCode_Success) ||
        (fsmData->disconnectionReason == DISCONNECTION_DETACH))
    {
        /* The status or code were not good, or a
         * network_selector_detach_req was received.
         */
        sme_trace_debug((TR_CXN_MGR, "%-25s: there_were_errors_or_a_ns_detach_req(%s, %s), disconnectionReason %d",
            fsm_current_state_name(context),
            trace_unifi_Status(status), trace_ResultCode(result),
            fsmData->disconnectionReason));

        if (fsmData->disconnectionReason != DISCONNECTION_DETACH)
        {
            fsmData->disconnectionReason = DISCONNECTION_ERROR;
        }

        shutdown_security_manager(context);
        return(TRUE);
    }

    return(FALSE);
}

/* ===========[ GENERIC HANDLER (STATE INDEPENDENT) FUNCTIONS ]=========== */

/**
 * @brief
 *   Sets the disconnectionReason flag when requested to detach
 *
 * @par Description
 *   The Network Selector has sent a detach_req.  As this generic handler is
 *   being called, the FSM is currenly waiting for something else to
 *   complete.
 *
 *   The abort status variables are set and the disconnectionReason flag is
 *   set.  The flag should be checked when the FSM is next able to do so.
 *
 * @param[in]    context           : FSM context
 * @param[in]    req               : event
 *
 * @return
 *   void
 */
static void set_detach_flag(FsmContext *context, const NetworkSelectorDetachReq_Evt * req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_info((TR_CXN_MGR, "%-25s: caught NS detach request from %s",
                    fsm_current_state_name(context), fsm_process_name_by_id(context, req->common.sender_)));

    fsmData->disconnectionReason = DISCONNECTION_DETACH;
}

/**
 * @brief
 *   While connecting, an unexpected event came from MLME.
 *
 * @par Description
 *   While connecting, an unexpected event came from MLME.  We assume that
 *   any error indicated by this event will be echoed in the confirm message
 *   for which we are actually waiting.  Therefore, we warn about it, but do
 *   not take any action.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void unexpected_event_indication(FsmContext *context, const FsmEvent *ind)
{
    sme_trace_info_code(SmeConfigData* cfg = get_sme_config(context);)
    sme_trace_warn((TR_CXN_MGR, "\"%s\" (%s) Unexpected event id %#x while in state %s",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                    ind->id, fsm_current_state_name(context)));
}

/* =================[ WAITING FOR NS JOIN STATE EVENT HANDLERS ]================ */

/**
 * @brief
 *   A Join Request has arrived to start the IEEE 802.11 connection process
 *
 * @par Description
 *   A network_selector_join_req has been received.  Start the process to
 *   connect to the requested network.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void waiting_for_ns_join__ns_join_req(FsmContext* context, const NetworkSelectorJoinReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_CXN_MGR,"waiting_for_ns_join__ns_join_req"));

    fsmData->reassociate = req->reassociate;
    fsmData->hostAdhoc = req->hostAdhoc;
    fsmData->skipResetOnError = req->skipResetOnError;

    fsmData->connectionReason = CONNECTION_JOIN;

    if (req->hostAdhoc)
    {
        if (!initiate_host_adhoc(context))
        {
            sme_trace_warn((TR_CXN_MGR, "Unable to start ad-hoc network"));
            send_network_selector_join_cfm(context, getSmeContext(context)->networkSelectorInstance, unifi_InvalidParameter);
            fsm_next_state(context, FSMSTATE_waitingForNsTerminate);
        }
    }
    else
    {
        initiate_joining_probe_scan(context);
    }
}

/**
 * @brief
 *   A Roam request requires the destination connection to be brought up
 *
 * @par Description
 *   A network_selector_roam_req was been received which has already caused
 *   the existing connection to be dropped.  Start the process to
 *   connect to the next network.
 *
 * @param[in]    context   : FSM context
 * @param[in]    roamData  : The AP to join next
 *
 * @return
 *   void
 */
static void ns_roam_req(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_CXN_MGR,"ns_roam_req"));

    fsmData->connectionReason = CONNECTION_ROAM;
    initiate_joining_probe_scan(context);
}


/**
 * @brief
 *   Send initial probe request for the new network
 *
 * @par Description
 *   A connection to a new network has been requested.  The first step is to
 *   send an active probe request to ensure the network still exists.
 *
 *   NOTE: FSMDATA->joinData must be set up with the details of the target
 *   network before this function is called.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void initiate_joining_probe_scan(FsmContext* context)
{
    BssType bssType = BssType_AnyBss;
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint32 now = fsm_get_time_of_day_ms(context);
	unifi_SSID* ssid = NULL;

    sme_trace_entry((TR_CXN_MGR, "initiate_joining_probe_scan"));

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) PROBE SCAN Channel %d on %s band",
                                trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                                trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                                cfg->connectionInfo.channelNumber,
                                trace_unifi_RadioIF(unifi_GHZ_2_4)));

    /* Clear the current Networks scan Data */
    cfg->connectionInfo.assocScanInfoElementsLength = 0;
    cfg->connectionInfo.assocScanInfoElements = NULL;

    if (cfg->connectionConfig.bssType != unifi_Infrastructure)
    {
        bssType = BssType_Independent;
    }

    /* Take a copy of the SSID */
    if (cfg->connectionInfo.ssid.length)
    {
    	ssid = (unifi_SSID*)CsrPmalloc(sizeof(unifi_SSID));
    	ssid->length = cfg->connectionInfo.ssid.length;
    	CsrMemCpy(ssid->ssid, cfg->connectionInfo.ssid.ssid, ssid->length);
    }

    /* cancel any other Scans as this takes precedence */
    send_sm_scan_cancel_ind(context, getSmeContext(context)->scanManagerInstance, now, TRUE);
    send_sm_scan_req(context, getSmeContext(context)->scanManagerInstance, now,
                     cfg->connectionInfo.channelNumber,
                     (cfg->connectionInfo.ssid.length?1:0),
                     ssid,
                     cfg->connectionInfo.bssid,
                     TRUE,
                     TRUE,
                     FALSE,
                     unifi_ScanStopFirstResult,
                     bssType,
                     ((bssType == BssType_Independent)?ADHOC_PROBE_SCAN_CHANNEL_TIME_TU:0));

    fsm_next_state(context, FSMSTATE_waitingForActiveScanProbe);
}


static CsrBool initiate_host_adhoc(FsmContext* context)
{
    FsmData       * fsmData       = FSMDATA;
    CsrUint8           startIeBuf[]  = { ADHOC_BASIC_DATA_RATES,
                                      ADHOC_EXTENDED_DATA_RATES,
                                      ADHOC_ATIM_WINDOW }; /* NB: must be last entry */
    CsrUint16          startIeBufLen = sizeof(startIeBuf);
    SmeConfigData * cfg           = get_sme_config(context);
    CsrUint8         * ieTotalBuf;   /* container for the start request's IE */
    CsrUint16          ieTotalSz;    /* total size of the start request's IE */
    CsrUint8         * assyPtr;      /* assembly pointer */
    CsrUint8           chNum;

    sme_trace_entry((TR_CXN_MGR, ">> initiate_host_adhoc()"));

    /*
     * Let's see if we are allowed to host an ad-hoc network before we start doing any
     * work
     */
    chNum = cfg->connectionConfig.adhocChannel;

    sme_trace_debug((TR_CXN_MGR, "Checking if allowed to host adhoc on channel %d", chNum));
    if (chNum == 0)
    {

        /* Set the band appropriately when support for 802.11a is introduced */
        chNum = srs_get_free_channel(context, unifi_80211OFDM24);

        /* srs_get_free_channel always returns a channel unless there are no channels
         * allowed by the regulatory domain subsystem
         */
        if (chNum == 0)
        {
            sme_trace_warn((TR_CXN_MGR, "Regulatory subsystem will not currently allow an ad-hoc start on any channel"));
            sme_trace_entry((TR_CXN_MGR, "<< initiate_host_adhoc()"));
            return FALSE;
        }
    }
    else if (regdom_is_channel_actively_usable(context, chNum) == FALSE)
    {
        sme_trace_warn((TR_CXN_MGR, "Regulatory subsystem will not allow an ad-hoc start on channel %d", chNum));
        sme_trace_entry((TR_CXN_MGR, "<< initiate_host_adhoc()"));
        fsm_next_state(context, FSMSTATE_waitingForNsTerminate);
        return FALSE;
    }

    /* set the configured atim window */
    startIeBuf[startIeBufLen - 2] = cfg->adHocConfig.atimWindowTu & 0xFF;
    startIeBuf[startIeBufLen - 1] = cfg->adHocConfig.atimWindowTu >> 8;

    /* Build typical IE for start Request */
    ieTotalSz = 2 + cfg->connectionConfig.ssid.length + startIeBufLen;

    assyPtr = ieTotalBuf = CsrPmalloc(ieTotalSz);

    /* Insert SSID IE into the buffer */
    (void)ie_setSSID(assyPtr, &cfg->connectionConfig.ssid);
    assyPtr += 2 + cfg->connectionConfig.ssid.length;

    /* Insert the 'standard' IEs into the buffer */
    CsrMemCpy(assyPtr, startIeBuf, startIeBufLen);
    assyPtr += startIeBufLen;

    fsmData->joinOrStartIeDR = regdom_create_ie_dr_src_buf(context, ieTotalBuf, ieTotalSz, FALSE);
    CsrPfree(ieTotalBuf);

    /* Set up the connectionInfo structure - the WiFi Network Application
     * may query this.
     */
    cfg->connectionInfo.ssid                   = cfg->connectionConfig.ssid;
    cfg->connectionInfo.ifIndex                = unifi_GHZ_2_4;
    cfg->connectionInfo.channelNumber          = chNum;
    cfg->connectionInfo.channelFrequency       = frequencyTable2point4Ghz[cfg->connectionInfo.channelNumber];
    cfg->connectionInfo.assocReqCapabilities   = 0x0022;
    cfg->connectionInfo.assocRspAssociationId  = 0;
    cfg->connectionInfo.assocReqListenIntervalBeacons = 0;
    cfg->connectionInfo.assocRspCapabilityInfo = cfg->connectionInfo.assocReqCapabilities;
    cfg->connectionInfo.assocRspResult         = unifi_IEEE80211ResultSuccess;
    cfg->connectionInfo.networkType80211       = unifi_80211Auto;
    cfg->connectionInfo.atimWindowTu           = cfg->adHocConfig.atimWindowTu;
    cfg->connectionInfo.beaconPeriodTu         = cfg->adHocConfig.beaconPeriodTu;

    /* If we are using WEP then set the privacy bit */
    if (cfg->connectionConfig.privacyMode == unifi_80211PrivacyEnabled)
    {
        cfg->connectionInfo.assocReqCapabilities |= 0x0010;
    }

    send_coex_connecting_ind(context, getSmeContext(context)->coexInstance);

    /* Create and start a new Security Manager instance for this connection */
    fsmData->securityManagerInstance = fsm_add_instance(context, &security_manager_fsm, TRUE);
    send_security_join_start_req(context, fsmData->securityManagerInstance);
    fsm_next_state(context, FSMSTATE_waitingForSecurityJoinStart);

    sme_trace_entry((TR_CXN_MGR, "<< initiate_host_adhoc()"));
    return TRUE;
}

/* =========[ WAITING_FOR_ACTIVE_SCAN_PROBE STATE EVENT HANDLERS ]=========== */

/**
 * @brief
 *   Deals with NETWORK_SELECTOR_DETACH_REQ while waiting for probe scan
 *
 * @par Description
 *  A NETWORK_SELECTOR_DETACH_REQ has arrived so we make a note in
 *  abortCode and then cancel the scan.  Detach-specific behaviour is
 *  triggered when Scan Manager has acknowleged.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void waiting_for_active_scan_probe__ns_detach_req(FsmContext *context, const NetworkSelectorDetachReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;
    CsrUint32 now = fsm_get_time_of_day_ms(context);
    sme_trace_entry((TR_CXN_MGR, "waiting_for_active_scan_probe__ns_detach_req"));
    sme_trace_info
    ((
        TR_CXN_MGR,
        "%-25s: received detach from %s during probe scan. "
        "Requesting scan cancel.",
        fsm_current_state_name(context),
        fsm_process_name_by_id(context, req->common.sender_)
    ));


    /* Ask the Scan Manager to prematurely cancel the scan operation. It will
     * return a scan_cfm message regardless.
     */
    send_sm_scan_cancel_ind(context, getSmeContext(context)->scanManagerInstance, now, FALSE);

    /* Set the detach flag so that when the scan_cfm comes back we will know
     * we were asked to abort the connection.
     */
    fsmData->disconnectionReason = DISCONNECTION_DETACH;
}

static CsrUint16 getAtimWindow(FsmContext* context, CsrUint16 infoElementsLength, CsrUint8* infoElements)
{
    CsrUint16 value = 0;
    CsrUint8* ibssIe;
    if (ie_success == ie_find(IE_ID_IBSS, infoElements, infoElementsLength, &ibssIe))
    {
        ibssIe+=2;
        ie_le_CsrUint16(ibssIe, value);
    }
    return value;
}

static void abort_connection(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;

    if (fsmData->skipResetOnError)
    {
        if (fsmData->connectionReason == CONNECTION_JOIN)
        {
            send_network_selector_join_cfm(context, getSmeContext(context)->networkSelectorInstance, unifi_Error);
        }
        else
        {
            send_network_selector_roam_cfm(context, getSmeContext(context)->networkSelectorInstance, unifi_Error);
        }

        fsmData->outstandingNSJoinCfm = FALSE;
        fsm_next_state(context, FSMSTATE_waitingForNsTerminate);
    }
    else
    {
        reset_unifi(context);
    }
}

/**
 * @brief
 *   Scan confirm in response to probe scan request
 *
 * @par Description
 *   A scan_cfm has been received.  This will indicate:
 *      - success
 *      - failure
 *
 *   Failure may be a genuine failure, or be caused by cancellation via an
 *   earlier network_selector_detach_req.  In the case of cancellation, a
 *   flag will have been set to make this obvious.
 *
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_active_scan_probe__active_scan_probe_cfm(FsmContext* context, const SmScanCfm_Evt * cfm)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    srs_scan_data* pJoinScanParameters;
    DataReference ieRef;
    sme_trace_entry((TR_CXN_MGR, "waiting_for_active_scan_probe__active_scan_probe_cfm(%s)", cfm->joinScanResult == FALSE ? "FALSE" : "TRUE"));

    if (fsmData->disconnectionReason != DISCONNECTION_NONE)
    {
        /* If a network_selector_detach_req was received earlier, abandon the
         * connection by resetting UniFi, which leads on to acknowledging the
         * detach, then termination.
         */
        sme_trace_info((TR_CXN_MGR,"__active_scan_probe_cfm() :: Join Cancelled"));
        reset_unifi(context);
        return;
    }
    else if (cfm->joinScanResult == FALSE)
    {
        /* If the scan_cfm was a failure give up. */
        sme_trace_info(( TR_CXN_MGR, "\"%s\" (%s) SCAN FOR JOINING NETWORK FAILED",
                         trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                         trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));

        /* We failed to find the AP... Remove it from the scan results list */
        (void)srs_delete_scan_result_profile(context, &cfg->connectionInfo.bssid, TRUE);

        abort_connection(context);
        return;
    }


    /* Get the current BSSID for this connection from the data acquired
     * by the probe scan.  For an adhoc network, it is randomly
     * generated when the adhoc network is created. */
    pJoinScanParameters = srs_get_join_scan_parameters(context);
    verify(TR_CXN_MGR, pJoinScanParameters != NULL);

    /*
     * Access the IEs for the network we've just scanned and push
     * them into the regulatory subsystem - they may be needed later
     * if we attempt to join this network. Also, pushing these into
     * the regulatory subsystem will cause any necessary mods to be
     * made to the list of IEs to make everything look nice 'n legal
     */
    ieRef = pJoinScanParameters->informationElements;
    if (fsmData->joinOrStartIeDR.dataLength)
    {
        pld_release(getPldContext(context), (PldHdl)fsmData->joinOrStartIeDR.slotNumber);
    }
    fsmData->joinOrStartIeDR = regdom_create_ie_dr(context, ieRef, TRUE);
    if (fsmData->joinOrStartIeDR.dataLength == 0)
    {
        sme_trace_warn(( TR_CXN_MGR, "\"%s\" (%s) REGULATORY PROBLEM WITH THIS NETWORK - CANNOT JOIN",
                         trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                         trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));

        abort_connection(context);
        return;
    }

    /* Extract the beacon period for this network and save it. We will need
     * it later to calculate the listen interval (this is passed to us in
     * msecs but we must specify it in beacon periods to the MLME)
     */
    fsmData->beaconPeriodTu = cfm->beaconPeriod;

    sme_trace_debug ((TR_CXN_MGR, "\"%s\" (%s) %d msec beacon period",
        trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
        fsmData->beaconPeriodTu));

    if ((fsmData->beaconPeriodTu < MINIMUM_SENSIBLE_BEACON_PERIOD_MS) ||
            (fsmData->beaconPeriodTu > MAXIMUM_SENSIBLE_BEACON_PERIOD_MS))
    {
        sme_trace_warn((TR_CXN_MGR, "Unusual beacon period %dms detected - quality may be poor", fsmData->beaconPeriodTu));

        if (fsmData->beaconPeriodTu == 0)
        {
            sme_trace_error((TR_CXN_MGR, "Insane beacon period 0ms, aborting connection"));
            abort_connection(context);
            return;
        }
    }

    /*
     * Make sure selected network is compatible with our config
     */
    if (!validate_ap(context, pJoinScanParameters, cfg))
    {
        sme_trace_info(( TR_CXN_MGR, "\"%s\" (%s) SCAN FOR JOINING NETWORK FAILED VALIDATION - CANNOT JOIN",
                         trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                         trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));

        abort_connection(context);
        return;
    }

    cfg->connectionInfo.bssid           = pJoinScanParameters->scanResult.bssid;
    cfg->connectionInfo.ssid            = pJoinScanParameters->scanResult.ssid;
    cfg->connectionInfo.channelNumber   = pJoinScanParameters->scanResult.channelNumber;
    cfg->connectionInfo.ifIndex         = pJoinScanParameters->scanResult.ifIndex;
    cfg->connectionInfo.atimWindowTu    = getAtimWindow(context, pJoinScanParameters->scanResult.informationElementsLength, pJoinScanParameters->scanResult.informationElements);
    cfg->connectionInfo.beaconPeriodTu  = pJoinScanParameters->scanResult.beaconPeriodTu;

    sme_trace_debug((TR_CXN_MGR, "\"%s\" (%s) Creating Security Manager instance",
        trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    send_coex_connecting_ind(context, getSmeContext(context)->coexInstance);

    /* Create a new Security Manager instance for this connection */
    fsmData->securityManagerInstance = fsm_add_instance(context, &security_manager_fsm, TRUE);

    send_security_join_start_req(context, fsmData->securityManagerInstance);

    fsm_next_state(context, FSMSTATE_waitingForSecurityJoinStart);
}

/* =========[ WAITING_FOR SECURITY JOIN_START STATE EVENT HANDLERS ]========= */

/**
 * @brief
 *   Deals with NETWORK_SELECTOR_DETACH_REQ while waiting for security setup
 *
 * @par Description
 *  A NETWORK_SELECTOR_DETACH_REQ has arrived so we cancel the security setup
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void waiting_for_security_join_start__ns_detach_req(FsmContext *context, const NetworkSelectorDetachReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_CXN_MGR, "waiting_for_security_join_start__ns_detach_req"));

    /* Set flags so we know later to acknowledge the detach and what to put
     * in the failed joincfm (if needed).
     */
    fsmData->disconnectionReason = DISCONNECTION_DETACH;
    shutdown_security_manager(context);
}

/**
 * @brief
 *   Security Manager has completed its initialisation
 *
 * @par Description
 *   Security Manager has completed its startup, using the information from
 *   the probe scan result.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_security_join_start__security_join_start_cfm(FsmContext* context, const SecurityJoinStartCfm_Evt* cfm)
{
    FsmData         * fsmData    = FSMDATA;
    SmeConfigData   * cfg        = get_sme_config(context);

    sme_trace_entry((TR_CXN_MGR,"waiting_for_security_join_start__security_join_start_cfm(%s), hosting %d",
                trace_unifi_Status(cfm->status), fsmData->hostAdhoc));

    if (there_were_errors_or_a_ns_detach_req(context, cfm->status, ResultCode_Success)) return;

    sme_trace_info(( TR_CXN_MGR, "\"%s\" (%s) PRE-JOIN SECURITY OK",
                     trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                     trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));

    if (fsmData->hostAdhoc == TRUE)
    {
        sme_trace_info((TR_CXN_MGR, "\"%s\" STARTING IBSS on channel(%d)",
                        trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                        cfg->connectionInfo.channelNumber));

        verify(TR_CXN_MGR, fsmData->joinOrStartIeDR.dataLength != 0);

        sme_trace_debug_code
        (
            {
                CsrUint8   * dbgBuf;
                CsrUint16    dbgBufLen;

                pld_access(getPldContext(context),
                           (PldHdl)fsmData->joinOrStartIeDR.slotNumber,
                           (void **)&dbgBuf, &dbgBufLen);

                sme_trace_debug((TR_CXN_MGR, "I/F Index     = %d", cfg->connectionInfo.ifIndex));
                sme_trace_debug((TR_CXN_MGR, "Beacon Period = %d msec", cfg->adHocConfig.beaconPeriodTu));
                sme_trace_debug((TR_CXN_MGR, "Probe Delay   = %d usec", HOSTING_ADHOC_PROBE_DELAY_US));
                sme_trace_debug((TR_CXN_MGR, "Capabilities  = 0x%04x", cfg->connectionInfo.assocReqCapabilities));
                sme_trace_hex((TR_CXN_MGR, TR_LVL_DEBUG, "station IE used", dbgBuf, dbgBufLen));
            }
        )

        /* Make sure Qos is Off in the Router */
        call_unifi_sys_qos_control_req(context, unifi_QoSOff);

        /*
         * Send the start request and allow the DD SAP to release the start IEs
         * We don't need these IEs anymore. Set DR dataLen to zero so we won't
         * try to release it ourselves on network teardown.
         */
        send_mlme_start_req(context, fsmData->joinOrStartIeDR,
                           cfg->connectionInfo.ifIndex,
                           cfg->adHocConfig.beaconPeriodTu,
                           cfg->connectionInfo.channelNumber,
                           HOSTING_ADHOC_PROBE_DELAY_US,
                           cfg->connectionInfo.assocReqCapabilities,
                           FALSE);

        fsmData->joinOrStartIeDR.dataLength = 0;
        fsm_next_state(context, FSMSTATE_waitingForMlmeStart);
    }
    else
    {
        srs_scan_data   * pJoinScanParameters = srs_get_join_scan_parameters(context);
        CsrUint16 joinFailureBeaconCount;

        sme_trace_debug((TR_CXN_MGR,"__security_join_start_cfm() :: Joining Network"));

        /*
         * To avoid a potential memory leak we must grab the information elements
         * from the probe scan now. If any kind of error has occurred we can then
         * delete these information elements.
         */
        verify(TR_CXN_MGR, pJoinScanParameters != NULL);

        /* Take responsibility for the data reference containing
         * security-related Information Elements needed for the
         * MLME_ASSOCIATE_REQ
         */
        fsmData->securityIeDataRef = cfm->dataref;

        sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) JOINING",
                        trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));

        /*
         * Set the connection info - this may be queried by the WiFi Network
         * Application. NB: authMode is set by security manager FSM
         */
        cfg->connectionInfo.assocReqCapabilities = pJoinScanParameters->scanResult.capabilityInformation;
        cfg->connectionInfo.channelNumber        = pJoinScanParameters->scanResult.channelNumber;
        cfg->connectionInfo.channelFrequency     = pJoinScanParameters->scanResult.channelFrequency;
        cfg->connectionInfo.networkType80211     = unifi_80211OFDM24;
        if (cfg->connectionInfo.assocScanInfoElementsLength != 0 &&
            !ie_hasGRates(cfg->connectionInfo.assocScanInfoElements, cfg->connectionInfo.assocScanInfoElementsLength))
        {
            cfg->connectionInfo.networkType80211 = unifi_80211DS;
        }

        /*lint -e437*/
        sme_trace_debug_code
        (
            sme_trace_debug((TR_CXN_MGR, "ifIndex %d", pJoinScanParameters->scanResult.ifIndex));
            sme_trace_debug((TR_CXN_MGR, "beaconPeriod %d", pJoinScanParameters->scanResult.beaconPeriodTu));
            sme_trace_debug((TR_CXN_MGR, "timeStamp %lld", pJoinScanParameters->scanResult.timeStamp)); /*lint !e437*/
            sme_trace_debug((TR_CXN_MGR, "localTime %lld", pJoinScanParameters->scanResult.localTime)); /*lint !e437*/
            sme_trace_debug((TR_CXN_MGR, "channelNumber %d", pJoinScanParameters->scanResult.channelNumber));
            sme_trace_debug((TR_CXN_MGR, "capabilityInformation %d", pJoinScanParameters->scanResult.capabilityInformation));
            if (fsmData->joinOrStartIeDR.dataLength)
            {
                CsrUint8   * ieBuf;
                CsrUint16    ieBufLen;

                pld_access(getPldContext(context),
                           (PldHdl)fsmData->joinOrStartIeDR.slotNumber,
                           (void **)&ieBuf, &ieBufLen);

                sme_trace_hex((TR_CXN_MGR, TR_LVL_DEBUG, "IEs from probe scan result", ieBuf, ieBufLen));
            }
        )


        /* Increment the reference count on the join info elements we've stored
         * although we (cxnMgr) own this data, passing it through the DD SAP
         * will cause it to be released, and we might need it again.
         */
        pld_add_ref(getPldContext(context), (PldHdl)fsmData->joinOrStartIeDR.slotNumber);

        /* calculate the number of beacons to allow before failing
         */
        joinFailureBeaconCount = (JOIN_FAILURE_TIMEOUT_MS / pJoinScanParameters->scanResult.beaconPeriodTu);

        /* The result should be 1 or above */
        if(joinFailureBeaconCount == 0)
        {
            joinFailureBeaconCount = 1;
        }

        /* Make sure Qos is Off in the Router */
        call_unifi_sys_qos_control_req(context, unifi_QoSOff);

        send_mlme_join_req(context,
                           fsmData->joinOrStartIeDR,
                           NullDataReference,
                           pJoinScanParameters->scanResult.ifIndex,
                           cfg->connectionInfo.bssid,
                           pJoinScanParameters->scanResult.beaconPeriodTu,
                           *((TsfTime*)&pJoinScanParameters->scanResult.timeStamp), /* These types are compatable */
                           *((TsfTime*)&pJoinScanParameters->scanResult.localTime), /* These types are compatable */
                           pJoinScanParameters->scanResult.channelNumber,
                           pJoinScanParameters->scanResult.capabilityInformation,
                           joinFailureBeaconCount,
                           JOIN_PROBE_DELAY_US);

        fsm_next_state(context, FSMSTATE_waitingForMlmeJoin);
    }
}

/**
 * @brief
 *   Security Manager has completed its initialisation
 *
 * @par Description
 *   Security Manager has completed its startup, make sure that we free any
 *   payloads that may be in the message
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void cleanup_security_join_start_cfm(FsmContext* context, const SecurityJoinStartCfm_Evt* cfm)
{
    if (cfm->dataref.dataLength != 0)
    {
        pld_release(getPldContext(context), cfm->dataref.slotNumber);
    }
}

/* ==============[ WAITING FOR MLME JOIN STATE EVENT HANDLERS ]============== */

/**
 * @brief
 *   Deal with an mlme_join_cfm message
 *
 * @par Description
 *   If the basic IEEE 802.11 join operation completed successfully, go to
 *   the authentication stage if the network is open system or WEP,
 *   otherwise start the sequence to set the group and pairwise keys
 *   (WPA/WPA2).
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_mlme_join__mlme_join_cfm(FsmContext* context, const MlmeJoinCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    CsrBool rsnaEnabled = get_security_rsna_enabled(context, fsmData->securityManagerInstance);

    sme_trace_entry((TR_CXN_MGR,"joining_mlme_join_cfm(%s)", trace_ResultCode(cfm->resultCode)));

    if (there_were_errors_or_a_ns_detach_req(context, unifi_Success, cfm->resultCode))
    {
        if (cfm->beaconFrame.dataLength)
        {
            pld_release(getPldContext(context), cfm->beaconFrame.slotNumber);
        }
        return;
    }

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) JOINED",
                    trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));

    /* Save as part of connectionInfo */
    /* Handle 802.11 N frames */
    if (cfg->connectionBeaconFrameDataRef.dataLength != 0)
    {
        pld_release(getPldContext(context), cfg->connectionBeaconFrameDataRef.slotNumber);
    }
    cfg->connectionBeaconFrameDataRef = cfm->beaconFrame;
    cfg->connectionInfo.beaconFrameLength = cfm->beaconFrame.dataLength;
    cfg->connectionInfo.beaconFrame = NULL;
    cfg->connectionInfo.assocScanInfoElementsLength = 0;
    cfg->connectionInfo.assocScanInfoElements = NULL;
    if (cfm->beaconFrame.dataLength)
    {
        CsrUint16 len;
        pld_access(getPldContext(context), cfg->connectionBeaconFrameDataRef.slotNumber, (void**)&cfg->connectionInfo.beaconFrame, &len);
        /* Get ref to info elements */
#define BEACON_FRAME_IE_OFFSET 36
        if (cfm->beaconFrame.dataLength > BEACON_FRAME_IE_OFFSET) {
            cfg->connectionInfo.assocScanInfoElementsLength = cfm->beaconFrame.dataLength - BEACON_FRAME_IE_OFFSET;
            cfg->connectionInfo.assocScanInfoElements = &cfg->connectionInfo.beaconFrame[BEACON_FRAME_IE_OFFSET];
        }
        sme_trace_hex((TR_CXN_MGR, TR_LVL_DEBUG, "MlmeJoinCfm->beaconFrame", cfg->connectionInfo.beaconFrame, cfg->connectionInfo.beaconFrameLength));
        sme_trace_hex((TR_CXN_MGR, TR_LVL_DEBUG, "MlmeJoinCfm->Info Elements", cfg->connectionInfo.assocScanInfoElements, cfg->connectionInfo.assocScanInfoElementsLength));
    }

    if (cfg->connectionConfig.bssType != unifi_Infrastructure)
    {
        send_security_connect_start_req(context, fsmData->securityManagerInstance);
        fsm_next_state(context, FSMSTATE_waitingForSecurityConnectStart);
        return;
    }

    /* Take a look at what the security manager told us about the
     * network we want to join - there are 2 params:
     * authentication_type and privacy_enabled. */
    if (!rsnaEnabled)
    {
        if (cfg->connectionConfig.privacyMode == unifi_80211PrivacyEnabled)
        {
            sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) WEP Secured Network",
                            trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                            trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));
        }
        else
        {
            sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) Open System Network",
                            trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                            trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));
        }

        /* To work around broken access points by forcing them into a known
         * state, first send a deauthentication request to the access point to
         * which we want to authenticate.  This should result in a
         * deauthenticate confirm regardless of whether the AP thought we were
         * connected or not (we are not at the moment).
         */
        sme_trace_debug((TR_CXN_MGR,"waiting_for_mlme_join__mlme_join_cfm: sending precautionary deauthenticate to force AP to known state"));
        do_deauth_req(context);
        fsm_next_state(context, FSMSTATE_waitingForJoinMlmeDeauthenticate);
    }
    else
    {
        sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) WPA/WPA2 Secured Network",
                        trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));

        sme_trace_debug((TR_CXN_MGR,"%-25s: ---------[ \"%s\" (%s) ]--- SETTING GROUP PROTECTION to None",
                         fsm_current_state_name(context),
                         trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                         trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

        send_mlme_setprotection_req(context, cfg->connectionInfo.bssid, ProtectType_None, KeyType_Group);

        fsm_next_state(context, FSMSTATE_waitingForGroupMlmeSetProtection);
    }
}

/* =====[ WAITING FOR MLME_START (ADHOC HOSTING) STATE EVENT HANDLERS ]====== */

/**
 * @brief
 *   mlme_start_cfm message received for adhoc hosting
 *
 * @par Description
 *   If the mlme_start_cfm reports an error, then the MLME has failed to
 *   start hosting an adhoc network and the attempt is abandoned.
 *   Otherwise, we move on to the SecurityConnectStart to sort out any
 *   security requirements.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waitingForMlmeStart__mlme_start_cfm(FsmContext* context, const MlmeStartCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CXN_MGR,"waitingForMlmeStart__mlme_start_cfm(%s)", trace_ResultCode(cfm->resultCode)));

    if (there_were_errors_or_a_ns_detach_req(context, unifi_Success, cfm->resultCode)) return;

    cfg->connectionInfo.bssid = cfm->bssid;
    cfg->connectionInfo.assocReqApAddress = cfm->bssid;

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) STARTED",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));


    send_security_connect_start_req(context, fsmData->securityManagerInstance);
    fsm_next_state(context, FSMSTATE_waitingForSecurityConnectStart);
}

/* ==========[ MLME SETTING GROUP PROTECTION STATE EVENT HANDLERS ]========== */

/**
 * @brief
 *   mlme_setprotection_cfm for Group mlme_setprotection_req
 *
 * @par Description
 *   For WPA/WPA2 - there are two protection steps: group and pairwise. This
 *   transition deals with the confirmation from our group request.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_group_mlme_set_protection__mlme_setprotection_cfm(FsmContext* context, const MlmeSetprotectionCfm_Evt* cfm)
{
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CXN_MGR, "waiting_for_group_mlme_set_protection__mlme_setprotection_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    if (there_were_errors_or_a_ns_detach_req(context, unifi_Success, cfm->resultCode)) return;

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) GROUP PROTECTION SET to None",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    send_mlme_setprotection_req(context, cfg->connectionInfo.bssid, ProtectType_None, KeyType_Pairwise);

    fsm_next_state(context, FSMSTATE_waitingForPairwiseMlmeSetProtection);
}

/* =============[ MLME SETTING PAIRWISE PROTECTION STATE EVENT HANDLERS ]============== */

/**
 * @brief
 *   mlme_setprotection_cfm for Pairwise mlme_setprotection_req
 *
 * @par Description
 *   For WPA/WPA2 - wait for confirmation from pairwise protection request.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_pairwise_mlme_set_protection__mlme_setprotection_cfm(FsmContext* context, const MlmeSetprotectionCfm_Evt* cfm)
{
    sme_trace_entry((TR_CXN_MGR,"__mlme_setprotection_cfm(%s)", trace_ResultCode(cfm->resultCode)));

    if (there_were_errors_or_a_ns_detach_req(context, unifi_Success, cfm->resultCode)) return;

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) PAIRWISE PROTECTION SET to None",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(get_sme_config(context)->connectionInfo.bssid, getMacAddressBuffer(context))));

    /* To work around broken access points by forcing them into a known
     * state, first send a deauthentication request to the access point to
     * which we want to authenticate.  This should result in a
     * deauthenticate confirm regardless of whether the AP thought we were
     * connected or not (we are not at the moment).
     */
    do_deauth_req(context);
    fsm_next_state(context, FSMSTATE_waitingForJoinMlmeDeauthenticate);
}

static void waiting_for_join_mlme_deauthenticate__mlme_deauthenticate_cfm(FsmContext* context,
    const MlmeDeauthenticateCfm_Evt* cfm)
{
    FsmData*            fsmData = FSMDATA;
    sme_trace_info_code(SmeConfigData* cfg = get_sme_config(context);)
    sme_trace_info((TR_CXN_MGR,"\"%s\" (%s) DEAUTHENTICATED",
                     trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                     trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));


    /* if configured for Open we do open only
     * if configured for Shared we do open and if that fails we try Shared
     * So at this stage we do open regardless
     */
    fsmData->chosenAuthenticationType = AuthenticationType_OpenSystem;
    do_auth_req(context,   fsmData->chosenAuthenticationType );

    fsm_next_state(context, FSMSTATE_waitingForMlmeAuthenticate);
}

/* ==========[ WAITING FOR MLME AUTHENTICATE STATE EVENT HANDLERS ]========== */

/**
 * @brief
 *   mlme_authenticate_cfm when authenticating
 *
 * @par Description
 *   Authentication Cfm from firmware.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_mlme_authenticate__mlme_authenticate_cfm(FsmContext *context,  const MlmeAuthenticateCfm_Evt *cfm)
{
    CsrUint16              listenIntervalMillisecs;
    FsmData*            fsmData = FSMDATA;
    SmeConfigData*      cfg = get_sme_config(context);

    sme_trace_entry((TR_CXN_MGR,
        ">> waiting_for_mlme_authenticate__mlme_authenticate_cfm(%s), authRetry=%d",
        trace_ResultCode(cfm->resultCode), fsmData->authRetry));

    if (cfm->informationElements.dataLength != 0)
    {
        pld_release(getPldContext(context), cfm->informationElements.slotNumber);
    }

    if (cfm->resultCode == ResultCode_Success)
    {
        sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) AUTHENTICATED",
                        trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

        /* If Network Selector sent a detach while we were authenticating,
         * we need to shut down.  Start with security and deauthentication
         * will follow on from that.
         */
        if (fsmData->disconnectionReason != DISCONNECTION_NONE)
        {
            shutdown_security_manager(context);
            return;
        }

        /* Else, we successfully authenticated, so when the connection comes
         * down, we will need to deauthenticate.  At present, this will be
         * active for infrastructure networks and not for adhoc networks,
         * but it is technically feasible to authenticate to an adhoc peer.
         */
        fsmData->currentlyAuthenticated = TRUE;

        /* Change the state of the uncontrolled port to
         * unifi_8021xPortClosedBlock so the driver can buffer the EAPOL
         * packets. If the association fails, the state should be set back
         * to unifi_8021xPortClosedDiscard.
         */
        (void)ns_port_configure(context, unifi_8021xPortClosedBlock, unifi_8021xPortClosedDiscard);

    }
    else if ( (cfm->resultCode == ResultCode_Refused)   /* its possbile the authenication has failed due to a misalignment of keys but exclude Open/Shared */
           && (cfg->connectionInfo.authMode != unifi_80211AuthOpen)
           && (cfg->connectionInfo.authMode != unifi_80211AuthShared) )
    {

        if (fsmData->joinRetry < MAX_MLME_JOIN_RETRYS)
        {
            srs_scan_data* pJoinScanParameters = srs_get_join_scan_parameters(context);

            sme_trace_debug((TR_CXN_MGR,"waiting_for_mlme_authenticate__mlme_authenticate_cfm() :: ResultCode_Refused"));

            verify(TR_CXN_MGR, pJoinScanParameters != NULL);

            fsmData->joinRetry++;

            sme_trace_debug((TR_CXN_MGR, "ifIndex %d", pJoinScanParameters->scanResult.ifIndex));
            sme_trace_debug((TR_CXN_MGR, "beaconPeriod %d", pJoinScanParameters->scanResult.beaconPeriodTu));
            sme_trace_debug((TR_CXN_MGR, "timestamp %lld", pJoinScanParameters->scanResult.timeStamp)); /*lint !e437*/
            sme_trace_debug((TR_CXN_MGR, "localTime %lld", pJoinScanParameters->scanResult.localTime)); /*lint !e437*/
            sme_trace_debug((TR_CXN_MGR, "channelNumber %d", pJoinScanParameters->scanResult.channelNumber));
            sme_trace_debug((TR_CXN_MGR, "capabilityInformation %d", pJoinScanParameters->scanResult.capabilityInformation));
            sme_trace_debug((TR_CXN_MGR, "informationElements %d", pJoinScanParameters->scanResult.informationElementsLength));

            /* Increment the reference count on the join info elements we've stored
             * although we (cxnMgr) own this data, passing it through the DD SAP
             * will cause it to be released, and we might need it again.
             */
            pld_add_ref(getPldContext(context), (PldHdl)fsmData->joinOrStartIeDR.slotNumber);
            send_mlme_join_req(context,
                               fsmData->joinOrStartIeDR,
                               NullDataReference,
                               pJoinScanParameters->scanResult.ifIndex,
                               cfg->connectionInfo.bssid,
                               pJoinScanParameters->scanResult.beaconPeriodTu,
                               *((TsfTime*)&pJoinScanParameters->scanResult.timeStamp), /* These types are compatable */
                               *((TsfTime*)&pJoinScanParameters->scanResult.localTime), /* These types are compatable */
                               pJoinScanParameters->scanResult.channelNumber,
                               pJoinScanParameters->scanResult.capabilityInformation,
                               (JOIN_FAILURE_TIMEOUT_MS / pJoinScanParameters->scanResult.beaconPeriodTu),
                               JOIN_PROBE_DELAY_US);

            fsm_next_state(context, FSMSTATE_waitingForMlmeJoin);

            return;
        }
        else
        {
            shutdown_security_manager(context);
            return;
        }
    }
    else
    {
        /* The authentication failed so consider retrying automatically as
         * we may be in a poor radio environment.  Obviously, do not retry
         * if we were already told to detach by the Network Selector.
         */
        if (fsmData->disconnectionReason == DISCONNECTION_NONE)
        {
            if (fsmData->authRetry < MAX_MLME_AUTHENTICATION_RETRYS)
            {
                fsmData->authRetry++;

                sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) RETRY AUTHENTICATION",
                                trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                                trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));

                do_auth_req(context, fsmData->chosenAuthenticationType );
                return;
            }
            else
            {
                if ( (cfg->connectionInfo.authMode == unifi_80211AuthShared) && (fsmData->chosenAuthenticationType == AuthenticationType_OpenSystem ) )
                {
                    /* if we were configured to do sharedKey authentication we would have tried OpenSystem
                     * and being here means we have failed after using the max number of retries
                     * then need to try sharedKey
                     * but also reset Retry Counter to have same number of attempts as we did for OpenSystem
                     */
                    sme_trace_info((TR_CXN_MGR,
                                       "Currently configured for SharedKey Authentication and First %d attempts using OpenSystem have failed So trying now with SharedKey",
                                       fsmData->authRetry ));
                    fsmData->authRetry =0;
                    fsmData->chosenAuthenticationType = AuthenticationType_SharedKey;
                    do_auth_req(context, fsmData->chosenAuthenticationType );
                    return;
                }

                /* Authentication Failed after using maximum number of retries
                 * and nothing else to try...
                 */
                sme_trace_info((TR_CXN_MGR,
                                "Authentication Failed after using maximum number of retries" ));
            }
        }

        shutdown_security_manager(context);
        return;
    }

    /* Authentication succeeded, Network Selector has not asked us to stop.
     * Carry on with the connection; next step is association for first
     * join, or reassociation for a roam.
     */

    /* Convert the user supplied listen interval (measured in msecs) into units
     * of beacon periods. Round up or down to the nearest beacon period.
     */
    listenIntervalMillisecs = get_sme_config(context)->powerConfig.listenIntervalBeacons;
    fsmData->listenIntervalBeacons =
        ((fsmData->beaconPeriodTu >> 1) + listenIntervalMillisecs) /
                fsmData->beaconPeriodTu;

    /* Although we would hope this never happens we must handle the case where
     * the listen interval (in msecs) requested is less than half the beacon
     * period.
     *
     * This manifests with our listen (in beacon periods) evaluating to zero
     */
    if (fsmData->listenIntervalBeacons == 0)
    {
        sme_trace_warn((TR_CXN_MGR,
            "%-25s: Requested listen interval (%d msec) less than half of the "
            "beacon period (%d msec)! Forcing unifi to wake for every beacon",
            fsm_current_state_name(context),
            listenIntervalMillisecs, fsmData->beaconPeriodTu));
    }
    else
    {
        /*
         * The firmware regards the listen interval as the number of beacons it can MISS, not how
         * frequently it has to wake (i.e. a listen interval of 1 is interpreted as 'I wake for every 
         * second beacon', not 'wake for every beacon'.
         *
         * Subtract 1 from the listenIntervalBeacons to make our interpretation match that of the firmware
         */
        fsmData->listenIntervalBeacons--; 
    }

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) ASSOCIATING (listen interval = %d beacons (%d msecs))",
        trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)), 
        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
        fsmData->listenIntervalBeacons,
        (fsmData->listenIntervalBeacons + 1) * fsmData->beaconPeriodTu));

    /* Save the assoc cfm info for later access via the app_get api */
    cfg->connectionInfo.assocReqListenIntervalBeacons = fsmData->listenIntervalBeacons;
    cfg->connectionInfo.assocReqApAddress = cfg->connectionInfo.bssid;

    /* build up all the */
    ie_create_associated_req_ies(context, fsmData->securityIeDataRef, cfg);

    /* the security data is no longer needed, free it */
    if (fsmData->securityIeDataRef.dataLength != 0)
    {
        pld_release(getPldContext(context), fsmData->securityIeDataRef.slotNumber);
        fsmData->securityIeDataRef.dataLength = 0;
    }

    fsm_next_state(context, FSMSTATE_waitingForUnifiSysQoSControl);
}

/* =============[ WAIT FOR UNIFI DD QOS CONTROL CFM STATE EVENT HANDLERS ]============= */

/**
 * @brief
 *   UnifiSysQosControlCfm before associating
 *
 * @par Description
 *   QoS Control confirm has been sent by UniFi.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_unifi_qos_control_cfm(FsmContext* context, const UnifiSysQosControlCfm_Evt *cfm)
{
    /* Check for Deauth */
    if (fsm_sniff_saved_event(context,MLME_DEAUTHENTICATE_IND_ID))
    {
        shutdown_security_manager(context);
        return;
    }

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) %sASSOCIATION",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(get_sme_config(context)->connectionInfo.bssid, getMacAddressBuffer(context)),
                    (FSMDATA->reassociate)?"RE":""));

    do_assoc_req(context);

    fsm_next_state(context, FSMSTATE_waitingForMlmeAssociate);
}

/* =============[ WAIT FOR MLME ASSOCIATE STATE EVENT HANDLERS ]============= */

/**
 * @brief
 *   mlme_associate_cfm when associating
 *
 * @par Description
 *   Association confirm has been sent by UniFi.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void process_associate_reassociate_cfm(
        FsmContext* context,
        const MlmeAssociateCfm_Evt *cfm,
        CsrBool isReAssociate)
{
    SmeConfigData* cfg = get_sme_config(context);
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_CXN_MGR, "waiting_for_mlme_associate__mlme_associate_cfm(%s)", trace_ResultCode(cfm->resultCode)));

    if (cfm->informationElements.dataLength != 0)
    {
        pld_release(getPldContext(context), cfm->informationElements.slotNumber);
    }
    if (cfg->connectionExchangedFramesDataRef.dataLength != 0)
    {
        pld_release(getPldContext(context), cfg->connectionExchangedFramesDataRef.slotNumber);
    }
    cfg->connectionInfo.associationReqFrameLength = 0;
    cfg->connectionInfo.associationReqFrame = NULL;
    cfg->connectionInfo.associationRspFrameLength = 0;
    cfg->connectionInfo.associationRspFrame = NULL;
    cfg->connectionInfo.assocReqInfoElementsLength = 0;
    cfg->connectionInfo.assocReqInfoElements = NULL;
    cfg->connectionInfo.assocRspInfoElementsLength = 0;
    cfg->connectionInfo.assocRspInfoElements = NULL;

    /* Save as part of connectionInfo */
    /* Handle 802.11 N frames */
    cfg->connectionExchangedFramesDataRef = cfm->exchangedFrames;
    if (cfm->exchangedFrames.dataLength)
    {
#define ASSOC_REQ_FRAME_IE_OFFSET 28
#define ASSOC_RSP_FRAME_IE_OFFSET 30
#define REASSOC_REQ_FRAME_IE_OFFSET 34

        CsrUint8* buffer = NULL;
        CsrUint16 len;
        CsrUint8 reqFrameIeOffset = ASSOC_REQ_FRAME_IE_OFFSET;
        CsrUint8 rspFrameIeOffset = ASSOC_RSP_FRAME_IE_OFFSET;

        if (isReAssociate == TRUE)
        {
            reqFrameIeOffset = REASSOC_REQ_FRAME_IE_OFFSET;
        }

        pld_access(getPldContext(context), cfg->connectionExchangedFramesDataRef.slotNumber, (void**)&buffer, &len);

        sme_trace_debug((TR_CXN_MGR, "MlmeAssociateCfm->exchangedFrames size = %d buffer = %p", len, buffer));
        sme_trace_hex((TR_CXN_MGR, TR_LVL_DEBUG, "MlmeAssociateCfm->exchangedFrames", buffer, len));

        /* Don't know if we actually received a response frame so only check for the request frame at this point*/
        verify(TR_CXN_MGR, cfm->exchangedFrames.dataLength >= (4 + reqFrameIeOffset));

        /*
         * Associate Req
         */
        cfg->connectionInfo.associationReqFrameLength = event_unpack_CsrUint16(&buffer);
        if (cfg->connectionInfo.associationReqFrameLength != 0)
        {
            verify(TR_CXN_MGR, cfg->connectionInfo.associationReqFrameLength >= reqFrameIeOffset);
            cfg->connectionInfo.associationReqFrame = buffer;
            cfg->connectionInfo.assocReqInfoElementsLength = cfg->connectionInfo.associationReqFrameLength - reqFrameIeOffset;

            sme_trace_debug((TR_CXN_MGR, "MlmeAssociateCfm->assoc_req size = %d buffer = %p", cfg->connectionInfo.associationReqFrameLength, buffer));
            sme_trace_hex((TR_CXN_MGR, TR_LVL_DEBUG, "MlmeAssociateCfm->assoc_req", cfg->connectionInfo.associationReqFrame, cfg->connectionInfo.associationReqFrameLength));

            if (cfg->connectionInfo.assocReqInfoElementsLength != 0)
            {
                cfg->connectionInfo.assocReqInfoElements = &buffer[reqFrameIeOffset];
                sme_trace_hex((TR_CXN_MGR, TR_LVL_DEBUG, "MlmeAssociateCfm->assoc_req ie", cfg->connectionInfo.assocReqInfoElements, cfg->connectionInfo.assocReqInfoElementsLength));
            }
        }
        else
        {
            sme_trace_debug((TR_CXN_MGR, "MlmeAssociateCfm->assoc_req Empty"));
        }

        buffer += cfg->connectionInfo.associationReqFrameLength;

        /*
         * Associate Cfm
         */
        cfg->connectionInfo.associationRspFrameLength = event_unpack_CsrUint16(&buffer);
        if (cfg->connectionInfo.associationRspFrameLength != 0)
        {
            verify(TR_CXN_MGR, cfg->connectionInfo.associationRspFrameLength >= rspFrameIeOffset);
            cfg->connectionInfo.associationRspFrame = buffer;
            cfg->connectionInfo.assocRspInfoElementsLength = cfg->connectionInfo.associationRspFrameLength - rspFrameIeOffset;

            sme_trace_debug((TR_CXN_MGR, "MlmeAssociateCfm->assoc_rsp size = %d buffer = %p", cfg->connectionInfo.associationRspFrame, buffer));
            sme_trace_hex((TR_CXN_MGR, TR_LVL_DEBUG, "MlmeAssociateCfm->assoc_rsp", cfg->connectionInfo.associationRspFrame, cfg->connectionInfo.associationRspFrameLength));
            if (cfg->connectionInfo.assocRspInfoElementsLength != 0)
            {
                cfg->connectionInfo.assocRspInfoElements = &buffer[rspFrameIeOffset];
                sme_trace_hex((TR_CXN_MGR, TR_LVL_DEBUG, "MlmeAssociateCfm->assoc_rsp ie", cfg->connectionInfo.assocRspInfoElements, cfg->connectionInfo.assocRspInfoElementsLength));

/*                ie_process_associated_cfm_ies(context, cfg->connectionInfo.assocRspInfoElements, cfg->connectionInfo.assocRspInfoElementsLength);
 *
 */
            }
        }
        else
        {
            sme_trace_debug((TR_CXN_MGR, "MlmeAssociateCfm->assoc_rsp Empty"));
        }

        verify(TR_CXN_MGR, cfm->exchangedFrames.dataLength == (4 + cfg->connectionInfo.associationRspFrameLength + cfg->connectionInfo.associationReqFrameLength));
    }

    /* Save the assoc cfm info for later access via the app_get api */
    cfg->connectionInfo.assocRspResult = cfm->resultCode;
    cfg->connectionInfo.assocRspCapabilityInfo = cfm->capabilityInformation;
    cfg->connectionInfo.assocRspAssociationId = cfm->associationId;

    /* If Network Selector sent a detach or we got deathed while we were associating,
     * we need to shut down.  Start with security and deauthentication
     * will follow on from that.
     */
    if (fsmData->disconnectionReason != DISCONNECTION_NONE)
    {
        sme_trace_info((TR_CXN_MGR,
            "%-25s: ---------[ \"%s\" (%s) ]--- Disconnecting due to earlier detach request or deauth",
            fsm_current_state_name(context),
            trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)), trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));
        shutdown_security_manager(context);
        return;
    }

    (void)qos_tspec_process_associate_cfm_ie(qos_get_tspecdatablk_context(context),
                                       cfg->connectionInfo.assocRspInfoElements,
                                       cfg->connectionInfo.assocRspInfoElementsLength,
                                       cfm->resultCode);

    if (cfm->resultCode == ResultCode_Success)
    {
        sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) ASSOCIATED",
                        trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    }
    else
    {
        sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) %sASSOCIATION FAILED (result=%s)",
                        trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                        (fsmData->reassociate)?"RE":"",
                        trace_ResultCode(cfm->resultCode)));

        switch (cfm->resultCode)
        {
        case ResultCode_UnspecifiedQosFailure:
        case ResultCode_WrongPolicy:
        case ResultCode_InsufficientBandwidth:
        case ResultCode_InvalidTspecParameters:
        {
            /* inform QoS that the TSPECs failed */
            send_qos_disassociated_ind(context, getSmeContext(context)->qosInstance, NullBssid, cfm->resultCode);
            break;
        }
        default:
        {
            /* The association failed so consider retrying automatically as
             * we may be in a poor radio environment. */
            if (fsmData->assocRetry < MAX_MLME_ASSOCIATE_RETRYS)
            {
                fsmData->assocRetry++;

                /* Do not use reassociate requests if we get invalid parameters */
                if (cfm->resultCode == ResultCode_InvalidParameters)
                {
                    fsmData->reassociate = FALSE;
                }

                sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) RETRY %sASSOCIATION",
                                trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                                trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                                (fsmData->reassociate)?"RE":""));

                do_assoc_req(context);

                return;
            }

            /* The uncontrolled port has been opened in the authentication cfm. */
            (void)ns_port_configure(context, unifi_8021xPortClosedDiscard, unifi_8021xPortClosedDiscard);
        }
        }

        shutdown_security_manager(context);
        return;
    }

    /* Association succeeded, Network Selector has not asked us to stop.
     * Carry on with the connection;
     */

    {
        CsrUint8 *wmmParamIe = ie_wmm_check_for_wmm_parameter_ie(cfg->connectionInfo.assocRspInfoElements, cfg->connectionInfo.assocRspInfoElementsLength);

        /* check for a WMM Parameter ie */
        if (wmmParamIe != NULL)
        {
            cfg->WMMAssociation = TRUE;

            /* update the AC Mask */
            cfg->WMMParamsACmask = build_acm_mask(cfg->connectionInfo.assocRspInfoElements, cfg->connectionInfo.assocRspInfoElementsLength);
        }
    }

    send_security_connect_start_req(context, fsmData->securityManagerInstance);
    send_link_quality_start_monitoring_req(context, getSmeContext(context)->linkQualityInstance);
    send_qos_associated_ind(context, getSmeContext(context)->qosInstance, cfg->connectionInfo.bssid);

    fsm_next_state(context, FSMSTATE_waitingForSecurityConnectStart);
}

/**
 * @brief
 *   mlme_reassociate_cfm when reassociating
 *
 * @par Description
 *   Reassociation confirm has been sent by UniFi.  As its parameters are
 *   identical to the associate confirm, we cast it to that type and call
 *   the existing handler to deal with it.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_mlme_associate__mlme_associate_cfm(FsmContext* context,
        const MlmeAssociateCfm_Evt *cfm)
{
    process_associate_reassociate_cfm(context, cfm, FALSE);
}

static void waiting_for_mlme_reassociate__mlme_reassociate_cfm(FsmContext* context,
        const MlmeReassociateCfm_Evt *cfm)
{
    /* Always treat the re-associate cfm type as an associate type. */
    process_associate_reassociate_cfm(context, (MlmeAssociateCfm_Evt *) cfm, TRUE);
}


/**
 * @brief
 *   mlme_deauthenticate_ind when associating
 *
 * @par Description
 *   Association confirm has been sent by UniFi.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_mlme_associate__mlme_deauthenticate_ind(FsmContext* context, const MlmeDeauthenticateInd_Evt *ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    FsmData* fsmData = FSMDATA;

    if (fsmData->disconnectionReason != DISCONNECTION_DETACH)
    {
        cfg->disassocReason = unifi_IEEE80211ReasonSuccess;
        cfg->deauthReason = ind->reasonCode;
        fsmData->disconnectionReason = DISCONNECTION_ERROR;
    }
}


/* =======[ WAITING FOR SECURITY CONNECT START STATE EVENT HANDLERS ]======== */

/**
 * @brief
 *   security start cfm when doing 8021x
 *
 * @par Description
 *   Security process has completed after association
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_security_connect_start__security_connect_start_cfm(FsmContext *context, const SecurityConnectStartCfm_Evt *cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_debug_code(SmeConfigData* cfg = get_sme_config(context);)
    sme_trace_entry((TR_CXN_MGR, "waiting_for_security_connect_start__security_connect_start_cfm(%s)", trace_unifi_Status(cfm->status)));

    if (there_were_errors_or_a_ns_detach_req(context, cfm->status, ResultCode_Success)) return;

    /* Security negotiations are now complete. */
    sme_trace_debug((TR_CXN_MGR, "\"%s\" (%s) CONNECT SECURITY OK",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    /* Tell network selector that the join is complete */
    if (fsmData->outstandingNSJoinCfm == TRUE)
    {
        if (fsmData->connectionReason == CONNECTION_JOIN)
        {
            send_network_selector_join_cfm(context, getSmeContext(context)->networkSelectorInstance, unifi_Success);
        }
        else
        {
            send_network_selector_roam_cfm(context, getSmeContext(context)->networkSelectorInstance, unifi_Success);
        }

        /* Having reached the connected state and sent a join_cfm, no
         * further join_cfm is wanted for this connection, regardless of the
         * eventual reason for disconnection.
         */
        fsmData->outstandingNSJoinCfm = FALSE;
    }

    send_coex_connected_ind(context, getSmeContext(context)->coexInstance);
    /* Clear skipResetOnError so we do a FULL Cleardown on Detach */
    fsmData->skipResetOnError = FALSE;
    fsm_next_state(context, FSMSTATE_connected);
}

/**
 * @brief
 *   Network Selector sent a detach_req during 802.1x setup
 *
 * @par Description
 *   While waiting for the Security Manager to finish negotiating security
 *   over the established IEEE 802.11 connection, Network Selector sent a
 *   detach_req to tear down the connection.
 *
 *   Send a _stop_req to Security Manager and wait for the confirm to come
 *   back.  A start_cfm arriving in the meantime will be ignored as
 *   irrelevant.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_security_connect_start__ns_detach_req(FsmContext *context, const NetworkSelectorDetachReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_CXN_MGR, "waiting_for_security_connect_start__ns_detach_req()"));

    fsmData->disconnectionReason = DISCONNECTION_DETACH;
    shutdown_security_manager(context);
}

/**
 * @brief
 *   The network has deauthenticated us in the middle of security negotiation
 *
 * @par Description
 *   While waiting for the Security Manager to finish negotiating security
 *   over the established IEEE 802.11 connection, we were deauthenticated by
 *   the peer.
 *
 *   Send a _stop_req to Security Manager and wait for the confirm to come
 *   back.  A start_cfm arriving in the meantime will be ignored as
 *   irrelevant.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_security_connect_start__mlme_deauthenticate_ind(FsmContext *context, const MlmeDeauthenticateInd_Evt *ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CXN_MGR, "waiting_for_security_connect_start__mlme_deauthenticate_ind()"));
    sme_trace_warn((TR_CXN_MGR, "\"%s\" (%s) MLME REPORTS DEAUTHENTICATED WHILE ATTEMPTING CONNECT SECURITY  - SETTING ABORT FLAGS, STOP SECURITY",
                                trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                                trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    cfg->disassocReason = unifi_IEEE80211ReasonSuccess;
    cfg->deauthReason = ind->reasonCode;
    shutdown_security_manager(context);
}

/**
 * @brief
 *   The network has disassociated us in the middle of security negotiation
 *
 * @par Description
 *   While waiting for the Security Manager to finish negotiating security
 *   over the established IEEE 802.11 connection, we were disassociated by
 *   the peer.  Although this was not a deauthentication, it is very unusual
 *   and unexpected, so we treat it as needing a connection shutdown.
 *
 *   Send a _stop_req to Security Manager and wait for the confirm to come
 *   back.  A start_cfm arriving in the meantime will be ignored as
 *   irrelevant.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_security_connect_start__mlme_disassociate_ind(FsmContext *context, const MlmeDisassociateInd_Evt *ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CXN_MGR, "waiting_for_security_connect_start__mlme_disassociate_ind()"));
    sme_trace_warn((TR_CXN_MGR, "\"%s\" (%s) MLME REPORTS DISASSOCIATED WHILE ATTEMPTING CONNECT SECURITY  - SETTING ABORT FLAGS, STOP SECURITY",
                                trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                                trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    cfg->disassocReason = ind->reasonCode;
    cfg->deauthReason = unifi_IEEE80211ReasonSuccess;
    shutdown_security_manager(context);
}

/**
 * @brief
 *   A mlme_connected_ind arrived during security negotiation
 *
 * @par Description
 *   While waiting for the Security Manager to finish negotiating security
 *   over the established IEEE 802.11 connection, a connection indication
 *   arrived.  If it is a disconnection, we take it to mean the connection
 *   has failed (unless it is for an adhoc network).  Send a _stop_req to
 *   Security Manager and wait for the confirm to come back.  A start_cfm
 *   arriving in the meantime will be ignored as irrelevant.
 *
 *   If it indicates connection, we just ignore it as we already assumed
 *   that we were connected.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_security_connect_start__mlme_connected_ind(FsmContext *context, const MlmeConnectedInd_Evt *ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CXN_MGR, "waiting_for_security_connect_start__mlme_connected_ind(%s)", trace_ConnectionStatus(ind->connectionStatus)));

    if (cfg->connectionConfig.bssType != unifi_Infrastructure)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndIbssStation, &appHandles);
        if (appHandleCount)
        {
            call_unifi_mgt_ibss_station_ind(context, appHandleCount, appHandles, &ind->peerMacAddress, (CsrBool)(ind->connectionStatus == ConnectionStatus_Connected));
        }
        return;
    }

    if (ind->connectionStatus == ConnectionStatus_Disconnected)
    {
        sme_trace_warn((TR_CXN_MGR, "\"%s\" (%s) MLME REPORTS DISCONNECTED WHILE ATTEMPTING CONNECT SECURITY - SETTING ABORT FLAGS, STOP SECURITY",
                        trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));
        shutdown_security_manager(context);
    }

    /* Otherwise, we assumed we were connected anyway, so no action needed */
}


/* ====================[ CONNECTED STATE EVENT HANDLERS ]=================== */

/**
 * @brief
 *   The network has deauthenticated us while we were connected
 *
 * @par Description
 *   While happily connected, we were deauthenticated by the peer.  We tell
 *   the Network Selector that the connection has been dropped, and expect
 *   it to tell us to shut down the connection in due course.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void connected__mlme_deauthenticate_ind(FsmContext *context, const MlmeDeauthenticateInd_Evt *ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_warn((TR_CXN_MGR,"\"%s\" (%s) MLME Deauthenticate Indication, reason %d",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                    ind->reasonCode));

    cfg->disassocReason = unifi_IEEE80211ReasonSuccess;
    cfg->deauthReason = ind->reasonCode;

    /* Send an alert to the network selector - tell it we've been disconnected */
    send_network_selector_roam_ind(context, getSmeContext(context)->networkSelectorInstance, unifi_RoamDeauthenticated);
}

/**
 * @brief
 *   The network has disassociated us while we were connected
 *
 * @par Description
 *   While happily connected, we were disassociated by the peer.  Although
 *   this was not a deauthentication, it is very unusual and unexpected.  We
 *   tell the Network Selector that the connection has been dropped, and
 *   expect it to tell us to shut down the connection in due course.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void connected__mlme_disassociate_ind(FsmContext *context, const MlmeDisassociateInd_Evt *ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_warn((TR_CXN_MGR, "\"%s\" (%s) MLME Disassociate Indication, reason %d",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                    ind->reasonCode));

    cfg->disassocReason = ind->reasonCode;
    cfg->deauthReason = unifi_IEEE80211ReasonSuccess;
    /* Send an alert to the network selector - tell it we've been disconnected */
    send_network_selector_roam_ind(context, getSmeContext(context)->networkSelectorInstance, unifi_RoamDisassociated);
}

/**
 * @brief
 *   The network security has disconnected us while we were connected
 *
 * @par Description
 *   While happily connected, we were disconnected due to security issues,
 *   probably due to a failure to renegotiate security keys or as a defence
 *   against attack. We tell the Network Selector that the connection has
 *   been dropped, and expect it to tell us to shut down the connection in
 *   due course.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void connected__security_disconnected_ind(FsmContext *context, const SecurityDisconnectedInd_Evt *ind)
{
    sme_trace_warn_code(SmeConfigData* cfg = get_sme_config(context);)
    sme_trace_warn((TR_CXN_MGR, "\"%s\" (%s)Security Disconnected Indication, status %s",
                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                    trace_unifi_Status(ind->status)));

    /* Send an alert to the network selector - tell it we've been disconnected */
    send_network_selector_roam_ind(context, getSmeContext(context)->networkSelectorInstance, unifi_RoamDeauthenticated);
}

/**
 * @brief
 *   mlme_connected_ind when connected
 *
 * @par Description
 *   The firmware has sent an mlme_connected_ind with a connectionStatus
 *   parameter that tells us we are either connected (beacons being
 *   received) or disconnected (beacons not received for some time).
 *
 *   At present, these indications mean nothing much for adhoc networks, so
 *   they are deliberately ignored in that context.  Otherwise, the Network
 *   Selector is informed appropriately.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void connected__mlme_connected_ind(FsmContext* context, const MlmeConnectedInd_Evt* ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CXN_MGR, "connected_mlme_connected_ind(%s): network type %s",
                trace_ConnectionStatus(ind->connectionStatus),
                trace_unifi_BSSType(cfg->connectionConfig.bssType)));

    if (cfg->connectionConfig.bssType != unifi_Infrastructure)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndIbssStation, &appHandles);
        if (appHandleCount)
        {
            call_unifi_mgt_ibss_station_ind(context, appHandleCount, appHandles, &ind->peerMacAddress, (CsrBool)(ind->connectionStatus == ConnectionStatus_Connected));
        }

        if (ind->connectionStatus == ConnectionStatus_Disconnected)
        {
            sme_trace_debug((TR_CXN_MGR, "Setting core connection status to unifi_HostingAdhoc"));
            send_link_quality_stop_monitoring_req(context, getSmeContext(context)->linkQualityInstance);
            send_qos_disassociated_ind(context, getSmeContext(context)->qosInstance, ind->peerMacAddress, ResultCode_Success);

            ns_set_media_connection_status(context, unifi_MediaDisconnected);
        }
        else if (ind->connectionStatus == ConnectionStatus_Connected)
        {
            sme_trace_debug((TR_CXN_MGR, "Setting core connection status to unifi_Ready"));
            send_link_quality_start_monitoring_req(context, getSmeContext(context)->linkQualityInstance);
            send_qos_associated_ind(context, getSmeContext(context)->qosInstance, ind->peerMacAddress);

            ns_set_media_connection_status(context, unifi_MediaConnected);
        }

        return;
    }

    if (ind->connectionStatus == ConnectionStatus_Disconnected)
    {
        sme_trace_warn((TR_CXN_MGR, "[ \"%s\" (%s) ]--- MLME indicates BEACON LOST",
                        trace_unifi_SSID(&cfg->connectionInfo.ssid, getSSIDBuffer(context)),
                        trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

        /* the link quality FSM must be stop to stop multiple roam request. */
        send_link_quality_stop_monitoring_req(context, getSmeContext(context)->linkQualityInstance);
        send_qos_disassociated_ind(context, getSmeContext(context)->qosInstance, ind->peerMacAddress, ResultCode_Success);

        /* Update the Scan result with -128 SNR and RSSI so we will not consider it for roaming except as a last resort */
        (void)srs_scan_result_quality_update(context, &cfg->connectionInfo.bssid, -128, -128);

        /* Tell network selector to roam as we lost the beacon */
        send_network_selector_roam_ind(context, getSmeContext(context)->networkSelectorInstance, unifi_RoamBeaconLost);
    }
}

/**
 * @brief
 *   network_selector_detach_req when connected
 *
 * @par Description
 *   The Network Selector has sent a Detach request. Shutdown the connection
 *   and release its resources.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void connected__ns_detach_req(FsmContext* context, const NetworkSelectorDetachReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CXN_MGR, "connected__ns_detach_req()"));

    fsmData->disconnectionReason = DISCONNECTION_DETACH;
    shutdown_security_manager(context);
    send_link_quality_stop_monitoring_req(context, getSmeContext(context)->linkQualityInstance);
    send_qos_disassociated_ind(context, getSmeContext(context)->qosInstance, cfg->connectionInfo.bssid, ResultCode_Success);
}


/**
 * @brief
 *   network_selector_roam_req when connected
 *
 * @par Description
 *   The Network Selector has sent a request to roam within the current ESS.
 *   Shutdown the current connection and release its resources.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void connected__ns_roam_req(FsmContext* context, const NetworkSelectorRoamReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CXN_MGR, "connected__ns_roam_req()"));

    fsmData->disconnectionReason = DISCONNECTION_ROAM;
    fsmData->skipResetOnError = req->skipResetOnError;
    fsmData->reassociate = req->reassociate;

    sme_trace_info((TR_CXN_MGR, "\"%s\" (%s) CONTROLLED PORT BLOCKED",
            trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
            trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    if (cfg->connectionInfo.authMode >= unifi_8021xAuthWPA)
    {
        (void)ns_port_configure(context, unifi_8021xPortClosedBlock, unifi_8021xPortClosedBlock);
    }
    else
    {
        (void)ns_port_configure(context, unifi_8021xPortOpen, unifi_8021xPortClosedBlock);
    }

    send_security_connect_stop_req(context, fsmData->securityManagerInstance);
    fsm_next_state(context, FSMSTATE_waitingForSecurityConnectStop);
    send_link_quality_stop_monitoring_req(context, getSmeContext(context)->linkQualityInstance);
    send_qos_disassociated_ind(context, getSmeContext(context)->qosInstance, cfg->connectionInfo.bssid, ResultCode_Success);
}


/* ========[ WAITING FOR SECURITY CONNECT STOP EVENT STATE HANDLERS ]======= */

/**
 * @brief
 *   Security Manager has been stopped, so it can be terminated
 *
 * @par Description
 *   The Security Manager is stopped so the final terminate request is sent
 *   to it to close down that instance.  Then the shutdown continues, either
 *   by deauthenticating from the peer network, or resetting UniFi if we had
 *   not authenticated.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_security_connect_stop__security_connect_stop_cfm(FsmContext* context, const SecurityConnectStopCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_debug_code(SmeConfigData* cfg = get_sme_config(context);)
    sme_trace_debug((TR_CXN_MGR, "\"%s\" (%s) Terminating Security Manager instance",
                                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    /* Terminate the security manager */
    send_security_terminate_req(context, fsmData->securityManagerInstance);

    sme_trace_debug((TR_CXN_MGR, "security_connect_stop_cfm(%d, %d)", fsmData->disconnectionReason, fsmData->skipResetOnError));

    if (fsmData->disconnectionReason == DISCONNECTION_ROAM)
    {
        /* When roaming in an ESS, we do not send a deauthenticate (which
         * would cause the AP to bin buffered data) or reset the UniFi
         * (which would cause it to forget the details of the previous
         * access point).  Nor do we terminate this instance of connection
         * manager, but we do go back to the start in order to connect to
         * the next access point.
         */
        reinitialise_connection_data(context);
        ns_roam_req(context);
    }
    else if (fsmData->disconnectionReason != DISCONNECTION_DETACH && fsmData->skipResetOnError)
    {
        if (fsmData->outstandingNSJoinCfm == TRUE)
        {
            /* We never reached the connected state so send a join_cfm with the failure status. */
            sme_trace_debug((TR_CXN_MGR, "security_connect_stop_cfm() :: never connected"));
            if (fsmData->connectionReason == CONNECTION_JOIN)
            {
                send_network_selector_join_cfm(context, getSmeContext(context)->networkSelectorInstance, unifi_Error);
            }
            else
            {
                send_network_selector_roam_cfm(context, getSmeContext(context)->networkSelectorInstance, unifi_Error);
            }

            fsmData->outstandingNSJoinCfm = FALSE;
        }
        fsm_next_state(context, FSMSTATE_waitingForNsTerminate);
    }
    else
    {
        /* If we have authenticated with the network, we need to deauthenticate
         * so that the other end can release resources.
         */
        if (fsmData->currentlyAuthenticated == TRUE)
        {
            do_deauth_req(context);
            return;
        }

        /* Otherwise, probably didn't get that far or we were using a basic
         * adhoc network.  Set UniFi back to a known state by resetting it.
         */
        reset_unifi(context);
    }
}

/* ================[ MLME DEAUTHENTICATE STATE EVENT HANDLERS ]============== */

/**
 * @brief
 *   Deauthentication has been completed by MLME
 *
 * @par Description
 *   A confirm to the request to deauthenticate the peer network has
 *   arrived.  In the case of a failure, there is not much that can be done.
 *   but the failure code is stored for the MMI if a failure is not already
 *   stored there.
 *
 *   A soft reset is sent to UniFi regardless, which shoudl restore it to a
 *   known state.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void waiting_for_mlme_deauthenticate__mlme_deauthenticate_cfm(FsmContext* context, const MlmeDeauthenticateCfm_Evt* cfm)
{
    sme_trace_entry((TR_CXN_MGR, "waiting_for_mlme_deauthenticate__mlme_deauthenticate_cfm(%s)", trace_ResultCode(cfm->resultCode)));

    if (cfm->resultCode != ResultCode_Success)
    {
        sme_trace_info_code(SmeConfigData* cfg = get_sme_config(context);)
        sme_trace_warn((TR_CXN_MGR, "\"%s\" (%s) MLME Deauthenticate Cfm failure(%s)",
                                    trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                                    trace_ResultCode(cfm->resultCode) ));
    }
    else
    {
        sme_trace_info_code(SmeConfigData* cfg = get_sme_config(context);)
        sme_trace_info((TR_CXN_MGR,"\"%s\" (%s) DEAUTENTICATED",
                         trace_unifi_SSID(&get_sme_config(context)->connectionInfo.ssid, getSSIDBuffer(context)),
                         trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)) ));
    }

    /* Set UniFi back to a known state by resetting it. */
    reset_unifi(context);
}

/**
 * @brief
 *  Acknowledges a NETWORK_SELECTOR_DETACH_REQ with success
 *
 * @par Description
 *  A NETWORK_SELECTOR_DETACH_REQ has arrived in a state when we are not
 *  actually connected to anything, so just agree that we are detached.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void bounce_detach(FsmContext *context, const NetworkSelectorDetachReq_Evt *req)
{
    sme_trace_entry((TR_CXN_MGR, "bounce_detach()"));
    send_network_selector_detach_cfm(context, getSmeContext(context)->networkSelectorInstance);
}

/**
 * @brief
 *   The order has come from Network Selector to terminate this instance
 *
 * @par Description
 *   Release any final resources, and take the next state to oblivion.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void waiting_for_ns_terminate__ns_terminate_req(FsmContext* context, const NetworkSelectorTerminateReq_Evt* req)
{
    sme_trace_entry((TR_CXN_MGR, "terminate"));

    send_coex_disconnected_ind(context, getSmeContext(context)->coexInstance);

    /* Deregister the connection manager with the hip proxy */
    hip_proxy_deregister_connection_manager(context, context->currentInstance->instanceId);

    /* This clears out the connection info data refs */
    reinitialise_connection_data(context);

    fsm_next_state(context, FSM_TERMINATE);
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/* FSM DEFINITION **********************************************/

#ifdef FSM_DEBUG_DUMP
/**
 * @brief
 *   Trace Dump Function Pointer
 *
 * @par Description
 *   Called when we want to trace the FSM
 *
 * @param[in]    context : FSM context
 * @param[in]    id      : fsm id
 *
 * @return
 *   void
 */
static void cm_fsm_dump(FsmContext* context, const CsrUint16 id)
{
    FsmData* fsmData = fsm_get_params_by_id(context, id, FsmData);
    sme_trace_crit_code(SmeConfigData* cfg = get_sme_config(context);)
    require(TR_NETWORK_SELECTOR_FSM, id < context->maxProcesses);
    require(TR_NETWORK_SELECTOR_FSM, context->instanceArray[id].fsmInfo->processId == CONNECTION_MANAGER_PROCESS);
    require(TR_NETWORK_SELECTOR_FSM, context->instanceArray[id].state != FSM_TERMINATE);

    sme_trace_crit((TR_FSM_DUMP, "   : bssid                          : %s",     trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));
    sme_trace_crit((TR_FSM_DUMP, "   : currentlyAuthenticated         : %s",     fsmData->currentlyAuthenticated?"True":"False"));
    sme_trace_crit((TR_FSM_DUMP, "   : beaconPeriodTu          : 0x%.4X", fsmData->beaconPeriodTu));
    sme_trace_crit((TR_FSM_DUMP, "   : hostAdhoc                      : %s",     fsmData->hostAdhoc?"True":"False"));
    sme_trace_crit((TR_FSM_DUMP, "   : securityManagerInstance        : 0x%.4X", fsmData->securityManagerInstance));
    sme_trace_crit((TR_FSM_DUMP, "   : beaconPeriodTu          : 0x%.4X", fsmData->beaconPeriodTu));
    sme_trace_crit((TR_FSM_DUMP, "   : authRetry                      : 0x%.2X", fsmData->authRetry));
    sme_trace_crit((TR_FSM_DUMP, "   : assocRetry                     : 0x%.2X", fsmData->assocRetry));
    sme_trace_crit((TR_FSM_DUMP, "   : outstandingNSJoinCfm           : %s",     fsmData->outstandingNSJoinCfm?"True":"False"));
    sme_trace_crit((TR_FSM_DUMP, "   : connectionReason               : 0x%.2X", fsmData->connectionReason));
    sme_trace_crit((TR_FSM_DUMP, "   : disconnectionReason            : 0x%.2X", fsmData->disconnectionReason));
}
#endif

static const FsmEventEntry waitingForNSJoinTransitions[] =
{
    fsm_event_table_entry(NETWORK_SELECTOR_JOIN_REQ_ID,         waiting_for_ns_join__ns_join_req),
    fsm_event_table_entry(NETWORK_SELECTOR_DETACH_REQ_ID,       fsm_error_event),
};

static const FsmEventEntry waitingForActiveScanProbeTransitions[] =
{
    fsm_event_table_entry(NETWORK_SELECTOR_DETACH_REQ_ID,       waiting_for_active_scan_probe__ns_detach_req),
    fsm_event_table_entry(SM_SCAN_CFM_ID,                       waiting_for_active_scan_probe__active_scan_probe_cfm),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                unexpected_event_indication),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           unexpected_event_indication),
};

static const FsmEventEntry waitingForSecurityJoinStartTransitions[] =
{
    fsm_event_table_entry(SECURITY_JOIN_START_CFM_ID,           waiting_for_security_join_start__security_join_start_cfm),
    fsm_event_table_entry(NETWORK_SELECTOR_DETACH_REQ_ID,       waiting_for_security_join_start__ns_detach_req),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                fsm_ignore_event),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           unexpected_event_indication),
};

static const FsmEventEntry waitingForMlmeJoinTransitions[] =
{
    fsm_event_table_entry(MLME_JOIN_CFM_ID,                     waiting_for_mlme_join__mlme_join_cfm),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                fsm_ignore_event), /* This is not unexpected Just ignore*/
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             unexpected_event_indication),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           unexpected_event_indication),
};

static const FsmEventEntry waitingForMlmeStartTransitions[] =
{
    fsm_event_table_entry(MLME_START_CFM_ID,                    waitingForMlmeStart__mlme_start_cfm),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                unexpected_event_indication),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             unexpected_event_indication),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           unexpected_event_indication),
};

static const FsmEventEntry waitingForGroupMlmeSetProtectionTransitions[] =
{
    fsm_event_table_entry(MLME_SETPROTECTION_CFM_ID,            waiting_for_group_mlme_set_protection__mlme_setprotection_cfm),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                unexpected_event_indication),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             unexpected_event_indication),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           unexpected_event_indication),
};

static const FsmEventEntry waitingForPairwiseMlmeSetProtectionTransitions[] =
{
    fsm_event_table_entry(MLME_SETPROTECTION_CFM_ID,            waiting_for_pairwise_mlme_set_protection__mlme_setprotection_cfm),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                unexpected_event_indication),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             unexpected_event_indication),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           unexpected_event_indication),
};

static const FsmEventEntry waitingForJoinMlmeDeauthenticateTransitions[] =
{
    fsm_event_table_entry(MLME_DEAUTHENTICATE_CFM_ID,           waiting_for_join_mlme_deauthenticate__mlme_deauthenticate_cfm),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             unexpected_event_indication),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           unexpected_event_indication),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                unexpected_event_indication),
};

static const FsmEventEntry waitingForMlmeAuthenticateTransitions[] =
{
    fsm_event_table_entry(MLME_AUTHENTICATE_CFM_ID,             waiting_for_mlme_authenticate__mlme_authenticate_cfm),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             unexpected_event_indication),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           unexpected_event_indication),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                unexpected_event_indication),
};

static const FsmEventEntry waitingForUnifiSysQoSControlTransitions[] =
{
    fsm_event_table_entry(UNIFI_SYS_QOS_CONTROL_CFM_ID,         waiting_for_unifi_qos_control_cfm),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             unexpected_event_indication),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           fsm_saved_event),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                unexpected_event_indication),
};

static const FsmEventEntry waitingForMlmeAssociateTransitions[] =
{
    fsm_event_table_entry(MLME_ASSOCIATE_CFM_ID,                waiting_for_mlme_associate__mlme_associate_cfm),
    fsm_event_table_entry(MLME_REASSOCIATE_CFM_ID,              waiting_for_mlme_reassociate__mlme_reassociate_cfm),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             unexpected_event_indication),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           waiting_for_mlme_associate__mlme_deauthenticate_ind),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                unexpected_event_indication),
};

static const FsmEventEntry waitingForSecurityConnectStartTransitions[] =
{
    fsm_event_table_entry(SECURITY_CONNECT_START_CFM_ID,        waiting_for_security_connect_start__security_connect_start_cfm),
    fsm_event_table_entry(SECURITY_DISCONNECTED_IND_ID,         unexpected_event_indication),
    fsm_event_table_entry(NETWORK_SELECTOR_DETACH_REQ_ID,       waiting_for_security_connect_start__ns_detach_req),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           waiting_for_security_connect_start__mlme_deauthenticate_ind),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             waiting_for_security_connect_start__mlme_disassociate_ind),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                waiting_for_security_connect_start__mlme_connected_ind),
};

static const FsmEventEntry connectedTransitions[] =
{
    fsm_event_table_entry(NETWORK_SELECTOR_DETACH_REQ_ID,       connected__ns_detach_req),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_REQ_ID,         connected__ns_roam_req),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                connected__mlme_connected_ind),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           connected__mlme_deauthenticate_ind),
    fsm_event_table_entry(MLME_DISASSOCIATE_IND_ID,             connected__mlme_disassociate_ind),
    fsm_event_table_entry(SECURITY_DISCONNECTED_IND_ID,         connected__security_disconnected_ind),
};

static const FsmEventEntry waitingForSecurityConnectStopTransitions[] =
{
    fsm_event_table_entry(SECURITY_CONNECT_STOP_CFM_ID,         waiting_for_security_connect_stop__security_connect_stop_cfm),
    /* SECURITY_JOIN_START_CFM_ID may occur if a ns_detach_req received in
     * FSMSTATE_waitingForSecurityJoinStart.  No action is needed, we just
     * continue to wait for the _stop_cfm.
     */
    fsm_event_table_entry(SECURITY_JOIN_START_CFM_ID,           cleanup_security_join_start_cfm),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                fsm_ignore_event),
    /* MLME_DEAUTHENTICATE_IND can be ignored as we are shutting down anyway */
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           fsm_ignore_event),
    fsm_event_table_entry(MLME_ASSOCIATE_CFM_ID,                fsm_ignore_event),
    fsm_event_table_entry(MLME_REASSOCIATE_CFM_ID,              fsm_ignore_event),
};

static const FsmEventEntry waitingForMlmeDeauthenticateTransitions[] =
{
    fsm_event_table_entry(MLME_DEAUTHENTICATE_CFM_ID,           waiting_for_mlme_deauthenticate__mlme_deauthenticate_cfm),
    /* Can happen if deauthentication_ind received during association */
    fsm_event_table_entry(MLME_ASSOCIATE_CFM_ID,                fsm_ignore_event),
    /* The Ind could happen in parallel with the deauthenticate request
     * going out to the network.  As we are guaranteed a deauth_cfm, and are
     * already on the shutdown path, we can ignore the ind and carry on
     * waiting.
     */
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           fsm_ignore_event),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                fsm_ignore_event),

    fsm_event_table_entry(MLME_REASSOCIATE_CFM_ID,              fsm_ignore_event),
};

/** Event handling in waiting_for_unifi_reset while waiting for unifi_reset_cfm */
static const FsmEventEntry waitingForUnifiResetTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_RESET_CFM_ID,                   waiting_for_unifi_reset__unifi_reset_cfm),
    /* MLME_CONNECTED/DEAUTHENTICATE_IND irrelevant at this stage at the
     * UniFi is about to be reset
     */
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                fsm_ignore_event),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,           fsm_ignore_event),
    fsm_event_table_entry(MLME_REASSOCIATE_CFM_ID,              fsm_ignore_event),
};

static const FsmEventEntry waitingForTerminateTransitions[] =
{
    fsm_event_table_entry(NETWORK_SELECTOR_DETACH_REQ_ID,       bounce_detach),
    fsm_event_table_entry(NETWORK_SELECTOR_TERMINATE_REQ_ID,    waiting_for_ns_terminate__ns_terminate_req ),
};

/*
 * Standard handlers if the event is not handled in the current state
 */
static const FsmEventEntry unhandledTransitions[] =
{
    fsm_event_table_entry(NETWORK_SELECTOR_DETACH_REQ_ID,       set_detach_flag),
    fsm_event_table_entry(UNIFI_SYS_QOS_CONTROL_CFM_ID,         fsm_ignore_event),
};

/** Connection Manager State Machine Definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
                         /* Signal Id,                                  Function                                    Save? */
   fsm_state_table_entry(FSMSTATE_waitingForNSJoin,                     waitingForNSJoinTransitions,                    FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForActiveScanProbe,            waitingForActiveScanProbeTransitions,           FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForSecurityJoinStart,          waitingForSecurityJoinStartTransitions,         FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForMlmeJoin,                   waitingForMlmeJoinTransitions,                  FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForMlmeStart,                  waitingForMlmeStartTransitions,                 FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForGroupMlmeSetProtection,     waitingForGroupMlmeSetProtectionTransitions,    FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForPairwiseMlmeSetProtection,  waitingForPairwiseMlmeSetProtectionTransitions, FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForJoinMlmeDeauthenticate,     waitingForJoinMlmeDeauthenticateTransitions,    FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForMlmeAuthenticate,           waitingForMlmeAuthenticateTransitions,          FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForUnifiSysQoSControl,         waitingForUnifiSysQoSControlTransitions,        FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForMlmeAssociate,              waitingForMlmeAssociateTransitions,             FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForSecurityConnectStart,       waitingForSecurityConnectStartTransitions,      FALSE),
   fsm_state_table_entry(FSMSTATE_connected,                            connectedTransitions,                           FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForSecurityConnectStop,        waitingForSecurityConnectStopTransitions,       FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForMlmeDeauthenticate,         waitingForMlmeDeauthenticateTransitions,        FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForUnifiReset,                 waitingForUnifiResetTransitions,                FALSE),
   fsm_state_table_entry(FSMSTATE_waitingForNsTerminate,                waitingForTerminateTransitions,                 FALSE),
};

const FsmProcessStateMachine connection_manager_fsm = {
#ifdef FSM_DEBUG
       "Connection Mgr",                                                       /* SM Process Name   */
#endif
       CONNECTION_MANAGER_PROCESS,                                             /* SM Process ID     */
       {FSMSTATE_MAX_STATE, stateTable},                                       /* Transition Tables */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE), /* Event handlers    */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                  /* Ignore Event handers */
       init_connection_mgr_Fsm,                                                /* Entry Function    */
       NULL,                                                                   /* Reset Function    */
#ifdef FSM_DEBUG_DUMP
       cm_fsm_dump                                                             /* Trace Dump Function   */
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

