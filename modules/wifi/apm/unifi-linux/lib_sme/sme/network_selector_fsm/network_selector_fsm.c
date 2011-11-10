/** @file network_selector_fsm.c
 *
 * Network Selector FSM Implementation
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
 *   This file implements the Network Selector Process of the UniFi Station
 *   Management Entity.  It selects and switches between wireless networks,
 *   either under direct control of the user.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/network_selector_fsm/network_selector_fsm.c#10 $
 *
 ****************************************************************************/

/** @{
 * @ingroup sme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "network_selector_fsm/network_selector_fsm.h"
#include "connection_manager_fsm/connection_manager_fsm.h"
#include "ap_utils/ap_validation.h"
#include "unifi_driver_fsm/unifi_driver_fsm.h"

#include "scan_manager_fsm/scan_results_storage.h"
#include "power_manager_fsm/power_manager_fsm.h"

#include "mgt_sap/mgt_sap_from_sme_interface.h"
#include "sys_sap/sys_sap_from_sme_interface.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "ie_access/ie_access.h"

#include "smeio/smeio_trace_types.h"

#ifdef CSR_AMP_ENABLE
#include "pal_manager/pal_manager.h"
#endif

#ifdef CCX_VARIANT
#include "ccx_fsm/ccx_fsm.h"
#include "ccx_enhanced_roaming/ccx_enhanced_roaming.h"
#endif

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Processes Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, FsmData))

#define MAX_CONNECT_ADDRESSES 10

/* Raom limit defines */
#define ROAM_LIMIT_TIMEFRAME_MS 60000
#define ROAM_LIMIT_COUNT        5


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
    FSMSTATE_waiting_for_startup,
    FSMSTATE_disconnected,
    FSMSTATE_scanning_for_network,
    FSMSTATE_searching_for_ibss,
    FSMSTATE_connecting,
    FSMSTATE_connected,
    FSMSTATE_disconnecting,
    FSMSTATE_MAX_STATE
} FsmState;

    CsrUint8                 channelNumber;
    srs_IECapabilityMask  ieCapabilityFlags;

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData
{
    CsrUint16 connectionInstance;

    /** currentSignal stores the latest link quality information for use
     * when comparing against a preferred network suggestion. */
    unifi_BasicUsability currentUsability;

    /** The number of join_req that have been received, so that we ensure we
     * send a join_cfm for each of them. */
    CsrBool mmiJoinRequested;

    /** The number of disconnect_req that have been received, so that we
     * ensure we send a disconnect_cfm for each of them. */
    CsrBool mmiDisconnectRequested;

    /** Record of the last media connection status sent to the APP SAP, so
     * that there will never be two consecutive events that are the same. */
    unifi_MediaStatus  mediaConnectionStatus;

    /* Have we detected Dynamic Wep? e.g PEAP via Eapol but no WPA/WPA2 */
    CsrBool dynamicWepKeys;

    CsrBool roaming;
    CsrBool roamingScan;
    CsrBool mgtRoamStartOutstanding;
    CsrUint8 numConnectAddresses;
    connectAddressesData connectAddresses[MAX_CONNECT_ADDRESSES];
    CsrUint8 currentConnectAddress;

    /* Number of times connected to the same AP */
    CsrUint8 roamConnectCount;

    CsrBool hostingadhoc;

    unifi_PortAction controlledPortState;
    unifi_PortAction uncontrolledPortState;

#ifdef CCX_VARIANT
    unifi_RoamReason associateReason;
    /* this stores the details of the last successful connection during a roam */
    JoinedAddress *lastSuccessfulRoamConnection;
#endif

    CsrUint8   adhocJoinScanAttempts; /* Number of Scans for a the current join only IBSS connect*/
    FsmTimerId adhocJoinScanTimerId;

    void* cfmAppHandle;
} FsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   Resets the FSM data back to sane initial values.  Valid to call on
 *   startup when all data is in an unknown state, and also for subsequent
 *   resets from initiated from any appropriate FSM state.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void initDataNetworkSelector(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "initDataNetworkSelector"));

    fsmData->connectionInstance = FSM_TERMINATE;
    fsmData->mediaConnectionStatus = unifi_MediaDisconnected;
    fsmData->mmiJoinRequested = FALSE;
    fsmData->mmiDisconnectRequested = FALSE;
    fsmData->currentUsability = unifi_NotConnected;
    fsmData->numConnectAddresses = 0;
    fsmData->currentConnectAddress = 0;
    fsmData->hostingadhoc = FALSE;
    fsmData->adhocJoinScanAttempts = 0;
    fsmData->adhocJoinScanTimerId.uniqueid = 0;

    fsmData->controlledPortState = unifi_8021xPortClosedDiscard;
    fsmData->uncontrolledPortState = unifi_8021xPortClosedDiscard;

    fsmData->mgtRoamStartOutstanding = FALSE;

#ifdef CCX_VARIANT
    fsmData->lastSuccessfulRoamConnection = NULL;
#endif
}

/**
 * @brief
 *   FSM initialisation
 *
 * @par Description
 *   Allocates and initialised the FSM data.  Should only be called once, at
 *   startup.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void initFsmNetworkSelector(FsmContext* context)
{
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "initFsmNetworkSelector()"));

    fsm_create_params(context, FsmData);

    initDataNetworkSelector(context);

    fsm_next_state(context, FSMSTATE_waiting_for_startup);
}

/**
 * @brief
 *   FSM initialisation
 *
 * @par Description
 *   Allocates and initialised the FSM data.  Should only be called once, at
 *   startup.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static srs_IECapabilityMask calc_join_ie_capabilites(
        FsmContext* context,
        SmeConfigData* cfg,
        srs_IECapabilityMask ieApCapabilityFlags)
{
    srs_IECapabilityMask mask = NONE_Capable;

    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "initFsmNetworkSelector()"));
    /* determine WMM capability */

    if ( (cfg->smeConfig.wmmModeMask !=  unifi_WmmDisabled) /* user */
       &&(ieApCapabilityFlags & WMM_Capable))                            /* AP   */
    {
        mask |= WMM_Capable;
    }

    /* determine WMM UAPSD capability */
    if ( (cfg->smeConfig.wmmModeMask & unifi_WmmPSEnabled) /* user */
       &&(ie_wmm_sme_uapsd_enable(cfg->connectionConfig.wmmQosInfo))     /* user */
       &&(ieApCapabilityFlags & WMMUAPSD_Capable)                        /* AP   */
       )
    {
        mask |= WMMUAPSD_Capable;
    }

    /* determine CCX capability */
#ifdef CSR_WIFI_CCX_ENABLE
    if (ieApCapabilityFlags & CCX_Capable)                               /* AP   */
    {
        mask |= CCX_Capable;
    }
#endif

    /* determine WPS capability */
    if (ieApCapabilityFlags & WPS_Capable)                               /* AP   */
    {
        mask |= WPS_Capable;
    }

    /* determine Pre-Authentication capability */
    if (ieApCapabilityFlags & PRE_AUTH_Capable)                          /* AP   */
    {
        mask |= PRE_AUTH_Capable;
    }

    /* determine DOT11N capability */
    if ( (cfg->highThroughputOptionEnabled)                              /* user */
       &&(ieApCapabilityFlags & DOT11N_Capable))                         /* AP   */
    {
        mask |= DOT11N_Capable;
    }

    return mask;
}

/**
 * @brief Fills in the connect bssid list
 *
 * @return
 *   void
 */
