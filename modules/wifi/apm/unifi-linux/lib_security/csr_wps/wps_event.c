/** @file wps_event.c
 *
 * Implementation of the Wi-Fi Protected Setup (WPS) event handling.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) event handling
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_event.c#1 $
 *
 ****************************************************************************/
#include "wps_event.h"
#include "wps_common.h"
#include "wps_eap.h"
#include "wps_decode.h"
#include "wps_encode.h"
#include "sme_trace/sme_trace.h"

WpsEvent eapRequestEvent(unsigned char *buffer, CsrUint16 bufferLength, CsrWpsContext *pCtx)
{
    WpsEvent event = EV_IGNORE;
    unsigned char messageType;
    unsigned char wps_eap_id[] = { 0x00, 0x37, 0x2a, 0x00, 0x00, 0x00, 0x01 };

    switch( buffer[16] )
    {
    case 0x01:
        /* EAP Identity */
        event = EV_EAP_IDENTITY_REQUEST;
        break;
    case 0xfe:
        /* Expanded EAP type */
        if( CsrMemCmp( buffer + 17, wps_eap_id, sizeof (wps_eap_id ) ) ) break;
        switch( buffer[24] )
        {
        case 0x01:
            /* WSC_Start */
            event = EV_WSC_START;
            break;
        case 0x02:
            /* WSC_ACK */
            sme_trace_info((TR_SECURITY_LIB, "WSC_ACK" ));
            break;
        case 0x03:
            /* WSC_NACK */
            sme_trace_info((TR_SECURITY_LIB, "WSC_NACK" ));
            event = EV_WSC_NACK;
            break;
        case 0x04:
            /* WSC_MSG */
            freeBuffer( &pCtx->in );
            initBuffer( &pCtx->in, (CsrUint8 *)CsrPmalloc( bufferLength - 26 ), bufferLength - 26 );
            CsrMemCpy( pCtx->in.pStart, buffer + 26, pCtx->in.size );
            messageType = getMessageType( &pCtx->in );
            switch(messageType)
            {
            case WPS_M2:
                event = EV_M2;
                break;
            case WPS_M2D:
                event = EV_M2D;
                break;
            case WPS_M4:
                event = EV_M4;
                break;
            case WPS_M6:
                event = EV_M6;
                break;
            case WPS_M8:
                event = EV_M8;
                break;
            default:
                sme_trace_info((TR_SECURITY_LIB, "WPS Unsupported message type (0x%02x)", messageType ));
                break;
            }
            break;
            case 0x05:
                /* WSC_Done */
                sme_trace_info((TR_SECURITY_LIB, "WSC_Done" ));
                break;
            case 0x06:
                /* WSC_FRAG_ACK */
                sme_trace_info((TR_SECURITY_LIB, "WSC_FRAG_ACK" ));
                break;
            default:
                sme_trace_info((TR_SECURITY_LIB, "Unknown WSC message %02x", buffer[23] ));
                break;
        }
        break;
        default:
            sme_trace_info((TR_SECURITY_LIB, "Unknown EAP type %02x", buffer[16] ));
            break;
    }

    return event;
}

static void print_pkt_type(CsrUint8 *buffer)
{
    sme_trace_entry((TR_SECURITY_LIB, "Entering print_pkt_type"));

    switch( buffer[9] )
    {
    case 0x00: /* EAP-Packet */
        switch( buffer[12] )
        {
        case 0x01: /* EAP Request */
            sme_trace_info((TR_SECURITY_LIB, "EAP Request" ));
            break;
        case 0x02: /* EAP Response */
            sme_trace_info((TR_SECURITY_LIB, "EAP Response" ));
            break;
        case 0x03: /* EAP Success */
            sme_trace_info((TR_SECURITY_LIB, "EAP Success" ));
            break;
        case 0x04: /* EAP Fail */
            sme_trace_info((TR_SECURITY_LIB, "EAP Fail" ));
            break;
        default:
            sme_trace_info((TR_SECURITY_LIB, "Unknown EAP code %02x", buffer[12] ));
            break;
        }
        break;
        case 0x01: /* EAPOL-Start */
            sme_trace_info((TR_SECURITY_LIB, "EAPOL-Start" ));
            break;
        case 0x02: /* EAPOL-Logoff */
            sme_trace_info((TR_SECURITY_LIB, "EAPOL-Logoff" ));
            break;
        case 0x03: /* EAPOL-Key */
            /* The access point is attempting a four-way handshake */
            sme_trace_info((TR_SECURITY_LIB, "EAPOL-Key" ));
            break;
        case 0x04: /* EAPOL-Encapsulated-ASF-Alert */
            sme_trace_info((TR_SECURITY_LIB, "EAPOL-Encapsulated-ASF-Alert" ));
            break;
        default:
            sme_trace_info((TR_SECURITY_LIB, "Unknown 802.1x packet type 0x%02x", buffer[9] ));
            break;
    }
}

