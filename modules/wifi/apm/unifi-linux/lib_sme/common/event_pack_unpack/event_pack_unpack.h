/** @file event_pack_unpack.h
 *
 * Event Packing and unpacking functions
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
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/event_pack_unpack/event_pack_unpack.h#1 $
 *
 ****************************************************************************/
#ifndef EVENT_PACK_UNPACK_H
#define EVENT_PACK_UNPACK_H

#ifdef __cplusplus
extern "C" {
#endif

/** @ingroup common
 * @{
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "hostio/hip_fsm_types.h"
/* #include "smeio/smeio_fsm_types.h" */

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Packs data into the result buffer
 *
 * @par Description
 *   Packs data into a buffer from Native format into wire format.
 *
 * @return
 *    CsrUint16 Number of bytes packed
 *
 */
extern CsrUint16 event_pack_CsrUint8 (CsrUint8** resultBuffer, CsrUint8  value);
extern CsrUint16 event_pack_CsrUint16(CsrUint8** resultBuffer, CsrUint16 value);
extern CsrUint16 event_pack_CsrUint32(CsrUint8** resultBuffer, CsrUint32 value);
#define event_pack_CsrInt8(resultBuffer, value)  event_pack_CsrUint8(resultBuffer, (CsrUint8)value)
#define event_pack_CsrInt16(resultBuffer, value) event_pack_CsrUint16(resultBuffer, (CsrUint16)value)
#define event_pack_CsrInt32(resultBuffer, value) event_pack_CsrUint32(resultBuffer, (CsrUint32)value)
extern CsrUint16 event_pack_buffer(CsrUint8** resultBuffer, const CsrUint8* value, CsrUint16 numberOfBytes);
extern CsrUint16 event_pack_string(CsrUint8** resultBuffer, const char* value);

extern CsrUint16 event_pack_hip_header(CsrUint8** signalBuffer, CsrUint16 id, CsrUint16 pid, const DataReference* dataref1, const DataReference* dataref2);


extern CsrUint8  event_unpack_CsrUint8 (CsrUint8** signalBuffer);
extern CsrUint16 event_unpack_CsrUint16(CsrUint8** signalBuffer);
extern CsrUint32 event_unpack_CsrUint32(CsrUint8** signalBuffer);
#define event_unpack_CsrInt8(signalBuffer)  (CsrInt8)event_unpack_CsrUint8(signalBuffer)
#define event_unpack_CsrInt16(signalBuffer) (CsrInt16)event_unpack_CsrUint16(signalBuffer)
#define event_unpack_CsrInt32(signalBuffer) (CsrInt32)event_unpack_CsrUint32(signalBuffer)
extern void       event_unpack_buffer(CsrUint8** signalBuffer, CsrUint8* resultBuffer, CsrUint16 numberOfBytes);
extern char* event_unpack_string(CsrUint8** signalBuffer);

/** @}
 */

#ifdef __cplusplus
}
#endif
#endif /* EVENT_PACK_UNPACK_H */
