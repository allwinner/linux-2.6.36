/** @file eapfast_authentication.c
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
 *   A suite of functions to handing authentication messages and generate
 *   responses.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/fast/eapfast_authentication.c#2 $
 *
 ****************************************************************************/
#include "eapfast_authentication.h"
#include "csr_types.h"
#include "sme_trace/sme_trace.h"
#include "csr_crypto.h"

/* Values for the extension_type field of client_hello. This is an EAP-FAST extension. */
#define EXTENSION_SERVER_NAME            0
#define EXTENSION_MAX_FRAGMENT_LENGTH    1
#define EXTENSION_CLIENT_CERTIFICATE_URL 2
#define EXTENSION_TRUSTED_CA_KEYS        3
#define EXTENSION_TRUNCATED_HMAC         4
#define EXTENSION_STATUS_REQUEST         5

/*
 * Description:
 * See description in eapfast_authentication.h
 */
/*---------------------------------------------------------------------------*/
CsrBool build_sha1_mac(FastContext *fastContext, CsrUint8 msg_type, CsrUint8 *data, CsrUint32 length, CsrUint8 *write_MAC_secret)
{
    CsrBool ret = FALSE;
    CsrUint8 *mac_source_data = CsrPmalloc(length+8+5);

    if (mac_source_data)
    {
        CsrMemCpy(mac_source_data, fastContext->tls.write_seq_no, 8);
        mac_source_data[8] = msg_type;
        mac_source_data[9] = 3;  /* protocol version hi */
        mac_source_data[10] = 1; /* protocol version low */
        mac_source_data[11] = (length >> 8) & 0xff;
        mac_source_data[12] = length & 0xff;
        CsrMemCpy(mac_source_data+8+5, data, length);

        CsrCryptoCallHmacSha1(fastContext->context->cryptoContext, NULL,
                              write_MAC_secret, MAC_LENGTH, mac_source_data, 8+5+length, data + length);
        /* for next time */
        inc_seq_no(fastContext->tls.write_seq_no);

        ret = TRUE;
    }
    else
        sme_trace_error((TR_SECURITY_LIB, "build_sha1_mac(). Memory allocation failure."));

    CsrPfree(mac_source_data);
    return ret;
}

/*
 * Description:
 * See description in eapfast_authentication.h
 */
/*---------------------------------------------------------------------------*/
CsrBool check_phase2_crypto_bind(eapMethod *context, eap_packet *eapReqData, CsrUint32 size, parse_tls_callback callback)
{
    eapfast_packet *ef = (eapfast_packet *)&(eapReqData->u.req.u.data);
    CsrUint32 ef_length;
    CsrUint32 flags;
    CsrUint8 isk[32];
    CsrUint8 msk[64]; /* Master Session Key */
    FastContext* fastContext = (FastContext*)context->methodContext;

    sme_trace_entry((TR_SECURITY_LIB, "check_phase2_crypto_bind()."));

    if (eapReqData->code != 1)
    {
        sme_trace_error((TR_SECURITY_LIB, "check_phase2_crypto_bind(): Unexpected EAP code (%d).", eapReqData->code));
        return FALSE;
    }
    if (ef->flags != 0x81)
    {
        sme_trace_error((TR_SECURITY_LIB, "check_phase2_crypto_bind(): EAP-FAST flags incorrect (0x%2x).", ef->flags));
        return FALSE;
    }
    if (eapReqData->u.req.type != 43)
    {
        sme_trace_error((TR_SECURITY_LIB, "check_phase2_crypto_bind(): EAP request type code ($d) not EAP-FAST (43).", eapReqData->u.req.type));
        return FALSE;
    }

    ef_length =
        (ef->u.lengthed.length[0] << 24) +
        (ef->u.lengthed.length[1] << 16) +
        (ef->u.lengthed.length[2] <<  8) +
        ef->u.lengthed.length[3];

    if (size < (ef_length + EAP_REQ_RESP_HEADER_LENGTH + EAPFAST_HEADER_LENGTH + sizeof(CsrUint32)))
    {
        sme_trace_error((TR_SECURITY_LIB, "check_phase2_crypto_bind(): Truncated packet."));
        return FALSE;
    }

    if (!parse_tls(fastContext, ef->u.lengthed.data, ef_length, &fastContext->tls, &flags, callback))
        return FALSE;

    /* Now calcuate the keys. We do it here so we can check the validity of the compound MAC */
    CsrMemSet(isk, 0, 32);

    t_prf(fastContext->tls.imck1, 40, "Session Key Generating Function", 31, fastContext->tls.imck1/*dummy*/, 0, msk, 64);

    /* We have the key now! First 32 octets of the MSK */
    sme_trace_info((TR_SECURITY_LIB, "check_phase2_crypto_bind() :: Updating PMK"));
    CsrWifiSecurityStorePmk(fastContext->context, msk);
    return TRUE;
}

