/** @file sme_configuration_fsm.h
 *
 * Public SME configuration FSM API
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_configuration/sme_configuration_fsm.h#5 $
 *
 ****************************************************************************/
#ifndef SME_CONFIGURATION_FSM_H
#define SME_CONFIGURATION_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup sme_configuration
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "security_manager_fsm/pmk_cache.h"
#include "scan_manager_fsm/scan_results_storage.h"
#include "qos_fsm/qos_tspec.h"


/* PUBLIC MACROS ************************************************************/

    /** fastest interval allowed to read the real STATS from the mib */
#define MIN_STATS_READ_INTERVAL_MS 1500

/** The maximum number of access points to track connections for*/
#define MAX_CONNECTED_AP_HISTORY 3

/** The maximum number of access points to track connections for*/
#define MAX_BLACKLIST_SIZE 3


/** The max number of entries in the scan results storage list.  More
 * entries than this will cause the lowest ranked entries to be thrown away.
 * If 0, the limit is disabled.
 */
#define SCAN_RECORD_LIST_MAX 0

/** Number of beacon periods to detect diconnect in an ibss */
#define IBSS_CONNECTION_TIMEOUT_BEACONS 100

/** Number of beacon periods to detect diconnect when connected to an AP */
#define INFRASTRUCTURE_CONNECTION_TIMEOUT_BEACONS 20


/* All thresholds in these macros were decided from empirical data gathered
 * on 18 Sep 2007.
 */
#define RSSI_LIMIT_BEST             (-5)
/** RSSI Threshold - Satisfactory */
#define RSSI_THRESHOLD_SATISFACTORY (-75)
/** RSSI Threshold - Poor */
#define RSSI_THRESHOLD_POOR         (-85)
/** RSSI Threshold - Unusable */
#define RSSI_THRESHOLD_UNUSABLE     (-95)
/** RSSI Limit Worst */
 #define RSSI_LIMIT_WORST            (-105)


/** SNR limit best */
#define SNR_LIMIT_BEST              (30)
/** SNR Threshold - Satisfactory */
#define SNR_THRESHOLD_SATISFACTORY  (15)
/** SNR Threshold - Poor */
#define SNR_THRESHOLD_POOR          (10)
/** SNR Threshold - Unusable */
#define SNR_THRESHOLD_UNUSABLE       (5)
/** SNR Limit  Worst */
#define SNR_LIMIT_WORST              (0)

#define THRESHOLD_OFFSET             (1)

#define RSSI_SATISFACTORY            (-30) /* approx half-way RSSI_LIMIT_BEST and RSSI_THRESHOLD_SATISFACTORY */
#define SNR_SATISFACTORY             (20)  /* approx half-way SNR_LIMIT_BEST and SNR_THRESHOLD_SATISFACTORY */

/* PUBLIC TYPES DEFINITIONS *************************************************/

typedef struct IndRegistration
{
    void*                 appHandle;
    unifi_IndicationsMask indMask;
} IndRegistration;

typedef struct JoinedAddress {
        unifi_MACAddress    address;
        CsrUint8            channel;
        BssType             bssType;
        unifi_SSID          ssid;
        CsrUint32           connDropTimeStamp;
} JoinedAddress;

/**
 * @brief
 *   SME Configuration data
 *
 * @par Description
 *   Struct defining the configuration data for the SME
 */
