/** @file eaptls_utils.h
 *
 * Definitions of utilities for a EAP-TLS method.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   This provides an implementation of utilities for a TLS authentication method.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/tls/eaptls_utils.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_EAPTLS_UTILS_H
#define SECURITY_METHOD_EAPTLS_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"
#include "csr_tls.h"

/**
 * @brief Add Certificate Verify to TLS message
 *
 * @par Description
 *   Add Certificate Verify to TLS message
 *
 * @param[inout]   fillout                 : pointer to current message build location
 * @param[in]      modulusLength           : length of RSA Private Key modulus
 * @param[in]      client_private          : pointer to ASN.1 DER RSA Private Key information
 * @param[inout]   tls_data                : pointer to TLS data structure
 *
 * @return
 *   CsrUint8*           fillout                 : updated pointer to current message build location
 */
CsrUint8 *TlsAddCertificateVerify(CsrUint8 *fillout, const CsrUint32 modulusLength, const CsrUint8 *client_private, tls_data *tls);

/**
 * @brief Add Client Key Exchange to TLS message
 *
 * @par Description
 *   Add Client Key Exchange to TLS message
 *
 * @param[inout]   fillout                 : pointer to current message build location
 * @param[in]      modulusLength           : length of RSA Private Key modulus
 * @param[in]      pre_master_secret       : pointer to buffer containing 48 byte Pre-Master Secret
 * @param[inout]   tls_data                : pointer to TLS data structure
 *
 * @return
 *   CsrUint8*           fillout                 : updated pointer to current message build location
 */
CsrUint8 *TlsAddClientKeyExchange(CsrUint8 *fillout, const CsrUint32 modulusLength, const CsrUint8 *pre_master_secret, tls_data *tls);

/**
 * @brief Build the Pre-Master Secret
 *
 * @par Description
 *   Build the Pre-Master Secret
 *
 * @param[inout]   pre_master_secret       : pointer to 48 byte buffer to receive Pre-Master Secret
 *
 * @return
 *   void
 */
void TlsBuildPreMasterSecret(CsrUint8 *pre_master_secret);

/**
 * @brief Add Client Certificate to TLS message
 *
 * @par Description
 *   Add Client Certificate to TLS message
 *
 * @param[inout]   fillout                 : pointer to current message build location
 * @param[in]      client_cert             : pointer to ASN.1 DER RSA Client Certificate
 * @param[inout]   tls_data                : pointer to TLS data structure
 *
 * @return
 *   CsrUint8*           fillout                 : updated pointer to current message build location
 */
CsrUint8 *TlsAddClientCertificate(CsrUint8 *fillout, const CsrUint8 *client_cert, CsrUint32 client_cert_length, tls_data *tls);

/**
 * @brief Derive the Master Secret
 *
 * @par Description
 *   Derive the Master Secret
 *
 * @param[in]      pre_master_secret       : pointer to 48 byte Pre-Master Secret
 * @param[inout]   tls_data                : pointer to TLS data structure
 *
 * @return
 *   void
 */
void TlsDeriveMasterSecret(const CsrUint8* const pre_master_secret, tls_data *tls);

/**
 * @brief Derive the PMK
 *
 * @par Description
 *   Derive the PMK
 *
 * @param[inout]   tlsContext              : pointer to TLS context
 *
 * @return
 *   void
 */
void TlsDerivePmk(TlsContext* tlsContext);

/**
 * @brief Add Finished to TLS message
 *
 * @par Description
 *   Add Finished to TLS message
 *
 * @param[inout]   fillout                 : pointer to current message build location
 * @param[inout]   tls_data                : pointer to TLS data structure
 *
 * @return
 *   CsrUint8*           fillout                 : updated pointer to current message build location
 */
CsrUint8 *TlsAddFinished(CsrUint8 *fillout, tls_data *tls);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_METHOD_EAPTLS_UTILS_H */
