/** @file power_manager_fsm.c
 *
 * Power Manager FSM Implementation
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
 *   Power Manager Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/power_manager_fsm/power_manager_fsm.c#3 $
 ****************************************************************************/

/** @{
 * @ingroup power_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "power_manager_fsm/power_manager_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "coex_fsm/coex_fsm.h"
#include "sys_sap/sys_sap_from_sme_interface.h"
#include "network_selector_fsm/network_selector_fsm.h"

#include "smeio/smeio_trace_types.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process's Custom data
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
typedef enum
{
    POW_RSP_NONE,
    POW_RSP_SUSPEND_CFM,
    POW_RSP_RESUME_CFM
} PowerManagerResponseType;

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
    FSMSTATE_waitForMlmeCfm,
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
    /** If non-zero indicates that MLME power saving temporarily suspended **/
    CsrUint16                      suspendCount;
    /** Holds the process ID of the power-management-state-altering entity **/
    CsrUint16                      caller;
    /** Determines type of response sent to initiating entity after MLME cfm **/
    PowerManagerResponseType    responseType;
    /** Shutdown flag, set if stop request caught midway through an MLME op **/
    CsrBool                     stopRequested;
    /** Pid of agent that sent stop req if in state where we can't handle it **/
    CsrUint16                      stopRequestPid;
    /** Last mode set in the Firmware **/
    PowerManagementMode         lastMlmePowerMode;

    unifi_LowPowerMode          lastSysCfgLowPower;
    CsrBool                     lastSysCfghostWakeup;
} FsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* ====================[ RUNTIME UTITILITY FUNCTIONS ]==================== */
/* ====================[ RUNTIME UTITILITY FUNCTIONS ]==================== */
/* ====================[ RUNTIME UTITILITY FUNCTIONS ]==================== */

/**
 * @brief
 *   Power Manager Process FSM Entry Function
 *
 * @par Description
 *   Called on Power Manager Process startup to initialise
 *   the Power Manager State Machine
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void powerManagerInit(FsmContext* context)
{
    sme_trace_entry((TR_POWER_MGR, ">>powerManagerInit()"));

    fsm_create_params(context, FsmData);

    fsm_next_state(context, FSMSTATE_stopped);

    sme_trace_entry((TR_POWER_MGR, "<<powerManagerInit()"));
}

/**
 * @brief
 *   Sends a response to a power change request
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void sendResponse(FsmContext* context, unifi_Status status)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_POWER_MGR, ">>sendResponse()"));
    switch (fsmData->responseType)
    {
    case POW_RSP_NONE:
        break;
    case POW_RSP_SUSPEND_CFM:
        sme_trace_info((TR_POWER_MGR, "%s: Sending suspend confirm (%s) to %s process", fsm_current_state_name(context), trace_unifi_Status(status), fsm_process_name_by_id(context, fsmData->caller)));
        send_pow_suspend_power_saving_cfm(context, fsmData->caller, status);
        break;
    case POW_RSP_RESUME_CFM:
        sme_trace_info((TR_POWER_MGR, "%s: Sending resume confirm (%s) to %s process", fsm_current_state_name(context), trace_unifi_Status(status), fsm_process_name_by_id(context, fsmData->caller)));
        send_pow_resume_power_saving_cfm(context, fsmData->caller, status);
        break;
    default:
        verify(TR_POWER_MGR, 0);    /*lint !e774*/
        break;
    }
    fsmData->responseType = POW_RSP_NONE;
}

