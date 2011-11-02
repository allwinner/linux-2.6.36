/** @file coex_manager_fsm.c
 *
 * PAL Coex Manager FSM
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
 *   This FSM is tasked with managing the unifi interactions to deal with activity from collocated radios locally and remotely.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/coex_manager_fsm/coex_manager_fsm.c#4 $
 *
 ****************************************************************************/

/** @{
 * @ingroup device_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "coex_manager_fsm/coex_manager_fsm.h"
#include "payload_manager/payload_manager.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "bt_sap/bt_sap_from_sme_interface.h"
#include "ie_access/ie_access.h"
#include "hip_proxy_fsm/mib_utils.h"
#include "hip_proxy_fsm/mib_encoding.h"
#include "hip_proxy_fsm/mib_action_sub_fsm.h"
#include "coex_fsm/coex_fsm.h"
#include "smeio/smeio_trace_types.h"


/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, CoexFsmData))

#define SEND_PAL_DM_ENABLE_RTS_CTS_REQ() send_pal_dm_enable_rts_cts_req(context, getSmeContext(context)->palDmFsmInstance,0)
#define SEND_PAL_DM_DISABLE_RTS_CTS_REQ() send_pal_dm_disable_rts_cts_req(context, getSmeContext(context)->palDmFsmInstance,0)

#define CREATE_BLACKOUT_ID(handle,index) (CsrUint16)(((handle)<<8)|(index))
#define GET_HANDLE_FROM_BLACKOUT_ID(blackoutId) (CsrUint8)(((blackoutId)&0xFF00)>>8)
#define GET_INDEX_FROM_BLACKOUT_ID(blackoutId) (CsrUint8)((blackoutId)&0xFF)

#define CONVERT_BT_SLOTS_MICRO_SEC(val) ((val)*BT_SLOT_MICROSECONDS) /* 1 slot is 625 micro seconds*/
/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/*lint -e749 */
/**
 * @brief
 *   FSM States
 *
 * @par Description
 *   Enum defining the FSM States for this FSM process
 */
typedef enum FsmState
{
    /** COEX waiting to be started */
    FSMSTATE_stopped,

    /** COEX ready to deal with requests from LM */
    FSMSTATE_disconnected,

    /** First link is connecting */
    FSMSTATE_connecting,

    /** one or more links connected */
    FSMSTATE_connected,

    /** configuring unifi for blackouts. it could be deleting or adding */
    FSMSTATE_configuring_blackout,

#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE
    /* configuring periodic */
    FSMSTATE_configuring_periodic,
#endif
    /** Last enum in the list */
    FSMSTATE_MAX_STATE

} FsmState;


#ifdef PAL_COEX_BLACKOUT_SUPPORTED

/**
 * @brief
 *   remote activity report entry
 *
 * @par Description
 *   Storage for remote activity reports that are already configured or being configured with unifi.
 *   One entry per physical link handle. Each new activity report from the same handle will essentially replace (overwrite)
 *    the existing entry
 */
typedef struct RemoteActivityReportEntry
{
    CsrUint8 phyLinkHandle;
    /* This holds activity reports for the physical link handle that are either already configured or being configured */
    PalActivityReport activityReports;
} RemoteActivityReportEntry;
#endif

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct CoexFsmData
{
    CsrUint8 numLinksConnected; /* total physical links that are in connected state */

#ifdef PAL_COEX_BLACKOUT_SUPPORTED
    CsrUint8 numDelBlackoutPending;
    CsrUint8 phyHandleToAddBlackoutAfterDeletionCompleted; /* phyLink handl for adding blackouts that is postponed until the ongoing ones are deleted */
    CsrUint8 disconnectPendingHandle; /* handle for which a disconnection is requested */

    /* This holds activity reports for each physical link handle that are either already configured or being configured */
    RemoteActivityReportEntry *remoteARList[PAL_MAX_PHYSICAL_LINKS];
#endif

    /* These variables are used for local coexistence. */
#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE
    CsrUint16            currentAddPeriodicId;
#endif
    BlackoutSource       currentBlackoutSource;
    Microseconds32       blackoutStartReference;
    BlackoutId           currentAddBlackoutId;
} CoexFsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* Copy from  coex_fsm.c . reusing this variable */
extern CsrUint16 frequencyTable2point4Ghz[];


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
static void init_pal_coex_data(FsmContext* context)
{
    CoexFsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_PAL_COEX_FSM, "init_pal_coex_data()"));
    CsrMemSet(fsmData, 0, sizeof(CoexFsmData));
}

/**
 * @brief
 *   Process FSM Entry Function
 *
 * @par Description
 *   Called on Coex Manager Process startup to initialise
 *   the coex manager data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void pal_coex_init(FsmContext* context)
{
    sme_trace_entry((TR_PAL_COEX_FSM, "pal_coex_init()"));

    fsm_create_params(context, CoexFsmData);
    init_pal_coex_data(context);
    fsm_next_state(context, FSMSTATE_stopped);
}

#define WMM_TSPEC_SIZE 61
#define WMM_TSPEC_IE_SIZE (WMM_TSPEC_SIZE + 2 )
#define TS_MIN_SERVICE_INTERVAL_OFFSET 15

/**
 * @brief
 *   forward activity report to LM instances.
 *
 * @par Description
 *   Forward the activity report to LM instances. if phyHandle is specified and valid the forward
 *   to the instance with matching handle only. The function is called when a pal-coex-connected-req is received
 *   or unifi-bt-unicore-pass-sync-start/change/stop-ind is received. Depending on the message that triggered
 *   the contents of activity report will vary.
 *
 *
 * @param[in]    context   : FSM context
 * @param[in]    phyHandle   : physical link handle for which activity reports to be sent to. Or zero is not specified
 * @param[in]    startTime   : start Time of the traffic. Or zero if no traffic or schedule is unknown
 * @param[in]    duration   : duration of the traffic. Or zero if no traffic or schedule is unknown
 * @param[in]    periodicity   : periodicity of the traffic. Or zero if no traffic or schedule is unknown
 * @param[in]    scheduleKnown   : TRUE is schedule is known else FALSE.
 *
 * @return
 *   void
 */
