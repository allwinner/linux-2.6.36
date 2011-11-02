/** @file csr_security_private.h
 *
 * Private Definitions for Security library.
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
 *   This provides an implementation of Security library
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_security_private.h#2 $
 *
 ****************************************************************************/
#ifndef CSR_SECURITY_PRIVATE_H
#define CSR_SECURITY_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif


/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"
#include "csr_util.h"
#include "csr_cstl/csr_wifi_list.h"
#include "csr_security.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PUBLIC TYPE DEFINITIONS ************************************************/


/* Pointer/length structure to replace DataReference */
typedef struct DataBuffer
{
    CsrUint16 dataLength;   /* Length of data contained in buf */
    CsrUint8 *buf;
}DataBuffer;

/* EAP NAK packet structure. */
/* --------------------------------------------------------------------- */
typedef struct eap_nak
{
    CsrUint8 recommended;
} eap_nak;

/* EAP requests and response packet structure and definitions. */
/* --------------------------------------------------------------------- */
typedef struct eap_req_resp
{
    CsrUint8 type;
    union
    {
        eap_nak nak;
        CsrUint8 data;    /* variable length data */
    }u;
} eap_req_resp;

#define EAP_MAX_FRAGMENTS       10      /* Maximum number of fragments */
#define EAP_MAX_FRAGMENT_SIZE   1500    /* Max fragment size including header */

/* EAP REQ/RESP type field */
#define EAP_TYPE_NONE           0
#define EAP_TYPE_IDENTITY       1
#define EAP_TYPE_NOTIFICATION   2
#define EAP_TYPE_NAK            3
#define EAP_TYPE_MD5            4
#define EAP_TYPE_OTP            5
#define EAP_TYPE_GTC            6
#define EAP_TYPE_TLS           13
#define EAP_TYPE_LEAP          17
#define EAP_TYPE_SIM           18
#define EAP_TYPE_TTLS          21
#define EAP_TYPE_PEAP          25
#define EAP_TYPE_AKA           23
#define EAP_TYPE_MSCHAPV2      26
#define EAP_TYPE_EAPFAST       43

/* EAP packet structure and definitions. */
/* --------------------------------------------------------------------- */
typedef struct eap_packet
{
    CsrUint8 code;
    CsrUint8 id;
    CsrUint8 eap_length_hi;
    CsrUint8 eap_length_low;
    union /* optional. Not present for success/fail types */
    {
        eap_req_resp req;
        eap_req_resp resp;
    }u;
} eap_packet;

/* 5 = eap_packet header(4) + req/resp type field(1) */
#define EAP_REQ_RESP_HEADER_LENGTH (5)

/* Endian-protected access to length field. */
#define EAP_LENGTH_FIELD(a) ((CsrUint16)(((a).eap_length_hi<<8) + ((a).eap_length_low)))
#define EAP_LENGTH_ASSIGN(a, val) {(a).eap_length_hi = (val)>>8; (a).eap_length_low = (val) & 0x00ff;}

/* EAP CODE FIELD */
#define EAP_CODE_REQ     1
#define EAP_CODE_RESP    2
#define EAP_CODE_SUCCESS 3
#define EAP_CODE_FAILURE 4


/* EAPOL-KEY packet structure and definitions. */
/* --------------------------------------------------------------------- */
typedef struct eapol_key_leap
{
    CsrUint8 type;
    CsrUint8 key_length_hi;        /* length in bytes */
    CsrUint8 key_length_low;
    CsrUint8 replay_counter[8];
    CsrUint8 iv[16];
    CsrUint8 flag_and_index;
    CsrUint8 signature[16];
    CsrUint8 keydata; /* variable length data */
} eapol_key_leap;

typedef struct eapol_key
{
    CsrUint8 type;
    CsrUint8 key_info_hi;
    CsrUint8 key_info_low;
    CsrUint8 key_length_hi;        /* length in bytes */
    CsrUint8 key_length_low;
    CsrUint8 replay_counter[8];
    CsrUint8 nonce[32];
    CsrUint8 iv[16];
    CsrUint8 rsc[8];
    CsrUint8 reserved[8];
    CsrUint8 mic[16];
    CsrUint8 key_data_length_hi;
    CsrUint8 key_data_length_low;
    CsrUint8 key_data; /* variable length data */
} eapol_key;


