/** @file unifi_driver_fsm.h
 *
 * Public Unifi Driver FSM API
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
 *   Public Unifi Driver FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/unifi_driver_fsm/unifi_driver_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef UNIFI_DRIVER_FSM_H
#define UNIFI_DRIVER_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup unifi_driver
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
 *   Defines the unifi driver state machine data
 */
extern const FsmProcessStateMachine unifi_driver_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief moves the UniFi Driver manager into D0 power state
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *        void
 */
extern void ufd_move_to_d0_state(FsmContext* context);

/**
 * @brief moves the UniFi Driver manager into D3 power state
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *        void
 */
extern void ufd_move_to_d3_state(FsmContext* context);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* UNIFI_DRIVER_FSM_H */
