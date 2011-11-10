/** @file link_quality_fsm.c
 *
 * Link Quality FSM Implementation
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
 *      Monitors Link quality and decides to roam when appropriate
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/link_quality_fsm/link_quality_fsm.c#2 $
 *
 ****************************************************************************/

/** @{
 * @ingroup sme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "link_quality_fsm/link_quality_fsm.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "ap_utils/ap_validation.h"

#include "hip_proxy_fsm/mib_utils.h"
#include "hip_proxy_fsm/mib_encoding.h"
#include "mgt_sap/mgt_sap_from_sme_interface.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
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
   FSMSTATE_idle,
   FSMSTATE_monitoring,
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
    unifi_RoamingBand   currentRoamingBand;

    CsrBool triggerInstalled;

    CsrInt16 currentUnifiMLMETriggeredGetLowRSSIThreshold;
    CsrInt16 currentUnifiMLMETriggeredGetHighRSSIThreshold;
    CsrInt16 currentUnifiMLMETriggeredGetLowSNRThreshold;
    CsrInt16 currentUnifiMLMETriggeredGetHighSNRThreshold;

} FsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   Inital FSM Entry function
 *
 * @param[in] context : FSM context
 *
 * @return
 *   void
 */
static void init_link_quality_fsm(FsmContext* context)
{
    FsmData* fsmData;
    sme_trace_entry((TR_LINK_QUALITY, "init_link_quality_fsm()"));
    fsm_create_params(context, FsmData);

    fsmData = FSMDATA;
    /* assume good performance */
    fsmData->currentRoamingBand = unifi_RoamingBand1;
    fsmData->triggerInstalled = FALSE;

    fsmData->currentUnifiMLMETriggeredGetLowRSSIThreshold  = -128;
    fsmData->currentUnifiMLMETriggeredGetHighRSSIThreshold =  127;
    fsmData->currentUnifiMLMETriggeredGetLowSNRThreshold   = -128;
    fsmData->currentUnifiMLMETriggeredGetHighSNRThreshold  =  127;

    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief convert between roamingband and basic usability
 *
 * @par Description:
 *  see Brief
 *
 * @param[in] roamingBand : the roaming band to be converted
 *
 * @return
 *      unifi_Unusable
 *      unifi_Poor
 *      unifi_Satisfactory
 */
/*---------------------------------------------------------------------------*/
static unifi_BasicUsability roaming_band_to_BasicUsability(const unifi_RoamingBand roamingBand)
{
    if(roamingBand == unifi_RoamingBand1)      return unifi_Satisfactory;
    else if(roamingBand == unifi_RoamingBand2) return unifi_Poor;
    else if(roamingBand == unifi_RoamingBand3) return unifi_Unusable;

    return unifi_Unusable;
}


/**
 * @brief Update the currentRoamingBand
 *
 * @param[in] context : FSM context
 *
 * @return
 *      CsrBool Has the band changed
 */
static CsrBool update_current_roaming_band(FsmContext* context, CsrInt16 currentRSSI, CsrInt16 currentSNR)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    unifi_RoamingBand bandIndex;
    unifi_RoamingBand lastRoamingBand = fsmData->currentRoamingBand;

    sme_trace_entry((TR_LINK_QUALITY, "update_roaming_band(%s, rssi:%d, snr:%d)",
                                      trace_unifi_RoamingBand(fsmData->currentRoamingBand),
                                      currentRSSI,
                                      currentSNR));

    fsmData->currentRoamingBand = unifi_RoamingBand1;

    for (bandIndex = unifi_RoamingBand1; bandIndex <= unifi_RoamingBand3; bandIndex++)
    {
        if( (currentRSSI <= cfg->roamingConfig.roamingBands[bandIndex].rssiHighThreshold)
          &&(currentRSSI >= cfg->roamingConfig.roamingBands[bandIndex].rssiLowThreshold))
        {
            /* Update the band we are in */
            fsmData->currentRoamingBand = bandIndex;
            break;
        }
    }

    for (bandIndex = unifi_RoamingBand1; bandIndex <= unifi_RoamingBand3; bandIndex++)
    {
        if( (currentSNR <= cfg->roamingConfig.roamingBands[bandIndex].snrHighThreshold)
          &&(currentSNR >= cfg->roamingConfig.roamingBands[bandIndex].snrLowThreshold))
        {
            /* Update the band ONLY if the SNR band is worse than the RSSI band */
            if (fsmData->currentRoamingBand < bandIndex )
            {
                fsmData->currentRoamingBand = bandIndex;
            }
            break;
        }
    }

    return fsmData->currentRoamingBand != lastRoamingBand;
}



