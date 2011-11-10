/** @file coex_fsm.h
 *
 * Public Coexistence FSM API
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
 *   Public Coexistence FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/coex_fsm/coex_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef COEX_PROCESS_H
#define COEX_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup coexfsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"

/* PUBLIC MACROS ************************************************************/

/* These macros are used by PAL Coex Manager as well.*/
#define BG_CHANNEL_BANDWIDTH 23
#define BT_SLOT_MICROSECONDS 625


/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the state machine data
 */
extern const FsmProcessStateMachine coex_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Get last mlme power mode
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   PowerManagementMode required power mode
 */
extern PowerManagementMode get_required_coex_power_mode(FsmContext* context);

/**
 * @brief
 *   Has Eapol activity been detected
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   CsrBool
 */
extern CsrBool coex_eapol_detected(FsmContext* context);

/**
 * @brief
 *   Has the wake host been set in an mlme_add_periodic
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   CsrBool
 */
extern CsrBool coex_current_wakeHost(FsmContext* context);


/**
 * @brief
 *   Used by security mgr to indicate keys have been installed
 *   (To allow power saving to be enabled)
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   void
 */
extern void coex_wpa_keys_installed(FsmContext* context);

/**
 * @brief
 *   Used by PAL Coext Manager & SME Coex FSM to get the current blackout configuration.
 *
 * @param[in]  context     : FSM context
 * @param[out]  wifiOnlyOrAcl     : TRUE if the trafiic is wifi only or ACL.
 * @param[out]  blackoutDurUs     : blackout duration in microseconds. Valid only if wifiOnlyOrAcl is FALSE
 * @param[out]  blackoutPrdUs     : blackout periodicity in microseconds. Valid only if wifiOnlyOrAcl is FALSE
 * @param[out]  blackoutSrc     : blackout source. Valid only if wifiOnlyOrAcl is FALSE
 *
 * @return
 *   TRUE if there is a valid blackout configuration available.
 */
extern CsrBool coex_get_blackout_configuration(FsmContext* context, 
                                                     CsrBool *wifiOnlyOrAcl,
                                                     CsrUint32 *blackoutDurUs,
                                                     CsrUint32 *blackoutPrdUs,
                                                     BlackoutSource *blackoutSrc);


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* COEX_PROCESS_H */
