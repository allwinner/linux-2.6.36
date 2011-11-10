/** @file sme_core_fsm.c
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
 *   Connection Manager Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_core_fsm/sme_core_fsm.c#3 $
 *
 ****************************************************************************/
/* Notes
 *
 * D3 powersaving.
 *   Windows running with a device driver power saving model ranging from D0
 * (fully awake) to D4(fully sleep).  D3, from our perceptive,  is a state when
 * the SME halts but leaves UniFi running.  The SME must stop timers and
 * change autonomous scanning appropriately..
 *
 * D0 restarts restores the SME...
 *
 */
/** @{
 * @ingroup sme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_debug.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "sme_core_fsm/sme_core_fsm.h"
#include "network_selector_fsm/network_selector_fsm.h"

#include "mgt_sap/mgt_sap_from_sme_interface.h"
#include "sys_sap/sys_sap_from_sme_interface.h"

#include "sme_configuration/sme_configuration_fsm.h"
#include "scan_manager_fsm/scan_results_storage.h"
#include "link_quality_fsm/link_quality_fsm.h"
#include "unifi_driver_fsm/unifi_driver_fsm.h"

#include "smeio/smeio_trace_types.h"

/* MACROS *******************************************************************/

/** Simple accessor for this Processes Custom data */
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
   FSMSTATE_stopped,
   FSMSTATE_starting,
   FSMSTATE_suspended,
   FSMSTATE_started,
   FSMSTATE_stopping,
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
    /** Stop Requested while starting */
    CsrBool mmiStopRequested;
    void*   mmiStopAppHandle;
    CsrBool fsmStopRequested;

    /** flightmode requested */
    CsrBool flightmodeBoot;
    void*   flightmodeBootAppHandle;

    /**
     * D3 suspend flag to indicate that the SME has been place
     * in a D3 power saving mode
     */
    CsrBool d3Suspended;

    /** start requested */
    CsrBool startRequested;
    void*   startReqAppHandle;

    /** Start status code while rewinding startup */
    unifi_Status startupStatus;

    /** stopping for suspend? */
    CsrUint8 suspends;

    /** starting for resume? */
    CsrBool resumes;

    /** Was the chip power cycled prior to the resume ? */
    CsrBool resumePowerMaintained;

    /** Holds the current startup sequence number */
    CsrUint8 currentValue;

    /** If the SME initiated shutdown itself, store why */
    unifi_ControlIndication internalShutdownReason;

    /* This list defines the start and stop order for the processes.  Starting
     * is in the list order and Stopping is done in reverse order.
     *
     * The Instance variables are declared and set in the sme_top_level module.
     */
    CsrUint16* orderedProcessList[13];
} FsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/
#define INVALID_INSTANCE_VALUE NULL

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
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void init_sme_core_fsm(FsmContext* context)
{
    int i = 0;
    FsmData* fsmdata;
    sme_trace_entry((TR_CORE, "init_sme_core_fsm()"));

    fsm_create_params(context, FsmData);
    fsmdata = FSMDATA;

    /* Fill in the Initialisation list */
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->mibAccessInstance;
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->unifiDriverInstance;
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->smeConfigurationInstance;
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->powerManagerInstance;
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->scanManagerInstance;
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->coexInstance;
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->networkSelectorInstance;
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->qosInstance;

#ifdef CSR_AMP_ENABLE
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->palMgrFsmInstance;
#endif
#ifdef CCX_VARIANT
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->ccxInstance;
    fsmdata->orderedProcessList[i++] = &getSmeContext(context)->smeMeasurementsInstance;
#endif

    fsmdata->orderedProcessList[i++] = INVALID_INSTANCE_VALUE;

    fsmdata->mmiStopRequested = FALSE;
    fsmdata->fsmStopRequested = FALSE;

    fsmdata->currentValue = 0;
    fsmdata->resumes = FALSE;
    fsmdata->suspends = 0;
    fsmdata->d3Suspended = FALSE;

    fsm_next_state(context, FSMSTATE_stopped);
}

static void cleanup_mib_files(unifi_DataBlockList* mibFiles)
{
    int i;
    if (mibFiles == NULL)
    {
        return;
    }
    for (i=0; i < mibFiles->numElements; i++)
    {
        CsrPfree(mibFiles->dataList[i].data);
    }
    if (mibFiles->dataList)
    {
        CsrPfree(mibFiles->dataList);
        mibFiles->dataList = NULL;
    }
    mibFiles->numElements = 0;
}

