/** @file mib_set_sub_fsm.h
 *
 * Public MIB Set Sub FSM API
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2007. All rights reserved.
 *
 * @section DESCRIPTION
 *   Public MIB Set Sub FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mib_action_sub_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef MIB_GET_SUB_FSM_H
#define MIB_GET_SUB_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup mib_access
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/
typedef void (*mibGetCompleteFnPtr)(FsmContext* context, const MlmeGetCfm_Evt* cfm);
typedef void (*mibGetNextCompleteFnPtr)(FsmContext* context, const MlmeGetNextCfm_Evt* cfm);
typedef void (*mibSetCompleteFnPtr)(FsmContext* context, const MlmeSetCfm_Evt* cfm);

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Start the sub fsm to handle the mib get
 *
 * @param[in]  context     : FSM context
 * @param[in]  dataRef     : Mib encoded data ref for get
 * @param[in]  sendToHipSap : TRUE  = Send the mlme_get_req direct to the FW
 *                          : FALSE = Send the mlme_get_req via mib_access_fsm
 *
 * @return
 *   void
 */
extern void mib_get_sub_fsm_start(FsmContext* context, mibGetCompleteFnPtr completeFn, const DataReference* dataRef, CsrBool sendToHipSap);

/**
 * @brief
 *   Start the sub fsm to handle the mib getnext
 *
 * @param[in]  context     : FSM context
 * @param[in]  dataRef     : Mib encoded data ref for getnext
 * @param[in]  sendToHipSap : TRUE  = Send the mlme_getnext_req direct to the FW
 *                          : FALSE = Send the mlme_getnext_req via mib_access_fsm
 *
 * @return
 *   void
 */
extern void mib_getnext_sub_fsm_start(FsmContext* context, mibGetNextCompleteFnPtr completeFn, const DataReference* dataRef, CsrBool sendToHipSap);

/**
 * @brief
 *   Start the sub fsm to handle the mib set
 *
 * @param[in]  context      : FSM context
 * @param[in]  dataRef      : Mib encoded data ref for set
 * @param[in]  sendToHipSap : TRUE  = Send the mlme_set_req direct to the FW
 *                          : FALSE = Send the mlme_set_req via mib_access_fsm
 *
 * @return
 *   void
 */
extern void mib_set_sub_fsm_start(FsmContext* context, mibSetCompleteFnPtr completeFn, const DataReference* dataRef, CsrBool sendToHipSap);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* MIB_GET_SUB_FSM_H */
