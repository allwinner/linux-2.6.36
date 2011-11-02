/** @file wps_crypto.c
 *
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
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_crypto.c#3 $
 *
 ****************************************************************************/

#include "wps_common.h"
#include "wps_debug.h"

#ifdef WPS_OPENSSL
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#else
#include "wps_random.h"
#include "csr_crypto.h"

#endif

#ifdef WPS_OPENSSL_SHA256
#define OPENSSL_NO_SHA512
#include <openssl/sha.h>
#endif

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

static const CsrUint8 prime[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
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


#if defined(WPS_KEYLEN) && ( WPS_KEYLEN <= 1536 )
    const static int keylen = WPS_KEYLEN;
#else
    const static keylen = 1536;
#endif

void initEnrolleeNonce( WpsContext *pCtx )
{
#ifdef WPS_OPENSSL
    (void)RAND_bytes( pCtx->eNonce, sizeof( pCtx->eNonce ) );
#else
    WpsRandomBytes( pCtx->eNonce, sizeof( pCtx->eNonce ) );
#endif
    WPS_DBG_PRINTHEX(( "WPS Enrollee Nonce", pCtx->eNonce, sizeof( pCtx->eNonce ) ));
}

void initKeyPair( WpsContext *pCtx )
{
#ifdef WPS_OPENSSL
    {
    DH *dh;

        dh = DH_new();
        dh->g = BN_bin2bn( &g, sizeof(g), NULL);
        dh->p = BN_bin2bn( prime, sizeof(prime), NULL);
        (void)DH_generate_key( dh );
        (void)BN_bn2bin( dh->pub_key, pCtx->ePubKey );
        (void)BN_bn2bin( dh->priv_key, pCtx->ePrvKey );
        DH_free( dh );
    }
#else
    {
    int i, j;
    int clear_bytes = (1536 - keylen) / 8;
    int clear_bits = (1536 - keylen) % 8;
    unsigned char bit_mask, mask;

        WPS_DBG_PRINT(( "Private key length = %d", keylen ));
        WpsRandomBytes( pCtx->ePrvKey, sizeof( pCtx->ePrvKey ) );

        for( i = 0; i < clear_bytes; i++ )
        {
        	pCtx->ePrvKey[i] = 0;
        }

        if( i < 192 )
        {
            for( bit_mask = 0x80, j = 0; j < clear_bits; j++ )
            {
        	    bit_mask >>= 1;
            }

            for( mask = 0, j = clear_bits; j < 8; j++ )
            {
        	    mask |= bit_mask;
        	    bit_mask >>= 1;
            }

            WPS_DBG_PRINT(( "Private key bit mask = 0x%02x", mask ));
            pCtx->ePrvKey[i] &= mask;
        }

        {
            CsrUint32 resultLength;
            CsrCryptoCallModExp(NULL, NULL,
                                &g, sizeof(g),
                                pCtx->ePrvKey, sizeof(pCtx->ePrvKey),
                                prime, sizeof(prime),
                                pCtx->ePubKey, &resultLength);
        }
    }
#endif
    WPS_DBG_PRINTHEX(( "WPS Enrollee Public Key", pCtx->ePubKey, sizeof( pCtx->ePubKey ) ));
    WPS_DBG_PRINTHEX(( "WPS Enrollee Private Key", pCtx->ePrvKey, sizeof( pCtx->ePrvKey ) ));
}

void computeSessionKeys( WpsContext *pCtx )
{
CsrUint8 sharedSecret[WPS_DIFFIE_HELLMAN_KEY_LENGTH];
CsrUint8 dhKey[WPS_SHA256_DIGEST_LENGTH];
CsrUint8 kdk[WPS_SHA256_DIGEST_LENGTH];
unsigned int len;
CsrUint8 sessionKeys[WPS_SHA256_DIGEST_LENGTH * 3]; /* 256bits x 3 to hold 640 bits */
char personalisation[] = "Wi-Fi Easy and Secure Key Derivation";
CsrUint8 message[sizeof(personalisation)+8];

    WPS_DBG_PRINTHEX(( "WPS Registrar Public Key", pCtx->rPubKey, sizeof( pCtx->rPubKey ) ));
#ifdef WPS_OPENSSL
    {
  	BN_CTX *bn_ctx = BN_CTX_new();
   	BIGNUM *bn_sharedSecret = BN_new();
   	BIGNUM *bn_ePrvKey = BN_bin2bn( pCtx->ePrvKey, sizeof( pCtx->ePrvKey ), NULL);
   	BIGNUM *bn_rPubKey = BN_bin2bn( pCtx->rPubKey, sizeof( pCtx->rPubKey ), NULL);
   	BIGNUM *bn_prime = BN_bin2bn( prime, sizeof( prime ), NULL);

        (void)BN_mod_exp( bn_sharedSecret, bn_rPubKey, bn_ePrvKey, bn_prime, bn_ctx );
        (void)BN_bn2bin( bn_sharedSecret, sharedSecret );

        BN_clear_free( bn_sharedSecret  );
        BN_clear_free( bn_ePrvKey  );
        BN_clear_free( bn_rPubKey  );
        BN_clear_free( bn_prime );
        BN_CTX_free( bn_ctx );
    }
#else
    {
        CsrUint32 resultLength;
        CsrCryptoCallModExp(NULL, NULL,
                            pCtx->rPubKey, sizeof(pCtx->rPubKey),
                            pCtx->ePrvKey, sizeof(pCtx->ePrvKey),
                            prime, sizeof(prime),
                            sharedSecret, &resultLength);
    }
#endif
    WPS_DBG_PRINTHEX(("shared secret", sharedSecret, sizeof(sharedSecret)));

#ifdef WPS_OPENSSL_SHA256
    {
    HMAC_CTX hmac_ctx;

        SHA256( sharedSecret, sizeof( sharedSecret ), dhKey );
        HMAC_CTX_init( &hmac_ctx );
        HMAC_Init( &hmac_ctx, dhKey, SHA256_DIGEST_LENGTH, EVP_sha256() );
        HMAC_Update( &hmac_ctx, pCtx->eNonce, sizeof(pCtx->eNonce ) );
        HMAC_Update( &hmac_ctx, pCtx->macAddress, sizeof( pCtx->macAddress ) );
        HMAC_Update( &hmac_ctx, pCtx->rNonce, sizeof( pCtx->rNonce ) );
        HMAC_Final( &hmac_ctx, kdk, &len );
        HMAC_CTX_cleanup( &hmac_ctx );
    }
#else
    {
        CsrUint8 *input, *fillout;

        CsrCryptoCallSha256(NULL, NULL,
                            sharedSecret,
                            sizeof(sharedSecret),
                            dhKey);
        WPS_DBG_PRINTHEX(( "DH Key", dhKey, WPS_SHA256_DIGEST_LENGTH));

        input = (CsrUint8*)malloc(sizeof(pCtx->eNonce) + sizeof(pCtx->macAddress) + sizeof(pCtx->rNonce));
        fillout = input;

        memcpy(fillout, pCtx->eNonce, sizeof(pCtx->eNonce));
        fillout += sizeof(pCtx->eNonce);
        memcpy(fillout, pCtx->macAddress, sizeof( pCtx->macAddress));
        fillout += sizeof( pCtx->macAddress);
        memcpy(fillout, pCtx->rNonce, sizeof( pCtx->rNonce));
        WPS_DBG_PRINTHEX(( "Registrar Nonce", pCtx->rNonce, sizeof(pCtx->rNonce)));

        CsrCryptoCallHmacSha256(NULL, NULL,
                                dhKey, WPS_SHA256_DIGEST_LENGTH,
                                input, sizeof(pCtx->eNonce) + sizeof(pCtx->macAddress) + sizeof(pCtx->rNonce),
                                kdk);
        WPS_DBG_PRINTHEX(( "KDK", kdk, WPS_SHA256_DIGEST_LENGTH));

        free(input);
    }
#endif

    /* Key derviation. Need to generate 640 bits using HMAC-SHA256 */
    /* This requires 3 iterations */
    message[0] = 0x00; /* Iteration in big endian */
    message[1] = 0x00;
    message[2] = 0x00;
    message[3] = 0x01;
    memcpy( &message[4], personalisation, sizeof(personalisation)-1 );
    message[4+sizeof(personalisation)-1] = 0x00; /* 640 in big endian */
    message[5+sizeof(personalisation)-1] = 0x00;
    message[6+sizeof(personalisation)-1] = 0x02;
    message[7+sizeof(personalisation)-1] = 0x80;

#ifdef WPS_OPENSSL_SHA256
    (void)HMAC( EVP_sha256(), kdk, SHA256_DIGEST_LENGTH, message, 7+sizeof(personalisation), sessionKeys, NULL );
#else
    {
        CsrCryptoCallHmacSha256(NULL, NULL,
                                kdk, WPS_SHA256_DIGEST_LENGTH,
                                message, 7+sizeof(personalisation),
                                sessionKeys);
    }
#endif

    message[3] = 0x02;

#ifdef WPS_OPENSSL_SHA256
    (void)HMAC( EVP_sha256(), kdk, SHA256_DIGEST_LENGTH, message, 7+sizeof(personalisation), &sessionKeys[SHA256_DIGEST_LENGTH], NULL );
#else
    {
        CsrCryptoCallHmacSha256(NULL, NULL,
                                kdk, WPS_SHA256_DIGEST_LENGTH,
                                message, 7+sizeof(personalisation),
                                &sessionKeys[WPS_SHA256_DIGEST_LENGTH]);
    }
#endif

    message[3] = 0x03;

#ifdef WPS_OPENSSL_SHA256
    (void)HMAC( EVP_sha256(), kdk, SHA256_DIGEST_LENGTH, message, 7+sizeof(personalisation), &sessionKeys[SHA256_DIGEST_LENGTH*2], NULL );
#else
    {
        CsrCryptoCallHmacSha256(NULL, NULL,
                                kdk, WPS_SHA256_DIGEST_LENGTH,
                                message, 7+sizeof(personalisation),
                                &sessionKeys[WPS_SHA256_DIGEST_LENGTH*2]);
    }
#endif

    WPS_DBG_PRINTHEX(( "WPS session keys", sessionKeys, sizeof( sessionKeys ) ));

    memcpy( pCtx->authKey, sessionKeys, sizeof( pCtx->authKey ) );
    WPS_DBG_PRINTHEX(( "WPS Auth Key", pCtx->authKey, sizeof( pCtx->authKey ) ));

    memcpy( pCtx->keyWrapKey, &sessionKeys[32], sizeof( pCtx->keyWrapKey ) );
    WPS_DBG_PRINTHEX(( "WPS Key Wrap Key", pCtx->keyWrapKey, sizeof( pCtx->keyWrapKey ) ));
}
void computeAuthenticator( WpsContext *pCtx, WpsBuffer *pPreviousMessage, WpsBuffer *pCurrentMessage, CsrUint8 *pAuthenticator )
{
CsrUint8 hash[WPS_SHA256_DIGEST_LENGTH];

#ifdef WPS_OPENSSL_SHA256
    {
    HMAC_CTX hmac_ctx;

        HMAC_CTX_init( &hmac_ctx );
        HMAC_Init( &hmac_ctx, pCtx->authKey, sizeof( pCtx->authKey ), EVP_sha256() );
        HMAC_Update( &hmac_ctx, pPreviousMessage->pStart, (int)pPreviousMessage->size );
        HMAC_Update( &hmac_ctx, pCurrentMessage->pStart, (int)pCurrentMessage->size );
        HMAC_Final( &hmac_ctx, hash, NULL );
    }
#else
    {
        CsrUint8 *input, *fillout;

        input = (CsrUint8 *)malloc((int)pPreviousMessage->size + (int)pCurrentMessage->size);
        fillout = input;

        memcpy(fillout, pPreviousMessage->pStart, (int)pPreviousMessage->size);
        fillout += (int)pPreviousMessage->size;
        memcpy(fillout, pCurrentMessage->pStart, (int)pCurrentMessage->size);

        CsrCryptoCallHmacSha256(NULL, NULL,
                                pCtx->authKey, sizeof(pCtx->authKey),
                                input, (int)pPreviousMessage->size + (int)pCurrentMessage->size,
                                hash);
        free(input);
    }
#endif

    memcpy( pAuthenticator, hash, WPS_AUTHENTICATOR_LENGTH );

    WPS_DBG_PRINTHEX(( "WPS authenticator", pAuthenticator, WPS_AUTHENTICATOR_LENGTH ));
}

CsrBool verifyAuthenticator( WpsContext *pCtx, WpsBuffer *pPreviousMessage, WpsBuffer *pCurrentMessage, CsrUint8 *pAuthenticator )
{
CsrBool result = TRUE;
CsrUint8 authenticator[WPS_AUTHENTICATOR_LENGTH];

    /* Do not include the 12 byte authenticator element in the current message */
    pCurrentMessage->size -= WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;
    computeAuthenticator( pCtx, pPreviousMessage, pCurrentMessage, authenticator );
    pCurrentMessage->size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    if( memcmp( authenticator, pAuthenticator, sizeof( authenticator ) ) )
    {
        WPS_DBG_PRINTHEX(( "WPS Authenticator error", pAuthenticator, WPS_AUTHENTICATOR_LENGTH ));
        WPS_DBG_PRINTHEX(( "WPS computed authenticator", authenticator, WPS_AUTHENTICATOR_LENGTH ));
        result = FALSE;
    }

    return result;
}

void computeEHash( WpsContext *pCtx, CsrUint8 *nonce, CsrUint8 *psk, CsrUint8 *pHash )
{
    /* The E-Hash is the HMAC-SHA256 of the concatenation of a secret
       nonce, PSK, the Enrollee Public Key and the Registrar Public Key,
       keyed with AuthKey. */
#ifdef WPS_OPENSSL_SHA256
    {
    HMAC_CTX hmac_ctx;

        HMAC_CTX_init( &hmac_ctx );
        HMAC_Init( &hmac_ctx, pCtx->authKey, SHA256_DIGEST_LENGTH, EVP_sha256() );
        HMAC_Update( &hmac_ctx, nonce, WPS_NONCE_LENGTH );                /* Secret Nonce*/
        HMAC_Update( &hmac_ctx, psk, WPS_NONCE_LENGTH );                  /* PSK */
        HMAC_Update( &hmac_ctx, pCtx->ePubKey, sizeof( pCtx->ePubKey ) ); /* Enrollee Public Key */
        HMAC_Update( &hmac_ctx, pCtx->rPubKey, sizeof( pCtx->rPubKey ) ); /* Registrar Public Key */
        HMAC_Final( &hmac_ctx, pHash, NULL );
    }
#else
    {
        CsrUint8 *input, *fillout;

        input = (CsrUint8 *)malloc(WPS_NONCE_LENGTH + WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey) + sizeof(pCtx->rPubKey));
        fillout = input;

        memcpy(fillout, nonce, WPS_NONCE_LENGTH);
        fillout += WPS_NONCE_LENGTH;
        memcpy(fillout, psk, WPS_NONCE_LENGTH);
        fillout += WPS_NONCE_LENGTH;
        memcpy(fillout, pCtx->ePubKey, sizeof(pCtx->ePubKey));
        fillout += sizeof( pCtx->ePubKey);
        memcpy(fillout, pCtx->rPubKey, sizeof(pCtx->rPubKey));

        CsrCryptoCallHmacSha256(NULL, NULL,
                                pCtx->authKey, WPS_SHA256_DIGEST_LENGTH,
                                input, WPS_NONCE_LENGTH + WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey) + sizeof(pCtx->rPubKey),
                                pHash);
        free(input);
    }
