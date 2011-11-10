/** @file wps_supplicant.c
 *
 * Implementation of the Wi-Fi Protected Setup supplicant interface
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
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_supplicant.c#6 $
 *
 ****************************************************************************/

#include "wps_supplicant.h"
#include "wps_debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

int indexCliCommand( int *pIndex, char *pFormat, ... )
{
int result = 0;
char buffer[100];
FILE *p;
va_list ap;

    va_start( ap, pFormat );
    vsprintf( buffer, pFormat, ap );
    va_end( ap );

    strcat( buffer, " 2>&1" );
    WPS_DBG_PRINT(( "%s", buffer ));
    p = popen( buffer, "r" );
    while( fgets( buffer, 100, p ) != NULL ) WPS_DBG_PRINT(( "%s", buffer ));
    pclose( p );
    if( sscanf( buffer, "%d", pIndex ) != 1 )
    {
        WPS_DBG_PRINT(( "No index retrieved" ));
        result = 1;
    }

    return result;
}

int cliCommand( char *pFormat, ... )
{
int result = 0;
char buffer[100];
FILE *p;
va_list ap;

    va_start( ap, pFormat );
    vsprintf( buffer, pFormat, ap );
    va_end( ap );

    strcat( buffer, " 2>&1" );
    WPS_DBG_PRINT(( "%s", buffer ));
    p = popen( buffer, "r" );
    while( fgets( buffer, 100, p ) != NULL ) WPS_DBG_PRINT(( "%s", buffer ));
    pclose( p );
    if( !strstr( buffer, "OK" ) )
    {
        result = 1;
    }

    return result;
}

int testCli()
{
int result = 0;
char buffer[100];
FILE *p;
int index;

/* Add a network and remove it again to check that WPA CLI is running OK */

    do
    {
        if( indexCliCommand( &index, "wpa_cli add_network" ) ) break;
        if( cliCommand( "wpa_cli remove_network %d", index ) ) break;
        result = 1;
    }
    while( 0 );

    return result;
}

int testConfSave()
{
int result = 0;
char buffer[100];
FILE *p;
int index = 0;

/* Try to save the config file */

    do
    {
        if( cliCommand( "wpa_cli save_config", index ) ) break;
        result = 1;
    }
    while( 0 );

    return result;
}

int addNetworkStub( char *ssid, int *pIndex )
{
int result = 0;
char buffer[100];
FILE *p;

    do
    {
        if( indexCliCommand( pIndex, "wpa_cli add_network" ) ) break;
        if( cliCommand( "wpa_cli set_network %d ssid \'\"%s\"\'", *pIndex, ssid ) ) break;
        if( cliCommand( "wpa_cli set_network %d key_mgmt NONE", *pIndex ) ) break;
        if( cliCommand( "wpa_cli enable_network %d", *pIndex ) ) break;
        if( cliCommand( "wpa_cli select_network %d", *pIndex ) ) break;
        if( cliCommand( "wpa_cli save_config", *pIndex ) ) break;
        if( cliCommand( "wpa_cli reconfigure" ) ) break;
        if( cliCommand( "wpa_cli disconnect" ) ) break;
        result = 1;
    }
    while( 0 );

    return result;
}

int removeNetwork( int index )
{
int result = 0;
char buffer[100];
FILE *p;

    do
    {
        if( cliCommand( "wpa_cli remove_network %d", index ) ) break;
        if( cliCommand( "wpa_cli save_config" ) ) break;
        if( cliCommand( "wpa_cli reconfigure" ) ) break;
        if( cliCommand( "wpa_cli reassociate" ) ) break;
        result = 1;
    }
    while( 0 );

    return result;
}

