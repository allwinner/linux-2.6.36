/** @file scan_manager_fsm.c
 *
 * Scan Manager FSM Implementation
 *
 * @section LEGALBroadcastBssid
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Channels divide the frequency spectrum into bands, under Version 1.0 only
 *   the 2.4GHz Spectrum is catered for (The 4 & 5 GHz ranges will be added in
 *   later revisions), and the bands are 5MHz wide allowing for 14 possible
 *   channels.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/scan_manager_fsm/scan_manager_fsm.c#13 $
 *
 ****************************************************************************/

/** @{
 * @ingroup scan_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "connection_manager_fsm/connection_manager_fsm.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "scan_manager_fsm/scan_manager_fsm.h"
#include "scan_manager_fsm/scan_results_storage.h"
#include "scan_manager_fsm/roaming_channel_lists.h"
#include "ap_utils/ap_validation.h"

#include "ie_access/ie_access.h"
#include "ie_message_handling/ie_access_scan_req.h"

#include "mgt_sap/mgt_sap_from_sme_interface.h"

#include "event_pack_unpack/event_pack_unpack.h"

#include "smeio/smeio_trace_types.h"

#include "sme_configuration/sme_configuration_fsm.h"
#include "regulatory_domain/regulatory_domain.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Processes Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, FsmData))

#define SEC_TO_USEC(sec) (sec * 1000000)

#define AUTO_SCAN_NONINTERLEAVED          0x00
#define AUTO_SCAN_INTERLEAVED             0x01
#define AUTO_SCAN_NONINTERLEAVED_ONE_SHOT 0x02
#define AUTO_SCAN_INTERLEAVED_ONE_SHOT    0x03


/* Number of link quality ticks at unusable before falling back to the poor scan timings */
#define UNUSABLE_FALLBACK_TICKS 10

/* GLOBAL VARIABLE DEFINITIONS **********************************************/
extern const CsrUint8 channelScanOrderList[];

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
   FSMSTATE_monitoring,
   FSMSTATE_scanning,
   FSMSTATE_MAX_STATE
} FsmState;

typedef struct
{
    CsrUint8  numChannels;
    CsrUint8  channels[HIGHEST_80211_b_g_CHANNEL_NUM];
} ChnlList;

typedef struct
{
    CsrBool          activeScan;
    ChnlList         chnlList;
    DataReference    channelDR;
    DataReference    ieDR;
    unifi_RadioIF    ifIndex;
    BssType          bssType;
    unifi_MACAddress searchBssid;
    TimeUnits        minChannelTime;
    TimeUnits        maxChannelTime;
} ScanConfig;

typedef enum ScanConfigIndex
{
    ACTIVE_2_4_GHZ,
    ACTIVE_5_0_GHZ,
    PASSIVE_2_4_GHZ,
    PASSIVE_5_0_GHZ,
    MAX_SCAN_CONFIGS
} ScanConfigIndex;


#define ROAM_SCAN_AUTONOMOUS_ID (MAX_SCAN_CONFIGS + 1)

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData
{
    CsrUint16            startRequesterId;           /* pid who requested we start up (usually core)  */
    CsrUint16            scanRequesterId;            /* pid who requested we manual scan              */
    CsrBool              cancellingScans;            /* pid who requested a scan cancel               */
    CsrUint32            scanCancelTime;             /* time the scan cancel req was received         */
    CsrUint16            scanCancelRequesterId;      /* pid who requested we cancel a manual scan     */
    CsrUint16            stopRequesterId;            /* pid who told us to stop ALL scanning activity */

    CsrUint8             paused;                     /* whether we're paused (supports nesting)       */
    CsrBool              stopping;                   /* we've been told to stop ALL scan activity     */

    CsrUint8             autoScanCount;              /* bitmask giving types of installed auto-scans  */

    CsrUint8             initialScanInProgress;      /* TRUE when performing initial startup scans    */
    CsrBool              initialCloakedScansDone;    /* TRUE when initial startup cloaked scans are kicked off */
    CsrBool              mmiScanRequested;           /* pending scan from outside world               */
    void*                cfmAppHandle;               /* AppHandle for scan cfm                        */
    CsrBool              resuming;                   /* TRUE if coming out of suspend                 */
    CsrUint32            scanPauseStopTime;          /* used for aging & ownership transfer of results*/

    unifi_ScanStopCondition scanStopCondition;    /* stop scanning after the condition is met      */
    CsrBool                 scanStopConditionMet; /* The Scan Stop Condition has been met          */

    CsrBool              probeScanning;              /* TRUE: capture network's beacon period & SSID  */
    unifi_SSID           probeSSID;                  /* The SSID extracted during probing             */
    CsrUint16            probeBeaconPeriodMs;     /* The beacon period extracted during probing    */

    unifi_BasicUsability currentUsability;

    /* Scan configs (Up to 4 of them active and passive for 2.4Ghz and 5Ghz)                        */
    CsrUint8            scanConfigCurrent;             /* scanConfigs index for the current scan        */
    ScanConfig          scanConfigs[MAX_SCAN_CONFIGS]; /* scanConfigs for the scan requests             */

    RegDomData          regulatoryData;          /* Regulatory Domain (802.11d) Data              */
    CsrBool             deferredAutoScanInstall; /* If true & finished init scans - install bg scn*/

    FsmTimerId          fallBackTimer;           /* aggressive scanning fallback timer            */
    CsrBool             aggresiveScanOverride;   /* If true & finished init scans - install bg scn*/

    CsrBool             oneshotRoamingScanInstalled;

} FsmData;


/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/
static void stop_monitoring(FsmContext* context);
static void start_roaming_scan(FsmContext* context);
static void stop_roaming_scan(FsmContext* context);

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* ====================[ RUNTIME UTITILITY FUNCTIONS ]==================== */
/* ====================[ RUNTIME UTITILITY FUNCTIONS ]==================== */
/* ====================[ RUNTIME UTITILITY FUNCTIONS ]==================== */

