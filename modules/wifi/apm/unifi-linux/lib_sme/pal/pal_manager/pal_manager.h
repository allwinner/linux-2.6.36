/** @file pal_manager.h
 *
 * PAL Manager API
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
 *   Public PAL Manager API
 *
 ****************************************************************************
 *
 * @section MODIFICATION HISTORY
 * @verbatim
 *   #1    16:mar:07 B-20453: created
 * @endverbatim
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/pal_manager/pal_manager.h#1 $
 *
 ****************************************************************************/
#ifndef PAL_MANAGER_H
#define PAL_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup pal_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "pal_hci_sap/pal_hci_sap_types.h"
#include "link_manager_fsm/pal_data_common.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/
typedef struct hciBottomContext hciBottomContext;
typedef struct palManagerContext palManagerContext;

/* FIXME: This structure can really go into pal_manager.c file. It is not now
 * because of the hciBottom which is accessed in a different file
 * called hci_bottom.c. Fix it as appropriate
 */
struct palManagerContext
{
    /* common link parameters */
    physicalLinkCommonAttrib phyLinkCommonAttrib;

    /* HCI Bottom Parameters */
    hciBottomContext *hciBottom;

    CsrUint16 maxPhysicalLinks;
    physicalLinkSharedAttrib *phyLinkSharedAttrib;

    /* Parameters to determine the AMP usability based on SME connection status */
    AmpStatus ampStatus;
    CsrBool initialLocalAmpInfoRead;

    void *appHandle;
};

/* 
 * Connection status code points to determine the 
 * AMP Status for hci_read_local_amp_info and
 * hci_amp_status_change event. it is also used to
 * determine if a new physical link can be allowed
 * 
 * According to BT 3.0 spec section section 7 of part E Vol 2
 *
* 0x00  (PAL_SME_CONNECTION_STATUS_POWER_OFF)
* The Controller is available but is currently physically powered
* down.
* This value shall be used if the AMP Controller is present and can
* be powered up by the AMP Manager.
* This value indicates that there may be a cost of time and power to
* use this AMP Controller (i.e., the time taken and power required to
* power up the AMP Controller). These costs are AMP type and
* AMP implementation dependent.
* 
* 0x01 (PAL_SME_CONNECTION_STATUS_DISABLED)
* This value indicates that the AMP Controller is only used by Bluetooth
* technology and will not be shared with other non-Bluetooth
* technologies.
* This value shall only be used if the AMP Controller is powered up.
* This value does not indicate how much bandwidth is currently free
* on the AMP Controller.
* 
* 0x02 (PAL_SME_CONNECTION_STATUS_CONNECTED_FULL_CAPACITY)
* The AMP Controller has no capacity available for Bluetooth operation
* This value indicates that all of the AMP Controllers bandwidth is
* currently allocated to servicing a non Bluetooth technology.
* A device is permitted to create a Physical Link to an AMP Controller
* that has this status.
* This value shall only be used if the AMP Controller is powered up.
* 
* 0x03 (PAL_SME_CONNECTION_STATUS_CONNECTED_HIGH_CAPICITY)
* The AMP Controller has low capacity available for Bluetooth operation.
* This value indicates that the majority of the AMP Controllers bandwidth
* is currently allocated to servicing a non Bluetooth technology.
* An AMP Controller with capacity in the approximate range of 0% to
* 30% should indicate this value.
* This value does not indicate how much of the capacity available for
* Bluetooth operation is currently available.
* This value shall only be used if the AMP Controller is powered up.
* 
* 0x04  (PAL_SME_CONNECTION_STATUS_CONNECTED_MEDIUM_CAPICITY)
* The AMP Controller has medium capacity available for Bluetooth
* operation.
* An AMP Controller with capacity in the approximate range of 30%
* to 70% should indicate this value.
* This value does not indicate how much of the capacity available for
* Bluetooth operation is currently available.
* This value shall only be used if the AMP Controller is powered up.
*
* 0x05  (PAL_SME_CONNECTION_STATUS_CONNECTED_LOW_CAPICITY)
* The AMP Controller has high capacity available for Bluetooth operation.
* This value indicates that the majority of the AMP Controllers bandwidth
* is currently allocated to servicing the Bluetooth technology.
* An AMP Controller with capacity in the approximate range of 70%
* to 100% should indicate this value.
* This value does not indicate how much of the capacity available for
* Bluetooth operation is currently available.
* This value shall only be used if the AMP Controller is powered up.
*  
* 0x06 (PAL_SME_CONNECTION_STATUS_DISCONNECTED)
* The AMP Controller has full capacity available for Bluetooth operation.
* This value indicates that while currently the AMP is only being used
* by Bluetooth the device allows a different technology to share the
* radio.
* This value shall be used by devices that are not capable of determining
* the current available capacity of an AMP that is shared by a
* different technology.
 This value does not indicate how much of the capacity available for
 Bluetooth operation is currently available.
 This value shall only be used if the AMP Controller is powered up.
 */