int addNetwork( WpsContext *pCtx, Network *pNetwork )
{
int result = 0;
FILE *p;
char networkKey[100];
char ssid[100];
char value[100];
int index;

    memcpy( networkKey, pCtx->networkKey.pStart, pCtx->networkKey.size );
    networkKey[pCtx->networkKey.size] = 0;

    memcpy( ssid, pCtx->ssid.pStart, pCtx->ssid.size );
    ssid[pCtx->ssid.size] = 0;

    switch( pCtx->authenticationType )
    {
    case WPS_AUTHENTICATION_TYPE_OPEN:
        WPS_DBG_PRINT(( "Authentication Type: Open" ));
        break;
    case WPS_AUTHENTICATION_TYPE_WPAPSK:
        WPS_DBG_PRINT(( "Authentication Type: WPAPSK" ));
        break;
    case WPS_AUTHENTICATION_TYPE_SHARED:
        WPS_DBG_PRINT(( "Authentication Type: Shared" ));
        break;
    case WPS_AUTHENTICATION_TYPE_WPA:
        WPS_DBG_PRINT(( "Authentication Type : WPA" ));
        break;
    case WPS_AUTHENTICATION_TYPE_WPA2:
        WPS_DBG_PRINT(( "Authentication Type: WPA2" ));
        break;
    case WPS_AUTHENTICATION_TYPE_WPA2PSK:
        WPS_DBG_PRINT(( "Authentication Type: WPA2PSK" ));
        break;
    default:
        WPS_DBG_PRINT((" Authentication Type unknown (%04x)", pCtx->authenticationType ));
        break;
    }

    switch( pCtx->encryptionType )
    {
    case WPS_ENCRYPTION_TYPE_NONE:
        WPS_DBG_PRINT(( "Encryption Type: None" ));
        break;
    case WPS_ENCRYPTION_TYPE_WEP:
        WPS_DBG_PRINT(( "Encryption Type: WEP" ));
        break;
    case WPS_ENCRYPTION_TYPE_TKIP:
        WPS_DBG_PRINT(( "Encryption Type: TKIP" ));
        break;
    case WPS_ENCRYPTION_TYPE_AES:
        WPS_DBG_PRINT(( "Encryption Type: AES" ));
        break;
    default:
        WPS_DBG_PRINT(( "Encryption Type unknown (%04x)", pCtx->encryptionType ));
        break;
    }

    do
    {
        if( indexCliCommand( &index, "wpa_cli add_network" ) ) break;
        if( cliCommand( "wpa_cli set_network %d ssid \'\"%s\"\'", index, ssid ) ) break;

        switch( pCtx->authenticationType )
        {
        case WPS_AUTHENTICATION_TYPE_WPAPSK:
        case WPS_AUTHENTICATION_TYPE_WPA2PSK:
            strcpy( value, "WPA-PSK" );
            break;
        case WPS_AUTHENTICATION_TYPE_WPA:
        case WPS_AUTHENTICATION_TYPE_WPA2:
            strcpy( value, "80211X" );
            break;
        case WPS_AUTHENTICATION_TYPE_OPEN:
        case WPS_AUTHENTICATION_TYPE_SHARED:
        default:
            strcpy( value, "NONE" );
            break;
        }

        if( cliCommand( "wpa_cli set_network %d key_mgmt %s", index, value ) ) break;

        if( pCtx->authenticationType == WPS_AUTHENTICATION_TYPE_WPAPSK || pCtx->authenticationType == WPS_AUTHENTICATION_TYPE_WPA2PSK )
        {
            WPS_DBG_PRINT(( "pCtx->networkKey.size = %d", pCtx->networkKey.size ));
            if( pCtx->networkKey.size == 64 )
            {
                if( cliCommand( "wpa_cli set_network %d psk %s", index, networkKey ) ) break;
            }
            else
            {
                if( cliCommand( "wpa_cli set_network %d psk \'\"%s\"\'", index, networkKey ) ) break;
            }
        }

        if( pCtx->encryptionType == WPS_ENCRYPTION_TYPE_WEP )
        {
            WPS_DBG_PRINT(( "pCtx->networkKey.size = %d", pCtx->networkKey.size ));
            WPS_DBG_PRINTHEX(( "Key", pCtx->networkKey.pStart, pCtx->networkKey.size));
            if(pCtx->networkKey.size == 26 || pCtx->networkKey.size == 10)
            {
                if( cliCommand( "wpa_cli set_network %d wep_key%d %s", index, pCtx->networkKeyIndex - 1, networkKey ) ) break;
            }
            else
            {
                if( cliCommand( "wpa_cli set_network %d wep_key%d \'\"%s\"\'", index, pCtx->networkKeyIndex - 1, networkKey ) ) break;
            }

            if( cliCommand( "wpa_cli set_network %d wep_tx_keyidx %d", index, pCtx->networkKeyIndex) ) break;
        }

        if( cliCommand( "wpa_cli enable_network %d", index ) ) break;
        if( cliCommand( "wpa_cli select_network %d", index ) ) break;
        if( cliCommand( "wpa_cli save_config" ) ) break;
        if( cliCommand( "wpa_cli reconfigure" ) ) break;
        if( cliCommand( "wpa_cli reassociate" ) ) break;
        result = 1;
    }
    while( 0 );

    return result;
}

