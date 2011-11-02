/** @file ie_access_associate_cfm.c
 *
 * Utilities to process WPS information elements
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
 *   Utilities to process WPS information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_message_handling/ie_access_associate_cfm.c#2 $
 *
 ****************************************************************************/
#include "ie_message_handling/ie_access_associate_cfm.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "abstractions/osa.h"
#include "fsm/csr_wifi_fsm.h"
#include "ie_access/ie_access.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "ie_access/ie_access.h"

#include "qos_fsm/qos_fsm.h"

#include "sys_sap/sys_sap_from_sme_interface.h"

#include "smeio/smeio_trace_types.h"


/* MACROS *******************************************************************/


/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in ie_access/ie_access_associate_cfm.h
 */
/*---------------------------------------------------------------------------*/
void ie_process_associated_cfm_ies(
        FsmContext* context,
        CsrUint8 *pTspecBuf,
        CsrUint8 tspecBufLength)
{
#ifdef CCX_VARIANT
    SmeConfigData* cfg = get_sme_config(context);
#endif

    sme_trace_entry((TR_IE_ACCESS, "ie_process_associated_cfm_ies"));

    sme_trace_hex((TR_QOS, TR_LVL_DEBUG, "ie_process_associated_cfm_ies", pTspecBuf, tspecBufLength));

#ifdef CCX_VARIANT
        if(cfg->joinIECapabilities & CCX_Capable)
/*            if( (cfg->joinIECapabilities & CCX_Capable)
              &&(cfg->joinIECapabilities & WMM_Capable))
*/
        {
/*            qos_tspec_process_associate_cfm_ie(qos_get_tspecdatablk_context(context), pTspecBuf, tspecBufLength);
 */
        }
#endif
}


