/** @file security_eap.c
 *
 * Implementation of the EAP peer state machine (RFC:4137).
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   This state machine is an implementation of the EAP peer machine in
 *   RFC:4137 with some extensions for EAPOL keying messages. It follows the
 *   RFC very closely and should be read in conjunction with that.
 *   NOTE: This EAP state machine operates on EAPOL messages including LLC SNAP
 *         headers. All incoming packets should start with a SNAP dsap field
 *         and all outgoing messages will do so likewise.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_eap/csr_eap.c#2 $
 *
 ****************************************************************************/
#include "csr_security_private.h"
#include "csr_eap/csr_eap.h"
#include "plugins/security_method.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "plugins/null/security_method_null.h"
#include "plugins/leap/leap.h"
#include "plugins/fast/security_method_eapfast.h"
#include "plugins/tls/eaptls.h"
#include "plugins/tls/csr_tls.h"
#include "plugins/ttls/eapttls.h"
#include "plugins/sim/eap_sim.h"


#define EAP_EMPTY_REQUEST_SIZE  6   /* Size of 0 data request packet (acts as an acknowledgement to previous response */

/* All possible EAP machine states. */
typedef enum
{
    UNINITIALISE, /* Something critical blocking operation. */
    DISABLED,
    INITIALISE,
    IDLE,
    RECEIVED,
    DISCARD,
    SEND_RESPONSE,
    METHOD,
    GET_METHOD,
    IDENTITY,
    NOTIFICATION,
    RETRANSMIT,
    FAILURE,
    SUCCESS
} eap_state;

/* This is the control structure for the EAP state machine.
 * It contains all the state variables. Almost all of this stuff is specified
 * in RFC 4137. */
struct EapContext
{
    CsrWifiSecurityContext* securityContext;

    eap_state   state;

    /* Variables - lower layer to peer. Defined by RFC:4137 */
    CsrBool eapReq;         /* Set to TRUE when a packet comes in. Indicates a request is available. */
    eapol_packet *eapolReqData; /* Contents of the available request from lower layers. */
    CsrUint16 req_data_size;      /* Size of packet in eapolReqData. */
    CsrBool allAccept;      /* Alternate indication of success - not used. */
    CsrBool allReject;      /* Alternate indication of failure - not used. */

    /* Variables - peer to lower layer. Defined by RFC:4137 */
    /* CsrBool eapResp;         Indicates a response is to be sent. Not needed as we use blocking sends. */
    DataBuffer eapolRespData; /* This points to a response to send. */
    eapKey *eapKeyData;      /* Set during the METHOD state when key data becomes available.
                                This structure is method-specific and therefore undefined in EAP. */
    /* CsrBool eapKeyAvailable; Not needed, as keys are applied by us as they come in. */

    /* Short Term (not maintained between packets) - set by parseEapRequest(). Defined by RFC:4137 */
    CsrBool rxReq;     /* Set in RECEIVED state. Indicates that the current received packet is an EAP request. */
    CsrBool rxKey;     /* Set to TRUE when an EAPOL key packet comes in. Additional to RFC 4137. */
    CsrBool rxSuccess; /* Set in RECEIVED state. Indicates that the current received packet is an EAP Success. */
    CsrBool rxFailure; /* Set in RECEIVED state. Indicates that the current received packet is an EAP Failure. */
    CsrUint8 reqId;       /* Set in RECEIVED state. The identifier value associated with the current EAP request. */
    CsrUint8 reqMethod;   /* Set in RECEIVED state. The method type of the current EAP request. */

    /* Long term (maintained between packets). Defined by RFC:4137 */
    CsrUint8 selectedMethod;                /* Set in GET_METHOD state. The method that the peer believes is currently "in progress". */
    CsrInt16 lastId;                          /* EAP id of the last packet sent. 0-255 or -1 = NONE */
    DataBuffer lastFragment;                /* Fragment pointer last sent */
    eapMethod method;                      /* The currently plugged-in method. */

};