typedef enum PAL_SmeConnectionStatus
{
    PAL_SME_CONNECTION_STATUS_POWER_OFF = 0x00,
    PAL_SME_CONNECTION_STATUS_DISABLED = 0x01,
    PAL_SME_CONNECTION_STATUS_CONNECTED_FULL_CAPACITY = 0x02,
    PAL_SME_CONNECTION_STATUS_CONNECTED_HIGH_CAPICITY = 0x03,
    PAL_SME_CONNECTION_STATUS_CONNECTED_MEDIUM_CAPICITY = 0x04,
    PAL_SME_CONNECTION_STATUS_CONNECTED_LOW_CAPICITY  = 0x05,
    PAL_SME_CONNECTION_STATUS_DISCONNECTED  = 0x06
}PAL_SmeConnectionStatus;

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the ip connection manager state machine data
 */
extern const FsmProcessStateMachine pal_mgr_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/


/**
 * @brief
 *    initialisation for HCI bottom
 *
 * @par Description
 *     The command is forwarded only if cmdStatus is HCI_STATUS_CODE_SUCCESS and
 * it hasn't reached the maximum allowed limit for processing commands.
 *
 * @param[in]    maxHciCommandsAllowed   :  maximum HCI command allowed
 *
 * @return
 *    pointer to the HCI bottom context
 */
extern void* pal_hci_bottom_init(CsrUint16 maxHciCommandsAllowed);

/**
 * @brief
 *    de-initialisation for HCI bottom
 *
 * @par Description
 *     The command is forwarded only if cmdStatus is HCI_STATUS_CODE_SUCCESS and
 * it hasn't reached the maximum allowed limit for processing commands.
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    void
 */
extern void pal_hci_bottom_deinit(FsmContext *context);

/**
 * @brief
 *    initialisation for PAL Manager
 *
 * @par Description
 *
 * @param[in]    maxPhysicalLinks   :  Maximum number of physical links supported
 * @param[in]    maxHciCommandsAllowed   :  maximum number of outstanding HCI command allowed
 *
 * @return
 *    pointer to the PAL Manager context
 */
extern palManagerContext *pal_manager_init(CsrUint16 maxPhysicalLinks, CsrUint16 maxHciCommandsAllowed);

/**
 * @brief
 *    de-initialisation for PAL Manager
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    void
 */
extern void pal_manager_deinit(FsmContext *context);

/**
 * @brief
 *    set the support status for guaranteed links
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    state   :  TRUE if guaranteed links supported else FALSE.
 *
 * @return
 *    void
 */
extern void pal_set_qos_support(FsmContext *context, CsrBool state);

/**
 * @brief
 *    set the security support status
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    state   :  TRUE if security handshake supported else FALSE.
 *
 * @return
 *    void
 */
extern void pal_set_security_support(FsmContext *context, CsrBool state);

/**
 * @brief
 *    set the channel number to lock
 *
 * @par Description
 *    PAL will only use the channel set and reject any request that cannot support this channel.
 * This is primarily to assist in debugging.
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    channel   :  channel number to lock to
 *
 * @return
 *    void
 */
extern void pal_fix_channel(FsmContext* context, CsrUint8 channel);

/**
 * @brief
 *    function to generate activity report for test purposes only.
 *
 * @par Description
 *    This function can be called to generate activity report from all active physical links.
 * It is introduced for test purposes only.
 *
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    scheduleKnown   :  TRUE if schedule is known , else FALSE.
 * @param[in]    startTime   :  start time 
 * @param[in]    duration   :  duration 
 * @param[in]    periodicity   :  periodicity 
 *
 * @return
 *    void
 */
extern void pal_generate_ar(FsmContext* context,
                            CsrBool scheduleKnown,
                            CsrUint32 startTime,
                            CsrUint32 duration,
                            CsrUint32 periodicity);

/**
 * @brief
 *   Decision point for allowing the SME to go ahead with connection.
 *
 * @par Description
 *   SME may decide to reject its  connection requests if AMP connection is in progress.
 *
 * @param[in] context               : FSM context
 *
 * @return
 *   TRUE if atleast one AMP connection is in progress else return FALSE.
 */
extern CsrBool pal_amp_connection_in_progress(FsmContext* context);

/**
 * @brief
 *   Function to call to update the SME connection status. 
 *
 *
 * @param[in] context               : FSM context
 * @param[in] connectionStatus               : new connection status from SME.
 *
 * @return
 *   void
 */
extern void pal_sme_connection_status_change_request(FsmContext* context, PAL_SmeConnectionStatus connectionStatus);

/**
 * @brief
 *   Function to call to update the SME connection status. If there is a 
 * change in the connection status, the function will send hci-amp-status-change event
 *
 *
 * @param[in] context               : FSM context
 * @param[in] connectionStatus               : new connection status from SME.
 *
 * @return
 *   void
 */
extern void pal_mgr_update_sme_connection_status(FsmContext* context, PAL_SmeConnectionStatus connectionStatus);

/**
 * @brief
 *   send hci-amp-status-change event if not masked
 *
 *
 * @param[in] context               : FSM context
 * @param[in] status               : event status.
 * @param[in] ampStatus               : amp connection status determined based on SME connection status.
 *
 * @return
 *   void
 */
extern void pal_hci_send_hci_amp_status_change_evt(FsmContext *context, HciStatusCode status, AmpStatus ampStatus);

/**
 * @brief
 *   Function to check if AMP connection can be allowed based on the current SME connection status
 *
 *
 * @param[in] context               : FSM context
 *
 * @return
 *   TRUE if connection is allowed else FALSE.
 */
extern CsrBool pal_mgr_amp_connections_allowed(FsmContext* context);


/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PAL_MANAGER_H */
