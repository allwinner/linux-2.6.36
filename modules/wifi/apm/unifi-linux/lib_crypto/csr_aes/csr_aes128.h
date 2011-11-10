/** @file csr_aes128.h
 *
 * Definitions for AES-128 functions.
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
 *   This provides an implementation of AES-128 functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_aes/csr_aes128.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_AES128_H
#define CSR_AES128_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

/**
 * @brief AES 128
 *
 * @par Description
 *      AES 128 encryption
 *
 * @param[in]  key     : Key
 * @param[in]  input   : Input data
 * @param[out] output  : Output data
 *
 * @return
 *   void
 */
extern void CsrCryptoAes128Encrypt(const CsrUint8 *key,
                                   const CsrUint8 *input,
                                   CsrUint8       *output);

/**
 * @brief AES 128
 *
 * @par Description
 *      AES 128 decryption
 *
 * @param[in]  key     : Key
 * @param[in]  input   : Input data
 * @param[out] output  : Output data
 *
 * @return
 *   void
 */
void CsrCryptoAes128Decrypt(const CsrUint8 *key,
                            const CsrUint8 *input,
                            CsrUint8       *output);

#ifdef __cplusplus
}
#endif

#endif /* CSR_AES128_H */
