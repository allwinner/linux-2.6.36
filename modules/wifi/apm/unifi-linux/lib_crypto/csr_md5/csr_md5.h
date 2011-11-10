/** @file csr_md5.h
 *
 * Definitions of MD5 functions
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
 *   Definitions of MD5 functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_md5/csr_md5.h#2 $
 *
 ****************************************************************************/

#ifndef CSR_MD5_H
#define CSR_MD5_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

#define CSR_CRYPTO_MD5_DIGEST_LENGTH 16

typedef struct CSR_CRYPTO_MD5_CTX CSR_CRYPTO_MD5_CTX;

/**
 * @brief Init function of MD5 (RFC 1321)
 *
 * @par Description
 *   Initialize MD5 context
 *
 * @return
 *   CSR_CRYPTO_MD5_CTX *                 ; Pointer to allocated context
 */
extern CSR_CRYPTO_MD5_CTX *CsrCryptoMd5Init(void);

/**
 * @brief Clone function for MD5 context
 *
 * @par Description
 *   Allocate new context and clone from another MD5 context
 *
 * @param[in]   md5_ctx                   : Context
 *
 * @return
 *   CSR_CRYPTO_MD5_CTX *                 ; Pointer to allocated cloned context
 */
extern CSR_CRYPTO_MD5_CTX *CsrCryptoCloneMd5Context(const CSR_CRYPTO_MD5_CTX *md5_ctx);

/**
 * @brief Update function of MD5
 *
 * @par Description
 *   Update MD5
 *
 * @param[inout]   md5_ctx                 : Context
 * @param[in]      plain                   : Plain data
 * @param[in]      length                  : Length of data
 *
 * @return
 *   void
 */
extern void CsrCryptoMd5Update(CSR_CRYPTO_MD5_CTX *md5_ctx, const void *plain, CsrUint32 length);

/**
 * @brief Final function of MD5
 *
 * @par Description
 *   Final function of MD5
 *
 * @param[inout]   digest                 : Output MD5 Digest
 * @param[inout]   md5_ctx                : Context
 *
 * @return
 *   void
 */
extern void CsrCryptoMd5Final(CsrUint8 *digest, CSR_CRYPTO_MD5_CTX *md5_ctx);

/**
 * @brief MD5 (RFC 1321)
 *
 * @par Description
 *   MD5 message digest (RFC 1321)
 *
 * @param[inout]   plain                    : Plain Data
 * @param[inout]   plainLen                 : Data Length
 * @param[inout]   digest                   : Output MD5 Digest
 *
 * @return
 *   void
 */
extern void CsrCryptoMd5( CsrUint8 *plain, CsrUint32 plainLen, CsrUint8 *digest );

#ifdef __cplusplus
}
#endif

#endif /*CSR_MD5_H*/

