/** @file coex_fsm.c
 *
 * Coex Implementation
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
 *   Coex Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/coex_fsm/coex_fsm.c#10 $
 *
 ****************************************************************************/

/** @{
 * @ingroup debug_test
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_debug.h"

#include "ie_access/ie_access.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "mgt_sap/mgt_sap_from_sme_interface.h"
#include "sys_sap/sys_sap_from_sme_interface.h"
#include "bt_sap/bt_sap_from_sme_interface.h"
#include "smeio/smeio_trace_types.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "dbg_sap/dbg_sap_from_sme_interface.h"

#include "coex_fsm/coex_fsm.h"
#include "coex_fsm/ta_analyse.h"
#include "hip_proxy_fsm/mib_action_sub_fsm.h"
#include "hip_proxy_fsm/mib_encoding.h"
#include "hip_proxy_fsm/mib_utils.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "power_manager_fsm/power_manager_fsm.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process's Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, FsmData))

#define BT_VERSION_MIN 0x01
#define BT_VERSION_MAX 0x01

#define POST_CONNECT_POWER_SCAN_DELAY_MS 15000
#define POST_CONNECT_POWER_SCAN_VARY_MS 2000

/** Max storage for link data...
 *  Unlikely to be more than 1 */
#define MAX_BT_LINK_DATA 1
#define MAX_BT_SCO_DATA 3


/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/** Lookup table for 802.11 b/g Channel to Frequency in GHz (*100) for use
 * in Bluetooth Coexistence  */
const CsrUint16 frequencyTable2point4Ghz[] =
    {   /* Channel 0 filler */ 0,
        /* Channel 1 & 2    */ 2412, 2417,
        /* Channel 3 & 4    */ 2422, 2427,
        /* Channel 5 & 6    */ 2432, 2437,
        /* Channel 7 & 8    */ 2442, 2447,
        /* Channel 9 & 10   */ 2452, 2457,
        /* Channel 11 & 12  */ 2462, 2467,
        /* Channel 13 & 14  */ 2472, 2484,
    };

/**
 * @brief
 *   FSM States
 *
 * @par Description
 *   Enum defining the FSM States for this FSM process
 */
typedef enum
{
    FSMSTATE_stopped,
    FSMSTATE_disconnected,
    FSMSTATE_connecting,
    FSMSTATE_connected,
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
    FsmTimerId connectTimerId;

    CsrBool scanManagerSuspended;
    CsrBool powerSuspended;
    CsrBool ipConnected;
    CsrBool waitingForKeys;

    struct {
        CsrUint8   hasLinkData;
        struct {
            CsrBool             hasLinkData;
            unifi_MACAddress    aclHandle;
            unifi_BtDeviceRole  role;
            unifi_BtDeviceMode  mode;
            CsrUint16           logicalChannelTypeMask;
            CsrUint8            numberOfGuaranteedLogicalChannels;
            struct {
                CsrBool             hasScoData;
                CsrUint16           scoHandle;
                CsrUint16           period;
                CsrUint16           durationMin;
                CsrUint16           durationMax;
            } scoData[MAX_BT_SCO_DATA];

        } linkData[MAX_BT_LINK_DATA];
    } btlinkData;

    /* Current Coex Periodic Settings */
    CoexistenceDirection currentCoexDirection;

    /* Current Settings */
    unifi_PowerSaveLevel currentPowerSave;
    PeriodicId           currentAddPeriodicId;
    BlackoutId           currentAddBlackoutId;
    BlackoutSource       currentBlackoutSource;
#ifdef CSR_AMP_ENABLE
    Microseconds32 blackoutStartReference;
#endif
    CsrUint16 currentChannelAvoid;

    /* Have we seen Eapol data in this Connection (used to detect dynamicWepKeys) */
    CsrBool eapolDetected;

    /* Packet Filter Info */
    CsrBool packetFilterInstalled;
    CsrBool packetFilterUpdated;
    DataReference packetFilter;
    Ipv4Address packetFilterAddress;
    unifi_PacketFilterMode packetFilterMode;

    /* Traffic Analysis */
    FsmTimerId taTimerId;
    ta_priv_t tapriv;

} FsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Test for WMM UAPSD or PS Poll availablility
 *
 * @param[in]    fsmData           : local data struct
 *
 * @return
 *   PeriodicSchedulingMode : PeriodicScheduleUapsd or PeriodicSchedulePsPoll
 */
static PeriodicSchedulingMode isUapsdOrPSPollAvailable(FsmContext* context)
{
    SmeConfigData* cfg = get_sme_config(context);
    /* this variable was set according to user preference and AP capability */
    CsrBool uapsdAvailable =  cfg->joinIECapabilities & WMMUAPSD_Capable;

    sme_trace_entry((TR_COEX, "isUapsdOrPSPollAvailable(%s)", uapsdAvailable?"PeriodicScheduleUapsd":"PeriodicScheduleNone"));

    return uapsdAvailable?PeriodicSchedulingMode_PeriodicScheduleUapsd:PeriodicSchedulingMode_PeriodicSchedulePsPoll;
}


/**
 * @brief
 *   Pause or Unpause Scan Manager
 *
 * @param[in]    context   : FSM context
 * @param[in]    pause     : True  = Pause the scan manager
 *                           False = Unpause the scan manager
 *
 * @return
 *   void
 */
static void scan_pause_state_change(FsmContext* context, CsrBool pause)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_COEX, "scan_pause_state_change(%s)", pause?"PAUSE":"UNPAUSE"));

    if (fsmData->scanManagerSuspended == pause) return; /* Nothing to do */

    fsmData->scanManagerSuspended = pause;
    if (pause)
    {
        send_sm_pause_req(context, getSmeContext(context)->scanManagerInstance);
    }
    else
    {
        send_sm_unpause_req(context, getSmeContext(context)->scanManagerInstance);
    }
}

/**
 * @brief
 *   Pause or Unpause Scan Manager
 *
 * @param[in]    context   : FSM context
 * @param[in]    suspend   : True  = suspend the Power Manager
 *                           False = Unsuspend the Power Manager
 *
 * @return
 *   void
 */
static void power_state_change(FsmContext* context, CsrBool suspend)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_COEX, "power_state_change(%s)", suspend?"SUSPEND":"RESUME"));

    if (fsmData->powerSuspended == suspend) return; /* Nothing to do */

    fsmData->powerSuspended = suspend;
    if (suspend)
    {
        sme_trace_info((TR_COEX, "power_state_change() Suspending Power Mgr"));
        send_pow_suspend_power_saving_req(context, getSmeContext(context)->powerManagerInstance);
    }
    else
    {
        sme_trace_info((TR_COEX, "power_state_change() Resuming Power Mgr"));
        send_pow_resume_power_saving_req(context, getSmeContext(context)->powerManagerInstance);
    }
}

/**
 * @brief
 *   Set or clear BT channel avoidance
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void update_bt_channel_avoidance(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    FsmState state = fsm_current_state(context);

    sme_trace_entry((TR_COEX, "update_bt_channel_avoidance()"));

    /* BT avoid channel */
    if (cfg->coexInfo.hasBtDevice)
    {
        CsrUint16 channel = 0;
        CsrUint16 bandwidth = 0;
        if ( cfg->coexConfig.coexEnable && cfg->coexConfig.coexAfhChannelEnable && (state == FSMSTATE_connecting || state == FSMSTATE_connected))
        {
            channel = frequencyTable2point4Ghz[cfg->connectionInfo.channelNumber];
            bandwidth = BG_CHANNEL_BANDWIDTH;
        }

        if (fsmData->currentChannelAvoid != channel)
        {
            sme_trace_info((TR_COEX, "update_bt_channel_avoidance() Setting channel:%d, bandwidth:%d", channel, bandwidth));
            call_unifi_bt_active_wifi_channel_req(context, channel, bandwidth);
            fsmData->currentChannelAvoid = channel;
        }
    }
}

/**
 * @brief
 *   Mlme del periodic
 *
 * @param[in]    context           : FSM context
 *
 * @return
 *   void
 */
static void send_mlme_del_periodic(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_COEX, "send_mlme_del_periodic()"));

    if (fsmData->currentAddPeriodicId != 0)
    {
        sme_trace_info((TR_COEX, "send_mlme_del_periodic() Deleting Periodic Entry"));

        send_mlme_del_periodic_req(context, fsmData->currentAddPeriodicId);
        fsmData->currentAddPeriodicId = 0;
        cfg->coexInfo.currentCoexPeriodMs = 0;
        cfg->coexInfo.currentCoexLatencyMs = 0;

        power_send_sys_configure_power_mode(context, FALSE);
    }
}

/**
 * @brief
 *   Mlme add periodic
 *   One of the values of period and latency MUST be zero
 *
 * @param[in]    context : FSM context
 * @param[in]    period  : Period to add
 * @param[in]    latency : latency to add
 *
 * @return
 *   void
 */
static void send_mlme_add_periodic(FsmContext* context, CsrUint16 period, CsrUint16 latency)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    PeriodicSchedulingMode schedulingMode = isUapsdOrPSPollAvailable(context);
    CsrUint32 periodToUse = period;
    if (period == 0)
    {
        periodToUse = latency;
    }

    if (cfg->coexInfo.currentCoexPeriodMs == period &&
        cfg->coexInfo.currentCoexLatencyMs == latency)
    {
        /* Nothing to do */
        return;
    }

    cfg->coexInfo.currentCoexPeriodMs = period;
    cfg->coexInfo.currentCoexLatencyMs = latency;

    sme_trace_entry((TR_COEX, "send_mlme_add_periodic(period:%d, latency:%d) :: %s",
                              cfg->coexInfo.currentCoexPeriodMs,
                              cfg->coexInfo.currentCoexLatencyMs,
                              trace_PeriodicSchedulingMode(schedulingMode)));
    /* make sure the only 1 value is set */
    require(TR_COEX, (cfg->coexInfo.currentCoexPeriodMs == 0 && cfg->coexInfo.currentCoexLatencyMs != 0) || (cfg->coexInfo.currentCoexPeriodMs != 0 && cfg->coexInfo.currentCoexLatencyMs == 0));

    fsmData->currentAddPeriodicId = 1;

    send_mlme_add_periodic_req(context,
                               fsmData->currentAddPeriodicId,
                               (periodToUse * 1000),
                               schedulingMode,
                               cfg->coexConfig.coexPeriodicWakeHost,
                               0 ); /*lint !e571*/

    power_send_sys_configure_power_mode(context, FALSE);
}

