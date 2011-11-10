/** @file nme_wps_fsm.h
 *
 * Public NME WPS FSM API
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
 *   Public NME WPS FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_wps_fsm/nme_wps_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_WIFI_NME_WPS_PROCESS_H
#define CSR_WIFI_NME_WPS_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_wps
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "nme_wps_fsm/nme_wps_fsm_types.h"
#include "nme_wps_fsm/nme_wps_fsm_events.h"

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
extern const FsmProcessStateMachine nme_wps_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/
/**
 * @brief
 *   Returns the current security manager instance associated with
 *   the wps fsm.
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext*   : FSM context
 *
 * @return
 *   CsrUint16 : This may be FSM_TERMINATE or an allocated instance.
 */
extern CsrUint16 nme_wps_get_security_fsm_instance(FsmContext* pContext);


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_NME_WPS_PROCESS_H */
