/** @file wps_crypto.c *
 * Implementation of the Wi-Fi Protected Setup (WPS) cryptographic functions
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) cryptographic functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_crypto.c#2 $
 *
 ****************************************************************************/
#include "sme_trace/sme_trace.h"
#include "wps_common.h"
#include "csr_aes/csr_aes.h"
#include "csr_bn/csr_bn.h"
#include "csr_hmac/csr_hmac.h"
#include "csr_sha256/csr_sha256.h"
#include "csr_crypto.h"

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

static const CsrUint8 prime[] =
      { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
        0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
        0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
        0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
        0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45, 0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
        0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
        0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
        0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D, 0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
        0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
        0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
        0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D, 0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
        0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x23, 0x73, 0x27, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const CsrUint8 g = 2;

static const char personalisation[] = "Wi-Fi Easy and Secure Key Derivation";

#if defined(CSR_WIFI_SECURITY_WPS_KEYLEN) && ( CSR_WIFI_SECURITY_WPS_KEYLEN <= 1536 )
static const CsrInt32 keylen = CSR_WIFI_SECURITY_WPS_KEYLEN;
#else
static const keylen = 1536;
#endif

void initKeyPair( CsrWpsContext *pCtx )
{
    CsrUint8 base[192];
    CsrInt32 i, j;
    CsrInt32 clear_bytes = (1536 - keylen) / 8;
    CsrInt32 clear_bits = (1536 - keylen) % 8;
    CsrUint8 bit_mask, mask;
    CsrUint32 length;

    CsrMemSet(base, 0, sizeof(base));
    base[191] = g;
    sme_trace_info((TR_SECURITY_LIB, "Private key length = %d", keylen));
    CsrWifiSecurityRandom(pCtx->ePrvKey, sizeof(pCtx->ePrvKey));

    for(i = 0; i < clear_bytes; i++)
    {
        pCtx->ePrvKey[i] = 0;
    }

    if(i < 192)
    {
        for(bit_mask = 0x80, j = 0; j < clear_bits; j++) /*lint !e681*/
        {
            bit_mask >>= 1;
        }

        for(mask = 0, j = clear_bits; j < 8; j++)
        {
            mask |= bit_mask;
            bit_mask >>= 1;
        }

        sme_trace_info((TR_SECURITY_LIB, "Private key bit mask = 0x%02x", mask));
        pCtx->ePrvKey[i] &= mask;
    }

    CsrCryptoModExp(base, sizeof(base),
                    pCtx->ePrvKey, sizeof(pCtx->ePrvKey),
                    prime, sizeof(prime),
                    pCtx->ePubKey,
                    &length);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Enrollee Public Key", pCtx->ePubKey, sizeof(pCtx->ePubKey)));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Enrollee Private Key", pCtx->ePrvKey, sizeof(pCtx->ePrvKey)));
}