int removeAllNetwork( char *target_ssid )
{
int result = 1;
char buffer[100];
FILE *p;
int index;

    sprintf( buffer, "wpa_cli list_networks 2>&1" );
    WPS_DBG_PRINT(( "%s", buffer ));
    p = popen( buffer, "r" );
    while( fgets( buffer, 100, p ) != NULL )
    {
        WPS_DBG_PRINT(( "%s", buffer ));
        if( sscanf( buffer, "%d", &index ) == 1 && strstr(buffer, target_ssid) != NULL )
        {
            if( !cliCommand( "wpa_cli remove_network %d", index ) ) result = 0;
        }
    }
    pclose( p );

    if( result )
    {
        result = 0;
        do
        {
            if( cliCommand( "wpa_cli save_config" ) ) break;
            if( cliCommand( "wpa_cli reconfigure" ) ) break;
            result = 1;
        }
        while( 0 );
    }

    return result;
}

int findEnabledNetworks(WpsNetworkRecord *pNetworkRecords, int *pNetworkRecordCount)
{
int result = 1;
char buffer[100];
FILE *p;
int index;
int n;

    sprintf(buffer, "wpa_cli list_networks 2>&1");
    WPS_DBG_PRINT(("%s", buffer));
    p = popen(buffer, "r");
    while(fgets(buffer, 100, p) != NULL)
    {
        WPS_DBG_PRINT(("%s", buffer));
        n = sscanf(buffer, "%d", &index);
        if(n)
        {
            pNetworkRecords[*pNetworkRecordCount].index = index;
            if(strstr(buffer, "[CURRENT]"))
            {
                WPS_DBG_PRINT(("Network %d status is CURRENT", index));
                pNetworkRecords[*pNetworkRecordCount].status = WPS_NETWORK_STATUS_ACTIVE;
            }
            else
            if(strstr(buffer, "[DISABLED]"))
            {
                WPS_DBG_PRINT(("Network %d status is DISABLED", index));
                pNetworkRecords[*pNetworkRecordCount].status = WPS_NETWORK_STATUS_DISABLED;
            }
            else
            {
            	WPS_DBG_PRINT(("Network %d status is ENABLED", index));
                pNetworkRecords[*pNetworkRecordCount].status = WPS_NETWORK_STATUS_ENABLED;
            }
            *pNetworkRecordCount += 1;
        }
    }
    pclose(p);

    return result;
}

int restoreNetworkStatus( WpsNetworkRecord *pNetworkRecords, int networkRecordCount )
{
int result = 1;
int i;

    WPS_DBG_PRINT(( "Restoring network status" ));
    WPS_DBG_PRINT(( "networkRecordCount = %d", networkRecordCount ));

    for( i = 0; i < networkRecordCount; i++ )
    {
    	if( pNetworkRecords[i].status == WPS_NETWORK_STATUS_ACTIVE )
    	{
    		WPS_DBG_PRINT(( "Restore network %d to active", pNetworkRecords[i].index ));
    		cliCommand( "wpa_cli select_network %d", pNetworkRecords[i].index );
    	}
    }

    for( i = 0; i < networkRecordCount; i++ )
    {
    	if( pNetworkRecords[i].status == WPS_NETWORK_STATUS_ENABLED )
    	{
    		WPS_DBG_PRINT(( "Restore network %d to enabled", pNetworkRecords[i].index ));
    		cliCommand( "wpa_cli enable_network %d", pNetworkRecords[i].index );
    	}
    	else
      	if( pNetworkRecords[i].status == WPS_NETWORK_STATUS_DISABLED )
       	{
       		WPS_DBG_PRINT(( "Restore network %d to disabled", pNetworkRecords[i].index ));
       		cliCommand( "wpa_cli disable_network %d", pNetworkRecords[i].index );
       	}
    }

    cliCommand( "wpa_cli save_config" );
    cliCommand( "wpa_cli reconfigure" );

    return result;
}
