/** @file network_selector_fsm.h
 *
 * Public Network Selector FSM API
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
 *   Public Network Selector FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/network_selector_fsm/network_selector_fsm.h#2 $
 *
 ****************************************************************************/
#ifndef NETWORK_SELECTOR_PROCESS_H
#define NETWORK_SELECTOR_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup networkselector
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "sme_configuration/sme_configuration_fsm.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

typedef enum ns_ConnectionStatus
{
    ns_ConnectionStatus_Disconnected = 0x0000,
    ns_ConnectionStatus_Connecting   = 0x0001,
    ns_ConnectionStatus_Connected    = 0x0002
} ns_ConnectionStatus;

/*
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct connectAddressesData
{
    unifi_MACAddress      bssid;
    unifi_SSID            ssid;
    CsrUint8              channelNumber;
#ifdef CCX_VARIANT    
    CsrInt16             rssi;
#endif
    srs_IECapabilityMask  joinIeCapabilityFlags;

} connectAddressesData;

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the network selector state machine data
 */
extern const FsmProcessStateMachine network_selector_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Accessor for the current media connection status
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   unifi_MediaStatus current status
 */
extern unifi_MediaStatus ns_get_media_connection_status(FsmContext* context);

/**
 * @brief
 *   Accessor to set the current media connection status
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context : FSM context
 * @param[in]  status  : new media status
 *
 * @return
 *   void
 */
extern void ns_set_media_connection_status(FsmContext* context, unifi_MediaStatus status);

/**
 * @brief
 *   Accessor for the current connection status
 *
 * @par Description
 *   see brief
 *
 * @param[in] context : FSM context
 *
 * @return
 *   ns_ConnectionStatus current status
 */
extern ns_ConnectionStatus ns_get_connection_status(FsmContext* context);

/**
 * @brief
 *   Accessor for the dynamic wep detected
 *
 * @param[in] context : FSM context
 *
 * @return
 *   CsrBool
 */
extern CsrBool ns_dynamic_wep_detected(FsmContext* context);

/**
 * @brief
 *   Sets the dynamic wep detected flag to true
 *
 * @param[in] context : FSM context
 *
 * @return
 *   void
 */
extern void ns_set_dynamic_wep(FsmContext* context);

/**
 * @brief
 *   Calls out to the driver to configure the controlled and uncontrolled ports
 *
 * @param[in] context               : FSM context
 * @param[in] uncontrolledPortState : uncontrolled port action
 * @param[in] controlledPortState   : controlled port action
 *
 * @return
 *   void
 */
extern CsrBool ns_port_configure(FsmContext* context, unifi_PortAction uncontrolledPortState, unifi_PortAction controlledPortState);

/**
 * @brief
 *   Gets the current controlled port state
 *
 * @param[in] context               : FSM context
 *
 * @return
 *   State of the controlled port
 */
extern unifi_PortAction ns_get_controlled_port_state(FsmContext* context);


/**
 * @brief
 *   Gets the current controlled port state
 *
 * @param[in] context : FSM context
 * @param[in] bssid   : bssid to search for
 *
 * @return
 *   State of the controlled port
 */
extern CsrBool isInRoamingList(FsmContext* context, const unifi_MACAddress* bssid );

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_SELECTOR_PROCESS_H */
