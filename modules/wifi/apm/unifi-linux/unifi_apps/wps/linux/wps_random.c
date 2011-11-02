/** @file wps_random.c
 *
 * Implementation of the Wi-Fi Protected Setup (WPS) random number functions
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
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_random.c#1 $
 *
 ****************************************************************************/

#include "wps_random.h"
#include "wps_debug.h"
#include <stdlib.h>
#include <fcntl.h>

/* MAPPING FUNCTION FOR CSR CRYPTO INTEGRATION ******************************/

unsigned int CsrRandom(void)
{
    unsigned int result;

    WpsRandomBytes((unsigned char *)&result, sizeof(result));

    return result;
}

/* PRIVATE CONSTANTS ********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

void WpsRandomBytes( unsigned char *pBuffer, unsigned int length )
{
int fd;
int n;

    fd = open( "/dev/urandom", O_RDONLY );
    if( fd < 0 )
    {
    	WPS_DBG_PRINT(( "Unable to open /dev/urandom" ));
    }
    else
    {
        n = read( fd, (char *)pBuffer, length );
    	WPS_DBG_PRINT(( "Read %d of %d random bytes", n, length ));
        close( fd );
    }
}
