/** @file nme_primitive_forwarding_utils.h
 *
 * Public NME Primitive Forwarding Utils API
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
 *   Public NME Primitive Forwarding Utils API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_signal_routing_fsm/nme_primitive_forwarding_utils.h#1 $
 *
 ****************************************************************************/
#ifndef NME_PRIMITIVE_FORWARDING_UTILS_H_
#define NME_PRIMITIVE_FORWARDING_UTILS_H_
#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"

    /* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

/**
 * @brief
 *   Forwards the received CSR_WIFI_NME event to the UNIFI_MGT SAP
 *
 * @par Description
 *   Forwards the received CSR_WIFI_NME event to the UNIFI_MGT SAP
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
extern void nme_forward_to_unifi_mgt_sap(
        FsmContext* pContext,
        const FsmEvent* pReq);

/**
 * @brief
 *   Forwards the received UNIFI_MGT SAP event to the CSR_WIFI_NME SAP
 *
 * @par Description
 *   Forwards the received UNIFI_MGT event to the CSR_WIFI_NME SAP
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    FsmEvent* : Event
 *
 * @return
 *   void
 */
extern void nme_forward_to_csr_wifi_nme_sap(
        FsmContext* pContext,
        const FsmEvent* pEvt);

#ifdef __cplusplus
}
#endif

#endif /* NME_PRIMITIVE_FORWARDING_UTILS_H_ */
