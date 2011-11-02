/** @file csr_bn_mod_exp.c
 *
 * CSR Crypto support function for BigNum Modular Eponentiation.
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
 *   BigNum implementation of Modular Exponentation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_mod_exp.c#1 $
 *
 ****************************************************************************/
#include "csr_bn_mod_exp.h"
#include "csr_bn_base.h"
#include "csr_util.h"
#include "csr_pmalloc.h"
#include "csr_bn_mont.h"
#include "csr_bn_private.h"

/* PRIVATE DATA DEFINITIONS *************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/*
 * Description:
 * See description in csr_bn_mod_exp.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoBnModExp(CsrBignum* result, const CsrBignum* b, const CsrBignum* e, const CsrBignum* n, CsrBignumCtx* ctx)
{
    CsrBignum* r = CsrCryptoBnGetMontgomeryR(n);
    CsrBignum* rr = CsrCryptoBnBinarySquareMod(r, n);
    CsrUint32 np0 = CsrBnGetMontgomeryNp0(n);
    CsrBignum* b_m = CsrCryptoBnNew();
    CsrBignum* one = CsrCryptoBnOneNew();
    CsrUint32 i, j;
    CsrUint32 mask;
    CsrBool doSquare = FALSE;

    /* Compute the Montgomery representation of the base using Montgomery multiplication with r^2 */
    CsrCryptoBnMonPro(b_m, b, rr, n, np0);

    /* Starting from r, which is the Montgomery representation of 1, compute the exponentiation
       using squaring and multiplication by the base. The doSquare CsrBool ensures that there are
       no redundant squaring operations until the MSB of the exponent is encountered */
    for(i = e->length; i > 0; i--)
    {
        for(j = 0, mask = 0x80000000; j < 32; j++, mask >>= 1)
        {
            if(doSquare) CsrCryptoBnMonPro(r, r, r, n, np0);
            if(e->digit[i - 1] & mask)
            {
                CsrCryptoBnMonPro(r, r, b_m, n, np0);
                doSquare = TRUE;
            }
        }
    }

    /* Convert back out of Montgomery representation by multiplying with 1 */
    CsrCryptoBnMonPro(result, r, one, n, np0);

    CsrCryptoBnFree(r);
    CsrCryptoBnFree(rr);
    CsrCryptoBnFree(b_m);
    CsrCryptoBnFree(one);

    return 1;
}
/*
 * Description:
 * See description in csr_bn_mod_exp.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoModExp(const CsrUint8 *base, const CsrUint32 baseLength,
                     const CsrUint8 *exponent, const CsrUint32 exponentLength,
                     const CsrUint8 *prime, const CsrUint32 primeLength, CsrUint8 *result, CsrUint32 *resultLength)
{
    CsrBignumCtx* ctx;
    CsrBignum *r = NULL, *b = NULL, *e = NULL, *n = NULL;

    ctx = CsrCryptoBnCtxNew();

    b = CsrCryptoBnBinToBn(base, baseLength, b);
    e = CsrCryptoBnBinToBn(exponent, exponentLength, e);
    n = CsrCryptoBnBinToBn(prime, primeLength, n);
    r = CsrCryptoBnNew();

    if (1 != CsrCryptoBnModExp(r, b, e, n, ctx))
    {
        /* Error */
        *resultLength = 0;
    }
    else
    {
        *resultLength = CsrCryptoBnBnToBin(r, result);
    }

    CsrCryptoBnFree(b);
    CsrCryptoBnFree(e);
    CsrCryptoBnFree(n);
    CsrCryptoBnFree(r);
    CsrCryptoBnCtxFree(ctx);
}
