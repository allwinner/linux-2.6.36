/** @file wps_crypto.h
 *
 * Definitions for Wi-Fi Protected Setup (WPS) cryptographic functions.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) cryptographic functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_crypto.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_CRYPTO_H
#define WPS_CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wps_common.h"

void initEnrolleeNonce( CsrWpsContext *pCtx );
void initKeyPair( CsrWpsContext *pCtx );
void computeSessionKeys( CsrWpsContext *pCtx );
CsrBool verifyAuthenticator( CsrWpsContext *pCtx, WpsBuffer *pPreviousMessage, WpsBuffer *pCurrentMessage, CsrUint8 *pAuthenticator );
void initEhash( CsrWpsContext *pCtx );
void encryptEncryptedSettingsData( CsrWpsContext *pCtx );

#ifdef __cplusplus
}
#endif

#endif /* WPS_CRYPTO_H */
