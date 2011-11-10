/** @file fsm_types.h
 *
 * FSM Types definitions
 *
 * @section LEGAL
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   FSM Types
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/fsm/fsm_types.h#2 $
 *
 ****************************************************************************/
#ifndef FSM_TYPES_H
#define FSM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup fsm
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "fsm/csr_wifi_fsm.h"

/* MACROS *******************************************************************/
#define FSM_MAX_TRANSITION_HISTORY 10

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/**
 * @brief
 *   FSM event list header.
 *
 * @par Description
 *   Singly linked list of events.
 */
typedef struct FsmEventList
{
    FsmEvent *first;
    FsmEvent *last;
} FsmEventList;


/**
 * @brief
 *   FSM timer id.
 *
 * @par Description
 *   Composite Id made up of the id, dest and a unique id so
 *   fsm_remove_timer knows where to look when removing the timer
 */
typedef struct FsmTimerId
{
    CsrUint16 id;
    CsrUint16 destination;
    CsrUint16 uniqueid;
} FsmTimerId;

/**
 * @brief
 *   FSM timer header.
 *
 * @par Description
 *   All timer MUST have this struct as the FIRST member.
 *   The first members of the structure MUST remain compatable
 *   with the FsmEvent so that timers are just specialised events
 */
typedef struct FsmTimer
{
    struct FsmTimer *next;
    CsrUint16 id;
    CsrUint16 destination;
    CsrUint16 sender_; /* trailing underscore to avoid clash with #define */

    FsmTimerId timerid;
    FsmTimerData timeoutData;
} FsmTimer;

/**
 * @brief
 *   FSM timer list header.
 *
 * @par Description
 *   Singly linked list of timers.
 */
typedef struct FsmTimerList
{
    FsmTimer *first;
    FsmTimer *last;
    CsrUint16 nexttimerid;
} FsmTimerList;

/**
 * @brief
 *   Process Entry Function Pointer
 *
 * @par Description
 *   Defines the entry function for a processes.
 *   Called at process initialisation.
 *
 * @param[in]    context : FSM context
 *
 * @return
 *   void
 */
typedef void (*FsmProcEntryFnPtr) (FsmContext* context);

/**
 * @brief
 *   Process Transition Function Pointer
 *
 * @par Description
 *   Defines a transition function for a processes.
 *   Called when an event causes a transition on a process
 *
 * @param[in]    context : FSM context
 * @param[in]    event   : event to process
 *
 * @return
 *   void
 */
typedef void (*FsmTransitionFnPtr)(FsmContext* context, const FsmEvent* event);

/**
 * @brief
 *   Process reset/shutdown Function Pointer
 *
 * @par Description
 *   Defines the reset/shutdown function for a processes.
 *   Called to reset or shutdown an fsm.
 *
 * @param[in]    context      : FSM context
 *
 * @return
 *   void
 */
typedef void (*FsmProcResetFnPtr) (FsmContext* context);

#ifdef FSM_DEBUG_DUMP
/**
 * @brief
 *   Trace Dump Function Pointer
 *
 * @par Description
 *   Called when we want to trace the FSM
 *
 * @param[in]    context : FSM context
 * @param[in]    id      : fsm id
 *
 * @return
 *   void
 */
typedef void (*FsmDumpFnPtr)(FsmContext* context, const CsrUint16 id);
#endif

/**
 * @brief
 *   Event ID to transition function entry
 *
 * @par Description
 *   Event ID to Transition Entry in a state table.
 */
typedef struct
{
   CsrUint16 eventid;
   FsmTransitionFnPtr transition;
#ifdef FSM_DEBUG
   const char* transitionName;
#endif
} FsmEventEntry;

/**
 * @brief
 *   Single State's Transition Table
 *
 * @par Description
 *   Stores Data for a single State's event to
 *   transition functions mapping
 */
typedef struct
{
   const CsrUint8 numEntries;
   const CsrBool saveAll;
   const FsmEventEntry* eventEntryArray; /* array of transition function pointers for state */
#ifdef FSM_DEBUG
   CsrUint16 stateNumber;
   const char* stateName;
#endif
} FsmTableEntry;

/**
 * @brief
 *   Process State Transtion table
 *
 * @par Description
 *   Stores Data for a processes State to transition table
 */
typedef struct
{
   CsrUint16                 numStates;          /* number of states    */
   const FsmTableEntry*   aStateEventMatrix;  /* state event matrix  */
} FsmTransitionFunctionTable;

/**
 * @brief
 *   Const Process definition
 *
 * @par Description
 *   Constant process specification.
 *   This is ALL the non dynamic data that defines
 *   a process.
 */
typedef struct
{
#ifdef FSM_DEBUG
   const char*                       processName;
#endif
   const CsrUint32                      processId;
   const FsmTransitionFunctionTable  transitionTable;
   const FsmTableEntry               unhandledTransitions;
   const FsmTableEntry               ignoreFunctions;
   const FsmProcEntryFnPtr           entryFn;
   const FsmProcResetFnPtr           resetFn;
#ifdef FSM_DEBUG_DUMP
   const FsmDumpFnPtr                dumpFn; /* Called to dump fsm specific trace if not NULL */
#endif
} FsmProcessStateMachine;

#ifdef FSM_DEBUG_DUMP
/**
 * @brief
 *   Storage for state transition info
 */
