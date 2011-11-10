/** @file security_method_eapfast.c
 *
 * Implementation of the EAP-FAST method.
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
 *   This provides an implementation of an EAP-FAST authentication method.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/fast/security_method_eapfast.c#1 $
 *
 ****************************************************************************/

#include "security_method_eapfast.h"
#include "sme_trace/sme_trace.h"
#include "eapfast_cisco_pacfile.h"
#include "eapfast_authentication.h"
#include "eapfast_inband_provisioning.h"

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * @brief Check the validity of an incoming EAP-FAST message.
 *
 * @par Description
 *      The packet is checked for both basic sanity and for expected
 *      packet type and content dependent upon our current state.
 *
 * @param[in]   eapReqData : the EAP message received from the AP.
 * @param[in]   size : the size of the incoming packet. Maybe be larger than the actual EAP message.
 *
 * @return
 *   TRUE - message OK, FALSE - bad or irrelevant message.
 */
static CsrBool EapFastCheck(eapMethod* context, eap_packet *eapReqData, CsrUint16 size)
{
    CsrInt32 i;
    eapfast_packet *ef_pkt = (eapfast_packet *)(&eapReqData->u.req.u.data);
    FastContext* fastContext = (FastContext*)context->methodContext;

    if (fastContext->state != EAPFAST_AWAIT_PROVISIONING_FAILURE)
    {
        if (size<EAP_REQ_RESP_HEADER_LENGTH + EAPFAST_HEADER_LENGTH)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: bad sized message."));
            return FALSE;
        }

        if ((ef_pkt->flags & EAPFAST_VERSION_BITS) != EAPFAST_VERSION_NO)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: bad version number in message."));
            return FALSE;
        }
    }

    switch(fastContext->state)
    {
    case EAPFAST_AWAIT_START:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_START"));
        /* Good packet if start bit set and other flags are zero. */
        if ((ef_pkt->flags & ~EAPFAST_VERSION_BITS) != EAPFAST_STARTBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: malformed start message."));
            return FALSE;
        }

        /* Remember the A-ID for later. */
        /*lint -save -e661 -e662 */
        for (i=0; i < ((size - EAP_REQ_RESP_HEADER_LENGTH) - EAPFAST_HEADER_LENGTH) && (i<MAX_AID_LENGTH); i++)
        {
            fastContext->aid[i] = ef_pkt->u.data[i];
        }
        /*lint -restore */
        for (;i<MAX_AID_LENGTH; i++)    /* pad with zeros */
            fastContext->aid[i] = 0;

        break;
    case EAPFAST_AWAIT_SERVER_HELLO:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_SERVER_HELLO"));
        if ((ef_pkt->flags & ~EAPFAST_VERSION_BITS) != EAPFAST_LENGTHBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: malformed server hello."));
            return FALSE;
        }
        /* We need to parse the whole message here and extract the fields
         * before we know the message is OK. */
        {
            CsrUint32 flags;
            CsrUint32 total_length = (ef_pkt->u.lengthed.length[0] << 24)
                + (ef_pkt->u.lengthed.length[1] << 16)
                + (ef_pkt->u.lengthed.length[2] << 8)
                +  ef_pkt->u.lengthed.length[3];

            if (!(parse_tls(fastContext, ef_pkt->u.lengthed.data, total_length, &fastContext->tls, &flags, NULL)))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: parse server hello failed."));
                return FALSE;
            }

            if (flags!= (SERVER_RANDOM_MODIFIED | CYPHER_SUITE_MODIFIED | CHANGE_CYPHER_SPEC_INCLUDED))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: server hello wrong content."));
                return FALSE;
            }
        }
        break;
    case EAPFAST_AWAIT_PROVISIONING_SERVER_HELLO:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_PROVISIONING_SERVER_HELLO"));
        if ((ef_pkt->flags & ~EAPFAST_VERSION_BITS) != EAPFAST_LENGTHBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: malformed server hello."));
            return FALSE;
        }

        /* We need to parse the whole message here and extract the fields
         * before we know the message is OK. */
        {
            CsrUint32 flags;
            CsrUint32 total_length = (ef_pkt->u.lengthed.length[0] << 24)
                + (ef_pkt->u.lengthed.length[1] << 16)
                + (ef_pkt->u.lengthed.length[2] << 8)
                +  ef_pkt->u.lengthed.length[3];

            if (!(parse_tls(fastContext, ef_pkt->u.lengthed.data, total_length, &fastContext->tls, &flags, NULL)))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: parse provisioning server hello failed."));
                return FALSE;
            }

            /* Check we have received all the things we were expecting to receive. */
            if (flags != (DH_G_MODIFIED | SERVER_RANDOM_MODIFIED | SERVER_PRIME_MODIFIED |
                          SERVER_DH_PUBLIC_KEY_MODIFIED | CYPHER_SUITE_MODIFIED | SERVER_DONE))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: server provisioning hello wrong content."));
                sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: flags = %04x", flags));
                return FALSE;
            }

            if (fastContext->tls.pending_cypher_suite != TLS_DH_anon_WITH_AES_128_CBC_SHA)
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: wrong cypher in server provisioning hello."));
                return FALSE;
            }
        }
        break;
    case EAPFAST_AWAIT_PROVISIONING_CHANGE_CYPHER_SPECS:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_PROVISIONING_CHANGE_CYPHER_SPECS"));
        if ((ef_pkt->flags & ~EAPFAST_VERSION_BITS) != EAPFAST_LENGTHBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: malformed change cipher specs."));
            return FALSE;
        }
        /* We need to parse the whole message here and extract the fields
         * before we know the message is OK. */
        {
            CsrUint32 flags;
            CsrUint32 total_length = (ef_pkt->u.lengthed.length[0] << 24)
                + (ef_pkt->u.lengthed.length[1] << 16)
                + (ef_pkt->u.lengthed.length[2] << 8)
                +  ef_pkt->u.lengthed.length[3];

            if (!(parse_tls(fastContext, ef_pkt->u.lengthed.data, total_length, &fastContext->tls, &flags, NULL)))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: parse provisioning server hello failed."));
                return FALSE;
            }

            /* Check we have received all the things we were expecting to receive. */
            if (flags != CHANGE_CYPHER_SPEC_INCLUDED)
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: server change cypher spec wrong content."));
                sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: flags = %04x", flags));
                return FALSE;
            }
        }
        break;
    case EAPFAST_AWAIT_PROVISIONING_ID_REQ:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_PROVISIONING_ID_REQ"));
        if ((ef_pkt->flags & ~EAPFAST_VERSION_BITS) != EAPFAST_LENGTHBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: malformed change cipher specs."));
            return FALSE;
        }
        /* We need to parse the whole message here and extract the fields
         * before we know the message is OK. */
        {
            CsrUint32 flags;
            CsrUint32 total_length = (ef_pkt->u.lengthed.length[0] << 24)
                + (ef_pkt->u.lengthed.length[1] << 16)
                + (ef_pkt->u.lengthed.length[2] << 8)
                +  ef_pkt->u.lengthed.length[3];

            if (!(parse_tls(fastContext, ef_pkt->u.lengthed.data, total_length, &fastContext->tls, &flags, tls_provisioning_callback)))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: parse provisioning id request."));
                return FALSE;
            }
            if (flags != ENCAPSULATED_ID_REQ_INCLUDED)
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: server encapsulated id req. not found."));
                return FALSE;
            }
        }
        break;
    case EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_CHALLENGE:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_CHALLENGE"));
        if ((ef_pkt->flags & ~EAPFAST_VERSION_BITS) != EAPFAST_LENGTHBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: malformed MSCHAPv2 challenge."));
            return FALSE;
        }
        /* We need to parse the whole message here and extract the fields
         * before we know the message is OK. */
        {
            CsrUint32 flags;
            CsrUint32 total_length = (ef_pkt->u.lengthed.length[0] << 24)
                + (ef_pkt->u.lengthed.length[1] << 16)
                + (ef_pkt->u.lengthed.length[2] << 8)
                +  ef_pkt->u.lengthed.length[3];

            if (!(parse_tls(fastContext, ef_pkt->u.lengthed.data, total_length, &fastContext->tls, &flags, tls_provisioning_callback)))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: parse provisioning MSCHAPv2 challenge."));
                return FALSE;
            }
        }
        break;
    case EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_RESULT:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_RESULT"));
        if ((ef_pkt->flags & ~EAPFAST_VERSION_BITS) != EAPFAST_LENGTHBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: malformed MSCHAPv2 result."));
            return FALSE;
        }
        /* We need to parse the whole message here and extract the fields
         * before we know the message is OK. */
        {
            CsrUint32 flags;
            CsrUint32 total_length = (ef_pkt->u.lengthed.length[0] << 24)
                + (ef_pkt->u.lengthed.length[1] << 16)
                + (ef_pkt->u.lengthed.length[2] << 8)
                +  ef_pkt->u.lengthed.length[3];

            if (!(parse_tls(fastContext, ef_pkt->u.lengthed.data, total_length, &fastContext->tls, &flags, tls_provisioning_callback)))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: parse provisioning MSCHAPv2 result."));
                return FALSE;
            }

            /* Check we have received all the things we were expecting to receive. */
            if (flags != ENCAPSULATED_MSCHAPV2_SUCCESS_INCLUDED)
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: NMSCHAPv2 result wrong content."));
                return FALSE;
            }
        }
        break;
    case EAPFAST_AWAIT_PROVISIONING_INTERMEDIATE_RESULT:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_PROVISIONING_INTERMEDIATE_RESULT"));
        if ((ef_pkt->flags & ~EAPFAST_VERSION_BITS) != EAPFAST_LENGTHBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: malformed Intermediate results."));
            return FALSE;
        }
        /* We need to parse the whole message here and extract the fields
         * before we know the message is OK. */
        {
            CsrUint32 flags;
            CsrUint32 total_length = (ef_pkt->u.lengthed.length[0] << 24)
                + (ef_pkt->u.lengthed.length[1] << 16)
                + (ef_pkt->u.lengthed.length[2] << 8)
                +  ef_pkt->u.lengthed.length[3];

            if (!(parse_tls(fastContext, ef_pkt->u.lengthed.data, total_length, &fastContext->tls, &flags, tls_provisioning_callback)))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: parse provisioning Intermediate results."));
                return FALSE;
            }

            /* Check we have received all the things we were expecting to receive. */
            if (flags != (ENCAPSULATED_CRYPTO_BINDING_NONCE_INCLUDED | ENCAPSULATED_INTERMEDIATE_RESULT_INCLUDED))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: Intermediate results wrong."));
                return FALSE;
            }
        }
        break;
    case EAPFAST_AWAIT_PROVISIONING_PAC:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_PROVISIONING_PAC"));
        if ((ef_pkt->flags & ~EAPFAST_VERSION_BITS) != EAPFAST_LENGTHBIT)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: malformed PAC message."));
            return FALSE;
        }
        /* We need to parse the whole message here and extract the fields
         * before we know the message is OK. */
        {
            CsrUint32 flags;
            CsrUint32 total_length = (ef_pkt->u.lengthed.length[0] << 24)
                + (ef_pkt->u.lengthed.length[1] << 16)
                + (ef_pkt->u.lengthed.length[2] << 8)
                +  ef_pkt->u.lengthed.length[3];

            if (!(parse_tls(fastContext, ef_pkt->u.lengthed.data, total_length, &fastContext->tls, &flags, tls_provisioning_callback)))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: parse provisioning PAC."));
                return FALSE;
            }

            /* Check we have received all the things we were expecting to receive. */
            if (flags != (PAC_KEY_INCLUDED | PAC_OPAQUE_INCLUDED | PAC_AID_INCLUDED | ENCAPSULATED_RESULT_INCLUDED))
            {
                sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: PAC message contents wrong."));
                return FALSE;
            }

            if(encode_pac_file(fastContext))
            {
                fastContext->provisioned = TRUE;
                sme_trace_info((TR_SECURITY_LIB, "Installing PAC"));
                (fastContext->context->callbacks.installPac)(fastContext->context->externalContext,
                                                             fastContext->context->setupData.fast_pac,
                                                             fastContext->context->setupData.fast_pac_length);

            }
        }
        break;
    case EAPFAST_AWAIT_PROVISIONING_FAILURE:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_PROVISIONING_FAILURE"));
        /* Note: The reception of a "failure" here is expected and a normal
         * part of the provisioning protocol. It indicates successful
         * provisioning. */
        if (eapReqData->code != EAP_CODE_FAILURE)
        {
            sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: PAC failure not received."));
            return FALSE;
        }
        break;
    case EAPFAST_AWAIT_PHASE2_ID:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_START"));
        if (!check_phase2_id(fastContext, eapReqData, size, tls_provisioning_callback))
            return FALSE;
        break;
    case EAPFAST_AWAIT_GTC_CRYPTO_BINDING:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_GTC_CRYPTO_BINDING"));
        if (!check_phase2_crypto_bind(context, eapReqData, size, tls_provisioning_callback))
            return FALSE;
        break;
    case EAPFAST_AWAIT_GTC_ACCESS_ACCEPT:
        sme_trace_info((TR_SECURITY_LIB, "EapFastCheck() :: EAPFAST_AWAIT_GTC_ACCESS_ACCEPT"));
        break;
    default:
        sme_trace_error((TR_SECURITY_LIB, "EapFastCheck() :: bad state."));
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Process an EAP-FAST message from the AP.
 *
 * @pre EapFastCheck() has been called to ensure the message's validity.
 *
 * @post processing has been done ready for EapFastBuild() to assemble a response.
 *
 * @param[in]   eapReqData : the EAP message received from the AP.
 */