/**
 * @brief
 *   mlme del blackout
 *
 * @param[in]    context           : FSM context
 *
 * @return
 *   void
 */
static void send_mlme_del_blackout(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_COEX, "send_mlme_del_periodic()"));

    if (fsmData->currentAddBlackoutId != 0)
    {
        sme_trace_info((TR_COEX, "send_mlme_del_blackout() Deleting Blackout Entry"));

        send_mlme_del_blackout_req(context, fsmData->currentAddBlackoutId);
        fsmData->currentAddBlackoutId = 0;
        cfg->coexInfo.currentBlackoutDurationUs = 0;
        cfg->coexInfo.currentBlackoutPeriodUs = 0;
    }
}

/**
 * @brief
 *   mlme add blackout
 *
 * @param[in]    context : FSM context
 *
 * @return
 *   void
 */
static void send_mlme_add_blackout(FsmContext* context, BlackoutSource blackoutSource, Microseconds32 blackoutStartReference, CsrUint32 blackoutDurationUs, CsrUint32 blackoutPeriodUs)
{
    FsmData* fsmData = FSMDATA;

    fsmData->currentAddBlackoutId = 1;
    sme_trace_info((TR_COEX, "mlme_add_blackout_req(1, BlackoutType_LocalDeviceOnly, %s, blackoutSource:0x%.8X, period:%d, duration:%d)",
                                  trace_BlackoutSource(blackoutSource),
                                  blackoutStartReference,
                                  blackoutPeriodUs,
                                  blackoutDurationUs));

    send_mlme_add_blackout_req(context,
                               fsmData->currentAddBlackoutId,
                               BlackoutType_LocalDeviceOnly,
                               blackoutSource,
                               blackoutStartReference,
                               blackoutPeriodUs,
                               blackoutDurationUs,
                               BroadcastBssid); /*lint !e571*/
}

/**
 * @brief
 *   tsf read result
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : mib get cfm
 *
 * @return
 *   void
 */
static void mibGetLocalTsfConfirm(FsmContext* context, const MlmeGetCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_COEX, "mibGetLocalTsfConfirm(%s)", trace_MibStatus(cfm->status)));

    if (cfm->status == MibStatus_Successful)
    {
        CsrInt32 dummy = 0;
        Microseconds32 blackoutStartReference = 0;

        (void)mib_decode_get_int64(context, &cfm->mibAttributeValue, 0, &dummy, &blackoutStartReference);
#ifdef CSR_AMP_ENABLE
        fsmData->blackoutStartReference = blackoutStartReference;
#endif

        sme_trace_entry((TR_COEX, "mibGetLocalTsfConfirm() mlme_add_blackout(startref: 0x%.8X, duration: %d, period: %d)", blackoutStartReference, cfg->coexInfo.currentBlackoutDurationUs, cfg->coexInfo.currentBlackoutPeriodUs));

        send_mlme_add_blackout(context, fsmData->currentBlackoutSource,
                                        blackoutStartReference,
                                        cfg->coexInfo.currentBlackoutDurationUs,
                                        cfg->coexInfo.currentBlackoutPeriodUs);
    }
    else
    {
        sme_trace_entry((TR_COEX, "mibGetLocalTsfConfirm(%s) Mib get of unifiTSFTime Failed", trace_MibStatus(cfm->status)));
    }

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }
}


/**
 * @brief
 *   mlme add blackout
 *
 * @param[in]    context : FSM context
 *
 * @return
 *   void
 */
static void mlme_add_blackout(FsmContext* context, BlackoutSource blackoutSource, CsrUint32 blackoutDurationUs, CsrUint32 blackoutPeriodUs)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    if (fsmData->currentBlackoutSource          != blackoutSource ||
        cfg->coexInfo.currentBlackoutDurationUs != blackoutDurationUs ||
        cfg->coexInfo.currentBlackoutPeriodUs   != blackoutPeriodUs)
    {
        cfg->coexInfo.currentBlackoutDurationUs = blackoutDurationUs;
        cfg->coexInfo.currentBlackoutPeriodUs = blackoutPeriodUs;
        fsmData->currentBlackoutSource = blackoutSource;

        if (blackoutSource == BlackoutSource_Dot11Local)
        {
            DataReference mibGetDataRef = mib_encode_create_get(context, 1);
            (void)mib_encode_add_get(context, &mibGetDataRef, unifiTSFTime, 0, 0);
            mib_get_sub_fsm_start(context, *mibGetLocalTsfConfirm, &mibGetDataRef, FALSE);
        }
        else
        {
            send_mlme_add_blackout(context, blackoutSource, 0, blackoutDurationUs, blackoutPeriodUs);
        }
    }
}

/**
 * @brief
 *   Update the packet filter setting
 *   One of the values of period and latency MUST be zero
 *
 * @param[in]    context : FSM context
 *
 * @return
 *   void
 */
static void update_packet_filter(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    DataReference filter = {0, 0};
    Ipv4Address filterAddress = 0xFFFFFFFF;

    /* Install the Filter ONLY IF
     * We have occasional wifi traffic
     * AND a filter to install ( tclass or valid ARP Address filter )
     * AND an ipaddress has been configured OR the connection timer has timed out.
     */
    CsrBool installFilter = cfg->coexInfo.hasTrafficData &&
                            cfg->coexInfo.currentTrafficType == unifi_TrafficOccasional &&
                            (fsmData->packetFilter.dataLength != 0 || fsmData->packetFilterAddress != filterAddress) &&
                            (fsmData->ipConnected || fsmData->connectTimerId.uniqueid == 0);

    sme_trace_entry((TR_COEX, "update_packet_filter(%s)", installFilter?"TRUE":"FALSE"));

    if (fsmData->packetFilterInstalled == installFilter)
    {
        /* return if no update was received OR we do not want to install a filter */
        if (!fsmData->packetFilterUpdated || installFilter == FALSE)
        {
            return;
        }
    }

    fsmData->packetFilterUpdated = FALSE;

    if (!installFilter)
    {
        /* Clear the current Filter */
        fsmData->packetFilterInstalled = FALSE;
    }
    else
    {
        /* Install the configured Filter */
        fsmData->packetFilterInstalled = TRUE;
        filterAddress = fsmData->packetFilterAddress;
        if (fsmData->packetFilter.dataLength != 0)
        {
            pld_add_ref(getPldContext(context), fsmData->packetFilter.slotNumber);
            filter = fsmData->packetFilter;
        }

        sme_trace_entry((TR_COEX, "connected_packet_filter() mode:%s, address:%.8X",
                                            trace_unifi_PacketFilterMode(fsmData->packetFilterMode),
                                            fsmData->packetFilterAddress));
    }

    send_mlme_set_unitdata_filter_req(context, filter, fsmData->packetFilterMode, filterAddress);
}

/**
 * @brief
 *   update the power mode
 *
 * @param[in]    context           : FSM context
 * @param[in]    requiredPowerSave : desired power mode
 * @param[in]    uapsd             : use uapsd?
 *
 * @return
 *   void
 */
static void update_power_mode(FsmContext* context, unifi_PowerSaveLevel requiredPowerSave)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_COEX, "update_power_mode(%s)", trace_unifi_PowerSaveLevel(requiredPowerSave)));

    if (fsmData->currentPowerSave != requiredPowerSave)
    {
        sme_trace_info((TR_COEX, "update_power_mode() Changing Power Mode from %s to %s", trace_unifi_PowerSaveLevel(fsmData->currentPowerSave), trace_unifi_PowerSaveLevel(requiredPowerSave)));
        fsmData->currentPowerSave = requiredPowerSave;
        send_pow_update_power_saving_req(context, getSmeContext(context)->powerManagerInstance);
    }
}

/**
 * @brief
 *   Update the System parameters for WIFI Only or ACL mode
 */
static void update_system_configuration_wifi_only_or_acl(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_COEX, "update_system_configuration_wifi_only_or_acl()"));

    send_mlme_del_blackout(context);

    if (cfg->hostConfig.powerMode == unifi_HostActive)
    {
        sme_trace_info((TR_COEX, "update_system_configuration_wifi_only_or_acl(unifi_HostActive)"));
        update_power_mode(context, unifi_PowerSaveLow);
        send_mlme_del_periodic(context);
    }
    else if (cfg->powerConfig.powerSaveLevel != unifi_PowerSaveAuto)
    {
        sme_trace_info((TR_COEX, "update_system_configuration_wifi_only_or_acl(cfg->powerConfig.powerSaveLevel != unifi_PowerSaveAuto :: %s)", trace_unifi_PowerSaveLevel(cfg->powerConfig.powerSaveLevel)));
        update_power_mode(context, cfg->powerConfig.powerSaveLevel);
        send_mlme_del_periodic(context);
    }
    else if (!cfg->coexInfo.hasTrafficData || cfg->coexInfo.currentTrafficType == unifi_TrafficOccasional)
    {
        sme_trace_info((TR_COEX, "update_system_configuration_wifi_only_or_acl(!cfg->coexInfo.hasTrafficData || cfg->coexInfo.currentTrafficType == unifi_TrafficOccasional)"));
        update_power_mode(context, unifi_PowerSaveHigh);
        send_mlme_del_periodic(context);
    }
    else if (cfg->coexInfo.currentTrafficType == unifi_TrafficBursty)
    {
        sme_trace_info((TR_COEX, "update_system_configuration_wifi_only_or_acl(cfg->coexInfo.currentTrafficType == unifi_TrafficBursty)"));
        update_power_mode(context, unifi_PowerSaveMed);
        send_mlme_del_periodic(context);
    }
    else if (cfg->coexInfo.currentTrafficType == unifi_TrafficContinuous)
    {
        unifi_PowerSaveLevel requiredPowerSave = fsmData->currentPowerSave;
        sme_trace_info((TR_COEX, "update_system_configuration_wifi_only_or_acl(cfg->coexInfo.currentTrafficType == unifi_TrafficContinuous)"));
        requiredPowerSave = unifi_PowerSaveLow;
        update_power_mode(context, requiredPowerSave);
        send_mlme_del_periodic(context);
    }
    else if (cfg->coexInfo.currentTrafficType == unifi_TrafficPeriodic)
    {
        sme_trace_info((TR_COEX, "update_system_configuration_wifi_only_or_acl(cfg->coexInfo.currentTrafficType == unifi_TrafficPeriodic, period == %d)", cfg->coexInfo.currentPeriodMs));
        update_power_mode(context, unifi_PowerSaveHigh);
        send_mlme_add_periodic(context, cfg->coexInfo.currentPeriodMs, 0);
    }
}

/**
 * @brief
 *   Update the System parameters for SCO Active
 */
