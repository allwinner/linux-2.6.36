/** @file ie_access.h
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
 *   $Id: //depot/dot11/v7.0p/host/lib_info_elements/ie_utils/ie_utils.h#2 $
 *
 ****************************************************************************/
#ifndef IE_UTILS_H
#define IE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup ie_access
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"
#include "csr_util.h"

/* MACROS *******************************************************************/

/** returns the IE ID index */
#define ie_id(p) ((p)[0])
/** returns the IE length index */
#define ie_len(p) ((p)[1])
/** returns the IE length index */
#define ie_total_len(p) (((p)[1])+2)
/** returns the IE info index */
#define ie_info(p) ((p) + 2)
/** returns the next IE */
#define ie_next(p) (ie_info(p) + ie_len(p))
/** valids the IE element */
#define ie_valid(p,end) (ie_info (p) <= (end) && ie_next (p) <= (end))

/** returns a IE type index */
#define ie_oui_type(p) ((p)[5])
/** returns a IE subtype index */
#define ie_oui_subtype(p) ((p)[6])


/**
 * Read a value and shift the pointer ( Byte order independant )
 */

/*lint -save -e773*/
/** read a CsrUint8 and shift by a 1 byte*/
#define ie_CsrUint8(p,value)  value = p[0]; p+=1
#define ie_CsrUint8_noshift(p,value)  value = p[0]
/** read a CsrUint16 and shift by a 2 byte*/
#define ie_le_CsrUint16(p,value) value         = (p[0]) | (p[1] << 8); p+=2
#define ie_le_CsrUint16_noshift(p,value) value = (p[0]) | (p[1] << 8)
#define ie_be_CsrUint16(p,value) value         = (p[0] << 8) | (p[1]); p+=2
#define ie_be_CsrUint16_noshift(p,value) value = (p[0] << 8) | (p[1])
/** read a 3 octets/24 bit and shift by 3 byte*/
#define ie_le_CsrUint24(p,value) value         = (p[0]) | (p[1] << 8) | (p[2] << 16); p+=3
#define ie_le_CsrUint24_noshift(p,value) value = (p[0]) | (p[1] << 8) | (p[2] << 16)
#define ie_be_CsrUint24(p,value) value         = (p[0] << 16) | (p[1] << 8) | (p[2]); p+=3
#define ie_be_CsrUint24_noshift(p,value) value = (p[0] << 16) | (p[1] << 8) | (p[2])
/** read a CsrUint32 and shift by a 4 byte*/
#define ie_le_CsrUint32(p,value) value         = (p[0]) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); p+=4
#define ie_le_CsrUint32_noshift(p,value) value = (p[0]) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24)
#define ie_be_CsrUint32(p,value) value         = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]); p+=4
#define ie_be_CsrUint32_noshift(p,value) value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3])


/** write a CsrUint8 and shift by a 1 byte*/
#define ie_write_CsrUint8(p,value) p[0] = value; p+=1
/** write a CsrUint16 and shift by a 2 byte*/
#define ie_le_write_CsrUint16(p,value) p[0] = (CsrUint8)(value)         & 0xFF; \
                                    p[1] = (CsrUint8)((value) >>  8) & 0xFF; \
                                    p+=2
#define ie_be_write_CsrUint16(p,value) p[0] = (CsrUint8)((value) >>  8) & 0xFF; \
                                    p[1] = (CsrUint8)(value)         & 0xFF; \
                                    p+=2
/** write a uint31 and shift by a 4 byte*/
#define ie_le_write_CsrUint32(p,value) p[0] = (CsrUint8)(value)         & 0xFF; \
                                    p[1] = (CsrUint8)((value) >>  8) & 0xFF; \
                                    p[2] = (CsrUint8)((value) >> 16) & 0xFF; \
                                    p[3] = (CsrUint8)((value) >> 24) & 0xFF; \
                                    p+=4

