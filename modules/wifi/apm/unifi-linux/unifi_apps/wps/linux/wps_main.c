/** @file wps_main.c
 *
 * Implementation of the Wi-Fi Protected Setup (WPS) main functions.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) main functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_main.c#2 $
 *
 ****************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/types.h>
#include <linux/wireless.h>
#include <string.h>
#include <net/if_arp.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <signal.h>

#include "wps_debug.h"
#include "wps_supplicant.h"
#include "wps_common.h"

#include "wps_types.h"

#include "unifiio.h"
#include "sigs.h"
#include "wps_event.h"
#include "wps_eap.h"
#include "wps_unifi.h"

#ifdef WPS_OPENSSL
#include <openssl/rand.h>
#else
#include "wps_random.h"
#endif

#include "os_version.h"

WpsContext wpsCtx;
unsigned char supp_mac[6];
WpsNetworkRecord networkRecords[WPS_MAX_NETWORK_RECORDS];
int networkRecordCount = 0;
CsrBool networkStubInstalled = FALSE;
int wpa_cli_index;

CsrBool doWps( Network *pCandidate )
{
fd_set readfds;
WpsEventResult result = WPS_RESULT_VOID;
int fd;
WpsEvent event = EV_IGNORE;
int eapId;
WpsState state = WPS_STATE_INIT;

    if( UnifiConnect( &fd, pCandidate ) )
    {
        /* Send EAPOL start to solicit an EAP Identity Request from the access point */
        WPS_DBG_PRINT(( "Sending EAPOL start" ));
        SendEapolStart( fd, pCandidate->bssid, supp_mac, &wpsCtx );

        wpsCtx.pCandidate = pCandidate;

        while( !result )
        {
            event = getEvent( fd, &wpsCtx, &eapId );
            result = HandleEvent( fd, eapId, event, &state, &wpsCtx, supp_mac );
        }

        WPS_DBG_PRINT(( "Close unifi descriptor = %d", fd ));
        close( fd );
    }

    return (result == WPS_RESULT_SUCCESS)? TRUE : FALSE;
}

void generatePin( unsigned char *pPin )
{
int i;
int digit;
int accum = 0;
unsigned char entropy[7];

#ifdef WPS_OPENSSL
    (void)RAND_bytes( entropy, sizeof( entropy ) );
#else
    WpsRandomBytes( entropy, sizeof( entropy ) );
#endif

    for( i = 0; i < 7; i++ )
    {
        digit = entropy[i] % 10;
        accum += digit * ((i%2) ? 1 : 3);
        *pPin++ = digit + '0';
    }
    digit = accum % 10;
    digit = (10 - digit) % 10;
    *pPin++ = digit + '0';
    *pPin = '\0';
}

void handleInterrupt( int signal )
{
	WPS_DBG_PRINT(( "User interrupt" ));

	if( networkStubInstalled )
	{
		WPS_DBG_PRINT(( "Removing Network Stub" ));
		removeNetwork( wpa_cli_index );
	}

    restoreNetworkStatus( networkRecords, networkRecordCount );
	exit(0);
}

