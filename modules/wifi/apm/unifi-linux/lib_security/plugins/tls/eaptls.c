/** @file eaptls.c
 *
 * Implementation of the EAP-TLS method.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   This provides an implementation of the EAP-TLS authentication protocol.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/tls/eaptls.c#1 $
 *
 ****************************************************************************/

#include "eaptls.h"
#include "eaptls_utils.h"
#include "sme_trace/sme_trace.h"
#include "csr_tls.h"
#include "csr_crypto.h"
#include "csr_security_private.h"
#include "csr_des/csr_des.h"

/* PRIVATE MACROS ***********************************************************/

void process_eaptls_server_start(TlsContext* tlsContext)
{
    /* Assemble the random challenge to put in the client hello. */
    tlsContext->tls.client_random.gmt_unix_time = (CsrUint32)(CsrGetTime()/1000); /* endian issue? */

    CsrWifiSecurityRandom(tlsContext->tls.client_random.data, 28);
}

/*
 * Description:
 * See description in tls.h
 */
/*---------------------------------------------------------------------------*/
/*lint -save -e415 -e416 -e419 -e661 -e662 */
void build_eaptls_client_hello(TlsContext* tlsContext, CsrUint8 reqId)
{
    CsrUint8 *fillout;
    CsrUint8 *handshake_length, *tls_length, *message_body_start;
    CsrUint16 length;
    CsrUint16 ciphersuite;
    CsrUint8 *for_hash;
    eapol_packet *eapol = (eapol_packet *)tlsContext->context->buffer;

    sme_trace_entry((TR_SECURITY_LIB, "build_eaptls_client_hello()."));

    tlsContext->dataref.dataLength = 0;
    tlsContext->dataref.buf = (CsrUint8 *)eapol;

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */
    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    length = 0;
    EAPOL_LENGTH_ASSIGN(eapol, length);
    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* EAP-FAST type and flags fields */
    eapol->u.eap.u.resp.type = tlsContext->eapType;
    ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->flags = 0;

    /* EAP-FAST TLS message length field */
    fillout = ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.data;

    /* Now fill out TLS record protocol TLSPlaintext message.                */
    /* --------------------------------------------------------------------- */

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 22; /* handshake */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    tls_length = fillout; /* Remember TLS length location to fill in later */
    *fillout++ = 0;       /* Set the TLS length to zero for now */
    *fillout++ = 0;

    /* Handshake message begins here */
    for_hash = fillout;

    /* Handshake type */
    *fillout++ = 1; /* client hello */

    /* Handshake length */
    handshake_length = fillout; /* Remember handshake length location to fill in later */
    *fillout++ = 0;             /* Set the handshake length to zero for now */
    *fillout++ = 0;
    *fillout++ = 0;

    /* Client Hello message begins here */
    message_body_start = fillout; /* Remember start of message body - this is the start of handshake length */

    /* Protocol Version field */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;

    /* Random field */
    CsrMemCpy(fillout, &tlsContext->tls.client_random, sizeof(Random));
    fillout += sizeof(Random);

    /* Session ID field - vector, length field = 8 bits */
    if(tlsContext->tls.session_id_length)
    {
        sme_trace_info((TR_SECURITY_LIB, "build_eaptls_client_hello() :: Adding session_id"));
        *fillout++ = tlsContext->tls.session_id_length & 0xff;
        CsrMemCpy(fillout, tlsContext->tls.session_id, tlsContext->tls.session_id_length);
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "build_eaptls_client_hello() :: session id", fillout, tlsContext->tls.session_id_length));
        fillout += tlsContext->tls.session_id_length;
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "build_eaptls_client_hello() :: No session_id"));
        *fillout++ = 0;
    }

    /* Cipher Suites field - vector, length field = 16 bits  */
    *fillout++ = 0;
    *fillout++ = 4;
    ciphersuite = TLS_RSA_WITH_3DES_EDE_CBC_SHA;
    *fillout++ = (ciphersuite >> 8) & 0xff;
    *fillout++ = ciphersuite & 0xff;
    ciphersuite = TLS_RSA_WITH_AES_128_CBC_SHA;
    *fillout++ = (ciphersuite >> 8) & 0xff;
    *fillout++ = ciphersuite & 0xff;

    /* Compression method field - vector, length field = 8 bits */
    *fillout++ = 1;
    *fillout++ = 0; /* NULL compression only */

    /* Work out length of handshake message and update the handshake message length */
    length = (CsrUint16)(fillout - message_body_start);
    *handshake_length++ = 0;
    *handshake_length++ = (length >> 8) & 0xff;
    *handshake_length = length & 0xff;

    /* Work out length of TLS message and update the TLS message length */
    length = (CsrUint16)(fillout - for_hash);
    *tls_length++ = (length >> 8) & 0xff;
    *tls_length = length & 0xff;

    /* Add this TLS handshake message to hashes of all handshake messages */
    CsrCryptoCallMd5Update(tlsContext->context->cryptoContext, NULL,
                           tlsContext->tls.md5_ctx, for_hash, length);
    CsrCryptoCallSha1Update(tlsContext->context->cryptoContext, NULL,
                            tlsContext->tls.sha1_ctx, for_hash, length);

    /* Work out the EAPOL data lengths and add them to the EAPOL header */
    length = (CsrUint16)(fillout - ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.data);
    length += EAP_REQ_RESP_HEADER_LENGTH + EAPTLS_HEADER_LENGTH;
       EAPOL_LENGTH_ASSIGN(eapol, length);
       eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
       eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* Work out the DataRef data length */
    tlsContext->dataref.dataLength = length + CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH;
}
/*lint -restore */

