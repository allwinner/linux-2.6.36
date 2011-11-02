/** @file nme_top_level_fsm.h
 *
 * NME Top Level
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   NME API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_top_level_fsm/nme_top_level_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_WIFI_NME_TOP_LEVEL_FSM_H
#define CSR_WIFI_NME_TOP_LEVEL_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_top_level_fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_internal.h"

#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "nmeio/nmeio_fsm_types.h"

#include "csr_wifi_nme_sap/csr_wifi_nme_sap_from_sme_interface.h"
#include "csr_wifi_nme_mgt_sap/csr_wifi_nme_mgt_sap_from_sme_interface.h"
#include "csr_wifi_nme_sys_sap/csr_wifi_nme_sys_sap_from_sme_interface.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

/* Require for unifi types i.e. unifi_MACAddress */
#include "hostio/hip_fsm_types.h"
/* Required for the SME MGT-SAP types */
#include "smeio/smeio_fsm_types.h"
#include "nmeio/nmeio_fsm_types.h"
#include "nmeio/nmeio_upper_fsm_events.h"
#include "nmeio/nmeio_lower_fsm_events.h"

#include "nme_core_fsm/nme_core_fsm_types.h"
#include "nme_core_fsm/nme_core_fsm_events.h"
#include "nme_core_fsm/nme_core_fsm.h"

#include "nme_signal_routing_fsm/nme_signal_routing_fsm.h"
#include "nme_signal_routing_fsm/nme_primitive_forwarding_utils.h"
#include "nme_signal_routing_fsm/nme_routing_manager.h"

#include "nme_security_manager_fsm/nme_security_manager_fsm_types.h"
#include "nme_security_manager_fsm/nme_security_manager_fsm_events.h"
#include "nme_security_manager_fsm/nme_security_manager_fsm.h"

#include "nme_connection_manager_fsm/nme_connection_manager_fsm_types.h"
#include "nme_connection_manager_fsm/nme_connection_manager_fsm_events.h"
#include "nme_connection_manager_fsm/nme_connection_manager_fsm.h"

#include "nme_network_selector_fsm/nme_network_selector_fsm_types.h"
#include "nme_network_selector_fsm/nme_network_selector_fsm_events.h"
#include "nme_network_selector_fsm/nme_network_selector_fsm.h"

#include "nme_profile_manager_fsm/nme_profile_manager_fsm.h"

#include "nme_wps_fsm/nme_wps_fsm_types.h"
#include "nme_wps_fsm/nme_wps_fsm_events.h"
#include "nme_wps_fsm/nme_wps_fsm.h"

#include "csr_wifi_nme_sap/csr_wifi_nme_sap.h"
#include "csr_wifi_nme_mgt_sap/csr_wifi_nme_mgt_sap.h"
#include "csr_wifi_nme_sys_sap/csr_wifi_nme_sys_sap.h"

#include "nmeio/nmeio_free_message_contents.h"
#include "smeio/smeio_free_message_contents.h"

#include "smeio/smeio_trace_types.h"
#include "nmeio/nmeio_trace_types.h"

#include "csr_wifi_nme_mgt_sap/csr_wifi_nme_mgt_sap_from_sme_interface.h"
#include "csr_wifi_nme_sys_sap/csr_wifi_nme_sys_sap_from_sme_interface.h"
#include "csr_wifi_nme_sap/csr_wifi_nme_sap_from_sme_interface.h"

#ifdef CCX_VARIANT
#include "nme_ccx/nme_ccx.h"
#endif


/* PUBLIC MACROS ************************************************************/
/* The getNmeContext is part of the code generation and used in __to_sme_interface.c
 * files
 */
#define getNmeContext(fsmContext) ((NmeContext*)fsmContext->applicationContext)

#define getNmeSecurityContext(fsmContext) ((struct CsrCryptoContext*)(getNmeContext(fsmContext)->cryptoContext))

/* App handle agreed between SME and NME. SME will use NULL and NME to use 2*/
#define NME_APP_HANDLE(context) (void *)(UNIFI_NME_APPHANDLE << 6)

#ifdef SME_TRACE_ENABLE
#define getNMESSIDBuffer(fsmContext) (getNmeContext(fsmContext)->traceSsidBuffer)
#define getNMEMacAddressBuffer(fsmContext) (getNmeContext(fsmContext)->traceMacAddressBuffer)
#endif

/**
 *   Fixed Process type ID's
 */
#define NME_SIGNAL_ROUTING_PROCESS (0x9201)
#define NME_CORE_PROCESS (0x9202)
#define NME_PROFILE_MANAGER_PROCESS (0x9203)
#define NME_CONNECTION_MANAGER_PROCESS (0x9204)
#define NME_SECURITY_MANAGER_PROCESS (0x9205)
#define NME_WPS_PROCESS (0x9206)
#define NME_NETWORK_SELECTOR_PROCESS (0x9207)

#define NME_NUM_OF_PROCESSES 7

/* Each dynamic FSM is assigned a reference to assist with the allocation
 * of a unique context.
 */
#define NME_DYN_CONNECTION_MANAGER_FSM 0
#define NME_DYN_SECURITY_MANAGER_FSM 1
#define NME_DYN_WPS_FSM 2
#define NME_NUM_OF_DYN_FSM 3

/* PUBLIC TYPES DEFINITIONS *************************************************/

typedef struct NmeContext
{

    CsrUint16 nmeSignalRoutingFsmInstance;
    CsrUint16 nmeCoreFsmInstance;
    CsrUint16 nmeProfileMgrFsmInstance;
    CsrUint16 nmeNetworkSelectorFsmInstance;

    FsmContext* fsmContext;

    void* cryptoContext;

#ifdef SME_TRACE_ENABLE
    char traceSsidBuffer[33];
    char traceMacAddressBuffer[19];
#endif

} NmeContext;


/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* BSSID of 00:00:00:00:00:00 */
extern const unifi_MACAddress NmeNullBssid;

/* BSSID of FF:FF:FF:FF:FF:FF */
extern const unifi_MACAddress NmeBroadcastBssid;


/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Based on the event id will call any appropriate message
 *   content free function.
 *
 * @par Description
 *   See brief.
 *
 * @param[in] const FsmEvent* : event
 *
 * @return
 *   void
 */
extern void nme_free_event(const FsmEvent* pEvt);

#ifdef SME_TRACE_ENABLE
/**
 * @brief
 *   Output trace information for the credentials supplied
 *
 * @par Description
 *   See brief
 *
 * @param[in] const unifi_Credentials * : credentials
 *
 * @return
 *   None
 */
extern void nme_trace_credentials(const unifi_Credentials *pCredentials);
#else
#define nme_trace_credentials(pCredentials)
#endif

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_NME_TOP_LEVEL_FSM_H */
