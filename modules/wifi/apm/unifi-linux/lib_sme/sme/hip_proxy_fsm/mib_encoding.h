/** @file mib_encoding.h
 *
 * mib access support function header file
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
 *   Provides the external function declarions for mib access process
 *   support functions.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mib_encoding.h#2 $
 *
 ****************************************************************************/
#ifndef MIB_ENCODING_H
#define MIB_ENCODING_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "hip_proxy_fsm/mibdefs.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

extern DataReference mib_encode_file(FsmContext* context, const CsrUint8* data, CsrUint32 length, CsrUint16 startIndex, CsrUint16* endIndex);

extern DataReference mib_encode_create_get(FsmContext* context, CsrUint8 numRequests);
extern unifi_Status  mib_encode_add_get(FsmContext* context, DataReference* dataRef, mib_ids id, CsrUint8 index1, CsrUint8 index2);
extern unifi_Status  mib_decode_get_CsrBool(FsmContext* context, const DataReference* dataRef, CsrUint16 valueIndex, CsrBool* value);
extern unifi_Status  mib_decode_get_int(FsmContext* context, const DataReference* dataRef, CsrUint16 valueIndex, CsrInt32* value);
extern unifi_Status  mib_decode_get_int64(FsmContext* context, const DataReference* dataRef, CsrUint16 valueIndex, CsrInt32* mswvalue, CsrUint32* lswvalue);
extern unifi_Status  mib_decode_get_os(FsmContext* context,  const DataReference* dataRef, CsrUint16 valueIndex, CsrUint8* data, CsrUint16 datalength);

extern DataReference mib_encode_create_set(FsmContext* context,  CsrUint8 numIntRequests, CsrUint8 numOsRequests);
extern unifi_Status  mib_encode_add_set_boolean(FsmContext* context, DataReference* dataRef, mib_ids id, CsrBool value, CsrUint8 index1, CsrUint8 index2);
extern unifi_Status  mib_encode_add_set_int(FsmContext* context, DataReference* dataRef, mib_ids id, CsrInt32 value, CsrUint8 index1, CsrUint8 index2);
extern unifi_Status  mib_encode_add_set_os (FsmContext* context, DataReference* dataRef, mib_ids id, const CsrUint8* data, CsrUint8 datalength, CsrUint8 index1, CsrUint8 index2);


#ifdef __cplusplus
}
#endif


#endif /* MIB_ACCESS_H */