#define ie_be_write_CsrUint32(p,value) p[0] = (CsrUint8)((value) >> 24)  & 0xFF; \
                                    p[1] = (CsrUint8)((value) >> 16) & 0xFF; \
                                    p[2] = (CsrUint8)((value) >>  8) & 0xFF; \
                                    p[3] = (CsrUint8)(value)         & 0xFF; \
                                    p+=4



/** read a CsrUint8 array and shift by a 1 byte x the array size */
#define ie_CsrUint8array(p, value,size) CsrMemMove(value, p, size); p += size

/** Calculates the number of bytes left in the element from the curr position */
#define ie_bytesleft(p,curr) (ie_len(p) - ((curr - p) - 2))
/*lint -restore*/

#define ie_2_byte_array_to_csruint8(p) (CsrUint8)((p[0]) | (p[1] << 8))
#define ie_3_byte_array_to_csruint8(p) (CsrUint8)((p[0]) | (p[1] << 8) | (p[2] << 16))

/** removes a single IE from a buffer of IEs
 *  NOTE: this does not resize any buffer */

#define ie_remove_ie_from_buffer(drBuf,drBufLen,Ie) ( CsrMemMove(Ie, ie_next(Ie),  drBufLen - (((CsrUint8)(Ie-drBuf)) + (ie_len(Ie)+2)) ) )

/* TYPES DEFINITIONS ********************************************************/

/**
 * @brief defines the IE indexes
 *
 * @par Description
 *   defines the IE indexes that are used to scan thought an Informational
 *   element.
 */
typedef enum ie_elementid
{
   IE_ID_SSID                           = (int)0,
   IE_ID_SUPPORTED_RATES                = (int)1,
   IE_ID_FH_PARAMS                      = (int)2,
   IE_ID_DS_PARAMS                      = (int)3,
   IE_ID_CF_PARAMS                      = (int)4,
   IE_ID_TIM                            = (int)5,
   IE_ID_IBSS                           = (int)6,
   IE_ID_COUNTRY                        = (int)7,
   IE_ID_HOPPING_PATTERN_PARAMS         = (int)8,
   IE_ID_HOPPING_PATTERN_TABLE          = (int)9,
   IE_ID_REQUEST                        = (int)10,
   IE_ID_QBSS_LOAD                      = (int)11,
   IE_ID_EDCA_PARAMS                    = (int)12,
   IE_ID_TRAFFIC_SPEC                   = (int)13,
   IE_ID_TRAFFIC_CLAS                   = (int)14,
   IE_ID_SCHEDULE                       = (int)15,
   IE_ID_CHALLENGE_TEXT                 = (int)16,
   IE_ID_POWER_CONSTRAINT               = (int)32,
   IE_ID_POWER_CAPABILITY               = (int)33,
   IE_ID_TPC_REQUEST                    = (int)34,
   IE_ID_TPC_REPORT                     = (int)35,
   IE_ID_SUPPORTED_CHANNELS             = (int)36,
   IE_ID_CHANNEL_SWITCH_ANNOUNCEMENT    = (int)37,
   IE_ID_MEASUREMENT_REQUEST            = (int)38,
   IE_ID_MEASUREMENT_REPORT             = (int)39,
   IE_ID_QUIET                          = (int)40,
   IE_ID_IBSS_DFS                       = (int)41,
   IE_ID_ERP_INFO                       = (int)42,
   IE_ID_TS_DELAY                       = (int)43,
   IE_ID_TCLAS                          = (int)44,
   IE_ID_HT_CAPABILITIES                = (int)45,
   IE_ID_QOS_CAPABILITY                 = (int)46,
   IE_ID_RSN                            = (int)48,
   IE_ID_EXTENDED_SUPPORTED_RATES       = (int)50,
   IE_ID_HT_INFORMATION                 = (int)61,
   IE_ID_WAPI                           = (int)68,
   IE_ID_VENDOR_SPECIFIC                = (int)221
} ie_elementid;

