/** @file sme_core_fsm.h
 *
 * Public Connection Manager FSM API
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
 *   Public Connection Manager FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_core_fsm/sme_core_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef SME_CORE_PROCESS_H
#define SME_CORE_PROCESS_H

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
extern const FsmProcessStateMachine sme_core_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/
extern CsrBool core_get_startupPowerMaintainedFlag(FsmContext* context, CsrUint16 instanceId);
extern CsrBool core_get_flightmodeBootFlag(FsmContext* context);
/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_CORE_PROCESS_H */
