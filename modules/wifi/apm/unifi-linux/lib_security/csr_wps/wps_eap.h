/** @file wps_eap.h
 *
 * Definitions for Wi-Fi Protected Setup (WPS) EAP handling.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of a Wi-Fi Protected Setup (WPS) EAP handling.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_eap.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_EAP_H
#define WPS_EAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wps_common.h"

void SendEapolStart(unsigned char *da, unsigned char *sa, CsrWpsContext *pCtx );
void SendResponse(unsigned char eapId, unsigned char opCode, unsigned char *da, unsigned char *sa, CsrWpsContext *pCtx );
void SendIdentityResponse(unsigned char eapId, unsigned char *da, unsigned char *sa, CsrWpsContext *pCtx );

#ifdef __cplusplus
}
#endif
#endif /* WPS_EAP_H */
