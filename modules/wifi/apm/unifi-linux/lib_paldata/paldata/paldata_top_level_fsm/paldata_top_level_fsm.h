/** @file paldata_top_level_fsm.h
 *
 * PAL Data Top Level
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
 *   PAL Data API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_paldata/paldata/paldata_top_level_fsm/paldata_top_level_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef PALDATA_TOP_LEVEL_FSM_H
#define PALDATA_TOP_LEVEL_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup paldata_top_level_fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "paldata_top_level_fsm/paldata.h"
#include "fsm/fsm_internal.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "palio/palio_fsm_events.h"
#include "paldataio/paldataio_fsm_events.h"

#include "pal_hci_sap/pal_hci_sap_types.h"
#include "pal_hci_sap/pal_hci_sap_signals.h"

#include "data_manager/data_manager_fsm_types.h"
#include "data_manager/data_manager_fsm_events.h"

/* PUBLIC MACROS ************************************************************/
#define getPalDataContext(fsmContext) ((PalDataContext*)fsmContext->applicationContext)

/* App handle agreed between SME and NME. SME will use NULL and NME to use 2*/
#define PALDATA_APP_HANDLE(context) (void *)((UNIFI_PALDATA_APPHANDLE << 6) | (context->currentInstance->instanceId)) 
/**
 *   Fixed Process type ID's
 */
#define PAL_DAM_PROCESS             (0x9099)


/* PUBLIC TYPES DEFINITIONS *************************************************/

typedef struct PalDataContext {

    CsrUint16 palDamFsmInstance;

    FsmContext* fsmContext;

} PalDataContext;


/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PALDATA_TOP_LEVEL_FSM_H */