static void suspend_callbackfunction(FsmContext* context)
{
    send_d3_transistion_complete_req_external(context, getSmeContext(context)->smeCoreInstance, TRUE);
}

static void initiate_d3_suspend(FsmContext* context)
{
    SmeConfigData* cfg = get_sme_config(context);

    /* suspend the link quality manager */
    lqm_move_to_d3_state(context);
    /* suspend the unifi driver */
    ufd_move_to_d3_state(context);

    /* suspend the scan manager */
    switch (cfg->powerConfig.d3AutoScanMode)
    {
    case unifi_PSOn:
        /* do nothing leave the scanning as is*/
        call_unifi_sys_suspend_rsp(context, unifi_Success);
        return;
    case unifi_PSOff:
        send_sm_scan_update_ind_external(context, getSmeContext(context)->scanManagerInstance, unifi_ScanStopOnly);
        break;
    case unifi_PSAuto:
    /* need to install new scan data */
        send_sm_scan_update_ind_external(context, getSmeContext(context)->scanManagerInstance, unifi_ScanStopStart);
        break;
    default:
        break;
    }

    /* register a call back function with the hip auto cfm to
     * report when all outstanding messages have been acknowledged*/
    hip_auto_cfm_register_empty_queue_callback(getSmeContext(context)->hipSapAuto, TRUE, suspend_callbackfunction);
}

/**
 * @brief
 *   Invalid flightmode req
 *
 * @par Description
 *   Kicks off the sme startup sequence
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void bounce_flight_mode(FsmContext* context, const UnifiMgtWifiFlightmodeReq_Evt* req)
{
    sme_trace_entry((TR_CORE, "bounce_flight_mode()"));
    if (isAccessRestricted(context, req->appHandle))
    {
        call_unifi_mgt_wifi_flightmode_cfm(context, req->appHandle, unifi_Restricted);
        return;
    }
    call_unifi_mgt_wifi_flightmode_cfm(context, req->appHandle, unifi_Unavailable);
}

/**
 * @brief
 *   flight mode boot
 *
 * @par Description
 *   Kicks off the sme startup sequence for booting into flightmode
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void stopped_flight_mode(FsmContext* context, const UnifiMgtWifiFlightmodeReq_Evt* req)
{
    FsmData* fsmdata = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CORE, "stopped_flight_mode()"));

    if (isAccessRestricted(context, req->appHandle))
    {
        call_unifi_mgt_wifi_flightmode_cfm(context, req->appHandle, unifi_Restricted);
        return;
    }

    fsmdata->flightmodeBoot = TRUE;
    fsmdata->flightmodeBootAppHandle = req->appHandle;
    fsmdata->startRequested = FALSE;
    fsmdata->suspends = 0;
    fsmdata->resumes = FALSE;
    fsmdata->mmiStopRequested = FALSE;
    fsmdata->fsmStopRequested = FALSE;
    fsmdata->startupStatus = unifi_Success;
    fsmdata->internalShutdownReason = unifi_ControlExit;
    fsmdata->currentValue = 0;
    fsmdata->resumePowerMaintained = FALSE;

    cfg->stationMacAddress = req->address;

    cleanup_mib_files(&cfg->mibFiles);
    cfg->mibFiles.numElements = req->mibFilesCount;
    cfg->mibFiles.dataList = req->mibFiles;

    send_core_start_req(context, *fsmdata->orderedProcessList[0], FALSE);

    fsm_next_state(context, FSMSTATE_starting);
}

/**
 * @brief
 *   UNIFI_MGT_start_req when stopped
 *
 * @par Description
 *   Kicks off the sme startup sequence
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void stopped_start(FsmContext* context, const UnifiMgtWifiOnReq_Evt* req)
{
    FsmData* fsmdata = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CORE, "stopped_start()"));

    if (isAccessRestricted(context, req->appHandle))
    {
        call_unifi_mgt_wifi_on_cfm(context, req->appHandle, unifi_Restricted);
        return;
    }

    fsmdata->flightmodeBoot = FALSE;
    fsmdata->startRequested = TRUE;
    fsmdata->startReqAppHandle = req->appHandle;
    fsmdata->suspends = 0;
    fsmdata->resumes = FALSE;
    fsmdata->mmiStopRequested = FALSE;
    fsmdata->fsmStopRequested = FALSE;
    fsmdata->startupStatus = unifi_Success;
    fsmdata->internalShutdownReason = unifi_ControlExit;
    fsmdata->currentValue = 0;
    fsmdata->resumePowerMaintained = FALSE;

    cfg->stationMacAddress = req->address;

    cleanup_mib_files(&cfg->mibFiles);
    cfg->mibFiles.numElements = req->mibFilesCount;
    cfg->mibFiles.dataList = req->mibFiles;

    send_core_start_req(context, *fsmdata->orderedProcessList[0], FALSE);

    fsm_next_state(context, FSMSTATE_starting);
}

/**
 * @brief
 *   UNIFI_MGT_stop_req when stopped
 *
 * @par Description
 *   Just return success as already topped
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void stopped_stop(FsmContext* context, const UnifiMgtWifiOffReq_Evt* req)
{
    sme_trace_entry((TR_CORE, "stopped_stop()"));

    if (req->common.sender_ == FSM_ENV)
    {
        unifi_Status status = unifi_Success;
        if (isAccessRestricted(context, req->appHandle))
        {
            status = unifi_Restricted;
        }

        call_unifi_mgt_wifi_off_cfm(context, req->appHandle, status);
    }
}

/**
 * @brief
 *   Card unplugged when stopped
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void stopped_internalstop(FsmContext* context, const InternalStopReq_Evt* ind)
{
    sme_trace_entry((TR_CORE, "stopped_internalstop()"));
    if (ind->reason == unifi_ControlExit)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndWifiOff, &appHandles);

        sme_trace_debug((TR_CORE, "stopped_internalstop() :: unifi_mgt_wifi_off_ind()"));
        if (appHandleCount)
        {
            call_unifi_mgt_wifi_off_ind(context, appHandleCount, appHandles, ind->reason);
        }
    }
    /* Next state is always stopped so we can reuse this fn when suspended */
    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   suspend when stopped
 *
 * @par Description
 *   Fail as not started
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void stopped_suspend(FsmContext* context, const UnifiSysSuspendInd_Evt* ind)
{
    sme_trace_entry((TR_CORE, "stopped_suspend()"));
    call_unifi_sys_suspend_rsp(context, unifi_Unavailable);
}

