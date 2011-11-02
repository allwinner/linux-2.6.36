/** @file csr_sha1.c
 *
 * RSA Reference sha1 file
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
 *   see below
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_sha1/csr_sha1.c#1 $
 *
 ****************************************************************************/


#include "csr_sha1/csr_sha1.h"
#include "csr_types.h"
#include "sme_trace/sme_pbc.h"

/* PRIVATE DATA DEFINITIONS *************************************************/
struct CSR_CRYPTO_SHA1_CTX
{
    CsrUint32 H[5];
    CsrUint8 plain[64];
    CsrUint32 remainder;
    CsrUint32 length;
};

/* PRIVATE MACROS ***********************************************************/

#define LROTATE( x, n ) (( (x) << (n) ) | ( (x) >> (32 - (n)) ))

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void process_sha1_block(CSR_CRYPTO_SHA1_CTX *sha1_ctx, CsrUint32 M[16])
{
    CsrUint32 W[80];
    CsrInt32 t;
    CsrUint32 a, b, c, d, e;
    CsrUint32 f, k;
    CsrUint32 temp;

    for (t = 0; t < 16; t++)
    {
        W[t] = M[t];
    }

    for (t = 16; t < 80; t++)
    {
        W[t] = LROTATE( W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16], 1 );
    }

    a = sha1_ctx->H[0];
    b = sha1_ctx->H[1];
    c = sha1_ctx->H[2];
    d = sha1_ctx->H[3];
    e = sha1_ctx->H[4];

    for (t = 0; t < 80; t++)
    {
        if (t >= 0 && t <= 19)
        {
            f = (b & c) | (~b & d);
            k = 0x5a827999;
        }
        else
            if (t >= 20 && t <= 39)
            {
                f = (b ^ c ^ d);
                k = 0x6ed9eba1;
            }
            else
                if (t >= 40 && t <= 59)
                {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8f1bbcdc;
                }
                else
                {
                    f = (b ^ c ^ d);
                    k = 0xca62c1d6;
                }

        temp = LROTATE( a, 5 ) + f + e + k + W[t];
        e = d;
        d = c;
        c = LROTATE( b, 30 );
        b = a;
        a = temp;
    }

    sha1_ctx->H[0] += a;
    sha1_ctx->H[1] += b;
    sha1_ctx->H[2] += c;
    sha1_ctx->H[3] += d;
    sha1_ctx->H[4] += e;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_sha1/csr_sha1.h
 */
/*---------------------------------------------------------------------------*/
CSR_CRYPTO_SHA1_CTX *CsrCryptoSha1Init(void)
{
    CSR_CRYPTO_SHA1_CTX *sha1_ctx = CsrPmalloc(sizeof(CSR_CRYPTO_SHA1_CTX));

    sha1_ctx->H[0] = 0x67452301;
    sha1_ctx->H[1] = 0xefcdab89;
    sha1_ctx->H[2] = 0x98badcfe;
    sha1_ctx->H[3] = 0x10325476;
    sha1_ctx->H[4] = 0xc3d2e1f0;

    sha1_ctx->remainder = 0;
    sha1_ctx->length = 0;

    return sha1_ctx;
}

/*
 * Description:
 * See description in csr_sha1/csr_sha1.h
 */
/*---------------------------------------------------------------------------*/
CSR_CRYPTO_SHA1_CTX *CsrCryptoCloneSha1Context(const CSR_CRYPTO_SHA1_CTX *sha1_ctx)
{
    CSR_CRYPTO_SHA1_CTX *newCtx = CsrPmalloc(sizeof(CSR_CRYPTO_SHA1_CTX));
    CsrMemCpy(newCtx, sha1_ctx, sizeof(CSR_CRYPTO_SHA1_CTX));

    return newCtx;
}
/*
 * Description:
 * See description in csr_sha1/csr_sha1.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoSha1Update(CSR_CRYPTO_SHA1_CTX *sha1_ctx, const void *plain, CsrUint32 length)
{
    CsrUint32 M[16];
    CsrInt32 i;
    CsrUint32 offset = 0;

    while (sha1_ctx->remainder + length >= 64)
    {
        CsrMemCpy( &sha1_ctx->plain[sha1_ctx->remainder], (CsrUint8 *)plain + offset, 64 - sha1_ctx->remainder );
        offset += 64 - sha1_ctx->remainder;
        length -= 64 - sha1_ctx->remainder;
        for (i = 0; i < 16; i++)
        {
            M[i] = sha1_ctx->plain[i * 4] << 24;
            M[i] |= sha1_ctx->plain[(i * 4) + 1] << 16;
            M[i] |= sha1_ctx->plain[(i * 4) + 2] << 8;
            M[i] |= sha1_ctx->plain[(i * 4) + 3];
        }
        process_sha1_block(sha1_ctx, M);
        sha1_ctx->length += 64 - sha1_ctx->remainder;
        sha1_ctx->remainder = 0;
    }

    if (sha1_ctx->remainder + length > 0)
    {
        /* Less that one full block - just add to remainder buffer */
        CsrMemCpy( &sha1_ctx->plain[sha1_ctx->remainder], (CsrUint8 *)plain + offset, length );
        sha1_ctx->remainder += length;
        sha1_ctx->length += length;
    }
}

