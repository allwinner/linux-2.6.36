/** @file csr_des.c
 *
 * Implementation of the DES key expansion functions
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
 *   This provides an implementation of DES key expansion functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_des/csr_des_key.c#1 $
 *
 ****************************************************************************/
#include "csr_des.h"
#include "csr_types.h"

/* PRIVATE CONSTANTS ********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void rotateKeySchedule(CsrKeySchedule *ks, CsrInt32 shift)
{
    CsrInt32 i;

    for( i = 0; i < shift; i++ )
    {
        ks->c <<= 1;
        if(ks->c & 0x10000000)
        {
            ks->c |= 0x00000001;
            ks->c &= 0x0FFFFFFF;
        }

        ks->d <<= 1;
        if(ks->d & 0x10000000)
        {
            ks->d |= 0x00000001;
            ks->d &= 0x0FFFFFFF;
        }
    }
}

static void pc1(CsrUint8 *key, CsrKeySchedule *ks)
{
    /* Permuted Choice 1 */
    ks->c = 0;
    if(key[7] & 0x80) ks->c |= 0x08000000; /* 57 */
    if(key[6] & 0x80) ks->c |= 0x04000000; /* 49 */
    if(key[5] & 0x80) ks->c |= 0x02000000; /* 41 */
    if(key[4] & 0x80) ks->c |= 0x01000000; /* 33 */
    if(key[3] & 0x80) ks->c |= 0x00800000; /* 25 */
    if(key[2] & 0x80) ks->c |= 0x00400000; /* 17 */
    if(key[1] & 0x80) ks->c |= 0x00200000; /*  9 */
    if(key[0] & 0x80) ks->c |= 0x00100000; /*  1 */

    if(key[7] & 0x40) ks->c |= 0x00080000; /* 58 */
    if(key[6] & 0x40) ks->c |= 0x00040000; /* 50 */
    if(key[5] & 0x40) ks->c |= 0x00020000; /* 42 */
    if(key[4] & 0x40) ks->c |= 0x00010000; /* 34 */
    if(key[3] & 0x40) ks->c |= 0x00008000; /* 26 */
    if(key[2] & 0x40) ks->c |= 0x00004000; /* 18 */
    if(key[1] & 0x40) ks->c |= 0x00002000; /* 10 */
    if(key[0] & 0x40) ks->c |= 0x00001000; /*  2 */

    if(key[7] & 0x20) ks->c |= 0x00000800; /* 59 */
    if(key[6] & 0x20) ks->c |= 0x00000400; /* 51 */
    if(key[5] & 0x20) ks->c |= 0x00000200; /* 43 */
    if(key[4] & 0x20) ks->c |= 0x00000100; /* 35 */
    if(key[3] & 0x20) ks->c |= 0x00000080; /* 27 */
    if(key[2] & 0x20) ks->c |= 0x00000040; /* 19 */
    if(key[1] & 0x20) ks->c |= 0x00000020; /* 11 */
    if(key[0] & 0x20) ks->c |= 0x00000010; /*  3 */

    if(key[7] & 0x10) ks->c |= 0x00000008; /* 60 */
    if(key[6] & 0x10) ks->c |= 0x00000004; /* 52 */
    if(key[5] & 0x10) ks->c |= 0x00000002; /* 44 */
    if(key[4] & 0x10) ks->c |= 0x00000001; /* 36 */

    ks->d = 0;
    if(key[7] & 0x02) ks->d |= 0x08000000; /* 63 */
    if(key[6] & 0x02) ks->d |= 0x04000000; /* 55 */
    if(key[5] & 0x02) ks->d |= 0x02000000; /* 47 */
    if(key[4] & 0x02) ks->d |= 0x01000000; /* 39 */
    if(key[3] & 0x02) ks->d |= 0x00800000; /* 31 */
    if(key[2] & 0x02) ks->d |= 0x00400000; /* 23 */
    if(key[1] & 0x02) ks->d |= 0x00200000; /* 15 */
    if(key[0] & 0x02) ks->d |= 0x00100000; /*  7 */

    if(key[7] & 0x04) ks->d |= 0x00080000; /* 62 */
    if(key[6] & 0x04) ks->d |= 0x00040000; /* 54 */
    if(key[5] & 0x04) ks->d |= 0x00020000; /* 46 */
    if(key[4] & 0x04) ks->d |= 0x00010000; /* 38 */
    if(key[3] & 0x04) ks->d |= 0x00008000; /* 30 */
    if(key[2] & 0x04) ks->d |= 0x00004000; /* 22 */
    if(key[1] & 0x04) ks->d |= 0x00002000; /* 14 */
    if(key[0] & 0x04) ks->d |= 0x00001000; /*  6 */

    if(key[7] & 0x08) ks->d |= 0x00000800; /* 61 */
    if(key[6] & 0x08) ks->d |= 0x00000400; /* 53 */
    if(key[5] & 0x08) ks->d |= 0x00000200; /* 45 */
    if(key[4] & 0x08) ks->d |= 0x00000100; /* 37 */
    if(key[3] & 0x08) ks->d |= 0x00000080; /* 29 */
    if(key[2] & 0x08) ks->d |= 0x00000040; /* 21 */
    if(key[1] & 0x08) ks->d |= 0x00000020; /* 13 */
    if(key[0] & 0x08) ks->d |= 0x00000010; /*  5 */

    if(key[3] & 0x10) ks->d |= 0x00000008; /* 28 */
    if(key[2] & 0x10) ks->d |= 0x00000004; /* 20 */
    if(key[1] & 0x10) ks->d |= 0x00000002; /* 12 */
    if(key[0] & 0x10) ks->d |= 0x00000001; /*  4 */
}

