/** @file lm_hip_fsm.h
 *
 * Public HIP FSM API
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
 *   Public HIP FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/lm_hip_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef PAL_LM_HIP_FSM_H
#define PAL_LM_HIP_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup ip_connection_mgr
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
 *   Defines the ip connection manager state machine data
 */
extern const FsmProcessStateMachine pal_lm_hip_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PAL_LM_HIP_FSM_H */
