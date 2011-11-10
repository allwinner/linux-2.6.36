/** @file eapttls.c
 *
 * Implementation of the EAP-TTLS v0 method.
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
 *   This provides an implementation of an EAP-TTLS v0 authentication method.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/ttls/eapttls.c#8 $
 *
 ****************************************************************************/


#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "csr_crypto.h"
#include "csr_mschap/csr_mschap.h"
#include "csr_security_private.h"
#include "plugins/tls/eaptls.h"
#include "plugins/tls/csr_tls.h"
#include "eapttls.h"

/* PRIVATE MACROS ***********************************************************/

/* RFC 2865 */
#define RADIUS_USER_NAME_TYPE                           1

/* RFC 2548 */
#define RADIUS_VENDOR_ID_MICROSOFT                      311
#define RADIUS_MS_CHAP_CHALLENGE                        11
#define RADIUS_MS_CHAP2_RESPONSE                        25
#define RADIUS_MS_CHAP2_SUCCESS                         26
#define RADIUS_MS_CHAP2_ERROR                           2

#define MS_CHAP2_AUTHENTICATOR_STRING_LENGTH            42
#define TTLS_MSCHAP_CHALLENGE_LENGTH                    16

/* TTLS Flags */
#define TTLS_FLAGS_LENGTH_INCLUDED(a)   ((a) & 0x80)
#define TTLS_FLAGS_MORE_FRAGMENTS(a)    ((a) & 0x40)
#define TTLS_FLAGS_START(a)             ((a) & 0x20)
#define TTLS_FLAGS_VERSION(a)           ((a) & 0x07)

#define TTLS_AVP_FLAGS_VENDOR_ID(a)     ((a) & 0x80)

typedef enum CsrTtlsState
{
    TTLS_PHASE1,               /* EAP-TLS procedure */
    TTLS_PHASE2
} CsrTtlsState;

/* Supported inner methods */
typedef enum inner_method
{
    TTLS_INNER_METHOD_MSCHAPV2

} CsrTtlsInnerAuthMethod;

typedef struct ttls_data
{
    CsrUint8 peerChallenge[TTLS_MSCHAP_CHALLENGE_LENGTH]; /* Challenge we send to the peer */
    CsrUint8 authResponse[MS_CHAP2_AUTHENTICATOR_STRING_LENGTH]; /* Authenticator response to verify server's response and authenticate server */

} CsrTtlsData;


/* This is the control structure for the TTLS state machine. */
typedef struct TtlsContext
{
    CsrWifiSecurityContext* securityContext;
    CsrTtlsState state;

    /* All the generic TTLS parameters go in here. */
    CsrTtlsData ttlsData;

    eapKey key;

    eapMethod tlsMethod;   /* TLS method for phase 1 */

    CsrTtlsInnerAuthMethod innerMethod;

    DataBuffer dataref;
} CsrTtlsContext;

/* Function to encode Diameter AVPs */
/* Length should be contained in 3 bytes */
static CsrUint8* TtlsAddDiameterAVP(CsrUint8 *fillout, CsrUint32 avpCode,
                                    CsrUint32 vendorId, CsrUint32 length, CsrUint8 *data)
{
    CsrUint8 flags = 0x40;
    CsrUint32 totalLength = length;

    if (vendorId != 0)
    {
        flags |= 0x80;
    }

    *fillout++ = (0xff000000 & avpCode) >> 24;
    *fillout++ = (0xff0000 & avpCode) >> 16;
    *fillout++ = (0xff00 & avpCode) >> 8;
    *fillout++ = (0xff & avpCode);

    *fillout++ = flags;

    /* length includes the header */
    totalLength += 4 /* AVP code */ + 4 /* flags and length field */;

    totalLength += (vendorId?4:0);

    /* 3 bytes of length in big endian format */
    *fillout++ = (0xff0000 & totalLength) >> 16;
    *fillout++ = (0xff00 & totalLength) >> 8;
    *fillout++ = (0xff & totalLength);

    if (vendorId != 0)
    {
        *fillout++ = (0xff000000 & vendorId) >> 24;
        *fillout++ = (0xff0000 & vendorId) >> 16;
        *fillout++ = (0xff00 & vendorId) >> 8;
        *fillout++ = (0xff & vendorId);
    }

    if (data != NULL)
    {
        CsrMemCpy(fillout, data, length);
        fillout += length;
    }

    /* Pad to the next four byte boundary */
    while ((totalLength % 4) != 0)
    {
        *fillout++ = 0;
        totalLength++;
    }

    return fillout;
}

