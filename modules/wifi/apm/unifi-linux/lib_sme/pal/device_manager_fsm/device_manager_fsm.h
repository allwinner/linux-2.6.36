/** @file device_manager_fsm.h
 *
 * PAL Device Manager API
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
 *   Public Device Manager FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/device_manager_fsm/device_manager_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef PAL_DEVICE_MANAGER_FSM_H
#define PAL_DEVICE_MANAGER_FSM_H

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
extern const FsmProcessStateMachine pal_dm_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Function to call to lock PAL to a particular during channel selection
 *
 * @param[in] context               : FSM context
 * @param[in] channel               : Channel to lock to
 *
 * @return
 *   CsrBool
 */
extern void pal_dm_set_channel_to_select(FsmContext* context, CsrUint8 channel);

/**
 * @brief
 *   Function to get the subscription handle for a protocol 
 *
 * @param[in] context               : FSM context
 * @param[in] prot               : protocol id
 *
 * @return
 *   the handle returned during subscription 
 */
extern CsrUint8 pal_dm_get_subscription_handle(FsmContext* context, PAL_DataProtocolId prot);
/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PAL_DEVICE_MANAGER_FSM_H */
