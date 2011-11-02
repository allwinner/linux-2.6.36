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
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_crypto.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_CRYPTO_H
#define WPS_CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "wps_common.h"

void initEnrolleeNonce( WpsContext *pCtx );
void initKeyPair( WpsContext *pCtx );
void computeSessionKeys( WpsContext *pCtx );
CsrBool verifyAuthenticator( WpsContext *pCtx, WpsBuffer *pPreviousMessage, WpsBuffer *pCurrentMessage, CsrUint8 *pAuthenticator );
void computeAuthenticator( WpsContext *pCtx, WpsBuffer *pPreviousMessage, WpsBuffer *pCurrentMessage, CsrUint8 *pAuthenticator );
void computeEHash( WpsContext *pCtx, CsrUint8 *nonce, CsrUint8 *psk, CsrUint8 *pHash );
CsrBool verifyEHash( WpsContext *pCtx, CsrUint8 *nonce, CsrUint8 *psk, CsrUint8 *pHash );
void initEhash( WpsContext *pCtx );
void decryptEncryptedSettings( WpsContext *pCtx, WpsBuffer *encryptedSettings, WpsBuffer *decryptedSettings );
void encryptEncryptedSettingsData( WpsContext *pCtx );
void encryptDecryptedSettings( WpsContext *pCtx, WpsBuffer *pDecryptedSettings, WpsBuffer *pEncryptedSettings );
void computeKeyWrapAuthenticator( WpsContext *pCtx, WpsBuffer *pMessage, CsrUint8 *pAuthenticator );
CsrBool verifyKeyWrapAuthenticator( WpsContext *pCtx, WpsBuffer *pMessage, CsrUint8 *pAuthenticator );

#ifdef __cplusplus
}
#endif

#endif /* WPS_CRYPTO_H */