static void forward_local_activity_report(FsmContext *context,
                                             CsrUint8 phyHandle,
                                             CsrUint32 startTime,
                                             CsrUint32 duration,
                                             CsrUint32 periodicity,
                                             CsrBool scheduleKnown)
{
    int i;
    PalActivityReport activityReport;

    sme_trace_entry((TR_PAL_COEX_FSM,"forward_local_activity_report():handle-%d startTime-0x%x, duration-0x%x, period-0x%x",phyHandle,startTime,duration,periodicity));

    CsrMemSet(&activityReport, 0, sizeof(PalActivityReport));

    activityReport.scheduleKnown=scheduleKnown;
    if (!startTime && !duration && !periodicity)
    {
        activityReport.numReports=0;
    }
    else
    {
        activityReport.numReports=1;
        activityReport.arTriplet = CsrPmalloc(sizeof(PalActivityReportTriplet)*activityReport.numReports);
        activityReport.arTriplet[0].duration = duration;
        activityReport.arTriplet[0].periodicity= periodicity;
        activityReport.arTriplet[0].startTime= startTime;
    }

    for (i=0; i<getPalMgrContext(context)->maxPhysicalLinks; i++)
    {
        if (getPalMgrContext(context)->phyLinkSharedAttrib[i].used &&
            VALID_PID(getPalMgrContext(context)->phyLinkSharedAttrib[i].hipFsmPid) &&
            (getPalMgrContext(context)->phyLinkSharedAttrib[i].physicalLinkHandle == phyHandle ||
             PAL_INVALID_PHYSICAL_LINK_HANDLE == phyHandle))
        {
            sme_trace_info((TR_PAL_COEX_FSM,"forward_local_activity_report():physical link entry found for handle-%d,hipPid-%d",
                           getPalMgrContext(context)->phyLinkSharedAttrib[i].physicalLinkHandle,
                           getPalMgrContext(context)->phyLinkSharedAttrib[i].linkFsmPid));
            send_pal_coex_local_activity_report_ind(context,
                                                    getPalMgrContext(context)->phyLinkSharedAttrib[i].hipFsmPid,
                                                    activityReport);

            if (getPalMgrContext(context)->phyLinkSharedAttrib[i].physicalLinkHandle == phyHandle)
            {
                sme_trace_info((TR_PAL_COEX_FSM,"forward_local_activity_report(): just forward to the specific handle and quit"));
                break;
            }
        }
    }
}

#ifdef PAL_COEX_BLACKOUT_SUPPORTED
/**
 * @brief
 *  get remote activity report information for a physical link handle
 *
 * @par Description
 *   get remote activity report information for a physical link handle. If there is no entry for the handle,
 *   then create an entry and return the pointer to the entry
 *
 * @param[in]    context   : FSM context
 * @param[in]    physicalLinkHandle   : phy link handle for which the activity report info needs to be fetched
 *
 * @return
 *   pointer to the entry which holds the information about the activity for that activity report
 */
static RemoteActivityReportEntry *get_ar_entry(FsmContext* context, CsrUint8 physicalLinkHandle)
{
    CoexFsmData *fsmData = FSMDATA;
    int i;

    sme_trace_entry((TR_PAL_COEX_FSM, "get_ar_entry(): handle-%d",physicalLinkHandle));

    for (i=0; i<PAL_MAX_PHYSICAL_LINKS; i++)
    {
        if (fsmData->remoteARList[i] &&
            fsmData->remoteARList[i]->phyLinkHandle == physicalLinkHandle)
        {
            sme_trace_info((TR_PAL_COEX_FSM, "get_ar_entry(): matching entry found"));
            return fsmData->remoteARList[i];
        }
    }

    sme_trace_info((TR_PAL_COEX_FSM, "get_ar_entry(): no existing entries for this. so try to create one"));

    for (i=0; i<PAL_MAX_PHYSICAL_LINKS; i++)
    {
        /* grab the unused entry in the list */
        if (NULL == fsmData->remoteARList[i])
        {
            sme_trace_info((TR_PAL_COEX_FSM, "get_ar_entry(): unused entry found. so create a new entry"));
            fsmData->remoteARList[i] = CsrPmalloc(sizeof(RemoteActivityReportEntry));
            CsrMemSet(fsmData->remoteARList[i], 0, sizeof(RemoteActivityReportEntry));
            fsmData->remoteARList[i]->phyLinkHandle = physicalLinkHandle;
            return fsmData->remoteARList[i];
        }
    }

    sme_trace_warn((TR_PAL_COEX_FSM, "get_ar_entry(): No matching entries found."));

    return NULL;
}

/**
 * @brief
 *  delete blackouts that are configured with unifi.
 *
 * @par Description
 *   delete blackouts that are configured for a parituclar handle. If handle is not specified then delete all blackouts
 *
 * @param[in]    context   : FSM context
 * @param[in]    physicalLinkHandle   : phy link handle for which the blackout needs to be deleted
 *
 * @return
 *  void
 */
