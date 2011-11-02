/** @file unifi_driver_fsm.c
 *
 * Unifi Driver FSM Implementation
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
 *   Unifi Driver Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/unifi_driver_fsm/unifi_driver_fsm.c#5 $
 *
 ****************************************************************************/

/** @{
 * @ingroup unifi_driver
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_debug.h"
#include "version/version.h"
#include "version/hip_version.h"

/* *_fsm_types.h files are needed to get the SAP API version numbers */
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "smeio/smeio_trace_types.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "sme_core_fsm/sme_core_fsm.h"
#include "unifi_driver_fsm/unifi_driver_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "scan_manager_fsm/scan_manager_fsm.h"
#include "power_manager_fsm/power_manager_fsm.h"
#include "coex_fsm/coex_fsm.h"
#include "link_quality_fsm/link_quality_fsm.h"

#include "sys_sap/sys_sap_from_sme_interface.h"
#include "sys_sap/sme_interface_hip_auto_cfm.h"
#include "mgt_sap/mgt_sap_from_sme_interface.h"
#include "sys_sap/sys_sap_from_sme_interface.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, UnifiDriverData))

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
    FSMSTATE_waiting_for_wifi_on_ind,
    FSMSTATE_mib_init,
    FSMSTATE_waiting_for_connected_ind,
    FSMSTATE_waiting_wifi_on_cfm,
    FSMSTATE_waiting_for_mlme_reset_cfm,
    FSMSTATE_ready,
    FSMSTATE_waiting_wifi_off_cfm,
    FSMSTATE_MAX_STATE
} FsmState;

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct UnifiDriverData
{
    /** startingUp TRUE if responding to a CoreStartReq, FALSE otherwise */
    CsrBool startingUp;
    /** closingDown TRUE if responding to a CoreStopReq */
    CsrBool closingDown;

    /** Timer used throughout the state machine, primarily for timing out
     * "things that should never fail". */
    FsmTimerId generalPurposeTimerId;

    /** Firmware watchdog timer
     *  Periodic poll for FW crash when waiting for a mlme cfm */
    FsmTimerId fwWatchdogTimerId;

    /** corePid stores the PID of the entity that sent the start or stop
     * request. */
    CsrUint16 corePid;

    /** resetRequester stores the PID of the entity that requested a soft
     * reset. */
    CsrUint16 resetRequester;

    /* indicate that teh SME should get wifion ind/cfm soon, ignore it. */
    CsrBool d3WifiOnReqSent;
} UnifiDriverData;


/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/** Interval for polling the FW */
#define FW_WATCHDOG_POLL_INTERVAL 30000
/** The FW_CFM_RESPONSE_TIME needs to be large so that under
 * extreame traffic conditions we do not timeout when we should not */
#define FW_CFM_RESPONSE_TIME 60000

/** TIMEOUT_%_SECONDS is the number of milliseconds in % seconds */
#define TIMEOUT_FIVE_SECONDS 5000 /* ms */
#define TIMEOUT_TWENTY_SECONDS 20000 /* ms */

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/
static void gn_driver_stop(FsmContext *context);
static void gn_driver_reset(FsmContext *context);

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
static void init_unifi_driver_fsm_data(FsmContext* context)
{
    UnifiDriverData* fsmData = FSMDATA;
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "init_unifi_driver_fsm_data()"));
    fsmData->startingUp = TRUE;
    fsmData->generalPurposeTimerId.uniqueid = 0;
    fsmData->fwWatchdogTimerId.uniqueid = 0;
    fsmData->closingDown = FALSE;
    fsmData->d3WifiOnReqSent = FALSE;

}

/**
 * @brief
 *   Unifi Driver Process FSM Entry Function
 *
 * @par Description
 *   Called on Unifi Driver Process startup to initialise
 *   the unifi driver data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void unifi_driver_fsm_init(FsmContext* context)
{
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "unifi_driver_fsm_init()"));

    fsm_create_params(context, UnifiDriverData);

    init_unifi_driver_fsm_data(context);
    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   Start the UniFi Device Driver
 *
 * @par Description
 *   Ask the device driver to startup - set a timer to fire five seconds
 *   into the future so that we'll wake up if it doesn't respond.
 *
 * @param[in]    context   : FSM context
 * @param[in]    request   : content of CoreStartReq
 *
 * @return
 *   void
 */
static void stopped__core_start(FsmContext* context, const CoreStartReq_Evt *req)
{
    UnifiDriverData* fsmData = FSMDATA;
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "stopped__core_start()"));

    /* Reset the driver data.  We left it hanging around in case the last
     * start failed and the MMI wanted the versions sent up to help find out
     * what went wrong. */
    init_unifi_driver_fsm_data(context);

    fsmData->corePid = req->common.sender_;
    call_unifi_sys_wifi_on_req(context, NULL);
    send_unifi_general_timer(context, fsmData->generalPurposeTimerId, TIMEOUT_FIVE_SECONDS*4, 1000);
    fsm_next_state(context, FSMSTATE_waiting_for_wifi_on_ind);
}

/**
 * @brief
 *   CORE_STOP_REQ received when already stopped, so confirm success
 *
 * @par Description
 *   CORE_STOP_REQ received when already stopped, so confirm success.  This
 *   usually happens if driver startup failed, so unifi_manager returned to
 *   the stopped state, but sme_core_fsm sends a CORE_STOP_REQ later on to
 *   tidy up its context.
 *
 * @param[in] context : FSM context
 *
 * @return
 *  void
 */
static void stopped__core_stop(FsmContext *context, const CoreStopReq_Evt *cfm)
{
    sme_trace_info((TR_UNIFI_DRIVER_FSM, "core_stop_req received - already there."));
    send_core_stop_cfm(context, cfm->common.sender_, unifi_Success);
}

/**
 * @brief Handle stray wifi off ind
 *
 * @return
 *  void
 */
