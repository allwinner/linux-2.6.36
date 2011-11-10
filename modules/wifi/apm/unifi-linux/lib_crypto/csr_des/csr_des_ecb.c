/** @file csr_des_ecb.c
 *
 * Implementation of the DES ECB (Electronic Code Book) functions
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
 *   This provides an implementation of DES ECB (Electronic Code Book) functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_des/csr_des_ecb.c#1 $
 *
 ****************************************************************************/
#include "csr_des.h"
#include "csr_types.h"
#include "csr_util.h"

/* PRIVATE CONSTANTS ********************************************************/

/* S-boxes have been re-arranged to allow direct indexing with a 6-bit value, instead of
 * using row and column indices as described in the DES specification */
static const CsrUint8 s1[] = {14,  0,  4, 15, 13,  7,  1,  4,  2, 14, 15,  2, 11, 13,  8,  1,
                            3, 10, 10,  6,  6, 12, 12, 11,  5,  9,  9,  5,  0,  3,  7,  8,
                            4, 15,  1, 12, 14,  8,  8,  2, 13,  4,  6,  9,  2,  1, 11,  7,
                           15,  5, 12, 11,  9,  3,  7, 14,  3, 10, 10,  0,  5,  6,  0, 13};

static const CsrUint8 s2[] = {15,  3,  1, 13,  8,  4, 14,  7,  6, 15, 11,  2,  3,  8,  4, 14,
                            9, 12,  7,  0,  2,  1, 13, 10, 12,  6,  0,  9,  5, 11, 10,  5,
                            0, 13, 14,  8,  7, 10, 11,  1, 10,  3,  4, 15, 13,  4,  1,  2,
                            5, 11,  8,  6, 12,  7,  6, 12,  9,  0,  3,  5,  2, 14, 15,  9};

static const CsrUint8 s3[] = {10, 13,  0,  7,  9,  0, 14,  9,  6,  3,  3,  4, 15,  6,  5, 10,
                            1,  2, 13,  8, 12,  5,  7, 14, 11, 12,  4, 11,  2, 15,  8,  1,
                           13,  1,  6, 10,  4, 13,  9,  0,  8,  6, 15,  9,  3,  8,  0,  7,
                           11,  4,  1, 15,  2, 14, 12,  3,  5, 11, 10,  5, 14,  2,  7, 12};

static const CsrUint8 s4[] = { 7, 13, 13,  8, 14, 11,  3,  5,  0,  6,  6, 15,  9,  0, 10,  3,
                            1,  4,  2,  7,  8,  2,  5, 12, 11,  1, 12, 10,  4, 14, 15,  9,
                           10,  3,  6, 15,  9,  0,  0,  6, 12, 10, 11,  1,  7, 13, 13,  8,
                           15,  9,  1,  4,  3,  5, 14, 11,  5, 12,  2,  7,  8,  2,  4, 14};

static const CsrUint8 s5[] = { 2, 14, 12, 11,  4,  2,  1, 12,  7,  4, 10,  7, 11, 13,  6,  1,
                            8,  5,  5,  0,  3, 15, 15, 10, 13,  3,  0,  9, 14,  8,  9,  6,
                            4, 11,  2,  8,  1, 12, 11,  7, 10,  1, 13, 14,  7,  2,  8, 13,
                           15,  6,  9, 15, 12,  0,  5,  9,  6, 10,  3,  4,  0,  5, 14,  3};

static const CsrUint8 s6[] = {12, 10,  1, 15, 10,  4, 15,  2,  9,  7,  2, 12,  6,  9,  8,  5,
                            0,  6, 13,  1,  3, 13,  4, 14, 14,  0,  7, 11,  5,  3, 11,  8,
                            9,  4, 14,  3, 15,  2,  5, 12,  2,  9,  8,  5, 12, 15,  3, 10,
                            7, 11,  0, 14,  4,  1, 10,  7,  1,  6, 13,  0, 11,  8,  6, 13};

static const CsrUint8 s7[] = { 4, 13, 11,  0,  2, 11, 14,  7, 15,  4,  0,  9,  8,  1, 13, 10,
                            3, 14, 12,  3,  9,  5,  7, 12,  5,  2, 10, 15,  6,  8,  1,  6,
                            1,  6,  4, 11, 11, 13, 13,  8, 12,  1,  3,  4,  7, 10, 14,  7,
                           10,  9, 15,  5,  6,  0,  8, 15,  0, 14,  5,  2,  9,  3,  2, 12};