static void processM2(CsrWpsContext *pCtx)
{
    /* pCtx->result is propogated from the lower functions onto here */
    pCtx->result = TRUE;
    parseM2(pCtx);
    if( pCtx->result == TRUE)
    {
        freeBuffer(&pCtx->out);
        sendM3(pCtx);
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "WPS M2 parse failed"));
        /* TODO: result = WPS_RESULT_M2_ERROR; */
        sendNack(pCtx, DECRYPTION_CRC_FAILURE);
    }
}

static void processM4(CsrWpsContext *pCtx)
{
    /* pCtx->result is propogated from the lower functions onto here */
    pCtx->result = TRUE;
    parseM4(pCtx);
    if(pCtx->result == TRUE)
    {
        freeBuffer(&pCtx->out);
        sendM5(pCtx);
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "WPS M4 parse failed"));
        /* result = WPS_RESULT_M4_ERROR; */
        sendNack(pCtx, DECRYPTION_CRC_FAILURE);
    }
}

static void processM6(CsrWpsContext *pCtx)
{
    /* pCtx->result is propogated from the lower functions onto here */
    pCtx->result = TRUE;
    parseM6(pCtx);
    if(pCtx->result == TRUE)
    {
        freeBuffer(&pCtx->out);
        sendM7(pCtx);
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "WPS M6 parse failed"));
        sendNack(pCtx, DECRYPTION_CRC_FAILURE);
        /* result = WPS_RESULT_M6_ERROR; */
    }
}

static void processM8(CsrWpsContext *pCtx)
{
    /* pCtx->result is propogated from the lower functions onto here */
    pCtx->result = TRUE;
    parseM8(pCtx);
    if(pCtx->result == TRUE)
    {
        freeBuffer(&pCtx->out);
        sendDone(pCtx);
        /* result = WPS_RESULT_SUCCESS; */
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "WPS M8 parse failed"));
        /* result = WPS_RESULT_M8_ERROR; */
        sendNack(pCtx, DECRYPTION_CRC_FAILURE);
    }
}

void HandleEvent(CsrUint8 eapId, CsrInt32 event, CsrWpsContext *pCtx)
{
    /* WpsEventResult result = WPS_RESULT_VOID; */

    pCtx->eapId = eapId;

    switch(event)
    {
        case EV_EAP_IDENTITY_REQUEST:
            sme_trace_info((TR_SECURITY_LIB, "Event: EV_EAP_IDENTITY_REQUEST" ));
            SendIdentityResponse(eapId, pCtx->securityContext->setupData.bssid,
                                 pCtx->securityContext->setupData.localMacAddress, pCtx);
            pCtx->state = WPS_STATE_WAIT_START;
            CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            break;
        case EV_WSC_START:
            sme_trace_info((TR_SECURITY_LIB, "Event: EV_WSC_START" ));
            if( WPS_STATE_WAIT_START == pCtx->state )
            {
                freeBuffer(&pCtx->out);
                sendM1( pCtx );
            }
            else
            {
                sme_trace_info((TR_SECURITY_LIB, "WSC_START ignored in state %d", pCtx->state ));
            }
            CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            break;

        case EV_WSC_NACK:
            sme_trace_info((TR_SECURITY_LIB, "Event: EV_WSC_NACK" ));
            CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            /* result = WPS_RESULT_NACK; */
            break;

        case EV_M2:
            sme_trace_info((TR_SECURITY_LIB, "Event: EV_M2" ));
            if( WPS_STATE_WAIT_M2 == pCtx->state )
            {
                processM2(pCtx);
            }
            else
            {
                sme_trace_info((TR_SECURITY_LIB, "M2 ignored in state %d", pCtx->state ));
                CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            }
            break;

        case EV_M2D:
            if( WPS_STATE_WAIT_M2 == pCtx->state )
            {
                sme_trace_info((TR_SECURITY_LIB, "Event: EV_M2D" ));
                freeBuffer( &pCtx->out );
                sendAck( pCtx );
            }
            else
            {
                sme_trace_info((TR_SECURITY_LIB, "M2D ignored in state %d", pCtx->state ));
            }
            CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            /* result = WPS_RESULT_M2D; */
            break;

        case EV_M4:
            if( WPS_STATE_WAIT_M4 == pCtx->state )
            {
                sme_trace_info((TR_SECURITY_LIB, "Event: EV_M4" ));
                processM4(pCtx);
            }
            else
            {
                sme_trace_info((TR_SECURITY_LIB, "M4 ignored in state %d", pCtx->state ));
                CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            }
            break;

        case EV_M6:
            if( WPS_STATE_WAIT_M6 ==  pCtx->state )
            {
                sme_trace_info((TR_SECURITY_LIB, "Event: EV_M6" ));
                processM6(pCtx);
            }
            else
            {
                sme_trace_info((TR_SECURITY_LIB, "M6 ignored in state %d", pCtx->state ));
                CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            }
            break;

        case EV_M8:
            if( WPS_STATE_WAIT_M8 == pCtx->state )
            {
                 sme_trace_info((TR_SECURITY_LIB, "Event: EV_M8" ));
                 processM8(pCtx);
            }
            else
            {
                sme_trace_info((TR_SECURITY_LIB, "M8 ignored in state %d", pCtx->state ));
                CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            }
            break;

        case EV_EAP_FAIL:
            sme_trace_info((TR_SECURITY_LIB, "Event: EV_EAP_FAIL" ));
            CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            /* result = WPS_RESULT_EAP_FAIL; */
            break;

        default:
            sme_trace_info((TR_SECURITY_LIB, "Unhandled event %d", event ));
            CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            break;
    }
}

