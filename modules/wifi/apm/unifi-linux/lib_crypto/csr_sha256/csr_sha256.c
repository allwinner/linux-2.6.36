/** @file csr_sha256.c
 *
 * Implementation of SHA256 functions
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
 *   This provides an implementation of SHA256 functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_sha256/csr_sha256.c#2 $
 *
 ****************************************************************************/

#include "csr_types.h"
#include "csr_util.h"
#include "csr_sha256.h"
#include "sme_trace/sme_trace.h"

/* PRIVATE CONSTANTS ********************************************************/

static const CsrUint32 K[64] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
                                    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                                    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
                                    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                                    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
                                    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                                    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
                                    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                                    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
                                    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                                    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
                                    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                                    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
                                    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                                    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                                    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

/* PRIVATE MACROS ***********************************************************/

#define ROTR( x, n ) (( (x) >> (n) ) | ( (x) << (32 - (n)) ))
#define SHR( x, n ) ( (x) >> (n) )
#define Ch( x, y, z ) (( (x) & (y) ) ^ ( ~(x) & (z) ))
#define Maj( x, y, z) (( (x) & (y) ) ^ ( (x) & (z) ) ^ ( (y) & (z) ))

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void process_sha256_block( CSR_CRYPTO_SHA256_CTX *ctx, CsrUint32 M[16] )
{
CsrUint32 W[64];
CsrInt32 t;
CsrUint32 a,b,c,d,e,f,g,h;
CsrUint32 T1, T2;

    for( t = 0; t < 16; t++ )
    {
        W[t] = M[t];
    }

    for( t = 16; t < 64; t++ )
    {
        W[t] = ROTR(W[t-2],17) ^ ROTR(W[t-2],19) ^ SHR(W[t-2],10);
        W[t] += W[t-7];
        W[t] += ROTR(W[t-15],7) ^ ROTR(W[t-15],18) ^ SHR(W[t-15],3);
        W[t] += W[t-16];
    }

    a = ctx->H[0];
    b = ctx->H[1];
    c = ctx->H[2];
    d = ctx->H[3];
    e = ctx->H[4];
    f = ctx->H[5];
    g = ctx->H[6];
    h = ctx->H[7];

    for( t = 0; t < 64; t++ )
    {
        T1 = h;
        T1 += ROTR(e,6) ^ ROTR(e,11) ^ ROTR(e,25);
        T1 += Ch( e, f, g );
        T1 += K[t];
        T1 += W[t];
        T2 = ROTR(a,2) ^ ROTR(a,13) ^ ROTR(a,22);
        T2 += Maj( a, b, c );

        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    ctx->H[0] += a;
    ctx->H[1] += b;
    ctx->H[2] += c;
    ctx->H[3] += d;
    ctx->H[4] += e;
    ctx->H[5] += f;
    ctx->H[6] += g;
    ctx->H[7] += h;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_sha256/csr_sha256.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoSha256Init( CSR_CRYPTO_SHA256_CTX *ctx )
{
    ctx->H[0] = 0x6a09e667;
    ctx->H[1] = 0xbb67ae85;
    ctx->H[2] = 0x3c6ef372;
    ctx->H[3] = 0xa54ff53a;
    ctx->H[4] = 0x510e527f;
    ctx->H[5] = 0x9b05688c;
    ctx->H[6] = 0x1f83d9ab;
    ctx->H[7] = 0x5be0cd19;

    ctx->remainder = 0;
    ctx->length = 0;
}
/*
 * Description:
 * See description in csr_sha256/csr_sha256.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoSha256Update( CSR_CRYPTO_SHA256_CTX *ctx, const void *plain, CsrUint32 length )
{
CsrUint32 M[16];
CsrUint32 i;
CsrUint32 offset = 0;

    while( ctx->remainder + length >= 64 )
    {
        CsrMemCpy( &ctx->plain[ctx->remainder], (CsrUint8*)plain + offset, 64 - ctx->remainder );
        offset += 64 - ctx->remainder;
        length -= 64 - ctx->remainder;
        for( i = 0; i < 16; i++ )
        {
            M[i] = ctx->plain[i*4] << 24;
            M[i] |= ctx->plain[(i*4)+1] << 16;
            M[i] |= ctx->plain[(i*4)+2] << 8;
            M[i] |= ctx->plain[(i*4)+3];
        }
        process_sha256_block( ctx, M );
        ctx->length += 64 - ctx->remainder;
        ctx->remainder = 0;
    }

    if( ctx->remainder + length > 0 )
    {
        /* Less that one full block - just add to remainder buffer */
        CsrMemCpy( &ctx->plain[ctx->remainder], (CsrUint8*)plain + offset, length );
        ctx->remainder += length;
        ctx->length += length;
    }
}

/*
 * Description:
 * See description in csr_sha256/csr_sha256.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoSha256Final( unsigned char *digest, CSR_CRYPTO_SHA256_CTX *ctx )
{
CsrUint32 i;
CsrUint32 M[16];

    ctx->plain[ctx->remainder++] = 0x80;

    if( ctx->remainder > 56 )
    {
        for( i = ctx->remainder; i < 64; i++ )
        {
            ctx->plain[i] = 0x00;
        }

        for( i = 0; i < 16; i++ )
        {
            M[i] = ctx->plain[i*4] << 24;
            M[i] |= ctx->plain[(i*4)+1] << 16;
            M[i] |= ctx->plain[(i*4)+2] << 8;
            M[i] |= ctx->plain[(i*4)+3];
        }
        process_sha256_block( ctx, M );
        ctx->remainder = 0;
    }

    for( i = ctx->remainder; i < 56; i++ )
    {
        ctx->plain[i] = 0x00;
    }

    for( i = 0; i < 14; i++ )
    {
        M[i] = ctx->plain[i*4] << 24;
        M[i] |= ctx->plain[(i*4)+1] << 16;
        M[i] |= ctx->plain[(i*4)+2] << 8;
        M[i] |= ctx->plain[(i*4)+3];
    }
    M[14] = 0;
    M[15] = ctx->length * 8;

    process_sha256_block( ctx, M );

    for( i = 0; i < 8; i++ )
    {
        digest[(i*4)+0] = (CsrUint8)(ctx->H[i] >> 24 & 0xff);
        digest[(i*4)+1] = (CsrUint8)(ctx->H[i] >> 16 & 0xff);
        digest[(i*4)+2] = (CsrUint8)(ctx->H[i] >> 8 & 0xff);
        digest[(i*4)+3] = (CsrUint8)(ctx->H[i] & 0xff);
    }
}

/*
 * Description:
 * See description in csr_sha256/csr_sha256.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoSha256(const void *plain, CsrUint32 length, unsigned char *digest)
{
    CSR_CRYPTO_SHA256_CTX ctx;

    CsrCryptoSha256Init(&ctx);
    CsrCryptoSha256Update(&ctx, plain, length);
    CsrCryptoSha256Final(digest, &ctx);
}