/*
 * Description:
 * See description in eapfast_authentication.h
 */
/*---------------------------------------------------------------------------*/
CsrBool check_phase2_id(FastContext* fastContext, eap_packet *eapReqData, CsrUint32 size, parse_tls_callback callback)
{
    eapfast_packet *ef = (eapfast_packet *)&(eapReqData->u.req.u.data);
    CsrUint32 ef_length;
    CsrUint32 flags;

    sme_trace_entry((TR_SECURITY_LIB, "check_phase2_id()."));

    if (eapReqData->code != 1)
    {
        sme_trace_error((TR_SECURITY_LIB, "check_phase2_id(): Unexpected EAP code (%d).", eapReqData->code));
        return FALSE;
    }
    if (eapReqData->u.req.type != 43)
    {
        sme_trace_error((TR_SECURITY_LIB, "check_phase2_id(): EAP request type code ($d) not EAP-FAST (43).", eapReqData->u.req.type));
        return FALSE;
    }
    if (ef->flags != 0x81)
    {
        sme_trace_error((TR_SECURITY_LIB, "check_phase2_id(): EAP-FAST flags incorrect (0x%2x).", ef->flags));
        return FALSE;
    }
    ef_length =
        (ef->u.lengthed.length[0] << 24) +
        (ef->u.lengthed.length[1] << 16) +
        (ef->u.lengthed.length[2] <<  8) +
        ef->u.lengthed.length[3];

    if (size < (ef_length + EAP_REQ_RESP_HEADER_LENGTH + EAPFAST_HEADER_LENGTH + sizeof(CsrUint32)))
    {
        sme_trace_error((TR_SECURITY_LIB, "check_phase2_id(): Truncated packet."));
        return FALSE;
    }

    if (!parse_tls(fastContext, ef->u.lengthed.data, ef_length, &fastContext->tls, &flags, callback))
        return FALSE;

    return TRUE;
}

/*
 * Description:
 * See description in eapfast_authentication.h
 */
/*---------------------------------------------------------------------------*/
void process_eapfast_server_start(FastContext* fastContext)
{
    /* Assemble the random challenge to put in the client hello. */
    /* This can be done using any decent source of random data,
     * Here we make use of the TLS pseudo random function. It's there so we
     * might as well use it. We seed it with the current time. */
    fastContext->tls.client_random.gmt_unix_time = (CsrUint32)(CsrGetTime()/1000); /* endianity issue? */
    prf(fastContext->aid, MAX_AID_LENGTH, /* Use the AID as the "secret" */
        "CSR generate random", 19,
        (CsrUint8 *)&fastContext->tls.client_random.gmt_unix_time, sizeof(CsrUint32),
        fastContext->tls.client_random.data, 28);
}

/*
 * Description:
 * See description in eapfast_authentication.h
 */