static void stopped__sys_wifi_off_ind(FsmContext *context, const UnifiSysWifiOffInd_Evt *ind)
{
    sme_trace_info((TR_UNIFI_DRIVER_FSM, "stopped__sys_wifi_off_ind()"));
    call_unifi_sys_wifi_off_rsp(context);

    /* Always send exit Indications */
    if (ind->controlIndication == unifi_ControlExit)
    {
        send_internal_stop_req(context, getSmeContext(context)->smeCoreInstance, ind->controlIndication);
    }
}

/**
 * @brief
 *   Soft-reset the UniFi device bounce
 *
 * @param[in]    context            : FSM context
 *
 * @return
 *  void
 */
static void stopped__soft_reset(FsmContext *context, const UnifiResetReq_Evt *req)
{
    sme_trace_debug((TR_UNIFI_DRIVER_FSM, "stopped__soft_reset"));
    send_unifi_reset_cfm(context, req->common.sender_);
}

/**
 * @brief
 *   Request from the driver to stop when waiting for a reset
 *
 *
 * @param[in]    context : FSM context
 * @param[in]    ind     : event
 *
 * @return
 *   void
 */
static void waiting_for_reset_sys_wifi_off_ind(FsmContext* context, const UnifiSysWifiOffInd_Evt *ind)
{
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "waiting_for_reset_sys_wifi_off_ind"));

    /* Do not try to talk to the Firmware */
    hip_auto_cfm_send_events(getSmeContext(context)->hipSapAuto);

    /* Save this for AFTER the mlme_reset Cfm */
    fsm_saved_event(context, (const FsmEvent*)ind);
}

/**
 * @brief
 *   Request from the driver to stop when starting
 *
 *
 * @param[in]    context : FSM context
 * @param[in]    ind     : event
 *
 * @return
 *   void
 */
static void sys_wifi_off_ind(FsmContext* context, const UnifiSysWifiOffInd_Evt *ind)
{
    UnifiDriverData* fsmData = FSMDATA;
    CsrBool sentInd = FALSE;
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "sys_wifi_off_ind"));

    if (fsmData->generalPurposeTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->generalPurposeTimerId);
        fsmData->generalPurposeTimerId.uniqueid = 0;
    }

    if (fsmData->fwWatchdogTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->fwWatchdogTimerId);
        fsmData->fwWatchdogTimerId.uniqueid = 0;
    }

    if (fsmData->startingUp)
    {
        send_core_start_cfm(context, fsmData->corePid, unifi_Error);
        fsmData->startingUp = FALSE;
    }
    else if (fsmData->closingDown)
    {
        send_core_stop_cfm(context, fsmData->corePid, unifi_Error);
        fsmData->closingDown = FALSE;
    }
    else
    {
        sentInd = TRUE;
        send_internal_stop_req(context, getSmeContext(context)->smeCoreInstance, ind->controlIndication);
    }

    /* Always send exit Indications */
    if (!sentInd && ind->controlIndication == unifi_ControlExit)
    {
        send_internal_stop_req(context, getSmeContext(context)->smeCoreInstance, ind->controlIndication);
    }

    /* Do not try to talk to the Firmware */
    hip_auto_cfm_send_events(getSmeContext(context)->hipSapAuto);
    call_unifi_sys_wifi_off_rsp(context);

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   Determines next course of action from UniFi Driver Start response
 *
 * @par Description
 *   The UniFi driver has sent a response to the start request.  This will
 *   either be success or error, so move to an appropriate state.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : content of UnifiSysStartCfm
 *
 * @return
 *   void
 */
