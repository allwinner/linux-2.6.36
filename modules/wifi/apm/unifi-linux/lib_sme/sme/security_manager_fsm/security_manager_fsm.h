/** @file security_manager_fsm.h
 *
 * Public Security Manager FSM API
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
 *   Public Security Manager FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/security_manager_fsm/security_manager_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_MANAGER_PROCESS_H
#define SECURITY_MANAGER_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup security_manager
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
 *   Defines the security manager state machine data
 */
extern const FsmProcessStateMachine security_manager_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Accessor for the RSNA enabled
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 * @param[in]  instanceId  : The instance to retreive the data from
 *
 * @return
 *   Is this a WPA Connection
 */
extern CsrBool get_security_rsna_enabled(FsmContext* context, CsrUint16 instanceId);

/** @}
 */
extern CsrBool check_have_set_keys(FsmContext* context, CsrUint16 instanceId);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_MANAGER_PROCESS_H */
