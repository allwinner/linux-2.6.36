/** @file csr_bn_mont.c
 *
 * CSR Crypto support function for Bignum Montgomery Multiplication.
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
 *   BigNum implementation of Montgomery Multiplication.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_mont.c#2 $
 *
 ****************************************************************************/
#include "csr_bn_mont.h"
#include "csr_bn_base.h"
#include "csr_util.h"
#include "csr_pmalloc.h"
#include "csr_bn_mod.h"
#include "csr_bn_private.h"

/* PRIVATE MACRO DEFINITIONS ************************************************/

#ifdef CSR_HAVE_64_BIT_INTEGERS
#ifdef _MSC_VER
typedef unsigned __int64 CsrUint64;
typedef __int64 CsrInt64;
#else
#include <stdint.h>
typedef uint64_t CsrUint64;
typedef int64_t CsrInt64;
#endif
#endif

#ifndef CSR_HAVE_64_BIT_INTEGERS
#define MULADD(digit, m1, m2, carry) CsrMulAdd(&(digit), (m1), (m2), &(carry))
#else
#define MULADD(digit, m1, m2, carry) \
do \
{ \
    CsrUint64 result64; \
    result64 = (CsrUint64)(m1) * (m2) + (digit) + (carry); \
    (digit) = result64 & 0xffffffff; \
    (carry) = result64 >> 32; \
} while(0)
#endif

/* PRIVATE DATA DEFINITIONS *************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

#ifndef CSR_HAVE_64_BIT_INTEGERS
static void CsrMulAdd(CsrUint32* digit, const CsrUint32 m1, const CsrUint32 m2, CsrUint32* carry)
{
CsrUint32 a, b, c, d;
CsrUint32 ac, ad, cb, bd;
CsrUint32 temp1, temp2;

  /* The purpose of this function is to compute *digit + (m1 x m2) + *carry
     and return the result in *digit and any generated carry in *carry */

  a = m1 >> 16;
  b = m1 & 0xffff;
  c = m2 >> 16;
  d = m2 & 0xffff;

  ac = a * c;
  ad = a * d;
  cb = c * b;
  bd = b * d;

  temp1 = (bd & 0xffff) + (*digit & 0xffff) + (*carry & 0xffff);
  temp2 = (ad & 0xffff) + (cb & 0xffff) + (bd >> 16) + (temp1 >> 16) + (*digit >> 16) + (*carry >> 16);

  *digit = (temp2 << 16) | (temp1 & 0xFFFF);
  *carry = (temp2 >> 16) + ac + (ad >> 16 ) + (cb >> 16 );
}
#endif

/* Computes the inverse of x mod 2^32 for odd x only */
static CsrUint32 CsrCryptoBnInverse32(const CsrUint32 x)
{
    CsrUint32 y = 1;
    CsrUint32 mask = 0xfffffffc;
    CsrUint32 power = 2;
    CsrUint32 i;

    for(i = 1; i < 32; i++)
    {
        if(((x * y) & ~mask) >= power)
        {
            y += power;
        }
        mask  <<= 1;
        power <<= 1;
    }

    return y;
}

CsrUint32 CsrBnGetMontgomeryNp0(const CsrBignum* n)
{
    CsrUint32 np0;

    /* Get the inverse of the least significant digit of the modulus, modulo 2^32 */
    np0 = CsrCryptoBnInverse32(n->digit[0]);

    /* Negate the inverse using 2s complement */
    np0 = ~np0 + 1;

    return np0;
}

/* This function returns the square of r within the modulus n */
CsrBignum *CsrCryptoBnBinarySquareMod(const CsrBignum* r, const CsrBignum* n)
{
    CsrBignum* result = CsrCryptoBnNew();
    CsrUint32 i;
    CsrUint32 bitCount = n->length * 32;

    CsrCryptoBnSet(result, r);

    /* Square in the modulus */
    for(i = 0; i < bitCount; i++)
    {
        CsrCryptoBnShiftLeft1(result);
        if(CsrCryptoBnUnsignedCompare(result, n) > 0)
        {
            CsrCryptoBnUnsignedSubtract(result, result, n);
        }
    }

    return result;
}

