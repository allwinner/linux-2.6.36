/** @file wps_unifi.c
 *
 * Implementation of the Wi-Fi Protected Setup UniFi interface
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
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_unifi.c#2 $
 *
 ****************************************************************************/
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/types.h>
#include <linux/wireless.h>
#include <string.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>

#include "csr_types.h"
#include "wps_types.h"
#include "unifiio.h"
#include "wps_unifi.h"
#include "wps_debug.h"
#include "wps_common.h"

void getMacAddress( unsigned char macAddress[6] )
{
struct ifreq ifr;
int sock;
int result;
int i;

    sock = socket( AF_INET, SOCK_DGRAM, 0 );

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy( ifr.ifr_name, "eth1", IFNAMSIZ-1 );
    result = ioctl( sock, SIOCGIFHWADDR, &ifr );
    WPS_DBG_PRINT(( "SIOCGIFHWADDR = %d (%s)", result, strerror( errno ) ));
    if( result )
    {
        memset( macAddress, 0, 6 );
    }
    else
    {
        memcpy( macAddress, ifr.ifr_hwaddr.sa_data, 6 );
    }
}

int UnifiConnect( int *pFd, Network *pCandidate )
{
struct iwreq wrq;
int sock = 0;
/*
10 4a Version
00 01
10

10 3a Request Type
00 01
01

*/
unsigned char ie[] = { 0xdd, 0x0e, 0x00, 0x50, 0xf2, 0x04, 0x10, 0x4a, 0x00, 0x01, 0x10, 0x10, 0x3a, 0x00, 0x01, 0x01 };
unsigned char zero_bssid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
int result = 0;
int r;
char *device = "/dev/unifiudi0";
int access = 1;
int attempt;
#define CONNECT_ATTEMPT_LIMIT 2

    do
    {
        sock = socket( AF_INET, SOCK_DGRAM, 0 );
        WPS_DBG_PRINT(( "Open socket = %d (%s)", sock, strerror( errno ) ));
        if( sock < 0 ) break;

        /* Set mode = mananged */
        strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
        wrq.u.mode = 2; /* Managed Mode */
        r = ioctl( sock, SIOCSIWMODE, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWMODE = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

        /* Setting key */
        strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
        wrq.u.data.pointer = (caddr_t) NULL;
        wrq.u.data.flags = IW_ENCODE_ENABLED | IW_ENCODE_NOKEY;
        wrq.u.data.length = 0;
        r = ioctl( sock, SIOCSIWENCODE, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWENCODE = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

       /* Set Privacy Enabled */
        strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
        wrq.u.param.flags = IW_AUTH_PRIVACY_INVOKED;
        if( pCandidate->privacy )
        {
            wrq.u.param.value = 1;
        }
        else
        {
            wrq.u.param.value = 0;
        }

        r = ioctl( sock, SIOCSIWAUTH, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWAUTH = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

        /* Set Open System */
        strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
        wrq.u.param.flags = IW_AUTH_80211_AUTH_ALG;
        wrq.u.param.value = IW_AUTH_ALG_OPEN_SYSTEM;
        r = ioctl( sock, SIOCSIWAUTH, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWAUTH = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

        /* Set WPA Version */
        strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
        wrq.u.param.flags = IW_AUTH_WPA_VERSION;
        if( pCandidate->wpaEnabled || pCandidate->wpa2Enabled )
        {
            wrq.u.param.value = IW_AUTH_WPA_VERSION_WPA | IW_AUTH_WPA_VERSION_WPA2;
        }
        else
        {
            wrq.u.param.value = IW_AUTH_WPA_VERSION_DISABLED;
        }
        r = ioctl( sock, SIOCSIWAUTH, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWAUTH = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

        /* Set Pairwise Cipher Suites */
        strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
        wrq.u.param.flags = IW_AUTH_CIPHER_PAIRWISE;
        wrq.u.param.value = IW_AUTH_CIPHER_NONE | IW_AUTH_CIPHER_WEP40 | IW_AUTH_CIPHER_TKIP | IW_AUTH_CIPHER_CCMP | IW_AUTH_CIPHER_WEP104;
        r = ioctl( sock, SIOCSIWAUTH, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWAUTH = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

        /* Set Cipher Groups */
        strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
        wrq.u.param.flags = IW_AUTH_CIPHER_GROUP;
        wrq.u.param.value = IW_AUTH_CIPHER_WEP40 | IW_AUTH_CIPHER_WEP104 | IW_AUTH_CIPHER_TKIP | IW_AUTH_CIPHER_CCMP;
        r = ioctl( sock, SIOCSIWAUTH, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWAUTH = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

        /* Set Key Management */
        strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
        wrq.u.param.flags = IW_AUTH_KEY_MGMT;
        wrq.u.param.value = IW_AUTH_KEY_MGMT_PSK;
        r = ioctl( sock, SIOCSIWAUTH, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWAUTH = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

        /* Inject WPS information element */
        strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
        wrq.u.data.length = sizeof( ie );
        wrq.u.data.pointer = ie;
        r = ioctl( sock, SIOCSIWGENIE, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWGENIE = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

        /* Disconnect */
        wrq.u.ap_addr.sa_family = ARPHRD_ETHER;
        memcpy(wrq.u.ap_addr.sa_data, zero_bssid, sizeof( zero_bssid ));
        r = ioctl( sock, SIOCSIWAP, &wrq );
        WPS_DBG_PRINT(( "SIOCSIWAP = %d (%s)", r, strerror( errno ) ));
        if( r ) break;

        /* Establish communication with Unifi char driver. */
        *pFd = open( device, O_RDWR );
        WPS_DBG_PRINT(( "Open Unifi character device = %d (%s)", *pFd, strerror( errno ) ));
        if( *pFd < 0 ) break;

        r = ioctl( *pFd, UNIFI_SET_UDI_ENABLE, &access );
        WPS_DBG_PRINT(( "UNIFI_SET_UDI_ENABLE = %d (%s)", r, strerror( errno ) ));
        if( r < 0 )
        {
            r = close( *pFd );
            WPS_DBG_PRINT(( "Close Unifi character device = %d (%s)", *pFd, strerror( errno ) ));
            break;
        }

        /* Connect */
        attempt = 0;
        do
        {
            wrq.u.ap_addr.sa_family = ARPHRD_ETHER;
            memcpy(wrq.u.ap_addr.sa_data, pCandidate->bssid, 6 );
            r = ioctl( sock, SIOCSIWAP, &wrq );
            WPS_DBG_PRINT(( "SIOCSIWAP = %d (%s)", r, strerror( errno ) ));
            if( r )
            {
                WPS_DBG_PRINT(( "Failed to connect" ));
                sleep( 1 );
            }
        }
        while( r != 0 && attempt++ < CONNECT_ATTEMPT_LIMIT );

        if( attempt >= CONNECT_ATTEMPT_LIMIT )
        {
            WPS_DBG_PRINT(( "Connect attempt limit reached" ));
            r = close( *pFd );
            WPS_DBG_PRINT(( "Close Unifi character device = %d (%s)", *pFd, strerror( errno ) ));
            break;
        }

        /* If no break by this point - everything is OK */
        result = 1;
    }
    while( 0 );

    if( !( sock < 0 ) )
    {
        WPS_DBG_PRINT(( "Close socket = %d (%s)", sock, strerror( errno ) ));
        close( sock );
    }

    return result;
}