/* EAPOL packet structure (inc. LLC header) and definitions */
/* --------------------------------------------------------------------- */

typedef struct llc
{
     CsrUint8 dsap; /* always 0xaa */
     CsrUint8 ssap; /* always 0xaa */
     CsrUint8 ctrl; /* always 0x03 */
     CsrUint8 oui[3];
     CsrUint8 protocol_hi; /* EAPOL code = 0x888e, WAPI code = 0x88b4 */
     CsrUint8 protocol_low;
} llc;

#define LLC_PROTOCOL_FIELD(a) ((CsrUint16)(((a)->protocol_hi<<8) + ((a)->protocol_low)))

typedef struct eapol_packet
{
    /* LLC SNAP header (length defined below) */
    llc   llc_header;
     /* EAPOL header */
    CsrUint8 version;      /* must be 1 */
    CsrUint8 packet_type;
    CsrUint8 eapol_length_hi;
    CsrUint8 eapol_length_low;

    union
    {
        eap_packet eap;
        eapol_key  key;
        eapol_key_leap  key_leap;
    }u;
} eapol_packet;

#define EAPOL_LENGTH_FIELD(a) ((CsrUint16)(((a)->eapol_length_hi<<8) + ((a)->eapol_length_low)))
#define EAPOL_LENGTH_ASSIGN(a, val) {(a)->eapol_length_hi = (((CsrUint16)(val)) >> 8 & 0xff); (a)->eapol_length_low = (val) & 0xff;}

/* Endian-protected access to EAPOL KEY Packet */
#define EAPOL_KEY_INFO_FIELD(a) ((CsrUint16)(((a)->u.key.key_info_hi<<8) + ((a)->u.key.key_info_low)))
#define EAPOL_KEY_INFO_ASSIGN(a, val) {(a)->u.key.key_info_hi = (val & 0xff00)>>8; (a)->u.key.key_info_low = (val) & 0x00ff;}
#define EAPOL_KEY_LENGTH_FIELD(a) ((CsrUint16)(((a)->u.key.key_length_hi<<8) + ((a)->u.key.key_length_low)))
#define EAPOL_KEY_LENGTH_ASSIGN(a, val) {(a)->u.key.key_length_hi = (val & 0xff00)>>8; (a)->u.key.key_length_low = (val) & 0x00ff;}
#define EAPOL_KEY_DATA_LENGTH_FIELD(a) ((CsrUint16)(((a)->u.key.key_data_length_hi<<8) + ((a)->u.key.key_data_length_low)))
#define EAPOL_KEY_DATA_LENGTH_ASSIGN(a, val) {(a)->u.key.key_data_length_hi = (val & 0xff00)>>8; (a)->u.key.key_data_length_low = (val) & 0x00ff;}
/* Complete length of the EAPOL Key packet including LLC header */
#define EAPOL_KEY_PACKET_LENGTH(a) ((&((a)->u.key.key_data) - &((a)->llc_header.dsap)) + EAPOL_KEY_DATA_LENGTH_FIELD((a)))

/* Endian-protected access to EAPOL KEY CCX Packet */
#define KEY_CCX_LENGTH_FIELD(a) ((CsrUint16)(((a)->key_length_hi<<8) + ((a)->key_length_low)))

#define EAPOL_HEADER_LENGTH (4)

/* Definitions for the type field of the eapol_packet. */
#define EAPOL_PACKET_TYPE_EAP          0
#define EAPOL_PACKET_TYPE_EAPOL_START  1
#define EAPOL_PACKET_TYPE_EAPOL_LOGOFF 2
#define EAPOL_PACKET_TYPE_EAPOL_KEY    3
#define EAPOL_PACKET_TYPE_EAPOL_ALERT  4

