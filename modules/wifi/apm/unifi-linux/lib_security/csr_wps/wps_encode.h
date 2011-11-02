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
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_encode.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_ENCODE_H
#define WPS_ENCODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wps_common.h"

void sendM1(CsrWpsContext *pCtx);
void sendM3(CsrWpsContext *pCtx);
void sendM5(CsrWpsContext *pCtx);
void encodeDecryptedSettingsM5(CsrWpsContext *pCtx);
void sendM7(CsrWpsContext *pCtx);
void encodeDecryptedSettingsM7(CsrWpsContext *pCtx);
void sendDone(CsrWpsContext *pCtx);
void sendAck(CsrWpsContext *pCtx);
void sendNack(CsrWpsContext *pCtx, unsigned char error);

#ifdef __cplusplus
}
#endif

#endif /* WPS_ENCODE_H */
