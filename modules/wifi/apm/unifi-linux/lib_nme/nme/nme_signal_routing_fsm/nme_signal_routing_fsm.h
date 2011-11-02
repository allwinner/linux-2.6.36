/** @file nme_signal_routing_fsm.h
 *
 * Public NME Signal Routing FSM API
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
 *   Public NME Signal ROuting FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_signal_routing_fsm/nme_signal_routing_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_WIFI_NME_SIGNAL_ROUTING_PROCESS_H
#define CSR_WIFI_NME_SIGNAL_ROUTING_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_signal_routing
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "nme_signal_routing_fsm/nme_routing_manager.h"

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
extern const FsmProcessStateMachine nme_signal_routing_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   returns a pointer to the routing control block
 *
 * @par Description
 *   see brief
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   RoutingCtrlblk* : routing control block pointer
 */
extern RoutingCtrlblk* nme_signal_routing_get_ctrlblk_context(FsmContext* pContext);

/**
 * @brief
 *   Returns the next dynamic FSM instance msg id number for the specified
 *   FSM.
 *
 * @par Description
 *   see brief
 *
 * @param[in]  FsmContext* : FSM context
 * @param[in]  const CsrUint8 : the dynamic FSM msg id
 *
 * @return
 *   CsrUint8
 */
extern CsrUint8 nme_signal_routing_get_dyn_fsm_msg_id(
        FsmContext* pContext,
        const CsrUint8 dynFsm);


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_NME_SIGNAL_ROUTING_PROCESS_H */
