/** @file security_manager_fsm.c
 *
 * Security Manager FSM Implementation
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
 *   Security Manager Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/security_manager_fsm/security_manager_fsm.c#13 $
 *
 ****************************************************************************/

/** @{
 * @ingroup security_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "security_manager_fsm/security_manager_fsm.h"
#include "scan_manager_fsm/scan_manager_fsm.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "security_manager_fsm/security_8021x.h"
#include "coex_fsm/coex_fsm.h"

#include "hip_proxy_fsm/hip_signal_proxy_fsm.h"
#include "hip_proxy_fsm/mib_encoding.h"

#include "scan_manager_fsm/scan_results_storage.h"
#include "ie_access/ie_access.h"

#include "mgt_sap/mgt_sap_from_sme_interface.h"
#include "sys_sap/sys_sap_from_sme_interface.h"
#include "hip_proxy_fsm/mibdefs.h"
#include "hip_proxy_fsm/mib_action_sub_fsm.h"

#include "smeio/smeio_trace_types.h"
#include "ap_utils/ap_validation.h"

/* MACROS *******************************************************************/

/**
 * @brief
 *   Simple accessor for this Processes Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, FsmData))

/**
 * @brief
 *   Sets a value in the secFlags data
 *
 * @par Description
 *   see brief
 */
#define SEC_SET_FLAG(fsmData, value) (fsmData->secFlags |= value)

/**
 * @brief
 *   Clears a value in the secFlags data
 *
 * @par Description
 *   see brief
 */
#define SEC_CLEAR_FLAG(fsmData, value) (fsmData->secFlags &= ~value)

/**
 * @brief
 *   Checks value in the secFlags data
 *
 * @par Description
 *   see brief
 */
#define SEC_IS_SET(fsmData, value) ((CsrBool)((fsmData->secFlags & value) != 0))

/**
 * @brief
 *   Timeout time in ms for the 8021x security
 *   Just in case we do not detect that it has failed
 *
 * @par Description
 *   see brief
 */
#define WPA_CONNECT_TIMEOUT_MS (20000)

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
   FSMSTATE_ready,
   FSMSTATE_ready_to_auth,
   FSMSTATE_waiting_for_keys,
   FSMSTATE_opening_control_port,
   FSMSTATE_control_port_open,
   FSMSTATE_waiting_for_mlme_cfm,
   FSMSTATE_waiting_for_terminate,
   FSMSTATE_MAX_STATE
} FsmState;


/**
 * @brief
 *   Security Flags bit indexes
 *
 * @par Description
 *   Enum defining the indexes in to secFlags
 */
typedef enum SecFlags
{
    SecFlag_securityConnectStopReceived                 = 0x0001,
    SecFlag_securityConnectionEstablished               = 0x0002,
    SecFlag_SettingPairWiseKey                          = 0x0020,
    SecFlag_SettingPairWiseKeyRxProtection              = 0x0040,
    SecFlag_SettingPairWiseKeyRxTxProtection            = 0x0080,
    SecFlag_HavePairWiseKey                             = 0x0100,
    SecFlag_SettingGroupKey                             = 0x0200,
    SecFlag_SettingGroupKeyProtection                   = 0x0400,
    SecFlag_HaveGroupKey                                = 0x0800,
    SecFlag_MichealMicFailureReceived                   = 0x1000,
    SecFlag_MichealMicFailureInvokeCounterMeasures      = 0x2000
} SecFlags;

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData {
    DataReference ieDataRef;

    /** michealMic Timer ID */
    FsmTimerId michealMicFailureTimerId;

    /** flags used with SecFlags */
    CsrUint16 secFlags;

    CsrBool rsnaEnabled;
    CsrBool wapiEnabled;
    CsrUint32 rsnGroupCipher;

    /** group Key Id */
    CsrUint16 groupKeyId;
    /** Pairwaise Key ID */
    CsrUint16 pairwiseKeyId;

    /** Owners instance id */
    CsrUint16 connectionManagerInstance;

    /** Security Status */
    unifi_Status status;

    CsrBool scanManagerPaused;

} FsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Used to check and action the securityConnectStopReceived flag,
 *
 * @par Description
 *   If a securityConnectStop event is received while waiting for
 *   a confirm the securityConnectStopReceived flag will be set.
 *   This function checks the flag and takes the correct action on.
 *
 * @param[in]    context  : FSM context
 *
 * @return
 *   CsrBool Was a Connect Stop received and actioned
 */
static CsrBool waitForTerminateOnStop(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    if (SEC_IS_SET(fsmData, SecFlag_securityConnectStopReceived))
    {
        sme_trace_entry((TR_SECURITY, "waitForTerminateOnStop() Stop received"));

        send_security_connect_stop_cfm(context, fsmData->connectionManagerInstance, TRUE);

        fsm_next_state(context, FSMSTATE_waiting_for_terminate);
    }
    return SEC_IS_SET(fsmData, SecFlag_securityConnectStopReceived);
}

/**
 * @brief
 *   Used to send the correct mesage to the connectin manager on finish
 *
 * @par Description
 *   If a securityConnectStop event is received while waiting for
 *   a confirm the securityConnectStopReceived flag will be set.
 *   This function checks the flag and takes the correct action on.
 *
 * @param[in]    context  : FSM context
 *
 * @return
 *   CsrBool Was a Connect Stop received and actioned
 */
static void waitForTerminateOnStopOrError(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    if (SEC_IS_SET(fsmData, SecFlag_securityConnectStopReceived))
    {
        sme_trace_entry((TR_SECURITY, "waitForTerminateOnStopOrError() Stop received"));
        send_security_connect_stop_cfm(context, fsmData->connectionManagerInstance, TRUE);
    }
    else if (SEC_IS_SET(fsmData, SecFlag_securityConnectionEstablished) != TRUE)
    {
        send_security_connect_start_cfm(context, fsmData->connectionManagerInstance, fsmData->status);
    }
    else if (FSMDATA->status != unifi_Success)
    {
        sme_trace_entry((TR_SECURITY, "waitForTerminateOnStopOrError() error occured"));
        send_security_disconnected_ind(context, fsmData->connectionManagerInstance, fsmData->status);
    }
    else
    {
        sme_trace_crit((TR_SECURITY, "waitForTerminateOnStopOrError() WHY ARE WE HERE?"));
        verify(TR_SECURITY, 0); /*lint !e774 */
    }
    fsm_next_state(context, FSMSTATE_waiting_for_terminate);
}


/**
 * @brief
 *   Called to reset the FSM if an error is detected prejoin
 *
 * @par Description
 *   see brief
 *
 * @param[in] context : FSM context
 * @param[in] status : status
 *
 * @return
 *   void
 */
static void preJoinResetToReadyOnError(FsmContext* context, unifi_Status status)
{
    sme_trace_entry((TR_SECURITY, "preJoinResetToReadyOnError()"));
    send_security_join_start_cfm(context, FSMDATA->connectionManagerInstance, NullDataReference ,status);

    fsm_next_state(context, FSMSTATE_waiting_for_terminate);
}

/**
 * @brief
 *   waiting_for_mib_confirm State MibSetSecurityElements transition
 *
 * @par Description
 *   Pre join complete inform the connection manager that the join can proceed.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : mib set cfm
 *
 * @return
 *   void
 */
static void mibSetSecurityElementsConfirm(FsmContext* context, const MlmeSetCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SECURITY, "WaitingForMibConfirm::MibSetSecurityElements_confirm()"));

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }

    /* --------------------------------------------------------------------- */
    /* Check for Stop / Error                                                */
    /* --------------------------------------------------------------------- */
    if (waitForTerminateOnStop(context)) return;
    if (cfm->status != MibStatus_Successful)
    {
        preJoinResetToReadyOnError(context, unifi_Error);
        return;
    }
    /* --------------------------------------------------------------------- */

    send_security_join_start_cfm(context, fsmData->connectionManagerInstance, fsmData->ieDataRef ,unifi_Success);

    /* we no longer have ownership */
    fsmData->ieDataRef = NullDataReference;

    fsm_next_state(context, FSMSTATE_ready_to_auth);
}

/**
 * @brief
 *   Sends events for all needed data
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context  : FSM context
 *
 * @return
 *   void
 */
