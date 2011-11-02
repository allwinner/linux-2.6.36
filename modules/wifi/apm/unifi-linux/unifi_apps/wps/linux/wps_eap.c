/** @file wps_eap.c
 *
 * Implementation of the Wi-Fi Protected Setup (WPS) EAP handling.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) EAP handling
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_eap.c#1 $
 *
 ****************************************************************************/

#include "wps_eap.h"
#include "wps_debug.h"
#include "wps_common.h"
#include "wps_types.h"
#include "sigs.h"
#include <string.h>

void SendEapolRequest( int fd, unsigned char *da, unsigned char *sa, unsigned char *body, unsigned int body_len )
{
unsigned char *pSignal, *pSignalIndex;
int signal_len;
int n;

CSR_DATAREF                                 Data;
CSR_DATAREF                                 Dummydataref2;
CSR_MACADDRESS                              Sa;
CSR_MACADDRESS                              Da;
CSR_PRIORITY                                Priority;
CSR_CLIENT_TAG                              HostTag;

    pSignal = pSignalIndex = malloc( 32 + body_len );

    /* SignalId */
    *pSignalIndex++ = CSR_MLME_EAPOL_REQUEST_ID & 0xff;
    *pSignalIndex++ = CSR_MLME_EAPOL_REQUEST_ID >> 8;

    /* ReceiverProcessId */
    *pSignalIndex++ = 0;
    *pSignalIndex++ = 0;

    /* SenderProcessId */
    *pSignalIndex++ = 0x13;
    *pSignalIndex++ = 0xC0;

    /* DataRef 1 slotnumber */
    *pSignalIndex++ = 0;
    *pSignalIndex++ = 0;

    /* DataRef 1 length */
    *pSignalIndex++ = body_len & 0xff;
    *pSignalIndex++ = body_len >> 8;

    /* DataRef 2 slotnumber */
    *pSignalIndex++ = 0;
    *pSignalIndex++ = 0;

    /* DataRef 2 length */
    *pSignalIndex++ = 0;
    *pSignalIndex++ = 0;

    memcpy( pSignalIndex, sa, 6 );
    pSignalIndex += 6;

    memcpy( pSignalIndex, da, 6 );
    pSignalIndex += 6;

    /* CSR_PRIORITY */
    *pSignalIndex++ = 0;
    *pSignalIndex++ = 0;

    /* CSR_CLIENT_TAG */
    *pSignalIndex++ = 0;
    *pSignalIndex++ = 0;
    *pSignalIndex++ = 0;
    *pSignalIndex++ = 0;

    memcpy( pSignalIndex, body, body_len );
    pSignalIndex += body_len;

    signal_len = pSignalIndex - pSignal;

    /* Send the command to the unifi. */
    if ( (n = write(fd, pSignal, signal_len)) != signal_len) {
        WPS_DBG_PRINT(( "Error writing to Unifi on fd %d: %d of %d bytes written (%s)", fd, n, signal_len, strerror( errno ) ));
    }
    else
    {
        WPS_DBG_PRINT(( "%d of %d bytes written on fd %d (%s)", n, signal_len, fd, strerror( errno ) ));
    }

}

void SendEapolStart( int fd, unsigned char *da, unsigned char *sa, WpsContext *pCtx )
{
unsigned char eapol_start[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e, 0x02, 0x01, 0x00, 0x00 };

    /* Save message for any retransmission */
    freeBuffer( &pCtx->lastTx );
    initBuffer( &pCtx->lastTx, (CsrUint8 *)malloc(sizeof( eapol_start )), sizeof( eapol_start ) );
    memcpy( pCtx->lastTx.pStart, eapol_start, sizeof( eapol_start) );
    pCtx->txCount = 0;

    SendEapolRequest( fd, da, sa, eapol_start, sizeof( eapol_start ) );
}

