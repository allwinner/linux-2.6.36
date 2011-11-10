/** @file eap_sim.h
 *
 * Definitions for a EAP-SIM method.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   This provides an implementation of a EAP-SIM authentication method.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/sim/eap_sim.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_EAPSIM_H
#define SECURITY_METHOD_EAPSIM_H

#include "plugins/security_method.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Optional Features */
#define EAP_SIM_STA_SUPPORTS_FAST_REAUTH   1
#define EAP_SIM_STA_SUPPORTS_ANONYMITY     1
#define EAP_SIM_STA_SUPPORTS_RESULTS_IND   0
#define EAP_SIM_STA_SUPPORTS_RAND_HISTORY  0

/* EAP-SIM Current Version */
#define EAP_SIM_VERSION 1

/* EAP-SIM Packet Subtypes */
#define EAP_SIM_SUBTYPE_START            10
#define EAP_SIM_SUBTYPE_CHALLENGE        11
#define EAP_SIM_SUBTYPE_NOTIFICATION     12
#define EAP_SIM_SUBTYPE_REAUTHENTICATION 13
#define EAP_SIM_SUBTYPE_CLIENT_ERROR     14

/* EAP-SIM Attribute IDs */
#define EAP_SIM_ATT_SKIP_THRESHOLD   127
#define EAP_SIM_AT_RAND                1
#define EAP_SIM_AT_PADDING             6
#define EAP_SIM_AT_NONCE_MT            7
#define EAP_SIM_AT_PERMANENT_ID_REQ   10
#define EAP_SIM_AT_MAC                11
#define EAP_SIM_AT_NOTIFICATION       12
#define EAP_SIM_AT_ANY_ID_REQ         13
#define EAP_SIM_AT_IDENTITY           14
#define EAP_SIM_AT_VERSION_LIST       15
#define EAP_SIM_AT_SELECTED_VERSION   16
#define EAP_SIM_AT_FULLAUTH_ID_REQ    17
#define EAP_SIM_AT_COUNTER            19
#define EAP_SIM_AT_COUNTER_TOO_SMALL  20
#define EAP_SIM_AT_NONCE_S            21
#define EAP_SIM_AT_CLIENT_ERROR_CODE  22
#define EAP_SIM_AT_IV                129
#define EAP_SIM_AT_ENCR_DATA         130
#define EAP_SIM_AT_NEXT_PSEUDONYM    132
#define EAP_SIM_AT_NEXT_REAUTH_ID    133
#define EAP_SIM_AT_RESULT_IND        135

/* Min and Max Number of Challenges */
#define EAP_SIM_MIN_NUM_CHALLENGES 2
#define EAP_SIM_MAX_NUM_CHALLENGES 3

/* MAX number of previous RANDs (to avoid their re-use) */
#define MAX_NUM_OF_RANDS_IN_HISTORY   20

/* GSM Random Challenge Length */
#define GSM_RAND_LEN 16

/* Lengths of various Fields in EAP-SIM packet */
#define IE_EAP_SIM__CODE_FIELD_LEN     1
#define IE_EAP_SIM__ID_FIELD_LEN       1
#define IE_EAP_SIM__LENTGH_FIELD_LEN   2
#define IE_EAP_SIM__TYPE_FIELD_LEN     1
#define IE_EAP_SIM__SUBTYPE_FIELD_LEN  1
#define IE_EAP_SIM__RESERVED_FIELD_LEN 2

/* Lengths for Keys */
#define EAP_SIM_KC_LEN                            8  /* GSM Ciphering Key */
#define EAP_SIM_SRES_LEN                          4
#define EAP_SIM_MASTER_KEY_LEN                   20  /* Master Key (160 bits) */
#define EAP_SIM_K_ENCR_KEY_LEN                   16  /* k_encr (128 bits) */
#define EAP_SIM_K_AUT_KEY_LEN                    16  /* k_aut (128 bits) */
#define EAP_SIM_MASTER_SESSION_KEY_LEN           64  /* Master Session Key (512 bits) */
#define EAP_SIM_EXTENDED_MASTER_SESSION_KEY_LEN  64  /* Extended Master Session Key (512 bits) */

/* Max number of EAP-SIM_Start Rounds */
#define EAP_SIM_MAX_START_ROUNDS 3

#define IE_EAP__HEADER_LEN (IE_EAP_SIM__CODE_FIELD_LEN \
                + IE_EAP_SIM__ID_FIELD_LEN \
                + IE_EAP_SIM__LENTGH_FIELD_LEN \
                + IE_EAP_SIM__TYPE_FIELD_LEN)

