/** @file security_eap.h
 *
 * Definitions for the EAP state machine.
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
 *   This file defines the interface to the EAP state machine and contains EAP
 *   and EAPOL definitions used by the it and its plug-in methods.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_eap/csr_eap.h#2 $
 *
 ****************************************************************************/
#ifndef CSR_EAP_H
#define CSR_EAP_H

#include "csr_security_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Initialises the EAP state machine.
 *
 * @param[in] context : Security Context
 */
EapContext* eap_init(CsrWifiSecurityContext* context);

/**
 * @brief Called by the security manager FSM to give us a received EAP
 *        packet.
 *
 * @pre eap_init() must be called before this.
 *
 * @param[in] pkt  : incoming packet.
 * @param[in] size : incoming packet size.
 *
 * @return TRUE=handshakeEapol, FALSE=eapEapol
 */
CsrBool eap_pkt_input(EapContext *eap_context, eapol_packet *pkt, CsrUint16 size);

void eap_timeout(EapContext *eap_context, CsrWifiSecurityTimeout timeoutIndex);

void eap_deinit(EapContext *eap_context);

#ifdef __cplusplus
}
#endif

#endif /*CSR_EAP_H*/
