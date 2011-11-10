/** @file csr_bn_mod.c
 *
 * CSR Crypto support function for BigNum Modular Reduction.
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
 *   BigNum Modular Reduction.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_mod.c#1 $
 *
 ****************************************************************************/
#include "csr_bn_mod.h"
#include "csr_util.h"
#include "csr_pmalloc.h"

/* PRIVATE DATA DEFINITIONS *************************************************/

void CsrCryptoBnMod(CsrBignum* remainder, const CsrBignum* x, const CsrBignum* n)
{

    if(CsrCryptoBnUnsignedCompare(x, n) < 0)
    {
            CsrCryptoBnSet(remainder, x);
    }
    else
    {
        CsrBignum* bit_weight = CsrCryptoBnNew();

        CsrCryptoBnSet(bit_weight, n);
        while(CsrCryptoBnUnsignedCompare(bit_weight, x) < 0)
        {
            CsrCryptoBnShiftLeft1(bit_weight);
        }

        CsrCryptoBnSet(remainder, x);
        while(CsrCryptoBnUnsignedCompare(remainder, n) > 0)
        {
            CsrCryptoBnShiftRight1(bit_weight);
            if(CsrCryptoBnUnsignedCompare(bit_weight, remainder) < 0)
            {
                CsrCryptoBnUnsignedSubtract(remainder, remainder, bit_weight);
            }
        }
        CsrCryptoBnFree(bit_weight);
    }
}