/*---------------------------------------------------------------------------*/
DataBuffer build_eapfast_client_hello(FastContext* fastContext, CsrUint8 reqId)
{
    /*      The extended EAP-FAST client hello message looks like this:
     *      typedef struct client_hello
     *      {
     *          ProtocolVersion client_version;
     *          Random random;
     *          SessionID session_id;                  - variable length (vector)
     *          CypherSuite cypher_suites;             - variable length (vector)
     *          CompressionMethod compression_methods; - variable length (vector)
     *          struct Extension
     *          {
     *              CsrUint16 extension_type
     *              CsrUint8 data;                        - variable length (vector)
     *          } client_hello_extension_list;         - variable number of (vector)
     *      } client_hello;
     */

    CsrUint8 *fillout;
    CsrUint8 *handshake_start;
    CsrUint16 ciphersuite;
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    DataBuffer dataref = {CSR_WIFI_SECURITY_SNAP_LENGTH
                          + EAPOL_HEADER_LENGTH
                          + EAP_REQ_RESP_HEADER_LENGTH
                          + EAPFAST_HEADER_LENGTH,
                          NULL};
    CsrUint16 tls_client_hello_length = sizeof(CsrUint16) /* protocol version */
        + sizeof(Random)
        + 1 /* session id */
        + 4 /* cypher suite */
        + 2 /* compression method */
        + 6 + fastContext->PAC_opaque_length /* extension field */;

    CsrUint16 tls_message_length = 1/*client hello type field*/ +3/*handshake length field*/ + tls_client_hello_length;

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_client_hello()."));

    dataref.buf = (CsrUint8 *)eapol;
    dataref.dataLength += /*4*/ /* TLS message length field */
          5 /* TLS record protocol TLSPlaintext header */
        + tls_message_length;

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */

    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    EAPOL_LENGTH_ASSIGN(eapol,
                        dataref.dataLength - (CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH));

    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* EAP-FAST type and flags fields */
    eapol->u.eap.u.resp.type = EAP_TYPE_EAPFAST;
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = /*EAPFAST_LENGTHBIT +*/ EAPFAST_VERSION_NO;

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;

    /* Now fill out TLS record protocol TLSPlaintext message.                */
    /* --------------------------------------------------------------------- */

    /*lint -save -e415 -e416 -e419 */

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 22; /* handshake */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = tls_message_length>>8; /* length (u16) */
    *fillout++ = tls_message_length & 0xff;

    /* Handshake message begins here */
    handshake_start = fillout;

    /* Handshake type */
    *fillout++ = 1; /* client hello */

    /* Handshake length */
    *fillout++ = 0;
    *fillout++ = tls_client_hello_length>>8;
    *fillout++ = tls_client_hello_length & 0xff;

    /* Client Hello message begins here */

    /* Protocol Version field */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;

    /* Random field */
    CsrMemCpy(fillout, &fastContext->tls.client_random, sizeof(Random));
    fillout += sizeof(Random);

    /* Session ID field - vector, length field = 8 bits */
    *fillout++ = 0;

    /* Cypher Suites field - vector, length field = 16 bits  */
    *fillout++ = 0;
    *fillout++ = 2;
    ciphersuite = TLS_RSA_WITH_RC4_128_SHA;
    *fillout++ = ciphersuite >> 8;
    *fillout++ = ciphersuite & 0xff;

    /* Compression method field - vector, length field = 8 bits */
    *fillout++ = 1;
    *fillout++ = 0; /* NULL compression only */

    /* Extension field - vector, length field = 16 bits */
    {
        CsrUint16 extension_length = fastContext->PAC_opaque_length + 4;
        *fillout++ = (CsrUint8)(extension_length>>8); /* endian safe */
        *fillout++ = (CsrUint8)(extension_length & 0xff);
    }

    /* PAC opaque extension */
    *fillout++ = 0;
    *fillout++ = 35; /* PAC-Opaque type */
    *fillout++ = (CsrUint8)(fastContext->PAC_opaque_length>>8); /* endian safe */
    *fillout++ = (CsrUint8)(fastContext->PAC_opaque_length & 0xff);

    /*lint -restore */

    CsrMemCpy(fillout, fastContext->PAC_opaque, fastContext->PAC_opaque_length);

    fillout += fastContext->PAC_opaque_length;

    CsrCryptoCallMd5Update(fastContext->context->cryptoContext, NULL,
                           fastContext->tls.md5_ctx, handshake_start, (CsrUint32)(fillout - handshake_start));
    CsrCryptoCallSha1Update(fastContext->context->cryptoContext, NULL,
                            fastContext->tls.sha1_ctx, handshake_start, (CsrUint32)(fillout - handshake_start));

    return dataref;
}