/*lint -save -e429 */

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static DataBuffer getNextFragment(EapContext *eap_context, DataBuffer data)
{
    DataBuffer nextFragment; /* Fragment to be transmitted */
    CsrBool firstFragment; /* First fragment of the series, Will be used in setting TLS Flag */

    /* TODO: Can move header creation (SNAP, EAPOL, EAP) from individual methods to this function */

    if (data.dataLength < CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE)
    {
        if (eap_context->lastFragment.buf == data.buf)
        {
            sme_trace_error((TR_SECURITY_LIB, "Previously sent (only) fragment requested again while not retransmission"));
            /* require(TR_SECURITY_LIB, 0); TODO: Temp remove this */
        }
        /* Default case, only one fragment to send */
        nextFragment = data;
    }
    else
    {
        firstFragment = FALSE;

        if (eap_context->lastFragment.buf == NULL)
        {
            /* first fragment */
            firstFragment = TRUE;
        }

        if ((firstFragment == FALSE) &&
                (data.buf + data.dataLength) <= (eap_context->lastFragment.buf + CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE))
        {
            sme_trace_error((TR_SECURITY_LIB, "Previously sent fragment requested again while not retransmission"));
            nextFragment = data;
            /* require(TR_SECURITY_LIB, 0);  TODO: Temp remove this */
        }
        else
        {
            /* Work out the next fragment */

            /* Fill up the header */
            switch(eap_context->selectedMethod)
            {
                case EAP_TYPE_TLS:
                {
                    eapol_packet *eapolPacket;
                    CsrUint8 headerLength; /* All headers before TLS data */

                    /*  EAP packet following SNAP and EAPOL header

                      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                      |     Code      |   Identifier  |            Length             |
                      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                      |     Type      |     Flags     |      TLS Message Length       |
                      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                      |     TLS Message Length        |       TLS Data...
                      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                      */

                    headerLength = EAP_REQ_RESP_HEADER_LENGTH + EAPTLS_HEADER_LENGTH +
                                   CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH;

                    if (firstFragment == TRUE)
                    {
                        nextFragment.buf = data.buf;
                        headerLength += EAPTLS_MESSAGE_LENGTH_LENGTH;
                    }
                    else
                    {
                        nextFragment.buf = eap_context->lastFragment.buf + CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE - headerLength;
                        CsrMemSet(nextFragment.buf, 0 , headerLength);
                    }

                    eapolPacket = (eapol_packet *)nextFragment.buf;

                    if ((data.buf + data.dataLength) <= (nextFragment.buf + CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE))
                    {
                        /* It is the last fragment */
                            nextFragment.dataLength = (CsrUint16)((data.buf + data.dataLength) - nextFragment.buf);
                        ((eaptls_packet *)(&eapolPacket->u.eap.u.resp.u.data))->flags = 0;
                    }
                    else
                    {
                        nextFragment.dataLength = EAP_MAX_FRAGMENT_SIZE;
                        ((eaptls_packet *)(&eapolPacket->u.eap.u.resp.u.data))->flags = TLS_MOREBIT;
                    }

                    eapolPacket->version = 1;
                    eapolPacket->packet_type = EAPOL_PACKET_TYPE_EAP;
                    EAPOL_LENGTH_ASSIGN(eapolPacket, (nextFragment.dataLength - (CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH)));

                    eapolPacket->u.eap.code = EAP_CODE_RESP;
                    eapolPacket->u.eap.id = eap_context->reqId;

                    eapolPacket->u.eap.eap_length_hi  = eapolPacket->eapol_length_hi;
                    eapolPacket->u.eap.eap_length_low = eapolPacket->eapol_length_low;

                    /* Type and flags fields */
                    eapolPacket->u.eap.u.resp.type = EAP_TYPE_TLS;

                    /* if it is the first fragment, add additional length field */
                    if (firstFragment == TRUE)
                    {
                        CsrUint32 length = data.dataLength - headerLength;
                        ((eaptls_packet *)(&eapolPacket->u.eap.u.resp.u.data))->flags |= TLS_LENGTHBIT;
                        ((eaptls_packet *)(&eapolPacket->u.eap.u.resp.u.data))->u.lengthed.length[0] = (length & 0xFF000000) >> 24;
                        ((eaptls_packet *)(&eapolPacket->u.eap.u.resp.u.data))->u.lengthed.length[1] = (length & 0xFF0000) >> 16;
                        ((eaptls_packet *)(&eapolPacket->u.eap.u.resp.u.data))->u.lengthed.length[2] = (length & 0xFF00) >> 8;
                        ((eaptls_packet *)(&eapolPacket->u.eap.u.resp.u.data))->u.lengthed.length[3] = length & 0xFF;
                    }
                }
                break;

                default:
                {
                    sme_trace_error((TR_SECURITY_LIB, "Fragmentation not supported in this method"));
                    nextFragment.buf = data.buf;
                    nextFragment.dataLength = CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE;
                }
                break;
            }
        }
    }

    /* Copy the SNAP header */
    CsrMemCpy(nextFragment.buf, eap_context->securityContext->setupData.protocolSnap, CSR_WIFI_SECURITY_SNAP_LENGTH);

    eap_context->lastFragment = nextFragment;
    return nextFragment;
}

/**
 * @brief Parse an incoming EAPOL message.
 *
 * @par Description
 *      Determines the code, identifier value, and type of the current response.
 *      In the case of a parsing error (e.g. the length field is longer than the
 *      received packet), rxResp will be set to FALSE. The values of reqId and
 *      respMethod may be undefined as a result.
 *      This function relies upon the eap_context structure for inputs and
 *      outputs.
 *      Returns (in eap_context):
 *          booleans (reReq, rxSuccess, rxFailure, rxKey and m->isEapolKeyHint),
 *          an integer (reqId)
 *          and an EAP type (reqMethod).
 */
