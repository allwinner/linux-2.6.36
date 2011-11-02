/** @file sme_measurements_fsm.h
 *
 * Public SME measurements FSM API
 * 
 * @section LEGAL
 *   CONFIDENTIAL
 * 
 *   Copyright (C) Cambridge Silicon Radio Ltd 2007. All rights reserved.
 * 
 * @section DESCRIPTION
 *   Public SME measurements FSM API
 * 
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_measurements/sme_measurements_fsm.h#1 $
 * 
 ****************************************************************************/
#ifndef SME_MEASUREMENTS_FSM_H
#define SME_MEASUREMENTS_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup sme_measurements
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
 *   Defines the sme measurements state machine data  
 */
extern const FsmProcessStateMachine sme_measurements_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_MEASUREMENTS_FSM_H */