/* Build the MSChap Challenge */
static void TtlsBuildMschapv2Challenge(CsrTtlsContext *ttlsContext, CsrUint8 reqId)
{
    CsrUint8 *fillout;
    eapol_packet *eapol = (eapol_packet *)ttlsContext->securityContext->buffer;
    CsrUint8 challenge[TTLS_MSCHAP_CHALLENGE_LENGTH + 1]; /* 1 byte for ident */
    CsrUint8 avpData[200], avpResponseData[60], *avpResponseParser, *eapParser;
    CsrUint8 sessionKey[32], ntResponse[24];
    CsrUint32 outLength;

    sme_trace_entry((TR_SECURITY_LIB, "TtlsBuildMschapv2Challenge"));

    ttlsContext->dataref.dataLength = 0;
    ttlsContext->dataref.buf = (CsrUint8 *)eapol;

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */
    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;

    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;

    /* Type and flags fields */
    eapol->u.eap.u.resp.type = EAP_TYPE_TTLS;
    eapParser = &eapol->u.eap.u.resp.u.data;
    *eapParser++ = 0; /* Flags field in TTLS packet */

    fillout = avpData;

    /* Add User-name, challenge and Response AVPs */
    fillout = TtlsAddDiameterAVP(fillout, RADIUS_USER_NAME_TYPE, 0,
                                 CsrStrLen(ttlsContext->securityContext->setupData.username),
                                 (CsrUint8*)ttlsContext->securityContext->setupData.username);

    TlsGetPrf(&ttlsContext->tlsMethod, "ttls challenge", challenge, TTLS_MSCHAP_CHALLENGE_LENGTH + 1);

    fillout = TtlsAddDiameterAVP(fillout, RADIUS_MS_CHAP_CHALLENGE, RADIUS_VENDOR_ID_MICROSOFT,
                                 TTLS_MSCHAP_CHALLENGE_LENGTH, challenge);

    avpResponseParser = avpResponseData;
    *avpResponseParser++ = challenge[TTLS_MSCHAP_CHALLENGE_LENGTH]; /* Identity field */
    *avpResponseParser++ = 0; /* reserved flags */

    /* Peer challenge */
    CsrWifiSecurityRandom(ttlsContext->ttlsData.peerChallenge, TTLS_MSCHAP_CHALLENGE_LENGTH);
    CsrMemCpy(avpResponseParser, ttlsContext->ttlsData.peerChallenge, TTLS_MSCHAP_CHALLENGE_LENGTH);
    avpResponseParser += TTLS_MSCHAP_CHALLENGE_LENGTH;
    CsrMemSet(avpResponseParser, 0, 8);
    avpResponseParser += 8;

    CsrCryptoCallMschapGenerateNTResponseAndSessionKey(ttlsContext->securityContext->cryptoContext, NULL,
                                                       challenge, ttlsContext->ttlsData.peerChallenge,
                                                       (CsrUint8*)ttlsContext->securityContext->setupData.username,
                                                       (CsrUint8*)ttlsContext->securityContext->setupData.password,
                                                       ntResponse, sessionKey);
    CsrMemCpy(avpResponseParser, ntResponse, 24);
    avpResponseParser += 24;

    fillout = TtlsAddDiameterAVP(fillout, RADIUS_MS_CHAP2_RESPONSE, RADIUS_VENDOR_ID_MICROSOFT,
                                 (CsrUint32)(avpResponseParser - avpResponseData), avpResponseData);

    /* Encrypt the data */
    BuildTlsApplicationData(&ttlsContext->tlsMethod, avpData, (CsrUint16)(fillout - avpData), eapParser, &outLength);

    outLength += 10; /* EAP header */
    /* EAPOL data length in EAPOL header */
    EAPOL_LENGTH_ASSIGN(eapol, outLength);
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    ttlsContext->dataref.dataLength = (CsrUint16)outLength + CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH;

    /* Calculate and store authenticator response, will be used to check auth response sent by server */
    GenerateAuthenticatorResponse(challenge, ttlsContext->ttlsData.peerChallenge,
                                  (CsrUint8*)ttlsContext->securityContext->setupData.username,
                                  (CsrUint8*)ttlsContext->securityContext->setupData.password,
                                  ntResponse, ttlsContext->ttlsData.authResponse);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Auth response", ttlsContext->ttlsData.authResponse, MS_CHAP2_AUTHENTICATOR_STRING_LENGTH));
}

