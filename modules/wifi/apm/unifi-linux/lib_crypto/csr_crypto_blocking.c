/** @file csr_crypto_blocking.c
 *
 * Implementation of the blocking crypto library
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
 *   This provides an implementation of blocking crypto calls
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_crypto_blocking.c#2 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_crypto.h"
#ifdef CSR_CRYPTO_HMAC_ENABLE
#include "csr_hmac/csr_hmac.h"
#endif
#ifdef CSR_CRYPTO_AES_ENABLE
#include "csr_aes/csr_aes128_wrap.h"
#include "csr_aes/csr_aes128_cbc.h"
#endif
#ifdef CSR_CRYPTO_RC4_ENABLE
#include "csr_rc4/csr_rc4.h"
#endif
#if defined (CSR_CRYPTO_HMAC_ENABLE) && defined (CSR_CRYPTO_SHA256_ENABLE) && defined (CSR_CRYPTO_KDHMACSHA256_ENABLE)
#include "csr_kd_hmac_sha256/csr_kd_hmac_sha256.h"
#endif
#ifdef CSR_CRYPTO_SMS4_ENABLE
#include "csr_sms4/csr_sms4.h"
#endif
#ifdef CSR_CRYPTO_DH_ENABLE
#include "csr_dh/csr_dh.h"
#endif
#ifdef CSR_CRYPTO_MSCHAP_ENABLE
#include "csr_mschap/csr_mschap.h"
#endif
#ifdef CSR_CRYPTO_MD4_ENABLE
#include "csr_md4/csr_md4.h"
#endif
#ifdef CSR_CRYPTO_BN_ENABLE
#include "csr_bn/csr_bn.h"
#endif

/* MACROS *******************************************************************/
/* GLOBAL VARIABLE DEFINITIONS **********************************************/
/* PRIVATE TYPES DEFINITIONS ************************************************/
/* PRIVATE CONSTANT DEFINITIONS *********************************************/
/* PRIVATE VARIABLE DEFINITIONS *********************************************/
/* PRIVATE FUNCTION PROTOTYPES **********************************************/
/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/******************* CRYPTO WRAPPER FUNCTIONS FOR BLOCKING MODE **************/

#if defined (CSR_CRYPTO_HMAC_ENABLE) && defined (CSR_CRYPTO_MD5_ENABLE)
/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallHmacMd5(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                       CsrUint8 *key, CsrUint32 keyLength, CsrUint8 *plain, CsrUint32 plainLen, CsrUint8 *digest)
{
    CsrCryptoHmacMd5(key, keyLength, plain, plainLen, digest);
    if (callbackContext)
    {
        callbackContext->resultBuffer = digest;
        callbackContext->cryptoCallBack(callbackContext);
    }
}
#endif

