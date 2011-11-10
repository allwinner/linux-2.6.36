/** @file eapfast_inband_provisioning.c
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
 *   A suite of functions to handing in-band provisioning messages and
 *   generate responses.
 *   The in-band provisioning proceeds like this:
 *      1) EAP-FAST start message received, containing:
 *                                    AID
 *         AID -> put into eapfast_context
 *         Calculate random client challenge.
 *         Client challenge -> put into fastContext->tls.client_random
 *         Assemble and send provisioning client hello, containing:
 *                                    random client challenge
 *                                    cypher suite: DH_anon_WITH_AES_128_CBC_SHA
 *                                    compression method: none
 *      2) EAP-FAST provisioning server hello received, containing:
 *                                    random server challenge
 *                                    cypher suite: DH_anon_WITH_AES_128_CBC_SHA
 *                                    compression method: none
 *                                    dh_?
 *                                    signature?
 *                                    server hello done
 *         Calculate client private and public DH key pair.
 *         Calculate shared secret? Keys?
 *         Assemble and send client key exchange, containing:
 *                                    client public key
 *                                    encrypted: client finished + MAC
 *      3) EAP-FAST change cypher spec received, containing:
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/fast/eapfast_inband_provisioning.c#2 $
 *
 ****************************************************************************/

#include "eapfast_inband_provisioning.h"
#include "eapfast_authentication.h"
#include "csr_types.h"
#include "sme_trace/sme_trace.h"
#include "csr_crypto.h"

typedef struct provisioning_key_block
{
    CsrUint8 client_write_MAC_secret[MAC_LENGTH]; /*20*/
    CsrUint8 server_write_MAC_secret[MAC_LENGTH]; /*20*/
    CsrUint8 client_write_key[16];
    CsrUint8 server_write_key[16];
    CsrUint8 client_write_IV[16];
    CsrUint8 server_write_IV[16];
    CsrUint8 session_key_seed[40];
    CsrUint8 server_challenge[16];
    CsrUint8 client_challenge[16];
} provisioning_key_block;

/* Correct place for this? */
CsrUint8 inner_method_session_key[32];

