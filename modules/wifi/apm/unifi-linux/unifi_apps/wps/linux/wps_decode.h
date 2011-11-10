/** @file wps_decode.h
 *
 * Definitions for Wi-Fi Protected Setup (WPS) decoding.
 * 
 * @section LEGAL
 *   CONFIDENTIAL
 * 
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 * 
 * @section DESCRIPTION
 *   This provides an implementation of a Wi-Fi Protected Setup (WPS) decoding.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_decode.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_DECODE_H
#define WPS_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wps_common.h"

CsrUint8 getMessageType( WpsBuffer *pMessage );
CsrBool parseM2( WpsContext *pCtx );
CsrBool parseM4( WpsContext *pCtx );
CsrBool parseEncryptedSettingsM4( WpsContext *pCtx, WpsBuffer *encryptedSettings, WpsBuffer *rHash1 );
CsrBool parseM6( WpsContext *pCtx );
CsrBool parseEncryptedSettingsM6( WpsContext *pCtx, WpsBuffer *pEncryptedSettings );
CsrBool parseM8( WpsContext *pCtx );
CsrBool parseEncryptedSettingsM8( WpsContext *pCtx, WpsBuffer *pEncryptedSettings );
CsrBool parseCredential( WpsContext *pCtx, WpsBuffer *pCredential, int credentialCount );

#ifdef __cplusplus
}
#endif

#endif /* WPS_DECODE_H */
