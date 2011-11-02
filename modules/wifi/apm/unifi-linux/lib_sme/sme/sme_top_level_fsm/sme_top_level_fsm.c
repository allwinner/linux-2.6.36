/** @file sme_top_level_fsm.c
 *
 * SME Top Level
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
 *   Creates and initialises SME FSM
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_top_level_fsm/sme_top_level_fsm.c#3 $
 *
 ****************************************************************************/

/** @{
 * @ingroup sme_top_level_fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_internal.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "sme_top_level_fsm/sme.h"
#include "security_manager_fsm/security_manager_fsm.h"
#include "hip_proxy_fsm/hip_signal_proxy_fsm.h"
#include "hip_proxy_fsm/mib_access_fsm.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "sme_core_fsm/sme_core_fsm.h"
#include "link_quality_fsm/link_quality_fsm.h"
#include "scan_manager_fsm/scan_manager_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "dbg_test_fsm/dbg_fsm.h"
#include "unifi_driver_fsm/unifi_driver_fsm.h"
#include "power_manager_fsm/power_manager_fsm.h"
#include "coex_fsm/coex_fsm.h"
#include "qos_fsm/qos_fsm.h"

#ifdef CCX_VARIANT
#include "ccx_fsm/ccx_fsm.h"
#include "sme_measurements/sme_measurements_fsm.h"
#endif

#ifdef CSR_AMP_ENABLE
#include "link_manager_fsm/lm_link_fsm.h"
#include "link_manager_fsm/lm_hip_fsm.h"
#include "device_manager_fsm/device_manager_fsm.h"
#include "coex_manager_fsm/coex_manager_fsm.h"
#include "pal_manager/pal_manager.h"
#endif

#include "sys_sap/hip_signal_header.h"
#include "sys_sap/sme_interface_hip_auto_cfm.h"
#include "payload_manager/payload_manager.h"



/* MACROS *******************************************************************/
#ifdef CSR_AMP_ENABLE
#define PAL_MAX_HCI_INSTANCES (1)
#define PAL_MAX_DM_INSTANCES (1)
#define PAL_MAX_COEX_INSTANCES (1)
#define PAL_MAX_LM_INSTANCES (2*PAL_MAX_PHYSICAL_LINKS)
#define PAL_MAX_FSM_INSTANCES (PAL_MAX_DM_INSTANCES+PAL_MAX_LM_INSTANCES+PAL_MAX_HCI_INSTANCES+PAL_MAX_COEX_INSTANCES)
#else
#define PAL_MAX_FSM_INSTANCES (0)
#endif

#ifdef CCX_VARIANT
#define CCX_FSM_INSTANCES (2)
#else
#define CCX_FSM_INSTANCES (0)
#endif

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

    /*
     * NOTE:
     * The internal event base numbers are documented in
     * sme_top_level_fsm/event_base_ids.txt
     */

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/
static void sme_ignore_hip_event(void* appContext, const FsmEvent* event)
{
    const SmeContext* smeContext = (const SmeContext*)appContext;

    if (event->id >= 0x0100 && event->id < 0x1000 )
    {
        hip_signal_header *drsig = (hip_signal_header*) event; /*lint !e1939 */
        sme_trace_entry((TR_FSM, "sme generic ignore hip message 0x%.4X", event->id));
        if (drsig->dataref1.dataLength != 0)
        {
            pld_release(smeContext->pldContext, drsig->dataref1.slotNumber);
        }
        if (drsig->dataref2.dataLength != 0)
        {
            pld_release(smeContext->pldContext, drsig->dataref2.slotNumber);
        }
    }
    else if (UNIFI_SYS_MA_UNITDATA_IND_ID == event->id)
    {
        UnifiSysMaUnitdataInd_Evt *ind = (UnifiSysMaUnitdataInd_Evt *)event;
        if (ind->freeFunction)
        {
            (*ind->freeFunction)(ind->frame);
        }
        else
        {
            CsrPfree(ind->frame);
        }
    }
}