void EapFastProcess(eapMethod* context, eap_packet *eapReqData, CsrUint8 reqId)
{
    FastContext* fastContext = (FastContext*)context->methodContext;

    switch(fastContext->state)
    {
    case EAPFAST_AWAIT_START:
        process_eapfast_server_start(fastContext);
        break;
    case EAPFAST_AWAIT_SERVER_HELLO:
        /*process_eapfast_server_hello(eapReqData);*/
        break;
    case EAPFAST_AWAIT_PROVISIONING_SERVER_HELLO:
        process_provisioning_server_hello(fastContext);
        break;
    case EAPFAST_AWAIT_PROVISIONING_CHANGE_CYPHER_SPECS:
    case EAPFAST_AWAIT_PROVISIONING_ID_REQ:
    case EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_CHALLENGE:
    case EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_RESULT:
    case EAPFAST_AWAIT_PROVISIONING_INTERMEDIATE_RESULT:
    case EAPFAST_AWAIT_PROVISIONING_PAC:
    case EAPFAST_AWAIT_PROVISIONING_FAILURE:
    case EAPFAST_AWAIT_PHASE2_ID:
    case EAPFAST_AWAIT_GTC_CRYPTO_BINDING:
    case EAPFAST_AWAIT_GTC_ACCESS_ACCEPT:
        /* No further processing to do in these states */
        break;
    default:
        sme_trace_error((TR_SECURITY_LIB, "EapFastProcess() :: bad state."));
        break;
    }
}

