/** @file profile_manager_fsm.h
 *
 * Public NME Profile Manager FSM API
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
 *   Public NME Profile Manager FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_profile_manager_fsm/nme_profile_manager_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_WIFI_NME_PROFILE_MANAGER_PROCESS_H
#define CSR_WIFI_NME_PROFILE_MANAGER_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_profile_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "nmeio/nmeio_fsm_types.h"

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
extern const FsmProcessStateMachine nme_profile_manager_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

/**
 * @brief
 *   Attempts to return a pointer to a copy of the profile that matches the
 *   supplied profile identity
 *
  * @par Description
 *   Attempts to return a pointer to a copy of the profile that matches the
 *   supplied profile identity.
 *
 * @param[in] context * : FSM context
 * @param[in] const unifi_ProfileIdentity * : possible MAC address and/or SSID to search for
 *
 * @return
 *   unifi_Profile * : Either NULL or a pointer to the profile
 */
extern unifi_Profile *nme_profile_manager_get_profile(
        FsmContext* pContext,
        const unifi_ProfileIdentity *pProfileIdentity);


/**
 * @brief
 *   Attempts to update the session data stored against the specified
 *   profile identity.
 *
 * @par Description
 *   Attempts to find a profile that matches the specified identities
 *   and will update the session data stored in the credentials.
 *
 * @param[in] context * : FSM context
 * @param[in] const unifi_ProfileIdentity * : MAC address and/or SSID to search for
 * @param[in] const CsrUint8 * : session data (could be NULL)
 * @param[in] const CsrUint32: session data Length (could be 0)
 *
 * @return
 *   None
 */
extern void nme_profile_manager_update_profile_session(
        FsmContext* pContext,
        const unifi_ProfileIdentity *pProfileIdentity,
        const CsrUint8 *pSession,
        const CsrUint32 sessionLength);

/**
 * @brief
 *   Attempts to update the fast PAC stored against the specified
 *   profile identity.
 *
 * @par Description
 *   Attempts to find a profile that matches the specified identities
 *   and if it is of FAST credential type will update the PAC stored
 *   in the credentials.
 *
 * @param[in] context * : FSM context
 * @param[in] const unifi_ProfileIdentity * : MAC address and/or SSID to search for
 * @param[in] const CsrUint8 * : PAC data (could be NULL)
 * @param[in] const CsrUint32: PAC Length (could be 0)
 *
 * @return
 *   None
 */
extern void nme_profile_manager_update_profile_pac(
        FsmContext* pContext,
        const unifi_ProfileIdentity *pProfileIdentity,
        const CsrUint8 *pPac,
        const CsrUint32 pacLength);


/**
 * @brief
 *   Returns any empty (NULL) profile.
 *
  * @par Description
 *   Returns any empty (NULL) profile.
 *
 * @param[in] None
 *
 * @return
 *   unifi_Profile * : null profile
 */
extern unifi_Profile *nme_profile_manager_get_null_profile(void);

/**
 * @brief
 *   Build the full cloaked ssid list
 *
  * @par Description
 *   Combines the User list with the cloaked profiles
 *
 * @param[in] None
 *
 * @return
 *   CsrUint8 : number of cloaked ssids
 */
extern CsrUint8 nme_profile_manager_get_cloaked_ssids(FsmContext* pContext, unifi_SSID** cloakedSsids);

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_NME_PROFILE_MANAGER_PROCESS_H */