static void delete_blackouts(FsmContext* context, CsrUint8 physicalLinkHandle)
{
    CoexFsmData *fsmData = FSMDATA;
    CsrUint16 blackoutId;
    int i,j;

    sme_trace_entry((TR_PAL_COEX_FSM, "delete_blackouts(): handle-%d",physicalLinkHandle));

    fsmData->numDelBlackoutPending=0;

    for(i=0;i<PAL_MAX_PHYSICAL_LINKS; i++)
    {
        sme_trace_info((TR_PAL_COEX_FSM, "delete_blackouts(): ptrToAR-%u",fsmData->remoteARList[i]));

        if (fsmData->remoteARList[i] &&
            (fsmData->remoteARList[i]->phyLinkHandle == physicalLinkHandle ||
             PAL_INVALID_PHYSICAL_LINK_HANDLE == physicalLinkHandle))
        {
            for(j=0;j<fsmData->remoteARList[i]->activityReports.numReports; j++)
            {
                fsmData->numDelBlackoutPending++;
                blackoutId = CREATE_BLACKOUT_ID(fsmData->remoteARList[i]->phyLinkHandle,j);
                send_mlme_del_blackout_req(context,blackoutId);
            }
        }
        if (fsmData->remoteARList[i])
        {
            CsrUint8 handle = fsmData->remoteARList[i]->phyLinkHandle;
            if (fsmData->remoteARList[i]->activityReports.arTriplet)
            {
                CsrPfree(fsmData->remoteARList[i]->activityReports.arTriplet);
            }
            CsrPfree(fsmData->remoteARList[i]);
            fsmData->remoteARList[i] = NULL;
            if (handle == physicalLinkHandle)
            {
                sme_trace_info((TR_PAL_COEX_FSM, "delete_blackouts(): exit the loop as we processed matching handle"));
                break;
            }
        }
    }

}
#endif


/*        The functions below are copied from coex_fsm.c and modified to fit with PAL requirements. They are actually a subset
 * of what SME coex does with minor modifications for PAL.  I have retained the same function names so that it will make life
 * easier if we plan to bring PAL Coex and SME Coex together to a single entity.
 *****************************START*****************************************/

#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE

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
    CoexFsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_PAL_COEX_FSM, "send_mlme_del_periodic()"));

    if (fsmData->currentAddPeriodicId != 0)
    {
        sme_trace_info((TR_PAL_COEX_FSM, "send_mlme_del_periodic() Deleting Periodic Entry"));

        send_mlme_del_periodic_req(context, fsmData->currentAddPeriodicId);
        fsmData->currentAddPeriodicId = 0;
    }
}
#endif

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
    CoexFsmData* fsmData = FSMDATA;

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
 *   mlme del blackout
 *
 * @param[in]    context           : FSM context
 *
 * @return
 *   void
 */
static void send_mlme_del_blackout(FsmContext* context)
{
    CoexFsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_COEX, "send_mlme_del_blackout()"));

    if (fsmData->currentAddBlackoutId != 0)
    {
        sme_trace_info((TR_COEX, "send_mlme_del_blackout() Deleting Blackout Entry"));

        send_mlme_del_blackout_req(context, fsmData->currentAddBlackoutId);
        fsmData->currentAddBlackoutId = 0;
        cfg->coexInfo.currentBlackoutDurationUs = 0;
        cfg->coexInfo.currentBlackoutPeriodUs = 0;

        /* let the remote know that the local BT traffic has stopped */
        forward_local_activity_report(context, PAL_INVALID_PHYSICAL_LINK_HANDLE, 0,0,0,TRUE);
    }
}


#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE

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
    CoexFsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    PeriodicSchedulingMode schedulingMode = PeriodicSchedulingMode_PeriodicSchedulePsPoll; /* FIXME: not clear what this should be - isUapsdOrPSPollAvailable(context); */
    CsrUint32 periodToUse = period;
    if (period == 0)
    {
        periodToUse = latency;
    }

    sme_trace_entry((TR_COEX, "send_mlme_add_periodic(period:%d, latency:%d) :: %s",
                              cfg->coexInfo.currentCoexPeriodMs,
                              cfg->coexInfo.currentCoexLatencyMs,
                              trace_PeriodicSchedulingMode(schedulingMode)));

    cfg->coexInfo.currentCoexPeriodMs = period;
    cfg->coexInfo.currentCoexLatencyMs = latency;

    /* make sure the only 1 value is set */
    require(TR_COEX, (cfg->coexInfo.currentCoexPeriodMs == 0 && cfg->coexInfo.currentCoexLatencyMs != 0) || (cfg->coexInfo.currentCoexPeriodMs != 0 && cfg->coexInfo.currentCoexLatencyMs == 0));

    fsmData->currentAddPeriodicId = 1;

    send_mlme_add_periodic_req(context,
                               fsmData->currentAddPeriodicId,
                               (periodToUse * 1000),
                               schedulingMode,
                               FALSE, /*cfg->coexConfig.coexPeriodicWakeHost,*/ /* FIXME: is this right for PAL ? */
                               0 ); /*lint !e571*/

    fsm_next_state(context, FSMSTATE_configuring_periodic);

}
#endif


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
    CoexFsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_COEX, "mibGetLocalTsfConfirm(%s)", trace_MibStatus(cfm->status)));

    if (cfm->status == MibStatus_Successful)
    {
        CsrInt32 dummy = 0;
        Microseconds32 blackoutStartReference = 0;

        (void)mib_decode_get_int64(context, &cfm->mibAttributeValue, 0, &dummy, &blackoutStartReference);
        fsmData->blackoutStartReference  = blackoutStartReference;

        sme_trace_entry((TR_COEX, "mibGetLocalTsfConfirm() mlme_add_blackout(startref: 0x%.8X, duration: %d, period: %d)", blackoutStartReference, cfg->coexInfo.currentBlackoutDurationUs, cfg->coexInfo.currentBlackoutPeriodUs));

        if (BlackoutSource_Dot11Local == fsmData->currentBlackoutSource)
        {
            send_mlme_add_blackout(context, 
                                   fsmData->currentBlackoutSource,
                                   blackoutStartReference,
                                   cfg->coexInfo.currentBlackoutDurationUs,
                                   cfg->coexInfo.currentBlackoutPeriodUs);
        }
        else
        {
            send_mlme_add_blackout(context, 
                                   fsmData->currentBlackoutSource, 
                                   0, 
                                   cfg->coexInfo.currentBlackoutDurationUs, 
                                   cfg->coexInfo.currentBlackoutPeriodUs);
        }

        /* Send the activity report */
        forward_local_activity_report(context,
                                      PAL_INVALID_PHYSICAL_LINK_HANDLE,
                                      fsmData->blackoutStartReference,
                                      cfg->coexInfo.currentBlackoutDurationUs,
                                      cfg->coexInfo.currentBlackoutPeriodUs,
                                      TRUE);
#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE
        /* assuming traffic is continuous */
        send_mlme_add_periodic(context, 0, cfg->coexConfig.coexTrafficContinuousLatencyMs);
#else
        fsm_next_state(context, FSMSTATE_connected);
#endif
    }
    else
    {
        sme_trace_entry((TR_COEX, "mibGetLocalTsfConfirm(%s) Mib get of unifiTSFTime Failed", trace_MibStatus(cfm->status)));
        fsm_next_state(context, FSMSTATE_connected);
    }

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }
}

