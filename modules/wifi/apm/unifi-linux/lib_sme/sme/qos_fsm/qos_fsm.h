/** @file qos_fsm.h
 *
 * Public SME quality of service FSM API
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
 *   Public SME quality of service FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef SME_QOS_FSM_H
#define SME_QOS_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup qos
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the sme qos state machine data
 */
extern const FsmProcessStateMachine qos_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Returns the TSpec Control Block
 *
 * @par Description
 * Counts the SSIDs in a CCX SSIDL IE
 *
 * @param[in] context : buffer containing a SSIDL IE
 *
 * @return
 *    qos_tspecDataBlk : The TSPEC control block
 */
extern qos_tspecDataBlk* qos_get_tspecdatablk_context(
        FsmContext* context);


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_QOS_FSM_H */
