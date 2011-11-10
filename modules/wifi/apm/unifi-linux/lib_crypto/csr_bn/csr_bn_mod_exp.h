/** @file csr_bn_mod_exp.h
 *
 * CSR Crypto support function for BigNum Modular Exponentiation.
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
 *   BigNum Modular Exponentiation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_mod_exp.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_BN_MOD_EXP_H
#define CSR_BN_MOD_EXP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"
#include "csr_bn_public.h"

CsrUint32 CsrCryptoBnModExp(CsrBignum* result, const CsrBignum* b, const CsrBignum* e, const CsrBignum* n, CsrBignumCtx* ctx);

void CsrCryptoModExp(const CsrUint8 *base, const CsrUint32 baseLength,
                     const CsrUint8 *exponent, const CsrUint32 exponentLength,
                     const CsrUint8 *prime, const CsrUint32 primeLength, CsrUint8 *result, CsrUint32 *resultLength);

#ifdef __cplusplus
}
#endif

#endif /*CSR_BN_MOD_EXP_H*/

