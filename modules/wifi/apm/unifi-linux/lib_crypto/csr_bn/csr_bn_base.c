/** @file csr_bn_base.c
 *
 * CSR Crypto base support functions for BigNum calculation.
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
 *   BigNum base support implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_base.c#3 $
 *
 ****************************************************************************/
#include "csr_bn_base.h"
#include "csr_util.h"
#include "csr_pmalloc.h"
#include "csr_bn_private.h"

/* PRIVATE DATA DEFINITIONS *************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* This function removes any leading zeros from a big number */
static void CsrCryptoBnTrim(CsrBignum *a)
{
    CsrUint32 *digit;
    CsrUint32 length;
    CsrUint32 i;

    length = a->length;
    digit = &a->digit[length - 1];

    for(i = 1; i < length; i++)
    {
        if(*digit != 0) break;
        digit--;
        a->length--;
    }

    a->unusedBytes = 0;
    /* Update unusedBytes to account for any leading 0x00 octets */
    if((*digit & 0xff000000) == 0) a->unusedBytes++;
    if((*digit & 0xffff0000) == 0) a->unusedBytes++;
    if((*digit & 0xffffff00) == 0) a->unusedBytes++;
}


void CsrCryptoBnUnsignedAdd(CsrBignum* result, const CsrBignum* a, const CsrBignum* b)
{
    CsrUint32 i;
    CsrUint32 carry = 0;
    CsrUint32 digit;

    result->length = (a->length > b->length)? a->length : b->length;
    for(i = 0; i < result->length; i++)
    {
        digit = a->digit[i];
        result->digit[i] = a->digit[i] + b->digit[i];
        if(result->digit[i] < digit)
        {
            result->digit[i] += carry;
            carry = 1;
        }
        else
        {
            result->digit[i] += carry;
            carry = 0;
        }
    }

    result->digit[result->length] = carry;
    result->length++;

    CsrCryptoBnTrim(result);
}