#endif
}

CsrBool verifyEHash( WpsContext *pCtx, CsrUint8 *nonce, CsrUint8 *psk, CsrUint8 *pHash )
{
CsrBool result = TRUE;
CsrUint8 hash[WPS_SHA256_DIGEST_LENGTH];

    computeEHash( pCtx, nonce, psk, hash );

    if( memcmp( hash, pHash, WPS_SHA256_DIGEST_LENGTH ) )
    {
        result = FALSE;
    }

    return result;
}

void initEhash( WpsContext *pCtx )
{
CsrUint8 hash[WPS_SHA256_DIGEST_LENGTH];
char pin1[5]; /* Assume maximum of 4 digit PIN halves */
char pin2[5];
int pin1_length, pin2_length;

    pin1_length = (strlen( pCtx->pin ) + 1) / 2;
    pin2_length = strlen( pCtx->pin ) - pin1_length;

    memcpy( pin1, pCtx->pin, pin1_length  );
    pin1[pin1_length] = '\0';
    memcpy( pin2, pCtx->pin + pin1_length, pin2_length );
    pin2[pin2_length] = '\0';
    WPS_DBG_PRINT((" pin = %s", pCtx->pin ));
    WPS_DBG_PRINT((" pin1 = %s", pin1 ));
    WPS_DBG_PRINT((" pin2 = %s", pin2 ));

#ifdef WPS_OPENSSL_SHA256
    HMAC( EVP_sha256(), pCtx->authKey, SHA256_DIGEST_LENGTH, (CsrUint8 *)pin1, strlen(pin1), hash, NULL );
#else
    {
        CsrCryptoCallHmacSha256(NULL, NULL,
                                pCtx->authKey, WPS_SHA256_DIGEST_LENGTH,
                                (CsrUint8 *)pin1, strlen(pin1),
                                hash);
    }
#endif
    memcpy( pCtx->psk1, hash, sizeof( pCtx->psk1 ) );
    WPS_DBG_PRINTHEX(( "WPS PSK1", pCtx->psk1, sizeof( pCtx->psk1 ) ));

#ifdef WPS_OPENSSL_SHA256
    HMAC( EVP_sha256(), pCtx->authKey, SHA256_DIGEST_LENGTH, (CsrUint8 *)pin2, strlen(pin2), hash, NULL );
#else
    {
        CsrCryptoCallHmacSha256(NULL, NULL,
                                pCtx->authKey, WPS_SHA256_DIGEST_LENGTH,
                                (CsrUint8 *)pin2, strlen(pin2),
                                hash);
    }
#endif

    memcpy( pCtx->psk2, hash, sizeof( pCtx->psk2 ) );
    WPS_DBG_PRINTHEX(( "WPS PSK2", pCtx->psk2, sizeof( pCtx->psk2 ) ));

#ifdef WPS_OPENSSL
    (void)RAND_bytes( pCtx->eSnonce1, sizeof( pCtx->eSnonce1 ) );
    (void)RAND_bytes( pCtx->eSnonce2, sizeof( pCtx->eSnonce2 ) );
#else
    WpsRandomBytes( pCtx->eSnonce1, sizeof( pCtx->eSnonce1 ) );
    WpsRandomBytes( pCtx->eSnonce2, sizeof( pCtx->eSnonce2 ) );
#endif

    WPS_DBG_PRINTHEX(( "WPS Enrollee Secret Nonce 1", pCtx->eSnonce1, sizeof( pCtx->eSnonce1 ) ));
    WPS_DBG_PRINTHEX(( "WPS Enrollee Secret Nonce 2", pCtx->eSnonce2, sizeof( pCtx->eSnonce2 ) ));
}

