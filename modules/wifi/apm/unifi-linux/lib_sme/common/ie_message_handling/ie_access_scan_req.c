/** @file ie_access_scan_req.c
 *
 * Utilities to populate scan req information elements
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
 *   Utilities to populate scan req information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_message_handling/ie_access_scan_req.c#2 $
 *
 ****************************************************************************/
#include "ie_message_handling/ie_access_scan_req.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in ie_access/ie_access_associate_req.h
 */
/*---------------------------------------------------------------------------*/
DataReference ie_create_scan_req_ies(
        FsmContext* context,
        CsrUint8 ssidCount,
        const unifi_SSID * const pSsid,
        CsrUint16 probeIeLength,
        const CsrUint8* const probeIe)
{
    DataReference scanReqInfoElements = {0, 0};
    CsrUint8 *pCurrent;
    CsrUint8* pBuf;
    CsrUint8 i;

    sme_trace_entry((TR_IE_ACCESS, "ie_create_scan_req_ies"));


    /* ------------------------------------------------------------------------
     * Calculate the length
     * ------------------------------------------------------------------------ */
    scanReqInfoElements.dataLength = probeIeLength;

    if (ssidCount > 0)
    {
        for(i=0; i<ssidCount; i++)
        {
            scanReqInfoElements.dataLength += pSsid[i].length + 2;
        }
    }

    if(scanReqInfoElements.dataLength == 0)
    {
        return scanReqInfoElements;
    }

    /* ------------------------------------------------------------------------
     * Create the IE buffer
     * ------------------------------------------------------------------------ */
    pld_create(getPldContext(context), scanReqInfoElements.dataLength, (void **)&pBuf, &scanReqInfoElements.slotNumber );
    pCurrent = pBuf;

    /* ------------------------------------------------------------------------
     * Populate the IE buffer
     * ------------------------------------------------------------------------ */
    if (ssidCount > 0)
    {
        for(i=0; i<ssidCount; i++)
        {
            (void)ie_setSSID(pCurrent, &pSsid[i]);
            pCurrent += pSsid[i].length+2;
        }
    }
    CsrMemCpy(pCurrent, probeIe, probeIeLength);

    return scanReqInfoElements;
}


