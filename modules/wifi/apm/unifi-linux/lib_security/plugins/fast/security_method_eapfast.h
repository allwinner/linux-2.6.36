/** @file security_method_eapfast.h
 *
 * Definitions for EAP-FAST method.
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
 *   This provides an implementation of an EAP-FAST authentication method.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/fast/security_method_eapfast.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_EAPFAST_H
#define SECURITY_METHOD_EAPFAST_H

#include "plugins/security_method.h"
#include "plugins/tls/csr_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Our one and only EAP-FAST plug-in method.
 * This is the method interface for use by the EAP state machine. */
extern void FastEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method);

/* EAP-FAST internal data */
/* ------------------------------------------------------------------------- */

/* Internal states of the EAP-FAST processing. Not externally visible. */
typedef enum eapfast_state
{
    EAPFAST_AWAIT_START,        /* awaiting EAP-FAST-START(A-ID) */

    /* In-band Provisioning states */
    EAPFAST_AWAIT_PROVISIONING_SERVER_HELLO,
    EAPFAST_AWAIT_PROVISIONING_CHANGE_CYPHER_SPECS,
    EAPFAST_AWAIT_PROVISIONING_ID_REQ,
    EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_CHALLENGE,
    EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_RESULT,
    EAPFAST_AWAIT_PROVISIONING_INTERMEDIATE_RESULT,
    EAPFAST_AWAIT_PROVISIONING_PAC,
    EAPFAST_AWAIT_PROVISIONING_FAILURE,

    /* Authentication Phase 1 */
    EAPFAST_AWAIT_SERVER_HELLO,

    /* Authentication Phase 2 */
    EAPFAST_AWAIT_PHASE2_ID,
    EAPFAST_AWAIT_GTC_CRYPTO_BINDING,
    EAPFAST_AWAIT_GTC_ACCESS_ACCEPT
} eapfast_state;

/* Our artificially imposed limit to the AID. No max stated in spec. */
#define MAX_AID_LENGTH 32

/* This is the control structure for the EAP-FAST state machine.
 * Most everything EAP-FAST gets put in here at some point. */
typedef struct FastContext
{
    CsrWifiSecurityContext* context;

    eapfast_state state;
    CsrUint8 aid[MAX_AID_LENGTH]; /* Authority identity, from AS in EAP-FAST-START message. */

    /* EAP-FAST makes use of the TLS protocol. All the generic TLS parameters go in here. */
    tls_data tls;

    /* Phases 0 and 1 results in the following keying information. */
    CsrUint8 session_key[40];
    CsrUint8 server_challenge[16];
    CsrUint8 client_challenge[16];

    /* Have we yet provisioned, i.e. do we have a PAC we can use? */
    CsrBool provisioned;

    /* PAC Opaque storage. The PAC_key is stored with the generic TLS parameters */
    CsrUint8  PAC_opaque[500];
    CsrUint16 PAC_opaque_length;

    /* Hold our eventual session key. */
    eapKey key;
} FastContext;

/* EAP-FAST data packets */
/* ------------------------------------------------------------------------- */

/* All EAP-FAST data contained within EAP packets follows this structure. */
typedef struct eapfast_packet
{
    /* (EAP header omitted) */

    CsrUint8 flags;        /* 5 bits flags, 3 bits version number */
    union
    {
        struct lengthed
        {
            char length[4];
            CsrUint8 data[1];    /* variable length data */
        } lengthed;
        CsrUint8 data[1];    /* variable length data */
    }u;
} eapfast_packet;

/* Bit definitions for the flags field  */
#define EAPFAST_STARTBIT     0x20
#define EAPFAST_MOREBIT      0x40
#define EAPFAST_LENGTHBIT    0x80
#define EAPFAST_VERSION_BITS 0x07
#define EAPFAST_VERSION_NO 1

/* Minumium size of EAP-FAST header is just the flags field */
#define EAPFAST_HEADER_LENGTH (1)

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_METHOD_EAPFAST_H */