/**
 * @brief Assemble a message, if necessary, to send to the AP.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet (if length = zero then
 *   no packet).
 */
DataBuffer EapFastBuildResp(eapMethod* context, CsrUint8 reqId)
{
    FastContext* fastContext = (FastContext*)context->methodContext;
    DataBuffer ret = {0, NULL};

    switch(fastContext->state)
    {
    case EAPFAST_AWAIT_START:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_START"));
        if (fastContext->provisioned == FALSE)
        {
            /* Use the ADHP provisioning protocol to get a PAC */
            ret = build_eapfast_provisioning_client_hello(fastContext, reqId);
            fastContext->state = EAPFAST_AWAIT_PROVISIONING_SERVER_HELLO;
        }
        else
        {
            /* We already have a PAC, so we can go straight to the authentication
             * protocol. */
            ret = build_eapfast_client_hello(fastContext, reqId);
            fastContext->state = EAPFAST_AWAIT_SERVER_HELLO;
        }
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_SERVER_HELLO:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_SERVER_HELLO"));
        ret = build_eapfast_client_finished(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_PHASE2_ID;
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_PROVISIONING_SERVER_HELLO:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_PROVISIONING_SERVER_HELLO"));
        ret = build_eapfast_provisioning_client_key_exchange(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_PROVISIONING_CHANGE_CYPHER_SPECS;
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_PROVISIONING_CHANGE_CYPHER_SPECS:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_PROVISIONING_CHANGE_CYPHER_SPECS"));
        ret = build_eapfast_provisioning_client_ack(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_PROVISIONING_ID_REQ;
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_PROVISIONING_ID_REQ:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_PROVISIONING_ID_REQ"));
        ret = build_eapfast_provisioning_id_resp(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_CHALLENGE;
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_CHALLENGE:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_CHALLENGE"));
        ret = build_eapfast_provisioning_mschap_resp(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_RESULT;
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_RESULT:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_PROVISIONING_MSCHAPv2_RESULT"));
        ret = build_eapfast_provisioning_mschap_success_response(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_PROVISIONING_INTERMEDIATE_RESULT;
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_PROVISIONING_INTERMEDIATE_RESULT:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_PROVISIONING_INTERMEDIATE_RESULT"));
        ret = build_eapfast_provisioning_intermediate_response(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_PROVISIONING_PAC;
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_PROVISIONING_PAC:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_PROVISIONING_PAC"));
        ret = build_eapfast_provisioning_pac_ack(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_PROVISIONING_FAILURE;
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_PROVISIONING_FAILURE:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_PROVISIONING_FAILURE"));
        /* Place holder for when Cisco define a method for authenticating
         * after provisioning without an authentication failure. */
        ret = build_eapfast_poke_start(reqId);

        /* The correct result of a successful provisioning is a failed association
         * (so says Cisco). Need to do a new 802.11 auth/association again with our
         * new PAC */
        context->decision = FAIL;
        context->methodState = DONE;
        fastContext->state = EAPFAST_AWAIT_START;
        break;
    case EAPFAST_AWAIT_PHASE2_ID:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_PHASE2_ID"));
        ret = build_eapfast_gtc_response(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_GTC_CRYPTO_BINDING;
        context->methodState = CONT;
        break;
    case EAPFAST_AWAIT_GTC_CRYPTO_BINDING:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_GTC_CRYPTO_BINDING"));
        ret = build_eapfast_crypto_binding_response(fastContext, reqId);
        fastContext->state = EAPFAST_AWAIT_GTC_ACCESS_ACCEPT;
        context->decision = COND_SUCC; /* We can succeed now if we like! */
        context->methodState = DONE;
        break;
    case EAPFAST_AWAIT_GTC_ACCESS_ACCEPT:
        sme_trace_info((TR_SECURITY_LIB, "EapFastBuildResp() :: EAPFAST_AWAIT_GTC_ACCESS_ACCEPT"));
        /* If we get here it is because of PAC refresh. */
        break;
    default:
        sme_trace_error((TR_SECURITY_LIB, "EapFastBuildResp() :: bad state."));
        break;
    }

    return ret;
}

/**
 * @brief Return a key to the caller.
 *
 * @pre
 *   The caller must check isKeyAvailable before calling this function.
 *   isKeyAvailable must be TRUE or the return value is undefined.
 *
 * @return encryption key.
 */
static eapKey *EapFastGetKey(eapMethod* context)
{
    FastContext* fastContext = (FastContext*)context->methodContext;
    return &fastContext->key;
}

static void EapFastDeinit(eapMethod* context)
{
    if(((FastContext *)context->methodContext)->provisioned)
    {
        sme_trace_info((TR_SECURITY_LIB, "EapFastDeinit() :: provisioned"));
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "EapFastDeinit() :: not provisioned"));
    }
    CsrPfree(context->methodContext);
    context->methodContext = NULL;
}

/* Our one and only EAP-FAST plug-in method.
 * This is the method interface for use by the EAP state machine. */
static const eapMethod fast_method =
{
    NULL,  /* Pointer to local Context */
    NONE,  /* state */
    FAIL,  /* decision */
    FALSE, /* allow notifications */
    FALSE, /* isKeyAvaiable */
    FALSE, /* isEapolKeyHint */
    EapFastDeinit,
    EapFastCheck,
    EapFastProcess,
    EapFastBuildResp,
    EapFastGetKey
};

/**
 * @brief Initialize the EAP-FAST method state machine.
 */
void FastEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method)
{
    FastContext* fastContext = (FastContext*) CsrPmalloc(sizeof(FastContext));

    sme_trace_info((TR_SECURITY_LIB, "FastEapMethodInit()"));

    CsrMemSet(fastContext, 0, sizeof(FastContext));

    CsrMemCpy(method, &fast_method, sizeof(fast_method));
    method->methodContext = fastContext;

    fastContext->tls.md5_ctx = CsrCryptoMd5Init();
    fastContext->tls.sha1_ctx = CsrCryptoSha1Init();

    method->methodState = INIT;
    method->isKeyAvailable = FALSE;

    fastContext->context = context;
    fastContext->state = EAPFAST_AWAIT_START;

    fastContext->PAC_opaque_length = 0;

    /* todo: locate pac file properly and index using AID */
    fastContext->provisioned = decode_pac_file(fastContext, NULL);
    if (fastContext->provisioned)
    {
        sme_trace_info((TR_SECURITY_LIB, "EapFastInit(): fastContext->provisioned = TRUE"));
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "EapFastInit(): fastContext->provisioned = FALSE"));
    }

    fastContext->tls.cypher_suite = 0;
}
