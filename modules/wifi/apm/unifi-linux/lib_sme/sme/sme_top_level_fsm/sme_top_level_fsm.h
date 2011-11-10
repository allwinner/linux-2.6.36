/** @file sme_top_level_fsm.h
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
 *   SME API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_top_level_fsm/sme_top_level_fsm.h#2 $
 *
 ****************************************************************************/
#ifndef SME_TOP_LEVEL_FSM_H
#define SME_TOP_LEVEL_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup sme_top_level_fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_internal.h"

#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"

#include "sys_sap/sys_sap_from_sme_interface.h"
#include "mgt_sap/mgt_sap_from_sme_interface.h"

#include "bt_sap/bt_sap_from_sme_interface.h"

#include "dbg_sap/dbg_sap_from_sme_interface.h"

#ifdef SME_SYNC_ACCESS
#include "sync_access/sync_access.h"
#endif

#ifdef CSR_AMP_ENABLE
#include "palio/palio_fsm_types.h"
#include "pal_ctrl_sap/pal_ctrl_sap_from_sme_interface.h"

#include "pal_hci_sap/pal_hci_sap_types.h"
#include "pal_hci_sap/pal_hci_sap.h"
#endif

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "payload_manager/payload_manager.h" /* For PLD_HND */

#include "hostio/hip_fsm_types.h"
#include "hostio/hip_fsm_events.h"

#include "smeio/smeio_fsm_types.h"
#include "smeio/smeio_fsm_events.h"

#include "smeio/smeio_trace_signals.h"
#include "smeio/smeio_free_message_contents.h"

#include "scan_manager_fsm/scan_results_storage.h"

#include "hip_proxy_fsm/mib_access_fsm.h"

#include "hip_proxy_fsm/hip_proxy_fsm_types.h"
#include "hip_proxy_fsm/hip_proxy_fsm_events.h"

#include "security_manager_fsm/security_manager_fsm_types.h"
#include "security_manager_fsm/security_manager_fsm_events.h"

#include "link_quality_fsm/link_quality_fsm_types.h"
#include "link_quality_fsm/link_quality_fsm_events.h"

#include "network_selector_fsm/network_selector_fsm_types.h"
#include "network_selector_fsm/network_selector_fsm_events.h"

#include "scan_manager_fsm/scan_manager_fsm_types.h"
#include "scan_manager_fsm/scan_manager_fsm_events.h"

#include "sme_core_fsm/sme_core_fsm_types.h"
#include "sme_core_fsm/sme_core_fsm_events.h"

#include "unifi_driver_fsm/unifi_driver_fsm_types.h"
#include "unifi_driver_fsm/unifi_driver_fsm_events.h"

#include "power_manager_fsm/power_manager_fsm_types.h"
#include "power_manager_fsm/power_manager_fsm_events.h"

#include "coex_fsm/coex_fsm_types.h"
#include "coex_fsm/coex_fsm_events.h"

#include "qos_fsm/qos_fsm_types.h"
#include "qos_fsm/qos_fsm_events.h"

#ifdef CCX_VARIANT
#include "ccx_fsm/ccx_fsm_types.h"
#include "ccx_fsm/ccx_fsm_events.h"
#endif

#ifdef CSR_AMP_ENABLE
#include "palio/palio_fsm_types.h"
#include "palio/palio_fsm_events.h"

#include "pal_hci_sap/pal_hci_sap_types.h"
#include "pal_hci_sap/pal_hci_sap_signals.h"

#include "pal_manager/pal_manager.h"

#include "device_manager_fsm/device_manager_fsm_types.h"
#include "device_manager_fsm/device_manager_fsm_events.h"

#include "link_manager_fsm/link_manager_fsm_types.h"
#include "link_manager_fsm/link_manager_fsm_events.h"
#include "link_manager_fsm/logical_link_manager.h"

#include "coex_manager_fsm/coex_manager_fsm_types.h"
#include "coex_manager_fsm/coex_manager_fsm_events.h"

#include "palio/palio_trace_signals.h"

#endif


/* PUBLIC MACROS ************************************************************/

/**
 *   Fixed Process type ID's
 */