#define IE_EAP_SIM__HEADER_LEN (IE_EAP_SIM__SUBTYPE_FIELD_LEN + IE_EAP_SIM__RESERVED_FIELD_LEN)

#define IE_EAP_SIM_PACKET__HEADER_LEN  (IE_EAP__HEADER_LEN + IE_EAP_SIM__HEADER_LEN)

/* Offsets of various Fields in EAP-SIM packet */
#define IE_EAP_SIM__CODE_FIELD_OFFSET      0
#define IE_EAP_SIM__ID_FIELD_OFFSET        (IE_EAP_SIM__CODE_FIELD_OFFSET    + IE_EAP_SIM__CODE_FIELD_LEN)
#define IE_EAP_SIM__LENTGH_FIELD_OFFSET    (IE_EAP_SIM__ID_FIELD_OFFSET      + IE_EAP_SIM__ID_FIELD_LEN)
#define IE_EAP_SIM__TYPE_FIELD_OFFSET      (IE_EAP_SIM__LENTGH_FIELD_OFFSET  + IE_EAP_SIM__LENTGH_FIELD_LEN)
#define IE_EAP_SIM__SUBTYPE_FIELD_OFFSET   (IE_EAP_SIM__TYPE_FIELD_OFFSET    + IE_EAP_SIM__TYPE_FIELD_LEN)
#define IE_EAP_SIM__RESERVED_FIELD_OFFSET  (IE_EAP_SIM__SUBTYPE_FIELD_OFFSET + IE_EAP_SIM__SUBTYPE_FIELD_LEN)

/* Lengths of various Attributes in EAP-SIM packet */
/* All EAP-SIM Attributes (except AT_PADDING) have a 4-byte header (Type: 1-byte, Length: 1-byte, Reserved: 2-byte)
 * AT_PADDING header has only the type and Length fields (2 bytes)
 */
#define IE_EAP_SIM__ATT_GEN_HEADER_LEN 4   /* SHOULDN'T BE 2 (only TL, as reserved bits are part of V) */
#define EAP_SIM__AT_PAD_HDR_LEN       2
#define EAP_SIM__AT_ENCR_DATA_HDR_LEN 2

/* Lengths  (items for Attributes) */
#define EAP_SIM_SEL_VERSION_LEN   2
#define EAP_SIM_NONCE_S_LEN      16
#define EAP_SIM_NONCE_MT_LEN     16
#define EAP_SIM_MAC_LEN          16
#define EAP_SIM_IV_LEN           16

/* Length of Attributes */
#define IE_EAP_SIM__AT_VERSION_LIST_MIN_LEN     8 /* includes only one version and padding */
#define IE_EAP_SIM__AT_PERMANENT_ID_REQ_LEN     4
#define IE_EAP_SIM__AT_FULLAUTH_ID_REQ_LEN      4
#define IE_EAP_SIM__AT_ANY_ID_REQ_LEN           4
#define IE_EAP_SIM__AT_IV_LEN                   (EAP_SIM_IV_LEN + 4) /*20*/
#define IE_EAP_SIM__AT_RESULT_IND_LEN           4
#define IE_EAP_SIM__AT_NOTIFICATION_LEN         4
#define IE_EAP_SIM__AT_MAC_LEN                  (EAP_SIM_MAC_LEN + 4) /*20*/
#define IE_EAP_SIM__AT_SELECTED_VERSION_LEN     (EAP_SIM_SEL_VERSION_LEN + 2)  /*4*/
#define IE_EAP_SIM__AT_NONCE_MT_LEN             (EAP_SIM_NONCE_MT_LEN + 4) /*20 */
#define IE_EAP_SIM__AT_NONCE_MS_LEN             (EAP_SIM_NONCE_S_LEN + 4) /*20*/
#define IE_EAP_SIM__AT_COUNTER_LEN              4
#define IE_EAP_SIM__AT_COUNTER_TOO_SMALL_LEN    4
#define IE_EAP_SIM__AT_CLIENT_ERROR_CODE_LEN    4

