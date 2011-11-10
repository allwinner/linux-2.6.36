/** @file data_manager_fsm.h
 *
 * PAL Data Manager
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
 *   APIs for data manager.
 *
 ****************************************************************************
 *
 * @section MODIFICATION HISTORY
 * @verbatim
 *   #1    17:Apr:08 B-36899: Created
 * @endverbatim
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_paldata/paldata/data_manager/data_manager_fsm.h#1 $
 *
 ****************************************************************************/

#ifndef PAL_DATA_MANAGER_FSM_H
#define PAL_DATA_MANAGER_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup data_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
extern const FsmProcessStateMachine pal_dam_fsm;


/* PUBLIC FUNCTION PROTOTYPES ***********************************************/


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* IP_CONNECTION_MGR_FSM_H */
