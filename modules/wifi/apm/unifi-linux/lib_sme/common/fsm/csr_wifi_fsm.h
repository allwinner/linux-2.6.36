/** @file fsm.h
 *
 * Public FSM header
 *
 * @section LEGAL
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   FSM header for the public FSM api
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/fsm/csr_wifi_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef FSM_H
#define FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/**
 * @brief
 *   Toplevel FSM context data
 *
 * @par Description
 *   Holds ALL FSM static and dynamic data for a FSM
 */
typedef struct FsmContext FsmContext;

/**
 * @brief
 *   FSM event header.
 *
 * @par Description
 *   All events MUST have this struct as the FIRST member.
 *   The next member is used internally for linked lists
 */
typedef struct FsmEvent
{
    struct FsmEvent *next;
    CsrUint16 id;
    CsrUint16 destination;
    CsrUint16 sender_; /* trailing underscore to avoid clash with #define */
} FsmEvent;

/**
 * @brief
 *   FSM timer Info
 *
 * @par Description
 *   Min timeout time and allowable extra time before timeout
 */
typedef struct FsmTimerData
{
    CsrUint32 timeoutTimeMs;
    CsrUint16 timeoutTimeExtraMs;
} FsmTimerData;

/**
 * @brief
 *   FSM External Wakeup CallbackFunction Pointer
 *
 * @par Description
 *   Defines the external wakeup function for the FSM
 *   to call when an external event is injected into the systen
 *
 * @param[in]    context : External context
 *
 * @return
 *   void
 */
typedef void (*FsmExternalWakupCallbackPtr) (void* context);

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Initialises a top level FSM context
 *
 * @par Description
 *   Initialises the FSM Context to an initial state and allocates
 *   space for "maxProcesses" number of instances
 *
 * @param[in]    osaContext         : OSA context
 * @param[in]    applicationContext : Internal fsm application context
 * @param[in]    externalContext    : External context
 * @param[in]    maxProcesses       : Max processes to allocate room for
 *
 * @return
 *   FsmContext* fsm context
 */
extern FsmContext* fsm_init_context(void* applicationContext, void* externalContext, CsrUint16 maxProcesses);

/**
 * @brief
 *   Resets the FSM's back to first conditions
 *
 * @par Description
 *   This function is used to free any dynamic resources allocated for the
 *   given context by fsm_init_context().
 *   The FSM's reset function is called to cleanup any fsm specific memory
 *   The reset funtion does NOT need to free the fsm data pointer as
 *   fsm_shutdown() will do it.
 *   the FSM's init function is call again to reinitialise the FSM context.
 *   fsm_reset() should NEVER be called when fsm_execute() is running.
 *
 * @param[in]    context    : FSM context
 *
 * @return
 *   void
 */
extern void fsm_reset(FsmContext* context);

/**
 * @brief
 *   Frees resources allocated by fsm_init_context
 *
 * @par Description
 *   This function is used to free any dynamic resources allocated for the
 *   given context by fsm_init_context(), prior to complete termination of
 *   the program.
 *   The FSM's reset function is called to cleanup any fsm specific memory.
 *   The reset funtion does NOT need to free the fsm data pointer as
 *   fsm_shutdown() will do it.
 *   fsm_shutdown() should NEVER be called when fsm_execute() is running.
 *
 * @param[in]    context       : FSM context
 *
 * @return
 *   void
 */
extern void fsm_shutdown(FsmContext* context);

/**
 * @brief
 *   Executes the fsm context
 *
 * @par Description
 *   Executes the FSM context and runs until ALL events in the context are processed.
 *   When no more events are left to process then fsm_execute() returns to a time
 *   specifying when to next call the fsm_execute()
 *   Scheduling, threading, blocking and external event notification are outside
 *   the scope of the FSM and fsm_execute().
 *
 * @param[in]    context  : FSM context
 *
 * @return
 *   FsmTimerData absolute time + allowed variation for next timeout OR 0xFFFFFFFF if no timers are set
 */
extern FsmTimerData fsm_execute(FsmContext* context);

/**
 * @brief
 *   Adds an event to the FSM context's external event queue for processing
 *
 * @par Description
 *   Adds an event to the contexts external queue
 *   This is thread safe and adds an event to the fsm's external event queue.
 *
 * @param[in]    context      : FSM context
 * @param[in]    event        : event to add to the event queue
 * @param[in]    destination  : destination of the event
 * @param[in]    id           : event id
 *
 * @return
 *   void
 */
extern void fsm_send_event_external(FsmContext *context, FsmEvent* event, CsrUint16 destination, CsrUint16 id);

/**
 * @brief
 *   Gets the time until the next FSM timer expiry
 *
 * @par Description
 *   Returns the next timeout time or 0 if no timers are set.
 *
 * @param[in]    context    : FSM context
 *
 * @return
 *   FsmTimerData The absolute time and allowed extra for the next timer expiry or 0xFFFFFFFF if no timers are set
 */
extern FsmTimerData fsm_get_next_timeout(FsmContext *context);


/**
 * @brief
 *    Check if the fsm has events to process
 *
 * @param[in]    context    : FSM context
 *
 * @return
 *   CsrBool returns TRUE if there are events for the FSM to process
 */
extern CsrBool fsm_has_events(FsmContext* context);

/**
 * @brief
 *   function that installs the contexts wakeup function
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
extern void fsm_install_wakeup_callback(FsmContext* context, FsmExternalWakupCallbackPtr callback);


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* FSM_H */

