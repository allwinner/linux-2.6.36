/** @file ie_access_dot11n_ht_cap.c
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_dot11n_ht_cap.c#2 $
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
CsrUint8 ie_dot11n_get_ht_cap_length(
                    void)
{
    return IE_HT_CAPABILITIES__TOTAL_SIZE;
}

/*
 * Description:
 * See description in dot11n/ie_access_dot11n.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8* ie_dot11n_generate_ht_cap_ie(
                    CsrUint8 *pbuf)
{
    sme_trace_debug((TR_IE_ACCESS, "ie_dot11n_generate_ht_cap_ie "));
    CsrMemSet(pbuf, 0, IE_HT_CAPABILITIES__TOTAL_SIZE);

    /*
    pbuf[IE_DOT11N_HT_CAP__ID_OFFSET] = IE_DOT11N_ID_HT_CAPABILITIES;
    pbuf[IE_DOT11N_HT_CAP__LENGTH_OFFSET] = (IE_HT_CAPABILITIES__TOTAL_SIZE-2); */ /* remove the header size */

    pbuf[0] = IE_DOT11N_ID_HT_CAPABILITIES;
    pbuf[1] = (IE_HT_CAPABILITIES__TOTAL_SIZE-2); /* remove the header size */

    /* REMINDER:
     * The firmware will populate this IE, we just allocate the space
     */

    sme_trace_hex((TR_IE_PRINT, TR_LVL_DEBUG, "    data", pbuf, IE_HT_CAPABILITIES__TOTAL_SIZE));

    return pbuf + IE_HT_CAPABILITIES__TOTAL_SIZE;
}

