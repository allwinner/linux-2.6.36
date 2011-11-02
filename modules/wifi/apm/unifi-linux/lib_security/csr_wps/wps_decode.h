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
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_decode.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_DECODE_H
#define WPS_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wps_common.h"

CsrUint8 getMessageType(WpsBuffer *pMessage);
void parseM2(CsrWpsContext *pCtx);
void parseM4(CsrWpsContext *pCtx);
void parseEncryptedSettingsM4(CsrWpsContext *pCtx);
void parseM6(CsrWpsContext *pCtx);
void parseEncryptedSettingsM6(CsrWpsContext *pCtx);
void parseM8(CsrWpsContext *pCtx);
void parseEncryptedSettingsM8(CsrWpsContext *pCtx);
CsrBool parseCredential(CsrWpsContext *pCtx, WpsBuffer *pCredential, CsrInt32 credentialCount);

#ifdef __cplusplus
}
#endif

#endif /* WPS_DECODE_H */
