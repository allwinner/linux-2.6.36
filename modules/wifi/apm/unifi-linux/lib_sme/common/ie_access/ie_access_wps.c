/** @file ie_access_wps.c
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_wps.c#1 $
 *
 ****************************************************************************/

#include "ie_access/ie_access_wps.h"

/* STANDARD INCLUDES ********************************************************/

#include "abstractions/osa.h"

/* PROJECT INCLUDES *********************************************************/

#include "ie_access/ie_access.h"


/* MACROS *******************************************************************/


/* GLOBAL VARIABLE DEFINITIONS **********************************************/
/** WPS OUI Definition */
const CsrUint8  wpsOui[4] = {0x00, 0x50, 0xF2, 0x04};

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in ie_access/ie_access_wps.h
 */
/*---------------------------------------------------------------------------*/
CsrBool ie_wps_check_ap_wps_capability(
                    CsrUint8 *pbuf,
                    CsrUint16 length)
{
    CsrUint8     *dummyIe;

    if(ie_success == ie_find_vendor_specific(wpsOui, NULL, 0, pbuf, length, &dummyIe))
    {
        return TRUE;
    }

    return FALSE;
}

