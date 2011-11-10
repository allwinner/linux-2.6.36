/** @file sme_configuration_fsm.c
 *
 * Public SME Configuration FSM API
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
 *   Public SME configuration FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_configuration/sme_configuration_fsm.c#11 $
 *
 ****************************************************************************/

/** @{
 * @ingroup sme_configuration
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "fsm/csr_wifi_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "link_quality_fsm/link_quality_fsm.h"
#include "coex_fsm/coex_fsm.h"

#include "version/version.h"
#include "version/hip_version.h"

#include "mgt_sap/mgt_sap_from_sme_interface.h"
#include "sys_sap/sys_sap_from_sme_interface.h"
#include "scan_manager_fsm/scan_results_storage.h"
#include "scan_manager_fsm/scan_manager_fsm.h"
#include "scan_manager_fsm/roaming_channel_lists.h"

#include "hip_proxy_fsm/hip_signal_proxy_fsm.h"
#include "hip_proxy_fsm/mib_encoding.h"
#include "hip_proxy_fsm/mib_utils.h"
#include "ie_access/ie_access.h"

#ifdef CCX_VARIANT
#include "ccx_fsm/ccx_defaults.h"
#endif

#include "smeio/smeio_trace_types.h"

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
 * Number of seconds a scan is valid for when not connected
 */
#define DEFAULT_DISCONNECTED_SCAN_PERIOD_SEC   20          /* seconds */
#define DEFAULT_DISCONNECTED_SCAN_VALIDITY_SEC 39          /* seconds */

/**
 * Number of seconds a scan is valid for when connected and not sending data
 */
#define DEFAULT_CONNECTED_SCAN_PERIOD_SEC       15    /* seconds */
#define DEFAULT_CONNECTED_SCAN_VALIDITY_SEC     29    /* seconds */

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
    FSMSTATE_waiting_for_cfm,
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
    SmeConfigData     config;

    CsrUint16         multicastAddressesId;
    CsrBool           hostKey;
    CsrBool           hostMulticast;
    unifi_ListAction  action;
    CsrUint16         getSetRequester;
    unifi_AppValueId  getSetType;
    void*             cfmAppHandle;
} FsmData;


/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   SME configuration FSM Entry Function
 *
 * @par Description
 *   Called on SME configuration Process startup to initialise
 *   the security manager data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void sme_configuration_init(FsmContext* context)
{
    FsmData* fsmData;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "sme_configuration_init()"));

    fsm_create_params(context, FsmData);
    fsmData = FSMDATA;

    /* -------------------------------------------- */
    /* 1 time Initialise                            */
    /* -------------------------------------------- */
    fsmData->config.wifiOff = TRUE;

    /* Versions Setup */
    CsrMemSet(&fsmData->config.versions, 0x00, sizeof(fsmData->config.versions));
    fsmData->config.versions.smeBuild = buildIdNum;
    fsmData->config.versions.smeVariant = buildVariant;
    fsmData->config.versions.smeHip = SME_SUPPORTED_HIP_VERSION;
    fsmData->config.versions.smeIdString = (char*) getSmeVersion();

    fsmData->config.permanentMacAddress = NullBssid;
    fsmData->config.stationMacAddress = NullBssid;
    fsmData->config.calibrationData.dataLength = 0;
    CsrMemSet(fsmData->config.lastJoinedAddresses, 0xFF, sizeof(fsmData->config.lastJoinedAddresses));
    CsrMemSet(fsmData->config.micFailureBlackList, 0xFF, sizeof(fsmData->config.micFailureBlackList));
    fsmData->config.pmk_cache = NULL;
    fsmData->config.multicastAddresses.addressesCount = 0;
    fsmData->config.multicastAddresses.addresses = NULL;
    fsmData->hostKey = FALSE;
    fsmData->hostMulticast = FALSE;

    fsmData->config.restrictedAccessActivated = FALSE;
    fsmData->config.restrictedAccessAppHandle = NULL;

    fsmData->config.userBlackList.addressesCount = 0;
    fsmData->config.userBlackList.addresses = NULL;

    fsmData->config.indRegistrationSize = 0;
    fsmData->config.indRegistrationCount = 0;
    fsmData->config.indRegistrations = NULL;
    fsmData->config.tmpIndList = NULL;

    fsmData->config.cloakedSsids.cloakedSsidsCount = 0;
    fsmData->config.cloakedSsids.cloakedSsids = NULL;

    CsrMemSet(&fsmData->config.mibFiles, 0x00, sizeof(fsmData->config.mibFiles));

    fsmData->config.regdomReadFromMib = FALSE;
    fsmData->config.dot11CurrentRegDomain = unifi_RegulatoryDomainFcc;

    /* Null out connection data not set until first connection */
    CsrMemSet(&fsmData->config.connectionInfo, 0x00, sizeof(fsmData->config.connectionInfo));
    CsrMemSet(&fsmData->config.connectionConfig, 0x00, sizeof(fsmData->config.connectionConfig));
    fsmData->config.connectionConfig.authModeMask = unifi_80211AuthOpen;
    fsmData->config.disassocReason = unifi_IEEE80211ReasonSuccess;
    fsmData->config.deauthReason = unifi_IEEE80211ReasonSuccess;

    /* Adhoc Defaults */
    fsmData->config.adHocConfig.atimWindowTu = 0;
    fsmData->config.adHocConfig.beaconPeriodTu = 100;
    fsmData->config.adHocConfig.joinOnlyAttempts = 1;
    fsmData->config.adHocConfig.joinAttemptIntervalMs = 2000;

    /* Coex Defaults */
    fsmData->config.coexConfig.coexEnable = TRUE;
    fsmData->config.coexConfig.coexAfhChannelEnable = TRUE;
    fsmData->config.coexConfig.coexAdvancedEnable = TRUE;
    fsmData->config.coexConfig.coexEnableSchemeManagement = FALSE;
    fsmData->config.coexConfig.coexDirection = unifi_CoexDirectionDot11Input;
    fsmData->config.coexConfig.coexPeriodicWakeHost = FALSE;
    fsmData->config.coexConfig.coexTrafficBurstyLatencyMs = 60;
    fsmData->config.coexConfig.coexTrafficContinuousLatencyMs = 10;
    fsmData->config.coexConfig.coexObexBlackoutDurationMs = 20;
    fsmData->config.coexConfig.coexObexBlackoutPeriodMs = 40;
    fsmData->config.coexConfig.coexA2dpBlackoutDurationMs = 75;
    fsmData->config.coexConfig.coexA2dpBlackoutPeriodMs = 95;

    /* Host Defaults */
    fsmData->config.hostConfig.powerMode = unifi_HostPowersave;
    fsmData->config.hostConfig.applicationDataPeriodMs = 0; /* 0 = No periodic data application */

    /* Power Defaults */
    fsmData->config.powerConfig.powerSaveLevel = unifi_PowerSaveAuto;
    fsmData->config.powerConfig.listenIntervalBeacons = 500;
    fsmData->config.powerConfig.rxDtims = TRUE;
    fsmData->config.powerConfig.d3AutoScanMode = unifi_PSOff;

    /* Sme Defaults */
    fsmData->config.smeConfig.connectionQualityRssiChangeTrigger = 10;
    fsmData->config.smeConfig.connectionQualitySnrChangeTrigger  = 10;
    fsmData->config.smeConfig.trustLevel = unifi_TrustStrict;
    fsmData->config.smeConfig.countryCode[0] = '0';
    fsmData->config.smeConfig.countryCode[1] = '0';
    fsmData->config.smeConfig.wmmModeMask = unifi_WmmEnabled;
    fsmData->config.smeConfig.ifIndex = unifi_GHZ_2_4;
    fsmData->config.smeConfig.firmwareDriverInterface = unifi_UnitDataInterface;
    fsmData->config.smeConfig.allowUnicastUseGroupCipher = TRUE;
    fsmData->config.smeConfig.enableStrictDraftN = FALSE;
    fsmData->config.smeConfig.enableRestrictedAccess = FALSE;
    fsmData->config.smeConfig.enableOpportunisticKeyCaching = TRUE;

    /* Scanning Defaults */
    fsmData->config.scanConfig.disableAutonomousScans = FALSE;
    fsmData->config.scanConfig.maxResults = SCAN_RECORD_LIST_MAX;
    fsmData->config.scanConfig.highRSSIThreshold  = -128;
    fsmData->config.scanConfig.lowRSSIThreshold   = -128;
    fsmData->config.scanConfig.deltaRSSIThreshold = 3;
    fsmData->config.scanConfig.highSNRThreshold   = -128;
    fsmData->config.scanConfig.lowSNRThreshold    = -128;
    fsmData->config.scanConfig.deltaSNRThreshold  = 3;
    fsmData->config.scanConfig.passiveChannelListCount = 0;
    fsmData->config.scanConfig.passiveChannelList = NULL;

    /* Experiments have shown that optimum values for MinChannelTime and MaxChannelTime
     * for active scanning are 10 and 30 (TU) respectively.
     * For passive scanning we use the same values
     */
    fsmData->config.scanConfig.scanCfg[unifi_Unusable].intervalSeconds            =  15; /* Seconds   */
    fsmData->config.scanConfig.scanCfg[unifi_Unusable].validitySeconds            =  29; /* Seconds   */
    fsmData->config.scanConfig.scanCfg[unifi_Unusable].minActiveChannelTimeTu     =  10; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_Unusable].maxActiveChannelTimeTu     =  30; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_Unusable].minPassiveChannelTimeTu    = 210; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_Unusable].maxPassiveChannelTimeTu    = 210; /* TimeUnits */

    fsmData->config.scanConfig.scanCfg[unifi_Poor].intervalSeconds                =  60; /* Seconds   */
    fsmData->config.scanConfig.scanCfg[unifi_Poor].validitySeconds                = 119; /* Seconds   */
    fsmData->config.scanConfig.scanCfg[unifi_Poor].minActiveChannelTimeTu         =  10; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_Poor].maxActiveChannelTimeTu         =  30; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_Poor].minPassiveChannelTimeTu        = 210; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_Poor].maxPassiveChannelTimeTu        = 210; /* TimeUnits */

    fsmData->config.scanConfig.scanCfg[unifi_Satisfactory].intervalSeconds          = 150; /* Seconds   */
    fsmData->config.scanConfig.scanCfg[unifi_Satisfactory].validitySeconds          = 299; /* Seconds   */
    fsmData->config.scanConfig.scanCfg[unifi_Satisfactory].minActiveChannelTimeTu   =  10; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_Satisfactory].maxActiveChannelTimeTu   =  30; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_Satisfactory].minPassiveChannelTimeTu  = 210; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_Satisfactory].maxPassiveChannelTimeTu  = 210; /* TimeUnits */

    fsmData->config.scanConfig.scanCfg[unifi_NotConnected].intervalSeconds          =  60; /* Seconds   */
    fsmData->config.scanConfig.scanCfg[unifi_NotConnected].validitySeconds          = 119; /* Seconds   */
    fsmData->config.scanConfig.scanCfg[unifi_NotConnected].minActiveChannelTimeTu   =  10; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_NotConnected].maxActiveChannelTimeTu   =  30; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_NotConnected].minPassiveChannelTimeTu  = 210; /* TimeUnits */
    fsmData->config.scanConfig.scanCfg[unifi_NotConnected].maxPassiveChannelTimeTu  = 210; /* TimeUnits */

    fsmData->config.roamingConfig.roamScanCfg[unifi_Unusable].intervalSeconds             =   3; /* Seconds   */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Unusable].validitySeconds             =   5; /* Seconds   */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Unusable].minActiveChannelTimeTu      =  10; /* TimeUnits */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Unusable].maxActiveChannelTimeTu      =  30; /* TimeUnits */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Unusable].minPassiveChannelTimeTu     = 210; /* TimeUnits */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Unusable].maxPassiveChannelTimeTu     = 210; /* TimeUnits */

    fsmData->config.roamingConfig.roamScanCfg[unifi_Poor].intervalSeconds                 =   3; /* Seconds   */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Poor].validitySeconds                 =   5; /* Seconds   */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Poor].minActiveChannelTimeTu          =  10; /* TimeUnits */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Poor].maxActiveChannelTimeTu          =  30; /* TimeUnits */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Poor].minPassiveChannelTimeTu         = 210; /* TimeUnits */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Poor].maxPassiveChannelTimeTu         = 210; /* TimeUnits */

    fsmData->config.roamingConfig.roamScanCfg[unifi_Satisfactory].intervalSeconds         =   6; /* Seconds   */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Satisfactory].validitySeconds         =  11; /* Seconds   */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Satisfactory].minActiveChannelTimeTu  =  10; /* TimeUnits */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Satisfactory].maxActiveChannelTimeTu  =  30; /* TimeUnits */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Satisfactory].minPassiveChannelTimeTu = 210; /* TimeUnits */
    fsmData->config.roamingConfig.roamScanCfg[unifi_Satisfactory].maxPassiveChannelTimeTu = 210; /* TimeUnits */

    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].rssiHighThreshold         =  RSSI_LIMIT_BEST;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].rssiLowThreshold          =  RSSI_THRESHOLD_SATISFACTORY - THRESHOLD_OFFSET;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].snrHighThreshold          =  SNR_LIMIT_BEST;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].snrLowThreshold           =  SNR_THRESHOLD_SATISFACTORY - THRESHOLD_OFFSET;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].monitorInterval           =  6;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].monitorWindow             =  0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].dot11RetryRatio           = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].dot11MultipleRetryRatio   = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].dot11AckFailureRatio      = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].dot11FcsErrorRatio        = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].dot11RtsFailureRatio      = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand1].beaconLossThreshold       = 0;

    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].rssiHighThreshold         =  RSSI_THRESHOLD_SATISFACTORY + THRESHOLD_OFFSET;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].rssiLowThreshold          =  RSSI_THRESHOLD_POOR - THRESHOLD_OFFSET;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].snrHighThreshold          =  SNR_THRESHOLD_SATISFACTORY + THRESHOLD_OFFSET;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].snrLowThreshold           =  SNR_THRESHOLD_POOR - THRESHOLD_OFFSET;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].monitorInterval           =  6;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].monitorWindow             =  0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].dot11RetryRatio           = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].dot11MultipleRetryRatio   = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].dot11AckFailureRatio      = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].dot11FcsErrorRatio        = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].dot11RtsFailureRatio      = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand2].beaconLossThreshold       = 0;

    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].rssiHighThreshold         =  RSSI_THRESHOLD_POOR + THRESHOLD_OFFSET;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].rssiLowThreshold          =  RSSI_LIMIT_WORST;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].snrHighThreshold          =  SNR_THRESHOLD_POOR + THRESHOLD_OFFSET;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].snrLowThreshold           =  SNR_LIMIT_WORST;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].monitorInterval           =  6;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].monitorWindow             =  0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].dot11RetryRatio           = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].dot11MultipleRetryRatio   = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].dot11AckFailureRatio      = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].dot11FcsErrorRatio        = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].dot11RtsFailureRatio      = 0;
    fsmData->config.roamingConfig.roamingBands[unifi_RoamingBand3].beaconLossThreshold       = 0;

    fsmData->config.roamingConfig.apBlockTimeMs       = 0;
    fsmData->config.roamingConfig.roamMonitorPeriodMs = 0;
    fsmData->config.roamingConfig.roamNumMaxTh        = 0;
    fsmData->config.roamingConfig.lowQualHystWindow   = 0;
    fsmData->config.roamingConfig.disableRoamScans    = FALSE;
    fsmData->config.roamingConfig.reconnectLimit      = 3;

    fsmData->config.regDomInfo.dot11MultiDomainCapabilityImplemented  = FALSE;
    fsmData->config.regDomInfo.dot11MultiDomainCapabilityEnabled      = FALSE;
    fsmData->config.regDomInfo.currentCountryCode[0]                  = '0';
    fsmData->config.regDomInfo.currentCountryCode[1]                  = '0';
    fsmData->config.regDomInfo.currentRegulatoryDomain                = unifi_RegulatoryDomainNone;

    fsmData->config.unifiTxPowerAdjustment_tpo        = 0;
    fsmData->config.unifiTxPowerAdjustment_eirp       = 0;
    fsmData->config.linkQuality.unifiRssi = -200;
    fsmData->config.linkQuality.unifiSnr = 0;

    fsmData->config.stats.unifiRxDataRate = 0;