/**
 * @brief
 *   resume when stopped
 *
 * @par Description
 *   Fail as not started
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void stopped_resume(FsmContext* context, const UnifiSysResumeInd_Evt* ind)
{
    sme_trace_entry((TR_CORE, "stopped_resume()"));
    call_unifi_sys_resume_rsp(context, unifi_Unavailable);
}

/**
 * @brief
 *   core_start_cfm when starting
 *
 * @par Description
 *   A cfm for a start req was received
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void starting_cfm(FsmContext* context, const CoreStartCfm_Evt* cfm)
{
    FsmData* fsmdata = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CORE, "starting_cfm(%d)", fsmdata->currentValue));

    if (fsmdata->mmiStopRequested || fsmdata->suspends)
    {
        /* Forget the actual startup status - the startup was canceled */
        fsmdata->startupStatus = unifi_Cancelled;
    }
    else if (fsmdata->fsmStopRequested)
    {
        /* The stop came from within the SME, so there must have been an
         * error during startup.
         *
         * Note: the error reason is in FSMDATA->internalShutdownReason
         * but there is no way of communicating this via the start_cfm
         * signal.
         */
        fsmdata->startupStatus = unifi_Error;
    }
    else
    {
        /* Startup status is that returned by the most recent FSM to start */
        fsmdata->startupStatus = cfm->status;
    }

    if ((fsmdata->startupStatus != unifi_Success) || fsmdata->suspends)
    {
        /* Startup failed or interrupted, so rewind the startup done so far */
        send_core_stop_req(context, *fsmdata->orderedProcessList[fsmdata->currentValue], (fsmdata->mmiStopRequested == 0 && fsmdata->fsmStopRequested == FALSE && fsmdata->suspends != 0));
        fsm_next_state(context, FSMSTATE_stopping);
        return;
    }

    /* If booting into flightmode then stop when we finish initialising the unifiDriverInstance */
    if (fsmdata->flightmodeBoot && *fsmdata->orderedProcessList[fsmdata->currentValue] == getSmeContext(context)->unifiDriverInstance)
    {
        send_core_stop_req(context, *fsmdata->orderedProcessList[fsmdata->currentValue], FALSE);
        fsm_next_state(context, FSMSTATE_stopping);
        return;
    }

    fsmdata->currentValue++;
    if (fsmdata->orderedProcessList[fsmdata->currentValue] == INVALID_INSTANCE_VALUE)
    {
        sme_trace_info((TR_CORE, "starting_cfm() :: Startup Complete"));

        cfg->wifiOff = FALSE;
        if (fsmdata->resumes)
        {
            fsmdata->resumes = FALSE;
            call_unifi_sys_resume_rsp(context, unifi_Success);
        }
        if (fsmdata->startRequested)
        {
            fsmdata->startRequested = FALSE;
            call_unifi_mgt_wifi_on_cfm(context, fsmdata->startReqAppHandle, unifi_Success);
        }

        fsm_next_state(context, FSMSTATE_started);
        return;
    }

    send_core_start_req(context, *fsmdata->orderedProcessList[fsmdata->currentValue], fsmdata->resumes);
}

