/** @file csr_md4.c
 *
 * Support function for MD4 calculation.
 * Implements the MD4 Message-Digest Algorithm (RFC:1320).
 * This implementation has been tested for correctness against the test suite
 * contained within RFC:1320.
 *
 * NOTE: Tested only on a little-endian platform. The implementation takes
 * account of endianity, but it hasn't been verified on a big-endian platform
 * yet.
 *
 * NOTE2: It is only recommended for small inputs. For large input buffers
 * you'll find this is rather memory hungry. See code for notes on what can
 * be done about that.
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
 *   MD4 Message-Digest Algorithm (RFC:1320) implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_md4/csr_md4.c#1 $
 *
 ****************************************************************************/

/* Put this in and compile this file on its own (e.g. "gcc md4.c") to be able
 * to produce a runnable MD4 program useful for validating operation. */
/*define COMMAND_LINE_TEST 1*/

#ifdef COMMAND_LINE_TEST

#ifndef CsrUint8
#define CsrUint8 unsigned char
#endif

#ifndef CsrUint32
#define CsrUint32 unsigned int
#endif

#ifndef CsrBool
#define CsrBool int
#define TRUE 1
#define FALSE 0
#endif

#else /* if COMMAND_LINE_TEST */

#include "csr_types.h"
#include "csr_util.h"
#include "csr_pmalloc.h"

#endif /* if COMMAND_LINE_TEST */

/* Define a circular shift operation (no operator for this in C). */
#define BARREL_ROTATE_32(A, n) {A = (((A) << (n)) | ((A) >> (32-(n))));}

/**
 * @brief Calculate the MD4 hash of a given buffer.
 *
 * @param[in] in  buffer to hash
 * @param[in] in_len  length of buffer to hash
 * @param[out] out  a 16-octet buffer in which the hash will be stored
 *
 * @return
 *   void
 */
