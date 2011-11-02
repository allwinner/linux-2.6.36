/** @file eapfast_authentication.h
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
 *   A suite of functions to handing authentication messages and generate
 *   responses.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/fast/eapfast_authentication.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_EAPFAST_AUTH_H
#define SECURITY_METHOD_EAPFAST_AUTH_H

#include "security_method_eapfast.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup ie_access
 */

/**
 * @brief Build a message authentication code of a TLS message.
 *
 * @par Description
 *      This takes a TLS message and adds a MAC code to the end of it.
 *      MAC code always placed immediately after the TLS message so the data
 *      buffer needs to have 20 octets of extra space on the end to take it.
 *
 * @param[in]   msg_type : The TLS content type of the message
 * @param[in/out]   data : The TLS message and room for the MAC
 * @param[in]   length : length of the message (not including MAC code). The
 *                       MAC code is placed AFTER this length in the data buffer.
 * @param[in]   write_MAC_secret : The MAC write secret to use in generating
 *                                 the MAC. This should be obtained from the
 *                                 appropriate key block in use at the time.
 */
    CsrBool build_sha1_mac(FastContext *fastContext, CsrUint8 msg_type, CsrUint8 *data, CsrUint32 length, CsrUint8 *write_MAC_secret);
/**
 * @brief Check the validity of an expected phase 2 crypto bind message.
 *
 * @param[in]   eapReqData : The EAP packet to verify.
 * @param[in]   size : The size of the EAP packet.
 *
 * @return
 *   TRUE = packet ok, FALSE = error.
 */
    CsrBool check_phase2_crypto_bind(eapMethod *context, eap_packet *eapReqData, CsrUint32 size, parse_tls_callback callback);

/**
 * @brief Check the validity of an expected phase 2 id message.
 *
 * @param[in]   eapReqData : The EAP packet to verify.
 * @param[in]   size : The size of the EAP packet.
 *
 * @return
 *   TRUE = packet ok, FALSE = error.
 */
    CsrBool check_phase2_id(FastContext* fastContext, eap_packet *eapReqData, CsrUint32 size, parse_tls_callback callback);

/**
 * @brief Process an EAP-FAST server start message received from the Access Point.
 *
 * @par Description
 *   We will need to respond to this with a client hello. We don't build that
 *   here, but we do prepare the data we will need to assemble it later.
 */
    void process_eapfast_server_start(FastContext* fastContext);

/**
 * @brief Assemble the regular EAP-FAST client hello message.
 *
 * @par Description
 *      This function builds a normal EAP-FAST client hello containing a
 *      PAC which we had previously obtained using in-band or out-band methods.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
    DataBuffer build_eapfast_client_hello(FastContext* fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the regular EAP-FAST client finished message.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
    DataBuffer build_eapfast_client_finished(FastContext* fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the EAP-FAST Phase 2 GTC client response message.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
    DataBuffer build_eapfast_gtc_response(FastContext *fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the EAP-FAST Phase 2 Crypto Binding response message.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
    DataBuffer build_eapfast_crypto_binding_response(FastContext* fastContext, CsrUint8 reqId);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_METHOD_EAPFAST_AUTH_H */