/**
 * @brief
 *   UNIFI_MGT_stop_req when starting
 *
 * @par Description
 *   Interrupt the startup and stop the SME
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void starting_stop(FsmContext* context, const FsmEvent* req)
{
    FsmData* fsmdata = FSMDATA;
    sme_trace_entry((TR_CORE, "starting_stop()"));

    /* If the stop was generated within the SME, and this is the first such
     * stop (most likely to represent the original problem), store the reason
     * UNIFI_SYS_STOP_IND's pass through the unifi_driver_fsm and will be handled there */
    if (req->id == INTERNAL_STOP_REQ_ID)
    {
        if (fsmdata->mmiStopRequested && ((InternalStopReq_Evt *) req)->reason != unifi_ControlExit)
        {
            sme_trace_warn((TR_CORE, "starting_stop: ignoring non mgt shutdown as we are already shutting down"));
            return;
        }
        fsmdata->fsmStopRequested = TRUE;
        sme_trace_debug((TR_CORE, "starting_stop() internal stop"));
        fsmdata->internalShutdownReason = ((InternalStopReq_Evt *) req)->reason;
    }
    else if (fsmdata->mmiStopRequested)
    {
        sme_trace_warn((TR_CORE, "starting_stop: saving unifi_mgt_wifi_off_req as we are already processing a unifi_mgt_wifi_off_req"));
        fsm_saved_event(context, req);
    }
    else
    {
        if (isAccessRestricted(context, ((UnifiMgtWifiOffReq_Evt *) req)->appHandle))
        {
            call_unifi_mgt_wifi_off_cfm(context, ((UnifiMgtWifiOffReq_Evt *) req)->appHandle, unifi_Restricted);
            return;
        }

        /* A UNIFI_MGT_stop_req is responsible */
        if (fsmdata->fsmStopRequested)
        {
            sme_trace_warn((TR_CORE, "starting_stop: clearing unifi_Control_SmeResetFailed as a stop is requested"));
            fsmdata->fsmStopRequested = FALSE;
            fsmdata->internalShutdownReason = unifi_ControlExit;
        }
        sme_trace_debug((TR_CORE, "starting_stop() unifi_mgt_wifi_off_req"));
        fsmdata->mmiStopRequested = TRUE;
        fsmdata->mmiStopAppHandle = ((UnifiMgtWifiOffReq_Evt *) req)->appHandle;
    }
}

/**
 * @brief
 *   suspend when starting
 *
 * @par Description
 *   Interrupt the startup and stop the SME
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void starting_suspend(FsmContext* context, const UnifiSysSuspendInd_Evt* ind)
{
    FsmData* fsmdata = FSMDATA;
    sme_trace_entry((TR_CORE, "starting_suspend()"));

    if (ind->hardSuspend)
    {
        /* Suspended so send ALL auto cfm's and shutdown */
        hip_auto_cfm_send_events(getSmeContext(context)->hipSapAuto);
    }

    /* We have to wait until the startup sequence has run its course */
    if (ind->d3Suspend)
    {
        /*
         * Force the primitive to save and resent to
         */
        fsm_saved_event(context, (const FsmEvent*)ind);
    }
    else
    {
        /* a normal suspend request */
        fsmdata->suspends++;
    }



}