void CsrCryptoMd4(const CsrUint8 *const in, const CsrUint32 in_len, CsrUint8 *out/* 16 bytes*/)
{
    /* This function uses variable naming and terminology closely matching
     * RFC:1320 for clarity */

    CsrInt32 i;

    /* Start state of the Message Digest - stored like this to avoid endian
     * issues. */
    const CsrUint8 MD_init[16] = {0x01, 0x23, 0x45, 0x67,
                               0x89, 0xab, 0xcd, 0xef,
                               0xfe, 0xdc, 0xba, 0x98,
                               0x76, 0x54, 0x32, 0x10};

    /* This is our message workspace. */
    CsrUint32 *M;
    CsrInt32 N; /* length of M in 32-bit words */

    /* MD buffer.
     * Use the output buffer. The output will be the eventual state of the MD
     * buffer so we might as well process it in situ. */
    CsrUint32 *A = (CsrUint32 *)out;
    CsrUint32 *B = (CsrUint32 *)(out+4);
    CsrUint32 *C = (CsrUint32 *)(out+8);
    CsrUint32 *D = (CsrUint32 *)(out+12);
    CsrUint32 AA;
    CsrUint32 BB;
    CsrUint32 CC;
    CsrUint32 DD;

    /* Step 1. Append Padding Bits.
     * Pad input message "congruent to 448 bits, modulo 512 bits" + room for
     * step 2, i.e. modulo 64 bytes*/
    CsrUint32 total_bytes = in_len + 9 /* at least 1 byte padding + 64-bit length */;
    total_bytes += 64 - (total_bytes%64);

    /* We malloc a buffer here big enough for everything. This isn't ideal
     * for large input buffers. In such a case we'd probably want to process
     * it in small chunks and just keep a 64-byte workspace (much like the
     * reference implementation in RFC:1320) but this is adequate for our
     * purposes and simplifies matters. */
    M = (CsrUint32 *)CsrPmalloc(total_bytes);
    if (!M)
    {
        return;
    }

    /* Add padding */
    CsrMemCpy((CsrUint8 *)M, in, in_len);
    ((CsrUint8 *)M)[in_len] = 0x80;
    CsrMemSet(((CsrUint8 *)M)+in_len+1, 0, (total_bytes-in_len) - 1);
    N = total_bytes >> 2;  /* N is length in 32 bit words */

    /* Step 2. Append original bit length */
    {
        /* Note that MD4 has a 64 bit length field here, but we make
         * assumptions (for speed & effort) that limit the effective input
         * buffer size to 536870912 bytes.*/

        /* This needs to be stored little-endian. */
        CsrUint32 bit_length  = in_len << 3;
        CsrUint8 *len = (CsrUint8 *)(&M[N-2]);
        *len++ = bit_length & 0x000000ff;
        *len++ = (bit_length & 0x0000ff00) >> 8;
        *len++ = (bit_length & 0x00ff0000) >> 16;
        *len = (bit_length & 0xff000000) >> 24;
    }

    /* Step 3. Initialize MD buffer. */
    CsrMemCpy(out, MD_init, 16);

    /* Step 4. Process Message in 16-Word Blocks */
#define F_aux(X, Y, Z) (((X) & (Y)) | ((~(X)) & (Z)))
#define G_aux(X, Y, Z) (((X) & (Y)) | ((X) & (Z)) | ((Y) & (Z)))
#define H_aux(X, Y, Z) ((X) ^ (Y) ^ (Z))

    for (i=0; i<N; i+=16)
    {
        /* RFC:1320 says "copy block i into X" here, but as X is then unchanged
         * we might as well just use block i where it already is, in M[]. */

        AA = *A;
        BB = *B;
        CC = *C;
        DD = *D;

        /* Round 1 */
#define OPERATION1(a,b,c,d,k,s) a += (F_aux(b,c,d)) + M[i+k]; BARREL_ROTATE_32(a,s)
        OPERATION1(*A,*B,*C,*D,0,3);
        OPERATION1(*D,*A,*B,*C,1,7);
        OPERATION1(*C,*D,*A,*B,2,11);
        OPERATION1(*B,*C,*D,*A,3,19);

        OPERATION1(*A,*B,*C,*D,4,3);
        OPERATION1(*D,*A,*B,*C,5,7);
        OPERATION1(*C,*D,*A,*B,6,11);
        OPERATION1(*B,*C,*D,*A,7,19);

        OPERATION1(*A,*B,*C,*D,8,3);
        OPERATION1(*D,*A,*B,*C,9,7);
        OPERATION1(*C,*D,*A,*B,10,11);
        OPERATION1(*B,*C,*D,*A,11,19);

        OPERATION1(*A,*B,*C,*D,12,3);
        OPERATION1(*D,*A,*B,*C,13,7);
        OPERATION1(*C,*D,*A,*B,14,11);
        OPERATION1(*B,*C,*D,*A,15,19);

        /* Round 2 */
#define OPERATION2(a,b,c,d,k,s) a += (G_aux(b,c,d)) + M[i+k] + 0x5a827999; BARREL_ROTATE_32(a,s)
        OPERATION2(*A,*B,*C,*D,0,3);
        OPERATION2(*D,*A,*B,*C,4,5);
        OPERATION2(*C,*D,*A,*B,8,9);
        OPERATION2(*B,*C,*D,*A,12,13);

        OPERATION2(*A,*B,*C,*D,1,3);
        OPERATION2(*D,*A,*B,*C,5,5);
        OPERATION2(*C,*D,*A,*B,9,9);
        OPERATION2(*B,*C,*D,*A,13,13);

        OPERATION2(*A,*B,*C,*D,2,3);
        OPERATION2(*D,*A,*B,*C,6,5);
        OPERATION2(*C,*D,*A,*B,10,9);
        OPERATION2(*B,*C,*D,*A,14,13);

        OPERATION2(*A,*B,*C,*D,3,3);
        OPERATION2(*D,*A,*B,*C,7,5);
        OPERATION2(*C,*D,*A,*B,11,9);
        OPERATION2(*B,*C,*D,*A,15,13);

        /* Round 3 */
#define OPERATION3(a,b,c,d,k,s) a += (H_aux(b,c,d)) + M[i+k] + 0x6ed9eba1;  BARREL_ROTATE_32(a,s)
        OPERATION3(*A,*B,*C,*D,0,3);
        OPERATION3(*D,*A,*B,*C,8,9);
        OPERATION3(*C,*D,*A,*B,4,11);
        OPERATION3(*B,*C,*D,*A,12,15);

        OPERATION3(*A,*B,*C,*D,2,3);
        OPERATION3(*D,*A,*B,*C,10,9);
        OPERATION3(*C,*D,*A,*B,6,11);
        OPERATION3(*B,*C,*D,*A,14,15);

        OPERATION3(*A,*B,*C,*D,1,3);
        OPERATION3(*D,*A,*B,*C,9,9);
        OPERATION3(*C,*D,*A,*B,5,11);
        OPERATION3(*B,*C,*D,*A,13,15);

        OPERATION3(*A,*B,*C,*D,3,3);
        OPERATION3(*D,*A,*B,*C,11,9);
        OPERATION3(*C,*D,*A,*B,7,11);
        OPERATION3(*B,*C,*D,*A,15,15);

        *A += AA;
        *B += BB;
        *C += CC;
        *D += DD;
    } /* for i */

    /* Step 5 Output. Copy MD to output buffer - already done. */

    CsrPfree(M);
}

#ifdef COMMAND_LINE_TEST

/* This function is complied in for COMMAND_LINE_TEST only and enables testing. */
CsrInt32 main(int argc, char **argv)
{
    unsigned char out[16], i;
    if (argc<2)
        md4("", 0, out);
    else
        md4(argv[1], CsrStrLen(argv[1]), out);
    for (i=0; i<16; i++)
        printf("%02x ", out[i]);
    printf("\n");
    return 0;
}

#endif /* if COMMAND_LINE_TEST */