void decryptEncryptedSettings( WpsContext *pCtx, WpsBuffer *pEncryptedSettings, WpsBuffer *pDecryptedSettings )
{
static CsrUint8 output[1000];
CsrUint32 outlen;

    WPS_DBG_PRINTHEX(( "WPS Encrypted Settings IV", pEncryptedSettings->pStart, WPS_IV_LENGTH ));
    WPS_DBG_PRINTHEX(( "WPS Encrypted Settings", &pEncryptedSettings->pStart[WPS_IV_LENGTH], pEncryptedSettings->size - WPS_IV_LENGTH ));

#ifdef WPS_OPENSSL
    {
   	EVP_CIPHER_CTX cipher_ctx;
   	int outlen2;

        EVP_CIPHER_CTX_init( &cipher_ctx );

        /* The IV is the first 16 bytes of the Encrypted Settings */
        (void)EVP_DecryptInit( &cipher_ctx, EVP_aes_128_cbc(), pCtx->keyWrapKey, pEncryptedSettings->pStart );

        /* Encrypted data follows IV at offset 16 in Encrypted Settings */
        (void)EVP_DecryptUpdate( &cipher_ctx,
                                 output,
                                 (unsigned int *)&outlen,
                                 &pEncryptedSettings->pStart[WPS_IV_LENGTH],
                                 pEncryptedSettings->size - WPS_IV_LENGTH);

        (void)EVP_DecryptFinal( &cipher_ctx, output+outlen, &outlen2 );
        initBuffer( pDecryptedSettings, output, outlen + outlen2 );
        WPS_DBG_PRINT(( "Decrypted %d bytes from Encrypted Settings", outlen + outlen2 ));
        WPS_DBG_PRINTHEX(( "WPS Decrypted Settings", output, outlen + outlen2 ));
    }
#else
    {
        CsrUint8 iv[WPS_IV_LENGTH];

        /* IV gets modified by Aes128CbcDecrypt but original message is required for MAC computation, so use a copy of IV */
        memcpy(iv, pEncryptedSettings->pStart, WPS_IV_LENGTH);
        CsrCryptoCallAes128CbcDecrypt(NULL, NULL,
                                      pCtx->keyWrapKey, iv,
                                      &pEncryptedSettings->pStart[WPS_IV_LENGTH], pEncryptedSettings->size - WPS_IV_LENGTH,
                                      output, &outlen,
                                      PAD_MODE_PKCS7);
        initBuffer( pDecryptedSettings, output, outlen );
        WPS_DBG_PRINT(( "Decrypted %d bytes from Encrypted Settings", outlen ));
        WPS_DBG_PRINTHEX(( "WPS Decrypted Settings", output, outlen ));
    }
#endif
}