/**
 * @brief
 *   Works out the desired power save level
 *
 *
 * @par Description
 *   Figure out the correct power save level and
 *   configure the firmware
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void setPowerSavingMode(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    PowerManagementMode mlmePowerMode = PowerManagementMode_ActiveMode;
    CsrUint16 listenIntervalBeacons, listenIntervalMillisecs;

    sme_trace_entry((TR_POWER_MGR, "setPowerSavingMode() suspendCount = %d", fsmData->suspendCount));

    /* NOTE: PowerManagementMode and unifi_PowerSaveLevel
     * have the same values except for unifi_PowerSaveAuto
     * They are only different because 1 is the FW api and 1 is the public SME api
     */
    /* If the not suspended then set the Power save state */
    if (fsmData->suspendCount == 0)
    {
        mlmePowerMode = get_required_coex_power_mode(context);
        if (mlmePowerMode == unifi_PowerSaveAuto)
        {
            mlmePowerMode = PowerManagementMode_FastPowerSave;
        }
    }

    sme_trace_debug((TR_POWER_MGR, "bsstype = %d atimwindow = %d",
            cfg->connectionConfig.bssType, cfg->connectionInfo.atimWindowTu));

    if ((mlmePowerMode != PowerManagementMode_ActiveMode)
            && (cfg->connectionConfig.bssType == unifi_Adhoc) && (cfg->connectionInfo.atimWindowTu == 0))
    {
        sendResponse(context, unifi_Unsupported);
        return;
    }

    /* if not suspended or the first suspend send the mlme power req */
    if (mlmePowerMode != fsmData->lastMlmePowerMode && (fsmData->suspendCount == 0 || (fsmData->suspendCount == 1 && fsmData->responseType == POW_RSP_SUSPEND_CFM)))
    {
        sme_trace_info (( TR_POWER_MGR, "%s: Configuring MLME power management:  mode %s, wakeup: \"OFF\" multicast receive: \"%s\"",
                                        fsm_current_state_name(context),
                                        trace_PowerManagementMode(mlmePowerMode),
                                        (cfg->powerConfig.rxDtims ? "ON" : "OFF") ));

        fsmData->lastMlmePowerMode = mlmePowerMode;
        cfg->coexInfo.currentPowerSave = fsmData->lastMlmePowerMode;

        /* Convert the user supplied listen interval (measured in msecs) into units
         * of beacon periods. Round up or down to the nearest beacon period.
         */
        require (TR_POWER_MGR, cfg->connectionInfo.beaconPeriodTu != 0);
        listenIntervalMillisecs = cfg->powerConfig.listenIntervalBeacons;
        listenIntervalBeacons =
            ((cfg->connectionInfo.beaconPeriodTu >> 1) + listenIntervalMillisecs) /
                cfg->connectionInfo.beaconPeriodTu;

        /* Although we would hope this never happens we must handle the case where
         * the listen interval (in msecs) requested is less than half the beacon
         * period.
         *
         * This manifests with our listen (in beacon periods) evaluating to zero
         */
        if (listenIntervalBeacons == 0)
        {
            sme_trace_warn((TR_CXN_MGR,
                "%-25s: Requested listen interval (%d msec) less than half of the "
                "beacon period (%d msec)! Forcing listen interval to one beacon period",
                fsm_current_state_name(context),
                listenIntervalMillisecs, cfg->connectionInfo.beaconPeriodTu));
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
            listenIntervalBeacons--; 
        }

        send_mlme_powermgt_req(context, mlmePowerMode, FALSE, cfg->powerConfig.rxDtims, listenIntervalBeacons);  /*lint !e571*/
        fsm_next_state(context, FSMSTATE_waitForMlmeCfm);
    }
    else
    {
        sendResponse(context, unifi_Success);
    }
}

/* ====================[ STOPPED STATE EVENT HANDLERS ]=================== */
/* ====================[ STOPPED STATE EVENT HANDLERS ]=================== */
/* ====================[ STOPPED STATE EVENT HANDLERS ]=================== */

/**
 * @brief
 *   Power Manager Stopped State Start Request
 *
 * @par Description
 *   Starts the power management entity
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Start request
 *
 * @return
 *   void
 */
static void start_request(FsmContext* context, const CoreStartReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_info((TR_POWER_MGR, "start_request()"));

    fsmData->suspendCount  = 0;
    fsmData->stopRequested = FALSE;
    fsmData->responseType  = POW_RSP_NONE;
    fsmData->lastMlmePowerMode = PowerManagementMode_ActiveMode;

    fsmData->lastSysCfgLowPower = unifi_LowPowerEnabled;
    fsmData->lastSysCfghostWakeup = FALSE;


    send_core_start_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_idle);
}