static void sendPreConnectRequests(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    CsrBool isWps = FALSE;
    CsrUint8 idx = (CsrUint8)cfg->connectionInfo.ifIndex;
    DataReference mibSetDataRef;
    CsrUint8 mibCount;

    /* Default the count */
    mibCount = 13;
    if (cfg->wapiOptionImplemented) { mibCount++; }

    mibSetDataRef = mib_encode_create_set(context, mibCount, 4);

    sme_trace_entry((TR_SECURITY, "sendPreConnectRequests()"));

    if (cfg->connectionConfig.mlmeAssociateReqInformationElementsLength != 0)
    {
        isWps = ie_wps_check_ap_wps_capability(cfg->connectionConfig.mlmeAssociateReqInformationElements, cfg->connectionConfig.mlmeAssociateReqInformationElementsLength);
    }

    {
    CsrInt32 privacyInvoked = (CsrInt32)cfg->connectionConfig.privacyMode;
    /* Choosing WPS when both WAPI and WPS are present
     * may need to change later and use user's input */
    CsrInt32 wapiEnabled = (CsrInt32)(fsmData->wapiEnabled && !isWps);
    CsrInt32 rsnaEnabled = (CsrInt32)(fsmData->rsnaEnabled && !isWps && !wapiEnabled);
    CsrInt32 connectionTimeout = (CsrInt32)(cfg->connectionConfig.bssType==unifi_Infrastructure?INFRASTRUCTURE_CONNECTION_TIMEOUT_BEACONS:
                                                                                               IBSS_CONNECTION_TIMEOUT_BEACONS);
    (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11PrivacyInvoked,        privacyInvoked, idx, 0);
    (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11RSNAEnabled,           rsnaEnabled, idx, 0);
    if (cfg->wapiOptionImplemented)
    {
        (void)mib_encode_add_set_int(context, &mibSetDataRef, gb15629dot11wapiEnabled,    wapiEnabled, idx, 0);
    }
    (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiMLMEConnectionTimeOut, connectionTimeout, idx, 0);

    if (cfg->defaultMibConfig.unifiFixTxDataRate != cfg->mibConfig.unifiFixTxDataRate)
    {
        (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiFixTxDataRate, cfg->mibConfig.unifiFixTxDataRate, 0, 0);
    }

    if (cfg->defaultMibConfig.unifiFixMaxTxDataRate != cfg->mibConfig.unifiFixMaxTxDataRate)
    {
        (void)mib_encode_add_set_boolean(context, &mibSetDataRef, unifiFixMaxTxDataRate, cfg->mibConfig.unifiFixMaxTxDataRate, 0, 0);
    }

    if (cfg->defaultMibConfig.dot11RtsThreshold != cfg->mibConfig.dot11RtsThreshold)
    {
        (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11RTSThreshold, cfg->mibConfig.dot11RtsThreshold, idx, 0);
    }

    if (cfg->defaultMibConfig.dot11FragmentationThreshold != cfg->mibConfig.dot11FragmentationThreshold)
    {
        (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11FragmentationThreshold, cfg->mibConfig.dot11FragmentationThreshold, idx, 0);
    }

    if (cfg->defaultMibConfig.dot11CurrentTxPowerLevel != cfg->mibConfig.dot11CurrentTxPowerLevel)
    {
        (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11CurrentTxPowerLevel, cfg->mibConfig.dot11CurrentTxPowerLevel, idx, 0);
    }

    if (fsmData->rsnaEnabled && !isWps)
    {
        CsrUint8 data[4];
        sme_trace_debug((TR_SECURITY, "sendPreConnectRequests(fsmData->rsnaEnabled && !isWps)"));
        data[0] = (CsrUint8)((fsmData->rsnGroupCipher >> 24) & 0xFF);
        data[1] = (CsrUint8)((fsmData->rsnGroupCipher >> 16) & 0xFF);
        data[2] = (CsrUint8)((fsmData->rsnGroupCipher >>  8) & 0xFF);
        data[3] = (CsrUint8)(fsmData->rsnGroupCipher & 0xFF);

        (void)mib_encode_add_set_os(context, &mibSetDataRef, dot11RSNAConfigGroupCipher, data, 4, idx, 0);
    }
    else if (cfg->connectionConfig.privacyMode && !isWps)
    {
        CsrBool dynamicWepKeys = ns_dynamic_wep_detected(context);
        CsrBool wepKeySet = FALSE;
        CsrUint8 i;
        sme_trace_debug((TR_SECURITY, "sendPreConnectRequests() Wep Tx Index:%d", cfg->wepkeys.txKey));

        for (i = 0; i < 4; ++i)
        {
            if (cfg->wepkeys.keys[i].keyLength != 0)
            {
                sme_trace_debug((TR_SECURITY, "sendPreConnectRequests() Wep Key Index:%d", i));
                if (dynamicWepKeys)
                {
                    /* Clear the Key */
                    cfg->IndexSecurityModeTable[i] = unifi_80211AuthOpen;
                    cfg->wepkeys.keys[i].keyLength = 0;
                }
                else
                {
                    wepKeySet = TRUE;
                }
                (void)mib_encode_add_set_os(context, &mibSetDataRef,dot11WEPDefaultKeyValue,
                        cfg->wepkeys.keys[i].key,cfg->wepkeys.keys[i].keyLength, idx, (CsrUint8)(i + 1));
            }
        }

        (void)mib_encode_add_set_boolean(context, &mibSetDataRef, dot11ExcludeUnencrypted, TRUE, idx, 0);
        (void)mib_encode_add_set_boolean(context, &mibSetDataRef, unifiExcludeUnencryptedExceptEAPOL, TRUE, idx, 0);

        if (wepKeySet)
        {
            (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11WEPDefaultKeyID, cfg->wepkeys.txKey, idx, 0);
        }
    }

    /* Force keep alive at 30 second interval to keep some AP happy */
    (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiKeepAliveTime, cfg->unifiKeepAliveTime, 0, 0);

    /* the mib value has been pre-calculated
     * always set this to make sure the default is correct */
    mib_set_sub_fsm_start(context, *mibSetSecurityElementsConfirm, &mibSetDataRef, FALSE);
    }
}

