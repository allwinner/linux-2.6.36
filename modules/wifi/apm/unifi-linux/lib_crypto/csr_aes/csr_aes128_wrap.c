/** @file csr_aes128_wrap.c
 *
 * Implementation of the AES-128 keywrap functions
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
 *   This provides an implementation of AES-128 keywrap functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_aes/csr_aes128_wrap.c#1 $
 *
 ****************************************************************************/

#include "csr_types.h"
#include "csr_util.h"

#include "csr_aes128_wrap.h"
#include "csr_aes128.h"
#include "csr_types.h"
#include "sme_trace/sme_trace.h"

/* PRIVATE CONSTANTS ********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_aes/csr_aes128_wrap.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoAes128Wrap(const CsrUint8 *key, const CsrUint32 n, const CsrUint8 *plain,
        CsrUint8 *cipher)
{
    CsrUint8 *a, *result, b[16];
    CsrUint32 i, j;

    a = cipher;
    result = cipher + 8;

    CsrMemSet(a, 0xa6, 8);
    CsrMemCpy(result, plain, (8 * n));

    for (j = 0; j <= 5; j++)
    {
        result = cipher + 8;
        for (i = 1; i <= n; i++)
        {
            CsrMemCpy(b, a, 8);
            CsrMemCpy(b + 8, result, 8);
            CsrCryptoAes128Encrypt(key, b, b);
            CsrMemCpy(a, b, 8);
            a[7] ^= (CsrUint8)(n * j + i);
            CsrMemCpy(result, b + 8, 8);
            result += 8;
        }
    }
}

/*
 * Description:
 * See description in csr_aes/csr_aes128_wrap.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoAes128Unwrap(const CsrUint8 *key, const CsrUint32 n, const CsrUint8 *cipher,
        CsrUint8 *plain)
{
    CsrUint8 a[8], *result, b[16];
    CsrUint32 i, j;

    CsrMemCpy(a, cipher, 8);
    CsrMemCpy(plain, cipher + 8, (8 * n));

    for (j = 0; j <= 5; j++)
    {
        result = plain + (n - 1) * 8;
        for (i = n; i >= 1; i--)
        {
            CsrMemCpy(b, a, 8);
            b[7] ^= (CsrUint8)(n * (5 - j) + i);

            CsrMemCpy(b + 8, result, 8);
            CsrCryptoAes128Decrypt(key, b, b);
            CsrMemCpy(a, b, 8);
            CsrMemCpy(result, b + 8, 8);
            result -= 8;
        }
    }

    /* a should contain 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6 at end of unwrap */
    sme_trace_hex((TR_CRYPTO_LIB, TR_LVL_INFO, "Final contents of A", a, 8));
}