void parseEapReq(EapContext *eap_context)
{
    sme_trace_entry((TR_SECURITY_LIB, "parseEapReq"));

    eap_context->rxReq = FALSE;
    eap_context->rxKey = FALSE;
    eap_context->rxSuccess = FALSE;
    eap_context->rxFailure = FALSE;
    eap_context->method.isEapolKeyHint = FALSE;

    if (eap_context->req_data_size < EAPOL_LENGTH_FIELD(eap_context->eapolReqData))
    {
        /* failure */
        sme_trace_error((TR_SECURITY_LIB, "packet type, size[%d] > EAPOL_LENGTH_FIELD[%d]",
                            eap_context->req_data_size,
                            EAPOL_LENGTH_FIELD(eap_context->eapolReqData)));
        return;
    }

    sme_trace_entry((TR_SECURITY_LIB, "packet type, %d", eap_context->eapolReqData->packet_type));

    switch (eap_context->eapolReqData->packet_type)
    {
    case EAPOL_PACKET_TYPE_EAP:
        eap_context->reqId = eap_context->eapolReqData->u.eap.id;

        sme_trace_info((TR_SECURITY_LIB, "EAPOL packet type, %d", eap_context->eapolReqData->u.eap.code));

        switch (eap_context->eapolReqData->u.eap.code)
        {
        case EAP_CODE_SUCCESS:
            sme_trace_info((TR_SECURITY_LIB, "EAP SUCCESS"));
            eap_context->rxSuccess = TRUE;
            break;
        case EAP_CODE_FAILURE:
            sme_trace_info((TR_SECURITY_LIB, "EAP FAILURE"));
            eap_context->rxFailure = TRUE;
            break;
        case EAP_CODE_RESP:
            /* Responses from the AS are treated like requests for the purposes
             * of this EAP state machine.
             * RFC:4137 assumes all messages from the AS are "requests", which
             * isn't always true. */
        case EAP_CODE_REQ:
            eap_context->rxReq = TRUE;
            eap_context->reqMethod = eap_context->eapolReqData->u.eap.u.req.type;
            break;
        default:
            sme_trace_info((TR_SECURITY_LIB, "parseEapReq() :: Unknown EAP code (0x%02x)", eap_context->eapolReqData->u.eap.code));
            break;
        }
        break;
    case EAPOL_PACKET_TYPE_EAPOL_START:
        /* As a station (supplicant), we shouldn't get one of these. */
        return;
    case EAPOL_PACKET_TYPE_EAPOL_LOGOFF:
        /* failure */
        return;
    case EAPOL_PACKET_TYPE_EAPOL_ALERT:
        /* ignore alerts */
        return;
    case EAPOL_PACKET_TYPE_EAPOL_KEY:
        /* Additional to RFC 4137 for EAPOL-KEY handling. */
        eap_context->method.isEapolKeyHint = TRUE;
        eap_context->rxKey = TRUE;
        return;
    default:
        sme_trace_info((TR_SECURITY_LIB, "parseEapReq() :: Unknown packet type (0x%02x)", eap_context->eapolReqData->packet_type));
        break;
    }
}

/**
 * @brief Perform any necessary processing of the incoming identity request.
 */
void processIdentity()
{
    /* nothing to do */
}

/**
 * @brief Assemble an EAPOL start message.
 *
 * @par Description
 *
 * @return a reference to the outgoing queued packet.
 */
DataBuffer buildEapolStart(EapContext *eap_context)
{
    eapol_packet *eapol = (eapol_packet *)eap_context->securityContext->buffer;
    CsrUint16 length;
    DataBuffer dataref = {CSR_WIFI_SECURITY_SNAP_LENGTH
                          + EAPOL_HEADER_LENGTH,
                          NULL};
    dataref.buf = (CsrUint8 *)eapol;

    /* 0-7 = space for the LLC SNAP header */

    /* Add the EAPOL data */
    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAPOL_START;
    length = 0;
    EAPOL_LENGTH_ASSIGN(eapol, length);

    return dataref;
}

/**
 * @brief Assemble an identity response message.
 *
 * @par Description
 *      An identity response is sent as a reply to an identity request from the
 *      AP. It contains a username.
 *
 * @return a reference to the outgoing queued packet.
 */
DataBuffer buildIdentity(EapContext *eap_context)
{
    eapol_packet *eapol = (eapol_packet *)eap_context->securityContext->buffer;
    CsrUint16 length;
    DataBuffer dataref = {CSR_WIFI_SECURITY_SNAP_LENGTH
                          + EAPOL_HEADER_LENGTH
                          + EAP_REQ_RESP_HEADER_LENGTH,
                          NULL};
    dataref.buf = (CsrUint8 *)eapol;
    dataref.dataLength += (CsrUint16)CsrStrLen((char *)eap_context->securityContext->setupData.identity);

    require(TR_SECURITY_LIB, eap_context->securityContext->setupData.identity != NULL);
    sme_trace_debug((TR_SECURITY_LIB, "eap_context->eapolReqData->u.eap.id %d", eap_context->eapolReqData->u.eap.id));

    /* 0-7 = space for the LLC SNAP header */

    /* Add the EAPOL data */
    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    length = EAP_REQ_RESP_HEADER_LENGTH + (CsrUint16)CsrStrLen((char *)eap_context->securityContext->setupData.identity);
    EAPOL_LENGTH_ASSIGN(eapol, length);

    /* Add the EAP header */
    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id   = eap_context->reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;
    eapol->u.eap.u.resp.type = EAP_TYPE_IDENTITY;
    CsrMemCpy(&eapol->u.eap.u.resp.u.data, eap_context->securityContext->setupData.identity,
                       CsrStrLen((char *)eap_context->securityContext->setupData.identity));

    return dataref;
}

/**
 * @brief Assemble a NAK message.
 *
 * @par Description
 *      The EAP NAK messages are used to indicate to the AP that a particular
 *      authentication protocol is unsupported by this supplicant. The NAK
 *      message contains a recommended authentication protocol as a suggested
 *      alternative to the AP.
 *
 * @param[in] reqId : EAP message ID of the corresponding AP message which we
 *                    are going to NAK.
 *
 * @return a reference to the outgoing queued packet.
 */
