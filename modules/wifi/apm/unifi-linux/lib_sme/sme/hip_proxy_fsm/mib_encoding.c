/** @file mib_encoding.c
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mib_encoding.c#2 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "hip_proxy_fsm/mib_encoding.h"
#include "payload_manager/payload_manager.h"
#include "smeio/smeio_trace_types.h"

/** the maximum oid length for a null oid is 25 (see mibdefs.h),
 * the maximum OS size is 300 (see varbind.h). Both 300 and 25
 * are scoped within structs, hence magic number
 */
#define MAX_BER_VARBIND_LENGTH 325

/* BER Tag values. */
#define TAG_INTEGER              (0x02)
#define TAG_OCTET_STRING         (0x04)
#define TAG_NULL                 (0x05)
#define TAG_OBJECT_IDENTIFIER    (0x06)
#define TAG_SEQUENCE             (0x30)
#define TAG_SEQUENCE_OF          (TAG_SEQUENCE)
#define TAG_COUNTER32_INTEGER    (0x41)  /* APPLICATION 1 IMPLICIT.  */
#define TAG_UNSIGNED32_INTEGER   (0x42)  /* APPLICATION 2 IMPLICIT.  */

#define CSR_WIFI_HOSTIO_MIB_SET_MAX 1800

static CsrUint16 multi_value_header(CsrUint8** buf)
{
    CsrUint16 result  = 0;
    if((*buf)[0] != TAG_SEQUENCE)
    {
        sme_trace_hex((TR_MIB_ACCESS, TR_LVL_ERROR, "multi_value_header() : Header is required to start with 0x30", *buf, 8));
        return 0;
    }
    (*buf)++;

    /* magic value => indefinite length. */
    if((*buf)[0] == 0x80) {
        (*buf)++;
        return 0;
    }

    if((*buf)[0] <= 127) {
        result = (*buf)[0];
        (*buf)++;
        return result;
    }

    if((*buf)[0] == 0x81) {
        result = (*buf)[1];
        (*buf)+=2;
        return result;
    }
    if((*buf)[0] == 0x82) {
        result = ((*buf)[1] << 8) + (*buf)[2];
        (*buf)+=3;
        return result;
    }

    return((CsrUint16)-1);
}


static unifi_Status varbind_ber_decode_int(const CsrUint8* buf, CsrInt32* mswresult, CsrUint32* lswresult)
{
    /* Offsets are fixed as we always use single byte length encoding */
  /*CsrUint8 length    = buf[1];*/
    CsrUint8 oidlength = buf[3];
    CsrUint8 tag       = buf[oidlength + 4];
    CsrUint8 valueLen  = buf[oidlength + 5];
    const CsrUint8* tempPtr = &buf[oidlength + 6];
    CsrUint8 i;

    if(buf[0] != TAG_SEQUENCE)
    {
        sme_trace_hex((TR_MIB_ACCESS, TR_LVL_ERROR, "varbind_ber_decode_int() : VarBind is required to start with 0x30", buf, 8));
        return unifi_Error;
    }

    if (buf[2] != TAG_OBJECT_IDENTIFIER)
    {
        sme_trace_hex((TR_MIB_ACCESS, TR_LVL_ERROR, "varbind_ber_decode_int() : Expected TAG_OBJECT_IDENTIFIER missing", buf, 8));
        return unifi_Error;
    }

    if (tag != TAG_INTEGER && tag != TAG_COUNTER32_INTEGER && tag != TAG_UNSIGNED32_INTEGER)
    {
        sme_trace_error((TR_MIB_ACCESS, "varbind_ber_decode_int() : Expected TAG_INTEGER missing got %d", tag));
        return unifi_Error;
    }

    *mswresult = 0;
    *lswresult = 0;

    /* Decode the integer Value */

    for(i = 0; i < valueLen; i++, tempPtr++)
    {
        CsrUint32* decodeValue = lswresult;
        if (valueLen >= 4 && i < valueLen - 4)
        {
            decodeValue = (CsrUint32*)mswresult;
        }

        if(i == 0)
        {
            /* Sign bit on first octet. */
            if(*tempPtr < 128)
            {
                *decodeValue = *tempPtr;
            }
            else
            {
                *decodeValue = *tempPtr - 256;
            }
        }
        else
        {
            *decodeValue = (*decodeValue << 8) + *tempPtr;
        }
    }

    return unifi_Success;
}