static void update_system_configuration_bt_link(FsmContext* context, BlackoutSource blackoutSource, CsrUint32 blackoutDurationUs, CsrUint32 blackoutPeriodUs)
{
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_COEX, "update_system_configuration_bt_link(%s, duration:%d, period:%d)", trace_BlackoutSource(blackoutSource), blackoutDurationUs, blackoutPeriodUs));

    mlme_add_blackout(context, blackoutSource, blackoutDurationUs, blackoutPeriodUs);
    update_power_mode(context, unifi_PowerSaveHigh);

    if (!cfg->coexInfo.hasTrafficData || cfg->coexInfo.currentTrafficType == unifi_TrafficOccasional)
    {
        sme_trace_info((TR_COEX, "update_system_configuration_bt_link(!cfg->coexInfo.hasTrafficData || cfg->coexInfo.currentTrafficType == unifi_TrafficOccasional)"));
        /* Add periodic data to enable unity
         * We wake up on the listen interval anyway so this effectively just protects the SCO/eSCO data */
        send_mlme_add_periodic(context, 0, (CsrUint16)((cfg->connectionInfo.beaconPeriodTu * cfg->connectionInfo.assocReqListenIntervalBeacons) + 30));
    }
    else if (cfg->coexInfo.currentTrafficType == unifi_TrafficBursty)
    {
        sme_trace_info((TR_COEX, "update_system_configuration_bt_link(cfg->coexInfo.currentTrafficType == unifi_TrafficBursty)"));
        send_mlme_add_periodic(context, 0, cfg->coexConfig.coexTrafficBurstyLatencyMs);
    }
    else if (cfg->coexInfo.currentTrafficType == unifi_TrafficContinuous)
    {
        sme_trace_info((TR_COEX, "update_system_configuration_bt_link(cfg->coexInfo.currentTrafficType == unifi_TrafficContinuous)"));
        send_mlme_add_periodic(context, 0, cfg->coexConfig.coexTrafficContinuousLatencyMs);
    }
    else if (cfg->coexInfo.currentTrafficType == unifi_TrafficPeriodic)
    {
        sme_trace_info((TR_COEX, "update_system_configuration_bt_link(cfg->coexInfo.currentTrafficType == unifi_TrafficPeriodic)"));
        send_mlme_add_periodic(context, cfg->coexInfo.currentPeriodMs, 0);
    }
}


/**
 * @brief
 *   Update the System parameters
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void update_system_configuration(FsmContext* context)
{
    CsrUint32 blackoutDurationUs;
    CsrUint32 blackoutPeriodUs;
    BlackoutSource blackoutSource;
    CsrBool wifiOnlyOrAcl;

    sme_trace_entry((TR_COEX, "update_system_configuration()"));

    if (coex_get_blackout_configuration(context,&wifiOnlyOrAcl, &blackoutDurationUs,&blackoutPeriodUs,&blackoutSource))
    {
        if (wifiOnlyOrAcl)
        {
            update_system_configuration_wifi_only_or_acl(context);
        }
        else
        {
            update_system_configuration_bt_link(context, blackoutSource, blackoutDurationUs, blackoutPeriodUs);
        }
    }
}

/**
 * @brief
 *   FSM Entry Function
 *
 * @par Description
 *   Called on startup to initialise the process
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void coex_init(FsmContext* context)
{
    FsmData* fsmData;

    sme_trace_entry((TR_COEX, "coex_init()"));
    fsm_create_params(context, FsmData);
    fsmData = FSMDATA;

    fsmData->packetFilter.dataLength = 0;
    fsmData->packetFilterAddress = 0xFFFFFFFF;
    fsmData->currentCoexDirection = CoexistenceDirection_CoexistenceNone;
    fsmData->currentChannelAvoid = 0;
    fsmData->currentBlackoutSource = BlackoutSource_Dot11Local;
    fsmData->currentAddPeriodicId = 0;

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   FSM Reset Function
 *
 * @par Description
 *   Called on sme hard reset
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void coex_reset(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_COEX, "coex_reset()"));
    if (fsmData->packetFilter.dataLength)
    {
        pld_release(getPldContext(context), fsmData->packetFilter.slotNumber);
    }
}

/**
 * @brief
 *   Start Request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void stopped_start(FsmContext* context, const CoreStartReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    unifi_TrafficConfig filter;

    sme_trace_entry((TR_COEX, "stopped_start()"));

    fsmData->scanManagerSuspended = FALSE;
    fsmData->powerSuspended = FALSE;
    fsmData->ipConnected = FALSE;
    fsmData->waitingForKeys = FALSE;
    fsmData->eapolDetected = FALSE;

    fsmData->currentPowerSave = unifi_PowerSaveLow;
    fsmData->currentAddPeriodicId = 0;
    fsmData->currentAddBlackoutId = 0;

    CsrMemSet(&fsmData->btlinkData, 0 , sizeof(fsmData->btlinkData));

    cfg->coexInfo.hasTrafficData = FALSE;
    cfg->coexInfo.currentTrafficType = unifi_TrafficOccasional;
    cfg->coexInfo.currentPeriodMs = 0;
    cfg->coexInfo.currentCoexPeriodMs = 0;
    cfg->coexInfo.currentCoexLatencyMs = 0;
    cfg->coexInfo.hasBtDevice = FALSE;
    cfg->coexInfo.currentBlackoutDurationUs = 0;
    cfg->coexInfo.currentBlackoutPeriodUs = 0;
    cfg->coexInfo.currentPowerSave = unifi_PowerSaveLow;
    cfg->coexInfo.currentCoexScheme = cfg->unifiCoexScheme;

    fsmData->connectTimerId.uniqueid = 0;
    fsmData->taTimerId.uniqueid = 0;

    fsmData->packetFilterInstalled = FALSE;
    fsmData->packetFilterUpdated = FALSE;

    if (cfg->coexConfig.coexEnableSchemeManagement && cfg->unifiCoexScheme != unifi_CoexSchemeDisabled)
    {
        cfg->coexInfo.currentCoexScheme = unifi_CoexSchemeDisabled;
        mib_util_send_set_int(context, unifiCoexScheme, unifi_CoexSchemeDisabled, 0, 0);
    }

    call_unifi_bt_wifi_active_req(context);

    /* Initial Suspend Power Saving */
    power_state_change(context, TRUE);

    /* Tell the Driver we want to receive dhcp and eapol packet indications */
    CsrMemSet(&filter.customFilter, 0x00, sizeof(filter.customFilter));
    filter.packetFilter = unifi_TrafficPacketEapol | unifi_TrafficPacketDhcp | unifi_TrafficPacketDhcpAck;
    call_unifi_sys_traffic_config_req(context, unifi_TrafficConfigFilter, &filter);

    send_core_start_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_disconnected);
}

/**
 * @brief
 *   Stop request when in disconnected state
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void disconnected_stop(FsmContext* context, const CoreStopReq_Evt* req)
{
    sme_trace_entry((TR_COEX, "disconnected_stop()"));

    /* Call final even if not hasBtDevice (messages may cross) */
    call_unifi_bt_wifi_inactive_req(context);

    send_core_stop_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   Connecting ind when in disconnected state
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void disconnected_connecting(FsmContext* context, const CoexConnectingInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_COEX, "disconnected_connecting()"));

    /* Pause the scan Manager */
    scan_pause_state_change(context, TRUE);

    /* Suspend Power Saving */
    power_state_change(context, TRUE);

    fsmData->eapolDetected = FALSE;

    fsm_next_state(context, FSMSTATE_connecting);

    /* Update BT Channel AFTER state change */
    update_bt_channel_avoidance(context);
}

/**
 * @brief
 *   Connected ind when in connecting state
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void connecting_connected(FsmContext* context, const CoexConnectedInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_COEX, "connecting_connected()"));

    /* Reset the current Coex settings as we may have had an MLME Reset */
    cfg->coexInfo.hasTrafficData = FALSE;
    cfg->coexInfo.currentTrafficType = unifi_TrafficOccasional;
    cfg->coexInfo.currentPeriodMs = 0;
    cfg->coexInfo.currentPowerSave = unifi_PowerSaveLow;

    cfg->coexInfo.currentCoexPeriodMs = 0;
    cfg->coexInfo.currentCoexLatencyMs = 0;

    cfg->coexInfo.currentBlackoutDurationUs = 0;
    cfg->coexInfo.currentBlackoutPeriodUs = 0;

    fsmData->eapolDetected = FALSE;
    fsmData->currentPowerSave = unifi_PowerSaveLow;
    fsmData->waitingForKeys = FALSE;
    if (cfg->connectionInfo.authMode != unifi_80211AuthOpen && cfg->connectionInfo.authMode != unifi_80211AuthShared)
    {
        sme_trace_debug((TR_COEX, "connecting_connected() waitingForKeys == TRUE"));
        fsmData->waitingForKeys = TRUE;
    }

    if (fsmData->ipConnected && !fsmData->waitingForKeys)
    {
        /* Unpause the scan Manager */
        scan_pause_state_change(context, FALSE);

        /* Update Coex / Power saving */
        update_system_configuration(context);

        /* Resume Power Saving */
        power_state_change(context, FALSE);

        /* Install the packet filter if available */
        update_packet_filter(context);

    }
    else
    {
        send_coex_connection_delay_timer(context, fsmData->connectTimerId, POST_CONNECT_POWER_SCAN_DELAY_MS, POST_CONNECT_POWER_SCAN_VARY_MS);
    }

    /* Start the TA Module */
    unifi_ta_analyse_init(&fsmData->tapriv, context);
    cfg->stats.unifiRxDataRate = 0;

    fsm_next_state(context, FSMSTATE_connected);
}

/**
 * @brief
 *   Disconnected ind when in connecting state
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void connecting_disconnected(FsmContext* context, const CoexDisconnectedInd_Evt* ind)
{
    sme_trace_entry((TR_COEX, "connecting_disconnected()"));

    /* Unpause the scan Manager */
    scan_pause_state_change(context, FALSE);

    fsm_next_state(context, FSMSTATE_disconnected);

    /* Update BT Channel AFTER state change */
    update_bt_channel_avoidance(context);
}

