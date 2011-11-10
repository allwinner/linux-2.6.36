/** @file event_pack_unpack.c
 *
 * event Packing and unpacking functions
 *
 * @section LEGAL
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Implements Event Packing and Unpacking of signals for compatability
 *   across platforms. different Endianness and Native Packing schemes need
 *   to be supported.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/event_pack_unpack/event_pack_unpack.c#1 $
 *
 ****************************************************************************/
/** @ingroup common
 * @{
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "event_pack_unpack/event_pack_unpack.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
CsrUint16 event_pack_CsrUint8 (CsrUint8** resultBuffer, CsrUint8  value)
{
    (*resultBuffer)[0] = value;
    (*resultBuffer)++;
    return 1;
}

/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
CsrUint16 event_pack_CsrUint16(CsrUint8** resultBuffer, CsrUint16 value)
{
    (*resultBuffer)[0] =  value        & 0xFF;
    (*resultBuffer)[1] = (value >>  8) & 0xFF;
    (*resultBuffer) += 2;
    return 2;
}

/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
CsrUint16 event_pack_CsrUint32(CsrUint8** resultBuffer, CsrUint32 value)
{
    (*resultBuffer)[0] = (CsrUint8)value         & 0xFF;
    (*resultBuffer)[1] = (CsrUint8)(value >>  8) & 0xFF;
    (*resultBuffer)[2] = (CsrUint8)(value >> 16) & 0xFF;
    (*resultBuffer)[3] = (CsrUint8)(value >> 24) & 0xFF;
    (*resultBuffer) += 4;
    return 4;
}

/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
CsrUint16 event_pack_buffer(CsrUint8** resultBuffer, const CsrUint8* value, CsrUint16 numberOfBytes)
{
    CsrMemMove(*resultBuffer, value, numberOfBytes);
    (*resultBuffer) += numberOfBytes;
    return numberOfBytes;
}

/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
CsrUint16 event_pack_string(CsrUint8** resultBuffer, const char* value)
{
    CsrUint16 len = (CsrUint16)CsrStrLen(value) + 1;
    (void)event_pack_CsrInt16(resultBuffer, (CsrInt16)len);
    CsrMemMove(*resultBuffer, value, len);
    (*resultBuffer) += len;
    return len + 2;
}


/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
CsrUint8 event_unpack_CsrUint8 (CsrUint8** signalBuffer)
{
    CsrUint8 result = (*signalBuffer)[0];
    (*signalBuffer)++;
    return result;
}

/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
CsrUint16 event_unpack_CsrUint16(CsrUint8** signalBuffer)
{
    CsrUint16 result = ((*signalBuffer)[1] << 8) |
                     (*signalBuffer)[0];
    (*signalBuffer)+=2;
    return result;
}

/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
CsrUint32 event_unpack_CsrUint32(CsrUint8** signalBuffer)
{
    CsrUint32 result = ((*signalBuffer)[3] << 24) |
                    ((*signalBuffer)[2] << 16) |
                    ((*signalBuffer)[1] <<  8) |
                     (*signalBuffer)[0];
    (*signalBuffer)+=4;
    return result;
}

/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
void event_unpack_buffer(CsrUint8** signalBuffer, CsrUint8* resultBuffer, CsrUint16 numberOfBytes)
{
    CsrMemMove(resultBuffer, *signalBuffer, numberOfBytes);
    (*signalBuffer) += numberOfBytes;
}

/**
 * See description in event_pack_unpack/event_pack_unpack.h
 */
char* event_unpack_string(CsrUint8** signalBuffer)
{
    char* result = NULL;
    CsrUint16 len = (CsrUint16)event_unpack_CsrInt16(signalBuffer);
    if (len > 0)
    {
        result = (char*)CsrPmalloc(len);
        if (result) {
            CsrMemMove(result, *signalBuffer, len);
            (*signalBuffer) += len;
        }
    }

    return result;
}


CsrUint16 event_pack_hip_header(CsrUint8** signalBuffer, CsrUint16 id, CsrUint16 pid, const DataReference* dataref1, const DataReference* dataref2)
{
    (void)event_pack_CsrUint16(signalBuffer, id);
    (void)event_pack_CsrUint16(signalBuffer, 0);
    (void)event_pack_CsrUint16(signalBuffer, pid);
    if (dataref1)
    {
        (void)event_pack_CsrUint16(signalBuffer, dataref1->slotNumber);
        (void)event_pack_CsrUint16(signalBuffer, dataref1->dataLength);
    }
    else
    {
        (void)event_pack_CsrUint32(signalBuffer, 0);
    }

    if (dataref2)
    {
        (void)event_pack_CsrUint16(signalBuffer, dataref2->slotNumber);
        (void)event_pack_CsrUint16(signalBuffer, dataref2->dataLength);
    }
    else
    {
        (void)event_pack_CsrUint32(signalBuffer, 0);
    }
    return 14;
}


/** @}
 */