/* Build an empty response - Indicates success */
static void TtlsBuildEmptyResponse(eapMethod* method, eap_packet *eapReqData, CsrUint8 reqId)
{
    CsrTtlsContext* ttlsContext = (CsrTtlsContext*)method->methodContext;
    eapol_packet *eapol = (eapol_packet *)ttlsContext->securityContext->buffer;

    /* Authentication is successful, Send an empty response message to indicate success */
    ttlsContext->dataref.dataLength = CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH + EAP_REQ_RESP_HEADER_LENGTH + 1;
    ttlsContext->dataref.buf = (CsrUint8 *)eapol;

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */

    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    EAPOL_LENGTH_ASSIGN(eapol,
            ttlsContext->dataref.dataLength - (CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH));

    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* EAP-TLS type and flags fields */
    eapol->u.eap.u.resp.type = EAP_TYPE_TTLS;
    *((CsrUint8*)(&eapol->u.eap.u.resp.u.data) + 1) = 0; /*lint !e415 */
}

/* Call back function (will be called from the TLS module), parses AVPs present as application data
 * within TLS records */
static CsrBool ParseAVP(void* context, CsrUint8 **data, CsrUint16 size, CsrUint32 *update_flags, tls_data *tls)
{
    CsrUint8 *scan = *data;
    CsrUint32 avpCode, flags;
    CsrUint32 avpLength;
    CsrBool cont = TRUE, ret = TRUE; /* continue processing? */
    CsrUint32 vendorId = 0;
    eapMethod* method = context;
    CsrTtlsContext* ttlsContext = method->methodContext;

    sme_trace_entry((TR_SECURITY_LIB, "ParseAVP()"));

    sme_trace_info((TR_SECURITY_LIB, "ParseAVP :: size = %d", size));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "ParseAVP :: scan", scan, size));

    /* TTLS application-specific data is stored in a series of Attribute-Value pairs
     * Loop examining each. */
    while(cont)
    {
        avpCode = (scan[0] << 24) | (scan[1] << 16) | (scan[2] << 8) | scan[3];
        scan += 4;
        flags = *scan++;
        avpLength = (scan[0] << 16) | (scan[1] << 8) | scan[2];
        scan += 3;
        if (TTLS_AVP_FLAGS_VENDOR_ID(flags))
        {
            vendorId = (scan[0] << 24) | (scan[1] << 16) | (scan[2] << 8) | scan[3];
            scan += 4;
        }

        if (vendorId != RADIUS_VENDOR_ID_MICROSOFT)
        {
            sme_trace_info((TR_SECURITY_LIB, "Unknown vendor ID"));
            break;
        }

        scan++; /* Identifier field */

        *data += (size - MAC_LENGTH);

        cont = FALSE;
        switch(avpCode)
        {
            case RADIUS_MS_CHAP2_SUCCESS:
            {
                sme_trace_info((TR_SECURITY_LIB, "Received MSCHAP2 Success"));
                sme_trace_info((TR_SECURITY_LIB, "AVP length = %d", avpLength));

                /* Authenticate the server */
                /* Verify if authenticator response is as expected */
                if (0 == CsrMemCmp(ttlsContext->ttlsData.authResponse, scan, MS_CHAP2_AUTHENTICATOR_STRING_LENGTH))
                {
                    sme_trace_info((TR_SECURITY_LIB, "Verified server auth response"));
                    method->decision = COND_SUCC;
                }
                else
                {
                    sme_trace_warn((TR_SECURITY_LIB, "Failed to authenticate server"));
                    cont = FALSE;
                    ret = FALSE; /* No need to process anything else, inform the calling function */
                    method->decision = FAIL;
                    method->methodState = DONE;
                }
            }
            break;

            case RADIUS_MS_CHAP2_ERROR:
            {
                sme_trace_info((TR_SECURITY_LIB, "Received MS CHAP error"));
                method->decision = FAIL;
                method->methodState = DONE;
            }
            break;

            default:
                sme_trace_info((TR_SECURITY_LIB, "Unhandled AVP received"));
                break;
        }

        scan = *data;
    }

    return ret;
}