/**
 * @brief
 *   Connecting ind when in disconnected state
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void connected_connecting(FsmContext* context, const CoexConnectingInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_COEX, "connected_connecting()"));

    cfg->coexInfo.hasTrafficData = FALSE;
    cfg->coexInfo.currentBlackoutDurationUs = 0;
    cfg->coexInfo.currentBlackoutPeriodUs = 0;
    fsmData->eapolDetected = FALSE;

    if (fsmData->connectTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->connectTimerId);
        fsmData->connectTimerId.uniqueid = 0;
    }
    if (fsmData->taTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->taTimerId);
        fsmData->taTimerId.uniqueid = 0;
    }

    /* Pause the scan Manager */
    scan_pause_state_change(context, TRUE);

    /* Suspend Power Saving */
    power_state_change(context, TRUE);

    /* Remove any installed packet filter */
    update_packet_filter(context);

    fsm_next_state(context, FSMSTATE_connecting);

    /* Update BT Channel AFTER state change */
    update_bt_channel_avoidance(context);
}

/**
 * @brief
 *   Disconnected ind when in connecting state
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void connected_disconnected(FsmContext* context, const CoexDisconnectedInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_COEX, "connected_disconnected()"));

    cfg->coexInfo.hasTrafficData = FALSE;
    cfg->coexInfo.currentBlackoutDurationUs = 0;
    cfg->coexInfo.currentBlackoutPeriodUs = 0;

    /* Unpause the scan Manager */
    scan_pause_state_change(context, FALSE);

    if (fsmData->connectTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->connectTimerId);
        fsmData->connectTimerId.uniqueid = 0;
    }
    if (fsmData->taTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->taTimerId);
        fsmData->taTimerId.uniqueid = 0;
    }

    /* Remove any installed packet filter */
    update_packet_filter(context);

    fsm_next_state(context, FSMSTATE_disconnected);

    /* Update BT Channel AFTER state change */
    update_bt_channel_avoidance(context);
}

/**
 * @brief
 *   Connection Delay Timer when in connecting state
 *
 * @param[in]    context   : FSM context
 * @param[in]    timer     : timer
 *
 * @return
 *   void
 */
static void connected_delay_timeout(FsmContext* context, const CoexConnectionDelayTimer_Evt* timer)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_COEX, "connected_delay_timeout()"));

    fsmData->connectTimerId.uniqueid = 0;

    /* Unpause the scan Manager */
    scan_pause_state_change(context, FALSE);

    /* Update Coex / Power saving */
    update_system_configuration(context);

    /* Resume Power Saving */
    power_state_change(context, FALSE);

    /* Install the packet filter if available */
    update_packet_filter(context);
}

/**
 * @brief
 *   BT Init request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void handle_bt_active(FsmContext* context, const UnifiBtBtActiveInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_COEX, "handle_bt_active()"));

    CsrMemSet(&fsmData->btlinkData, 0 , sizeof(fsmData->btlinkData));
    cfg->coexInfo.hasBtDevice = TRUE;
    fsmData->currentChannelAvoid = 0;

    /* Enable Coex lines in the Firmware */
    if (cfg->coexConfig.coexEnableSchemeManagement && cfg->unifiCoexScheme != unifi_CoexSchemeDisabled)
    {
        cfg->coexInfo.currentCoexScheme = cfg->unifiCoexScheme;
        mib_util_send_set_int(context, unifiCoexScheme, cfg->unifiCoexScheme, 0, 0);
    }

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        update_bt_channel_avoidance(context);

        /* If connected then Update the Coex mode to Wifi + ACL BT */
        update_system_configuration(context);
    }
}

/**
 * @brief
 *   BT Final request
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void handle_bt_inactive(FsmContext* context, const UnifiBtBtInactiveInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_COEX, "handle_bt_inactive()"));

    cfg->coexInfo.hasBtDevice = FALSE;
    fsmData->currentChannelAvoid = 0;

    /* Disable Coex lines in the Firmware */
    if (cfg->coexConfig.coexEnableSchemeManagement && cfg->unifiCoexScheme != unifi_CoexSchemeDisabled)
    {
        cfg->coexInfo.currentCoexScheme = unifi_CoexSchemeDisabled;
        mib_util_send_set_int(context, unifiCoexScheme, unifi_CoexSchemeDisabled, 0, 0);
    }

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        /* If connected then ::
         * 1) Update the Coex mode to Wifi Only
         */
        update_system_configuration(context);
    }
}

/**
 * @brief
 *   BT acl start
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void handle_bt_acl_start(FsmContext* context, const UnifiBtAclStartInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    CsrUint16 i;
    CsrBool duplicate = FALSE;
    sme_trace_entry((TR_COEX, "handle_bt_acl_start(%s, %s, %s)",
                               trace_unifi_MACAddress(ind->aclHandle, getMacAddressBuffer(context)),
                               trace_unifi_BtDeviceRole(ind->role),
                               trace_unifi_BtDeviceMode(ind->mode) ));

    /* Check for duplicate Data */
    for (i = 0; i < MAX_BT_LINK_DATA; ++i)
    {
        if (fsmData->btlinkData.linkData[i].hasLinkData &&
            CsrMemCmp(fsmData->btlinkData.linkData[i].aclHandle.data, ind->aclHandle.data, 6) == 0)
        {
            sme_trace_error((TR_COEX, "handle_bt_acl_start() Duplicate ACL link start requst for %s", trace_unifi_MACAddress(ind->aclHandle, getMacAddressBuffer(context))));
            duplicate = TRUE;
            fsmData->btlinkData.linkData[i].mode = ind->mode;
            fsmData->btlinkData.linkData[i].role = ind->role;
            break;
        }
    }

    if (!duplicate)
    {
        for (i = 0; i < MAX_BT_LINK_DATA; ++i)
        {
            if (!fsmData->btlinkData.linkData[i].hasLinkData)
            {
                fsmData->btlinkData.hasLinkData++;
                fsmData->btlinkData.linkData[i].hasLinkData = TRUE;
                fsmData->btlinkData.linkData[i].aclHandle = ind->aclHandle;
                fsmData->btlinkData.linkData[i].mode = ind->mode;
                fsmData->btlinkData.linkData[i].role = ind->role;
                fsmData->btlinkData.linkData[i].logicalChannelTypeMask = unifi_BtDeviceLogicalChannelNone;
                fsmData->btlinkData.linkData[i].numberOfGuaranteedLogicalChannels = 0;
                CsrMemSet(fsmData->btlinkData.linkData[i].scoData, 0x00 , sizeof(fsmData->btlinkData.linkData[i].scoData));
                break;
            }
        }
    }
    if (i == MAX_BT_LINK_DATA)
    {
        sme_trace_error((TR_COEX, "handle_bt_acl_start() Data discarded as MAX_BT_LINK_DATA limit reached "));
        return;
    }

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        update_system_configuration(context);
    }
#ifdef CSR_AMP_ENABLE
    else
    {
        /* Forward the event only if SME is in disconnected state to avoid clashes. Need to think how to handle stuff when both
        * AMP and SME is in connected state. */
        fsm_forward_event(context,getSmeContext(context)->palCoexFsmInstance,(FsmEvent *)ind);
    }
#endif

}

/**
 * @brief
 *   BT acl update
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void handle_bt_acl_change(FsmContext* context, const UnifiBtAclChangeInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    CsrUint16 i;
    sme_trace_entry((TR_COEX, "handle_bt_acl_change(%s, %s, %s)",
                               trace_unifi_MACAddress(ind->aclHandle, getMacAddressBuffer(context)),
                               trace_unifi_BtDeviceRole(ind->role),
                               trace_unifi_BtDeviceMode(ind->mode) ));
    for (i = 0; i < MAX_BT_LINK_DATA; ++i)
    {
        if (fsmData->btlinkData.linkData[i].hasLinkData &&
            CsrMemCmp(fsmData->btlinkData.linkData[i].aclHandle.data, ind->aclHandle.data, 6) == 0)
        {
            fsmData->btlinkData.linkData[i].mode = ind->mode;
            fsmData->btlinkData.linkData[i].role = ind->role;
            break;
        }
    }

    if (i == MAX_BT_LINK_DATA)
    {
        sme_trace_error((TR_COEX, "handle_bt_acl_change() Data discarded as MAX_BT_LINK_DATA limit reached "));
        return;
    }

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        update_system_configuration(context);
    }
#ifdef CSR_AMP_ENABLE
    else
    {
        /* Forward the event only if SME is in disconnected state to avoid clashes. Need to think how to handle stuff when both
        * AMP and SME is in connected state. */
        fsm_forward_event(context,getSmeContext(context)->palCoexFsmInstance,(FsmEvent *)ind);
    }
#endif

}

/**
 * @brief
 *   BT acl channel types update
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void handle_bt_acl_channel_types(FsmContext* context, const UnifiBtAclChannelTypesInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    CsrUint16 i;
    sme_trace_entry((TR_COEX, "handle_bt_acl_channel_types(%s, 0x%.4X, %d)",
                               trace_unifi_MACAddress(ind->aclHandle, getMacAddressBuffer(context)),
                               ind->logicalChannelTypeMask,
                               ind->numberOfGuaranteedLogicalChannels ));
    for (i = 0; i < MAX_BT_LINK_DATA; ++i)
    {
        if (fsmData->btlinkData.linkData[i].hasLinkData &&
            CsrMemCmp(fsmData->btlinkData.linkData[i].aclHandle.data, ind->aclHandle.data, 6) == 0)
        {
            fsmData->btlinkData.linkData[i].logicalChannelTypeMask = ind->logicalChannelTypeMask;
            fsmData->btlinkData.linkData[i].numberOfGuaranteedLogicalChannels = ind->numberOfGuaranteedLogicalChannels;
            break;
        }
    }

    if (i == MAX_BT_LINK_DATA)
    {
        sme_trace_error((TR_COEX, "handle_bt_acl_channel_types() Data discarded as MAX_BT_LINK_DATA limit reached "));
        return;
    }

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        update_system_configuration(context);
    }
#ifdef CSR_AMP_ENABLE
    else
    {
        /* Forward the event only if SME is in disconnected state to avoid clashes. Need to think how to handle stuff when both
        * AMP and SME is in connected state. */
        fsm_forward_event(context,getSmeContext(context)->palCoexFsmInstance,(FsmEvent *)ind);
    }
#endif
}

