/** @file eaptls_utils.c
 *
 * Implementation of utilities for the EAP-TLS method.
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
 *   This provides an implementation of utilities for the EAP-TLS authentication protocol.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/tls/eaptls_utils.c#1 $
 *
 ****************************************************************************/

#include "eaptls_utils.h"
#include "sme_trace/sme_trace.h"
#include "csr_tls.h"
#include "csr_crypto.h"
#include "csr_bn/csr_bn.h"
#include "csr_asn1/csr_asn1.h"
#include "csr_hmac/csr_hmac.h"
#include "csr_des/csr_des.h"
#include "csr_security_private.h"

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void TlsRsaEncryptKeyExchange(CsrUint8 *key_exchange, CsrUint32 baseLength, tls_data *tls)
{
    CsrBignum *modulus, *exponent, *input, *output;
    CsrBignumCtx *ctx;
    sme_trace_info((TR_SECURITY_LIB, "TlsRsaEncryptKeyExchange()"));
    sme_trace_info((TR_SECURITY_LIB, "TlsRsaEncryptKeyExchange() :: modulus length = %d", tls->server_modulus_length));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsRsaEncryptKeyExchange() :: modulus", tls->server_modulus, tls->server_modulus_length));
    modulus =  CsrCryptoBnBinToBn(tls->server_modulus, tls->server_modulus_length, NULL);
    sme_trace_info((TR_SECURITY_LIB, "TlsRsaEncryptKeyExchange() :: exponent length = %d", tls->server_exponent_length));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsRsaEncryptKeyExchange() :: exponent", tls->server_exponent, tls->server_exponent_length));
    exponent = CsrCryptoBnBinToBn(tls->server_exponent, tls->server_exponent_length, NULL);
    sme_trace_info((TR_SECURITY_LIB, "TlsRsaEncryptKeyExchange() :: base length = %d", baseLength));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsRsaEncryptCertificateVerify() :: base", key_exchange, baseLength));
    input = CsrCryptoBnBinToBn(key_exchange, baseLength, NULL);
    output = CsrCryptoBnNew();
    ctx = CsrCryptoBnCtxNew();
    (void)CsrCryptoBnModExp(output, input, exponent, modulus, ctx);
    CsrCryptoBnCtxFree(ctx);

    (void)CsrCryptoBnBnToBin(output, key_exchange);
    sme_trace_info((TR_SECURITY_LIB, "TlsRsaEncryptKeyExchange() :: output length = %d", CsrCryptoBnNumOctets(output)));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsRsaEncryptKeyExchange() :: result", key_exchange, CsrCryptoBnNumOctets(output)));
    CsrCryptoBnFree(modulus);
    CsrCryptoBnFree(exponent);
    CsrCryptoBnFree(input);
    CsrCryptoBnFree(output);
}

static CsrUint8 *TlsAddCertificateVerifyHashes(CsrUint8 *fillout, tls_data *tls)
{
    /* Clone the current MD5 and SHA1 contexts, so that intermediate hashes can be obtained without disturbing the hash states */
    CSR_CRYPTO_MD5_CTX *md5_ctx = CsrCryptoCloneMd5Context(tls->md5_ctx);
    CSR_CRYPTO_SHA1_CTX *sha1_ctx = CsrCryptoCloneSha1Context(tls->sha1_ctx);

    CsrCryptoCallMd5Final(NULL, NULL,
                          fillout, md5_ctx);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddCertificateVerifyHashes() :: MD5 hash", fillout, CSR_CRYPTO_MD5_DIGEST_LENGTH));
    fillout += CSR_CRYPTO_MD5_DIGEST_LENGTH;
    CsrCryptoCallSha1Final(NULL, NULL,
                           fillout, sha1_ctx);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddCertificateVerifyHashes() :: SHA1 hash", fillout, CSR_SHA1_DIGEST_LENGTH));
    fillout += CSR_SHA1_DIGEST_LENGTH;

    return fillout;
}