static void sys_wifi_on_ind(FsmContext* context, const UnifiSysWifiOnInd_Evt *ind)
{
    UnifiDriverData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    unifi_Status status = unifi_Success;
    unifi_SmeVersions smeVersions;
    smeVersions.firmwarePatch = cfg->versions.firmwarePatch;
    smeVersions.smeBuild      = cfg->versions.smeBuild;
    smeVersions.smeHip        = cfg->versions.smeHip;
    smeVersions.smeIdString   = cfg->versions.smeIdString;
    smeVersions.smeVariant    = cfg->versions.smeVariant;

    sme_trace_info((TR_UNIFI_DRIVER_FSM,  "sys_wifi_on_ind: UnifiSysStartInd; Versions: Chip ID %#x, version %#x; firmware build %u, HIP version %#06x; driver build %u, HIP version %#06x\n",
        ind->driverVersions.chipId, ind->driverVersions.chipVersion,
        ind->driverVersions.firmwareBuild, ind->driverVersions.firmwareHip,
        ind->driverVersions.driverBuild, ind->driverVersions.driverHip));

    /* This is a result of a D3 suspend, ignore and return */
    if (fsmData->d3WifiOnReqSent)
    {
        sme_trace_debug((TR_UNIFI_DRIVER_FSM, "sys_wifi_on_ind: D3 resume, ignore"));
        fsmData->d3WifiOnReqSent = FALSE;
        return;
    }

    fsm_remove_timer(context, fsmData->generalPurposeTimerId);
    fsmData->generalPurposeTimerId.uniqueid = 0;

    /* Driver had an error */
    if (ind->status != unifi_Success)
    {
        fsmData->startingUp = FALSE;
        call_unifi_sys_wifi_on_rsp(context, ind->status, &cfg->stationMacAddress, &smeVersions, FALSE);
        fsm_next_state(context, FSMSTATE_waiting_wifi_on_cfm);
        return;
    }

    /* Copy the current version information into our private data */
    cfg->versions.chipId = ind->driverVersions.chipId;
    cfg->versions.chipVersion = ind->driverVersions.chipVersion;
    cfg->versions.firmwareBuild = ind->driverVersions.firmwareBuild;
    cfg->versions.firmwareHip = ind->driverVersions.firmwareHip;
    cfg->versions.driverBuild = ind->driverVersions.driverBuild;
    cfg->versions.driverHip = ind->driverVersions.driverHip;

    /* Test that HIP versions are all compatible, assuming that the information is available */
    if ((cfg->versions.firmwareHip != 0) && (cfg->versions.driverHip != 0))
    {
        /* We refuse to start if there are Major version differences */
        if ((HIP_VERSION_MAJOR_NUMBER(cfg->versions.firmwareHip)) != (HIP_VERSION_MAJOR_NUMBER(cfg->versions.driverHip)) ||
            (HIP_VERSION_MINOR_NUMBER(cfg->versions.firmwareHip)) <  (HIP_VERSION_MINOR_NUMBER(cfg->versions.driverHip)))
        {
            sme_trace_error((TR_UNIFI_DRIVER_FSM,
                "sys_wifi_on_ind: firmware(0x%.4X) and driver(0x%.4X) HIP versions are incompatible",
                cfg->versions.firmwareHip, cfg->versions.driverHip));
            status = unifi_Error;
        }
        else if ((HIP_VERSION_MAJOR_NUMBER(cfg->versions.firmwareHip)) != (HIP_VERSION_MAJOR_NUMBER(cfg->versions.smeHip)) ||
                 (HIP_VERSION_MINOR_NUMBER(cfg->versions.firmwareHip)) <  (HIP_VERSION_MINOR_NUMBER(cfg->versions.smeHip)))
        {
            sme_trace_error((TR_UNIFI_DRIVER_FSM,
                    "sys_wifi_on_ind: firmware(0x%.4X) and sme(0x%.4X) HIP versions are incompatible",
                cfg->versions.firmwareHip, cfg->versions.smeHip));
            status = unifi_Error;
        }
        /* We just warn if there are Minor version differences */
        sme_trace_warn_code
        (
        if ((HIP_VERSION_MINOR_NUMBER(cfg->versions.firmwareHip)) != (HIP_VERSION_MINOR_NUMBER(cfg->versions.driverHip)))
        {
            sme_trace_warn((TR_UNIFI_DRIVER_FSM,
                "sys_wifi_on_ind: firmware (%#06x) and driver (%#06x) HIP version minor numbers are different",
                cfg->versions.firmwareHip, cfg->versions.driverHip));
        }
        else if ((HIP_VERSION_MINOR_NUMBER(cfg->versions.firmwareHip)) != (HIP_VERSION_MINOR_NUMBER(cfg->versions.smeHip)))
        {
            sme_trace_warn((TR_UNIFI_DRIVER_FSM,
                "sys_wifi_on_ind: firmware/driver (%#06x) and sme (%#06x) HIP version minor numbers are different",
                cfg->versions.firmwareHip, cfg->versions.driverHip));
        }
        );
    }
    else
    {
        sme_trace_warn((TR_UNIFI_DRIVER_FSM, "sys_wifi_on_ind: firmware and driver HIP versions unavailable"));
    }

    /* The FW/Driver/SME versions are not compatable. Fail the startup! */
    if (status == unifi_Error)
    {
        fsmData->startingUp = FALSE;
        call_unifi_sys_wifi_on_rsp(context, status, &cfg->stationMacAddress, &smeVersions, FALSE);
        fsm_next_state(context, FSMSTATE_waiting_wifi_on_cfm);
        return;
    }


    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "sys_wifi_on_ind: UnifiSysStartCfm reported success"));
    /* Set the auto cfm mechanism to allow mlme messages */
    hip_auto_cfm_store_events(getSmeContext(context)->hipSapAuto);

    /* Check if UniFi was powered prior to this start (resume) request.
     * If so, we must inform the core that we are 'ready' and skip most
     * of the initialisation sequence. Of particular importance is that
     * we do not wait for a connected_ind(false) from the firmware as
     * this only happens on the first mlme_reset after a power-cycle
     * (i.e. the SME will get stuck if UniFi isn't power cycled and
     * we wait for a connected_ind). */
    if (core_get_startupPowerMaintainedFlag(context, getSmeContext(context)->smeCoreInstance))
    {    hip_auto_cfm_store_events(getSmeContext(context)->hipSapAuto);

        sme_trace_info((TR_UNIFI_DRIVER_FSM, "power was maintained since last shutdown, engaging quick startup"));

        /* Start the FW Watchdog Timer */
        send_unifi_fw_watchdog_timer(context, FSMDATA->fwWatchdogTimerId, FW_WATCHDOG_POLL_INTERVAL, 200);

        call_unifi_sys_wifi_on_rsp(context, unifi_Success, &cfg->stationMacAddress, &smeVersions, FALSE);
        fsm_next_state(context, FSMSTATE_waiting_wifi_on_cfm);
    }
    else
    {
        /* Start the FW Watchdog Timer */
        send_unifi_fw_watchdog_timer(context, FSMDATA->fwWatchdogTimerId, FW_WATCHDOG_POLL_INTERVAL, 200);

        /* Tell the mib_access FSM to initialise the UniFi MIB */
        send_mib_init_req(context, getSmeContext(context)->mibAccessInstance);
        fsm_next_state(context, FSMSTATE_mib_init);
    }
}

/**
 * @brief
 *   End of Driver startup.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : event
 *
 * @return
 *   void
 */
static void sys_wifi_on_cfm(FsmContext* context, const UnifiSysWifiOnCfm_Evt *cfm)
{
    UnifiDriverData* fsmData = FSMDATA;
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "sys_wifi_on_cfm(%s)", trace_MibStatus(cfm->status)));

    /* This is a result of a D3 suspend, ignore and return */
    if (fsmData->d3WifiOnReqSent)
    {
        sme_trace_debug((TR_UNIFI_DRIVER_FSM, "sys_wifi_on_ind: D3 resume, ignore"));
        fsmData->d3WifiOnReqSent = FALSE;
        return;
    }

    fsm_remove_timer(context, fsmData->generalPurposeTimerId);
    fsmData->generalPurposeTimerId.uniqueid = 0;

    fsmData->startingUp = FALSE;
    if (cfm->status != unifi_Success)
    {
        if (fsmData->generalPurposeTimerId.uniqueid != 0)
        {
            fsm_remove_timer(context, fsmData->generalPurposeTimerId);
            fsmData->generalPurposeTimerId.uniqueid = 0;
        }

        if (fsmData->fwWatchdogTimerId.uniqueid != 0)
        {
            fsm_remove_timer(context, fsmData->fwWatchdogTimerId);
            fsmData->fwWatchdogTimerId.uniqueid = 0;
        }

        /* Close the Hip Sap */
        hip_auto_cfm_store_events(getSmeContext(context)->hipSapAuto);
        send_core_start_cfm(context, fsmData->corePid, cfm->status);
#ifdef ANDROID
        /*
         * NB: Might be better implementing this in a custom implementation of unifi_mgt_wifi_on_cfm
         */
        property_set("wlan.driver.status", "failed");
#endif
        fsm_next_state(context, FSMSTATE_stopped);
        return;
    }

    /*
     * Check current state - if sys_wifi_on_cfm(success) then must have
     * previously received a sys_wifi_on_ind.
     */
    verify(TR_UNIFI_DRIVER_FSM, context->currentInstance->state == FSMSTATE_waiting_wifi_on_cfm);

    /* This is to guarantee the state of the Controlled port at the start of the next connect */
    (void)ns_port_configure(context, unifi_8021xPortClosedDiscard, unifi_8021xPortClosedDiscard);

    power_send_sys_configure_power_mode(context, TRUE);

    send_core_start_cfm(context, fsmData->corePid, cfm->status);