#ifdef CCX_VARIANT
    fsmData->config.ccxConfig.keepAliveTimeMs = H85_IE_REFRESH_RATE_DEFAULT;
    fsmData->config.ccxConfig.apRoamingEnabled = TRUE;
    fsmData->config.ccxConfig.ccxRadioMgtEnabled = TRUE;
#endif

    fsmData->config.highThroughputOptionEnabled = FALSE;

    csr_list_init(&fsmData->config.roamingChannelLists);

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   SME configuration FSM Reset Function
 *
 * @par Description
 *   Clean up any memory hanging around
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void sme_configuration_reset(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "sme_configuration_reset()"));
    if (fsmData->config.pmk_cache)
    {
        sec_shutdown_pmk_cache(fsmData->config.pmk_cache);
        fsmData->config.pmk_cache = NULL;
    }

    CsrPfree(fsmData->config.multicastAddresses.addresses);
    fsmData->config.multicastAddresses.addressesCount = 0;
    fsmData->config.multicastAddresses.addresses = NULL;

    CsrPfree(fsmData->config.connectionConfig.mlmeAssociateReqInformationElements);
    fsmData->config.connectionConfig.mlmeAssociateReqInformationElements = NULL;
    fsmData->config.connectionConfig.ssid.length = 0;

    CsrPfree(fsmData->config.tmpIndList);
    fsmData->config.tmpIndList = NULL;
}


/**
 * @brief
 *   SME configuration FSM Reset Function
 *
 * @par Description
 *   Clean up any memory hanging around
 *   This version is used when the SME is shutdown
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void sme_configuration_reset_on_shutdown(FsmContext* context)
{
    CsrUint8 i;
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "sme_configuration_reset_full()"));

    sme_configuration_reset(context);

    CsrPfree(fsmData->config.userBlackList.addresses);
    fsmData->config.userBlackList.addressesCount = 0;
    fsmData->config.userBlackList.addresses = NULL;

    CsrPfree(fsmData->config.indRegistrations);
    CsrPfree(fsmData->config.cloakedSsids.cloakedSsids);

    for(i = 0; i < fsmData->config.mibFiles.numElements; i++)
    {
        CsrPfree(fsmData->config.mibFiles.dataList[i].data);
    }
    CsrPfree(fsmData->config.mibFiles.dataList);
}

/**
 * @brief
 *   Stopped state Start transition
 *
 * @par Description
 *  Starts up the configuration fsm
 * and subsequently moving into the idle state.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Start request
 *
 * @return
 *   void
 */
static void stopped_start(FsmContext* context, const CoreStartReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "stopped_start()"));

    /* -------------------------------------------- */
    /* Read Only                                    */
    /* -------------------------------------------- */
    fsmData->config.connectionBeaconFrameDataRef.dataLength = 0;
    fsmData->config.connectionExchangedFramesDataRef.dataLength = 0;
    fsmData->config.assocReqIeDataRef.dataLength = 0;
    fsmData->config.lastStatsRead = (fsm_get_time_of_day_ms(context) - MIN_STATS_READ_INTERVAL_MS) - 1;
    fsmData->config.lastLinkQualityRead = fsmData->config.lastStatsRead;
    fsmData->config.pmk_cache = sec_initialise_pmk_cache();

    fsmData->config.WMMAssociation = FALSE;
    /* -------------------------------------------- */
    /* Read Write                                   */
    /* -------------------------------------------- */
    if (!req->resume)
    {
        CsrMemSet(&fsmData->config.wepkeys, 0x00, sizeof(fsmData->config.wepkeys) );
        fsmData->config.IndexSecurityModeTable[0]   = unifi_80211AuthOpen;
        fsmData->config.IndexSecurityModeTable[1]   = unifi_80211AuthOpen;
        fsmData->config.IndexSecurityModeTable[2]   = unifi_80211AuthOpen;
        fsmData->config.IndexSecurityModeTable[3]   = unifi_80211AuthOpen;
        fsmData->config.IndexSecurityModeTable[4]   = unifi_80211AuthOpen;
    }

    fsm_next_state(context, FSMSTATE_idle);
    send_core_start_cfm(context, req->common.sender_, unifi_Success);
}

/**
 * @brief
 *  Return to stopped state
 *
 * @par Description
 *   A CORE_STOP_REQ has been received.
 *
 * @param[in]    context            : FSM context
 * @param[in]    request            : content of the CoreStopReq
 *
 * @return
 *  Void
 */
static void idle__stop_req(FsmContext *context, const CoreStopReq_Evt *request)
{
    FsmData *fsmData = FSMDATA;
    PldContext* pldContext = getPldContext(context);
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "stopped_start()"));

    /* Clean up the previous Associations info */
    if (fsmData->config.connectionBeaconFrameDataRef.dataLength != 0)
    {
        pld_release(pldContext, fsmData->config.connectionBeaconFrameDataRef.slotNumber);
    }
    if (fsmData->config.connectionExchangedFramesDataRef.dataLength != 0)
    {
        pld_release(pldContext, fsmData->config.connectionExchangedFramesDataRef.slotNumber);
    }
    if (fsmData->config.assocReqIeDataRef.dataLength != 0)
    {
        pld_release(pldContext, fsmData->config.assocReqIeDataRef.slotNumber);
    }

    send_core_stop_cfm(context, request->common.sender_, unifi_Success);

    sme_configuration_reset(context);

    fsmData->hostKey = FALSE;
    fsmData->config.regdomReadFromMib = FALSE;
    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   Gets the config values available without asyncronous behaviour
 *
 * @par Description
 *   NOTE : The current instance for the FSM may not be set SO
 *          sending an event from this function will need some thought
 *
 * @param[in]    context    : FSM context
 * @param[in]    fsmData    : Config Data
 * @param[inout] appValue   : The result data
 *
 * @return
 *   void
 */