/*
 * Description:
 * See description in tls.h
 */
/*---------------------------------------------------------------------------*/
/*lint -save -e415 -e416 */
void build_eaptls_client_finished(TlsContext* tlsContext, CsrUint8 reqId)
{
    CsrUint8 *fillout;
    CsrUint8 *message_length, *message_body_start;
    eapol_packet *eapol = (eapol_packet *)tlsContext->context->buffer;
    CsrUint32 length;

    sme_trace_entry((TR_SECURITY_LIB, "build_eaptls_client_finished()"));

    /* TLS packet may be fragmented */
    fillout = ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.data;

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x14; /* Change Cipher Spec */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = 0x00; /* two octet length: 1 */
    *fillout++ = 0x01;
    *fillout++ = 0x01; /* Change Cipher Spec message */

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x16; /* handshake */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    message_length = fillout; /* Remember message length location to fill in later */
    *fillout++ = 0x00;        /* Use zero as message length until it can be worked out */
    *fillout++ = 0x00;
    message_body_start = fillout; /* Remember start of message body - this will be where the length starts */

    tls_key_expansion(&tlsContext->tls);

    /* Add an encrypted Finished handshake message */
    fillout = TlsAddFinished(fillout, &tlsContext->tls);

    /* Now the message length can be added */
    length = (CsrUint16)(fillout - message_body_start);
    *message_length++ = (length >> 8) & 0xff;
    *message_length = length & 0xff;

    /* Work out the EAPOL data lengths and add them to the EAPOL header */
    length = (CsrUint16)(fillout - ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.data);


    /* Determine if the packet has to be fragmented, if so, then there needs to be a TLS length
     * field in the first packet */

    if ((length + EAP_REQ_RESP_HEADER_LENGTH + EAPTLS_HEADER_LENGTH + EAPTLS_MESSAGE_LENGTH_LENGTH)
            > CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE)
    {
        /* TLS message length field */
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.length[0] = (length & 0xFF000000) >> 24;
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.length[1] = (length & 0xFF0000) >> 16;
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.length[2] = (length & 0xFF00) >> 8;
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.length[3] = length & 0xFF;
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->flags = TLS_LENGTHBIT;
        length += EAPTLS_MESSAGE_LENGTH_LENGTH;
    }
    else
    {
        /* Everything fits into a single fragment, shift the start of the packet to exclude message length field */
        eapol = (eapol_packet*)((CsrUint8*)tlsContext->context->buffer + 4);
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->flags = 0;
    }

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */
    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;

    /* Type and flags fields */
    eapol->u.eap.u.resp.type = tlsContext->eapType;

    length += EAP_REQ_RESP_HEADER_LENGTH + EAPTLS_HEADER_LENGTH;
    EAPOL_LENGTH_ASSIGN(eapol, length);
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* Work out the DataRef data length */
    tlsContext->dataref.dataLength = (CsrUint16)(length + CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH);
    tlsContext->dataref.buf = (CsrUint8 *)eapol;
}
/*lint -restore */

