/** @file csr_crypto.h
 *
 * Definitions for crypto library. Both blocking and nonblocking implementation should
 * provide the function implementations
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
 *   Header file for crypto functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_crypto.h#1 $
 *
 ****************************************************************************/

#ifndef CSR_CRYPTO_H
#define CSR_CRYPTO_H


#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"
#include "csr_crypto_common.h"

#ifdef CSR_CRYPTO_EC_ENABLE
#include "csr_ec/csr_ec.h"
#endif

/* MACROS *******************************************************************/
/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PUBLIC TYPE DEFINITIONS ************************************************/

typedef struct CsrCryptoContext CsrCryptoContext;
typedef struct CsrCryptoCallbackContext CsrCryptoCallbackContext;
typedef void (*CryptoCallBack)(CsrCryptoCallbackContext* callbackContext);

struct CsrCryptoCallbackContext
{
    CryptoCallBack     cryptoCallBack;
    CsrUint8           *resultBuffer;
    void            *context;
};



/* PRIVATE CONSTANT DEFINITIONS *********************************************/
/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES **********************************************/

#if defined (CSR_CRYPTO_HMAC_ENABLE) && defined (CSR_CRYPTO_MD5_ENABLE)
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallHmacMd5(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                              CsrUint8 *key, CsrUint32 keyLength, CsrUint8 *plain, CsrUint32 plainLen, CsrUint8 *digest);
#endif

#ifdef CSR_CRYPTO_MD5_ENABLE
#include "csr_md5/csr_md5.h"

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallMd5Update(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                   CSR_CRYPTO_MD5_CTX *md5_ctx, const void *plain, CsrUint32 length);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallMd5Final(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                  CsrUint8 *digest, CSR_CRYPTO_MD5_CTX *md5_ctx);
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallMd5(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                             CsrUint8 *plain, CsrUint32 plainLen, CsrUint8 *digest);

#endif

#ifdef CSR_CRYPTO_AES_ENABLE
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallAes128Unwrap(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                   const CsrUint8 *kek, const CsrUint32 length, const CsrUint8 *cipher,
                                   CsrUint8 *plain);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallAes128Wrap(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                 const CsrUint8 *kek, const CsrUint32 n, CsrUint8 *plain, CsrUint8 *cipher);


typedef struct  CsrCryptoAes128CbcDecryptResult
{
    CsrUint32   outputLength;
    CsrUint8   *output;
} CsrCryptoAes128CbcDecryptResult;
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallAes128CbcDecrypt(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                          CsrUint8 *keyWrapKey, CsrUint8 *iv,
                                          CsrUint8 *input, CsrUint32 inLength,
                                          CsrUint8 *output, CsrUint32 *outLength,
                                          CsrCryptoPadMode padMode);


typedef struct  CsrCryptoAes128CbcEncryptResult
{
    CsrUint32   outputLength;
    CsrUint8   *output;
} CsrCryptoAes128CbcEncryptResult;
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallAes128CbcEncrypt(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                          CsrUint8 *keyWrapKey, CsrUint8 *iv,
                                          CsrUint8 *input, CsrUint32 inLength,
                                          CsrUint8 *output, CsrUint32 *outLength,
                                          CsrCryptoPadMode padMode);

#endif

#ifdef CSR_CRYPTO_BN_ENABLE

typedef struct  CsrCryptoModExpResult
{
    CsrUint32   outputLength;
    CsrUint8   *output;
} CsrCryptoModExpResult;

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallModExp(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
        const CsrUint8 *base, CsrUint16 baseLength,
        const CsrUint8 *exponent, CsrUint16 exponentLength,
        const CsrUint8 *prime, CsrUint16 primeLength,
        CsrUint8 *result, CsrUint32 *resultLength);
#endif

#if defined (CSR_CRYPTO_HMAC_ENABLE) && defined (CSR_CRYPTO_SHA1_ENABLE)
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallHmacSha1(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                               const CsrUint8 *key, CsrUint32 keyLength, CsrUint8 *plain, CsrUint32 plainLen, CsrUint8 *digest);
#endif

#ifdef CSR_CRYPTO_RC4_ENABLE
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallRc4Discard256(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                                    const CsrUint16 keyLength, const CsrUint8 *key, const CsrUint16 len,
                                    const CsrUint8 *inBuf, CsrUint8 *outBuf);
#endif

#if defined (CSR_CRYPTO_HMAC_ENABLE) && defined (CSR_CRYPTO_SHA256_ENABLE)
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallHmacSha256(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                              CsrUint8* key, CsrUint32 keyLen, CsrUint8* text, CsrUint32 textLen, CsrUint8* output);
#endif

#if defined (CSR_CRYPTO_HMAC_ENABLE) && defined (CSR_CRYPTO_SHA256_ENABLE) && defined (CSR_CRYPTO_KDHMACSHA256_ENABLE)
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallKdHmacSha256(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                               CsrUint8* text, CsrUint32 textLen, CsrUint8* key, CsrUint32 keyLen, CsrUint8* output,
                               CsrUint32 length);
