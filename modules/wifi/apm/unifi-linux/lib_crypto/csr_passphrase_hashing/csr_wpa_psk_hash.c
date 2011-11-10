/** @file wpa_psk_hash.c
 *
 * Pairwise Master Key derivation
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
 *   Computes Pairwise Master Key from SSID and WPA passphrase
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_passphrase_hashing/csr_wpa_psk_hash.c#1 $
 *
 ****************************************************************************/

#include "csr_passphrase_hashing.h"
#include "csr_hmac/csr_hmac.h"
#include "csr_util.h"

#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void PBKDF2_F(CsrUint8 *key, CsrUint32 key_length, CsrUint8 *input,
        CsrUint32 input_length, CsrUint8 hash[20])
{
    CsrUint8 digest[20], digest2[20];
    CsrUint32 i, j;

    /* This is an implementation of the F function defined for PBKDF2 in RFC2898 using 4096 iterations of
     underlying pseudorandom function HMAC_SHA1 */

    /* Perform initial keyed hash on the input data */
    CsrHmacSha1(key, key_length, input, input_length, digest);
    CsrMemCpy(hash, digest, 20);

    for (i = 1; i < 4096; i++)
    {
        CsrHmacSha1(key, key_length, digest, 20, digest2);
        for (j = 0; j < 20; j++)
            hash[j] ^= (digest[j] = digest2[j]);
    }
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in wpa_psk_hash.h
 */
/*---------------------------------------------------------------------------*/

void CsrCryptoWpaPskHash(char *passphrase, CsrUint8 *ssid, CsrUint8 ssidLength, CsrUint8 *pmk)
{
    CsrUint8 input[36];
    CsrUint8 hash[20];

    /* The SSID is concatenated with a four-octet encoding of an integer block index, most
     significant octet first. As the underlying HMAC-SHA1 only provides 160 bits, it is
     necessary to run the PBKDF2 F function twice with block indexes of 1 and 2 to
     construct the required 256 bits for the PMK. The block index needs to be
     assigned with either 1 or 2 for the two invocations of PBKDF2_F */

    /* Concatenate SSID and block index of 1 for first invocation of PBKDF2_F */
    CsrMemCpy( input, ssid, ssidLength );

    /* Add four-octet encoding of block index, most significant octet first */
    input[ssidLength + 0] = 0;
    input[ssidLength + 1] = 0;
    input[ssidLength + 2] = 0;
    input[ssidLength + 3] = 1;

    PBKDF2_F((CsrUint8 *)passphrase, CsrStrLen(passphrase), input, ssidLength + 4, hash);
    /* Copy 160 bits to first part of 256 bit PMK */
    CsrMemCpy( pmk, hash, 20 );

    /* Update block index to 2 for the second invocation of PBKDF2_F */
    input[ssidLength + 3] = 2;

    PBKDF2_F((CsrUint8 *)passphrase, CsrStrLen(passphrase), input, ssidLength + 4, hash);
    /* Copy 96 bits to complete remainder of 256 bit PMK */
    CsrMemCpy( pmk+20, hash, 12 );
}

#endif
