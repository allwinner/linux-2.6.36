/** @file coex_manager_fsm.h
 *
 * PAL Coex Manager API
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
 *   Public Coex Manager FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/coex_manager_fsm/coex_manager_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef PAL_COEX_MANAGER_FSM_H
#define PAL_COEX_MANAGER_FSM_H

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
extern const FsmProcessStateMachine pal_coex_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   get local PAL Coex capabilities that includes Activity Report support and AR Scheduling support.
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[out]    cap   : local PAL coex capability
 *
 * @return
 *   void
 */
extern void pal_get_local_coex_capability(FsmContext *context, PAL_CoexCapabilities *cap);

/**
 * @brief
 *    function to generate activity report for test purposes only.
 *
 * @par Description
 *    This function can be called to generate activity report from all active physical links.
 * It is introduced for test purposes only.
 *
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    scheduleKnown   :  TRUE if schedule is known , else FALSE.
 * @param[in]    startTime   :  start time 
 * @param[in]    duration   :  duration 
 * @param[in]    periodicity   :  periodicity 
 *
 * @return
 *    void
 */
extern void pal_coex_generate_ar(FsmContext* context,
                                 CsrBool scheduleKnown,
                                 CsrUint32 startTime,
                                 CsrUint32 duration,
                                 CsrUint32 periodicity);


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PAL_COEX_MANAGER_FSM_H */