/**
 * @brief
 *   Security Manager Process FSM Entry Function
 *
 * @par Description
 *   Called on Security Manager Process startup to initialise
 *   the security manager data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void securityManagerInit(FsmContext* context)
{
    FsmData* fsmData;
    sme_trace_entry((TR_SECURITY, "securityManagerInit(%d bytes)", sizeof(FsmData)));

    fsm_create_params(context, FsmData);

    fsmData = FSMDATA;
    fsmData->connectionManagerInstance = FSM_TERMINATE;

    CsrMemSet(&fsmData->michealMicFailureTimerId, 0, sizeof(fsmData->michealMicFailureTimerId));

    fsmData->secFlags = 0;

    fsmData->ieDataRef = NullDataReference;

    fsmData->rsnaEnabled=FALSE;
    fsmData->wapiEnabled=FALSE;
    fsmData->rsnGroupCipher=0;

    fsmData->michealMicFailureTimerId.uniqueid = 0;

    fsmData->status = unifi_Success;

    fsmData->scanManagerPaused = FALSE;

    fsm_next_state(context, FSMSTATE_ready);
}

/**
 * @brief
 *   FSM Reset Function
 *
 * @par Description
 *   Called on reset/shutdown to cleanout any memory in use
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void securityManagerReset(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SECURITY, "securityManagerReset()"));

    if ( fsmData->michealMicFailureTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->michealMicFailureTimerId);
    }
    if (fsmData->ieDataRef.dataLength != 0)
    {
        pld_release(getPldContext(context), fsmData->ieDataRef.slotNumber);
    }

    fsm_next_state(context, FSM_TERMINATE);
}

static void buildSecie(FsmContext* context, const ie_rsn_data *rsnData, const CsrUint32 pairwiseCipherSuite, const CsrUint32 akmSuite)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    unifi_DataBlock result = {0, NULL};
    CsrUint16 offset;
    unifi_Pmkid* pmkid;
    CsrUint16 pmkidCount = 0;

    sme_trace_entry((TR_SECURITY, "buildRsnie()"));

    /* Calculate the number of PMKID's to put in the RSN */
    if (rsnData->rsntype == IE_RSN_TYPE_WPA2)
    {
        (void)sec_get_pmk_cached_data(get_sme_config(context)->pmk_cache, &cfg->connectionInfo.bssid, &pmkid);

        if (pmkid)
        {
            pmkidCount = 1;
            sme_trace_info((TR_SECURITY, "buildRsnie(): Got PMKID for AP (%s) - assembling into IE", 
                            trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));
        }
        else if (cfg->smeConfig.enableOpportunisticKeyCaching)
        {
            /* Opportunistic Key Caching */
            csr_list* pmkidList = sec_get_pmk_list_handle(cfg->pmk_cache);
            csr_list_pmk* node = csr_list_gethead_t(csr_list_pmk*, pmkidList);

            sme_trace_info((TR_SECURITY, "buildRsnie(): No PMKID for AP (%s) - attempting OKC", 
                            trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

            while(node != NULL)
            {
                if (srs_ssid_compare(&node->ssid, &cfg->connectionInfo.ssid) )
                {
                    sme_trace_debug((TR_SECURITY, "buildRsnie(): OKC, try PMKID from AP (%s)", 
                                     trace_unifi_MACAddress(node->pmkid.bssid, getMacAddressBuffer(context))));
                    pmkidCount++;
                }
                node = csr_list_getnext_t(csr_list_pmk*, pmkidList, &node->node);
            }
            sme_trace_info((TR_SECURITY, "buildRsnie(): Inserting %d Opportunistic PMKIDs", pmkidCount));
        }
        else
        {
            sme_trace_info((TR_SECURITY, "buildRsnie(): Opportunistic Key Caching disabled - expect full EAP-exchange"));
        }
    }

    result.data = (CsrUint8*)CsrPmalloc(24 + (UNIFI_PMKID_KEY_SIZE * pmkidCount )); /* max size == 24 bytes + 16 bytes per pmkid */

    if (rsnData->rsntype == IE_RSN_TYPE_WPA2)
    {
        static const CsrUint8 rsnieheader[4] = {0x30, 0x14, 0x01, 0x00};
        offset = sizeof(rsnieheader);
        CsrMemCpy(result.data, rsnieheader, sizeof(rsnieheader));
    }
    else
    {
        static const CsrUint8 wpaieheader[8] = {0xDD, 0x16, 0x00, 0x50, 0xF2, 0x01, 0x01, 0x00};
        offset = sizeof(wpaieheader);
        CsrMemCpy(result.data, wpaieheader, sizeof(wpaieheader));
    }

    result.data[offset++] = (CsrUint8)(rsnData->groupCipherSuite >> 24);          /* Group Cipher Suite       */
    result.data[offset++] = (CsrUint8)((rsnData->groupCipherSuite >> 16) & 0xFF); /* Group Cipher Suite       */
    result.data[offset++] = (CsrUint8)((rsnData->groupCipherSuite >> 8) & 0xFF);  /* Group Cipher Suite       */
    result.data[offset++] = (CsrUint8)(rsnData->groupCipherSuite & 0xFF);         /* Group Cipher Suite       */

    result.data[offset++] = 0x01; result.data[offset++] = 0x00;   /* Pairwise Suite Count     */
    result.data[offset++] = (CsrUint8)(pairwiseCipherSuite >> 24);                    /* Pairwise Suite           */
    result.data[offset++] = (CsrUint8)((pairwiseCipherSuite >> 16) & 0xFF);           /* Pairwise Suite           */
    result.data[offset++] = (CsrUint8)((pairwiseCipherSuite >> 8) & 0xFF);            /* Pairwise Suite           */
    result.data[offset++] = (CsrUint8)(pairwiseCipherSuite & 0xFF);                   /* Pairwise Suite           */

    result.data[offset++] = 0x01; result.data[offset++] = 0x00;   /* Akm Suite Count          */
    result.data[offset++] = (CsrUint8)(akmSuite >> 24);                               /* Akm Suite                */
    result.data[offset++] = (CsrUint8)((akmSuite >> 16) & 0xFF);                      /* Akm Suite                */
    result.data[offset++] = (CsrUint8)((akmSuite >> 8) & 0xFF);                       /* Akm Suite                */
    result.data[offset++] = (CsrUint8)(akmSuite & 0xFF);                              /* Akm Suite                */

    if (rsnData->rsntype == IE_RSN_TYPE_WPA2)
    {
        result.data[offset++] = 0x00; result.data[offset++] = 0x00; /* Capabilities           */
    }

    if ((rsnData->rsntype == IE_RSN_TYPE_WPA2) && pmkidCount)
    {
        result.data[offset++] = (CsrUint8)(pmkidCount & 0xFF); /* PMKID Count */
        result.data[offset++] = (CsrUint8)((pmkidCount >> 8) & 0xFF); /* PMKID Count */
        result.data[1] += 2;

        (void)sec_get_pmk_cached_data(get_sme_config(context)->pmk_cache, &cfg->connectionInfo.bssid, &pmkid);
        if (pmkid)
        {
            CsrMemCpy(&result.data[offset], pmkid->pmkid, UNIFI_PMKID_KEY_SIZE);        /* PMKID */
            offset += 16;
            result.data[1] += 16;
        }
        else
        {
            /* Opportunistic Key Caching
             * Put all the PMKID's for the same ESS in the RSN and hope 1 is usable */
            csr_list* pmkidList = sec_get_pmk_list_handle(cfg->pmk_cache);
            csr_list_pmk* node = csr_list_gethead_t(csr_list_pmk*, pmkidList);

            while(node != NULL)
            {
                if (srs_ssid_compare(&node->ssid, &cfg->connectionInfo.ssid))
                {
                    CsrMemCpy(&result.data[offset], &node->pmkid.pmkid[0], UNIFI_PMKID_KEY_SIZE);
                    /* PMKID */

                    offset += 16;
                    result.data[1] += 16;
                }
                node = csr_list_getnext_t(csr_list_pmk*, pmkidList, &node->node);
            }
        }
    }

    result.length = offset;

    fsmData->ieDataRef.dataLength = offset;
    pld_store(getPldContext(context), (void*)result.data, result.length, (PldHdl*)&fsmData->ieDataRef.slotNumber);

    /* free the original buffer */
    CsrPfree(result.data);
}

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
static void buildWapiIe(FsmContext* context, const ie_rsn_data *rsnData, const CsrUint32 pairwiseCipherSuite, const CsrUint32 akmSuite)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    unifi_DataBlock result = {0, NULL};
    CsrUint16 offset;
    unifi_Pmkid* pmkid;
    CsrUint16 pmkidCount = 0;
    static const CsrUint8 wapiIeHeader[4] = {0x44, 0x16, 0x01, 0x00};

    sme_trace_entry((TR_SECURITY, "buildWapiIe()"));

    /* Calculate the number of PMKID's to put in the IE */
    (void)sec_get_pmk_cached_data(get_sme_config(context)->pmk_cache, &cfg->connectionInfo.bssid, &pmkid);

    if (pmkid)
    {
        pmkidCount = 1;
    }
    else
    {
        /* Opportunistic Key Caching */
        csr_list* pmkidList = sec_get_pmk_list_handle(cfg->pmk_cache);
        csr_list_pmk* node = csr_list_gethead_t(csr_list_pmk*, pmkidList);

        while(node != NULL)
        {
            if (srs_ssid_compare(&node->ssid, &cfg->connectionInfo.ssid) )
            {
                pmkidCount++;
            }
            node = csr_list_getnext_t(csr_list_pmk*, pmkidList, &node->node);
        }
    }

    result.data = (CsrUint8*)CsrPmalloc(24 + (UNIFI_PMKID_KEY_SIZE * pmkidCount )); /* max size == 24 bytes + 16 bytes per pmkid */

    offset = sizeof(wapiIeHeader);
    CsrMemCpy(result.data, wapiIeHeader, sizeof(wapiIeHeader));

    result.data[offset++] = 0x01; result.data[offset++] = 0x00;   /* Akm Suite Count          */
    result.data[offset++] = (CsrUint8)(akmSuite >> 24);                               /* Akm Suite                */
    result.data[offset++] = (CsrUint8)((akmSuite >> 16) & 0xFF);                      /* Akm Suite                */
    result.data[offset++] = (CsrUint8)((akmSuite >> 8) & 0xFF);                       /* Akm Suite                */
    result.data[offset++] = (CsrUint8)(akmSuite & 0xFF);                              /* Akm Suite                */

    result.data[offset++] = 0x01; result.data[offset++] = 0x00;   /* Pairwise Suite Count     */
    result.data[offset++] = (CsrUint8)(pairwiseCipherSuite >> 24);                    /* Pairwise Suite           */
    result.data[offset++] = (CsrUint8)((pairwiseCipherSuite >> 16) & 0xFF);           /* Pairwise Suite           */
    result.data[offset++] = (CsrUint8)((pairwiseCipherSuite >> 8) & 0xFF);            /* Pairwise Suite           */
    result.data[offset++] = (CsrUint8)(pairwiseCipherSuite & 0xFF);                   /* Pairwise Suite           */

    result.data[offset++] = (CsrUint8)(rsnData->groupCipherSuite >> 24);          /* Group Cipher Suite       */
    result.data[offset++] = (CsrUint8)((rsnData->groupCipherSuite >> 16) & 0xFF); /* Group Cipher Suite       */
    result.data[offset++] = (CsrUint8)((rsnData->groupCipherSuite >> 8) & 0xFF);  /* Group Cipher Suite       */
    result.data[offset++] = (CsrUint8)(rsnData->groupCipherSuite & 0xFF);         /* Group Cipher Suite       */
    result.data[offset++] = 0x00; result.data[offset++] = 0x00; /* Capabilities           */

    if (pmkidCount)
    {
        result.data[offset++] = (CsrUint8)(pmkidCount & 0xFF); /* PMKID Count */
        result.data[offset++] = (CsrUint8)((pmkidCount >> 8) & 0xFF); /* PMKID Count */
        result.data[1] += 2;

        (void)sec_get_pmk_cached_data(get_sme_config(context)->pmk_cache, &cfg->connectionInfo.bssid, &pmkid);
        if (pmkid)
        {
            CsrMemCpy(&result.data[offset], pmkid->pmkid, UNIFI_PMKID_KEY_SIZE);        /* PMKID */
            offset += 16;
            result.data[1] += 16;
        }
        else
        {
            /* Opportunistic Key Caching
             * Put all the PMKID's for the same ESS in the RSN and hope 1 is usable */
            csr_list* pmkidList = sec_get_pmk_list_handle(cfg->pmk_cache);
            csr_list_pmk* node = csr_list_gethead_t(csr_list_pmk*, pmkidList);

            while(node != NULL)
            {
                if (srs_ssid_compare(&node->ssid, &cfg->connectionInfo.ssid))
                {
                    CsrMemCpy(&result.data[offset], &node->pmkid.pmkid[0], UNIFI_PMKID_KEY_SIZE);
                    /* PMKID */

                    offset += 16;
                    result.data[1] += 16;
                }
                node = csr_list_getnext_t(csr_list_pmk*, pmkidList, &node->node);
            }
        }
    }
    else
    {
        /* BKID mandatory for WAPI */
        result.data[offset++] = 0; /* BKID Count */
        result.data[offset++] = 0; /* BKID Count */
    }

    result.length = offset;

    fsmData->ieDataRef.dataLength = offset;
    pld_store(getPldContext(context), (void*)result.data, result.length, (PldHdl*)&fsmData->ieDataRef.slotNumber);

    /* free the original buffer */
    CsrPfree(result.data);
}
#endif

