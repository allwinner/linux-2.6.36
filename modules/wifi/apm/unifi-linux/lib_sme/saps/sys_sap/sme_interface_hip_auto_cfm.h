/** @file sme_interface_hip_auto_cfm.h
 *
 * Stores cfm's for use if comms with the driver is lost
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
 *   Stores auto cfm's for outstanding hip requests that are used to send to the
 *   sme in the event of a loss of comms with the driver / firmware
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/saps/sys_sap/sme_interface_hip_auto_cfm.h#2 $
 *
 ****************************************************************************/

#ifndef HIP_AUTO_CFM_HEADER_H_
#define HIP_AUTO_CFM_HEADER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "fsm/csr_wifi_fsm.h"

typedef struct HipAutoCfm HipAutoCfm;

/**
 * @brief
 *   Initialises the auto cfm mechanism
 *
 * @par Description
 *   see brief
 *
 * @return
 *   void
 */
extern HipAutoCfm* hip_auto_cfm_init(FsmContext* context);

/**
 * @brief
 *   Shutsdown the auto cfm mechanism
 *
 * @return
 *   void
 */
extern void hip_auto_cfm_shutdown(HipAutoCfm* context);

/**
 * @brief
 *   Adds a cfm to the list for use on an error
 *
 * @par Description
 *   see brief
 *
 * @param[in]    cfm   : auto cfm for use on error
 *
 * @return
 *   void
 */
extern void hip_auto_cfm_add(HipAutoCfm* context, FsmEvent* cfm);

/**
 * @brief
 *   Check for matching cfm
 *
 * @param[in]    id   : Matching Id
 *
 * @return
 *   CsrBool cfm exists in the queue
 */
extern CsrBool hip_auto_cfm_has_matching_cfm(HipAutoCfm* context, CsrUint16 id);

/**
 * @brief
 *   Remove matching cfm
 *
 * @param[in]    id   : Matching Id
 *
 * @return
 *   CsrBool Was the message removal successfull
 */
extern CsrBool hip_auto_cfm_remove_matching_cfm(HipAutoCfm* context, CsrUint16 id, CsrBool optional);

/**
 * @brief
 *   Sets the auto cfm to auto send messages
 *
 * @par Description
 *   Call this when there is an error and you do not expect
 *   any cfm's from the device
 *
 * @return
 *   void
 */
extern void hip_auto_cfm_send_events(HipAutoCfm* context);

/**
 * @brief
 *   Check for old messages
 *
 * @par Description
 *   Check if there is are messages older than timeoutMs in the queue
 *
 * @param[in]    timeoutMs   : how old can the message be before it is considered old
 *
 * @return
 *   CsrBool : TRUE  : Old message in the queue
 *             FALSE : No old messgae in the queue
 */
extern CsrBool hip_auto_cfm_message_timedout(HipAutoCfm* context, CsrUint16 timeoutMs);

/**
 * @brief
 *   Sets the auto cfm to store messages
 *
 * @par Description
 *   Call this when any error has been resolved and you expect
 *   cfm from the device
 *
 * @return
 *   void
 */
extern void hip_auto_cfm_store_events(HipAutoCfm* context);

/**
 * @brief
 *   Is the hip sap open for send and receive
 *
 * @return
 *   CsrBool
 */
extern CsrBool hip_auto_cfm_is_hip_sap_open(HipAutoCfm* context);

/**
 * @brief
 *   register a call back that will inform the interested party
 *   that the auto cfm queue is now empty.
 *
 * @param[in]    context    : Context of execution
 * @param[in]    singleShot : Flag to indicate a single call back
 * @param[in]    funcPtr    : Callback Function
 *
 * @return
 *   void
 */
extern void hip_auto_cfm_register_empty_queue_callback(HipAutoCfm* context, CsrBool singleShot, void (*funcPtr)(FsmContext* context));

#ifdef __cplusplus
}
#endif

#endif /* HIP_AUTO_CFM_HEADER_H_ */