static unifi_Status varbind_ber_decode_os(const CsrUint8* buf, CsrUint8* data, CsrUint16 datalength)
{
    /* Offsets are fixed as we always use single byte length encoding */
  /*CsrUint8 length    = buf[1];*/
    CsrUint8 oidlength = buf[3];
    CsrUint8 tag       = buf[oidlength + 4];
    CsrUint8 valueLen  = buf[oidlength + 5];

    sme_trace_hex((TR_MIB_ACCESS, TR_LVL_DEBUG, "varbind_ber_decode_os()", buf, oidlength + 3));

    if(buf[0] != TAG_SEQUENCE)
    {
        sme_trace_hex((TR_MIB_ACCESS, TR_LVL_ERROR, "varbind_ber_decode_os() : VarBind is required to start with 0x30", buf, 8));
        return unifi_Error;
    }

    if (buf[2] != TAG_OBJECT_IDENTIFIER)
    {
        sme_trace_hex((TR_MIB_ACCESS, TR_LVL_ERROR, "varbind_ber_decode_os() : Expected TAG_OBJECT_IDENTIFIER missing", buf, 8));
        return unifi_Error;
    }

    if (tag != TAG_OCTET_STRING)
    {
        sme_trace_error((TR_MIB_ACCESS, "varbind_ber_decode_os() : Expected TAG_INTEGER missing got %d", tag));
        return unifi_Error;
    }

    if (valueLen > datalength)
    {
        sme_trace_error((TR_MIB_ACCESS, "varbind_ber_decode_os() : Supplied Buffer not large enough. Was given %d bytes but need %d bytes", datalength, valueLen));
        return unifi_Error;
    }

    CsrMemCpy(data, &buf[oidlength + 6], valueLen);
    return unifi_Success;
}


static DataReference mib_encode_create(FsmContext* context, CsrUint16 size)
{
    CsrUint8* buf = NULL;
    DataReference dataRef = {0, 4};
    pld_create(getPldContext(context), size, (void**)&buf, (PldHdl*)&dataRef.slotNumber);

    /* Set up the header */
    buf[0] = 0x30;
    buf[1] = 0x82;
    buf[2] = 0x00;
    buf[3] = 0x00;
    return dataRef;
}

DataReference mib_encode_create_get(FsmContext* context, CsrUint8 numrequests)
{
    return mib_encode_create(context, (CsrUint16)(numrequests * MAX_ACCESSOR_LEN));
}

DataReference mib_encode_create_set(FsmContext* context, CsrUint8 numIntRequests, CsrUint8 numOsRequests)
{
    return mib_encode_create(context, (CsrUint16)((numIntRequests * MAX_ACCESSOR_LEN) + (numOsRequests * MAX_BER_VARBIND_LENGTH)) ); /*lint !e734*/
}

static unifi_Status insert_indices(CsrUint8* berOid, CsrUint8 index1, CsrUint8 index2, CsrUint8 indexsRequired)
{
    CsrUint16 nullPos = berOid[1];

    if (indexsRequired == 0)
    {
        return unifi_Success;
    }

    if (indexsRequired == 1 && index1 == 0)
    {
        sme_trace_error((TR_MIB_ACCESS, "Expected index1 was not supplied"));
        return unifi_Error;
    }
    if (indexsRequired == 2 && (index1 == 0 || index2 == 0))
    {
        sme_trace_error((TR_MIB_ACCESS, "Expected 2 index's but got index1(%d) and index2(%d)", index1, index2));
        return unifi_Error;
    }

    berOid[nullPos - indexsRequired] = index1;
    if (indexsRequired == 2)
    {
        berOid[nullPos - 1] = index2;
    }

    return unifi_Success;
}

