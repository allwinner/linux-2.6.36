/** @file paldata_top_level_fsm.c
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
 *   Creates and initialises PAL Data manager
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_paldata/paldata/paldata_top_level_fsm/paldata_top_level_fsm.c#2 $
 *
 ****************************************************************************/

/** @{
 * @ingroup paldata_top_level_fsm
 */


/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_internal.h"

#include "paldata_top_level_fsm/paldata_top_level_fsm.h"

#include "data_manager/data_manager_fsm.h"


/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC CONSTANT DEFINITIONS **********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in paldata_top_level_fsm/paldata.h
 */
FsmContext* paldata_init(void* externalContext)
{
    PalDataContext* paldataContext = (PalDataContext*)CsrPmalloc(sizeof(PalDataContext));

    paldataContext->fsmContext = fsm_init_context(paldataContext, externalContext, 1);

    fsm_install_app_ignore_event_callback(paldataContext->fsmContext, NULL);

    paldataContext->palDamFsmInstance = fsm_add_instance(paldataContext->fsmContext, &pal_dam_fsm, FALSE);

    fsm_call_entry_functions(paldataContext->fsmContext);
    return paldataContext->fsmContext;
}

/**
 * See description in paldata_top_level_fsm/paldata.h
 */
FsmContext* paldata_reset(FsmContext* context)
{
    void* extContext = context->externalContext;
    paldata_shutdown(context);
    return paldata_init(extContext);
}

/**
 * See description in paldata_top_level_fsm/paldata.h
 */
void paldata_shutdown(FsmContext* context)
{
    PalDataContext* paldataContext = getPalDataContext(context);
    fsm_shutdown(context);
    CsrPfree(paldataContext);
}

/**
 * See description in paldata_top_level_fsm/paldata.h
 */
FsmTimerData paldata_execute(FsmContext* context)
{
    return fsm_execute(context);
}

/**
 * See description in paldata_top_level_fsm/paldata.h
 */
void paldata_install_wakeup_callback(FsmContext* context, FsmExternalWakupCallbackPtr callback)
{
    fsm_install_wakeup_callback(context, callback);
}

/** @}
 */
