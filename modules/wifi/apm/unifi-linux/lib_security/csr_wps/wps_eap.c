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
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_eap.c#2 $
 *
 ****************************************************************************/

#include "wps_eap.h"
#include "wps_common.h"
#include "sme_trace/sme_trace.h"


void SendEapolStart(unsigned char *da, unsigned char *sa, CsrWpsContext *pCtx )
{
    unsigned char eapol_start[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e, 0x02, 0x01, 0x00, 0x00 };

    /* Save message for any retransmission */
    freeBuffer( &pCtx->lastTx );
    initBuffer( &pCtx->lastTx, (CsrUint8 *)CsrPmalloc(sizeof( eapol_start )), sizeof( eapol_start ) );
    CsrMemCpy( pCtx->lastTx.pStart, eapol_start, sizeof( eapol_start) );
    pCtx->txCount = 0;

    /* Reset timeout */
    pCtx->securityContext->callbacks.stopTimer(pCtx->securityContext->externalContext,
                                               CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);

    /* start timer */
    pCtx->securityContext->callbacks.startTimer(pCtx->securityContext->externalContext,
                                                pCtx->securityContext->setupData.responseTimeout,
                                                CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);

    pCtx->securityContext->callbacks.sendPacket(pCtx->securityContext->externalContext,
                                                eapol_start,
                                                sizeof(eapol_start),
                                                pCtx->securityContext->setupData.localMacAddress,
                                                pCtx->securityContext->setupData.peerMacAddress);
}

void SendResponse(unsigned char eapId, unsigned char opCode, unsigned char *da, unsigned char *sa, CsrWpsContext *pCtx )
{
    unsigned char *response;
    unsigned char *pBuffer;
    CsrUint32 length;
    CsrUint32 resp_len;
    unsigned char wps_eap_id[] = { 0x00, 0x37, 0x2a, 0x00, 0x00, 0x00, 0x01 };

    response = (unsigned char *)CsrPmalloc(4096);
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
    CsrMemCpy( pBuffer, wps_eap_id, sizeof( wps_eap_id ) );
    pBuffer += sizeof( wps_eap_id );
    *pBuffer++ = opCode; /* WSC OP CODE */
    *pBuffer++ = 0x00; /* Flags */
    CsrMemCpy( pBuffer, pCtx->out.pStart, pCtx->out.size );
    pBuffer += pCtx->out.size;
    resp_len = (CsrUint32) (pBuffer - response);

    /* Save message for any retransmission */
    freeBuffer( &pCtx->lastTx );
    initBuffer( &pCtx->lastTx, (CsrUint8 *)CsrPmalloc( resp_len ), resp_len );
    CsrMemCpy( pCtx->lastTx.pStart, response, resp_len );
    pCtx->txCount = 0;

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Sending EAP response", response, resp_len ));

    /* Reset timeout */
    pCtx->securityContext->callbacks.stopTimer(pCtx->securityContext->externalContext,
                                               CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);

    /* start timer */
    pCtx->securityContext->callbacks.startTimer(pCtx->securityContext->externalContext,
                                                pCtx->securityContext->setupData.responseTimeout,
                                                CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);

    pCtx->securityContext->callbacks.sendPacket(pCtx->securityContext->externalContext,
                                                response,
                                                resp_len,
                                                pCtx->securityContext->setupData.localMacAddress,
                                                pCtx->securityContext->setupData.peerMacAddress);

    CsrPfree(response);
}

void SendIdentityResponse( unsigned char eapId, unsigned char *da, unsigned char *sa, CsrWpsContext *pCtx )
{
    unsigned char *response;
    unsigned char *pBuffer;
    char wps_string[] = "WFA-SimpleConfig-Enrollee-1-0";
    CsrUint32 length;
    CsrUint32 resp_len;

    response = (unsigned char *)CsrPmalloc(100);
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
    length = 5 + CsrStrLen( wps_string );
    *pBuffer++ = (length >> 8) & 0xff;
    *pBuffer++ = length & 0xff;
    *pBuffer++ = 0x02; /* Code response */
    *pBuffer++ = eapId; /* Id */
    *pBuffer++ = (length >> 8) & 0xff;
    *pBuffer++ = length & 0xff;
    *pBuffer++ = 0x01; /* Identity */
    CsrMemCpy( pBuffer, wps_string, CsrStrLen( wps_string ) );
    pBuffer += CsrStrLen( wps_string );
    resp_len = (CsrUint32) (pBuffer - response);

    /* Save message for any retransmission */
    freeBuffer( &pCtx->lastTx );
    initBuffer( &pCtx->lastTx, (CsrUint8 *)CsrPmalloc( resp_len ), resp_len );
    CsrMemCpy( pCtx->lastTx.pStart, response, resp_len );
    pCtx->txCount = 0;

    /* Reset timeout */
    pCtx->securityContext->callbacks.stopTimer(pCtx->securityContext->externalContext,
                                               CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Sending Identity Response", response, resp_len ));

    /* start timer */
    pCtx->securityContext->callbacks.startTimer(pCtx->securityContext->externalContext,
                                                pCtx->securityContext->setupData.responseTimeout,
                                                CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);

    pCtx->securityContext->callbacks.sendPacket(pCtx->securityContext->externalContext,
                                                response,
                                                resp_len,
                                                pCtx->securityContext->setupData.localMacAddress,
                                                pCtx->securityContext->setupData.peerMacAddress);

    CsrPfree(response);
}