static void rebuildrequest(CsrUint8* writeBuf, const mib_accessor* mibacc)
{
    /* Copy the data */
    CsrMemCpy(writeBuf, mibacc->oidheader->oidheader, mibacc->oidheader->length);
    CsrMemCpy(&writeBuf[mibacc->oidheader->length], mibacc->oid, mibacc->oidlength);

    writeBuf[mibacc->oidheader->length + mibacc->oidlength] = 0x05;
    writeBuf[mibacc->oidheader->length + mibacc->oidlength + 1] = 0x00;

    /* And write the correct lengths */
    writeBuf[1] = mibacc->oidheader->length + mibacc->oidlength;
    writeBuf[3] = writeBuf[1] - 4;
}

unifi_Status mib_encode_add_get(FsmContext* context, DataReference* dataRef, mib_ids id, CsrUint8 index1, CsrUint8 index2)
{
    unifi_Status result = unifi_Success;
    CsrUint16 length = 0;
    CsrUint8* writeBuf = NULL;
    CsrUint8* pldBuf = NULL;
    CsrUint16 pldBufLength = 0;
    CsrUint8 oidLength = mib_accessors[id].oidheader->length + mib_accessors[id].oidlength + 2;

    sme_trace_entry((TR_MIB_ACCESS, "mib_encode_add_get(%d, %d, %d)", id, index1, index2));

    pld_access(getPldContext(context), dataRef->slotNumber, (void**)&pldBuf, &pldBufLength);

    length = pldBuf[2] << 8 | pldBuf[3];
    writeBuf = &pldBuf[length + 4];

    if (length+4+oidLength > pldBufLength)
    {
        sme_trace_crit((TR_MIB_ACCESS, "mib_decode_get_int() mib get buffer not big enough. Buffer Size = %d bytes. Need %d bytes.", pldBufLength, length+4+oidLength));
        return unifi_Error;
    }

    rebuildrequest(writeBuf, &mib_accessors[id]);
    result = insert_indices(writeBuf, index1, index2, mib_accessors[id].indices);

    /* Update the Length for the full multi get req */
    length += oidLength;
    pldBuf[2] = length >> 8;
    pldBuf[3] = length & 0xFF;

    dataRef->dataLength += oidLength;

    sme_trace_hex((TR_MIB_ACCESS, TR_LVL_DEBUG, "mib_encode_add_get() Encoded", writeBuf, oidLength));

    return result;
}

static CsrUint8* get_encoded_buffer_at(FsmContext* context, const DataReference* dataRef, CsrUint16 valueIndex)
{
    CsrUint8* readBuf = NULL;
    CsrUint8* pldBuf = NULL;
    CsrUint16 pldBufLength = 0;
    CsrUint16 i;

    sme_trace_entry((TR_MIB_ACCESS, "get_encoded_buffer_at(%d)", valueIndex));
    pld_access(getPldContext(context), dataRef->slotNumber, (void**)&pldBuf, &pldBufLength);
    sme_trace_hex((TR_MIB_ACCESS, TR_LVL_DEBUG, "get_encoded_buffer_at()", pldBuf, pldBufLength));

    readBuf = pldBuf;
    (void)multi_value_header(&readBuf);

    if (readBuf[0] != TAG_SEQUENCE )
    {
        if (valueIndex != 0)
        {
            sme_trace_error((TR_MIB_ACCESS, "get_encoded_buffer_at(%d) single value encode with a requested index != 0", valueIndex));
            return NULL;
        }
        /* Must be a single value encode so reset the  buffer */
        readBuf = pldBuf;
    }

    for (i = 0; i < valueIndex; ++i)
    {
        if (readBuf >= &pldBuf[pldBufLength])
        {
            sme_trace_error((TR_MIB_ACCESS, "get_encoded_buffer_at() could not find mib value at requested index"));
            return NULL;
        }
        /* move to the encoded value index */
        readBuf += readBuf[1] + 2;
    }

    if (readBuf >= &pldBuf[pldBufLength])
    {
        sme_trace_error((TR_MIB_ACCESS, "get_encoded_buffer_at() could not find mib value at requested index.. Ran off end of data"));
        return NULL;
    }

    sme_trace_hex((TR_MIB_ACCESS, TR_LVL_DEBUG, "get_encoded_buffer_at() Result", pldBuf, (CsrUint32)((&pldBuf[pldBufLength]) - readBuf) ));

    return readBuf;
}

