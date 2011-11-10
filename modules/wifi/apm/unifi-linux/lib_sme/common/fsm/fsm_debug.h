/** @file fsm_debug.h
 *
 * Debug FSM header
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
 *   FSM header for the non public FSM code
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/fsm/fsm_debug.h#1 $
 *
 ****************************************************************************/
#ifndef FSM_DEBUG_H
#define FSM_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif


/** @{
 * @ingroup fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   function that installs the contexts on create callback function
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG
extern void fsm_install_on_create_callback(FsmContext* context, FsmOnCreateFnPtr callback);
#else
#define fsm_install_on_create_callback(context, callback)
#endif

/**
 * @brief
 *   function that installs the contexts event transition callback function
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG
extern void fsm_install_on_transition_callback(FsmContext* context, FsmOnTransitionFnPtr callback);
#else
#define fsm_install_on_transition_callback(context, callback)
#endif

/**
 * @brief
 *   function that installs the contexts event unhandled callback function
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG
extern void fsm_install_unhandled_event_callback(FsmContext* context, FsmOnTransitionFnPtr callback);
#else
#define fsm_install_unhandled_event_callback(context, callback)
#endif

/**
 * @brief
 *   function that installs the contexts State Change callback function
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG
extern void fsm_install_on_state_change_callback(FsmContext* context, FsmOnStateChangeFnPtr callback);
#else
#define fsm_install_on_state_change_callback(context, callback)
#endif

/**
 * @brief
 *   function that installs the contexts ignore event callback function
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG
extern void fsm_install_ignore_event_callback(FsmContext* context, FsmOnEventFnPtr callback);
#else
#define fsm_install_ignore_event_callback(context, callback)
#endif

/**
 * @brief
 *   function that installs the contexts save event callback function
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG
extern void fsm_install_save_event_callback(FsmContext* context, FsmOnEventFnPtr callback);
#else
#define fsm_install_save_event_callback(context, callback)
#endif

/**
 * @brief
 *   function that installs the contexts error event callback function
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG
extern void fsm_install_error_event_callback(FsmContext* context, FsmOnEventFnPtr callback);
#else
#define fsm_install_error_event_callback(context, callback)
#endif

/**
 * @brief
 *   function that installs the contexts invalid event callback function
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG
extern void fsm_install_invalid_event_callback(FsmContext* context, FsmOnEventFnPtr callback);
#else
#define fsm_install_invalid_event_callback(context, callback)
#endif

/**
 * @brief
 *   Dumps a report using TR_FSM_DUMP and TR_CRIT
 *
 * @param[in]  context     : FSM to report on
 * @param[in]  fullFatDump : Verbose trace...
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG_DUMP
extern void fsm_debug_dump_internal(FsmContext *context, CsrBool fullFatDump, const char* file, CsrUint32 line);
#define fsm_debug_dump(context) fsm_debug_dump_internal(context, TRUE, __FILE__, __LINE__)
#define fsm_debug_dump_lite(context) fsm_debug_dump_internal(context, FALSE, __FILE__, __LINE__)
#else
#define fsm_debug_dump(context)
#define fsm_debug_dump_lite(context)
#endif

/**
 * @brief
 *   Fast forwards the fsm timers by ms Milliseconds
 *
 * @param[in]  context : FSM context
 * @param[in]  ms      : Milliseconds to fast forward by
 *
 * @return
 *   void
 */
#ifdef FSM_TEST_SUPPORT
extern void fsm_fast_forward(FsmContext *context, CsrUint16 ms);
#else
#define fsm_fast_forward(context, ms)
#endif


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* FSM_PRIVATE_H */