/**
 * @brief Install the triggered get
 *
 * @par Description:
 *  This function installs the triggered mib get for when the
 *  RSSI or SNR crosses a threshold
 *
 * @param[in] context : FSM context
 *
 * @return
 *      void
 */
static void update_trigger_mib(FsmContext* context, CsrInt16 currentRSSI, CsrInt16 currentSNR)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    DataReference mibGetDataRef;
    CsrBool mibSetNeeded = FALSE;

    CsrInt16 newRSSIUpper = currentRSSI + cfg->smeConfig.connectionQualityRssiChangeTrigger;
    CsrInt16 newRSSILower = currentRSSI - cfg->smeConfig.connectionQualityRssiChangeTrigger;
    CsrInt16 newSNRUpper  = currentSNR  + cfg->smeConfig.connectionQualitySnrChangeTrigger;
    CsrInt16 newSNRLower  = currentSNR  - cfg->smeConfig.connectionQualitySnrChangeTrigger;

    if (!fsmData->triggerInstalled)
    {
        sme_trace_debug((TR_LINK_QUALITY, "update_trigger_mib() No Trigger Installed"));
        return;
    }
    sme_trace_entry((TR_LINK_QUALITY, "update_trigger_mib() Current Band = %s", trace_unifi_RoamingBand(fsmData->currentRoamingBand)));

    mibGetDataRef = mib_encode_create_set(context, 4, 0);

    /* Trigger on the next unifi_mgt_connection_quality_ind() trigger value OR the roaming threshold (Which ever is closest) */
    newRSSILower = CSRMAX(newRSSILower, cfg->roamingConfig.roamingBands[fsmData->currentRoamingBand].rssiLowThreshold);
    newRSSIUpper = CSRMIN(newRSSIUpper, cfg->roamingConfig.roamingBands[fsmData->currentRoamingBand].rssiHighThreshold);
    newSNRLower  = CSRMAX(newSNRLower,  cfg->roamingConfig.roamingBands[fsmData->currentRoamingBand].snrLowThreshold);
    newSNRUpper  = CSRMIN(newSNRUpper,  cfg->roamingConfig.roamingBands[fsmData->currentRoamingBand].snrHighThreshold);

    if (newRSSILower != fsmData->currentUnifiMLMETriggeredGetLowRSSIThreshold)
    {
        mibSetNeeded = TRUE;
        fsmData->currentUnifiMLMETriggeredGetLowRSSIThreshold = newRSSILower;
        sme_trace_debug((TR_LINK_QUALITY, "update_trigger_mib(rssiLowThreshold:%d)", newRSSILower));
        (void)mib_encode_add_set_int(context, &mibGetDataRef, unifiMLMETriggeredGetLowRSSIThreshold,  newRSSILower,  0, 0);
    }
    if (newRSSIUpper != fsmData->currentUnifiMLMETriggeredGetHighRSSIThreshold)
    {
        mibSetNeeded = TRUE;
        fsmData->currentUnifiMLMETriggeredGetHighRSSIThreshold = newRSSIUpper;
        sme_trace_debug((TR_LINK_QUALITY, "update_trigger_mib(rssiHighThreshold:%d)", newRSSIUpper));
        (void)mib_encode_add_set_int(context, &mibGetDataRef, unifiMLMETriggeredGetHighRSSIThreshold, newRSSIUpper, 0, 0);
    }

    if (newSNRLower != fsmData->currentUnifiMLMETriggeredGetLowSNRThreshold)
    {
        mibSetNeeded = TRUE;
        fsmData->currentUnifiMLMETriggeredGetLowSNRThreshold = newSNRLower;
        sme_trace_debug((TR_LINK_QUALITY, "update_trigger_mib(snrLowThreshold:%d)", newSNRLower));
        (void)mib_encode_add_set_int(context, &mibGetDataRef, unifiMLMETriggeredGetLowSNRThreshold,  newSNRLower,  0, 0);
    }
    if (newSNRUpper != fsmData->currentUnifiMLMETriggeredGetHighSNRThreshold)
    {
        mibSetNeeded = TRUE;
        fsmData->currentUnifiMLMETriggeredGetHighSNRThreshold = newSNRUpper;
        sme_trace_debug((TR_LINK_QUALITY, "update_trigger_mib(snrHighThreshold:%d)", newSNRUpper));
        (void)mib_encode_add_set_int(context, &mibGetDataRef, unifiMLMETriggeredGetHighSNRThreshold, newSNRUpper, 0, 0);
    }

    if(mibSetNeeded)
    {
        send_mlme_set_req_internal(context, getSmeContext(context)->mibAccessInstance, mibGetDataRef);
    }
    else
    {
        sme_trace_debug((TR_LINK_QUALITY, "update_trigger_mib() No update required"));
        pld_release(getPldContext(context), mibGetDataRef.slotNumber);
    }
}