/**
 * @brief
 *   Update the System parameters for SCO Active
 */
static void update_system_configuration_bt_link(FsmContext* context, BlackoutSource blackoutSource, CsrUint32 blackoutDurationUs, CsrUint32 blackoutPeriodUs)
{
    CoexFsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_COEX, "update_system_configuration_bt_link(%s, duration:%d, period:%d)", trace_BlackoutSource(blackoutSource), blackoutDurationUs, blackoutPeriodUs));

    if (fsmData->currentBlackoutSource          != blackoutSource ||
        cfg->coexInfo.currentBlackoutDurationUs != blackoutDurationUs ||
        cfg->coexInfo.currentBlackoutPeriodUs   != blackoutPeriodUs)
    {
        DataReference mibGetDataRef = mib_encode_create_get(context, 1);

        cfg->coexInfo.currentBlackoutDurationUs = blackoutDurationUs;
        cfg->coexInfo.currentBlackoutPeriodUs = blackoutPeriodUs;
        fsmData->currentBlackoutSource = blackoutSource;

        /* Read the TSF in any case because TSF is required for activity report */
        (void)mib_encode_add_get(context, &mibGetDataRef, unifiTSFTime, 0, 0);
        mib_get_sub_fsm_start(context, *mibGetLocalTsfConfirm, &mibGetDataRef, FALSE);
    }

}

/**
 * @brief
 *   Update the System parameters for WIFI Only or ACL mode
 */
static void update_system_configuration_wifi_only_or_acl(FsmContext* context)
{
    sme_trace_entry((TR_COEX, "update_system_configuration_wifi_only_or_acl()"));

    send_mlme_del_blackout(context);

#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE
    sme_trace_info((TR_COEX, "update_system_configuration_wifi_only_or_acl(cfg->coexInfo.currentTrafficType == unifi_TrafficContinuous)"));
    send_mlme_del_periodic(context);
#endif
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
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_COEX, "update_bt_channel_avoidance()"));

    /* BT avoid channel */
    if (cfg->coexInfo.hasBtDevice)
    {
        CsrUint16 channel = frequencyTable2point4Ghz[pal_get_selected_channel_no(context)];

        sme_trace_info((TR_COEX, "update_bt_channel_avoidance() Setting channel:%d, bandwidth:%d", channel, BG_CHANNEL_BANDWIDTH));
        call_unifi_bt_active_wifi_channel_req(context, channel, BG_CHANNEL_BANDWIDTH);
    }
}

/*******************************END ****************************************/


/* PUBLIC FUNCTION DEFINITIONS *********************************************/

/* FSM DEFINITION **********************************************/


/******************** TRANSITION FUNCTIONS ***********************************************************/

static void stopped__core_start_req(FsmContext* context, const CoreStartReq_Evt* req)
{
    send_core_start_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_disconnected);
}

static void disconnected__core_start_req(FsmContext* context, const CoreStopReq_Evt* req)
{
    send_core_stop_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_stopped);
}

static void disconnected__pal_coex_link_connecting_req(FsmContext* context, const PalCoexLinkConnectingReq_Evt* req)
{
    sme_trace_entry((TR_PAL_COEX_FSM,"disconnected__pal_coex_link_connecting_req: channel-%d",req->channel));

    update_bt_channel_avoidance(context);

    fsm_next_state(context, FSMSTATE_connecting);
}

static void disconnected__pal_coex_link_disconnected_req(FsmContext* context, const PalCoexLinkDisconnectedReq_Evt* req)
{
    sme_trace_entry((TR_PAL_COEX_FSM,"disconnected__pal_coex_link_disconnected_req:"));
    send_pal_coex_link_disconnected_cfm(context,
                                        pal_get_link_fsm_pid_from_handle(context,req->physicalLinkHandle));
}


static void connecting__pal_coex_link_connected_req(FsmContext* context, const PalCoexLinkConnectedReq_Evt* req)
{
    CoexFsmData *fsmData = FSMDATA;
    CsrUint32 blackoutDurationUs;
    CsrUint32 blackoutPeriodUs;
    BlackoutSource blackoutSource;
    CsrBool wifiOnlyOrAcl;

    fsmData->numLinksConnected++;

    /* Enable RTS/CTS by default for all packets. Clause 5.2.2. PAL Spec */
    SEND_PAL_DM_ENABLE_RTS_CTS_REQ();

    sme_trace_entry((TR_PAL_COEX_FSM, "connecting__pal_coex_link_connected_req(): "));

    /*  Find out if we already have some BT traffic in progress.  If so, configure Unifi and send appropriate activity report */
    if (coex_get_blackout_configuration(context,&wifiOnlyOrAcl, &blackoutDurationUs,&blackoutPeriodUs,&blackoutSource))
    {
        if (wifiOnlyOrAcl)
        {
            sme_trace_info((TR_PAL_COEX_FSM, "connecting__pal_coex_link_connected_req(): No BT traffic. Send a report with no activity")); 
            forward_local_activity_report(context,req->physicalLinkHandle, 0,0,0,TRUE);
            fsm_next_state(context, FSMSTATE_connected);
        }
        else
        {
            sme_trace_info((TR_PAL_COEX_FSM, "connecting__pal_coex_link_connected_req(): BT traffic active. Configure unifi and send an activity report")); 
            update_system_configuration_bt_link(context, blackoutSource, blackoutDurationUs, blackoutPeriodUs);
        }
    }
}