/*
 * Description:
 * See description in tls.h
 */
/*---------------------------------------------------------------------------*/
/*lint -save -e415 -e416 */
void build_eaptls_client_key_exchange(TlsContext* tlsContext, CsrUint8 reqId)
{
    CsrUint8 *fillout;
    CsrUint8 *message_length, *message_body_start;
    eapol_packet *eapol = (eapol_packet *)tlsContext->context->buffer;
    CsrUint8 pre_master_secret[48];
    CsrUint32 modulusLength, length;

    sme_trace_entry((TR_SECURITY_LIB, "build_eaptls_client_key_exchange()"));

    /* Round down server modulus length to a multiple of 8 bytes */
    modulusLength = (tlsContext->tls.server_modulus_length / 8) * 8;
    sme_trace_info((TR_SECURITY_LIB, "build_eaptls_client_key_exchange() :: modulusLength = %d", modulusLength));

    /* TLS packet may be fragmented */
    fillout = ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.data;

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x16; /* handshake */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    message_length = fillout; /* Remember message length location to fill in later */
    *fillout++ = 0;           /* Use zero as message length until it can be worked out */
    *fillout++ = 0;
    message_body_start = fillout; /* Remember start of message body - this will be where the length starts */

    /* No client certificate in case of TTLS and PEAP for now. TODO: Can include certificate if provided */
    if ((tlsContext->eapType != EAP_TYPE_TTLS) && (tlsContext->eapType != EAP_TYPE_PEAP))
    {
        fillout = TlsAddClientCertificate(fillout, tlsContext->context->setupData.clientCertificate, tlsContext->context->setupData.clientCertificateLength, &tlsContext->tls);
    }

    TlsBuildPreMasterSecret(pre_master_secret);

    fillout = TlsAddClientKeyExchange(fillout, modulusLength, pre_master_secret, &tlsContext->tls);

    /* TODO: Can include certificate if provided */
    if ((tlsContext->eapType != EAP_TYPE_TTLS) && (tlsContext->eapType != EAP_TYPE_PEAP))
    {
        fillout = TlsAddCertificateVerify(fillout, modulusLength, tlsContext->context->setupData.clientPrivateKey, &tlsContext->tls);
    }

    /* Now the message length can be added */
    length = (CsrUint16)(fillout - message_body_start);
    *message_length++ = (length >> 8) & 0xff;
    *message_length = length & 0xff;

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x14; /* Change Cipher Spec */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = 0x00; /* two octet length: 1 */
    *fillout++ = 0x01;
    *fillout++ = 0x01; /* Change Cipher Spec message */

    /* TLS record protocol TLSPlaintext header */
    *fillout++ = 0x16; /* handshake */
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    message_length = fillout; /* Remember message length location to fill in later */
    *fillout++ = 0x00;        /* Use zero as message length until it can be worked out */
    *fillout++ = 0x00;
    message_body_start = fillout; /* Remember start of message body - this will be where the length starts */

    /* The rest of the message has to be encrypted - time to derive the master secret */
    TlsDeriveMasterSecret(pre_master_secret, &tlsContext->tls);

    tls_key_expansion(&tlsContext->tls);

    /* Add an encrypted Finished handshake message */
    fillout = TlsAddFinished(fillout, &tlsContext->tls);

    /* Now the message length can be added */
    length = (CsrUint16)(fillout - message_body_start);
    *message_length++ = (length >> 8) & 0xff;
    *message_length = length & 0xff;

    /* Work out the EAPOL data lengths and add them to the EAPOL header */
    length = (CsrUint16)(fillout - ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.data);


    /* Determine if the packet has to be fragmented, if so, then there needs to be a TLS length
     * field in the first packet */

    if ((length + EAP_REQ_RESP_HEADER_LENGTH + EAPTLS_HEADER_LENGTH + EAPTLS_MESSAGE_LENGTH_LENGTH)
            > CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE)
    {
        /* TLS message length field */
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.length[0] = (length & 0xFF000000) >> 24;
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.length[1] = (length & 0xFF0000) >> 16;
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.length[2] = (length & 0xFF00) >> 8;
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->u.lengthed.length[3] = length & 0xFF;
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->flags = TLS_LENGTHBIT;
        length += EAPTLS_MESSAGE_LENGTH_LENGTH;
    }
    else
    {
        /* Everything fits into a single fragment, shift the start of the packet to exclude message length field */
        eapol = (eapol_packet*)((CsrUint8*)tlsContext->context->buffer + 4);
        ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->flags = 0;
    }

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */
    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;

    /* Type and flags fields */
    eapol->u.eap.u.resp.type = tlsContext->eapType;

    length += EAP_REQ_RESP_HEADER_LENGTH + EAPTLS_HEADER_LENGTH;
    EAPOL_LENGTH_ASSIGN(eapol, length);
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* Work out the DataRef data length */
    tlsContext->dataref.dataLength = (CsrUint16)(length + CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH);
    tlsContext->dataref.buf = (CsrUint8 *)eapol;
}
/*lint -restore */

