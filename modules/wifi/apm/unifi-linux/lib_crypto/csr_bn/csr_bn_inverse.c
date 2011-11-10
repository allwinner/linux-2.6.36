/** @file csr_bn_inverse.c
 *
 * CSR Crypto base support functions for modular inverse.
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
 *   BigNum modular inverse implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_inverse.c#1 $
 *
 ****************************************************************************/
#include "csr_bn_inverse.h"
#include "csr_bn_base.h"
#include "csr_util.h"
#include "csr_pmalloc.h"
#include "csr_bn_private.h"

/* PRIVATE DATA DEFINITIONS *************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

CsrBool CsrCryptoBnInverse(CsrBignum* result, const CsrBignum* x, const CsrBignum* n)
{
    CsrBignum* g;
    CsrBignum* u;
    CsrBignum* v;
    CsrBignum* a;
    CsrBignum* b;
    CsrBignum* c;
    CsrBignum* d;
    CsrBool isValid = FALSE;

    g = CsrCryptoBnNew();
    u = CsrCryptoBnNew();
    v = CsrCryptoBnNew();
    a = CsrCryptoBnNew();
    b = CsrCryptoBnNew();
    c = CsrCryptoBnNew();
    d = CsrCryptoBnNew();

    (void)CsrCryptoBnSetWord(g, 1);

    CsrCryptoBnSet(u, x);
    CsrCryptoBnSet(v, n);

    while(CsrCryptoBnIsEven(u) && CsrCryptoBnIsEven(v))
    {
        CsrCryptoBnShiftRight1(u);
        CsrCryptoBnShiftRight1(v);
        CsrCryptoBnShiftLeft1(g);
    }

    (void)CsrCryptoBnSetWord(a, 1);
    (void)CsrCryptoBnSetWord(b, 0);
    (void)CsrCryptoBnSetWord(c, 0);
    (void)CsrCryptoBnSetWord(d, 1);

    do
    {
        while(CsrCryptoBnIsEven(u))
        {
            CsrCryptoBnShiftRight1(u);
            if(CsrCryptoBnIsEven(a) && CsrCryptoBnIsEven(b))
            {
                CsrCryptoBnShiftRight1(a);
                CsrCryptoBnShiftRight1(b);
            }
            else
            {
                CsrCryptoBnAdd(a, a, n);
                CsrCryptoBnShiftRight1(a);
                CsrCryptoBnSubtract(b, b, x);
                CsrCryptoBnShiftRight1(b);
            }
        }

        while(CsrCryptoBnIsEven(v))
        {
            CsrCryptoBnShiftRight1(v);
            if(CsrCryptoBnIsEven(c) && CsrCryptoBnIsEven(d))
            {
                CsrCryptoBnShiftRight1(c);
                CsrCryptoBnShiftRight1(d);
            }
            else
            {
                CsrCryptoBnAdd(c, c, n);
                CsrCryptoBnShiftRight1(c);
                CsrCryptoBnSubtract(d, d, x);
                CsrCryptoBnShiftRight1(d);
            }
        }

        if(CsrCryptoBnUnsignedCompare(u, v) >= 0)
        {
            CsrCryptoBnSubtract(u, u, v);
            CsrCryptoBnSubtract(a, a, c);
            CsrCryptoBnSubtract(b, b, d);
        }
        else
        {
            CsrCryptoBnSubtract(v, v, u);
            CsrCryptoBnSubtract(c, c, a);
            CsrCryptoBnSubtract(d, d, b);
        }
    }
    while(CsrCryptoBnUnsignedCompareWord(u, 0) != 0);

    /* Final check that inverse exists */
    if(CsrCryptoBnUnsignedCompareWord(g, 1) == 0 && CsrCryptoBnUnsignedCompareWord(v, 1) == 0)
    {
        /* Normalise the result in the modulus */
        if(CsrCryptoBnUnsignedCompare(c, n) > 0)
        {
            CsrCryptoBnUnsignedSubtract(c, c, n);
        }

        if(c->negative)
        {
            CsrCryptoBnUnsignedSubtract(result, n, c);
        }
        else
        {
            CsrCryptoBnSet(result, c);
        }
        isValid = TRUE;
    }

    CsrCryptoBnFree(g);
    CsrCryptoBnFree(u);
    CsrCryptoBnFree(v);
    CsrCryptoBnFree(a);
    CsrCryptoBnFree(b);
    CsrCryptoBnFree(c);
    CsrCryptoBnFree(d);

    return isValid;
}
