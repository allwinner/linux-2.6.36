/** @file csr_mschap.h
 *
 * Definitions for functions to handle MSCHAPv2 extensions.
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
 *   Microsoft PPP CHAP extensions (v2) are defined in RFC 2759. This file
 *   defines the functions that perform the appropriate cryptographic
 *   operations.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_mschap/csr_mschap.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_CRYPTO_MSCHAP_H
#define CSR_CRYPTO_MSCHAP_H

#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate the 24-octet NT response and session key to an MSCHAPv2 challenge.
 *
 * @param[in]   auth_challenge : the 16-octet challenge from the authenticator
 * @param[in]   peer_challenge : our 16-octet challenge
 * @param[in]   username       : the null-terminated unicode username
 * @param[in]   password       : the users raw password
 * @param[out]  response       : to hold the 24-octet result.
 * @param[out]  session_key    : the MSCHAPv2 session key.
 */
void CsrCryptoMschapGenerateNTResponseAndSessionKey(CsrUint8 *auth_challenge,
                                     CsrUint8 *peer_challenge,
                                     CsrUint8 *username,
                                     CsrUint8 *password,
                                     CsrUint8 *response,
                                     CsrUint8 *session_key);

/**
 * @brief Generate the MS-CHAPv2 Challenge response as per RFC 2759.
 *
 * @param[in]   challenge :  the 8-octet challenge
 * @param[in]   hash      :  the 16-octect password hash
 * @param[out]  response  :  the 24-octect MSCHAPv2 response
 */
void CsrCryptoMschapChallengeResponse(CsrUint8 *challenge, CsrUint8 *hash, CsrUint8 *response);

/**
 * @brief Generate the NT password hash as per RFC 2759.
 *
 * @param[in]   password : the null-terminated unicode username
 * @param[out]  hash     : to hold the 16-octet result.
 */
void CsrCryptoMschapNtPasswordHash(CsrUint8 *password, CsrUint8 *hash);

/**
 * @brief Generate Authenticator response as per RFC 2759.
 *
 * @param[in]   auth_challenge : the 16-octet challenge from the authenticator
 * @param[in]   peer_challenge : our 16-octet challenge
 * @param[in]   username       : the null-terminated unicode username
 * @param[in]   password       : the users raw password
 * @param[out]  nt_response    : NT response sent to the server
 * @param[out]  auth_response  : to hold 42 octet Auth response
 */
void GenerateAuthenticatorResponse(CsrUint8 *auth_challenge,
                                   CsrUint8 *peer_challenge,
                                   CsrUint8 *username,
                                   CsrUint8 *password,
                                   CsrUint8 *nt_response,
                                   CsrUint8 *auth_response);

#ifdef __cplusplus
}
#endif

#endif /*CSR_CRYPTO_MSCHAP_H*/