static unifi_Status set_value(FsmContext* context, FsmData* fsmData, const unifi_AppValue* appValue)
{
    unifi_Status status = unifi_Success;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "set_value(%s)", trace_unifi_AppValueId(appValue->id)));

    switch (appValue->id)
    {
    case unifi_CalibrationDataValue:

        if (fsmData->config.calibrationData.dataLength != 0)
        {
            pld_release(getPldContext(context), fsmData->config.calibrationData.slotNumber);
        }
        fsmData->config.calibrationData.dataLength = appValue->unifi_Value_union.calibrationData.length;
        if (fsmData->config.calibrationData.dataLength != 0)
        {
            pld_store(getPldContext(context),
                      appValue->unifi_Value_union.calibrationData.data,
                      appValue->unifi_Value_union.calibrationData.length,
                      &fsmData->config.calibrationData.slotNumber);
        }
        /* Free the Copied Data */
        CsrPfree(appValue->unifi_Value_union.calibrationData.data);

        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CalibrationDataValue"));
        break;
    case unifi_AdHocConfigValue:
        fsmData->config.adHocConfig = appValue->unifi_Value_union.adHocConfig;
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_AdHocConfigValue.atimWindowTu [%d]", fsmData->config.adHocConfig.atimWindowTu));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_AdHocConfigValue.beaconPeriodTu [%d]", fsmData->config.adHocConfig.beaconPeriodTu));
        break;

    case unifi_CoexConfigValue:
    {
        if (!fsmData->config.wifiOff)
        {
            /* Kick Coex FSM for the config update and pass the OLD Enabled value */
            send_coex_config_update_ind_external(context, getSmeContext(context)->coexInstance, fsmData->config.coexConfig.coexEnable);
        }
        fsmData->config.coexConfig = appValue->unifi_Value_union.coexConfig;

        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CoexConfigValue.coexEnable [%s]", fsmData->config.coexConfig.coexEnable?"TRUE":"FALSE"));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CoexConfigValue.coexAfhChannelEnable [%s]", fsmData->config.coexConfig.coexAfhChannelEnable?"TRUE":"FALSE"));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CoexConfigValue.coexAdvancedEnable [%s]", fsmData->config.coexConfig.coexAdvancedEnable?"TRUE":"FALSE"));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CoexConfigValue.coexAdvancedEnable [%s]", fsmData->config.coexConfig.coexAdvancedEnable?"TRUE":"FALSE"));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CoexConfigValue.coexDirection [%s]", trace_unifi_CoexDirection(fsmData->config.coexConfig.coexDirection)));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CoexConfigValue.coexPeriodicWakeHost [%s]", fsmData->config.coexConfig.coexPeriodicWakeHost?"TRUE":"FALSE"));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CoexConfigValue.coexTrafficBurstyLatencyMs [%d]", fsmData->config.coexConfig.coexTrafficBurstyLatencyMs));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CoexConfigValue.coexTrafficContinuousLatencyMs [%d]", fsmData->config.coexConfig.coexTrafficContinuousLatencyMs));
        break;
    }
    case unifi_HostConfigValue:
        if (!fsmData->config.wifiOff &&
             (fsmData->config.hostConfig.powerMode != appValue->unifi_Value_union.hostConfig.powerMode ||
              fsmData->config.hostConfig.applicationDataPeriodMs != appValue->unifi_Value_union.hostConfig.applicationDataPeriodMs) )
        {
            /* Kick Coex FSM for the config update */
            send_coex_power_update_ind_external(context, getSmeContext(context)->coexInstance);
        }
        fsmData->config.hostConfig = appValue->unifi_Value_union.hostConfig;
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_HostConfigValue.powerMode [%s]", trace_unifi_HostPowerMode(fsmData->config.hostConfig.powerMode)));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_HostConfigValue.applicationDataPeriodMs [%d]", fsmData->config.hostConfig.applicationDataPeriodMs));
        break;

    case unifi_PowerConfigValue:
        if (!fsmData->config.wifiOff &&
            fsmData->config.powerConfig.powerSaveLevel != appValue->unifi_Value_union.powerConfig.powerSaveLevel)
        {
            /* Kick Coex FSM for the config update */
            send_coex_power_update_ind_external(context, getSmeContext(context)->coexInstance);
        }
        fsmData->config.powerConfig = appValue->unifi_Value_union.powerConfig;
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_PowerConfigValue.powerSaveLevel [%s]", trace_unifi_PowerSaveLevel(fsmData->config.powerConfig.powerSaveLevel)));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_PowerConfigValue.listenIntervalBeacons [%d]", fsmData->config.powerConfig.listenIntervalBeacons));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_PowerConfigValue.rxDtims [%s]", fsmData->config.powerConfig.rxDtims?"TRUE":"FALSE"));
        break;

    case unifi_SmeConfigValue:

        /* Kick the regdomain if wifi is on and the country code changes */
        if (!fsmData->config.wifiOff &&
            fsmData->config.smeConfig.countryCode[0] != appValue->unifi_Value_union.smeConfig.countryCode[0] &&
            fsmData->config.smeConfig.countryCode[1] != appValue->unifi_Value_union.smeConfig.countryCode[1])
        {
            send_sm_adjunct_tech_signal_ind_external(context, getSmeContext(context)->scanManagerInstance);
        }

        if(appValue->unifi_Value_union.smeConfig.ifIndex & unifi_GHZ_5_0 &&
           appValue->unifi_Value_union.smeConfig.trustLevel != unifi_TrustDisabled)
        {
            sme_trace_warn((TR_SME_CONFIGURATION_FSM, "Enabling of 5GHz interface requires regulatory domain subsystem to be disabled - disabling"));
            fsmData->config.smeConfig = appValue->unifi_Value_union.smeConfig;
            fsmData->config.smeConfig.trustLevel = unifi_TrustDisabled;
            if (!fsmData->config.wifiOff)
            {
                regdom_init(context, get_regulatory_data(context));
            }
        }
        else if (!fsmData->config.wifiOff && fsmData->config.smeConfig.trustLevel != appValue->unifi_Value_union.smeConfig.trustLevel)
        {
            sme_trace_warn((TR_SME_CONFIGURATION_FSM,
                            "set_value: 802.11d trust level modified (%d -> %d) - restarting regulatory subsystem",
                            fsmData->config.smeConfig.trustLevel, appValue->unifi_Value_union.smeConfig.trustLevel));

            fsmData->config.smeConfig = appValue->unifi_Value_union.smeConfig;
            regdom_init(context, get_regulatory_data(context));
        }
        else
        {
            fsmData->config.smeConfig = appValue->unifi_Value_union.smeConfig;
        }

        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_SMEConfigValue.connectionQualityRssiChangeTrigger [%d]", fsmData->config.smeConfig.connectionQualityRssiChangeTrigger));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_SMEConfigValue.connectionQualitySnrChangeTrigger  [%d]", fsmData->config.smeConfig.connectionQualitySnrChangeTrigger));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_SMEConfigValue.trustLevel                         [%s]", trace_unifi_80211dTrustLevel(fsmData->config.smeConfig.trustLevel)));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_SMEConfigValue.countryCode                        [%c%c]", fsmData->config.smeConfig.countryCode[0], fsmData->config.smeConfig.countryCode[1]));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_SMEConfigValue.wmmMode                            [0x%.4X]", fsmData->config.smeConfig.wmmModeMask));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_SMEConfigValue.ifIndex                            [%s]", trace_unifi_RadioIF(fsmData->config.smeConfig.ifIndex)));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_SMEConfigValue.firmwareDriverInterface            [%s]", trace_unifi_FirmwareDriverInterface(fsmData->config.smeConfig.firmwareDriverInterface)));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_SMEConfigValue.enableStrictDraftN                 [%d]", fsmData->config.smeConfig.enableStrictDraftN ));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_SMEConfigValue.enableOpportunisticKeyCaching      [%d]", fsmData->config.smeConfig.enableOpportunisticKeyCaching ));
        break;

    case unifi_ScanConfigValue:
    {
        sme_trace_info_code (CsrUint8 i;)

        if (fsmData->config.scanConfig.passiveChannelListCount)
        {
            CsrPfree(fsmData->config.scanConfig.passiveChannelList);
        }

        fsmData->config.scanConfig = appValue->unifi_Value_union.scanConfig;
        if (!fsmData->config.wifiOff)
        {
            /* Kick Scan FSM for the config update */
            send_sm_scan_update_ind_external(context, getSmeContext(context)->scanManagerInstance, unifi_ScanStopStart);
        }
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue.maxResults [%d]", fsmData->config.scanConfig.maxResults));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue.highrSSIThreshold [%d]", fsmData->config.scanConfig.highRSSIThreshold));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue.lowRSSIThreshold [%d]", fsmData->config.scanConfig.lowRSSIThreshold));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue.deltaRSSIThreshold [%d]", fsmData->config.scanConfig.deltaRSSIThreshold));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue.highSNRThreshold [%d]", fsmData->config.scanConfig.highSNRThreshold));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue.lowSNRThreshold [%d]", fsmData->config.scanConfig.lowSNRThreshold));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue.deltaSNRThreshold [%d]", fsmData->config.scanConfig.deltaSNRThreshold));

        sme_trace_info_code (
        for (i=0; i<4 ;i++)
        {
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue[%s].intervalSeconds=%d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.scanConfig.scanCfg[i].intervalSeconds));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue[%s].validitySeconds=%d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.scanConfig.scanCfg[i].validitySeconds));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue[%s].MinActiveChannelTimeTu= %d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.scanConfig.scanCfg[i].minActiveChannelTimeTu));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue[%s].MaxActiveChannelTimeTu= %d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.scanConfig.scanCfg[i].maxActiveChannelTimeTu));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue[%s].MinPassiveChannelTimeTu= %d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.scanConfig.scanCfg[i].minPassiveChannelTimeTu));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_ScanConfigValue[%s].MaxPassiveChannelTimeTu= %d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.scanConfig.scanCfg[i].maxPassiveChannelTimeTu));
        }
        )
        break;
    }
    case unifi_CloakedSsidConfigValue:
    {
        sme_trace_info_code (CsrUint8 i;)
        if (fsmData->config.cloakedSsids.cloakedSsids)
        {
            CsrPfree(fsmData->config.cloakedSsids.cloakedSsids);
        }
        fsmData->config.cloakedSsids = appValue->unifi_Value_union.cloakedSsids;
        if (!fsmData->config.wifiOff)
        {
            /* Kick Scan FSM for the config update */
            send_sm_scan_update_ind_external(context, getSmeContext(context)->scanManagerInstance, unifi_ScanCloakedSsidRescan);
        }
        sme_trace_info_code (
        for (i=0; i<appValue->unifi_Value_union.cloakedSsids.cloakedSsidsCount ;i++)
        {
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_CloakedSsidConfig[%d] %s",
                            i, trace_unifi_SSID(&appValue->unifi_Value_union.cloakedSsids.cloakedSsids[i], getSSIDBuffer(context))));
        }
        )
        break;
    }
    case unifi_MibConfigValue:
        if (fsmData->config.wifiOff)
        {
            sme_trace_error((TR_SME_CONFIGURATION_FSM, "set_value: Cannot set value unifi_MIBConfigValue when wifi is off"));
            status = unifi_Unavailable;
            break;
        }

        fsmData->config.mibConfig = appValue->unifi_Value_union.mibConfig;
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.unifiFixMaxTxDataRate [%s]", fsmData->config.mibConfig.unifiFixMaxTxDataRate?"TRUE":"FALSE"));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.unifiFixTxDataRate [%d]", fsmData->config.mibConfig.unifiFixTxDataRate));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.dot11RtsThreshold [%d]", fsmData->config.mibConfig.dot11RtsThreshold));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.dot11FragmentationThreshold [%d]", fsmData->config.mibConfig.dot11FragmentationThreshold));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.dot11CurrentTxPowerLevel [%d]", fsmData->config.mibConfig.dot11CurrentTxPowerLevel));

        /* Send an event to self to action the write. This will be handled in idle__set_value_req*/
        send_unifi_mgt_set_value_req_external(context, getSmeContext(context)->smeConfigurationInstance, NULL, *appValue);
        break;
    case unifi_RoamingConfigValue:
    {
        sme_trace_info_code (CsrUint8 i;)

        fsmData->config.roamingConfig = appValue->unifi_Value_union.roamingConfig;

        if (fsmData->config.roamingConfig.disableRoamScans == TRUE)
        {
            roaming_channel_list_flush(context);
        }

        if (!fsmData->config.wifiOff)
        {
            /* Kick Scan FSM for the config update */
            send_sm_scan_update_ind_external(context, getSmeContext(context)->scanManagerInstance, unifi_ScanStopStart);
        }

        sme_trace_debug_code (
        for (i=0; i<3; i++)
        {
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].rssiHighThreshold         [%d]", i, fsmData->config.roamingConfig.roamingBands[i].rssiHighThreshold));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].rssiPoorThreshold         [%d]", i, fsmData->config.roamingConfig.roamingBands[i].rssiLowThreshold));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].snrHighThreshold          [%d]", i, fsmData->config.roamingConfig.roamingBands[i].snrHighThreshold));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].snrLowThreshold           [%d]", i, fsmData->config.roamingConfig.roamingBands[i].snrLowThreshold));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].monitorInterval           [%d]", i, fsmData->config.roamingConfig.roamingBands[i].monitorInterval));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].monitorWindow             [%d]", i, fsmData->config.roamingConfig.roamingBands[i].monitorWindow));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].dot11RetryRatio         [%d]", i, fsmData->config.roamingConfig.roamingBands[i].dot11RetryRatio));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].dot11MultipleRetryRatio [%d]", i, fsmData->config.roamingConfig.roamingBands[i].dot11MultipleRetryRatio));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].dot11AckFailureRatio    [%d]", i, fsmData->config.roamingConfig.roamingBands[i].dot11AckFailureRatio));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].dot11FcsErrorRatio      [%d]", i, fsmData->config.roamingConfig.roamingBands[i].dot11FcsErrorRatio));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamingBands[%d].dot11RtsFailureRatio    [%d]", i, fsmData->config.roamingConfig.roamingBands[i].dot11RtsFailureRatio));
        }

        for (i=0; i<3 ;i++)
        {
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue[%s].intervalSeconds=%d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.roamingConfig.roamScanCfg[i].intervalSeconds));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue[%s].validitySeconds=%d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.roamingConfig.roamScanCfg[i].validitySeconds));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue[%s].MinActiveChannelTimeTu= %d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.roamingConfig.roamScanCfg[i].minActiveChannelTimeTu));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue[%s].MaxActiveChannelTimeTu= %d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.roamingConfig.roamScanCfg[i].maxActiveChannelTimeTu));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue[%s].MinPassiveChannelTimeTu= %d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.roamingConfig.roamScanCfg[i].minPassiveChannelTimeTu));

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue[%s].MaxPassiveChannelTimeTu= %d",
                            trace_unifi_BasicUsability(i),
                            fsmData->config.roamingConfig.roamScanCfg[i].maxPassiveChannelTimeTu));
        }
        )

        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.apBlockTimeMs        [%d]", fsmData->config.roamingConfig.apBlockTimeMs));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamMonitorPeriodMs  [%d]", fsmData->config.roamingConfig.roamMonitorPeriodMs));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.roamNumMaxTh         [%d]", fsmData->config.roamingConfig.roamNumMaxTh));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.lowQualHystWindow    [%d]", fsmData->config.roamingConfig.lowQualHystWindow));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_RoamingConfigValue.disableRoamScans     [%s]", fsmData->config.roamingConfig.disableRoamScans?"TRUE":"FALSE"));
        break;
    }