/* Handler for all phase 2 packets when the inner method is MSChapv2 */
static void TtlsProcessMschapv2(eapMethod* method, eap_packet *eapReqData, CsrUint8 reqId)
{
    CsrTtlsContext* ttlsContext = (CsrTtlsContext*)method->methodContext;
    CsrUint8 *parser = (CsrUint8*)(&eapReqData->u.req.u.data);
    CsrUint32 length; /* length of the data contained in TTLS packet */
    CsrUint32 flags;

    sme_trace_entry((TR_SECURITY_LIB, "TtlsProcessMschapv2"));

    /* Parse the TTLS packet */
    /*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Code      |   Identifier  |            Length             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |     Flags     |        Message Length         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        Message Length         |             Data...           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */

    length = EAP_LENGTH_FIELD(*eapReqData);

    /* flags */
    if(TTLS_FLAGS_LENGTH_INCLUDED(*parser))
    {
        parser++;
        length = (parser[0] << 24) | (parser[1] << 16) | (parser[2] << 8) | parser[3]; /*lint !e415 !e416 */
        parser += 4; /*lint !e416 */
    }
    else
    {
        length -= 6;
        parser++;
    }

    /* TLS Record layer */
    if (!parse_tls(method, parser, length, &(((TlsContext*)(ttlsContext->tlsMethod.methodContext))->tls), &flags, ParseAVP))
    {
        sme_trace_error((TR_SECURITY_LIB, "Failed to decrypt/parse TLS records"));
        return;
    }

    if (method->decision == COND_SUCC)
    {
        CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH];

        /* Authentication successful build an empty response */
        TtlsBuildEmptyResponse(method, eapReqData, reqId);

        sme_trace_info((TR_SECURITY_LIB, "TtlsProcessMschapv2() :: Updating PMK"));
        TlsGetPrf(&ttlsContext->tlsMethod, "ttls keying material", pmk, CSR_WIFI_SECURITY_PMK_LENGTH);
        CsrWifiSecurityStorePmk(ttlsContext->securityContext, pmk);

        method->methodState = DONE;

        /* Save TLS session, can be used to resume later on */
        install_tls_session((TlsContext*)(ttlsContext->tlsMethod.methodContext));
    }
    else
    {
        /* Clear the buffer */
        ttlsContext->dataref.dataLength = 0;
        ttlsContext->dataref.buf = NULL;
    }
}

/**
 * @brief Check an incoming EAP meassage from the AP.
 *
 * @par Description
 *   This function checks an incoming EAP message for validity and relevance to
 *   TTLS.
 *
 * @param[in]   eapReqData : the EAP message received from the AP.
 * @param[in]   size : the size of the incoming packet. Maybe be larger than the actual EAP message.
 *
 * @return
 *   TRUE - TTLS message ok, FALSE - bad or irrelevant message.
 */
static CsrBool TtlsCheck(eapMethod* method, eap_packet *eapReqData, CsrUint16 size)
{
    CsrTtlsContext* ttlsContext = (CsrTtlsContext*)method->methodContext;

    sme_trace_entry((TR_SECURITY_LIB, "TtlsCheck"));

    switch(ttlsContext->state)
    {
        case TTLS_PHASE1:
        {
            sme_trace_info((TR_SECURITY_LIB, "TTLS_PHASE1"));

            /* Let the TLS module handle phase 1 */
            return (ttlsContext->tlsMethod.check(&ttlsContext->tlsMethod, eapReqData, size));
        }

        case TTLS_PHASE2:
        {


        }
        break;
    }

    return TRUE;
}

/**
 * @brief Process a received message from the AP.
 *
 * @pre TtlsCheck() has been called to ensure the message's validity.
 *
 * @post processing has been done ready for TtlsBuild() to assemble a response.
 *
 * @param[in]   eapReqData : the EAP packet arrived from the AP.
 */