static void build_network_join_list(FsmContext* context, CsrBool beaconLost)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    srs_scan_data *currentScan;

    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "build_network_join_list()"));

    fsmData->numConnectAddresses = 0;
    fsmData->currentConnectAddress = 0;

    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "build_network_join_list(%s, %s)",
                     trace_unifi_MACAddress(cfg->connectionConfig.bssid, getMacAddressBuffer(context)),
                     trace_unifi_SSID(&cfg->connectionConfig.ssid, getSSIDBuffer(context)) ));

    /* Loop through All the scan results and build the list fron validated Accesspoints */
    currentScan = srs_get_scan_parameters_first(context,
                                                &cfg->connectionConfig.ssid,
                                                &cfg->connectionConfig.bssid);

    while (currentScan)
    {
        if (cfg->roamingConfig.reconnectLimit != 0 &&
            fsmData->roamConnectCount == cfg->roamingConfig.reconnectLimit &&
            CsrMemCmp(&currentScan->scanResult.bssid, &cfg->connectionInfo.bssid, sizeof(currentScan->scanResult.bssid.data)) == 0)
        {
            sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "build_network_join_list(%s, %s) Reconnection Limit reached.",
                             trace_unifi_MACAddress(cfg->connectionConfig.bssid, getMacAddressBuffer(context)),
                             trace_unifi_SSID(&cfg->connectionConfig.ssid, getSSIDBuffer(context)) ));
        }
        else if (validate_ap(context, currentScan, cfg))
        {
            if (!beaconLost ||
                fsm_current_state(context) != FSMSTATE_connected ||
                CsrMemCmp(&currentScan->scanResult.bssid, &cfg->connectionInfo.bssid, sizeof(currentScan->scanResult.bssid.data)) != 0)
            {
                if ( cfg->connectionConfig.ssid.length==0)
                {
                    fsmData->connectAddresses[fsmData->numConnectAddresses].ssid = currentScan->scanResult.ssid;
                }
                else
                {
                    fsmData->connectAddresses[fsmData->numConnectAddresses].ssid = cfg->connectionConfig.ssid;
                }

                fsmData->connectAddresses[fsmData->numConnectAddresses].bssid = currentScan->scanResult.bssid;
                fsmData->connectAddresses[fsmData->numConnectAddresses].channelNumber = currentScan->scanResult.channelNumber;
                fsmData->connectAddresses[fsmData->numConnectAddresses].joinIeCapabilityFlags = calc_join_ie_capabilites(context, cfg, currentScan->ieCapabilityFlags);
#ifdef CCX_VARIANT
                fsmData->connectAddresses[fsmData->numConnectAddresses].rssi = currentScan->scanResult.rssi ;
#endif
                fsmData->numConnectAddresses++;

                if (fsmData->numConnectAddresses == MAX_CONNECT_ADDRESSES)
                {
                    break;
                }
            }
        }

        currentScan = srs_get_scan_parameters_next(context, &currentScan->scanResult.bssid,
                                                            &cfg->connectionConfig.ssid,
                                                            &cfg->connectionConfig.bssid);
    }

#ifdef CCX_VARIANT

    /*  Take Neighbour's list membership into account */
    ccx_reh_improve_join_list_with_neighbours( context,
                                               fsmData->numConnectAddresses,
                                               fsmData->connectAddresses );
#endif

    /* Add the current connected as the last 1 in the list */
    if (beaconLost &&
        fsm_current_state(context) == FSMSTATE_connected &&
        fsmData->numConnectAddresses != MAX_CONNECT_ADDRESSES &&
        (cfg->roamingConfig.reconnectLimit == 0 || fsmData->roamConnectCount != cfg->roamingConfig.reconnectLimit))
    {
        fsmData->connectAddresses[fsmData->numConnectAddresses].bssid = cfg->connectionInfo.bssid;
        if ( (currentScan != NULL)||(cfg->connectionConfig.ssid.length==0))
        {
            fsmData->connectAddresses[fsmData->numConnectAddresses].ssid = cfg->connectionInfo.ssid;
        }
        else
        {
            fsmData->connectAddresses[fsmData->numConnectAddresses].ssid = cfg->connectionInfo.ssid;
        }
        fsmData->connectAddresses[fsmData->numConnectAddresses].channelNumber = cfg->connectionInfo.channelNumber;
        fsmData->connectAddresses[fsmData->numConnectAddresses].joinIeCapabilityFlags = cfg->joinIECapabilities;
        fsmData->numConnectAddresses++;
    }

    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "fsmData->numConnectAddresses %d", fsmData->numConnectAddresses));
}

/**
 * @brief
 *   Process has been ordered to start.
 *
 * @par Description
 *   A CORE_START_REQ has been received.  Ensure data is set to known values
 *   then move to waiting_for_mmi_radio_on state.
 *
 * @param[in]    context : FSM context
 * @param[in]    req     : content of the CoreStopReq
 *
 * @return
 *  Void
 */
static void stopped__start(FsmContext *context, const CoreStartReq_Evt *req)
{
    initDataNetworkSelector(context);
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "stopped__start()"));
    send_core_start_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_disconnected);
}

/**
 * @brief
 *   A CORE_STOP_REQ was received while connected
 *
 * @param[in]    context            : FSM context
 * @param[in]    request            : content of the CoreStopReq
 *
 * @return
 *  Void
 */
static void connected__core_stop(FsmContext *context, const CoreStopReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "connected__core_stop()"));

    send_network_selector_detach_req(context, fsmData->connectionInstance);
    fsm_next_state(context, FSMSTATE_disconnecting);

    /* This Saves the event for later processing  (After we disconnect)*/
    fsm_saved_event(context, (const FsmEvent*)req);
}

/**
 * @brief
 *   Scans all channels for a bssid or ssid
 *
 * @param[in] context   : FSM context
 *
 * @return
 *  void
 */
static void do_prejoin_scan(FsmContext *context)
{
    FsmData* fsmData = FSMDATA;
    unifi_ScanStopCondition scanStopCondition = unifi_ScanStopAllResults;
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint32 now = fsm_get_time_of_day_ms(context);
	unifi_SSID* ssid = NULL;

    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "do_prejoin_scan(%s, %s)",
                     trace_unifi_MACAddress(cfg->connectionConfig.bssid, getMacAddressBuffer(context)),
                     trace_unifi_SSID(&cfg->connectionConfig.ssid, getSSIDBuffer(context))));


    if (fsmData->roamingScan)
    {
        /* Find a good roaming candidate */
        scanStopCondition = unifi_ScanStopSatisfactoryRoam;
    }

    if (CsrMemCmp( &BroadcastBssid, &cfg->connectionConfig.bssid, sizeof(cfg->connectionConfig.bssid.data)) != 0)
    {
        /* Fixed BSSID so only 1 result possible */
        scanStopCondition = unifi_ScanStopFirstResult;
    }

    if (cfg->connectionConfig.ssid.length)
    {
    	ssid = (unifi_SSID*)CsrPmalloc(sizeof(unifi_SSID));
    	ssid->length = cfg->connectionConfig.ssid.length;
    	CsrMemCpy(ssid->ssid, cfg->connectionConfig.ssid.ssid, ssid->length);
    }

    /* cancel any other Scans as this takes precedence */
    send_sm_scan_cancel_ind(context, getSmeContext(context)->scanManagerInstance, now, TRUE);
    send_sm_scan_req(context, getSmeContext(context)->scanManagerInstance, now,
                     0, /* channel 0 = All channels */
                     (cfg->connectionConfig.ssid.length == 0?0:1),
                     ssid,
                     cfg->connectionConfig.bssid,
                     TRUE,  /* probe req */
                     FALSE,
                     FALSE,
                     scanStopCondition,
                     cfg->connectionConfig.bssType,
                     0);
}

/**
 * @brief
 *   While connected, Link Quality has decided we should roam.
 *   Or Connection Manager has lost the beacon
 *
 * @param[in]    context : FSM context
 * @param[in]    ind     : Event
 *
 * @return
 *  void
 */
static void connected__roam(FsmContext *context, const NetworkSelectorRoamInd_Evt *ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    sme_trace_info((TR_NETWORK_SELECTOR_FSM, "connected__roam()"));

    /* log the disconnect time */
    cfg->lastJoinedAddresses[0].connDropTimeStamp  = now;

    build_network_join_list(context, (CsrBool)(ind->reason == unifi_RoamBeaconLost));

    /* Roam request with only 0 or 1 (The current AP) candidates.
     * We need to scan for alternatives */
    if (fsmData->roamingScan == FALSE && fsmData->numConnectAddresses <= 1)
    {
        sme_trace_info((TR_NETWORK_SELECTOR_FSM, "connected__roam() :: No roaming candidates. Scanning!"));
        fsmData->roamingScan = TRUE;
        do_prejoin_scan(context);
        fsm_next_state(context, FSMSTATE_scanning_for_network);

        /* This Saves the event for later processing  (After we have scanned) */
        fsm_saved_event(context, (const FsmEvent*)ind);
        return;
    }
    fsmData->roamingScan = FALSE;

    /* Can have a case where there are no candidates (if the current AP is blacklisted) */
    if (fsmData->numConnectAddresses == 0)
    {
        sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "connected__roam() :: No valid roam candidates... Disconnecting"));
        send_sm_pause_req(context, getSmeContext(context)->scanManagerInstance);
        send_network_selector_detach_req(context, fsmData->connectionInstance);
        fsm_next_state(context, FSMSTATE_disconnecting);
        return;
    }

#ifdef CCX_VARIANT
    fsmData->associateReason= ind->reason;