main( int argc, char *argv[] )
{
int opt = 0;
static const char *optString = "p:Pdgvt:";
struct timeval tv;
fd_set rfds;
int retval;
int ch;
unsigned char bssid[6];
int i;
Network candidates[10];
ScanMode scanMode;
int result;
char pin[9];
time_t startTime, endTime;
time_t walkTime = WPS_WALK_TIME;
CsrBool scanning = TRUE;
int count;

    /* Initialise the PIN for this transaction */
    strcpy( wpsCtx.pin, WPS_PUSHBUTTON_PIN );

    /* Set stdout to line buffered mode */
    setvbuf( stdout, NULL, _IOLBF, 0 );

    opt = getopt( argc, argv, optString );
    while( opt != -1 ) {
        switch( opt ) {
            case 'p':
                if( strlen( optarg ) == 8 )
                {
                    for( i = 0; i < strlen( optarg ); i++ ) if( !isdigit(optarg[i]) ) break;
                    if( i == strlen( optarg ) )
                    {
                        strcpy( wpsCtx.pin, optarg );
                    }
                    else
                    {
                        WPS_DBG_PRINT(( "Invalid PIN" ));
                    }
                }
                else
                {
                    WPS_DBG_PRINT(( "Invalid PIN" ));
                }
                break;
            case 'P':
                generatePin( pin );
                strcpy( wpsCtx.pin, pin );
                printf( "PIN: %s\n", pin );
                break;
            case 'g':
                generatePin( pin );
                printf( "%s\n", pin );
                exit( 0 );
            case 'd':
                dbgEnabled = 1;
                break;
            case 't':
                walkTime = atoi( optarg );
                break;
            case 'v':
                printf( "v%s (build: %d) %s %s\n", UNIFI_DRIVER_VERSION, UNIFI_DRIVER_BUILD_ID, __DATE__, __TIME__ );
            	exit( 0 );
            	break;
            default:
                break;
        }

        opt = getopt( argc, argv, optString );
    }

    if( getuid() != 0 && geteuid() != 0 )
    {
        printf( "This application needs to be run as root\n" );
        exit( 1 );
    }

    printf( "Wi-Fi Protected Setup v%s (build: %d)\n", UNIFI_DRIVER_VERSION, UNIFI_DRIVER_BUILD_ID );

    WPS_DBG_PRINT(( "PIN = %s", wpsCtx.pin ));

    getMacAddress( supp_mac );

    if( !testCli() )
    {
        printf( "Unable to communicate with wpa_cli\n" );
        printf( "Check that wpa_supplicant is running and wpa_cli configured correctly\n" );
        exit( 0 );
    }

    if( !testConfSave() )
    {
        printf( "Unable to write to wpa_supplicant configuration file\n" );
        printf( "Check that update_config=1 has been added to configuration file\n" );
        exit( 0 );
    }

    findEnabledNetworks( networkRecords, &networkRecordCount );
    signal( SIGINT, handleInterrupt );

    printf( "Searching for active Registrar...\n" );

    startTime = time( NULL );
    endTime = startTime + walkTime;

    while( scanning )
    {
           count = doScan( &wpsCtx, &candidates, &scanMode );
           switch( scanMode )
           {
           case WPS_SCAN_MODE_ENABLED:
        	   WPS_DBG_PRINT(( "Scan mode = enabled" ));
        	   break;
           case WPS_SCAN_MODE_ACTIVATED:
        	   WPS_DBG_PRINT(( "Scan mode = activated" ));
        	   break;
           default:
        	   WPS_DBG_PRINT(( "Unknown scan mode (%d)", scanMode ));
        	   break;
           }

           if( count > 1 && scanMode == WPS_SCAN_MODE_ACTIVATED )
           {
               printf( "Session Overlap - Try again later\n" );
               scanning = FALSE;
           }
           else
           {
        	   if( strcmp( wpsCtx.pin, WPS_PUSHBUTTON_PIN ) == 0 && scanMode == WPS_SCAN_MODE_ENABLED )
        	   {
        		   /* Suppress all enabled mode scans during PBC */
        		   count = 0;
        	   }
        	   else
        	   if( time( NULL ) < startTime + 30 && scanMode == WPS_SCAN_MODE_ENABLED )
        	   {
        		   /* Suppress any enabled mode scans for the first 30 seconds */
        		   count = 0;
        	   }
        	   else
        	   /* Restrict active search depth during first section of walk time as WPS
        	    * access points are ranked by power level and target is most likely to be
        	    * nearby and have a high power ranking. This improves initial performance
        	    *  in the presence of many WPS enabled access points */
        	   if( time( NULL ) < startTime + 50 )
        	   {
        		   if( count > 1 )
        		   {
        			   count = 1;
        			   WPS_DBG_PRINT(( "Search depth restricted to 1" ));
        		   }
        	   }
        	   else
        	   if( time( NULL ) < startTime + 70 )
        	   {
        		   if( count > 2 )
        		   {
        			   count = 2;
        			   WPS_DBG_PRINT(( "Search depth restricted to 2" ));
        		   }
        	   }
        	   else
        	   if( time( NULL ) < startTime + 90 )
        	   {
        		   if( count > 4 )
        		   {
        			   count = 4;
        			   WPS_DBG_PRINT(( "Search depth restricted to 4" ));
        		   }
        	   }

        	   for( i = 0; i < count && scanning; i++ )
        	   {
                   WPS_DBG_PRINT(( "WPS candidate %d detected @ %02x:%02x:%02x:%02x:%02x:%02x", i,
                		                                                                        candidates[i].bssid[0], candidates[i].bssid[1],
                                                                                                candidates[i].bssid[2], candidates[i].bssid[3],
                                                                                                candidates[i].bssid[4], candidates[i].bssid[5] ));
                   if( addNetworkStub( candidates[i].ssid, &wpa_cli_index ) )
                   {
                	   networkStubInstalled = TRUE;
                       if( doWps( &candidates[i] ) )
                       {
                           removeAllNetwork( candidates[i].ssid );
                           networkStubInstalled = FALSE;

                           if( addNetwork( &wpsCtx, &candidates[i] ) )
                           {
                               printf( "Wi-Fi Protected Setup successful\n" );
                               scanning = FALSE;
                           }
                           else
                           {
                               printf( "Unable to update credentials in configuration file\n" );
                               scanning = FALSE;
                           }
                       }
                       else
                       {
                           removeNetwork( wpa_cli_index );
                           networkStubInstalled = FALSE;
                       }
                   }
                   else
                   {
                       printf( "Unable to update configuation file\n" );
                       scanning = FALSE;
                   }
        	   }
           }

           if( walkTime && time( NULL ) > endTime ) break;
    }

    if( scanning )
    {
          restoreNetworkStatus( networkRecords, networkRecordCount );
          printf( "Wi-Fi Protected Setup failed\n" );
          printf( "Walk time expired\n" );
    }

    return 0;
}