DataBuffer buildNak(EapContext *eap_context)
{
    eapol_packet *eapol = (eapol_packet *)eap_context->securityContext->buffer;
    CsrUint16 length;
    DataBuffer dataref = {CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH + EAP_REQ_RESP_HEADER_LENGTH + sizeof(eap_nak), NULL};
    dataref.buf = (CsrUint8 *)eapol;

    /* EAPOL fields */
    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    length = EAP_REQ_RESP_HEADER_LENGTH + sizeof(eap_nak);
    EAPOL_LENGTH_ASSIGN(eapol,length);

    /* EAP fields */
    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id   = eap_context->reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;
    eapol->u.eap.u.resp.type = EAP_TYPE_NAK;


    sme_trace_debug((TR_SECURITY_LIB, "recommending = %d", convertSecurityTypeToEapID(eap_context->securityContext->setupData.securityType) ));

    /* Recommend EAP-TLS for testing EAP-TLS */
    eapol->u.eap.u.resp.u.nak.recommended = convertSecurityTypeToEapID(eap_context->securityContext->setupData.securityType);

    return dataref;
}

/**
 * @brief Determine if a requested authentication protocol is supported,
 *
 * @param[in] requested_method : EAP "type" code of the required authentication
 *                               protocol.
 */
CsrBool allowMethod(EapContext *eap_context)
{
    CsrBool result = FALSE;
    CsrUint8 userRequestedEapId = convertSecurityTypeToEapID(eap_context->securityContext->setupData.securityType);

    sme_trace_info((TR_SECURITY_LIB, "allowMethod()::requested_method = %d, userRequestedEapId = %d", eap_context->reqMethod, userRequestedEapId));

    if (eap_context->reqMethod != userRequestedEapId)
    {
        return FALSE;
    }

    switch(eap_context->reqMethod)
    {
#ifdef CSR_WIFI_SECURITY_LEAP_ENABLE
    case EAP_TYPE_LEAP:
#endif
#ifdef CSR_WIFI_SECURITY_FAST_ENABLE
    case EAP_TYPE_EAPFAST:
#endif
#ifdef CSR_WIFI_SECURITY_TLS_ENABLE
    case EAP_TYPE_TLS:
#endif
#ifdef CSR_WIFI_SECURITY_TTLS_ENABLE
    case EAP_TYPE_TTLS:
#endif
#ifdef CSR_WIFI_SECURITY_SIM_ENABLE
    case EAP_TYPE_SIM:
#endif
        result = TRUE;
        break;
#ifdef CSR_WIFI_SECURITY_PEAP_ENABLE
    case EAP_TYPE_PEAP:
#endif
    default:
        result = FALSE;
        break;
    }

    return result;
}

/**
 * @brief Install a plug in method to this EAP state machine.
 *
 * @par Description
 *      Only one method can be installed at one time. This function will
 *      overwrite any previously plugged-in method.
 *
 * @pre it is best to call allowMethod() to check that we can support the
 *      method first.
 *
 * @param[in] requested_method : EAP "type" code of the required authentication
 *                               protocol.
 */
void selectMethod(EapContext *eap_context)
{
    switch(eap_context->selectedMethod)
    {
#ifdef CSR_WIFI_SECURITY_LEAP_ENABLE
    case EAP_TYPE_LEAP:
        sme_trace_info((TR_SECURITY_LIB, "selectMethod() :: LEAP"));

        /* Deinit the Old Method(Probably the NULL Method) */
        eap_context->method.deinit(&eap_context->method);
        LeapEapMethodInit(eap_context->securityContext, &eap_context->method);

        break;
#endif
#ifdef CSR_WIFI_SECURITY_FAST_ENABLE
    case EAP_TYPE_EAPFAST:
        sme_trace_info((TR_SECURITY_LIB, "selectMethod() :: EAP-FAST"));
        /* Deinit the Old Method(Probably the NULL Method) */
        eap_context->method.deinit(&eap_context->method);
        FastEapMethodInit(eap_context->securityContext, &eap_context->method);
        break;
#endif
#ifdef CSR_WIFI_SECURITY_TLS_ENABLE
    case EAP_TYPE_TLS:
        sme_trace_info((TR_SECURITY_LIB, "selectMethod() :: EAP-TLS"));
        /* Deinit the Old Method(Probably the NULL Method) */
        eap_context->method.deinit(&eap_context->method);
        TlsEapMethodInit(eap_context->securityContext, &eap_context->method);
        break;
#endif
#ifdef CSR_WIFI_SECURITY_TTLS_ENABLE
    case EAP_TYPE_TTLS:
        sme_trace_info((TR_SECURITY_LIB, "selectMethod() :: EAP-TTLS"));
        /* Deinit the Old Method(Probably the NULL Method) */
        eap_context->method.deinit(&eap_context->method);
        TtlsEapMethodInit(eap_context->securityContext, &eap_context->method);
        break;
#endif
#ifdef CSR_WIFI_SECURITY_SIM_ENABLE
    case EAP_TYPE_SIM:
        sme_trace_info((TR_SECURITY_LIB, "selectMethod() :: EAP-SIM"));
        /* Deinit the Old Method(Probably the NULL Method) */
        eap_context->method.deinit(&eap_context->method);
        SimEapMethodInit(eap_context->securityContext, &eap_context->method);
        break;
#endif
    default:
        sme_trace_info((TR_SECURITY_LIB, "selectMethod() :: Unsupported method(%d)", eap_context->selectedMethod));
        break;
    }
}