/*
 * Description:
 * See description in csr_sha1/csr_sha1.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoSha1Final(CsrUint8 *digest, CSR_CRYPTO_SHA1_CTX *sha1_ctx)
{
    CsrUint32 i;
    CsrUint32 M[16];

    sha1_ctx->plain[sha1_ctx->remainder++] = 0x80;

    if (sha1_ctx->remainder > 56)
    {
        for (i = sha1_ctx->remainder; i < 64; i++)
        {
            sha1_ctx->plain[i] = 0x00;
        }

        for (i = 0; i < 16; i++)
        {
            M[i] = sha1_ctx->plain[i * 4] << 24;
            M[i] |= sha1_ctx->plain[(i * 4) + 1] << 16;
            M[i] |= sha1_ctx->plain[(i * 4) + 2] << 8;
            M[i] |= sha1_ctx->plain[(i * 4) + 3];
        }
        process_sha1_block(sha1_ctx, M);
        sha1_ctx->remainder = 0;
    }

    for (i = sha1_ctx->remainder; i < 56; i++)
    {
        sha1_ctx->plain[i] = 0x00;
    }

    for (i = 0; i < 14; i++)
    {
        M[i] = sha1_ctx->plain[i * 4] << 24;
        M[i] |= sha1_ctx->plain[(i * 4) + 1] << 16;
        M[i] |= sha1_ctx->plain[(i * 4) + 2] << 8;
        M[i] |= sha1_ctx->plain[(i * 4) + 3];
    }
    M[14] = 0;
    M[15] = sha1_ctx->length * 8;

    process_sha1_block(sha1_ctx, M);

    for (i = 0; i < 5; i++)
    {
        digest[(i * 4) + 0] = (CsrUint8)(sha1_ctx->H[i] >> 24 & 0xff);
        digest[(i * 4) + 1] = (CsrUint8)(sha1_ctx->H[i] >> 16 & 0xff);
        digest[(i * 4) + 2] = (CsrUint8)(sha1_ctx->H[i] >> 8 & 0xff);
        digest[(i * 4) + 3] = (CsrUint8)(sha1_ctx->H[i] & 0xff);
    }

    CsrPfree(sha1_ctx);
}

/*
 * Description:
 * See description in csr_sha1/csr_sha1.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoSha1Final_zeroPadding(CsrUint8 *digest, CSR_CRYPTO_SHA1_CTX *sha1_ctx)
{
    CsrUint32 i;
    CsrUint32 M[16];

    sha1_ctx->plain[sha1_ctx->remainder++] = 0x00;

    if (sha1_ctx->remainder > 56)
    {
        for (i = sha1_ctx->remainder; i < 64; i++)
        {
            sha1_ctx->plain[i] = 0x00;
        }

        for (i = 0; i < 16; i++)
        {
            M[i] = sha1_ctx->plain[i * 4] << 24;
            M[i] |= sha1_ctx->plain[(i * 4) + 1] << 16;
            M[i] |= sha1_ctx->plain[(i * 4) + 2] << 8;
            M[i] |= sha1_ctx->plain[(i * 4) + 3];
        }
        process_sha1_block(sha1_ctx, M);
        sha1_ctx->remainder = 0;
    }

    for (i = sha1_ctx->remainder; i < 56; i++)
    {
        sha1_ctx->plain[i] = 0x00;
    }

    for (i = 0; i < 14; i++)
    {
        M[i] = sha1_ctx->plain[i * 4] << 24;
        M[i] |= sha1_ctx->plain[(i * 4) + 1] << 16;
        M[i] |= sha1_ctx->plain[(i * 4) + 2] << 8;
        M[i] |= sha1_ctx->plain[(i * 4) + 3];
    }
    M[14] = 0;
    M[15] = 0;

    process_sha1_block(sha1_ctx, M);

    for (i = 0; i < 5; i++)
    {
        digest[(i * 4) + 0] = (CsrUint8)(sha1_ctx->H[i] >> 24 & 0xff);
        digest[(i * 4) + 1] = (CsrUint8)(sha1_ctx->H[i] >> 16 & 0xff);
        digest[(i * 4) + 2] = (CsrUint8)(sha1_ctx->H[i] >> 8 & 0xff);
        digest[(i * 4) + 3] = (CsrUint8)(sha1_ctx->H[i] & 0xff);
    }

    CsrPfree(sha1_ctx);
}

/*
 * Description:
 * See description in csr_sha1/csr_sha1.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoSha1(CsrUint8 *plain, CsrUint16 plainLen, CsrUint8 *digest)
{
    CSR_CRYPTO_SHA1_CTX *sha1_ctx;

    require(TR_CRYPTO_LIB, digest != NULL);

    sha1_ctx = CsrCryptoSha1Init();
    CsrCryptoSha1Update(sha1_ctx, plain, plainLen);
    CsrCryptoSha1Final(digest, sha1_ctx);
}

/*
 * Description:
 * See description in csr_sha1/csr_sha1.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoSha1ZeroPadding(const CsrUint8 *plain, const CsrUint16 plainLen, CsrUint8 *digest)
{
    CSR_CRYPTO_SHA1_CTX *ctx;

    ctx = CsrCryptoSha1Init();
    CsrCryptoSha1Update(ctx, plain, plainLen);
    CsrCryptoSha1Final_zeroPadding(digest, ctx);
}
