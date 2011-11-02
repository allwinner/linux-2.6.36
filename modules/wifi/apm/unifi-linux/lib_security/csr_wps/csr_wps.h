/** @file csr_wps.h
 *
 * Definitions for wps interface.
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
 *   This provides an implementation of WPS module
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/csr_wps.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_WPS_H
#define CSR_WPS_H

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
 *   Entry point for WPS packets
 *
 * @par Description
 *   Called to process WPS Key packets
 *
 * @param
 *   CsrwpsContext *   : Context obtained during init
 *   CsrUint8*             : WPS packet
 *   CsrUint16             : Length of the packet
 *
 * @return
 *   void
 */
void csr_wps_pkt_input(CsrWpsContext *context, CsrUint8 *pkt, CsrUint16 length);

/**
 * @brief
 *   Init function for the WPS module
 *
 * @par Description
 *   Called during init, this function initializes module data structures
 *
 * @param
 *   CsrWifiSecurityContext* : Top level security context
 *
 * @return
 *   CsrWpsContext* : Newly allocated WPS Context
 */
CsrWpsContext* csr_wps_init(CsrWifiSecurityContext* context);

/**
 * @brief
 *      Deinit of WPS personal module
 *
 * @par Description
 *   Frees any memory that is allocated and deinits the module
 *
 * @param
 *   CsrWpsContext *: Context obtained during init
 *
 * @return
 *   void
 */
void csr_wps_deinit(CsrWpsContext *pCtx);

/**
 * @brief
 *   Handle a timeout
 *
 * @par Description
 *   Called when a registered timeout has expired.
 *   Retransmission or abort happens.
 *
 * @param
 *   CsrWpsContext *: Context obtained during init
 *   CsrWifiSecurityTimeout: Nature of the timeout
 * @return
 *   void
 */
void csr_wps_timeout(CsrWpsContext *pCtx, CsrWifiSecurityTimeout timeoutIndex);

/**
 * @brief
 *   Function to initiate the WPS procedure
 *
 * @par Description
 *   Called after init, this function starts WPS procedure by
 *   sending EAPOL start
 *
 * @param
 *   CsrWpsContext *: Context obtained during init
 *
 * @return
 *   void
 */
void csr_wps_start(CsrWpsContext *context);

#ifdef __cplusplus
}
#endif

#endif /* CSR_WPS_H */