#endif

    cfg->connectionInfo.bssid = fsmData->connectAddresses[0].bssid;
    cfg->connectionInfo.assocReqApAddress = fsmData->connectAddresses[0].bssid;
    cfg->connectionInfo.ssid = fsmData->connectAddresses[0].ssid;
    cfg->connectionInfo.channelNumber = fsmData->connectAddresses[0].channelNumber;
    cfg->joinIECapabilities = fsmData->connectAddresses[0].joinIeCapabilityFlags;

    fsmData->roaming = (ind->reason != unifi_RoamDeauthenticated && ind->reason != unifi_RoamDisassociated);
    /* Tell the connection manager to connect to the network */
    send_sm_pause_req(context, getSmeContext(context)->scanManagerInstance);
    send_network_selector_roam_req(context,
                                   fsmData->connectionInstance,
                                   fsmData->roaming,
                                   (fsmData->numConnectAddresses != 1));

    /* Indicate Roaming attempt to user */
    {
    void* appHandles;
    CsrUint16 appHandleCount = get_appHandles(context, unifi_IndRoamStart, &appHandles);
    if (appHandleCount)
    {
        call_unifi_mgt_roam_start_ind(context, appHandleCount, appHandles, ind->reason, cfg->deauthReason);
    }
    }
    fsmData->mgtRoamStartOutstanding = TRUE;

    /* Indicate connection attempt to user */
    if (cfg->connectionConfig.bssType == unifi_Infrastructure)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndAssociationStart, &appHandles);
        if (appHandleCount)
        {
            call_unifi_mgt_association_start_ind(context, appHandleCount, appHandles, &cfg->connectionInfo.bssid, &cfg->connectionInfo.ssid);
        }
    }

    fsm_next_state(context, FSMSTATE_connecting);
}

/**
 * @brief
 *   While not connected to a network, the SME is being stopped
 *
 * @par Description
 *   The MMI has sent a CORE_STOP_REQ while we are in the disconnected
 *   state.  We disable the scan manager before heading for the
 *   waiting_for_startup state.
 *
 * @param[in]    context            : FSM context
 *
 * @return
 *  Void
 */
static void disconnected__core_stop(FsmContext *context, const CoreStopReq_Evt *req)
{
    sme_trace_info((TR_NETWORK_SELECTOR_FSM, "disconnected__core_stop()"));
    send_core_stop_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_waiting_for_startup);
}

/**
 * @brief
 *   While not connected to a network, a disconnect request was received
 *
 * @par Description
 *   As we are already disconnected, just confirm it as successful
 *
 * @param[in]    context            : FSM context
 *
 * @return
 *  Void
 */
static void disconnected__disconnect(FsmContext *context, const UnifiMgtDisconnectReq_Evt *req)
{
    unifi_Status status = unifi_Success;

    sme_trace_info((TR_NETWORK_SELECTOR_FSM, "disconnected__disconnect()"));
    if (isAccessRestricted(context, req->appHandle))
    {
        sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "disconnected__disconnect() unifi_Restricted"));
        status = unifi_Restricted;
    }

    call_unifi_mgt_disconnect_cfm(context, req->appHandle, status);
}

/**
 * @brief
 *   Pass a request to Join a network on to the Connection Manager
 *
 * @par Description
 *   See brief.
 *
 * @param[in] context : FSM context
 * @param[in] req     : network to join
 *
 * @return
 *  Void
 */
static void disconnected__connect(FsmContext *context, const UnifiMgtConnectReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "disconnected__connect(%s : %s)",
                        trace_unifi_MACAddress(req->connectionConfig.bssid, getMacAddressBuffer(context)),
                        trace_unifi_SSID(&req->connectionConfig.ssid, getSSIDBuffer(context))));

    if (isAccessRestricted(context, req->appHandle))
    {
        sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "disconnected__connect() unifi_Restricted"));
        call_unifi_mgt_connect_cfm(context, req->appHandle, unifi_Restricted);
        return;
    }

    if (req->connectionConfig.ssid.length > sizeof(req->connectionConfig.ssid.ssid))
    {
        sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "disconnected__connect() SSID len too large"));
        call_unifi_mgt_connect_cfm(context, req->appHandle, unifi_InvalidParameter);
        return;
    }

#ifdef CSR_AMP_ENABLE
    if (pal_amp_connection_in_progress(context))
    {
        sme_trace_info((TR_NETWORK_SELECTOR_FSM, "disconnected__connect() Rejecting connection request as AMP PAL is in use"));
        call_unifi_mgt_connect_cfm(context, req->appHandle, unifi_Unavailable);
        return;
    }
    else
    {
        pal_sme_connection_status_change_request(context,PAL_SME_CONNECTION_STATUS_CONNECTED_FULL_CAPACITY);
    }
#endif

    fsmData->cfmAppHandle = req->appHandle;

    fsmData->hostingadhoc = FALSE;
    fsmData->roaming = FALSE;
    fsmData->roamingScan = FALSE;
    fsmData->dynamicWepKeys = FALSE;
    fsmData->roamConnectCount = 0;

#ifdef CCX_VARIANT
    fsmData->lastSuccessfulRoamConnection = NULL;
#endif

    /* Free Old data */
    CsrPfree(cfg->connectionConfig.mlmeAssociateReqInformationElements);

    cfg->connectionConfig = req->connectionConfig;

    build_network_join_list(context, FALSE);

    /* First Time Around the loop
     * If Not scan data the do a scan */
    if (fsmData->numConnectAddresses == 0 && fsmData->mmiJoinRequested == FALSE)
    {
        fsmData->mmiJoinRequested = TRUE;
        do_prejoin_scan(context);
        fsm_next_state(context, FSMSTATE_scanning_for_network);

        /* Clear the info pointer pointer on save as we do not want
         * it to be deleted when this function is run again after saving */
        cfg->connectionConfig.mlmeAssociateReqInformationElementsLength = 0;
        cfg->connectionConfig.mlmeAssociateReqInformationElements = NULL;

        /* This Saves the event for later processing  (After we have scanned) */
        fsm_saved_event(context, (const FsmEvent*)req);
        return;
    }

    fsmData->mmiJoinRequested = TRUE;

    /* Second Time Around the loop
     * If still no scan data then
     * Infrastructure   : cfm Error
     * Adhoc            : Host */
    if (fsmData->numConnectAddresses == 0)
    {
        /* only if we are in adhoc and have a specific SSID can we host */
        if (cfg->connectionConfig.bssType == unifi_Adhoc &&
            cfg->connectionConfig.adhocJoinOnly == FALSE &&
            cfg->connectionConfig.ssid.length != 0 )
        {
            fsmData->hostingadhoc = TRUE;
            cfg->joinIECapabilities = 0;
            /* create a new connection mgr instance for this connection */
            fsmData->connectionInstance = fsm_add_instance(context, &connection_manager_fsm, TRUE);

            /* Tell the connection manager to connect to the network */
            send_sm_pause_req(context, getSmeContext(context)->scanManagerInstance);

            /* Tell the connection manager to connect to the network */
            send_network_selector_join_req(context, fsmData->connectionInstance, FALSE, TRUE, TRUE);
            fsm_next_state(context, FSMSTATE_connecting);
        }
        else if (cfg->connectionConfig.bssType == unifi_Adhoc &&
                 cfg->connectionConfig.adhocJoinOnly == TRUE &&
                 (cfg->adHocConfig.joinOnlyAttempts == 0 || cfg->adHocConfig.joinOnlyAttempts > 1))
        {
            /* Start periodically scanning for the IBSS */
            fsmData->adhocJoinScanAttempts = 1;
            send_network_selector_adhoc_join_scan_timer(context, fsmData->adhocJoinScanTimerId, cfg->adHocConfig.joinAttemptIntervalMs, 100);

            fsm_next_state(context, FSMSTATE_searching_for_ibss);
            return;
        }
        else
        {
            fsmData->mmiJoinRequested = FALSE;
            call_unifi_mgt_connect_cfm(context, req->appHandle, unifi_NotFound);
#ifdef CSR_AMP_ENABLE
            pal_sme_connection_status_change_request(context,PAL_SME_CONNECTION_STATUS_DISCONNECTED);
#endif

        }
        return;
    }

    cfg->connectionInfo.bssid = fsmData->connectAddresses[0].bssid;
    cfg->connectionInfo.assocReqApAddress = fsmData->connectAddresses[0].bssid;
    cfg->connectionInfo.ssid = fsmData->connectAddresses[0].ssid;
    cfg->connectionInfo.channelNumber = fsmData->connectAddresses[0].channelNumber;
    cfg->joinIECapabilities = fsmData->connectAddresses[0].joinIeCapabilityFlags;

    /* create a new connection mgr instance for this connection */
    fsmData->connectionInstance = fsm_add_instance(context, &connection_manager_fsm, TRUE);

    /* Tell the connection manager to connect to the network */
    send_sm_pause_req(context, getSmeContext(context)->scanManagerInstance);
    send_network_selector_join_req(context,
                                   fsmData->connectionInstance,
                                   FALSE, /* Not a reassociate */
                                   (fsmData->numConnectAddresses != 1), /* Only cleanup on error when last AP to try */
                                   FALSE); /* Not host Adhoc */

    /* Indicate connection attempt to user */
    if (cfg->connectionConfig.bssType == unifi_Infrastructure)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndAssociationStart, &appHandles);
        if (appHandleCount)
        {
            call_unifi_mgt_association_start_ind(context, appHandleCount, appHandles, &cfg->connectionInfo.bssid, &cfg->connectionInfo.ssid);
        }
    }

    fsm_next_state(context, FSMSTATE_connecting);
}