/**
 * @brief Install the key into the firmware
 *
 * @param[in] key : the key to be installed.
 */
void apply_key(EapContext *eap_context)
{
    sme_trace_info((TR_SECURITY_LIB, "apply_key() called"));
    sme_trace_info((TR_SECURITY_LIB, "apply_key() :: key  length = %d", eap_context->eapKeyData->length));

/*    CipherSuiteSelector css; */
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "apply_key() :: keydata", eap_context->eapKeyData->keydata, (CsrUint32)eap_context->eapKeyData->length));
    sme_trace_info((TR_SECURITY_LIB, "apply_key() :: index = %d", eap_context->eapKeyData->index));

    switch(eap_context->eapKeyData->keytype)
    {
    case KeyType_Pairwise:
        sme_trace_info((TR_SECURITY_LIB, "apply_key() :: KeyType_Pairwise"));
        /* FixMe Pass index etc ? */
        if(eap_context->securityContext->callbacks.installPairwiseKey)
        {
            (eap_context->securityContext->callbacks.installPairwiseKey)(eap_context->securityContext->externalContext,
                                                                 CSR_WIFI_SECURITY_KEYTYPE_LEAPWEP,
                                                                 eap_context->eapKeyData->keydata,
                                                                 eap_context->eapKeyData->length,
                                                                 NULL,
                                                                 1);
        }
        else
        {
            sme_trace_info((TR_SECURITY_LIB, "apply_key() :: No Pairwise key callback installed"));
        }
        break;
    case KeyType_Group:
        sme_trace_info((TR_SECURITY_LIB, "apply_key() :: KeyType_Group"));
        /* FixMe Pass index etc ? */
        if(eap_context->securityContext->callbacks.installGroupKey)
        {
            (eap_context->securityContext->callbacks.installGroupKey)(eap_context->securityContext->externalContext,
                                                              CSR_WIFI_SECURITY_KEYTYPE_LEAPWEP,
                                                              eap_context->eapKeyData->keydata,
                                                              eap_context->eapKeyData->length,
                                                              NULL,
                                                              2);
        }
        else
        {
            sme_trace_info((TR_SECURITY_LIB, "apply_key() :: No Group key callback installed"));
        }
        break;
    case KeyType_PAC:
        sme_trace_info((TR_SECURITY_LIB, "apply_key() :: KeyType_PAC"));
        if(eap_context->securityContext->callbacks.installPac)
        {
            (eap_context->securityContext->callbacks.installPac)(eap_context->securityContext->externalContext,
                                                         eap_context->securityContext->setupData.fast_pac,
                                                         eap_context->securityContext->setupData.fast_pac_length);
        }
        else
        {
            sme_trace_info((TR_SECURITY_LIB, "apply_key() :: No PAC key callback installed"));
        }
        break;
    case KeyType_Session:
        sme_trace_info((TR_SECURITY_LIB, "apply_key() :: KeyType_Session"));
        (eap_context->securityContext->callbacks.installSession)(eap_context->securityContext->externalContext,
                                                                 eap_context->eapKeyData->keydata,
                                                                 (CsrUint32)eap_context->eapKeyData->length);
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "apply_key() :: Unknown KeyType = %d", eap_context->eapKeyData->keytype));
        break;
    }
}

/**
 * @brief Assemble a message, if necessary, to send to the AP.
 *
 * @par Description
 *   This function enacts the RFC:4137 state machine. It should be run every
 *   time an EAP event occurs, i.e. anything that has changed the eap_context.
 *   In practice this will be:
 *      Initialization or
 *      EAP packet arrival from the AP.
 *   This function will run recursively until all transitions triggered by the
 *   event have finished, whereupon it will end up in the IDLE, SUCCESS or
 *   FAILURE state.
 *   There are no explicit incoming parameters or results because everything is
 *   done on the eap_context object.
 */