static void connecting__pal_coex_link_disconnected_req(FsmContext* context, const PalCoexLinkDisconnectedReq_Evt* req)
{
    SmeConfigData* cfg = get_sme_config(context);

    update_bt_channel_avoidance(context);

    /* Send channel avoidance to BT only BT device is present as tracked by SME's coex fsm*/
    if (cfg->coexInfo.hasBtDevice)
    {
        sme_trace_info((TR_PAL_COEX_FSM, "connecting__pal_coex_link_disconnected_req() "));
        call_unifi_bt_active_wifi_channel_req(context, 0,0);
    }

    send_pal_coex_link_disconnected_cfm(context,
                                        pal_get_link_fsm_pid_from_handle(context,req->physicalLinkHandle));

    fsm_next_state(context, FSMSTATE_disconnected);
}

static void connected__pal_coex_link_connected_req(FsmContext* context, const PalCoexLinkConnectedReq_Evt* req)
{
    CoexFsmData *fsmData = FSMDATA;
    CsrUint32 blackoutDurationUs;
    CsrUint32 blackoutPeriodUs;
    BlackoutSource blackoutSource;
    CsrBool wifiOnlyOrAcl;

    sme_trace_entry((TR_PAL_COEX_FSM, "connected__pal_coex_link_connected_req(): num link connected excluding this one-%d",fsmData->numLinksConnected));

    fsmData->numLinksConnected++;
        
    /*  Find out if we already have some BT traffic in progress.  If so, send appropriate activity report */
    if (coex_get_blackout_configuration(context,&wifiOnlyOrAcl, &blackoutDurationUs,&blackoutPeriodUs,&blackoutSource))
    {
        if (wifiOnlyOrAcl)
        {
            sme_trace_info((TR_PAL_COEX_FSM, "connected__pal_coex_link_connected_req(): No BT traffic. Send a report with no activity")); 
            forward_local_activity_report(context,req->physicalLinkHandle, 0,0,0,TRUE);
        }
        else
        {
            sme_trace_info((TR_PAL_COEX_FSM, "connected__pal_coex_link_connected_req(): BT traffic active. send an activity report")); 
            /* Send the activity report  witht the existing configuration*/
            forward_local_activity_report(context,
                                          req->physicalLinkHandle,
                                          fsmData->blackoutStartReference,
                                          blackoutDurationUs,
                                          blackoutPeriodUs,
                                          TRUE);

        }
    }

}

static void connected__pal_coex_link_disconnected_req(FsmContext* context, const PalCoexLinkDisconnectedReq_Evt* req)
{
    CoexFsmData *fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    verify(TR_PAL_COEX_FSM,fsmData->numLinksConnected>0);

    sme_trace_entry((TR_PAL_COEX_FSM, "connecting__pal_coex_link_disconnected_req(): num link connected-%d",fsmData->numLinksConnected));

    fsmData->numLinksConnected--;

    if (0 == fsmData->numLinksConnected)
    {
        /* Send channel avoidance to BT only BT device is present as tracked by SME's coex fsm*/
        if (cfg->coexInfo.hasBtDevice)
        {
            sme_trace_info((TR_PAL_COEX_FSM, "connecting__pal_coex_link_disconnected_req() "));
            call_unifi_bt_active_wifi_channel_req(context, 0,0);
        }
    }

#ifdef PAL_COEX_BLACKOUT_SUPPORTED
    /* delete all ARs. */
    delete_blackouts(context,PAL_INVALID_PHYSICAL_LINK_HANDLE);
    if (fsmData->numDelBlackoutPending)
    {
        fsmData->phyHandleToAddBlackoutAfterDeletionCompleted = PAL_INVALID_PHYSICAL_LINK_HANDLE;
        fsmData->disconnectPendingHandle = req->physicalLinkHandle;
        fsm_next_state(context, FSMSTATE_configuring_blackout);
    }
    else
    {
        send_pal_coex_link_disconnected_cfm(context,
                                            pal_get_link_fsm_pid_from_handle(context,req->physicalLinkHandle));
        if (0 == fsmData->numLinksConnected)
        {
            fsm_next_state(context, FSMSTATE_disconnected);
        }
        /* Else remain in connected state */
    }
#else
    send_pal_coex_link_disconnected_cfm(context,
                                        pal_get_link_fsm_pid_from_handle(context,req->physicalLinkHandle));

    if (0 == fsmData->numLinksConnected)
    {
        fsm_next_state(context, FSMSTATE_disconnected);
    }
#endif

}