/**
 * @brief Install the triggered get
 *
 * @par Description:
 *  This function installs the triggered mib get for when the
 *  RSSI or SNR crosses a threshold
 *
 * @param[in] context : FSM context
 *
 * @return
 *      void
 */
static void install_triggered_get(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    DataReference mibGetDataRef;

    if (fsmData->triggerInstalled)
    {
        sme_trace_debug((TR_LINK_QUALITY, "install_triggered_get() Trigger Already Installed"));
        return;
    }
    sme_trace_entry((TR_LINK_QUALITY, "install_triggered_get()"));

    fsmData->triggerInstalled = TRUE;
    mib_util_encode_linkq_req(context, &mibGetDataRef, cfg->connectionInfo.ifIndex);

    send_mlme_add_triggered_get_req(context, mibGetDataRef, 1);
}


/**
 * @brief Remove the triggered get
 *
 * @par Description:
 *  This function removes the triggered mib get
 *
 * @param[in] context : FSM context
 *
 * @return
 *      void
 */
static void remove_triggered_get(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    if (!fsmData->triggerInstalled)
    {
        sme_trace_debug((TR_LINK_QUALITY, "install_triggered_get() No Trigger Installed"));
        return;
    }
    sme_trace_entry((TR_LINK_QUALITY, "remove_triggered_get()"));

    fsmData->triggerInstalled = FALSE;
    send_mlme_del_triggered_get_req(context, 1);
}

/**
 * @brief
 *   updates the current connection scan result
 *
 * @par Description
 *   See brief
 *
 * @param[in] context : FSM context
 * @param[in] cfg     : configuration data pointer
 *
 * @return
 *   void
 */
static void update_scan_result(FsmContext* const context, const SmeConfigData* const cfg)

{
    srs_scan_data* pScanData = srs_scan_result_quality_update(
                                            context,
                                            &cfg->connectionInfo.bssid,
                                            cfg->linkQuality.unifiRssi,
                                            cfg->linkQuality.unifiSnr);

    sme_trace_entry((TR_LINK_QUALITY, "update_scan_result: RSSI %d, SNR band %d", cfg->linkQuality.unifiRssi, cfg->linkQuality.unifiSnr ));

    if(pScanData != NULL)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndScanResult, &appHandles);

        sme_trace_info((TR_LINK_QUALITY, "update_scan_result: unifi_mmi_scan_result_ind: BSSID %s SSID %s",
            trace_unifi_MACAddress(pScanData->scanResult.bssid, getMacAddressBuffer(context)),
            trace_unifi_SSID(&pScanData->scanResult.ssid, getSSIDBuffer(context)) ));

        if (appHandleCount)
        {
            call_unifi_mgt_scan_result_ind(context, appHandleCount, appHandles, &pScanData->scanResult);
        }
    }
}

