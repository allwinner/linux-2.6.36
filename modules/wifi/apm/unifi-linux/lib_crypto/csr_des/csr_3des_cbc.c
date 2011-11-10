/** @file csr_3des_cbc.c
 *
 * Implementation of the 3DES CBC (Cipher Block Chaining) functions
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
 *   This provides an implementation of 3DES CBC (Cipher Block Chaining) functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_des/csr_3des_cbc.c#1 $
 *
 ****************************************************************************/
#include "csr_des.h"
#include "csr_types.h"
#include "csr_util.h"

/* PRIVATE CONSTANTS ********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/
/*
 * Description:
 * See description in csr_des/csr_des.h
 */
/*---------------------------------------------------------------------------*/
void CsrCrypto3DesCbcEncrypt(CsrKeySchedule *ks1, CsrKeySchedule *ks2, CsrKeySchedule *ks3,
                         CsrUint8 *iv,
                         CsrUint8 *input, CsrUint32 inLength,
                         CsrUint8 *output, CsrUint32 *outLength,
                         CsrCryptoPadMode padMode)
{
    CsrUint32 rounds = inLength / 8;
    CsrUint32 remainder = inLength % 8;
    CsrUint32 length = 0;
    CsrUint32 i, n;
    CsrUint8 pad;

    for(n = 0; n < rounds; n++)
    {
        for(i = 0; i < 8; i++)
        {
            iv[i] ^= (input + length)[i];
        }
        CsrCrypto3DesEcbEncrypt(iv, ks1, ks2, ks3, output + length);
        CsrMemCpy(iv, output + length, 8);
        length += 8;
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
        for( pad = (CsrUint8)(8 - remainder), i = remainder; i < 8; i++ )
        {
            iv[i] ^= pad;
        }
        CsrCrypto3DesEcbEncrypt(iv, ks1, ks2, ks3, output + length);
        CsrMemCpy(iv, output + length, 8);
        *outLength = length + 8;
        break;
    case PAD_MODE_TLS:
        for( pad = (CsrUint8)(8 - remainder), i = remainder; i < 8; i++ )
        {
            iv[i] ^= pad - 1;
        }
        CsrCrypto3DesEcbEncrypt(iv, ks1, ks2, ks3, output + length);
        CsrMemCpy(iv, output + length, 8);
        *outLength = length + 8;
        break;
    default:
        *outLength = length;
        break;
    }
}

/*
 * Description:
 * See description in csr_des/csr_des.h
 */
/*---------------------------------------------------------------------------*/
void CsrCrypto3DesCbcDecrypt(CsrKeySchedule *ks1, CsrKeySchedule *ks2, CsrKeySchedule *ks3,
                         CsrUint8 *iv,
                         CsrUint8 *input, CsrUint32 inLength,
                         CsrUint8 *output, CsrUint32 *outLength,
                         CsrCryptoPadMode padMode)
{
    CsrUint32 rounds = inLength / 8;
    CsrUint32 pad;
    CsrUint32 length = 0;
    CsrUint32 i, n;

    for(n = 0; n < rounds; n++)
    {
        CsrCrypto3DesEcbDecrypt(input + length, ks1, ks2, ks3, output + length);
        for(i = 0; i < 8; i++)
        {
            (output + length)[i] ^= iv[i];
        }
        CsrMemCpy(iv, input + length, 8);
        length += 8;
    }

    /* Determine padding and set outlength */

    switch(padMode)
    {
    case PAD_MODE_PKCS7:
        pad = *(output + length - 1);
        *outLength = inLength - pad;
        break;
    case PAD_MODE_TLS:
        /* Last byte contains length of padding (excluding 1 byte length) */
        pad = *(output + length - 1);
        *outLength = (inLength - pad) - 1;
        break;

    default:
        *outLength = length;
        break;
    }
}
