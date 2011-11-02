/** @file csr_sha1.h
 *
 * CSR Crypto support function for Secure Hash Algorithm calculation.
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
 *   Secure Hash Algorithm implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_sha1/csr_sha1.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_SHA1_H
#define CSR_SHA1_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

#define CSR_SHA1_DIGEST_LENGTH 20

typedef struct CSR_CRYPTO_SHA1_CTX CSR_CRYPTO_SHA1_CTX;

/**
 * @brief Init function of SHA1
 *
 * @par Description
 *   Initialize SHA1 context
 *
 * @return
 *   CSR_CRYPTO_SHA1_CTX *                 ; Pointer to allocated context
 */extern CSR_CRYPTO_SHA1_CTX *CsrCryptoSha1Init(void);

 /**
  * @brief Clone function for SHA1 context
  *
  * @par Description
  *   Allocate new context and clone from another SHA1 context
  *
  * @param[in]   sha1_ctx                  : Context
  *
  * @return
  *   CSR_CRYPTO_SHA1_CTX *                ; Pointer to allocated cloned context
  */
 extern CSR_CRYPTO_SHA1_CTX *CsrCryptoCloneSha1Context(const CSR_CRYPTO_SHA1_CTX *sha1_ctx);

/**
 * @brief Update function of SHA1
 *
 * @par Description
 *   Update SHA1
 *
 * @param[inout]   sha1_ctx                : Context
 * @param[in]      plain                   : Plain data
 * @param[in]      length                  : Length of data
 *
 * @return
 *   void
 */
extern void CsrCryptoSha1Update(CSR_CRYPTO_SHA1_CTX *sha1_ctx, const void *plain, CsrUint32 length);

/**
 * @brief Final function of SHA1
 *
 * @par Description
 *   Final function of SHA1
 *
 * @param[inout]   digest                  : Output SHA1 Digest
 * @param[inout]   sha1_ctx                : Context
 *
 * @return
 *   void
 */
extern void CsrCryptoSha1Final(CsrUint8 *digest, CSR_CRYPTO_SHA1_CTX *sha1_ctx);
/* Another version which pads with zero */
extern void CsrCryptoSha1Final_zeroPadding(CsrUint8 *digest, CSR_CRYPTO_SHA1_CTX *sha1_ctx);

/**
 * @brief SHA1
 *
 * @par Description
 *   SHA1 message digest
 *
 * @param[inout]   plain                    : Plain Data
 * @param[inout]   plainLen                 : Data Length
 * @param[inout]   digest                   : Output SHA1 Digest
 *
 * @return
 *   void
 */
void CsrCryptoSha1(CsrUint8 *plain, CsrUint16 plainLen, CsrUint8 *digest);
/* another version using zero padding */
extern void CsrCryptoSha1ZeroPadding(const CsrUint8 *plain, const CsrUint16 plainLen, CsrUint8 *digest);

#ifdef __cplusplus
}
#endif

#endif /* CSR_SHA1_H */