#ifdef ANDROID
        /*
         * NB: Might be better implementing this in a custom implementation of unifi_mgt_wifi_on_cfm
         */
        property_set("wlan.driver.status", "ok");
#endif
    fsm_next_state(context, FSMSTATE_ready);
}

/**
 * @brief
 *   End of Driver startup.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : event
 *
 * @return
 *   void
 */
static void sys_wifi_on_cfm_timer_expired(FsmContext* context, const UnifiGeneralTimer_Evt* timer)
{
    UnifiDriverData* fsmData = FSMDATA;
    sme_trace_error((TR_UNIFI_DRIVER_FSM, "sys_wifi_on_cfm_timer_expired()"));

    if (fsmData->generalPurposeTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->generalPurposeTimerId);
        fsmData->generalPurposeTimerId.uniqueid = 0;
    }

    if (fsmData->fwWatchdogTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->fwWatchdogTimerId);
        fsmData->fwWatchdogTimerId.uniqueid = 0;
    }

    send_core_start_cfm(context, fsmData->corePid, unifi_TimedOut);
    fsmData->startingUp = FALSE;

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   UniFi MIB initialisation confirmation has returned
 *
 * @par Description
 *   The MIB has now been initialised, in which case the settings are made
 *   permanent by sending a reset to UniFi.
 *
 *   If the MIB initialisation failed, we abort the startup sequence.
 *
 * @param[in]    context            : FSM context
 * @param[in]    confirm            : Result of the MIB Initialisation
 *
 * @return
 *  void
 */
static void mib_init__mib_init_cfm(FsmContext *context, const MibInitCfm_Evt *cfm)
{
    UnifiDriverData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "mib_init__mib_init_cfm(%s)", trace_MibStatus(cfm->status)));

    fsm_remove_timer(context, fsmData->generalPurposeTimerId);
    fsmData->generalPurposeTimerId.uniqueid = 0;

    if (cfm->status == MibStatus_Successful)
    {
        FsmEvent* autoevt = NULL;

        /* Reset UniFi which will make the MIB setup permanent across
         * subsequent soft-resets.
         * NOTE: this is not quite the same reset as that generated by
         * gn_driver_reset, because this one specifies our configured MAC
         * address, rather than the broadcast MAC address.
         */
        send_mlme_reset_req(context, get_sme_config(context)->stationMacAddress, TRUE); /*lint !e666*/

        /* Add a dummy connected ind to the Auto cfm queue so
         * that an error or hard suspend will flush immediatly
         * This is only used For the initial startup mlme reset */
        build_mlme_connected_ind(autoevt, getSmeContext(context)->unifiDriverInstance, ConnectionStatus_Disconnected, get_sme_config(context)->stationMacAddress);
        hip_auto_cfm_add(getSmeContext(context)->hipSapAuto, autoevt);

        /* On the Reset the Power mode is reset */
        reset_power_state(context);

        /* reset autonomous scan on mlme reset */
        reset_autonomous_scan(context);

        /* reset link quality on mlme reset */
        reset_link_quality(context);

        /* Set driver reset timeout */
        send_unifi_general_timer(context, fsmData->generalPurposeTimerId,  TIMEOUT_FIVE_SECONDS, 1000);

        fsm_next_state(context, FSMSTATE_waiting_for_mlme_reset_cfm);
    }
    else
    {
        unifi_SmeVersions smeVersions;
        smeVersions.firmwarePatch = cfg->versions.firmwarePatch;
        smeVersions.smeBuild      = cfg->versions.smeBuild;
        smeVersions.smeHip        = cfg->versions.smeHip;
        smeVersions.smeIdString   = cfg->versions.smeIdString;
        smeVersions.smeVariant    = cfg->versions.smeVariant;
        sme_trace_error((TR_UNIFI_DRIVER_FSM, "Initialisation of MIB failed (%s)", trace_MibStatus(cfm->status)));
        call_unifi_sys_wifi_on_rsp(context, unifi_Error, &cfg->stationMacAddress, &smeVersions, FALSE);
        fsm_next_state(context, FSMSTATE_waiting_wifi_on_cfm);
    }
}

/**
 * @brief
 *   Determines next course of action from UniFi Driver Stop response
 *
 * @par Description
 *   The UniFi driver has sent a response to the stop request.  This will
 *   either be success or error, so move to an appropriate state.
 *
 * @param[in]    context : FSM context
 * @param[in]    cfm     : event
 *
 * @return
 *   void
 */