unifi_Status mib_decode_get_CsrBool(FsmContext* context, const DataReference* dataRef, CsrUint16 valueIndex, CsrBool* value)
{
    unifi_Status result = unifi_Success;
    CsrUint8* readBuf = NULL;
    CsrInt32 mibValue;
    CsrInt32 dummy = 0;

    sme_trace_entry((TR_MIB_ACCESS, "mib_decode_get_CsrBool(%d)", valueIndex));

    readBuf = get_encoded_buffer_at(context, dataRef, valueIndex);

    if (readBuf == NULL)
    {
        return unifi_Error;
    }

    result = varbind_ber_decode_int(readBuf, &dummy, (CsrUint32*)&mibValue);

    *value = (CsrBool)mibValue % 2;

    sme_trace_debug((TR_MIB_ACCESS, "mib_decode_get_CsrBool(%d) decoded %s :: %s", valueIndex, trace_unifi_Status(result), ((*value)?"TRUE":"FALSE") ));

    return result;
}

unifi_Status mib_decode_get_int(FsmContext* context, const DataReference* dataRef, CsrUint16 valueIndex, CsrInt32* value)
{
    unifi_Status result = unifi_Success;
    CsrUint8* readBuf = NULL;
    CsrInt32 dummy = 0;

    sme_trace_entry((TR_MIB_ACCESS, "mib_decode_get_int(%d)", valueIndex));

    readBuf = get_encoded_buffer_at(context, dataRef, valueIndex);

    if (readBuf == NULL)
    {
        return unifi_Error;
    }

    result = varbind_ber_decode_int(readBuf, &dummy, (CsrUint32*)value);

    sme_trace_debug((TR_MIB_ACCESS, "mib_decode_get_int(%d) decoded %s :: %d", valueIndex, trace_unifi_Status(result), *value));

    return result;
}

unifi_Status mib_decode_get_int64(FsmContext* context, const DataReference* dataRef, CsrUint16 valueIndex, CsrInt32* mswvalue, CsrUint32* lswvalue)
{
    unifi_Status result = unifi_Success;
    CsrUint8* readBuf = NULL;

    sme_trace_entry((TR_MIB_ACCESS, "mib_decode_get_uint64(%d)", valueIndex));

    readBuf = get_encoded_buffer_at(context, dataRef, valueIndex);

    if (readBuf == NULL)
    {
        return unifi_Error;
    }
    result = varbind_ber_decode_int(readBuf, mswvalue, lswvalue);

    sme_trace_debug((TR_MIB_ACCESS, "mib_decode_get_int(%d) decoded %s :: 0x%.8X%.8X", valueIndex, trace_unifi_Status(result), *mswvalue, *lswvalue));

    return result;
}

unifi_Status mib_decode_get_os(FsmContext* context, const DataReference* dataRef, CsrUint16 valueIndex, CsrUint8* data, CsrUint16 datalength)
{
    unifi_Status result = unifi_Success;
    CsrUint8* readBuf = NULL;

    sme_trace_entry((TR_MIB_ACCESS, "mib_decode_get_os(%d)", valueIndex));

    readBuf = get_encoded_buffer_at(context, dataRef, valueIndex);
    if (readBuf == NULL)
    {
        return unifi_Error;
    }

    result = varbind_ber_decode_os(readBuf, data, datalength);

    sme_trace_debug((TR_MIB_ACCESS, "mib_decode_get_os(%d) decoded %s", valueIndex, trace_unifi_Status(result)));
    sme_trace_hex((TR_MIB_ACCESS, TR_LVL_DEBUG, "mib_decode_get_os()", data, datalength));

    return result;
}

