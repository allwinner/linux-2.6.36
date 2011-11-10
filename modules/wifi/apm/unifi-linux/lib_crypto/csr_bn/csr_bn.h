/** @file csr_bn.h
 *
 * CSR Crypto support function for BigNum calculation.
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
 *   BigNum implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_BN_H
#define CSR_BN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

#define CsrCryptoBnNumBytes(a) ((CsrCryptoBnNumBits(a)+7)/8)

#ifdef CSR_CRYPTO_BN_ENABLE

#include "csr_bn_base.h"
#include "csr_bn_mod_exp.h"

#else
#include <openssl/bn.h>
typedef void CsrBignum;
typedef void CsrBignumCtx;
#define CsrCryptoBnCtxNew() ((void*)BN_CTX_new())
#define CsrCryptoBnBnToBin(bigNum, to) (BN_bn2bin((const BIGNUM*)bigNum, to))
#define CsrCryptoBnFree(bigNum) (BN_free((BIGNUM*)bigNum))
#define CsrCryptoBnNumBits(bigNum) (BN_num_bits((const BIGNUM*)bigNum))
#define CsrCryptoBnNumOctets(bigNum) (BN_num_bytes((const BIGNUM*)bigNum))
#define CsrCryptoBnCtxFree(bnContext) (BN_CTX_free((BN_CTX*)bnContext))
#define CsrCryptoBnModExp(bigNumR, bigNumA, bigNumP, bigNumM, ctx) \
            (BN_mod_exp((BIGNUM*)bigNumR, (const BIGNUM*)bigNumA, (const BIGNUM*)bigNumP,(const BIGNUM*)bigNumM, (BN_CTX*)ctx))
#define CsrCryptoBnBinToBn(s, len, ret) (BN_bin2bn(s, len, (BIGNUM*)ret))
#define CsrCryptoBnNew() ((void*)BN_new())
#define CsrCryptoBnSetWord(a, w) (BN_set_word((BIGNUM*) a, (BN_ULONG) w))
#endif

#ifdef __cplusplus
}
#endif

#endif /*CSR_BN_H*/

