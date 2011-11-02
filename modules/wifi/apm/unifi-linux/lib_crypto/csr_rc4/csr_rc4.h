/** @file csr_rc4.h
 *
 * CSR Crypto support function for Rivest Cipher 4 calculation.
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
 *   Rivest Cipher 4 implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_rc4/csr_rc4.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_RC4_H
#define CSR_RC4_H

#include "csr_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup crypto
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

/* MACROS *******************************************************************/

#define CSR_RC4_STATE_SIZE  256

/* TYPES DEFINITIONS ********************************************************/
/**
 * @brief defines Parameters collected as a result of a join scan
 *
 * @par Description
 *   Parameters collected as a result of a join scan
 */
typedef struct CsrRc4Key
{
    CsrUint8 i,j;
    CsrUint8 state[CSR_RC4_STATE_SIZE];
} CsrRc4Key;


/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/


/**
 * @brief Sets the key for RC4 encryption (RSA)
 *
 * @par Description
 *  Sets the key for RC4 encryption (RSA). Initialization function.
 *
 * @param[inout] key    : Key context
 * @param[in]    len    : Length of Key
 * @param[in]    data   : Key
 *
 * @return
 *   void
 */
extern void CsrRc4SetKey(CsrRc4Key *key,
                         const CsrUint16 len,
                         const CsrUint8 *data);


/**
 * @brief RC4 encryption (RSA)
 *
 * @par Description
 *  RC4 encryption (RSA) discards the first 256 bytes of output, also sets the key and uses its own context
 *
 * @param[inout] keyCtx : Key context
 * @param[in]    keyLen : Length of key
 * @param[in]    key    : key
 * @param[in]    len    : Length of input data
 * @param[in]    inBuf  : Input data buffer
 * @param[out]   outBuf : Output buffer
 *
 * @return
 *   void
 */
extern void CsrRc4Discard256 (const CsrUint16 keyLen,
                              const CsrUint8 *key,
                              const CsrUint16 len,
                              const CsrUint8 *inBuf,
                              CsrUint8 *outBuf);


/**
 * @brief RC4 encryption (RSA)
 *
 * @par Description
 *  RC4 encryption (RSA)
 *
 * @param[inout] key    : Key context
 * @param[in]    len    : Length of key
 * @param[in]    inBuf  : Input data buffer
 * @param[out]   outBuf : Output buffer
 *
 * @return
 *   void
 */
extern void CsrRc4(CsrRc4Key *key,
                   const CsrUint16 len,
                   const CsrUint8 *inBuf,
                   CsrUint8 *outBuf);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /*CSR_RC4_H*/
