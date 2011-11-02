/** @file csr_dh.h
 *
 * Security Manager CCX support function for Diffie-Hellman calculation.
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
 *   Diffie-Hellman computation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_dh/csr_dh.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_DH_H
#define CSR_DH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

/**
 * @brief Calculate a Diffie-Hellman public/private key pair.
 *
 * @param[in]      g_base : the base for the Diffie-Hellman calculation. Usually 2 or 5.
 * @param[in]      prime : the prime number for the Diffie-Hellman calculation.
 * @param[in]      size : size of prime and resulting keys (in octets).
 * @param[in/out]  private_key : private key (supplied or generated as per do_private).
 * @param[out]     public_key : generated public key.
 * @param[in]      do_private : if TRUE then generate a new private key. If FALSE then the private key is supplied.
 *
 * @return
 *   TRUE = success. eapfast_context contains the key pair. FALSE = failure.
 */
CsrBool CsrCryptoDhGenerateKeys
(CsrUint32 g_base, CsrUint8 *prime, CsrUint32 size, CsrUint8 *private_key, CsrUint8 *public_key, CsrBool do_private);

/**
 * @brief Calculate the DH shared secret from the private key, other parties public key and the prime.
 *
 * @param[out]  result : buffer to put the shared secret in
 * @param[in]   others_pub : buffer containing public key from the other party
 * @param[in]   our_priv : buffer containing our local private key
 * @param[in]   prime : buffer containing the DH prime
 * @param[in]   size : the size of all buffers (they must all be the same)
 */
void CsrCryptoDhCalculateSharedSecret
(CsrUint8 *result, CsrUint8 *others_pub, CsrUint8 *out_priv, CsrUint8 *prime, CsrUint32 size);

#ifdef __cplusplus
}
#endif

#endif /*CSR_DH_H*/