/**
 * @brief
 *   While connected to a network, disconnection is requested
 *
 * @par Description
 *   The MMI has requested disconnection while we are in the connected state.
 *   We need to commence shutdown by closing down the IP stack.
 *
 * @param[in]    context            : FSM context
 *
 * @return
 *  Void
 */
static void connected__disconnect(FsmContext *context, const UnifiMgtDisconnectReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_info((TR_NETWORK_SELECTOR_FSM, "connected__disconnect()"));

    verify(TR_NETWORK_SELECTOR_FSM, fsmData->mmiDisconnectRequested == FALSE);

    if (isAccessRestricted(context, req->appHandle))
    {
        sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "connected__disconnect() unifi_Restricted"));
        call_unifi_mgt_disconnect_cfm(context, req->appHandle, unifi_Restricted);
        return;
    }

    fsmData->cfmAppHandle = req->appHandle;
    fsmData->mmiDisconnectRequested = TRUE;
    send_sm_pause_req(context, getSmeContext(context)->scanManagerInstance);
    send_network_selector_detach_req(context, fsmData->connectionInstance);
    fsm_next_state(context, FSMSTATE_disconnecting);
}

/**
 * @brief
 *   Bounce inappropriate unifi_mgt_disconnect_req
 *
 * @par Description
 *   See brief
 *
 * @param[in] context : FSM context
 *
 * @return
 *  Void
 */
static void stopped__disconnect(FsmContext *context, const UnifiMgtDisconnectReq_Evt * req)
{
    sme_trace_error((TR_NETWORK_SELECTOR_FSM, "stopped__disconnect: inappropriate unifi_mgt_disconnect_req"));
    call_unifi_mgt_disconnect_cfm(context, req->appHandle, unifi_Error);
}

/**
 * @brief
 *   Connection manager disconnected so continue shutdown
 *
 * @par Description
 *   The network selector has been asked to turn off the radio, and it sent
 *   a request to the connection manager to disconnect prior to this.
 *   The connection manager has responded, so the connection manager
 *   instance can now be terminated before continuing to tidy things up.
 *
 * @param[in] context : FSM context
 *
 * @return
 *  Void
 */
static void disconnecting__disconnected(FsmContext *context, const NetworkSelectorDetachCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint32 now = fsm_get_time_of_day_ms(context);
    sme_trace_info((TR_NETWORK_SELECTOR_FSM, "disconnecting__disconnected()"));

    /* log the disconnect time */
    cfg->lastJoinedAddresses[0].connDropTimeStamp  = now;

    send_network_selector_terminate_req(context, fsmData->connectionInstance);

    /* Connection Complete release instance */
    fsmData->connectionInstance = FSM_TERMINATE;

    if (fsmData->mmiJoinRequested)
    {
        /* Indicate connection complete */
        if (cfg->connectionConfig.bssType == unifi_Infrastructure)
        {
            void* appHandles;
            CsrUint16 appHandleCount = get_appHandles(context, unifi_IndAssociationComplete, &appHandles);
            if (appHandleCount)
            {
                call_unifi_mgt_association_complete_ind(context, appHandleCount, appHandles, unifi_Error, &cfg->connectionInfo, cfg->deauthReason);
            }
        }

        fsmData->mmiJoinRequested= FALSE;
        call_unifi_mgt_connect_cfm(context, fsmData->cfmAppHandle, unifi_Error);
    }

    if (fsmData->mgtRoamStartOutstanding)
    {
        if (cfg->connectionConfig.bssType == unifi_Infrastructure)
        {
            void* appHandles;
            CsrUint16 appHandleCount = get_appHandles(context, unifi_IndAssociationComplete, &appHandles);
            if (appHandleCount)
            {
                call_unifi_mgt_association_complete_ind(context, appHandleCount, appHandles, unifi_Error, &cfg->connectionInfo, cfg->deauthReason);
            }
            appHandleCount = get_appHandles(context, unifi_IndRoamComplete, &appHandles);
            if (appHandleCount)
            {
                call_unifi_mgt_roam_complete_ind(context, appHandleCount, appHandles, unifi_Error);
            }
            fsmData->mgtRoamStartOutstanding = FALSE;
        }
    }

    ns_set_media_connection_status(context, unifi_MediaDisconnected);
    if (fsmData->mmiDisconnectRequested)
    {
        fsmData->mmiDisconnectRequested= FALSE;
        call_unifi_mgt_disconnect_cfm(context, fsmData->cfmAppHandle, unifi_Success);
    }

    /* This is to guarantee the state of the Controlled port at the start of the next connect */
    (void)ns_port_configure(context, unifi_8021xPortClosedDiscard, unifi_8021xPortClosedDiscard);

    send_sm_unpause_req(context, getSmeContext(context)->scanManagerInstance);

#ifdef CSR_AMP_ENABLE
    pal_sme_connection_status_change_request(context,PAL_SME_CONNECTION_STATUS_DISCONNECTED);
#endif

    fsm_next_state(context, FSMSTATE_disconnected);
}

/**
 * @brief
 *   Scan Manager confirms it is it's scan is complete
 *
 * @param[in]    context        : FSM context
 * @param[in]    cfm            : Event
 *
 * @return
 *  Void
 */
static void scanning__scan_cfm(FsmContext *context, const SmScanCfm_Evt * cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "scanning__scan_cfm()"));

    if (fsmData->roamingScan)
    {
        fsm_next_state(context, FSMSTATE_connected);
    }
    else
    {
        fsm_next_state(context, FSMSTATE_disconnected);
    }
}


/**
 * @brief
 *   All layer 2 and 3 configuration successful, enter connected state
 *
 * @par Description
 *   All layer 2 and layer 3 configuration is complete.  Set the scan
 *   manager and Power Manager to the appropriate modes and update the MMI
 *   status.  Move to the connected state as there is nothing more to do
 *   unless the connection fails, we decide to roam or we are shut down.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void connection_complete(FsmContext *context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint16 i;

    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "connection_complete()"));

    fsm_next_state(context, FSMSTATE_connected);

#ifdef CCX_VARIANT
    if( ((cfg->joinIECapabilities & CCX_Capable) == CCX_Capable)
       &&(cfg->ccxConfig.apRoamingEnabled))
    {
        if (fsmData->lastSuccessfulRoamConnection != NULL)
        {
            CsrUint32 now = fsm_get_time_of_day_ms(context);
            CsrUint16 duration;
            CsrUint32 timeRoamingStopped;

            timeRoamingStopped = now;

            if (timeRoamingStopped >= fsmData->lastSuccessfulRoamConnection->connDropTimeStamp)
            {
                /* No wrapping */
                duration = (CsrUint16) CsrTimeSub(timeRoamingStopped, fsmData->lastSuccessfulRoamConnection->connDropTimeStamp);
            }
            else
            {
                /* Time counter wrapped */
                duration = (CsrUint16) CsrTimeAdd(timeRoamingStopped, (((CsrUint32) (-1)) - fsmData->lastSuccessfulRoamConnection->connDropTimeStamp) );
            }
            sme_trace_info((TR_NETWORK_SELECTOR_FSM, "connection_complete: last connection Roam took %lums",
                    duration));

            send_ccx_successful_roam_ind(context,
                                     getSmeContext(context)->ccxInstance,
                                     cfg->connectionInfo.bssid,
                                     fsmData->lastSuccessfulRoamConnection->address,
                                     fsmData->lastSuccessfulRoamConnection->ssid,
                                     fsmData->lastSuccessfulRoamConnection->channel,
                                     duration);
        }
    }
