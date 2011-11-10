/** @file wps_supplicant.h
 *
 * Definitions for Wi-Fi Protected Setup supplicant interface.
 * 
 * @section LEGAL
 *   CONFIDENTIAL
 * 
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 * 
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup supplicant interface
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_supplicant.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_SUPPLICANT_H
#define WPS_SUPPLICANT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wps_common.h"

#define WPS_MAX_NETWORK_RECORDS 50
	
typedef enum
{
	WPS_NETWORK_STATUS_ENABLED,
	WPS_NETWORK_STATUS_DISABLED,
	WPS_NETWORK_STATUS_ACTIVE
}WpsNetworkStatus;

typedef struct WpsNetworkRecord
{
	int index;
	WpsNetworkStatus status;
}WpsNetworkRecord;

int indexCliCommand( int *pIndex, char *pFormat, ... );
int cliCommand( char *pFormat, ... );
int testCli();
int testConfSave();
int addNetworkStub( char *ssid, int *pIndex );
int removeNetwork( int index );
int addNetwork( WpsContext *pCtx, Network *pNetwork );
int removeAllNetwork( char *ssid );
int findEnabledNetworks( WpsNetworkRecord *pNetworkRecords, int *pNetworkRecordCount );
int restoreNetworkStatus( WpsNetworkRecord *pNetworkRecords, int networkRecordCount );

#ifdef __cplusplus
}
#endif

#endif /* WPS_DEBUG_H */