void csr_wps_pkt_input(CsrWpsContext *pCtx, CsrUint8 *buffer, CsrUint16 length)
{
    WpsEvent event = EV_IGNORE;
    CsrUint8 eapId;

    sme_trace_entry((TR_SECURITY_LIB, "Entering csr_wps_pkt_input"));

    print_pkt_type(buffer);

    if (buffer[9] == 0) /* EAP-Packet */
    {
        eapId = buffer[13];

        if (buffer[12] == 1) /* EAP Request */
        {
            event = eapRequestEvent(buffer, length, pCtx);
        }
        else
        if (buffer[12] == 4) /* EAP Fail */
        {
            event = EV_EAP_FAIL;
        }

        HandleEvent(eapId, event, pCtx);
        /* !!!WARNING ATTENTION ACHTUNG!!! No statements beyond this point */
    }
    else
    {
        sme_trace_error((TR_SECURITY_LIB, "Non EAP packet inside WPS, ignoring"));
        CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
    }
}

void csr_wps_timeout(CsrWpsContext *pCtx, CsrWifiSecurityTimeout timeoutIndex)
{
    sme_trace_entry((TR_SECURITY_LIB, "Entering csr_wps_timeout"));

    if (timeoutIndex == CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY)
    {
        if( pCtx->txCount++ < 2 )
        {
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Retransmitting last message", pCtx->lastTx.pStart, pCtx->lastTx.size ));
            pCtx->securityContext->callbacks.sendPacket(pCtx->securityContext->externalContext,
                                                        pCtx->lastTx.pStart,
                                                        pCtx->lastTx.size,
                                                        pCtx->securityContext->setupData.localMacAddress,
                                                        pCtx->securityContext->setupData.peerMacAddress);
            /* start timer */
            pCtx->securityContext->callbacks.startTimer(pCtx->securityContext->externalContext,
                                                        pCtx->securityContext->setupData.responseTimeout,
                                                        CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);
        }
        else
        {
            sme_trace_info((TR_SECURITY_LIB, "Retransmission limit reached" ));
            pCtx->securityContext->callbacks.abortProcedure(pCtx->securityContext->externalContext);
        }
    }
}

void csr_wps_start(CsrWpsContext *context)
{
    sme_trace_entry((TR_SECURITY_LIB, "Entering csr_wps_start"));

    SendEapolStart(context->securityContext->setupData.peerMacAddress,
                   context->securityContext->setupData.localMacAddress, context);
}


CsrWpsContext* csr_wps_init(CsrWifiSecurityContext* context)
{
    CsrWpsContext *pCtx;

    sme_trace_entry((TR_SECURITY_LIB, "Entering csr_wps_init"));

    sme_trace_info((TR_SECURITY_LIB, "CsrWpsContext = %d bytes", sizeof(CsrWpsContext)));

    pCtx =  (CsrWpsContext*) CsrPmalloc(sizeof(CsrWpsContext));
    CsrMemSet(pCtx, 0, sizeof(CsrWpsContext));
    pCtx->securityContext = context;

    sme_trace_info((TR_SECURITY_LIB, "psendPacket = %p", pCtx->securityContext->callbacks.sendPacket));
    sme_trace_info((TR_SECURITY_LIB, "pInstallPairwiseKey = %p", pCtx->securityContext->callbacks.installPairwiseKey));
    sme_trace_info((TR_SECURITY_LIB, "pInstallGroupKey = %p", pCtx->securityContext->callbacks.installGroupKey));
    sme_trace_info((TR_SECURITY_LIB, "pWpsDone = %p", pCtx->securityContext->callbacks.wpsDone));
    sme_trace_info((TR_SECURITY_LIB, "pStartTimer = %p", pCtx->securityContext->callbacks.startTimer));
    sme_trace_info((TR_SECURITY_LIB, "pStopTimer = %p", pCtx->securityContext->callbacks.stopTimer));
    sme_trace_info((TR_SECURITY_LIB, "pAbortProcedure = %p", pCtx->securityContext->callbacks.abortProcedure));

    return pCtx;
}

void csr_wps_deinit(CsrWpsContext *pCtx)
{
    sme_trace_entry((TR_SECURITY_LIB, "csr_wps_deinit"));

    freeBuffer( &pCtx->out );
    freeBuffer( &pCtx->in );
    freeBuffer( &pCtx->lastTx );
    CsrPfree(pCtx);
}
