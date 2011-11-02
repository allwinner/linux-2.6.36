/** @file link_quality_fsm.h
 *
 * Public Link Quality FSM API
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
 *   Public Link Quality FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/link_quality_fsm/link_quality_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef LINK_QUALITY_PROCESS_H
#define LINK_QUALITY_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup sme_core
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
 *   Defines the connection manager state machine data
 */
extern const FsmProcessStateMachine link_quality_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Resets the internal link quality data when the unifi is reset
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *        void
 */
extern void reset_link_quality(FsmContext* context);

/**
 * @brief moves the Link Quality manager into D0 power state
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *        void
 */
extern void lqm_move_to_d0_state(FsmContext* context);

/**
 * @brief moves the Link Quality manager into D3 power state
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *        void
 */
extern void lqm_move_to_d3_state(FsmContext* context);

/**
 * @brief
 *   decides if a roam is required
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
extern CsrBool check_if_we_should_roam(FsmContext* context);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINK_QUALITY_PROCESS_H */
