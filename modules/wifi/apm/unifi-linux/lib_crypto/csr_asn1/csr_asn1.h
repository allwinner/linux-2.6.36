/** @file csr_asn1.h
 *
 * CSR Crypto support function for ASN.1 encoded information.
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
 *   ASN.1 implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_asn1/csr_asn1.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_ASN1_H
#define CSR_ASN1_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

/**
 * @brief Extract RSA Public Modulus from X.509 DER Certificate
 *
 * @par Description
 *   Get Public Modulus
 *
 * @param[inout]   scan                    : address of pointer to X.509 DER certificate encoding
 *
 * @return
 *   int       length                  : length of RSA Public Modulus starting from *scan, or 0 if parsing error
 */
extern CsrUint32 CsrCryptoAsn1GetRsaPublicModulus(const CsrUint8 **scan);

/**
 * @brief Extract RSA Public Exponent from X.509 DER Certificate
 *
 * @par Description
 *   Get Public Exponent
 *
 * @param[inout]   scan                    : address of pointer to X.509 DER certificate encoding (following CsrCryptoAsn1GetRsaPublicModulus)
 *
 * @return
 *   int       length                  : length of RSA Public Exponent starting from *scan, or 0 if parsing error
 */
extern CsrUint32 CsrCryptoAsn1GetRsaPublicExponent(const CsrUint8 **scan);

/**
 * @brief Extract RSA Private Modulus from PKCS #1 RSAPrivateKey (RFC2313)
 *
 * @par Description
 *   Get Public Modulus
 *
 * @param[inout]   scan                    : address of pointer to DER certificate encoding
 *
 * @return
 *   int       length                  : length of RSA Public Modulus starting from *scan, or 0 if parsing error
 */
extern CsrUint32 CsrCryptoAsn1GetRsaPrivateModulus(const CsrUint8 **scan);

/**
 * @brief Extract RSA Private Exponent from PKCS #1 RSAPrivateKey (RFC2313)
 *
 * @par Description
 *   Get Public Exponent
 *
 * @param[inout]   scan                    : address of pointer to DER certificate encoding (following CsrCryptoAsn1GetRsaPublicModulus)
 *
 * @return
 *   int       length                  : length of RSA Public Exponent starting from *scan, or 0 if parsing error
 */
extern CsrUint32 CsrCryptoAsn1GetRsaPrivateExponent(const CsrUint8 **scan);

/**
 * @brief Extract subject name from x509v3 certificate
 *
 * @par Description
 *   Extract subject name from x509v3 certificate
 *
 * @param[inout]   scan                    : address of pointer to DER certificate encoding, contains pointer to
 *                                           subject name after function returns
 *
 * @return
 *   CsrInt32           length                  : length of subject name starting from *scan, or 0 if parsing error
 */
extern CsrUint32 CsrCryptoAsn1GetEcSubjectName(const CsrUint8 **scan);

/**
 * @brief Extract issuer from x509v3 certificate
 *
 * @par Description
 *   Extract issuer from x509v3 certificate
 *
 * @param[inout]   scan                    : address of pointer to DER certificate encoding, contains pointer to
 *                                           issuer after function returns
 *
 * @return
 *   CsrInt32           length                  : length of issuer starting from *scan, or 0 if parsing error
 */
extern CsrUint32 CsrCryptoAsn1GetEcIssuer(const CsrUint8 **scan);

/**
 * @brief Extract serial number from x509v3 certificate
 *
 * @par Description
 *   Extract serial number from x509v3 certificate
 *
 * @param[inout]   scan                    : address of pointer to DER certificate encoding, contains pointer to
 *                                           serial number after function returns
 *
 * @return
 *   CsrInt32           length                  : length of serial number field starting from *scan, or 0 if parsing error
 */
extern CsrUint32 CsrCryptoAsn1GetEcSerialNumber(const CsrUint8 **scan);

/**
 * @brief Extract Public key from x509v3 certificate
 *
 * @par Description
 *   Extract public key from x509v3 certificate
 *
 * @param[inout]   scan                    : address of pointer to DER certificate encoding, contains pointer to
 *                                           public key after function returns
 *
 * @return
 *   CsrInt32           length                  : length of public key starting from *scan, or 0 if parsing error
 */
extern CsrUint32 CsrCryptoAsn1GetEcPublicKey(const CsrUint8 **scan);

/**
 * @brief Extract private key from DER encoded private key file
 *
 * @par Description
 *   Extract private key from DER encoded private key file. Format described in
 *
 *    STANDARDS FOR EFFICIENT CRYPTOGRAPHY
 *       SEC 1: Elliptic Curve Cryptography
 *            Certicom Research
 *   Contact: secg-talk@lists.certicom.com
 *            September 20, 2000
 *                Version 1.0
 *
 * @param[inout]   scan                    : address of pointer to DER certificate encoding, contains pointer to
 *                                           private key after function returns
 *
 * @return
 *   CsrInt32           length                  : length of private key starting from *scan, or 0 if parsing error
 */
extern CsrUint32 CsrCryptoAsn1GetEcPrivateKey(const CsrUint8 **scan);

#ifdef __cplusplus
}
#endif

#endif /* CSR_ASN1_H */