/* PUBLIC CONSTANT DEFINITIONS **********************************************/
/**
 * See description in sme_top_level_fsm/sme_top_level_fsm.h
 */
const unifi_MACAddress NullBssid = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

/**
 * See description in sme_top_level_fsm/sme_top_level_fsm.h
 */
const unifi_MACAddress BroadcastBssid = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

/**
 * See description in sme_top_level_fsm/sme_top_level_fsm.h
 */
const unifi_DataBlock NullDataBlock = {0, NULL};

/**
 * See description in sme_top_level_fsm/sme_top_level_fsm.h
 */
const DataReference NullDataReference = {0xDEAD, 0};


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in sme_top_level_fsm/sme.h
 */
FsmContext* sme_init(void* externalContext, void* cryptoContext)
{
    SmeContext* smeContext = (SmeContext*)CsrPmalloc(sizeof(SmeContext));

    /* Initialise the Payload Manager */
    smeContext->pldContext = pld_init(64, 128);

    smeContext->fsmContext = fsm_init_context(smeContext, externalContext, (14+PAL_MAX_FSM_INSTANCES+CCX_FSM_INSTANCES));

    smeContext->hipSapAuto = hip_auto_cfm_init(smeContext->fsmContext);
    smeContext->scanData = srs_initialise_scan_result_storage();

    fsm_install_app_ignore_event_callback(smeContext->fsmContext, sme_ignore_hip_event);

    smeContext->hipProxyInstance            = fsm_add_instance(smeContext->fsmContext, &hip_signal_proxy_fsm, FALSE);
    smeContext->mibAccessInstance           = fsm_add_instance(smeContext->fsmContext, &mib_access_fsm, FALSE);
    smeContext->smeCoreInstance             = fsm_add_instance(smeContext->fsmContext, &sme_core_fsm, FALSE);
    smeContext->linkQualityInstance         = fsm_add_instance(smeContext->fsmContext, &link_quality_fsm, FALSE);
    smeContext->scanManagerInstance         = fsm_add_instance(smeContext->fsmContext, &scanManagerFsm, FALSE);
    smeContext->dbgInstance                 = fsm_add_instance(smeContext->fsmContext, &dbg_fsm, FALSE);
    smeContext->networkSelectorInstance     = fsm_add_instance(smeContext->fsmContext, &network_selector_fsm, FALSE);
    smeContext->unifiDriverInstance         = fsm_add_instance(smeContext->fsmContext, &unifi_driver_fsm, FALSE);
    smeContext->smeConfigurationInstance    = fsm_add_instance(smeContext->fsmContext, &sme_configuration_fsm, FALSE);
    smeContext->powerManagerInstance        = fsm_add_instance(smeContext->fsmContext, &power_manager_fsm, FALSE);
    smeContext->coexInstance                = fsm_add_instance(smeContext->fsmContext, &coex_fsm, FALSE);
    smeContext->qosInstance                 = fsm_add_instance(smeContext->fsmContext, &qos_fsm, FALSE);
#ifdef CCX_VARIANT
    smeContext->smeMeasurementsInstance     = fsm_add_instance(smeContext->fsmContext, &sme_measurements_fsm, FALSE);
    smeContext->ccxInstance                 = fsm_add_instance(smeContext->fsmContext, &ccx_fsm, FALSE);
#endif

#ifdef CSR_AMP_ENABLE

#define PAL_NUM_LOGICAL_LINKS PAL_MAX_LOGICAL_LINKS
#define PAL_NUM_GURANTEED_LOGICAL_LINKS PAL_MAX_GUARANTEED_LOGICAL_LINKS
#define PAL_NUM_PHYSICAL_LINKS PAL_MAX_PHYSICAL_LINKS
#define PAL_NUM_MAX_CONCURRENT_HCI_COMMANDS PAL_HCI_MAX_ALLOWED_COMMANDS

    smeContext->cryptoContext = cryptoContext;
    smeContext->palMgrContext = pal_manager_init(PAL_NUM_PHYSICAL_LINKS,PAL_NUM_MAX_CONCURRENT_HCI_COMMANDS);
    smeContext->llMgrContext = pal_llm_init(PAL_MAX_LOGICAL_LINKS, PAL_MAX_GUARANTEED_LOGICAL_LINKS);

    smeContext->palDmFsmInstance            = fsm_add_instance(smeContext->fsmContext, &pal_dm_fsm, FALSE);
    smeContext->palMgrFsmInstance           = fsm_add_instance(smeContext->fsmContext, &pal_mgr_fsm, FALSE);
    smeContext->palCoexFsmInstance           = fsm_add_instance(smeContext->fsmContext, &pal_coex_fsm, FALSE);

#endif

#ifdef SME_SYNC_ACCESS
    smeContext->syncAccessLocked = FALSE;
    smeContext->syncAccessScanResults.numElements = 0;
    smeContext->syncAccessScanResults.results = NULL;
    smeContext->syncAccessPmkid.numElements = 0;
    smeContext->syncAccessPmkid.pmkids = NULL;
#endif


    fsm_call_entry_functions(smeContext->fsmContext);
    return smeContext->fsmContext;
}