/* WAPI Packet Definitions */
/* ------------------------------------------------------------------------------- */
typedef struct wapi_unicast_key_negotiation_header
{
    CsrUint8 flag;
    CsrUint8 bkid[16];
    CsrUint8 uskid;
    CsrUint8 addid[12];
    CsrUint8 var;          /* Variable data depending on the sub type */
} wapi_unicast_key_negotiation_header;

typedef struct wapi_multicast_key_negotiation_header
{
    CsrUint8 flag;
    CsrUint8 mskid;
    CsrUint8 uskid;
    CsrUint8 addid[12];
    CsrUint8 var;          /* Variable data depending on the sub type */
} wapi_multicast_key_negotiation_header;

typedef struct wapi_packet
{
    /* LLC SNAP header (length defined below) */
    llc   llc_header;

    /* WAPI header */
    CsrUint8 version_hi;      /* must be 0 */
    CsrUint8 version_low;     /* must be 1 */
    CsrUint8 type;
    CsrUint8 subType;
    CsrUint8 reserved_hi;
    CsrUint8 reserved_low;
    CsrUint8 length_hi;
    CsrUint8 length_low;
    CsrUint8 packet_seq_hi;
    CsrUint8 packet_seq_low;
    CsrUint8 fragment_seq;
    CsrUint8 flag;

    /* Data field */
    union
    {
        wapi_unicast_key_negotiation_header   unicast_key_negotiation_header;
        wapi_multicast_key_negotiation_header multicast_key_negotiation_header;
    }u;
} wapi_packet;

/* Should these be here?? */
#define WAPI_BIT(flag, bit)                          (((0x1 << (bit)) & (flag)) >> (bit))
/* Get 'num' number of higher bits starting from position 'bit' from 'flag' */
#define WAPI_BITS(flag, bit, num)                    (((((1 << (num)) - 1) << (bit)) & (flag)) >> (bit))

/* Flags in the data part of the WAPI packet */
#define WAPI_FLAG_BKREKEY(flag)                      (WAPI_BIT(flag, 0))
#define WAPI_FLAG_PREAUTH(flag)                      (WAPI_BIT(flag, 1))
#define WAPI_FLAG_CERT_AUTH_REQ(flag)                (WAPI_BIT(flag, 2))
#define WAPI_FLAG_OPTIONAL(flag)                     (WAPI_BIT(flag, 3))
#define WAPI_FLAG_USKREKEY(flag)                     (WAPI_BIT(flag, 4))
#define WAPI_FLAG_STAKEY_NEG(flag)                   (WAPI_BIT(flag, 5))
#define WAPI_FLAG_STAKEY_REVOK(flag)                 (WAPI_BIT(flag, 6))

#define WAPI_FLAG_BKREKEY_SET(flag, val)             {*(flag) = ((*(flag) & 0xfe) | (val << 0));}
#define WAPI_FLAG_PREAUTH_SET(flag, val)             {*(flag) = ((*(flag) & 0xfd) | (val << 1));}
#define WAPI_FLAG_CERT_AUTH_REQ_SET(flag, val)       {*(flag) = ((*(flag) & 0xfb) | (val << 2));}
#define WAPI_FLAG_OPTIONAL_SET(flag, val)            {*(flag) = ((*(flag) & 0xf7) | (val << 3));}
#define WAPI_FLAG_USKREKEY_SET(flag, val)            {*(flag) = ((*(flag) & 0xef) | (val << 4));}
#define WAPI_FLAG_STAKEY_NEG_SET(flag, val)          {*(flag) = ((*(flag) & 0xdf) | (val << 5));}
#define WAPI_FLAG_STAKEY_REVOK_SET(flag, val)        {*(flag) = ((*(flag) & 0xbf) | (val << 6));}

#define WAPI_FLAG_MORE_FRAG(flag)                    (WAPI_BIT(flag, 0))
#define WAPI_FLAG_MORE_FRAG_SET(flag, val)           (((flag) & 0xfe) | (val))