static void TlsRsaEncryptCertificateVerify(CsrUint8 *certificate_verify, CsrUint32 certificate_verify_length, const CsrUint8 *client_private)
{
    void *modulus, *exponent, *input, *output;
    void *ctx;
    const CsrUint8 *scan = client_private;
    CsrUint32 length;

    /* Extract Client modulus and private exponent from ASN.1 DER representation of PKCS #1 RSAPrivateKey  */
    length = CsrCryptoAsn1GetRsaPrivateModulus(&scan);
    sme_trace_info((TR_SECURITY_LIB, "TlsRsaEncryptCertificateVerify() :: modulus length = %d", length));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsRsaEncryptCertificateVerify() :: modulus", scan, length));
    modulus =  CsrCryptoBnBinToBn(scan, length, NULL);
    scan += length;
    length = CsrCryptoAsn1GetRsaPrivateExponent(&scan);
    sme_trace_info((TR_SECURITY_LIB, "TlsRsaEncryptCertificateVerify() :: exponent length = %d", length));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsRsaEncryptCertificateVerify() :: exponent", scan, length));
    exponent = CsrCryptoBnBinToBn(scan, length, NULL);
    sme_trace_info((TR_SECURITY_LIB, "TlsRsaEncryptCertificateVerify() :: base length = %d", certificate_verify_length));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsRsaEncryptCertificateVerify() :: base", certificate_verify, certificate_verify_length));
    input = CsrCryptoBnBinToBn(certificate_verify, certificate_verify_length, NULL);
    output = CsrCryptoBnNew();

    ctx = CsrCryptoBnCtxNew();
    (void)CsrCryptoBnModExp(output, input, exponent, modulus, ctx);
    CsrCryptoBnCtxFree(ctx);

    (void)CsrCryptoBnBnToBin(output, certificate_verify);
    sme_trace_info((TR_SECURITY_LIB, "TlsRsaEncryptCertificateVerify() :: output length = %d", CsrCryptoBnNumOctets(output)));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsRsaEncryptCertificateVerify() :: result", certificate_verify, CsrCryptoBnNumOctets(output)));
    CsrCryptoBnFree(modulus);
    CsrCryptoBnFree(exponent);
    CsrCryptoBnFree(input);
    CsrCryptoBnFree(output);
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

CsrUint8 *TlsAddCertificateVerify(CsrUint8 *fillout, const CsrUint32 modulusLength, const CsrUint8 *client_private, tls_data *tls)
{
    CsrUint8 *start = fillout;
    CsrUint8 *certificate_verify;
    CsrUint32 length;
    CsrUint32 padLength;

    /* Handshake type */
    *fillout++ = 0x0f; /* certificate verify */

    /* Handshake message length */
    length = modulusLength + 2;
    *fillout++ = (length >> 16) & 0xff;
    *fillout++ = (length >> 8) & 0xff;
    *fillout++ = length & 0xff;

    /* Encrypted block length */
    length = modulusLength;
    *fillout++ = (length >> 8) & 0xff;
    *fillout++ = length & 0xff;

    /* Build the plaintext Certificate Verify, this will then be encrypted in-situ */
    certificate_verify = fillout; /* Remember where the certificate verify starts in the message fillout */
    *fillout++ = 0x00; /* Leading zero octet */
    *fillout++ = 0x01; /* Block Type: Private Key */

    padLength = modulusLength;
    padLength -= 2; /* Leading zero octet and Block Type */
    padLength -= 1; /* Zero separator */
    padLength -= (CSR_CRYPTO_MD5_DIGEST_LENGTH + CSR_SHA1_DIGEST_LENGTH);
    sme_trace_info((TR_SECURITY_LIB, "TlsAddCertificateVerify() :: pad length = %d", padLength));

    CsrMemSet(fillout, 0xff, padLength); /* 0xff padding */
    fillout += padLength;
    *fillout++ = 0x00; /* Zero separator */

    fillout = TlsAddCertificateVerifyHashes(fillout, tls);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddCertificateVerify() :: certificate verify plaintext", certificate_verify, modulusLength));
    /* RSA encrypt certificate verify plaintext using the client private key */
    TlsRsaEncryptCertificateVerify(certificate_verify, modulusLength, client_private);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddCertificateVerify() :: certificate verify encrypted", certificate_verify, modulusLength));
    CsrCryptoCallMd5Update(NULL, NULL,
                           tls->md5_ctx, start, (CsrUint32)(fillout - start));
    CsrCryptoCallSha1Update(NULL, NULL,
                            tls->sha1_ctx, start, (CsrUint32)(fillout - start));

    return fillout;
}

CsrUint8 *TlsAddClientKeyExchange(CsrUint8 *fillout, const CsrUint32 modulusLength, const CsrUint8 *pre_master_secret, tls_data *tls)
{
    CsrUint8 *start = fillout;
    CsrUint8 *key_exchange;
    CsrUint8 *padding;
    CsrUint32 length;
    CsrUint32 padLength;

    /* Handshake type */
    *fillout++ = 0x10; /* client key exchange */

    length = modulusLength + 2;
    /* Handshake message length */
    *fillout++ = (length >> 16) & 0xff;
    *fillout++ = (length >> 8) & 0xff;
    *fillout++ = length & 0xff;

    /* Encrypted block length */
    length = modulusLength;
    *fillout++ = (length >> 8) & 0xff;
    *fillout++ = length & 0xff;
    key_exchange = fillout;
    *fillout++ = 0x00; /* Leading 0x00 octet */
    *fillout++ = 0x02; /* Block Type: Public Key */
    sme_trace_info((TR_SECURITY_LIB, "TlsAddClientKeyExchange() :: Adding padding"));

    padLength = modulusLength;
    padLength -= 2;  /* Leading zero octet and Block Type */
    padLength -= 1;  /* Zero separator */
    padLength -= 48; /* Pre-Master Secret */
    sme_trace_info((TR_SECURITY_LIB, "TlsAddClientKeyExchange() :: pad length = %d", padLength));

    CsrWifiSecurityRandom(fillout, padLength);
    /* Need to ensure there are no zero bytes in the padding */
    for(padding = fillout; fillout < padding + padLength; fillout++)
    {
        if(*fillout == 0x00) *fillout = 0x01;
    }
    *fillout++ = 0x00; /* Separator 0x00 octet */

    CsrMemCpy(fillout, pre_master_secret, 48);
    fillout += 48;
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddClientKeyExchange() :: Key Exchange plaintext", key_exchange, modulusLength));
    TlsRsaEncryptKeyExchange(key_exchange, modulusLength, tls);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddClientKeyExchange() :: Key Exchange ciphertext", key_exchange, modulusLength));
    sme_trace_info((TR_SECURITY_LIB, "TlsAddClientKeyExchange() :: Hashing Client Key Exchange (%d)", fillout - start));

    CsrCryptoCallMd5Update(NULL, NULL,
                           tls->md5_ctx, start, (CsrUint32)(fillout - start));
    CsrCryptoCallSha1Update(NULL, NULL,
                            tls->sha1_ctx, start, (CsrUint32)(fillout - start));

    return fillout;
}

void TlsBuildPreMasterSecret(CsrUint8 *pre_master_secret)
{
    CsrWifiSecurityRandom(pre_master_secret, 48);
    /* The first two bytes of the pre-master-secret have to match the Version number in the Client Hello */
    pre_master_secret[0] = TLS_PROTOCOL_VERSION_MAJOR;
    pre_master_secret[1] = TLS_PROTOCOL_VERSION_MINOR;
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsBuildPreMasterSecret() :: pre_master_secret", pre_master_secret, 48));
}

CsrUint8 *TlsAddClientCertificate(CsrUint8 *fillout, const CsrUint8 *client_cert, CsrUint32 client_cert_length, tls_data *tls)
{
    CsrUint8 *start = fillout;

    sme_trace_info((TR_SECURITY_LIB, "TlsAddClientCertificate() :: Client certificate length = %d", client_cert_length));

    /* Handshake type */
    *fillout++ = 11; /* certificate */

    /* Handshake message length */
    *fillout++ = ((client_cert_length + 6) >> 16) & 0xff;
    *fillout++ = ((client_cert_length + 6) >> 8) & 0xff;
    *fillout++ = (client_cert_length + 6) & 0xff;

    /* Certificates Length */
    *fillout++ = ((client_cert_length + 3) >> 16) & 0xff;
    *fillout++ = ((client_cert_length + 3) >> 8) & 0xff;
    *fillout++ = (client_cert_length + 3) & 0xff;

    /* Certificate Length */
    *fillout++ = (client_cert_length >> 16) & 0xff;
    *fillout++ = (client_cert_length >> 8) & 0xff;
    *fillout++ = client_cert_length & 0xff;

    CsrMemCpy(fillout, client_cert, client_cert_length);
    fillout += client_cert_length;

    sme_trace_info((TR_SECURITY_LIB, "TlsAddClientCertificate() :: Hashing Certificate (%d)", fillout - start));

    CsrCryptoCallMd5Update(NULL, NULL,
                           tls->md5_ctx, start, (CsrUint32)(fillout - start));
    CsrCryptoCallSha1Update(NULL, NULL,
                            tls->sha1_ctx, start, (CsrUint32)(fillout - start));

    return fillout;
}

void TlsDeriveMasterSecret(const CsrUint8 *pre_master_secret, tls_data *tls)
{
    CsrUint8 random_seed[64];
    const char *label;

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsDeriveMasterSecret() :: Server random", (CsrUint8 *)&tls->server_random, 32));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsDeriveMasterSecret() :: Client random", (CsrUint8 *)&tls->client_random, 32));

    CsrMemCpy(random_seed, &tls->client_random, 32);
    CsrMemCpy(&(random_seed[32]), &tls->server_random, 32);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsDeriveMasterSecret() :: Client+Server random", random_seed, 64));

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsDeriveMasterSecret() :: pre_master_secret", pre_master_secret, 48));
    label = "master secret";
    prf(pre_master_secret, 48, label, CsrStrLen(label),
            random_seed, 64,
            tls->master_secret, 48);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsDeriveMasterSecret() :: master secret", tls->master_secret, 48));
}