/* Length Field Value (Total Length expressed in Multiples of four bytes */
#define IE_EAP_SIM__AT_PERMANENT_ID_REQ_LEN_FIELD    (IE_EAP_SIM__AT_PERMANENT_ID_REQ_LEN/4)
#define IE_EAP_SIM__AT_FULLAUTH_ID_REQ_LEN_FIELD     (IE_EAP_SIM__AT_FULLAUTH_ID_REQ_LEN/4)
#define IE_EAP_SIM__AT_ANY_ID_REQ_LEN_FIELD          (IE_EAP_SIM__AT_ANY_ID_REQ_LEN/4)
#define IE_EAP_SIM__AT_IV_LEN_FIELD                  (IE_EAP_SIM__AT_IV_LEN/4)
#define IE_EAP_SIM__AT_RESULT_IND_LEN_FIELD          (IE_EAP_SIM__AT_RESULT_IND_LEN/4)
#define IE_EAP_SIM__AT_NOTIFICATION_LEN_FIELD        (IE_EAP_SIM__AT_NOTIFICATION_LEN/4)
#define IE_EAP_SIM__AT_MAC_LEN_FIELD                 (IE_EAP_SIM__AT_MAC_LEN/4)
#define IE_EAP_SIM__AT_SELECTED_VERSION_LEN_FIELD    (IE_EAP_SIM__AT_SELECTED_VERSION_LEN/4)
#define IE_EAP_SIM__AT_NONCE_MT_LEN_FIELD            (IE_EAP_SIM__AT_NONCE_MT_LEN/4)
#define IE_EAP_SIM__AT_NONCE_MS_LEN_FIELD            (IE_EAP_SIM__AT_NONCE_MS_LEN/4)
#define IE_EAP_SIM__AT_COUNTER_LEN_FIELD             (IE_EAP_SIM__AT_COUNTER_LEN/4)
#define IE_EAP_SIM__AT_COUNTER_TOO_SMALL_LEN_FIELD   (IE_EAP_SIM__AT_COUNTER_TOO_SMALL_LEN/4)
#define IE_EAP_SIM__AT_CLIENT_ERROR_CODE_LEN_FIELD   (IE_EAP_SIM__AT_CLIENT_ERROR_CODE_LEN/4)

/* Offset of  Attributes in EAP-SIM packet */
#define IE_EAP_SIM__FIRST_ATTRIBUTE_OFFSET (IE_EAP_SIM__CODE_FIELD_OFFSET + IE_EAP_SIM_PACKET__HEADER_LEN)

/*
 *  *** Constant for PRF (FIPS 186-2) Algorithm
 */
#define EAP_SIM_PRF_OUTPUT_xj_SIZE            40  /* or 320 bits */
#define EAP_SIM_PRF_CONST_2POW160             "010000000000000000000000000000000000000000"
#define EAP_SIM_PRF_RESULT_BUF_K_ENCR_OFFSET  0
#define EAP_SIM_PRF_RESULT_BUF_K_AUT_OFFSET   EAP_SIM_K_ENCR_KEY_LEN
#define EAP_SIM_PRF_RESULT_BUF_MSK_OFFSET     (EAP_SIM_K_ENCR_KEY_LEN \
                                                  + EAP_SIM_K_AUT_KEY_LEN)
#define EAP_SIM_PRF_RESULT_BUF_EMSK_OFFSET    (EAP_SIM_K_ENCR_KEY_LEN \
                                                  + EAP_SIM_K_AUT_KEY_LEN \
                                                  + EAP_SIM_MASTER_SESSION_KEY_LEN)
#define EAP_SIM_PRF_RESULT_BUF_LEN            (EAP_SIM_K_ENCR_KEY_LEN \
                                                  + EAP_SIM_K_AUT_KEY_LEN \
                                                  + EAP_SIM_MASTER_SESSION_KEY_LEN \
                                                  + EAP_SIM_EXTENDED_MASTER_SESSION_KEY_LEN)

#define EAP_SIM_PRF_RESULT_BUF_MSK_REAUTH_OFFSET     0
#define EAP_SIM_PRF_RESULT_BUF_EMSK_REAUTH_OFFSET    EAP_SIM_MASTER_SESSION_KEY_LEN

/* Notification Code Values for AT_NOTIFICATION Attribute */
#define EAP_SIM_SUCCESS                     32768

#define EAP_SIM_PBIT_MASK    0x4000

/* TYPE DEFS ***************************************************************/

/*
 * Structure for Optional Features
 * Each Feature could either be ON or OFF
 */
typedef struct CsrSimConfig
{
    /* Fast Reauthentication */
    CsrBool fastReauthOn;

    /* Anonymity */
    CsrBool anonymityOn;

    /* Protected Success Indications */
    CsrBool protectedResultIndConfigured;

    /* RAND History (to avoid previous RANDs being re-used again) */
    CsrBool randHistoryOn;

} CsrSimConfig;

