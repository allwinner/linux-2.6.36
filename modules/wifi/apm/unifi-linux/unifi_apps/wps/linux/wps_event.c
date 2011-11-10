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
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_event.c#1 $
 *
 ****************************************************************************/
#include <stdlib.h>
#include "wps_event.h"
#include "wps_types.h"
#include "wps_common.h"
#include "wps_debug.h"
#include "wps_eap.h"
#include "unifiio.h"
#include "sigs.h"

unsigned char wps_eap_id[] = { 0x00, 0x37, 0x2a, 0x00, 0x00, 0x00, 0x01 };
unsigned char buffer[10000];

WpsEvent eapRequestEvent( unsigned char *pBuffer, WpsContext *pCtx )
{
WpsEvent event = EV_IGNORE;
unsigned char messageType;
WpsBuffer value;

unsigned char *pSignal;
int signal_and_data;
unsigned char *pData;
int length;
int signal_length;

    /* We get a UDI message header, and the signal is appended right after the structure. */
    pSignal = pBuffer;

    length = *pSignal++;
    length |= *pSignal++ << 8;
    length |= *pSignal++ << 16;
    length |= *pSignal++ << 24;

    pSignal += 4; /* Ignore timestamp */
    pSignal += 4; /* Ignore direction */

    signal_length = *pSignal++;
    signal_length |= *pSignal++ << 8;
    signal_length |= *pSignal++ << 16;
    signal_length |= *pSignal++ << 24;

    /* Total length of signal body + any bulk data */
    signal_and_data = length - ( pSignal - pBuffer );

    /* Total length of signal body + any bulk data */
    signal_and_data = length - ( pSignal - pBuffer );
    pData = pSignal + signal_length;

    switch( pData[16] )
    {
    case 0x01:
        /* EAP Identity */
        event = EV_EAP_IDENTITY_REQUEST;
        break;
    case 0xfe:
       /* Expanded EAP type */
       if( memcmp( pData + 17, wps_eap_id, sizeof (wps_eap_id ) ) ) break;
       switch( pData[24] )
       {
       case 0x01:
          /* WSC_Start */
          event = EV_WSC_START;
          break;
       case 0x02:
          /* WSC_ACK */
          WPS_DBG_PRINT(( "WSC_ACK" ));
          break;
       case 0x03:
          /* WSC_NACK */
          WPS_DBG_PRINT(( "WSC_NACK" ));
          event = EV_WSC_NACK;
          break;
       case 0x04:
          /* WSC_MSG */
          freeBuffer( &pCtx->in );
          initBuffer( &pCtx->in, (CsrUint8 *)malloc( signal_and_data - signal_length - 26 ), signal_and_data - signal_length - 26 );
          memcpy( pCtx->in.pStart, pSignal + signal_length + 26, pCtx->in.size );
          switch( messageType = getMessageType( &pCtx->in ) )
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
              WPS_DBG_PRINT(( "WPS Unsupported message type (0x%02x)", messageType ));
              break;
          }
          break;
       case 0x05:
          /* WSC_Done */
          WPS_DBG_PRINT(( "WSC_Done" ));
          break;
       case 0x06:
          /* WSC_FRAG_ACK */
          WPS_DBG_PRINT(( "WSC_FRAG_ACK" ));
          break;
       default:
          WPS_DBG_PRINT(( "Unknown WSC message %02x", pData[23] ));
          break;
       }
       break;
    default:
        WPS_DBG_PRINT(( "Unknown EAP type %02x", pData[16] ));
        break;
    }

    return event;
}

