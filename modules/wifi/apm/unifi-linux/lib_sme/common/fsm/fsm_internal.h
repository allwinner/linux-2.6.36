/** @file fsm_internal.h
 *
 * Public FSM header
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
 *   FSM header for the public FSM api
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/fsm/fsm_internal.h#3 $
 *
 ****************************************************************************/
#ifndef FSM_INTERNAL_H
#define FSM_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "fsm/fsm_types.h"

/* PUBLIC MACROS ************************************************************/
#define FSM_START     (0x0000)
#define FSM_TERMINATE (0xFFFF)
#define FSM_ENV       (0xFFFF)

/**
 * @brief
 *   macro function that CsrPmalloc's a process specific data block
 *
 * @par Description
 *   Allocates a data block for the process.
 *
 * @param[in]  context : FSM context
 * @param[in]  type    : Type of the processes data block
 *
 * @return
 *   void
 */
#define fsm_create_params(context, type) ((context)->currentInstance->params = CsrPmalloc(sizeof(type)))

/**
 * @brief
 *   macro function that accesses and casts the process specific data
 *
 * @par Description
 *   Returns the process data cast to the correct pointer type
 *
 * @param[in]  context : FSM context
 * @param[in]  id      : FSM process instance id
 * @param[in]  type    : Type of the processes data block
 *
 * @return
 *   (type*) cast process data
 */
#define fsm_get_params_by_id(context, id, type) ((type*)(context)->instanceArray[id].params)

/**
 * @brief
 *   macro function that accesses a specific contest by ID
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context : FSM context
 * @param[in]  id      : FSM process instance id
 *
 * @return
 *   FsmContext* - requested context
 */
#define fsm_get_context_by_id(context, id) (&context->instanceArray[id])

/**
 * @brief
 *   macro function that accesses and casts the process specific data
 *
 * @par Description
 *   Returns the process data cast to the correct pointer type
 *
 * @param[in]    context : FSM context
 * @param[type]  type    : Type of the processes data block
 *
 * @return
 *   (type*) cast process data
 */
#define fsm_get_params(context, type) ((type*)(context)->currentInstance->params)

/**
 * @brief
 *   macro function that sets the current proecess's next state
 *
 * @par Description
 *   Sets the current processes state.
 *
 * @param[in]    context    : FSM context
 * @param[type]  nextstate  : New state for the process
 *
 * @return
 *   void
 */
#ifdef FSM_DEBUG
#define fsm_next_state(context, nextstate) \
    sme_trace_msc((TR_MSC, "MSC STATE :: Process(%s) OldState(%s) NewState(%s)", \
                           (context)->currentInstance->fsmInfo->processName, \
                           (context)->currentInstance->fsmInfo->transitionTable.aStateEventMatrix[context->currentInstance->state].stateName, \
                           nextstate!=FSM_TERMINATE?(context)->currentInstance->fsmInfo->transitionTable.aStateEventMatrix[nextstate].stateName:"FSM_TERMINATE")); \
    sme_trace_info((TR_FSM, "fsm_next_state() %s(%s) => %s", \
                            (context)->currentInstance->fsmInfo->processName, \
                            (context)->currentInstance->fsmInfo->transitionTable.aStateEventMatrix[context->currentInstance->state].stateName, \
                            nextstate!=FSM_TERMINATE?(context)->currentInstance->fsmInfo->transitionTable.aStateEventMatrix[nextstate].stateName:"FSM_TERMINATE")); \
    if ((context)->onStateChange != NULL) \
    { \
        (*(context)->onStateChange)(context, nextstate); \
    } \
    (context)->currentInstance->state = nextstate
#else
#define fsm_next_state(context, nextstate) ((context)->currentInstance->state = nextstate)
#endif

/**
 * @brief
 *   macro function that gets the current proecess's state
 *
 * @par Description
 *   Accessor for the current process's State
 *
 * @param[in]    context    : FSM context
 *
 * @return
 *   CsrUint32 The current process's state
 */
#define fsm_current_state(context) ((context)->currentInstance == NULL?FSM_TERMINATE:(context)->currentInstance->state)

