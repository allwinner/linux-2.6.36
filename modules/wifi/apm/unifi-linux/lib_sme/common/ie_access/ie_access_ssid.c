/** @file ie_access_ssid.c
 *
 * Utilities to read and write SSID elements
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
 *   Utilities to read and write SSID elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_ssid.c#1 $
 *
 ****************************************************************************/

#include "ie_access/ie_access.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
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
 * See description in ie_access/ie_access.h
 */
/*---------------------------------------------------------------------------*/
ie_result ie_getSSIDStr(
        CsrUint8* element,
        char* result)
{
    require(TR_IE_ACCESS, element != NULL);
    require(TR_IE_ACCESS, result != NULL);

    result[0] = '\0';

    if (ie_id(element) == IE_ID_SSID)
    {
        CsrUint8 element_length = ie_len(element);
        CsrMemCpy(result, (char*)ie_info(element), element_length);
        result[element_length] = '\0';
        return ie_success;
    }
    return ie_not_found;
}

/*
 * Description:
 * See description in ie_access/ie_access.h
 */
/*---------------------------------------------------------------------------*/
ie_result ie_getSSID(
        CsrUint8* element,
        unifi_SSID* result)
{
    require(TR_IE_ACCESS, element != NULL);
    require(TR_IE_ACCESS, result != NULL);

    if (ie_id(element) == IE_ID_SSID)
    {
        if (ie_len(element) > 32)
        {
            return ie_error;
        }
        result->length = ie_len(element);
        CsrMemCpy(result->ssid, ie_info(element), result->length);

        return ie_success;
    }
    return ie_not_found;
}

/*
 * Description:
 * See description in ie_access/ie_access.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8 ie_get_ssid_length(
        const unifi_SSID * const pSsid)
{
    require(TR_IE_ACCESS, pSsid != NULL);

    sme_trace_entry((TR_IE_ACCESS, "pSsid->length [%d]", pSsid->length));

    if (pSsid->length == 0)
    {
        return 0;
    }
    else
    {
        return (pSsid->length+2);
    }
}

/*
 * Description:
 * See description in ie_access/ie_access.h
 */
/*---------------------------------------------------------------------------*/
ie_result ie_setSSID(
        CsrUint8* element,
        const unifi_SSID* const result)
{
    require(TR_IE_ACCESS, element != NULL);
    require(TR_IE_ACCESS, result != NULL);

    element[0] = IE_ID_SSID;
    element[1] = result->length;
    /* Length can == 0 when used in scan req */
    if (result->length != 0)
    {
        CsrMemCpy(ie_info(element), result->ssid, result->length);
    }
    return ie_success;
}

