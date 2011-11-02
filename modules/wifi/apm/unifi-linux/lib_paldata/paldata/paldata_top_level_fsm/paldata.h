/** @file paldata.h
 *
 * PAL Data Interface
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
 *   PAL Data API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_paldata/paldata/paldata_top_level_fsm/paldata.h#4 $
 *
 ****************************************************************************/
#ifndef PALDATA_H
#define PALDATA_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/csr_wifi_fsm.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "palio/palio_fsm_types.h"

#include "paldata_ctrl_sap/paldata_ctrl_sap.h"
#include "paldata_acl_sap/paldata_acl_sap.h"
#include "paldata_sys_sap/paldata_sys_sap.h"


/**
 * @brief
 *   Function to call if the caller needs to directly invoke data manager without going through FsmExecute()
 *
 * @par Description
 *     Function provides direct access to data manager to process ma-unitdata-ind. This means the caller can bypass
 * the FsmExecute() function to process the unitdata in PAL-D.
 * NOTE: The function is run thread safe only if FSM_MUTEX_ENABLE and FSM_TRANSITION_LOCK and CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE are defined.
 *           The caller can lock the mutex as long as CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE is not defined. 
 *            Useful when there are multiple events to process and avoid context switch.
 *
 *IMPORTANT: The call is safe as long as there are no state changes in the FSM. There are no state changes
 * at the time of writing this API. The potential problems or risk of a state change inside the call is unknown 
 * at the moment.
 *
 * @return
 *   void
 */
extern void paldata_process_unitdata_ind(FsmContext* context,
                                       CsrUint8 subscriptionHandle,
                                       CsrUint16 frameLength,
                                       const CsrUint8 *frame,
                                       unifi_FrameFreeFunction freeFunction,
                                       unifi_ReceptionStatus receptionStatus,
                                       unifi_Priority priority,
                                       unifi_ServiceClass serviceClass);

/**
 * @brief
 *   Function to call if the caller needs to directly invoke data manager without going through FsmExecute()
 *
 * @par Description
 *     Function provides direct access to data manager to process acl data. This means the caller can bypass
 * the FsmExecute() function to process the unitdata in PAL-D.
 * NOTE: The function is run thread safe only if FSM_MUTEX_ENABLE and FSM_TRANSITION_LOCK and CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE are defined.
 *           The caller can lock the mutex as long as CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE is not defined. 
 *            Useful when there are multiple events to process and avoid context switch.
 *
 *IMPORTANT: The call is safe as long as there are no state changes in the FSM. There are no state changes
 * at the time of writing this API. The potential problems or risk of a state change inside the call is unknown 
 * at the moment.
 *
 * @return
 *   returns the next timeout value in microseconds if there was timer started during the call. Useful to take action by caller
 * if there was a timer started.
 */
extern CsrUint32 paldata_process_acl_data_req(FsmContext *context, 
                                        CsrUint16 dataLength, 
                                        CsrUint8  *data,
                                        CsrUint16 logicalLinkHandle,
                                        CsrUint16 aclOffset,
                                        unifi_FrameFreeFunction freeFunction);

/**
 * @brief
 *   Function to call if the caller needs to directly invoke data manager without going through FsmExecute()
 *
 * @par Description
 *     Function provides direct access to data manager to process ma-unitdata-cfm. This means the caller can bypass
 * the FsmExecute() function to process the unitdata in PAL-D.
 * NOTE: The function is run thread safe only if FSM_MUTEX_ENABLE and FSM_TRANSITION_LOCK and CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE are defined.
 *           The caller can lock the mutex as long as CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE is not defined. 
 *            Useful when there are multiple events to process and avoid context switch.
 *
 *IMPORTANT: The call is safe as long as there are no state changes in the FSM. There are no state changes
 * at the time of writing this API. The potential problems or risk of a state change inside the call is unknown 
 * at the moment.
 *
 * @return
 *   returns the next timeout value in microseconds if there was timer started during the call. Useful to take action by caller
 * if there was a timer started.
 */
extern CsrUint32 paldata_process_unitdata_cfm(FsmContext *context,
                                        unifi_Status result,
                                        unifi_TransmissionStatus transmissionStatus,
                                        unifi_Priority providedPriority,
                                        unifi_ServiceClass providedServiceClass,
                                        CsrUint32 reqIdentifier);

/**
 * @brief
 *   Initialises the top level FSM context
 *
 * @par Description
 *   This initialises the creating all the processes
 *   and configuring the FSM context.
 *   This function should only be called once!
 *
 *   See also paldata_shutdown().
 *
 * @return
 *   FsmContext* new fsm context
 */
extern FsmContext* paldata_init(void* externalContext);

/**
 * @brief
 *   Resets the top level FSM context
 *
 * @par Description
 *   This function calls paldata_shutdown() and then paldata_init().
 *
 * @return
 *   FsmContext* new fsm context
 */
extern FsmContext* paldata_reset(FsmContext* context);


/**
 * @brief
 *   Shuts down the top level FSM context
 *
 * @par Description
 *   This function frees the resources allocated by paldata_init() prior to
 *   termination of the program.
 *   This should not be called when fsm_execute() is running
 *
 * @return
 *   void
 */
extern void paldata_shutdown(FsmContext* context);

/**
 * @brief
 *   Executes the fsm
 *
 * @par Description
 *   Executes the context and runs until ALL events processed.
 *   When no more events are left to process then paldata_execute() returns a time
 *   specifying when to next call the paldata_execute()
 *   Scheduling, threading, blocking and external event notification are outside
 *   the scope of paldata_execute().
 *
 * @param[in]    context  : FSM context
 *
 * @return
 *   FsmTimerData absolute time + allowed variation for next timeout OR 0xFFFFFFFF if no timers are set
 */
extern FsmTimerData paldata_execute(FsmContext* context);

/**
 * @brief
 *   function that installs the wakeup function
 *
 * @par Description
 *   The installed callback function is called whenever the paldata is
 *   needs to wakeup. This will be caused by calls to the paldata's
 *   sap api functions.
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
extern void paldata_install_wakeup_callback(FsmContext* context, FsmExternalWakupCallbackPtr callback);

#ifdef __cplusplus
}
#endif

#endif /* PALDATA_H */
