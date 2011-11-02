/** @file nme_network_selector_fsm.h
 *
 * Public NME Network Selector FSM API
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009-2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Public NME Network Selector FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_network_selector_fsm/nme_network_selector_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_WIFI_NME_NETWORK_SELECTOR_PROCESS_H
#define CSR_WIFI_NME_NETWORK_SELECTOR_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_network_selector
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "nme_network_selector_fsm/nme_network_selector_fsm_types.h"
#include "nme_network_selector_fsm/nme_network_selector_fsm_events.h"

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
extern const FsmProcessStateMachine nme_network_selector_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Returns the current connection manager instance
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   CsrUint16 : This may be FSM_TERMINATE or an allocated instance.
 */
extern CsrUint16 nme_ntw_selector_get_connection_mgr_fsm_instance(FsmContext* pContext);


/**
 * @brief
 *   Returns the current wps FSM instance
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   CsrUint16 : This may be FSM_TERMINATE or an allocated instance.
 */
extern CsrUint16 nme_ntw_selector_get_wps_fsm_instance(FsmContext* pContext);

/**
 * @brief
 *   Returns the current security FSM instance
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   CsrUint16 : This may be FSM_TERMINATE or an allocated instance.
 */
extern CsrUint16 nme_ntw_selector_get_security_fsm_instance(FsmContext* pContext);

/**
 * @brief
 *   Check if we are disconnected
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   CsrBool : Returns TRUE if disconnected
 */
extern CsrBool nme_ntw_selector_is_disconnected(FsmContext* pContext);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_NME_NETWORK_SELECTOR__PROCESS_H */