/**
 * @brief
 *   macro function that gets the name of the current process
 *
 * @par Description
 *   Expands to a string containing the name of the process that is currently
 *   executing
 *
 * @param[in]    context    : FSM context
 *
 * @return
 *   char * The name of the current process's current state
 */
#ifdef FSM_DEBUG
#define fsm_current_process_name(context) ((context)->currentInstance == NULL?"ENV":(context)->currentInstance->fsmInfo->processName)
#else
#define fsm_current_process_name(context) ""
#endif

/**
 * @brief
 *   macro function that gets the name of a given process from its ID
 *
 * @par Description
 *   Expands to a string containing the name of the process that has the ID
 *   specified
 *
 * @param[in]    context    : FSM context
 * @param[in]    id         : The process ID
 *
 * @return
 *   char * The name of the current process's current state
 */
#ifdef FSM_DEBUG
#define fsm_process_name_by_id(context,id) ((id == FSM_ENV)?"ENV":context->instanceArray[id].fsmInfo->processName)
#else
#define fsm_process_name_by_id(context, id) ""
#endif

/**
 * @brief
 *   macro function that gets the current process's state name
 *
 * @par Description
 *   Expands to a string containing the name of the state that the current
 *   process is in.
 *
 * @param[in]    context    : FSM context
 *
 * @return
 *   char * The name of the current process's current state
 */
#ifdef FSM_DEBUG
#define fsm_current_state_name(context) (context->currentInstance->fsmInfo->transitionTable.aStateEventMatrix[context->currentInstance->state].stateName)
#else
#define fsm_current_state_name(context) ""
#endif

/**
 * @brief
 *   macro function that gets the process's state name
 *
 * @par Description
 *   Expands to a string containing the name of the state that the
 *   process is in.
 *
 * @param[in]    context    : FSM context
 * @param[in]    state      : state to get name of
 *
 * @return
 *   char * The name of the  process's current state
 */
#ifdef FSM_DEBUG
#define fsm_state_name(context,state) (context->currentInstance->fsmInfo->transitionTable.aStateEventMatrix[state].stateName)
#else
#define fsm_state_name(context,state) ""
#endif

/**
 * @brief
 *   macro function that builds a state table entry
 *
 * @par Description
 *   Returns a FsmTableEntry
 *
 * @param[in]    stateName    : State Name (For Trace)
 * @param[in]    tablename    : State transition table
 * @param[in]    save         : Save *?
 *
 * @return
 *   FsmTableEntry Table entry
 */
#ifdef FSM_DEBUG
#define fsm_state_table_entry(stateEnum, tablename, save) {(sizeof(tablename) / sizeof(FsmEventEntry)), save, tablename, stateEnum, #stateEnum}
#else
#define fsm_state_table_entry(stateEnum, tablename, save) {(sizeof(tablename) / sizeof(FsmEventEntry)), save, tablename}
#endif

/**
 * @brief
 *   macro function that builds a event table entry
 *
 * @par Description
 *   Returns a FsmEventEntry
 *
 * @param[in]    eventid        : EventId for the transition
 * @param[in]    transitionfn   : State transition table
 *
 * @return
 *   FsmEventEntry Table entry
 */
#ifdef FSM_DEBUG
#define fsm_event_table_entry(eventid, transitionfn) {eventid, (FsmTransitionFnPtr)transitionfn, #eventid}
#else
#define fsm_event_table_entry(eventid, transitionfn) {eventid, (FsmTransitionFnPtr)transitionfn}
#endif


/**
 * @brief
 *   macro function installs a application ignore event callback
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : callback function
 *
 * @return
 *   void
 */
#define fsm_install_app_ignore_event_callback(context, callback) (context)->appIgnoreCallback = callback;

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Creates a new instance of a process
 *
 * @par Description
 *   Creates a new Process instance an adds it to the FSM context's
 *   Instance list
 *
 * @param[in]    context  : FSM context
 * @param[in]    sminfo   : static FSM discription for this instance
 * @param[in]    callEntry: Call the processes entry function if it exists
 *
 * @return
 *   CsrUint16 instance number the newly created process
 */