#ifdef CCX_VARIANT
    case unifi_CcxConfigValue:
    {
        fsmData->config.ccxConfig = appValue->unifi_Value_union.ccxConfig;
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: ccxConfig.keepAliveTimeMs  [%d]", fsmData->config.ccxConfig.keepAliveTimeMs));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: ccxConfig.apRoamingEnabled [%s]", fsmData->config.ccxConfig.apRoamingEnabled?"TRUE":"FALSE"));
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: ccxConfig.measurementsMask [%d]", fsmData->config.ccxConfig.measurementsMask));
        break;
    }
#endif

    case unifi_ConnectionStatsValue:
    case unifi_StationMACAddressValue:
    case unifi_PermanentMACAddressValue:
    case unifi_ConnectionConfigValue:
    case unifi_ConnectionInfoValue:
    case unifi_CoexInfoValue:
    case unifi_VersionsValue:
    case unifi_RegulatoryDomainInfoValue:
    case unifi_LinkQualityValue:
        sme_trace_error((TR_SME_CONFIGURATION_FSM, "set_value: Cannot set read only value %s", trace_unifi_AppValueId(appValue->id)));
        if (fsmData->config.wifiOff)
        {
            status = unifi_WifiOff;
        }
        else
        {
            status = unifi_Unavailable;
        }
        break;
    default:
        sme_trace_error((TR_SME_CONFIGURATION_FSM, "set_value() Cannot set unhandled %s", trace_unifi_AppValueId(appValue->id)));
        status = unifi_NotFound;
        break;
    }

    return status;
}


/**
 * @brief
 *  Stooped state set value
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void stopped__set_value_req(FsmContext* context, const UnifiMgtSetValueReq_Evt* req)
{
    FsmData *fsmData = FSMDATA;
    unifi_Status status;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "stopped__set_value_req(%s)", trace_unifi_AppValueId(req->appValue.id)));

    status = set_value(context, fsmData, &req->appValue);

    call_unifi_mgt_set_value_cfm(context, req->appHandle, status, req->appValue.id);
}

/**
 * @brief
 *   set a sme config value
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void idle__set_value_req(FsmContext* context, const UnifiMgtSetValueReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    unifi_Status status;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "idle__set_value_req(%s)", trace_unifi_AppValueId(req->appValue.id)));

    fsmData->getSetRequester = req->common.sender_;
    fsmData->getSetType = req->appValue.id;

    if (isAccessRestricted(context, req->appHandle))
    {
        status =  unifi_Restricted;
    }
    else
    {
        switch (req->appValue.id)
        {
        case unifi_MibConfigValue:
        {
            unifi_RadioIF ifIndex;
            DataReference mibSetDataRef = mib_encode_create_set(context, 8, 0);
            fsmData->config.mibConfig = req->appValue.unifi_Value_union.mibConfig;
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.unifiFixMaxTxDataRate [%s]", fsmData->config.mibConfig.unifiFixMaxTxDataRate?"TRUE":"FALSE"));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.unifiFixTxDataRate [%d]", fsmData->config.mibConfig.unifiFixTxDataRate));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.dot11RtsThreshold [%d]", fsmData->config.mibConfig.dot11RtsThreshold));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.dot11FragmentationThreshold [%d]", fsmData->config.mibConfig.dot11FragmentationThreshold));
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "set_value: unifi_MIBConfigValue.dot11CurrentTxPowerLevel [%d]", fsmData->config.mibConfig.dot11CurrentTxPowerLevel));

            (void)mib_encode_add_set_boolean(context, &mibSetDataRef, unifiFixMaxTxDataRate, fsmData->config.mibConfig.unifiFixMaxTxDataRate, 0, 0);
            (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiFixTxDataRate, (CsrInt32)fsmData->config.mibConfig.unifiFixTxDataRate, 0, 0);

            for(ifIndex = unifi_GHZ_2_4; ifIndex <= unifi_GHZ_5_0; ifIndex++)
            {
                if (ifIndex & fsmData->config.smeConfig.ifIndex)
                {
                    (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11RTSThreshold, (CsrInt32)fsmData->config.mibConfig.dot11RtsThreshold, (CsrUint8)ifIndex, 0);
                    (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11FragmentationThreshold, (CsrInt32)fsmData->config.mibConfig.dot11FragmentationThreshold, (CsrUint8)ifIndex, 0);
                    (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11CurrentTxPowerLevel, (CsrInt32)fsmData->config.mibConfig.dot11CurrentTxPowerLevel, (CsrUint8)ifIndex, 0);
                }
            }
            fsmData->cfmAppHandle = req->appHandle;
            send_mlme_set_req_internal(context, getSmeContext(context)->mibAccessInstance, mibSetDataRef);

            fsm_next_state(context, FSMSTATE_waiting_for_cfm);
            return;
        }
        case unifi_SmeConfigValue:
            /* Check Access Restriction change */
            if (fsmData->config.smeConfig.enableRestrictedAccess != req->appValue.unifi_Value_union.smeConfig.enableRestrictedAccess)
            {
                if (fsmData->config.smeConfig.enableRestrictedAccess)
                {
                    if (fsmData->config.restrictedAccessActivated && fsmData->config.restrictedAccessAppHandle != req->appHandle)
                    {
                        sme_trace_warn((TR_SME_CONFIGURATION_FSM, "idle__set_value_req: unifi_SMEConfigValue.enableRestrictedAccess[%s] Restricted", req->appValue.unifi_Value_union.smeConfig.enableRestrictedAccess?"TRUE":"FASLE"));
                        status = unifi_Restricted;
                        break;
                    }
                }
            }
            status = set_value(context, fsmData, &req->appValue);
            break;
        default:
            status = set_value(context, fsmData, &req->appValue);
        };
    }

    if (req->appValue.id == unifi_VersionsValue && req->appValue.unifi_Value_union.versions.smeIdString)
    {
        CsrPfree(req->appValue.unifi_Value_union.versions.smeIdString);
    }
    else if (req->appValue.id == unifi_ConnectionConfigValue)
    {
        CsrPfree(req->appValue.unifi_Value_union.connectionConfig.mlmeAssociateReqInformationElements);
    }
    else if (req->appValue.id == unifi_ConnectionInfoValue)
    {
        CsrPfree(req->appValue.unifi_Value_union.connectionInfo.assocScanInfoElements);
        CsrPfree(req->appValue.unifi_Value_union.connectionInfo.assocReqInfoElements);
        CsrPfree(req->appValue.unifi_Value_union.connectionInfo.assocRspInfoElements);
    }

    call_unifi_mgt_set_value_cfm(context, req->appHandle, status, req->appValue.id);
}

/**
 * @brief
 *   Gets the config values available without asyncronous behaviour
 *
 *   NOTE : The current instance for the FSM may not be set SO
 *          sending an event from this function will need some thought
 *
 * @param[in]    context    : FSM context
 * @param[in]    fsmData    : Config Data
 * @param[in]    type       : The value to get
 * @param[inout] appValue   : The result data
 *
 * @return
 *   void
 */
static unifi_Status get_value(FsmContext* context, FsmData* fsmData, unifi_AppValue* appValue)
{
    unifi_Status status = unifi_Success;

    CsrMemSet(&appValue->unifi_Value_union, 0x00, sizeof(appValue->unifi_Value_union));

    sme_trace_info((TR_SME_CONFIGURATION_FSM, "get_value(%s)", trace_unifi_AppValueId(appValue->id)));

    /* Not Available When The WIFI is OFF */
    if (fsmData->config.wifiOff)
    {
        switch (appValue->id)
        {
        case unifi_StationMACAddressValue:
        case unifi_MibConfigValue:
        case unifi_PermanentMACAddressValue:
        case unifi_ConnectionConfigValue:
        case unifi_ConnectionInfoValue:
        case unifi_ConnectionStatsValue:
        case unifi_CoexInfoValue:
        case unifi_VersionsValue:
        case unifi_RegulatoryDomainInfoValue:
        case unifi_LinkQualityValue:
            sme_trace_error((TR_SME_CONFIGURATION_FSM, "get_value() Cannot read %s when the wifi is off", trace_unifi_AppValueId(appValue->id)));
            if (appValue->id == unifi_VersionsValue)
            {
                appValue->unifi_Value_union.versions.smeIdString = ""; /*lint !e1776 */
            }
            return unifi_WifiOff;
        default:
            break;
        }
    }

    /* Not Available When Disconnected */
    if (ns_get_connection_status(context) == ns_ConnectionStatus_Disconnected)
    {
        switch (appValue->id)
        {
        case unifi_ConnectionConfigValue:
        case unifi_ConnectionStatsValue:
        case unifi_ConnectionInfoValue:
        case unifi_LinkQualityValue:
            sme_trace_warn((TR_SME_CONFIGURATION_FSM, "get_value() Cannot read %s when the wifi is disconnected", trace_unifi_AppValueId(appValue->id)));
            return unifi_Unavailable;
        default:
            break;
        }
    }

    switch (appValue->id)
    {
    case unifi_CalibrationDataValue:
    {
        appValue->unifi_Value_union.calibrationData.length = fsmData->config.calibrationData.dataLength;
        if (fsmData->config.calibrationData.dataLength != 0)
        {
            CsrUint16 bufLen;
            pld_access(getPldContext(context), fsmData->config.calibrationData.slotNumber,
                       (void**)&appValue->unifi_Value_union.calibrationData.data, &bufLen);
        }
        break;
    }
    case unifi_AdHocConfigValue:
        appValue->unifi_Value_union.adHocConfig = fsmData->config.adHocConfig;
        break;

    case unifi_CoexConfigValue:
        appValue->unifi_Value_union.coexConfig = fsmData->config.coexConfig;
        break;

    case unifi_HostConfigValue:
        appValue->unifi_Value_union.hostConfig = fsmData->config.hostConfig;
        break;

    case unifi_PowerConfigValue:
        appValue->unifi_Value_union.powerConfig = fsmData->config.powerConfig;
        break;

    case unifi_SmeConfigValue:
        appValue->unifi_Value_union.smeConfig = fsmData->config.smeConfig;
        break;

    case unifi_ScanConfigValue:
        appValue->unifi_Value_union.scanConfig = fsmData->config.scanConfig;
        break;
    case unifi_CloakedSsidConfigValue:
        appValue->unifi_Value_union.cloakedSsids = fsmData->config.cloakedSsids;
        break;
    case unifi_MibConfigValue:
        appValue->unifi_Value_union.mibConfig = fsmData->config.mibConfig;
        break;

    case unifi_ConnectionStatsValue:
        appValue->unifi_Value_union.connectionStats = fsmData->config.stats;
        break;
    case unifi_LinkQualityValue:
        appValue->unifi_Value_union.linkQuality = fsmData->config.linkQuality;
        break;
    case unifi_StationMACAddressValue:
        appValue->unifi_Value_union.stationMacAddress = fsmData->config.stationMacAddress;
        break;
    case unifi_PermanentMACAddressValue:
        appValue->unifi_Value_union.permanentMacAddress = fsmData->config.permanentMacAddress;
        break;
    case unifi_ConnectionConfigValue:
        appValue->unifi_Value_union.connectionConfig = fsmData->config.connectionConfig;
        break;
    case unifi_ConnectionInfoValue:
        appValue->unifi_Value_union.connectionInfo = fsmData->config.connectionInfo;
        break;
    case unifi_CoexInfoValue:
        appValue->unifi_Value_union.coexInfo = fsmData->config.coexInfo;
        break;
    case unifi_VersionsValue:
        appValue->unifi_Value_union.versions = fsmData->config.versions;
        break;
    case unifi_RoamingConfigValue:
        appValue->unifi_Value_union.roamingConfig = fsmData->config.roamingConfig;
        break;
    case unifi_RegulatoryDomainInfoValue:
    {
        RegDomData* regDomData = get_regulatory_data(context);
        if (fsmData->config.regDomInfo.dot11MultiDomainCapabilityImplemented &&
            fsmData->config.regDomInfo.dot11MultiDomainCapabilityEnabled) {
            if (regDomData->CurrentLocation.locStatus == Location_Valid)
            {
                fsmData->config.regDomInfo.currentCountryCode[0]   = (CsrUint8)regDomData->CurrentLocation.countryString[0];
                fsmData->config.regDomInfo.currentCountryCode[1]   = (CsrUint8)regDomData->CurrentLocation.countryString[1];
                fsmData->config.regDomInfo.currentRegulatoryDomain = regDomData->CurrentLocation.regDomain;
            }
            else
            {
                fsmData->config.regDomInfo.currentCountryCode[0]   = (CsrUint8)regDomData->DefaultLocation.countryString[0];
                fsmData->config.regDomInfo.currentCountryCode[1]   = (CsrUint8)regDomData->DefaultLocation.countryString[1];
                fsmData->config.regDomInfo.currentRegulatoryDomain = regDomData->DefaultLocation.regDomain;
            }
        }
        else
        {
            fsmData->config.regDomInfo.currentCountryCode[0]   = '0';
            fsmData->config.regDomInfo.currentCountryCode[1]   = '0';
            fsmData->config.regDomInfo.currentRegulatoryDomain = unifi_RegulatoryDomainNone;
        }
        appValue->unifi_Value_union.regDomInfo = fsmData->config.regDomInfo;
        break;
    }
#ifdef CCX_VARIANT
    case unifi_CcxConfigValue:
    {
        appValue->unifi_Value_union.ccxConfig = fsmData->config.ccxConfig;
        break;
    }
#endif
    default:
        sme_trace_error((TR_SME_CONFIGURATION_FSM, "get_value() Cannot read unhandled %s", trace_unifi_AppValueId(appValue->id)));
        status = unifi_NotFound;
        break;
    }

    return status;
}