/* =====================[ IDLE STATE EVENT HANDLERS ]===================== */
/* =====================[ IDLE STATE EVENT HANDLERS ]===================== */
/* =====================[ IDLE STATE EVENT HANDLERS ]===================== */

/**
 * @brief
 *   Called to update the power mode
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void update_power_save(FsmContext *context, const PowUpdatePowerSavingReq_Evt * req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_info((TR_POWER_MGR, "update_power_save()"));

    fsmData->responseType = POW_RSP_NONE;
    setPowerSavingMode(context);
}

/**
 * @brief
 *   Suspends the MLME power saving mechanism (i.e. temporarily disables)
 *
 * @par Description
 *   Called prior to whenever a connection establishment critical operation
 *   is about to begin, this function will temporarily disable the MLME power
 *   saving mode functionality.
 * @par
 *   Although it is not anticipated that nested calls to suspend/resume the
 *   power saving functionality should ever be made, this function does
 *   nonetheless deal with nested calls and only disables the MLME power save
 *   functionality if it is currently in operation. (NOTE: the phrase "in
 *   operation" does not necessarily mean that power saving has been enabled -
 *   the SME preferred power save mode may have been configured to "active")
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : MLME Power saving configuration indication
 *
 * @return
 *   void
 *
 * @remarks
 *   Suspension and resumption must never be performed for anything other than
 *   critical connection establishment procedures (such as performing a 4-way
 *   handshake or obtaining an IP address). Specifically, no application layer
 *   QoS or traffic stream requirements should *EVER* lead to a suspension
 *   request. Instead, if it is necessary to disable power saving mode to
 *   support a traffic stream, unifi_mmi_power_save_req() should be used.
 */
static void suspend_saving(FsmContext *context, const PowSuspendPowerSavingReq_Evt * req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_info
    ((
        TR_POWER_MGR,
        "%s: Received Power Saving Suspend Request from %s process",
        fsm_current_state_name(context),
        fsm_process_name_by_id(context, req->common.sender_)
    ));

    /*
     * We are about to suspend (if not already suspended) the configured power
     * saving scheme. Increment the suspension count and if we haven't already,
     * request that the MLME stop its current power saving operations.
     */
    fsmData->suspendCount++;

    fsmData->caller = req->common.sender_;
    fsmData->responseType = POW_RSP_SUSPEND_CFM;
    setPowerSavingMode(context);
}

/**
 * @brief
 *   Resumes the configured power saving mechanism
 *
 * @par Description
 *   Called whenever a connection establishment critical operation has
 *   completed, this function will resume whatever power saving mode was
 *   suspended previously.
 * @par
 *   Although it is not anticipated that nested calls to suspend/resume the
 *   power saving functionality should ever be made, this function does
 *   nonetheless deal with nested calls and only resumes power saving operations
 *   one all suspensions have been cancelled by matching resume requests.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : MLME Power saving configuration indication
 *
 * @return
 *   void
 *
 * @remarks
 *   Suspension and resumption must never be performed for anything other than
 *   critical connection establishment procedures (such as performing a 4-way
 *   handshake or obtaining an IP address). Specifically, no application layer
 *   QoS or traffic stream requirements should *EVER* lead to a suspension
 *   request. Instead, if it is necessary to disable power saving mode to
 *   support a traffic stream, unifi_mmi_power_save_req() should be used.
 */
static void resume_saving(FsmContext *context, const PowResumePowerSavingReq_Evt * req)
{
    FsmData *fsmData = FSMDATA;
    sme_trace_info (( TR_POWER_MGR, "%s: Received Power Saving Resume Request from %s process",
                                    fsm_current_state_name(context),
                                    fsm_process_name_by_id(context, req->common.sender_) ));

    require(TR_POWER_MGR, fsmData->suspendCount != 0);

    fsmData->caller = req->common.sender_;
    fsmData->responseType = POW_RSP_RESUME_CFM;

    if (fsmData->suspendCount == 0)
    {
        sme_trace_error (( TR_POWER_MGR, "%s: Received resume request from %s process but "
                                         "not in suspended state", fsm_current_state_name(context) ));

        sendResponse(context, unifi_Error);
        return;
    }

    fsmData->suspendCount --;
    setPowerSavingMode(context);
}