WpsEvent getEvent( int fd, WpsContext *pCtx, int *pEapId )
{
fd_set readfds;
struct timeval tv;
int r;
int signal_and_data;
WpsEvent event = EV_IGNORE;
unsigned char *pSignal, *pSignalIndex;
int length;
int direction;
int signal_length;
unsigned int signalId;
unsigned char *pData;

    *pEapId = 0;

    FD_ZERO( &readfds );
    FD_SET( fd, &readfds );

    /* Set up 1 second timeout on select */
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    /* This will poll in the char device for an event. */
    r = select( fd+1, &readfds, NULL, NULL, &tv );
    if( r < 0 )
    {
        WPS_DBG_PRINT(( "Error: select = %d (%s)", r, strerror( errno ) ));
    }

    if( r == 0 )
    {
        return EV_TIMEOUT;
    }

    /* This read will obtain the indication. */
    r = read( fd, buffer, 8192 );
    if( r < 0 )
    {
        WPS_DBG_PRINT(( "Error: read = %d (%s)", r, strerror( errno ) ));
    }

    /* We get a UDI message header, and the signal is appended right after the structure. */
    pSignal = buffer;

    length = *pSignal++;
    length |= *pSignal++ << 8;
    length |= *pSignal++ << 16;
    length |= *pSignal++ << 24;

    pSignal += 4; /* Ignore timestamp */

    direction = *pSignal++;
    direction |= *pSignal++ << 8;
    direction |= *pSignal++ << 16;
    direction |= *pSignal++ << 24;

    signal_length = *pSignal++;
    signal_length |= *pSignal++ << 8;
    signal_length |= *pSignal++ << 16;
    signal_length |= *pSignal++ << 24;

    /* Total length of signal body + any bulk data */
    signal_and_data = length - ( pSignal - buffer );

    /*
     * Check if the indication is actual a config indication,
     * rather than a signal..
     */
    if( direction == UDI_CONFIG_IND )
    {
        /* .. in this case the information is just one byte. */
        WPS_DBG_PRINT(( "UDI_CONFIG_IND received" ));
    }

    signalId = *pSignal++;
    signalId |= *pSignal++ << 8;

    pSignal += 2; /* Ignore ReceiverProcessId */
    pSignal += 2; /* Ignore SenderProcessId */

    switch( signalId )
    {
    case CSR_MA_UNITDATA_INDICATION_ID:
        pSignal += 4 + 4 + 6 + 6 + 2 + 2 + 2 + 2; /* Ignore CSR_DATAREF, CSR_DATAREF, CSR_MACADDRESS, CSR_MACADDRESS, CSR_ROUTING_INFORMATION,
                                                     CSR_RECEPTION_STATUS, CSR_PRIORITY and CSR_SERVICE_CLASS */
        pData = pSignal;

        WPS_DBG_PRINTHEX(( "CSR_MA_UNITDATA_INDICATION", pData, signal_and_data - signal_length ));
        /* Only process EAP over LAN (IEEE 802.1X) frames */
        if( pData[12] == 0x88 && pData[13] == 0x8e )
        {
            switch( pData[15] )
            {
            case 0x00: /* EAP-Packet */
                *pEapId = pData[19];
                switch( pData[18] )
                {
                case 0x01: /* EAP Request */
                    event = eapRequestEvent( buffer, pCtx );
                    break;
                case 0x02: /* EAP Response */
                    WPS_DBG_PRINT(( "EAP Response" ));
                    break;
                case 0x03: /* EAP Success */
                    WPS_DBG_PRINT(( "EAP Success" ));
                    break;
                case 0x04: /* EAP Fail */
                    WPS_DBG_PRINT(( "EAP Fail" ));
                    event = EV_EAP_FAIL;
                    break;
                default:
                    WPS_DBG_PRINT(( "Unknown EAP code %02x", pData[18] ));
                    break;
                }
                break;
            case 0x01: /* EAPOL-Start */
                WPS_DBG_PRINT(( "EAPOL-Start" ));
                break;
            case 0x02: /* EAPOL-Logoff */
                WPS_DBG_PRINT(( "EAPOL-Logoff" ));
                break;
            case 0x03: /* EAPOL-Key */
                /* The access point is attempting a four-way handshake */
                WPS_DBG_PRINT(( "EAPOL-Key" ));
                break;
            case 0x04: /* EAPOL-Encapsulated-ASF-Alert */
                WPS_DBG_PRINT(( "EAPOL-Encapsulated-ASF-Alert" ));
                break;
            default:
                WPS_DBG_PRINT(( "Unknown 802.1x packet type 0x%02x", pData[15] ));
                break;
            }
        }
        else
        {
        	WPS_DBG_PRINT(( "Discarded EtherType = 0x%02x%02x", pData[12], pData[13] ));
        }
        break;
    default:
        break;
    }

    return event;
}