static CsrBool isInUserChannelList(const unifi_DataBlock* channelList, CsrUint8 channel)
{
    int i;
    /* An empty list means accept all */
    if (channelList->length == 0)
    {
        return TRUE;
    }
    for(i = 0; i < channelList->length; i++)
    {
        if (channelList->data[i] == channel)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void update_channel_lists(FsmContext* context, unifi_ScanType scanType, const unifi_DataBlock* channelList)
{
    FsmData      * fsmData    = FSMDATA;
    SmeConfigData* cfg        = get_sme_config(context);
    CsrUint8 i;

    sme_trace_entry(( TR_SCAN, ">> update_channel_lists() ifindex = %s", trace_unifi_RadioIF(cfg->smeConfig.ifIndex) ));

    /* Zero content of each list */
    for(i = 0; i < MAX_SCAN_CONFIGS; i++)
    {
        fsmData->scanConfigs[i].chnlList.numChannels = 0;
    }

    if (cfg->smeConfig.ifIndex & unifi_GHZ_2_4)
    {
        ChnlList     * activeChL  = &fsmData->scanConfigs[ACTIVE_2_4_GHZ].chnlList;
        ChnlList     * passiveChL = &fsmData->scanConfigs[PASSIVE_2_4_GHZ].chnlList;
        CsrUint8       ordered_chan_index;

        /* Fill each list from data stored in '...ChannelsInUsedList...' */
        for ( i=0 ; i<HIGHEST_80211_b_g_CHANNEL_NUM ; i++ )
        {
            ordered_chan_index = channelScanOrderList[i];
            /* Skip this channel if not requested in the channel list */
            if (!isInUserChannelList(channelList, ordered_chan_index))
            {
               continue;
            }

            if ((fsmData->regulatoryData.ChannelsInUseList.Dott11_b_g_ChanList[i].chan_scan_mode != channelScanMode_none && scanType == unifi_ScanPassive) ||
                (fsmData->regulatoryData.ChannelsInUseList.Dott11_b_g_ChanList[i].chan_scan_mode == channelScanMode_passive && scanType != unifi_ScanActive))
            {
              /* sme_trace_debug(( TR_SCAN, "802.11b_g channel %02d -> PASSIVE", ordered_chan_index )); */
              passiveChL->channels[passiveChL->numChannels] = ordered_chan_index;
              passiveChL->numChannels++;
            }
            else if ( fsmData->regulatoryData.ChannelsInUseList.Dott11_b_g_ChanList[i].chan_scan_mode == channelScanMode_active && scanType != unifi_ScanPassive)
            {
              /* sme_trace_debug((TR_SCAN, "802.11b_g channel %02d -> ACTIVE", ordered_chan_index )); */
              activeChL->channels[activeChL->numChannels] = ordered_chan_index;
              activeChL->numChannels++;
            }
        }
        /* Now print each updated list separately */
        sme_trace_hex((TR_SCAN, TR_LVL_DEBUG, "update_channel_lists() ACTIVE_2_4_GHZ Channel List", activeChL->channels, activeChL->numChannels ));
        sme_trace_hex((TR_SCAN, TR_LVL_DEBUG, "update_channel_lists() PASSIVE_2_4_GHZ Channel List", passiveChL->channels, passiveChL->numChannels ));
    }

    if (cfg->smeConfig.ifIndex & unifi_GHZ_5_0 && scanType != unifi_ScanPassive)
    {
        /* 802.11a Channels
         * See IEEE802.11-2007 Annex J*/
        static const CsrUint8 active50ghzChannelsArray[] = {36, 40, 44, 48};
        static const unifi_DataBlock active50ghzChannels = {(CsrUint16)sizeof(active50ghzChannelsArray), (CsrUint8*)active50ghzChannelsArray};

        fsmData->scanConfigs[ACTIVE_5_0_GHZ].chnlList.numChannels = 0;
        for (i = 0; i < active50ghzChannels.length; i++)
        {
            /* Skip this channel if not requested in the channel list */
            if (!isInUserChannelList(channelList, (CsrUint8)active50ghzChannels.data[i]))
            {
                continue;
            }
            fsmData->scanConfigs[ACTIVE_5_0_GHZ].chnlList.channels[fsmData->scanConfigs[ACTIVE_5_0_GHZ].chnlList.numChannels] = active50ghzChannels.data[i];
            fsmData->scanConfigs[ACTIVE_5_0_GHZ].chnlList.numChannels++;
        }



        fsmData->regulatoryData.ChannelsInUseList.listChangedFlag = FALSE;
        sme_trace_hex((TR_SCAN, TR_LVL_DEBUG, "update_channel_lists() ACTIVE_5_0_GHZ Channel List",
                        fsmData->scanConfigs[ACTIVE_5_0_GHZ].chnlList.channels,
                        fsmData->scanConfigs[ACTIVE_5_0_GHZ].chnlList.numChannels ));
    }

    fsmData->regulatoryData.ChannelsInUseList.listChangedFlag = FALSE;
    sme_trace_entry(( TR_SCAN, "<< update_channel_lists()" ));
}

/**
 * @brief
 *   Test whether a channel is in a particular list
 *
 * @par Description
 *   This function allows a test to be performed on a given channel and a given
 *   list. If the given channel (ID) currently resides in the given list
 *   (active or passive) then the function returns true.
 *
 * @param[in]    context  : FSM context
 * @param[in]    channel  : The channel ID to move
 * @param[in]    whichL   : The list to search (active or passive)
 *
 * @return
 *   CsrBool: TRUE if channel resides in specified list, FALSE otherwise
 */
static CsrBool isInChannelList(FsmContext * context, CsrUint8 channel, const ChnlList* channalList)
{
    CsrUint8 i;
    for (i = 0; i < channalList->numChannels; i++)
    {
        if (channalList->channels[i] == channel)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void cleanup_scan_configs(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    CsrUint8 i;
    for(i = 0; i < MAX_SCAN_CONFIGS; i++)
    {
        if (fsmData->scanConfigs[i].channelDR.dataLength)
        {
            pld_release(getPldContext(context), (PldHdl)fsmData->scanConfigs[i].channelDR.slotNumber);
            fsmData->scanConfigs[i].channelDR.dataLength = 0;
        }
        if (fsmData->scanConfigs[i].ieDR.dataLength)
        {
            pld_release(getPldContext(context), (PldHdl)fsmData->scanConfigs[i].ieDR.slotNumber);
            fsmData->scanConfigs[i].ieDR.dataLength = 0;
        }
    }
}

/**
 * @brief
 *   Create Manual Scan Configuration
 *
 * @par Description
 *   This function handles the setup of the information necessary to
 *   execute an individual scan request - this may involve just active
 *   scanning, just passive scanning, or a combination of both.
 *
 *   There are many different requirements on how a scan should be performed;
 *   for instance probe scan for a single network, scan a single channel, scan
 *   all channels. This function handles all these cases and populates the
 *   active & passive scan config structures in the FSMs data accordingly.
 *
 * @param[in]    context      : FSM context
 * @param[in]    channel      : Channel # to scan (or 0 for all)
 * @param[in]    ieDr         : Any information elements needed in the scan req
 * @param[in]    bssid        : The bssid to scan for (FF:FF:FF:FF:FF:FF for any)
 * @param[in]    joinReq      : True if want to probe for a particular network
 * @param[in]    singleChannel: True if only scanning a single channel
 * @param[in]    singleResult : True if only want a single result
 * @param[in]    bssType      : The network type (ad-hoc or infrastructure)
 *
 * @return
 *   CsrBool: TRUE if successfully creates scan config, FALSE otherwise
 */
static CsrBool create_scan_config(FsmContext *     context,
                                  CsrUint8            channel,
                                  DataReference    ieDR,
                                  unifi_MACAddress bssid,
                                  CsrBool          joinReq,
                                  CsrBool          singleChannel,
                                  unifi_ScanStopCondition scanStopCondition,
                                  BssType          bssType,
                                  unifi_ScanType   scanType,
                                  const unifi_DataBlock* channelList)
{
    FsmData* fsmData = FSMDATA;
    DataReference* dataRefPtr = NULL;
    CsrBool result = TRUE;
    ns_ConnectionStatus connectionStatus = ns_get_connection_status(context);
    CsrUint8 i;
    CsrUint8 channelCount = 0;
    SmeConfigData*        cfg              = get_sme_config(context);
    unifi_ScanConfigData* scanData         = &cfg->scanConfig.scanCfg[unifi_NotConnected];

    sme_trace_entry((TR_SCAN, "create_scan_config: channel=%d singleChannel=%d joinReq=%d",
                    channel, singleChannel, joinReq));

    update_channel_lists(context, scanType, channelList);

    sme_trace_info_code
    (
        if (ieDR.dataLength)
        {
            pld_trace(getPldContext(context), TR_SCAN, TR_LVL_INFO, "create_scan_config: ie passed in", ieDR.slotNumber);
        }
    )

    if (connectionStatus == ns_ConnectionStatus_Connected)
    {
        sme_trace_info((TR_SCAN, "%-24s: create_scan_config(): connected to network. Usability is %s",
                        fsm_current_state_name(context),
                        trace_unifi_BasicUsability(fsmData->currentUsability) ));

        if (fsmData->currentUsability == unifi_Unusable && fsmData->aggresiveScanOverride)
        {
            scanData = &cfg->scanConfig.scanCfg[unifi_Poor];
        }
        else
        {
            scanData = &cfg->scanConfig.scanCfg[fsmData->currentUsability];
        }
    }
    else
    {
        sme_trace_info((TR_SCAN,
                        "%-24s: create_scan_config(): in disconnected state",
                        fsm_current_state_name(context)));
    }

    for(i = 0; i < MAX_SCAN_CONFIGS; i++)
    {
        fsmData->scanConfigs[i].channelDR.slotNumber  = 0;
        fsmData->scanConfigs[i].channelDR.dataLength  = 0;
        fsmData->scanConfigs[i].ieDR.slotNumber       = 0;
        fsmData->scanConfigs[i].ieDR.dataLength       = 0;
        fsmData->scanConfigs[i].searchBssid           = bssid;
        fsmData->scanConfigs[i].bssType               = bssType;

        /* if (fsmData->scanConfigs[i].activeScan) */
        if ( (i==ACTIVE_2_4_GHZ) || (i==ACTIVE_5_0_GHZ) )
        {
            fsmData->scanConfigs[i].minChannelTime        = scanData->minActiveChannelTimeTu;
            fsmData->scanConfigs[i].maxChannelTime        = scanData->maxActiveChannelTimeTu;
        }
        else /* if ( (i==PASSIVE_2_4_GHZ) || (i==PASSIVE_5_0_GHZ) ) */
        {
            fsmData->scanConfigs[i].minChannelTime        = scanData->minPassiveChannelTimeTu;
            fsmData->scanConfigs[i].maxChannelTime        = scanData->maxPassiveChannelTimeTu;
        }

        /* Disable Passive scanning when connected */
        if (!fsmData->scanConfigs[i].activeScan && connectionStatus == ns_ConnectionStatus_Connected)
        {
            fsmData->scanConfigs[i].chnlList.numChannels = 0;
        }
    }

    /* Firstly handle the case where we're scanning a single channel */
    if (channel != 0 && singleChannel)
    {
        sme_trace_debug((TR_SCAN, "Configuring for single channel scan"));

        for(i = 0; i < MAX_SCAN_CONFIGS; i++)
        {
            /* Skip empty configs */
            if (fsmData->scanConfigs[i].chnlList.numChannels == 0)
            {
                continue;
            }

            if (isInChannelList(context, channel, &fsmData->scanConfigs[i].chnlList))
            {
                /* Use ONLY the channel requested */
                fsmData->scanConfigs[i].chnlList.numChannels = 1;
                fsmData->scanConfigs[i].chnlList.channels[0] = channel;

                /* Single channel join scans MUST be active so fail if not allowed */
                if (!fsmData->scanConfigs[i].activeScan && joinReq)
                {
                    sme_trace_warn((TR_SCAN, "%-24s: Cannot configure for active probe scan on channel %d, we are not allowed to transmit on this channel",
                                             fsm_current_state_name(context), channel ));
                    if (ieDR.dataLength)
                    {
                        pld_release(getPldContext(context), (PldHdl)ieDR.slotNumber);
                    }
                    return FALSE;
                }

                /* Modify timings if we're performing a 'join' scan */
                if (joinReq)
                {
                    if (bssType & BssType_Independent)
                    {
                        fsmData->scanConfigs[i].minChannelTime = SCANMGR_TMPFIXED_SINGLESCAN_ADHOC_TU;
                    }
                    else
                    {
                        fsmData->scanConfigs[i].minChannelTime = fsmData->scanConfigs[i].activeScan ?
                                                    SCANMGR_TMPFIXED_SINGLESCAN_ACTIVE_TU:
                                                    SCANMGR_TMPFIXED_SINGLESCAN_PASSIVE_TU;
                    }
                    fsmData->scanConfigs[i].maxChannelTime = fsmData->scanConfigs[i].minChannelTime;
                }
            }
            else
            {
                /* Clear the channels as this scan Config will not be used */
                fsmData->scanConfigs[i].chnlList.numChannels = 0;
            }
        }
    }

    /* Create data references of the channels */
    for(i = 0; i < MAX_SCAN_CONFIGS; i++)
    {
        /* Skip empty configs */
        if (fsmData->scanConfigs[i].chnlList.numChannels == 0)
        {
            continue;
        }

        channelCount += fsmData->scanConfigs[i].chnlList.numChannels;

        /* Okay, put the selected channels into a new buffer and data reference for the firmware */
        dataRefPtr = &fsmData->scanConfigs[i].channelDR;
        pld_store(getPldContext(context), fsmData->scanConfigs[i].chnlList.channels, fsmData->scanConfigs[i].chnlList.numChannels,
                  &dataRefPtr->slotNumber);
        dataRefPtr->dataLength = fsmData->scanConfigs[i].chnlList.numChannels;

        /* If channel is specified then make sure the specified channel is scanned first */
        if (channel && fsmData->scanConfigs[i].chnlList.numChannels > 1)
        {
            CsrUint8   j;
            CsrUint8 * buffer;
            CsrUint16  numChannels;

            pld_access(getPldContext(context), dataRefPtr->slotNumber, (void **)&buffer, &numChannels);
            for (j = 0; j < numChannels; j++)
            {
                if (buffer[j] == channel)
                {
                    CsrMemMove(&buffer[1], buffer, j);
                    buffer[0] = channel;
                    break;
                }
            }
        }
    }


    for(i = 0; i < MAX_SCAN_CONFIGS; i++)
    {
        /* Skip empty configs */
        if (fsmData->scanConfigs[i].chnlList.numChannels == 0)
        {
            continue;
        }

        if (fsmData->scanConfigs[i].activeScan)
        {
            fsmData->scanConfigs[i].ieDR = regdom_create_ie_dr(context, ieDR, FALSE);
            if (fsmData->scanConfigs[i].ieDR.dataLength == 0 && ieDR.dataLength > 0)
            {
                sme_trace_error((TR_SCAN, "Regulatory domain subsystem returned empty IE DataReference - scan config failed"));
                result = FALSE;
                break;
            }
        }
        else
        {
            /* Passive Scans just use what is passed in */
            fsmData->scanConfigs[i].ieDR = ieDR;
            if (ieDR.dataLength)
            {
                pld_add_ref(getPldContext(context), (PldHdl)ieDR.slotNumber);
            }
        }
    }

    /* Make sure at least 1 config has something to scan */
    if (channelCount == 0)
    {
        result = FALSE;
    }

    /* Release the passed in data as we have added references as needed */
    if (ieDR.dataLength)
    {
        pld_release(getPldContext(context), (PldHdl)ieDR.slotNumber);
    }

    /* On error cleanup any payloads created */
    if (!result)
    {
        cleanup_scan_configs(context);
    }

    return result;
}

static CsrBool perform_next_scan(FsmContext *context)
{
    FsmData* fsmData = FSMDATA;
    ScanConfig* scanCfg = NULL;
    unifi_MACAddress da = BroadcastBssid;
    sme_trace_entry((TR_SCAN, "perform_next_scan()"));

    if (fsmData->cancellingScans)
    {
        sme_trace_entry((TR_SCAN, "perform_next_scan() : Cancelling Scan"));
        return FALSE;
    }

    /* Find first config with channels to scan */
    for(; fsmData->scanConfigCurrent < MAX_SCAN_CONFIGS; fsmData->scanConfigCurrent++)
    {
        if (fsmData->scanConfigs[fsmData->scanConfigCurrent].chnlList.numChannels != 0)
        {
            scanCfg = &fsmData->scanConfigs[fsmData->scanConfigCurrent];
            break;
        }
    }

    if (scanCfg == NULL)
    {
        sme_trace_entry((TR_SCAN, "perform_next_scan() : All scans complete"));
        return FALSE;
    }

    verify(TR_SCAN, fsmData->scanConfigCurrent < MAX_SCAN_CONFIGS);

    sme_trace_debug((TR_SCAN, "perform_next_scan(%s):: Initiating %s scan: ifIndex:%s, Channels %d",
                     trace_unifi_MACAddress(scanCfg->searchBssid, getMacAddressBuffer(context)),
                     scanCfg->activeScan?"active":"passive",
                     trace_unifi_RadioIF(scanCfg->ifIndex),
                     scanCfg->chnlList.numChannels));

    if (scanCfg->bssType == BssType_Infrastructure)
    {
        da = scanCfg->searchBssid;
    }

    send_mlme_scan_req(context,
                       scanCfg->channelDR,
                       scanCfg->ieDR,
                       scanCfg->ifIndex,
                       scanCfg->bssType,
                       da,
                       scanCfg->searchBssid,
                       (scanCfg->activeScan ? ScanType_ActiveScan : ScanType_PassiveScan),
                       SCANMGR_TMPFIXED_PROBE_DELAY_USEC,
                       scanCfg->minChannelTime,
                       scanCfg->maxChannelTime);

    /* Now we have used the Payloads clear the reference */
    scanCfg->channelDR.dataLength = 0;
    scanCfg->ieDR.dataLength = 0;

    fsm_next_state(context, FSMSTATE_scanning);
    return TRUE;
}

/**
 * @brief
 *   Perform Manual Scan (MLME_SCAN_REQ) Request
 *
 * @par Description
 *   This function handles the generation and issue of the appropriate
 *   MLME_SCAN_REQ based upon the passive and active scan configurations
 *   that were set up by a prior call to create_scan_config().
 *
 *   Depending on the aforementioned scan configuration, this function
 *   will generate a single scan request down to the firmware. This
 *   request will be either an active or passive scan request. In cases
 *   where two scan requests are needed (i.e. we may need to scan some
 *   channels actively, others passively) this function will issue the
 *   active scan and rely upon the mlme_scan_complete handler to issue
 *   any secondary passive scan
 *
 * @param[in]    context      : FSM context
 *
 * @return
 *   void
 */
static void perform_scan(FsmContext *context)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_info((TR_SCAN, "perform_next_scan()"));
    fsmData->scanConfigCurrent = 0;
    (void)perform_next_scan(context);
}


/**
 * @brief
 *   Notify Manual Scan Complete
 *
 * @par Description
 *   This function handles the sending of confirmation messages to entities that
 *   requested some form of manual (i.e. non-autonomous) scan - be it from some
 *   external source (e.g. MMI), or some internal process (e.g. connection mgr).
 *
 * @param[in]    context      : FSM context
 * @param[in]    scanSucceed  : TRUE if mlme_scan_req completed successfully
 *
 * @return
 *   void
 */
static void notify_manual_scan_complete(FsmContext *context, ResultCode scanResult)
{
    FsmData* fsmData = FSMDATA;

    /*
     * Sanity check the ID of the process that requested the non-autonomous
     * scan, it should be set to something other than FSM_TERMINATE UNLESS
     * the request came from outside of the SME.
     */
    if (fsmData->scanRequesterId != FSM_TERMINATE)
    {
        /*
         * If the scan request came from outside of the scan manager itself
         * then we must send a confirmation (scanManager queues scan requests
         * but doesn't want the confirms)
         */
        if (fsmData->scanRequesterId != getSmeContext(context)->scanManagerInstance)
        {
            CsrBool joinResult = FALSE;

            /*
             * If we're performing a JOIN scan then we need to figure out
             * whether we succeeded in finding the network we scanned for.
             */
            if (fsmData->probeScanning)
            {
                joinResult = scanResult==ResultCode_Success && srs_get_join_scan_parameters(context) != NULL;
            }
            send_sm_scan_cfm(context, fsmData->scanRequesterId, joinResult, scanResult, fsmData->probeBeaconPeriodMs);
        }
    }

    if (fsmData->initialScanInProgress)
    {
        sme_trace_debug((TR_SCAN, "Initial scan complete %d", fsmData->initialScanInProgress));
        fsmData->initialScanInProgress--;
        if (!fsmData->initialScanInProgress)
        {
            const FsmEvent* pScanFullReqEvt = fsm_sniff_saved_event(context, UNIFI_MGT_SCAN_FULL_REQ_ID);

#if 0
            if (!fsmData->initialCloakedScansDone)
            {
                fsmData->initialCloakedScansDone = TRUE;
                /* Initial scans done. Now scan for Cloaked networks we may have seen */
                fsmData->initialScanInProgress += srs_scan_cloaked_ssids(context);
            }
#else
            (void)srs_scan_cloaked_ssids(context);
            fsmData->initialCloakedScansDone = TRUE;
#endif
            /* If there are any queued full scan requests then we don't need
             * to perform a scan as we've just done one, just generate cfms
             * for these requests.
             */
            while (NULL != pScanFullReqEvt)
            {
                const UnifiMgtScanFullReq_Evt* pReq = (UnifiMgtScanFullReq_Evt*)pScanFullReqEvt;

                /* Remove any user requested scans that that would already be satisfied
                 * by the initial scanning performed, this basically means any very basic
                 * scan request request for any BSS type with the broadcast address specified.
                 * Any user request that is different to this is left on the queue.
                 */
                if (!pReq->forceScan &&
                    0 == pReq->channelListCount &&
                    0 == pReq->probeIeLength &&
                    0 == pReq->ssidCount &&
                    unifi_AnyBss == pReq->bssType &&
                    0 == CsrMemCmp(pReq->bssid.data, BroadcastBssid.data, sizeof(BroadcastBssid.data)))
                {
                    sme_trace_info((TR_SCAN,"Manual scan complete: sending scan confirm (%s) to environment",
                                    trace_unifi_Status(scanResult==ResultCode_Success?unifi_Success:unifi_Error)));
                    call_unifi_mgt_scan_full_cfm(context, pReq->appHandle, scanResult==ResultCode_Success? unifi_Success:unifi_Error);
                    fsm_remove_saved_event(context, (FsmEvent*)pScanFullReqEvt);
                    free_unifi_mgt_scan_full_req_contents(pReq);
                    CsrPfree((void*)pReq);
                    pScanFullReqEvt = fsm_sniff_saved_event(context, UNIFI_MGT_SCAN_FULL_REQ_ID);
                }
                else
                {
                    sme_trace_info((TR_SCAN,"Manual scan complete: retaining queued full scan request"));
                    pScanFullReqEvt = NULL;
                }
            }
            /*
             * Once the initial scans that the scan manager queues on startup have
             * completed, we need to save the calibration data.
             */
            if (!fsmData->initialScanInProgress)
            {
                send_mib_save_calibration_data_req(context, getSmeContext(context)->mibAccessInstance);
                fsmData->deferredAutoScanInstall = TRUE;
            }
        }
    }

    /*
     * The scan that has just completed may have been requested by an outside
     * entity, alternatively, external scan requests may have been made while
     * we were scanning for some internal entity (e.g. connection manager).
     * Make sure we respond appropriately to any external scan requests
     */
    if (fsmData->mmiScanRequested)
    {
        /*
         * Do not tell any outside entity that the scan is complete if we're
         * busy doing the initial scans. A caveat to this is if we're resuming
         * and the scan manager has completed doing all its initial single
         * channel scans (looking for any networks we recently connected to)
         */
        if (!fsmData->initialScanInProgress || fsmData->stopping || (fsmData->resuming && fsmData->initialScanInProgress == 1))
        {
            sme_trace_debug
            ((
                TR_SCAN,
                "Manual scan complete: %s with %d external requests queued",
                fsmData->initialScanInProgress?"normal operation":"resuming and completed initial single channel scans",
                fsmData->mmiScanRequested
            ));

            if (fsmData->mmiScanRequested)
            {
                fsmData->mmiScanRequested = FALSE;
                sme_trace_info((TR_SCAN,"Manual scan complete: sending scan confirm (%s) to environment",
                                 trace_unifi_Status(scanResult==ResultCode_Success?unifi_Success:unifi_Error)));
                call_unifi_mgt_scan_full_cfm(context, fsmData->cfmAppHandle, scanResult==ResultCode_Success? unifi_Success:unifi_Error);
            }
        }
    }
}

static void reset_volatile_fsm_data(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;

    fsmData->startRequesterId         = FSM_TERMINATE;
    fsmData->scanRequesterId          = FSM_TERMINATE;
    fsmData->scanCancelRequesterId    = FSM_TERMINATE;
    fsmData->stopRequesterId          = FSM_TERMINATE;
    fsmData->cancellingScans          = FALSE;
    fsmData->scanCancelTime           = 0;
    fsmData->paused                   = FALSE;
    fsmData->stopping                 = FALSE;
    fsmData->autoScanCount            = 0;
    fsmData->initialScanInProgress    = 0;
    fsmData->initialCloakedScansDone  = FALSE;
    fsmData->mmiScanRequested         = FALSE;

    fsmData->scanPauseStopTime        = 0;
    fsmData->scanStopCondition        = FALSE;
    fsmData->scanStopConditionMet     = FALSE;
    fsmData->probeScanning            = FALSE;
    fsmData->probeBeaconPeriodMs      = 0;
    fsmData->currentUsability         = unifi_Satisfactory;
    fsmData->deferredAutoScanInstall  = FALSE;

    fsmData->aggresiveScanOverride    = FALSE;
    fsmData->fallBackTimer.uniqueid   = 0;

    fsmData->oneshotRoamingScanInstalled = FALSE;

    CsrMemSet(&fsmData->probeSSID, 0, sizeof(unifi_SSID));
    CsrMemSet(&fsmData->regulatoryData, 0, sizeof(RegDomData));

    /* Setup the ScanConfigs */
    CsrMemSet(fsmData->scanConfigs, 0, sizeof(fsmData->scanConfigs));
    fsmData->scanConfigs[ACTIVE_2_4_GHZ].activeScan = TRUE;
    fsmData->scanConfigs[ACTIVE_2_4_GHZ].ifIndex = unifi_GHZ_2_4;
    fsmData->scanConfigs[PASSIVE_2_4_GHZ].activeScan = FALSE;
    fsmData->scanConfigs[PASSIVE_2_4_GHZ].ifIndex = unifi_GHZ_2_4;
    fsmData->scanConfigs[ACTIVE_5_0_GHZ].activeScan = TRUE;
    fsmData->scanConfigs[ACTIVE_5_0_GHZ].ifIndex = unifi_GHZ_5_0;
    fsmData->scanConfigs[PASSIVE_5_0_GHZ].activeScan = FALSE;
    fsmData->scanConfigs[PASSIVE_5_0_GHZ].ifIndex = unifi_GHZ_5_0;
}

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
static void init_scan_manager_fsm(FsmContext* context)
{
    FsmData* fsmData;

    sme_trace_entry((TR_SCAN, "init_scan_manager_fsm()"));
    fsm_create_params(context, FsmData);

    reset_volatile_fsm_data(context);

    fsmData                           = FSMDATA;
    fsmData->resuming                 = FALSE; /* not safe to reset on start */

    fsm_next_state(context, FSMSTATE_stopped);
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
static void reset_scan_manager_fsm(FsmContext* context)
{
    sme_trace_entry((TR_SCAN, "reset_scan_manager_fsm()"));
    srs_reset_scan_result_storage(context);
    roaming_channel_list_flush(context);
}

/**
 * @brief
 *   Builds and adds a autonomous scan ie to payload manager
 *
 * @return
 *   DataReference
 */
static DataReference build_autonomous_scan_ie(FsmContext* context, CsrBool interleaved, unifi_ScanConfigData* scanData, const unifi_SSID* ssid)
{
    /* Ie is made of:
     *   "CSR Autonomous Scan Timing"     17 Bytes
     *   "CSR Autonomous Scan Attributes" 19 Bytes
     *   "SSID"                           3 -> 34 Bytes (Optional)
     */
    static const CsrUint8 csrAutonomousScanTimingHeader[]     = {0xDD, 0x0F, 0x00, 0x02, 0x5B, 0x01, 0x01, 0x01};
    static const CsrUint8 csrAutonomousScanAttributesHeader[] = {0xDD, 0x11, 0x00, 0x02, 0x5B, 0x01, 0x02};
    SmeConfigData*        cfg = get_sme_config(context);
    CsrUint8              ie[17 + 19 + UNIFI_SSID_MAX_OCTETS + 2];    /*lint !e785*/
    DataReference         ieDr;
    CsrUint8*             iePtr;

    sme_trace_entry((TR_SCAN, "build_autonomous_scan_ie: interval=%d validity=%d, activeChTimes=%d:%d, passvChTimes=%d:%d",
                     scanData->intervalSeconds,
                     scanData->validitySeconds,
                     scanData->minActiveChannelTimeTu,
                     scanData->maxActiveChannelTimeTu,
                     scanData->minPassiveChannelTimeTu,
                     scanData->maxPassiveChannelTimeTu));

    /* Configure CSR Autonomous Scan Timing */
    CsrMemCpy(ie, csrAutonomousScanTimingHeader, sizeof(csrAutonomousScanTimingHeader));
    iePtr = &ie[sizeof(csrAutonomousScanTimingHeader)];
    (void)event_pack_CsrUint32(&iePtr, SEC_TO_USEC(scanData->intervalSeconds));
    (void)event_pack_CsrUint32(&iePtr, SEC_TO_USEC(scanData->intervalSeconds) + SEC_TO_USEC(1));
    (void)event_pack_CsrUint8(&iePtr,  ((CsrUint8)(interleaved?AUTO_SCAN_INTERLEAVED:AUTO_SCAN_NONINTERLEAVED)));
    ieDr.dataLength = 17;

    sme_trace_entry((TR_SCAN, "build_autonomous_scan_ie: rssi(H,D,L)=%d,%d,%d snr(H,D,L)=%d,%d,%d",
                     cfg->scanConfig.highRSSIThreshold,
                     cfg->scanConfig.deltaRSSIThreshold,
                     cfg->scanConfig.lowRSSIThreshold,
                     cfg->scanConfig.highSNRThreshold,
                     cfg->scanConfig.deltaSNRThreshold,
                     cfg->scanConfig.lowSNRThreshold));

    /* Configure CSR Autonomous Scan Attributes */
    CsrMemCpy(iePtr, csrAutonomousScanAttributesHeader, sizeof(csrAutonomousScanAttributesHeader));
    iePtr += sizeof(csrAutonomousScanAttributesHeader);
    (void)event_pack_CsrUint16(&iePtr, (CsrUint16)cfg->scanConfig.highRSSIThreshold);  /*lint !e571*/
    (void)event_pack_CsrUint16(&iePtr, (CsrUint16)cfg->scanConfig.lowRSSIThreshold);   /*lint !e571*/
    (void)event_pack_CsrUint8 (&iePtr, (CsrUint8) cfg->scanConfig.deltaRSSIThreshold);
    (void)event_pack_CsrUint16(&iePtr, (CsrUint16)cfg->scanConfig.highSNRThreshold);   /*lint !e571*/
    (void)event_pack_CsrUint16(&iePtr, (CsrUint16)cfg->scanConfig.lowSNRThreshold);    /*lint !e571*/
    (void)event_pack_CsrUint8 (&iePtr, (CsrUint8) cfg->scanConfig.deltaSNRThreshold);
    (void)event_pack_CsrUint16(&iePtr, scanData->validitySeconds);

    ieDr.dataLength += 19;

    /* Configure SSID */
    if(ssid && ssid->length)
    {
        (void)ie_setSSID(iePtr, ssid);
        ieDr.dataLength += cfg->connectionInfo.ssid.length + 2;
    }
    sme_trace_hex((TR_SCAN, TR_LVL_DEBUG, "build_autonomous_scan_ie()", ie, ieDr.dataLength ));

    pld_store(getPldContext(context), ie, ieDr.dataLength, &ieDr.slotNumber);
    return ieDr;
}

/**
 * @brief
 *   sets up a monitor auto scan for all channels
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void start_monitoring(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    DataReference         csrAutoIe;
    ns_ConnectionStatus   connectionStatus = ns_get_connection_status(context);
    SmeConfigData*        cfg              = get_sme_config(context);
    unifi_ScanConfigData* scanData         = &cfg->scanConfig.scanCfg[unifi_NotConnected];
    srs_scan_data*        joinScan         = srs_get_join_scan_parameters(context);
    CsrUint8 i;


    sme_trace_entry((TR_SCAN, ">> start_monitoring()"));

    /* Start Roam Scanning */
    if (!cfg->roamingConfig.disableRoamScans &&
        ns_get_connection_status(context) == ns_ConnectionStatus_Connected &&
        joinScan != NULL &&
        joinScan->scanResult.bssType == unifi_Infrastructure)
    {
        start_roaming_scan(context);
    }

    if (cfg->scanConfig.disableAutonomousScans)
    {
        sme_trace_debug((TR_SCAN, "start_monitoring() :: Autonomous scanning is disabled"));
        if (fsmData->autoScanCount)
        {
            sme_trace_info((TR_SCAN, "start_monitoring: Auto scan currently running though - deleting"));
            stop_monitoring(context);
            fsmData->autoScanCount = 0;
        }
        return; /* bail early, stop_monitoring will transfer ownership of auto-scan results */
    }

    if (connectionStatus == ns_ConnectionStatus_Connected)
    {
        sme_trace_info((TR_SCAN, "%-24s: start_monitoring: connected to network. Usability is %s",
                        fsm_current_state_name(context),
                        trace_unifi_BasicUsability(fsmData->currentUsability) ));

        if (fsmData->currentUsability == unifi_Unusable && fsmData->aggresiveScanOverride)
        {
            scanData = &cfg->scanConfig.scanCfg[unifi_Poor];
        }
        else
        {
            scanData = &cfg->scanConfig.scanCfg[fsmData->currentUsability];
        }
    }
    else
    {
        sme_trace_info((TR_SCAN, "%-24s: start_monitoring: in disconnected state",
                        fsm_current_state_name(context)));
        /* reset usability when moving back to disconnected state */
        fsmData->currentUsability = unifi_Satisfactory;
    }

    sme_trace_debug((TR_SCAN, "start_monitoring: interval=%d validity=%d, activeChTimes=%d:%d, passvChTimes=%d:%d",
                     scanData->intervalSeconds, scanData->validitySeconds,
                     scanData->minActiveChannelTimeTu, scanData->maxActiveChannelTimeTu,
                     scanData->minPassiveChannelTimeTu, scanData->maxPassiveChannelTimeTu));

    /*
     * If the scan interval is zero then do not install an autonomous scan - check if one is
     * already running, if so, delete it.
     */
    if (scanData->intervalSeconds == 0)
    {
        sme_trace_info((TR_SCAN, "start_monitoring: Autonomous scan interval is zero so not installing new auto-scan"));
        if (fsmData->autoScanCount)
        {
            sme_trace_info((TR_SCAN, "start_monitoring: Auto scan currently running though - deleting"));
            stop_monitoring(context);
            fsmData->autoScanCount = 0;
        }
        return; /* bail early, stop_monitoring will transfer ownership of auto-scan results */
    }

    if (fsmData->autoScanCount == 0 && fsmData->scanPauseStopTime != 0)
    {
        srs_take_scan_results_ownership(context, (CsrUint32)CsrTimeSub(fsm_get_time_of_day_ms(context), fsmData->scanPauseStopTime));
    }
    else
    {
        srs_take_scan_results_ownership(context, 0);
    }
    fsmData->scanPauseStopTime = 0;

    csrAutoIe = build_autonomous_scan_ie(context, (CsrBool)(connectionStatus == ns_ConnectionStatus_Connected), scanData, NULL);

    (void)create_scan_config(context, 0, csrAutoIe, BroadcastBssid, FALSE, FALSE, unifi_ScanStopAllResults, BssType_AnyBss, unifi_ScanAll, &NullDataBlock);

    for(i = 0; i < MAX_SCAN_CONFIGS; i++)
    {
        /* Skip empty configs */
        if (fsmData->scanConfigs[i].chnlList.numChannels == 0)
        {
            continue;
        }

        sme_trace_debug((TR_SCAN, "start_monitoring():: Initiating Autonomous %s scan: ifIndex:%s, Channels %d, MaxChTime=%d, MinChTime=%d)",
                         fsmData->scanConfigs[i].activeScan ? "Active" : "Passive",
                         trace_unifi_RadioIF(fsmData->scanConfigs[i].ifIndex),
                         fsmData->scanConfigs[i].chnlList.numChannels,
                         fsmData->scanConfigs[i].minChannelTime,
                         fsmData->scanConfigs[i].maxChannelTime ));

        fsmData->autoScanCount++;

        send_mlme_add_autonomous_scan_req( context,
                                           fsmData->scanConfigs[i].channelDR,
                                           fsmData->scanConfigs[i].ieDR,
                                           fsmData->autoScanCount,
                                           fsmData->scanConfigs[i].ifIndex,
                                           0,
                                           fsmData->scanConfigs[i].bssType,
                                           BroadcastBssid,
                                           (fsmData->scanConfigs[i].activeScan?ScanType_ActiveScan:ScanType_PassiveScan),
                                           SCANMGR_TMPFIXED_PROBE_DELAY_USEC,
                                           fsmData->scanConfigs[i].minChannelTime,
                                           fsmData->scanConfigs[i].maxChannelTime );

        /* Now we have used the Payloads clear the reference */
        fsmData->scanConfigs[i].channelDR.dataLength = 0;
        fsmData->scanConfigs[i].ieDR.dataLength = 0;

    }

    if (fsmData->autoScanCount == 0 && fsmData->scanPauseStopTime != 0)
    {
        srs_take_scan_results_ownership( context, (CsrUint32)CsrTimeSub(fsm_get_time_of_day_ms(context), fsmData->scanPauseStopTime) ); /*lint !e571*/
    }
    else
    {
        srs_take_scan_results_ownership( context, 0 );
    }
    fsmData->scanPauseStopTime = 0;

    sme_trace_entry((TR_SCAN, "<< start_monitoring()"));
}

/**
 * @brief
 *   deletes and outstanding auto scans
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void stop_monitoring(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SCAN, "stop_monitoring()"));

    /* kill the aggressive fallback timer */
    if(fsmData->aggresiveScanOverride)
    {
        fsmData->aggresiveScanOverride = FALSE;
        sme_trace_debug((TR_SCAN, "fsm_remove_timer fallBackTimer"));
        fsm_remove_timer(context, fsmData->fallBackTimer);
    }

    if (fsmData->autoScanCount)
    {
        CsrUint8 i;
        fsmData->scanPauseStopTime = fsm_get_time_of_day_ms(context);
        for(i = 1; i <= fsmData->autoScanCount; i++)
        {
            sme_trace_info((TR_SCAN, "%-24s: Deleting autonomous scan %d", fsm_current_state_name(context), i));
            send_mlme_del_autonomous_scan_req(context, i);
        }
        fsmData->autoScanCount = 0;
        srs_take_scan_results_ownership(context, 0);
    }

    stop_roaming_scan(context);
}

/* =====================[ EVENT HANDLERS: STOPPED STATE ]==================== */

/**
 * @brief
 *   Start the FSM
 *
 * @param[in] context : FSM context
 *
 * @return
 *  Void
 */
static void stopped_start(FsmContext *context, const CoreStartReq_Evt *req)
{
    FsmData               * fsmData = FSMDATA;
    SmeConfigData         * cfg         = get_sme_config(context);
    FsmTimerId timerId;
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    sme_trace_entry((TR_SCAN, ">> stopped_start()"));

    reset_volatile_fsm_data(context);
    fsmData->resuming         = req->resume;
    fsmData->startRequesterId = req->common.sender_;

    /* Initialising 802.11d Data
     * including the channels scan mode (Passive/Active)
     * depending on trust level and Default (MIB) Regulatory domain
     *
     * Note: at this stage relevant MIB values have been read so
     *       the initialise function can do its job properly...
     */
    regdom_init(context, &fsmData->regulatoryData);

    /* Kick Off Channel_Check_Expiry Timer  (but only if 802.11d operation is enabled) */
    if (fsmData->regulatoryData.regulatoryDomainOperation == regDomain_On)
    {
       send_sm_check_channels_expiry_timer ( context,
                                             timerId,
                                             CHECK_CHANNEL_EXPIRY_TIMER_DURATION,
                                             CHECK_CHANNEL_EXPIRY_TIMER_TOLERANCE );
       fsmData->regulatoryData.chkChanXpryTimerId = timerId;
    }

    srs_reset_scan_result_storage(context);
    send_core_start_cfm(context,fsmData->startRequesterId, unifi_Success); /* using the saved startRequesterId */
    fsmData->startRequesterId = FSM_TERMINATE; /* Dont need it so reset its value */

    fsmData->initialScanInProgress = 0;
    fsmData->initialCloakedScansDone = FALSE;

    /* Queue up a bunch of quick scans for all the networks we were recently connected to */
    while (fsmData->initialScanInProgress < MAX_CONNECTED_AP_HISTORY &&
           CsrMemCmp(&cfg->lastJoinedAddresses[fsmData->initialScanInProgress].address,
                   BroadcastBssid.data, sizeof(BroadcastBssid.data)) != 0)
    {
        sme_trace_info
        ((
            TR_SCAN, "%-24s: enabling: queuing internal directed scan for %s",
            fsm_current_state_name(context),
            trace_unifi_MACAddress(cfg->lastJoinedAddresses[fsmData->initialScanInProgress].address, getMacAddressBuffer(context))
        ));
        send_sm_scan_req(context, getSmeContext(context)->scanManagerInstance, now,
                         cfg->lastJoinedAddresses[fsmData->initialScanInProgress].channel,
                         0,
                         NULL,
                         cfg->lastJoinedAddresses[fsmData->initialScanInProgress].address,
                         FALSE,
                         TRUE,
                         FALSE,
                         unifi_ScanStopFirstResult,
                         cfg->lastJoinedAddresses[fsmData->initialScanInProgress].bssType,
                         0);
        fsmData->initialScanInProgress++;
    }

    sme_trace_info
    ((
        TR_SCAN, "%-24s: enabling: queuing internal FULL scan request",
                 fsm_current_state_name(context)
    ));
    send_sm_scan_req(context, getSmeContext(context)->scanManagerInstance, now, 0,
                     0, NULL, BroadcastBssid, FALSE, FALSE, FALSE, unifi_ScanStopAllResults, BssType_AnyBss, 0);
    fsmData->initialScanInProgress++;

    /*
     * Even though we haven't installed autonomous scans yet, we can still enter monitoring state -
     * as soon as the initial scans have completed, autonomous scan will get installed.
     */
    fsm_next_state(context, FSMSTATE_monitoring);

}

/**
 * @brief
 *   Bounce Scan Reqest request as it is not appropriate in this state
 *
 * @param[in] context : FSM context
 * @param[in] req     : Event
 *
 * @return
 *  void
 */
static void bounce_mmi_full_scan(FsmContext *context, const UnifiMgtScanFullReq_Evt *req)
{
    sme_trace_entry
    ((
        TR_SCAN, "%-24s: BOUNCING external (full) scan request",
        fsm_current_state_name(context)
    ));

    call_unifi_mgt_scan_full_cfm(context, req->appHandle, unifi_WifiOff);
    free_unifi_mgt_scan_full_req_contents(req);
}

/**
 * @brief
 *   Bounce Scan Reqest request as it is not appropriate in this state
 *
 * @param[in] context : FSM context
 * @param[in] req     : Event
 *
 * @return
 *  void
 */
static void bounce_scan_req(FsmContext *context, const SmScanReq_Evt *req)
{
    sme_trace_info
    ((
        TR_SCAN, "%-24s: BOUNCING scan request from %s process",
        fsm_current_state_name(context),
        fsm_process_name_by_id(context, req->common.sender_)
    ));

    if (req->common.sender_ != getSmeContext(context)->scanManagerInstance)
    {
        send_sm_scan_cfm(context, req->common.sender_, FALSE, ResultCode_UnspecifiedFailure, 0);
    }

    CsrPfree(req->ssid);
}

static void ignore_scan_req(FsmContext *context, const SmScanReq_Evt *req)
{
    CsrPfree(req->ssid);
}

/* ====================[ EVENT HANDLERS: MONITORING STATE ]================== */

/**
 * @brief
 *   Idle mmi_full_scan transition
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void monitoring_ext_full_scan(FsmContext *context, const UnifiMgtScanFullReq_Evt *req)
{
    FsmData * fsmData  = FSMDATA;
    SmeConfigData * cfg = get_sme_config(context);
    ns_ConnectionStatus connectState;

    sme_trace_entry((TR_SCAN, "monitoring_ext_full_scan(ssidCount:%d)", req->ssidCount));
    connectState = ns_get_connection_status(context);

    /*
     * The cancellingScans flag does not clear unless an internal scan request is made.
     * It is possible that a scan may be cancelled (e.g. a probe scan due to a disconnect
     * request) that will not be followed by a subsequent internal scan request. This would
     * prevent all external scan requests from being honoured.
     *
     * However, if we receive an external scan request and the cancelling flags is set - unless
     * the scan was cancelled in order to do some other scan, there is no reason not to honour
     * the external scan request. If the scanCancelRequesterId is FSM_TERMINATE then we know that
     * no subsequent internal scan is being scheduled so can safely clear the cancelling flag!
     */
    if (fsmData->cancellingScans && fsmData->scanCancelRequesterId == FSM_TERMINATE)
    {
        sme_trace_debug((TR_SCAN, "monitoring_ext_full_scan: resetting the cancellingScans flag"));
        fsmData->cancellingScans = FALSE;
    }

    /*
     * Do not scan either:
     *     a) when paused, or,
     *     b) when connected AND auto scan installed AND NOT periodic traffic AND NOT continuous traffic
     *     c) cancelling
     * Simply respond immediately and allow the external entity to read any
     * cached scan results.
     */
    if (fsmData->cancellingScans ||
        fsmData->paused ||
        (fsmData->autoScanCount && connectState != ns_ConnectionStatus_Disconnected &&
         cfg->coexInfo.currentTrafficType != unifi_TrafficOccasional &&
         cfg->coexInfo.currentTrafficType != unifi_TrafficBursty &&
         !req->forceScan)
       )
    {
        sme_trace_debug_code(
            if (fsmData->cancellingScans)
            {
                sme_trace_debug((TR_SCAN, "cancelling external scan request"));
            }
            else if (fsmData->paused)
            {
                sme_trace_debug((TR_SCAN, "scanning operations suspended so ignoring external scan request"));
            }
            else if (fsmData->autoScanCount && connectState != ns_ConnectionStatus_Disconnected)
            {
                sme_trace_debug((TR_SCAN, "auto scan running, not in disconnected state and not unifi_TrafficOccasional and not unifi_TrafficBursty - ignoring external scan request"));
            }
        )

        call_unifi_mgt_scan_full_cfm(context, req->appHandle, unifi_Unavailable);
        free_unifi_mgt_scan_full_req_contents(req);
        return;
    }

    {
        DataReference   scanReqIEs = {0, 0};
        unifi_DataBlock localchannelList;
        localchannelList.length = req->channelListCount;
        localchannelList.data = req->channelList;

        sme_trace_debug_code
        (
            if (fsmData->autoScanCount)
            {
                sme_trace_debug((TR_SCAN, "auto scan installed but in disconnected state - honoring external scan request"));
            }
            else
            {
                sme_trace_debug((TR_SCAN, "auto scan is disabled - honoring external scan request; %s disconnected",
                                connectState == ns_ConnectionStatus_Disconnected ? "" : "NOT "));
            }
        )

        /* Flush the old scan results */
        if (cfg->scanConfig.scanCfg[fsmData->currentUsability].validitySeconds == 0)
        {
            sme_trace_info((TR_SCAN, "monitoring_ext_full_scan() Flush srs_delete_unconnected_scan_results()"));
            srs_delete_unconnected_scan_results(context);
        }

        fsmData->mmiScanRequested      = TRUE;
        fsmData->cfmAppHandle          = req->appHandle;
        fsmData->scanCancelRequesterId = FSM_TERMINATE;
        fsmData->scanRequesterId       = FSM_TERMINATE;
        fsmData->probeScanning         = FALSE;
        fsmData->probeSSID.length      = 0;
        fsmData->scanCancelRequesterId = FSM_TERMINATE;
        fsmData->scanRequesterId       = FSM_TERMINATE;
        fsmData->probeScanning         = FALSE;
        fsmData->scanStopCondition     = FALSE;
        fsmData->scanStopConditionMet  = FALSE;

        /*
         * If we've been asked to scan for a particular SSID we'll need to
         * generate an IE for the firmware. If no SSID is present in the
         * request, this function will return with a NULL dataReference
         */
        scanReqIEs = ie_create_scan_req_ies(context, req->ssidCount, req->ssid, req->probeIeLength, req->probeIe);

        sme_trace_debug_code(
            if (scanReqIEs.dataLength)
            {
                sme_trace_debug((TR_SCAN, "MGT Scan request contains an SSID"));
            }
            else
            {
                sme_trace_debug((TR_SCAN, "MGT Scan does NOT contain an SSID"));
            }
        )
        /* Kick off a full scan */
        if (create_scan_config(context, 0, scanReqIEs, req->bssid, FALSE, FALSE, FALSE, req->bssType, req->scanType, &localchannelList))
        {
            perform_scan(context);
        }
        else
        {
            sme_trace_error((TR_SCAN, "Scan config failed - not performing scan, sending cfm(Err) to MGT SAP"));
            fsmData->mmiScanRequested = FALSE;
            call_unifi_mgt_scan_full_cfm(context, req->appHandle, unifi_Error);
        }
    }

    free_unifi_mgt_scan_full_req_contents(req);
}

/**
 * @brief
 *   Process scan indications
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void mlme_scan_ind(FsmContext* context, const MlmeScanInd_Evt* ind)
{
    FsmData       * fsmData            = FSMDATA;
    CsrBool         bScanResultDeleted = FALSE;
    SmeConfigData * cfg                = get_sme_config(context);
    srs_scan_data * scanData;

    sme_trace_entry((TR_SCAN, "mlme_scan_ind(%s,%s,%s(%d))", trace_unifi_MACAddress(ind->bssid, getMacAddressBuffer(context)),
                                                         fsmData->stopping?"stopping":"running",
                                                         fsmData->paused?"paused":"unpaused",
                                                         fsmData->paused));

    /* Following function just returns if TrustLevel= unifi_Trust_Disabled */
    regdom_process_beacon(context,
                          &fsmData->regulatoryData,
                          ind->bssType,
                          ind->channel,
                          ind->channelFrequency,
                          ind->informationElements);

    scanData = srs_create_scan_result(context, 0, /* not owned by the FW */
                                 ind->informationElements,
                                 ind->ifIndex,
                                 ind->bssType,
                                 ind->bssid,
                                 ind->beaconPeriod,
                                 ind->timestamp,
                                 ind->localTime,
                                 ind->channel,
                                 ind->channelFrequency,
                                 ind->capabilityInformation,
                                 ind->rssi,
                                 ind->snr);

    if (scanData == NULL)
    {
        sme_trace_warn((TR_SCAN, "srs_create_scan_result returned NULL"));
        return;
    }

    if (fsmData->probeScanning)
    {
        srs_update_join_scan_result(context, scanData);

        fsmData->probeBeaconPeriodMs = ind->beaconPeriod;
    }
    srs_crop_scan_result_list(context, &fsmData->probeSSID, scanData, &bScanResultDeleted);

    if (!bScanResultDeleted)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndScanResult, &appHandles);
        sme_trace_info((TR_SCAN, "mlme_scan_ind: unifi_mmi_scan_result_ind: BSSID %s [%d] SSID %s",
            trace_unifi_MACAddress(scanData->scanResult.bssid, getMacAddressBuffer(context)),
            ind->channel,
            trace_unifi_SSID(&scanData->scanResult.ssid, getSSIDBuffer(context))
            ));

        if (appHandleCount)
        {
            call_unifi_mgt_scan_result_ind(context, appHandleCount, appHandles, &scanData->scanResult);
        }
    }

    if (!bScanResultDeleted && fsmData->scanStopCondition == unifi_ScanStopFirstResult)
    {
        sme_trace_debug((TR_SCAN, "mlme_scan_ind: only want single result so cancelling remaining scan"));
        send_mlme_scan_cancel_req(context);
        fsmData->scanStopConditionMet = TRUE;
    }
    else if (!bScanResultDeleted && fsmData->scanStopCondition == unifi_ScanStopSatisfactoryRoam)
    {
        if (scanData->scanResult.usability == unifi_Satisfactory && validate_ap(context, scanData, cfg))
        {
            sme_trace_debug((TR_SCAN, "mlme_scan_ind: Found Roaming candidate %s::%s ",
                    trace_unifi_SSID(&scanData->scanResult.ssid, getSSIDBuffer(context)),
                    trace_unifi_MACAddress(scanData->scanResult.bssid, getMacAddressBuffer(context))));
            send_mlme_scan_cancel_req(context);
            fsmData->scanStopConditionMet = TRUE;
        }
    }

}