static void driver_stop_response(FsmContext* context, const UnifiSysWifiOffCfm_Evt *cfm)
{
    UnifiDriverData* fsmData = FSMDATA;

    /* Cancel the pending timeout */
    if (fsmData->generalPurposeTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->generalPurposeTimerId);
        fsmData->generalPurposeTimerId.uniqueid = 0;
    }

    if (fsmData->fwWatchdogTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->fwWatchdogTimerId);
        fsmData->fwWatchdogTimerId.uniqueid = 0;
    }

    send_core_stop_cfm(context, fsmData->corePid, unifi_Success);
    fsmData->closingDown = FALSE;

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   UniFi Driver did not reply to start request within the time allowed
 *
 * @par Description
 *  A timeout was received while waiting for a response from the UniFi
 *  device driver.  Move to the appropriate next state.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void driver_start_timer_expired(FsmContext *context, const UnifiGeneralTimer_Evt* timer)
{
    UnifiDriverData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    unifi_SmeVersions smeVersions;
    smeVersions.firmwarePatch = cfg->versions.firmwarePatch;
    smeVersions.smeBuild      = cfg->versions.smeBuild;
    smeVersions.smeHip        = cfg->versions.smeHip;
    smeVersions.smeIdString   = cfg->versions.smeIdString;
    smeVersions.smeVariant    = cfg->versions.smeVariant;

    sme_trace_error((TR_UNIFI_DRIVER_FSM, "driver_start_timer_expired: UnifiSysStartCfm timed out"));

    if (fsmData->generalPurposeTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->generalPurposeTimerId);
        fsmData->generalPurposeTimerId.uniqueid = 0;
    }

    if (fsmData->fwWatchdogTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->fwWatchdogTimerId);
        fsmData->fwWatchdogTimerId.uniqueid = 0;
    }

    call_unifi_sys_wifi_on_rsp(context, unifi_Error, &cfg->stationMacAddress, &smeVersions, FALSE);
    send_core_start_cfm(context, fsmData->corePid, unifi_TimedOut);

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   Close down the UniFi device and driver
 *
 * @par Description
 *   The UniFi device and driver should be silenced and halted where
 *   possible.
 *
 * @param[in]    context            : FSM context
 *
 * @return
 *  void
 */
static void driver_stop(FsmContext *context, const CoreStopReq_Evt *req)
{
    sme_trace_debug((TR_UNIFI_DRIVER_FSM, "driver_stop"));

    FSMDATA->closingDown = TRUE;
    if (core_get_flightmodeBootFlag(context))
    {
        /* Skip the mlme_reset when shuting down after a flight mode boot */
        gn_driver_stop(context);
    }
    else
    {
        gn_driver_reset(context);
        fsm_next_state(context, FSMSTATE_waiting_for_mlme_reset_cfm);
    }
}

/**
 * @brief
 *   Soft-reset the UniFi device to make sure it is not doing anything
 *
 * @par Description
 *   An MLME_RESET_REQ is sent to the UniFi device
 *
 * @param[in]    context            : FSM context
 *
 * @return
 *  void
 */
static void driver_soft_reset(FsmContext *context, const UnifiResetReq_Evt *req)
{
    UnifiDriverData* fsmData = FSMDATA;
    sme_trace_debug((TR_UNIFI_DRIVER_FSM, "driver_soft_reset"));

    /* Store the PID of the sender of the UnifiResetReq_Evt */
    fsmData->resetRequester = req->common.sender_;

    /* Make sure the current state is correct */
    fsmData->closingDown = FALSE;
    gn_driver_reset(context);

    fsm_next_state(context, FSMSTATE_waiting_for_mlme_reset_cfm);
}

/**
 * @brief
 *   Determines next course of action from UniFi Driver Reset response
 *
 * @par Description
 *   The UniFi driver has sent a response to the MLME_RESET request.  This
 *   will either be success or error, so move to an appropriate state.
 *
 * @param[in]    context   : FSM context
 * @param[in]    confirm   : content of MlmeResetCfm
 *
 * @return
 *   void
 */
static void waiting_for_reset_mlme_reset_cfm(FsmContext* context, const MlmeResetCfm_Evt *cfm)
{
    UnifiDriverData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    /* Cancel the pending timeout */
    fsm_remove_timer(context, fsmData->generalPurposeTimerId);
    fsmData->generalPurposeTimerId.uniqueid = 0;

    /* Always send an internal stop on error unless the hip sap is closed */
    if (cfm->resultCode != ResultCode_Success && hip_auto_cfm_is_hip_sap_open(getSmeContext(context)->hipSapAuto))
    {
        sme_trace_info((TR_UNIFI_DRIVER_FSM,  "waiting_for_reset_mlme_reset_cfm: Reset Failure sending internal stop"));
        send_internal_stop_req(context, getSmeContext(context)->smeCoreInstance, unifi_ControlError);
    }

    if (cfm->resultCode == ResultCode_Success)
    {
        sme_trace_entry((TR_UNIFI_DRIVER_FSM, "waiting_for_reset_mlme_reset_cfm: MlmeResetCfm reported success"));

        if (fsmData->startingUp == TRUE)
        {
            /* The final thing to happen in the startup sequence is that a
             * mlme_connected_ind will arrive to show that the UniFi
             * firmware has completed initialisation following the reset.
             * We need to wait for this so that no-one tries to use the
             * UniFi before it is ready.
             */

            sme_trace_entry((TR_UNIFI_DRIVER_FSM, "SET TIMER"));
            /* Set a connected_ind timeout, just in case */
            send_unifi_general_timer(context, fsmData->generalPurposeTimerId, TIMEOUT_TWENTY_SECONDS, 1000);

            fsm_next_state(context, FSMSTATE_waiting_for_connected_ind);
            return;
        }
        else
        {
            if (fsmData->closingDown == TRUE)
            {
                /* We are here as part of a stop request.  The reset has
                 * been done so we just need to stop the driver.
                 */
                gn_driver_stop(context);
                return;
            }
            else
            {
                /* We are here because we asked for a soft reset.  Tell the
                 * requestor that it has completed.
                 */
                send_unifi_reset_cfm(context, fsmData->resetRequester);
            }
        }

        power_send_sys_configure_power_mode(context, TRUE);

        /* Wait for further commands */
        fsm_next_state(context, FSMSTATE_ready);
    }
    else
    {
        sme_trace_error((TR_UNIFI_DRIVER_FSM, "waiting_for_reset_mlme_reset_cfm: MlmeResetCfm reported failure (%#x)", cfm->resultCode));

        if (fsmData->startingUp == TRUE)
        {
            unifi_SmeVersions smeVersions;
            smeVersions.firmwarePatch = cfg->versions.firmwarePatch;
            smeVersions.smeBuild      = cfg->versions.smeBuild;
            smeVersions.smeHip        = cfg->versions.smeHip;
            smeVersions.smeIdString   = cfg->versions.smeIdString;
            smeVersions.smeVariant    = cfg->versions.smeVariant;
            /* Tell outside world that the start has been unsuccessful */
            call_unifi_sys_wifi_on_rsp(context, unifi_Error, &cfg->stationMacAddress, &smeVersions, FALSE);
            fsm_next_state(context, FSMSTATE_waiting_wifi_on_cfm);
        }
        else if (FSMDATA->closingDown == FALSE)
        {
            /* Tell the soft reset requestor that the reset procedure completed,
             * even if it was unsuccessful. */
            send_unifi_reset_cfm(context, FSMDATA->resetRequester);

            /* Wait for further commands (hopefully soon being the
             * core_stop_req) */
            fsm_next_state(context, FSMSTATE_ready);
        }
        else
        {
            /* there is nothing to say if we were shutting down anyway */
            /* Try to stop the driver */
            gn_driver_stop(context);
        }
    }
}

