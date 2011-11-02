/** @file ie_access.c
 *
 * Utilities to read and write information elements
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
 *   Utilities to read and write information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access.c#2 $
 *
 ****************************************************************************/

#include "ie_access/ie_access.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "abstractions/osa.h"
#include "fsm/csr_wifi_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

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
 * See description in ie_access/ie_access.h
 */
/*---------------------------------------------------------------------------*/
CsrBool ie_hasGRates(
                    CsrUint8* elements,
                    CsrUint32 length)
{
    CsrUint8 i;
    CsrUint8 len;
    CsrUint8* rates;

    if (ie_find(IE_ID_SUPPORTED_RATES, elements, length, &rates) != ie_success)
    {
        return FALSE;
    }

    len = rates[1];
    rates += 2;
    for(i = 0; i < len; ++i, ++rates)
    {
        CsrUint8 rate = *rates & 0x7f;
        if((rate == 0x0c) || (rate == 0x12) || (rate == 0x18) || (rate == 0x24) ||
           (rate == 0x30) || (rate == 0x48) || (rate == 0x60) || (rate == 0x6c))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Description:
 * See description in ie_access/ie_access.h
 */
/*---------------------------------------------------------------------------*/
void ie_printIEInfo(
                    CsrUint8* elements,
                    CsrUint32 length)
{
    CsrUint8* curr = elements;
    CsrUint8* end = elements + length;
    unifi_SSID ssid;

    require(TR_IE_ACCESS, elements != NULL);
    require(TR_IE_ACCESS, length != 0);

    if(ie_success == ie_getSSID(elements, &ssid))
    {
        sme_trace_debug_code(char ssidBuffer[33];)
        sme_trace_debug((TR_IE_ACCESS, "ssid: %d",  trace_unifi_SSID(&ssid, ssidBuffer)));
    };

    while (curr < end)
    {
        if (!ie_valid(curr, end))
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::ie_find() :: ie_invalid :: id = %d, length = %d",
                                                ie_id(elements), ie_len(elements)));
            verify(TR_IE_ACCESS, 0);    /*lint !e774*/
        }

        sme_trace_hex((TR_IE_ACCESS, TR_LVL_INFO, "ie Dump :: pkt dump",
                        &ie_id(curr),
                        ie_len(curr)+2));

        curr = ie_next(curr);
    }
}