#define WAPI_PACKET_LENGTH(a) ((CsrUint16)(((a)->length_hi<<8) + ((a)->length_low)))
#define WAPI_PACKET_LENGTH_ASSIGN(a, val) {(a)->length_hi = ((val) & 0xff00)>>8; (a)->length_low = (val) & 0x00ff;}

#define WAPI_PACKET_VERSION(a) ((CsrUint16)(((a)->version_hi<<8) + ((a)->version_low)))
#define WAPI_PACKET_VERSION_ASSIGN(a, val) {(a)->version_hi = ((val) & 0xff00)>>8; (a)->version_low = (val) & 0x00ff;}

#define WAPI_PACKET_SEQ_FIELD(a) ((CsrUint16)(((a)->packet_seq_hi<<8) + ((a)->packet_seq_low)))
#define WAPI_PACKET_SEQ_ASSIGN(a, val) {(a)->packet_seq_hi = ((val) & 0xff00)>>8; (a)->packet_seq_low = (val) & 0x00ff;}

#define WAPI_LENGTH_FIELD(a) ((CsrUint16)(((*(CsrUint8*)(a))<<8) + (*((CsrUint8*)(a) + 1))))
#define WAPI_LENGTH_ASSIGN(a, val) {*(CsrUint8*)(a) = (CsrUint8)(((val) & 0xff00)>>8); *((CsrUint8*)(a) + 1) = (CsrUint8)((val) & 0x00ff);}
#define WAPI_IDENTIFIER_FIELD(a) ((CsrUint16)(((*(CsrUint8*)(a))<<8) + (*((CsrUint8*)(a) + 1))))
#define WAPI_IDENTIFIER_ASSIGN(a, val) {*(CsrUint8*)(a) = (CsrUint8)(((val) & 0xff00)>>8); *((CsrUint8*)(a) + 1) = (CsrUint8)((val) & 0x00ff);}

/* ------------------------------------------------------------------------------------ */

/* Forward declare implementation context types */

#if defined (CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE) || defined(CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE)
typedef struct CsrWpaPersonalCtx CsrWpaPersonalCtx;
#endif

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
typedef struct CsrWapiContext        CsrWapiContext;
#endif

#ifdef CSR_WIFI_SECURITY_EAP_ENABLE
typedef struct EapContext EapContext;
#endif

#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
typedef struct CsrWpsContext CsrWpsContext;
#endif

typedef struct packetNode {
    CsrUint32 appCookie;
    CsrUint16 packetLength;
    CsrUint8 *packet;
} packetNode;

struct CsrWifiSecurityContext
{
    void*                       externalContext;
    struct CsrCryptoContext*    cryptoContext;

    CsrWifiSecuritycallbacks    callbacks;
    CsrWifiSecuritySetup        setupData;

#if defined (CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE) || defined(CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE)
    CsrWpaPersonalCtx           *wpaContext;
#endif

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    CsrWapiContext              *wapiContext;
#endif

#ifdef CSR_WIFI_SECURITY_EAP_ENABLE
    EapContext                  *eapContext;
#endif

#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
    CsrWpsContext               *wpsContext;
#endif

    CsrUint8 retransmissionCount; /* count for retransmissions */

    /* Temporary buffer for building messages */
    CsrUint8                    buffer[8192];
    CsrUint32                   bufferLength;

    /* List to hold incoming messages temporarily until the processing of previous message is complete */
    csr_list                    inputQueue;
    CsrUint8                    processingInProgress;

    CsrBool                     startTimerOn;
};

/* PUBLIC CONSTANT DEFINITIONS *********************************************/
/* PUBLIC FUNCTION PROTOTYPES ***********************************************/
void CsrWifiSecurityPacketProcessingDone(CsrWifiSecurityContext* context);
void CsrWifiSecurityRandom(CsrUint8* buff, CsrUint32 length);
CsrUint8 convertSecurityTypeToEapID(CsrWifiSecurityType securityType);
void CsrWifiSecurityStorePmk(CsrWifiSecurityContext* context, CsrUint8 *pmk);

#ifdef __cplusplus
}
#endif

#endif /* CSR_SECURITY_PRIVATE_H */