/**
 * @brief
 *   UNIFI_MGT_start_req when started
 *
 * @par Description
 *   Just return success as already started
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void started_start(FsmContext* context, const UnifiMgtWifiOnReq_Evt* req)
{
    unifi_Status status = unifi_Success;
    unifi_DataBlockList mibFiles;
    sme_trace_entry((TR_CORE, "started_start()"));

    mibFiles.numElements = req->mibFilesCount;
    mibFiles.dataList = req->mibFiles;

    cleanup_mib_files(&mibFiles);
    if (isAccessRestricted(context, req->appHandle))
    {
        status = unifi_Restricted;
    }
    call_unifi_mgt_wifi_on_cfm(context, req->appHandle, status);
}

/**
 * @brief
 *   suspend when started
 *
 * @par Description
 *   Put the SME in suspend mode.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void started_suspend(FsmContext* context, const UnifiSysSuspendInd_Evt* ind)
{
    FsmData* fsmdata = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CORE, "started_suspend()"));

    if (ind->d3Suspend)
    {
        fsmdata->d3Suspended = TRUE;

        initiate_d3_suspend(context);
        return;
    }

    cfg->wifiOff = TRUE;
    fsmdata->suspends = 1;

    if (ind->hardSuspend)
    {
        /* Suspended so send ALL auto cfm's and shutdown */
        hip_auto_cfm_send_events(getSmeContext(context)->hipSapAuto);
    }

    /* The shutdown order is the reverse of startup.  Move currentValue back
     * to point at the last valid process in the list.
     */
    fsmdata->currentValue--;
    send_core_stop_req(context, *fsmdata->orderedProcessList[fsmdata->currentValue], TRUE);

    fsm_next_state(context, FSMSTATE_stopping);
}


static void started_resume(FsmContext* context, const UnifiSysResumeInd_Evt* ind)
{
    FsmData* fsmdata = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_CORE, "started_resume()"));

    /* check to see if a we were D3 suspended */
    if (fsmdata->d3Suspended)
    {
        fsmdata->d3Suspended = FALSE;

        /* move the link quality manager back to it D0 state*/
        lqm_move_to_d0_state(context);
        /* move the unifi driver back to it D0 state*/
        ufd_move_to_d0_state(context);

        /* resume the scan manager */
        switch (cfg->powerConfig.d3AutoScanMode)
        {
        case unifi_PSOn:
            /* do nothing leave the scanning as is*/
            break;
        case unifi_PSOff:
            send_sm_scan_update_ind_external(context, getSmeContext(context)->scanManagerInstance, unifi_ScanStartOnly);
            break;
        case unifi_PSAuto:
        /* need to install new scan data */
            send_sm_scan_update_ind_external(context, getSmeContext(context)->scanManagerInstance, unifi_ScanStopStart);
            break;
        default:
            break;
        }

        call_unifi_sys_resume_rsp(context, unifi_Success);

        return;
    }
    else
    {
        call_unifi_sys_resume_rsp(context, unifi_Unavailable);
    }
}

/**
 * @brief
 *   UNIFI_MGT_resume_ind when suspended
 *
 * @par Description
 *   Resume SME Operations
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void suspended_stop(FsmContext* context, const UnifiMgtWifiOffReq_Evt* req)
{
    sme_trace_entry((TR_CORE, "suspended_stop()"));
    if (req->common.sender_ == FSM_ENV)
    {
        if (isAccessRestricted(context, req->appHandle))
        {
            call_unifi_mgt_wifi_off_cfm(context, req->appHandle, unifi_Restricted);
            return;
        }
        else
        {
            call_unifi_mgt_wifi_off_cfm(context, req->appHandle, unifi_Success);
        }
    }
    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   UNIFI_MGT_start_req when suspended
 *
 * @par Description
 *   Bounce the request
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void suspended_start(FsmContext* context, const UnifiMgtWifiOnReq_Evt* req)
{
    unifi_Status status = unifi_Unavailable;

    sme_trace_entry((TR_CORE, "suspended_start()"));

    cleanup_mib_files((unifi_DataBlockList*)req->mibFiles);
    if (isAccessRestricted(context, req->appHandle))
    {
        status = unifi_Restricted;
    }
    call_unifi_mgt_wifi_on_cfm(context, req->appHandle, status);
}

/**
 * @brief
 *   suspend when suspended
 *
 * @par Description
 *   Fail as not started
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void suspended_suspend(FsmContext* context, const UnifiSysSuspendInd_Evt* ind)
{
    sme_trace_entry((TR_CORE, "suspended_suspend()"));

    if (ind->d3Suspend)
    {
        /* we cannot D3 Suspend if we have already suspended */
        call_unifi_sys_suspend_rsp(context, unifi_Unavailable);
    }
    else
    {
        call_unifi_sys_suspend_rsp(context, unifi_Success);
    }
}
/**
 * @brief
 *   UNIFI_MGT_resume_ind when suspended
 *
 * @par Description
 *   Resume SME Operations
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : Event
 *
 * @return
 *   void
 */
