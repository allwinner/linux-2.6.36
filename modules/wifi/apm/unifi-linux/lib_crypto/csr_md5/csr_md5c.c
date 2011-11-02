/** @file csr_md5c.c
 *
 * Implementation of MD5 functions
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
 *   Implementation of MD5 functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_md5/csr_md5c.c#2 $
 *
 ****************************************************************************/

#include "csr_md5/csr_md5.h"
#include "csr_types.h"
#include "csr_util.h"
#include "csr_pmalloc.h"

/* PRIVATE DATA DEFINITIONS *************************************************/
struct CSR_CRYPTO_MD5_CTX
{
    CsrUint32 ABCD[4];
    CsrUint8 plain[64];
    CsrUint32 remainder;
    CsrUint32 length;
};

/* PRIVATE MACRO DEFINITIONS *************************************************/
#define F(X,Y,Z) ((X & Y) | (~X & Z))
#define G(X,Y,Z) ((X & Z) | (Y & ~Z))
#define H(X,Y,Z) (X ^ Y ^ Z)
#define I(X,Y,Z) (Y ^ (X | ~Z))
#define LROTATE(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define OP(f,a,b,c,d,X,s,T) ( a = b + LROTATE((a + f(b,c,d) + X + T),s) )

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void process_md5_block( CSR_CRYPTO_MD5_CTX *ctx, CsrUint32 X[16] )
{
CsrUint32 A = ctx->ABCD[0];
CsrUint32 B = ctx->ABCD[1];
CsrUint32 C = ctx->ABCD[2];
CsrUint32 D = ctx->ABCD[3];

                 /* Round 1 */
/*lint -e(123)*/ OP(F,A,B,C,D,X[ 0], 7,0xd76aa478);
/*lint -e(123)*/ OP(F,D,A,B,C,X[ 1],12,0xe8c7b756);
/*lint -e(123)*/ OP(F,C,D,A,B,X[ 2],17,0x242070db);
/*lint -e(123)*/ OP(F,B,C,D,A,X[ 3],22,0xc1bdceee);
/*lint -e(123)*/ OP(F,A,B,C,D,X[ 4], 7,0xf57c0faf);
/*lint -e(123)*/ OP(F,D,A,B,C,X[ 5],12,0x4787c62a);
/*lint -e(123)*/ OP(F,C,D,A,B,X[ 6],17,0xa8304613);
/*lint -e(123)*/ OP(F,B,C,D,A,X[ 7],22,0xfd469501);
/*lint -e(123)*/ OP(F,A,B,C,D,X[ 8], 7,0x698098d8);
/*lint -e(123)*/ OP(F,D,A,B,C,X[ 9],12,0x8b44f7af);
/*lint -e(123)*/ OP(F,C,D,A,B,X[10],17,0xffff5bb1);
/*lint -e(123)*/ OP(F,B,C,D,A,X[11],22,0x895cd7be);
/*lint -e(123)*/ OP(F,A,B,C,D,X[12], 7,0x6b901122);
/*lint -e(123)*/ OP(F,D,A,B,C,X[13],12,0xfd987193);
/*lint -e(123)*/ OP(F,C,D,A,B,X[14],17,0xa679438e);
/*lint -e(123)*/ OP(F,B,C,D,A,X[15],22,0x49b40821);

                 /* Round 2 */
/*lint -e(123)*/ OP(G,A,B,C,D,X[ 1], 5,0xf61e2562);
/*lint -e(123)*/ OP(G,D,A,B,C,X[ 6], 9,0xc040b340);
/*lint -e(123)*/ OP(G,C,D,A,B,X[11],14,0x265e5a51);
/*lint -e(123)*/ OP(G,B,C,D,A,X[ 0],20,0xe9b6c7aa);
/*lint -e(123)*/ OP(G,A,B,C,D,X[ 5], 5,0xd62f105d);
/*lint -e(123)*/ OP(G,D,A,B,C,X[10], 9,0x02441453);
/*lint -e(123)*/ OP(G,C,D,A,B,X[15],14,0xd8a1e681);
/*lint -e(123)*/ OP(G,B,C,D,A,X[ 4],20,0xe7d3fbc8);
/*lint -e(123)*/ OP(G,A,B,C,D,X[ 9], 5,0x21e1cde6);
/*lint -e(123)*/ OP(G,D,A,B,C,X[14], 9,0xc33707d6);
/*lint -e(123)*/ OP(G,C,D,A,B,X[ 3],14,0xf4d50d87);
/*lint -e(123)*/ OP(G,B,C,D,A,X[ 8],20,0x455a14ed);
/*lint -e(123)*/ OP(G,A,B,C,D,X[13], 5,0xa9e3e905);
/*lint -e(123)*/ OP(G,D,A,B,C,X[ 2], 9,0xfcefa3f8);
/*lint -e(123)*/ OP(G,C,D,A,B,X[ 7],14,0x676f02d9);
/*lint -e(123)*/ OP(G,B,C,D,A,X[12],20,0x8d2a4c8a);

                 /* Round 3 */
/*lint -e(123)*/ OP(H,A,B,C,D,X[ 5], 4,0xfffa3942);
/*lint -e(123)*/ OP(H,D,A,B,C,X[ 8],11,0x8771f681);
/*lint -e(123)*/ OP(H,C,D,A,B,X[11],16,0x6d9d6122);
/*lint -e(123)*/ OP(H,B,C,D,A,X[14],23,0xfde5380c);
/*lint -e(123)*/ OP(H,A,B,C,D,X[ 1], 4,0xa4beea44);
/*lint -e(123)*/ OP(H,D,A,B,C,X[ 4],11,0x4bdecfa9);
/*lint -e(123)*/ OP(H,C,D,A,B,X[ 7],16,0xf6bb4b60);
/*lint -e(123)*/ OP(H,B,C,D,A,X[10],23,0xbebfbc70);
/*lint -e(123)*/ OP(H,A,B,C,D,X[13], 4,0x289b7ec6);
/*lint -e(123)*/ OP(H,D,A,B,C,X[ 0],11,0xeaa127fa);
/*lint -e(123)*/ OP(H,C,D,A,B,X[ 3],16,0xd4ef3085);
/*lint -e(123)*/ OP(H,B,C,D,A,X[ 6],23,0x04881d05);
/*lint -e(123)*/ OP(H,A,B,C,D,X[ 9], 4,0xd9d4d039);
/*lint -e(123)*/ OP(H,D,A,B,C,X[12],11,0xe6db99e5);
/*lint -e(123)*/ OP(H,C,D,A,B,X[15],16,0x1fa27cf8);
/*lint -e(123)*/ OP(H,B,C,D,A,X[ 2],23,0xc4ac5665);

    /* Round 4 */
/*lint -e(123)*/ OP(I,A,B,C,D,X[ 0], 6,0xf4292244);
/*lint -e(123)*/ OP(I,D,A,B,C,X[ 7],10,0x432aff97);
/*lint -e(123)*/ OP(I,C,D,A,B,X[14],15,0xab9423a7);
/*lint -e(123)*/ OP(I,B,C,D,A,X[ 5],21,0xfc93a039);
/*lint -e(123)*/ OP(I,A,B,C,D,X[12], 6,0x655b59c3);
/*lint -e(123)*/ OP(I,D,A,B,C,X[ 3],10,0x8f0ccc92);
/*lint -e(123)*/ OP(I,C,D,A,B,X[10],15,0xffeff47d);
/*lint -e(123)*/ OP(I,B,C,D,A,X[ 1],21,0x85845dd1);
/*lint -e(123)*/ OP(I,A,B,C,D,X[ 8], 6,0x6fa87e4f);
/*lint -e(123)*/ OP(I,D,A,B,C,X[15],10,0xfe2ce6e0);
/*lint -e(123)*/ OP(I,C,D,A,B,X[ 6],15,0xa3014314);
/*lint -e(123)*/ OP(I,B,C,D,A,X[13],21,0x4e0811a1);
/*lint -e(123)*/ OP(I,A,B,C,D,X[ 4], 6,0xf7537e82);
/*lint -e(123)*/ OP(I,D,A,B,C,X[11],10,0xbd3af235);
/*lint -e(123)*/ OP(I,C,D,A,B,X[ 2],15,0x2ad7d2bb);
/*lint -e(123)*/ OP(I,B,C,D,A,X[ 9],21,0xeb86d391);

    ctx->ABCD[0] += A;
    ctx->ABCD[1] += B;
    ctx->ABCD[2] += C;
    ctx->ABCD[3] += D;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_md5.h
 */
/*---------------------------------------------------------------------------*/
CSR_CRYPTO_MD5_CTX *CsrCryptoMd5Init()
{
    CSR_CRYPTO_MD5_CTX *md5_ctx = CsrPmalloc(sizeof(CSR_CRYPTO_MD5_CTX));

    md5_ctx->ABCD[0] = 0x67452301;
    md5_ctx->ABCD[1] = 0xefcdab89;
    md5_ctx->ABCD[2] = 0x98badcfe;
    md5_ctx->ABCD[3] = 0x10325476;

    md5_ctx->remainder = 0;
    md5_ctx->length = 0;

   return md5_ctx;
}

/*
 * Description:
 * See description in csr_md5.h
 */
/*---------------------------------------------------------------------------*/
CSR_CRYPTO_MD5_CTX *CsrCryptoCloneMd5Context(const CSR_CRYPTO_MD5_CTX *md5_ctx)
{
    CSR_CRYPTO_MD5_CTX *newCtx = CsrPmalloc(sizeof(CSR_CRYPTO_MD5_CTX));
    CsrMemCpy(newCtx, md5_ctx, sizeof(CSR_CRYPTO_MD5_CTX));

    return newCtx;
}

/*
 * Description:
 * See description in csr_md5.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoMd5Update(CSR_CRYPTO_MD5_CTX *md5_ctx, const void *plain, CsrUint32 length)
{
    CsrUint32 M[16];
    CsrUint32 i;
    CsrUint32 offset = 0;

    while (md5_ctx->remainder + length >= 64)
    {
        CsrMemCpy( &md5_ctx->plain[md5_ctx->remainder], (CsrUint8 *)plain + offset, 64 - md5_ctx->remainder );
        offset += 64 - md5_ctx->remainder;
        length -= 64 - md5_ctx->remainder;
        for (i = 0; i < 16; i++)
        {
            M[i] = md5_ctx->plain[i * 4];
            M[i] |= md5_ctx->plain[(i * 4) + 1] << 8;
            M[i] |= md5_ctx->plain[(i * 4) + 2] << 16;
            M[i] |= md5_ctx->plain[(i * 4) + 3] << 24;
        }
        process_md5_block(md5_ctx, M);
        md5_ctx->length += 64 - md5_ctx->remainder;
        md5_ctx->remainder = 0;
    }

    if (md5_ctx->remainder + length > 0)
    {
        /* Less that one full block - just add to remainder buffer */
        CsrMemCpy( &md5_ctx->plain[md5_ctx->remainder], (CsrUint8 *)plain + offset, length );
        md5_ctx->remainder += length;
        md5_ctx->length += length;
    }
}

