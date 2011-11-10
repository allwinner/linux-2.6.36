/** @file csr_bn_private.h
 *
 * CSR Crypto private data.
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
 *   BigNum private data.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_private.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_BN_PRIVATE_H
#define CSR_BN_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

    struct CsrBignum
    {
        CsrUint32 digit[128]; /* The digits in the big number are 32 bits each - allow up to 4096 bits */
        CsrUint32 length;        /* Number of 32 bit digits in use */
        CsrUint32 unusedBytes;   /* Number of bytes unused in most significant digit from 0-3 */
        CsrBool negative;  /* True if number has negative sign */
    };

#ifdef __cplusplus
}
#endif

#endif /*CSR_BN_PRIVATE_H*/

