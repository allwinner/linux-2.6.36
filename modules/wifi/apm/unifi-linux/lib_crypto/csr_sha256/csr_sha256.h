/** @file csr_sha256.h
 *
 * Definitions for SHA256 functions.
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
 *   This provides SHA256 functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_sha256/csr_sha256.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_CRYPTO_SHA256_H
#define CSR_CRYPTO_SHA256_H

#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CSR_CRYPTO_SHA256_DIGEST_LENGTH 32

typedef struct CSR_CRYPTO_SHA256_CTX
{
    CsrUint32 H[8];
    unsigned char plain[64];
    CsrUint32 remainder;
    CsrUint32 length;
}CSR_CRYPTO_SHA256_CTX;

/**
 * @brief Init function of SHA256
 *
 * @par Description
 *   Initialize SHA256 context
 *
 * @param[inout]   ctx                    : Context
 *
 * @return
 *   void
 */
extern void CsrCryptoSha256Init( CSR_CRYPTO_SHA256_CTX *ctx );

/**
 * @brief Update function of SHA256
 *
 * @par Description
 *   Update SHA256
 *
 * @param[inout]   ctx                    : Context
 * @param[in]      plain                  : Plain data
 * @param[in]      length                  : Length of data
 *
 * @return
 *   void
 */
extern void CsrCryptoSha256Update( CSR_CRYPTO_SHA256_CTX *ctx, const void *plain, CsrUint32 length );

/**
 * @brief Final function of SHA256
 *
 * @par Description
 *   Final function of SHA256
 *
 * @param[inout]   digest                 : Output SHA256 Digest
 * @param[inout]   ctx                    : Context
 *
 * @return
 *   void
 */
extern void CsrCryptoSha256Final( unsigned char *digest, CSR_CRYPTO_SHA256_CTX *ctx );

/**
 * @brief SHA256
 *
 * @par Description
 *   SHA256 hash
 *
 * @param[in]      plain                  : Message
 * @param[in]      length                  : length of message
 * @param[inout]   digest                 : Output SHA256 Digest
 *
 * @return
 *   void
 */
extern void CsrCryptoSha256( const void *plain, CsrUint32 length, unsigned char *digest );

#ifdef __cplusplus
}
#endif

#endif /* CSR_SHA256_H */
