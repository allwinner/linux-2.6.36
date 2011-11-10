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
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_common.c#1 $
 *
 ****************************************************************************/

#include "wps_common.h"

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

void initBuffer( WpsBuffer *pBuf, CsrUint8 *pData, CsrUint32 size )
{
   pBuf->pStart = pBuf->pIndex = pData;
   pBuf->size = size;
}

void freeBuffer( WpsBuffer *pBuf )
{
    if( pBuf->pStart )
    {
        CsrPfree( pBuf->pStart );
        pBuf->pStart = pBuf->pIndex = NULL;
        pBuf->size = 0;
    }
}