static CsrUint8 xencode_integer(CsrUint8* buf, CsrInt32 value)
{
    CsrUint8 nt = 0;
    if(value < -128 || value > 127)
    {
        nt = xencode_integer(buf, (value/256));
    }
    buf[nt] = (CsrUint8)(value & 0xff);
    return(nt + 1);
}

unifi_Status mib_encode_add_set_int(FsmContext* context, DataReference* dataRef, mib_ids id, CsrInt32 value, CsrUint8 index1, CsrUint8 index2)
{
    unifi_Status result = unifi_Success;
    CsrUint16 length = 0;
    CsrUint8 intEncodeLength = 0;
    CsrUint8* writeBuf = NULL;
    CsrUint8* pldBuf = NULL;
    CsrUint16 pldBufLength = 0;
    CsrUint8 oidLength = mib_accessors[id].oidheader->length + mib_accessors[id].oidlength + 2;

    sme_trace_entry((TR_MIB_ACCESS, "mib_encode_add_set_int(%d, %d, %d)", id, index1, index2));

    pld_access(getPldContext(context), dataRef->slotNumber, (void**)&pldBuf, &pldBufLength);

    length = pldBuf[2] << 8 | pldBuf[3];
    writeBuf = &pldBuf[length + 4];

    if (length+4+oidLength+4 > pldBufLength)
    {
        sme_trace_crit((TR_MIB_ACCESS, "mib_encode_add_set_int() mib get buffer not big enough. Buffer Size = %d bytes. Need %d bytes.", pldBufLength, length+4+oidLength+4));
        return unifi_Error;
    }

    rebuildrequest(writeBuf, &mib_accessors[id]);
    result = insert_indices(writeBuf, index1, index2, mib_accessors[id].indices);

    /* Add Encoded Integer */
    writeBuf[oidLength - 2] = TAG_INTEGER;
    intEncodeLength = xencode_integer(&writeBuf[oidLength], value);
    writeBuf[oidLength - 1] = intEncodeLength;

    /* Update Length for OID */
    writeBuf[1] += intEncodeLength;

    /* Update the Length for the full multi set req */
    length += oidLength + intEncodeLength;
    pldBuf[2] = length >> 8;
    pldBuf[3] = length & 0xFF;

    dataRef->dataLength += oidLength + intEncodeLength;

    sme_trace_hex((TR_MIB_ACCESS, TR_LVL_DEBUG, "mib_encode_add_set_int() Encoded", writeBuf, oidLength + intEncodeLength));

    return result;
}

unifi_Status  mib_encode_add_set_boolean(FsmContext* context, DataReference* dataRef, mib_ids id, CsrBool value, CsrUint8 index1, CsrUint8 index2)
{
    /* Convert CsrBool into a MIB TruthValue. 1 = TRUE, 2 = FALSE */
    return mib_encode_add_set_int(context, dataRef, id, value?1:2, index1, index2);
}