static const CsrUint8 s8[] = {13,  1,  2, 15,  8, 13,  4,  8,  6, 10, 15,  3, 11,  7,  1,  4,
                           10, 12,  9,  5,  3,  6, 14, 11,  5,  0,  0, 14, 12,  9,  7,  2,
                            7,  2, 11,  1,  4, 14,  1,  7,  9,  4, 12, 10, 14,  8,  2, 13,
                            0, 15,  6, 12, 10,  9, 13,  0, 15,  3,  3,  5,  5,  6,  8, 11};

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void ip(CsrUint8 *plain, CsrUint32 *lhs, CsrUint32 *rhs)
{
    *lhs = 0;
    if(plain[7] & 0x40) *lhs |= 0x80000000; /* 58 */
    if(plain[6] & 0x40) *lhs |= 0x40000000; /* 50 */
    if(plain[5] & 0x40) *lhs |= 0x20000000; /* 42 */
    if(plain[4] & 0x40) *lhs |= 0x10000000; /* 34 */
    if(plain[3] & 0x40) *lhs |= 0x08000000; /* 26 */
    if(plain[2] & 0x40) *lhs |= 0x04000000; /* 18 */
    if(plain[1] & 0x40) *lhs |= 0x02000000; /* 10 */
    if(plain[0] & 0x40) *lhs |= 0x01000000; /*  2 */
    if(plain[7] & 0x10) *lhs |= 0x00800000; /* 60 */
    if(plain[6] & 0x10) *lhs |= 0x00400000; /* 52 */
    if(plain[5] & 0x10) *lhs |= 0x00200000; /* 44 */
    if(plain[4] & 0x10) *lhs |= 0x00100000; /* 36 */
    if(plain[3] & 0x10) *lhs |= 0x00080000; /* 28 */
    if(plain[2] & 0x10) *lhs |= 0x00040000; /* 20 */
    if(plain[1] & 0x10) *lhs |= 0x00020000; /* 12 */
    if(plain[0] & 0x10) *lhs |= 0x00010000; /*  4 */
    if(plain[7] & 0x04) *lhs |= 0x00008000; /* 62 */
    if(plain[6] & 0x04) *lhs |= 0x00004000; /* 54 */
    if(plain[5] & 0x04) *lhs |= 0x00002000; /* 46 */
    if(plain[4] & 0x04) *lhs |= 0x00001000; /* 38 */
    if(plain[3] & 0x04) *lhs |= 0x00000800; /* 30 */
    if(plain[2] & 0x04) *lhs |= 0x00000400; /* 22 */
    if(plain[1] & 0x04) *lhs |= 0x00000200; /* 14 */
    if(plain[0] & 0x04) *lhs |= 0x00000100; /*  6 */
    if(plain[7] & 0x01) *lhs |= 0x00000080; /* 64 */
    if(plain[6] & 0x01) *lhs |= 0x00000040; /* 56 */
    if(plain[5] & 0x01) *lhs |= 0x00000020; /* 48 */
    if(plain[4] & 0x01) *lhs |= 0x00000010; /* 40 */
    if(plain[3] & 0x01) *lhs |= 0x00000008; /* 32 */
    if(plain[2] & 0x01) *lhs |= 0x00000004; /* 24 */
    if(plain[1] & 0x01) *lhs |= 0x00000002; /* 16 */
    if(plain[0] & 0x01) *lhs |= 0x00000001; /*  8 */

    *rhs = 0;
    if(plain[7] & 0x80) *rhs |= 0x80000000; /* 57 */
    if(plain[6] & 0x80) *rhs |= 0x40000000; /* 49 */
    if(plain[5] & 0x80) *rhs |= 0x20000000; /* 41 */
    if(plain[4] & 0x80) *rhs |= 0x10000000; /* 33 */
    if(plain[3] & 0x80) *rhs |= 0x08000000; /* 25 */
    if(plain[2] & 0x80) *rhs |= 0x04000000; /* 17 */
    if(plain[1] & 0x80) *rhs |= 0x02000000; /*  9 */
    if(plain[0] & 0x80) *rhs |= 0x01000000; /*  1 */
    if(plain[7] & 0x20) *rhs |= 0x00800000; /* 59 */
    if(plain[6] & 0x20) *rhs |= 0x00400000; /* 51 */
    if(plain[5] & 0x20) *rhs |= 0x00200000; /* 43 */
    if(plain[4] & 0x20) *rhs |= 0x00100000; /* 35 */
    if(plain[3] & 0x20) *rhs |= 0x00080000; /* 27 */
    if(plain[2] & 0x20) *rhs |= 0x00040000; /* 19 */
    if(plain[1] & 0x20) *rhs |= 0x00020000; /* 11 */
    if(plain[0] & 0x20) *rhs |= 0x00010000; /*  3 */
    if(plain[7] & 0x08) *rhs |= 0x00008000; /* 61 */
    if(plain[6] & 0x08) *rhs |= 0x00004000; /* 53 */
    if(plain[5] & 0x08) *rhs |= 0x00002000; /* 45 */
    if(plain[4] & 0x08) *rhs |= 0x00001000; /* 37 */
    if(plain[3] & 0x08) *rhs |= 0x00000800; /* 29 */
    if(plain[2] & 0x08) *rhs |= 0x00000400; /* 21 */
    if(plain[1] & 0x08) *rhs |= 0x00000200; /* 13 */
    if(plain[0] & 0x08) *rhs |= 0x00000100; /*  5 */
    if(plain[7] & 0x02) *rhs |= 0x00000080; /* 63 */
    if(plain[6] & 0x02) *rhs |= 0x00000040; /* 55 */
    if(plain[5] & 0x02) *rhs |= 0x00000020; /* 47 */
    if(plain[4] & 0x02) *rhs |= 0x00000010; /* 39 */
    if(plain[3] & 0x02) *rhs |= 0x00000008; /* 31 */
    if(plain[2] & 0x02) *rhs |= 0x00000004; /* 23 */
    if(plain[1] & 0x02) *rhs |= 0x00000002; /* 15 */
    if(plain[0] & 0x02) *rhs |= 0x00000001; /*  7 */
}