int HandleEvent( int fd, int eapId, int event, WpsState *pState, WpsContext *pCtx, unsigned char *supp_mac )
{
WpsEventResult result = WPS_RESULT_VOID;

    switch( event )
    {
    case EV_TIMEOUT:
        WPS_DBG_PRINT(( "Event: EV_TIMEOUT" ));
        if( pCtx->txCount++ < 2 )
        {
            WPS_DBG_PRINTHEX(( "Retransmitting last message", pCtx->lastTx.pStart, pCtx->lastTx.size ));
            SendEapolRequest( fd, pCtx->pCandidate->bssid, supp_mac, pCtx->lastTx.pStart, pCtx->lastTx.size );
        }
        else
        {
            WPS_DBG_PRINT(( "Retransmission limit reached" ));
            result = WPS_RESULT_TIMEOUT;
        }
        break;
    case EV_EAP_IDENTITY_REQUEST:
        WPS_DBG_PRINT(( "Event: EV_EAP_IDENTITY_REQUEST" ));
        SendIdentityResponse( fd, eapId, pCtx->pCandidate->bssid, supp_mac, pCtx );
        *pState = WPS_STATE_WAIT_START;
        break;
    case EV_WSC_START:
        WPS_DBG_PRINT(( "Event: EV_WSC_START" ));
        if( WPS_STATE_WAIT_START == *pState )
        {
            getMacAddress( pCtx->macAddress );
            freeBuffer( &pCtx->out );
            encodeM1( pCtx );
            WPS_DBG_PRINT(( "Sending M1" ));
            SendResponse( fd, eapId, WPS_OPCODE_MSG, pCtx->pCandidate->bssid, supp_mac, pCtx );
            *pState = WPS_STATE_WAIT_M2;
        }
        else
        {
        	WPS_DBG_PRINT(( "WSC_START ignored in state %d", *pState ));
        }
        break;
    case EV_WSC_NACK:
        WPS_DBG_PRINT(( "Event: EV_WSC_NACK" ));
        result = WPS_RESULT_NACK;
        break;
    case EV_M2:
        WPS_DBG_PRINT(( "Event: EV_M2" ));
        if( WPS_STATE_WAIT_M2 == *pState )
        {
            if( parseM2( pCtx ) )
            {
                freeBuffer( &pCtx->out );
                encodeM3( pCtx );
                WPS_DBG_PRINT(( "Sending M3" ));
                SendResponse( fd, eapId, WPS_OPCODE_MSG, pCtx->pCandidate->bssid, supp_mac, pCtx );
                *pState = WPS_STATE_WAIT_M4;
            }
            else
            {
                WPS_DBG_PRINT(( "WPS M2 parse failed" ));
                encodeNack( pCtx, DECRYPTION_CRC_FAILURE );
                WPS_DBG_PRINT(( "Sending Nack" ));
                SendResponse( fd, eapId, WPS_OPCODE_NACK, pCtx->pCandidate->bssid, supp_mac, pCtx );
                result = WPS_RESULT_M2_ERROR;
            }
        }
        else
        {
        	WPS_DBG_PRINT(( "M2 ignored in state %d", *pState ));
        }
        break;
    case EV_M2D:
    	if( WPS_STATE_WAIT_M2 == *pState )
    	{
            WPS_DBG_PRINT(( "Event: EV_M2D" ));
            encodeAck( pCtx );
            WPS_DBG_PRINT(( "Sending Ack" ));
            SendResponse( fd, eapId, WPS_OPCODE_ACK, pCtx->pCandidate->bssid, supp_mac, pCtx );
    	}
    	else
    	{
    		WPS_DBG_PRINT(( "M2D ignored in state %d", *pState ));
    	}
        result = WPS_RESULT_M2D;
        break;
    case EV_M4:
    	if( WPS_STATE_WAIT_M4 == *pState )
    	{
            WPS_DBG_PRINT(( "Event: EV_M4" ));
            if( parseM4( pCtx ) )
            {
                freeBuffer( &pCtx->out );
                encodeM5( pCtx );
                WPS_DBG_PRINT(( "Sending M5" ));
                SendResponse( fd, eapId, WPS_OPCODE_MSG, pCtx->pCandidate->bssid, supp_mac, pCtx );
                *pState = WPS_STATE_WAIT_M6;
            }
            else
            {
                WPS_DBG_PRINT(( "WPS M4 parse failed" ));
                encodeNack( pCtx, DECRYPTION_CRC_FAILURE );
                WPS_DBG_PRINT(( "Sending Nack" ));
                SendResponse( fd, eapId, WPS_OPCODE_NACK, pCtx->pCandidate->bssid, supp_mac, pCtx );
                result = WPS_RESULT_M4_ERROR;
            }
    	}
    	else
    	{
    		WPS_DBG_PRINT(( "M4 ignored in state %d", *pState ));
    	}
        break;
    case EV_M6:
        if( WPS_STATE_WAIT_M6 ==  *pState )
        {
            WPS_DBG_PRINT(( "Event: EV_M6" ));
            if( parseM6( pCtx ) )
            {
                freeBuffer( &pCtx->out );
                encodeM7( pCtx );
                WPS_DBG_PRINT(( "Sending M7" ));
                SendResponse( fd, eapId, WPS_OPCODE_MSG, pCtx->pCandidate->bssid, supp_mac, pCtx );
                *pState = WPS_STATE_WAIT_M8;
            }
            else
            {
                WPS_DBG_PRINT(( "WPS M6 parse failed" ));
                encodeNack( pCtx, DECRYPTION_CRC_FAILURE );
                WPS_DBG_PRINT(( "Sending Nack" ));
                SendResponse( fd, eapId, WPS_OPCODE_NACK, pCtx->pCandidate->bssid, supp_mac, pCtx );
                result = WPS_RESULT_M6_ERROR;
            }
        }
        else
        {
        	WPS_DBG_PRINT(( "M6 ignored in state %d", *pState ));
        }
        break;
    case EV_M8:
        if( WPS_STATE_WAIT_M8 == *pState  )
        {
            WPS_DBG_PRINT(( "Event: EV_M8" ));
            if( parseM8( pCtx ) )
            {
                freeBuffer( &pCtx->out );
                encodeDone( pCtx );
                WPS_DBG_PRINT(( "Sending Done" ));
                SendResponse( fd, eapId, WPS_OPCODE_DONE, pCtx->pCandidate->bssid, supp_mac, pCtx );
                result = WPS_RESULT_SUCCESS;
            }
            else
            {
                WPS_DBG_PRINT(( "WPS M8 parse failed" ));
                result = WPS_RESULT_M8_ERROR;
                encodeNack( pCtx, DECRYPTION_CRC_FAILURE );
                WPS_DBG_PRINT(( "Sending Nack" ));
                SendResponse( fd, eapId, WPS_OPCODE_NACK, pCtx->pCandidate->bssid, supp_mac, pCtx );
            }
        }
        else
        {
        	WPS_DBG_PRINT(( "M8 ignored in state %d", *pState ));
        }
        break;
    case EV_EAP_FAIL:
        WPS_DBG_PRINT(( "Event: EV_EAP_FAIL" ));
        result = WPS_RESULT_EAP_FAIL;
        break;
    default:
        if( event ) WPS_DBG_PRINT(( "Unhandled event %d", event ));
        break;
    }

    return result;
}