static unifi_Status checkAndConfigForSecurity(FsmContext* context, unifi_DataBlock scanData)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    CsrUint16 authMode = cfg->connectionConfig.authModeMask;
    CsrUint16 encryptionMode = cfg->connectionConfig.encryptionModeMask;

    if (scanData.length != 0)
    {
        ie_rsn_ctrl rsnieValue;
        ie_result ieCode = ie_getRSNInfo(scanData.data, scanData.length, &rsnieValue, cfg->smeConfig.allowUnicastUseGroupCipher);

        if (ieCode == ie_success)
        {
            CsrUint8 i;
            CsrBool pairwiseFound = FALSE;
            CsrUint32 pairwiseCipherSuite = 0;
            CsrBool akmFound = FALSE;
            CsrUint32 akmSuite = 0;
            ie_rsn_data *rsnData;

            if (encryptionMode == unifi_EncryptionCipherNone)
            {
                sme_trace_error((TR_SECURITY, "sec_8021xNew(PIG) :: Cannot connect to secure AP with Encrytion Status == %s", trace_unifi_EncryptionMode(encryptionMode)));
                return unifi_Unavailable;
            }

            if( ((authMode & unifi_8021xAuthWPA2) && (rsnieValue.authMode8021x & unifi_8021xAuthWPA2))
              ||((authMode & unifi_8021xAuthWPA2PSK) && (rsnieValue.authMode8021x & unifi_8021xAuthWPA2PSK)) )
            {
                sme_trace_debug((TR_SECURITY, "sec_8021xNew() :: AuthMode == WPA2 (0x%.4X)", authMode));
                rsnData = &rsnieValue.rsnDataWPA2;
            }
            else if( ((authMode & unifi_8021xAuthWPA)  && (rsnieValue.authMode8021x & unifi_8021xAuthWPA) )
                   ||((authMode & unifi_8021xAuthWPAPSK) && (rsnieValue.authMode8021x & unifi_8021xAuthWPAPSK)) )
            {
                sme_trace_debug((TR_SECURITY, "sec_8021xNew() :: AuthMode == WPA (0x%.4X)", authMode));
                rsnData = &rsnieValue.rsnDataWPA;
            }
            else if( (cfg->wapiOptionImplemented) && (((authMode & unifi_WAPIAuthWAI)
                    && (rsnieValue.authMode8021x & unifi_WAPIAuthWAI) )
                    ||((authMode & unifi_WAPIAuthWAIPSK) && (rsnieValue.authMode8021x & unifi_WAPIAuthWAIPSK))) )
            {
                sme_trace_debug((TR_SECURITY, "sec_8021xNew() :: AuthMode == WPA (0x%.4X)", authMode));
                rsnData = &rsnieValue.rsnDataWAPI;
            }
            else
            {
                sme_trace_error((TR_SECURITY, "sec_8021xNew() :: Incorrect AuthMode == 0x%.4X", authMode));
                return unifi_Unavailable;
            }


            fsmData->rsnGroupCipher = rsnData->groupCipherSuite;

            /* we should not have reach this point, but leave it in as a sanity check for now */
            if ( ((encryptionMode & unifi_EncryptionCipherPairwiseTkip) != unifi_EncryptionCipherPairwiseTkip)
               &&((encryptionMode & unifi_EncryptionCipherPairwiseCcmp) != unifi_EncryptionCipherPairwiseCcmp)
               &&((encryptionMode & unifi_EncryptionCipherPairwiseSms4) != unifi_EncryptionCipherPairwiseSms4))
            {
                sme_trace_error((TR_SECURITY, "sec_8021xNew() :: Cannot connect to Secure AP with Encrytion Status == %s", trace_unifi_EncryptionMode(encryptionMode)));
                return unifi_Unavailable;
            }

            /* ---------------------------------------------------------- */
            /* Select the pairwise cipher suite */
            /* ---------------------------------------------------------- */
            if (rsnData->pairwiseCipherSuiteCount == 0)
            {
                sme_trace_error((TR_SECURITY, "sec_8021xNew() :: No Pairwise Cipher Suite in RSN"));
                return unifi_Unavailable;
            }
            for (i = 0; i < rsnData->pairwiseCipherSuiteCount; i++)
            {
                sme_trace_debug((TR_SECURITY, "encryptionMode %x cipherCapabilityFlags %x combined %x",
                                                encryptionMode,  unifi_EncryptionCipherPairwiseCcmp,
                                                (encryptionMode & unifi_EncryptionCipherPairwiseCcmp) ));

                if ((rsnData->rsntype == IE_RSN_TYPE_WAPI) &&
                    (rsnData->pairwiseCipherSuites[i] == IE_WAPI_CIPHER_SMS4))
                {
                    if ( (encryptionMode & unifi_EncryptionCipherPairwiseSms4) == unifi_EncryptionCipherPairwiseSms4)
                    {
                        pairwiseCipherSuite = rsnData->pairwiseCipherSuites[i];
                        pairwiseFound = TRUE;
                        break; /* SMS4 seems to be the only one for WAPI so stop looking here */
                    }
                }

                /* if CCMP then stop as that is the best */
                if (rsnData->pairwiseCipherSuites[i] == IE_WPA_CIPHER_CCMP ||
                    rsnData->pairwiseCipherSuites[i] == IE_WPA2_CIPHER_CCMP )
                {
                    if ( (encryptionMode & unifi_EncryptionCipherPairwiseCcmp) == unifi_EncryptionCipherPairwiseCcmp)
                    {
                        pairwiseCipherSuite = rsnData->pairwiseCipherSuites[i];
                        pairwiseFound = TRUE;
                        break; /* AES is as good as it can get so stop looking here */
                    }
                }

                if (rsnData->pairwiseCipherSuites[i] == IE_WPA_CIPHER_TKIP ||
                    rsnData->pairwiseCipherSuites[i] == IE_WPA2_CIPHER_TKIP)
                {
                    if ( (encryptionMode & unifi_EncryptionCipherPairwiseTkip) == unifi_EncryptionCipherPairwiseTkip)
                    {
                        pairwiseCipherSuite = rsnData->pairwiseCipherSuites[i];
                        pairwiseFound = TRUE;
                        /*break; Do not break as AES will override this if it exists */
                    }
                }
            }

            /* was a suitable pairwise suite found? */
            if (pairwiseFound == FALSE)
            {
                sme_trace_error((TR_SECURITY, "sec_8021xNew() :: No usable Pairwise Cipher Suite found in RSN"));
                return unifi_Unavailable;
            }
            /* ---------------------------------------------------------- */

            /* ---------------------------------------------------------- */
            /* Select the akm suite */
            /* ---------------------------------------------------------- */
            if (rsnData->akmSuiteCount == 0)
            {
                sme_trace_error((TR_SECURITY, "sec_8021xNew() :: No akm Cipher Suite in RSN"));
                return unifi_Unavailable;
            }
            for (i = 0; i < rsnData->akmSuiteCount; i++)
            {
                if (rsnData->rsntype == IE_RSN_TYPE_WPA && rsnData->akmSuites[i] == IE_WPA_AKM_PSK)
                {
                    sme_trace_debug((TR_SECURITY, "sec_8021xNew(WPA, IE_RSN_AKM_PSK)"));
                    akmSuite = rsnData->akmSuites[i];
                    akmFound = TRUE;
                    cfg->connectionInfo.authMode = unifi_8021xAuthWPAPSK;
                }
                else if (rsnData->rsntype == IE_RSN_TYPE_WPA && rsnData->akmSuites[i] == IE_WPA_AKM_PSKSA)
                {
                    sme_trace_debug((TR_SECURITY, "sec_8021xNew(WPA, IE_RSN_AKM_PSK)"));
                    akmSuite = rsnData->akmSuites[i];
                    akmFound = TRUE;
                    cfg->connectionInfo.authMode = unifi_8021xAuthWPA;
                    break;
                }
                else if (rsnData->rsntype == IE_RSN_TYPE_WPA2 && rsnData->akmSuites[i] == IE_WPA2_AKM_PSK)
                {
                    sme_trace_debug((TR_SECURITY, "sec_8021xNew(WPA, IE_RSN_AKM_PSK)"));
                    akmSuite = rsnData->akmSuites[i];
                    akmFound = TRUE;
                    cfg->connectionInfo.authMode = unifi_8021xAuthWPA2PSK;
                }
                else if (rsnData->rsntype == IE_RSN_TYPE_WPA2 && rsnData->akmSuites[i] == IE_WPA2_AKM_PSKSA)
                {
                    sme_trace_debug((TR_SECURITY, "sec_8021xNew(WPA, IE_RSN_AKM_PSK)"));
                    akmSuite = rsnData->akmSuites[i];
                    akmFound = TRUE;
                    cfg->connectionInfo.authMode = unifi_8021xAuthWPA2;
                    break;
                }
                else if (rsnData->rsntype == IE_RSN_TYPE_WAPI && rsnData->akmSuites[i] == IE_WAPI_AKM_PSK)
                {
                    sme_trace_debug((TR_SECURITY, "sec_8021xNew(WAPI, IE_WAPI_AKM_PSK)"));
                    akmSuite = rsnData->akmSuites[i];
                    akmFound = TRUE;
                    cfg->connectionInfo.authMode = unifi_WAPIAuthWAIPSK;
                }
                else if (rsnData->rsntype == IE_RSN_TYPE_WAPI && rsnData->akmSuites[i] == IE_WAPI_AKM_PSKSA)
                {
                    sme_trace_debug((TR_SECURITY, "sec_8021xNew(WPA, IE_WAPI_AKM_PSKSA)"));
                    akmSuite = rsnData->akmSuites[i];
                    akmFound = TRUE;
                    cfg->connectionInfo.authMode = unifi_WAPIAuthWAI;
                    break;
                }
            }
            if (akmFound == FALSE)
            {
                sme_trace_error((TR_SECURITY, "sec_8021xNew() :: No usable AKM Suite found in RSN"));
                return unifi_Unavailable;
            }
            /* ---------------------------------------------------------- */

            fsmData->rsnaEnabled = TRUE;
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
            if (rsnData->rsntype == IE_RSN_TYPE_WAPI)
            {
                fsmData->wapiEnabled = TRUE;
                buildWapiIe(context, rsnData, pairwiseCipherSuite, akmSuite);
            }
            else
#endif
            {
                buildSecie(context, rsnData, pairwiseCipherSuite, akmSuite);
            }
        }
    }
    return unifi_Success;
}