/* Error Codes for AT_CLIENT_ERROR_CODE attribute (as defined in rfc-4186) */
typedef enum eEapSimErrorCodes
{
    eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET=0,
    eapSIM_ERROR_UNSUPPORTED_VERSION=1,
    eapSIM_ERROR_INSUFFICIENT_NUM_OF_CHAL=2,
    eapSIM_ERROR_RAND_NOT_FRESH=3
} eEapSimErrorCodes;

typedef enum eEAP_SIM_RESULTS
{
    eapSIM_FAILURE=0,
    eapSIM_SUCCESS=1
} eEAP_SIM_RESULTS;

typedef enum eEAP_SIM_ATT_RESULTS
{
    eapSIM_AT_VALID=0,
    eapSIM_AT_INVALID=1,
    eapSIM_AT_NOT_FOUND=2,
    eapSIM_AT_MULTIPLE=3
} eEAP_SIM_ATT_RESULTS;

typedef enum eEAP_SIM_RAND_RESULTS
{
    eapSIM_RAND_VALID=0,
    eapSIM_RAND_INVALID=1,
    eapSIM_RAND_INSUFFICIENT=2,
    eapSIM_RAND_NOT_FRESH=3,
    eapSIM_RAND_NOT_FOUND=4
} eEAP_SIM_RAND_RESULTS;

typedef enum eID_TYPE_NETWORK_REQ
{
    NO_TYPE,
    ANY_TYPE,
    FULLAUTH_TYPE,
    PERMANENT_TYPE
} eID_TYPE_NETWORK_REQ;

typedef enum eID_TO_SEND
{
    SEND_NO_ID=0,  /* must have lowest value */
    SEND_FAST_REAUTH_ID=1,
    SEND_PSEUDONYM=2,
    SEND_PERMANENT_ID=3
} eID_TO_SEND;

typedef enum LAST_RX_EAP_SIM_PACKET
{
    EAP_SIM_NO_PACKET_RECEIVED,
    EAP_SIM_START_REQ,
    EAP_SIM_CHALLENGE_REQ,
    EAP_SIM_REAUTHENTICATION_REQ,
    EAP_SIM_NOTIFICATION_REQ,
    EAP_SIM_INVALID_REQ
} LAST_RX_EAP_SIM_PACKET;

typedef struct GSM_TRIPLET
{
    CsrUint8 RAND[GSM_RAND_LEN];
    CsrUint8 Kc[EAP_SIM_KC_LEN];
    CsrUint8 SRES[EAP_SIM_SRES_LEN];
} GSM_TRIPLET;

typedef struct CsrSimData
{
    CsrUint8 numEapSimStartRounds;
    CsrUint8 numNotificationRounds;
    CsrUint8 numFastReAuthRounds;
    CsrUint8 numIdReqs;

    LAST_RX_EAP_SIM_PACKET lastReceivedPacket;

    /* Version */
    CsrUint8 *versionList;
    CsrUint8 versionListLen;
    CsrUint16 selectedVersion;

    /* NONCE values */
    CsrUint8 nonce_mt[EAP_SIM_NONCE_MT_LEN]; /* for Full Authentication */
    CsrUint8 nonce_s[EAP_SIM_NONCE_S_LEN];   /* for Fast Reauthentication */

    /* GSM Keys and Challenges */
    GSM_TRIPLET gsmTriplet[EAP_SIM_MAX_NUM_CHALLENGES];
    CsrUint8 numRANDs;
    CsrUint8 randHistory[MAX_NUM_OF_RANDS_IN_HISTORY][GSM_RAND_LEN];
    CsrUint16 randHistoryIndex;

    /* EAP-SIM Keying Material */
    CsrUint8 mk[EAP_SIM_MASTER_KEY_LEN];
    CsrUint8 mkReauth[EAP_SIM_MASTER_KEY_LEN];
    CsrUint8 k_aut[EAP_SIM_K_AUT_KEY_LEN];
    CsrUint8 k_encr[EAP_SIM_K_ENCR_KEY_LEN];
    CsrUint8 msk[EAP_SIM_MASTER_SESSION_KEY_LEN];
    CsrUint8 emsk[EAP_SIM_EXTENDED_MASTER_SESSION_KEY_LEN];

    /* Identity */
    eID_TYPE_NETWORK_REQ idTypeReqByNetwork;
    CsrUint8 *pseudonym;
    CsrUint16 pseudonymLen;
    CsrUint8 *reauthId;
    CsrUint16 reauthIdLen;

    CsrBool networkSentAtResultInd;

    /* Counter */
    CsrUint16 counterValue;
    CsrBool   counterValueMismatch;
} CsrSimData;

/* PUBLIC FUNCTION PROTOTYPES **********************************************/

extern void SimEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_METHOD_EAPSIM_H */