/**
 * @brief
 *   Fail a get value request
 *
 * @par Description
 *   Invalid Get value request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void stopped__get_value_req(FsmContext* context, const UnifiMgtGetValueReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    unifi_AppValue appValue;
    unifi_Status status = unifi_Success;
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "bounce__get_value_req()"));

    appValue.id = req->appValueId;
    status = get_value(context, fsmData, &appValue);

    call_unifi_mgt_get_value_cfm(context, req->appHandle, status, &appValue);
}

/**
 * @brief
 *   Get a sme config value
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void idle__get_value_req(FsmContext* context, const UnifiMgtGetValueReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    unifi_Status status = unifi_Success;
    unifi_AppValue appValue;
    appValue.id = req->appValueId;

    fsmData->getSetRequester = req->common.sender_;
    fsmData->getSetType = appValue.id;

    switch (appValue.id)
    {
    case unifi_ConnectionStatsValue:
    {
        if (ns_get_media_connection_status(context) == unifi_MediaConnected)
        {
            if (!(CsrTimeLe(fsm_get_time_of_day_ms(context), CsrTimeAdd(fsmData->config.lastStatsRead, MIN_STATS_READ_INTERVAL_MS))))
            {
                sme_trace_debug((TR_SME_CONFIGURATION_FSM, "stats values timed out reading from mib"));
                fsmData->cfmAppHandle = req->appHandle;
                mib_util_send_stats_req(context, fsmData->config.connectionInfo.ifIndex);
                fsm_next_state(context, FSMSTATE_waiting_for_cfm);
                return;
            }
        }
        status = get_value(context, fsmData, &appValue);
        break;
    }

    case unifi_LinkQualityValue:
    {
        if (ns_get_media_connection_status(context) == unifi_MediaConnected)
        {
            if (!(CsrTimeLe(fsm_get_time_of_day_ms(context), CsrTimeAdd(fsmData->config.lastLinkQualityRead, MIN_STATS_READ_INTERVAL_MS))))
            {
                sme_trace_debug((TR_SME_CONFIGURATION_FSM, "linkq values timed out reading from mib"));
                fsmData->cfmAppHandle = req->appHandle;
                mib_util_send_linkq_req(context, fsmData->config.connectionInfo.ifIndex);
                fsm_next_state(context, FSMSTATE_waiting_for_cfm);
                return;
            }
        }
        status = get_value(context, fsmData, &appValue);
        break;
    }

    default:
    {
        status = get_value(context, fsmData, &appValue);
        break;
    }
    }

    call_unifi_mgt_get_value_cfm(context, req->appHandle, status, &appValue);
}

/**
 * @brief
 *   Mib get Cfm
 *
 * @par Description
 *   Mib get response
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void mlme_get_cfm(FsmContext* context, const MlmeGetCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    unifi_Status status = unifi_Success;
    unifi_AppValue appValue;

    appValue.id = fsmData->getSetType;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "mlme_get_cfm(%s)", trace_MibStatus(cfm->status)));

    switch(fsmData->getSetType)
    {
    case unifi_ConnectionStatsValue:
        if (mib_util_decode_stats(context, &cfm->mibAttributeValue, cfm->status, cfm->errorIndex, &fsmData->config.stats))
        {
            fsmData->config.lastStatsRead = fsm_get_time_of_day_ms(context);
            appValue.unifi_Value_union.connectionStats = fsmData->config.stats;

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "get_value: unifi_StatsValue"));
        }
        else
        {
            /* in this case, null the structure so it is initialized memory */
            CsrMemSet(&appValue, 0, sizeof(unifi_AppValue));
            appValue.id = fsmData->getSetType;

            status = unifi_Error;
        }
        break;

    case unifi_LinkQualityValue:
        if (mib_util_decode_linkq(context, &cfm->mibAttributeValue, cfm->status, cfm->errorIndex, &fsmData->config.linkQuality))
        {
            fsmData->config.lastLinkQualityRead = fsm_get_time_of_day_ms(context);
            appValue.unifi_Value_union.linkQuality = fsmData->config.linkQuality;

            sme_trace_info((TR_SME_CONFIGURATION_FSM, "get_value: unifi_LinkQualityValue [%d,%d]",
                                                      appValue.unifi_Value_union.linkQuality.unifiRssi,
                                                      appValue.unifi_Value_union.linkQuality.unifiSnr));
        }
        else
        {
            /* in this case, null the structure so it is initialized memory */
            CsrMemSet(&appValue, 0, sizeof(unifi_AppValue));
            appValue.id = fsmData->getSetType;

            status = unifi_Error;
        }
        break;

    default:
        status = unifi_NotFound;
        break;
    }

    /* free the payload only if one exists.
     * if a mibgetcfm error occurs then a payload will notbe present
     */
    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }

    call_unifi_mgt_get_value_cfm(context, fsmData->cfmAppHandle, status, &appValue);
    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   Mib set Cfm
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void mlme_set_cfm(FsmContext* context, const MlmeSetCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    unifi_Status status = cfm->status == MibStatus_Successful?unifi_Success:unifi_Error;

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);

    }

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "mlme_set_cfm(%s)", trace_MibStatus(cfm->status)));

    if (fsmData->hostMulticast)
    {
        if (fsmData->multicastAddressesId == UNIFI_MGT_MULTICAST_ADDRESS_REQ_ID && fsmData->getSetRequester == FSM_ENV)
        {
            call_unifi_mgt_multicast_address_cfm(context, fsmData->cfmAppHandle, status, fsmData->action, 0, NULL);
        }
        else if (fsmData->multicastAddressesId == UNIFI_SYS_MULTICAST_ADDRESS_IND_ID && fsmData->getSetRequester == FSM_ENV)
        {
            call_unifi_sys_multicast_address_rsp(context, status, fsmData->action, 0, NULL);
        }
        fsmData->hostMulticast = FALSE;
    }
    else if (fsmData->hostKey)
    {
        call_unifi_mgt_key_cfm(context, fsmData->cfmAppHandle, status, fsmData->action);
        fsmData->hostKey = FALSE;
    }
    else if (fsmData->getSetRequester == FSM_ENV)
    {
        call_unifi_mgt_set_value_cfm(context, fsmData->cfmAppHandle, status, fsmData->getSetType);
    }
    /* Else it will be a Sync set */
    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   Mib set addresses Cfm
 *
 * @par Description
 *   Mib set addresses response
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void mlmeDeleteKeys_cfm(FsmContext* context, const MlmeDeletekeysCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "mlmeDeleteKeys_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    call_unifi_mgt_key_cfm(context, fsmData->cfmAppHandle, (cfm->resultCode == ResultCode_Success?unifi_Success:unifi_Error), fsmData->action);
    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   Reject a pmkid request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void stopped__pmkid_req(FsmContext* context, const UnifiMgtPmkidReq_Evt* req)
{
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "stopped__pmkid_req(%s)", trace_unifi_ListAction(req->action)));

    /* Free the pmkid Data copied into the sme */
    CsrPfree(req->setPmkids);

    call_unifi_mgt_pmkid_cfm(context, req->appHandle, unifi_Unavailable, req->action, 0, NULL);
}

static unifi_Status update_pmkids(FsmContext* context, SmeConfigData* cfg, unifi_ListAction action, unifi_PmkidList* pmkids)
{
    unifi_Status status = unifi_Success;
    int i;
    if (action != unifi_ListActionFlush && pmkids->numElements != 0 && pmkids->pmkids == NULL)
    {
        sme_trace_error((TR_SME_CONFIGURATION_FSM, "update_pmkids() unifi_PmkidList.pmkids MUST NOT BE NULL"));
        return unifi_InvalidParameter;
    }

    switch(action)
    {
    case unifi_ListActionFlush:
        sec_clear_pmk_cache(cfg->pmk_cache);
        break;
    case unifi_ListActionAdd:
    {
        static const unifi_SSID emptyssid = {{0}, 0}; /*lint !e943 */
        unifi_SSID pmkidssid = emptyssid;
        for (i = 0; i < pmkids->numElements; ++i)
        {
            srs_scan_data* scandata = srs_get_scan_parameters_first(context, NULL, &pmkids->pmkids[i].bssid);
            if (scandata)
            {
                pmkidssid = scandata->scanResult.ssid;
            }
            sme_trace_debug((TR_SME_CONFIGURATION_FSM, "PMKID ADD %s :: %s", trace_unifi_SSID(&pmkidssid, getSSIDBuffer(context)),
                                                                             trace_unifi_MACAddress(pmkids->pmkids[i].bssid, getMacAddressBuffer(context))));
            sec_set_pmk_cached_data(cfg->pmk_cache, &pmkidssid, &pmkids->pmkids[i]);
        }
        break;
    }
    case unifi_ListActionRemove:
        for (i = 0; i < pmkids->numElements; ++i)
        {
            sec_delete_pmk_cached_data(cfg->pmk_cache, &pmkids->pmkids[i].bssid);
        }
        break;
    case unifi_ListActionGet:
    {
        csr_list* pmkidList = sec_get_pmk_list_handle(cfg->pmk_cache);
        csr_list_pmk* node = csr_list_gethead_t(csr_list_pmk*, pmkidList);
        pmkids->numElements = 0;
        pmkids->pmkids = CsrPmalloc(sizeof(unifi_Pmkid) * sec_get_pmk_list_count(cfg->pmk_cache));
        while(node != NULL)
        {
            pmkids->pmkids[pmkids->numElements] = node->pmkid;
            pmkids->numElements++;
            node = csr_list_getnext_t(csr_list_pmk*, pmkidList, &node->node);
        }
        break;
    }
    default:
        break;
    }
    return status;
}


/**
 * @brief
 *   Action a pmkid request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void pmkid_req(FsmContext* context, const UnifiMgtPmkidReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    unifi_Status status;
    unifi_PmkidList pmkids;
    pmkids.numElements = req->setPmkidsCount;
    pmkids.pmkids = req->setPmkids;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "pmkid_req(%s)", trace_unifi_ListAction(req->action)));

    if (isAccessRestricted(context, req->appHandle))
    {
        CsrPfree(req->setPmkids);
        call_unifi_mgt_pmkid_cfm(context, req->appHandle, unifi_Restricted, req->action, 0, NULL);
        return;
    }

    status = update_pmkids(context, &fsmData->config, req->action, &pmkids);
    call_unifi_mgt_pmkid_cfm(context, req->appHandle, status, req->action, pmkids.numElements, pmkids.pmkids);

    if (req->action == unifi_ListActionGet)
    {
        CsrPfree(pmkids.pmkids);
    }

    /* Free the pmkid Data copied into the sme */
    CsrPfree(req->setPmkids);
}