/*
 * Description:
 * See description in tls.h
 */
/*---------------------------------------------------------------------------*/
void build_eaptls_response(TlsContext* tlsContext, CsrUint8 reqId)
{
    eapol_packet *eapol = (eapol_packet *)tlsContext->context->buffer;

    sme_trace_entry((TR_SECURITY_LIB, "build_eaptls_response()."));

    tlsContext->dataref.dataLength = CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH + EAP_REQ_RESP_HEADER_LENGTH + EAPTLS_HEADER_LENGTH;
    tlsContext->dataref.buf = (CsrUint8 *)eapol;

    /* Fill out the normal EAP (etc.) header stuff */
    /* --------------------------------------------------------------------- */

    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    EAPOL_LENGTH_ASSIGN(eapol,
                        tlsContext->dataref.dataLength - (CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH));

    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    /* EAP-TLS type and flags fields */
    eapol->u.eap.u.resp.type = tlsContext->eapType;
    ((eaptls_packet *)(&eapol->u.eap.u.resp.u.data))->flags = 0;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/


/**
 * @brief Check an incoming EAP meassage from the AP.
 *
 * @par Description
 *   This function checks an incoming EAP message for validity and relevance to
 *   TLS.
 *
 * @param[in]   eapReqData : the EAP message received from the AP.
 * @param[in]   size : the size of the incoming packet. Maybe be larger than the actual EAP message.
 *
 * @return
 *   TRUE - TLS message ok, FALSE - bad or irrelevant message.
 */
static CsrBool TlsCheck(eapMethod* context, eap_packet *eapReqData, CsrUint16 size)
{
    TlsContext* tlsContext = (TlsContext*)context->methodContext;
    eaptls_packet* tls_pkt = (eaptls_packet *)(&eapReqData->u.req.u.data);

    sme_trace_info((TR_SECURITY_LIB, "TlsCheck"));

    switch(tlsContext->state)
    {
    case TLS_AWAIT_START:
        sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: TLS_AWAIT_START"));
        /* Good packet if start bit set and other flags are zero. */
        if ((tls_pkt->flags & TLS_STARTBIT) != TLS_STARTBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "TlsCheck() :: malformed start message."));
            return FALSE;
        }
        sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: EAP-TLS start"));
        tlsContext->tls_message_length = 0;
        break;
    case TLS_AWAIT_SERVER_FINISHED:
    case TLS_AWAIT_SERVER_HELLO:

        if (tlsContext->state == TLS_AWAIT_SERVER_FINISHED)
        {
            sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: TLS_AWAIT_SERVER_FINISHED"));
        }
        else
        {
            sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: TLS_AWAIT_SERVER_HELLO"));
        }
        {
            CsrUint32 tls_data_length;

            tlsContext->tls_pkt_flags = tls_pkt->flags;

            if (tlsContext->tls_pkt_flags & TLS_STARTBIT)
            {
                sme_trace_info((TR_SECURITY_LIB, "TLS Start message unhandled in this state"));
                return FALSE;
            }

            if (tlsContext->tls_pkt_flags & TLS_LENGTHBIT)
            {
                sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: length = %02x %02x %02x %02x", tls_pkt->u.lengthed.length[0],
                        tls_pkt->u.lengthed.length[1], tls_pkt->u.lengthed.length[2], tls_pkt->u.lengthed.length[3]));
                tls_data_length = size - (EAP_REQ_RESP_HEADER_LENGTH + EAPTLS_HEADER_LENGTH
                                                                     + EAPTLS_MESSAGE_LENGTH_LENGTH);
                if(tlsContext->tls_message_length + tls_data_length > TLS_MAX_MESSAGE_LENGTH)
                {
                    sme_trace_info((TR_SECURITY_LIB, "TLS message too big"));
                    return FALSE;
                }
                CsrMemCpy(&tlsContext->tls_message[tlsContext->tls_message_length], tls_pkt->u.lengthed.data, tls_data_length);
            }
            else
            {
                tls_data_length = size - (EAP_REQ_RESP_HEADER_LENGTH + EAPTLS_HEADER_LENGTH);
                if(tlsContext->tls_message_length + tls_data_length > TLS_MAX_MESSAGE_LENGTH)
                {
                    sme_trace_info((TR_SECURITY_LIB, "TLS message too big"));
                    return FALSE;
                }
                CsrMemCpy(&tlsContext->tls_message[tlsContext->tls_message_length], tls_pkt->u.data, tls_data_length);
            }

            sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: EAP-TLS data length = %d", tls_data_length));
            sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: tlsContext->tls_message_length = %d", tlsContext->tls_message_length));


            tlsContext->tls_message_length += tls_data_length;

            if(tls_pkt->flags & TLS_MOREBIT)
            {
                sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: More data"));
            }
            else
            {
                if (!(parse_tls(NULL, tlsContext->tls_message, tlsContext->tls_message_length, &tlsContext->tls,
                        &tlsContext->tls_parse_flags, NULL)))
                {
                    sme_trace_error((TR_SECURITY_LIB, "TlsCheck() :: parse server hello failed."));
                    tlsContext->tls_message_length = 0;
                    return FALSE;
                }
                tlsContext->tls_message_length = 0;

                sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: flags = 0x%04x", tlsContext->tls_parse_flags));
                if(tlsContext->tls_parse_flags & SERVER_DONE)
                {
                    sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: SERVER DONE"));
                }
            }
        }

        break;
    case TLS_AWAIT_SUCCESS:
        sme_trace_info((TR_SECURITY_LIB, "TlsCheck() :: TLS_AWAIT_SUCCESS"));
        break;
    }

    return TRUE;
}

