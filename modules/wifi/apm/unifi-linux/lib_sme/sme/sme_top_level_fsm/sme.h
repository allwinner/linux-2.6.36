/** @file sme.h
 *
 * SME Interface
 *
 * @section LEGAL
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   SME API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_top_level_fsm/sme.h#2 $
 *
 ****************************************************************************/
#ifndef SME_H
#define SME_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/csr_wifi_fsm.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"

#include "sys_sap/sys_sap.h"
#include "mgt_sap/mgt_sap.h"

#include "bt_sap/bt_sap.h"

#include "dbg_sap/dbg_sap.h"

#ifdef SME_SYNC_ACCESS
#include "sync_access/sync_access.h"
#endif

#ifdef CSR_AMP_ENABLE
#include "palio/palio_fsm_types.h"
#include "pal_ctrl_sap/pal_ctrl_sap.h"

#include "pal_hci_sap/pal_hci_sap_types.h"
#include "pal_hci_sap/pal_hci_sap.h"
#endif

/**
 * @brief
 *   Initialises the SME top level FSM context
 *
 * @par Description
 *   This initialises the SME creating all the SME processes
 *   and configuring the SME's FSM context.
 *   This function should only be called once!
 *
 *   See also sme_shutdown().
 *
 * @return
 *   FsmContext* new fsm context
 */
extern FsmContext* sme_init(void* externalContext, void* cryptoContext);

/**
 * @brief
 *   Resets the SME top level FSM context
 *
 * @par Description
 *   This function calls sme_shutdown() and then sme_init().
 *   Key data from the SME is saved and the restored before the reset.
 *   This includes calibration data and last connected ap list
 *   This should not be called when fsm_execute() is running
 *
 * @return
 *   FsmContext* new fsm context
 */
extern FsmContext* sme_reset(FsmContext* context);


/**
 * @brief
 *   Shuts down the SME top level FSM context
 *
 * @par Description
 *   This function frees the resources allocated by sme_init() prior to
 *   termination of the program.
 *   This should not be called when fsm_execute() is running
 *
 * @return
 *   void
 */
extern void sme_shutdown(FsmContext* context);


/**
 * @brief
 *   Executes the sme
 *
 * @par Description
 *   Executes the SME context and runs until ALL events processed.
 *   When no more events are left to process then sme_execute() returns a time
 *   specifying when to next call the sme_execute()
 *   Scheduling, threading, blocking and external event notification are outside
 *   the scope of sme_execute().
 *
 * @param[in]    context  : FSM context
 *
 * @return
 *   FsmTimerData absolute time + allowed variation for next timeout OR 0xFFFFFFFF if no timers are set
 */
extern FsmTimerData sme_execute(FsmContext* context);

/**
 * @brief
 *   function that installs the sme wakeup function
 *
 * @par Description
 *   The installed callback function is called whenever the sme is
 *   needs to wakeup. This will be caused by calls to the sme's
 *   sap api functions.
 *
 * @param[in]    context    : FSM context
 * @param[in]    callback   : Callback function pointer
 *
 * @return
 *   void
 */
extern void sme_install_wakeup_callback(FsmContext* context, FsmExternalWakupCallbackPtr callback);


#ifdef __cplusplus
}
#endif

#endif /* SME_H */
