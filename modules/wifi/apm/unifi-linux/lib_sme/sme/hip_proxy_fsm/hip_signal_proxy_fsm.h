/** @file hip_signal_proxy_fsm.h
 *
 * Public Hip Signal Proxy FSM API
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
 *   Public Hip Signal Proxy FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/hip_signal_proxy_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef HIP_SIGNAL_PROXY_PROCESS_H
#define HIP_SIGNAL_PROXY_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup hip_proxy
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the hip signal proxy state machine data
 */
extern const FsmProcessStateMachine hip_signal_proxy_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Registers a security manager instance for receiving messages
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 * @param[in]  securityMgrInstance  : The Security Manager instance to register
 * @param[in]  macAddress  : The Mac address the Security Manager is managing
 *
 * @return
 *   void
 */
extern void hip_proxy_register_security_manager(FsmContext* context, CsrUint16 securityMgrInstance, unifi_MACAddress macAddress );

/**
 * @brief
 *   Gets the registered security manager instance
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   CsrUint16 security manager instance ID or FSM_TERMINATE
 */
extern CsrUint16 hip_proxy_get_security_manager(FsmContext* context);

/**
 * @brief
 *   Deregisters a security manager instance from receiving messages
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 * @param[in]  securityMgrInstanceId  : The Security Manager instance to deregister
 *
 * @return
 *   void
 */
extern void hip_proxy_deregister_security_manager(FsmContext* context, CsrUint16 securityMgrInstanceId);

/**
 * @brief
 *   Registers a connection manager instance for receiving messages
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 * @param[in]  connectionMgrInstanceId  : The Connection Manager instance to register
 *
 * @return
 *   void
 */
extern void hip_proxy_register_connection_manager(FsmContext* context, CsrUint16 connectionMgrInstanceId);

/**
 * @brief
 *   Deregisters a connection manager instance from receiving messages
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 * @param[in]  connectionMgrInstanceId  : The Security Manager instance to deregister
 *
 * @return
 *   void
 */
extern void hip_proxy_deregister_connection_manager(FsmContext* context, CsrUint16 connectionMgrInstanceId );


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* HIP_SIGNAL_PROXY_PROCESS_H */
