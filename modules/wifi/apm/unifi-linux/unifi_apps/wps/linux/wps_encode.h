/** @file wps_encode.h
 *
 * Definitions for Wi-Fi Protected Setup (WPS) encoding.
 * 
 * @section LEGAL
 *   CONFIDENTIAL
 * 
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 * 
 * @section DESCRIPTION
 *   This provides an implementation of a Wi-Fi Protected Setup (WPS) encoding.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_encode.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_ENCODE_H
#define WPS_ENCODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wps_common.h"

void encodeM1( WpsContext *pCtx );
void encodeM3( WpsContext *pCtx );
void encodeM5( WpsContext *pCtx );
void encodeDecryptedSettingsM5( WpsContext *pCtx, WpsBuffer *pDecryptedSettings );
void encodeM7( WpsContext *pCtx );
void encodeDecryptedSettingsM7( WpsContext *pCtx, WpsBuffer *pDecryptedSettings );
void encodeDone( WpsContext *pCtx );
void encodeAck( WpsContext *pCtx );
void encodeNack( WpsContext *pCtx, unsigned char error );

#ifdef __cplusplus
}
#endif

#endif /* WPS_ENCODE_H */