/**
 * @brief Process a received message from the AP.
 *
 * @pre TlsCheck() has been called to ensure the message's validity.
 *
 * @post processing has been done ready for TlsBuild() to assemble a response.
 *
 * @param[in]   eapReqData : the EAP packet arrived from the AP.
 */
static void TlsProcess(eapMethod* context, eap_packet *eapReqData, CsrUint8 reqId)
{
    TlsContext* tlsContext = (TlsContext*)context->methodContext;
    sme_trace_info((TR_SECURITY_LIB, "TlsProcess"));
    context->isKeyAvailable = FALSE;

    /* No integrity check needed here, as TlsCheck() has already been called. */
    switch(tlsContext->state)
    {
    case TLS_AWAIT_START:
        sme_trace_info((TR_SECURITY_LIB, "TlsProcess() :: TLS_AWAIT_START"));
        process_eaptls_server_start(tlsContext);
        build_eaptls_client_hello(tlsContext, reqId);
        tlsContext->state = TLS_AWAIT_SERVER_HELLO;
        break;
    case TLS_AWAIT_SERVER_HELLO:
        sme_trace_info((TR_SECURITY_LIB, "TlsProcess() :: TLS_AWAIT_SERVER_HELLO"));
        sme_trace_info((TR_SECURITY_LIB, "TlsProcess() :: tls_pkt_flags = 0x%02x)", tlsContext->tls_pkt_flags));
        sme_trace_info((TR_SECURITY_LIB, "TlsProcess() :: tls_parse_flags = 0x%04x)", tlsContext->tls_parse_flags));
        if(tlsContext->tls_pkt_flags & TLS_MOREBIT)
        {
            sme_trace_info((TR_SECURITY_LIB, "TlsBuildResp() :: More data"));
            build_eaptls_response(tlsContext, reqId);
        }
        else
        if(tlsContext->tls_parse_flags & SERVER_DONE)
        {
            sme_trace_info((TR_SECURITY_LIB, "TlsBuildResp() :: SERVER_DONE"));
            build_eaptls_client_key_exchange(tlsContext, reqId);
            tlsContext->state = TLS_AWAIT_SERVER_FINISHED;
        }

        if(tlsContext->tls.session_flags & SESSION_ID_MATCH)
        {
            sme_trace_info((TR_SECURITY_LIB, "TlsProcess() :: SESSION_ID_MATCH"));
            build_eaptls_client_finished(tlsContext, reqId);
            if (tlsContext->eapType == EAP_TYPE_TLS)
            {
                TlsDerivePmk(tlsContext);
            }
            context->decision = COND_SUCC;
            tlsContext->state = TLS_AWAIT_SUCCESS;
        }
        break;
    case TLS_AWAIT_SERVER_FINISHED:
        sme_trace_info((TR_SECURITY_LIB, "TlsProcess() :: TLS_AWAIT_SERVER_FINISHED"));
        build_eaptls_response(tlsContext, reqId);
        if (tlsContext->eapType == EAP_TYPE_TLS)
        {
            TlsDerivePmk(tlsContext);
        }
        context->decision = COND_SUCC;
        tlsContext->state = TLS_AWAIT_SUCCESS;

        /* Install the session information only if it is TLS authentication
         * other auth methods that use TLS need to complete phase two before session can be stored */
        if (tlsContext->eapType == EAP_TYPE_TLS)
        {
            install_tls_session(tlsContext);
        }

        break;
    case TLS_AWAIT_SUCCESS:
        sme_trace_info((TR_SECURITY_LIB, "TlsProcess() :: TLS_AWAIT_SUCCESS"));
        break;
    }/* switch */
}

