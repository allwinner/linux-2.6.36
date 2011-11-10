/** @file wps_debug.c
 *
 * Implementation of the Wi-Fi Protected Setup debug functions
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup debug functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_debug.c#2 $
 *
 ****************************************************************************/

#include "wps_debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* STUB FUNCTIONS FOR CSR CRYPTO INTEGRATION ********************************/

void sme_trace_hex_fn(int id, int level, const char* const message, const void* address, int length)
{
    dbgPrintHex((const char *)message, (unsigned char *)address, length);
}

void sme_trace_info_fn(int id, const char* const format, ...)
{
}

void sme_trace_error_fn(int id, const char* const format, ...)
{
}

void CsrPanic(unsigned char tech, unsigned short int reason, const char *p)
{
}

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

int dbgEnabled = 0;

void dbgPrint( const char *format, ... )
{
va_list ap;
char buffer[255];
char *pChar;

    if( !dbgEnabled ) return;

    va_start( ap, format );
    vsprintf( buffer, format, ap );
    va_end( ap );

    /* Replace any newline characters with space unless first character */
    /* This is to clean up blank lines in the debug output */
    while( (pChar = strrchr( buffer + 1, '\n' )) != NULL ) *pChar = ' ';

    fprintf( stderr, "%s", buffer );
}

#define WIDTH 16

void dbgPrintHex( const char *string, unsigned char *pBuffer, unsigned int length )
{
int i, j;

    if( !dbgEnabled ) return;

    fprintf( stderr, "%s\n", string );

    for( i = 0; i + WIDTH < length; i += WIDTH )
    {
        for( j = 0; j < WIDTH; j++ )
        {
            fprintf( stderr, "%02x ", pBuffer[i+j] );
        }

        fprintf( stderr, "%*s", 3, " " );

        for( j = 0; j < WIDTH; j++ )
        {
            fprintf( stderr, "%c", isprint( pBuffer[i+j] ) ? pBuffer[i+j] : '.' );
        }
        fprintf( stderr, "\n" );
    }

    for( j = 0; j+i < length; j++ )
    {
        fprintf( stderr, "%02x ", pBuffer[j+i] );
    }

    while( j++ < WIDTH )
    {
        fprintf( stderr, "%*s", 3, " " );
    }

    fprintf( stderr, "%*s", 3, " " );

    for( j = 0; j+i < length; j++ )
    {
        fprintf( stderr, "%c", isprint( pBuffer[i+j] ) ? pBuffer[i+j] : '.' );
    }

    if( i < length ) fprintf( stderr, "\n" );
}