#define IE_ID_CCX_NEIGHBOUR            0x28
#define IE_ID_CCX_NGHBR_SUB_IE_RF      0x01
#define IE_ID_CCX_NGHBR_SUB_IE_TSF     0x02
#define IE_ID_CCX_NGHBR_SUB_IE_ACE     0x03

typedef enum ie_result
{
    ie_success,
    ie_error,
    ie_invalid,
    ie_not_found
} ie_result;

#define OUI_SIZE             4
#define OUISUBTYPE_SIZE      1

extern const CsrUint8  ieWmmInformationOui[OUI_SIZE];
extern const CsrUint8  ieWmmInformationOuiSubtype[OUISUBTYPE_SIZE];
extern const CsrUint8  ieWmmParameterOui[OUI_SIZE];
extern const CsrUint8  ieWmmParameterOuiSubtype[OUISUBTYPE_SIZE];
extern const CsrUint8  ieWmmTspecOui[OUI_SIZE];
extern const CsrUint8  ieWmmTspecOuiSubtype[OUISUBTYPE_SIZE];
extern const CsrUint8  ieWpaOui[OUI_SIZE];
extern const CsrUint8  ieWpaOuiSubtype[OUISUBTYPE_SIZE];
extern const CsrUint8  ieWpsOui[OUI_SIZE];
extern const CsrUint8  ieWpsOuiSubtype[OUISUBTYPE_SIZE];

extern const CsrUint8  ieCcxTsMetricsOui[OUI_SIZE];
extern const CsrUint8  ieCcxTsMetricsOuiSubtype[OUISUBTYPE_SIZE];
extern const CsrUint8  ieCcxSsidlOui[OUI_SIZE];
extern const CsrUint8  ieCcxSsidlOuiSubtype[OUISUBTYPE_SIZE];
extern const CsrUint8  ieCcxMbssidOui[OUI_SIZE];


/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Finds the Information element in the buffer.
 *
 * @par Description
 * Finds the Information element in the buffer
 *   See Doc P802.11-REVma/D7.0 -> 7.3.2
 *
 * @param[in] id : Id if the element to find.
 * @param[in] elements : Pointer to the IE buffer.
 * @param[in] length : Length of the IE buffer.
 * @param[out] result : pointer to the output buffer.
 *
 * @return
 *        ie_success   : element found
 *        ie_invalid   : IE field length invalid
 *        ie_not_found : IE element not found
 */
extern ie_result ie_find(
                    ie_elementid id,
                    CsrUint8* elements,
                    CsrUint32 length,
                    CsrUint8** result);

/**
 * @brief Finds the vendor_specific element in the buffer.
 *
 * @par Description
 * Finds the vendor_specific element in the buffer.
 *   See Doc P802.11-REVma/D7.0 -> 7.3.2
 *
 * @param[in] oui : Id if the element to find.
 * @param[in] extra : Extra match data after the OUI
 * @param[in] extralength : Length Extra match data.
 * @param[in] elements : Pointer to the IE buffer.
 * @param[in] length : Length of the IE buffer.
 * @param[out] result : pointer to the output buffer.
 *
 * @return
 *        ie_success   : element found
 *        ie_invalid   : IE field length invalid
 *        ie_not_found : IE element not found
 */
extern ie_result ie_find_vendor_specific(
                    const CsrUint8* oui,
                    const CsrUint8* extra,
                    const CsrUint32 extralength,
                    CsrUint8* elements,
                    CsrUint32 length,
                    CsrUint8** result);

/**
 * @brief Validates an IE buffer.
 *
 * @par Description
 * Validates an IE buffer
 *   See Doc P802.11-REVma/D7.0 -> 7.3.2
 *
 * @param[in] elements : Pointer to the IE buffer.
 * @param[in] length : Length of the IE buffer.
 *
 * @return
 *        ie_success   : element found
 *        ie_invalid   : IE field length invalid
 */
extern ie_result ie_validate_ie_buffer(
                    CsrUint8* elements,
                    CsrUint32 length);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* IE_UTILS_H */