/**
 * @brief
 *   ready State SecurityJoinStart transition
 *
 * @par Description
 *   Checks the for the security type and credential requirements
 *   for the join either:
 *   1) Gets the user Identity
 *   2) Gets the user Credentials
 *   3) Sets the Mib data
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Start request
 *
 * @return
 *   void
 */
static void ready_start(FsmContext* context, const SecurityJoinStartReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    unifi_Status status;
    srs_scan_data* joinScanParameters = srs_get_join_scan_parameters(context);
    unifi_DataBlock scanData = {0, NULL};
    sme_trace_entry((TR_SECURITY, "ready_start()"));

    /* --------------------------------------------------- */
    /* Init the config data                                */
    /* --------------------------------------------------- */
    fsmData->connectionManagerInstance = req->common.sender_;

    /* --------------------------------------------------- */
    /* Register the security manager with the hip proxy    */
    /* --------------------------------------------------- */
    hip_proxy_register_security_manager(context, context->currentInstance->instanceId, cfg->connectionInfo.bssid);

    if (joinScanParameters != 0)
    {
        scanData.length = joinScanParameters->scanResult.informationElementsLength;
        scanData.data = joinScanParameters->scanResult.informationElements;
    }

    cfg->connectionInfo.authMode = unifi_80211AuthOpen;

    /* If Privacy Enabled */
    if (cfg->connectionConfig.privacyMode == unifi_80211PrivacyEnabled)
    {
        if (cfg->connectionConfig.authModeMask & unifi_80211AuthShared)
        {
            cfg->connectionInfo.authMode = unifi_80211AuthShared;
        }

        status = checkAndConfigForSecurity(context, scanData);
        if (status != unifi_Success)
        {
            preJoinResetToReadyOnError(context, status);
            return;
        }
    }
    sendPreConnectRequests(context);
}

/**
 * @brief
 *   ready_to_auth State SecurityConnectStart transition
 *
 * @par Description
 *   Starts the WPA/WPA2 auth if needed or opens the control port.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Start Request
 *
 * @return
 *   void
 */
static void readyToAuthConnectStart(FsmContext* context, const SecurityConnectStartReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SECURITY, "ReadyToAuth::SecurityConnectStart_request"));

    if (fsmData->rsnaEnabled == TRUE)
    {
        fsmData->scanManagerPaused = TRUE;
        (void)ns_port_configure(context, unifi_8021xPortClosedBlock, unifi_8021xPortClosedDiscard);
        send_sm_pause_req(context, getSmeContext(context)->scanManagerInstance);
        SEC_SET_FLAG(fsmData, SecFlag_securityConnectionEstablished);
        send_security_connect_start_cfm(context, fsmData->connectionManagerInstance, unifi_Success);
        fsm_next_state(context, FSMSTATE_waiting_for_keys);
    }
    else
    {
        sme_trace_info((TR_SECURITY, "CONTROLLED PORT OPEN"));
        (void)ns_port_configure(context, unifi_8021xPortOpen, unifi_8021xPortOpen);

        fsm_next_state(context, FSMSTATE_opening_control_port);
    }
}

/**
 * @brief
 *   8021x_connection_control State SecuritySetkey transition
 *
 * @par Description
 *   Message from the 8021x subsystem to pass to the firmware.
 *   A key is ready to install
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : SecuritySetkey
 *
 * @return
 *   void
 */
static void setKey8021x(FsmContext* context, const MlmeSetkeysReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    ie_rsn_ctrl rsnCtrl;
    CsrUint32 css;
    ie_rsn_data *rsnData;


    sme_trace_entry((TR_SECURITY, "Sec8021xConnectionControl::SecuritySetkey_request"));

    (void)ie_getRSNInfo(cfg->connectionInfo.assocReqInfoElements,
                         cfg->connectionInfo.assocReqInfoElementsLength,
                         &rsnCtrl, cfg->smeConfig.allowUnicastUseGroupCipher);

    sme_trace_debug((TR_SECURITY, "setKey8021x() :: rsnCtrl.authMode8021x = 0x%.4X", rsnCtrl.authMode8021x));

    /* determine the WPA/WPA2 RSN type that is ready to send to the AP */
    /* NOTE: There should only ever be one */
    if( (rsnCtrl.authMode8021x & unifi_8021xAuthWPA)
      ||(rsnCtrl.authMode8021x & unifi_8021xAuthWPAPSK))
    {
        sme_trace_debug((TR_SECURITY, "setKey8021x() :: rsnDataWPA"));
        rsnData = &rsnCtrl.rsnDataWPA;
    }
    else if( (rsnCtrl.authMode8021x & unifi_8021xAuthWPA2)
           ||(rsnCtrl.authMode8021x & unifi_8021xAuthWPA2PSK))
    {
        sme_trace_debug((TR_SECURITY, "setKey8021x() :: rsnDataWPA2"));
        rsnData = &rsnCtrl.rsnDataWPA2;
    }
    else if( (rsnCtrl.authMode8021x & unifi_WAPIAuthWAI)
           ||(rsnCtrl.authMode8021x & unifi_WAPIAuthWAIPSK))
    {
        sme_trace_debug((TR_SECURITY, "setKey8021x() :: rsnDataWAPI"));
        rsnData = &rsnCtrl.rsnDataWAPI;
    }
    else
    {
        sme_trace_error((TR_SECURITY, "setKey8021x() :: AuthMode == 0x%.4X", rsnCtrl.authMode8021x));
        return;
    }


    if (req->keyType == KeyType_Group)
    {
        sme_trace_debug((TR_SECURITY, "setKey8021x() :: KeyType_Group 0x%.8X", rsnData->groupCipherSuite));
        fsmData->groupKeyId = req->keyId;
        SEC_SET_FLAG(fsmData, SecFlag_SettingGroupKey);
        css =  (rsnData->groupCipherSuite >> 24)             |
              ((rsnData->groupCipherSuite >> 8 ) & 0xFF00)   |
              ((rsnData->groupCipherSuite << 8 ) & 0xFF0000) |
              ((rsnData->groupCipherSuite << 24) & 0xFF000000);
    }
    else
    {
        sme_trace_debug((TR_SECURITY, "setKey8021x() :: KeyType_Pairwise 0x%.8X", rsnData->pairwiseCipherSuites[0]));
        fsmData->pairwiseKeyId = req->keyId;
        SEC_SET_FLAG(fsmData, SecFlag_SettingPairWiseKey);
        css =  (rsnData->pairwiseCipherSuites[0] >> 24)             |
              ((rsnData->pairwiseCipherSuites[0] >> 8 ) & 0xFF00)   |
              ((rsnData->pairwiseCipherSuites[0] << 8 ) & 0xFF0000) |
              ((rsnData->pairwiseCipherSuites[0] << 24) & 0xFF000000);
    }

    sme_trace_info((TR_SECURITY, "SET WPA KEY (AuthMode=0x%.4X, Type=%s, Index=%d, Length = %d)",
                                 rsnCtrl.authMode8021x, trace_KeyType(req->keyType), req->keyId, req->length));

    send_mlme_setkeys_req(context,
                          req->key,
                          req->length,
                          req->keyId,
                          req->keyType,
                          req->address,
                          req->sequenceNumber,
                          req->authenticatorSupplicantorInitiatorPeer,
                          css);
}