/**
 * @brief Send message to the AP.
 *
 * @par Description
 *   This function's name contains "Resp" as a reference to RFC:4137 (EAP
 *   state machines) which assumes anything a supplicant sends out is a
 *   response to a request from an AP. This isn't strictly true and this
 *   function may return a response, a request, or nothing depending upon
 *   the current TLS state.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet (if length = zero then
 *   no packet).
 */
static DataBuffer TlsBuildResp(eapMethod* context, CsrUint8 reqId)
{
    TlsContext* tlsContext = (TlsContext*)context->methodContext;

    sme_trace_info((TR_SECURITY_LIB, "TlsBuildResp"));

    return tlsContext->dataref;
}

/**
 * @brief Return a key to the caller.
 *
 * @par Description
 *   This function returns the key associated with the last EAPOL-Key
 *   message. In the case of the session key, this will have been calculated
 *   by us and triggered by a minimal EAPOL-Key message. In the case of the
 *   multicast key, this will have been put out on air by the AP.
 *
 * @pre
 *   The caller must check isKeyAvailable before calling this function.
 *   isKeyAvailable must be TRUE or the return value is undefined.
 *
 * @return
 *   session or group key - whichever was most recently obtained.
 */
static eapKey *TlsGetKey(eapMethod* context)
{
    TlsContext* tlsContext = (TlsContext*)context->methodContext;
    return &tlsContext->key;
}

static void TlsDeinit(eapMethod* context)
{
    TlsContext* tlsContext = (TlsContext*)context->methodContext;

    if(tlsContext->tls.md5_ctx)
    {
        CsrPfree(tlsContext->tls.md5_ctx);
    }
    if(tlsContext->tls.sha1_ctx)
    {
        CsrPfree(tlsContext->tls.sha1_ctx);
    }
    CsrPfree(context->methodContext);
    context->methodContext = NULL;
}