CsrBool process_eap_state(EapContext *eap_context)
{
    CsrBool handshakeEapol = FALSE;

/*   The coding of this function is intended to stick as rigidly as possible to
 *   RFC:4137, for clarity, rather than being particularly efficient. */
    sme_trace_entry((TR_SECURITY_LIB, "process_eap_state"));

    switch (eap_context->state)
    {
    case UNINITIALISE:
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: UNINITIALISE."));
        break;

    case DISABLED:
        /* This state is reached whenever service from the lower layers is
         * interrupted or unavailable. Immediate transition to INITIALISED
         * occurs when the port becomes available.
         * Not currently used. Port always open when we get into this state
         * machine. */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: DISABLED."));
        eap_context->state = INITIALISE;
        (void)process_eap_state(eap_context);
        break;

    case INITIALISE:
        /* Initialize variables when state machine is activated. */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: INITIALISE."));

        /* Not explicitly mentioned in RFC 4137. */
        eap_context->rxSuccess = FALSE;
        eap_context->rxFailure = FALSE;
        eap_context->allAccept = FALSE;
        eap_context->allReject = FALSE;
        NullEapMethodInit(eap_context->securityContext, &eap_context->method);

        /* Explicitly mentioned in RFC 4137. */
        eap_context->selectedMethod = EAP_TYPE_NONE;
        eap_context->method.methodState = NONE;
        eap_context->rxReq = FALSE;
        eap_context->eapReq = FALSE;
        /* eap_context.eapResp = FALSE; Not needed as we use blocking sends. */
        eap_context->lastId = -1;
        eap_context->eapKeyData = NULL;
        /* eap_context.eapKeyAvailable = FALSE; Not needed */

        eap_context->state = IDLE;
        (void)process_eap_state(eap_context);
        break;

    case IDLE:
        /* The state machine spends most of its time here, waiting for something
         * to happen. */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: IDLE."));

        if (eap_context->eapReq)
        {
            eap_context->state = RECEIVED;
            (void)process_eap_state(eap_context);
        }
        else if ( (eap_context->allAccept && eap_context->method.decision != FAIL) ||
                  (eap_context->method.decision == UNCOND_SUCC) )
        {
            eap_context->state = SUCCESS;
            (void)process_eap_state(eap_context);
        }
        else if ( (eap_context->allReject) ||
                  /* (eap_context.m->decision != UNCOND_SUCC) && (idleWhile == 0)|| */
                  ( (eap_context->allAccept) &&
                    (eap_context->method.methodState != CONT) &&
                    (eap_context->method.decision == FAIL) ) )
        {
            eap_context->state = FAILURE;
            (void)process_eap_state(eap_context);
        }
        break;

    case RECEIVED:
        /* This state is entered when an EAP packet is received. The packet
         * header is parsed here. */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: RECEIVED."));
        parseEapReq(eap_context);
        sme_trace_info((TR_SECURITY_LIB, "rxReq [%d] reqId [%d] lastId [%d] reqMethod [%d] m->allowNotifications [%d] ",
                                eap_context->rxReq,
                                eap_context->reqId, eap_context->lastId,
                                eap_context->reqMethod,
                                eap_context->method.allowNotifications));

        sme_trace_info((TR_SECURITY_LIB, "rxReq [%d] reqId [%d] lastId [%d] reqMethod [%d] m->allowNotifications [%d] rxsuccess [%d], decision [%d] ",
                                eap_context->rxReq,
                                eap_context->reqId, eap_context->lastId,
                                eap_context->reqMethod,
                                eap_context->method.allowNotifications,
                                eap_context->rxSuccess,
                                eap_context->method.decision));
        if ( (eap_context->rxSuccess) &&
             (eap_context->reqId >= eap_context->lastId) &&  /* (eap_context->reqId == eap_context->lastId) && */
             (eap_context->method.decision != FAIL) )
        {
            /* This test is additional to RFC 4137 in order to support LEAP,
             * which has a success message cropping up in the middle of the
             * exchange. */
            eap_context->state = SUCCESS;
        }
        else if ( (eap_context->method.methodState != CONT) &&
                  ( (eap_context->rxFailure && eap_context->method.decision != UNCOND_SUCC) ||
                    (eap_context->rxSuccess && eap_context->method.decision == FAIL) ) &&
                  (eap_context->reqId == eap_context->lastId) )
        {
            eap_context->state = FAILURE;
        }
        else if ( (eap_context->rxReq) &&
                  (eap_context->reqId != eap_context->lastId) &&
                  (eap_context->selectedMethod == NONE) &&
                  (eap_context->reqMethod != EAP_TYPE_IDENTITY) &&
                  (eap_context->reqMethod != EAP_TYPE_NOTIFICATION) )

        {
            eap_context->state = GET_METHOD;
        }
        else if ( (eap_context->rxReq) &&
                  (eap_context->reqId != eap_context->lastId) &&
                  (eap_context->selectedMethod == EAP_TYPE_NONE) &&
                  (eap_context->reqMethod == EAP_TYPE_IDENTITY) )
        {
            eap_context->state = IDENTITY;
        }
        else if ( (eap_context->rxReq) &&
                  (eap_context->reqId != eap_context->lastId) &&
                  (eap_context->reqMethod == EAP_TYPE_NOTIFICATION) &&
                  (eap_context->method.allowNotifications) )
        {
            eap_context->state = NOTIFICATION;
        }
        else if ( (eap_context->rxReq) &&
                  (eap_context->reqId == eap_context->lastId) )
        {
            sme_trace_info((TR_SECURITY_LIB, "Retransmission"));
            eap_context->state = RETRANSMIT;
        }
        else if ( ( (eap_context->rxReq) &&
                    (eap_context->reqId != eap_context->lastId) &&
                    (eap_context->reqMethod == eap_context->selectedMethod) &&
                    (eap_context->method.methodState != DONE) &&
                    (eap_context->selectedMethod == EAP_TYPE_TLS) &&    /* Support of fragmentation now available only for TLS */
                    (eap_context->lastFragment.buf != NULL) &&
                    (EAP_TLS_START_BIT_IS_SET(eap_context->eapolReqData->u.eap) == FALSE) &&
                    (EAPOL_LENGTH_FIELD(eap_context->eapolReqData) == EAP_EMPTY_REQUEST_SIZE) ) )
        {
            sme_trace_info((TR_SECURITY_LIB, "Ack for previous transmission"));
            eap_context->state = SEND_RESPONSE;
        }
        else if ( ( (eap_context->rxReq) &&
                    (eap_context->reqId != eap_context->lastId) &&
                    (eap_context->reqMethod == eap_context->selectedMethod) &&
                    (eap_context->method.methodState != DONE) ) ||

                  /* Additional to RFC:4137. Needed for LEAP which sends SUCCESS
                     messages in the middle of the sequence. */
                  ( ( (eap_context->rxSuccess) || (eap_context->rxFailure) ) &&
                    (eap_context->reqId == eap_context->lastId) ) )
        {
            sme_trace_info((TR_SECURITY_LIB, " METHOD selection A"));
            eap_context->state = METHOD;
        }
        else if (eap_context->rxKey)
        {
            /* This isn't covered exactly like this in RFC:4137. The RFC is
             * fuzzy on the details of key handling. It does assume that keys
             * come in as normal EAP packets and get sent to the plug-in method.
             * EAPOL keys actually come in as special (non-EAP) EAPOL-KEY
             * messages.
             * Nevertheless, it is RFC:4137 policy to let the plug-in method
             * deal with key messages howsoever they arrive, so here goes ... */
            sme_trace_info((TR_SECURITY_LIB, " METHOD selection B"));
            eap_context->state = METHOD;
        }
        else
        {
            eap_context->state = DISCARD;
        }
        (void)process_eap_state(eap_context);
        break;

    case DISCARD:
        /* This state means that an incoming request was discarded and no
         * response will be sent this time. */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: DISCARD."));

        eap_context->eapReq = FALSE;
        /* eap_context->eapNoResp = TRUE; - not needed. */
        eap_context->state = IDLE;
        (void)process_eap_state(eap_context);
        break;
    case SEND_RESPONSE:
        {
            DataBuffer respData;

            /* This state signals the lower layer that a response packet is ready
             * to be sent. */
            sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: SEND_RESPONSE."));

            eap_context->lastId = eap_context->reqId;

            /* eap_context->eapResp = TRUE; - Not needed as we use blocking sends. */
            eap_context->eapReq = FALSE;

            respData = getNextFragment(eap_context, eap_context->eapolRespData);

            /* If we have more fragments, send them one after the other */
            sme_trace_info((TR_SECURITY_LIB, "Fragment length  = %d", respData.dataLength));
            sme_trace_info((TR_SECURITY_LIB, "Fragment = %p", respData.buf));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Fragment", respData.buf, respData.dataLength));

            (eap_context->securityContext->callbacks.sendPacket)(eap_context->securityContext->externalContext,
                                                                 respData.buf,
                                                                 respData.dataLength,
                                                                 eap_context->securityContext->setupData.localMacAddress,
                                                                 eap_context->securityContext->setupData.peerMacAddress);

            eap_context->state = IDLE;
            (void)process_eap_state(eap_context);
        }
        break;

    case METHOD:
        /* The method processing happens here. The request from the
         * authenticator is processed, and an appropriate response packet is
         * built.
         * Processing an EAP packet involves calling functions in the method in
         * this order:
         *      check()
         *      process()
         *      build()
         * then entering the SEND_RESPONSE state to sending any resulting
         * queued message.
         */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: METHOD."));

        if (eap_context->method.check(&eap_context->method,
                                       &(eap_context->eapolReqData->u.eap),
                                       eap_context->req_data_size))
        {
            eap_context->method.process(&eap_context->method, &(eap_context->eapolReqData->u.eap), eap_context->reqId);
            if ( (eap_context->method.methodState == DONE) &&
                 (eap_context->method.decision == FAIL) )
            {
                eap_context->state = FAILURE;
            }
            else
            {
                eap_context->eapolRespData = eap_context->method.buildResp(&eap_context->method, eap_context->reqId);

                eap_context->lastFragment.buf = NULL;
                eap_context->lastFragment.dataLength = 0;

                sme_trace_info((TR_SECURITY_LIB, "eap_context->eapolRespData.dataLength = %d", eap_context->eapolRespData.dataLength ));
                sme_trace_info((TR_SECURITY_LIB, "eap_context->eapolRespData.buf = %p", eap_context->eapolRespData.buf ));

                if (eap_context->method.isKeyAvailable)
                {
                    eap_context->eapKeyData = eap_context->method.getKey(&eap_context->method);
                    apply_key(eap_context);
                }

                /* Additional to RFC:4137, because there isn't always a
                 * response to send. */
                if (eap_context->eapolRespData.dataLength != 0)
                {
                    eap_context->state = SEND_RESPONSE;
                }
                else
                {
                    sme_trace_info((TR_SECURITY_LIB, "eap_context->eapolRespData.dataLength = 0"));
                    eap_context->state = DISCARD;
                }
            }
        }
        else
        {
            eap_context->state = DISCARD;
        }
        (void)process_eap_state(eap_context);
        break;

    case GET_METHOD:
        /* This state is entered when a request for a new type comes in. Either
         * the correct method is started, or a nak response is built. */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: GET_METHOD."));

        if (allowMethod(eap_context))
        {
            eap_context->selectedMethod = eap_context->reqMethod;
            selectMethod(eap_context);
            eap_context->state = METHOD;
        }
        else
        {
            eap_context->lastFragment.buf = NULL;
            eap_context->lastFragment.dataLength = 0;
            eap_context->eapolRespData = buildNak(eap_context);
            eap_context->state = SEND_RESPONSE;
        }
        (void)process_eap_state(eap_context);
        break;

    case IDENTITY:
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: IDENTITY."));
        processIdentity();
        eap_context->lastFragment.buf = NULL;
        eap_context->lastFragment.dataLength = 0;
        eap_context->eapolRespData = buildIdentity(eap_context);
        eap_context->state = SEND_RESPONSE;
        (void)process_eap_state(eap_context);
        break;

    case NOTIFICATION:
        /* Would normally handle notifications and build a response. */
        /* Notifications during exchanges unimplemented.
         * HERE is where it goes. */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: NOTIFICATION."));
        eap_context->state = IDLE;
        (void)process_eap_state(eap_context);
        break;

    case RETRANSMIT:
        {
            /* Retransmits the previous response packet. */

            /* eap_context->eapResp = TRUE; - Not needed as we use blocking sends. */
            eap_context->eapReq = FALSE;

            /* If we have more fragments, send them one after the other */
            sme_trace_info((TR_SECURITY_LIB, "Fragment length  = %d", eap_context->lastFragment.dataLength));
            sme_trace_info((TR_SECURITY_LIB, "Fragment = %p", eap_context->lastFragment.buf));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Fragment", eap_context->lastFragment.buf,
                    eap_context->lastFragment.dataLength));

            (eap_context->securityContext->callbacks.sendPacket)(eap_context->securityContext->externalContext,
                                                                 eap_context->lastFragment.buf,
                                                                 eap_context->lastFragment.dataLength,
                                                                 eap_context->securityContext->setupData.localMacAddress,
                                                                 eap_context->securityContext->setupData.peerMacAddress);

            eap_context->state = IDLE;
            (void)process_eap_state(eap_context);
        }
        break;

    case FAILURE:
        /* EAP authentication finished unsuccessfully. We stay in this state
         * pending an eap_init(). */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: FAILURE."));
        sme_trace_info((TR_SECURITY_LIB, "Aborting process in FAILURE state"));
        eap_context->securityContext->callbacks.abortProcedure(eap_context->securityContext->externalContext);
        /* eap_context->eapFail = TRUE; - Not needed */
        break;

    case SUCCESS:
        /* EAP authentication finished successfully. We stay in this state
         * pending an eap_init(). */
        sme_trace_info((TR_SECURITY_LIB, "process_eap_state() :: SUCCESS."));

        /* This isn't needed, as keys were applied as they came in.
         * if (eap_context->eapKeyData != NULL)
         *      eap_context->eapKeyAvailable = TRUE;
         */
        /* eap_context->eapSuccess = TRUE; - Not needed */
        /* This packet needs to be forwarded to 4-way handshake module */
        handshakeEapol = TRUE;
        break;
    }

    /* Exit this recursion.
     * Will exit completely when all transitions for this event have finished. */
    return handshakeEapol;
}