/**
 * @brief
 *   LinkQualityStartMonitoring_req while waiting_for_start_monitor
 *
 * @par Description
 *   Initiates link monitoring
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void idle_monitor_start(FsmContext* context, const LinkQualityStartMonitoringReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    srs_scan_data *pJoinscan;

    sme_trace_entry((TR_LINK_QUALITY, "idle_monitor_start()"));

    fsmData->currentRoamingBand = unifi_RoamingBand1;

    pJoinscan = srs_get_scan_parameters_first(context, NULL, &cfg->connectionInfo.bssid);

    /* pJoinscan can be NULL when hosting an Adhoc Network hence use predefined
     * values instead (otherwise reading NULL creating a crash) */
    if ( pJoinscan )
    {
        /* initialise the results with the current scan result*/
        cfg->linkQuality.unifiRssi = pJoinscan->scanResult.rssi;
        cfg->linkQuality.unifiSnr  = pJoinscan->scanResult.snr;
    }
    else
    {
        /* initialise the results with predefined values */
        cfg->linkQuality.unifiRssi = RSSI_SATISFACTORY;
        cfg->linkQuality.unifiSnr  = SNR_SATISFACTORY;
    }

    /* Do Nothing for an adhoc connection */
    if (cfg->connectionConfig.bssType != unifi_Adhoc)
    {
        (void)update_current_roaming_band(context, cfg->linkQuality.unifiRssi, cfg->linkQuality.unifiSnr);
        update_trigger_mib(context, cfg->linkQuality.unifiRssi, cfg->linkQuality.unifiSnr);

        install_triggered_get(context);
    }
    fsm_next_state(context, FSMSTATE_monitoring);
}

/**
 * @brief
 *   decides if a roam is required
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
CsrBool check_if_we_should_roam(FsmContext* context)
{
    /* This function can be called from other FSM's so DO NOT USE THE FSMDATA macro */
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->linkQualityInstance, FsmData);

    SmeConfigData* cfg = get_sme_config(context);
    srs_scan_data* currentAP = NULL;
    CsrUint8 currentRankValue;
    srs_scan_data* bestAlternative = NULL;

    if (ns_get_connection_status(context) != ns_ConnectionStatus_Connected)
    {
        return FALSE;
    }

    sme_trace_entry((TR_LINK_QUALITY, "check_if_we_should_roam()"));

    if (CsrMemCmp( &cfg->connectionConfig.bssid, &cfg->connectionInfo.bssid, sizeof(cfg->connectionInfo.bssid.data)) == 0)
    {
        sme_trace_debug((TR_LINK_QUALITY, "check_if_we_should_roam() : fixed BSSID, do not roam"));
        return FALSE; /* Fixed BSSID so do not roam */
    }

    /* Precondition :
     * No roaming with Adhoc */
    if (cfg->connectionConfig.bssType == unifi_Adhoc)
    {
        sme_trace_debug((TR_LINK_QUALITY, "check_if_we_should_roam() : Adhoc do not roam"));
        return FALSE;
    }

    /*
     * Stage One
     * Check the current connection  ?
     */
    if (fsmData->currentRoamingBand == unifi_RoamingBand1)
    {
        sme_trace_debug((TR_LINK_QUALITY, "check_if_we_should_roam() : unifi_RoamingBand1 do not roam"));
        return FALSE;
    }

    currentAP = srs_get_scan_parameters_first(context, &cfg->connectionInfo.ssid, &cfg->connectionInfo.bssid);
    /* Corner case */
    if (currentAP == NULL)
    {
        sme_trace_error((TR_LINK_QUALITY, "check_if_we_should_roam() : Current AP scan data not found"));
        return FALSE;
    }
    /*currentRankValue = calculate_signal_values(&currentAP->scanResult); */
    currentRankValue = (CsrUint8) calculate_ranking_metric( currentAP );

    /*
     * Stage Two
     * is there a better AP to roam to ?
     */
    bestAlternative = srs_get_scan_parameters_first(context, &cfg->connectionConfig.ssid, &cfg->connectionConfig.bssid);
    while (bestAlternative)
    {
        if (currentAP == bestAlternative)
        {
            sme_trace_debug((TR_LINK_QUALITY, "check_if_we_should_roam() : Current AP is the best do not roam"));
            /* currentRankValue = calculate_signal_values(&currentAP->scanResult);*/
            currentRankValue = (CsrUint8) calculate_ranking_metric( currentAP );
            return FALSE;
        }

        /* its a physical match in terms or bandparameters */
        if (validate_ap(context, bestAlternative, cfg))
        {
            /* Compare the rank values */
            CsrUint8 altRankValue = (CsrUint8) calculate_ranking_metric( bestAlternative );
            if ((currentRankValue + THRESHOLD_OFFSET) < altRankValue)
            {
                /* there is at least one better alternative */
                sme_trace_error((TR_SCAN, "check_if_we_should_roam() : bestAlternative: BSSID %s SSID %s roaming (%2d,%3d,%3d -> %2d,%3d,%3d)",
                    trace_unifi_MACAddress(bestAlternative->scanResult.bssid, getMacAddressBuffer(context)),
                    trace_unifi_SSID(&bestAlternative->scanResult.ssid, getSSIDBuffer(context)),
                    currentRankValue,
                    cfg->linkQuality.unifiRssi,
                    cfg->linkQuality.unifiSnr,
                    altRankValue,
                    bestAlternative->scanResult.rssi,
                    bestAlternative->scanResult.snr ));

                send_network_selector_roam_ind(context, getSmeContext(context)->networkSelectorInstance, unifi_RoamBetterAPFound);
                return TRUE;
            }
        }
        bestAlternative = srs_get_scan_parameters_next(context, &bestAlternative->scanResult.bssid, &cfg->connectionConfig.ssid, &cfg->connectionConfig.bssid);
    }
    return FALSE;
}

