/** @file wps_scan.c
 *
 * Implementation of the Wi-Fi Protected Setup (WPS) scan functionality.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) scan functionality
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_scan.c#1 $
 *
 ****************************************************************************/
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/types.h>
#include <linux/wireless.h>
#include <string.h>
#include <net/if_arp.h>
#include "wps_scan.h"
#include "wps_common.h"
#include "wps_debug.h"

void wpaEnabledCheck( unsigned char *pBuffer, Network *pNetwork )
{
unsigned char wps_oui[] = { 0x00, 0x50, 0xf2, 0x01 };

    /* Check for WPA information element */
    if( pBuffer[0] == 0xdd && !memcmp( pBuffer+2, wps_oui, sizeof( wps_oui ) ) )
    {
        pNetwork->wpaEnabled = TRUE;
    }
}

void wpa2EnabledCheck( unsigned char *pBuffer, Network *pNetwork )
{
int i;
int count;
unsigned char cipher_suite_oui[] = { 0x00, 0x0f, 0xac };

    /* Check for WPA2/RSN information element */
    if( pBuffer[0] == 0x30 )
    {
        pNetwork->wpa2Enabled = TRUE;
        /* RSN header */
        pBuffer += 4;

        /* Group cipher suite */
        if( !memcmp( pBuffer, cipher_suite_oui, sizeof( cipher_suite_oui ) ) )
        {
            pBuffer += sizeof( cipher_suite_oui );
            switch( *pBuffer )
            {
            case 0: /* Use group cipher suite */
                /* Not valid for Group cipher suite */
                break;
            case 1: /* WEP-40 */
                pNetwork->groupCipherSuite = WPS_CIPHER_SUITE_WEP_40;
                break;
            case 2: /* TKIP */
                pNetwork->groupCipherSuite = WPS_CIPHER_SUITE_TKIP;
                break;
            case 3: /* Reserved */
                break;
            case 4: /* CCMP */
                pNetwork->groupCipherSuite = WPS_CIPHER_SUITE_CCMP;
                break;
            case 5: /* WEP-104 */
                pNetwork->groupCipherSuite = WPS_CIPHER_SUITE_WEP_104;
                break;
            default: /* Reserved */
                break;
            }
            pBuffer += 1;
        }
        else
        {
            WPS_DBG_PRINT(( "Vendor or other OUI not supported (%02x %02x %02x)", *pBuffer, *(pBuffer+1), *(pBuffer+2) ));
            pBuffer += 4;
        }

        count = *pBuffer++;
        count |= *pBuffer++ << 8;

        /* There may be multiple Pairwise Cipher Suites. Need to select the strongest suite offered */
        for( i = 0; i < count; i++ )
        {
            if( !memcmp( pBuffer, cipher_suite_oui, sizeof( cipher_suite_oui ) ) )
            {
                pBuffer += sizeof( cipher_suite_oui );
                switch( *pBuffer )
                {
                case 0: /* Use group cipher suite */
                    if( pNetwork->groupCipherSuite > pNetwork->pairwiseCipherSuite ) pNetwork->pairwiseCipherSuite = pNetwork->groupCipherSuite;
                    break;
                case 1: /* WEP-40 */
                    if( WPS_CIPHER_SUITE_WEP_40 > pNetwork->pairwiseCipherSuite ) pNetwork->pairwiseCipherSuite = WPS_CIPHER_SUITE_WEP_40;
                    break;
                case 2: /* TKIP */
                    if( WPS_CIPHER_SUITE_TKIP > pNetwork->pairwiseCipherSuite ) pNetwork->pairwiseCipherSuite = WPS_CIPHER_SUITE_TKIP;
                    break;
                case 3: /* Reserved */
                    break;
                case 4:
                    if( WPS_CIPHER_SUITE_CCMP > pNetwork->pairwiseCipherSuite ) pNetwork->pairwiseCipherSuite = WPS_CIPHER_SUITE_CCMP;
                    break;
                case 5:
                    if( WPS_CIPHER_SUITE_WEP_104 > pNetwork->pairwiseCipherSuite ) pNetwork->pairwiseCipherSuite = WPS_CIPHER_SUITE_WEP_104;
                    break;
                default: /* Reserved */
                    break;
                }

                pBuffer += 1;
            }
            else
            {
                WPS_DBG_PRINT(( "Vendor or other OUI not supported (%02x %02x %02x)", *pBuffer, *(pBuffer+1), *(pBuffer+2) ));
                pBuffer += 4;
            }
        }
    }
}

void wpsEnabledCheck( unsigned char *pBuffer, Network *pNetwork )
{
unsigned char wps_oui[] = { 0x00, 0x50, 0xf2, 0x04 };

    /* Check to see if this is a WPS information element */
    if( pBuffer[0] == 0xdd && !memcmp( pBuffer+2, wps_oui, sizeof( wps_oui ) ) )
    {
        pNetwork->wpsEnabled = TRUE;
    }
}

