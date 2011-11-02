/** @file qos_tclas.c
 *
 * Measurement support functions
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
 * tclas support functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_tclas.c#1 $
 *
 ****************************************************************************/

/** @{
 * @ingroup qos
 */

#include "qos_fsm/qos_tclas.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "ie_access/ie_access.h"


/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * @brief Process measurements commands
 *
 * @par Description:
 * Process measurements commands
 *
 * @ingroup ccx
 *
 * @param[in] context : Context of calling FSM
 * @param[in] command : Command pointer
 *
 * @return
 *      void
 */
/*---------------------------------------------------------------------------*/
unifi_TspecResultCode validate_tclas(
        FsmContext* context,
        CsrUint16 tclasLength,
        CsrUint8 *tclas)
{
    /* no validation at present */

    sme_trace_entry((TR_QOS, "validate_tclas()"));

    return unifi_TspecResultSuccess;
}