/* This function converts a two character hex value pointed to by s into a CsrUint8 */
static CsrUint8 CsrCryptoBnHexToBin(const char* s)
{
    CsrUint8 result = 0;
    CsrUint32 i;

    for(i = 0; i < 2; i++)
    {
        result <<= 4;
        if(s[i] >= 'A' && s[i] <= 'F') result |= ((s[i] - 'A') + 10);
        else
        if(s[i] >= 'a' && s[i] <= 'f') result |= ((s[i] - 'a') + 10);
        else
        if(s[i] >= '0' && s[i] <= '9') result |= (s[i] - '0');
    }

    return result;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
CsrBignumCtx* CsrCryptoBnCtxNew(void)
{
    /* This context is not currently used by the CSR big number library */
    return NULL;
}
/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoBnBnToBin(const CsrBignum* a, CsrUint8* to)
{
    CsrUint32 length = a->length;
    CsrUint32 i;
    CsrUint8* data;

    /* Convert from the little endian big number format into a big endian byte buffer */
    for( i = 0, data = to + (length * 4) - a->unusedBytes; i < length; i++ )
    {
        *--data = (CsrUint8)(a->digit[i] & 0xff);
        *--data = (CsrUint8)((a->digit[i] >> 8) & 0xff);
        *--data = (CsrUint8)((a->digit[i] >> 16) & 0xff);
        *--data = (CsrUint8)((a->digit[i] >> 24) & 0xff);
    }

    /* Output any remainder bytes */
    if(--data >= to)
    {
        *data = (CsrUint8)(a->digit[i] & 0xff);
        if(--data >= to)
        {
            *data = (CsrUint8)((a->digit[i] >> 8) & 0xff);
            if(--data >= to)
            {
                *data = (CsrUint8)((a->digit[i] >> 16) & 0xff);
            }
        }
    }

    return (a->length * 4) - a->unusedBytes;
}

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoBnNumBits(const CsrBignum* a)
{
    return (a->length * 32) - (a->unusedBytes * 8);
}

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoBnNumOctets(const CsrBignum* a)
{
    return (a->length * 4) - a->unusedBytes;
}

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoBnFree(CsrBignum* a)
{
    CsrPfree(a);
}

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoBnCtxFree(CsrBignumCtx* bnContext)
{
    /* Nothing to do */
}

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
CsrBignum* CsrCryptoBnBinToBn(const CsrUint8* s, const CsrUint32 len, CsrBignum* result)
{
    CsrUint32 i;
    const CsrUint8* data;

    if(result == NULL)
    {
        result = CsrCryptoBnNew();
    }

    result->length = (len + 3) / 4;
    result->unusedBytes = (result->length * 4) - len;

    /* Convert from a big endian byte buffer into the little endian big number integer format */
    /* Transfer everything that can be packed into complete 32 bit digits */
    for( i = 0, data = s + len; i < len / 4; i++ )
    {
        result->digit[i] =  *--data;
        result->digit[i] |= *--data << 8;
        result->digit[i] |= *--data << 16;
        result->digit[i] |= *--data << 24;
    }

    /* Pack any remainder bytes into the final 32 bit digit */
    if(--data >= s)
    {
        result->digit[i] = *data;
        if(--data >= s)
        {
            result->digit[i] |= *data << 8;
            if(--data >= s)
            {
                result->digit[i] |= *data << 16;
            }
        }
    }

    CsrCryptoBnTrim(result);

    return result;
}

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
CsrBignum* CsrCryptoBnHexToBn(const char* s, CsrBignum* result)
{
    CsrUint32 i;
    CsrUint32 length;
    const char* data;

    if(result == NULL)
    {
        result = CsrCryptoBnNew();
    }

    length = CsrStrLen(s) / 2;
    result->length = (length + 3) / 4;
    result->unusedBytes = (result->length * 4) - length;

    /* Convert from a big endian byte buffer into the little endian big number integer format */
    /* Transfer everything that can be packed into complete 32 bit digits */
    for( i = 0, data = s + ((length - 1) * 2); i < length / 4; i++ )
    {
        result->digit[i] =  CsrCryptoBnHexToBin(data);
        data -= 2;
        result->digit[i] |= CsrCryptoBnHexToBin(data) << 8;
        data -= 2;
        result->digit[i] |= CsrCryptoBnHexToBin(data) << 16;
        data -= 2;
        result->digit[i] |= CsrCryptoBnHexToBin(data) << 24;
        data -= 2;
    }

    /* Pack any remainder bytes into the final 32 bit digit */
    if(data >= s)
    {
        result->digit[i] = CsrCryptoBnHexToBin(data);
        data -= 2;
        if(data >= s)
        {
            result->digit[i] |= CsrCryptoBnHexToBin(data) << 8;
            data -= 2;
            if(data >= s)
            {
                result->digit[i] |= CsrCryptoBnHexToBin(data) << 16;
            }
        }
    }

    CsrCryptoBnTrim(result);

    return result;
}

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
CsrBignum* CsrCryptoBnNew(void)
{
    CsrBignum* result = (CsrBignum *)CsrPmalloc(sizeof(CsrBignum));
    CsrMemSet(result, 0, sizeof(CsrBignum));
    return result;
}

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
CsrBignum* CsrCryptoBnOneNew(void)
{
    CsrBignum* result = (CsrBignum *)CsrPmalloc(sizeof(CsrBignum));
    CsrMemSet(result, 0, sizeof(CsrBignum));
    result->length = 1;
    result->digit[0] = 1;
    return result;
}

/*
 * Description:
 * See description in csr_bn.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoBnSetWord(CsrBignum* a, const CsrUint32 w)
{
    CsrMemSet(a, 0, sizeof(CsrBignum));
    a->digit[0] = w;
    a->length = 1;
    a->unusedBytes = 0;

    /* return 1 for success */
    return 1;
}

/* This function performs an unsigned compare between two big numbers */
CsrInt32 CsrCryptoBnUnsignedCompare(const CsrBignum* a, const CsrBignum* b)
{
    CsrInt32 result;
    CsrUint32 i;
    CsrUint32 length;
    const CsrUint32 *digitA, *digitB;

    /* Set the length to match the biggest operand - this assumes the smaller big number will extend into zeros */
    length = (a->length > b->length)? a->length : b->length;

    /* Work from the most significant digit downwards */
    digitA = &a->digit[length - 1];
    digitB = &b->digit[length - 1];

    for(i = 0; i < length; i++)
    {
        if(*digitA != *digitB) break;
        digitA--;
        digitB--;
    }

    if(i >= length)
    {
        /* Equality */
        result = 0;
    }
    else
    if(*digitA < *digitB)
    {
        /* bnA < bnB */
        result = -1;
    }
    else
    {
        /* bnA > bnB */
        result = 1;
    }

    return result;
}

void CsrCryptoBnSet(CsrBignum* a, const CsrBignum* b)
{
    if(a != b)
    {
        CsrMemCpy(a, b, sizeof(CsrBignum));
    }
}

void CsrCryptoBnShiftLeft1(CsrBignum* a)
{
    CsrUint32 i;
    CsrUint32 carry = 0, next_carry = 0;
    CsrUint32 *digit;

    /* Lengthen by one to allow for any overflow */
    a->length += 1;

    for(i = 0, digit = a->digit; i < a->length; i++, digit++)
    {
        next_carry = *digit & 0x80000000;
        *digit <<= 1;
        if(carry) *digit |= 0x00000001;
        carry = next_carry;
    }

    CsrCryptoBnTrim(a);
}