typedef struct SmeConfigData
{
    /* -------------------------------------------- */
    /* 1 time Initialise                            */
    /* -------------------------------------------- */
    CsrBool                     wifiOff;

    unifi_MACAddress            permanentMacAddress;
    unifi_MACAddress            stationMacAddress;

    /* -------------------------------------------- */
    /* Read Only                                    */
    /* -------------------------------------------- */
    /** Timestamp of last stats Read */
    CsrUint32                   lastStatsRead;
    unifi_ConnectionStats       stats;
    CsrUint32                   lastLinkQualityRead;

    unifi_ConnectionInfo        connectionInfo;
    DataReference               connectionBeaconFrameDataRef;
    DataReference               connectionExchangedFramesDataRef;
    DataReference               assocReqIeDataRef;
    unifi_IEEE80211Reason       disassocReason;
    unifi_IEEE80211Reason       deauthReason;
    /* -------------------------------------------- */

    unifi_AdHocConfig              adHocConfig;
    DataReference                  calibrationData;
    unifi_CoexConfig               coexConfig;
    unifi_CoexInfo                 coexInfo;
    unifi_ConnectionConfig         connectionConfig;
    unifi_HostConfig               hostConfig;
    unifi_PowerConfig              powerConfig;
    unifi_SmeConfig                smeConfig;
    unifi_ScanConfig               scanConfig;
    unifi_Versions                 versions;
    unifi_MibConfig                mibConfig;
    unifi_RoamingConfig            roamingConfig;
    unifi_CcxConfig                ccxConfig;
    unifi_RegulatoryDomainInfo     regDomInfo;
    unifi_LinkQuality              linkQuality;
    unifi_CloakedSsidConfig        cloakedSsids;

    /* Saved so we know which values have been modified */
    unifi_MibConfig                defaultMibConfig;

    /* -------------------------------------------- */
    /* Read Write (From the Mib)                    */
    /* -------------------------------------------- */
    CsrUint16                   dot11CurrentRegDomain;
    char                        dot11CountryString[4];
    CsrInt8                     unifiTxPowerAdjustment_tpo;
    CsrInt8                     unifiTxPowerAdjustment_eirp;
    CsrBool                     regdomReadFromMib;
    unifi_CoexScheme            unifiCoexScheme;
    CsrBool                     wapiOptionImplemented;
    CsrUint8                    unifiCCXVersionImplemented;
    CsrBool                     scheduledInterrupt;

    /* draft n HighThroughput */
    CsrBool                     highThroughputOptionEnabled;

    CsrUint16                   unifiKeepAliveTime;

    /* -------------------------------------------- */
    /* Mib File Data                                */
    /* -------------------------------------------- */
    unifi_DataBlockList mibFiles;

    /* -------------------------------------------- */
    /* Get Set Data                                 */
    /* -------------------------------------------- */
    unifi_AddressList multicastAddresses;

    /* -------------------------------------------- */
    /* Security Data                                */
    /* -------------------------------------------- */
    struct {
        CsrUint8 txKey; /* 0 == No keys  1 -> 4 == tx key index */
        struct {
            CsrUint8 keyLength; /* 0 = No Key, 5 = wep40, 13 = = wep104 */
            CsrUint8 key[13];
        } keys[4];
    } wepkeys;

    unifi_AuthMode IndexSecurityModeTable[5];
    /* -------------------------------------------- */

    /* -------------------------------------------- */
    /* Suspend Resume Data                          */
    /* -------------------------------------------- */
    JoinedAddress lastJoinedAddresses[MAX_CONNECTED_AP_HISTORY];
    /* -------------------------------------------- */

    /* -------------------------------------------- */
    /* Mic Failure Blacklist                        */
    /* -------------------------------------------- */
    struct {
        unifi_MACAddress address;
        CsrUint32        restrictUntilTimestamp;
    } micFailureBlackList[MAX_BLACKLIST_SIZE];

    /* -------------------------------------------- */
    /* User defined Blacklist                       */
    /* -------------------------------------------- */
    unifi_AddressList userBlackList;

    /* -------------------------------------------- */
    /* PMKID's list                                 */
    /* -------------------------------------------- */
    PmkCacheContext* pmk_cache;

    srs_IECapabilityMask  joinIECapabilities;

    /* -------------------------------------------- */
    /* Force init 802.11b passive scan channel mask */
    /* -------------------------------------------- */
    unifi_DataBlock        passiveChannelList;

    /* -------------------------------------------- */
    /* WMM data */
    /* -------------------------------------------- */
    CsrBool                WMMAssociation;
    qos_ac_mask            WMMParamsACmask;

    CsrUint16           indRegistrationSize;
    CsrUint16           indRegistrationCount;
    IndRegistration* indRegistrations;
    void**           tmpIndList;

    /* -------------------------------------------- */
    /* Restricted Access Data */
    /* -------------------------------------------- */
    CsrBool restrictedAccessActivated;
    void* restrictedAccessAppHandle;


    /* -------------------------------------------- */
    /* Roaming Network channel lists */
    /* -------------------------------------------- */
    csr_list roamingChannelLists;

    /* -------------------------------------------- */
    /* PAL data */
    /* -------------------------------------------- */
#ifdef CSR_AMP_ENABLE
    CsrUint16  dot11LongRetryLimit;
    CsrUint16  dot11ShortRetryLimit;
    CsrBool dot11ShortSlotTimeOptionImplemented;
    CsrBool dot11ShortSlotTimeOptionEnabled;
#endif

} SmeConfigData;

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the sme configuration state machine data
 */
extern const FsmProcessStateMachine sme_configuration_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Accessor for the sme config data
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 * @param[in]  instanceId  : The instance to retreive the data from
 *
 * @return
 *   SmeConfigData* - Configuration data
 */
extern SmeConfigData* get_sme_config(FsmContext* context);

/**
 * @brief
 *   Syncronously gets a config value
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context    : FSM context
 * @param[in]    type       : The value to get
 * @param[inout] appValue   : The result data
 *
 * @return
 *   unifi_Status error code
 */
extern unifi_Status get_sme_config_value(FsmContext* context, unifi_AppValue* appValue);

/**
 * @brief
 *   Syncronously sets a config value
 *
 * @par Description
 *   see brief
 *
 * @param[in] context    : FSM context
 * @param[in] appValue   : Data to set
 *
 * @return
 *   unifi_Status error code
 */
extern unifi_Status set_sme_config_value(FsmContext* context, const unifi_AppValue* appValue);

/**
 * @brief
 *   Synchronously updates pmkid's
 *
 * @return
 *   unifi_Status error code
 */
extern unifi_Status set_sme_config_pmkids(FsmContext* context, unifi_ListAction action, unifi_PmkidList *list);

/**
 * @brief
 *   Synchronously updates the user blacklist
 *
 * @return
 *   unifi_Status error code
 */
extern unifi_Status set_sme_config_blacklist(FsmContext* context, unifi_ListAction action, unifi_AddressList *list);


extern void black_list_flush(FsmContext* context);
extern CsrBool black_list_is_blacklisted(FsmContext* context, const unifi_MACAddress* address);
extern void black_list_add(FsmContext* context, const unifi_MACAddress* address, CsrUint32 blackListTimeoutMs);

extern CsrUint16 get_appHandles(FsmContext* context, unifi_IndicationsMask ind, void** appHandles);

extern CsrBool isAccessRestricted(FsmContext* context, void* appHandle);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_CONFIGURATION_FSM_H */