void encryptDecryptedSettings( WpsContext *pCtx, WpsBuffer *pDecryptedSettings, WpsBuffer *pEncryptedSettings )
{
static CsrUint8 output[1000];
CsrUint32 outlen;
CsrUint8 iv[WPS_IV_LENGTH];

    WPS_DBG_PRINTHEX(( "WPS Decrypted Settings", pDecryptedSettings->pStart, pDecryptedSettings->size ));

    /* Create random IV */
#ifdef WPS_OPENSSL
    (void)RAND_bytes( iv, sizeof( iv ) );
#else
    WpsRandomBytes( iv, sizeof( iv ) );
#endif

    WPS_DBG_PRINTHEX(( "WPS Encrypted Settings IV", iv, sizeof(iv) ));
    memcpy( output, iv, sizeof( iv ) );

#ifdef WPS_OPENSSL
    {
  	EVP_CIPHER_CTX cipher_ctx;
  	int outlen2;

        EVP_CIPHER_CTX_init( &cipher_ctx );

        (void)EVP_EncryptInit( &cipher_ctx, EVP_aes_128_cbc(), pCtx->keyWrapKey, iv );
        /* Encrypted data follows IV at offset 16 in Encrypted Settings */
        (void)EVP_EncryptUpdate( &cipher_ctx,
                                 output + sizeof(iv),
                                 (unsigned int *)&outlen,
                                 pDecryptedSettings->pStart,
                                 pDecryptedSettings->size);
        WPS_DBG_PRINT(( "WPS EncryptUpdate size = %d", outlen ));

        (void)EVP_EncryptFinal( &cipher_ctx, output + sizeof(iv) + outlen, &outlen2 );
        WPS_DBG_PRINT(( "WPS EncryptFinal size = %d", outlen2 ));
        initBuffer( pEncryptedSettings, output, sizeof(iv) + outlen + outlen2 );
    }
#else
    CsrCryptoCallAes128CbcEncrypt(NULL, NULL,
                                  pCtx->keyWrapKey, iv,
                                  pDecryptedSettings->pStart, pDecryptedSettings->size,
                                  output + sizeof(iv), &outlen,
                                  PAD_MODE_PKCS7);
    initBuffer( pEncryptedSettings, output, sizeof(iv) + outlen );
#endif

    WPS_DBG_PRINT(( "WPS Encrypted Settings size = %d", pEncryptedSettings->size ));
    WPS_DBG_PRINTHEX(( "WPS Encrypted Settings", pEncryptedSettings->pStart, pEncryptedSettings->size ));
}