static void connected__pal_coex_remote_activity_report_req(FsmContext* context, const PalCoexRemoteActivityReportReq_Evt* req)
{
#ifdef PAL_COEX_BLACKOUT_SUPPORTED
    CoexFsmData *fsmData = FSMDATA;
#endif
    sme_trace_entry((TR_PAL_COEX_FSM, "connected__pal_coex_remote_activity_report_req()"));

    if (FALSE == req->activityReport.scheduleKnown)
    {
        sme_trace_info((TR_PAL_COEX_FSM, "connected__pal_coex_remote_activity_report_req():remote intereference unknown. so enable RTS/CTS"));
#ifdef PAL_COEX_BLACKOUT_SUPPORTED

        /* Enable RTS/CTS  so that when blackouts are gone RTS/CTS kicks in for all links*/
        SEND_PAL_DM_ENABLE_RTS_CTS_REQ();

        /* delete all ARs.  If one of the physical links does not know about the schedule then it is better to delete all our blackouts
                * and just leave RTS/CTS enabled. This will work for all links. RTS/CTS needs to be enabled anyways for this particular physical
                * link so we might as well keep for all other links and avoid overhead of the blackout if there is any for other physical links.
                */
        delete_blackouts(context,PAL_INVALID_PHYSICAL_LINK_HANDLE);
        if (fsmData->numDelBlackoutPending)
        {
            fsm_next_state(context, FSMSTATE_configuring_blackout);
        }
#else
        SEND_PAL_DM_ENABLE_RTS_CTS_REQ();
#endif
    }
    else if (TRUE == req->activityReport.scheduleKnown && 0 == req->activityReport.numReports)
    {
        sme_trace_info((TR_PAL_COEX_FSM, "connected__pal_coex_remote_activity_report_req():No remote intereference. so disable RTS/CTS"));
#ifdef PAL_COEX_BLACKOUT_SUPPORTED
        /* Disable RTS/CTS  just in case it was enabled. */
        SEND_PAL_DM_DISABLE_RTS_CTS_REQ();

        /* delete blackout for this handle only*/
        delete_blackouts(context,req->physicalLinkHandle);
        if (fsmData->numDelBlackoutPending)
        {
            fsm_next_state(context, FSMSTATE_configuring_blackout);
        }
#else
        SEND_PAL_DM_DISABLE_RTS_CTS_REQ();
#endif
    }
    else
    {
#ifdef PAL_COEX_BLACKOUT_SUPPORTED
        RemoteActivityReportEntry *remoteAR;

        /* Delete the previous reports if any to overwrite with the new ones */
        delete_blackouts(context,req->physicalLinkHandle);

        remoteAR = get_ar_entry(context,req->physicalLinkHandle);
        verify(TR_PAL_COEX_FSM, remoteAR!=NULL);
        if (fsmData->numDelBlackoutPending)
        {
            fsmData->phyHandleToAddBlackoutAfterDeletionCompleted = req->physicalLinkHandle;
        }
        remoteAR->activityReports = req->activityReport;

        if (PAL_INVALID_PHYSICAL_LINK_HANDLE == fsmData->phyHandleToAddBlackoutAfterDeletionCompleted)
        {
            unifi_MACAddress remoteMacAddress = *pal_get_remote_mac_address(context,req->physicalLinkHandle);
            CsrUint16 blackoutId;

            blackoutId = CREATE_BLACKOUT_ID(req->physicalLinkHandle,0);
            sme_trace_info((TR_PAL_COEX_FSM, "connected__pal_coex_remote_activity_report_req():sending mlme-add-blackout-req with id-0x%x",blackoutId));

            send_mlme_add_blackout_req(context,
                                       blackoutId,
                                       BlackoutType_SpecifiedPeer,
                                       BlackoutSource_Dot11Remote,
                                       remoteAR->activityReports.arTriplet[0].startTime,
                                       remoteAR->activityReports.arTriplet[0].periodicity,
                                       remoteAR->activityReports.arTriplet[0].duration,
                                       remoteMacAddress);
        }
        fsm_next_state(context, FSMSTATE_configuring_blackout);
#else
        sme_trace_info((TR_PAL_COEX_FSM, "connected__pal_coex_remote_activity_report_req():Remote intereference present.Enable RTS/CTS or configure Unifi for remote Interference. scheduleKnown-%d, numReports-%d",req->activityReport.scheduleKnown,req->activityReport.numReports));
        /* keep the RTS/CTS enabled until this is fixed if blackouts cannot be configured */
        SEND_PAL_DM_ENABLE_RTS_CTS_REQ();
        CsrPfree(req->activityReport.arTriplet);
#endif
    }
}