void computeSessionKeys( CsrWpsContext *pCtx )
{
    CsrUint8 plain[WPS_NONCE_LENGTH + 6 + WPS_NONCE_LENGTH];
    CsrUint8 dhKey[WPS_SHA256_DIGEST_LENGTH];
    CsrUint8 sharedSecret[WPS_DIFFIE_HELLMAN_KEY_LENGTH];
    CsrUint32 length;

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Registrar Public Key", pCtx->rPubKey, sizeof( pCtx->rPubKey ) ));

    CsrCryptoModExp(pCtx->rPubKey, sizeof(pCtx->rPubKey),
                    pCtx->ePrvKey, sizeof(pCtx->ePrvKey),
                    prime, sizeof(prime),
                    sharedSecret,
                    &length);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "shared secret",
                   sharedSecret,
                   WPS_DIFFIE_HELLMAN_KEY_LENGTH));

    CsrCryptoSha256(sharedSecret,
                    WPS_DIFFIE_HELLMAN_KEY_LENGTH,
                    dhKey);

    CsrMemCpy(plain, pCtx->eNonce, WPS_NONCE_LENGTH);
    CsrMemCpy(plain + WPS_NONCE_LENGTH, pCtx->securityContext->setupData.localMacAddress, 6);
    CsrMemCpy(plain + 6 + WPS_NONCE_LENGTH, pCtx->rNonce, WPS_NONCE_LENGTH);

    pCtx->kdk = (CsrUint8*)CsrPmalloc(WPS_SHA256_DIGEST_LENGTH);

    CsrCryptoHmacSha256(dhKey, 32, plain, sizeof(plain), pCtx->kdk);

    pCtx->message = (CsrUint8*)CsrPmalloc(sizeof(personalisation)+8);
    pCtx->sessionKeys = (CsrUint8*)CsrPmalloc(WPS_SHA256_DIGEST_LENGTH * 3); /* 256bits x 3 to hold 640 bits */

    /* Key derviation. Need to generate 640 bits using HMAC-SHA256 */
    /* This requires 3 iterations */
    pCtx->message[0] = 0x00; /* Iteration in big endian */
    pCtx->message[1] = 0x00;
    pCtx->message[2] = 0x00;
    pCtx->message[3] = 0x01;
    CsrMemCpy( &pCtx->message[4], personalisation, sizeof(personalisation)-1 );
    pCtx->message[4+sizeof(personalisation)-1] = 0x00; /* 640 in big endian */
    pCtx->message[5+sizeof(personalisation)-1] = 0x00;
    pCtx->message[6+sizeof(personalisation)-1] = 0x02;
    pCtx->message[7+sizeof(personalisation)-1] = 0x80;

    CsrCryptoHmacSha256(pCtx->kdk, 32, pCtx->message, 7+sizeof(personalisation), pCtx->sessionKeys);
    pCtx->message[3] = 0x02;

    CsrCryptoHmacSha256(pCtx->kdk, 32, pCtx->message, 7+sizeof(personalisation), &pCtx->sessionKeys[WPS_SHA256_DIGEST_LENGTH]);
    pCtx->message[3] = 0x03;

    CsrCryptoHmacSha256(pCtx->kdk, 32, pCtx->message, 7+sizeof(personalisation), &pCtx->sessionKeys[WPS_SHA256_DIGEST_LENGTH*2]);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS session keys", pCtx->sessionKeys, sizeof(pCtx->sessionKeys)));

    CsrMemCpy(pCtx->authKey, pCtx->sessionKeys, sizeof(pCtx->authKey));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Auth Key", pCtx->authKey, sizeof(pCtx->authKey)));

    CsrMemCpy(pCtx->keyWrapKey, &pCtx->sessionKeys[32], sizeof(pCtx->keyWrapKey));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Key Wrap Key", pCtx->keyWrapKey, sizeof(pCtx->keyWrapKey)));

    CsrPfree(pCtx->message);
    CsrPfree(pCtx->sessionKeys);
    CsrPfree(pCtx->kdk);
}

void initEhash( CsrWpsContext *pCtx )
{
    char pin1[5]; /* Assume maximum of 4 digit PIN halves */
    char pin2[5];
    CsrUint32 pin1_length, pin2_length;

    pin1_length = (CsrStrLen((char*)pCtx->securityContext->setupData.pin ) + 1) / 2;

    CsrMemCpy(pin1, pCtx->securityContext->setupData.pin, pin1_length);
    pin1[pin1_length] = '\0';
    sme_trace_info((TR_SECURITY_LIB," pin = %s", pCtx->securityContext->setupData.pin));
    sme_trace_info((TR_SECURITY_LIB," pin1 = %s", pin1));

    CsrCryptoHmacSha256(pCtx->authKey, 32, (CsrUint8 *)pin1, CsrStrLen(pin1), pCtx->psk1);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS PSK1", pCtx->psk1, sizeof(pCtx->psk1)));

    pin1_length = (CsrStrLen((char*)pCtx->securityContext->setupData.pin) + 1) / 2;
    pin2_length = CsrStrLen((char*)pCtx->securityContext->setupData.pin) - pin1_length;

    CsrMemCpy(pin2, pCtx->securityContext->setupData.pin + pin1_length, pin2_length);
    pin2[pin2_length] = '\0';
    sme_trace_info((TR_SECURITY_LIB," pin2 = %s", pin2));

    CsrCryptoHmacSha256(pCtx->authKey, 32, (CsrUint8 *)pin2, CsrStrLen(pin2), pCtx->psk2);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS PSK2", pCtx->psk2, sizeof(pCtx->psk2)));

    CsrWifiSecurityRandom(pCtx->eSnonce1, sizeof(pCtx->eSnonce1));
    CsrWifiSecurityRandom(pCtx->eSnonce2, sizeof(pCtx->eSnonce2));

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Enrollee Secret Nonce 1", pCtx->eSnonce1, sizeof(pCtx->eSnonce1)));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Enrollee Secret Nonce 2", pCtx->eSnonce2, sizeof(pCtx->eSnonce2)));
}
