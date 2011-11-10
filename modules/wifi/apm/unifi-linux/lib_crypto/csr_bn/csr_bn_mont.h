/** @file csr_bn_mont.h
 *
 * CSR Crypto support function for BigNum Montgomery multiplication.
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
 *   BigNum Montgomery multiplication implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_mont.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_BN_MONT_H
#define CSR_BN_MONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_bn_base.h"

extern CsrBignum *CsrCryptoBnGetMontgomeryR(const CsrBignum* n);
extern CsrBignum *CsrCryptoBnBinarySquareMod(const CsrBignum* r, const CsrBignum* n);
extern CsrUint32 CsrBnGetMontgomeryNp0(const CsrBignum* n);
extern void CsrCryptoBnMonPro(CsrBignum* result, const CsrBignum* a, const CsrBignum* b, const CsrBignum* n, const CsrUint32 np0);
extern void CsrCryptoBnMultiplyMod(CsrBignum *result, CsrBignum *a, CsrBignum *b, CsrBignum *p);

#ifdef __cplusplus
}
#endif

#endif /*CSR_BN_MONT_H*/

