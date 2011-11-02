/*
 * ---------------------------------------------------------------------------
 *
 * FILE: conversions.h
 * 
 * PURPOSE:
 *      This header file provides the macros for converting to and from
 *      wire format.
 *      These macros *MUST* work for little-endian AND big-endian hosts.
 *
 * Copyright (C) 2006-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#ifndef __CONVERSIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SIZEOF_UINT16           2
#define SIZEOF_UINT32           4
#define SIZEOF_UINT64           8

#define SIZEOF_SIGNAL_HEADER    6
#define SIZEOF_DATAREF          4


/* 
 * Macro to retrieve the signal ID from a wire-format signal.
 */
#define GET_SIGNAL_ID(_buf)     COAL_GET_UINT16_FROM_LITTLE_ENDIAN((_buf))

/* 
 * Macros to retrieve and set the DATAREF fields in a packed (i.e. wire-format)
 * HIP signal.
 */
#define GET_PACKED_DATAREF_SLOT(_buf, _ref)                             \
    COAL_GET_UINT16_FROM_LITTLE_ENDIAN(((_buf) + SIZEOF_SIGNAL_HEADER + ((_ref)*SIZEOF_DATAREF) + 0))

#define GET_PACKED_DATAREF_LEN(_buf, _ref)                              \
    COAL_GET_UINT16_FROM_LITTLE_ENDIAN(((_buf) + SIZEOF_SIGNAL_HEADER + ((_ref)*SIZEOF_DATAREF) + 2))

#define SET_PACKED_DATAREF_SLOT(_buf, _ref, _slot)                      \
    COAL_COPY_UINT16_TO_LITTLE_ENDIAN((_slot), ((_buf) + SIZEOF_SIGNAL_HEADER + ((_ref)*SIZEOF_DATAREF) + 0))

#define SET_PACKED_DATAREF_LEN(_buf, _ref, _len)                        \
    COAL_COPY_UINT16_TO_LITTLE_ENDIAN((_len), ((_buf) + SIZEOF_SIGNAL_HEADER + ((_ref)*SIZEOF_DATAREF) + 2))

#define GET_PACKED_MA_UNIDATA_REQUEST_FRAME_PRIORITY(_buf)              \
    COAL_GET_UINT16_FROM_LITTLE_ENDIAN(((_buf) + SIZEOF_SIGNAL_HEADER + UNIFI_MAX_DATA_REFERENCES*SIZEOF_DATAREF + 14))

#define GET_PACKED_MA_PACKET_REQUEST_FRAME_PRIORITY(_buf)              \
    COAL_GET_UINT16_FROM_LITTLE_ENDIAN(((_buf) + SIZEOF_SIGNAL_HEADER + UNIFI_MAX_DATA_REFERENCES*SIZEOF_DATAREF + 6))

CsrInt32 get_packed_struct_size(const CsrUint8 *buf);
CsrInt32 read_unpack_signal(const CsrUint8 *ptr, CSR_SIGNAL *sig);
CsrInt32 write_pack(const CSR_SIGNAL *sig, CsrUint8 *ptr, CsrUint16 *sig_len);

#ifdef __cplusplus
}
#endif

#endif /* __CONVERSIONS_H__ */