#define SECURITY_MANAGER_PROCESS    (0x9001)
#define HIP_PROXY_PROCESS           (0x9002)
#define CONNECTION_MANAGER_PROCESS  (0x9003)
#define LINK_QUALITY_PROCESS        (0x9004)
#define NETWORK_SELECTOR_PROCESS    (0x9005)
#define SCAN_MANAGER_PROCESS        (0x9006)
#define SME_CORE_PROCESS            (0x9007)
#define MIB_ACCESS_PROCESS          (0x900A)
#define DBG_PROCESS                 (0x900C)
#define UNIFI_DRIVER_PROCESS        (0x900D)
#define SME_CONFIGURATION_PROCESS   (0x900F)
#define POWER_MANAGER_PROCESS       (0x9010)
#define SME_MEASUREMENT_PROCESS     (0x9011)
#define COEX_PROCESS                (0x9012)
#define QOS_PROCESS                 (0x9013)
#define QOS_ACTION_PROCESS          (0x9014)
#define MIB_ACTION_PROCESS          (0x9015)
#ifdef CCX_VARIANT
#define CCX_PROCESS                 (0x9016)
#endif

#ifdef CSR_AMP_ENABLE
#define PAL_LM_LINK_PROCESS         (0x9101)
#define PAL_LM_HIP_PROCESS          (0x9102)
#define PAL_DM_PROCESS              (0x9103)
#define PAL_CM_PROCESS              (0x9104)
#define PAL_HCI_PROCESS             (0x9105)
#define PAL_COEX_PROCESS            (0x9105)

#endif


/* PUBLIC TYPES DEFINITIONS *************************************************/

typedef struct SmeContext {

    /* Instance ID's. Allocated in sme_init() */
    CsrUint16 hipProxyInstance;
    CsrUint16 linkQualityInstance;
    CsrUint16 networkSelectorInstance;
    CsrUint16 scanManagerInstance;
    CsrUint16 smeCoreInstance;
    CsrUint16 mibAccessInstance;
    CsrUint16 dbgInstance;
    CsrUint16 unifiDriverInstance;
    CsrUint16 smeConfigurationInstance;
    CsrUint16 powerManagerInstance;
    CsrUint16 coexInstance;
    CsrUint16 qosInstance;
#ifdef CCX_VARIANT
    CsrUint16 ccxInstance;
    CsrUint16 smeMeasurementsInstance;
#endif

#ifdef CSR_AMP_ENABLE

    CsrUint16 palDmFsmInstance;
    CsrUint16 palMgrFsmInstance;
    CsrUint16 palCoexFsmInstance;

    palManagerContext *palMgrContext;
    logicalLinkManagerContext *llMgrContext;
    /* common link data context -> This data is accessed by all modules */
    /* Logical Link Data -> data related to the logical links. accessed by Link Manager and Data Manager only */
    /* payload context -> data owned by the payload manager */

    void* cryptoContext;

#endif

    FsmContext* fsmContext;
    PldContext* pldContext;
    HipAutoCfm* hipSapAuto;
    ScanResults* scanData;

#ifdef SME_TRACE_ENABLE
    char traceSsidBuffer[33];
    char traceMacAddressBuffer[19];
#endif

#ifdef SME_SYNC_ACCESS
    CsrBool              syncAccessLocked;
    unifi_ScanResultList syncAccessScanResults;
    unifi_PmkidList      syncAccessPmkid;
#endif

} SmeContext;

#define getSmeContext(fsmContext) ((SmeContext*)fsmContext->applicationContext)
#define getPldContext(fsmContext) (getSmeContext(fsmContext)->pldContext)
#define getPmkCacheContext(fsmContext) (getSmeContext(fsmContext)->pmkCache)

#ifdef CSR_AMP_ENABLE
#define getPalMgrContext(fsmContext) ((palManagerContext *)(getSmeContext(fsmContext)->palMgrContext))
#define getLlmContext(fsmContext) ((logicalLinkManagerContext *)(getSmeContext(fsmContext)->llMgrContext))

#endif


#ifdef SME_TRACE_ENABLE
#define getSSIDBuffer(fsmContext) (getSmeContext(fsmContext)->traceSsidBuffer)
#define getMacAddressBuffer(fsmContext) (getSmeContext(fsmContext)->traceMacAddressBuffer)
#endif

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* BSSID of 00:00:00:00:00:00 */
extern const unifi_MACAddress NullBssid;

/* BSSID of FF:FF:FF:FF:FF:FF */
extern const unifi_MACAddress BroadcastBssid;

/* DataBlock initialised to { 0, NULL }; */
extern const unifi_DataBlock NullDataBlock;

/* NullDataReference initialised to { 0xDEAD, 0 }; */
extern const DataReference NullDataReference;

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_TOP_LEVEL_FSM_H */