/**
 * @brief
 *   BT acl stop
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void handle_bt_acl_stop(FsmContext* context, const UnifiBtAclStopInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    CsrUint16 i;

    sme_trace_entry((TR_COEX, "handle_bt_acl_stop(%s)", trace_unifi_MACAddress(ind->aclHandle, getMacAddressBuffer(context))));

    for (i = 0; i < MAX_BT_LINK_DATA; ++i)
    {
        if (fsmData->btlinkData.linkData[i].hasLinkData &&
            CsrMemCmp(fsmData->btlinkData.linkData[i].aclHandle.data, ind->aclHandle.data, 6) == 0)
        {
            fsmData->btlinkData.hasLinkData--;
            fsmData->btlinkData.linkData[i].hasLinkData = FALSE;
            break;
        }
    }

    if (i == MAX_BT_LINK_DATA)
    {
        sme_trace_error((TR_COEX, "handle_bt_acl_stop() Start data discarded as MAX_BT_LINK_DATA limit reached "));
        return;
    }

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        update_system_configuration(context);
    }
#ifdef CSR_AMP_ENABLE
    else
    {
        /* Forward the event only if SME is in disconnected state to avoid clashes. Need to think how to handle stuff when both
        * AMP and SME is in connected state. */
        fsm_forward_event(context,getSmeContext(context)->palCoexFsmInstance,(FsmEvent *)ind);
    }
#endif

}

/**
 * @brief
 *   BT sync start request
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void handle_bt_sco_start(FsmContext* context, const UnifiBtScoStartInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint16 i, j;
    CsrBool duplicate = FALSE;
    sme_trace_entry((TR_COEX, "handle_bt_sco_start(%s, 0x%.4X, %d, %d, %d)",
                               trace_unifi_MACAddress(ind->aclHandle, getMacAddressBuffer(context)),
                               ind->scoHandle,
                               ind->periodSlots,
                               ind->durationMinSlots,
                               ind->durationMaxSlots ));

    /* Ignore unless a BT Init has been recieved */
    if (!cfg->coexInfo.hasBtDevice) return;

    for (i = 0; i < MAX_BT_LINK_DATA; ++i)
    {
        if (fsmData->btlinkData.linkData[i].hasLinkData &&
            CsrMemCmp(fsmData->btlinkData.linkData[i].aclHandle.data, ind->aclHandle.data, 6) == 0)
        {
            for (j = 0; j < MAX_BT_SCO_DATA; ++j)
            {
                if (!fsmData->btlinkData.linkData[i].scoData[j].hasScoData &&
                    fsmData->btlinkData.linkData[i].scoData[j].scoHandle == ind->scoHandle)
                {
                    sme_trace_error((TR_COEX, "handle_bt_sco_start() Duplicate SCO link start requst for %d", ind->scoHandle));
                    duplicate = TRUE;
                    fsmData->btlinkData.linkData[i].scoData[j].period = ind->periodSlots;
                    fsmData->btlinkData.linkData[i].scoData[j].durationMin = ind->durationMinSlots;
                    fsmData->btlinkData.linkData[i].scoData[j].durationMax = ind->durationMaxSlots;
                    break;
                }
            }
        }
    }

    /* Replace the data if this handle is in use */
    if (!duplicate)
    {
        for (i = 0; i < MAX_BT_LINK_DATA; ++i)
        {
            if (fsmData->btlinkData.linkData[i].hasLinkData &&
                CsrMemCmp(fsmData->btlinkData.linkData[i].aclHandle.data, ind->aclHandle.data, 6) == 0)
            {
                for (j = 0; j < MAX_BT_SCO_DATA; ++j)
                {
                    if (!fsmData->btlinkData.linkData[i].scoData[j].hasScoData)
                    {
                        fsmData->btlinkData.linkData[i].scoData[j].hasScoData = TRUE;
                        fsmData->btlinkData.linkData[i].scoData[j].scoHandle = ind->scoHandle;
                        fsmData->btlinkData.linkData[i].scoData[j].period = ind->periodSlots;
                        fsmData->btlinkData.linkData[i].scoData[j].durationMin = ind->durationMinSlots;
                        fsmData->btlinkData.linkData[i].scoData[j].durationMax = ind->durationMaxSlots;
                        sme_trace_info((TR_COEX, "handle_bt_sco_start()  duration: 0x%x ., period: 0x%x ",
                            fsmData->btlinkData.linkData[i].scoData[j].durationMin,
                            fsmData->btlinkData.linkData[i].scoData[j].period));

                        break;
                    }
                }
                if (j == MAX_BT_SCO_DATA)
                {
                    sme_trace_error((TR_COEX, "handle_bt_sco_start() Start data discarded as MAX_BT_SCO_DATA limit reached "));
                    return;
                }
                break;
            }
        }
    }
    if (i == MAX_BT_LINK_DATA)
    {
        sme_trace_error((TR_COEX, "handle_bt_sco_start() Start data discarded as MAX_BT_LINK_DATA limit reached "));
        return;
    }

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        /* If connected then Update the Coex mode to Wifi + SCO/eSCO BT*/
        update_system_configuration(context);
    }
#ifdef CSR_AMP_ENABLE
    else
    {
        /* Forward the event only if SME is in disconnected state to avoid clashes. Need to think how to handle stuff when both
        * AMP and SME is in connected state. */
        fsm_forward_event(context,getSmeContext(context)->palCoexFsmInstance,(FsmEvent *)ind);
    }
#endif

}

/**
 * @brief
 *   BT sync start request
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void handle_bt_sco_change(FsmContext* context, const UnifiBtScoChangeInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint16 i, j;
    sme_trace_entry((TR_COEX, "handle_bt_sco_change(%s, 0x%.4X, %d, %d, %d)",
                               trace_unifi_MACAddress(ind->aclHandle, getMacAddressBuffer(context)),
                               ind->scoHandle,
                               ind->periodSlots,
                               ind->durationMinSlots,
                               ind->durationMaxSlots ));

    /* Ignore unless a BT Init has been recieved */
    if (!cfg->coexInfo.hasBtDevice) return;

    for (i = 0; i < MAX_BT_LINK_DATA; ++i)
    {
        if (fsmData->btlinkData.linkData[i].hasLinkData &&
            CsrMemCmp(fsmData->btlinkData.linkData[i].aclHandle.data, ind->aclHandle.data, 6) == 0)
        {
            for (j = 0; j < MAX_BT_SCO_DATA; ++j)
            {
                if (fsmData->btlinkData.linkData[i].scoData[j].hasScoData &&
                    fsmData->btlinkData.linkData[i].scoData[j].scoHandle == ind->scoHandle)
                {
                    fsmData->btlinkData.linkData[i].scoData[j].period = ind->periodSlots;
                    fsmData->btlinkData.linkData[i].scoData[j].durationMin = ind->durationMinSlots;
                    fsmData->btlinkData.linkData[i].scoData[j].durationMax = ind->durationMaxSlots;
                    break;
                }
            }
            if (j == MAX_BT_SCO_DATA)
            {
                sme_trace_error((TR_COEX, "handle_bt_sco_change() Start data discarded as MAX_BT_SCO_DATA limit reached "));
                return;
            }
            break;
        }
    }
    if (i == MAX_BT_LINK_DATA)
    {
        sme_trace_error((TR_COEX, "handle_bt_sco_change() Change data discarded as MAX_BT_LINK_DATA limit reached "));
        return;
    }

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        /* If connected then Update the Coex mode to Wifi + SCO/eSCO BT*/
        update_system_configuration(context);
    }
#ifdef CSR_AMP_ENABLE
    else
    {
        /* Forward the event only if SME is in disconnected state to avoid clashes. Need to think how to handle stuff when both
        * AMP and SME is in connected state. */
        fsm_forward_event(context,getSmeContext(context)->palCoexFsmInstance,(FsmEvent *)ind);
    }
#endif

}

/**
 * @brief
 *   BT sync start request
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void handle_bt_sco_stop(FsmContext* context, const UnifiBtScoStopInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint16 i, j;
    sme_trace_entry((TR_COEX, "handle_bt_sco_stop(%s, 0x%.4X)",
                               trace_unifi_MACAddress(ind->aclHandle, getMacAddressBuffer(context)),
                               ind->scoHandle ));
    /* Ignore unless a BT Init has been recieved */
    if (!cfg->coexInfo.hasBtDevice) return;

    for (i = 0; i < MAX_BT_LINK_DATA; ++i)
    {
        if (fsmData->btlinkData.linkData[i].hasLinkData &&
            CsrMemCmp(fsmData->btlinkData.linkData[i].aclHandle.data, ind->aclHandle.data, 6) == 0)
        {
            for (j = 0; j < MAX_BT_SCO_DATA; ++j)
            {
                if(fsmData->btlinkData.linkData[i].scoData[j].hasScoData &&
                   fsmData->btlinkData.linkData[i].scoData[j].scoHandle == ind->scoHandle)
                {
                    fsmData->btlinkData.linkData[i].scoData[j].hasScoData = FALSE;
                    fsmData->btlinkData.linkData[i].scoData[j].scoHandle = 0;
                    break;
                }
            }
            if (j == MAX_BT_SCO_DATA)
            {
                sme_trace_error((TR_COEX, "handle_bt_sco_stop() Start data discarded as MAX_BT_SCO_DATA limit reached "));
                return;
            }
            break;
        }
    }
    if (i == MAX_BT_LINK_DATA)
    {
        sme_trace_warn((TR_COEX, "handle_bt_sco_stop() Stop data discarded as MAX_BT_LINK_DATA limit reached "));
        return;
    }

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        /* If connected then update the Coex mode to Wifi + ACL BT*/
        update_system_configuration(context);
    }
#ifdef CSR_AMP_ENABLE
    else
    {
        /* Forward the event only if SME is in disconnected state to avoid clashes. Need to think how to handle stuff when both
        * AMP and SME is in connected state. */
        fsm_forward_event(context,getSmeContext(context)->palCoexFsmInstance,(FsmEvent *)ind);
    }
#endif

}

/**
 * @brief
 *   Wifi traffic message when in connected state
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void connected_wifi_traffic(FsmContext* context, const CoexTrafficClassificationInd_Evt* ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    CsrBool oldHasTrafficData = cfg->coexInfo.hasTrafficData;
    unifi_TrafficType oldTrafficType = cfg->coexInfo.currentTrafficType;
    sme_trace_entry((TR_COEX, "connected_wifi_traffic(%s, period:%d)", trace_unifi_TrafficType(ind->trafficType), ind->period));

    if (cfg->coexInfo.hasTrafficData && cfg->coexInfo.currentTrafficType == ind->trafficType && cfg->coexInfo.currentPeriodMs == ind->period)
    {
        /* No Change */
        return;
    }

    cfg->coexInfo.hasTrafficData = TRUE;
    cfg->coexInfo.currentTrafficType = ind->trafficType;
    cfg->coexInfo.currentPeriodMs = ind->period;

    if (ind->trafficType == unifi_TrafficPeriodic && ind->period == 0)
    {
        sme_trace_error((TR_COEX, "connected_wifi_traffic() Period of 0 ms is not allowed with unifi_TrafficPeriodic"));
        cfg->coexInfo.currentTrafficType = unifi_TrafficContinuous;
    }

    /* Update Powersave settings */
    update_system_configuration(context);

    /* Update the Packet Filter when moving to or from occasional traffic */
    if ((!oldHasTrafficData && ind->trafficType == unifi_TrafficOccasional) ||
        (oldHasTrafficData && oldTrafficType != ind->trafficType &&
         (oldTrafficType == unifi_TrafficOccasional || ind->trafficType == unifi_TrafficOccasional))
       )
    {
        update_packet_filter(context);
    }
}