void computeKeyWrapAuthenticator( WpsContext *pCtx, WpsBuffer *pMessage, CsrUint8 *pAuthenticator )
{
CsrUint8 hash[WPS_SHA256_DIGEST_LENGTH];

#ifdef WPS_OPENSSL_SHA256
    {
    HMAC_CTX hmac_ctx;

        HMAC_CTX_init( &hmac_ctx );
        HMAC_Init( &hmac_ctx, pCtx->authKey, sizeof(  pCtx->authKey ), EVP_sha256() );
        HMAC_Update( &hmac_ctx, pMessage->pStart, (int)pMessage->size );
        HMAC_Final( &hmac_ctx, hash, NULL );
    }
#else
    {
        CsrCryptoCallHmacSha256(NULL, NULL,
                                pCtx->authKey, sizeof(pCtx->authKey),
                                pMessage->pStart, (int)pMessage->size,
                                hash);
    }
#endif

    memcpy( pAuthenticator, hash, WPS_AUTHENTICATOR_LENGTH );
    WPS_DBG_PRINTHEX(( "WPS authenticator", pAuthenticator, WPS_AUTHENTICATOR_LENGTH ));
}

CsrBool verifyKeyWrapAuthenticator( WpsContext *pCtx, WpsBuffer *pMessage, CsrUint8 *pAuthenticator )
{
CsrBool result = TRUE;
CsrUint8 authenticator[WPS_AUTHENTICATOR_LENGTH];

    /* Do not include the 12 byte key wrap authenticator element in the calculation */
    pMessage->size -= WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;
    computeKeyWrapAuthenticator( pCtx, pMessage, authenticator );
    pMessage->size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    if( memcmp( authenticator, pAuthenticator, WPS_AUTHENTICATOR_LENGTH ) )
    {
        WPS_DBG_PRINTHEX(( "WPS authenticator", pAuthenticator, WPS_AUTHENTICATOR_LENGTH ));
        WPS_DBG_PRINTHEX(( "WPS computed authenticator", authenticator, WPS_AUTHENTICATOR_LENGTH ));
        result = FALSE;
    }

    return result;
}
