/** @file nme.h
 *
 * Network Managemnet Entity (NME) Interface
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
 *   Network Managemnet Entity (NME) API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_top_level_fsm/nme.h#2 $
 *
 ****************************************************************************/
#ifndef CSR_WIFI_NME_H
#define CSR_WIFI_NME_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/csr_wifi_fsm.h"

#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "nmeio/nmeio_fsm_types.h"

#include "csr_wifi_nme_sap/csr_wifi_nme_sap.h"
#include "csr_wifi_nme_mgt_sap/csr_wifi_nme_mgt_sap.h"
#include "csr_wifi_nme_sys_sap/csr_wifi_nme_sys_sap.h"

/**
 * @brief
 *   Initialises the top level FSM context
 *
 * @par Description
 *   This initialises the creating all the processes
 *   and configuring the FSM context.
 *   This function should only be called once!
 *
 *   See also csr_wifi_nme_shutdown().
 *
 * @param[in]    void * : external context
 *
 * @return
 *   FsmContext* new fsm context
 */
extern FsmContext* csr_wifi_nme_init(void* pExternalContext, void* cryptoContext);

/**
 * @brief
 *   Resets the top level FSM context
 *
 * @par Description
 *   This function calls csr_wifi_nme_shutdown() and then csr_wifi_nme_init().
 *
 * @param[in]    context * : FSM context
 *
 * @return
 *   FsmContext* new fsm context
 */
extern FsmContext* csr_wifi_nme_reset(FsmContext* pContext);


/**
 * @brief
 *   Shuts down the top level FSM context
 *
 * @par Description
 *   This function frees the resources allocated by csr_wifi_nme_init() prior to
 *   termination of the program.
 *   This should not be called when fsm_execute() is running
 *
 * @param[in]    context * : FSM context
 *
 * @return
 *   void
 */
extern void csr_wifi_nme_shutdown(FsmContext* pContext);

/**
 * @brief
 *   Executes the fsm
 *
 * @par Description
 *   Executes the context and runs until ALL events processed.
 *   When no more events are left to process then csr_wifi_nme_execute() returns a time
 *   specifying when to next call the csr_wifi_nme_execute()
 *   Scheduling, threading, blocking and external event notification are outside
 *   the scope of csr_wifi_nme_execute().
 *
 * @param[in]    context * : FSM context
 *
 * @return
 *   FsmTimerData absolute time + allowed variation for next timeout OR 0xFFFFFFFF if no timers are set
 */
extern FsmTimerData csr_wifi_nme_execute(FsmContext* pContext);

/**
 * @brief
 *   function that installs the wakeup function
 *
 * @par Description
 *   The installed callback function is called whenever the NME is
 *   needs to wakeup. This will be caused by calls to the NME's
 *   sap api functions.
 *
 * @param[in]    context *  : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
extern void csr_wifi_nme_install_wakeup_callback(FsmContext* pContext, FsmExternalWakupCallbackPtr callback);

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_NME_H */