static void suspended_resume(FsmContext* context, const UnifiSysResumeInd_Evt* ind)
{
    FsmData* fsmdata = FSMDATA;
    sme_trace_entry((TR_CORE, "suspended_resume()"));

    fsmdata->suspends = 0;
    fsmdata->resumes = TRUE;
    fsmdata->mmiStopRequested = FALSE;
    fsmdata->fsmStopRequested = FALSE;
    fsmdata->startupStatus = unifi_Success;
    fsmdata->currentValue = 0;
    fsmdata->resumePowerMaintained = ind->powerMaintained;
    send_core_start_req(context, *fsmdata->orderedProcessList[0], TRUE);

    fsm_next_state(context, FSMSTATE_starting);
}

/**
 * @brief
 *   UNIFI_MGT_stop_req when started
 *
 * @par Description
 *   Stop each FSM as appropriate.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Event
 *
 * @return
 *   void
 */
static void started_stop(FsmContext* context, const FsmEvent* req)
{
    FsmData* fsmdata = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_CORE, "started_stop()"));

    if (req->id == INTERNAL_STOP_REQ_ID)
    {
        fsmdata->fsmStopRequested = TRUE;
    }
    else
    {
        if (isAccessRestricted(context, ((UnifiMgtWifiOffReq_Evt *) req)->appHandle))
        {
            call_unifi_mgt_wifi_off_cfm(context, ((UnifiMgtWifiOffReq_Evt *) req)->appHandle, unifi_Restricted);
            return;
        }

        fsmdata->mmiStopRequested = TRUE;
        fsmdata->mmiStopAppHandle = ((UnifiMgtWifiOffReq_Evt *) req)->appHandle;
    }
    cfg->wifiOff = TRUE;

    /* The shutdown order is the reverse of startup.  Move currentValue back
     * to point at the last valid process in the list.
     */
    fsmdata->currentValue--;
    send_core_stop_req(context, *fsmdata->orderedProcessList[fsmdata->currentValue], FALSE);

    /* If the stop was generated within the SME, store the reason */
    if (req->id == INTERNAL_STOP_REQ_ID)
    {
        fsmdata->internalShutdownReason = ((InternalStopReq_Evt *) req)->reason;
    }

    if (fsmdata->d3Suspended)
    {
        fsmdata->d3Suspended = FALSE;

        /* we cannot D3 Suspend if we have already suspended */
        call_unifi_sys_suspend_rsp(context, unifi_Unavailable);

        /* cancel the hip function call back */
    }

    fsm_next_state(context, FSMSTATE_stopping);
}

/**
 * @brief
 *   stop_cfm when stopping
 *
 * @par Description
 *   A cfm for a stop req was received
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void stopping_cfm(FsmContext* context, const CoreStopCfm_Evt* cfm)
{
    FsmData* fsmdata = FSMDATA;
    sme_trace_entry((TR_CORE, "stopping_cfm(%s, %d, %d, %d)",
                trace_unifi_Status(cfm->status), fsmdata->currentValue, fsmdata->fsmStopRequested, fsmdata->mmiStopRequested));

    if (fsmdata->currentValue == 0)
    {
#ifndef SME_PBC_NO_VERIFY
        FsmTimerData timerData;
#endif
        SmeConfigData* cfg = get_sme_config(context);

        /* only remove the mibfiles if the SME is not suspending */
        if(!fsmdata->suspends)
        {
            cleanup_mib_files(&cfg->mibFiles);
        }

        if (fsmdata->startupStatus != unifi_Success)
        {
            fsm_next_state(context, FSMSTATE_stopped);
        }

        if (fsmdata->flightmodeBoot)
        {
            call_unifi_mgt_wifi_flightmode_cfm(context, fsmdata->flightmodeBootAppHandle, fsmdata->startupStatus);
        }

        /* Need to send a Resume Cfm? */
        if (fsmdata->resumes)
        {
            fsmdata->resumes = FALSE;
            call_unifi_sys_resume_rsp(context, fsmdata->startupStatus);
        }

        if (fsmdata->startRequested)
        {
            fsmdata->startRequested = FALSE;
            call_unifi_mgt_wifi_on_cfm(context, fsmdata->startReqAppHandle, fsmdata->startupStatus);
        }

        if (fsmdata->suspends)
        {
            fsm_next_state(context, FSMSTATE_suspended);
        }

        if (fsmdata->flightmodeBoot || fsmdata->mmiStopRequested || fsmdata->fsmStopRequested)
        {
            fsm_next_state(context, FSMSTATE_stopped);
        }

        while (fsmdata->suspends)
        {
            fsmdata->suspends--;
            call_unifi_sys_suspend_rsp(context, (fsm_current_state(context) == FSMSTATE_suspended?unifi_Success:unifi_Error));
        }

        if (fsmdata->d3Suspended)
        {
            fsmdata->d3Suspended = FALSE;

            /* we cannot D3 Suspend if we have already suspended */
            call_unifi_sys_suspend_rsp(context, unifi_Unavailable);
        }

        /* Need to send an unsolicited Stop Indication with reason?
         * ALWAYS Send if the reason is exit */
        if (fsmdata->fsmStopRequested && (!fsmdata->mmiStopRequested || fsmdata->internalShutdownReason == unifi_ControlExit))
        {
            void* appHandles;
            CsrUint16 appHandleCount = get_appHandles(context, unifi_IndWifiOff, &appHandles);

            sme_trace_debug((TR_CORE, "stopping_cfm(%d, %d) :: unifi_mgt_wifi_off_ind()", fsmdata->fsmStopRequested, fsmdata->mmiStopRequested));
            if (appHandleCount)
            {
                call_unifi_mgt_wifi_off_ind(context, appHandleCount, appHandles, fsmdata->internalShutdownReason);
            }
        }

        /* Need to send a Stop Cfm? */
        if (fsmdata->mmiStopRequested)
        {
            sme_trace_debug((TR_CORE, "stopping_cfm() :: call_unifi_mgt_stop_cfm()"));
            fsmdata->mmiStopRequested = FALSE;
            call_unifi_mgt_wifi_off_cfm(context, fsmdata->mmiStopAppHandle, unifi_Success);
        }