/**
 * @brief
 *   Handles stop request when in idle state
 *
 * @par Description
 *   Simply moves into stopped state - there is no need to modify the MLME,
 *   a stop request causes termination of any current connection (and thereby
 *   powers the radio down).
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : CoreStopReq_Evt
 *
 * @return
 *   void
 */
static void idle_stop_req(FsmContext* context, const CoreStopReq_Evt* req)
{
    sme_trace_info((TR_POWER_MGR, "%s: received stop request from %s process",
                                  fsm_current_state_name(context),
                                  fsm_process_name_by_id(context, req->common.sender_) ));

    send_core_stop_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_stopped);

    sme_trace_info((TR_POWER_MGR, "Power management services stopped"));
}


/* ==========[ MODIFYING MLME POWER SAVE SETTING EVENT HANDLERS ]========= */
/* ==========[ MODIFYING MLME POWER SAVE SETTING EVENT HANDLERS ]========= */
/* ==========[ MODIFYING MLME POWER SAVE SETTING EVENT HANDLERS ]========= */

/**
 * @brief
 *   Processes power management confirmations from the MLME
 *
 * @par Description
 *   Takes the MLME confirmation and, where appropriate, sends a further
 *   confirmation to any entity that requested a change to the power save setup
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : MLME power management confirmation
 *
 * @return
 *   void
 */
static void process_mlme_cfm(FsmContext *context, const MlmePowermgtCfm_Evt * cfm)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_POWER_MGR, "process_mlme_cfm(%s)", trace_ResultCode(cfm->resultCode)));

    sendResponse(context, cfm->resultCode == ResultCode_Success?unifi_Success:unifi_Error);

    if (cfm->resultCode == ResultCode_Success)
    {
        power_send_sys_configure_power_mode(context, FALSE);
    }

    /*
     * Check to see if a stop request was sent to us while we were waiting for
     * the MLME to respond to our power management request.
     */
    if (fsmData->stopRequested)
    {
        sme_trace_info((TR_POWER_MGR, "%s: A stop request was recieved earlier - now servicing",
                                      fsm_current_state_name(context)));

        send_core_stop_cfm(context, fsmData->stopRequestPid, unifi_Success);
        fsm_next_state(context, FSMSTATE_stopped);

        sme_trace_info((TR_POWER_MGR, "Power management services stopped"));
    }
    else
    {
        fsm_next_state(context, FSMSTATE_idle);
    }
}

/* ====================[ GENERIC STATE EVENT HANDLERS ]=================== */
/* ====================[ GENERIC STATE EVENT HANDLERS ]=================== */
/* ====================[ GENERIC STATE EVENT HANDLERS ]=================== */

static void generic_stop_req(FsmContext *context, const CoreStopReq_Evt * req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_info
    ((
        TR_POWER_MGR,
        "%s: received stop request from %s process with another operation "
        "in progress - deferring",
        fsm_current_state_name(context),
        fsm_process_name_by_id(context, req->common.sender_)
    ));
    fsmData->stopRequested  = TRUE;
    fsmData->stopRequestPid = req->common.sender_;
}

/* PUBLIC FUNCTION DEFINITIONS *********************************************/

/**
 * See description in power_manager_fsm/power_manager_fsm.h
 */
void reset_power_state(FsmContext* context)
{
    sme_trace_entry((TR_POWER_MGR, "reset_power_state()"));
    require(TR_NETWORK_SELECTOR_FSM, context->instanceArray[getSmeContext(context)->powerManagerInstance].state != FSM_TERMINATE);

    (fsm_get_params_by_id(context, getSmeContext(context)->powerManagerInstance, FsmData))->lastMlmePowerMode = PowerManagementMode_ActiveMode;

}

/**
 * See description in power_manager_fsm/power_manager_fsm.h
 */