extern CsrUint16 fsm_add_instance(FsmContext* context, const FsmProcessStateMachine* sminfo, CsrBool callEntry);

/**
 * @brief
 *   Creates a new sub fsm instance of a process
 *
 * @par Description
 *   Creates a new sub fsm instance an adds it to the current instance's
 *   sub fsm list
 *
 * @param[in]    context  : FSM context
 * @param[in]    sminfo   : static FSM discription for this instance
 *
 * @return
 *   FsmInstanceEntry* : New Sub instance entry data
 */
extern FsmInstanceEntry* fsm_add_sub_instance(FsmContext* context, const FsmProcessStateMachine* sminfo);

/**
 * @brief
 *   Calls the a function in the owner FSM's context
 *
 * @par Description
 *   1) Save the current instance pointer
 *   2) Set the current instance  = owners instance
 *   3) Call the function
 *   4) Restore the current instance pointer
 *
 * @param[in]    context  : FSM context
 * @param[in]    fnCall   : Code to call the owners function
 *
 */
#define fsm_call_parent_fn(context, fnCall) \
{ \
    FsmInstanceEntry* savedInstance = context->currentInstance; \
    context->currentInstance = context->ownerInstance; \
    fnCall; \
    context->currentInstance = savedInstance; \
}

/**
 * @brief
 *   Calls the complete function in the owners FSM
 *
 * @par Description
 *   1) Save the current instance pointer
 *   2) Set the current instance  = owners instance
 *   3) Call the function
 *   4) Restore the current instance pointer
 *
 * @param[in]    context  : FSM context
 * @param[in]    fnCall   : Code to call the owners function
 *
 */
#define fsm_call_sub_instance_complete(context, fnCall) fsm_call_parent_fn(context, fnCall)


/**
 * @brief
 *   Updates the current instance pointer
 *
 * @par Description
 *   1) returns the current instance
 *   2) Set the current instance = owners instance
 *   Usage : FsmInstanceEntry* savedInstance = fsm_save_current(context, &newInstance);
 *
 * @param[in]    context     : FSM context
 * @param[in]    newInstance : pointer to the new current instance
 *
 */
#define fsm_save_current_instance(context, newInstance) \
    context->currentInstance; \
    context->currentInstance = newInstance

/**
 * @brief
 *   Updates the current instance pointer
 *
 * @par Description
 *   Looks up the instance pointer by ID and calls fsm_save_current_instance
 *   Usage : FsmInstanceEntry* savedInstance = fsm_save_current(context, instanceId);
 *
 * @param[in]    context    : FSM context
 * @param[in]    instanceId : Instance Id of the new current instance
 *
 */
#define fsm_save_current_instance_by_id(context, newInstanceId) \
    fsm_save_current_instance(context, fsm_get_context_by_id(context, newInstanceId))

/**
 * @brief
 *   Restores the current instance pointer
 *
 * @par Description
 *   Set the current instance = saved instance
 *   Usage : fsm_restore_current(context, savedInstance);
 *
 * @param[in]    context     : FSM context
 * @param[in]    savedInstance : pointer to the instance to restore
 *
 */
#define fsm_restore_current_instance(context, savedInstance) \
    (context->currentInstance = savedInstance)

/**
 * @brief
 *   Calls all the instance's entry functions
 *
 * @par Description
 *   Each instances entry funtion is called.
 *   This function should be called AFTER all the calls to fsm_add_instance()
 *   and before calls to fsm_execute()
 *   Instance list
 *
 * @param[in]    context  : FSM context
 *
 * @return
 *   void
 */
extern void fsm_call_entry_functions(FsmContext* context);

/**
 * @brief
 *   Standard transition event for ignored events
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context      : FSM context
 * @param[in]    ignoreEvent  : Event to ignore
 *
 * @return
 *   void
 */
extern void fsm_ignore_event(FsmContext* context, const FsmEvent* ignoreEvent);

/**
 * @brief
 *   Standard transition event for saved events
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context    : FSM context
 * @param[in]    saveEvent  : Event to save
 *
 * @return
 *   void
 */
extern void fsm_saved_event(FsmContext* context, const FsmEvent* saveEvent);