static void expansion(CsrUint32 lhs, CsrUint32 rhs, CsrWord64 *e)
{
    e->hi = 0;
    if(lhs & 0x00000001) e->hi |= 0x00800000; /* 32 */
    if(lhs & 0x80000000) e->hi |= 0x00400000; /*  1 */
    if(lhs & 0x40000000) e->hi |= 0x00200000; /*  2 */
    if(lhs & 0x20000000) e->hi |= 0x00100000; /*  3 */
    if(lhs & 0x10000000) e->hi |= 0x00080000; /*  4 */
    if(lhs & 0x08000000) e->hi |= 0x00040000; /*  5 */
    if(lhs & 0x10000000) e->hi |= 0x00020000; /*  4 */
    if(lhs & 0x08000000) e->hi |= 0x00010000; /*  5 */
    if(lhs & 0x04000000) e->hi |= 0x00008000; /*  6 */
    if(lhs & 0x02000000) e->hi |= 0x00004000; /*  7 */
    if(lhs & 0x01000000) e->hi |= 0x00002000; /*  8 */
    if(lhs & 0x00800000) e->hi |= 0x00001000; /*  9 */
    if(lhs & 0x01000000) e->hi |= 0x00000800; /*  8 */
    if(lhs & 0x00800000) e->hi |= 0x00000400; /*  9 */
    if(lhs & 0x00400000) e->hi |= 0x00000200; /* 10 */
    if(lhs & 0x00200000) e->hi |= 0x00000100; /* 11 */
    if(lhs & 0x00100000) e->hi |= 0x00000080; /* 12 */
    if(lhs & 0x00080000) e->hi |= 0x00000040; /* 13 */
    if(lhs & 0x00100000) e->hi |= 0x00000020; /* 12 */
    if(lhs & 0x00080000) e->hi |= 0x00000010; /* 13 */
    if(lhs & 0x00040000) e->hi |= 0x00000008; /* 14 */
    if(lhs & 0x00020000) e->hi |= 0x00000004; /* 15 */
    if(lhs & 0x00010000) e->hi |= 0x00000002; /* 16 */
    if(lhs & 0x00008000) e->hi |= 0x00000001; /* 17 */

    e->lo = 0;
    if(lhs & 0x00010000) e->lo |= 0x00800000; /* 16 */
    if(lhs & 0x00008000) e->lo |= 0x00400000; /* 17 */
    if(lhs & 0x00004000) e->lo |= 0x00200000; /* 18 */
    if(lhs & 0x00002000) e->lo |= 0x00100000; /* 19 */
    if(lhs & 0x00001000) e->lo |= 0x00080000; /* 20 */
    if(lhs & 0x00000800) e->lo |= 0x00040000; /* 21 */
    if(lhs & 0x00001000) e->lo |= 0x00020000; /* 20 */
    if(lhs & 0x00000800) e->lo |= 0x00010000; /* 21 */
    if(lhs & 0x00000400) e->lo |= 0x00008000; /* 22 */
    if(lhs & 0x00000200) e->lo |= 0x00004000; /* 23 */
    if(lhs & 0x00000100) e->lo |= 0x00002000; /* 24 */
    if(lhs & 0x00000080) e->lo |= 0x00001000; /* 25 */
    if(lhs & 0x00000100) e->lo |= 0x00000800; /* 24 */
    if(lhs & 0x00000080) e->lo |= 0x00000400; /* 25 */
    if(lhs & 0x00000040) e->lo |= 0x00000200; /* 26 */
    if(lhs & 0x00000020) e->lo |= 0x00000100; /* 27 */
    if(lhs & 0x00000010) e->lo |= 0x00000080; /* 28 */
    if(lhs & 0x00000008) e->lo |= 0x00000040; /* 29 */
    if(lhs & 0x00000010) e->lo |= 0x00000020; /* 28 */
    if(lhs & 0x00000008) e->lo |= 0x00000010; /* 29 */
    if(lhs & 0x00000004) e->lo |= 0x00000008; /* 30 */
    if(lhs & 0x00000002) e->lo |= 0x00000004; /* 31 */
    if(lhs & 0x00000001) e->lo |= 0x00000002; /* 32 */
    if(lhs & 0x80000000) e->lo |= 0x00000001; /*  1 */
}