/**
 * @brief
 *   Process scan indications
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void mlme_autonomous_scan_ind(FsmContext* context, const MlmeAutonomousScanInd_Evt* ind)
{
    FsmData       * fsmData = FSMDATA;
    SmeConfigData * cfg = get_sme_config(context);
    srs_scan_data * scanData;
    CsrBool         bScanResultDeleted;

    sme_trace_entry((TR_SCAN, "mlme_autonomous_scan_ind(%s,%s,%s(%d))", trace_unifi_MACAddress(ind->bssid, getMacAddressBuffer(context)),
                                                                    fsmData->stopping?"stopping":"running",
                                                                    fsmData->paused?"paused":"unpaused",
                                                                    fsmData->paused));

    /* Check for deleted AP data */
    if (ind->beaconPeriod == 0)
    {
        verify(TR_SCAN, ind->informationElements.dataLength == 0);
        /* This will call unifi_mmi_scan_result_ind() if needed */
        (void)srs_delete_scan_result_profile(context, &ind->bssid, FALSE);
        return;
    }

    /* Following function just returns if TrustLevel=0 (802.11d Operation Disabled) */
    regdom_process_beacon(context,
                          &fsmData->regulatoryData,
                          ind->bssType,
                          ind->channel,
                          ind->channelFrequency,
                          ind->informationElements);

    sme_trace_debug((TR_SCAN, "mlme_autonomous_scan_ind(%s) Add/Update Scan Result", trace_unifi_MACAddress(ind->bssid, getMacAddressBuffer(context))));
    scanData = srs_create_scan_result(context,
                                 ind->autonomousScanId, /* owned by the FW */
                                 ind->informationElements,
                                 cfg->smeConfig.ifIndex,
                                 ind->bssType,
                                 ind->bssid,
                                 ind->beaconPeriod,
                                 ind->timestamp,
                                 ind->localTime,
                                 ind->channel,
                                 ind->channelFrequency,
                                 ind->capabilityInformation,
                                 ind->rssi,
                                 ind->snr);

    /* Only update autonomous scanning when not manually scanning. */
    if (fsm_current_state(context) != FSMSTATE_scanning &&
        fsmData->regulatoryData.ChannelsInUseList.listChangedFlag == TRUE)
    {
        sme_trace_info((TR_SCAN, "mlme_autonomous_scan_ind() Regulatory subsystem has changed channel config - updating"));

        if (!fsmData->initialScanInProgress && !fsmData->paused)
        {
            sme_trace_info((TR_SCAN, "mlme_autonomous_scan_ind() Channel Configuration has changed - reinstalling auto scan(s)"));
            stop_monitoring(context);
            start_monitoring(context);
        }
        else
        {
            sme_trace_info((TR_SCAN, "mlme_autonomous_scan_ind() Channel Configuration has changed - but %s so defering auto-scan installation",
                            fsmData->initialScanInProgress?"performing startup scans":"scanning operations suspended"));
        }
    }

    if (scanData == NULL)
    {
        sme_trace_warn((TR_SCAN, "srs_create_scan_result returned NULL"));
        return;
    }


    srs_crop_scan_result_list(context, &fsmData->probeSSID, scanData, &bScanResultDeleted);

    if (!bScanResultDeleted)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndScanResult, &appHandles);
        sme_trace_debug((TR_SCAN, "mlme_autonomous_scan_ind(%d): unifi_mmi_scan_result_ind: BSSID %s [%d,%d,%d] SSID %s",
            ind->autonomousScanId,
            trace_unifi_MACAddress(scanData->scanResult.bssid, getMacAddressBuffer(context)),
            ind->channel,
            ind->rssi,
            ind->snr,
            trace_unifi_SSID(&scanData->scanResult.ssid, getSSIDBuffer(context))
            ));

        if (appHandleCount)
        {
            call_unifi_mgt_scan_result_ind(context, appHandleCount, appHandles, &scanData->scanResult);
        }
    }

}