/**
 * See description in sme_top_level_fsm/sme.h
 */
FsmContext* sme_reset(FsmContext* context)
{
    void* cryptoContext = NULL;
    void* extContext = context->externalContext; /* Save the external Context */
    SmeConfigData* cfg = get_sme_config(context);
    void* lastJoinedAddresses = CsrPmalloc(sizeof(cfg->lastJoinedAddresses));
    unifi_DataBlock  calibrationData = {0, NULL};
    void* pldCalData = NULL;
    CsrUint16 pldLength = 0;

    /* Save Callibration Data and last connected AP's list*/
    calibrationData.length = cfg->calibrationData.dataLength;

    if (calibrationData.length)
    {
        calibrationData.data = CsrPmalloc(calibrationData.length);
        pld_access(getPldContext(context), cfg->calibrationData.slotNumber, (void**)&pldCalData, &pldLength);
        CsrMemCpy(calibrationData.data, pldCalData, calibrationData.length);
    }
    CsrMemCpy(lastJoinedAddresses, cfg->lastJoinedAddresses, sizeof(cfg->lastJoinedAddresses));

#ifdef CSR_AMP_ENABLE
    cryptoContext = getSmeContext(context)->cryptoContext;
#endif
    /* Reset the SME */
    sme_shutdown(context);
    context = sme_init(extContext, cryptoContext);

    /* Restore Calibration Data and last connected AP's list*/
    cfg = get_sme_config(context);
    if (calibrationData.length)
    {
        cfg->calibrationData.dataLength = calibrationData.length;
        pld_store(getPldContext(context), (void*)calibrationData.data, calibrationData.length, &cfg->calibrationData.slotNumber);
        CsrPfree(calibrationData.data);
    }

    CsrMemCpy(cfg->lastJoinedAddresses, lastJoinedAddresses, sizeof(cfg->lastJoinedAddresses));
    CsrPfree(lastJoinedAddresses);
    return context;
}

/**
 * See description in sme_top_level_fsm/sme.h
 */
void sme_shutdown(FsmContext* context)
{
    SmeContext* smeContext = getSmeContext(context);

#ifdef CSR_AMP_ENABLE
    pal_manager_deinit(context);
    pal_llm_deinit(getLlmContext(context));
#endif

    hip_auto_cfm_shutdown(smeContext->hipSapAuto);

    fsm_shutdown(context);

    srs_destroy_scan_result_storage(smeContext->scanData);
    pld_shutdown(smeContext->pldContext);



    CsrPfree(smeContext);
}

/**
 * See description in sme_top_level_fsm/sme.h
 */
FsmTimerData sme_execute(FsmContext* context)
{
    return fsm_execute(context);
}

/**
 * See description in sme_top_level_fsm/sme.h
 */
void sme_install_wakeup_callback(FsmContext* context, FsmExternalWakupCallbackPtr callback)
{
    fsm_install_wakeup_callback(context, callback);
}

/** @}
 */