/**
 * @brief
 *   8021x_connection_control State MlmeSetkeys transition
 *
 * @par Description
 *   Message from the firmware to say that the key has been installed.
 *   A key has been installed
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : MlmeSetkeys
 *
 * @return
 *   void
 */
static void setKeyCfm8021x(FsmContext* context, const MlmeSetkeysCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_SECURITY, "Sec8021xConnectionControl::MlmeSetkeys_confirm(0x%.4X)", cfm->resultCode));

    /* Do not set protection on a rekey */
    if (SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey) && SEC_IS_SET(fsmData, SecFlag_HaveGroupKey))
    {
        sme_trace_info((TR_SECURITY, "CONTROLLED PORT OPEN"));

        if (SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKey))
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_SettingPairWiseKey);
        }
        else if (SEC_IS_SET(fsmData, SecFlag_SettingGroupKey))
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_SettingGroupKey);
        }

        /* Tell coex we have the keys */
        coex_wpa_keys_installed(context);

        /* update the candidate list */
        srs_build_pre_auth_candidate_list(context);

        /* only change state if the port required configuring */
        if (ns_port_configure(context, unifi_8021xPortOpen, unifi_8021xPortOpen))
        {
            fsm_next_state(context, FSMSTATE_opening_control_port);
        }
        return;
    }

    if (SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKey))
    {
        SEC_CLEAR_FLAG(fsmData, SecFlag_SettingPairWiseKey);

        if (cfm->resultCode == ResultCode_Success &&
            !SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey))
        {
            ie_rsn_ctrl rsnCtrl;
            CsrBool isWpa = FALSE;

            (void)ie_getRSNInfo(cfg->connectionInfo.assocReqInfoElements,
                                 cfg->connectionInfo.assocReqInfoElementsLength,
                                 &rsnCtrl, cfg->smeConfig.allowUnicastUseGroupCipher);

            if( (rsnCtrl.authMode8021x & unifi_8021xAuthWPA)    ||
                (rsnCtrl.authMode8021x & unifi_8021xAuthWPAPSK) ||
                (rsnCtrl.authMode8021x & unifi_8021xAuthWPA2)   ||
                (rsnCtrl.authMode8021x & unifi_8021xAuthWPA2PSK) )
            {
                isWpa = TRUE;
            }

            /* When in the control_port_open state this will always be a rekey.
             * The M4 packet is used in Wpa/Wpa2 neggotiations */
            if (fsm_current_state(context) == FSMSTATE_control_port_open || !isWpa)
            {
                SEC_SET_FLAG(fsmData, SecFlag_SettingPairWiseKeyRxTxProtection);
                send_mlme_setprotection_req(context, cfg->connectionInfo.bssid, ProtectType_RxTx, KeyType_Pairwise);
            }
            else
            {
                SEC_SET_FLAG(fsmData, SecFlag_SettingPairWiseKeyRxProtection);
                send_mlme_setprotection_req(context, cfg->connectionInfo.bssid, ProtectType_Rx, KeyType_Pairwise);
            }
        }
        return;
    }
    if (SEC_IS_SET(fsmData, SecFlag_SettingGroupKey))
    {
        SEC_CLEAR_FLAG(fsmData, SecFlag_SettingGroupKey);

        if (cfm->resultCode == ResultCode_Success &&
            !SEC_IS_SET(fsmData, SecFlag_HaveGroupKey))
        {
            SEC_SET_FLAG(fsmData, SecFlag_SettingGroupKeyProtection);
            send_mlme_setprotection_req(context, cfg->connectionInfo.bssid, ProtectType_RxTx, KeyType_Group);
        }
        return;
    }
}

/**
 * @brief
 *   8021x_connection_control State MlmeSetprotection transition
 *
 * @par Description
 *   Message from the firmware to say that the key protections have been set.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : MlmeSetprotection
 *
 * @return
 *   void
 */
static void setSetProtectionCfm8021x(FsmContext* context, const MlmeSetprotectionCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SECURITY, "Sec8021xConnectionControl::MlmeSetprotection_confirm(0x%.4X)", cfm->resultCode));

    if (SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxProtection))
    {
        SEC_CLEAR_FLAG(fsmData, SecFlag_SettingPairWiseKeyRxProtection);

        /* Request the Router transmit m4 now */
        call_unifi_sys_m4_transmit_req(context);

        if (SEC_IS_SET(fsmData, SecFlag_SettingGroupKeyProtection))
        {
            sme_trace_debug((TR_SECURITY, "setSetProtectionCfm8021x() set pairwise waiting to set group protection"));
            return;
        }
    }
    else if (SEC_IS_SET(fsmData, SecFlag_SettingGroupKeyProtection))
    {
        SEC_CLEAR_FLAG(fsmData, SecFlag_SettingGroupKeyProtection);

        if (cfm->resultCode == ResultCode_Success)
        {
            SEC_SET_FLAG(fsmData, SecFlag_HaveGroupKey);
        }

        if (SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxProtection)  ||
            SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxTxProtection) )
        {
            sme_trace_debug((TR_SECURITY, "setSetProtectionCfm8021x() set group waiting to set pairwise protection"));
            return;
        }
    }
    else if (SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxTxProtection))
    {
        SEC_CLEAR_FLAG(fsmData, SecFlag_SettingPairWiseKeyRxTxProtection);

        if (cfm->resultCode == ResultCode_Success)
        {
            SEC_SET_FLAG(fsmData, SecFlag_HavePairWiseKey);
        }

        if (SEC_IS_SET(fsmData, SecFlag_SettingGroupKeyProtection))
        {
            sme_trace_debug((TR_SECURITY, "setSetProtectionCfm8021x() set pairwise waiting to set group protection"));
            return;
        }
    }
    else
    {
        sme_trace_error((TR_SECURITY, "setSetProtectionCfm8021x() Unexpected MLME_SETPROTECTION_CFM ignoring"));
        return;
    }

    if (SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey) && SEC_IS_SET(fsmData, SecFlag_HaveGroupKey))
    {
        sme_trace_info((TR_SECURITY, "CONTROLLED PORT OPEN"));

        /* Tell coex we have the keys */
        coex_wpa_keys_installed(context);

        fsmData->scanManagerPaused = FALSE;
        send_sm_unpause_req(context, getSmeContext(context)->scanManagerInstance);
        (void)ns_port_configure(context, unifi_8021xPortOpen, unifi_8021xPortOpen);
        fsm_next_state(context, FSMSTATE_opening_control_port);
    }
}


/**
 * @brief
 *   8021x 4 message has been transmitted.
 *
 * @par Description
 *   Message from the firmware to say that a Michael Mic failure has been detected.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void m4MessageTransmitted(FsmContext* context, const UnifiSysM4TransmittedInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_SECURITY, "m4MessageTransmitted()"));

    SEC_SET_FLAG(fsmData, SecFlag_SettingPairWiseKeyRxTxProtection);
    send_mlme_setprotection_req(context, cfg->connectionInfo.bssid, ProtectType_RxTx, KeyType_Pairwise);
}

/**
 * @brief
 *   8021x_connection_control State MlmeMichaelmicfailure transition
 *
 * @par Description
 *   Message from the firmware to say that a Michael Mic failure has been detected.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : MlmeMichaelmicfailure
 *
 * @return
 *   void
 */