/**
 * @brief
 *   unhandled Stop Request
 *
 * @par Description
 *   Stops the collection of stats
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void monitoring_stop_monitoring(FsmContext* context, const LinkQualityStopMonitoringReq_Evt* req)
{
    sme_trace_entry((TR_LINK_QUALITY, "monitoring_stop_monitoring()"));

    remove_triggered_get(context);

    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   unhandled Stop Request
 *
 * @par Description
 *   Stops the collection of stats
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void monitoring_trigger_ind(FsmContext* context, const MlmeTriggeredGetInd_Evt* ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint32 now = fsm_get_time_of_day_ms(context);
    CsrBool decodeResult;
    CsrBool roamingBandUpdated;


    decodeResult = mib_util_decode_linkq(context, &ind->mibAttributeValue, ind->status, ind->errorIndex, &cfg->linkQuality);
    sme_trace_entry((TR_LINK_QUALITY, "monitoring_trigger_ind(%s, %d)", trace_MibStatus(ind->status), ind->errorIndex));

    if (ind->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), ind->mibAttributeValue.slotNumber);
    }
    if (!decodeResult)
    {
        sme_trace_error((TR_LINK_QUALITY, "monitoring_trigger_ind() Mib Read error %s", trace_MibStatus(ind->status)));
        return;
    }

    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndConnectionQuality, &appHandles);
        if (appHandleCount)
        {
            call_unifi_mgt_connection_quality_ind(context, appHandleCount, appHandles, &cfg->linkQuality);
        }
    }

    /* Update the Config and Scan data with new data */
    update_scan_result(context, cfg);

    roamingBandUpdated = update_current_roaming_band(context, cfg->linkQuality.unifiRssi, cfg->linkQuality.unifiSnr);

    cfg->lastLinkQualityRead = now;

    if (!check_if_we_should_roam(context))
    {
        update_trigger_mib(context, cfg->linkQuality.unifiRssi, cfg->linkQuality.unifiSnr);
        if (roamingBandUpdated)
        {
            FsmData* fsmData = FSMDATA;
            send_sm_scan_quality_ind(context,
                                     getSmeContext(context)->scanManagerInstance,
                                     roaming_band_to_BasicUsability(fsmData->currentRoamingBand));
        }
    }

}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in link_quality_fsm/link_quality_storage.h
 */
