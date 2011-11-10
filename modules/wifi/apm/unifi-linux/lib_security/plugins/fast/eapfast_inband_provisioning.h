/** @file eapfast_inband_provisioning.h
 *
 * Definitions for a suite of functions to handing in-band provisioning
 * messages and generate responses.
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
 *   Definitions for a suite of functions to handing in-band provisioning
 *   messages and generate responses. In-band provisioning happens over the
 *   air, as a "phase 0" before the actual authentication begins. It is an
 *   alternative to out-band provisioning.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/fast/eapfast_inband_provisioning.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_EAPFAST_IB_PROV_H
#define SECURITY_METHOD_EAPFAST_IB_PROV_H

#include "security_method_eapfast.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup <name> as defined in sme/docs/doxygen.h
 */

/**
 * @brief Do any processing on the data from the server hello in order to
 *        allow us to assemble a client key exchange.
 */
void process_provisioning_server_hello(FastContext* fastContext);

/**
 * @brief Assemble EAP-FAST provisioning client hello message.
 *
 * @par Description
 *      This is assembles a client hello of the type to instruct the AP to
 *      commence an ADHP provisioning protocol. We have no PAC in the message
 *      here. A PAC will be delivered to us later in the exchange.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
DataBuffer build_eapfast_provisioning_client_hello(FastContext *fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the provisioning client key exchange message.
 *
 * @par Description
 *      The client key exchange message contains the client key exchange,
 *      a change cipher specs record and an encrypted finished indication.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
DataBuffer build_eapfast_provisioning_client_key_exchange(FastContext* fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the provisioning client acknowledge message.
 *
 * @par Description
 *      The acknowledge message contains a simple success code.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
DataBuffer build_eapfast_provisioning_client_ack(FastContext* fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the provisioning id response message.
 *
 * @par Description
 *      The id response message contains the username.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
DataBuffer build_eapfast_provisioning_id_resp(FastContext* fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the provisioning MSCHAPv2 response message.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
DataBuffer build_eapfast_provisioning_mschap_resp(FastContext* fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the provisioning MSCHAPv2 success response message.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
DataBuffer build_eapfast_provisioning_mschap_success_response(FastContext* fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the provisioning intermediate response and crypto bind message.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
DataBuffer build_eapfast_provisioning_intermediate_response(FastContext* fastContext, CsrUint8 reqId);

/**
 * @brief Assemble the provisioning PAC acknowledgment and result message.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
DataBuffer build_eapfast_provisioning_pac_ack(FastContext* fastContext, CsrUint8 reqId);

/**
 * @brief Assemble an EAP start message, in order to poke the AS into starting authentication.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
DataBuffer build_eapfast_poke_start(CsrUint8 reqId);

/**
 * @brief The TLS callback for all provisioning application-specific data.
 *
 * @par Description
 *      The TLS routines only handle TLS standard data. All application
 *      specific data is passed to a callback function for processing.
 *      This is the callback for all EAP-FAST provisioning data.
 *
 * @return
 *      TRUE = processed ok. FALSE = error.
 *
 * @param[in/out]      data : pointer to the start of the (deciphered) application data.
 *                            The callback function should advance this to the first byte
 *                            after the application data on return.
 * @param[in]          size : size of the data buffer following the data parameter.
 *                            Should be at least as big as the actual application data.
 * @param[out] update_flags : flags to indicate which fields of tls have been
 *                            modified and what content has been discovered in
 *                            the packet.
 * @param[in]          tls  : the tls context structure
 */
CsrBool tls_provisioning_callback(void* fastContext, CsrUint8 **data, CsrUint16 size, CsrUint32 *update_flags, tls_data *tls);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_METHOD_EAPFAST_IB_PROV_H */