/**
 * @brief
 *   UniFi Driver did not reply to reset request within the time allowed
 *
 * @par Description
 *  A timeout was received while waiting for a response from the UniFi
 *  device driver.  Move to the appropriate next state.
 *
 * @param[in]    context            : FSM context
 *
 * @return
 *  void
 */
static void driver_reset_timer_expired(FsmContext *context, const UnifiGeneralTimer_Evt *timer)
{
    sme_trace_error((TR_UNIFI_DRIVER_FSM,  "driver_reset_timer_expired: MlmeResetCfm timed out"));
    send_internal_stop_req(context, getSmeContext(context)->smeCoreInstance, unifi_ControlError);
    hip_auto_cfm_send_events(getSmeContext(context)->hipSapAuto);

}

/**
 * @brief
 *   UniFi Driver did not reply to stop request within the time allowed
 *
 * @par Description
 *  A timeout was received while waiting for a response from the UniFi
 *  device driver.  Move to the appropriate next state.
 *
 * @param[in]    context            : FSM context
 *
 * @return
 *  void
 */
static void driver_stop_timer_expired(FsmContext *context, const UnifiGeneralTimer_Evt *timer)
{
    UnifiDriverData* fsmData = FSMDATA;
    sme_trace_error((TR_UNIFI_DRIVER_FSM,  "driver_stop_timer_expired: UnifiSysStopCfm timed out"));

    if (fsmData->generalPurposeTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->generalPurposeTimerId);
        fsmData->generalPurposeTimerId.uniqueid = 0;
    }

    if (fsmData->fwWatchdogTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->fwWatchdogTimerId);
        fsmData->fwWatchdogTimerId.uniqueid = 0;
    }

    if (fsmData->closingDown == TRUE)
    {
        send_core_stop_cfm(context, fsmData->corePid, unifi_TimedOut);
        fsmData->closingDown = FALSE;
    }

    fsm_next_state(context, FSMSTATE_stopped);
}

static void connected_ind(FsmContext *context, const MlmeConnectedInd_Evt *ind)
{
    UnifiDriverData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    unifi_SmeVersions smeVersions;
    smeVersions.firmwarePatch = cfg->versions.firmwarePatch;
    smeVersions.smeBuild      = cfg->versions.smeBuild;
    smeVersions.smeHip        = cfg->versions.smeHip;
    smeVersions.smeIdString   = cfg->versions.smeIdString;
    smeVersions.smeVariant    = cfg->versions.smeVariant;
    sme_trace_info((TR_UNIFI_DRIVER_FSM, "Received MlmeConnectedInd(%s) - firmware setup complete, allowing startup to continue",
                    trace_ConnectionStatus(ind->connectionStatus)));

    /* Cancel the pending timeout */
    fsm_remove_timer(context, fsmData->generalPurposeTimerId);
    fsmData->generalPurposeTimerId.uniqueid = 0;
    call_unifi_sys_wifi_on_rsp(context, unifi_Success, &cfg->stationMacAddress, &smeVersions, cfg->scheduledInterrupt);
    fsm_next_state(context, FSMSTATE_waiting_wifi_on_cfm);
}

/**
 * @brief
 *   UniFi did not send the expected connected_ind within the time allowed
 *
 * @par Description
 *  A timeout was received while waiting for a connected_ind from the UniFi
 *  firmware, which is supposed to signal that it is ready for use.  This is
 *  a fatal startup error as far as we are concerned.
 *
 * @param[in]    context   : FSM context
 * @param[in]    timer     : The timer that fired so we don't get stuck
 *
 * @return
 *   void
 */
static void connected_ind_timer_expired(FsmContext *context, const UnifiGeneralTimer_Evt *timer)
{
    SmeConfigData* cfg = get_sme_config(context);
    unifi_SmeVersions smeVersions;
    smeVersions.firmwarePatch = cfg->versions.firmwarePatch;
    smeVersions.smeBuild      = cfg->versions.smeBuild;
    smeVersions.smeHip        = cfg->versions.smeHip;
    smeVersions.smeIdString   = cfg->versions.smeIdString;
    smeVersions.smeVariant    = cfg->versions.smeVariant;

    sme_trace_error((TR_UNIFI_DRIVER_FSM, "connected_ind_timer_expired()"));
    call_unifi_sys_wifi_on_rsp(context, unifi_Error, &cfg->stationMacAddress, &smeVersions, FALSE);
    fsm_next_state(context, FSMSTATE_waiting_wifi_on_cfm);
}

/**
 * @brief
 *   Reset the UniFi Driver
 *
 * @par Description
 *   A reset request is sent to the UniFi driver, and a timeout is set up in
 *   case the driver never responds.
 *
 * @param[in] context : FSM context
 *
 * @return
 *  void
 */
static void gn_driver_reset(FsmContext *context)
{
    UnifiDriverData* fsmData = FSMDATA;
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "gn_driver_reset: send_mlme_reset_req and set timeout"));

    send_mlme_reset_req(context, BroadcastBssid, TRUE);

    /* On the Reset the Power mode is reset */
    reset_power_state(context);

    /* reset autonomous scan on mlme reset */
    reset_autonomous_scan(context);

    /* reset link quality on mlme reset */
    reset_link_quality(context);

    /* Set driver reset timeout */
    send_unifi_general_timer(context, fsmData->generalPurposeTimerId, TIMEOUT_FIVE_SECONDS, 1000);
}