#endif

    /* Only set the media connected to connected in infrastructure
     * Connection manager will handle the Adhoc case */
    if (cfg->connectionConfig.bssType == unifi_Infrastructure || !fsmData->hostingadhoc)
    {
        ns_set_media_connection_status(context, unifi_MediaConnected);
    }

    if (fsmData->mmiJoinRequested)
    {
        fsmData->mmiJoinRequested = FALSE;
        call_unifi_mgt_connect_cfm(context, fsmData->cfmAppHandle, unifi_Success);
    }

    /* Open the uncontrolled port AFTER the media connect and connect cfm */
    if (cfg->connectionInfo.authMode >= unifi_8021xAuthWPA || fsmData->dynamicWepKeys)
    {
        (void)ns_port_configure(context, unifi_8021xPortOpen, unifi_8021xPortClosedBlock);
    }

    /* Reconnected to the SAME AP as last time? */
    if (CsrMemCmp(cfg->lastJoinedAddresses[0].address.data, cfg->connectionInfo.bssid.data, sizeof(cfg->connectionInfo.bssid.data)) == 0)
    {
        fsmData->roamConnectCount++;
    }
    else
    {
        fsmData->roamConnectCount = 1;
    }
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "connection_complete() roamConnectCount = %d", fsmData->roamConnectCount));


    /*
     * Only move old network entries in the list is the most recent is NOT for
     * an IBSS and we've previously added something (most recent address is
     * not the broadcast address
     */
    if (cfg->lastJoinedAddresses[0].bssType != BssType_Independent &&
        CsrMemCmp(cfg->lastJoinedAddresses[0].address.data, BroadcastBssid.data, sizeof(BroadcastBssid.data)))
    {
        /*
         * Update the 'networks we've connected to' list. Start by checking to see
         * if the network we've just connected to is already in the list.
         */
        for (i = 0; i < MAX_CONNECTED_AP_HISTORY; i++)
        {
            if (CsrMemCmp(cfg->lastJoinedAddresses[i].address.data, cfg->connectionInfo.bssid.data, sizeof(cfg->connectionInfo.bssid.data)) == 0)
            {
                break;
            }
        }

        /*
         * if i == MAX_CONNECTED_AP_HISTORY then the current network is not in the list
         */
        if (i == MAX_CONNECTED_AP_HISTORY)
        {
            i--;
        }

        if (i != 0)
        {
            CsrMemMove(&cfg->lastJoinedAddresses[1], &cfg->lastJoinedAddresses[0], sizeof(cfg->lastJoinedAddresses[0]) * i);
        }
    }
    else
    {
        sme_trace_debug_code
        (
            if (!CsrMemCmp(cfg->lastJoinedAddresses[0].address.data, BroadcastBssid.data, sizeof(BroadcastBssid.data)))
            {
                sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "No need to reorganise network history list - it is empty"));
            }
            if (cfg->lastJoinedAddresses[0].bssType == BssType_Independent)
            {
                sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "Most recent network in history list is for IBSS - overwriting"));
            }
        )
    }

    cfg->lastJoinedAddresses[0].ssid     = cfg->connectionInfo.ssid;
    cfg->lastJoinedAddresses[0].address  = cfg->connectionInfo.bssid;
    cfg->lastJoinedAddresses[0].channel  = cfg->connectionInfo.channelNumber;
    cfg->lastJoinedAddresses[0].bssType  = cfg->connectionConfig.bssType;

    send_sm_unpause_req(context, getSmeContext(context)->scanManagerInstance);
}

static void process_join_or_roam_confirm(FsmContext *context, const FsmEvent* cfm, unifi_Status result)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "process_join_or_roam_confirm(%s)", trace_unifi_Status(result)));

    /* Set up the remaining global information after the link quality
     * signal exchange, as the wireless profile may not be available from
     * the store yet. */
    if (result == unifi_Success)
    {
        /* Indicate connection complete */
        if (cfg->connectionConfig.bssType == unifi_Infrastructure)
        {
            void* appHandles;
            CsrUint16 appHandleCount = get_appHandles(context, unifi_IndAssociationComplete, &appHandles);
            if (appHandleCount)
            {
                call_unifi_mgt_association_complete_ind(context, appHandleCount, appHandles, result, &cfg->connectionInfo, cfg->deauthReason);
            }
        }

        /* Indicate Roaming attempt to user */
        if (fsmData->mgtRoamStartOutstanding)
        {
            void* appHandles;
            CsrUint16 appHandleCount = get_appHandles(context, unifi_IndRoamComplete, &appHandles);
            if (appHandleCount)
            {
                call_unifi_mgt_roam_complete_ind(context, appHandleCount, appHandles, result);
            }
            fsmData->mgtRoamStartOutstanding = FALSE;
        }

        /* save the flag that gets reset in connection_complete() */
        /* Tell link quality to start monitoring the layer 2 connection */
        connection_complete(context);

        /* make sure that the scan results are upto date */
        srs_expire_old_scan_data(context);

        /* clear out the list */
        fsmData->currentConnectAddress = 0;
        fsmData->numConnectAddresses = 0;
    }
    else
    {
        /* Indicate connection Attempt Failed to user*/
        if (cfg->connectionConfig.bssType == unifi_Infrastructure)
        {
            void* appHandles;
            CsrUint16 appHandleCount = get_appHandles(context, unifi_IndAssociationComplete, &appHandles);
            if (appHandleCount)
            {
                call_unifi_mgt_association_complete_ind(context, appHandleCount, appHandles, result, &cfg->connectionInfo, cfg->deauthReason);
            }
        }

        /* Terminate the connection manager that failed to join */
        send_network_selector_terminate_req(context, fsmData->connectionInstance);
        fsmData->connectionInstance = FSM_TERMINATE;

        fsmData->currentConnectAddress++;
        if (fsmData->currentConnectAddress < fsmData->numConnectAddresses)
        {
            cfg->connectionInfo.bssid = fsmData->connectAddresses[fsmData->currentConnectAddress].bssid;
            cfg->connectionInfo.assocReqApAddress = fsmData->connectAddresses[fsmData->currentConnectAddress].bssid;
            cfg->connectionInfo.ssid = fsmData->connectAddresses[fsmData->currentConnectAddress].ssid;
            cfg->connectionInfo.channelNumber = fsmData->connectAddresses[fsmData->currentConnectAddress].channelNumber;
            cfg->joinIECapabilities = fsmData->connectAddresses[fsmData->currentConnectAddress].joinIeCapabilityFlags;

            /* create a new connection mgr instance for this connection */
            fsmData->connectionInstance = fsm_add_instance(context, &connection_manager_fsm, TRUE);

            /* Tell the connection manager to connect to the network */
            /* Set the skipResetOnError flag to true on the LAST connect attampt */
            send_network_selector_join_req(context,
                                           fsmData->connectionInstance,
                                           fsmData->roaming, /* TRUE = reassociate FALSE = associate */
                                           (fsmData->currentConnectAddress < (fsmData->numConnectAddresses-1)), /* Only cleanup on error when last AP to try */
                                           FALSE); /* Not host Adhoc */

            /* Indicate connection attempt to user */
            if (cfg->connectionConfig.bssType == unifi_Infrastructure)
            {
                void* appHandles;
                CsrUint16 appHandleCount = get_appHandles(context, unifi_IndAssociationStart, &appHandles);
                if (appHandleCount)
                {
                    call_unifi_mgt_association_start_ind(context, appHandleCount, appHandles, &cfg->connectionInfo.bssid, &cfg->connectionInfo.ssid);
                }
            }
        }
        else if (cfg->connectionConfig.bssType == unifi_Adhoc &&
                 cfg->connectionConfig.adhocJoinOnly == FALSE &&
                 fsmData->currentConnectAddress == fsmData->numConnectAddresses &&
                 cfg->connectionConfig.ssid.length != 0 )
        {
            /* Host an Adhoc network if we are not able to join a previously existing network */
            fsmData->currentConnectAddress++;

            fsmData->hostingadhoc = TRUE;
            cfg->joinIECapabilities = 0;
            /* Create a new connection mgr instance for this connection */
            fsmData->connectionInstance = fsm_add_instance(context, &connection_manager_fsm, TRUE);

            /* Tell the connection manager to connect to the network */
            send_network_selector_join_req(context, fsmData->connectionInstance, FALSE, TRUE, TRUE);
        }
        else
        {
            /* Indicate roaming attempt failed to user */
            if (fsmData->mgtRoamStartOutstanding)
            {
                void* appHandles;
                CsrUint16 appHandleCount = get_appHandles(context, unifi_IndRoamComplete, &appHandles);
                if (appHandleCount)
                {
                    call_unifi_mgt_roam_complete_ind(context, appHandleCount, appHandles, result);
                }
                fsmData->mgtRoamStartOutstanding = FALSE;
            }

            fsmData->currentConnectAddress = 0;
            fsmData->numConnectAddresses = 0;

            if (fsmData->mmiJoinRequested)
            {
                fsmData->mmiJoinRequested= FALSE;
                call_unifi_mgt_connect_cfm(context, fsmData->cfmAppHandle, unifi_Error);
            }
            else
            {
                ns_set_media_connection_status(context, unifi_MediaDisconnected);
            }

            send_sm_unpause_req(context, getSmeContext(context)->scanManagerInstance);

            /* make sure that the scan results are upto date */
            srs_expire_old_scan_data(context);
#ifdef CSR_AMP_ENABLE
            pal_sme_connection_status_change_request(context,PAL_SME_CONNECTION_STATUS_DISCONNECTED);
#endif

            fsm_next_state(context, FSMSTATE_disconnected);
        }
    }
}

/**
 * @brief
 *   Process a Join confirm from the Connection Manager
 *
 * @par Description
 *   If the join was initiated manually then the success or failure of the
 *   join needs to be communicated to the MMI.
 *
 *   If the join was sucessful then start link quality monitoring and wait
 *   for confirmation, else clear down to the disconnected state.
 *
 * @param[in] context : FSM context
 * @param[in] cfm     : result of the join operation
 *
 * @return
 *  Void
 */