/* todo - make local and use TLS keys */
static provisioning_key_block provisioning_kb;

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
DataBuffer build_eapfast_provisioning_client_hello(FastContext *fastContext, CsrUint8 reqId)
{
    CsrUint8 *fillout;
    CsrUint8 *handshake_start;
    CsrUint16 ciphersuite;
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;

    CsrUint16 tls_client_hello_length = sizeof(CsrUint16) /* protocol version */
        + sizeof(Random)
        + 1 /* session id */
        + 4 /* cypher suite */
        + 2 /* compression method */;

    CsrUint16 tls_message_length = 1 /* client hello type field */ + 3 /* handshake length field */ + tls_client_hello_length;

    DataBuffer dataref = { CSR_WIFI_SECURITY_SNAP_LENGTH
                           + EAPOL_HEADER_LENGTH
                           + EAP_REQ_RESP_HEADER_LENGTH
                           + EAPFAST_HEADER_LENGTH,
                           NULL};

    dataref.buf = (CsrUint8 *)eapol;

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_provisioning_client_hello()."));

    dataref.dataLength += 4/* EAPOL's TLS message length field */
        + 5 /* TLS record protocol TLSPlaintext header */
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
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = EAPFAST_LENGTHBIT + EAPFAST_VERSION_NO;

    /*lint -save -e415 -e416 -e419 */

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;
    *fillout++ = 0;
    *fillout++ = 0;
    *fillout++ = 0;
    *fillout++ = (CsrUint8)tls_message_length + 5 /* TLS record protocol TLSPlaintext header */;

    /* Now fill out TLS record protocol TLSPlaintext message.                */
    /* --------------------------------------------------------------------- */

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
    ciphersuite = TLS_DH_anon_WITH_AES_128_CBC_SHA;
    *fillout++ = ciphersuite >> 8;
    *fillout++ = ciphersuite & 0xff;

    /* Compression method field - vector, length field = 8 bits */
    *fillout++ = 1;
    *fillout++ = 0; /* NULL compression only */

    /*lint -restore */

    CsrCryptoCallMd5Update(fastContext->context->cryptoContext, NULL,
                           fastContext->tls.md5_ctx, handshake_start, (CsrUint32)(fillout - handshake_start));
    CsrCryptoCallSha1Update(fastContext->context->cryptoContext, NULL,
                            fastContext->tls.sha1_ctx, handshake_start, (CsrUint32)(fillout - handshake_start));

    return dataref;
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
void process_provisioning_server_hello(FastContext* fastContext)
{
    CsrUint8 pre_master_secret[256];

    CsrUint8 seed[64];

    /* We'll need a public/private key pair for our next message.
     * We can generate this now because we know the server base (dh_g) and prime. */
    CsrCryptoCallDhGenerateKeys(fastContext->context->cryptoContext, NULL,
                                fastContext->tls.dh_g, fastContext->tls.server_prime, 256,
                                fastContext->tls.client_dh_private_key,
                                fastContext->tls.client_dh_public_key, TRUE);

    /* Calculate the pre-master secret as per EAP-FAST spec.:
     * pre-master secret = (DH_Ys[server public key])^(peer-private-DH-key) mod DH_p[prime] */
    CsrCryptoCallDhCalculateSharedSecret(fastContext->context->cryptoContext, NULL,
                                         pre_master_secret,
                                         fastContext->tls.server_dh_public_key,
                                         fastContext->tls.client_dh_private_key,
                                         fastContext->tls.server_prime, 256);

    /* Prepare the seed for the master secret calculation */
    CsrMemCpy(seed, &fastContext->tls.client_random, 32);
    CsrMemCpy(seed+32, &fastContext->tls.server_random, 32);

    /* Calculate the master secret as per EAP-FAST spec.:
     * PRF(pre-master secret, "master secret", client_random + server_random) */
    prf(pre_master_secret, 256,
        "master secret", 13,
        seed, 64,
        fastContext->tls.master_secret /*result*/, 48);

    /* Prepare the seed for the key block calculation */
    CsrMemCpy(seed, &fastContext->tls.server_random, 32);
    CsrMemCpy(seed+32, &fastContext->tls.client_random, 32);

    /* Generate keying material as per EAP-FAST spec.:
     * key_block = PRF(master_secret, "key expansion", server_random + client_random) */
    prf(fastContext->tls.master_secret, 48,
        "key expansion", 13,
        seed, 64,
        (CsrUint8 *)&provisioning_kb /*result*/, sizeof(struct provisioning_key_block)/*176*/);

    /* Copy the key material needed by the TLS routines */
    CsrMemCpy(fastContext->tls.keys.client_write_MAC_secret, provisioning_kb.client_write_MAC_secret,  MAC_LENGTH);
    CsrMemCpy(fastContext->tls.keys.server_write_MAC_secret, provisioning_kb.server_write_MAC_secret,  MAC_LENGTH);
    CsrMemCpy(fastContext->tls.keys.client_write_key, provisioning_kb.client_write_key, 16);
    CsrMemCpy(fastContext->tls.keys.server_write_key, provisioning_kb.server_write_key, 16);
    CsrMemCpy(fastContext->tls.keys.client_write_IV, provisioning_kb.client_write_IV, 16);
    CsrMemCpy(fastContext->tls.keys.server_write_IV, provisioning_kb.server_write_IV, 16);
    CsrMemCpy(fastContext->tls.keys.session_key_seed, provisioning_kb.session_key_seed, 40);

#if 0
    /* Test code using in developing this function - no longer needed.
     * Here for an emergency. */
    CsrUint8 seed[64];
    CsrUint8 master_secret[100];
    CsrUint8 key_block[176];
    CsrUint8 pre_master_secret[256];
    CsrCryptoDhCalculateSharedSecret
        (pre_master_secret, /* result */
         server_dh_public_key, /* dh_ys */
         client_dh_private_key,
         server_prime, /* dh_p */
         256);

    /* 0xab 0xcd 0x49 0xc0 0xb9 0xe2 0x88 0xb9
     * 0x54 0x15 0x1b 0x11 0xe0 0xf1 0xb4 0x26 */

    CsrMemCpy(seed, client_random, 32);
    CsrMemCpy(seed+32, server_random ,32);

    /* prf(pre_master_secret, 256, "master secret", 13, seed, 64, master_secret, 48); */

    /* 0xc3 0x62 0x72 0xcd 0x45 0xc3 0x49 0x5a
     * 0x69 0xce 0x2e 0x0c 0x14 0x17 0xb3 0x9d
     * 0x67 0x0c 0x95 0x5f 0x7f 0x72 0x3e 0x6f
     * 0xc1 0x24 0xbf 0x93 0x28 0x46 0xf6 0xcf
     * 0x58 0xbd 0xd6 0x8d 0x5d 0xe0 0xd5 0x8a */

    CsrMemCpy(seed, server_random, 32);
    CsrMemCpy(seed+32, client_random, 32);

    /* prf(master_secret, 48, "key expansion", 13, seed, 64, key_block, 176); */
    sme_trace_info((TR_SECURITY_LIB, "%d.", key_block[0]));
#endif
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
DataBuffer build_eapfast_provisioning_client_key_exchange(FastContext* fastContext, CsrUint8 reqId)
{
    CsrUint8 *fillout;
    CsrUint8 *handshake_start;
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    CsrUint8 *pt = NULL;
    CsrUint8 plaintext[200];
    CsrUint8 seed[CSR_CRYPTO_MD5_DIGEST_LENGTH + CSR_SHA1_DIGEST_LENGTH];
    CsrUint16 tls_client_key_exchange_length = 4 /*TLS message header*/ + 2 /*CKE length field*/ + 256 /*dh_Yc*/;
    CsrUint16 tls_finished_length = 4 /*TLS message header*/ + 12 /*finshed length*/;
    CsrUint16 total_TLV_length = 5 /* TLS record protocol TLSPlaintext header */
                           + tls_client_key_exchange_length
                           + 5 /* TLS record protocol TLSPlaintext header */
                           + 1 /* Change Cypher Spec */
                           + 5 /* TLS record protocol TLSPlaintext header */
                           + tls_finished_length
                           + MAC_LENGTH
                           + 0x0c /* padding for 128 bit block cypher */;
    DataBuffer dataref = {0, NULL};

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_provisioning_client_key_exchange()."));

    dataref.buf = (CsrUint8 *)eapol;

    dataref.dataLength =
          CSR_WIFI_SECURITY_SNAP_LENGTH
        + EAPOL_HEADER_LENGTH
        + EAP_REQ_RESP_HEADER_LENGTH
        + EAPFAST_HEADER_LENGTH
        + 4 /* EAP-FAST header TLV length field */
        + total_TLV_length;

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
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = EAPFAST_LENGTHBIT + EAPFAST_VERSION_NO;

    /*lint -save -e415 -e416 -e419 */

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;
    *fillout++ = 0;
    *fillout++ = 0;
    *fillout++ = total_TLV_length >> 8;
    *fillout++ = total_TLV_length & 0xff;

    /* Fill out first TLS record protocol TLSPlaintext message.
     * (client key exchange)
     * --------------------------------------------------------------------- */

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 22; /* handshake */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = tls_client_key_exchange_length>>8;
    *fillout++ = tls_client_key_exchange_length & 0xff;

    /* Handshake message begins here */
    handshake_start = fillout;

    /* Client Key Exchange */
    *fillout++ = 16; /* type = client key exchange */

    /* length of key change */
    *fillout++ = 0;
    *fillout++ = 1;
    *fillout++ = 2;

    /* Client public DH key (dh_Yc) */
    *fillout++ = 1;     /* length = 256 */
    *fillout++ = 0;
    CsrMemCpy(fillout, fastContext->tls.client_dh_public_key, 256);
    fillout += 256;

    CsrCryptoCallMd5Update(fastContext->context->cryptoContext, NULL,
                           fastContext->tls.md5_ctx, handshake_start, (CsrUint32)(fillout - handshake_start));
    CsrCryptoCallSha1Update(fastContext->context->cryptoContext, NULL,
                            fastContext->tls.sha1_ctx, handshake_start, (CsrUint32)(fillout - handshake_start));

    /* Fill out Change Cypher Spec message (non-handshake).
     * --------------------------------------------------------------------- */
    *fillout++ = 20; /* change cypher */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = 0; /* length (u16) */
    *fillout++ = 1;
    *fillout++ = 1; /* fixed value */

    /* Fill out second TLS record protocol TLSPlaintext message.
     * (finished)
     * --------------------------------------------------------------------- */

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 22; /* handshake */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = 0; /* length (u16) */
    *fillout++ = 4 + 12 + MAC_LENGTH
        + 0x0c/* padding for 128 bit block cypher */;

    /*lint -restore */

    /* The rest of the data is to be encrypted, so build the temporary
     * plaintext in the "plaintext" array. */
    pt = plaintext;

    /* Handshake message begins here */
    handshake_start = pt;

    /* finished header */
    *pt++ = 20; /* type = finished */
    *pt++ = 0;  /* length (uint24) */
    *pt++ = 0;
    *pt++ = 0x0c;

    /* Calculate "finished" data as per RFC-2246, thus:
     * PRF(master_secret, "client finished", MD5(handshake_messages) + SHA1(handshake_messages)
     * "handshake_messages" is all handshake data up to but not including this message. */

    /* Define this to use CCX test vector data instead.
     * This will break authentication, but it will generate the encrypted section of the message
     * in the CCX spec, to verify the encryption process.
     *
     *#define CCX_TEST_DATA 1*/

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
        pt /*result*/, 12);

    pt += 12;

    /* Is this the correct place for setting this? */
    CsrMemSet(fastContext->tls.write_seq_no, 0, 8);

    CsrCryptoCallMd5Update(fastContext->context->cryptoContext, NULL,
                           fastContext->tls.md5_ctx, handshake_start, (CsrUint32)(pt - handshake_start));
    CsrCryptoCallSha1Update(fastContext->context->cryptoContext, NULL,
                            fastContext->tls.sha1_ctx, handshake_start, (CsrUint32)(pt - handshake_start));

    /* Fill out the MAC.
     * --------------------------------------------------------------------- */
    (void)build_sha1_mac(fastContext, 22, plaintext, 4+12, provisioning_kb.client_write_MAC_secret);
    pt += MAC_LENGTH;

    {
        CsrUint32 outLength;

        CsrCryptoAes128CbcEncrypt(provisioning_kb.client_write_key, provisioning_kb.client_write_IV,
            plaintext, (CsrUint32)(pt - plaintext), fillout, &outLength, PAD_MODE_TLS);

        fillout += outLength;
     }

    return dataref;
}


/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
DataBuffer build_eapfast_provisioning_client_ack(FastContext* fastContext, CsrUint8 reqId)
{
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    DataBuffer dataref = {0, NULL};

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_provisioning_client_ack()."));

    dataref.buf = (CsrUint8 *)eapol;

    dataref.dataLength =
          CSR_WIFI_SECURITY_SNAP_LENGTH
        + EAPOL_HEADER_LENGTH
        + EAP_REQ_RESP_HEADER_LENGTH
        + 1 /* ack success code */;

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
    eapol->u.eap.u.resp.u.data = 1;

    return dataref;
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
DataBuffer build_eapfast_provisioning_id_resp(FastContext* fastContext, CsrUint8 reqId)
{
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    DataBuffer dataref = {0, NULL};
    CsrUint8 *fillout;
    CsrUint16 encapsulated_tlv_length;
    CsrUint8 plaintext[200];
    CsrUint8 *pt = NULL;
    CsrUint8 padding_length;

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_provisioning_id_resp()."));


    encapsulated_tlv_length =
          4 /* EAP Payload TLV header */
        + 5 /* TLS id response header */
        + (CsrUint16)CsrStrLen(fastContext->context->setupData.username)
        + MAC_LENGTH;



    /* Pad to 16-byte size, for block cypher. */
    padding_length = 16 - (encapsulated_tlv_length & 0x0f);
    encapsulated_tlv_length += padding_length;

    dataref.buf = (CsrUint8 *)eapol;

    dataref.dataLength =
          CSR_WIFI_SECURITY_SNAP_LENGTH
        + EAPOL_HEADER_LENGTH
        + EAP_REQ_RESP_HEADER_LENGTH
        + EAPFAST_HEADER_LENGTH
        + 4 /* EAP-FAST header TLV length field */
        + 5 /* TLS record protocol TLSPlaintext header */
        + encapsulated_tlv_length;

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
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = EAPFAST_LENGTHBIT + EAPFAST_VERSION_NO;

    /*lint -save -e415 -e416 */

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;
    *fillout++ = 0;
    *fillout++ = 0;
    *fillout++ = (encapsulated_tlv_length + 5) >> 8;
    *fillout++ = (encapsulated_tlv_length + 5) & 0xff;

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x17; /* application data */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = encapsulated_tlv_length>>8;
    *fillout++ = encapsulated_tlv_length & 0xff;

    /*lint -restore */

    /* The rest of the data is to be encrypted, so build the temporary
     * plaintext in the "plaintext" array. */
    pt = plaintext;

    /* Payload TLV header */
    *pt++ = 0x80; /* fixed value */
    *pt++ = 9;  /* TLV type = application data */
    *pt++ = ((5 + CsrStrLen(fastContext->context->setupData.username)) >> 8) & 0xff;
    *pt++ = (5 + CsrStrLen(fastContext->context->setupData.username)) & 0xff;

    /* Eap id response */
    *pt++ = EAP_CODE_RESP; /* fixed value */
    *pt++ = reqId;  /* TLV type = application data */ /* Todo: Eeek! Sub id value? */
    *pt++ = ((5 + CsrStrLen(fastContext->context->setupData.username)) >> 8) & 0xff;
    *pt++ = (5 + CsrStrLen(fastContext->context->setupData.username)) & 0xff;
    *pt++ = EAP_TYPE_IDENTITY;

    /* username */
    CsrStrCpy((char *)pt, (char *)fastContext->context->setupData.username);
    pt += CsrStrLen(fastContext->context->setupData.username); /* no zero terminator? */;

    /* Fill out the MAC.
     * --------------------------------------------------------------------- */
    (void)build_sha1_mac(fastContext, 23, plaintext, (CsrUint32)(pt - plaintext), provisioning_kb.client_write_MAC_secret);
    pt += MAC_LENGTH;

    {
        CsrUint32 outLength;

        CsrCryptoAes128CbcEncrypt(provisioning_kb.client_write_key, provisioning_kb.client_write_IV,
            plaintext, (CsrUint32)(pt - plaintext), fillout, &outLength, PAD_MODE_TLS);

        fillout += outLength;
     }

    return dataref;
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
DataBuffer build_eapfast_provisioning_mschap_resp(FastContext* fastContext, CsrUint8 reqId)
{
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    DataBuffer dataref = {0, NULL};
    CsrUint16 payload_tlv_length;
    CsrUint16 encapsulated_tlv_length;
    CsrUint8 padding_length;
    CsrUint8 plaintext[200];
    CsrUint8 *pt = NULL;
    CsrUint8 *fillout;

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_provisioning_id_resp()."));

    payload_tlv_length =
          5 /* TLS MSCHAPv2 response header */
        + 5 + 16 + 8 + 24 + 1 + (CsrUint16)CsrStrLen(fastContext->context->setupData.username); /*fields of the MSCHAPv2 response*/

    encapsulated_tlv_length =
          4 /* EAP Payload TLV header */
        + payload_tlv_length
        + MAC_LENGTH;

    /* Pad to 16-byte size, for block cypher. */
    padding_length = 16 - (encapsulated_tlv_length & 0x0f);
    encapsulated_tlv_length += padding_length;

    dataref.buf = (CsrUint8 *)eapol;

    dataref.dataLength =
          CSR_WIFI_SECURITY_SNAP_LENGTH
        + EAPOL_HEADER_LENGTH
        + EAP_REQ_RESP_HEADER_LENGTH
        + EAPFAST_HEADER_LENGTH
        + 4 /* EAP-FAST header TLV length field */
        + 5 /* TLS record protocol TLSPlaintext header */
        + encapsulated_tlv_length;

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
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = EAPFAST_LENGTHBIT + EAPFAST_VERSION_NO;

    /*lint -save -e415 -e416 */

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;
    *fillout++ = 0;
    *fillout++ = 0;
    *fillout++ = ((encapsulated_tlv_length + 5) >> 8) & 0xff;
    *fillout++ = (encapsulated_tlv_length + 5) & 0xff;

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x17; /* application data */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = (encapsulated_tlv_length >> 8) & 0xff;
    *fillout++ = encapsulated_tlv_length & 0xff;

    /*lint -restore */

    /* The rest of the data is to be encrypted, so build the temporary
     * plaintext in the "plaintext" array. */
    pt = plaintext;

    /* Payload TLV header */
    *pt++ = 0x80; /* fixed value */
    *pt++ = 9;  /* TLV type = application data */
    *pt++ = (payload_tlv_length >> 8) & 0xff;
    *pt++ = payload_tlv_length & 0xff;

    /* Eap MSCHAPv2 response */
    *pt++ = EAP_CODE_RESP; /* fixed value */
    *pt++ = reqId;  /* TLV type = application data */
    *pt++ = (payload_tlv_length >> 8) & 0xff;
    *pt++ = payload_tlv_length & 0xff;
    *pt++ = EAP_TYPE_MSCHAPV2;

    /* Eap MSCHAPv2 challenge response */
    *pt++ = EAP_CODE_RESP; /* fixed value */
    *pt++ = reqId;  /* TLV type = application data */
    *pt++ = ((CsrUint16)(payload_tlv_length-5) >> 8) & 0xff;
    *pt++ = (payload_tlv_length-5) & 0xff;
    *pt++ = 49; /* response length */
    CsrMemSet(pt, 0, 16+8); /*challenge and reserved fields*/
    pt += 16+8;

    CsrCryptoCallMschapGenerateNTResponseAndSessionKey(fastContext->context->cryptoContext, NULL,
                                    provisioning_kb.server_challenge,
                                    provisioning_kb.client_challenge,
                                    (CsrUint8 *)fastContext->context->setupData.username,
                                    (CsrUint8 *)fastContext->context->setupData.password,
                                    pt,
                                    inner_method_session_key);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "build_eapfast_provisioning_mschap_resp :: inner_method_session_key",
            inner_method_session_key, 32));

    pt += 24;
    *pt++ = 0; /*flag*/
    CsrStrCpy((char *)pt, (char *)fastContext->context->setupData.username); /*username*/
    pt += CsrStrLen(fastContext->context->setupData.username); /* no zero terminator? */;

    /* Fill out the MAC.
     * --------------------------------------------------------------------- */
    (void)build_sha1_mac(fastContext, 23, plaintext, (CsrUint32)(pt - plaintext), provisioning_kb.client_write_MAC_secret);
    pt += MAC_LENGTH;

    {
        CsrUint32 outLength;

        CsrCryptoAes128CbcEncrypt(provisioning_kb.client_write_key, provisioning_kb.client_write_IV,
            plaintext, (CsrUint32)(pt - plaintext), fillout, &outLength, PAD_MODE_TLS);
     }

    return dataref;
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
DataBuffer build_eapfast_provisioning_mschap_success_response(FastContext* fastContext, CsrUint8 reqId)
{
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    DataBuffer dataref = {0, NULL};
    CsrUint16 payload_tlv_length;
    CsrUint16 encapsulated_tlv_length;
    CsrUint8 padding_length;
    CsrUint8 plaintext[200];
    CsrUint8 *pt = NULL;
    CsrUint8 *fillout;

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_provisioning_mschap_success_response()."));

    payload_tlv_length =
          5  /* TLS MSCHAPv2 response header */
        + 1; /* Opcode */

    encapsulated_tlv_length =
          4 /* EAP Payload TLV header */
        + payload_tlv_length
        + MAC_LENGTH;

    /* Pad to 16-byte size, for block cipher. */
    padding_length = 16 - (encapsulated_tlv_length & 0x0f);
    encapsulated_tlv_length += padding_length;

    dataref.buf = (CsrUint8 *)eapol;

    dataref.dataLength =
          CSR_WIFI_SECURITY_SNAP_LENGTH
        + EAPOL_HEADER_LENGTH
        + EAP_REQ_RESP_HEADER_LENGTH
        + EAPFAST_HEADER_LENGTH
        + 4 /* EAP-FAST header TLV length field */
        + 5 /* TLS record protocol TLSPlaintext header */
        + encapsulated_tlv_length;

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
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = EAPFAST_LENGTHBIT + EAPFAST_VERSION_NO;

    /*lint -save -e415 -e416 */

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;
    *fillout++ = 0;
    *fillout++ = 0;
    *fillout++ = ((encapsulated_tlv_length + 5) >> 8) & 0xff;
    *fillout++ = (encapsulated_tlv_length + 5) & 0xff;

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x17; /* application data */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = (encapsulated_tlv_length >> 8) & 0xff;
    *fillout++ = encapsulated_tlv_length & 0xff;

    /*lint -restore */

    /* The rest of the data is to be encrypted, so build the temporary
     * plaintext in the "plaintext" array. */
    pt = plaintext;

    /* Payload TLV header */
    *pt++ = 0x80; /* fixed value */
    *pt++ = 9;  /* TLV type = application data */
    *pt++ = (payload_tlv_length >> 8) & 0xff;
    *pt++ = payload_tlv_length & 0xff;

    /* Eap MSCHAPv2 success response */
    *pt++ = EAP_CODE_RESP; /* fixed value */
    *pt++ = reqId;  /* TLV type = application data */
    *pt++ = (payload_tlv_length >> 8) & 0xff;
    *pt++ = payload_tlv_length & 0xff;
    *pt++ = EAP_TYPE_MSCHAPV2;
    *pt++ = EAP_CODE_SUCCESS; /* fixed value */

    /* Fill out the MAC.
     * --------------------------------------------------------------------- */
    (void)build_sha1_mac(fastContext, 23, plaintext, (CsrUint32)(pt - plaintext), provisioning_kb.client_write_MAC_secret);
    pt += MAC_LENGTH;

    {
        CsrUint32 outLength;

        CsrCryptoAes128CbcEncrypt(provisioning_kb.client_write_key, provisioning_kb.client_write_IV,
            plaintext, (CsrUint32)(pt - plaintext), fillout, &outLength, PAD_MODE_TLS);
     }

    return dataref;
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
DataBuffer build_eapfast_provisioning_intermediate_response(FastContext* fastContext, CsrUint8 reqId)
{
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    DataBuffer dataref = {0, NULL};
    CsrUint16 intermediate_result_tlv_length;
    CsrUint16 crypto_bind_tlv_length;
    CsrUint16 total_application_data_length;
    CsrUint8 padding_length;
    CsrUint8 plaintext[200];
    CsrUint8 *pt = NULL;
    CsrUint8 *fillout;
    CsrUint8 *tlv = NULL;
    CsrUint32 i;
    CsrUint8 compound_mac[CSR_SHA1_DIGEST_LENGTH];

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_provisioning_intermediate_response()."));

    intermediate_result_tlv_length =
          4 /* EAP Payload TLV header */
        + 2; /* Result code */

    crypto_bind_tlv_length =
          4 /* EAP Payload TLV header */
        + 56 /* crypto-bind length */;

    /* Pad to 16-byte size, for block cipher. */
    padding_length = 16 - ((intermediate_result_tlv_length + crypto_bind_tlv_length + MAC_LENGTH) & 0x0f);

    total_application_data_length =
          intermediate_result_tlv_length
        + crypto_bind_tlv_length
        + MAC_LENGTH
        + padding_length;

    dataref.buf = (CsrUint8 *)eapol;

    dataref.dataLength =
          CSR_WIFI_SECURITY_SNAP_LENGTH
        + EAPOL_HEADER_LENGTH
        + EAP_REQ_RESP_HEADER_LENGTH
        + EAPFAST_HEADER_LENGTH
        + 4 /* EAP-FAST header TLV length field */
        + 5 /* TLS record protocol TLSPlaintext header */
        + total_application_data_length;

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
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = EAPFAST_LENGTHBIT + EAPFAST_VERSION_NO;

    /*lint -save -e415 -e416 */

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;
    *fillout++ = 0;
    *fillout++ = 0;
    *fillout++ = ((total_application_data_length + 5) >> 8) & 0xff;
    *fillout++ = (total_application_data_length + 5) & 0xff;

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x17; /* application data */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = (total_application_data_length >> 8) & 0xff;
    *fillout++ = total_application_data_length & 0xff;

    /*lint -restore */

    /* The rest of the data is to be encrypted, so build the temporary
     * plaintext in the "plaintext" array. */
    pt = plaintext;

    /* Intermediate result TLV */
    *pt++ = 0x80; /* fixed value */
    *pt++ = 10;  /* TLV type = intermediate result */
    *pt++ = 0; /* length - hi */
    *pt++ = 2; /* length - low */
    *pt++ = 0; /* Result - 0x0001 = success */
    *pt++ = 1;

    /* Crypto binding TLV */
    tlv = pt;     /* Start of compound MAC data */
    *pt++ = 0x80; /* fixed value */
    *pt++ = 12;   /* TLV type = crypto binding */
    *pt++ = 0;    /* length - hi */
    *pt++ = 56;   /* length - low */
    *pt++ = 0;    /* reserved */
    *pt++ = 1;    /* crypto binding version */
    *pt++ = 1;    /* EAP-FAST version */
    *pt++ = 1;    /* type 0 = binding response */
    CsrMemCpy(pt, fastContext->tls.nonce, 32); /* Nonce */
    for (i=32; i>0; i--)       /* Increment Nonce */
        if (++pt[i - 1] != 0) break;
    pt += 32;

    /* Add Compound MAC */
    CsrMemSet(pt, 0, CSR_SHA1_DIGEST_LENGTH);
    CsrCryptoCallHmacSha1(fastContext->context->cryptoContext, NULL,
            fastContext->tls.imck1+40, 20, tlv, 60, compound_mac);

    CsrMemCpy(pt, compound_mac, 20);
    pt += CSR_SHA1_DIGEST_LENGTH;

    /* Fill out the MAC.
     * --------------------------------------------------------------------- */
    (void)build_sha1_mac(fastContext,
                         23             /* message type */,
                         plaintext      /* data */,
                         (CsrUint32)(pt - plaintext) /* length */,
                         provisioning_kb.client_write_MAC_secret /* write MAC secret */);
    pt += MAC_LENGTH;

    {
        CsrUint32 outLength;

        CsrCryptoAes128CbcEncrypt(provisioning_kb.client_write_key, provisioning_kb.client_write_IV,
            plaintext, (CsrUint32)(pt - plaintext), fillout, &outLength, PAD_MODE_TLS);
     }

    return dataref;
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
DataBuffer build_eapfast_provisioning_pac_ack(FastContext* fastContext, CsrUint8 reqId)
{
    eapol_packet *eapol = (eapol_packet *)fastContext->context->buffer;
    DataBuffer dataref = {0, NULL};
    CsrUint8 plaintext[200];
    CsrUint8 *pt = NULL;
    CsrUint16 result_tlv_length;
    CsrUint16 pac_ack_length;
    CsrUint16 total_application_data_length;
    CsrUint8 padding_length;
    CsrUint8 *fillout;

    sme_trace_entry((TR_SECURITY_LIB, "build_eapfast_provisioning_pac_ack()."));

    result_tlv_length =
          4 /* EAP Payload TLV header */
        + 2; /* Result code */

    pac_ack_length =
          4 /* EAP Payload TLV header */
        + 6 /* PAC ack TLV */;

    /* Pad to 16-byte size, for block cipher. */
    padding_length = 16 - ((result_tlv_length + pac_ack_length + MAC_LENGTH) & 0x0f);

    total_application_data_length =
          result_tlv_length
        + pac_ack_length
        + MAC_LENGTH
        + padding_length;

    dataref.buf = (CsrUint8 *)eapol;

    dataref.dataLength =
          CSR_WIFI_SECURITY_SNAP_LENGTH
        + EAPOL_HEADER_LENGTH
        + EAP_REQ_RESP_HEADER_LENGTH
        + EAPFAST_HEADER_LENGTH
        + 4 /* EAP-FAST header TLV length field */
        + 5 /* TLS record protocol TLSPlaintext header */
        + total_application_data_length;

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
    ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->flags = EAPFAST_LENGTHBIT + EAPFAST_VERSION_NO;

    /*lint -save -e415 -e416 */

    /* EAP-FAST TLS message length field */
    fillout = ((eapfast_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;
    *fillout++ = 0;
    *fillout++ = 0;
    *fillout++ = ((total_application_data_length + 5) >> 8) & 0xff;
    *fillout++ = (total_application_data_length + 5) & 0xff;

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x17; /* application data */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = (total_application_data_length >> 8) & 0xff;
    *fillout++ = total_application_data_length & 0xff;

    /*lint -restore */

    /* The rest of the data is to be encrypted, so build the temporary
     * plaintext in the "plaintext" array. */
    pt = plaintext;

    /* Result TLV */
    *pt++ = 0x80; /* fixed value */
    *pt++ = 3;  /* TLV type = result */
    *pt++ = 0; /* length - hi */
    *pt++ = 2; /* length - low */
    *pt++ = 0; /* Result - 0x0001 = success */
    *pt++ = 1;

    /* PAC ack TLV */
    *pt++ = 0x80; /* fixed value */
    *pt++ = 0x0b;  /* TLV type = PAC ack */
    *pt++ = 0; /* length - hi */
    *pt++ = 6; /* length - low */
    *pt++ = 0; /* PAC ack */
    *pt++ = 8;
    *pt++ = 0; /* length */
    *pt++ = 2;
    *pt++ = 0; /* success */
    *pt++ = 1;

    /* Fill out the MAC.
     * --------------------------------------------------------------------- */
    (void)build_sha1_mac(fastContext,
                         23             /* message type */,
                         plaintext      /* data */,
                         (CsrUint32)(pt - plaintext) /* length */,
                         provisioning_kb.client_write_MAC_secret /* write MAC secret */);
    pt += MAC_LENGTH;

    {
        CsrUint32 outLength;

        CsrCryptoAes128CbcEncrypt(provisioning_kb.client_write_key, provisioning_kb.client_write_IV,
            plaintext, (CsrUint32)(pt - plaintext), fillout, &outLength, PAD_MODE_TLS);
     }

    return dataref;
}


/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
DataBuffer build_eapfast_poke_start(CsrUint8 reqId)
{
    DataBuffer dataref = {0, NULL};

    /* NOT CURRENTLY USED. CISCO KIT CAN'T BE POKED INTO DOING AN AUTHENTICATION
     * AFTER PROVISIONING WITHOUT RESTARTING THE 802.11 LEVEL AUTHENTICATION. */

    return dataref;
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
CsrBool tls_provisioning_callback(void* fastContext, CsrUint8 **data, CsrUint16 size, CsrUint32 *update_flags, tls_data *tls)
{
    CsrUint8 *scan = *data;
    CsrUint8 tlv_type;
    CsrUint16 tlv_length;
    CsrBool cont = TRUE; /* continue processing? */

    sme_trace_info((TR_SECURITY_LIB, "tls_provisioning_callback :: size = %d", size));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "tls_provisioning_callback :: scan", scan, size));

    /* EAP-FAST application-specific data is stored in a series of Type-Length-Value
     * structures.
     * Loop examining each. */
    sme_trace_info((TR_SECURITY_LIB, "tls_provisioning_callback()"));
    while(cont)
    {
        if (*scan++ != 0x80)
        {
            sme_trace_error((TR_SECURITY_LIB, "tls_provisioning_callback(). Bad payload."));
            return FALSE;
        }

        tlv_type = *scan++;
        tlv_length = (scan[0] << 8) + scan[1];
        scan += 2;

        switch(tlv_type)
        {
        case 9/*EAP Payload TLV*/:
            if (*scan++ != EAP_CODE_REQ)
            {
                sme_trace_error((TR_SECURITY_LIB, "tls_provisioning_callback(). Bad payload TLV."));
                return FALSE;
            }

            tls->gtc_reqId = *scan++;

            if ( (scan[0] << 8) + scan[1] != tlv_length )
            {
                sme_trace_error((TR_SECURITY_LIB, "tls_provisioning_callback(). Bad payload TLV length."));
                return FALSE;
            }
            scan += 2;

            switch(*scan++) /* EAP-TYPE field */
            {
            case EAP_TYPE_IDENTITY:
                sme_trace_info((TR_SECURITY_LIB, "tls_provisioning_callback() :: EAP_TYPE_IDENTITY"));
                *update_flags |= ENCAPSULATED_ID_REQ_INCLUDED;
                break;
            case EAP_TYPE_MSCHAPV2:
                sme_trace_info((TR_SECURITY_LIB, "tls_provisioning_callback() :: EAP_TYPE_MSCHAPV2"));
                switch(*scan) /*opcode field*/
                {
                case 1: /*challenge*/
                    /* We don't actually extract the challenge data here. EAP-FAST zeros
                     * it out and uses the server_challenge derived from an earlier part
                     * of the provisioning exchange (it is in the key block).
                     * Just mark the challenge received and skip over it. */
                    *update_flags |= ENCAPSULATED_MSCHAPv2_CHALLENGE_INCLUDED;
                    break;
                case 3: /*success*/
                    *update_flags |= ENCAPSULATED_MSCHAPV2_SUCCESS_INCLUDED;
                    break;
                default:
                    sme_trace_error
                        ((TR_SECURITY_LIB,
                          "tls_provisioning_callback(). Unexpected MSCHAPv2 Opcode (%d).",
                          *scan));
                    return FALSE;
                }
                scan += tlv_length - 5 /* length of TLV header */;
                break;
            case EAP_TYPE_GTC:
                sme_trace_info((TR_SECURITY_LIB, "tls_provisioning_callback() :: EAP_TYPE_GTC"));
                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "tls_provisioning_callback :: EAP_TYPE_GTC text", scan, (CsrUint32)((*data + size) - scan)));
                /* Skip over the GTC text */
                scan = *data + size;
                break;
            default:
                sme_trace_error
                    ((TR_SECURITY_LIB,
                      "tls_provisioning_callback(). EAP type (%d) not supported.",
                      *(scan-1)));
                return FALSE;
            } /* switch(*scan++) */
            cont = FALSE;
            break;
        case 10/*Intermediate Result TLV*/:
            if ((*scan!=0) || (*(scan+1) != 1))
            {
                sme_trace_error
                    ((TR_SECURITY_LIB,
                      "tls_provisioning_callback(). Bad intermediate results."));
                return FALSE;
            }
            *update_flags |= ENCAPSULATED_INTERMEDIATE_RESULT_INCLUDED;
            scan += 2;
            cont = TRUE; /* There will be a crypto bind following this TLV. */
            break;
        case 3/*EAP Result TLV*/:
            scan++;
            if (*scan++ != 1/*success*/)
            {
                sme_trace_error
                    ((TR_SECURITY_LIB,
                      "tls_provisioning_callback(). Authentication failure code."));
                return FALSE;
            }
            *update_flags |= ENCAPSULATED_RESULT_INCLUDED;
            cont = TRUE;
            break;
        case 12/*Crypto binding TLV*/:
            if ( (tlv_length != 56) ||
                 /* Error in EAP-FAST spec: (*scan_decyphered++ != 0) || - reserved */
                 (*scan++ != 0) ||   /*crypto binding version hi*/
                 (*scan++ != 1) ||   /*crypto binding version low*/
                 (*scan++ != 1) ||   /*eapfast version*/
                 (*scan++ != 0)      /*binding request*/
                )
            {
                sme_trace_error
                    ((TR_SECURITY_LIB,
                      "tls_provisioning_callback(). Crypto binding TLV incorrect."));
                return FALSE;
            }
            CsrMemCpy(tls->crypto_binding_tlv, scan-8, 60);
            CsrMemCpy(tls->nonce, scan, 32);
            scan += 32/*nonce*/;

            /* Verify the compound MAC */
            {
                CsrUint8 compound_mac[CSR_SHA1_DIGEST_LENGTH];
                CsrUint8 old_compound_mac[CSR_SHA1_DIGEST_LENGTH];
                CsrMemCpy(old_compound_mac, ((FastContext *)fastContext)->tls.crypto_binding_tlv+40, CSR_SHA1_DIGEST_LENGTH);
                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "tls_provisioning_callback :: old_compound_mac", old_compound_mac, CSR_SHA1_DIGEST_LENGTH));
                CsrMemSet(((FastContext*)fastContext)->tls.crypto_binding_tlv+40, 0, CSR_SHA1_DIGEST_LENGTH);

                t_prf(((FastContext *)fastContext)->tls.keys.session_key_seed, 40, /* key, len */
                      "Inner Methods Compound Keys", 27,             /* label, len */
                      inner_method_session_key, 32,                  /* seed, len */
                      ((FastContext *)fastContext)->tls.imck1, 60);                /* output, len */

                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "tls_provisioning_callback :: inner_method_session_key",
                        inner_method_session_key, 32));
                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "tls_provisioning_callback :: crypto_binding_tlv",
                        ((FastContext *)fastContext)->tls.crypto_binding_tlv, 60));

                CsrCryptoCallHmacSha1(((FastContext *)fastContext)->context->cryptoContext, NULL,
                                      ((FastContext *)fastContext)->tls.imck1+40, 20,
                                      ((FastContext *)fastContext)->tls.crypto_binding_tlv, 60,
                                      compound_mac);

                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "tls_provisioning_callback :: compound_mac", compound_mac, CSR_SHA1_DIGEST_LENGTH));
                if (CsrMemCmp(old_compound_mac, compound_mac, CSR_SHA1_DIGEST_LENGTH) != 0)
                {
                    sme_trace_error((TR_SECURITY_LIB, "tls_provisioning_callback(): Compound MAC failure."));

                    /* EAP-GTC uses zeros for the session key, so try that as an interim solution */
                    CsrMemSet(inner_method_session_key, 0, 32);
                    t_prf(((FastContext *)fastContext)->tls.keys.session_key_seed, 40, /* key, len */
                          "Inner Methods Compound Keys", 27,             /* label, len */
                          inner_method_session_key, 32,                  /* seed, len */
                          ((FastContext *)fastContext)->tls.imck1, 60);                /* output, len */

                    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "tls_provisioning_callback :: inner_method_session_key",
                            inner_method_session_key, 32));
                    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "tls_provisioning_callback :: crypto_binding_tlv",
                            ((FastContext *)fastContext)->tls.crypto_binding_tlv, 60));

                    CsrCryptoCallHmacSha1(((FastContext *)fastContext)->context->cryptoContext, NULL,
                                          ((FastContext *)fastContext)->tls.imck1+40, 20,
                                          ((FastContext *)fastContext)->tls.crypto_binding_tlv, 60,
                                          compound_mac);


                    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_ERROR, "tls_provisioning_callback :: compound_mac", compound_mac, CSR_SHA1_DIGEST_LENGTH));
                    if (CsrMemCmp(old_compound_mac, compound_mac, CSR_SHA1_DIGEST_LENGTH) != 0)
                    {
                        sme_trace_error((TR_SECURITY_LIB, "tls_provisioning_callback(): EAP-GTC Compound MAC failure."));
                        return FALSE;
                    }
                }
            }

            scan += MAC_LENGTH/*Compound MAC*/;
            *update_flags |= ENCAPSULATED_CRYPTO_BINDING_NONCE_INCLUDED;
            cont = FALSE;
            break;
        case 11/*PAC TLV*/:
            {
                CsrUint8 *end = scan + tlv_length;
                while(scan < end)
                {

                    if (*scan++ != 0)
                    {
                        sme_trace_error((TR_SECURITY_LIB, "tls_provisioning_callback(): PAC_TLV failure."));
                        return FALSE;
                    }

                    switch(*scan++) /* PAC TLV element type */
                    {
                    case 1/*PAC-KEY*/:
                        {
                            CsrUint16 key_len = (scan[0]<<8) + scan[1];
                            scan += 2;
                            sme_trace_info((TR_SECURITY_LIB, "tls_provisioning_callback(): storing PAC-KEY"));
                            CsrMemCpy(tls->PAC_key, scan, key_len);
                            *update_flags |= PAC_KEY_INCLUDED;
                            scan += key_len;
                        }
                        break;
                    case 2/*PAC-OPAQUE*/:
                        sme_trace_info((TR_SECURITY_LIB, "tls_provisioning_callback(): storing PAC-OPAQUE"));
                        ((FastContext *)fastContext)->PAC_opaque_length = (scan[0]<<8) + scan[1];
                        scan += 2;
                        CsrMemCpy(((FastContext *)fastContext)->PAC_opaque, scan, ((FastContext *)fastContext)->PAC_opaque_length);
                        *update_flags |= PAC_OPAQUE_INCLUDED;
                        scan += ((FastContext *)fastContext)->PAC_opaque_length;
                        break;
                    case 9/*PAC-INFO*/:
                        {
                            CsrUint8 *info_end;
                            CsrUint16 info_len = (scan[0]<<8) + scan[1];
                            scan += 2;
                            info_end = scan + info_len;
                            while(scan < info_end)
                            {
                                if (*scan++ != 0)
                                {
                                    sme_trace_error((TR_SECURITY_LIB, "tls_provisioning_callback(): PAC-INFO failure."));
                                    return FALSE;
                                }
                                switch(*scan++)
                                {
                                case 4 /* A-ID */:
                                    {
                                        CsrUint16 aid_len = (scan[0]<<8) + scan[1];
                                        scan += 2;
                                        sme_trace_info((TR_SECURITY_LIB, "tls_provisioning_callback(): storing A-ID"));
                                        CsrMemCpy(((FastContext *)fastContext)->aid, scan, aid_len);
                                        *update_flags |= PAC_AID_INCLUDED;
                                        scan += aid_len;
                                    }
                                    break;
                                default:
                                    scan += (scan[0]<<8) + scan[1] + 2/*length field*/;
                                    break;
                                }
                            }
                        }
                        break;
                    default:
                        /* jump over other types of PAC TLV elements */
                        scan += (scan[0]<<8) + scan[1] + 2/*length field*/;
                        break;
                    }
                }

                cont = FALSE;
            } /* case 11: PAC TLV */
            break;
        default:
            sme_trace_error
                ((TR_SECURITY_LIB,
                  "tls_provisioning_callback(). Unrecognized TLV type."));
            return FALSE;
        } /* switch(tlv_type) */

    } /* while(cont) */

    *data = scan;
    return TRUE;
}