/**
 * @brief
 *   Stop the UniFi Driver
 *
 * @par Description
 *   A stop request is sent to the UniFi driver, and a timeout is set up in
 *   case the driver never responds.
 *
 * @param[in] context : FSM context
 *
 * @return
 *  void
 */
static void gn_driver_stop(FsmContext *context)
{
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "gn_driver_stop: unifi_dd_stop_req and set timeout"));

    /* Stop the FW Watchdog Timer */
    if (FSMDATA->fwWatchdogTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, FSMDATA->fwWatchdogTimerId);
        FSMDATA->fwWatchdogTimerId.uniqueid = 0;
    }

    /* no longer send or accept hip messages */
    hip_auto_cfm_send_events(getSmeContext(context)->hipSapAuto);

    /* Try to close the driver gracefully */
    call_unifi_sys_wifi_off_req(context, NULL);

    /* Set driver shutdown timeout */
    send_unifi_general_timer(context, FSMDATA->generalPurposeTimerId,  TIMEOUT_FIVE_SECONDS, 1000);

    fsm_next_state(context, FSMSTATE_waiting_wifi_off_cfm);
}

/**
 * @brief
 *   Deal with an unsolicited unifi_reset_cfm
 *
 * @par Description
 *   This function is used when a unifi_reset_confirm is received
 *   unexpectedly.  Empirical evidence suggests this happens if UniFi or its
 *   driver get out of step with unifi_driver_fsm.
 *
 * @param[in] context : FSM context
 *
 * @return
 *  void
 */
static void gn_unexpected_reset_confirm(FsmContext *context, const MlmeResetCfm_Evt *cfm)
{
    sme_trace_warn((TR_UNIFI_DRIVER_FSM, "gn_unexpected_reset_confirm: received"));
}

/**
 * @brief
 *   Poll the driver to check for a fw crash
 *
 * @param[in] context : FSM context
 * @param[in] timer   : timer
 *
 * @return
 *  void
 */
static void gn_check_hw(FsmContext *context, const UnifiFwWatchdogTimer_Evt *timer)
{
    sme_trace_debug((TR_UNIFI_DRIVER_FSM, "gn_check_hw\n"));
    if (hip_auto_cfm_message_timedout(getSmeContext(context)->hipSapAuto, FW_CFM_RESPONSE_TIME) == TRUE)
    {
        /* Timeout waiting for a HIP message
         * Stop the SME! */
        sme_trace_crit((TR_UNIFI_DRIVER_FSM, "gn_check_hw() :: HIP cfm message not received in time...\n"));
        hip_auto_cfm_send_events(getSmeContext(context)->hipSapAuto);
        send_internal_stop_req(context, getSmeContext(context)->smeCoreInstance, unifi_ControlError);
    }
    else
    {
        send_unifi_fw_watchdog_timer(context, FSMDATA->fwWatchdogTimerId, FW_WATCHDOG_POLL_INTERVAL, 200);
    }

    sme_trace_debug((TR_UNIFI_DRIVER_FSM, "check scan result ages..."));
    srs_expire_old_scan_data(context);
}

/**
 * @brief
 *   Mention that a connection_ind was received, as it may be of interest
 *
 * @par Description
 *   This connection_ind was forwarded to this FSM by hip_proxy_fsm because
 *   there was no Connection Manager available to receive it.
 *
 *   We were not in the state where we were waiting for it (duing firmware
 *   setup) so we are not going to do anything with it for the moment, other
 *   than to display it, because it might become useful in future.
 *
 * @param[in] context : FSM context
 *
 * @return
 *  void
 */
static void gn_unhandled__connected_ind(FsmContext *context, const MlmeConnectedInd_Evt *ind)
{
    sme_trace_info((TR_UNIFI_DRIVER_FSM, "Received MlmeConnectedInd(%s) outside of firmware setup context - ignoring",
                    trace_ConnectionStatus(ind->connectionStatus)));
}

/**
 * @brief Handle stray wifi on ind
 *
 * @return
 *  void
 */
static void sys_wifi_on_ind__success(FsmContext *context, const UnifiSysWifiOnInd_Evt* ind)
{
    UnifiDriverData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    unifi_SmeVersions smeVersions;
    smeVersions.firmwarePatch = cfg->versions.firmwarePatch;
    smeVersions.smeBuild      = cfg->versions.smeBuild;
    smeVersions.smeHip        = cfg->versions.smeHip;
    smeVersions.smeIdString   = cfg->versions.smeIdString;
    smeVersions.smeVariant    = cfg->versions.smeVariant;
    sme_trace_info((TR_UNIFI_DRIVER_FSM, "sys_wifi_on_ind__success()"));

    /* no need to act on this, just clear the flag */
    fsmData->d3WifiOnReqSent = FALSE;

    call_unifi_sys_wifi_on_rsp(context, unifi_Success, &cfg->stationMacAddress, &smeVersions, cfg->scheduledInterrupt);
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in unifi_driver_fsm/unifi_driver_fsm.h
 */
void ufd_move_to_d0_state(FsmContext* context)
{
    UnifiDriverData* fsmData; /* set after context change*/
    FsmInstanceEntry* currentInstance = fsm_save_current_instance_by_id(context, getSmeContext(context)->unifiDriverInstance);

    /* clear to run normally */
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "ufd_move_to_d0_state"));

    fsmData = FSMDATA;

    /* Start the FW Watchdog Timer */
    if (fsmData->fwWatchdogTimerId.uniqueid == 0)
    {
        send_unifi_fw_watchdog_timer(context, fsmData->fwWatchdogTimerId, FW_WATCHDOG_POLL_INTERVAL, 200);
    }

    /* send a wifi on to the driver to keep it happy */
    call_unifi_sys_wifi_on_req(context, NULL);
    fsmData->d3WifiOnReqSent = TRUE;

    /* restore the calling context */
    fsm_restore_current_instance(context, currentInstance);
}

