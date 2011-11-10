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
 *   $Id: //depot/dot11/v7.0p/host/lib_info_elements/ie_utils/ie_utils.c#1 $
 *
 ****************************************************************************/

#include "ie_utils/ie_utils.h"
#ifdef LIB_INFO_ELEMENT_TRACE_ENABLE
#include "sme_trace/sme_trace.h"
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

const CsrUint8  ieWmmInformationOui[OUI_SIZE]               = {0x00, 0x50, 0xF2, 0x02};
const CsrUint8  ieWmmInformationOuiSubtype[OUISUBTYPE_SIZE] = {0x00};
const CsrUint8  ieWmmParameterOui[OUI_SIZE]                 = {0x00, 0x50, 0xF2, 0x02};
const CsrUint8  ieWmmParameterOuiSubtype[OUISUBTYPE_SIZE]   = {0x01};
const CsrUint8  ieWmmTspecOui[OUI_SIZE]                     = {0x00, 0x50, 0xF2, 0x02};
const CsrUint8  ieWmmTspecOuiSubtype[OUISUBTYPE_SIZE]       = {0x02};

const CsrUint8  ieWpaOui[OUI_SIZE]                          = {0x00, 0x50, 0xF2, 0x01};
const CsrUint8  ieWpaOuiSubtype[OUISUBTYPE_SIZE]            = {0x00};

const CsrUint8  ieWpsOui[OUI_SIZE]                          = {0x00, 0x50, 0xF2, 0x04};
const CsrUint8  ieWpsOuiSubtype[OUISUBTYPE_SIZE]            = {0x00};

const CsrUint8  ieCcxTsMetricsOui[OUI_SIZE]                 = {0x00, 0x40, 0x96, 0x07};
const CsrUint8  ieCcxTsMetricsOuiSubtype[OUISUBTYPE_SIZE]   = {0x00};

const CsrUint8  ieCcxSsidlOui[OUI_SIZE]                     = {0x00, 0x50, 0xF2, 0x05};
const CsrUint8  ieCcxSsidlOuiSubtype[OUISUBTYPE_SIZE]       = {0x00};


const CsrUint8  ieCcxMbssidOui[OUI_SIZE]                    = {0x00, 0x40, 0x96, 0x01};

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
ie_result ie_find(
                    ie_elementid id,
                    CsrUint8* elements,
                    CsrUint32 length,
                    CsrUint8** result)
{
    CsrUint8* curr = elements;
    CsrUint8* end = elements + length;

    *result = NULL;

    while (curr < end)
    {
        if (!ie_valid(curr, end))
        {
            return ie_invalid;
        }
        if (ie_id(curr) == id)
        {
            *result = curr;
            return ie_success;
        }
        curr = ie_next(curr);
    }

    return ie_not_found;
}

/*
 * Description:
 * See description in ie_access/ie_access.h
 */
/*---------------------------------------------------------------------------*/
ie_result ie_find_vendor_specific(
                    const CsrUint8* oui,
                    const CsrUint8* extra,
                    const CsrUint32 extralength,
                    CsrUint8* elements,
                    CsrUint32 length,
                    CsrUint8** result)
{
    CsrUint8* curr = elements;
    CsrUint8* end = elements + length;

    *result = NULL;

    while (curr < end)
    {
        if (!ie_valid(curr, end))
        {
            return ie_invalid;
        }
        if (ie_id(curr) == IE_ID_VENDOR_SPECIFIC && CsrMemCmp(&curr[2], oui, 4) == 0)
        {
            if (extralength == 0 || CsrMemCmp(&curr[6], extra, extralength) == 0)
            {
                *result = curr;
                return ie_success;
            }
        }
        curr = ie_next(curr);
    }

    return ie_not_found;
}

/*
 * Description:
 * See description in ie_access/ie_access.h
 */
/*---------------------------------------------------------------------------*/
ie_result ie_validate_ie_buffer(
                    CsrUint8* elements,
                    CsrUint32 length)
{
    CsrUint8* curr = elements;
    CsrUint8* end = elements + length;

    /* make sure there is */
    if(length == 0)
        return ie_invalid;

    /* scan through and validate all IEs */
    while (curr < end)
    {
        if (!ie_valid(curr, end))
        {
            return ie_invalid;
        }
        curr = ie_next(curr);
    }

    return ie_success;
}
#ifdef LIB_INFO_ELEMENT_TRACE_ENABLE
/*
 *  This is a Temporary implementation
 */
static void sme_trace_line_port2(sme_trace_module_id id, sme_trace_level level, const char* const format, va_list args)
{
    printf("                         IE_PRT , TR_LVL_ERROR, ");
    (void)vprintf(format, args); /*lint !e64 */
    printf("\n");
    va_end(args);
    (void)fflush(stdout);
}
#endif

void CsrLog(CsrUint16 logLevel, CsrString * prefixString, const CsrString * format, ...)
{
#ifdef LIB_INFO_ELEMENT_TRACE_ENABLE
    va_list args;
    va_start(args, format);

    sme_trace_line_port2(TR_IE_PRINT, TR_LVL_ERROR, (const char*)format, args);
#endif
}

void CsrLogBuffer(CsrUint16 logLevel, CsrString* prefixString, CsrUint8* buffer, CsrUint16 BufferLength, const CsrString* format, ...)
{

}
