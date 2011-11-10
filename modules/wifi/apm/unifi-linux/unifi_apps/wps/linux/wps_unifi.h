/** @file wps_unifi.h
 *
 * Definitions for Wi-Fi Protected Setup UniFi interface.
 * 
 * @section LEGAL
 *   CONFIDENTIAL
 * 
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 * 
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup UniFi interface
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_unifi.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_UNIFI_H
#define WPS_UNIFI_H

#include "wps_common.h"

void getMacAddress( unsigned char *macAddress );
int UnifiConnect( int *pFd, Network *pCandidate );

#endif /* WPS_UNIFI_H */
