/** @file mib_access_fsm.h
 *
 * Public MIB Access FSM API
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
 *   Public MIB Access FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mib_access_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef MIB_ACCESS_PROCESS_H
#define MIB_ACCESS_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup mib_access
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "payload_manager/payload_manager.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the MIB Access state machine data
 */
extern const FsmProcessStateMachine mib_access_fsm;


/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* MIB_ACCESS_FSM_H */