static void pc2(CsrKeySchedule *ks, CsrWord64 *sk)
{
    sk->hi = 0;
    if(ks->c & 0x00004000) sk->hi |= 0x00800000; /* 14 */
    if(ks->c & 0x00000800) sk->hi |= 0x00400000; /* 17 */
    if(ks->c & 0x00020000) sk->hi |= 0x00200000; /* 11 */
    if(ks->c & 0x00000010) sk->hi |= 0x00100000; /* 24 */
    if(ks->c & 0x08000000) sk->hi |= 0x00080000; /*  1 */
    if(ks->c & 0x00800000) sk->hi |= 0x00040000; /*  5 */
    if(ks->c & 0x02000000) sk->hi |= 0x00020000; /*  3 */
    if(ks->c & 0x00000001) sk->hi |= 0x00010000; /* 28 */
    if(ks->c & 0x00002000) sk->hi |= 0x00008000; /* 15 */
    if(ks->c & 0x00400000) sk->hi |= 0x00004000; /*  6 */
    if(ks->c & 0x00000080) sk->hi |= 0x00002000; /* 21 */
    if(ks->c & 0x00040000) sk->hi |= 0x00001000; /* 10 */
    if(ks->c & 0x00000020) sk->hi |= 0x00000800; /* 23 */
    if(ks->c & 0x00000200) sk->hi |= 0x00000400; /* 19 */
    if(ks->c & 0x00010000) sk->hi |= 0x00000200; /* 12 */
    if(ks->c & 0x01000000) sk->hi |= 0x00000100; /*  4 */
    if(ks->c & 0x00000004) sk->hi |= 0x00000080; /* 26 */
    if(ks->c & 0x00100000) sk->hi |= 0x00000040; /*  8 */
    if(ks->c & 0x00001000) sk->hi |= 0x00000020; /* 16 */
    if(ks->c & 0x00200000) sk->hi |= 0x00000010; /*  7 */
    if(ks->c & 0x00000002) sk->hi |= 0x00000008; /* 27 */
    if(ks->c & 0x00000100) sk->hi |= 0x00000004; /* 20 */
    if(ks->c & 0x00008000) sk->hi |= 0x00000002; /* 13 */
    if(ks->c & 0x04000000) sk->hi |= 0x00000001; /*  2 */

    sk->lo = 0;
    if(ks->d & 0x00008000) sk->lo |= 0x00800000; /* 41 */
    if(ks->d & 0x00000010) sk->lo |= 0x00400000; /* 52 */
    if(ks->d & 0x02000000) sk->lo |= 0x00200000; /* 31 */
    if(ks->d & 0x00080000) sk->lo |= 0x00100000; /* 37 */
    if(ks->d & 0x00000200) sk->lo |= 0x00080000; /* 47 */
    if(ks->d & 0x00000002) sk->lo |= 0x00040000; /* 55 */
    if(ks->d & 0x04000000) sk->lo |= 0x00020000; /* 30 */
    if(ks->d & 0x00010000) sk->lo |= 0x00010000; /* 40 */
    if(ks->d & 0x00000020) sk->lo |= 0x00008000; /* 51 */
    if(ks->d & 0x00000800) sk->lo |= 0x00004000; /* 45 */
    if(ks->d & 0x00800000) sk->lo |= 0x00002000; /* 33 */
    if(ks->d & 0x00000100) sk->lo |= 0x00001000; /* 48 */
    if(ks->d & 0x00001000) sk->lo |= 0x00000800; /* 44 */
    if(ks->d & 0x00000080) sk->lo |= 0x00000400; /* 49 */
    if(ks->d & 0x00020000) sk->lo |= 0x00000200; /* 39 */
    if(ks->d & 0x00000001) sk->lo |= 0x00000100; /* 56 */
    if(ks->d & 0x00400000) sk->lo |= 0x00000080; /* 34 */
    if(ks->d & 0x00000008) sk->lo |= 0x00000040; /* 53 */
    if(ks->d & 0x00000400) sk->lo |= 0x00000020; /* 46 */
    if(ks->d & 0x00004000) sk->lo |= 0x00000010; /* 42 */
    if(ks->d & 0x00000040) sk->lo |= 0x00000008; /* 50 */
    if(ks->d & 0x00100000) sk->lo |= 0x00000004; /* 36 */
    if(ks->d & 0x08000000) sk->lo |= 0x00000002; /* 29 */
    if(ks->d & 0x01000000) sk->lo |= 0x00000001; /* 32 */
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_des/csr_des.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoDesSetKey(CsrUint8 *key, CsrKeySchedule *ks)
{
    CsrInt32 i;
    const CsrInt32 shift[16] = { 1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1 };

    pc1(key, ks);

    for(i = 0; i < 16; i++)
    {
        rotateKeySchedule(ks, shift[i]);
        pc2(ks, &(ks->subkey[i]));
    }
}