static void connecting__ns_join_cfm(FsmContext *context, const NetworkSelectorJoinCfm_Evt *cfm)
{
    sme_trace_entry_code(SmeConfigData* cfg = get_sme_config(context);)
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "connecting__ns_join_cfm(%s, %s)",
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                    trace_unifi_Status(cfm->status)));

#ifdef CCX_VARIANT
    if ( cfm->status == unifi_Success)
    {
        /* when 3rd param is false then 2nd param is ignored */
        ccx_set_roaming_reason( context, unifi_RoamBeaconLost, FALSE );
    }
#endif

    process_join_or_roam_confirm(context, (FsmEvent*)cfm, cfm->status);
}

/**
 * @brief
 *   Process a Roam confirm from the Connection Manager
 *
 * @par Description
 *
 *   If the join was sucessful then start link quality monitoring and wait
 *   for confirmation, else clear down to the disconnected state.
 *
 * @param[in] context : FSM context
 * @param[in] cfm     : result of the roam operation
 *
 * @return
 *  Void
 */
static void connecting__ns_roam_cfm(FsmContext *context, const NetworkSelectorRoamCfm_Evt *cfm)
{
#ifdef CCX_VARIANT
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
#else
    sme_trace_entry_code( SmeConfigData* cfg = get_sme_config(context);)
#endif

    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "connecting__ns_roam_cfm(%s, %s)",
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context)),
                    trace_unifi_Status(cfm->status)));

#ifdef CCX_VARIANT
    if ( cfm->status == unifi_Success)
    {
        ccx_set_roaming_reason( context, fsmData->associateReason, TRUE );
        fsmData->lastSuccessfulRoamConnection = &cfg->lastJoinedAddresses[0];
    }
#endif

    process_join_or_roam_confirm(context, (FsmEvent*)cfm, cfm->status);
}

/**
 * @brief
 *   MMI wants a specific network.  Need to tear down IP stack first
 *
 * @par Description
 *   The MMI has sent a manual join request.  As we are connected, we will
 *   need to tear down the current IP stack first before telling connection
 *   manager to join to the new network.  So actually to keep things simple,
 *   we will go through most of a normal shutdown procedure and just invoke
 *   the new join at the end of it.
 *
 * @param[in] context : FSM context
 * @param[in] request : network to join
 *
 * @return
 *  Void
 */
static void connected__connect(FsmContext *context, const UnifiMgtConnectReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "connected__connect(%s : %s)",
                        trace_unifi_MACAddress(req->connectionConfig.bssid, getMacAddressBuffer(context)),
                        trace_unifi_SSID(&req->connectionConfig.ssid, getSSIDBuffer(context))));

    if (isAccessRestricted(context, req->appHandle))
    {
        sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "connected__connect() unifi_Restricted"));
        call_unifi_mgt_connect_cfm(context, req->appHandle, unifi_Restricted);
        return;
    }

    /* Special for Adhoc.
     * If we are already on the adhoc network and are asked for the
     * same network again. Just confirm the join and leave the network alone.
     * This is so if we drift out of range and a join for the same IBSS
     * is sent (WZC does this) we will not change the BSSID by hosting a new network.
     */
    if (cfg->connectionConfig.bssType == unifi_Adhoc && req->connectionConfig.bssType == unifi_Adhoc)
    {
        CsrBool ssidMatch = req->connectionConfig.ssid.length == cfg->connectionInfo.ssid.length &&
                            CsrMemCmp(req->connectionConfig.ssid.ssid, cfg->connectionInfo.ssid.ssid, cfg->connectionInfo.ssid.length) == 0;

        CsrBool bssidMatch = CsrMemCmp(cfg->connectionInfo.bssid.data, req->connectionConfig.bssid.data, sizeof(cfg->connectionInfo.bssid.data)) == 0 ||
                             CsrMemCmp(BroadcastBssid.data, req->connectionConfig.bssid.data, sizeof(BroadcastBssid.data)) == 0;

        /* If SSID or BSSID join and SSID or BSSID match current Network */
        if (ssidMatch && bssidMatch && fsmData->mediaConnectionStatus == unifi_MediaConnected)
        {
            sme_trace_info((TR_NETWORK_SELECTOR_FSM, "connected__connect() :: Already connected to adhoc network. "
                    "Ignoring join request"));

            CsrPfree(req->connectionConfig.mlmeAssociateReqInformationElements);

            call_unifi_mgt_connect_cfm(context, req->appHandle, unifi_Success);
            return;
        }
    }

    send_network_selector_detach_req(context, fsmData->connectionInstance);
    fsm_next_state(context, FSMSTATE_disconnecting);

    /* This Saves the event for later processing  (After we disconnect) */
    fsm_saved_event(context, (const FsmEvent*)req);
}

/**
 * @brief
 *   A disconnect request arrived while we waited for a connection to come up
 *
 * @par Description
 *   A network_selector_join_req was outstanding to Connection Manager, but
 *   a unifi_mgt_disconnect_req has come in from the MMI SAP.  If we
 *   are roaming, tear down the suspended IP stack and let the normal
 *   sequence cause a detach.  If we are not roaming, just send the detach
 *   and move to a dedicated state to handle the fallout.
 *
 * @param[in] context : FSM context
 * @param[in] req     : Content of the request
 *
 * @return
 *  Void
 */
static void connecting__disconnect(FsmContext *context, const UnifiMgtDisconnectReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM,
        "waiting_for_network_selector_join_cfm: unifi_mgt_disconnect_req received"));

    if (isAccessRestricted(context, req->appHandle))
    {
        sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "connecting__disconnect() unifi_Restricted"));
        call_unifi_mgt_disconnect_cfm(context, req->appHandle, unifi_Restricted);
        return;
    }

    send_network_selector_detach_req(context, fsmData->connectionInstance);
    fsm_next_state(context, FSMSTATE_disconnecting);

    /* This Saves the event for later processing  (After we disconnect)*/
    fsm_saved_event(context, (const FsmEvent*)req);

}

/**
 * @brief
 *   A stop command arrived while we waited for a connection to come up
 *
 * @par Description
 *   A network_selector_join_req was outstanding to Connection Manager, but
 *   a stop command has come in from the APP SAP.  If we are roaming, tear
 *   down the suspended IP stack and let the normal sequence cause a detach.
 *   If we are not roaming, just send the detach and move to a dedicated
 *   state to handle the fallout.
 *
 * @param[in] context : FSM context
 * @param[in] req     : Content of the request
 *
 * @return
 *  Void
 */
static void connecting__stop(FsmContext *context, const CoreStopReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "waiting_for_network_selector_join_cfm: stop received"));

    send_network_selector_detach_req(context, fsmData->connectionInstance);
    fsm_next_state(context, FSMSTATE_disconnecting);

    /* This Saves the event for later processing  (After we disconnect)*/
    fsm_saved_event(context, (const FsmEvent*)req);
}

/**
 * @brief
 *   Refuse a join request with an error, as we are in an inappropriate state
 *
 * @par Description
 *   This function is used when a join request has not been serviced by a
 *   preferred handler.
 *
 * @param[in] context : FSM context
 * @param[in] request : the join request
 *
 * @return
 *  Void
 */
static void stopped__connect(FsmContext *context, const UnifiMgtConnectReq_Evt *req)
{
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "stopped__connect()"));

    CsrPfree(req->connectionConfig.mlmeAssociateReqInformationElements);

    call_unifi_mgt_connect_cfm(context, req->appHandle, unifi_Unavailable);
}

/**
 * @brief
 *   Remember a join request when connecting or disconnecting
 *
 * @par Description
 *   If we are already closing down for a new connection then
 *   send a cfm for the old connection
 *
 * @param[in] context : FSM context
 * @param[in] req     : event
 *
 * @return
 *  Void
 */
static void connecting__connect(FsmContext *context, const UnifiMgtConnectReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "connecting__connect()"));

    if (isAccessRestricted(context, req->appHandle))
    {
        sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "connecting__connect() unifi_Restricted"));
        call_unifi_mgt_connect_cfm(context, req->appHandle, unifi_Restricted);
        return;
    }

    send_network_selector_detach_req(context, fsmData->connectionInstance);
    fsm_next_state(context, FSMSTATE_disconnecting);

    /* This Saves the event for later processing  (After we disconnect)*/
    fsm_saved_event(context, (const FsmEvent*)req);
}
/**
 * @brief
 *   Remember a join request when connecting or disconnecting
 *
 * @par Description
 *   If we are already closing down for a new connection then
 *   send a cfm for the old connection
 *
 * @param[in] context : FSM context
 * @param[in] req     : event
 *
 * @return
 *  Void
 */