/**
 * @brief
 *   Wifi traffic sample message when in connected state
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void connected_wifi_traffic_sample(FsmContext* context, const UnifiSysTrafficSampleInd_Evt* ind)
{
    CsrUint32 nextTaTimeout = 0;
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    /*sme_trace_entry((TR_COEX, "connected_wifi_traffic_sample()"));*/

    if (fsmData->taTimerId.uniqueid)
    {
        fsm_remove_timer(context, fsmData->taTimerId);
        fsmData->taTimerId.uniqueid = 0;
    }

    unifi_ta_data(&fsmData->tapriv, &ind->stats);
    nextTaTimeout = unifi_ta_run(&fsmData->tapriv);
    if (ind->stats.rxMeanRate)
    {
        cfg->stats.unifiRxDataRate = (CsrUint8)ind->stats.rxMeanRate;
    }

    if (nextTaTimeout)
    {
        /*sme_trace_debug((TR_COEX, "connected_wifi_traffic_sample() Set timer for %d ms", nextTaTimeout));*/
        send_coex_traffic_tick_timer(context, fsmData->taTimerId, nextTaTimeout, 100);
    }
}

/**
 * @brief
 *   Wifi traffic sample message when in connected state
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void connected_wifi_traffic_packet(FsmContext* context, const UnifiSysTrafficProtocolInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_COEX, "connected_wifi_traffic_packet(%s, %s, %s)", trace_unifi_TrafficPacketType(ind->packetType),
                                                                           trace_unifi_ProtocolDirection(ind->direction),
                                                                           trace_unifi_MACAddress(ind->srcAddress, getMacAddressBuffer(context))));

    if (ind->packetType == unifi_TrafficPacketEapol)
    {
        fsmData->eapolDetected = TRUE;
        fsmData->waitingForKeys = TRUE;

        sme_trace_debug((TR_COEX, "connected_wifi_traffic_packet(unifi_TrafficPacketEapol) waitingForKeys = %s, ipConnected = %s",
                                  fsmData->waitingForKeys?"TRUE":"FALSE",
                                  fsmData->ipConnected?"TRUE":"FALSE"));
    }


    if (ind->packetType == unifi_TrafficPacketEapol || ind->packetType == unifi_TrafficPacketDhcp)
    {
        /* If Timer running then Reset it */
        if (fsmData->connectTimerId.uniqueid != 0)
        {
            sme_trace_debug((TR_COEX, "connected_wifi_traffic_packet() resetting timer"));
            fsm_remove_timer(context, fsmData->connectTimerId);
        }
        else
        {
            /* Pause the scan Manager */
            scan_pause_state_change(context, TRUE);

            /* Suspend Power Saving */
            /* This seems to be the correct action Even with a Sco/eSco connection */
            power_state_change(context, TRUE);
        }

        send_coex_connection_delay_timer(context, fsmData->connectTimerId, POST_CONNECT_POWER_SCAN_DELAY_MS, POST_CONNECT_POWER_SCAN_VARY_MS);
    }
    else if (ind->packetType == unifi_TrafficPacketDhcpAck && fsmData->connectTimerId.uniqueid != 0 && !fsmData->waitingForKeys)
    {
        /* If Timer running then Reset it */
        fsm_remove_timer(context, fsmData->connectTimerId);
        fsmData->connectTimerId.uniqueid = 0;

        /* Unpause the scan Manager */
        scan_pause_state_change(context, FALSE);

        /* Unsuspend Power Saving */
        power_state_change(context, FALSE);
    }
}

/**
 * @brief
 *   traffic analysis tick timer has fired
 *
 * @param[in]    context   : FSM context
 * @param[in]    timer     : timer
 *
 * @return
 *   void
 */
static void connected_traffic_tick(FsmContext* context, const CoexTrafficTickTimer_Evt* timer)
{
    CsrUint32 nextTaTimeout = 0;
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_COEX, "connected_traffic_tick()"));

    fsmData->taTimerId.uniqueid = 0;
    nextTaTimeout = unifi_ta_run(&fsmData->tapriv);
    if (nextTaTimeout)
    {
        sme_trace_debug((TR_COEX, "connected_traffic_tick() Set timer for %d ms", nextTaTimeout));
        send_coex_traffic_tick_timer(context, fsmData->taTimerId, nextTaTimeout, 100);
    }
}

/**
 * @brief
 *   Action on IpAddress change
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void save_ip_address_status(FsmContext* context, const UnifiSysIpConfiguredInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_COEX,  "ip_address_status_change(%s)", ind->ipConfigured?"TRUE":"FALSE"));
    fsmData->ipConnected = ind->ipConfigured;
}

/**
 * @brief
 *   Save the packet filter info
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void save_packet_filter(FsmContext* context, const UnifiMgtPacketFilterSetReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    unifi_Status status = unifi_Restricted;

    sme_trace_entry((TR_COEX, "save_packet_filter()"));

    if (!isAccessRestricted(context, req->appHandle))
    {
        status = unifi_Success;

        /* aa.bb.cc.dd becomes dd.cc.bb.aa. */
        fsmData->packetFilterUpdated = TRUE;
        fsmData->packetFilterAddress  = (req->arpFilterAddress.a[0] << 24);
        fsmData->packetFilterAddress += (req->arpFilterAddress.a[1] << 16);
        fsmData->packetFilterAddress += (req->arpFilterAddress.a[2] <<  8);
        fsmData->packetFilterAddress += (req->arpFilterAddress.a[3]      );

        sme_trace_entry((TR_COEX, "save_packet_filter(): IP %d.%d.%d.%d",
                         *((CsrUint8*)(&fsmData->packetFilterAddress)),
                         *(((CsrUint8*)(&fsmData->packetFilterAddress)) + 1),
                         *(((CsrUint8*)(&fsmData->packetFilterAddress)) + 2),
                         *(((CsrUint8*)(&fsmData->packetFilterAddress)) + 3)));

        fsmData->packetFilterMode = req->mode;

        if (fsmData->packetFilter.dataLength)
        {
            pld_release(getPldContext(context), fsmData->packetFilter.slotNumber);
            fsmData->packetFilter.dataLength = 0;
        }

        if(req->filterLength > 0)
        {
            fsmData->packetFilter.dataLength = req->filterLength;
            pld_store(getPldContext(context), req->filter, req->filterLength, &fsmData->packetFilter.slotNumber);
            CsrPfree(req->filter);
        }
    }

    call_unifi_mgt_packet_filter_set_cfm(context, req->appHandle, status);
}

/**
 * @brief
 *   Action on custom settings change
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void handle_config_update(FsmContext* context, const CoexConfigUpdateInd_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_COEX,  "handle_config_update(coexEnable:%s, coexAfhChannelEnable:%s, coexAdvancedEnable:%s, coexDirection:%s, coexPeriodicWakeHost:%s)",
                               cfg->coexConfig.coexEnable?"TRUE":"FALSE",
                               cfg->coexConfig.coexAfhChannelEnable?"TRUE":"FALSE",
                               cfg->coexConfig.coexAdvancedEnable?"TRUE":"FALSE",
                               trace_unifi_CoexDirection(cfg->coexConfig.coexDirection),
                               cfg->coexConfig.coexPeriodicWakeHost?"TRUE":"FALSE"));

    sme_trace_entry((TR_COEX,  "handle_config_update(coexTrafficBurstyLatencyMs:%d, coexTrafficContinuousLatencyMs:%d)",
                               cfg->coexConfig.coexTrafficBurstyLatencyMs,
                               cfg->coexConfig.coexTrafficContinuousLatencyMs));

    /* Enabling Coex so send init */
    if (cfg->coexConfig.coexEnable && !req->oldEnabledValue)
    {
        call_unifi_bt_wifi_active_req(context);
    }

    if (!cfg->coexConfig.coexEnable && cfg->coexInfo.hasBtDevice)
    {
        cfg->coexInfo.hasBtDevice = FALSE;
        fsmData->currentChannelAvoid = 0;
        call_unifi_bt_wifi_inactive_req(context);
    }

    /* Config Update so reset the unifiCoexScheme scheme in the mib */
    if (!cfg->coexConfig.coexEnableSchemeManagement)
    {
        mib_util_send_set_int(context, unifiCoexScheme, cfg->unifiCoexScheme, 0, 0);
    }
    else if (cfg->coexInfo.hasBtDevice && cfg->unifiCoexScheme != unifi_CoexSchemeDisabled)
    {
        cfg->coexInfo.currentCoexScheme = cfg->unifiCoexScheme;
        mib_util_send_set_int(context, unifiCoexScheme, cfg->unifiCoexScheme, 0, 0);
    }
    else
    {
        cfg->coexInfo.currentCoexScheme = unifi_CoexSchemeDisabled;
        mib_util_send_set_int(context, unifiCoexScheme, unifi_CoexSchemeDisabled, 0, 0);
    }


    /* Update BT Channel AFTER state change */
    update_bt_channel_avoidance(context);

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        /* If connected then update the Coex mode No Coex */
        update_system_configuration(context);
    }
}

/**
 * @brief
 *   Action on IpAddress change
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void connected_ip_address_status(FsmContext* context, const UnifiSysIpConfiguredInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_COEX,  "connected_ip_address_status(%s)", ind->ipConfigured?"TRUE":"FALSE"));

    sme_trace_debug((TR_COEX, "connected_ip_address_status() waitingForKeys = %s, ipConnected = %s",
                              fsmData->waitingForKeys?"TRUE":"FALSE",
                              fsmData->ipConnected?"TRUE":"FALSE"));

    /* Check if the IP Address is configured and the connection timer is running */
    if ( ind->ipConfigured && fsmData->ipConnected != ind->ipConfigured && !fsmData->waitingForKeys)
    {
        fsmData->ipConnected = ind->ipConfigured;
        /* If connection Timer is running ::
         * 1) stop it
         * 2) unpause scan manager
         * 3) Update power saving coex settings
         */
        if (fsmData->connectTimerId.uniqueid != 0)
        {
            fsm_remove_timer(context, fsmData->connectTimerId);
            fsmData->connectTimerId.uniqueid = 0;

            /* Unpause the scan Manager */
            scan_pause_state_change(context, FALSE);

            /* Update Coex / Power settings */
            update_system_configuration(context);

            /* Resume Power Saving */
            power_state_change(context, FALSE);

            /* Install the packet filter if available */
            update_packet_filter(context);
        }

        return;
    }
    fsmData->ipConnected = ind->ipConfigured;
}

