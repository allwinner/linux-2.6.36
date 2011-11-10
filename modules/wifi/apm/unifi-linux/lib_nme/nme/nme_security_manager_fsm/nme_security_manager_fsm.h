/** @file nem_security_manager_fsm.h
 *
 * Public NME Security Manager FSM API
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
 *   Public NME Security Manager FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_security_manager_fsm/nme_security_manager_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef NME_SECURITY_MANAGER_PROCESS_H
#define NME_SECURITY_MANAGER_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nem_security_manager
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
 *   Defines the nme security manager state machine data
 */
extern const FsmProcessStateMachine nme_security_manager_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Attempts to return a pointer to a copy of the credentials stored within
 *   the security FSM
 *
 * @par Description
 *   Attempts to return a pointer to a copy of the credentials stored within
 *   the security FSM
 *
 * @param[in] context * : FSM context
 *
 * @return
 *   unifi_Credentials * : Either NULL or a pointer to the credentials
 */
extern unifi_Credentials *nme_sec_get_credentials(FsmContext* context);

/**
 * @brief
 *   Return a copy of WPS SSID
 *
 * @par Description
 *   Return a copy of WPS SSID
 *
 * @param[in] context * : FSM context
 *
 * @return
 *   unifi_SSID : SSID.  a zero length indicates no SSID found.
 */
extern unifi_SSID nme_sec_get_wps_ssid(FsmContext* context);

/**
 * @brief
 *   Return the current auth mode mask
 *
 * @par Description
 *   Return the current auth mode mask
 *
 * @param[in] context * : FSM context
 *
 * @return
 *   unifi_AuthMode
 */
extern unifi_AuthMode nme_sec_get_auth_mode_mask(FsmContext* context);


#ifdef __cplusplus
}
#endif

#endif /* NME_SECURITY_MANAGER_PROCESS_H */
