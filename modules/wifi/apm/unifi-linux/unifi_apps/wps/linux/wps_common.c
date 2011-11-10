/** @file wps_common.c
 *
 * Implementation of the Wi-Fi Protected Setup (WPS) common functions
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) common functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_common.c#1 $
 *
 ****************************************************************************/

#include "wps_common.h"
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if.h>
#include "wps_debug.h"

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

void initBuffer( WpsBuffer *pBuf, CsrUint8 *pData, unsigned int size )
{
   pBuf->pStart = pBuf->pIndex = pData;
   pBuf->size = size;
}

void freeBuffer( WpsBuffer *pBuf )
{
    if( pBuf->pStart )
    {
        free( pBuf->pStart );
        pBuf->pStart = pBuf->pIndex = NULL;
        pBuf->size = 0;
    }
}
