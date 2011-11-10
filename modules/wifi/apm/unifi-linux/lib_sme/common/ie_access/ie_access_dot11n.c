/** @file ie_access_dot11n.c
 *
 * Utilities to process 802.11n information elements
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
 *   Utilities to process 802.11n information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_dot11n.c#2 $
 *
 ****************************************************************************/

#include "ie_access/ie_access_dot11n.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "ie_access/ie_access.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in dot11n/ie_access_dot11n.h
 */
/*---------------------------------------------------------------------------*/
CsrBool ie_dot11n_check_ap_capability(
        CsrUint8 *pbuf,
        CsrUint16 length)
{
    CsrUint8 *dummyIe;

    sme_trace_entry((TR_DOT11N, "ie_dot11n_check_ap_capability "));

    require(TR_DOT11N, pbuf != NULL);
    require(TR_DOT11N, length != 0);

    /* decode and print the ies */
    sme_trace_debug_code(
         ie_trace_ht_capabilities(1, pbuf);
         ie_trace_ht_information(1, pbuf);
    )

    if( (ie_success == ie_find(IE_DOT11N_ID_HT_CAPABILITIES, pbuf, length, &dummyIe))
      ||(ie_success == ie_find(IE_DOT11N_ID_HT_INFO, pbuf, length, &dummyIe))
      )
    {
        return TRUE;
    }

    return FALSE;
}

