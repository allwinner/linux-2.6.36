/** @file fsm_private.h
 *
 * Private FSM header
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/fsm/fsm_private.h#1 $
 *
 ****************************************************************************/
#ifndef FSM_PRIVATE_H
#define FSM_PRIVATE_H

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

/**
 * @brief
 *   function macro to initialise a event list
 *
 * @par Description
 *   Initialises a event list's elements
 *
 * @param[in]  eventList : List to initialise
 *
 * @return
 *   void
 */
#define fsm_init_signal_list(eventList) \
    (eventList)->first = NULL; \
    (eventList)->last = NULL;

/**
 * @brief
 *   function macro to initialise a timer list
 *
 * @par Description
 *   Initialises a timer list's elements
 *
 * @param[in]  timerList : List to initialise
 *
 * @return
 *   void
 */
#define fsm_init_timer_list(timerList) \
    (timerList)->nexttimerid = 1; \
    (timerList)->first = NULL; \
    (timerList)->last = NULL

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Adds an event to a event list
 *
 * @par Description
 *   Links the event to the END of the event list.
 *
 * @param[in]  eventList : List to add the event to
 * @param[in]  event     : The event to add to the list
 *
 * @return
 *   void
 */
extern void fsm_add_event(FsmEventList *eventList, FsmEvent* event);

/**
 * @brief
 *   Finds an event with defined id in an event list
 *
 * @par Description
 *   Returns the first instance of an event type from a list
 *   This does not remove the evnt from the list
 *
 * @param[in]  eventList : List to add the event to
 * @param[in]  id        : Event Id to search for
 *
 * @return
 *   FsmEvent* pointer to the found event or NULL
 */
extern FsmEvent* fsm_find_event_by_id(FsmEventList *eventList, CsrUint16 id);

/**
 * @brief
 *   Removes an event from a event list
 *
 * @par Description
 *   see brief
 *
 * @param[in]  eventList : List to remove the event from
 * @param[in]  event     : The event to remove from the list
 *
 * @return
 *   void
 */
extern void fsm_remove_event(FsmEventList *eventList, FsmEvent* event);

/**
 * @brief
 *   Pops an event off a event list
 *
 * @par Description
 *   Unlinks and returns the FIRST event in the event list.
 *   Returns NULL if the event list is empty.
 *
 * @param[in]  eventList : List get the event from
 *
 * @return
 *   FsmEvent* pointer to a event or NULL
 */
extern FsmEvent *fsm_pop_event(FsmEventList *eventList);

/**
 * @brief
 *   Pops an event off a event list for a fsm
 *
 * @par Description
 *   Unlinks and returns the FIRST event in the event list
 *   whos detination matches destination
 *   Returns NULL if the event list is empty.
 *
 * @param[in]  eventList   : List get the event from
 * @param[in]  destination : Event destination id
 *
 * @return
 *   FsmEvent* pointer to a event or NULL
 */
extern FsmEvent *fsm_pop_event_for_destination(FsmEventList *eventList, CsrUint16 destination);

/**
 * @brief
 *   Pops an event off a event list from a fsm
 *
 * @par Description
 *   Unlinks and returns the FIRST event in the event list
 *   whos source matches source
 *   Returns NULL if the event list is empty.
 *
 * @param[in]  eventList : List get the event from
 * @param[in]  source    : Event source id
 *
 * @return
 *   FsmEvent* pointer to a event or NULL
 */
extern FsmEvent *fsm_pop_event_from_source(FsmEventList *eventList, CsrUint16 source);


/**
 * @brief
 *   Threadsafe version of fsm_pop_event_for_destination
 *
 * @par Description
 *   see fsm_pop_event_for_destination
 *
 * @param[in]  eventList   : List get the event from
 * @param[in]  destination : Event destination id
 *
 * @return
 *   FsmEvent* pointer to a event or NULL
 */
#ifdef FSM_MUTEX_ENABLE
extern FsmEvent *fsm_pop_event_for_destination_thread_safe(CsrMutexHandle mutex, FsmEventList *eventList, CsrUint16 destination);
#else
#define fsm_pop_event_for_destination_thread_safe fsm_pop_event_for_destination
#endif
/**
 * @brief
 *   Threadsafe version of fsm_add_event
 *
 * @par Description
 *   see fsm_add_event
 *
 * @param[in]  eventList : List to add the event to
 * @param[in]  event     : The event to add to the list
 *
 * @return
 *   void
 */
#ifdef FSM_MUTEX_ENABLE
extern void fsm_add_event_thread_safe(CsrMutexHandle mutex, FsmEventList *eventList, FsmEvent* event);
#else
#define fsm_add_event_thread_safe fsm_add_event
#endif

/**
 * @brief
 *   Threadsafe version of fsm_find_event_by_id
 *
 * @par Description
 *   see fsm_find_event_by_id
 *
 * @param[in]  eventList : List to add the event to
 * @param[in]  id        : Event Id to search for
 *
 * @return
 *   FsmEvent* pointer to the found event or NULL
 */
#ifdef FSM_MUTEX_ENABLE
extern FsmEvent* fsm_find_event_by_id_thread_safe(CsrMutexHandle mutex, FsmEventList *eventList, CsrUint16 id);
#else
#define fsm_find_event_by_id_thread_safe fsm_find_event_by_id
#endif

/**
 * @brief
 *   Threadsafe version of fsm_remove_event
 *
 * @par Description
 *   see fsm_remove_event
 *
 * @param[in]  eventList : List to remove the event from
 * @param[in]  event     : The event to remove from the list
 *
 * @return
 *   void
 */
#ifdef FSM_MUTEX_ENABLE
extern void fsm_remove_event_thread_safe(CsrMutexHandle mutex, FsmEventList *eventList, FsmEvent* event);
#else
#define fsm_remove_event_thread_safe fsm_remove_event
#endif

/**
 * @brief
 *   Threadsafe version of fsm_pop_event
 *
 * @par Description
 *   see fsm_pop_event
 *
 * @param[in]  eventList : List get the event from
 *
 * @return
 *   FsmEvent* pointer to a event or NULL
 */
#ifdef FSM_MUTEX_ENABLE
extern FsmEvent *fsm_pop_event_thread_safe(CsrMutexHandle mutex, FsmEventList *eventList);
#else
#define fsm_pop_event_thread_safe fsm_pop_event
#endif

/**
 * @brief
 *   Adds an timer to a timer list
 *
 * @par Description
 *   Links the timer to the END of the timer list.
 *
 * @param[in]  timerList : List to add the timer to
 * @param[in]  timer     : The timer to add to the list
 *
 * @return
 *   void
 */
extern void fsm_add_timer(FsmTimerList *timerList, FsmTimer* timer);

/**
 * @brief
 *   Pops an expired timer off a timer list
 *
 * @par Description
 *   Unlinks and returns the FIRST timer in the timer list.
 *   Returns NULL if the event list is empty or if the first timer
 *   has not yet expired.
 *
 * @param[in]  timerList : List get the timer from
 *
 * @return
 *   FsmTimer* pointer to a timer or NULL
 */
extern FsmTimer *fsm_pop_expired_timer(FsmContext* context, FsmTimerList *timerList);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* FSM_PRIVATE_H */