/*
 * Description:
 * See description in eapfast_authentication.h
 */
/*---------------------------------------------------------------------------*/
DataBuffer build_eapfast_client_finished(FastContext* fastContext, CsrUint8 reqId)
{
    CsrUint8 *fillout;
    /* The rest of the data is to be encrypted */
    CsrUint8 plaintext[100];
    CsrUint8 *pt = NULL;
    CsrUint8 seed[CSR_CRYPTO_MD5_DIGEST_LENGTH + CSR_SHA1_DIGEST_LENGTH];

    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    DataBuffer dataref = {CSR_WIFI_SECURITY_SNAP_LENGTH
                          + EAPOL_HEADER_LENGTH
                          + EAP_REQ_RESP_HEADER_LENGTH
                          + EAPFAST_HEADER_LENGTH,
                          NULL};

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_client_finished()."));

    dataref.buf = (CsrUint8 *)eapol;
    dataref.dataLength += /*4*/ /* TLS message length field */
          5 /* TLS record protocol TLSPlaintext header */
        + 1 /* change cypher spec*/
        + 5 /* TLS record protocol TLSPlaintext header */
        + 4 /* finshed header */
        + 12/* finished */
        + MAC_LENGTH;

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */

    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    EAPOL_LENGTH_ASSIGN(eapol,
                        dataref.dataLength - (CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH));

    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* EAP-FAST type and flags fields */
    eapol->u.eap.u.resp.type = EAP_TYPE_EAPFAST;
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = /*EAPFAST_LENGTHBIT +*/ EAPFAST_VERSION_NO;

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;

    /* Now fill out TLS record protocol TLSPlaintext message.                */
    /* --------------------------------------------------------------------- */

    /*lint -save -e415 -e416 */

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 20; /* change cypher */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = 0; /* length (u16) */
    *fillout++ = 1;

    /* Change Cypher Spec message */
    *fillout++ = 1;

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 22; /* handshake */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = 0; /* length (u16) */
    *fillout++ = 4+12+MAC_LENGTH;

    /*lint -restore */

    /* The rest of the data is to be encrypted */
    pt = plaintext;

    /* finished header */
    *pt++ = 20; /* finished */
    *pt++ = 0;  /* length (uint24) */
    *pt++ = 0;
    *pt++ = 0x0c;

    /* PRF(master_secret, "client finished", MD5(handshake_messages) + SHA1(handshake_messages) */

    {
        CSR_CRYPTO_MD5_CTX *md5_ctx = CsrCryptoCloneMd5Context(fastContext->tls.md5_ctx);
        CsrCryptoCallMd5Final(fastContext->context->cryptoContext, NULL,
                              seed, md5_ctx);
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "build_eaptls_client_key_exchange() :: MD5 hash", seed, CSR_CRYPTO_MD5_DIGEST_LENGTH));
    }

    {
        CSR_CRYPTO_SHA1_CTX *sha1_ctx = CsrCryptoCloneSha1Context(fastContext->tls.sha1_ctx);
        CsrCryptoCallSha1Final(fastContext->context->cryptoContext, NULL,
                               &seed[CSR_CRYPTO_MD5_DIGEST_LENGTH], sha1_ctx);
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "build_eaptls_client_key_exchange() :: SHA1 hash", &seed[CSR_CRYPTO_MD5_DIGEST_LENGTH], CSR_SHA1_DIGEST_LENGTH));
    }

    prf(fastContext->tls.master_secret, 48,
        "client finished", 15,
        seed, CSR_CRYPTO_MD5_DIGEST_LENGTH + CSR_SHA1_DIGEST_LENGTH,
        pt, 12);

    pt += 12;

    /* Is this the correct place for setting this? */
    CsrMemSet(fastContext->tls.write_seq_no, 0, 8);

    (void)build_sha1_mac(fastContext, 22/*handshake*/, plaintext, 4+12, fastContext->tls.keys.client_write_MAC_secret);

    /* First encryption. Set up the key. */
    CsrRc4SetKey(&fastContext->tls.rc4_client_write_key, 16, fastContext->tls.keys.client_write_key);

    /* Encrypt the data */
    CsrRc4(&fastContext->tls.rc4_client_write_key, 36, plaintext, fillout);

    return dataref;
}

