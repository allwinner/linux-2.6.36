/** @file wps_random.h
 *
 * Definitions for Wi-Fi Protected Setup (WPS) random number functions.
 * 
 * @section LEGAL
 *   CONFIDENTIAL
 * 
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 * 
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) random number functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_random.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_RANDOM_H
#define WPS_RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

void WpsRandomBytes( unsigned char *pBuffer, unsigned int length );

#ifdef __cplusplus
}
#endif

#endif /* WPS_RANDOM_H */
