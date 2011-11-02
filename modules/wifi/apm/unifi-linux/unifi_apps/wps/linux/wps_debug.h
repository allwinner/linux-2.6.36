/** @file wps_debug.h
 *
 * Definitions for Wi-Fi Protected Setup debug.
 * 
 * @section LEGAL
 *   CONFIDENTIAL
 * 
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 * 
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup debug
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_debug.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_DEBUG_H
#define WPS_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

#if 1
#define WPS_DBG_PRINT( args ) { dbgPrint( "%s@%04d: ", __FILE__, __LINE__ ); dbgPrint args; dbgPrint( "\n" ); errno = 0; }
#define WPS_DBG_PRINTHEX( args ) { dbgPrint( "%s@%04d: ", __FILE__, __LINE__ ); dbgPrintHex args; }
#else
#define WPS_DBG_PRINT( args )
#define WPS_DBG_PRINTHEX( args )
#endif

extern int dbgEnabled;

void dbgPrint( const char *format, ... );
void dbgPrintHex( const char *string, unsigned char *pBuffer, unsigned int length );

#ifdef __cplusplus
}
#endif

#endif /* WPS_DEBUG_H */