void CsrCryptoBnShiftRight1(CsrBignum* a)
{
    CsrUint32 i;
    CsrUint32 carry = 0, next_carry = 0;
    CsrUint32 *digit;

    for(i = 0, digit = &a->digit[a->length -1]; i < a->length; i++, digit--)
    {
        next_carry = *digit & 0x00000001;
        *digit >>= 1;
        if(carry) *digit |= 0x80000000;
        carry = next_carry;
    }

    CsrCryptoBnTrim(a);
}

/* This function can only be used with a > b */
void CsrCryptoBnUnsignedSubtract(CsrBignum* result, const CsrBignum* a, const CsrBignum* b)
{
    CsrUint32 i;
    CsrUint32 borrow;

    borrow = 0;
    /* Get the length from bnA as this must be the larger operand */
    result->length = a->length;
    for(i = 0; i < a->length; i++)
    {
        /* If digits are equal and no borrow - result is zero */
        if((a->digit[i] == b->digit[i]) && !borrow)
        {
            result->digit[i] = 0;
        }
        else
        /* If A > B, always possible to subtract to non zero value and adjust for any borrow */
        if(a->digit[i] > b->digit[i])
        {
            result->digit[i] = a->digit[i] - b->digit[i];
            if(borrow)
            {
                result->digit[i]--;
                borrow = 0;
            }
        }
        else
        /* If A < B or A==B with borrow, need to borrow again to subtract */
        {
            result->digit[i] = a->digit[i] + (0xffffffff - b->digit[i]);
            if(!borrow)
            {
                result->digit[i]++;
                borrow = 1;
            }
        }
    }

    CsrCryptoBnTrim(result);
}

void CsrCryptoBnUnsignedAddMod(CsrBignum* result, const CsrBignum* a, const CsrBignum* b, const CsrBignum* n)
{
    CsrCryptoBnUnsignedAdd(result, a, b);
    if(CsrCryptoBnUnsignedCompare(result, n) > 0)
    {
        CsrCryptoBnUnsignedSubtract(result, result, n);
    }
}

void CsrCryptoBnUnsignedSubMod(CsrBignum* result, const CsrBignum* a, const CsrBignum* b, const CsrBignum* n)
{
    CsrCryptoBnSet(result, a);
    if(CsrCryptoBnUnsignedCompare(result, b) < 0)
    {
        CsrCryptoBnUnsignedAdd(result, result, n);
    }
    CsrCryptoBnUnsignedSubtract(result, result, b);
}

CsrInt32 CsrCryptoBnUnsignedCompareWord(const CsrBignum* a, const CsrUint32 w)
{
    CsrInt32 result = 1;
    CsrUint32 i;
    CsrUint32 length;
    const CsrUint32* digit;

    /* Work from the most significant digit downwards */
    digit = &a->digit[a->length - 1];
    length = a->length;

    for(i = 1; i < length; i++)
    {
        if(*digit != 0) break;
        digit--;
    }

    if(i >= length)
    {
        if(*digit == w)
        {
            /* Equality */
            result = 0;
        }
        else
        if(*digit < w)
        {
            /* a < w */
            result = -1;
        }
        else
        {
            /* a > w */
            result = 1;
        }
    }

    return result;
}

CsrBool CsrCryptoBnIsEven(const CsrBignum* a)
{
    CsrBool result = TRUE;

    if(a->length > 0)
    {
        result = (a->digit[0] & 1)? FALSE : TRUE;
    }

    return result;
}

void CsrCryptoBnAdd(CsrBignum* result, const CsrBignum* a, const CsrBignum* b)
{
    if(a->negative == b->negative)
    {
        result->negative = a->negative;
        CsrCryptoBnUnsignedAdd(result, a, b);
    }
    else
    if(CsrCryptoBnUnsignedCompare(a, b) < 0)
    {
        result->negative = b->negative;
        CsrCryptoBnUnsignedSubtract(result, b, a);
    }
    else
    {
        result->negative = a->negative;
        CsrCryptoBnUnsignedSubtract(result, a, b);
    }
}

void CsrCryptoBnSubtract(CsrBignum* result, const CsrBignum* a, const CsrBignum* b)
{
    if(a->negative != b->negative)
    {
        result->negative = a->negative;
        CsrCryptoBnUnsignedAdd(result, a, b);
    }
    else
    if(CsrCryptoBnUnsignedCompare(a, b) >= 0)
    {
        result->negative = a->negative;
        CsrCryptoBnUnsignedSubtract(result, a, b);
    }
    else
    {
        result->negative = (a->negative)? FALSE : TRUE;
        CsrCryptoBnUnsignedSubtract(result, b, a);
    }
}