/**
 * @brief
 *   Add Auto scan cfm
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void mlme_add_auto_scan_cfm(FsmContext* context, const MlmeAddAutonomousScanCfm_Evt* cfm)
{
    sme_trace_entry((TR_SCAN, "mlme_add_auto_scan_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    sme_trace_error_code(
        if (cfm->resultCode != ResultCode_Success)
        {
            sme_trace_error((TR_SCAN, "mlme_add_auto_scan_cfm(%s) for ID %d",
                trace_ResultCode(cfm->resultCode), cfm->autonomousScanId));
        })
}

/**
 * @brief
 *   Del Auto scan cfm
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void mlme_del_auto_scan_cfm(FsmContext* context, const MlmeDelAutonomousScanCfm_Evt* cfm)
{
    sme_trace_entry((TR_SCAN, "mlme_del_auto_scan_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    sme_trace_error_code(
        if (cfm->resultCode != ResultCode_Success)
        {
            sme_trace_error((TR_SCAN, "mlme_del_auto_scan_cfm(%s)", trace_ResultCode(cfm->resultCode)));
        })
}


/* ====================[ EVENT HANDLERS: MONITORING STATE ]================== */

/**
 * @brief
 *   Scan requested
 *
 * @param[in] context : FSM context
 * @param[in] req     : event
 *
 * @return
 *  Void
 */