/**
 * @brief
 *   Reject a key request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void stopped__key_req(FsmContext* context, const UnifiMgtKeyReq_Evt* req)
{
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "stopped__key_req(%s)", trace_unifi_ListAction(req->action)));
    call_unifi_mgt_key_cfm(context, req->appHandle, unifi_Unavailable, req->action);
}

static void flush_keys(FsmContext* context, const UnifiMgtKeyReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "flush_keys()"));

    CsrMemSet(&fsmData->config.wepkeys, 0x00, sizeof(fsmData->config.wepkeys));
    if (ns_get_connection_status(context) == ns_ConnectionStatus_Connected  &&
        (fsmData->config.IndexSecurityModeTable[0] == unifi_80211AuthShared ||
         fsmData->config.IndexSecurityModeTable[1] == unifi_80211AuthShared ||
         fsmData->config.IndexSecurityModeTable[2] == unifi_80211AuthShared ||
         fsmData->config.IndexSecurityModeTable[3] == unifi_80211AuthShared ) )
    {
        CsrUint8 i;
        DataReference mibSetDataRef = mib_encode_create_set(context, 0, 4);
        for (i = 0; i < 4; ++i)
        {
            if(fsmData->config.IndexSecurityModeTable[i] == unifi_80211AuthShared)
            {
                /* Send a NULL OS... */
                (void)mib_encode_add_set_os(context, &mibSetDataRef, dot11WEPDefaultKeyValue, fsmData->config.wepkeys.keys[i].key, 0, (CsrUint8)fsmData->config.connectionInfo.ifIndex, (CsrUint8)(i + 1));
            }
        }
        fsmData->hostKey = TRUE;
        fsmData->cfmAppHandle = req->appHandle;
        send_mlme_set_req_internal(context, getSmeContext(context)->mibAccessInstance, mibSetDataRef);
        fsm_next_state(context, FSMSTATE_waiting_for_cfm);
    }
    else
    {
        /* Not connected so no Keys to remove */
        call_unifi_mgt_key_cfm(context, req->appHandle, unifi_Success, fsmData->action);
    }
}

static void add_key(FsmContext* context, const UnifiMgtKeyReq_Evt* req, const unifi_Key* key)
{
    FsmData* fsmData = FSMDATA;
    ns_ConnectionStatus connectionStatus = ns_get_connection_status(context);
    unifi_Status status = unifi_Success;
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "add_key(index: %d, length:%d)", key->keyIndex, key->keyLength));
    if (key->keyIndex > 3)
    {
        sme_trace_error((TR_SME_CONFIGURATION_FSM, "add_key(): key index(%d) out of range 0 - 3", key->keyIndex));
        call_unifi_mgt_key_cfm(context, req->appHandle, unifi_InvalidParameter, fsmData->action);
        return;
    }

    if ((key->keyLength == 0 && key->wepTxKey == TRUE) || key->keyLength == 5 || key->keyLength == 13)
    {
        /* Set a WEP KEY */
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "add_key(): WEP Key : Index:%d, TxKey:%s, Length:%d", key->keyIndex, key->wepTxKey?"TRUE":"FALSE", key->keyLength));
        if (key->keyLength != 0)
        {
            fsmData->config.IndexSecurityModeTable[key->keyIndex] = unifi_80211AuthShared;
            CsrMemCpy(fsmData->config.wepkeys.keys[key->keyIndex].key, key->key, key->keyLength);
            fsmData->config.wepkeys.keys[key->keyIndex].keyLength = key->keyLength;
        }
        if (key->wepTxKey == TRUE)
        {
            fsmData->config.wepkeys.txKey = key->keyIndex;
        }
        if (connectionStatus == ns_ConnectionStatus_Connected)
        {
            DataReference mibSetDataRef = mib_encode_create_set(context, 3, 1);

            if (key->wepTxKey == TRUE)
            {
                if (!ns_dynamic_wep_detected(context) && coex_eapol_detected(context))
                {
                    /* Dynamic Wep Detected */
                    sme_trace_info((TR_SME_CONFIGURATION_FSM, "add_key(): Dynamic WEP Key detected!"));
                    ns_set_dynamic_wep(context);
                }
                (void)ns_port_configure(context, unifi_8021xPortOpen, unifi_8021xPortOpen);

                (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11WEPDefaultKeyID, (CsrInt32)key->keyIndex, (CsrUint8)fsmData->config.connectionInfo.ifIndex, 0);
            }
            if (key->keyLength != 0)
            {
                (void)mib_encode_add_set_os(context, &mibSetDataRef, dot11WEPDefaultKeyValue, key->key, key->keyLength, (CsrUint8)fsmData->config.connectionInfo.ifIndex, (CsrUint8)(key->keyIndex+1));
            }
            (void)mib_encode_add_set_boolean(context, &mibSetDataRef, dot11ExcludeUnencrypted, TRUE, (CsrUint8)fsmData->config.connectionInfo.ifIndex, 0);
            (void)mib_encode_add_set_boolean(context, &mibSetDataRef, unifiExcludeUnencryptedExceptEAPOL, FALSE, (CsrUint8)fsmData->config.connectionInfo.ifIndex, 0);

            fsmData->cfmAppHandle = req->appHandle;
            send_mlme_set_req_internal(context, getSmeContext(context)->mibAccessInstance, mibSetDataRef);
            fsmData->hostKey = TRUE;
            fsm_next_state(context, FSMSTATE_waiting_for_cfm);
            return;
        }
    }
    else if (key->keyLength != 0)
    {
        /* Set a WPA KEY */
        sme_trace_info((TR_SME_CONFIGURATION_FSM, "add_key(): WPA Key : Index = %d, Length = %d", key->keyIndex, key->keyLength));
        if (connectionStatus == ns_ConnectionStatus_Connected &&
            fsmData->config.connectionInfo.authMode != unifi_80211AuthOpen &&
            fsmData->config.connectionInfo.authMode != unifi_80211AuthShared )
        {
            CsrUint16 secMgrInstance = hip_proxy_get_security_manager(context);
            if (secMgrInstance == FSM_TERMINATE)
            {
                sme_trace_error((TR_SME_CONFIGURATION_FSM, "unifi_WPAKeyValue Security Manager not found"));
                status = unifi_Unavailable;
            }
            else
            {
                DataReference dataref;

                /* mark the index as a wpa key */
                fsmData->config.IndexSecurityModeTable[key->keyIndex] = unifi_8021xAuthWPA;

                dataref.dataLength = key->keyLength;
                pld_store(getPldContext(context), (void*)key->key, key->keyLength, &dataref.slotNumber);

                send_mlme_setkeys_req_internal(context, secMgrInstance,
                                               dataref,
                                               (key->keyLength * 8), /* length in bits */
                                               key->keyIndex,
                                               key->keyType,
                                               key->address,
                                               key->keyRsc,
                                               key->authenticator,
                                               0 /* css filled in by sec mgr */);
            }
        }
        else
        {
            sme_trace_error((TR_SME_CONFIGURATION_FSM, "add_key(): Attempted to set a WPA key when not connected to wpa AP"));
            status = unifi_Unavailable;
        }
    }
    else
    {
        sme_trace_error((TR_SME_CONFIGURATION_FSM, "add_key(): Attempted to set a 0 length WPA key"));
        status = unifi_InvalidParameter;
    }

    call_unifi_mgt_key_cfm(context, req->appHandle, status, fsmData->action);
}

static void delete_key(FsmContext* context, const UnifiMgtKeyReq_Evt* req, const unifi_Key* key)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "delete_key(%d)", key->keyIndex));

    if (key->keyIndex > 4)
    {
        sme_trace_error((TR_SME_CONFIGURATION_FSM, "delete_key(): key index(%d) out of range 0 - 4", key->keyIndex));
        call_unifi_mgt_key_cfm(context, req->appHandle, unifi_InvalidParameter, fsmData->action);
        return;
    }
    if(fsmData->config.IndexSecurityModeTable[key->keyIndex] == unifi_80211AuthOpen)
    {
        sme_trace_warn((TR_SME_CONFIGURATION_FSM, "delete_key(): key index(%d) has not got a key set", key->keyIndex));
        call_unifi_mgt_key_cfm(context, req->appHandle, unifi_Success, fsmData->action);
        return;
    }

    if(fsmData->config.IndexSecurityModeTable[key->keyIndex] == unifi_80211AuthShared)
    {
        CsrMemSet(&fsmData->config.wepkeys.keys[key->keyIndex], 0x00, sizeof(fsmData->config.wepkeys.keys[key->keyIndex]));
    }

    if (ns_get_connection_status(context) == ns_ConnectionStatus_Connected)
    {
        if(fsmData->config.IndexSecurityModeTable[key->keyIndex] == unifi_80211AuthShared)
        {
            /* Send a NULL OS... */
            fsmData->cfmAppHandle = req->appHandle;
            mib_util_send_set_os(context, dot11WEPDefaultKeyValue, fsmData->config.wepkeys.keys[key->keyIndex].key, 0, (CsrUint8)fsmData->config.connectionInfo.ifIndex, (CsrUint8)(key->keyIndex + 1));
            fsmData->hostKey = TRUE;
            fsm_next_state(context, FSMSTATE_waiting_for_cfm);
        }
        else if(fsmData->config.IndexSecurityModeTable[key->keyIndex] == unifi_8021xAuthWPA)
        {
            fsmData->cfmAppHandle = req->appHandle;
            send_mlme_deletekeys_req(context, key->keyIndex, key->keyType, key->address);
            fsmData->hostKey = TRUE;
            fsm_next_state(context, FSMSTATE_waiting_for_cfm);
        }
    }
    else
    {
        /* Not connected so no Keys to set */
        call_unifi_mgt_key_cfm(context, req->appHandle, unifi_Success, fsmData->action);
    }

    fsmData->config.IndexSecurityModeTable[key->keyIndex] = unifi_80211AuthOpen;
}

/**
 * @brief
 *   Action a key request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void key_req(FsmContext* context, const UnifiMgtKeyReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "key_req(%s)", trace_unifi_ListAction(req->action)));

    if (isAccessRestricted(context, req->appHandle))
    {
        call_unifi_mgt_key_cfm(context, req->appHandle, unifi_Restricted, req->action);
        return;
    }

    fsmData->action = req->action;

    switch(req->action)
    {
    case unifi_ListActionFlush:
        flush_keys(context, req);
        break;
    case unifi_ListActionAdd:
        add_key(context, req, &req->key);
        break;
    case unifi_ListActionRemove:
        delete_key(context, req, &req->key);
        break;
    case unifi_ListActionGet:
        sme_trace_error((TR_SME_CONFIGURATION_FSM, "key_req() unifi_ListActionGet Not Supported"));
        call_unifi_mgt_key_cfm(context, req->appHandle, unifi_Unsupported, req->action);
        return;
    default:
        break;
    }
}

/**
 * @brief
 *   Reject a multicast address request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void stopped__multicast_address_req(FsmContext* context, const FsmEvent* event)
{
    if (event->id == UNIFI_MGT_MULTICAST_ADDRESS_REQ_ID)
    {
        UnifiMgtMulticastAddressReq_Evt* req = (UnifiMgtMulticastAddressReq_Evt*)event;
        sme_trace_entry((TR_SME_CONFIGURATION_FSM, "stopped__multicast_address_req(%s) from mgt", trace_unifi_ListAction(req->action)));
        call_unifi_mgt_multicast_address_cfm(context, req->appHandle, unifi_Unavailable, req->action, 0, NULL);
        CsrPfree(req->setAddresses);
    }
    else
    {
        UnifiSysMulticastAddressInd_Evt* ind = (UnifiSysMulticastAddressInd_Evt*)event;
        sme_trace_entry((TR_SME_CONFIGURATION_FSM, "stopped__multicast_address_req(%s) from sys", trace_unifi_ListAction(ind->action)));
        call_unifi_sys_multicast_address_rsp(context, unifi_Unavailable, ind->action, 0, NULL);
        CsrPfree(ind->setAddresses);
    }
}

static void addressListAdd(unifi_AddressList* addresses, const unifi_AddressList* newAddress)
{
    unifi_MACAddress* newList;
    CsrUint8 i, j;

    /* No addresses to add */
    if (newAddress->addressesCount == 0)
    {
        return;
    }

    newList = (unifi_MACAddress*)CsrPmalloc(sizeof(unifi_MACAddress) * (addresses->addressesCount + newAddress->addressesCount));

    CsrMemCpy(newList, addresses->addresses, (sizeof(unifi_MACAddress) * addresses->addressesCount) );
    /* Only add address if not already in the list */
    for (j = 0; j < newAddress->addressesCount; ++j)
    {
        CsrBool found = FALSE;
        for (i = 0; i < addresses->addressesCount && found == FALSE; ++i)
        {
            if (CsrMemCmp(&newList[i], &newAddress->addresses[j], sizeof(newAddress->addresses[j].data)) == 0)
            {
                found = TRUE;
            }
        }
        if (found == FALSE)
        {
            CsrMemCpy(&newList[addresses->addressesCount], &newAddress->addresses[j], sizeof(unifi_MACAddress) );
            addresses->addressesCount++;
        }
    }

    CsrPfree(addresses->addresses);

    addresses->addresses = newList;
}

