/** @file car_aes128_wrap.h
 *
 * Definitions for AES-128 keywrap functions.
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
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_aes/csr_aes128_wrap.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_CRYPTO_AES128_WRAP_H
#define CSR_CRYPTO_AES128_WRAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

/**
 * @brief AES Key Wrap (RFC 3394)
 *
 * @par Description
 *      AES Key Wrap (RFC 3394)
 *
 * @param[in]  key      : Key
 * @param[in]  n        : Length of Plain data / 8
 * @param[in]  plain    : Plaintext data
 * @param[inout] cipher : Ciphertext data
 *
 * @return
 *   void
 */
extern void CsrCryptoAes128Wrap(const CsrUint8 *key,
                                const CsrUint32 n,
                                const CsrUint8 *plain,
                                CsrUint8 *cipher);

/**
 * @brief AES Key Wrap (RFC 3394)
 *
 * @par Description
 *      AES Key Wrap (RFC 3394)
 *
 * @param[in]  key      : Key
 * @param[in]  n        : Length of ciphered data / 8
 * @param[in]  cipher   : Ciphertext data
 * @param[inout] plain  : plain data
 *
 * @return
 *   void
 */

extern void CsrCryptoAes128Unwrap(const CsrUint8 *key,
                                  const CsrUint32 n,
                                  const CsrUint8 *cipher,
                                  CsrUint8 *plain);

#ifdef __cplusplus
}
#endif

#endif /* CSR_CRYPTO_AES128_WRAP_H */