static void michaelMicFailure8021x(FsmContext* context, const MlmeMichaelmicfailureInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    void* appHandles;
    CsrUint16 appHandleCount = get_appHandles(context, unifi_IndMicFailure, &appHandles);
    sme_trace_entry((TR_SECURITY, "Sec8021xConnectionControl::MlmeMichaelmicfailure_indication"));

    if (appHandleCount)
    {
        call_unifi_mgt_mic_failure_ind(context,
                                       appHandleCount,
                                       appHandles,
                                       SEC_IS_SET(fsmData, SecFlag_MichealMicFailureReceived),
                                       ind->count,
                                       &ind->address,
                                       ind->keyType,
                                       ind->keyId,
                                       ind->tsc); /*lint !e730*/ /* These types are compatable */
    }
    if (SEC_IS_SET(fsmData, SecFlag_MichealMicFailureReceived))
    {
        /* BSSID FOR 60 Seconds */
        black_list_add(context, &cfg->connectionInfo.bssid, 60000);

        SEC_SET_FLAG(fsmData, SecFlag_MichealMicFailureInvokeCounterMeasures);
        fsmData->status = unifi_Error;

        fsm_remove_timer(context, fsmData->michealMicFailureTimerId);
        return;
    }

    SEC_SET_FLAG(fsmData, SecFlag_MichealMicFailureReceived);
    sme_trace_entry((TR_SECURITY, "setMicFailureTimer(%d)"));

    send_security_mlme_michael_mic_failure_timer(context, fsmData->michealMicFailureTimerId, 60000, 2000);
}

/**
 * @brief
 *   8021x_connection_control State MlmeMichaelmicfailure_timer Timeout transition
 *
 * @par Description
 *   The MlmeMichaelmicfailure_timer has expired so clear the michealMicFailureReceived
 *   flag. This means that 60 seconds have passed with only 1 failure
 *
 * @param[in]    context   : FSM context
 * @param[in]    timer     : MlmeMichaelmicfailure_timer
 *
 * @return
 *   void
 */
static void michealMicFailureTimout8021x(FsmContext* context, const SecurityMlmeMichaelMicFailureTimer_Evt* timer)
{
    sme_trace_entry((TR_SECURITY, "Sec8021xConnectionControl::michealMicFailureTimer"));
    /* No more MIC failures in 1 minute so clear the flag */
    SEC_CLEAR_FLAG(FSMDATA, SecFlag_MichealMicFailureReceived);
}

/**
 * @brief
 *   Initiate security shutdown
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void initiate_termination(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_SECURITY, "initiate_termination()"));

    if (fsmData->scanManagerPaused == TRUE)
    {
        send_sm_unpause_req(context, getSmeContext(context)->scanManagerInstance);
    }

    if (SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey) || SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKey))
    {
        SEC_SET_FLAG(fsmData, SecFlag_HavePairWiseKey);
        send_mlme_deletekeys_req(context, fsmData->pairwiseKeyId, KeyType_Pairwise, cfg->connectionInfo.bssid);
    }
    if (SEC_IS_SET(fsmData, SecFlag_HaveGroupKey) || SEC_IS_SET(fsmData, SecFlag_SettingGroupKey))
    {
        SEC_SET_FLAG(fsmData, SecFlag_HaveGroupKey);
        send_mlme_deletekeys_req(context, fsmData->groupKeyId, KeyType_Group, cfg->connectionInfo.bssid);
    }

    if (fsmData->scanManagerPaused == FALSE &&
        !SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKey) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxProtection) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxTxProtection) &&
        !SEC_IS_SET(fsmData, SecFlag_HaveGroupKey) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingGroupKey) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingGroupKeyProtection) )
    {
        sme_trace_debug((TR_SECURITY, "initiate_termination() No need to wait for any cfms. Terminating"));
        waitForTerminateOnStopOrError(context);
    }
    else
    {
        fsm_next_state(context, FSMSTATE_waiting_for_mlme_cfm);
    }
}

/**
 * @brief
 *   opening_control_port State UnifiSysControlledPortConfigure transition
 *
 * @par Description
 *   Response from the driver to indicate the control port state
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : UnifiSysControlledPortConfigure
 *
 * @return
 *   void
 */
static void openingControlPortCfm(FsmContext* context, const UnifiSysPortConfigureCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SECURITY, "OpeningControlPort::UnifiSysControlledPortConfigure_confirm"));

    /* --------------------------------------------------------------------- */
    /* CHECK FOR CONNECT STOP */
    /* --------------------------------------------------------------------- */
    if (SEC_IS_SET(fsmData, SecFlag_securityConnectStopReceived))
    {
        initiate_termination(context);
        return;
    }
    /* --------------------------------------------------------------------- */

    if (!SEC_IS_SET(fsmData, SecFlag_securityConnectionEstablished))
    {
        SEC_SET_FLAG(fsmData, SecFlag_securityConnectionEstablished);
        send_security_connect_start_cfm(context, fsmData->connectionManagerInstance, unifi_Success);
    }

    fsm_next_state(context, FSMSTATE_control_port_open);
}

/**
 * @brief
 *   SecurityConnectStop requested
 *
 * @par Description
 *   Stop the connection.
 *   Close the control port
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : SecurityConnectStop
 *
 * @return
 *   void
 */
static void connectedStop(FsmContext* context, const SecurityConnectStopReq_Evt* req)
{
    sme_trace_entry((TR_SECURITY, "ControlPortOpen::SecurityConnectStop_request"));
    SEC_SET_FLAG(FSMDATA, SecFlag_securityConnectStopReceived);
    initiate_termination(context);
}

/**
 * @brief
 *   Wait for mlme cfms when terminating
 *
 * @par Description
 *   Wait for all the mlme key and protection cfm messages
 *   Then terminate
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : mlme cfm message
 *
 * @return
 *   void
 */
static void waitforCfms(FsmContext* context, const FsmEvent* cfm)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SECURITY, "waitforCfms()"));

    switch(cfm->id)
    {
    case MLME_SETKEYS_CFM_ID:
        if (SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKey))
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_SettingPairWiseKey);
        }
        else if (SEC_IS_SET(fsmData, SecFlag_SettingGroupKey))
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_SettingGroupKey);
        }
        else
        {
            sme_trace_error((TR_SECURITY, "waitforCfms() Unexpected MLME_SETKEYS_CFM"));
        }
        break;
    case MLME_SETPROTECTION_CFM_ID:
        if (SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxProtection))
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_SettingPairWiseKeyRxProtection);
        }
        else if (SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxTxProtection))
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_SettingPairWiseKeyRxTxProtection);
        }
        else if (SEC_IS_SET(fsmData, SecFlag_SettingGroupKeyProtection))
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_SettingGroupKeyProtection);
        }
        else
        {
            sme_trace_error((TR_SECURITY, "waitforCfms() Unexpected MLME_SETPROTECTION_CFM"));
        }
        break;
    case MLME_DELETEKEYS_CFM_ID:
        if (SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey))
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_HavePairWiseKey);
        }
        else if (SEC_IS_SET(fsmData, SecFlag_HaveGroupKey))
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_HaveGroupKey);
        }
        else
        {
            sme_trace_error((TR_SECURITY, "waitforCfms() Unexpected MLME_DELETEKEYS_CFM"));
        }
        break;
    case SM_UNPAUSE_CFM_ID:
        fsmData->scanManagerPaused = FALSE;
        break;
    default:
        break;
    };

    if (fsmData->scanManagerPaused == FALSE &&
        !SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKey) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxProtection) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKeyRxTxProtection) &&
        !SEC_IS_SET(fsmData, SecFlag_HaveGroupKey) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingGroupKey) &&
        !SEC_IS_SET(fsmData, SecFlag_SettingGroupKeyProtection) )
    {
        sme_trace_debug((TR_SECURITY, "waitforCfms() All cfm messages received. Terminating"));
        waitForTerminateOnStopOrError(context);
    }
    else
    {
        sme_trace_debug((TR_SECURITY, "waitforCfms() still waiting flags = 0x%x", fsmData->secFlags));
    }

}

/**
 * @brief
 *   "*" State SecurityConnectStop transition
 *
 * @par Description
 *   Tracks the reception of the SecurityConnectStop and sets the flags
 *   for later checking.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : SecurityConnectStop
 *
 * @return
 *   void
 */
static void stopReqSetFlag(FsmContext* context, const SecurityConnectStopReq_Evt* req)
{
    sme_trace_entry((TR_SECURITY, "* SecurityConnectStop_request Received : Flag set"));
    SEC_SET_FLAG(FSMDATA, SecFlag_securityConnectStopReceived);

    if (fsm_current_state(context) == FSMSTATE_ready ||
        fsm_current_state(context) == FSMSTATE_ready_to_auth)
    {
        (void)waitForTerminateOnStop(context);
    }
}

