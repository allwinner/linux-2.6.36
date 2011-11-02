/** @file csr_hmac.h
 *
 * CSR Crypto support function for hash message authentication code calculation
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
 *   hash message authentication code implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_hmac/csr_hmac.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_HMAC_H
#define CSR_HMAC_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup security_plugin
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"
#include "csr_sha1/csr_sha1.h"
#include "csr_md5/csr_md5.h"
#include "csr_sha256/csr_sha256.h"

#ifdef CSR_CRYPTO_HMAC_ENABLE

#ifdef CSR_CRYPTO_SHA1_ENABLE
typedef struct CSR_HMAC_SHA1_CTX
{
    CSR_CRYPTO_SHA1_CTX *sha1_ctx;
    CsrUint8 opad[64];
}CSR_HMAC_SHA1_CTX;

/**
 * @brief HMAC_SHA1 Keyed-Hashing for Message Authentication - Init function
 *
 * @par Description
 *   Init for HMAC hash digest based on SHA1
 *
 * @param[inout]  ctx                     : Context
 * @param[in]     key                     : Key
 * @param[in]     keyLen                  : Length of key
 *
 * @return
 *   void
 */
extern void CsrHmacSha1Init(CSR_HMAC_SHA1_CTX *ctx,
                               const CsrUint8 *key,
                               CsrUint32 length);

/**
 * @brief HMAC_SHA1 Keyed-Hashing for Message Authentication - Update function
 *
 * @par Description
 *   Update for HMAC hash digest based on SHA1
 *
 * @param[inout]  ctx                     : Context
 * @param[in]     plain                   : Data
 * @param[in]     length                  : Length of data
 *
 * @return
 *   void
 */
extern void CsrHmacSha1Update(CSR_HMAC_SHA1_CTX *ctx,
                                 const void *plain,
                                 CsrUint32 length);

/**
 * @brief HMAC_SHA1 Keyed-Hashing for Message Authentication - Final function
 *
 * @par Description
 *   Final for HMAC hash digest based on SHA1
 *
 * @param[inout]  digest                 : Computed hash
 * @param[inout]  ctx                    : Context
 *
 * @return
 *   void
 */
extern void CsrHmacSha1Final(CsrUint8 *digest,
                                CSR_HMAC_SHA1_CTX *ctx );

/**
 * @brief HMAC_SHA1 Keyed-Hashing for Message Authentication
 *
 * @par Description
 *   Computes HMAC hash digest based on SHA1.
 *
 * @param[in]     key                     : Key
 * @param[in]     keyLen                  : Length of key
 * @param[in]     plain                   : Data to be hashed
 * @param[in]     plainLength             : Length of data to be hashed
 * @param[inout]  digest                  : Computed hash
 *
 * @return
 *   void
 */
extern void CsrHmacSha1(const CsrUint8 *key,
                          const CsrUint32 keyLen,
                          const CsrUint8 *plain,
                          const CsrUint32 plainLen,
                          CsrUint8 *digest);
#endif

#ifdef CSR_CRYPTO_MD5_ENABLE
typedef struct CSR_CRYPTO_HMAC_MD5_CTX
{
    CSR_CRYPTO_MD5_CTX *md5_ctx;
    CsrUint8 opad[64];
}CSR_CRYPTO_HMAC_MD5_CTX;

/**
 * @brief HMAC_MD5 Keyed-Hashing for Message Authentication - Init function
 *
 * @par Description
 *   Init for HMAC hash digest based on MD5
 *
 * @param[inout]  ctx                     : Context
 * @param[in]     key                     : Key
 * @param[in]     length                  : Length of key
 *
 * @return
 *   void
 */
extern void CsrCryptoHmacMd5Init( CSR_CRYPTO_HMAC_MD5_CTX *ctx, CsrUint8 *key, CsrUint32 length );