/*
 * Description:
 * See description in eapfast_authentication.h
 */
/*---------------------------------------------------------------------------*/
DataBuffer build_eapfast_gtc_response(FastContext *fastContext, CsrUint8 reqId)
{
    CsrUint8 *fillout;
    /* The rest of the data is to be encrypted */
    CsrUint8 plaintext[200];
    CsrUint8 *pt = NULL;

    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    DataBuffer dataref = {CSR_WIFI_SECURITY_SNAP_LENGTH
                          + EAPOL_HEADER_LENGTH
                          + EAP_REQ_RESP_HEADER_LENGTH
                          + EAPFAST_HEADER_LENGTH,
                          NULL};
    CsrUint8 gtc_response_len =
          5 /* EAP GTC response header */
        + 9 /* "RESPONSE=" string*/
        + (CsrUint8)CsrStrLen(fastContext->context->setupData.username)
        + 1 /*zero terminator*/
        + (CsrUint8)CsrStrLen(fastContext->context->setupData.password);

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_gtc_response()."));

    dataref.buf = (CsrUint8 *)eapol;
    dataref.dataLength += /*4*/ /* TLS message length field */
          5 /* TLS record protocol TLSPlaintext header */
        + 4 /* EAP Payload TLV  header */
        + gtc_response_len
        + MAC_LENGTH;

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */

    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    EAPOL_LENGTH_ASSIGN(eapol,
                        dataref.dataLength - (CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH));

    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* EAP-FAST type and flags fields */
    eapol->u.eap.u.resp.type = EAP_TYPE_EAPFAST;
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = /*EAPFAST_LENGTHBIT +*/ EAPFAST_VERSION_NO;

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;

    /* Now fill out TLS record protocol TLSPlaintext message.                */
    /* --------------------------------------------------------------------- */

    /*lint -save -e415 -e416 */

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 23; /* application data */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = 0; /* length (u16) */
    *fillout++ = 4 /*EAP Payload TLV header*/ + gtc_response_len + MAC_LENGTH;

    /*lint -restore */

    /* The rest of the data is to be encrypted */
    pt = plaintext;

    /* EAP Payload TLV */
    *pt++ = 0x80;
    *pt++ = 0x09;
    *pt++ = 0;
    *pt++ = gtc_response_len;

    /* EAP GTC response header */
    *pt++ = EAP_CODE_RESP;
    *pt++ = fastContext->tls.gtc_reqId;
    *pt++ = 0; /* length same as above */
    *pt++ = gtc_response_len;
    *pt++ = EAP_TYPE_GTC;

    CsrStrCpy((char *)pt, "RESPONSE=");
    pt += 9;
    CsrStrCpy((char *)pt, (char *)fastContext->context->setupData.username);
    pt += CsrStrLen(fastContext->context->setupData.username) + 1/*zero terminator*/;
    CsrStrCpy((char *)pt, (char *)fastContext->context->setupData.password);
    pt += CsrStrLen(fastContext->context->setupData.password); /*no terminator*/

    (void)build_sha1_mac(fastContext,
                         23,
                         plaintext,
                         4 /*EAP Payload TLV  header*/ + gtc_response_len,
                         fastContext->tls.keys.client_write_MAC_secret);

    /* Encrypt the data */
    CsrRc4(&fastContext->tls.rc4_client_write_key,
        4/* EAP Payload TLV header*/ + gtc_response_len + MAC_LENGTH,
        plaintext, fillout);

    return dataref;
}