void TlsDerivePmk(TlsContext* tlsContext)
{
    CsrUint8 random_seed[64];
    CsrUint8 output[128];
    const char *label = "client EAP encryption";

    CsrMemCpy(random_seed, &(tlsContext->tls.client_random), 32);
    CsrMemCpy(&(random_seed[32]), &(tlsContext->tls.server_random), 32);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsDerivePmk() :: Client+Server random", random_seed, 64));

    prf(tlsContext->tls.master_secret, TLS_MASTER_SECRET_LENGTH, label, CsrStrLen(label), random_seed, 64, output, 128);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsDerivePmk() :: PMK", output, CSR_WIFI_SECURITY_PMK_LENGTH));

    sme_trace_info((TR_SECURITY_LIB, "TlsDerivePmk() :: Updating PMK"));
    CsrWifiSecurityStorePmk(tlsContext->context, output);
}

CsrUint8 *TlsAddFinished(CsrUint8 *fillout, tls_data *tls)
{
    CsrUint8 random_seed[64];
    CsrUint8 plaintext[100], *plain;
    CsrUint8 mac_header[5];
    CSR_HMAC_SHA1_CTX ctx;
    const char *label = "client finished";
    CSR_CRYPTO_MD5_CTX *md5_ctx = CsrCryptoCloneMd5Context(tls->md5_ctx);
    CSR_CRYPTO_SHA1_CTX *sha1_ctx = CsrCryptoCloneSha1Context(tls->sha1_ctx);

    plain = plaintext;
    *plain++ = 0x14; /* Handshake Type: Finished */
    *plain++ = 0x00; /* Length: 12 */
    *plain++ = 0x00;
    *plain++ = 0x0c;

    CsrCryptoCallMd5Final(NULL, NULL,
                          random_seed, md5_ctx);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: MD5 hash", random_seed, 16));

    CsrCryptoCallSha1Final(NULL, NULL,
                           &random_seed[CSR_CRYPTO_MD5_DIGEST_LENGTH], sha1_ctx);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: SHA1 hash", &random_seed[CSR_CRYPTO_MD5_DIGEST_LENGTH], CSR_SHA1_DIGEST_LENGTH));

    prf(tls->master_secret, 48,
        label, CsrStrLen(label),
        random_seed, CSR_CRYPTO_MD5_DIGEST_LENGTH + CSR_SHA1_DIGEST_LENGTH,
        plain, 12);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: digest", plain, 12));
    plain += 12;

    CsrMemSet(tls->write_seq_no, 0, 8);

    mac_header[0] = 22; /* Handshake */
    mac_header[1] = TLS_PROTOCOL_VERSION_MAJOR;  /* protocol version hi */
    mac_header[2] = TLS_PROTOCOL_VERSION_MINOR; /* protocol version low */
    mac_header[3] = 0; /* Two octet plaintext length - high octet will always be zero */
    mac_header[4] = (plain - plaintext) & 0xff;

    CsrHmacSha1Init(&ctx, tls->keys.client_write_MAC_secret, CSR_SHA1_DIGEST_LENGTH);
    CsrHmacSha1Update(&ctx, tls->write_seq_no, 8);
    CsrHmacSha1Update(&ctx, mac_header, 5);
    CsrHmacSha1Update(&ctx, plaintext, 16);
    CsrHmacSha1Final(plain, &ctx );
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: SHA1 MAC", plain, CSR_SHA1_DIGEST_LENGTH));
    plain += 20;

    switch(tls->pending_cypher_suite)
    {
    case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
        sme_trace_info((TR_SECURITY_LIB, "TlsAddFinished() :: TLS_RSA_WITH_3DES_EDE_CBC_SHA cipher suite"));
        {
            CsrKeySchedule ks1, ks2, ks3;
            CsrUint32 inLength, outLength;

            inLength = (CsrUint32)(plain - plaintext);
            sme_trace_info((TR_SECURITY_LIB, "TlsAddFinished() :: 3DES plaintext length = %d", inLength));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: 3DES plaintext", plaintext, inLength));
            CsrCryptoDesSetKey(&tls->keys.client_write_key[0], &ks1);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: 3DES key 1", &tls->keys.client_write_key[0], 8));
            CsrCryptoDesSetKey(&tls->keys.client_write_key[8], &ks2);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: 3DES key 2", &tls->keys.client_write_key[8], 8));
            CsrCryptoDesSetKey(&tls->keys.client_write_key[16], &ks3);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: 3DES key 3", &tls->keys.client_write_key[16], 8));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: 3DES IV", tls->keys.client_write_IV, 8));
            CsrCrypto3DesCbcEncrypt(&ks1, &ks2, &ks3, tls->keys.client_write_IV, plaintext, inLength, fillout, &outLength, PAD_MODE_TLS);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: Finished encrypted", fillout, outLength));
            fillout += outLength;
        }
        break;
    case TLS_RSA_WITH_AES_128_CBC_SHA:
        sme_trace_info((TR_SECURITY_LIB, "TlsAddFinished() :: TLS_RSA_WITH_AES_128_CBC_SHA cipher suite"));
        {
            CsrUint32 inLength, outLength;

            inLength = (CsrUint32)(plain - plaintext);
            sme_trace_info((TR_SECURITY_LIB, "TlsAddFinished() :: AES128 plaintext length = %d", inLength));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: AES128 plaintext", plaintext, inLength));
            CsrCryptoAes128CbcEncrypt(tls->keys.client_write_key, tls->keys.client_write_IV, plaintext, inLength, fillout, &outLength, PAD_MODE_TLS);
            sme_trace_info((TR_SECURITY_LIB, "TlsAddFinished() :: AES128 ciphertext length = %d", outLength));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "TlsAddFinished() :: AES128 ciphertext", fillout, outLength));
            fillout += outLength;
        }
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "TlsAddFinished() :: Unsupported cipher suite(%04x)", tls->pending_cypher_suite));
        break;
    }

    CsrCryptoCallMd5Update(NULL, NULL,
                           tls->md5_ctx, plaintext, 16);
    CsrCryptoCallSha1Update(NULL, NULL,
                            tls->sha1_ctx, plaintext, 16);

    return fillout;
}