/**
 * @brief
 *   Action on packetfilter change
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void connected_packet_filter(FsmContext* context, const UnifiMgtPacketFilterSetReq_Evt* req)
{
    sme_trace_entry((TR_COEX, "connected_packet_filter()"));

    /* Save the packet filter data */
    save_packet_filter(context, req);

    update_packet_filter(context);
}

/**
 * @brief
 *   User power mode changed
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void handle_user_power_mode(FsmContext* context, const CoexPowerUpdateInd_Evt* ind)
{
    sme_trace_entry((TR_COEX,  "handle_user_power_mode()"));

    if (fsm_current_state(context) == FSMSTATE_connected)
    {
        /* If connected then update the Coex mode No Coex */
        update_system_configuration(context);

        power_send_sys_configure_power_mode(context, FALSE);
    }
}

static void handle_mlme_add_periodic_cfm(FsmContext* context, const MlmeAddPeriodicCfm_Evt* cfm)
{
    sme_trace_entry((TR_COEX,  "handle_mlme_add_periodic_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    if (cfm->resultCode != ResultCode_Success)
    {
        sme_trace_crit((TR_COEX,  "handle_mlme_add_periodic_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    }

}

static void handle_mlme_add_blackout_cfm(FsmContext* context, const MlmeAddBlackoutCfm_Evt* cfm)
{
    sme_trace_entry((TR_COEX,  "handle_mlme_add_blackout_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    if (cfm->resultCode != ResultCode_Success)
    {
        sme_trace_crit((TR_COEX,  "handle_mlme_add_blackout_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    }

}




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
static void coex_fsm_dump(FsmContext* context, const CsrUint16 id)
{
    FsmData* fsmData = fsm_get_params_by_id(context, id, FsmData);
    SmeConfigData* cfg = get_sme_config(context);

    require(TR_COEX, id < context->maxProcesses);
    require(TR_COEX, context->instanceArray[id].fsmInfo->processId == COEX_PROCESS);
    require(TR_COEX, context->instanceArray[id].state != FSM_TERMINATE);

    if (context->instanceArray[id].state == FSMSTATE_stopped)
    {
        return;
    }

    sme_trace_crit((TR_FSM_DUMP, "   : coexEnabled                       : %s",     cfg->coexConfig.coexEnable?"True":"False"));
    sme_trace_crit((TR_FSM_DUMP, "   : coexAfhChannelEnabled             : %s",     cfg->coexConfig.coexAfhChannelEnable?"True":"False"));
    sme_trace_crit((TR_FSM_DUMP, "   : coexAdvancedEnabled               : %s",     cfg->coexConfig.coexAdvancedEnable?"True":"False"));
    sme_trace_crit((TR_FSM_DUMP, "   : coexPeriodicWakeHost              : %s",     cfg->coexConfig.coexPeriodicWakeHost?"True":"False"));
    sme_trace_crit((TR_FSM_DUMP, "   : coexTrafficBurstyLatencyMs        : %d",     cfg->coexConfig.coexTrafficBurstyLatencyMs));
    sme_trace_crit((TR_FSM_DUMP, "   : coexTrafficContinuousLatencyMs    : %d",     cfg->coexConfig.coexTrafficContinuousLatencyMs));
    sme_trace_crit((TR_FSM_DUMP, "   : coexDirection                     : %s",     trace_unifi_CoexDirection(cfg->coexConfig.coexDirection)));

    sme_trace_crit((TR_FSM_DUMP, "   : scanManagerSuspended              : %s",     fsmData->scanManagerSuspended?"True":"False"));
    sme_trace_crit((TR_FSM_DUMP, "   : ipConnected                       : %s",     fsmData->ipConnected?"True":"False"));
    sme_trace_crit((TR_FSM_DUMP, "   : connectTimerId running?           : %s",     fsmData->connectTimerId.uniqueid != 0?"True":"False"));

    sme_trace_crit((TR_FSM_DUMP, "   : currentPowerSave                  : %s",     trace_unifi_PowerSaveLevel(fsmData->currentPowerSave)));
    sme_trace_crit((TR_FSM_DUMP, "   : currentAddPeriodicId              : %d",     fsmData->currentAddPeriodicId));

    sme_trace_crit((TR_FSM_DUMP, "   : BT   : hasBtDevice                : %s",     cfg->coexInfo.hasBtDevice?"True":"False"));
    if (cfg->coexInfo.hasBtDevice)
    {
        CsrUint16 i, j;
        sme_trace_crit((TR_FSM_DUMP, "   : BT   : hasLinkData                : %d",     fsmData->btlinkData.hasLinkData));
        for (i = 0; i < MAX_BT_LINK_DATA; ++i)
        {
            if (fsmData->btlinkData.linkData[i].hasLinkData)
            {
                sme_trace_crit((TR_FSM_DUMP, "   : BT   : [%d]aclHandle                          : %s",     i, trace_unifi_MACAddress(fsmData->btlinkData.linkData[i].aclHandle, getMacAddressBuffer(context))));
                sme_trace_crit((TR_FSM_DUMP, "   : BT   : [%d]role                               : %s",     i, trace_unifi_BtDeviceRole(fsmData->btlinkData.linkData[i].role)));
                sme_trace_crit((TR_FSM_DUMP, "   : BT   : [%d]mode                               : %s",     i, trace_unifi_BtDeviceMode(fsmData->btlinkData.linkData[i].mode)));
                sme_trace_crit((TR_FSM_DUMP, "   : BT   : [%d]logicalChannelTypeMask             : 0x%.4X", i, fsmData->btlinkData.linkData[i].logicalChannelTypeMask));
                sme_trace_crit((TR_FSM_DUMP, "   : BT   : [%d]numberOfGuaranteedLogicalChannels  : %d",     i, fsmData->btlinkData.linkData[i].numberOfGuaranteedLogicalChannels));
                for (j = 0; j < MAX_BT_SCO_DATA; ++j)
                {
                    if (fsmData->btlinkData.linkData[i].scoData[j].hasScoData)
                    {
                        sme_trace_crit((TR_FSM_DUMP, "   : BT   : [%d][%d]scoHandle           : 0x%.4X", i, j, fsmData->btlinkData.linkData[i].scoData[j].scoHandle));
                        sme_trace_crit((TR_FSM_DUMP, "   : BT   : [%d][%d]period              : %d",     i, j, fsmData->btlinkData.linkData[i].scoData[j].period));
                        sme_trace_crit((TR_FSM_DUMP, "   : BT   : [%d][%d]durationMin         : %d",     i, j, fsmData->btlinkData.linkData[i].scoData[j].durationMin));
                        sme_trace_crit((TR_FSM_DUMP, "   : BT   : [%d][%d]durationMax         : %d",     i, j, fsmData->btlinkData.linkData[i].scoData[j].durationMax));
                        break;
                    }
                }
            }
        }
    }

    sme_trace_crit((TR_FSM_DUMP, "   : WIFI : hasTrafficData              : %s",     cfg->coexInfo.hasTrafficData?"True":"False"));
    if (cfg->coexInfo.hasTrafficData)
    {
        sme_trace_crit((TR_FSM_DUMP, "   : WIFI : trafficType                : %s",     trace_unifi_TrafficType(cfg->coexInfo.currentTrafficType)));
        sme_trace_crit((TR_FSM_DUMP, "   : WIFI : period                     : %d",     cfg->coexInfo.currentPeriodMs));
    }
}
#endif

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in coex_fsm/coex_fsm.h
 */
PowerManagementMode get_required_coex_power_mode(FsmContext* context)
{
    sme_trace_entry((TR_COEX, "get_required_coex_power_mode()"));

    require(TR_COEX, context->instanceArray[getSmeContext(context)->coexInstance].state != FSM_TERMINATE);

    return (fsm_get_params_by_id(context, getSmeContext(context)->coexInstance, FsmData))->currentPowerSave;
}

/**
 * See description in coex_fsm/coex_fsm.h
 */
CsrBool coex_eapol_detected(FsmContext* context)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->coexInstance, FsmData);
    return fsmData->eapolDetected;
}

/**
 * See description in coex_fsm/coex_fsm.h
 */
CsrBool coex_current_wakeHost(FsmContext* context)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->coexInstance, FsmData);
    SmeConfigData* cfg = get_sme_config(context);

    return fsmData->currentAddPeriodicId == 1 && cfg->coexConfig.coexPeriodicWakeHost;
}

/**
 * See description in coex_fsm/coex_fsm.h
 */
void coex_wpa_keys_installed(FsmContext* context)
{
    FsmData* fsmData;
    FsmInstanceEntry* currentInstance = fsm_save_current_instance_by_id(context, getSmeContext(context)->coexInstance);
    fsmData = FSMDATA;

    sme_trace_entry((TR_COEX, "coex_wpa_keys_installed() waitingForKeys = %s, ipConnected = %s",
                              fsmData->waitingForKeys?"TRUE":"FALSE",
                              fsmData->ipConnected?"TRUE":"FALSE"));


    fsmData->waitingForKeys = FALSE;
    if (fsmData->ipConnected)
    {
        fsm_remove_timer(context, fsmData->connectTimerId);
        fsmData->connectTimerId.uniqueid = 0;

        /* Unpause the scan Manager */
        scan_pause_state_change(context, FALSE);

        /* Update Coex / Power saving */
        update_system_configuration(context);

        /* Resume Power Saving */
        power_state_change(context, FALSE);

        /* Install the packet filter if available */
        update_packet_filter(context);
    }

    fsm_restore_current_instance(context, currentInstance);
}

/**
 *   Get the blackout parameters. This API is used by PAL Coex Manager as well to get the configuration parameters. Hence the reaosn why
 *   the fsmData is retrieved the way it is (should get the right fsmData which ever fsm instance calls it).
 *
 * See description in coex_fsm/coex_fsm.h
 *
 */
CsrBool coex_get_blackout_configuration(FsmContext* context,
                                              CsrBool *wifiOnlyOrAcl,
                                              CsrUint32 *blackoutDurUs,
                                              CsrUint32 *blackoutPrdUs,
                                              BlackoutSource *blackoutSrc)
{
    FsmData *fsmData = fsm_get_params_by_id(context, getSmeContext(context)->coexInstance, FsmData);
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint16 i, j;
    CsrUint8 hasSco = 0;
    CsrUint8 hasA2dp = 0;
    CsrUint8 hasObex = 0;
    CsrUint32 blackoutDurationUs = 0;
    CsrUint32 blackoutPeriodUs = 0;
    BlackoutSource blackoutSource = BlackoutSource_Dot11Local;

    sme_trace_entry((TR_COEX, "coex_get_blackout_configuration()"));

    *wifiOnlyOrAcl=FALSE;

    /* No Coex required */
    if (cfg->connectionInfo.ifIndex == unifi_GHZ_5_0 ||
        !cfg->coexInfo.hasBtDevice ||
        !cfg->coexConfig.coexEnable ||
        !cfg->coexConfig.coexAdvancedEnable ||
        !fsmData->btlinkData.hasLinkData)
    {
        *wifiOnlyOrAcl=TRUE;
        return TRUE;
    }

    /* Check for the type of links running */
    for (i = 0; i < MAX_BT_LINK_DATA; ++i)
    {
        if (fsmData->btlinkData.linkData[i].logicalChannelTypeMask & unifi_BtDeviceLogicalChannelData)
        {
            hasObex++;
            /* Obex Ratio */
            if (!hasA2dp && !hasSco)
            {
                blackoutDurationUs = cfg->coexConfig.coexObexBlackoutDurationMs * 1000;
                blackoutPeriodUs = cfg->coexConfig.coexObexBlackoutPeriodMs * 1000;
            }
        }
        if (fsmData->btlinkData.linkData[i].logicalChannelTypeMask & unifi_BtDeviceLogicalChannelGarrenteed &&
                fsmData->btlinkData.linkData[i].numberOfGuaranteedLogicalChannels != 0 )
        {
            hasA2dp++;
            if (!hasSco)
            {
                blackoutDurationUs = cfg->coexConfig.coexA2dpBlackoutDurationMs * 1000;
                blackoutPeriodUs = cfg->coexConfig.coexA2dpBlackoutPeriodMs * 1000;
            }
        }

        if (fsmData->btlinkData.linkData[i].hasLinkData)
        {
            for (j = 0; j < MAX_BT_SCO_DATA; ++j)
            {
                if (fsmData->btlinkData.linkData[i].scoData[j].hasScoData)
                {
                    if (!hasSco)
                    {
                        blackoutDurationUs = 0;
                        blackoutPeriodUs = 0;
                    }
                    hasSco++;

                    if (cfg->coexConfig.coexDirection == CoexistenceDirection_CoexistenceDot11Input)
                    {
                        blackoutSource = BlackoutSource_OtherRadio;
#if 1
                        blackoutPeriodUs   = fsmData->btlinkData.linkData[i].scoData[j].period * BT_SLOT_MICROSECONDS;
                        blackoutDurationUs = 2 * BT_SLOT_MICROSECONDS;

                        if(fsmData->btlinkData.linkData[i].scoData[j].period != 6 &&
                           fsmData->btlinkData.linkData[i].scoData[j].durationMax > 2)
                        {
                            /* Never allow blackout of more than 4 slots (max 2 retransmission slots) */
                            blackoutDurationUs = 4 * BT_SLOT_MICROSECONDS;
                        }
#else
                        blackoutDurationUs += fsmData->btlinkData.linkData[i].scoData[j].durationMin * BT_SLOT_MICROSECONDS;
                        blackoutPeriodUs = fsmData->btlinkData.linkData[i].scoData[j].period * BT_SLOT_MICROSECONDS;
#endif
                        sme_trace_info((TR_COEX, "coex_get_blackout_configuration(): blackoutPeriod - 0x%x",blackoutPeriodUs));
                    }
                }
            }
        }
    }

    if ((hasSco + hasA2dp + hasObex) > 1)
    {
        sme_trace_error((TR_COEX, "coex_get_blackout_configuration() More than 1 type of link active. This is unsupported."));
        return FALSE;
    }

    if (hasSco || hasA2dp || hasObex)
    {
        *blackoutDurUs = blackoutDurationUs;
        *blackoutPrdUs = blackoutPeriodUs;
        *blackoutSrc = blackoutSource;
    }
    else
    {
        *wifiOnlyOrAcl=TRUE;
    }
    return TRUE;
}


/* FSM DEFINITION **********************************************/

static const FsmEventEntry stoppedTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                            stopped_start ),

    fsm_event_table_entry(UNIFI_BT_BT_ACTIVE_IND_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_BT_INACTIVE_IND_ID,                  fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_ACL_START_IND_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_ACL_CHANGE_IND_ID,                   fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_ACL_CHANNEL_TYPES_IND_ID,            fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_ACL_STOP_IND_ID,                     fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_SCO_START_IND_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_SCO_CHANGE_IND_ID,                   fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_SCO_STOP_IND_ID,                     fsm_ignore_event),

};

static const FsmEventEntry disconnectedTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                             disconnected_stop),
    fsm_event_table_entry(COEX_CONNECTING_IND_ID,                       disconnected_connecting),
    fsm_event_table_entry(COEX_DISCONNECTED_IND_ID,                     fsm_ignore_event),
};

static const FsmEventEntry connectingTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(COEX_CONNECTED_IND_ID,                        connecting_connected),
    fsm_event_table_entry(COEX_DISCONNECTED_IND_ID,                     connecting_disconnected),
};

static const FsmEventEntry connectedTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(COEX_CONNECTING_IND_ID,                       connected_connecting),
    fsm_event_table_entry(COEX_DISCONNECTED_IND_ID,                     connected_disconnected),

    fsm_event_table_entry(COEX_TRAFFIC_CLASSIFICATION_IND_ID,           connected_wifi_traffic),
    fsm_event_table_entry(UNIFI_SYS_IP_CONFIGURED_IND_ID,               connected_ip_address_status),
    fsm_event_table_entry(UNIFI_MGT_PACKET_FILTER_SET_REQ_ID,           connected_packet_filter),

    fsm_event_table_entry(UNIFI_SYS_TRAFFIC_SAMPLE_IND_ID,              connected_wifi_traffic_sample),
    fsm_event_table_entry(UNIFI_SYS_TRAFFIC_PROTOCOL_IND_ID,            connected_wifi_traffic_packet),

    fsm_event_table_entry(COEX_CONNECTION_DELAY_TIMER_ID,               connected_delay_timeout),
    fsm_event_table_entry(COEX_TRAFFIC_TICK_TIMER_ID,                   connected_traffic_tick),
};