void wpsActivatedCheck( unsigned char *pBuffer, Network *pNetwork, WpsContext *pCtx )
{
int i;
unsigned char *pBufferEnd, *pIndex;
unsigned int type;
unsigned int len;
unsigned int attribute;
#define WPS_DEVICE_PASSWORD_ID 0x1012
unsigned char wps_oui[] = { 0x00, 0x50, 0xf2, 0x04 };

    /* First check to see if this is a WPS information element */
    if( pBuffer[0] == 0xdd && !memcmp( pBuffer+2, wps_oui, sizeof( wps_oui ) ) )
    {
        pIndex = pBuffer + 2 + sizeof( wps_oui );
        pBufferEnd = pBuffer + pBuffer[1] + 2;

        while( pIndex < pBufferEnd )
        {
            type = *pIndex++ << 8;
            if( pIndex > pBufferEnd ) break;
            type |= *pIndex++;
            if( pIndex > pBufferEnd ) break;
            len = *pIndex++ << 8;
            if( pIndex > pBufferEnd ) break;
            len |= *pIndex++;
            if( pIndex > pBufferEnd ) break;

            if( type == WPS_DEVICE_PASSWORD_ID )
            {
                attribute = *pIndex << 8;
                attribute |= *(pIndex + 1 );
                /* attribute will be 0x0000 for PIN mode and 0x0004 for pushbutton mode */
                if( attribute == 0x0000 && strcmp( pCtx->pin, WPS_PUSHBUTTON_PIN ) )
                {
                    WPS_DBG_PRINT(( "PIN mode activated" ));
                    pNetwork->wpsActivated = TRUE;
                    break;
                }
                if( attribute & 0x0004 && !strcmp( pCtx->pin, WPS_PUSHBUTTON_PIN ) )
                {
                    WPS_DBG_PRINT(( "Pushbutton mode activated" ));
                    pNetwork->wpsActivated = TRUE;
                    break;
                }
            }

            pIndex += len;
        }
    }
}

int compare_ssid( const void *pA, const void *pB )
{
    return strcmp( ((Network *)pA)->ssid, ((Network *)pB)->ssid );
}

int compare_signal( const void *pA, const void *pB )
{
int result;

    if( ((Network *)pA)->signal > ((Network *)pB)->signal ) result = -1;
    else
    if( ((Network *)pA)->signal < ((Network *)pB)->signal ) result = 1;
    else
    result = 0;

    return result;
}