static CsrUint32 sbox(CsrWord64 *e, CsrWord64 *sk)
{
    CsrUint32 result;
    CsrUint32 temp;

    temp = 0;
    temp |= s1[(e->hi & 0x00FC0000) >> 18] << 28;
    temp |= s2[(e->hi & 0x0003F000) >> 12] << 24;
    temp |= s3[(e->hi & 0x00000FC0) >> 6] << 20;
    temp |= s4[e->hi  & 0x0000003F] << 16;
    temp |= s5[(e->lo & 0x00FC0000) >> 18] << 12;
    temp |= s6[(e->lo & 0x0003F000) >> 12] << 8;
    temp |= s7[(e->lo & 0x00000FC0) >> 6] << 4;
    temp |= s8[e->lo  & 0x0000003F];

    result = 0;
    if(temp & 0x00010000) result |= 0x80000000; /* 16 */
    if(temp & 0x02000000) result |= 0x40000000; /*  7 */
    if(temp & 0x00001000) result |= 0x20000000; /* 20 */
    if(temp & 0x00000800) result |= 0x10000000; /* 21 */
    if(temp & 0x00000008) result |= 0x08000000; /* 29 */
    if(temp & 0x00100000) result |= 0x04000000; /* 12 */
    if(temp & 0x00000010) result |= 0x02000000; /* 28 */
    if(temp & 0x00008000) result |= 0x01000000; /* 17 */
    if(temp & 0x80000000) result |= 0x00800000; /*  1 */
    if(temp & 0x00020000) result |= 0x00400000; /* 15 */
    if(temp & 0x00000200) result |= 0x00200000; /* 23 */
    if(temp & 0x00000040) result |= 0x00100000; /* 26 */
    if(temp & 0x08000000) result |= 0x00080000; /*  5 */
    if(temp & 0x00004000) result |= 0x00040000; /* 18 */
    if(temp & 0x00000002) result |= 0x00020000; /* 31 */
    if(temp & 0x00400000) result |= 0x00010000; /* 10 */
    if(temp & 0x40000000) result |= 0x00008000; /*  2 */
    if(temp & 0x01000000) result |= 0x00004000; /*  8 */
    if(temp & 0x00000100) result |= 0x00002000; /* 24 */
    if(temp & 0x00040000) result |= 0x00001000; /* 14 */
    if(temp & 0x00000001) result |= 0x00000800; /* 32 */
    if(temp & 0x00000020) result |= 0x00000400; /* 27 */
    if(temp & 0x20000000) result |= 0x00000200; /*  3 */
    if(temp & 0x00800000) result |= 0x00000100; /*  9 */
    if(temp & 0x00002000) result |= 0x00000080; /* 19 */
    if(temp & 0x00080000) result |= 0x00000040; /* 13 */
    if(temp & 0x00000004) result |= 0x00000020; /* 30 */
    if(temp & 0x04000000) result |= 0x00000010; /*  6 */
    if(temp & 0x00000400) result |= 0x00000008; /* 22 */
    if(temp & 0x00200000) result |= 0x00000004; /* 11 */
    if(temp & 0x10000000) result |= 0x00000002; /*  4 */
    if(temp & 0x00000080) result |= 0x00000001; /* 25 */

    return result;
}