/* This function returns a binary power larger than the modulus, reduced in that modulus */
CsrBignum *CsrCryptoBnGetMontgomeryR(const CsrBignum* n)
{
    CsrBignum* result = CsrCryptoBnNew();

    /* Build binary power which is larger than the modulus */
    result->length = n->length + 1;
    result->digit[result->length - 1] = 1;
    result->unusedBytes = 3;

    CsrCryptoBnMod(result, result, n);

    return result;
}

void CsrCryptoBnMonPro(CsrBignum* result, const CsrBignum* a, const CsrBignum* b, const CsrBignum* n, const CsrUint32 np0)
{
CsrUint32* c;
CsrUint32* d;
CsrUint32 carry;
CsrUint32 m;
CsrUint32 borrow;
CsrUint32 temp;
CsrUint32 i, j, k;
CsrUint32 size;

    /* Determine big number size from modulus */
    size = n->length;

    /* C must be big enough to hold a big number multiplication result and initialised to zero.
       Both C and D need one extra word for overflow */
    c = (CsrUint32 *)CsrPmalloc(((size * 2) + 1) * sizeof(CsrUint32));
    CsrMemSet(c, 0, ((size * 2) + 1) * sizeof(CsrUint32));
    d = (CsrUint32 *)CsrPmalloc((size + 1) * sizeof(CsrUint32));

    for( i = 0; i < size; i++ )
    {
        for( carry = 0, j = 0; j < size; j++ )
        {
            MULADD(c[i+j], a->digit[i], b->digit[j], carry);
        }
        c[i+size] = carry;
    }

    for( i = 0; i < size; i++ )
    {
        m = c[i] * np0;
        for( carry = 0, j = 0; j < size; j++ )
        {
            MULADD(c[i+j], m, n->digit[j], carry);
        }

        for( k = i + size; carry; k++ )
        {
            temp = c[k] + carry;
            carry = ( temp < c[k] )? 1 : 0;
            c[k] = temp;
        }
    }

    for( j = 0; j < size+1; j++ )
    {
        d[j] = c[j+size];
    }

    for( borrow = 0, i = 0; i < size; i++ )
    {
        temp = d[i] - borrow;
        borrow = (d[i] < borrow)? 1 : 0;
        c[i] = temp - n->digit[i];
        borrow = (temp < n->digit[i] || borrow)? 1 : 0;
    }
    c[size] = d[size] - borrow;
    borrow = ( d[size] < borrow )? 1 : 0;

    if( borrow )
    {
        for( i = 0; i < size; i++ )
        {
            result->digit[i] = d[i];
        }
    }
    else
    {
        for( i = 0; i < size; i++ )
        {
            result->digit[i] = c[i];
        }
    }

    result->length = size;
    result->unusedBytes = 0;

    CsrPfree(c);
    CsrPfree(d);
}

void CsrCryptoBnMultiplyMod(CsrBignum *result, CsrBignum *a, CsrBignum *b, CsrBignum *p)
{
    CsrBignum* r = CsrCryptoBnGetMontgomeryR(p);
    CsrBignum* rr = CsrCryptoBnBinarySquareMod(r, p);
    CsrUint32 np0 = CsrBnGetMontgomeryNp0(p);
    CsrBignum* one = CsrCryptoBnOneNew();
    CsrBignum* mA = CsrCryptoBnNew();
    CsrBignum* mB = CsrCryptoBnNew();
    CsrBignum* mR = CsrCryptoBnNew();

    /* Convert the operands to Montgomery form */
    CsrCryptoBnMonPro(mA, a, rr, p, np0);
    CsrCryptoBnMonPro(mB, b, rr, p, np0);

    /* Do the multiplication */
    CsrCryptoBnMonPro(mR, mA, mB, p, np0);

    /* Convert the result out of Montgomery form */
    CsrCryptoBnMonPro(result, mR, one, p, np0);

    CsrCryptoBnFree(r);
    CsrCryptoBnFree(rr);
    CsrCryptoBnFree(one);
    CsrCryptoBnFree(mA);
    CsrCryptoBnFree(mB);
    CsrCryptoBnFree(mR);
}