static void TtlsProcess(eapMethod* method, eap_packet *eapReqData, CsrUint8 reqId)
{
    CsrTtlsContext* ttlsContext = (CsrTtlsContext*)method->methodContext;
    sme_trace_entry((TR_SECURITY_LIB, "TtlsProcess"));

    method->isKeyAvailable = FALSE;

    switch(ttlsContext->state)
    {
        case TTLS_PHASE1:
        {
            tls_phase1_state tlsState;
            ttlsContext->tlsMethod.process(&ttlsContext->tlsMethod, eapReqData, reqId);
            method->decision = ttlsContext->tlsMethod.decision;
            method->methodState = ttlsContext->tlsMethod.methodState;

            tlsState = TlsPhase1State(&ttlsContext->tlsMethod);

            if (TLS_SESSION_RESUMPTION == tlsState)
            {
                CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH];
                /* Session resumption, TTLS just has to generate new keys */
                TlsGetPrf(&ttlsContext->tlsMethod, "ttls keying material", pmk, CSR_WIFI_SECURITY_PMK_LENGTH);
                CsrWifiSecurityStorePmk(ttlsContext->securityContext, pmk);
            }
            else if (TLS_DONE == tlsState)
            {
                method->methodState = CONT;
                method->decision = FAIL;

                /* Go to processing phase 2 */
                ttlsContext->state = TTLS_PHASE2;
                sme_trace_info((TR_SECURITY_LIB, "TTLS Phase 2"));

                if (ttlsContext->innerMethod == TTLS_INNER_METHOD_MSCHAPV2)
                {
                    /* Form a challenge */
                    TtlsBuildMschapv2Challenge(ttlsContext, reqId);
                }
            }
        }
        break;

        case TTLS_PHASE2:
        {
            if (ttlsContext->innerMethod == TTLS_INNER_METHOD_MSCHAPV2)
            {
                TtlsProcessMschapv2(method, eapReqData, reqId);
            }
        }
        break;
    }/* switch */
}

/**
 * @brief Send message to the AP.
 *
 * @par Description
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet (if length = zero then
 *   no packet).
 */
static DataBuffer TtlsBuildResp(eapMethod* method, CsrUint8 reqId)
{
    CsrTtlsContext* ttlsContext = (CsrTtlsContext*)method->methodContext;

    sme_trace_entry((TR_SECURITY_LIB, "TtlsBuildResp"));

    switch(ttlsContext->state)
    {
        case TTLS_PHASE1:
        {
            return (ttlsContext->tlsMethod.buildResp(&ttlsContext->tlsMethod, reqId));
        }

        case TTLS_PHASE2:
        {
            return ttlsContext->dataref;
        }

        default:
        {
            require(TR_SECURITY_LIB, 0); /*lint !e774 */
        }
        break;
    }/* switch */

    return ttlsContext->dataref;
}

/**
 * @brief Return a key to the caller.
 *
 * @par Description
 *
 * @pre
 *   The caller must check isKeyAvailable before calling this function.
 *   isKeyAvailable must be TRUE or the return value is undefined.
 *
 * @return
 *   session or group key - whichever was most recently obtained.
 */
static eapKey *TtlsGetKey(eapMethod* method)
{
    CsrTtlsContext* ttlsContext = (CsrTtlsContext*)method->methodContext;
    return &ttlsContext->key;
}

static void TtlsDeinit(eapMethod* method)
{
    CsrTtlsContext* ttlsContext = (CsrTtlsContext*)method->methodContext;

    ttlsContext->tlsMethod.deinit(&ttlsContext->tlsMethod);
    CsrPfree(method->methodContext);
    method->methodContext = NULL;
}

/* This is the method interface for use by the EAP state machine. */
static const eapMethod ttls_method =
{
    NULL,  /* Pointer to local Context */
    NONE,  /* state */
    FAIL,  /* decision */
    FALSE, /* allow notifications */
    FALSE, /* isKeyAvaiable */
    FALSE, /* isEapolKeyHint */
    TtlsDeinit,
    TtlsCheck,
    TtlsProcess,
    TtlsBuildResp,
    TtlsGetKey
};


/**
 * @brief Initialize the TTLS method state machine.
 */
void TtlsEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method)
{
    CsrTtlsContext* ttlsContext = (CsrTtlsContext*) CsrPmalloc(sizeof(CsrTtlsContext));
    CsrMemSet(ttlsContext, 0, sizeof(CsrTtlsContext));

    sme_trace_info((TR_SECURITY_LIB, "TtlsEapMethodInit"));
    CsrMemCpy(method, &ttls_method, sizeof(ttls_method));
    method->methodContext = ttlsContext;

    ttlsContext->securityContext = context;
    ttlsContext->state = TTLS_PHASE1;
    ttlsContext->innerMethod = TTLS_INNER_METHOD_MSCHAPV2;

    /* Initialize TLS module */
    TlsEapMethodInit(context, &ttlsContext->tlsMethod);
    TlsSetEapType(&ttlsContext->tlsMethod, EAP_TYPE_TTLS);
}