/*
 * Description:
 * See description in unifi_driver_fsm/unifi_driver_fsm.h
 */
void ufd_move_to_d3_state(FsmContext* context)
{
    UnifiDriverData* fsmData = fsm_get_params_by_id(context, getSmeContext(context)->unifiDriverInstance, UnifiDriverData);

    /*
     * NOTE : No context change required, remove Timer is context insensitive
     */
    sme_trace_entry((TR_UNIFI_DRIVER_FSM, "ufd_move_to_d3_state"));
    /* Stop the FW Watchdog Timer */
    if (fsmData->fwWatchdogTimerId.uniqueid != 0)
    {
        fsm_remove_timer(context, fsmData->fwWatchdogTimerId);
        fsmData->fwWatchdogTimerId.uniqueid = 0;
    }
}


/* FSM DEFINITION **********************************************/
/** State stopped transitions */
static const FsmEventEntry stoppedTransitions[] =
{
    /* Signal Id,                                                       Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                            stopped__core_start),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                             stopped__core_stop),
    fsm_event_table_entry(UNIFI_SYS_WIFI_OFF_IND_ID,                    stopped__sys_wifi_off_ind),
    fsm_event_table_entry(UNIFI_SYS_WIFI_OFF_CFM_ID,                    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_RESET_REQ_ID,                           stopped__soft_reset),
};

/** Event handling in waiting_for_driver_startup while waiting for unifi_dd_start_cfm signal */
static const FsmEventEntry waitingForWifiOnIndTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_SYS_WIFI_ON_IND_ID,                     sys_wifi_on_ind),
    fsm_event_table_entry(UNIFI_SYS_WIFI_ON_CFM_ID,                     sys_wifi_on_cfm),
    fsm_event_table_entry(MLME_RESET_CFM_ID,                            fsm_ignore_event),
    fsm_event_table_entry(UNIFI_GENERAL_TIMER_ID,                       driver_start_timer_expired)
};

/** Event handling in waiting_for_mib_init while waiting for mib_init_cfm signal */
static const FsmEventEntry mibInitTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(MIB_INIT_CFM_ID,                              mib_init__mib_init_cfm),
};

/** Event handling in waiting_for_connected_ind while waiting for connected_ind signal */
static const FsmEventEntry waitingForConnectedIndTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                        connected_ind),
    fsm_event_table_entry(UNIFI_GENERAL_TIMER_ID,                       connected_ind_timer_expired),
};

/** Event handling in waiting_for_driver_reset while waiting for mlme_reset_cfm signal */
static const FsmEventEntry waitingForMlmeResetTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(MLME_RESET_CFM_ID,                            waiting_for_reset_mlme_reset_cfm),
    fsm_event_table_entry(UNIFI_GENERAL_TIMER_ID,                       driver_reset_timer_expired),
    fsm_event_table_entry(UNIFI_SYS_WIFI_OFF_IND_ID,                    waiting_for_reset_sys_wifi_off_ind),
};

static const FsmEventEntry waitingForWifiOnCfmTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_SYS_WIFI_ON_CFM_ID,                     sys_wifi_on_cfm),
    fsm_event_table_entry(UNIFI_GENERAL_TIMER_ID,                       sys_wifi_on_cfm_timer_expired),
};

/** State idle transitions */
static const FsmEventEntry readyTransitions[] =
{
    /* Signal Id,                                                       Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                             driver_stop),
    fsm_event_table_entry(UNIFI_RESET_REQ_ID,                           driver_soft_reset),
};

static const FsmEventEntry waitingWifiOffTransitions[] =
{
    /* Signal Id,                                                       Function */
    fsm_event_table_entry(UNIFI_SYS_WIFI_OFF_CFM_ID,                    driver_stop_response),
    fsm_event_table_entry(UNIFI_GENERAL_TIMER_ID,                       driver_stop_timer_expired),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_RESET_CFM_ID,                      gn_unexpected_reset_confirm),
    fsm_event_table_entry(UNIFI_FW_WATCHDOG_TIMER_ID,             gn_check_hw),
    /* The HIP Proxy will forward any mlme_connected_ind messages received
     * if there is no Connection Manager available to process them.  The
     * very first one to arrive is used to tell us that the UniFi firmware
     * has completed initialisation and is ready for action.  Any others are
     * strays which should not happen in normal running and can be ignored
     * if they do happen.
     */
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,                  gn_unhandled__connected_ind),
    fsm_event_table_entry(UNIFI_SYS_WIFI_OFF_IND_ID,              sys_wifi_off_ind),
    fsm_event_table_entry(MIB_INIT_CFM_ID,                        fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_WIFI_ON_IND_ID,               sys_wifi_on_ind__success),
    fsm_event_table_entry(UNIFI_SYS_WIFI_ON_CFM_ID,               fsm_ignore_event),
};


/** State table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                       State                                           State                       Save  */
   /*                       Name                                            Transitions                 *     */
   fsm_state_table_entry(FSMSTATE_stopped,                      stoppedTransitions,                    FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_for_wifi_on_ind,      waitingForWifiOnIndTransitions,        FALSE),
   fsm_state_table_entry(FSMSTATE_mib_init,                     mibInitTransitions,                    FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_for_connected_ind,    waitingForConnectedIndTransitions,     FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_wifi_on_cfm,          waitingForWifiOnCfmTransitions,        FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_for_mlme_reset_cfm,   waitingForMlmeResetTransitions,        FALSE),
   fsm_state_table_entry(FSMSTATE_ready,                        readyTransitions,                      FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_wifi_off_cfm,         waitingWifiOffTransitions,             FALSE),
};

const FsmProcessStateMachine unifi_driver_fsm =
{
#ifdef FSM_DEBUG
       "Unifi Driver",                                           /* SM Process Name       */
#endif
       UNIFI_DRIVER_PROCESS,                                     /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                         /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions,FALSE),   /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),  /* Ignore Event handers */
       unifi_driver_fsm_init,                                    /* Entry Function        */
       NULL,                                                     /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                      /* Trace Dump Function   */
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