static void monitoring_scan(FsmContext *context, const SmScanReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;
    DataReference scanReqIEs = {0, 0};

    sme_trace_info((TR_SCAN, "%sScan request received from %s: channel is %d wanting %s, %s",
                    req->joinReq?"JOIN ":"",
                    fsm_process_name_by_id(context, req->common.sender_),
                    req->channel,
                    req->singleChannel?"single-channel":"multi-channel",
                    trace_unifi_ScanStopCondition(req->scanStopCondition)));
    /*
     * Because we may have saved scan requests but actioned a scan cancel
     * we need to go through and decline all scan requests that were made
     * prior to the scan cancel.
     */
    if (fsmData->cancellingScans)
    {
        sme_trace_debug
        ((
            TR_SCAN,
            "A scan cancel was issued earlier (by %s), checking if this scan request is older than the cancel",
            fsm_process_name_by_id(context, fsmData->scanCancelRequesterId)
        ));

        if (req->cloakedSsidScan)
        {
            sme_trace_debug((TR_SCAN,"monitoring_scan() Scan cloaked ssid scan for later. DO NOT CANCEL!"));
            fsm_saved_event(context, (const FsmEvent*)req);
            return;
        }

        if (CsrTimeLt(req->reqTime, fsmData->scanCancelTime))
        {
            sme_trace_warn
            ((
                TR_SCAN,
                "%-24s: saved scan from %s preempted by a later scan cancel - scan req at %ld, cancellation received %ld",
                fsm_current_state_name(context),
                fsm_process_name_by_id(context, req->common.sender_),
                req->reqTime, fsmData->scanCancelTime
            ));

            if (fsmData->initialScanInProgress)
            {
                fsmData->initialScanInProgress--;
                CsrPfree(req->ssid);
                return;
            }

            /*
             * Hopefully there will not be any saved events for the entity
             * requesting the cancel - this, however, can happen so check
             * to make sure we don't respond to an entity that's not expecting
             * us to
             */
            if (fsmData->scanCancelRequesterId == FSM_TERMINATE ||
                fsmData->scanCancelRequesterId != req->common.sender_)
            {
                send_sm_scan_cfm(context, req->common.sender_, FALSE, ResultCode_UnspecifiedFailure, 0);
            }
            CsrPfree(req->ssid);
            return;
        }
        else
        {
            sme_trace_debug
            ((
                TR_SCAN,
                "No, this scan request was sent after the cancel request - actioning"
            ));
            fsmData->cancellingScans       = FALSE;
            fsmData->scanCancelRequesterId = FSM_TERMINATE;
        }
    }

    fsmData->scanCancelRequesterId = FSM_TERMINATE;
    fsmData->scanRequesterId       = req->common.sender_;
    fsmData->probeScanning         = req->joinReq;
    fsmData->scanStopCondition     = req->scanStopCondition;
    fsmData->scanStopConditionMet  = FALSE;

    if (fsmData->probeScanning)
    {
        sme_trace_debug ((TR_SCAN, "Scan request was for a network join : ssdiCount = %d", req->ssidCount));
        /* reset the join */
        srs_reset_join_scan_data(context);
        fsmData->probeSSID.length = 0;
        if (req->ssidCount)
        {
            fsmData->probeSSID = req->ssid[0];
        }
    }

    scanReqIEs = ie_create_scan_req_ies(context, req->ssidCount, req->ssid, 0, NULL);

    if (create_scan_config(context,
                           req->channel,
                           scanReqIEs,
                           req->addr,
                           req->joinReq,
                           req->singleChannel,
                           req->scanStopCondition,
                           req->bssType,
                           unifi_ScanAll,
                           &NullDataBlock))
    {
        sme_trace_debug ((TR_SCAN, "Performing scan"));
        perform_scan(context);
    }
    else
    {
        sme_trace_warn((TR_SCAN, "Unable to create scan config - sending scan_cfm to %s with error",
                        fsm_process_name_by_id(context, req->common.sender_)));
        send_sm_scan_cfm(context, req->common.sender_, FALSE, ResultCode_UnspecifiedFailure, 0);

        if (fsmData->initialScanInProgress) fsmData->initialScanInProgress--;
    }
    CsrPfree(req->ssid);
}