/**
 * @brief
 *   waiting_for_terminate State Stop transition
 *
 * @par Description
 *   Tracks the reception of the SecurityConnectStop and sets the flags
 *   for later checking.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : SecurityConnectStop
 *
 * @return
 *   void
 */
static void terminate_stop_req(FsmContext* context, const SecurityConnectStopReq_Evt* req)
{
    sme_trace_entry((TR_SECURITY, "waiting_for_terminate::terminate_stop_req Received"));
    send_security_connect_stop_cfm(context, FSMDATA->connectionManagerInstance, TRUE);
}

/**
 * @brief
 *   waiting_for_terminate State terminate transition
 *
 * @par Description
 *   Indication that the security Manager can terminate
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void terminate(FsmContext* context, const SecurityTerminateReq_Evt* req)
{
    sme_trace_entry((TR_SECURITY, "waiting_for_terminate::terminate"));

    /* --------------------------------------------------- */
    /* Deregister the security manager with the hip proxy  */
    /* --------------------------------------------------- */
    hip_proxy_deregister_security_manager(context, context->currentInstance->instanceId);

    securityManagerReset(context);
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in security_manager_fsm/security_manager_fsm.h
 */
CsrBool get_security_rsna_enabled(FsmContext* context, CsrUint16 instanceId)
{
    require(TR_SECURITY, instanceId < context->maxProcesses);
    require(TR_SECURITY, context->instanceArray[instanceId].fsmInfo->processId == SECURITY_MANAGER_PROCESS);

    return (fsm_get_params_by_id(context, instanceId, FsmData))->rsnaEnabled;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in security_manager_fsm/security_manager_fsm.h
 */
CsrBool check_have_set_keys(FsmContext* context, CsrUint16 instanceId)
{
    FsmData* fsmData;

    if (instanceId == FSM_TERMINATE)
    {
        return FALSE;
    }

    fsmData = fsm_get_params_by_id(context, instanceId, FsmData);

    require(TR_SECURITY, instanceId < context->maxProcesses);
    require(TR_SECURITY, context->instanceArray[instanceId].fsmInfo->processId == SECURITY_MANAGER_PROCESS);

    return SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey) && SEC_IS_SET(fsmData, SecFlag_HaveGroupKey);
}

/* FSM DEFINITION **********************************************/

/** State ready transitions */
static const FsmEventEntry readyTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(SECURITY_JOIN_START_REQ_ID,                   ready_start ),
};

/** State readyToAuth transitions */
static const FsmEventEntry readyToAuthTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(SECURITY_CONNECT_START_REQ_ID,                readyToAuthConnectStart ),
};

/** State s8021xConnectionControl transitions */
static const FsmEventEntry waitingForKeysTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(MLME_SETKEYS_REQ_ID,                          setKey8021x ),

    fsm_event_table_entry(MLME_SETKEYS_CFM_ID,                          setKeyCfm8021x ),
    fsm_event_table_entry(MLME_SETPROTECTION_CFM_ID,                    setSetProtectionCfm8021x ),
    fsm_event_table_entry(UNIFI_SYS_M4_TRANSMITTED_IND_ID,              m4MessageTransmitted ),

    fsm_event_table_entry(SECURITY_CONNECT_STOP_REQ_ID,                 connectedStop ),
    fsm_event_table_entry(MLME_MICHAELMICFAILURE_IND_ID,                michaelMicFailure8021x ),
    fsm_event_table_entry(SECURITY_MLME_MICHAEL_MIC_FAILURE_TIMER_ID,   michealMicFailureTimout8021x ),
};


/** State openingControlPort transitions */
static const FsmEventEntry openingControlPortTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_SYS_PORT_CONFIGURE_CFM_ID,              openingControlPortCfm ),
    fsm_event_table_entry(MLME_EAPOL_CFM_ID,                            fsm_saved_event ),
    fsm_event_table_entry(MLME_SETKEYS_CFM_ID,                          fsm_saved_event ),
    fsm_event_table_entry(MLME_MICHAELMICFAILURE_IND_ID,                michaelMicFailure8021x ),
    fsm_event_table_entry(SECURITY_MLME_MICHAEL_MIC_FAILURE_TIMER_ID,   michealMicFailureTimout8021x ),
};

/** State controlPortOpen transitions */
static const FsmEventEntry controlPortOpenTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(SECURITY_CONNECT_STOP_REQ_ID,                 connectedStop ),

    fsm_event_table_entry(MLME_MICHAELMICFAILURE_IND_ID,                michaelMicFailure8021x ),
    fsm_event_table_entry(SECURITY_MLME_MICHAEL_MIC_FAILURE_TIMER_ID,   michealMicFailureTimout8021x ),

    fsm_event_table_entry(MLME_SETKEYS_REQ_ID,                          setKey8021x ),

    fsm_event_table_entry(MLME_SETKEYS_CFM_ID,                          setKeyCfm8021x ),
    fsm_event_table_entry(MLME_SETPROTECTION_CFM_ID,                    setSetProtectionCfm8021x ),
};

/** State waitingForDeletePairwiseKey transitions */
static const FsmEventEntry waitingForMlmeCfmsTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(MLME_SETKEYS_CFM_ID,                          waitforCfms),
    fsm_event_table_entry(MLME_SETPROTECTION_CFM_ID,                    waitforCfms),
    fsm_event_table_entry(MLME_DELETEKEYS_CFM_ID,                       waitforCfms),
    fsm_event_table_entry(SM_UNPAUSE_CFM_ID,                            waitforCfms),
};


/** State waitingForDeleteGroupKey transitions */
static const FsmEventEntry waitingForTerminateTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(SECURITY_CONNECT_STOP_REQ_ID,                 terminate_stop_req ),
    fsm_event_table_entry(SECURITY_TERMINATE_REQ_ID,                    terminate ),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(SECURITY_CONNECT_STOP_REQ_ID,                 stopReqSetFlag ),
    fsm_event_table_entry(MLME_EAPOL_CFM_ID,                            fsm_ignore_event ),

    fsm_event_table_entry(MA_UNITDATA_CFM_ID,                           fsm_ignore_event ),
    fsm_event_table_entry(MLME_PROTECTEDFRAMEDROPPED_IND_ID,            fsm_ignore_event ),
    fsm_event_table_entry(MLME_SETKEYS_CFM_ID,                          fsm_ignore_event ),
    fsm_event_table_entry(MLME_DELETEKEYS_CFM_ID,                       fsm_ignore_event ),
    fsm_event_table_entry(MLME_SETPROTECTION_CFM_ID,                    fsm_ignore_event ),
    fsm_event_table_entry(UNIFI_SYS_PORT_CONFIGURE_CFM_ID,              fsm_ignore_event ),

    fsm_event_table_entry(SM_PAUSE_CFM_ID,                              fsm_ignore_event),
    fsm_event_table_entry(SM_UNPAUSE_CFM_ID,                            fsm_ignore_event),

    fsm_event_table_entry(MLME_MICHAELMICFAILURE_IND_ID,                fsm_ignore_event ),
    fsm_event_table_entry(SECURITY_MLME_MICHAEL_MIC_FAILURE_TIMER_ID,   fsm_ignore_event ),
    fsm_event_table_entry(UNIFI_SYS_M4_TRANSMITTED_IND_ID,              fsm_ignore_event ),

};

/** Security Manager state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                         State                                   State                                   Save     */
   /*                         Name                                    Transitions                               *      */
   fsm_state_table_entry(FSMSTATE_ready,                           readyTransitions,                          FALSE),
   fsm_state_table_entry(FSMSTATE_ready_to_auth,                   readyToAuthTransitions,                    FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_for_keys,                waitingForKeysTransitions,                 FALSE),
   fsm_state_table_entry(FSMSTATE_opening_control_port,            openingControlPortTransitions,             TRUE),
   fsm_state_table_entry(FSMSTATE_control_port_open,               controlPortOpenTransitions,                FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_for_mlme_cfm,            waitingForMlmeCfmsTransitions,             TRUE ),
   fsm_state_table_entry(FSMSTATE_waiting_for_terminate,           waitingForTerminateTransitions,            FALSE )
};

const FsmProcessStateMachine security_manager_fsm = {
#ifdef FSM_DEBUG
       "Security Mgr",                                              /* SM Process Name       */
#endif
       SECURITY_MANAGER_PROCESS,                                    /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                            /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),      /* Ignore Event handers */
       securityManagerInit,                                         /* Entry Function        */
       securityManagerReset,                                        /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                         /* Trace Dump Function   */
#endif
};

/*
 * FSM Scripts Config for this FSM
 *                              state                                       nextstate                  event
 *                              -----                                       ---------                  -----
 * fsm::accept_transition       closing_control_port                        waiting_for_terminate      .*
 * fsm::accept_transition       waiting_for_delete_group_key                waiting_for_terminate      .*
 * fsm::remove_transition       .*                                          waiting_for_terminate      .*
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/** @}
 */
