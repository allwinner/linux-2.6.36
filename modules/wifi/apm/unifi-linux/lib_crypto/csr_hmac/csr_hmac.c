/** @file csr_hmac.c
 *
 * CSR Crypto support functions for Keyed-Hashing Message Authentication
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
 *   Keyed-Hashing Message Authentication code implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_hmac/csr_hmac.c#1 $
 *
 ****************************************************************************/

#include "csr_hmac/csr_hmac.h" /* Include this first to check standalone compilation */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

#ifdef CSR_CRYPTO_SHA1_ENABLE
/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrHmacSha1Init(CSR_HMAC_SHA1_CTX *ctx, const CsrUint8 *key, CsrUint32 length)
{
    CsrUint8 ipad[64];
    CsrUint32 i;
    CsrUint8 hashedKey[CSR_SHA1_DIGEST_LENGTH];
    const CsrUint8 *theKey;

    for (i = 0; i < 64; i++)
    {
        ipad[i] = 0x36;
        ctx->opad[i] = 0x5c;
    }

    if (length > 64)
    {
        ctx->sha1_ctx = CsrCryptoSha1Init();
        CsrCryptoSha1Update(ctx->sha1_ctx, key, length);
        CsrCryptoSha1Final(hashedKey, ctx->sha1_ctx);
        length = CSR_SHA1_DIGEST_LENGTH;
        theKey = hashedKey;
    }
    else
    {
        theKey = key;
    }

    for (i = 0; i < length; i++)
    {
        ipad[i] ^= theKey[i];
        ctx->opad[i] ^= theKey[i];
    }

    ctx->sha1_ctx = CsrCryptoSha1Init();
    CsrCryptoSha1Update(ctx->sha1_ctx, ipad, 64);
}

/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrHmacSha1Update(CSR_HMAC_SHA1_CTX *ctx, const void *plain,
        CsrUint32 length)
{
    CsrCryptoSha1Update(ctx->sha1_ctx, plain, length);
}

/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrHmacSha1Final(CsrUint8 *digest, CSR_HMAC_SHA1_CTX *ctx)
{
    CsrCryptoSha1Final(digest, ctx->sha1_ctx);
    ctx->sha1_ctx = CsrCryptoSha1Init();
    CsrCryptoSha1Update(ctx->sha1_ctx, ctx->opad, 64);
    CsrCryptoSha1Update(ctx->sha1_ctx, digest, CSR_SHA1_DIGEST_LENGTH );
    CsrCryptoSha1Final(digest, ctx->sha1_ctx);
}

/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrHmacSha1(const CsrUint8 *key, const CsrUint32 keyLen, const CsrUint8 *plain, const CsrUint32 plainLen,
        CsrUint8 *digest)
{
    CSR_HMAC_SHA1_CTX sha1_ctx;

    CsrHmacSha1Init(&sha1_ctx, key, keyLen);
    CsrHmacSha1Update(&sha1_ctx, plain, plainLen);
    CsrHmacSha1Final(digest, &sha1_ctx);
}

#endif

#ifdef CSR_CRYPTO_MD5_ENABLE
/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoHmacMd5Init(CSR_CRYPTO_HMAC_MD5_CTX *ctx, CsrUint8 *key, CsrUint32 length)
{
    CsrUint8 ipad[64];
    CsrUint32 i;
    CsrUint8 hashedKey[CSR_CRYPTO_MD5_DIGEST_LENGTH], *theKey;

    for (i = 0; i < 64; i++)
    {
        ipad[i] = 0x36;
        ctx->opad[i] = 0x5c;
    }

    if (length > 64)
    {
        ctx->md5_ctx = CsrCryptoMd5Init();
        CsrCryptoMd5Update(ctx->md5_ctx, key, length);
        CsrCryptoMd5Final(hashedKey, ctx->md5_ctx);
        length = CSR_CRYPTO_MD5_DIGEST_LENGTH;
        theKey = hashedKey;
    }
    else
    {
        theKey = key;
    }

    for (i = 0; i < length; i++)
    {
        ipad[i] ^= theKey[i];
        ctx->opad[i] ^= theKey[i];
    }

    ctx->md5_ctx = CsrCryptoMd5Init();
    CsrCryptoMd5Update(ctx->md5_ctx, ipad, 64);
}