/*
 * Description:
 * See description in eapfast_authentication.h
 */
/*---------------------------------------------------------------------------*/
DataBuffer build_eapfast_crypto_binding_response(FastContext* fastContext, CsrUint8 reqId)
{
    CsrInt32 i;
    CsrUint8 *fillout;
    CsrUint8 plaintext[200];
    CsrUint8 *pt = NULL;
    CsrUint8 *tlv = NULL;
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    CsrUint8 compound_mac[CSR_SHA1_DIGEST_LENGTH];

    DataBuffer dataref = {CSR_WIFI_SECURITY_SNAP_LENGTH
                          + EAPOL_HEADER_LENGTH
                          + EAP_REQ_RESP_HEADER_LENGTH
                          + EAPFAST_HEADER_LENGTH,
                          NULL};
    CsrUint8 gtc_response_len =
          6 /* Result TLV */
        + 60; /* Crypto binding TLV */

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_crypto_binding_response()."));

    dataref.buf = (CsrUint8 *)eapol;
    dataref.dataLength += /*4*/ /* TLS message length field */
        5 /*Record protocol header*/
        + gtc_response_len
        + MAC_LENGTH;

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */

    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    EAPOL_LENGTH_ASSIGN(eapol,
                        dataref.dataLength - (CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH));

    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* EAP-FAST type and flags fields */
    eapol->u.eap.u.resp.type = EAP_TYPE_EAPFAST;
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = /*EAPFAST_LENGTHBIT +*/ EAPFAST_VERSION_NO;

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;

    /* Now fill out TLS record protocol TLSPlaintext message.                */
    /* --------------------------------------------------------------------- */

    /*lint -save -e415 -e416 */

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 23; /* application data */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = 0; /* length (u16) */
    *fillout++ = gtc_response_len + MAC_LENGTH;

    /*lint -restore */

    /* The rest of the data is to be encrypted */
    pt = plaintext;

    /* EAP Result TLV */
    *pt++ = 0x80;
    *pt++ = 0x03; /* result TLV  */
    *pt++ = 0;    /* length hi   */
    *pt++ = 2;    /* length low  */
    *pt++ = 0;    /* success hi  */
    *pt++ = 1;    /* success low */

    /* EAP Crypto binding TLV */
    tlv = pt;     /* Start of compound MAC data */
    *pt++ = 0x80; /* fixed value */
    *pt++ = 0x0c; /* crypto binding TLV */
    *pt++ = 0;    /* length hi   */
    *pt++ = 0x38; /* length low  */
    *pt++ = 0;    /* version hi  */
    *pt++ = 1;    /* version low */
    *pt++ = 1;    /* eapfast version  */
    *pt++ = 1;    /* binding repsonse */
    /* increment the nonce and put it in the TLV */
    for(i=31; i>0; i--)
        if (fastContext->tls.nonce[i]++ != 255)
            break;
    CsrMemCpy(pt, fastContext->tls.nonce, 32);
    pt += 32;

    /* compute the Compound MAC and put it in the TLV */
    CsrMemSet(pt, 0, CSR_SHA1_DIGEST_LENGTH);
    CsrCryptoCallHmacSha1(fastContext->context->cryptoContext, NULL,
            fastContext->tls.imck1+40, 20, tlv, 60, compound_mac);
    CsrMemCpy(pt, compound_mac, 20);

    (void)build_sha1_mac(fastContext,
                         23,
                         plaintext,
                         gtc_response_len,
                         fastContext->tls.keys.client_write_MAC_secret);

    /* Encrypt the data */
    CsrRc4(&fastContext->tls.rc4_client_write_key,
        gtc_response_len + MAC_LENGTH,
        plaintext, fillout);

    return dataref;
}