static void addressListRemove(unifi_AddressList* addresses, const unifi_AddressList* removeAddresses)
{
    CsrUint8 i, j;
    for (j = 0; j < removeAddresses->addressesCount; ++j)
    {
        for (i = 0; i < addresses->addressesCount; ++i)
        {
            if (CsrMemCmp(&addresses->addresses[i], &removeAddresses->addresses[j], sizeof(removeAddresses->addresses[j].data)) == 0)
            {
                /* Move the rest of the list Up 1  */
                CsrMemMove(&addresses->addresses[i], &addresses->addresses[i+1], ((addresses->addressesCount  - i) - 1)*(sizeof(unifi_MACAddress)));
                addresses->addressesCount--;
                break;
            }
        }
    }
}

static void addressListFlush(unifi_AddressList* addresses)
{
    /* Clear the current List (Passing NULL to CsrPfree is OK) */
    CsrPfree(addresses->addresses);
    addresses->addressesCount = 0;
    addresses->addresses = NULL;
}


/**
 * @brief
 *   Action a multicast address request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void multicast_address_req(FsmContext* context, const FsmEvent* event)
{
    UnifiMgtMulticastAddressReq_Evt* req = (UnifiMgtMulticastAddressReq_Evt*)event;
    FsmData* fsmData = FSMDATA;
    CsrUint8 currentAddressCount = fsmData->config.multicastAddresses.addressesCount;
    ns_ConnectionStatus connectionStatus = ns_get_connection_status(context);
    DataReference mibSetDataRef;
    CsrUint8 i;
    unifi_ListAction  action;
    unifi_AddressList setAddressList;

    if (event->id == UNIFI_SYS_MULTICAST_ADDRESS_IND_ID)
    {
        UnifiSysMulticastAddressInd_Evt* ind = (UnifiSysMulticastAddressInd_Evt*)event;
        action = ind->action;
        setAddressList.addressesCount = ind->setAddressesCount;
        setAddressList.addresses = ind->setAddresses;
    }
    else
    {
        action = req->action;
        setAddressList.addressesCount = req->setAddressesCount;
        setAddressList.addresses = req->setAddresses;
    }

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "multicast_address_req(%s)", trace_unifi_ListAction(action)));

    switch(action)
    {
    case unifi_ListActionFlush:
        addressListFlush(&fsmData->config.multicastAddresses);
        break;
    case unifi_ListActionAdd:
        addressListAdd(&fsmData->config.multicastAddresses, &setAddressList);
        break;
    case unifi_ListActionRemove:
        addressListRemove(&fsmData->config.multicastAddresses, &setAddressList);
        break;
    case unifi_ListActionGet:
        {
        if (event->id == UNIFI_MGT_MULTICAST_ADDRESS_REQ_ID)
        {
            call_unifi_mgt_multicast_address_cfm(context,
                                                 req->appHandle,
                                                 unifi_Success,
                                                 action,
                                                 fsmData->config.multicastAddresses.addressesCount,
                                                 fsmData->config.multicastAddresses.addresses);
        }
        else
        {
            call_unifi_sys_multicast_address_rsp(context,
                                                 unifi_Success,
                                                 action,
                                                 fsmData->config.multicastAddresses.addressesCount,
                                                 fsmData->config.multicastAddresses.addresses);

        }
        /* Free the Multicast Addresses Data copied into the sme */
        CsrPfree(setAddressList.addresses);
        setAddressList.addresses = NULL;
        setAddressList.addressesCount = 0;
        return;
    }
    default:
        break;
    }

    /* Free the Multicast Addresses Data copied into the sme */
    CsrPfree(setAddressList.addresses);
    setAddressList.addresses = NULL;
    setAddressList.addressesCount = 0;

    if (connectionStatus == ns_ConnectionStatus_Connected &&
        (fsmData->config.multicastAddresses.addressesCount || currentAddressCount) &&
        ns_get_controlled_port_state(context) == unifi_8021xPortOpen)
    {
        fsmData->getSetRequester = event->sender_;
        fsmData->multicastAddressesId = event->id;
        mibSetDataRef = mib_encode_create_set(context, (CsrUint8)CSRMAX(fsmData->config.multicastAddresses.addressesCount, currentAddressCount), 2);

        for (i = 0; i < fsmData->config.multicastAddresses.addressesCount; ++i)
        {
            (void)mib_encode_add_set_os (context, &mibSetDataRef, dot11Address, (CsrUint8*)&fsmData->config.multicastAddresses.addresses[i], 6, (CsrUint8)fsmData->config.connectionInfo.ifIndex, (CsrUint8)(i+1));
            (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11GroupAddressesStatus, 1 /* Active */, (CsrUint8)fsmData->config.connectionInfo.ifIndex, (CsrUint8)(i+1));
        }
        for (; i < currentAddressCount; i++)
        {
            (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11GroupAddressesStatus, 6 /* destroy */, (CsrUint8)fsmData->config.connectionInfo.ifIndex, (CsrUint8)(i+1));
        }
        send_mlme_set_req_internal(context, getSmeContext(context)->mibAccessInstance, mibSetDataRef);

        fsmData->hostMulticast = TRUE;
        fsmData->action = action;

        fsmData->cfmAppHandle = req->appHandle;
        fsm_next_state(context, FSMSTATE_waiting_for_cfm);
    }
    else if (event->id == UNIFI_MGT_MULTICAST_ADDRESS_REQ_ID && event->sender_ == FSM_ENV)
    {
        call_unifi_mgt_multicast_address_cfm(context,
                                             req->appHandle,
                                             unifi_Success,
                                             action,
                                             fsmData->config.multicastAddresses.addressesCount,
                                             fsmData->config.multicastAddresses.addresses);
    }
    else if (event->id == UNIFI_SYS_MULTICAST_ADDRESS_IND_ID && event->sender_ == FSM_ENV)
    {
        call_unifi_sys_multicast_address_rsp(context,
                                             unifi_Success,
                                             action,
                                             fsmData->config.multicastAddresses.addressesCount,
                                             fsmData->config.multicastAddresses.addresses);
    }
}


/**
 * @brief
 *   Action a blacklist request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void process_blacklist_req(FsmContext* context, unifi_ListAction action, unifi_AddressList* addressList)
{
    SmeConfigData* cfg = get_sme_config(context);
    unifi_AddressList emptyAddressList = {0, NULL};
    if (addressList == NULL) {
        addressList = &emptyAddressList;
    }

    switch(action)
    {
    case unifi_ListActionFlush:
        addressListFlush(&cfg->userBlackList);
        break;
    case unifi_ListActionAdd:
    {
        ns_ConnectionStatus connectionStatus = ns_get_connection_status(context);
        addressListAdd(&cfg->userBlackList, addressList);
        if (connectionStatus == ns_ConnectionStatus_Connected &&
            black_list_is_blacklisted(context, &cfg->connectionInfo.bssid))
        {
            send_network_selector_roam_ind(context, getSmeContext(context)->networkSelectorInstance, unifi_RoamBetterAPFound);
        }
        break;
    }
    case unifi_ListActionRemove:
        addressListRemove(&cfg->userBlackList, addressList);
        break;
    case unifi_ListActionGet:
        *addressList = cfg->userBlackList;
        break;
    default:
        break;
    }
}

/**
 * @brief
 *   Action a blacklist request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void blacklist_req(FsmContext* context, const UnifiMgtBlacklistReq_Evt* req)
{
    unifi_AddressList addressList;
    unifi_Status status = unifi_Success;

    if (isAccessRestricted(context, req->appHandle))
    {
        status = unifi_Restricted;
        addressList.addressesCount = 0;
        addressList.addresses      = NULL;
    }
    else
    {
        addressList.addressesCount = req->setAddressCount;
        addressList.addresses      = req->setAddresses;
        process_blacklist_req(context, req->action, &addressList);
    }
    call_unifi_mgt_blacklist_cfm(context,
                                 req->appHandle,
                                 status,
                                 req->action,
                                 addressList.addressesCount,
                                 addressList.addresses);

    /* Free the Blacklist Data copied into the sme */
    CsrPfree(req->setAddresses);
}

/**
 * @brief
 *   Action a ind register request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void event_mask_set_req(FsmContext* context, const UnifiMgtEventMaskSetReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    CsrBool foundExisting = FALSE;
    CsrUint8 i;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "event_mask_set_req(%p, 0x%.4X) Size = %d Count = %d", req->appHandle, req->indMask,
                                                                                                    fsmData->config.indRegistrationSize,
                                                                                                    fsmData->config.indRegistrationCount));
    for (i = 0; i < fsmData->config.indRegistrationCount; ++i)
    {
        if (req->appHandle == fsmData->config.indRegistrations[i].appHandle)
        {
            if (req->indMask == unifi_IndNone)
            {
                sme_trace_debug((TR_SME_CONFIGURATION_FSM, "event_mask_set_req(%p, 0x%.4X) Removed", req->appHandle, req->indMask));
                /* Move the rest of the list Up 1  */
                CsrMemMove(&fsmData->config.indRegistrations[i],
                            &fsmData->config.indRegistrations[i+1],
                            ((fsmData->config.indRegistrationCount  - i) - 1)*(sizeof(IndRegistration)));
                fsmData->config.indRegistrationCount--;
            }
            else
            {
                sme_trace_debug((TR_SME_CONFIGURATION_FSM, "event_mask_set_req(%p, 0x%.4X) Updated", req->appHandle, req->indMask));
                fsmData->config.indRegistrations[i].indMask = req->indMask;
            }
            foundExisting = TRUE;
            break;
        }
    }

    if (foundExisting == FALSE && req->indMask != unifi_IndNone)
    {
        sme_trace_debug((TR_SME_CONFIGURATION_FSM, "event_mask_set_req(%p, 0x%.4X) Added", req->appHandle, req->indMask));
        /* Add new entry */
        if (fsmData->config.indRegistrationCount == fsmData->config.indRegistrationSize)
        {
            /* Need to allocate more memory */
            IndRegistration* newIndRegistrations = CsrPmalloc((fsmData->config.indRegistrationSize + 4) * sizeof(IndRegistration));
            fsmData->config.indRegistrationSize += 4;

            if (fsmData->config.indRegistrationCount > 0)
            {
                /* Copy existing data */

                CsrMemCpy(newIndRegistrations, fsmData->config.indRegistrations, fsmData->config.indRegistrationCount * sizeof(IndRegistration));
                CsrPfree(fsmData->config.indRegistrations);
            }
            fsmData->config.indRegistrations = newIndRegistrations;
        }
        fsmData->config.indRegistrations[fsmData->config.indRegistrationCount].appHandle = req->appHandle;
        fsmData->config.indRegistrations[fsmData->config.indRegistrationCount].indMask   = req->indMask;
        fsmData->config.indRegistrationCount++;
    }

    call_unifi_mgt_event_mask_set_cfm(context, req->appHandle, unifi_Success);
}


