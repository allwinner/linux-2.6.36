/** @file power_manager_fsm.h
 *
 * Power Manager FSM API
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
 *   Power Manager FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/power_manager_fsm/power_manager_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef POWER_MANAGER_FSM_H
#define POWER_MANAGER_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup power_manager
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
 *   Defines the power manager state machine data
 */
extern const FsmProcessStateMachine power_manager_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Resets the power state when an mlme_reset is called
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   void
 */
extern void reset_power_state(FsmContext* context);

/**
 * @brief
 *   Sends a unifi_sys_configure_power_mode_req if needed
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   void
 */
extern void power_send_sys_configure_power_mode(FsmContext* context, CsrBool force);


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* POWER_MANAGER_FSM_H */