int doScan( WpsContext *pCtx, Network *pCandidates, ScanMode *pScanMode )
{
int sock;
int r;
int err;
struct iwreq wrq;
unsigned char *pBufferStart = NULL;
unsigned char *pBuffer, *pBufferEnd;
int bufLen = 0;
int count = 0;
struct iw_event iwe;
unsigned char current_bssid[6];
int bssid_count = 0;
Network list[100];
unsigned char zero_bssid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
int i;
int n;

   sock = socket( AF_INET, SOCK_DGRAM, 0 );

   /* Always disconnect as scanning is disabled when connected */
   strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
   wrq.u.ap_addr.sa_family = ARPHRD_ETHER;
   memcpy(wrq.u.ap_addr.sa_data, zero_bssid, sizeof( zero_bssid ));
   r = ioctl( sock, SIOCSIWAP, &wrq );
   if( r < 0 ) WPS_DBG_PRINT(( "SIOCSIWAP = %d (%d: %s)", r, errno, strerror( errno ) ));

   strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
   wrq.u.data.pointer = NULL;
   wrq.u.data.flags = 0;
   wrq.u.data.length = 0;
   r = ioctl( sock, SIOCSIWSCAN, &wrq );
   if( r < 0 ) WPS_DBG_PRINT(( "SIOCSIWSCAN = %d (%d: %s)", r, errno, strerror( errno ) ));

   WPS_DBG_PRINT(( "Reading scan results" ));
   do
   {
       bufLen += IW_SCAN_MAX_DATA;
       pBufferStart = realloc( pBufferStart, bufLen );
       strncpy( wrq.ifr_name, "eth1", IFNAMSIZ );
       wrq.u.data.pointer = pBufferStart;
       wrq.u.data.flags = 0;
       wrq.u.data.length = bufLen;
       r = ioctl( sock, SIOCGIWSCAN, &wrq );
       err = errno;
       WPS_DBG_PRINT(( "SIOCGIWSCAN = %d (%d: %s])", r, err, strerror( err ) ));
   }
   while( r < 0 && err == E2BIG );

   close( sock );

   pBufferEnd = pBufferStart + wrq.u.data.length;
   pBuffer = pBufferStart;

    while( pBuffer < pBufferEnd )
    {
        memcpy( &iwe, pBuffer, IW_EV_LCP_LEN );
        switch( iwe.cmd )
        {
        case SIOCGIWAP:
            /* Delimeter between access points is SIOCGIWAP */
            memcpy( &iwe, pBuffer, IW_EV_ADDR_LEN );
            if( iwe.u.ap_addr.sa_family == ARPHRD_ETHER )
            {
                bssid_count++;
                memcpy( list[bssid_count-1].bssid, iwe.u.ap_addr.sa_data, 6 );
                list[bssid_count-1].privacy = FALSE;
                list[bssid_count-1].wpsEnabled = FALSE;
                list[bssid_count-1].wpsActivated = FALSE;
                list[bssid_count-1].wpaEnabled = FALSE;
                list[bssid_count-1].wpa2Enabled = FALSE;
                list[bssid_count-1].groupCipherSuite = WPS_CIPHER_SUITE_UNKNOWN;
                list[bssid_count-1].pairwiseCipherSuite = WPS_CIPHER_SUITE_UNKNOWN;
            }
            else
            {
                WPS_DBG_PRINT(( "Uknown protocol family = %d", iwe.u.ap_addr.sa_family ));
            }
            break;
        case SIOCGIWESSID:
            {
                memset( list[bssid_count-1].ssid, 0, 32 );
                memcpy( list[bssid_count-1].ssid, pBuffer + IW_EV_POINT_LEN, iwe.len - IW_EV_POINT_LEN );
            }
            break;
        case SIOCGIWENCODE:
            memcpy( (void *)&iwe + IW_EV_LCP_LEN + IW_EV_POINT_OFF, pBuffer + IW_EV_LCP_LEN, iwe.len - IW_EV_LCP_LEN );
            if( iwe.u.data.length != 0 || iwe.u.data.flags != IW_ENCODE_DISABLED )
            {
                list[bssid_count-1].privacy = TRUE;
            }
            break;
        case IWEVGENIE:
            {
            unsigned char *pIEs = pBuffer + IW_EV_POINT_LEN;
            int ieLength = iwe.len - IW_EV_POINT_LEN;
            unsigned char *pIE;

                for( pIE = pIEs; pIE < pIEs + ieLength; pIE += pIE[1] + 2 )
                {
                   wpaEnabledCheck( pIE, &list[bssid_count-1] );
                   wpa2EnabledCheck( pIE, &list[bssid_count-1] );
                   wpsEnabledCheck( pIE, &list[bssid_count-1] );
                   wpsActivatedCheck( pIE, &list[bssid_count-1], pCtx );
                }
            }
            break;
        case IWEVQUAL:
            memcpy( (void *)&iwe + IW_EV_LCP_LEN, pBuffer + IW_EV_LCP_LEN, iwe.len - IW_EV_LCP_LEN );
            list[bssid_count-1].signal = iwe.u.qual.level - 256;
            break;
        default:
            break;
        }

        pBuffer += iwe.len;
    }

   if( pBufferStart )free( pBufferStart );

   WPS_DBG_PRINT(( "bssid_count = %d", bssid_count ));

   qsort( list, bssid_count, sizeof( Network ), compare_signal );

   for( i = 0; i < bssid_count; i++ )
   {
       WPS_DBG_PRINT(( "%2d: %02x:%02x:%02x:%02x:%02x:%02x [%d:%d:%d:%d:%d] %4ddBm %s", i, list[i].bssid[0], list[i].bssid[1], list[i].bssid[2],
                                                                   list[i].bssid[3], list[i].bssid[4], list[i].bssid[5],
                                                                   list[i].privacy ? 1 : 0,
                                                                   list[i].wpaEnabled ? 1 : 0,
                                                                   list[i].wpa2Enabled ? 1 : 0,
                                                                   list[i].wpsEnabled ? 1 : 0,
                                                                   list[i].wpsActivated ? 1 : 0,
                                                                   list[i].signal,
                                                                   list[i].ssid ));
   }

   for( count = 0, i = 0; i < bssid_count; i++ )
   {
       if( list[i].wpsActivated == TRUE )
       {
           memcpy( &pCandidates[count], &list[i], sizeof( Network ) );
           count++;
           *pScanMode = WPS_SCAN_MODE_ACTIVATED;
       }
   }

   if( count == 0 )
   {
	   WPS_DBG_PRINT(( "No WPS activated networks detected" ));
	   for( count = 0, i = 0; i < bssid_count; i++ )
	   {
	       if( list[i].wpsEnabled == TRUE )
	       {
	           memcpy( &pCandidates[count], &list[i], sizeof( Network ) );
	           count++;
	           *pScanMode = WPS_SCAN_MODE_ENABLED;
	       }
	   }
	   if( count == 0 )
	   {
		   WPS_DBG_PRINT(( "No WPS enabled networks detected" ));
	   }
	   else
	   {
		   WPS_DBG_PRINT(( "%d WPS enabled networks detected", count ));
	   }
   }
   else
   {
	   WPS_DBG_PRINT(( "%d WPS activated networks detected", count ));
   }

   return count;
}