typedef struct
{
   CsrUint16 transitionNumber;
   FsmEvent event;
   CsrUint16 fromState;
   CsrUint16 toState;
   FsmTransitionFnPtr transitionFn;
   CsrUint16 transitionCount; /* number consecutive of times this transition was seen */
#ifdef FSM_DEBUG
   const char* transitionName;
#endif
} FsmTransitionRecord;

/**
 * @brief
 *   Storage for the last state X transitions
 */
typedef struct
{
    CsrUint16 numTransitions;
    FsmTransitionRecord records[FSM_MAX_TRANSITION_HISTORY];
} FsmTransitionRecords;
#endif

/**
 * @brief
 *   Dynamic Process data
 *
 * @par Description
 *   Dynamic process data that is used to keep track of the
 *   state and data for a process instance
 */
typedef struct
{
   const FsmProcessStateMachine*  fsmInfo;            /* state machine info that is constant regardless of context */
   CsrUint16                         instanceId;         /* Runtime process id */
   CsrUint16                         state;              /* Current state */
   void*                          params;             /* Instance user data */
   FsmEventList                   savedEventQueue;    /* The saved event queue */
   struct FsmInstanceEntry*       subFsm;             /* Sub Fsm instance data */
#ifdef FSM_DEBUG_DUMP
   FsmTransitionRecords           transitionRecords;  /* Last X transitions in the FSM */
#endif
} FsmInstanceEntry;

/**
 * @brief
 *   OnCreate Callback Function Pointer
 *
 * @par Description
 *   Called when an fsm is created.
 *
 * @param[in]    extContext : External context
 * @param[in]    instance : FSM instance
 *
 * @return
 *   void
 */
typedef void (*FsmOnCreateFnPtr) (void* extContext, const FsmInstanceEntry* instance);

/**
 * @brief
 *   OnTransition Callback Function Pointer
 *
 * @par Description
 *   Called when an event is processed by a fsm
 *
 * @param[in]    extContext : External context
 * @param[in]    eventEntryArray : Entry data
 * @param[in]    event : Event
 *
 * @return
 *   void
 */
typedef void (*FsmOnTransitionFnPtr) (void* extContext, const FsmEventEntry* eventEntryArray, const FsmEvent* event);

/**
 * @brief
 *   OnStateChange Callback Function Pointer
 *
 * @par Description
 *   Called when fsm_next_state is called
 *
 * @param[in]    extContext : External context
 *
 * @return
 *   void
 */
typedef void (*FsmOnStateChangeFnPtr) (void* extContext, CsrUint16 nextstate);

/**
 * @brief
 *   OnIgnore,OnError or OnInvalid Callback Function Pointer
 *
 * @par Description
 *   Called when an event is processed by a fsm
 *
 * @param[in]    extContext : External context
 * @param[in]    event : Event
 *
 * @return
 *   void
 */
typedef void (*FsmOnEventFnPtr) (void* extContext, const FsmEvent* event);

/**
 * @brief
 *   Toplevel FSM context data
 *
 * @par Description
 *   Holds ALL FSM static and dynamic data for a FSM
 */
struct FsmContext
{
   FsmEventList       eventQueue;               /* The internal event queue                     */
   FsmEventList       externalEventQueue;       /* The external event queue                     */
#ifdef FSM_MUTEX_ENABLE
   CsrMutexHandle     externalEventQueueLock;   /* The external event queue mutex               */
#endif
   FsmTimerList       timerQueue;               /* The internal timer queue                     */
   CsrBool            useTempSaveList;          /* Should the temp save list be used            */
   FsmEventList       tempSaveList;             /* The temp save event queue                    */
   FsmEvent*          eventForwardedOrSaved;    /* The event that was forwarded or Saved        */
   CsrUint16          maxProcesses;             /* Size of instanceArray                        */
   CsrUint16          numProcesses;             /* Current number allocated in instanceArray    */
   FsmInstanceEntry*  instanceArray;            /* Array of processes for this component        */
   FsmInstanceEntry*  ownerInstance;            /* The Process that owns currentInstance (SubFsm support) */
   FsmInstanceEntry*  currentInstance;          /* Current Process that is executing            */
   FsmExternalWakupCallbackPtr externalEventFn; /* External event Callback                      */
   FsmOnEventFnPtr    appIgnoreCallback;        /* Application Ignore event Callback            */

   void*              applicationContext;       /* Internal fsm application context             */
   void*              externalContext;          /* External context (set by the userof the fsm) */

#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
   CsrMutexHandle     transitionLock;           /* Lock when calling transition functions        */
#endif
#endif

#ifdef FSM_DEBUG
   FsmOnCreateFnPtr      onCreate;              /* Debug Transition Callback                    */
   FsmOnTransitionFnPtr  onTransition;          /* Debug Transition Callback                    */
   FsmOnTransitionFnPtr  onUnhandedCallback;    /* Unhanded event Callback                      */
   FsmOnStateChangeFnPtr onStateChange;         /* Debug State Change Callback                  */
   FsmOnEventFnPtr       onIgnoreCallback;      /* Ignore event Callback                        */
   FsmOnEventFnPtr       onSaveCallback;        /* Save event Callback                          */
   FsmOnEventFnPtr       onErrorCallback;       /* Error event Callback                         */
   FsmOnEventFnPtr       onInvalidCallback;     /* Invalid event Callback                       */
#endif
#ifdef FSM_DEBUG_DUMP
   CsrUint16                masterTransitionNumber; /* Increments on every transition              */
#endif
};

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* FSM_TYPES_H */
