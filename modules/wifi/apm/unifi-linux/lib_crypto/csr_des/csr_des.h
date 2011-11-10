/** @file csr_des.h
 *
 * CSR Crypto support function for Data Encryption Standard calculation.
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
 *   Data Encryption Standard implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_des/csr_des.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_DES_H
#define CSR_DES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"
#include "csr_crypto_common.h"

typedef struct CsrWord64
{
    CsrUint32 hi;
    CsrUint32 lo;
}CsrWord64;

typedef struct CsrKeySchedule
{
    CsrUint32 c;
    CsrUint32 d;
    CsrWord64 subkey[16];
}CsrKeySchedule;

void CsrCryptoDesSetKey(CsrUint8 *key, CsrKeySchedule *ks);

void CsrCryptoDesEcbEncrypt(CsrUint8 *input, CsrKeySchedule *ks, CsrUint8 *output);

void CsrCryptoDesEcbDecrypt(CsrUint8 *input, CsrKeySchedule *ks, CsrUint8 *output);

void CsrCrypto3DesEcbEncrypt(CsrUint8 *input, CsrKeySchedule *ks1, CsrKeySchedule *ks2, CsrKeySchedule *ks3, CsrUint8 *output);

void CsrCrypto3DesEcbDecrypt(CsrUint8 *input, CsrKeySchedule *ks1, CsrKeySchedule *ks2, CsrKeySchedule *ks3, CsrUint8 *output);

/**
 * @brief 3 DES CBC Encrypt
 *
 * @par Description
 *      3 DES CBC Encryption
 *
 * @param[in]  ks1     : Key Schedule 1 obtained by call to CsrCryptoDesSetKey
 * @param[in]  ks2     : Key Schedule 2 obtained by call to CsrCryptoDesSetKey
 * @param[in]  ks3     : Key Schedule 3 obtained by call to CsrCryptoDesSetKey
 * @param[in]  iv      : IV
 * @param[in]  input   : Input data
 * @param[in]  inLength: Input length
 * @param[out] output  : Output data
 * @param[out]  outLength: Output length
 * @param[in]  padMode: Pad mode (PAD_MODE_PKCS7/PAD_MODE_TLS)
 *
 * @return
 *   void
 */
void CsrCrypto3DesCbcEncrypt(CsrKeySchedule *ks1, CsrKeySchedule *ks2, CsrKeySchedule *ks3,
                         CsrUint8 *iv,
                         CsrUint8 *input, CsrUint32 inLength,
                         CsrUint8 *output, CsrUint32 *outLength,
                         CsrCryptoPadMode padMode);

/**
 * @brief 3 DES CBC Decrypt
 *
 * @par Description
 *      3 DES CBC Decryption
 *
 * @param[in]  ks1     : Key Schedule 1 obtained by call to CsrCryptoDesSetKey
 * @param[in]  ks2     : Key Schedule 2 obtained by call to CsrCryptoDesSetKey
 * @param[in]  ks3     : Key Schedule 3 obtained by call to CsrCryptoDesSetKey
 * @param[in]  iv      : IV
 * @param[in]  input   : Input data
 * @param[in]  inLength: Input length
 * @param[out] output  : Output data
 * @param[out]  outLength: Output length
 * @param[in]  padMode: Pad mode (PAD_MODE_PKCS7/PAD_MODE_TLS)
 *
 * @return
 *   void
 */
void CsrCrypto3DesCbcDecrypt(CsrKeySchedule *ks1, CsrKeySchedule *ks2, CsrKeySchedule *ks3,
                         CsrUint8 *iv,
                         CsrUint8 *input, CsrUint32 inLength,
                         CsrUint8 *output, CsrUint32 *outLength,
                         CsrCryptoPadMode padMode);

#ifdef __cplusplus
}
#endif

#endif /*CSR_DES_H*/