/**
 * @brief
 *   Idle pause transition
 *
 * @param[in] context : FSM context
 * @param[in] req     : event
 *
 * @return
 *  Void
 */
static void monitoring_pause(FsmContext *context, const SmPauseReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_info((TR_SCAN,
                     "monitoring_pause: received pause request from %s entity. Current Pause Count = %d",
                     fsm_process_name_by_id(context, req->common.sender_), fsmData->paused));

    if (!fsmData->paused)
    {
        stop_monitoring(context);
    }
    fsmData->paused++;

    send_sm_pause_cfm(context, req->common.sender_);
}

/**
 * @brief
 *   Idle unpause transition
 *
 * @param[in] context : FSM context
 * @param[in] req     : event
 *
 * @return
 *  Void
 */
static void monitoring_unpause(FsmContext *context, const SmUnpauseReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_info((TR_SCAN,
                     "monitoring_unpause: received unpause request from %s entity. Current Pause Count = %d",
                     fsm_process_name_by_id(context, req->common.sender_), fsmData->paused));

    if (fsmData->paused)
    {
        fsmData->paused--;
        if (!fsmData->paused)
        {
            sme_trace_info((TR_SCAN, "Re-enabling background auto-scan"));

            start_monitoring(context);
        }
    }
    else
    {
        sme_trace_warn((TR_SCAN, "Unpause request received (entity = %s) when not paused",
                        fsm_process_name_by_id(context, req->common.sender_)));
    }
    send_sm_unpause_cfm(context, req->common.sender_);
}


/**
 * @brief
 *   Update the autonomous scanning
 *
 * @par Description
 *    Something has caused a trigger to reevaluate the current
 *    Autonomous Scanning
 *
 * @param[in] context : FSM context
 * @param[in] ind     : event
 *
 * @return
 *  Void
 */
