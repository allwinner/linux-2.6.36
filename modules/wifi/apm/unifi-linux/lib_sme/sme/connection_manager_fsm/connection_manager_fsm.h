/** @file connection_manager_fsm.h
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/connection_manager_fsm/connection_manager_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef CONNECTION_MANAGER_PROCESS_H
#define CONNECTION_MANAGER_PROCESS_H

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
/**
 * @brief mlme_join_req retrys
 *
 * @par Description
 *   Number of retrys of join before giving up.
 */
#define MAX_MLME_JOIN_RETRYS 2

/**
 * @brief mlme_authenticate_req retrys
 *
 * @par Description
 *   Number of retrys of authentication before giving up.
 */
#define MAX_MLME_AUTHENTICATION_RETRYS 2

/**
 * @brief mlme_associate_req retrys
 *
 * @par Description
 *   Number of retrys of association before giving up.
 */
#define MAX_MLME_ASSOCIATE_RETRYS 2

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/
/**
 * @brief list of GHz (*100) for 802.11 b &
 */
extern const CsrUint16 frequencyTable2point4Ghz[];

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the connection manager state machine data
 */
extern const FsmProcessStateMachine connection_manager_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* CONNECTION_MANAGER_PROCESS_H */