void reset_link_quality(FsmContext* context)
{
    FsmData* fsmData;

    FsmInstanceEntry* currentInstance = fsm_save_current_instance_by_id(context, getSmeContext(context)->linkQualityInstance);

    fsmData = FSMDATA;

    sme_trace_entry((TR_LINK_QUALITY, "reset_link_quality()"));

    require(TR_SCAN, context->instanceArray[getSmeContext(context)->linkQualityInstance].state != FSM_TERMINATE);

    fsmData->triggerInstalled = FALSE;

    fsmData->currentUnifiMLMETriggeredGetLowRSSIThreshold  = -128;
    fsmData->currentUnifiMLMETriggeredGetHighRSSIThreshold =  127;
    fsmData->currentUnifiMLMETriggeredGetLowSNRThreshold   = -128;
    fsmData->currentUnifiMLMETriggeredGetHighSNRThreshold  =  127;

    /* Change state to Idle */
    fsm_next_state(context, FSMSTATE_idle);

    fsm_restore_current_instance(context, currentInstance);
}

/*
 * Description:
 * See description in link_quality_fsm/link_quality_storage.h
 */
void lqm_move_to_d0_state(FsmContext* context)
{
    FsmInstanceEntry* currentInstance = fsm_save_current_instance_by_id(context, getSmeContext(context)->linkQualityInstance);

    /* clear to run normally */
    sme_trace_entry((TR_LINK_QUALITY, "lqm_move_to_d0_state"));

    if (fsm_current_state(context) == FSMSTATE_monitoring)
    {
        install_triggered_get(context);
    }

    fsm_restore_current_instance(context, currentInstance);
}

/*
 * Description:
 * See description in link_quality_fsm/link_quality_storage.h
 */
void lqm_move_to_d3_state(FsmContext* context)
{
    FsmInstanceEntry* currentInstance = fsm_save_current_instance_by_id(context, getSmeContext(context)->linkQualityInstance);

    /* clear to run normally */
    sme_trace_entry((TR_LINK_QUALITY, "lqm_move_to_d3_state"));

    remove_triggered_get(context);

    fsm_restore_current_instance(context, currentInstance);
}

/* FSM DEFINITION **********************************************/

/** State idle transitions */
static const FsmEventEntry idleTransitions[] =
{
    /* Signal Id,                                               Function */
    fsm_event_table_entry(LINK_QUALITY_START_MONITORING_REQ_ID, idle_monitor_start),
    fsm_event_table_entry(LINK_QUALITY_STOP_MONITORING_REQ_ID,  fsm_ignore_event),
};

/** State monitoring transitions */
static const FsmEventEntry monitoringTransitions[] =
{
    /* Signal Id,                                               Function */
    fsm_event_table_entry(LINK_QUALITY_START_MONITORING_REQ_ID, fsm_ignore_event),
    fsm_event_table_entry(LINK_QUALITY_STOP_MONITORING_REQ_ID,  monitoring_stop_monitoring),
    fsm_event_table_entry(MLME_TRIGGERED_GET_IND_ID,            monitoring_trigger_ind),

};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /* Signal Id,                                               Function */
    fsm_event_table_entry(MLME_ADD_TRIGGERED_GET_CFM_ID,        fsm_ignore_event),
    fsm_event_table_entry(MLME_DEL_TRIGGERED_GET_CFM_ID,        fsm_ignore_event),
    fsm_event_table_entry(MLME_TRIGGERED_GET_IND_ID,            fsm_ignore_event),
    fsm_event_table_entry(MLME_SET_CFM_ID,                      fsm_ignore_event ),
};

/** Link Quality state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                         State                               State                      Save     */
   /*                         Name                                Transitions                 *       */
   fsm_state_table_entry(FSMSTATE_idle,                         idleTransitions,            FALSE),
   fsm_state_table_entry(FSMSTATE_monitoring,                   monitoringTransitions,      FALSE),
};

const FsmProcessStateMachine link_quality_fsm = {
#ifdef FSM_DEBUG
       "Link Quality",                                                /* SM Process Name       */
#endif
       LINK_QUALITY_PROCESS,                                          /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                              /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE),       /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),         /* Ignore Event handers */
       init_link_quality_fsm,                                         /* Entry Function        */
       NULL,                                                          /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                           /* Trace Dump Function   */
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