static void monitoring_update(FsmContext *context, const SmScanUpdateInd_Evt *ind)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SCAN, "monitoring_update(%d)", fsmData->paused));
    if (!fsmData->paused)
    {
        sme_trace_entry((TR_SCAN, "scanUpdateAction(%d)", ind->scanUpdateAction));
        switch(ind->scanUpdateAction)
        {
        case unifi_ScanStopOnly:
            stop_monitoring(context);
            break;
        case unifi_ScanStartOnly:
            start_monitoring(context);
            break;
        case unifi_ScanStopStart:
            stop_monitoring(context);
            start_monitoring(context);
            break;
        case unifi_ScanCloakedSsidRescan:
            (void)srs_scan_cloaked_ssids(context);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief
 *   Update the autonomous scanning
 *
 * @par Description
 *    Something has caused a trigger to reevaluate the current
 *    Autonomous Scanning
 *
 * @param[in] context : FSM context
 * @param[in] ind     : event
 *
 * @return
 *  Void
 */
static void scan_quality_update(FsmContext *context, const SmScanQualityInd_Evt *ind)
{
    FsmData           * fsmData          = FSMDATA;
    CsrBool             autoScanReconfig = FALSE;
    CsrBool             startFallBackTimer = FALSE;

    sme_trace_entry((TR_SCAN, ">> scan_quality_update(%s)", trace_unifi_BasicUsability(ind->usability)));

    if (ns_get_connection_status(context) == ns_ConnectionStatus_Connected)
    {
        /*
         * We have to handle 'link unusable' quite carefully... In this mode we
         * 'panic scan' (more aggresive scan) for a period of time and then relax
         * the scan to be like that used when the quality is 'poor' - this is
         * done to stop us making a bad situation worse by going off channel a lot.
         */
        if (ind->usability == unifi_Unusable)
        {
            /*
             * If this is the onset of 'unusable' link quality
             * start the fall back timer
             * and set the current link quality to 'unusable'
             */
            if (fsmData->currentUsability != unifi_Unusable)
            {
                sme_trace_debug((TR_SCAN, "scan_quality_update: start fallback timer"));
                startFallBackTimer = TRUE;

                fsmData->currentUsability      = unifi_Unusable;
                autoScanReconfig               = TRUE;
            }
        }
        else
        {
            /*
             * If the link quality has changed, we need to reconfigure the auto scan timings
             */
            if (fsmData->currentUsability != ind->usability)
            {
                if (fsmData->currentUsability == unifi_Unusable && fsmData->aggresiveScanOverride)
                {
                    sme_trace_entry((TR_SCAN, "fsm_remove_timer fallBackTimer"));
                    fsm_remove_timer(context, fsmData->fallBackTimer);
                    fsmData->aggresiveScanOverride = FALSE;
                }

                sme_trace_info((TR_SCAN, "scan_quality_update: link quality has changed %s -> %s",
                                trace_unifi_BasicUsability(fsmData->currentUsability),
                                trace_unifi_BasicUsability(ind->usability)));
                fsmData->currentUsability = ind->usability;
                autoScanReconfig          = TRUE;
            }
        }

        /*
         * Only reinstall the auto scan if one is already running - if not (i.e.
         * auto-scan is paused) then don't worry, once the scan manager is
         * unpaused, an auto-scan with the correct parameters will be installed
         */
        if (autoScanReconfig && !fsmData->paused)
        {
            sme_trace_info((TR_SCAN, "scan_quality_update: Reconfiguring autonomous scan for link quality '%s'",
                            trace_unifi_BasicUsability(fsmData->currentUsability)));
            stop_monitoring(context);
            start_monitoring(context);
        }

        /* this must be started after the auto scan has been reconfigured */
        if(startFallBackTimer)
        {
            send_sm_fallback_expiry_timer(context,
                                          fsmData->fallBackTimer,
                                          60000,
                                          1000);

            fsmData->aggresiveScanOverride = TRUE;
        }
    }
    else
    {
        sme_trace_error((TR_SCAN, "scan_quality_update: received link quality update when not connected to a network"));
    }
    sme_trace_entry((TR_SCAN, "<< scan_quality_update(%s)", trace_unifi_BasicUsability(ind->usability)));
}


/**
 * @brief
 *   Update the autonomous scanning
 *
 * @par Description
 *    Something has caused a trigger to reevaluate the current
 *    Autonomous Scanning
 *
 * @param[in] context : FSM context
 * @param[in] ind     : event
 *
 * @return
 *  Void
 */
static void fallback_timer_expired(FsmContext *context, const SmFallbackExpiryTimer_Evt *ind)
{
    FsmData           * fsmData          = FSMDATA;

    sme_trace_entry((TR_SCAN, ">> fallback_timer_expired"));
    sme_trace_info((TR_SCAN, "fallback_timer_expired: link has been unusable for a long time, cease panic scanning"));

    fsmData->aggresiveScanOverride = FALSE;
    fsmData->currentUsability = unifi_Poor;

    if (!fsmData->paused)
    {
        sme_trace_info((TR_SCAN, "scan_quality_update: Reconfiguring autonomous scan for link quality '%s'",
                        trace_unifi_BasicUsability(fsmData->currentUsability)));
        stop_monitoring(context);
        start_monitoring(context);
    }

    sme_trace_entry((TR_SCAN, "<< fallback_timer_expired"));
}


/**
 * @brief
 *   Idle disable transition
 *
 * @par Description
 *   Disables the scan manager
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void monitoring_stop(FsmContext *context, const CoreStopReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SCAN, "monitoring_stop(%d)", fsmData->paused));

    if (!fsmData->paused)
    {
        stop_monitoring(context);
    }

    if (fsmData->regulatoryData.chkChanXpryTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->regulatoryData.chkChanXpryTimerId);
        fsmData->regulatoryData.chkChanXpryTimerId.uniqueid = 0;
    }

    srs_reset_scan_result_storage(context);

    sme_trace_info
    ((
        TR_SCAN, "[1] Sending stop confirm to %s",
        fsm_process_name_by_id(context, req->common.sender_)
    ));

    send_core_stop_cfm(context, req->common.sender_, unifi_Success);

    fsm_next_state(context, FSMSTATE_stopped);
}

/* =====================[ EVENT HANDLERS: SCANNING STATE ]=================== */

/**
 * @brief
 *   Disables the scan manager
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void scanning_stop(FsmContext *context, const CoreStopReq_Evt *req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SCAN, "scanning_stop()"));
    fsmData->stopping = TRUE;
    fsmData->cancellingScans = TRUE;
    fsmData->scanCancelTime  = fsm_get_time_of_day_ms(context);

    send_mlme_scan_cancel_req(context);

    /* Save the event for processing when the scanning is canceled */
    fsm_saved_event(context, (FsmEvent*) req);
}

/**
 * @brief
 *   Cancel the scan
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void scanning_cancel(FsmContext *context, const SmScanCancelInd_Evt *ind)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SCAN, "scanning_cancel(%s)", ind->preEmptiveScanFollowing?"true":"false"));

    sme_trace_warn
    ((
        TR_SCAN, "%-24s: %s process has requested the current scan "
        "be cancelled.", fsm_current_state_name(context),
        fsm_process_name_by_id(context, ind->common.sender_)
    ));

    /*
     * If the cancelling entity is queuing a fresh directed scan then we need
     * to keep track of his ID so that we don't send a directed-scan-cfm to
     * him (to notify him that the scan has completed)
     */
    if (ind->preEmptiveScanFollowing)
    {
        fsmData->scanCancelRequesterId = ind->common.sender_;
    }
    else
    {
        fsmData->scanCancelRequesterId = FSM_TERMINATE;
    }
    fsmData->cancellingScans = TRUE;
    fsmData->scanCancelTime  = ind->indTime;

    if (fsmData->probeScanning)
    {
        fsmData->probeScanning = FALSE;
        fsmData->probeSSID.length = 0;
    }
    send_mlme_scan_cancel_req(context);
}


/**
 * @brief
 *   Scan Complete
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : event
 *
 * @return
 *   void
 */
static void mlme_scan_complete(FsmContext *context, const MlmeScanCfm_Evt *cfm)
{
    FsmData * fsmData = FSMDATA;

    sme_trace_entry((TR_SCAN, "mlme_scanning_complete(%s: initialScan=%s, stopping=%s, mmiScanRequested=%d)",
                    trace_ResultCode(cfm->resultCode),
                    fsmData->initialScanInProgress?"yes":"no",
                    fsmData->stopping?"yes":"no",
                    fsmData->mmiScanRequested));

    /* Try next scan config */
    fsmData->scanConfigCurrent++;
    if (perform_next_scan(context) == FALSE)
    {
        /* Cleanup Scans FIRST. as the scan configs may be reused */
        cleanup_scan_configs(context);

        /* last scan Complete */
        notify_manual_scan_complete(context, cfm->resultCode);

        sme_trace_info((TR_SCAN, "mlme_scan_complete() initialScanInProgress == %d, deferredAutoScanInstall = %d, paused = %d, stopping = %d", fsmData->initialScanInProgress, fsmData->deferredAutoScanInstall, fsmData->paused, fsmData->stopping));

        if (!fsmData->stopping)
        {
            if ((fsmData->initialScanInProgress == 0) && fsmData->deferredAutoScanInstall && !fsmData->paused)
            {
                sme_trace_info((TR_SCAN, "mlme_scan_complete() Install of autonomous scan was deferred, installing now initial scans have completed"));
                start_monitoring(context);
                fsmData->deferredAutoScanInstall = FALSE;
            }

            if ( fsmData->regulatoryData.ChannelsInUseList.listChangedFlag == TRUE )
            {
                sme_trace_info((TR_SCAN, "mlme_scan_complete() Regulatory subsystem has changed channel config - updating"));

                if (!fsmData->initialScanInProgress && !fsmData->paused)
                {
                    sme_trace_info((TR_SCAN, "mlme_scan_complete() Channel Configuration has changed - reinstalling auto scan(s)"));
                    stop_monitoring(context);
                    start_monitoring(context);
                }
                else
                {
                    sme_trace_info((TR_SCAN, "mlme_scan_complete() Channel Configuration has changed - but %s so deferring auto-scan installation",
                                    fsmData->initialScanInProgress?"performing startup scans":"scanning operations suspended"));
                }
            }
        }

        fsm_next_state(context, FSMSTATE_monitoring);
    }
}

/**
 * @brief
 *   get first scan result
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void scan_results_get_req(FsmContext* context, const UnifiMgtScanResultsGetReq_Evt* req)
{
    unifi_Status status;
    CsrUint16         scanResultsCount = 0;
    unifi_ScanResult* scanResults;

    CsrUint16 bufferSize = (CsrUint16) (sizeof(unifi_ScanResult) * srs_scanresults_count(context));

    sme_trace_entry((TR_SCAN, "scan_results_get_req(%d bytes required)", bufferSize));

    if (FSMDATA->paused)
    {
        sme_trace_debug((TR_SCAN, "Scanning operations currently paused - not ageing scan data"));
    }
    else
    {
        srs_expire_old_scan_data(context);
    }

    bufferSize = (CsrUint16)(sizeof(unifi_ScanResult) * srs_scanresults_count(context));
    scanResults =  (unifi_ScanResult*)CsrPmalloc(bufferSize);
    sme_trace_debug((TR_SCAN, "scan_results_get_req() Allocating %d bytes", bufferSize));


    status = srs_copy_scanresults(context, &scanResultsCount, scanResults, bufferSize, FALSE);

    sme_trace_debug((TR_SCAN, "scan_results_get_req()  %d scan results copied", scanResultsCount));
    call_unifi_mgt_scan_results_get_cfm(context, req->appHandle, status, scanResultsCount, scanResults);
    CsrPfree(scanResults);
}

/**
 * @brief
 *   get first scan result
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void bounce_scan_results_req(FsmContext* context, const UnifiMgtScanResultsGetReq_Evt* req)
{
    sme_trace_entry((TR_SCAN, "bounce_scan_results_req()"));
    call_unifi_mgt_scan_results_get_cfm(context, req->appHandle, unifi_WifiOff, 0, NULL);
}

/**
 * @brief
 *   Flush the scan results
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void scan_results_flush_req(FsmContext* context, const UnifiMgtScanResultsFlushReq_Evt* req)
{
    sme_trace_entry((TR_SCAN, "scan_results_flush_req()"));

    srs_delete_unconnected_scan_results(context);

    call_unifi_mgt_scan_results_flush_cfm(context, req->appHandle, unifi_Success);
}

static void start_roaming_scan(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    RoamingChannelSet* roamlist = roaming_channel_list_get(context, &cfg->connectionInfo.ssid);

    DataReference         csrAutoIe;
    DataReference         csrAutoIeTmp;
    DataReference         channels       = {0, 0};
    unifi_ScanConfigData*  scanData = &cfg->roamingConfig.roamScanCfg[fsmData->currentUsability];

    sme_trace_entry((TR_SCAN, "start_roaming_scan: Usability is %s", trace_unifi_BasicUsability(fsmData->currentUsability) ));

    if (roamlist == NULL)
    {
        return;
    }

    /*
     * If the scan interval is zero then do not install an autonomous scan -
     * check if one is already running, if so, delete it.
     */
    if (scanData->intervalSeconds == 0)
    {
        sme_trace_info((TR_SCAN, "start_roaming_scan: Autonomous scan interval is zero so not installing new auto-scan"));
        return;
    }

    csrAutoIe = build_autonomous_scan_ie(context, TRUE, scanData, &cfg->connectionInfo.ssid);

    /* Add any regdomain info needed */
    csrAutoIeTmp = csrAutoIe;
    csrAutoIe = regdom_create_ie_dr(context, csrAutoIeTmp, FALSE);
    pld_release(getPldContext(context), csrAutoIeTmp.slotNumber);

    pld_store(getPldContext(context), roamlist->channelList, roamlist->channelListCount, &channels.slotNumber);
    channels.dataLength = roamlist->channelListCount;

    /* Needs 5Ghz support */
    send_mlme_add_autonomous_scan_req( context,
                                       channels,
                                       csrAutoIe,
                                       ROAM_SCAN_AUTONOMOUS_ID,
                                       unifi_GHZ_2_4,
                                       0,
                                       BssType_Infrastructure,
                                       BroadcastBssid,
                                       ScanType_ActiveScan,
                                       SCANMGR_TMPFIXED_PROBE_DELAY_USEC,
                                       scanData->minActiveChannelTimeTu,
                                       scanData->maxActiveChannelTimeTu);

    fsmData->oneshotRoamingScanInstalled = TRUE;
}

