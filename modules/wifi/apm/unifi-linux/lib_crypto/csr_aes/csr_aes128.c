/** @file csr_aes128.c
 *
 * Implementation of the AES-128 functions
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
 *   This provides an implementation of AES-128 functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_aes/csr_aes128.c#1 $
 *
 ****************************************************************************/

#include "csr_aes128.h"
#include "csr_types.h"
#include "csr_pmalloc.h"

/* PRIVATE CONSTANTS ********************************************************/

static const CsrUint8 SubByte[] = {
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };

static const CsrUint8 InvSubByte[] = {
0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d };

static const CsrUint32 Rcon[] = { 0x00000000, 0x01000000, 0x02000000, 0x04000000,
                                     0x08000000, 0x10000000, 0x20000000, 0x40000000,
                                     0x80000000, 0x1b000000, 0x36000000 };

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static CsrUint8 gmul(CsrUint8 x, CsrUint8 y)
{
    CsrInt32 i;
    CsrUint8 r = 0;

    for (i = 0; i < 8; i++)
    {
        if (y & 0x01)
        {
            r ^= x;
        }

        if (x & 0x80)
        {
            x <<= 1;
            x ^= 0x1b;
        }
        else
        {
            x <<= 1;
        }

        y >>= 1;
    }
    return r;
}

static void SubByteTransform(CsrUint32 *state)
{
    CsrInt32 i;

    /* No endian issues as each byte processed independently */
    for (i = 0; i < 16; i++)
    {
        ((CsrUint8 *) state)[i] = SubByte[((CsrUint8 *) state)[i]];
    }
}

static void ShiftRowsTransform(CsrUint32 *state)
{
    CsrUint32 state2[4];
    CsrInt32 i;

    /* Initialise state2 array with the 1 row shift */
    state2[0] = state[1] & 0x00ff0000;
    state2[1] = state[2] & 0x00ff0000;
    state2[2] = state[3] & 0x00ff0000;
    state2[3] = state[0] & 0x00ff0000;

    for (i = 0; i < 4; i++)
    {
        state[i] &= 0xff00ffff;
        state[i] |= state2[i];
    }

    /* Initialise state2 array with the 2 row shift */
    state2[0] = state[2] & 0x0000ff00;
    state2[1] = state[3] & 0x0000ff00;
    state2[2] = state[0] & 0x0000ff00;
    state2[3] = state[1] & 0x0000ff00;

    for (i = 0; i < 4; i++)
    {
        state[i] &= 0xffff00ff;
        state[i] |= state2[i];
    }

    /* Initialise state2 array with the 3 row shift */
    state2[0] = state[3] & 0x000000ff;
    state2[1] = state[0] & 0x000000ff;
    state2[2] = state[1] & 0x000000ff;
    state2[3] = state[2] & 0x000000ff;

    for (i = 0; i < 4; i++)
    {
        state[i] &= 0xffffff00;
        state[i] |= state2[i];
    }
}

static void MixColumnsTransform(CsrUint32 *state)
{
    CsrInt32 i;
    CsrUint8 b0, b1, b2, b3;

    for (i = 0; i < 4; i++)
    {
        b0 = (CsrUint8)((state[i] >> 24) & 0xff);
        b1 = (CsrUint8)((state[i] >> 16) & 0xff);
        b2 = (CsrUint8)((state[i] >> 8) & 0xff);
        b3 = (CsrUint8)(state[i] & 0xff);

        state[i] = 0;
        state[i] |= (gmul(b0, 2) ^ gmul(b1, 3) ^ b2 ^ b3) << 24;
        state[i] |= (b0 ^ gmul(b1, 2) ^ gmul(b2, 3) ^ b3) << 16;
        state[i] |= (b0 ^ b1 ^ gmul(b2, 2) ^ gmul(b3, 3)) << 8;
        state[i] |= (gmul(b0, 3) ^ b1 ^ b2 ^ gmul(b3, 2));
    }
}

static void AddRoundKey(CsrUint32 *state, CsrInt32 round, CsrUint32 *w)
{
    CsrInt32 i;

    for (i = 0; i < 4; i++)
    {
        state[i] ^= w[(round * 4) + i];
    }
}

static void KeyExpansion(const CsrUint8 *key, CsrUint32 *expkey) /*lint -e429 */
{
    CsrInt32 i;
    CsrUint32 temp;

    /* Initialise a byte at a time to avoid endian issues */
    for (i = 0; i < 4; i++)
    {
        expkey[i] = key[(i * 4) + 0] << 24;
        expkey[i] |= key[(i * 4) + 1] << 16;
        expkey[i] |= key[(i * 4) + 2] << 8;
        expkey[i] |= key[(i * 4) + 3];
    }

    for (i = 4; i < 44; i++)
    {
        temp = expkey[i - 1];
        if (i % 4 == 0)
        {
            /* ROTWORD transform */
            temp = temp << 8 | temp >> 24;

            /* No endian issues as each byte is processed independently */
            ((CsrUint8 *) &temp)[0] = SubByte[((CsrUint8 *) &temp)[0]];
            ((CsrUint8 *) &temp)[1] = SubByte[((CsrUint8 *) &temp)[1]];
            ((CsrUint8 *) &temp)[2] = SubByte[((CsrUint8 *) &temp)[2]];
            ((CsrUint8 *) &temp)[3] = SubByte[((CsrUint8 *) &temp)[3]];

            temp ^= Rcon[i / 4];
        }
        expkey[i] = temp ^ expkey[i - 4];
    }
}