static const FsmEventEntry unhandledTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_BT_BT_ACTIVE_IND_ID,                    handle_bt_active),
    fsm_event_table_entry(UNIFI_BT_BT_INACTIVE_IND_ID,                  handle_bt_inactive),
    fsm_event_table_entry(UNIFI_BT_ACL_START_IND_ID,                    handle_bt_acl_start),
    fsm_event_table_entry(UNIFI_BT_ACL_CHANGE_IND_ID,                   handle_bt_acl_change),
    fsm_event_table_entry(UNIFI_BT_ACL_CHANNEL_TYPES_IND_ID,            handle_bt_acl_channel_types),
    fsm_event_table_entry(UNIFI_BT_ACL_STOP_IND_ID,                     handle_bt_acl_stop),
    fsm_event_table_entry(UNIFI_BT_SCO_START_IND_ID,                    handle_bt_sco_start),
    fsm_event_table_entry(UNIFI_BT_SCO_CHANGE_IND_ID,                   handle_bt_sco_change),
    fsm_event_table_entry(UNIFI_BT_SCO_STOP_IND_ID,                     handle_bt_sco_stop),
    fsm_event_table_entry(COEX_TRAFFIC_CLASSIFICATION_IND_ID,           fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_TRAFFIC_SAMPLE_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_TRAFFIC_PROTOCOL_IND_ID,            fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_IP_CONFIGURED_IND_ID,               save_ip_address_status),
    fsm_event_table_entry(UNIFI_MGT_PACKET_FILTER_SET_REQ_ID,           save_packet_filter),

    fsm_event_table_entry(COEX_POWER_UPDATE_IND_ID,                     handle_user_power_mode),
    fsm_event_table_entry(COEX_CONFIG_UPDATE_IND_ID,                    handle_config_update),

    fsm_event_table_entry(SM_PAUSE_CFM_ID,                              fsm_ignore_event),
    fsm_event_table_entry(SM_UNPAUSE_CFM_ID,                            fsm_ignore_event),
    fsm_event_table_entry(POW_RESUME_POWER_SAVING_CFM_ID,               fsm_ignore_event),
    fsm_event_table_entry(POW_SUSPEND_POWER_SAVING_CFM_ID,              fsm_ignore_event),
    fsm_event_table_entry(MLME_ADD_PERIODIC_CFM_ID,                     handle_mlme_add_periodic_cfm),
    fsm_event_table_entry(MLME_DEL_PERIODIC_CFM_ID,                     fsm_ignore_event),
    fsm_event_table_entry(MLME_ADD_BLACKOUT_CFM_ID,                     handle_mlme_add_blackout_cfm),
    fsm_event_table_entry(MLME_DEL_BLACKOUT_CFM_ID,                     fsm_ignore_event),
    fsm_event_table_entry(MLME_SET_UNITDATA_FILTER_CFM_ID,              fsm_ignore_event),

    fsm_event_table_entry(MLME_SET_CFM_ID,                              fsm_ignore_event),
};


/** Debug Test state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
    /*                    State                        State                         Save    */
    /*                    Name                         Transitions                    *      */
    fsm_state_table_entry(FSMSTATE_stopped,            stoppedTransitions,           FALSE),
    fsm_state_table_entry(FSMSTATE_disconnected,       disconnectedTransitions,      FALSE),
    fsm_state_table_entry(FSMSTATE_connecting,         connectingTransitions,        FALSE),
    fsm_state_table_entry(FSMSTATE_connected,          connectedTransitions,         FALSE),
};

const FsmProcessStateMachine coex_fsm = {
#ifdef FSM_DEBUG
       "COEX",                                                                   /* SM Process Name       */
#endif
       COEX_PROCESS,                                                             /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                                         /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions,FALSE),    /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                    /* ignore event handers  */
       coex_init,                                                                /* Entry Function        */
       coex_reset,                                                               /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       coex_fsm_dump                                                             /* Trace Dump Function   */
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