static void connected__bt_sco_ind(FsmContext* context, const FsmEvent* ind)
{
    CsrUint32 blackoutDurationUs;
    CsrUint32 blackoutPeriodUs;
    BlackoutSource blackoutSource;
    CsrBool wifiOnlyOrAcl;

    sme_trace_entry((TR_PAL_COEX_FSM, "connected__bt_sco_ind(): "));

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

static void connected__bt_acl_ind(FsmContext* context, const FsmEvent* ind)
{
    CsrUint32 blackoutDurationUs;
    CsrUint32 blackoutPeriodUs;
    BlackoutSource blackoutSource;
    CsrBool wifiOnlyOrAcl;

    sme_trace_entry((TR_PAL_COEX_FSM, "connected__bt_acl_ind(): "));

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

static void connected_bt_acl_channel_types_ind(FsmContext* context, const UnifiBtAclChannelTypesInd_Evt* ind)
{
    CsrUint32 blackoutDurationUs;
    CsrUint32 blackoutPeriodUs;
    BlackoutSource blackoutSource;
    CsrBool wifiOnlyOrAcl;

    sme_trace_entry((TR_PAL_COEX_FSM, "UnifiBtAclChannelTypesInd_Evt(): "));

    if (coex_get_blackout_configuration(context,&wifiOnlyOrAcl, &blackoutDurationUs,&blackoutPeriodUs,&blackoutSource))
    {
        if (wifiOnlyOrAcl)
        {
            update_system_configuration_wifi_only_or_acl(context);
        }
        else
        {
            update_system_configuration_bt_link(context, blackoutSource, blackoutDurationUs, blackoutPeriodUs);
#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE
            fsm_next_state(context, FSMSTATE_configuring_periodic);
#endif
        }
    }
}


#ifdef PAL_COEX_BLACKOUT_SUPPORTED
static void configuring_blackout__mlme_add_blackout_cfm(FsmContext* context, const MlmeAddBlackoutCfm_Evt *cfm)
{
    CoexFsmData *fsmData = FSMDATA;
    CsrUint8 phyLinkHandle = GET_HANDLE_FROM_BLACKOUT_ID(cfm->blackoutId);
    CsrUint8 arIndex = GET_INDEX_FROM_BLACKOUT_ID(cfm->blackoutId);
    CsrUint16 blackoutId;

    RemoteActivityReportEntry *remoteAR = get_ar_entry(context,phyLinkHandle);
    verify(TR_PAL_COEX_FSM, remoteAR!=NULL);

    sme_trace_entry((TR_PAL_COEX_FSM, "configuring_blackout__mlme_add_blackout_cfm():"));

    if (cfm->resultCode != ResultCode_Success)
    {
        delete_blackouts( context,PAL_INVALID_PHYSICAL_LINK_HANDLE);

        /* we failed to configure blackout. So keep the RTS/CTS enabled */
        SEND_PAL_DM_ENABLE_RTS_CTS_REQ();

        if (!fsmData->numDelBlackoutPending)
        {
            fsm_next_state(context, FSMSTATE_connected);
        }
    }
    else
    {
        if (arIndex+1 == remoteAR->activityReports.numReports)
        {
            sme_trace_info((TR_PAL_COEX_FSM, "connected__pal_coex_remote_activity_report_req(): all ARs configured successfully"));
            fsm_next_state(context, FSMSTATE_connected);
        }
        else
        {
            unifi_MACAddress remoteMacAddress = *pal_get_remote_mac_address(context,phyLinkHandle);
            blackoutId = CREATE_BLACKOUT_ID(phyLinkHandle,(arIndex+1));
            sme_trace_info((TR_PAL_COEX_FSM, "connected__pal_coex_remote_activity_report_req():sending mlme-add-blackout-req with id-0x%x",blackoutId));

            send_mlme_add_blackout_req(context,
                                       blackoutId,
                                       BlackoutType_SpecifiedPeer,
                                       BlackoutSource_Dot11Remote,
                                       remoteAR->activityReports.arTriplet[arIndex+1].startTime,
                                       remoteAR->activityReports.arTriplet[arIndex+1].periodicity,
                                       remoteAR->activityReports.arTriplet[arIndex+1].duration,
                                       remoteMacAddress);
        }
    }
}

static void configuring_blackout__mlme_del_blackout_cfm(FsmContext* context, const MlmeDelBlackoutCfm_Evt *cfm)
{
    CoexFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_COEX_FSM, "configuring_blackout__mlme_del_blackout_cfm():result-%d",cfm->resultCode));
    verify(TR_PAL_COEX_FSM, fsmData->numDelBlackoutPending>0);
    fsmData->numDelBlackoutPending--;
    if (!fsmData->numDelBlackoutPending)
    {
        sme_trace_info((TR_PAL_COEX_FSM, "configuring_blackout__mlme_del_blackout_cfm():All ARs deleted"));
        if (fsmData->phyHandleToAddBlackoutAfterDeletionCompleted != PAL_INVALID_PHYSICAL_LINK_HANDLE)
        {
            RemoteActivityReportEntry *remoteAR = get_ar_entry(context,fsmData->phyHandleToAddBlackoutAfterDeletionCompleted);
            unifi_MACAddress remoteMacAddress = *pal_get_remote_mac_address(context,fsmData->phyHandleToAddBlackoutAfterDeletionCompleted);
            CsrUint16 blackoutId;
            verify(TR_PAL_COEX_FSM, remoteAR!=NULL);

            blackoutId = CREATE_BLACKOUT_ID(fsmData->phyHandleToAddBlackoutAfterDeletionCompleted,0);
            sme_trace_info((TR_PAL_COEX_FSM, "configuring_blackout__mlme_del_blackout_cfm():sending mlme-add-blackout-req with id-0x%x",blackoutId));

            send_mlme_add_blackout_req(context,
                                       blackoutId,
                                       BlackoutType_SpecifiedPeer,
                                       BlackoutSource_Dot11Remote,
                                       remoteAR->activityReports.arTriplet[0].startTime,
                                       remoteAR->activityReports.arTriplet[0].periodicity,
                                       remoteAR->activityReports.arTriplet[0].duration,
                                       remoteMacAddress);
            fsmData->phyHandleToAddBlackoutAfterDeletionCompleted=0;
        }
        else if (0 == fsmData->numLinksConnected)
        {
            if (fsmData->disconnectPendingHandle != 0)
            {
                send_pal_coex_link_disconnected_cfm(context,
                                                    pal_get_link_fsm_pid_from_handle(context,fsmData->disconnectPendingHandle));
                fsmData->disconnectPendingHandle = 0;
            }
            fsm_next_state(context, FSMSTATE_disconnected);
        }
        else
        {
            fsm_next_state(context, FSMSTATE_connected);
        }
    }
}
#endif

#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE
static void configuring_periodic__mlme_add_periodic_cfm(FsmContext* context, const MlmeAddPeriodicCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_COEX_FSM, "configuring_periodic__mlme_add_periodic_cfm(): result-%d",cfm->resultCode));

    if (ResultCode_Success != cfm->resultCode)
    {
        sme_trace_crit((TR_PAL_COEX_FSM, "configuring_periodic__mlme_add_periodic_cfm(): failed"));
        send_mlme_del_blackout(context);
    }

    fsm_next_state(context, FSMSTATE_connected);
}
#endif

static void handle_mlme_add_blackout_cfm(FsmContext* context, const MlmeAddBlackoutCfm_Evt* cfm)
{
    sme_trace_entry((TR_COEX,  "handle_mlme_add_blackout_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    if (cfm->resultCode != ResultCode_Success)
    {
        sme_trace_crit((TR_COEX,  "handle_mlme_add_blackout_cfm(%s)", trace_ResultCode(cfm->resultCode)));
    }
}


#ifdef FSM_DEBUG_DUMP
static void coex_fsm_dump(FsmContext* context, const CsrUint16 id)
{
}
#endif

static void coex_fsm_reset(FsmContext* context)
{

}

/** State idle transitions */
static const FsmEventEntry stoppedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                      stopped__core_start_req),

};

static const FsmEventEntry connectingTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                        disconnected__core_start_req),
    fsm_event_table_entry(PAL_COEX_LINK_CONNECTED_REQ_ID,          connecting__pal_coex_link_connected_req),
    fsm_event_table_entry(PAL_COEX_LINK_DISCONNECTED_REQ_ID,       connecting__pal_coex_link_disconnected_req),
};