/**
 * @brief
 *   Activate Access Restrictions
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void restricted_access_enable_req(FsmContext* context, const UnifiMgtRestrictedAccessEnableReq_Evt* req)
{
    SmeConfigData* cfg = get_sme_config(context);
    unifi_Status status  = unifi_Restricted;
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "restricted_access_enable_req(%p)", req->appHandle));

    if ( cfg->smeConfig.enableRestrictedAccess == FALSE ||
        (cfg->smeConfig.enableRestrictedAccess == TRUE && cfg->restrictedAccessActivated == FALSE) ||
        (cfg->smeConfig.enableRestrictedAccess == TRUE && cfg->restrictedAccessActivated == TRUE && cfg->restrictedAccessAppHandle == req->appHandle))
    {
        /* Implicit enabling of access restrictions */
        cfg->smeConfig.enableRestrictedAccess = TRUE;

        cfg->restrictedAccessActivated = TRUE;
        cfg->restrictedAccessAppHandle = req->appHandle;
        status = unifi_Success;
    }

    call_unifi_mgt_restricted_access_enable_cfm(context, req->appHandle, status);
}

/**
 * @brief
 *   Activate Access Restrictions
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void restricted_access_disable_req(FsmContext* context, const UnifiMgtRestrictedAccessDisableReq_Evt* req)
{
    unifi_Status status  = unifi_Restricted;
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "restricted_access_disable_req(%p)", req->appHandle));
    if (!isAccessRestricted(context, req->appHandle))
    {
        SmeConfigData* cfg = get_sme_config(context);

        cfg->restrictedAccessActivated = FALSE;
        cfg->restrictedAccessAppHandle = NULL;
        status = unifi_Success;
    }
    call_unifi_mgt_restricted_access_disable_cfm(context, req->appHandle, status);
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in connection_manager_fsm/sme_configuration_fsm.h
 */
SmeConfigData* get_sme_config(FsmContext* context)
{
    return &(fsm_get_params_by_id(context, getSmeContext(context)->smeConfigurationInstance, FsmData))->config;
}

unifi_Status get_sme_config_value(FsmContext* context, unifi_AppValue* appValue)
{
    return get_value(context, fsm_get_params_by_id(context, getSmeContext(context)->smeConfigurationInstance, FsmData), appValue);
}

static void copy_datablock(unifi_DataBlock* data)
{
    if (data->length)
    {
        CsrUint8* buf = (CsrUint8*) CsrPmalloc(data->length);
        CsrMemCpy(buf, data->data, data->length);
        data->data = buf;
    }
}

unifi_Status set_sme_config_value(FsmContext* context, const unifi_AppValue* appValue)
{
    /* Need to copy the set data here */
    unifi_AppValue appValueCopy = *appValue;
    switch(appValueCopy.id)
    {
    case unifi_CalibrationDataValue:
        copy_datablock(&appValueCopy.unifi_Value_union.calibrationData);
        break;
    default:
        break;
    }

    return set_value(context, fsm_get_params_by_id(context, getSmeContext(context)->smeConfigurationInstance, FsmData), &appValueCopy);
}

unifi_Status set_sme_config_pmkids(FsmContext* context, unifi_ListAction action, unifi_PmkidList *pmkids)
{
    SmeConfigData* cfg = get_sme_config(context);
    if (context->instanceArray[getSmeContext(context)->smeConfigurationInstance].state == FSMSTATE_stopped)
    {
        return unifi_Unavailable;
    }

    return update_pmkids(context, cfg, action, pmkids);
}

unifi_Status set_sme_config_blacklist(FsmContext* context, unifi_ListAction action, unifi_AddressList *list)
{
    if (context->instanceArray[getSmeContext(context)->smeConfigurationInstance].state == FSMSTATE_stopped)
    {
        return unifi_Unavailable;
    }

    process_blacklist_req(context, action, list);
    return unifi_Success;
}


void black_list_flush(FsmContext* context)
{
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint16 i;
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "black_list_flush()"));

    /* Is AP selectable (MIC failure) */
    for (i = 0; i < MAX_BLACKLIST_SIZE; ++i)
    {
        if (CsrMemCmp(&BroadcastBssid, &cfg->micFailureBlackList[i].address, sizeof(BroadcastBssid.data)) != 0 &&
            CsrTimeLe(cfg->micFailureBlackList[i].restrictUntilTimestamp, now))
        {
            sme_trace_info((TR_SME_CONFIGURATION_FSM, "black_list_flush(%s)", trace_unifi_MACAddress(cfg->micFailureBlackList[i].address, getMacAddressBuffer(context))));
            /* Clear the blacklist as the time has expired */
            cfg->micFailureBlackList[i].address = BroadcastBssid;
        }
    }
}

CsrBool black_list_is_blacklisted(FsmContext* context, const unifi_MACAddress* address)
{
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint16 i;

    /* Check the User blacklist */
    for (i = 0; i < cfg->userBlackList.addressesCount; ++i)
    {
        /* If the user sets a blacklist address of 0xFFFFFFFFFFFF then ALL bssid's are blacklisted */
        if (CsrMemCmp(cfg->userBlackList.addresses[i].data, BroadcastBssid.data, sizeof(BroadcastBssid.data)) == 0)
        {
            sme_trace_debug((TR_SME_CONFIGURATION_FSM, "black_list_is_blacklisted(%s) User Global Blacklist TRUE", trace_unifi_MACAddress(*address, getMacAddressBuffer(context))));
            return TRUE;
        }

        if (CsrMemCmp(cfg->userBlackList.addresses[i].data, address->data, sizeof(address->data)) == 0)
        {
            sme_trace_debug((TR_SME_CONFIGURATION_FSM, "black_list_is_blacklisted(%s) User Blacklist TRUE", trace_unifi_MACAddress(*address, getMacAddressBuffer(context))));
            return TRUE;
        }

    }

    /* Check the SME blacklist */
    black_list_flush(context);
    for (i = 0; i < MAX_BLACKLIST_SIZE; ++i)
    {
        if (CsrMemCmp(cfg->micFailureBlackList[i].address.data, address->data, sizeof(address->data)) == 0)
        {
            sme_trace_debug((TR_SME_CONFIGURATION_FSM, "black_list_is_blacklisted(%s) SME Blacklist TRUE", trace_unifi_MACAddress(*address, getMacAddressBuffer(context))));
            return TRUE;
        }
    }

    sme_trace_debug((TR_SME_CONFIGURATION_FSM, "black_list_is_blacklisted(%s) FALSE", trace_unifi_MACAddress(*address, getMacAddressBuffer(context))));
    return FALSE;
}

void black_list_add(FsmContext* context, const unifi_MACAddress* address, CsrUint32 blackListTimeoutMs)
{
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint16 i;
    CsrUint32 now = fsm_get_time_of_day_ms(context);
    black_list_flush(context);

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "black_list_add(%s, %d)", trace_unifi_MACAddress(*address, getMacAddressBuffer(context)), blackListTimeoutMs));
    for (i = 0; i < MAX_BLACKLIST_SIZE; ++i)
    {
        if (CsrMemCmp(&BroadcastBssid, address, sizeof(BroadcastBssid.data)) == 0)
        {
            sme_trace_debug((TR_SME_CONFIGURATION_FSM, "black_list_add()%s at index %d", trace_unifi_MACAddress(*address, getMacAddressBuffer(context)), i));
            /* Clear the blacklist as the time has expired */
            cfg->micFailureBlackList[i].address = *address;
            cfg->micFailureBlackList[i].restrictUntilTimestamp = (CsrUint32)CsrTimeAdd(now, blackListTimeoutMs);
            break;
        }
    }
    if (i == MAX_BLACKLIST_SIZE)
    {
        sme_trace_debug((TR_SME_CONFIGURATION_FSM, "black_list_add()%s at index 0", trace_unifi_MACAddress(*address, getMacAddressBuffer(context))));
        /* Not enough room in blacklist so stomp on the oldest (this only works once) */
        cfg->micFailureBlackList[0].address = *address;
        cfg->micFailureBlackList[0].restrictUntilTimestamp = (CsrUint32)CsrTimeAdd(now, blackListTimeoutMs);
    }
}

CsrUint16 get_appHandles(FsmContext* context, unifi_IndicationsMask ind, void** appHandles)
{
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint16 count = 0;
    CsrUint8 i;

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "get_appHandles(%s)", trace_unifi_IndicationsMask(ind)));

    if (cfg->indRegistrationCount == 0)
    {
        return 0;
    }

    /* Allocate the max size possible */
    CsrPfree(cfg->tmpIndList);
    cfg->tmpIndList = CsrPmalloc(cfg->indRegistrationCount * sizeof(void*));
    *appHandles = cfg->tmpIndList;

    for (i = 0; i < cfg->indRegistrationCount; ++i)
    {
        if (cfg->indRegistrations[i].indMask & ind)
        {
            cfg->tmpIndList[count] = cfg->indRegistrations[i].appHandle;
            count++;
        }
    }

    return count;
}

CsrBool isAccessRestricted(FsmContext* context, void* appHandle)
{
    SmeConfigData* cfg = get_sme_config(context);

    /*sme_trace_entry((TR_SME_CONFIGURATION_FSM, "isAccessRestricted(%p)", appHandle));*/

    if (cfg->smeConfig.enableRestrictedAccess == FALSE)
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

/** State idle transitions */
static const FsmEventEntry stoppedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                      stopped_start),
    fsm_event_table_entry(UNIFI_MGT_SET_VALUE_REQ_ID,             stopped__set_value_req),
    fsm_event_table_entry(UNIFI_MGT_GET_VALUE_REQ_ID,             stopped__get_value_req),
    fsm_event_table_entry(UNIFI_MGT_PMKID_REQ_ID,                 stopped__pmkid_req),
    fsm_event_table_entry(UNIFI_MGT_MULTICAST_ADDRESS_REQ_ID,     stopped__multicast_address_req),
    fsm_event_table_entry(UNIFI_SYS_MULTICAST_ADDRESS_IND_ID,     stopped__multicast_address_req),
    fsm_event_table_entry(UNIFI_MGT_KEY_REQ_ID,                   stopped__key_req),
};

/** State idle transitions */
static const FsmEventEntry idleTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(UNIFI_MGT_SET_VALUE_REQ_ID,             idle__set_value_req),
    fsm_event_table_entry(UNIFI_MGT_GET_VALUE_REQ_ID,             idle__get_value_req),

    fsm_event_table_entry(CORE_STOP_REQ_ID,                       idle__stop_req),
    fsm_event_table_entry(UNIFI_MGT_PMKID_REQ_ID,                 pmkid_req),
    fsm_event_table_entry(UNIFI_MGT_MULTICAST_ADDRESS_REQ_ID,     multicast_address_req),
    fsm_event_table_entry(UNIFI_SYS_MULTICAST_ADDRESS_IND_ID,     multicast_address_req),
    fsm_event_table_entry(UNIFI_MGT_KEY_REQ_ID,                   key_req),
};

/** State idle transitions */
static const FsmEventEntry waitingForTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_GET_CFM_ID,                        mlme_get_cfm),
    fsm_event_table_entry(MLME_SET_CFM_ID,                        mlme_set_cfm),
    fsm_event_table_entry(MLME_DELETEKEYS_CFM_ID,                 mlmeDeleteKeys_cfm),
    fsm_event_table_entry(UNIFI_MGT_PMKID_REQ_ID,                 pmkid_req),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_MGT_BLACKLIST_REQ_ID,                 blacklist_req),
    fsm_event_table_entry(UNIFI_MGT_EVENT_MASK_SET_REQ_ID,            event_mask_set_req),
    fsm_event_table_entry(UNIFI_MGT_RESTRICTED_ACCESS_ENABLE_REQ_ID,  restricted_access_enable_req),
    fsm_event_table_entry(UNIFI_MGT_RESTRICTED_ACCESS_DISABLE_REQ_ID, restricted_access_disable_req),
};


/** Profile Storage state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
    /*                       State                           State                       Save     */
    /*                       Name                            Transitions                  *       */
    fsm_state_table_entry(FSMSTATE_stopped,             stoppedTransitions,         FALSE ),
    fsm_state_table_entry(FSMSTATE_idle,                idleTransitions,            FALSE ),
    fsm_state_table_entry(FSMSTATE_waiting_for_cfm,     waitingForTransitions,      TRUE ),
};

const FsmProcessStateMachine sme_configuration_fsm =
{
#ifdef FSM_DEBUG
       "SME config",                                            /* SM Process Name       */
#endif
       SME_CONFIGURATION_PROCESS,                               /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                        /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions,FALSE),   /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),   /* Ignore Event handers */
       sme_configuration_init,                                  /* Entry Function        */
       sme_configuration_reset_on_shutdown,                     /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                     /* Trace Dump Function   */
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