static void cleanup_connect(FsmContext *context, const UnifiMgtConnectReq_Evt *req)
{
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "cleanup_connect()"));

    CsrPfree(req->connectionConfig.mlmeAssociateReqInformationElements);
}

/**
 * @brief
 *   Kick off another scan for the IBSS we are connecting to
 *
 *
 * @param[in] context : FSM context
 * @param[in] timer   : timer
 *
 * @return
 *  void
 */
static void seaching_for_ibss__ibss_scan_timeout(FsmContext *context, const NetworkSelectorAdhocJoinScanTimer_Evt *timer)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "seaching_for_ibss__ibss_scan_timeout()"));
    fsmData->adhocJoinScanTimerId.uniqueid = 0;

    fsmData->adhocJoinScanAttempts++;
    do_prejoin_scan(context);
}

/**
 * @brief
 *   IBSS Scan Complete
 *
 * @param[in]    context        : FSM context
 * @param[in]    cfm            : Event
 *
 * @return
 *  Void
 */
static void seaching_for_ibss__scan_cfm(FsmContext *context, const SmScanCfm_Evt * cfm)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    srs_scan_data* scanData;
    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "seaching_for_ibss__scan_cfm()"));

    build_network_join_list(context, FALSE);
    if (fsmData->numConnectAddresses == 0 )
    {
        if (cfg->adHocConfig.joinOnlyAttempts == 0 || cfg->adHocConfig.joinOnlyAttempts > fsmData->adhocJoinScanAttempts)
        {
            /* Restart the timer */
            send_network_selector_adhoc_join_scan_timer(context, fsmData->adhocJoinScanTimerId, cfg->adHocConfig.joinAttemptIntervalMs, 100);
        }
        else
        {
            fsmData->mmiJoinRequested = FALSE;
            call_unifi_mgt_connect_cfm(context, fsmData->cfmAppHandle, unifi_NotFound);

#ifdef CSR_AMP_ENABLE
            pal_sme_connection_status_change_request(context,PAL_SME_CONNECTION_STATUS_DISCONNECTED);
#endif

            fsm_next_state(context, FSMSTATE_disconnected);
        }
    }
    else
    {
        /* Join the ibss */
        cfg->connectionInfo.bssid = fsmData->connectAddresses[0].bssid;
        cfg->connectionInfo.assocReqApAddress = fsmData->connectAddresses[0].bssid;
        scanData = srs_get_scan_parameters_first(context, NULL, &cfg->connectionInfo.bssid);
        cfg->connectionInfo.ssid = scanData->scanResult.ssid;
        cfg->connectionInfo.channelNumber = scanData->scanResult.channelNumber;
        cfg->joinIECapabilities = calc_join_ie_capabilites(context, cfg, scanData->ieCapabilityFlags);

        /* create a new connection mgr instance for this connection */
        fsmData->connectionInstance = fsm_add_instance(context, &connection_manager_fsm, TRUE);

        /* Tell the connection manager to connect to the network */
        send_sm_pause_req(context, getSmeContext(context)->scanManagerInstance);
        send_network_selector_join_req(context,
                                       fsmData->connectionInstance,
                                       FALSE, /* Not a reassociate */
                                       (fsmData->numConnectAddresses != 1), /* Only cleanup on error when last AP to try */
                                       FALSE); /* Not host Adhoc */

        fsm_next_state(context, FSMSTATE_connecting);
    }
}

/**
 * @brief
 *   Stop Searching for IBSS and process save evt for processing in the disconnected state
 *
 *
 * @param[in] context : FSM context
 * @param[in] timer   : timer
 *
 * @return
 *  void
 */
static void seaching_for_ibss__stop_searching(FsmContext *context, const FsmEvent *evt)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_info((TR_NETWORK_SELECTOR_FSM, "seaching_for_ibss__stop_searching()"));

    if (evt->id == UNIFI_MGT_CONNECT_REQ_ID)
    {
        const UnifiMgtConnectReq_Evt *req = (UnifiMgtConnectReq_Evt*) evt;
        if (isAccessRestricted(context, req->appHandle))
        {
            sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "seaching_for_ibss__stop_searching() unifi_Restricted"));
            call_unifi_mgt_disconnect_cfm(context, req->appHandle, unifi_Restricted);
            return;
        }
    }
    else if (evt->id == UNIFI_MGT_DISCONNECT_REQ_ID)
    {
        const UnifiMgtDisconnectReq_Evt *req = (UnifiMgtDisconnectReq_Evt*) evt;
        if (isAccessRestricted(context, req->appHandle))
        {
            sme_trace_warn((TR_NETWORK_SELECTOR_FSM, "seaching_for_ibss__stop_searching() unifi_Restricted"));
            call_unifi_mgt_disconnect_cfm(context, req->appHandle, unifi_Restricted);
            return;
        }
    }

    fsm_remove_timer(context, fsmData->adhocJoinScanTimerId);
    fsmData->adhocJoinScanTimerId.uniqueid = 0;

    fsmData->mmiJoinRequested = FALSE;
    call_unifi_mgt_connect_cfm(context, fsmData->cfmAppHandle, unifi_Cancelled);

    /* This Saves the event for later processing  (After we change state) */
    fsm_saved_event(context, evt);

#ifdef CSR_AMP_ENABLE
    pal_sme_connection_status_change_request(context,PAL_SME_CONNECTION_STATUS_DISCONNECTED);
#endif

    fsm_next_state(context, FSMSTATE_disconnected);
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/


/**
 * See description in connection_manager_fsm/network_selector_fsm.h
 */
ns_ConnectionStatus ns_get_connection_status(FsmContext* context)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->networkSelectorInstance, FsmData);
    FsmState currentState = context->instanceArray[getSmeContext(context)->networkSelectorInstance].state;
    /* When canning for roaming alternatives we are still considered connected */
    if (currentState == FSMSTATE_connected ||
        (currentState == FSMSTATE_scanning_for_network && fsmData->roamingScan))
    {
        return ns_ConnectionStatus_Connected;
    }

    if (currentState == FSMSTATE_connecting)
    {
        return ns_ConnectionStatus_Connecting;
    }

    return ns_ConnectionStatus_Disconnected;
}

/**
 * See description in connection_manager_fsm/network_selector_fsm.h
 */
CsrBool isInRoamingList(FsmContext* context, const unifi_MACAddress* bssid )
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->networkSelectorInstance, FsmData);
    CsrUint8 i;

    for(i=0; i<fsmData->numConnectAddresses; i++)
    {
        if(CsrMemCmp(&fsmData->connectAddresses[i], bssid, sizeof(bssid->data)))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * See description in connection_manager_fsm/network_selector_fsm.h
 */
CsrBool ns_dynamic_wep_detected(FsmContext* context)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->networkSelectorInstance, FsmData);
    return fsmData->dynamicWepKeys;
}

/**
 * See description in connection_manager_fsm/network_selector_fsm.h
 */
void ns_set_dynamic_wep(FsmContext* context)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->networkSelectorInstance, FsmData);
    fsmData->dynamicWepKeys = TRUE;
}

/**
 * See description in connection_manager_fsm/network_selector_fsm.h
 */
CsrBool ns_port_configure(FsmContext* context, unifi_PortAction uncontrolledPortState, unifi_PortAction controlledPortState)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->networkSelectorInstance, FsmData);

    /* the SYS SAP should no be asked to open the control port if its already opened */
    if (fsmData->controlledPortState == controlledPortState && fsmData->uncontrolledPortState == uncontrolledPortState)
    {
        return FALSE;
    }

    /* When we open the controlled port :
     * 1) Write any multicast addresses to the FW
     * 2) Tell Mib Access to save the Cal Data */
    if (controlledPortState == unifi_8021xPortOpen && fsmData->controlledPortState != controlledPortState)
    {
        /* This will kick the Config fsm to install the updates */
        send_unifi_mgt_multicast_address_req(context,
                                             getSmeContext(context)->smeConfigurationInstance,
                                             NULL,
                                             unifi_ListActionAdd,
                                             0,
                                             NULL);

        send_mib_save_calibration_data_req(context, getSmeContext(context)->mibAccessInstance);
    }

    fsmData->controlledPortState = controlledPortState;
    fsmData->uncontrolledPortState = uncontrolledPortState;

    sme_trace_info((TR_NETWORK_SELECTOR_FSM, "unifi_sys_port_configure_req(%s, %s)", trace_unifi_PortAction(uncontrolledPortState), trace_unifi_PortAction(controlledPortState)));
    call_unifi_sys_port_configure_req(context, uncontrolledPortState, controlledPortState, &BroadcastBssid);


    return TRUE;
}

/**
 * See description in connection_manager_fsm/network_selector_fsm.h
 */
unifi_PortAction ns_get_controlled_port_state(FsmContext* context)
{
    return fsm_get_params_by_id(context, getSmeContext(context)->networkSelectorInstance, FsmData)->controlledPortState;
}