#endif

#ifdef CSR_CRYPTO_SHA256_ENABLE
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallSha256(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                            const void *pPlain, CsrUint32 length, unsigned char *pDigest);
#endif

#ifdef CSR_CRYPTO_SMS4_ENABLE
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallSms4(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                          CsrUint8 *iv, CsrUint8 *Key, CsrUint8 *input, CsrUint16 length, CsrUint8 *output);
#endif


#ifdef CSR_CRYPTO_EC_ENABLE

typedef struct  CsrCryptoEcdhCalculateSharedKeyResult
{
    CsrUint8 sharedKey[CSR_CRYPTO_EC_SYMMETRIC_KEY_LENGTH];
    CsrInt32   result;
} CsrCryptoEcdhCalculateSharedKeyResult;

typedef struct  CsrCryptoEcGenerateNewKeyResult
{
    CsrUint8 privateKey[CSR_CRYPTO_EC_PRIVATE_KEY_LENGTH];
    CsrUint8 publicKey[CSR_CRYPTO_EC_PUBLIC_KEY_LENGTH];
    CsrInt32   result;
} CsrCryptoEcGenerateNewKeyResult;

typedef struct CsrCryptoEcdsaSignResult
{
    CsrUint8 signature[CSR_CRYPTO_ECDSA_SIGNATURE_LENGTH];
    CsrInt32   result;
} CsrCryptoEcdsaSignResult;

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallEcdhCalculateSharedKey(void *cryptoContext,
                                                CsrCryptoCallbackContext *callbackContext,
                                                void *group, CsrUint8 *privateKey, CsrUint8 *othersPublicKey,
                                                CsrUint16 privateKeyLength, CsrUint16 publicKeyLength,
                                                CsrCryptoEcdhCalculateSharedKeyResult *output);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallEcGenerateNewKey(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                          void *group, CsrCryptoEcGenerateNewKeyResult *output);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallEcCreateGroup(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                       const char *pString, const char *aString, const char *bString,
                                       const char *xString, const char *yString, const char *orderString,
                                       CsrUint8 *output);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallEcdsaSign(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                   void *group, CsrUint8 *digest, CsrUint16 digestLength, CsrUint8 *privateKey,
                                   CsrCryptoEcdsaSignResult *output);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallEcdsaVerify(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                 void *group, CsrUint8 *digest, CsrUint16 digestLength, CsrUint8 *signature,
                                 CsrUint16 sigLength, CsrUint8 *publicKey, CsrUint16 keyLen, CsrInt32 *result);

#endif

#ifdef CSR_CRYPTO_DH_ENABLE

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallDhGenerateKeys(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                        CsrUint32 g_base, CsrUint8 *prime, CsrUint32 size, CsrUint8 *private_key,
                                        CsrUint8 *public_key, CsrBool do_private);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallDhCalculateSharedSecret(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                                 CsrUint8 *result, CsrUint8 *others_pub, CsrUint8 *out_priv,
                                                 CsrUint8 *prime, CsrUint32 size);
#endif

#ifdef CSR_CRYPTO_MSCHAP_ENABLE

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallMschapGenerateNTResponseAndSessionKey(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                                               CsrUint8 *auth_challenge,
                                                        CsrUint8 *peer_challenge,
                                                        CsrUint8 *username,
                                                        CsrUint8 *password,
                                                        CsrUint8 *response,
                                                        CsrUint8 *session_key);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallMschapNtPasswordHash(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                              CsrUint8 *password, CsrUint8 *hash);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallMschapChallengeResponse(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                             CsrUint8 *challenge, CsrUint8 *hash, CsrUint8 *response);

#endif

#ifdef CSR_CRYPTO_MD4_ENABLE

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
extern void CsrCryptoCallMd4(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                             const CsrUint8 *const in, const CsrUint32 in_len, CsrUint8 *out);

#endif

#ifdef CSR_CRYPTO_SHA1_ENABLE
#include "csr_sha1/csr_sha1.h"
/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
void CsrCryptoCallSha1Update(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                             CSR_CRYPTO_SHA1_CTX *sha1_ctx, const void *plain, CsrUint32 length);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
void CsrCryptoCallSha1Final(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                            CsrUint8 *digest, CSR_CRYPTO_SHA1_CTX *sha1_ctx);

/**
 * @brief
 *   Wrapper function to crypto API
 *
 * @par Description
 *
 *
 * @param
 *  The first parameter is pointer to the crypto context structure, second is the call back context
 *  that will be used when crypto processing is finished
 *  Other parameters are similar to that of the crypto API that is called
 *
 * @return
 *   void
 */
void CsrCryptoCallSha1(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                       CsrUint8 *plain, CsrUint16 plainLen, CsrUint8 *digest);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* CSR_CRYPTO_H */
