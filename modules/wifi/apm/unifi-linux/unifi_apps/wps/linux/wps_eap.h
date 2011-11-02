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
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_eap.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_EAP_H
#define WPS_EAP_H

#include "wps_common.h"

void SendEapolStart( int fd, unsigned char *da, unsigned char *sa, WpsContext *pCtx );
void SendResponse( int fd, unsigned char eapId, unsigned char opCode, unsigned char *da, unsigned char *sa, WpsContext *pCtx );
void SendIdentityResponse( int fd, unsigned char eapId, unsigned char *da, unsigned char *sa, WpsContext *pCtx );

#endif /* WPS_EAP_H */