#ifdef CSR_CRYPTO_MD5_ENABLE

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallMd5Update(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                            CSR_CRYPTO_MD5_CTX *md5_ctx, const void *plain, CsrUint32 length)
{
    CsrCryptoMd5Update(md5_ctx, plain, length);
    if (callbackContext)
    {
        callbackContext->resultBuffer = NULL;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallMd5Final(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                           CsrUint8 *digest, CSR_CRYPTO_MD5_CTX *md5_ctx)
{
    CsrCryptoMd5Final(digest, md5_ctx);
    if (callbackContext)
    {
        callbackContext->resultBuffer = digest;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallMd5(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                      CsrUint8 *plain, CsrUint32 plainLen, CsrUint8 *digest)
{
    CsrCryptoMd5(plain, plainLen, digest);
    if (callbackContext)
    {
        callbackContext->resultBuffer = digest;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

#endif

#ifdef CSR_CRYPTO_AES_ENABLE
/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallAes128Unwrap(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                            const CsrUint8 *kek, const CsrUint32 n, const CsrUint8 *cipher, CsrUint8 *plain){
    CsrCryptoAes128Unwrap(kek, n, cipher, plain);
    if (callbackContext)
    {
        callbackContext->resultBuffer = plain;
        callbackContext->cryptoCallBack(callbackContext);
    }

}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallAes128Wrap(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                          const CsrUint8* kek, const CsrUint32 length, CsrUint8 *plain, CsrUint8 *cipher)
{
    CsrCryptoAes128Wrap(kek, length, plain, cipher);
    if (callbackContext)
    {
        callbackContext->resultBuffer = cipher;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallAes128CbcEncrypt(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                   CsrUint8 *keyWrapKey, CsrUint8 *iv,
                                   CsrUint8 *input, CsrUint32 inLength,
                                   CsrUint8 *output, CsrUint32 *outLength,
                                   CsrCryptoPadMode padMode)
{
    CsrCryptoAes128CbcEncrypt(keyWrapKey, iv, input, inLength, output, outLength, padMode);
    if (callbackContext)
    {
        callbackContext->resultBuffer = output;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallAes128CbcDecrypt(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                   CsrUint8 *keyWrapKey, CsrUint8 *iv,
                                   CsrUint8 *input, CsrUint32 inLength,
                                   CsrUint8 *output, CsrUint32 *outLength,
                                   CsrCryptoPadMode padMode)
{
    CsrCryptoAes128CbcDecrypt(keyWrapKey, iv, input, inLength, output, outLength, padMode);
    if (callbackContext)
    {
        callbackContext->resultBuffer = output;
        callbackContext->cryptoCallBack(callbackContext);
    }
}
#endif

#if defined (CSR_CRYPTO_HMAC_ENABLE) && defined (CSR_CRYPTO_SHA1_ENABLE)
/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallHmacSha1(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                               const CsrUint8 *key, CsrUint32 keyLength, CsrUint8 *plain, CsrUint32 plainLen, CsrUint8 *digest)
{
    CsrHmacSha1(key, keyLength, plain, plainLen, digest);
    if (callbackContext)
    {
        callbackContext->resultBuffer = digest;
        callbackContext->cryptoCallBack(callbackContext);
    }

}
#endif

#ifdef CSR_CRYPTO_RC4_ENABLE
/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallRc4Discard256(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                             const CsrUint16 keyLength, const CsrUint8 *key, const CsrUint16 len, const CsrUint8 *inBuf, CsrUint8 *outBuf)
{
    CsrRc4Discard256(keyLength, key, len, inBuf, outBuf);
    if (callbackContext)
    {
        callbackContext->resultBuffer = outBuf;
        callbackContext->cryptoCallBack(callbackContext);
    }

}
#endif

#if defined (CSR_CRYPTO_HMAC_ENABLE) && defined (CSR_CRYPTO_SHA256_ENABLE)
/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallHmacSha256(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                          CsrUint8* key, CsrUint32 keyLen, CsrUint8* text, CsrUint32 textLen, CsrUint8* output)
{
    CsrCryptoHmacSha256 (key, keyLen, text, textLen, output);
    if (callbackContext)
    {
        callbackContext->resultBuffer = output;
        callbackContext->cryptoCallBack(callbackContext);
    }

}
#endif

#if defined (CSR_CRYPTO_HMAC_ENABLE) && defined (CSR_CRYPTO_SHA256_ENABLE) && defined (CSR_CRYPTO_KDHMACSHA256_ENABLE)
/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallKdHmacSha256(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                               CsrUint8* text, CsrUint32 textLen, CsrUint8* key, CsrUint32 keyLen,
                               CsrUint8* output, CsrUint32 length)
{
    CsrCryptoKdHmacSha256(text, textLen, key, keyLen, output, length);
    if (callbackContext)
    {
        callbackContext->resultBuffer = output;
        callbackContext->cryptoCallBack(callbackContext);
    }

}
#endif

#ifdef CSR_CRYPTO_SHA256_ENABLE
/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallSha256(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                     const void *pPlain, CsrUint32 length, unsigned char *pDigest)
{
    CsrCryptoSha256(pPlain, length, pDigest);
    if (callbackContext)
    {
        callbackContext->resultBuffer = pDigest;
        callbackContext->cryptoCallBack(callbackContext);
    }

}
#endif

#ifdef CSR_CRYPTO_SMS4_ENABLE
/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallSms4(void* cryptoContext, CsrCryptoCallbackContext* callbackContext,
                   CsrUint8 *iv, CsrUint8 *Key, CsrUint8 *input, CsrUint16 length, CsrUint8 *output)
{
    CsrCryptoSms4(iv, Key, input, length, output);
    if (callbackContext)
    {
        callbackContext->resultBuffer = output;
        callbackContext->cryptoCallBack(callbackContext);
    }
}
#endif

#ifdef CSR_CRYPTO_EC_ENABLE
/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallEcdhCalculateSharedKey(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                         void *group, CsrUint8 *privateKey, CsrUint8 *othersPublicKey,
                                         CsrUint16 privateKeyLength, CsrUint16 publicKeyLength,
                                         CsrCryptoEcdhCalculateSharedKeyResult *output)
{
    CsrCryptoEcdhCalculateSharedKey(group, privateKey, othersPublicKey, output->sharedKey,
                                    privateKeyLength, publicKeyLength, &output->result);
    if (callbackContext)
    {
        callbackContext->resultBuffer = (CsrUint8*)output;
        callbackContext->cryptoCallBack(callbackContext);
    }

}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallEcGenerateNewKey(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                   void *group, CsrCryptoEcGenerateNewKeyResult *output)
{
    CsrCryptoEcGenerateNewKey(group, output->privateKey, output->publicKey, &output->result);
    if (callbackContext)
    {
        callbackContext->resultBuffer = (CsrUint8*)output;
        callbackContext->cryptoCallBack(callbackContext);
    }

}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallEcCreateGroup(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                const char *pString, const char *aString, const char *bString,
                                const char *xString, const char *yString, const char *orderString,
                                CsrUint8 *output)
{
    CsrCryptoEcCreateGroup(pString, aString, bString, xString, yString, orderString, (void**)output);
    if (callbackContext)
    {
        callbackContext->resultBuffer = output;
        callbackContext->cryptoCallBack(callbackContext);
    }

}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallEcdsaSign(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                            void *group, CsrUint8 *digest, CsrUint16 digestLength, CsrUint8 *privateKey,
                            CsrCryptoEcdsaSignResult *output)
{
    CsrCryptoEcdsaSign(group, digest, digestLength, output->signature, privateKey, &output->result);
    if (callbackContext)
    {
        callbackContext->resultBuffer = (CsrUint8*)output;
        callbackContext->cryptoCallBack(callbackContext);
    }

}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallEcdsaVerify(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                              void *group, CsrUint8 *digest, CsrUint16 digestLength, CsrUint8 *signature,
                              CsrUint16 sigLength, CsrUint8 *othersPublicKey, CsrUint16 keyLen, CsrInt32 *output)
{
    CsrCryptoEcdsaVerify(group, digest, digestLength, signature, sigLength, othersPublicKey, keyLen, output);
    if (callbackContext)
    {
        callbackContext->resultBuffer = (CsrUint8*)output;
        callbackContext->cryptoCallBack(callbackContext);
    }

}
#endif

#ifdef CSR_CRYPTO_DH_ENABLE

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
 void CsrCryptoCallDhGenerateKeys(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                 CsrUint32 g_base, CsrUint8 *prime, CsrUint32 size, CsrUint8 *private_key,
                                 CsrUint8 *public_key, CsrBool do_private)
{
    (void)CsrCryptoDhGenerateKeys(g_base, prime, size, private_key, public_key, do_private);
    if (callbackContext)
    {
        callbackContext->resultBuffer = public_key;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

 /*
  * Description:
  * See description in csr_crypto.h
  */
 /*---------------------------------------------------------------------------*/
void CsrCryptoCallDhCalculateSharedSecret(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                          CsrUint8 *result, CsrUint8 *others_pub, CsrUint8 *out_priv,
                                          CsrUint8 *prime, CsrUint32 size)
{
    CsrCryptoDhCalculateSharedSecret(result, others_pub, out_priv, prime, size);
    if (callbackContext)
    {
        callbackContext->resultBuffer = result;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

#endif

#ifdef CSR_CRYPTO_MSCHAP_ENABLE

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallMschapGenerateNTResponseAndSessionKey(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                                        CsrUint8 *auth_challenge,
                                                        CsrUint8 *peer_challenge,
                                                        CsrUint8 *username,
                                                        CsrUint8 *password,
                                                        CsrUint8 *response,
                                                        CsrUint8 *session_key)
{
    CsrCryptoMschapGenerateNTResponseAndSessionKey(auth_challenge, peer_challenge,
                                                   username, password, response, session_key);
    if (callbackContext)
    {
        callbackContext->resultBuffer = session_key;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallMschapNtPasswordHash(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                       CsrUint8 *password, CsrUint8 *hash)
{
    CsrCryptoMschapNtPasswordHash(password, hash);
    if (callbackContext)
    {
        callbackContext->resultBuffer = hash;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallMschapChallengeResponse(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                                      CsrUint8 *challenge, CsrUint8 *hash, CsrUint8 *response)
{
    CsrCryptoMschapChallengeResponse(challenge, hash, response);
    if (callbackContext)
    {
        callbackContext->resultBuffer = response;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

#endif

#ifdef CSR_CRYPTO_MD4_ENABLE

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallMd4(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                      const CsrUint8 *const in, const CsrUint32 in_len, CsrUint8 *out)
{
    CsrCryptoMd4(in, in_len, out);
    if (callbackContext)
    {
        callbackContext->resultBuffer = out;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

#endif

#ifdef CSR_CRYPTO_BN_ENABLE

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallModExp(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                         const CsrUint8 *pBase, CsrUint16 baseLength,
                         const CsrUint8 *pExponent, CsrUint16 exponentLength,
                         const CsrUint8 *pPrime, CsrUint16 primeLength, CsrUint8 *pResult, CsrUint32 *resultLength)
{
    CsrCryptoModExp(pBase, baseLength, pExponent, exponentLength, pPrime, primeLength, pResult, resultLength);
    if (callbackContext)
    {
        callbackContext->resultBuffer = pResult;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

#endif

#ifdef CSR_CRYPTO_SHA1_ENABLE

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallSha1Update(void *cryptoContext, CsrCryptoCallbackContext *callbackContext,
                             CSR_CRYPTO_SHA1_CTX *sha1_ctx, const void *plain, CsrUint32 length)
{
    CsrCryptoSha1Update(sha1_ctx, plain, length);
    if (callbackContext)
    {
        callbackContext->resultBuffer = NULL;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallSha1Final(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                            CsrUint8 *digest, CSR_CRYPTO_SHA1_CTX *sha1_ctx)
{
    CsrCryptoSha1Final(digest, sha1_ctx);
    if (callbackContext)
    {
        callbackContext->resultBuffer = digest;
        callbackContext->cryptoCallBack(callbackContext);
    }
}

/*
 * Description:
 * See description in csr_crypto.h
 */
/*---------------------------------------------------------------------------*/
void CsrCryptoCallSha1(void* cryptoContext, CsrCryptoCallbackContext *callbackContext,
                       CsrUint8 *plain, CsrUint16 plainLen, CsrUint8 *digest)
{
    CsrCryptoSha1(plain, plainLen, digest);
    if (callbackContext)
    {
        callbackContext->resultBuffer = digest;
        callbackContext->cryptoCallBack(callbackContext);
    }
}
#endif

