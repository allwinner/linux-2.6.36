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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_action_sub_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef QOS_SUB_FSM_H
#define QOS_SUB_FSM_H

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
typedef void (*qosTspecAddCompleteFnPtr)(FsmContext* context, const MlmeAddtsCfm_Evt* cfm);
typedef void (*qosTspecDelCompleteFnPtr)(FsmContext* context, const MlmeDeltsCfm_Evt* cfm);

typedef void (*qosTclasAddCompleteFnPtr)(FsmContext* context, const UnifiSysTclasAddCfm_Evt* cfm);
typedef void (*qosTclasDelCompleteFnPtr)(FsmContext* context, const UnifiSysTclasDelCfm_Evt* cfm);

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Start the sub fsm to handle the mib get
 *
 * @param[in]  context     : FSM context
 * @param[in]  dataRef     : Mib encoded data ref for get
 *
 * @return
 *   void
 */
extern void qos_del_tspec_fsm_start(
        FsmContext* context,
        qosTspecDelCompleteFnPtr completeFn,
        const CsrUint32 tsinfo);


/**
 * @brief
 *   Start the sub fsm to handle the mib set
 *
 * @param[in]  context      : FSM context
 * @param[in]  dataRef      : Mib encoded data ref for set
 *
 * @return
 *   void
 */
extern void qos_add_tspec_fsm_start(
        FsmContext* context,
        qosTspecAddCompleteFnPtr completeFn,
        const DataReference dataRef,
        DialogToken dialogToken);

/**
 * @brief
 *   Start the sub fsm to handle the mib get
 *
 * @param[in]  context     : FSM context
 * @param[in]  dataRef     : Mib encoded data ref for get
 *
 * @return
 *   void
 */
extern void qos_del_tclas_fsm_start(
        FsmContext* context,
        qosTclasDelCompleteFnPtr completeFn,
        const CsrUint32 tsinfo);


/**
 * @brief
 *   Start the sub fsm to handle the mib set
 *
 * @param[in]  context      : FSM context
 * @param[in]  dataRef      : Mib encoded data ref for set
 *
 * @return
 *   void
 */
extern void qos_add_tclas_fsm_start(
        FsmContext* context,
        qosTclasAddCompleteFnPtr completeFn,
        const CsrUint16 tclasLength,
        const CsrUint8 *tclas);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* QOS_SUB_FSM_H */