/**
 * @brief HMAC_MD5 Keyed-Hashing for Message Authentication - update function
 *
 * @par Description
 *   Update for HMAC hash digest based on MD5
 *
 * @param[inout]  ctx                     : Context
 * @param[in]     plain                   : Data
 * @param[in]     length                  : Length of data
 *
 * @return
 *   void
 */
extern void CsrCryptoHmacMd5Update( CSR_CRYPTO_HMAC_MD5_CTX *ctx, const void *plain, CsrUint32 length );

/**
 * @brief HMAC_MD5 Keyed-Hashing for Message Authentication - Final function
 *
 * @par Description
 *   Final for HMAC hash digest based on MD5
 *
 * @param[inout]  digest                 : Computed hash
 * @param[inout]  ctx                    : Context
 *
 * @return
 *   void
 */
extern void CsrCryptoHmacMd5Final( CsrUint8 *digest, CSR_CRYPTO_HMAC_MD5_CTX *ctx );

/**
 * @brief HMAC_MD5 Keyed-Hashing for Message Authentication
 *
 * @par Description
 *   Computes HMAC hash digest based on MD5
 *
 * @param[in]     key                     : Key
 * @param[in]     keyLen                  : Length of key
 * @param[in]     plain                   : Data to be hashed
 * @param[in]     plainLength             : Length of data to be hashed
 * @param[inout]  digest                  : Computed hash
 *
 * @return
 *   void
 */
extern void CsrCryptoHmacMd5(CsrUint8 *key,
                         CsrUint32 keyLen,
                         CsrUint8 *plain,
                         CsrUint32 plainLen,
                         CsrUint8 *digest);

#endif

#ifdef CSR_CRYPTO_SHA256_ENABLE

typedef struct CSR_CRYPTO_HMAC_SHA256_CTX
{
    CSR_CRYPTO_SHA256_CTX ctx;
    unsigned char opad[64];
}CSR_CRYPTO_HMAC_SHA256_CTX;

/**
 * @brief HMAC_SHA256 - Init function
 *
 * @par Description
 *   Init for HMAC hash digest based on SHA256
 *
 * @param[inout]  ctx                     : Context
 * @param[in]     key                     : Key
 * @param[in]     length                  : Length
 *
 * @return
 *   void
 */
extern void CsrCryptoHmacSha256Init( CSR_CRYPTO_HMAC_SHA256_CTX *ctx, unsigned char *key, CsrUint32 length );

/**
 * @brief HMAC_SHA256 - update function
 *
 * @par Description
 *   Update for HMAC hash digest based on SHA256
 *
 * @param[inout]  ctx                     : Context
 * @param[in]     plain                   : Data
 * @param[in]     length                  : Length of data
 *
 * @return
 *   void
 */
extern void CsrCryptoHmacSha256Update( CSR_CRYPTO_HMAC_SHA256_CTX *ctx, const void *plain, CsrUint32 length );

/**
 * @brief HMAC_SHA256 - Final function
 *
 * @par Description
 *   Final for HMAC hash digest based on SHA256
 *
 * @param[inout]  digest                 : Computed hash
 * @param[inout]  ctx                    : Context
 *
 * @return
 *   void
 */
extern void CsrCryptoHmacSha256Final( unsigned char *digest, CSR_CRYPTO_HMAC_SHA256_CTX *ctx );

/**
 * @brief HMAC_SHA256
 *
 * @par Description
 *   HMAC hash digest based on SHA256
 *
 * @param[in]   key                    : Key
 * @param[in]   keyLen                 : Length of the key
 * @param[in]   plain                  : Plain text
 * @param[in]   plainLen               : Length of plain text
 * @param[inout]digest                 : Computed hash
 *
 * @return
 *   void
 */
extern void CsrCryptoHmacSha256 (CsrUint8 *key, CsrUint32 keyLen, CsrUint8 *plain, CsrUint32 plainLen,
        CsrUint8 *digest);

#endif

#endif /* CSR_CRYPTO_HMAC_ENABLE */

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* CSR_HMAC_H */