void SendResponse( int fd, unsigned char eapId, unsigned char opCode, unsigned char *da, unsigned char *sa, WpsContext *pCtx )
{
unsigned char response[4096];
unsigned char *pBuffer;
int length;
int resp_len;
unsigned char wps_eap_id[] = { 0x00, 0x37, 0x2a, 0x00, 0x00, 0x00, 0x01 };

    pBuffer = response;
    *pBuffer++ = 0xaa; /* LLC header */
    *pBuffer++ = 0xaa;
    *pBuffer++ = 0x03;
    *pBuffer++ = 0x00;
    *pBuffer++ = 0x00;
    *pBuffer++ = 0x00;
    *pBuffer++ = 0x88;
    *pBuffer++ = 0x8e;
    *pBuffer++ = 0x02; /* Version */
    *pBuffer++ = 0x00; /* EAP packet */
    length = 5 + sizeof( wps_eap_id ) + 2 + pCtx->out.size;
    *pBuffer++ = (length >> 8) & 0xff;
    *pBuffer++ = length & 0xff;
    *pBuffer++ = 0x02; /* Code response */
    *pBuffer++ = eapId; /* Id */
    *pBuffer++ = (length >> 8) & 0xff;
    *pBuffer++ = length & 0xff;
    *pBuffer++ = 0xfe; /* Expanded type */
    memcpy( pBuffer, wps_eap_id, sizeof( wps_eap_id ) );
    pBuffer += sizeof( wps_eap_id );
    *pBuffer++ = opCode; /* WSC OP CODE */
    *pBuffer++ = 0x00; /* Flags */
    memcpy( pBuffer, pCtx->out.pStart, pCtx->out.size );
    pBuffer += pCtx->out.size;
    resp_len = pBuffer - response;

    /* Save message for any retransmission */
    freeBuffer( &pCtx->lastTx );
    initBuffer( &pCtx->lastTx, (CsrUint8 *)malloc( resp_len ), resp_len );
    memcpy( pCtx->lastTx.pStart, response, resp_len );
    pCtx->txCount = 0;

    WPS_DBG_PRINTHEX(( "Sending EAP response", response, resp_len ));
    SendEapolRequest( fd, da, sa, response, resp_len );
}

void SendIdentityResponse( int fd, unsigned char eapId, unsigned char *da, unsigned char *sa, WpsContext *pCtx )
{
unsigned char response[4096];
unsigned char *pBuffer;
char wps_string[] = "WFA-SimpleConfig-Enrollee-1-0";
int length;
int resp_len;

    pBuffer = response;
    *pBuffer++ = 0xaa; /* LLC header */
    *pBuffer++ = 0xaa;
    *pBuffer++ = 0x03;
    *pBuffer++ = 0x00;
    *pBuffer++ = 0x00;
    *pBuffer++ = 0x00;
    *pBuffer++ = 0x88;
    *pBuffer++ = 0x8e;
    *pBuffer++ = 0x02; /* Version */
    *pBuffer++ = 0x00; /* EAP packet */
    length = 5 + strlen( wps_string );
    *pBuffer++ = (length >> 8) & 0xff;
    *pBuffer++ = length & 0xff;
    *pBuffer++ = 0x02; /* Code response */
    *pBuffer++ = eapId; /* Id */
    *pBuffer++ = (length >> 8) & 0xff;
    *pBuffer++ = length & 0xff;
    *pBuffer++ = 0x01; /* Identity */
    memcpy( pBuffer, wps_string, strlen( wps_string ) );
    pBuffer += strlen( wps_string );
    resp_len = pBuffer - response;

    /* Save message for any retransmission */
    freeBuffer( &pCtx->lastTx );
    initBuffer( &pCtx->lastTx, (CsrUint8 *)malloc( resp_len ), resp_len );
    memcpy( pCtx->lastTx.pStart, response, resp_len );
    pCtx->txCount = 0;

    WPS_DBG_PRINTHEX(( "Sending Identity Response", response, resp_len ));
    SendEapolRequest( fd, da, sa, response, resp_len );
}