/**
 * @brief Get TTLS implicit challenge
 *
 * @par Description
 *   This function computes and returns the implicit challenge as required by TTLSv0
 *   Can be extended, if required, for other methods
 *
 * @pre
  *
 * @return
 *   void
 */
void TlsGetPrf(eapMethod* method, const char *label, CsrUint8 *outputChallenge, CsrUint16 outputLength)
{
    TlsContext* tlsContext = (TlsContext*)method->methodContext;
    CsrUint8 random_seed[64];

    CsrMemCpy(random_seed, &(tlsContext->tls.client_random), 32);
    CsrMemCpy(&(random_seed[32]), &(tlsContext->tls.server_random), 32);

    prf(tlsContext->tls.master_secret, TLS_MASTER_SECRET_LENGTH, label,
            CsrStrLen(label), random_seed, 64, outputChallenge, outputLength);
}


/**
 * @brief Set the EAP authentication type (TLS, TTLS, PEAP, FAST etc)
 *
 * @par Description
 *   This function sets the actual authentication mechanism in use.
 *
 * @pre
  *
 * @return
 *   void
 */
void TlsSetEapType(eapMethod* method, CsrUint8 eapType)
{
    TlsContext* tlsContext = (TlsContext*)method->methodContext;
    tlsContext->eapType = eapType;
}

/**
 * @brief Communicate state of TLS when it is phase 1 authentication method (eg. when using TTLS, PEAP, FAST)
 *
 * @par Description
 *   This function returns state.
 *
 * @pre
  *
 * @return
 *   tls_phase1_state
 */
tls_phase1_state TlsPhase1State(eapMethod* method)
{
    TlsContext* tlsContext = (TlsContext*)method->methodContext;

    if ((tlsContext->state == TLS_AWAIT_SUCCESS) && (tlsContext->tls.session_flags & SESSION_ID_MATCH))
    {
        return TLS_SESSION_RESUMPTION;
    }
    else if (tlsContext->state == TLS_AWAIT_SUCCESS)
    {
        return TLS_DONE;
    }

    return TLS_CONTINUES;
}

/* This is the method interface for use by the EAP state machine. */
static const eapMethod tls_method =
{
    NULL,  /* Pointer to local Context */
    NONE,  /* state */
    FAIL,  /* decision */
    FALSE, /* allow notifications */
    FALSE, /* isKeyAvaiable */
    FALSE, /* isEapolKeyHint */
    TlsDeinit,
    TlsCheck,
    TlsProcess,
    TlsBuildResp,
    TlsGetKey
};


/**
 * @brief Initialize the TLS method state machine.
 */
void TlsEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method)
{
    TlsContext* tlsContext = (TlsContext*) CsrPmalloc(sizeof(TlsContext));

    CsrMemSet(tlsContext, 0, sizeof(TlsContext));

    sme_trace_entry((TR_SECURITY_LIB, "TlsEapMethodInit"));
    CsrMemCpy(method, &tls_method, sizeof(tls_method));
    method->methodContext = tlsContext;

    tlsContext->tls.md5_ctx = CsrCryptoMd5Init();
    tlsContext->tls.sha1_ctx = CsrCryptoSha1Init();

    if(context->setupData.session_length > TLS_MASTER_SECRET_LENGTH &&
       context->setupData.session_length <= TLS_MASTER_SECRET_LENGTH + TLS_MAX_SESSION_ID_LENGTH)
    {
        CsrMemCpy(tlsContext->tls.master_secret, context->setupData.session, TLS_MASTER_SECRET_LENGTH);
        tlsContext->tls.session_id_length = context->setupData.session_length - TLS_MASTER_SECRET_LENGTH;
        CsrMemCpy(tlsContext->tls.session_id, context->setupData.session + TLS_MASTER_SECRET_LENGTH, tlsContext->tls.session_id_length);
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "TlsEapMethodInit() :: Invalid session token size (%d)", context->setupData.session_length));
    }

    tlsContext->context = context;
    tlsContext->state = TLS_AWAIT_START;

    /* Default EAP type */
    tlsContext->eapType = EAP_TYPE_TLS;
}