#ifndef SME_PBC_NO_VERIFY
        timerData = fsm_get_next_timeout(context);
        if (timerData.timeoutTimeMs != 0xFFFFFFFF || timerData.timeoutTimeExtraMs != 0)
        {
            fsm_debug_dump(context);
        }
        verify(TR_CORE, timerData.timeoutTimeMs == 0xFFFFFFFF && timerData.timeoutTimeExtraMs == 0);
#endif
    }
    else
    {
        if (cfm->status != unifi_Success)
        {
            sme_trace_warn((TR_CORE, "stopping_cfm: status for FSM %d was %s, carrying on with shutdown",
                 fsmdata->currentValue, trace_unifi_Status(cfm->status)));
        }

        /* Send a core stop request to the next module to be closed down. */
        fsmdata->currentValue--;
        send_core_stop_req(context, *fsmdata->orderedProcessList[fsmdata->currentValue], (fsmdata->suspends != 0));
    }
}

/**
 * @brief
 *   stop_cfm when stopping
 *
 * @par Description
 *   A cfm for a stop req was received
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Event
 *
 * @return
 *   void
 */
static void d3_transistion_complete(FsmContext* context, const D3TransistionCompleteReq_Evt* req)
{
    FsmData* fsmdata = FSMDATA;
    sme_trace_entry((TR_CORE, "d3_transistion_complete"));

    if (fsmdata->d3Suspended)
    {
        if(req->suspend)
        {
            call_unifi_sys_suspend_rsp(context, unifi_Success);
        }
        else
        {
            call_unifi_sys_resume_rsp(context, unifi_Success);
        }
    }
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/
CsrBool core_get_startupPowerMaintainedFlag(FsmContext* context, CsrUint16 instanceId)
{
    require(TR_CORE, instanceId < context->maxProcesses);
    require(TR_CORE, context->instanceArray[instanceId].fsmInfo->processId == SME_CORE_PROCESS);
    require(TR_CORE, context->instanceArray[instanceId].state != FSM_TERMINATE);

    return (fsm_get_params_by_id(context, instanceId, FsmData))->resumePowerMaintained;
}

CsrBool core_get_flightmodeBootFlag(FsmContext* context)
{
    return (fsm_get_params_by_id(context, getSmeContext(context)->smeCoreInstance, FsmData))->flightmodeBoot;
}


/* FSM DEFINITION **********************************************/
/** State stopped transitions */
static const FsmEventEntry stoppedTransitions[] =
{
    /*                              Signal Id,                Function */
    fsm_event_table_entry(UNIFI_MGT_WIFI_FLIGHTMODE_REQ_ID,   stopped_flight_mode),
    fsm_event_table_entry(UNIFI_MGT_WIFI_ON_REQ_ID,           stopped_start),
    fsm_event_table_entry(UNIFI_MGT_WIFI_OFF_REQ_ID,          stopped_stop),
    fsm_event_table_entry(UNIFI_SYS_SUSPEND_IND_ID,           stopped_suspend),
    fsm_event_table_entry(UNIFI_SYS_RESUME_IND_ID,            stopped_resume),
    fsm_event_table_entry(INTERNAL_STOP_REQ_ID,               stopped_internalstop),
};

/** State startingCfm transitions */
static const FsmEventEntry startingCfmTransitions[] =
{
    /*                              Signal Id,                Function */
    fsm_event_table_entry(CORE_START_CFM_ID,                  starting_cfm),
    fsm_event_table_entry(UNIFI_MGT_WIFI_ON_REQ_ID,           fsm_saved_event),
    fsm_event_table_entry(UNIFI_MGT_WIFI_OFF_REQ_ID,          starting_stop),
    fsm_event_table_entry(INTERNAL_STOP_REQ_ID,               starting_stop),
    fsm_event_table_entry(UNIFI_SYS_SUSPEND_IND_ID,           starting_suspend),
};

/** State started transitions */
static const FsmEventEntry startedTransitions[] =
{
    /*                              Signal Id,                Function */
    fsm_event_table_entry(UNIFI_MGT_WIFI_ON_REQ_ID,           started_start),
    fsm_event_table_entry(UNIFI_SYS_SUSPEND_IND_ID,           started_suspend),
    fsm_event_table_entry(UNIFI_SYS_RESUME_IND_ID,            started_resume),
    fsm_event_table_entry(UNIFI_MGT_WIFI_OFF_REQ_ID,          started_stop),
    fsm_event_table_entry(INTERNAL_STOP_REQ_ID,               started_stop),
    fsm_event_table_entry(UNIFI_MGT_WIFI_FLIGHTMODE_REQ_ID,   bounce_flight_mode),
};

/** State suspended transitions */
static const FsmEventEntry suspendedTransitions[] =
{
    /*                              Signal Id,                Function */
    fsm_event_table_entry(UNIFI_MGT_WIFI_ON_REQ_ID,           suspended_start),
    fsm_event_table_entry(UNIFI_SYS_RESUME_IND_ID,            suspended_resume),
    fsm_event_table_entry(UNIFI_MGT_WIFI_OFF_REQ_ID,          suspended_stop),
    fsm_event_table_entry(UNIFI_SYS_SUSPEND_IND_ID,           suspended_suspend),
    fsm_event_table_entry(UNIFI_MGT_WIFI_FLIGHTMODE_REQ_ID,   bounce_flight_mode),
    fsm_event_table_entry(INTERNAL_STOP_REQ_ID,               stopped_internalstop),
};

/** State stoppingCfm transitions */
static const FsmEventEntry stoppingCfmTransitions[] =
{
    /*                              Signal Id,                Function */
    fsm_event_table_entry(UNIFI_MGT_WIFI_ON_REQ_ID,           fsm_saved_event),
    fsm_event_table_entry(CORE_STOP_CFM_ID,                   stopping_cfm),
    fsm_event_table_entry(UNIFI_MGT_WIFI_OFF_REQ_ID,          starting_stop),
    fsm_event_table_entry(INTERNAL_STOP_REQ_ID,               starting_stop),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry defaultHandlers[] =
{
                       /* Signal Id,                          Function */
    fsm_event_table_entry(SM_SCAN_CFM_ID,                     fsm_ignore_event),
    fsm_event_table_entry(D3_TRANSISTION_COMPLETE_REQ_ID,     d3_transistion_complete),
};


/** SME Core state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                           State          State                           Save     */
   /*                           Name           Transitions                      *       */
   fsm_state_table_entry(FSMSTATE_stopped,          stoppedTransitions,        FALSE),
   fsm_state_table_entry(FSMSTATE_starting,         startingCfmTransitions,    TRUE),
   fsm_state_table_entry(FSMSTATE_suspended,        suspendedTransitions,      FALSE),
   fsm_state_table_entry(FSMSTATE_started,          startedTransitions,        FALSE),
   fsm_state_table_entry(FSMSTATE_stopping,         stoppingCfmTransitions,    TRUE),
};

const FsmProcessStateMachine sme_core_fsm =
{
#ifdef FSM_DEBUG
       "Sme Core",                              /* SM Process Name       */
#endif
       SME_CORE_PROCESS,                        /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},        /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, defaultHandlers, FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),  /* Ignore Event handers */
       init_sme_core_fsm,                       /* Entry Function        */
       NULL,                                    /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                     /* Trace Dump Function   */
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
