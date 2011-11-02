/** @file csr_bn_inverse.h
 *
 * CSR Crypto support function for BigNum modular inverse.
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
 *   BigNum modular inverse.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_inverse.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_BN_INVERSE_H
#define CSR_BN_INVERSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"
#include "csr_bn_public.h"

CsrBool CsrCryptoBnInverse(CsrBignum* result, const CsrBignum* x, const CsrBignum* n);

#ifdef __cplusplus
}
#endif

#endif /*CSR_BN_INVERSE_H*/