/*
 * Description:
 * See description in csr_md5.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoMd5Final(CsrUint8 *digest, CSR_CRYPTO_MD5_CTX *md5_ctx)
{
    CsrUint32 i;
    CsrUint32 M[16];

    md5_ctx->plain[md5_ctx->remainder++] = 0x80;

    if (md5_ctx->remainder > 56)
    {
        for (i = md5_ctx->remainder; i < 64; i++)
        {
            md5_ctx->plain[i] = 0x00;
        }

        for (i = 0; i < 16; i++)
        {
            M[i] = md5_ctx->plain[i * 4];
            M[i] |= md5_ctx->plain[(i * 4) + 1] << 8;
            M[i] |= md5_ctx->plain[(i * 4) + 2] << 16;
            M[i] |= md5_ctx->plain[(i * 4) + 3] << 24;
        }
        process_md5_block(md5_ctx, M);
        md5_ctx->remainder = 0;
    }

    for (i = md5_ctx->remainder; i < 56; i++)
    {
        md5_ctx->plain[i] = 0x00;
    }

    for (i = 0; i < 14; i++)
    {
        M[i] = md5_ctx->plain[i * 4];
        M[i] |= md5_ctx->plain[(i * 4) + 1] << 8;
        M[i] |= md5_ctx->plain[(i * 4) + 2] << 16;
        M[i] |= md5_ctx->plain[(i * 4) + 3] << 24;
    }
    M[14] = md5_ctx->length * 8;
    M[15] = 0;

    process_md5_block(md5_ctx, M);

    for (i = 0; i < 4; i++)
    {
        digest[(i * 4) + 0] = (CsrUint8)(md5_ctx->ABCD[i] & 0xff);
        digest[(i * 4) + 1] = (CsrUint8)(md5_ctx->ABCD[i] >> 8 & 0xff);
        digest[(i * 4) + 2] = (CsrUint8)(md5_ctx->ABCD[i] >> 16 & 0xff);
        digest[(i * 4) + 3] = (CsrUint8)(md5_ctx->ABCD[i] >> 24 & 0xff);
    }

    CsrPfree(md5_ctx);
}

/*
 * Description:
 * See description in csr_md5.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoMd5(CsrUint8 *plain, CsrUint32 plainLen, CsrUint8 *digest)
{
    CSR_CRYPTO_MD5_CTX *md5_ctx;

    md5_ctx = CsrCryptoMd5Init();
    CsrCryptoMd5Update(md5_ctx, plain, plainLen);
    CsrCryptoMd5Final(digest, md5_ctx);
}
