/** @file csr_md4.h
 *
 * Support function for MD4 calculation.
 * Definitions of the MD4 Message-Digest Algorithm (RFC 1320) function.
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
 *   MD4 Message-Digest Algorithm (RFC 1320) implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_md4/csr_md4.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_MD4_H
#define CSR_MD4_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

/**
 * @brief Implements the MD4 algorithm as specified in RFC 1320.
 *
 * @pre Output buffer has been allocated and has room for 16 bytes.
 *
 * @param[in] in : The input message. This routine works only on whole bytes,
 *                 although strictly speaking the algorithm is applicable to
 *                 arbitary bit lengths.
 * @param[in] len : Byte length of the input message.
 * @param[out] out : A 16-byte buffer into which to store the output hash.
 *
 * @return
 *      void
 */
void CsrCryptoMd4(const CsrUint8 *const in, const CsrUint32 in_len, CsrUint8 *out);

#ifdef __cplusplus
}
#endif

#endif /*CSR_MD4_H*/