static void stop_roaming_scan(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    if (fsmData->oneshotRoamingScanInstalled)
    {
        fsmData->oneshotRoamingScanInstalled = FALSE;
        send_mlme_del_autonomous_scan_req(context, ROAM_SCAN_AUTONOMOUS_ID);
    }
}

/**
 * @brief
 *   Install a roaming scan
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
void install_roaming_scan(FsmContext* context)
{
    SmeConfigData* cfg = get_sme_config(context);
    srs_scan_data* joinScan = srs_get_join_scan_parameters(context);

    /* This can be called from Scan Storage so for safety I am saving and restoring the current context */
    FsmInstanceEntry* currentInstance = fsm_save_current_instance_by_id(context, getSmeContext(context)->scanManagerInstance);

    sme_trace_entry((TR_SCAN, "install_roaming_scan()"));

    stop_roaming_scan(context);

    /* Should we install a roaming scan */
    if (!cfg->roamingConfig.disableRoamScans &&
        ns_get_connection_status(context) == ns_ConnectionStatus_Connected &&
        joinScan != NULL &&
        joinScan->scanResult.bssType == unifi_Infrastructure)
    {
        start_roaming_scan(context);
    }

    fsm_restore_current_instance(context, currentInstance);
}

/**
 * @brief
 *   handles signals from an adjunct technology module
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : event
 *
 * @return
 *   void
 */
static void adjunct_technology_signal_handler( FsmContext* context, const SmAdjunctTechSignalInd_Evt* ind )
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    regdom_process_location_signal( context, &fsmData->regulatoryData, cfg->smeConfig.countryCode);
}

/* TIMER HANDLING FUNCTION DEFINITIONS **********************************************/
static void channels_expiry_handler( FsmContext* context,
                                     const SmCheckChannelsExpiryTimer_Evt* req )
{
    FsmData* fsmData = FSMDATA;
    CsrBool hasAnyChannelChanged=FALSE;
    CsrBool ChannelsAbout2Xpire=FALSE;
    FsmTimerId timerId;

    sme_trace_entry(( TR_SCAN, ">> channels_expiry_handler(): ******* Check_Channel_Expiry Timer Expired" ));

    /* Cancel Timer */
    timerId = fsmData->regulatoryData.chkChanXpryTimerId;
    if ( timerId.uniqueid != 0 )
    {
         fsm_remove_timer( context, timerId );
         timerId.uniqueid = 0;
    }

    /* Re-Start Channel_Check_Expiry Timer */
    send_sm_check_channels_expiry_timer(context,
                                      timerId,
                                      CHECK_CHANNEL_EXPIRY_TIMER_DURATION,
                                      CHECK_CHANNEL_EXPIRY_TIMER_TOLERANCE );
    fsmData->regulatoryData.chkChanXpryTimerId = timerId;

    ChannelsAbout2Xpire= regdom_check_for_scan_refresh(context, &fsmData->regulatoryData);
    if (ChannelsAbout2Xpire && !fsmData->paused)
    {
        sme_trace_debug(( TR_SCAN, "channels_expiry_handler(): Refreshing Autonomous Scan" ));
        stop_monitoring(context);
        start_monitoring(context);
    }
    else
    {
        hasAnyChannelChanged = regdom_process_expired_channels(context, &fsmData->regulatoryData);
        if (hasAnyChannelChanged  && !fsmData->paused)
        {
            stop_monitoring(context);
            start_monitoring(context);
        }
    }

    sme_trace_entry(( TR_SCAN, "<< channels_expiry_handler()" ));
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

unifi_ScanConfigData* get_current_scan_data(FsmContext* context, unifi_ScanConfigData** roamScanData)
{
    SmeConfigData* cfg = get_sme_config(context);
    require(TR_SCAN, context->instanceArray[getSmeContext(context)->scanManagerInstance].state != FSM_TERMINATE);

    if (ns_get_connection_status(context) == ns_ConnectionStatus_Connected)
    {
        unifi_BasicUsability currentUsability = (fsm_get_params_by_id(context, getSmeContext(context)->scanManagerInstance, FsmData))->currentUsability;
        sme_trace_debug((TR_SCAN, "get_current_scan_data: usability %s",
            trace_unifi_BasicUsability(currentUsability)));
        *roamScanData = &cfg->roamingConfig.roamScanCfg[currentUsability];
        return &cfg->scanConfig.scanCfg[currentUsability];
    }

    sme_trace_debug((TR_SCAN, "get_current_scan_data: no connection so usability DISCONNECTED"));
    *roamScanData = NULL;
    return &cfg->scanConfig.scanCfg[unifi_NotConnected];
}

void reset_autonomous_scan(FsmContext* context)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->scanManagerInstance, FsmData);

    sme_trace_entry((TR_SCAN, "reset_autonomous_scan()"));

    require(TR_SCAN, context->instanceArray[getSmeContext(context)->scanManagerInstance].state != FSM_TERMINATE);

    fsmData->autoScanCount = 0;
    srs_take_scan_results_ownership(context, 0);
}

RegDomData * get_regulatory_data(FsmContext* context)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->scanManagerInstance, FsmData);

    return (&fsmData->regulatoryData);
}

CsrBool cloaked_scanning_enabled(FsmContext* context)
{
    return fsm_get_params_by_id(context, getSmeContext(context)->scanManagerInstance, FsmData)->initialCloakedScansDone;
}

/* FSM DEFINITION ***********************************************************/

static const FsmEventEntry stoppedTransitions[] =
{
                       /* Signal Id,                    Function */
    fsm_event_table_entry(CORE_START_REQ_ID,            stopped_start),
    fsm_event_table_entry(SM_SCAN_REQ_ID,               bounce_scan_req),
    fsm_event_table_entry(UNIFI_MGT_SCAN_FULL_REQ_ID,   bounce_mmi_full_scan),
    fsm_event_table_entry(UNIFI_MGT_SCAN_RESULTS_GET_REQ_ID, bounce_scan_results_req),
    fsm_event_table_entry(SM_CHECK_CHANNELS_EXPIRY_TIMER_ID,   channels_expiry_handler),
};

static const FsmEventEntry monitoringTransitions[] =
{
                       /* Signal Id,                    Function */
    fsm_event_table_entry(SM_SCAN_REQ_ID,               monitoring_scan),
    fsm_event_table_entry(MLME_AUTONOMOUS_SCAN_IND_ID,  mlme_autonomous_scan_ind),

    fsm_event_table_entry(CORE_STOP_REQ_ID,             monitoring_stop),
    fsm_event_table_entry(SM_PAUSE_REQ_ID,              monitoring_pause),
    fsm_event_table_entry(SM_UNPAUSE_REQ_ID,            monitoring_unpause),
    fsm_event_table_entry(SM_SCAN_UPDATE_IND_ID,        monitoring_update),
    fsm_event_table_entry(SM_SCAN_QUALITY_IND_ID,       scan_quality_update),
    fsm_event_table_entry(SM_FALLBACK_EXPIRY_TIMER_ID,  fallback_timer_expired),

    fsm_event_table_entry(UNIFI_MGT_SCAN_FULL_REQ_ID,   monitoring_ext_full_scan),
    fsm_event_table_entry(SM_CHECK_CHANNELS_EXPIRY_TIMER_ID,   channels_expiry_handler),
    fsm_event_table_entry(SM_ADJUNCT_TECH_SIGNAL_IND_ID,   adjunct_technology_signal_handler),
};

static const FsmEventEntry scanningTransitions[] =
{
                       /* Signal Id,                    Function */
    fsm_event_table_entry(SM_SCAN_REQ_ID,               fsm_saved_event),
    fsm_event_table_entry(MLME_SCAN_IND_ID,             mlme_scan_ind),
    fsm_event_table_entry(MLME_AUTONOMOUS_SCAN_IND_ID,  mlme_autonomous_scan_ind),
    fsm_event_table_entry(MLME_SCAN_CFM_ID,             mlme_scan_complete),
    fsm_event_table_entry(SM_SCAN_CANCEL_IND_ID,        scanning_cancel),
    fsm_event_table_entry(CORE_STOP_REQ_ID,             scanning_stop),

    fsm_event_table_entry(SM_PAUSE_REQ_ID,              fsm_saved_event),
    fsm_event_table_entry(SM_UNPAUSE_REQ_ID,            fsm_saved_event),
    fsm_event_table_entry(SM_SCAN_UPDATE_IND_ID,        fsm_saved_event),
    fsm_event_table_entry(SM_SCAN_QUALITY_IND_ID,       scan_quality_update),
    fsm_event_table_entry(SM_FALLBACK_EXPIRY_TIMER_ID,  fallback_timer_expired),

    fsm_event_table_entry(UNIFI_MGT_SCAN_FULL_REQ_ID,   fsm_saved_event),
    fsm_event_table_entry(SM_CHECK_CHANNELS_EXPIRY_TIMER_ID,   channels_expiry_handler),
    fsm_event_table_entry(SM_ADJUNCT_TECH_SIGNAL_IND_ID,   adjunct_technology_signal_handler),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry defaultHandlers[] =
{
                       /* Signal Id,                    Function */
    fsm_event_table_entry(UNIFI_MGT_SCAN_RESULTS_GET_REQ_ID,        scan_results_get_req),
    fsm_event_table_entry(UNIFI_MGT_SCAN_RESULTS_FLUSH_REQ_ID,      scan_results_flush_req),

    fsm_event_table_entry(MLME_AUTONOMOUS_SCAN_IND_ID,              fsm_ignore_event),
    fsm_event_table_entry(MLME_ADD_AUTONOMOUS_SCAN_CFM_ID,          mlme_add_auto_scan_cfm),
    fsm_event_table_entry(MLME_DEL_AUTONOMOUS_SCAN_CFM_ID,          mlme_del_auto_scan_cfm),

    fsm_event_table_entry(SM_SCAN_CANCEL_IND_ID,                    fsm_ignore_event),
    fsm_event_table_entry(SM_SCAN_CFM_ID,                           fsm_ignore_event), /* startup scans */
    fsm_event_table_entry(SM_CHECK_CHANNELS_EXPIRY_TIMER_ID,        fsm_ignore_event),
    fsm_event_table_entry(SM_ADJUNCT_TECH_SIGNAL_IND_ID,            fsm_ignore_event),
    fsm_event_table_entry(MLME_SET_CFM_ID,                          fsm_ignore_event),
};

static const FsmEventEntry ignoreHandlers[] =
{
                       /* Signal Id,                    Function */
    fsm_event_table_entry(SM_SCAN_REQ_ID,               ignore_scan_req),
};

/** Scan Manager state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                    State                          State                           Save     */
   /*                     Name                          Transitions                      *       */
   fsm_state_table_entry(FSMSTATE_stopped,              stoppedTransitions,             FALSE),
   fsm_state_table_entry(FSMSTATE_monitoring,           monitoringTransitions,          FALSE),
   fsm_state_table_entry(FSMSTATE_scanning,             scanningTransitions,            FALSE),
};

const FsmProcessStateMachine scanManagerFsm = {
#ifdef FSM_DEBUG
       "Scan Mgr",                                                         /* FSM Process Name */
#endif
       SCAN_MANAGER_PROCESS,                                               /* FSM PROCESS ID */
       {FSMSTATE_MAX_STATE, stateTable},                                   /* FSM Transition Tables */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, defaultHandlers, FALSE),  /* FSM Default Event Handlers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, ignoreHandlers, FALSE),   /* FSM Ignore Event handers */
       init_scan_manager_fsm,                                              /* FSM Initialisation Function */
       reset_scan_manager_fsm,                                             /* FSM Reset Function */
#ifdef FSM_DEBUG_DUMP
       NULL                                                                /* Trace Dump Function   */
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