/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoHmacMd5Update(CSR_CRYPTO_HMAC_MD5_CTX *ctx, const void *plain,
        CsrUint32 length)
{
    CsrCryptoMd5Update(ctx->md5_ctx, plain, length);
}

/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoHmacMd5Final(CsrUint8 *digest, CSR_CRYPTO_HMAC_MD5_CTX *ctx)
{
    CsrCryptoMd5Final(digest, ctx->md5_ctx);
    ctx->md5_ctx = CsrCryptoMd5Init();
    CsrCryptoMd5Update(ctx->md5_ctx, ctx->opad, 64);
    CsrCryptoMd5Update(ctx->md5_ctx, digest, CSR_CRYPTO_MD5_DIGEST_LENGTH );
    CsrCryptoMd5Final(digest, ctx->md5_ctx);
}

/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoHmacMd5(CsrUint8 *key, CsrUint32 keyLen, CsrUint8 *plain, CsrUint32 plainLen,
        CsrUint8 *digest)
{
    CSR_CRYPTO_HMAC_MD5_CTX ctx;

    CsrCryptoHmacMd5Init(&ctx, key, keyLen);
    CsrCryptoHmacMd5Update(&ctx, plain, plainLen);
    CsrCryptoHmacMd5Final(digest, &ctx);
}
#endif

#ifdef CSR_CRYPTO_SHA256_ENABLE
/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoHmacSha256Init( CSR_CRYPTO_HMAC_SHA256_CTX *ctx, unsigned char *key, CsrUint32 length )
{
unsigned char ipad[64];
CsrUint32 i;

    for( i = 0; i < 64; i++ )
    {
        ipad[i] = 0x36;
        ctx->opad[i] = 0x5c;
    }

    if( length > 64 )
    {
        CsrCryptoSha256Init( &ctx->ctx );
        CsrCryptoSha256Update( &ctx->ctx, key, length );
        CsrCryptoSha256Final( key, &ctx->ctx );
        length = CSR_CRYPTO_SHA256_DIGEST_LENGTH;
    }

    for( i = 0; i < length; i++ )
    {
        ipad[i] ^= key[i];
        ctx->opad[i] ^= key[i];
    }

    CsrCryptoSha256Init( &ctx->ctx );
    CsrCryptoSha256Update( &ctx->ctx, ipad, 64 );
}

/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoHmacSha256Update( CSR_CRYPTO_HMAC_SHA256_CTX *ctx, const void *plain, CsrUint32 length )
{
    CsrCryptoSha256Update( &ctx->ctx, plain, length );
}

/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoHmacSha256Final( unsigned char *digest, CSR_CRYPTO_HMAC_SHA256_CTX *ctx )
{
    CsrCryptoSha256Final( digest, &ctx->ctx );
    CsrCryptoSha256Init( &ctx->ctx );
    CsrCryptoSha256Update( &ctx->ctx, ctx->opad, 64 );
    CsrCryptoSha256Update( &ctx->ctx, digest, CSR_CRYPTO_SHA256_DIGEST_LENGTH );
    CsrCryptoSha256Final( digest, &ctx->ctx );
}

/*
 * Description:
 * See description in csr_hmac.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoHmacSha256 (CsrUint8 *key, CsrUint32 keyLen, CsrUint8 *plain, CsrUint32 plainLen,
        CsrUint8 *digest)
{
    CSR_CRYPTO_HMAC_SHA256_CTX ctx;

    CsrCryptoHmacSha256Init(&ctx, key, keyLen);
    CsrCryptoHmacSha256Update(&ctx, plain, plainLen);
    CsrCryptoHmacSha256Final(digest, &ctx);
}
#endif