void power_send_sys_configure_power_mode(FsmContext* context, CsrBool force)
{
    FsmData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->powerManagerInstance, FsmData);
    unifi_LowPowerMode lowPower = unifi_LowPowerEnabled;
    CsrBool hostWakeup = coex_current_wakeHost(context);

    require(TR_NETWORK_SELECTOR_FSM, context->instanceArray[getSmeContext(context)->powerManagerInstance].state != FSM_TERMINATE);

    sme_trace_info((TR_POWER_MGR, "unifi_sys_configure_power_mode_req(%s, %s)  %s", trace_unifi_LowPowerMode(lowPower), hostWakeup?"TRUE":"FALSE", trace_PowerManagementMode(fsmData->lastMlmePowerMode)));

    /* Disable Deep sleep in the driver when the Connected and power mode is active */
    if (fsmData->lastMlmePowerMode == PowerManagementMode_ActiveMode && ns_get_media_connection_status(context) == unifi_MediaConnected)
    {
        lowPower = unifi_LowPowerDisabled;
    }

    if (fsmData->lastMlmePowerMode != PowerManagementMode_PowerSave)
    {
        hostWakeup = FALSE;
    }

    if (!force && fsmData->lastSysCfgLowPower == lowPower && fsmData->lastSysCfghostWakeup == hostWakeup)
    {
        /* No Change so do not update */
        sme_trace_debug((TR_POWER_MGR, "unifi_sys_configure_power_mode_req(%s, %s) NO CHANGE", trace_unifi_LowPowerMode(lowPower), hostWakeup?"TRUE":"FALSE"));
        return;
    }

    sme_trace_info((TR_POWER_MGR, "unifi_sys_configure_power_mode_req(%s, %s, forced:%s)", trace_unifi_LowPowerMode(lowPower), hostWakeup?"TRUE":"FALSE", force?"TRUE":"FALSE"));
    fsmData->lastSysCfgLowPower = lowPower;
    fsmData->lastSysCfghostWakeup = hostWakeup;
    call_unifi_sys_configure_power_mode_req(context, lowPower, hostWakeup);
}

/* FSM DEFINITION **********************************************************/


/** State ready transitions */
static const FsmEventEntry stoppedTransitions[] =
{
                          /* Signal Id,                          Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                     start_request ),
};

static const FsmEventEntry idleTransitions[] =
{
                          /* Signal Id,                          Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                      idle_stop_req),
    fsm_event_table_entry(POW_UPDATE_POWER_SAVING_REQ_ID,        update_power_save),
    fsm_event_table_entry(POW_SUSPEND_POWER_SAVING_REQ_ID,       suspend_saving),
    fsm_event_table_entry(POW_RESUME_POWER_SAVING_REQ_ID,        resume_saving),
};

static const FsmEventEntry waitingForMlmeCfm[] =
{
                          /* Signal Id,                          Function */
    fsm_event_table_entry(MLME_POWERMGT_CFM_ID,                  process_mlme_cfm ),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                      generic_stop_req),
};

static const FsmEventEntry unhandledTransitions[] =
{
                          /* Signal Id,                           Function */
    fsm_event_table_entry(MLME_POWERMGT_CFM_ID,                   fsm_ignore_event ),
    fsm_event_table_entry(POW_UPDATE_POWER_SAVING_REQ_ID,         fsm_ignore_event),
};


/** Power Manager state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                    State                              State                                        Save    */
   /*                    Name                               Transitions                                  *       */
   fsm_state_table_entry(FSMSTATE_stopped,                  stoppedTransitions,                          FALSE),
   fsm_state_table_entry(FSMSTATE_idle,                     idleTransitions,                             FALSE),
   fsm_state_table_entry(FSMSTATE_waitForMlmeCfm,           waitingForMlmeCfm,                           TRUE),
};


const FsmProcessStateMachine power_manager_fsm = {
#ifdef FSM_DEBUG
       "Power Mgr",                                                 /* FSM Process Name       */
#endif
       POWER_MANAGER_PROCESS,                                            /* FSM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                                 /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions,FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),            /* ignore event handers */
       powerManagerInit,                                                 /* Entry Function        */
       NULL,                                                            /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                              /* Trace Dump Function   */
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