/**
 * @brief
 *   Standard transition event for error events
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context    : FSM context
 * @param[in]    errorEvent : error Event
 *
 * @return
 *   void
 */
extern void fsm_error_event(FsmContext* context, const FsmEvent* errorEvent);

/**
 * @brief
 *   Standard transition event for invalid events
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context    : FSM context
 * @param[in]    saveEvent  : invalid Event
 *
 * @return
 *   void
 */
extern void fsm_invalid_event(FsmContext* context, const FsmEvent* invalidEvent);

/**
 * @brief
 *   Adds an event to the FSM context's event queue for processing
 *
 * @par Description
 *   Adds an event to the contexts internal queue
 *   This is not thread safe so should only be called from code that
 *   executes withing the fsm_execute() function call.
 *
 * @param[in]    context      : FSM context
 * @param[in]    event        : event to add to the event queue
 * @param[in]    destination  : destination of the event
 * @param[in]    id           : event id
 *
 * @return
 *   void
 */
extern void fsm_send_event(FsmContext *context, FsmEvent* event, CsrUint16 destination, CsrUint16 id);

/**
 * @brief
 *   Forwards an event to the FSM context's event queue for processing
 *
 * @par Description
 *   Adds an event to the contexts internal queue
 *   The forwarded event should not be accessed again after being
 *   forwarded.
 *   This is not thread safe so should only be called from code that
 *   executes withing the fsm_execute() function call.
 *
 * @param[in]    context      : FSM context
 * @param[in]    destination  : destination of the event
 * @param[in]    event        : event to add to the event queue
 *
 * @return
 *   void
 */
extern void fsm_forward_event(FsmContext *context, CsrUint16 destination, const FsmEvent* event);

/**
 * @brief
 *   Sets a timer for a process
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context   : FSM context
 * @param[in]    timer     : timer to add to the timer queue
 * @param[in]    timeInMs  : timeout time in ms
 * @param[in]    id        : timer event id
 *
 * @return
 *   CsrUint32 the timerid if the new timer
 */
extern FsmTimerId fsm_set_timer(FsmContext *context, FsmTimer* timer, CsrUint32 timeInMs, CsrUint16 timeExtraMs, CsrUint16 id);

/**
 * @brief
 *   Removes a timer from the timer list
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context   : FSM context
 * @param[in]    timerid   : timerid to remove
 *
 * @return
 *   void
 */
extern void fsm_remove_timer(FsmContext *context, FsmTimerId timerid);

/**
 * @brief
 *   Current time of day in ms
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context   : FSM context
 * @param[in]    timerid   : timerid to remove
 *
 * @return
 *   void
 */
extern CsrUint32 fsm_get_time_of_day_ms(FsmContext *context);

/**
 * @brief
 *   shift the current time of day by ms amount
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context  : FSM context
 * @param[in]    ms       : ms to adjust time by
 *
 * @return
 *   void
 */
#ifdef FSM_TEST_SUPPORT
extern void fsm_test_advance_time(FsmContext *context, CsrUint32 ms);
#endif



/**
 * @brief
 *   returns the first event of a given type in the saved event list
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context   : FSM context
 * @param[in]    timerid   : timerid to remove
 *
 * @return
 *   void
 */
extern const FsmEvent* fsm_sniff_saved_event(FsmContext *context, CsrUint16 eventid);

/**
 * @brief
 *   removes the specified event from the saved event list
 *
 * @par Description
 *   see brief
 *
 * @param[in]    FsmContext *: FSM context
 * @param[in]    FsmEvent *: pointer to event to remove
 *
 * @return
 *   void
 */
extern void fsm_remove_saved_event(FsmContext *context, FsmEvent *event);


/**
 * @brief
 *   removes all saved events from "srcid"
 *
 * @par Description
 *   see brief
 *
 * @param[in]    context : FSM context
 * @param[in]    fsmid   : id of the fsm who's saved queue needs to be flushed
 * @param[in]    srcid   : event srcid to remove
 *
 * @return
 *   void
 */
extern void fsm_flush_saved_events_from(FsmContext *context, CsrUint16 fsmid, CsrUint16 srcid);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* FSM_H */