static void inverse_ip(CsrUint32 lhs, CsrUint32 rhs, CsrUint8 *cipher)
{
    CsrMemSet(cipher, 0, 8);
    if(lhs & 0x01000000) cipher[0] |= 0x80; /* 40 */
    if(rhs & 0x01000000) cipher[0] |= 0x40; /*  8 */
    if(lhs & 0x00010000) cipher[0] |= 0x20; /* 48 */
    if(rhs & 0x00010000) cipher[0] |= 0x10; /* 16 */
    if(lhs & 0x00000100) cipher[0] |= 0x08; /* 56 */
    if(rhs & 0x00000100) cipher[0] |= 0x04; /* 24 */
    if(lhs & 0x00000001) cipher[0] |= 0x02; /* 64 */
    if(rhs & 0x00000001) cipher[0] |= 0x01; /* 32 */

    if(lhs & 0x02000000) cipher[1] |= 0x80; /* 39 */
    if(rhs & 0x02000000) cipher[1] |= 0x40; /*  7 */
    if(lhs & 0x00020000) cipher[1] |= 0x20; /* 47 */
    if(rhs & 0x00020000) cipher[1] |= 0x10; /* 15 */
    if(lhs & 0x00000200) cipher[1] |= 0x08; /* 55 */
    if(rhs & 0x00000200) cipher[1] |= 0x04; /* 23 */
    if(lhs & 0x00000002) cipher[1] |= 0x02; /* 63 */
    if(rhs & 0x00000002) cipher[1] |= 0x01; /* 31 */

    if(lhs & 0x04000000) cipher[2] |= 0x80; /* 38 */
    if(rhs & 0x04000000) cipher[2] |= 0x40; /*  6 */
    if(lhs & 0x00040000) cipher[2] |= 0x20; /* 46 */
    if(rhs & 0x00040000) cipher[2] |= 0x10; /* 14 */
    if(lhs & 0x00000400) cipher[2] |= 0x08; /* 54 */
    if(rhs & 0x00000400) cipher[2] |= 0x04; /* 22 */
    if(lhs & 0x00000004) cipher[2] |= 0x02; /* 62 */
    if(rhs & 0x00000004) cipher[2] |= 0x01; /* 30 */

    if(lhs & 0x08000000) cipher[3] |= 0x80; /* 37 */
    if(rhs & 0x08000000) cipher[3] |= 0x40; /*  5 */
    if(lhs & 0x00080000) cipher[3] |= 0x20; /* 45 */
    if(rhs & 0x00080000) cipher[3] |= 0x10; /* 13 */
    if(lhs & 0x00000800) cipher[3] |= 0x08; /* 53 */
    if(rhs & 0x00000800) cipher[3] |= 0x04; /* 21 */
    if(lhs & 0x00000008) cipher[3] |= 0x02; /* 61 */
    if(rhs & 0x00000008) cipher[3] |= 0x01; /* 29 */

    if(lhs & 0x10000000) cipher[4] |= 0x80; /* 36 */
    if(rhs & 0x10000000) cipher[4] |= 0x40; /*  4 */
    if(lhs & 0x00100000) cipher[4] |= 0x20; /* 44 */
    if(rhs & 0x00100000) cipher[4] |= 0x10; /* 12 */
    if(lhs & 0x00001000) cipher[4] |= 0x08; /* 52 */
    if(rhs & 0x00001000) cipher[4] |= 0x04; /* 20 */
    if(lhs & 0x00000010) cipher[4] |= 0x02; /* 60 */
    if(rhs & 0x00000010) cipher[4] |= 0x01; /* 28 */

    if(lhs & 0x20000000) cipher[5] |= 0x80; /* 35 */
    if(rhs & 0x20000000) cipher[5] |= 0x40; /*  3 */
    if(lhs & 0x00200000) cipher[5] |= 0x20; /* 43 */
    if(rhs & 0x00200000) cipher[5] |= 0x10; /* 11 */
    if(lhs & 0x00002000) cipher[5] |= 0x08; /* 51 */
    if(rhs & 0x00002000) cipher[5] |= 0x04; /* 19 */
    if(lhs & 0x00000020) cipher[5] |= 0x02; /* 59 */
    if(rhs & 0x00000020) cipher[5] |= 0x01; /* 27 */

    if(lhs & 0x40000000) cipher[6] |= 0x80; /* 34 */
    if(rhs & 0x40000000) cipher[6] |= 0x40; /*  2 */
    if(lhs & 0x00400000) cipher[6] |= 0x20; /* 42 */
    if(rhs & 0x00400000) cipher[6] |= 0x10; /* 10 */
    if(lhs & 0x00004000) cipher[6] |= 0x08; /* 50 */
    if(rhs & 0x00004000) cipher[6] |= 0x04; /* 18 */
    if(lhs & 0x00000040) cipher[6] |= 0x02; /* 58 */
    if(rhs & 0x00000040) cipher[6] |= 0x01; /* 26 */

    if(lhs & 0x80000000) cipher[7] |= 0x80; /* 33 */
    if(rhs & 0x80000000) cipher[7] |= 0x40; /*  1 */
    if(lhs & 0x00800000) cipher[7] |= 0x20; /* 41 */
    if(rhs & 0x00800000) cipher[7] |= 0x10; /*  9 */
    if(lhs & 0x00008000) cipher[7] |= 0x08; /* 49 */
    if(rhs & 0x00008000) cipher[7] |= 0x04; /* 17 */
    if(lhs & 0x00000080) cipher[7] |= 0x02; /* 57 */
    if(rhs & 0x00000080) cipher[7] |= 0x01; /* 25 */
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/
/*
 * Description:
 * See description in csr_des/csr_des.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoDesEcbEncrypt(CsrUint8 *input, CsrKeySchedule *ks, CsrUint8 *output)
{
    CsrInt32 i;
    CsrUint32 temp;
    CsrUint32 lhs;
    CsrUint32 rhs;
    CsrWord64 e;

    ip(input, &lhs, &rhs);

    for( i = 0; i < 16; i++ )
    {
        temp = lhs;
        lhs = rhs;
        expansion(lhs, rhs, &e);
        /* XOR expansion with round subkey */
        e.hi ^= ks->subkey[i].hi;
        e.lo ^= ks->subkey[i].lo;
        rhs = temp ^ sbox(&e, &(ks->subkey[i]));
    }

    inverse_ip(lhs, rhs, output);
}

/*
 * Description:
 * See description in csr_des/csr_des.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoDesEcbDecrypt(CsrUint8 *input, CsrKeySchedule *ks, CsrUint8 *output)
{
    CsrInt32 i;
    CsrUint32 temp;
    CsrUint32 lhs;
    CsrUint32 rhs;
    CsrWord64 e;

    ip(input, &lhs, &rhs);

    for( i = 15; i >= 0; i-- )
    {
        temp = lhs;
        lhs = rhs;
        expansion(lhs, rhs, &e);
        /* XOR expansion with round subkey */
        e.hi ^= ks->subkey[i].hi;
        e.lo ^= ks->subkey[i].lo;
        rhs = temp ^ sbox(&e, &(ks->subkey[i]));
    }

    inverse_ip(lhs, rhs, output);
}