static const FsmEventEntry connectedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                        disconnected__core_start_req),
    fsm_event_table_entry(PAL_COEX_LINK_CONNECTED_REQ_ID,          connected__pal_coex_link_connected_req),
    fsm_event_table_entry(PAL_COEX_LINK_DISCONNECTED_REQ_ID,       connected__pal_coex_link_disconnected_req),
    fsm_event_table_entry(PAL_COEX_REMOTE_ACTIVITY_REPORT_REQ_ID,  connected__pal_coex_remote_activity_report_req),

    fsm_event_table_entry(UNIFI_BT_SCO_START_IND_ID,               connected__bt_sco_ind),
    fsm_event_table_entry(UNIFI_BT_SCO_CHANGE_IND_ID,              connected__bt_sco_ind),
    fsm_event_table_entry(UNIFI_BT_SCO_STOP_IND_ID,                connected__bt_sco_ind),

    fsm_event_table_entry(UNIFI_BT_ACL_START_IND_ID,               connected__bt_acl_ind),
    fsm_event_table_entry(UNIFI_BT_ACL_CHANGE_IND_ID,              connected__bt_acl_ind),
    fsm_event_table_entry(UNIFI_BT_ACL_STOP_IND_ID,                connected__bt_acl_ind),

    fsm_event_table_entry(UNIFI_BT_ACL_CHANNEL_TYPES_IND_ID,       connected_bt_acl_channel_types_ind),

};

static const FsmEventEntry configuringBlackoutTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(0,       fsm_ignore_event),

#ifdef PAL_COEX_BLACKOUT_SUPPORTED
    fsm_event_table_entry(MLME_ADD_BLACKOUT_CFM_ID,                     configuring_blackout__mlme_add_blackout_cfm),
    fsm_event_table_entry(MLME_DEL_BLACKOUT_CFM_ID,                     configuring_blackout__mlme_del_blackout_cfm),
#endif
};

static const FsmEventEntry disconnectedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                        disconnected__core_start_req),
    fsm_event_table_entry(PAL_COEX_LINK_CONNECTING_REQ_ID,         disconnected__pal_coex_link_connecting_req),
    fsm_event_table_entry(PAL_COEX_LINK_DISCONNECTED_REQ_ID,       disconnected__pal_coex_link_disconnected_req),
};

#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE
static const FsmEventEntry configuringPeriodicTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(MLME_ADD_PERIODIC_CFM_ID,                     configuring_periodic__mlme_add_periodic_cfm),
};
#endif

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /* Signal Id,                                                   Function */
    fsm_event_table_entry(UNIFI_BT_SCO_START_IND_ID,                fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_SCO_CHANGE_IND_ID,               fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_SCO_STOP_IND_ID,                 fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_ACL_START_IND_ID,                fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_ACL_CHANGE_IND_ID,               fsm_ignore_event),
    fsm_event_table_entry(UNIFI_BT_ACL_STOP_IND_ID,                 fsm_ignore_event),
    fsm_event_table_entry(MLME_ADD_BLACKOUT_CFM_ID,                 handle_mlme_add_blackout_cfm),
    fsm_event_table_entry(MLME_DEL_BLACKOUT_CFM_ID,                 fsm_ignore_event),
#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE
    fsm_event_table_entry(MLME_DEL_PERIODIC_CFM_ID,                 fsm_ignore_event),
#endif
    fsm_event_table_entry(UNIFI_BT_ACL_CHANNEL_TYPES_IND_ID,        fsm_ignore_event),
};

/** Profile Storage state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                       State                   State                               Save     */
   /*                       Name                    Transitions                          *       */
   fsm_state_table_entry(FSMSTATE_stopped, stoppedTransitions, FALSE),
   fsm_state_table_entry(FSMSTATE_disconnected, disconnectedTransitions, FALSE),
   fsm_state_table_entry(FSMSTATE_connecting, connectingTransitions, FALSE),
   fsm_state_table_entry(FSMSTATE_connected, connectedTransitions, FALSE),
   fsm_state_table_entry(FSMSTATE_configuring_blackout, configuringBlackoutTransitions, TRUE),
#ifdef PAL_COEX_CONFIGURE_PERIODIC_ENABLE
   fsm_state_table_entry(FSMSTATE_configuring_periodic, configuringPeriodicTransitions, TRUE),
#endif
};

const FsmProcessStateMachine pal_coex_fsm =
{
#ifdef FSM_DEBUG
       "PAL COEX",                                  /* Process Name       */
#endif
       PAL_COEX_PROCESS,                                /* Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                         /* Transition Tables  */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE),   /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                    /* ignore event handers */
       pal_coex_init,                                    /* Entry Function     */
       coex_fsm_reset,                                                               /* Reset Function        */
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


/* PUBLIC FUNCTIONS *********************************************************/

void pal_get_local_coex_capability(FsmContext *context, PAL_CoexCapabilities *cap)
{
    cap->handleActivityReports = TRUE;

#ifdef PAL_COEX_BLACKOUT_SUPPORTED
    cap->handleSchedulingInfo = TRUE;
#else
    cap->handleSchedulingInfo = FALSE;
#endif
}

void pal_coex_generate_ar(FsmContext* context,
                              CsrBool scheduleKnown,
                              CsrUint32 startTime,
                              CsrUint32 duration,
                              CsrUint32 periodicity)
{
    forward_local_activity_report(context,
                                  PAL_INVALID_PHYSICAL_LINK_HANDLE,
                                  CONVERT_BT_SLOTS_MICRO_SEC(startTime),
                                  CONVERT_BT_SLOTS_MICRO_SEC(duration),
                                  CONVERT_BT_SLOTS_MICRO_SEC(periodicity),
                                  scheduleKnown);

}


/** @}
 */
