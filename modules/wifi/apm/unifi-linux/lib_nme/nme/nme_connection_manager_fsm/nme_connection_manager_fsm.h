/** @file nme_connection_manager_fsm.h
 *
 * Public NME Connection Manager FSM API
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
 *   Public NME Connection Manager FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_connection_manager_fsm/nme_connection_manager_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_WIFI_NME_CONNECTION_MANAGER_PROCESS_H
#define CSR_WIFI_NME_CONNECTION_MANAGER_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_connection_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "nmeio/nmeio_fsm_types.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the state machine data
 */
extern const FsmProcessStateMachine nme_connection_manager_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Returns the connection status for the connection manager instance.
 *
 * @par Description
 *   See brief
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   unifi_ConnectionStatus :
 */
extern unifi_ConnectionStatus nme_connection_manager_connection_status_get(FsmContext* pContext);

/**
 * @brief
 *   Returns the current security manager instance associated with
 *   the connection manager.
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   CsrUint16 : This may be FSM_TERMINATE or an allocated instance.
 */
extern CsrUint16 nme_connection_manager_get_security_fsm_instance(FsmContext* pContext);

/**
 * @brief
 *   Returns the current profile identity, if there is no active
 *   connection or the connection was not triggered by a NME
 *   PROFILE CONNECT REQ then a profile identity containing
 *   all zeros is returned.
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   unifi_ProfileIdentity.
 */
extern unifi_ProfileIdentity nme_connection_manager_get_profile_identity(FsmContext* pContext);

/**
 * @brief
 *   Returns the current fsm instance reference assigned to the
 *   connection manager fsm.
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   CsrUint8 - 0 if there is no connection manager FSM instance.
 */
extern CsrUint8 nme_connection_manager_get_fsm_inst_ref(FsmContext* pContext);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_NME_CONNECTION_MANAGER_PROCESS_H */