/*lint -restore */

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/* See header */
EapContext *eap_init(CsrWifiSecurityContext* context)
{
    EapContext *eap_context;

    eap_context = CsrPmalloc(sizeof(EapContext));

    sme_trace_entry((TR_SECURITY_LIB, "eap_init"));

    CsrMemSet(eap_context, 0x00, sizeof(EapContext));
    eap_context->securityContext = context;

    /* Go straight to INITIALISE. Port already enabled when we get here. */
    eap_context->state = INITIALISE;

    (void)process_eap_state(eap_context);

    return eap_context;
}

/* See header */
CsrBool eap_pkt_input(EapContext *eap_context, eapol_packet *pkt, CsrUint16 size)
{
    CsrBool handshakeEapol = FALSE;

    eap_context->eapolReqData = pkt;
    eap_context->req_data_size = size;
    eap_context->eapReq = TRUE;

    sme_trace_info((TR_SECURITY_LIB, "eap_pkt_input size [%d]",size ));

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "eap_pkt_input() :: pkt EAP dump", &(pkt->u.eap), size));

    sme_trace_info((TR_SECURITY_LIB, "eap_pkt_input oui [%d] [%d] [%d].",pkt->llc_header.oui[0] ,pkt->llc_header.oui[1], pkt->llc_header.oui[2] ));

    handshakeEapol = process_eap_state(eap_context);
/*
    if (eap_context->eapolReqData->u.eap.code == EAP_CODE_SUCCESS)
    {
        CsrWifiSecurityPacketProcessingDone(eap_context->context) ;
    }
    else
*/
    if (!handshakeEapol)
    {
        CsrWifiSecurityPacketProcessingDone(eap_context->securityContext);
    }

    return handshakeEapol;
}

/* See header */
void eap_timeout(EapContext *eap_context, CsrWifiSecurityTimeout timeoutIndex)
{
    if (timeoutIndex == CSR_WIFI_SECURITY_TIMEOUT_SECURITY_ASSOCIATION)
    {
        sme_trace_error((TR_SECURITY_LIB, "Config SA Timeout: Aborting handshake"));
        eap_context->securityContext->callbacks.abortProcedure(eap_context->securityContext->externalContext);
    }
}

/* See header */
void eap_deinit(EapContext *eap_context)
{
    eap_context->method.deinit(&eap_context->method);
    CsrPfree(eap_context);
}