static void InitialiseState(const CsrUint8 *buffer, CsrUint32 *state)
{
    CsrInt32 i;

    /* Initialise a byte at a time to avoid endian issues */
    for (i = 0; i < 4; i++)
    {
        state[i] = buffer[(i * 4) + 0] << 24;
        state[i] |= buffer[(i * 4) + 1] << 16;
        state[i] |= buffer[(i * 4) + 2] << 8;
        state[i] |= buffer[(i * 4) + 3];
    }
}

static void OutputState(CsrUint32 *state, CsrUint8 *buffer)
{
    CsrInt32 i;

    /* Output a byte at a time to avoid endian issues */
    for (i = 0; i < 4; i++)
    {
        buffer[(i * 4) + 0] = (CsrUint8)((state[i] >> 24) & 0xff);
        buffer[(i * 4) + 1] = (CsrUint8)((state[i] >> 16) & 0xff);
        buffer[(i * 4) + 2] = (CsrUint8)((state[i] >> 8) & 0xff);
        buffer[(i * 4) + 3] = (CsrUint8)(state[i] & 0xff);
    }
}

static void InvShiftRowsTransform(CsrUint32 *state)
{
    CsrUint32 state2[4];
    CsrInt32 i;

    /* Initialise state2 array with the 1 row shift */
    state2[0] = state[3] & 0x00ff0000;
    state2[1] = state[0] & 0x00ff0000;
    state2[2] = state[1] & 0x00ff0000;
    state2[3] = state[2] & 0x00ff0000;

    for (i = 0; i < 4; i++)
    {
        state[i] &= 0xff00ffff;
        state[i] |= state2[i];
    }

    /* Initialise state2 array with the 2 row shift */
    state2[0] = state[2] & 0x0000ff00;
    state2[1] = state[3] & 0x0000ff00;
    state2[2] = state[0] & 0x0000ff00;
    state2[3] = state[1] & 0x0000ff00;

    for (i = 0; i < 4; i++)
    {
        state[i] &= 0xffff00ff;
        state[i] |= state2[i];
    }

    /* Initialise state2 array with the 3 row shift */
    state2[0] = state[1] & 0x000000ff;
    state2[1] = state[2] & 0x000000ff;
    state2[2] = state[3] & 0x000000ff;
    state2[3] = state[0] & 0x000000ff;

    for (i = 0; i < 4; i++)
    {
        state[i] &= 0xffffff00;
        state[i] |= state2[i];
    }
}

static void InvSubBytesTransform(CsrUint32 *pState)
{
    CsrInt32 i;

    /* No endian issues as each byte processed independently */
    for (i = 0; i < 16; i++)
    {
        ((CsrUint8 *) pState)[i] = InvSubByte[((CsrUint8 *) pState)[i]];
    }
}

static void InvMixColumnsTransform(CsrUint32 *state)
{
    CsrInt32 i;
    CsrUint8 b0, b1, b2, b3;

    for (i = 0; i < 4; i++)
    {
        b0 = (CsrUint8)((state[i] >> 24) & 0xff);
        b1 = (CsrUint8)((state[i] >> 16) & 0xff);
        b2 = (CsrUint8)((state[i] >> 8) & 0xff);
        b3 = (CsrUint8)(state[i] & 0xff);

        state[i] = 0;
        state[i] |= (gmul(b0, 14) ^ gmul(b1, 11) ^ gmul(b2, 13) ^ gmul(b3, 9)) << 24;
        state[i] |= (gmul(b0, 9) ^ gmul(b1, 14) ^ gmul(b2, 11) ^ gmul(b3, 13)) << 16;
        state[i] |= (gmul(b0, 13) ^ gmul(b1, 9) ^ gmul(b2, 14) ^ gmul(b3, 11)) << 8;
        state[i] |= (gmul(b0, 11) ^ gmul(b1, 13) ^ gmul(b2, 9) ^ gmul(b3, 14));
    }
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_aes/csr_aes128.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoAes128Encrypt(const CsrUint8 *key, const CsrUint8 *input, CsrUint8 *output)
{
    CsrInt32 round;
    CsrUint32 *w;
    CsrUint32 state[4];

    w = (CsrUint32*) CsrPmalloc(44 * sizeof(CsrUint32));

    KeyExpansion(key, w);
    InitialiseState(input, state);
    AddRoundKey(state, 0, w);

    for (round = 1; round < 10; round++)
    {
        SubByteTransform(state);
        ShiftRowsTransform(state);
        MixColumnsTransform(state);
        AddRoundKey(state, round, w);
    }

    SubByteTransform(state);
    ShiftRowsTransform(state);
    AddRoundKey(state, 10, w);
    OutputState(state, output);

    CsrPfree(w);
}

/*
 * Description:
 * See description in csr_aes/csr_aes128.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoAes128Decrypt(const CsrUint8 *key, unsigned const char *input, CsrUint8 *output)
{
    CsrInt32 round;
    CsrUint32 *w;
    CsrUint32 state[4];

    w = (CsrUint32*) CsrPmalloc(44 * sizeof(CsrUint32));

    KeyExpansion(key, w);
    InitialiseState(input, state);
    AddRoundKey(state, 10, w);

    for (round = 9; round >= 1; round--)
    {
        InvShiftRowsTransform(state);
        InvSubBytesTransform(state);
        AddRoundKey(state, round, w);
        InvMixColumnsTransform(state);
    }

    InvShiftRowsTransform(state);
    InvSubBytesTransform(state);
    AddRoundKey(state, 0, w);
    OutputState(state, output);

    CsrPfree(w);
}
