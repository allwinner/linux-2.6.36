/** @file csr_handshake.h
 *
 * Definitions for WPA Personal library.
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
 *   This provides an implementation of WPA Personal library
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_handshake/csr_handshake.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_HANDSHAKE_H
#define CSR_HANDSHAKE_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_security_private.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PUBLIC CONSTANT DEFINITIONS *********************************************/

/* PUBLIC TYPE DEFINITIONS ************************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/
/**
 * @brief
 *   Init function for the handshake library
 *
 * @par Description
 *   Called during init, this function initializes module data structures
 *
 * @param
 *   CsrWifiSecurityContext* : Top level security context
 *
 * @return
 *   CsrWpaPersonalCtx* : Newly allocated WPA personal Context
 */

CsrWpaPersonalCtx* csr_handshake_init(CsrWifiSecurityContext* context);

/**
 * @brief
 *   Function to initiate the handshake procedure
 *
 * @par Description
 *   Called after init, this function starts handshake procedure by
 *   sending M1 of 4 way handshake if mode is authenticator
 *
 * @param
 *   CsrWpaPersonalCtx *: Context obtained during init
 *
 * @return
 *   void
 */
void csr_handshake_start(CsrWpaPersonalCtx *context);

/**
 * @brief
 *   Entry point for EAPOL Key packets
 *
 * @par Description
 *   Called to process EAPOL Key packets
 *
 * @param
 *   CsrWpaPersonalCtx *: Context obtained during init
 *   eapol_packet*      : EAPOL key packet
 *   CsrUint16             : Length of the key packet
 *
 * @return
 *   void
 */
void csr_handshake_pkt_input(CsrWpaPersonalCtx *context, eapol_packet *pkt, CsrUint16 length);

/**
 * @brief
 *   Handle a timeout
 *
 * @par Description
 *   Called when a registered timeout has expired.
 *   Retransmission or abort happens.
 *
 * @param
 *   CsrWpaPersonalCtx *: Context obtained during init
 *
 * @return
 *   void
 */
void csr_handshake_timeout(CsrWpaPersonalCtx *context, CsrWifiSecurityTimeout timeoutIndex);

/**
 * @brief
 *   Send an EAPOL error
 *
 * @par Description
 *   Called to send an EAPOL error report frame in case of MIC failure.
 *
 * @param
 *   CsrWpaPersonalCtx *: Context obtained during init
 *   CsrUint8*          : Transmit sequence count of the packet that failed MIC
 *
 * @return
 *   void
 */
void csr_handshake_send_eapol_error (CsrWpaPersonalCtx *pCtx,  CsrBool pairwise, CsrUint8 *tsc);
/**
 * @brief
 *      Deinit of WPA personal module
 *
 * @par Description
 *   Frees any memory that is allocated and deinits the module
 *
 * @param
 *   CsrWpaPersonalCtx *: Context obtained during init
 *
 * @return
 *   void
 */
void csr_handshake_deinit(CsrWpaPersonalCtx *context);

#ifdef __cplusplus
}
#endif

#endif /* CSR_WPA_PERSONAL_H */
