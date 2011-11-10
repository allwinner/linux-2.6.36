/** @file csr_aes128_cbc.c
 *
 * Implementation of AES-128 CBC functions
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of AES-128 CBC functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_aes/csr_aes128_cbc.c#1 $
 *
 ****************************************************************************/
#include "csr_util.h"

#include "csr_aes128_cbc.h"
#include "csr_aes128.h"

/* PRIVATE CONSTANTS ********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

void CsrCryptoAes128CbcDecrypt(CsrUint8 *keyWrapKey, CsrUint8 *iv,
                               CsrUint8 *input, CsrUint32 inLength,
                               CsrUint8 *output, CsrUint32 *outLength,
                               CsrCryptoPadMode padMode)
{
    CsrUint32 rounds = inLength / 16;
    CsrUint32 i, n;
    CsrUint32 length = 0;

    for( n = 0; n < rounds; n++ )
    {
        CsrCryptoAes128Decrypt(keyWrapKey, input + length, output + length);
        for( i = 0; i < 16; i++ )
        {
            (output + length)[i] ^= iv[i];
            iv[i] = (input + length)[i];
        }
        length += 16;
    }

    /* Adjust length to lose any padding */
    switch(padMode)
    {
    case PAD_MODE_PKCS7:
        length -= output[length-1];
        break;
    case PAD_MODE_TLS:
        length -= output[length-1] + 1;
        break;
    default:
        break;
    }

    *outLength = length;
}

void CsrCryptoAes128CbcEncrypt(CsrUint8 *keyWrapKey, CsrUint8 *iv,
                               CsrUint8 *input, CsrUint32 inLength,
                               CsrUint8 *output, CsrUint32 *outLength,
                               CsrCryptoPadMode padMode)
{
    CsrUint32 rounds = inLength / 16;
    CsrUint32 remainder = inLength % 16;
    CsrUint32 i, n;
    CsrUint32 length = 0;
    CsrUint8 pad;

    for( n = 0; n < rounds; n++ )
    {
        for( i = 0; i < 16; i++ )
        {
            iv[i] ^= (input + length)[i];
        }
        CsrCryptoAes128Encrypt(keyWrapKey, iv, output + length);
        CsrMemCpy(iv, output + length, 16);
        length += 16;
    }

    /* There should be no remainder in PAD_MODE_NONE, so ignore any remainder that is present */
    if(padMode != PAD_MODE_NONE)
    {
        for( i = 0; i < remainder; i++ )
        {
            iv[i] ^= (input + length)[i];
        }
    }

    /* Add any padding */
    switch(padMode)
    {
    case PAD_MODE_PKCS7:
        for( pad = (CsrUint8)(16 - remainder), i = remainder; i < 16; i++ )
        {
            iv[i] ^= pad;
        }
        CsrCryptoAes128Encrypt(keyWrapKey, iv, output + length);
        CsrMemCpy(iv, output + length, 16);
        *outLength = length + 16;
        break;
    case PAD_MODE_TLS:
        for( pad = (CsrUint8)(16 - remainder), i = remainder; i < 16; i++ )
        {
            iv[i] ^= pad - 1;
        }
        CsrCryptoAes128Encrypt(keyWrapKey, iv, output + length);
        CsrMemCpy(iv, output + length, 16);
        *outLength = length + 16;
        break;
    default:
        *outLength = length;
        break;
    }
}