/**
 * See description in sme_core_fsm/network_selector_fsm.h
 */
void ns_set_media_connection_status(FsmContext* context, unifi_MediaStatus status)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->networkSelectorInstance, FsmData);
    SmeConfigData* cfg = get_sme_config(context);

    /* B-31162: a driver requirement that consecutive
     * unifi_MediaConnected indications are allowed when roaming has
     * taken place.
     */
    if ((fsmData->mediaConnectionStatus == unifi_MediaConnected) ||
        (fsmData->mediaConnectionStatus != status))
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndMediaStatus, &appHandles);
        unifi_ConnectionInfo connectionInfo = get_sme_config(context)->connectionInfo;

        sme_trace_info((TR_NETWORK_SELECTOR_FSM, "Set media connection status from %s to %s",
            trace_unifi_MediaStatus(fsmData->mediaConnectionStatus),
            trace_unifi_MediaStatus(status)));

        /* Zero the data on disconnected */
        if (status == unifi_MediaDisconnected)
        {
            CsrMemSet(&connectionInfo, 0x00, sizeof(connectionInfo));
        }

        call_unifi_sys_media_status_req(context, status, unifi_MediaType80211);
        if (appHandleCount)
        {
            call_unifi_mgt_media_status_ind(context, appHandleCount, appHandles, status, &connectionInfo, cfg->disassocReason, cfg->deauthReason);
        }
    }

    /* Disable Deep sleep in the driver when the Connected and power mode is active */
    if (fsmData->mediaConnectionStatus != status)
    {
        /* Assign here as the power_send_sys_configure_power_mode() will read this */
        fsmData->mediaConnectionStatus = status;
        power_send_sys_configure_power_mode(context, TRUE);
    }
    fsmData->mediaConnectionStatus = status;
}

/**
 * See description in sme_core_fsm/network_selector_fsm.h
 */
unifi_MediaStatus ns_get_media_connection_status(FsmContext *context)
{
    return (fsm_get_params_by_id(context, getSmeContext(context)->networkSelectorInstance, FsmData))->mediaConnectionStatus;
}


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
static void ns_fsm_dump(FsmContext* context, const CsrUint16 id)
{
    FsmData* fsmData = fsm_get_params_by_id(context, id, FsmData);
    require(TR_NETWORK_SELECTOR_FSM, id < context->maxProcesses);
    require(TR_NETWORK_SELECTOR_FSM, context->instanceArray[id].fsmInfo->processId == NETWORK_SELECTOR_PROCESS);
    require(TR_NETWORK_SELECTOR_FSM, context->instanceArray[id].state != FSM_TERMINATE);

    sme_trace_crit((TR_FSM_DUMP, "   : connectionInstance             : 0x%.4X", fsmData->connectionInstance));
    sme_trace_crit((TR_FSM_DUMP, "   : mmiJoinRequested               : %s",     fsmData->mmiJoinRequested?"TRUE":"FALSE"));
    sme_trace_crit((TR_FSM_DUMP, "   : mmiDisconnectRequested         : %s",     fsmData->mmiDisconnectRequested?"TRUE":"FALSE"));
    sme_trace_crit((TR_FSM_DUMP, "   : mediaConnectionStatus          : %s",     trace_unifi_MediaStatus(fsmData->mediaConnectionStatus)));
}
#endif

/** Event handling in waiting_for_startup while waiting for mmi_start_req signal */
static const FsmEventEntry waitingForStartupTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                            stopped__start),
    fsm_event_table_entry(UNIFI_MGT_CONNECT_REQ_ID,                     stopped__connect),
    fsm_event_table_entry(UNIFI_MGT_DISCONNECT_REQ_ID,                  stopped__disconnect),
};

/** Event handling in disconnected while waiting for whatever happens next */
static const FsmEventEntry disconnectedTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                             disconnected__core_stop),
    fsm_event_table_entry(UNIFI_MGT_DISCONNECT_REQ_ID,                  disconnected__disconnect),
    fsm_event_table_entry(UNIFI_MGT_CONNECT_REQ_ID,                     disconnected__connect),
};

static const FsmEventEntry scanningForNetworkTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(SM_SCAN_CFM_ID,                               scanning__scan_cfm),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_IND_ID,                 fsm_saved_event),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_IND_ID,                 fsm_ignore_event),
    /* NOTE :: Save* is active so -> Save all other events in this State */
};

static const FsmEventEntry searchingForIbssTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(NETWORK_SELECTOR_ADHOC_JOIN_SCAN_TIMER_ID,    seaching_for_ibss__ibss_scan_timeout),
    fsm_event_table_entry(SM_SCAN_CFM_ID,                               seaching_for_ibss__scan_cfm),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                             seaching_for_ibss__stop_searching),
    fsm_event_table_entry(UNIFI_MGT_CONNECT_REQ_ID,                     seaching_for_ibss__stop_searching),
    fsm_event_table_entry(UNIFI_MGT_DISCONNECT_REQ_ID,                  seaching_for_ibss__stop_searching),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_IND_ID,                 fsm_ignore_event),
    /* NOTE :: Save* is active so -> Save all other events in this State */
};

/** Event handling in waiting_for_network_selector_join_cfm while waiting for network_selector_join_cfm signal */
static const FsmEventEntry connectingTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(NETWORK_SELECTOR_JOIN_CFM_ID,                 connecting__ns_join_cfm),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_CFM_ID,                 connecting__ns_roam_cfm),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                             connecting__stop),
    fsm_event_table_entry(UNIFI_MGT_DISCONNECT_REQ_ID,                  connecting__disconnect),
    fsm_event_table_entry(UNIFI_MGT_CONNECT_REQ_ID,                     connecting__connect),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_IND_ID,                 fsm_ignore_event),
    /* NOTE :: Save* is active so -> Save all other events in this State */
};

/** Event handling in connected while waiting for whatever happens next */
static const FsmEventEntry connectedTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                             connected__core_stop),
    fsm_event_table_entry(UNIFI_MGT_DISCONNECT_REQ_ID,                  connected__disconnect),
    fsm_event_table_entry(UNIFI_MGT_CONNECT_REQ_ID,                     connected__connect),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_IND_ID,                 connected__roam),
};

/** Event handling in waiting_for_disconnect while waiting for network_selector_detach */
static const FsmEventEntry disconnectingTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(NETWORK_SELECTOR_DETACH_CFM_ID,               disconnecting__disconnected),
    fsm_event_table_entry(NETWORK_SELECTOR_JOIN_CFM_ID,                 fsm_ignore_event),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_CFM_ID,                 fsm_ignore_event),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_IND_ID,                 fsm_ignore_event),
    /* NOTE :: Save* is active so -> Save all other events in this State */
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /*                    Signal Id,                                    Function */
    /* May have canceled the join so ignore spurious Detach cfm's*/
    fsm_event_table_entry(NETWORK_SELECTOR_DETACH_CFM_ID,               fsm_ignore_event),
    fsm_event_table_entry(NETWORK_SELECTOR_ROAM_IND_ID,                 fsm_ignore_event),
    fsm_event_table_entry(SM_PAUSE_CFM_ID,                              fsm_ignore_event),
    fsm_event_table_entry(SM_UNPAUSE_CFM_ID,                            fsm_ignore_event),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry cleanupTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_MGT_CONNECT_REQ_ID,                     cleanup_connect),
};


/** Table of states, their state transition tables, and whether unhandled
 * events should be saved
 */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                         State                                      State                   Save
    *                         Name                                    Transitions                 *   */
   fsm_state_table_entry(FSMSTATE_waiting_for_startup,         waitingForStartupTransitions,     FALSE),
   fsm_state_table_entry(FSMSTATE_disconnected,                disconnectedTransitions,          FALSE),
   fsm_state_table_entry(FSMSTATE_scanning_for_network,        scanningForNetworkTransitions,    TRUE ),
   fsm_state_table_entry(FSMSTATE_searching_for_ibss,          searchingForIbssTransitions,      TRUE ),
   fsm_state_table_entry(FSMSTATE_connecting,                  connectingTransitions,            TRUE ),
   fsm_state_table_entry(FSMSTATE_connected,                   connectedTransitions,             FALSE),
   fsm_state_table_entry(FSMSTATE_disconnecting,               disconnectingTransitions,         TRUE ),
};

/** FSM process description */
const FsmProcessStateMachine network_selector_fsm =
{
#ifdef FSM_DEBUG
       "Network Selector",                                                      /* Process Name */
#endif
       NETWORK_SELECTOR_PROCESS,                                                /* Process ID */
       {FSMSTATE_MAX_STATE, stateTable},                                        /* Transition Tables */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE),  /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, cleanupTransitions,FALSE),    /* Ignore Event handers */
       initFsmNetworkSelector,                                                  /* Entry Function */
       NULL,                                                                    /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       ns_fsm_dump                                                              /* Trace Dump Function   */
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
