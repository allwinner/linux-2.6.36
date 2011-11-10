/** @file csr_aes128_cbc.h
 *
 * Definitions for AES-128 CBC functions.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of AES-128 CBC functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_aes/csr_aes128_cbc.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_AES128_CBC_H
#define CSR_AES128_CBC_H

#include "csr_types.h"
#include "csr_crypto_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrCryptoAes128CbcDecrypt(CsrUint8 *keyWrapKey, CsrUint8 *iv,
                               CsrUint8 *input, CsrUint32 inLength,
                               CsrUint8 *output, CsrUint32 *outLength,
                               CsrCryptoPadMode padMode);

void CsrCryptoAes128CbcEncrypt(CsrUint8 *keyWrapKey, CsrUint8 *iv,
                               CsrUint8 *input, CsrUint32 inLength,
                               CsrUint8 *output, CsrUint32 *outLength,
                               CsrCryptoPadMode padMode);

#ifdef __cplusplus
}
#endif

#endif /* CSR_AES128_CBC_H */
