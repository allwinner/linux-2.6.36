/** @file wpa_psk_hash.h
 *
 * Pairwise Master Key derivation header
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
 *   Provides API for Pairwise Master Key derivation from SSID and WPA passphrase
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_passphrase_hashing.h#1 $
 *
 ****************************************************************************/

#ifndef CSR_PASSPHRASE_HASHING_H
#define CSR_PASSPHRASE_HASHING_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup security_plugin
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"
#include "csr_util.h"
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE

#ifdef CSR_CRYPTO_KDHMACSHA256_ENABLE
#include "csr_kd_hmac_sha256/csr_kd_hmac_sha256.h"
#endif
/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Derives PMK from SSID and passphrase
 *
 * @par Description
 *   Derives PMK from SSID and passphrase.

 * @param[in]     passphrase              : Zero terminated string containing WPA passphrase
 * @param[in]     ssid                    : SSID in sec_api_SSID structure format
 * @param[inout]  pmk                     : Computed 256 bit Pairwise Master Key
 *
 * @return
 *   void
 */
extern void CsrCryptoWpaPskHash(
                    char *passphrase,
                    CsrUint8 *ssid,
                    CsrUint8 ssidLength,
                    CsrUint8 pmk[32]);

/**
 * @brief Derives PMK from SSID and passphrase
 *
 * @par Description
 *   Derives PMK from SSID and passphrase.

 * @param[in]     passphrase              : Zero terminated string containing WPA passphrase
 * @param[in]     ssid                    : SSID in sec_api_SSID structure format
 * @param[inout]  pmk                     : Computed 256 bit Pairwise Master Key
 *
 * @return
 *   void
 */
#ifdef CSR_CRYPTO_KDHMACSHA256_ENABLE
#define CSR_CRYPTO_WAPI_PSK_LABEL "preshared key expansion for authentication and key negotiation"
#define CsrCryptoWapiPskHash(k,kl,o,ol) CsrCryptoKdHmacSha256((CsrUint8*)CSR_CRYPTO_WAPI_PSK_LABEL,CsrStrLen(CSR_CRYPTO_WAPI_PSK_LABEL),(CsrUint8*)k,kl,(CsrUint8*)o,ol);
#endif

/** \@}
 */

#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_PASSPHRASE_HASHING_H */
