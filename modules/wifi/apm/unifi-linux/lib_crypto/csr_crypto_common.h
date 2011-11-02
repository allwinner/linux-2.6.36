/** @file csr_crypto_common.h
 *
 * Common definitions for crypto library.
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
 *   Header file for common crypto definitions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_crypto_common.h#1 $
 *
 ****************************************************************************/

#ifndef CSR_CRYPTO_COMMON_H
#define CSR_CRYPTO_COMMON_H


#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"


/* PUBLIC TYPE DEFINITIONS ************************************************/

typedef enum CsrCryptoPadMode
{
    PAD_MODE_NONE,
    PAD_MODE_PKCS7,
    PAD_MODE_TLS
}CsrCryptoPadMode;

#ifdef __cplusplus
}
#endif

#endif  /* CSR_CRYPTO_COMMON_H */