unifi_Status mib_encode_add_set_os(FsmContext* context, DataReference* dataRef, mib_ids id, const CsrUint8* data, CsrUint8 datalength, CsrUint8 index1, CsrUint8 index2)
{
    unifi_Status result = unifi_Success;
    CsrUint16 length = 0;
    CsrUint8* writeBuf = NULL;
    CsrUint8* pldBuf = NULL;
    CsrUint16 pldBufLength = 0;
    CsrUint8 oidLength = mib_accessors[id].oidheader->length + mib_accessors[id].oidlength + 2;

    sme_trace_entry((TR_MIB_ACCESS, "mib_encode_add_set_os(%d, %d, %d)", id, index1, index2));

    pld_access(getPldContext(context), dataRef->slotNumber, (void**)&pldBuf, &pldBufLength);

    length = pldBuf[2] << 8 | pldBuf[3];
    writeBuf = &pldBuf[length + 4];

    if (length+4+oidLength+datalength > pldBufLength)
    {
        sme_trace_crit((TR_MIB_ACCESS, "mib_encode_add_set_os() mib get buffer not big enough. Buffer Size = %d bytes. Need %d bytes.", pldBufLength, length+4+oidLength+datalength));
        return unifi_Error;
    }

    rebuildrequest(writeBuf, &mib_accessors[id]);
    result = insert_indices(writeBuf, index1, index2, mib_accessors[id].indices);

    /* Add Encoded Integer */
    writeBuf[oidLength - 2] = TAG_OCTET_STRING;
    CsrMemCpy(&writeBuf[oidLength], data, datalength);
    writeBuf[oidLength - 1] = datalength;

    /* Update Length for OID */
    writeBuf[1] += datalength;

    /* Update the Length for the full multi set req */
    length += oidLength + datalength;
    pldBuf[2] = length >> 8;
    pldBuf[3] = length & 0xFF;

    dataRef->dataLength += oidLength + datalength;

    sme_trace_hex((TR_MIB_ACCESS, TR_LVL_DEBUG, "mib_encode_add_set_os() Encoded", writeBuf, oidLength + datalength));

    return result;
}


/**
 * @brief Sends a MIB Set Request verbatim to the MLME
 *
 * @par Description
 *  A varbind has been read directly out of a binary MIB configuration file
 *  and is to be parsed through transparently.
 *
 *  Format of the data should be:
 *      Byte[0]: LSB of total length of the data to follow
 *      Byte[1]: LSB of total length of the data to follow
 *      Byte[2]: <don't care>
 *      Byte[3]: Length of data to follow, not including [2] & [3]
 *
 *  This function sends everything from Byte[2] onwards in the MLME SET
 *  Request
 *
 * @param[in]     varbind: unknown varbind to be passed through transparently
 *
 */
DataReference mib_encode_file(FsmContext* context, const CsrUint8 *data, CsrUint32 length, CsrUint16 startIndex, CsrUint16* endIndex)
{
    DataReference dataRef = {0, 0};
    CsrUint8* buf;
    const CsrUint8* tempbuf = data;
    CsrUint32 i;

    sme_trace_entry((TR_MIB_ACCESS, ">> mib_encode_file()"));

    *endIndex = 0;

    /* Ignore the first "startIndex" entries */
    for (i = 0; i < startIndex; ++i) {
        tempbuf += ((tempbuf[1] << 8) | tempbuf[0]) + 2;
        (*endIndex)++;
        if (tempbuf >= &data[length])
        {
            return dataRef;
        }
    }

    pld_create(getPldContext(context), (CsrUint16)(length + 4), (void**)&buf, &dataRef.slotNumber); /*lint !e734*/

    dataRef.dataLength = 4;

    /* Multi mib value Set Header */
    buf[0] = 0x30;
    buf[1] = 0x82;

    while (tempbuf < data + length)
    {
        CsrUint16 totalLength = (tempbuf[1] << 8) | tempbuf[0];

        /* Limit the max size of the buffer sent to the FW */
        if (dataRef.dataLength + totalLength > CSR_WIFI_HOSTIO_MIB_SET_MAX)
        {
            break;
        }

        tempbuf += 2;

        /* "2" is the first two bytes containing the total length count, which
         * are _not_ included in the total length count
        */
        CsrMemCpy(&buf[dataRef.dataLength], tempbuf, totalLength);
        tempbuf += totalLength;
        dataRef.dataLength += totalLength;
        (*endIndex)++;
    }

    buf[2] = (dataRef.dataLength - 4) >> 8; /*lint !e702*/
    buf[3] = (dataRef.dataLength - 4) & 0xFF;

    sme_trace_hex((TR_MIB_ACCESS, TR_LVL_DEBUG, "mib_encode_file():", buf, dataRef.dataLength));

    sme_trace_entry((TR_MIB_ACCESS, "<< mib_encode_file()"));
    return dataRef;
}
