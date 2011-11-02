/** @file hci_bottom.c
 *
 * PAL Manager - HCI Bottom
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
 *   This file implements the HCI part of the PAL manager.
 *
 ****************************************************************************
 *
 * @section MODIFICATION HISTORY
 * @verbatim
 *   #1   1:may:08 B-36899 :created
 * @endverbatim
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/pal_manager/hci_bottom.c#6 $
 *
 ****************************************************************************/

/** @{
 * @ingroup pal_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"

#include "pal_manager/pal_manager.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "pal_hci_sap/pal_hci_sap_up_pack.h"
#include "pal_hci_sap/pal_hci_sap_types.h"
#include "pal_hci_sap/pal_hci_sap_signals.h"
#include "event_pack_unpack/event_pack_unpack.h"
#include "pal_manager/pal_manager.h"
#include "pal_ctrl_sap/pal_ctrl_sap_from_sme_interface.h"
#include "device_manager_fsm/device_manager_fsm.h"
#include "sys_sap/sys_sap_from_sme_interface.h"
#include "smeio/smeio_trace_types.h"


/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */
#define getHciBottomContext(fsmContext) ((hciBottomContext *)(getPalMgrContext(fsmContext)->hciBottom))

#define PAL_HCI_EVENT_MASK_PAGE_SIZE (8)

/* Always check validity before calling PAL_SET_TIMEOUT_VALUE()
 * timeout==0 is workaround to disable the timer to help with manual testing
 */
/* range check for link accept timers */
#define PAL_LA_TIMEOUT_IN_VALID_RANGE(timeout) ((((timeout>=LINK_ACCEPT_TIMEOUT_MANDATORY_RANGE_MIN)&&(timeout<=LINK_ACCEPT_TIMEOUT_RANGE_MAX))||(timeout==0))?TRUE:FALSE)

#define PAL_GET_TIMEOUT_VALUE(timer) (((timer).value*1000)/LINK_ACCEPT_TIMEOUT_UNIT_IN_MICROS)

/* bit positions in event mask page 2 */
#define EVENT_MASK_HCI_PHYSICAL_LINK_COMPLETE_EVENT_BIT_POS 0
#define EVENT_MASK_HCI_CHANNEL_SELECT_EVENT_BIT_POS 1
#define EVENT_MASK_HCI_DISCONNECT_PHYSICAL_LINK_EVENT_BIT_POS 2
#define EVENT_MASK_HCI_LOGICAL_LINK_COMPLETE_EVENT_BIT_POS 5
#define EVENT_MASK_HCI_DISCONNECT_LOGICAL_LINK_COMPLETE_EVENT_BIT_POS 6
#define EVENT_MASK_HCI_FLOW_SPEC_MODIFY_COMPLETE_EVENT_BIT_POS 7
#define EVENT_MASK_HCI_NUMBER_OF_COMPLETED_DATA_BLOCKS_EVENT_BIT_POS 8
#define EVENT_MASK_HCI_AMP_START_TEST_EVENT_BIT_POS 9
#define EVENT_MASK_HCI_AMP_TEST_END_EVENT_BIT_POS 10
#define EVENT_MASK_AMP_RECEIVER_REPORT_BIT_POS 11
#define EVENT_MASK_HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE_EVENT_BIT_POS 12
#define EVENT_MASK_HCI_AMP_STATUS_CHANGE_EVENT_BIT_POS 13

/* bit positions in event mask */
#define EVENT_MASK_HCI_QOS_VIOLATION_EVENT_BIT_POS 29
#define EVENT_MASK_HCI_ENHANCED_FLUSH_COMPLETE_EVENT_BIT_POS 56

/*Bit poisitions for supported commands */
#define SUPPORTED_COMMANDS_HCI_SET_EVENT_MASK_CMD_BIT_POS ((5*8)+6)
#define SUPPORTED_COMMANDS_HCI_RESET_CMD_BIT_POS ((5*8)+7)

#define SUPPORTED_COMMANDS_HCI_READ_CONNECTION_ACCEPT_TIMEOUT_CMD_BIT_POS ((7*8)+2)
#define SUPPORTED_COMMANDS_HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT_CMD_BIT_POS ((7*8)+3)

#define SUPPORTED_COMMANDS_HCI_READ_LINK_SUPERVISION_TIMEOUT_CMD_BIT_POS ((11*8)+0)
#define SUPPORTED_COMMANDS_HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CMD_BIT_POS ((11*8)+1)

#define SUPPORTED_COMMANDS_HCI_READ_LOCAL_VERSION_INFORMATION_CMD_BIT_POS ((14*8)+3)

#define SUPPORTED_COMMANDS_HCI_READ_FAILED_CONTACT_COUNTER_CMD_BIT_POS ((15*8)+2)
#define SUPPORTED_COMMANDS_HCI_RESET_FAILED_CONTACT_COUNTER_CMD_BIT_POS ((15*8)+3)
#define SUPPORTED_COMMANDS_HCI_READ_LINK_QUALITY_CMD_BIT_POS ((15*8)+4)
#define SUPPORTED_COMMANDS_HCI_READ_RSSI_CMD_BIT_POS ((15*8)+5)

#define SUPPORTED_COMMANDS_HCI_ENHANCED_FLUSH_CMD_BIT_POS ((19*8)+6)

#define SUPPORTED_COMMANDS_HCI_CREATE_PHYSICAL_LINK_CMD_BIT_POS ((21*8)+0)
#define SUPPORTED_COMMANDS_HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CMD_BIT_POS ((21*8)+1)
#define SUPPORTED_COMMANDS_HCI_DISCONNECT_PHYSICAL_LINK_CMD_BIT_POS ((21*8)+2)
#define SUPPORTED_COMMANDS_HCI_CREATE_LOGICAL_LINK_CMD_BIT_POS ((21*8)+3)
#define SUPPORTED_COMMANDS_HCI_ACCEPT_LOGICAL_LINK_CMD_BIT_POS ((21*8)+4)
#define SUPPORTED_COMMANDS_HCI_DISCONNECT_LOGICAL_LINK_CMD_BIT_POS ((21*8)+5)
#define SUPPORTED_COMMANDS_HCI_LOGICAL_LINK_CANCEL_CMD_BIT_POS ((21*8)+6)
#define SUPPORTED_COMMANDS_HCI_FLOW_SPEC_MODIFY_CMD_BIT_POS ((21*8)+7)

#define SUPPORTED_COMMANDS_HCI_READ_LOGICAL_LINK_ACCEPT_TIMEOUT_CMD_BIT_POS ((22*8)+0)
#define SUPPORTED_COMMANDS_HCI_WRITE_LOGICAL_LINK_ACCEPT_TIMEOUT_CMD_BIT_POS ((22*8)+1)
#define SUPPORTED_COMMANDS_HCI_SET_EVENT_MASK_PAGE2_CMD_BIT_POS ((22*8)+2)
#define SUPPORTED_COMMANDS_HCI_READ_LOCAL_AMP_INFO_CMD_BIT_POS ((22*8)+5)
#define SUPPORTED_COMMANDS_HCI_READ_LOCAL_AMP_ASSOC_CMD_BIT_POS ((22*8)+6)
#define SUPPORTED_COMMANDS_HCI_WRITE_REMOTE_AMP_ASSOC_CMD_BIT_POS ((22*8)+7)
/* We don't support these now */
#define SUPPORTED_COMMANDS_HCI_READ_LOCATION_DATA_CMD_BIT_POS ((22*8)+3)
#define SUPPORTED_COMMANDS_HCI_WRITE_LOCATION_DATA_CMD_BIT_POS ((22*8)+4)


#define SUPPORTED_COMMANDS_HCI_READ_DATA_BLOCK_SIZE_CMD_BIT_POS ((23*8)+2)

#define SUPPORTED_COMMANDS_HCI_READ_BEST_EFFORT_FLUSH_TIMEOUT_CMD_BIT_POS ((24*8)+2)
#define SUPPORTED_COMMANDS_HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CMD_BIT_POS ((24*8)+3)
#define SUPPORTED_COMMANDS_HCI_SHORT_RANGE_MODE_CMD_BIT_POS ((24*8)+4)

#define IS_BIT_SET(bitfield, pos) ((bitfield[(pos)/8] >> ((pos)%8))?TRUE:FALSE)
#define SET_BIT(bitfield, pos) ((bitfield[(pos)/8]) |= (1<<((pos)%8)))

#define HCI_COMMAND_ALLOWED(id) hci_check_command_allowed_and_supported(context, id)
#define HCI_COMMAND_NOT_ALLOWED(id) (!hci_check_command_allowed_and_supported(context, id))

/* PRIVATE TYPES DEFINITIONS ************************************************/

/**
 * @brief
 *   
 *
 * @par Description
 *   context parameters for hci bottom
 */
struct hciBottomContext
{
    CsrUint8 maxCommandsAllowed;
    CsrUint8 numCommandsAllowed;
    EventMaskPage2 eventMaskPage2;
    EventMask eventMask;
    SupportedCommands supportedCommands;
};

/**
 * @brief
 *   FSM States
 *
 * @par Description
 *   Enum defining the FSM States for this FSM process
 */
typedef enum FsmState
{
    FSMSTATE_stopped,
    FSMSTATE_starting,
    FSMSTATE_ready,
    FSMSTATE_resetting,
    FSMSTATE_stopping,
    FSMSTATE_MAX_STATE
} FsmState;


/* GLOBAL VARIABLE DEFINITIONS **********************************************/


/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *    Returns the bit position for the command code supplied in the supported command bitmap according to 
 * BT3.0+HS spec Vol 4 part E 6.26.
 *
 * @par Description
 *
 * @param[in]    pid   :  Command code 
 *
 * @return
 *    returns command position in the bitmap
 */
static int hci_get_cmd_pos(CsrUint16 cmdCode)
{
    int cmdPos=-1;

    sme_trace_entry((TR_PAL_MGR_FSM,"hci_get_cmd_pos(): Command code - 0x%2.2x ",cmdCode));

    switch(cmdCode)
    {
        case HCI_CREATE_PHYSICAL_LINK_CODE             :
            cmdPos = SUPPORTED_COMMANDS_HCI_CREATE_PHYSICAL_LINK_CMD_BIT_POS;
            break;
        case HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CODE     :
            cmdPos = SUPPORTED_COMMANDS_HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CMD_BIT_POS;
            break;
        case HCI_DISCONNECT_PHYSICAL_LINK_CODE         :
            cmdPos = SUPPORTED_COMMANDS_HCI_DISCONNECT_PHYSICAL_LINK_CMD_BIT_POS;
            break;
        case HCI_CREATE_LOGICAL_LINK_CODE              :
            cmdPos = SUPPORTED_COMMANDS_HCI_CREATE_LOGICAL_LINK_CMD_BIT_POS;
            break;
        case HCI_ACCEPT_LOGICAL_LINK_CODE              :
            cmdPos = SUPPORTED_COMMANDS_HCI_ACCEPT_LOGICAL_LINK_CMD_BIT_POS;
            break;
        case HCI_DISCONNECT_LOGICAL_LINK_CODE          :
            cmdPos = SUPPORTED_COMMANDS_HCI_DISCONNECT_LOGICAL_LINK_CMD_BIT_POS;
            break;
        case HCI_LOGICAL_LINK_CANCEL_CODE              :
            cmdPos = SUPPORTED_COMMANDS_HCI_LOGICAL_LINK_CANCEL_CMD_BIT_POS;
            break;
        case HCI_FLOW_SPEC_MODIFY_CODE                 :
            cmdPos = SUPPORTED_COMMANDS_HCI_FLOW_SPEC_MODIFY_CMD_BIT_POS;
            break;
        case HCI_SET_EVENT_MASK_CODE                   :
            cmdPos = SUPPORTED_COMMANDS_HCI_SET_EVENT_MASK_CMD_BIT_POS;
            break;
        case HCI_RESET_CODE                            :
            cmdPos = SUPPORTED_COMMANDS_HCI_RESET_CMD_BIT_POS;
            break;
        case HCI_FLUSH_CODE                            :
            break;
        case HCI_READ_CONNECTION_ACCEPT_TIMEOUT_CODE   :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_CONNECTION_ACCEPT_TIMEOUT_CMD_BIT_POS;
            break;
        case HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT_CODE  :
            cmdPos = SUPPORTED_COMMANDS_HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT_CMD_BIT_POS;
            break;
        case HCI_READ_LINK_SUPERVISION_TIMEOUT_CODE    :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_LINK_SUPERVISION_TIMEOUT_CMD_BIT_POS;
            break;
        case HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CODE   :
            cmdPos = SUPPORTED_COMMANDS_HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CMD_BIT_POS;
            break;
        case HCI_ENHANCED_FLUSH_CODE                   :
            cmdPos = SUPPORTED_COMMANDS_HCI_ENHANCED_FLUSH_CMD_BIT_POS;
            break;
        case HCI_READ_LOGICAL_LINK_ACCEPT_TIMEOUT_CODE :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_LOGICAL_LINK_ACCEPT_TIMEOUT_CMD_BIT_POS;
            break;
        case HCI_WRITE_LOGICAL_LINK_ACCEPT_TIMEOUT_CODE:
            cmdPos = SUPPORTED_COMMANDS_HCI_WRITE_LOGICAL_LINK_ACCEPT_TIMEOUT_CMD_BIT_POS;
            break;
        case HCI_SET_EVENT_MASK_PAGE2_CODE             :
            cmdPos = SUPPORTED_COMMANDS_HCI_SET_EVENT_MASK_PAGE2_CMD_BIT_POS;
            break;
        case HCI_READ_LOCATION_DATA_CODE               :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_LOCATION_DATA_CMD_BIT_POS;
            break;
        case HCI_WRITE_LOCATION_DATA_CODE              :
            cmdPos = SUPPORTED_COMMANDS_HCI_WRITE_LOCATION_DATA_CMD_BIT_POS;
            break;
        case HCI_READ_BEST_EFFORT_FLUSH_TIMEOUT_CODE   :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_BEST_EFFORT_FLUSH_TIMEOUT_CMD_BIT_POS;
            break;
        case HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CODE  :
            cmdPos = SUPPORTED_COMMANDS_HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CMD_BIT_POS;
            break;
        case HCI_SHORT_RANGE_MODE_CODE                 :
            cmdPos = SUPPORTED_COMMANDS_HCI_SHORT_RANGE_MODE_CMD_BIT_POS;
            break;
        case HCI_READ_LOCAL_VERSION_INFORMATION_CODE   :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_LOCAL_VERSION_INFORMATION_CMD_BIT_POS;
            break;
        case HCI_READ_LOCAL_SUPPORTED_COMMANDS_CODE    :
            break;
        case HCI_READ_DATA_BLOCK_SIZE_CODE             :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_DATA_BLOCK_SIZE_CMD_BIT_POS;
            break;
        case HCI_READ_FAILED_CONTACT_COUNTER_CODE      :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_FAILED_CONTACT_COUNTER_CMD_BIT_POS;
            break;
        case HCI_RESET_FAILED_CONTACT_COUNTER_CODE     :
            cmdPos = SUPPORTED_COMMANDS_HCI_RESET_FAILED_CONTACT_COUNTER_CMD_BIT_POS;
            break;
        case HCI_READ_LINK_QUALITY_CODE                :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_LINK_QUALITY_CMD_BIT_POS;
            break;
        case HCI_READ_RSSI_CODE                        :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_RSSI_CMD_BIT_POS;
            break;
        case HCI_READ_LOCAL_AMP_INFO_CODE              :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_LOCAL_AMP_INFO_CMD_BIT_POS;
            break;
        case HCI_READ_LOCAL_AMP_ASSOC_CODE             :
            cmdPos = SUPPORTED_COMMANDS_HCI_READ_LOCAL_AMP_ASSOC_CMD_BIT_POS;
            break;
        case HCI_WRITE_REMOTE_AMP_ASSOC_CODE           :
            cmdPos = SUPPORTED_COMMANDS_HCI_WRITE_REMOTE_AMP_ASSOC_CMD_BIT_POS;
            break;
        case HCI_READ_LOOPBACK_MODE_CODE               :
            break;
        case HCI_WRITE_LOOPBACK_MODE_CODE              :
            break;
        case HCI_ENABLE_AMP_TEST_MODE_CODE             :
            break;
        case HCI_SET_AMP_TRANSMIT_POWER_TEST_CODE      :
            break;
        case HCI_ENABLE_AMP_RECEIVER_REPORTS_CODE      :
            break;
        case HCI_TRANSMITTER_AMP_TEST_CODE             :
            break;
        case HCI_RECEIVER_AMP_TEST_CODE                :
            break;
        case HCI_AMP_TEST_END_CODE                     :
            break;
        default:
            sme_trace_warn((TR_PAL_MGR_FSM,"hci_get_cmd_pos(): Invalid command code - 0x%2.2x ",cmdCode));
            break;
    }
    verify(TR_PAL_MGR_FSM,cmdPos != -1);

    sme_trace_info((TR_PAL_MGR_FSM,"<<hci_get_cmd_pos(): Command Position - %d ",cmdPos));
    return cmdPos;
}


/**
 * @brief
 *    Check if command is allowed and supported.
 *
 * @par Description
 *     Return TRUE if the command is allowed and command is supported by this version of software
 * @return
 *    TRUE if command is allowed and supported else FALSE
 */
static CsrBool hci_check_command_allowed_and_supported(FsmContext *context, CsrUint16 id)
{
    int bitPosition = hci_get_cmd_pos(id);
    return (getHciBottomContext(context)->numCommandsAllowed && IS_BIT_SET(getHciBottomContext(context)->supportedCommands.m,bitPosition));
}

/**
 * @brief
 *    Check the bit to see if the event is enabled by the host.
 *
 * @par Description
 *     A event is enabled if the corresponding bit is set. An event if forwarded only if the host
 * has enabled the event.
 * section 7.3.6.9 of HCI spec
 * Value                             Parameter Description
 * 0x0000000000000000 No events specified (default)
 * 0x0000000000000001 Physical Link Complete Event
 * 0x0000000000000002 Channel Selected Event
 * 0x0000000000000004 Disconnect Physical Link Event
 * 0x0000000000000008 Physical Link Loss Early Warning Event
 * 0x0000000000000010 Physical Link Recovery Event
 * 0x0000000000000020 Logical Link Complete Event
 * 0x0000000000000040 Disconnection Logical Link Complete Event
 * 0x0000000000000080 Flow Spec Modify Complete Event
 * 0x0000000000000100 Number of Completed Data Blocks Event
 * 0x0000000000000200 AMP Start Test Event
 * 0x0000000000000400 AMP Test End Event
 * 0x0000000000000800 AMP Receiver Report Event
 * 0x0000000000001000 Short Range Mode Change Complete Event
 * 0x0000000000002000 AMP Status Change Event
 *
 * And in hci_set_event_mask command
 * section  7.3.1
 * 0x0000000020000000 QoS Violation Event
 * 0x0100000000000000 Enhanced Flush Complete Event
 *
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    eventId   :  event id to be checked
 *
 * @return
 *    TRUE if event is enabled, else FALSE
 */
static CsrBool hci_event_enabled(FsmContext *context, CsrUint16 eventId)
{
    CsrBool enabled=FALSE;

    sme_trace_entry((TR_PAL_MGR_FSM, "hci_event_enabled(): eventId-0x%2.2x",eventId));
    sme_trace_hex((TR_PAL_MGR_FSM, TR_LVL_ENTRY, "hci_event_enabled(): HCI Event Page2 Mask:: ", getHciBottomContext(context)->eventMaskPage2.m, 
        sizeof(getHciBottomContext(context)->eventMaskPage2.m)));
    sme_trace_hex((TR_PAL_MGR_FSM, TR_LVL_ENTRY, "hci_event_enabled(): HCI Event Mask:: ", getHciBottomContext(context)->eventMask.m, 
        sizeof(getHciBottomContext(context)->eventMask.m)));


    switch (eventId)
    {
        case HCI_PHYSICAL_LINK_COMPLETE_CODE:
            if (IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_PHYSICAL_LINK_COMPLETE_EVENT_BIT_POS))
            {
                enabled=TRUE;
            }
            break;

        case HCI_CHANNEL_SELECT_CODE:
            if (IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_CHANNEL_SELECT_EVENT_BIT_POS))
            {
                enabled=TRUE;
            }
            break;

        case HCI_DISCONNECT_PHYSICAL_LINK_COMPLETE_CODE:
            sme_trace_entry((TR_PAL_MGR_FSM, "hci_event_enabled():bit pos-%d, mask-0x%2.2x",EVENT_MASK_HCI_DISCONNECT_PHYSICAL_LINK_EVENT_BIT_POS, getHciBottomContext(context)->eventMaskPage2.m[EVENT_MASK_HCI_DISCONNECT_PHYSICAL_LINK_EVENT_BIT_POS/8]));
            if (IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_DISCONNECT_PHYSICAL_LINK_EVENT_BIT_POS))
            {
                sme_trace_entry((TR_PAL_MGR_FSM, "hci_event_enabled(): HCI_DISCONNECT_PHYSICAL_LINK_COMPLETE_CODE  enabled"));
                enabled=TRUE;
            }
            break;

/*
        case HCI_PHYSICAL_LINK_LOSS_EARLY_WARNING_CODE:
        case HCI_PHYSICAL_LINK_RECOVERY_CODE:
            break;
         802.11 AMPs don't support these events . Refer section 2.10 & 2.11 of PAL spec
*/
        case HCI_LOGICAL_LINK_COMPLETE_CODE:
            if (IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_LOGICAL_LINK_COMPLETE_EVENT_BIT_POS))
            {
                enabled=TRUE;
            }
            break;

        case HCI_DISCONNECT_LOGICAL_LINK_COMPLETE_CODE:
            if (IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_DISCONNECT_LOGICAL_LINK_COMPLETE_EVENT_BIT_POS))
            {
                enabled=TRUE;
            }
            break;

        case HCI_FLOW_SPEC_MODIFY_COMPLETE_CODE:
            if (IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_FLOW_SPEC_MODIFY_COMPLETE_EVENT_BIT_POS))
            {
                enabled=TRUE;
            }
            break;

/*
        This will be done by data manager 
        case HCI_NUMBER_OF_COMPLETED_DATA_BLOCKS_CODE:
        case HCI_QOS_VIOLATION_CODE:
            break;
*/
        /* these events are not implemented yet in PAL */
#ifdef NOT_SUPPORTED
        case AMP_START_TEST_CODE:
            if (getHciBottomContext(context)->eventMaskPage2.m[1] & 0x02)
            {
                enabled=TRUE;
            }
            break;

        case AMP_TEST_END_CODE:
            if (getHciBottomContext(context)->eventMaskPage2.m[1] & 0x04)
            {
                enabled=TRUE;
            }
            break;
#endif

        case AMP_RECEIVER_REPORT_CODE:
            if (IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_AMP_RECEIVER_REPORT_BIT_POS))
            {
                enabled=TRUE;
            }
            break;

        case HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE_CODE:
            if (IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE_EVENT_BIT_POS))
            {
                enabled=TRUE;
            }
            break;

        case HCI_AMP_STATUS_CHANGE_CODE:
            if (IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_AMP_STATUS_CHANGE_EVENT_BIT_POS))
            {
                enabled=TRUE;
            }
            break;

        case HCI_ENHANCED_FLUSH_COMPLETE_CODE:
            sme_trace_entry((TR_PAL_MGR_FSM, "hci_event_enabled():bit pos-%d, mask-0x%2.2x",EVENT_MASK_HCI_ENHANCED_FLUSH_COMPLETE_EVENT_BIT_POS, getHciBottomContext(context)->eventMask.m[EVENT_MASK_HCI_ENHANCED_FLUSH_COMPLETE_EVENT_BIT_POS/8]));
            if (IS_BIT_SET(getHciBottomContext(context)->eventMask.m, EVENT_MASK_HCI_ENHANCED_FLUSH_COMPLETE_EVENT_BIT_POS))
            {
                enabled=TRUE;
            }
            break;

        default:
            verify(TR_PAL_LM_HIP_FSM,eventId==0); /* Something that will definitely fail and should not generate lint warning */
            break;
    }
    return enabled;
}

/**
 * @brief
 *    Forward the HCI command to appropriate FSM process ID.
 *
 * @par Description
 *     The command is forwarded only if cmdStatus is HCI_STATUS_CODE_SUCCESS and
 * it hasn't reached the maximum allowed limit for processing commands.
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    pid   :  process ID for which the event is forwarded to
 * @param[in]    evt   :  event to be forwarded
 * @param[in]    cmdStatus   :  status of the command. Forwarded only if this is set to HCI_STATUS_CODE_SUCCESS
 *
 * @return
 *    void
 */
static void hci_forward_cmd(FsmContext *context, CsrUint16 pid, const FsmEvent *evt, HciStatusCode cmdStatus)
{
    CsrBool sendCmdStatus=TRUE;

    if (HCI_COMMAND_NOT_ALLOWED(evt->id))
    {
        sme_trace_warn((TR_PAL_MGR_FSM,"hci_forward_cmd(): Command not allowed as max allowed(%d) pending already",getHciBottomContext(context)->numCommandsAllowed));
        cmdStatus = HCI_STATUS_CODE_COMMAND_DISALLOWED;
    }
    else if (HCI_STATUS_CODE_SUCCESS == cmdStatus)
    {
        sendCmdStatus=FALSE;
        if (HCI_RESET_CODE == evt->id)
        {
            sme_trace_info((TR_PAL_MGR_FSM,"hci_forward_cmd(): reset command- no further command allowed until this is complete!!"));
            getHciBottomContext(context)->numCommandsAllowed = 0;
        }
        else
        {
            getHciBottomContext(context)->numCommandsAllowed--;
        }
        fsm_forward_event(context, pid, evt);
    }

    if (sendCmdStatus)
    {
        /* FIXME: What about those commands which has HCI-Command-Complete as a response. 
        * It is unclear from the HCI spec whether those should also get the HCI-Command-Status event
        */
        sme_trace_info((TR_PAL_MGR_FSM, "hci_forward_cmd(): command-0x%x failed with status-%d",evt->id,cmdStatus));
        call_hci_command_status_evt(context, cmdStatus,
                                           getHciBottomContext(context)->numCommandsAllowed,evt->id);
    }
}

/* FSM DEFINITION **********************************************/

/**
 * @brief
 *    PAL Manager FSM initialisation
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    void
 */
static void pal_mgr_init(FsmContext* context)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "pal_mgr_init()"));
    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *    PAL Manager FSM dump
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    id   :  process id
 *
 * @return
 *    void
 */
#ifdef FSM_DEBUG_DUMP
static void lm_mgr_dump(FsmContext* context, const CsrUint16 id)
{

}
#endif

/**
 * @brief
 *    PAL Manager FSM reset
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    void
 */
static void lm_mgr_reset(FsmContext* context)
{


}

/**
 * @brief
 *    get next active LM process ID
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    return process ID if an LM exits else returns FSM_TERMINATE
 */
static CsrUint16 get_next_lm_instance(FsmContext *context)
{
    int i;

    sme_trace_entry((TR_PAL_MGR_FSM,"get_next_lm_instance()"));
    for (i=0; i<getPalMgrContext(context)->maxPhysicalLinks; i++)
    {
        if (getPalMgrContext(context)->phyLinkSharedAttrib[i].used)
        {
            sme_trace_info((TR_PAL_MGR_FSM,"physical link entry found for handle-%d,pid-%d",
                           getPalMgrContext(context)->phyLinkSharedAttrib[i].physicalLinkHandle,
                           getPalMgrContext(context)->phyLinkSharedAttrib[i].linkFsmPid));
            return getPalMgrContext(context)->phyLinkSharedAttrib[i].linkFsmPid;
        }
    }
    return FSM_TERMINATE;
}

/**
 * @brief
 *    Wifi is powered off
 *
 * @par Description
 *    
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    void
 */
static void pal_hci_stopped(FsmContext *context)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "pal_hci_stopped(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));
    pal_mgr_update_sme_connection_status(context,PAL_SME_CONNECTION_STATUS_POWER_OFF);
}

/**
 * @brief
 *    Wifi is powered on
 *
 * @par Description
 *    .
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    void
 */
static void pal_hci_started(FsmContext *context)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "pal_hci_started(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));
    pal_mgr_update_sme_connection_status(context,PAL_SME_CONNECTION_STATUS_DISCONNECTED);
}

/**
 * @brief
 *    Forward  HCI command based on the logical link handle
 *
 * @par Description
 *   Function wil determine the Link FSM process id based on the logical link handle supplied
 * to forward the command.
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    cmd  :  command to forward
 * @param[in]    logicalLinkHandle   :  logical link handle
 *
 * @return
 *    void
 */
static void hci_forward_on_logical_link_handle(FsmContext* context, const FsmEvent *cmd, CsrUint16 logicalLinkHandle)
{
    CsrUint16 lmFsmPid;
    HciStatusCode cmdStatus=HCI_STATUS_CODE_SUCCESS;

    lmFsmPid = pal_get_link_fsm_pid_from_handle(context,
                        pal_llm_get_phy_link_handle_from_logical_link_handle(getLlmContext(context), logicalLinkHandle));

    if (FSM_TERMINATE==lmFsmPid)
    {
        cmdStatus = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
    }
    hci_forward_cmd(context, lmFsmPid, cmd, cmdStatus);
}

/**
 * @brief
 *    Forward  HCI command based on the physical link handle
 *
 * @par Description
 *   Function wil determine the Link FSM process id based on the physical link handle supplied
 * to forward the command.
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    cmd  :  command to forward
 * @param[in]    handle   :  physical link handle
 *
 * @return
 *    void
 */
static void hci_forward_on_phy_link_handle(FsmContext* context, const FsmEvent *cmd, CsrUint8 handle)
{
    CsrUint16 lmFsmPid;
    HciStatusCode cmdStatus=HCI_STATUS_CODE_SUCCESS;

    lmFsmPid = pal_get_link_fsm_pid_from_handle(context, handle);

    sme_trace_info((TR_PAL_MGR_FSM,"hci_forward_on_phy_link_handle():LinkFSM-Pid-%d, handle-%d",lmFsmPid,handle));
    if (FSM_TERMINATE==lmFsmPid)
    {
        cmdStatus = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
    }
    hci_forward_cmd(context, lmFsmPid, cmd, cmdStatus);
}

/**
 * @brief
 *    Forward  PAL-CTRL event based on the physical link handle
 *
 * @par Description
 *   Function wil determine the Link FSM process id based on the physical link handle supplied
 * to forward the event.
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    evt  :  event to forward
 * @param[in]    physicalLinkHandle   :  physical link handle
 *
 * @return
 *    void
 */
static void pal_ctrl_forward_on_physical_link_handle(FsmContext* context, const FsmEvent *evt, CsrUint8 physicalLinkHandle)
{
    CsrUint16 lmFsmPid;

    lmFsmPid = pal_get_link_fsm_pid_from_handle(context, physicalLinkHandle);

    if (FSM_TERMINATE!=lmFsmPid)
    {
        fsm_forward_event(context, lmFsmPid, evt);
    }
}

/**
 * @brief
 *    Forward  PAL-CTRL event based on the logical link handle
 *
 * @par Description
 *   Function wil determine the Link FSM process id based on the logical link handle supplied
 * to forward the event.
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    evt  :  event to forward
 * @param[in]    logicalLinkHandle   :  logical link handle
 *
 * @return
 *    void
 */
static void pal_ctrl_forward_on_logical_link_handle(FsmContext* context, const FsmEvent *evt, CsrUint16 logicalLinkHandle)
{
    pal_ctrl_forward_on_physical_link_handle(context,
                                             evt,
                                             pal_llm_get_phy_link_handle_from_logical_link_handle(getLlmContext(context), logicalLinkHandle));
}


/********************* TRANSITION FUNCTIONS *************************************************************/

static void stopped__hci_command_complete_evt(FsmContext *context, const HciCommandCompleteEvt *evt)
{
    getHciBottomContext(context)->numCommandsAllowed++;
    sme_trace_entry((TR_PAL_MGR_FSM, "stopped__hci_command_complete_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));

    call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &evt->returnParameters);

    if (HCI_READ_LOCAL_AMP_INFO_CODE == evt->returnParameters.commandCode &&
        HCI_STATUS_CODE_SUCCESS == evt->returnParameters.hciReturnParam.readLocalAmpInfoReturn.status &&
        !getPalMgrContext(context)->initialLocalAmpInfoRead)
    {
        sme_trace_info((TR_PAL_MGR_FSM, "stopped__hci_command_complete_evt(): Initial Local-Amp-Info read"));
        getPalMgrContext(context)->initialLocalAmpInfoRead = TRUE;
    }
}


static void ready__hci_command_status_evt(FsmContext *context, const HciCommandStatusEvt *evt)
{
    /* currently used only to let host know that PAL is available  */
    getHciBottomContext(context)->numCommandsAllowed++;
    sme_trace_entry((TR_PAL_MGR_FSM, "ready__hci_command_status_evt(): status-%d,num allowed command-%d",evt->status, getHciBottomContext(context)->numCommandsAllowed));

    if ((HCI_CREATE_PHYSICAL_LINK_CODE == evt->commandOpcode || HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CODE == evt->commandOpcode)
        && evt->status != HCI_STATUS_CODE_SUCCESS)
    {
        sme_trace_info((TR_PAL_MGR_FSM, "ready__hci_command_status_evt(): delete lm instance as the link creation failed"));
        pal_delete_lm_instance(context, evt->common.sender_);
    }

    call_hci_command_status_evt(context, evt->status,
                                       getHciBottomContext(context)->numCommandsAllowed,evt->commandOpcode);
}

/* functions called by PAL Core to send events */
static void ready__hci_command_complete_evt(FsmContext *context, const HciCommandCompleteEvt *evt)
{
    getHciBottomContext(context)->numCommandsAllowed++;
    sme_trace_entry((TR_PAL_MGR_FSM, "ready__hci_command_complete_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));

    call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &evt->returnParameters);

    if (HCI_READ_LOCAL_AMP_INFO_CODE == evt->returnParameters.commandCode &&
        HCI_STATUS_CODE_SUCCESS == evt->returnParameters.hciReturnParam.readLocalAmpInfoReturn.status &&
        !getPalMgrContext(context)->initialLocalAmpInfoRead)
    {
        sme_trace_info((TR_PAL_MGR_FSM, "ready__hci_command_complete_evt(): Initial Local-Amp-Info read"));
        getPalMgrContext(context)->initialLocalAmpInfoRead = TRUE;
    }
}

static void pal_call_hci_reset_complete(FsmContext *context)
{
    ReturnParameters returnParams;

    getHciBottomContext(context)->numCommandsAllowed = getHciBottomContext(context)->maxCommandsAllowed;
    sme_trace_entry((TR_PAL_MGR_FSM, "pal_call_hci_reset_complete(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));

    returnParams.commandCode = HCI_RESET_CODE;
    returnParams.hciReturnParam.readLocalAmpInfoReturn.status = HCI_STATUS_CODE_SUCCESS;
    call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &returnParams);
}

static void ready__hci_disconnect_physical_link_complete_evt(FsmContext *context, HciDisconnectPhysicalLinkCompleteEvt *evt)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "ready__hci_disconnect_physical_link_complete_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));

    /* delete the lm instance entry*/
    pal_delete_lm_instance(context, evt->common.sender_);
    if (hci_event_enabled(context, evt->common.id))
    {
        call_hci_disconnect_physical_link_complete_evt(context, evt->status, evt->physicalLinkHandle, evt->reason);
    }
}

static void ready__hci_physical_link_complete_evt(FsmContext *context, HciPhysicalLinkCompleteEvt *evt)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "ready__hci_physical_link_complete_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));

    if (evt->status != HCI_STATUS_CODE_SUCCESS)
    {
        sme_trace_info((TR_PAL_MGR_FSM, "ready__hci_physical_link_complete_evt(): delete lm instance as the link creation failed"));
        pal_delete_lm_instance(context, evt->common.sender_);
    }

    if (hci_event_enabled(context, evt->common.id))
    {
        call_hci_physical_link_complete_evt(context, evt->status, evt->physicalLinkHandle);
    }
}

static void ready__hci_channel_select_evt(FsmContext *context, HciChannelSelectEvt *evt)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "ready__hci_channel_select_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));
    if (hci_event_enabled(context, evt->common.id))
    {
        call_hci_channel_select_evt(context, evt->physicalLinkHandle);
    }
}

static void ready__hci_disconnect_logical_link_complete_evt(FsmContext *context, HciDisconnectLogicalLinkCompleteEvt *evt)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "ready__hci_disconnect_logical_link_complete_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));
    if (hci_event_enabled(context, evt->common.id))
    {
        call_hci_disconnect_logical_link_complete_evt(context, evt->status, evt->logicalLinkHandle, evt->reason);
    }
}

static void ready__hci_logical_link_complete_evt(FsmContext *context, HciLogicalLinkCompleteEvt *evt)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "ready__hci_logical_link_complete_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));
    if (hci_event_enabled(context, evt->common.id))
    {
        call_hci_logical_link_complete_evt(context, evt->status, evt->logicalLinkHandle, evt->physicalLinkHandle, evt->txFlowSpecId);
    }
}

static void ready__hci_flow_spec_modify_complete_evt(FsmContext *context, HciFlowSpecModifyCompleteEvt *evt)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "pal_call_hci_modify_flow_spec_complete(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));
    if (hci_event_enabled(context, evt->common.id))
    {
        call_hci_flow_spec_modify_complete_evt(context, evt->status, evt->handle);
    }
}

static void ready__hci_short_range_mode_change_complete_evt(FsmContext *context, HciShortRangeModeChangeCompleteEvt *evt)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "pal_call_hci_short_range_mode_change_complete(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));
    if (hci_event_enabled(context, evt->common.id))
    {
        call_hci_short_range_mode_change_complete_evt(context, evt->status, evt->physicalLinkHandle, evt->shortRangeModeState);
    }
}

static void ready__hci_enhanced_flush_complete_evt(FsmContext *context, HciEnhancedFlushCompleteEvt *evt)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "ready__hci_enhanced_flush_complete_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));
    if (hci_event_enabled(context, evt->common.id))
    {
        call_hci_enhanced_flush_complete_evt(context, evt->handle);
    }
}

static void ready__hci_reset_cmd(FsmContext* context, const HciResetCmd *cmd)
{
    CsrUint16 lmFsmPid = get_next_lm_instance(context);
    if (VALID_PID(lmFsmPid))
    {
        hci_forward_cmd(context,lmFsmPid, (FsmEvent *)cmd, HCI_STATUS_CODE_SUCCESS);
    }
    else
    {
        hci_forward_cmd(context,getSmeContext(context)->palDmFsmInstance, (FsmEvent *)cmd, HCI_STATUS_CODE_SUCCESS);
    }
    fsm_next_state(context, FSMSTATE_resetting);
}


static void ready__hci_write_connection_accept_timeout_cmd(FsmContext *context, const HciWriteConnectionAcceptTimeoutCmd *cmd)
{
   ReturnParameters retParams;

   retParams.commandCode = HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT_CODE;
   if (HCI_COMMAND_ALLOWED(cmd->common.id))
   {
       if (PAL_LA_TIMEOUT_IN_VALID_RANGE(cmd->connAcceptTimeout))
       {
           retParams.hciReturnParam.writeConnectionAcceptTimeoutStatus = HCI_STATUS_CODE_SUCCESS;
           pal_set_connection_accept_timer(context, cmd->connAcceptTimeout);
       }
       else
       {
           retParams.hciReturnParam.writeConnectionAcceptTimeoutStatus = HCI_STATUS_CODE_INVALID_HCI_COMMAND_PARAMETERS;
       }
   }
   else
   {
       retParams.hciReturnParam.writeConnectionAcceptTimeoutStatus = HCI_STATUS_CODE_COMMAND_DISALLOWED;
   }
   call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &retParams);
}

static void ready__hci_write_logical_link_accept_timeout_cmd(FsmContext *context, const HciWriteLogicalLinkAcceptTimeoutCmd *cmd)
{
   ReturnParameters retParams;

   retParams.commandCode = HCI_WRITE_LOGICAL_LINK_ACCEPT_TIMEOUT_CODE;
   if (HCI_COMMAND_ALLOWED(cmd->common.id))
   {
       if (PAL_LA_TIMEOUT_IN_VALID_RANGE(cmd->logicalLinkAcceptTimeout))
       {
           retParams.hciReturnParam.writeLogicalLinkAcceptTimeout = HCI_STATUS_CODE_SUCCESS;
           pal_set_logical_link_accept_timer(context, cmd->logicalLinkAcceptTimeout);
       }
       else
       {
           retParams.hciReturnParam.writeLogicalLinkAcceptTimeout = HCI_STATUS_CODE_UNSPECIFIED_ERROR;
       }
   }
   else
   {
       retParams.hciReturnParam.writeLogicalLinkAcceptTimeout = HCI_STATUS_CODE_COMMAND_DISALLOWED;
   }
   call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &retParams);
}

static void ready__hci_read_connection_accept_timeout_cmd(FsmContext *context, const HciReadConnectionAcceptTimeoutCmd *cmd)
{
   ReturnParameters retParams;
   PAL_Timer *timer;

   retParams.commandCode = HCI_READ_CONNECTION_ACCEPT_TIMEOUT_CODE;
   if (HCI_COMMAND_ALLOWED(cmd->common.id))
   {
       retParams.hciReturnParam.readConnectionAcceptTimeout.status = HCI_STATUS_CODE_SUCCESS;

       timer = pal_get_connection_accept_timer(context);
       retParams.hciReturnParam.readConnectionAcceptTimeout.connAcceptTimeout = (CsrUint16)PAL_GET_TIMEOUT_VALUE(*timer);
   }
   else
   {
       retParams.hciReturnParam.readConnectionAcceptTimeout.status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
   }
   call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &retParams);
}

static void ready__hci_read_logical_link_accept_timeout_cmd(FsmContext *context, const HciReadLogicalLinkAcceptTimeoutCmd *cmd)
{
   ReturnParameters retParams;
   PAL_Timer *timer;

   retParams.commandCode = HCI_READ_LOGICAL_LINK_ACCEPT_TIMEOUT_CODE;
   if (HCI_COMMAND_ALLOWED(cmd->common.id))
   {
       retParams.hciReturnParam.readLogicalLinkAcceptTimeout.status = HCI_STATUS_CODE_SUCCESS;

       timer = pal_get_connection_accept_timer(context);
       retParams.hciReturnParam.readLogicalLinkAcceptTimeout.logicalLinkAcceptTimeout = (CsrUint16)PAL_GET_TIMEOUT_VALUE(*timer);
   }
   else
   {
       retParams.hciReturnParam.readLogicalLinkAcceptTimeout.status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
   }
   call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &retParams);
}

static void ready__hci_read_local_version_information_cmd(FsmContext *context, const HciReadLocalVersionInformationCmd *cmd)
{
   ReturnParameters retParams;

   retParams.commandCode = HCI_READ_LOCAL_VERSION_INFORMATION_CODE;
   if (HCI_COMMAND_ALLOWED(cmd->common.id))
   {
       retParams.hciReturnParam.readLocalVersionInformationReturn.status = HCI_STATUS_CODE_SUCCESS;

       retParams.hciReturnParam.readLocalVersionInformationReturn.hciVersion=PAL_HCI_VERSION;
       retParams.hciReturnParam.readLocalVersionInformationReturn.hciRevision=PAL_HCI_REVISION;
       retParams.hciReturnParam.readLocalVersionInformationReturn.palVersion=PAL_VERSION;
       retParams.hciReturnParam.readLocalVersionInformationReturn.manufacturerName=PAL_BT_SIG_COMPANY_IDENTIFIER;
       retParams.hciReturnParam.readLocalVersionInformationReturn.palSubversion=PAL_SUB_VERSION;
   }
   else
   {
       retParams.hciReturnParam.readLocalVersionInformationReturn.status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
   }
   call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &retParams);
}

static void ready__hci_read_local_supported_commands_cmd(FsmContext *context, const HciReadLocalSupportedCommandsCmd *cmd)
{
   ReturnParameters retParams;

   retParams.commandCode = HCI_READ_LOCAL_SUPPORTED_COMMANDS_CODE;
   if (getHciBottomContext(context)->numCommandsAllowed)
   {
       retParams.hciReturnParam.readLocalSupportedCommandsReturn.status = HCI_STATUS_CODE_SUCCESS;
       retParams.hciReturnParam.readLocalSupportedCommandsReturn.supportedCommands = getHciBottomContext(context)->supportedCommands;
   }
   else
   {
       retParams.hciReturnParam.readLocalSupportedCommandsReturn.status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
   }
   call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &retParams);
}

static void hci_unsupported_cmd(FsmContext* context, FsmEvent *cmd)
{
    call_hci_command_status_evt(context, HCI_STATUS_CODE_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE,
                                       getHciBottomContext(context)->numCommandsAllowed,
                                       cmd->id);
}

static void ready__hci_create_physical_link_cmd(FsmContext* context, const HciCreatePhysicalLinkCmd *cmd)
{
    CsrUint16 lmFsmPid=FSM_TERMINATE;
    HciStatusCode cmdStatus=HCI_STATUS_CODE_COMMAND_DISALLOWED;

    sme_trace_entry((TR_PAL_MGR_FSM,"hci_create_physical_link_cmd(): handle-%d",cmd->physicalLinkHandle));
    /* create it only if commands are allowed */
    if (HCI_COMMAND_ALLOWED(cmd->common.id) &&
        pal_mgr_amp_connections_allowed(context))
    {
        lmFsmPid = pal_lm_create_instance(context, cmd->physicalLinkHandle);

        sme_trace_info((TR_PAL_MGR_FSM,"hci_create_physical_link_cmd():LinkFSM-Pid-%d, handle-%d",lmFsmPid,cmd->physicalLinkHandle));
        if (FSM_TERMINATE != lmFsmPid)
        {
            cmdStatus = HCI_STATUS_CODE_SUCCESS;
        }
    }

    if (!pal_mgr_amp_connections_allowed(context))
    {
        /* FIXME: Is this the right status code??? */
        sme_trace_info((TR_PAL_MGR_FSM,"hci_create_physical_link_cmd(): AMP connection not allowed by SME"));
        cmdStatus=HCI_STATUS_CODE_UNSPECIFIED_ERROR;
    }

    hci_forward_cmd(context,lmFsmPid, (FsmEvent *)cmd, cmdStatus);
}

static void ready__hci_accept_physical_link_request_cmd(FsmContext* context, const HciAcceptPhysicalLinkRequestCmd *cmd)
{
    CsrUint16 lmFsmPid=FSM_TERMINATE;
    HciStatusCode cmdStatus=HCI_STATUS_CODE_COMMAND_DISALLOWED;

    sme_trace_entry((TR_PAL_MGR_FSM,"hci_accept_physical_link_request_cmd(): handle-%d",cmd->physicalLinkHandle));
    /* create it only if commands are allowed */
    if (HCI_COMMAND_ALLOWED(cmd->common.id) &&
        pal_mgr_amp_connections_allowed(context))
    {
        lmFsmPid = pal_lm_create_instance(context, cmd->physicalLinkHandle);

        sme_trace_info((TR_PAL_MGR_FSM,"hci_accept_physical_link_request_cmd():LinkFSM-Pid-%d, handle-%d",lmFsmPid,cmd->physicalLinkHandle));
        if (FSM_TERMINATE != lmFsmPid)
        {
            cmdStatus = HCI_STATUS_CODE_SUCCESS;
        }
    }

    if (!pal_mgr_amp_connections_allowed(context))
    {
        /* FIXME: Is this the right status code??? */
        sme_trace_info((TR_PAL_MGR_FSM,"hci_accept_physical_link_request_cmd(): AMP connection not allowed by SME"));
        cmdStatus=HCI_STATUS_CODE_UNSPECIFIED_ERROR;
    }

    hci_forward_cmd(context,lmFsmPid, (FsmEvent *)cmd, cmdStatus);
}

static void ready__hci_disconnect_physical_link_cmd(FsmContext* context, const HciDisconnectPhysicalLinkCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_disconnect_physical_link_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, cmd->physicalLinkHandle);
}

static void ready__hci_create_logical_link_cmd(FsmContext* context, const HciCreateLogicalLinkCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_create_logical_link_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, cmd->physicalLinkHandle);
}

static void ready__hci_accept_logical_link_cmd(FsmContext* context, const HciAcceptLogicalLinkCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_accept_logical_link_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, cmd->physicalLinkHandle);
}

static void ready__hci_logical_link_cancel_cmd(FsmContext* context, const HciLogicalLinkCancelCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_logical_link_cancel_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, cmd->physicalLinkHandle);
}

static void ready__hci_write_remote_amp_assoc_cmd(FsmContext* context, const HciWriteRemoteAmpAssocCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_write_remote_amp_assoc_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, cmd->physicalLinkHandle);
}

static void ready__hci_read_link_quality_cmd(FsmContext* context, const HciReadLinkQualityCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_read_link_quality_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, (CsrUint8)cmd->handle);
}

static void ready__hci_read_rssi_cmd(FsmContext* context, const HciReadRssiCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_read_rssi_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, (CsrUint8)cmd->handle);
}

static void ready__hci_read_link_supervision_timeout_cmd(FsmContext* context, const HciReadLinkSupervisionTimeoutCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_read_link_supervision_timeout_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, (CsrUint8)cmd->handle);
}

static void ready__hci_write_link_supervision_timeout_cmd(FsmContext* context, const HciWriteLinkSupervisionTimeoutCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_read_link_supervision_timeout_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, (CsrUint8)cmd->handle);
}

static void ready__hci_short_range_mode_cmd(FsmContext* context, const HciShortRangeModeCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_short_range_mode_cmd():"));
    hci_forward_on_phy_link_handle(context, (FsmEvent *)cmd, cmd->physicalLinkHandle);
}

static void ready__hci_disconnect_logical_link_cmd(FsmContext* context, const HciDisconnectLogicalLinkCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_disconnect_logical_link_cmd():"));
    hci_forward_on_logical_link_handle(context, (FsmEvent *)cmd, cmd->logicalLinkHandle);
}

static void ready__hci_flow_spec_modify_cmd(FsmContext* context, const HciFlowSpecModifyCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_flow_spec_modify_cmd():"));
    hci_forward_on_logical_link_handle(context, (FsmEvent *)cmd, cmd->handle);
}

static void ready__hci_read_best_effort_flush_timeout_cmd(FsmContext* context, const HciReadBestEffortFlushTimeoutCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_read_best_effort_flush_timeout_cmd():"));
    hci_forward_on_logical_link_handle(context, (FsmEvent *)cmd, cmd->logicalLinkHandle);
}

static void ready__hci_write_best_effort_flush_timeout_cmd(FsmContext* context, const HciWriteBestEffortFlushTimeoutCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_write_best_effort_flush_timeout_cmd():"));
    hci_forward_on_logical_link_handle(context, (FsmEvent *)cmd, cmd->logicalLinkHandle);
}

static void ready__hci_enhanced_flush_cmd(FsmContext* context, const HciEnhancedFlushCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_enhanced_flush_cmd():"));
    hci_forward_on_logical_link_handle(context, (FsmEvent *)cmd, cmd->handle);
}

static void ready__hci_read_failed_contact_counter_cmd(FsmContext* context, const HciReadFailedContactCounterCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_read_failed_contact_counter_cmd():"));
    hci_forward_on_logical_link_handle(context, (FsmEvent *)cmd, cmd->handle);
}

static void ready__hci_reset_failed_contact_counter_cmd(FsmContext* context, const HciResetFailedContactCounterCmd *cmd)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"hci_reset_failed_contact_counter_cmd():"));
    hci_forward_on_logical_link_handle(context, (FsmEvent *)cmd, cmd->handle);
}

static void ready__hci_read_local_amp_assoc_cmd(FsmContext* context, const HciReadLocalAmpAssocCmd *cmd)
{
    sme_trace_info((TR_PAL_MGR_FSM,"hci_read_local_amp_assoc_cmd():handle-%d",cmd->physicalLinkHandle));
    /* If handle is zero then forward it directly */

    if (HCI_COMMAND_NOT_ALLOWED(cmd->common.id))
    {
        call_hci_command_status_evt(context, HCI_STATUS_CODE_COMMAND_DISALLOWED,
                                           getHciBottomContext(context)->numCommandsAllowed,
                                           cmd->common.id);
    }
    else
    {
        CsrUint16 lmFsmPid=getSmeContext(context)->palDmFsmInstance;

        if (cmd->physicalLinkHandle != 0)
        {
            lmFsmPid = pal_get_link_fsm_pid_from_handle(context, cmd->physicalLinkHandle);
        }

        if (FSM_TERMINATE==lmFsmPid)
        {
            /* send a command-complete only if commands are allowed. Otherwise a command status
             * will go as normal.
             */
            ReturnParameters returnParameters;

            returnParameters.commandCode = HCI_READ_LOCAL_AMP_ASSOC_CODE;
            returnParameters.hciReturnParam.readLocalAmpAssocReturn.status=HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
            returnParameters.hciReturnParam.readLocalAmpAssocReturn.physicalLinkHandle = cmd->physicalLinkHandle;
            returnParameters.hciReturnParam.readLocalAmpAssocReturn.remainingLength = 0;
            returnParameters.hciReturnParam.readLocalAmpAssocReturn.assocFragment.dataLen = 0;
            returnParameters.hciReturnParam.readLocalAmpAssocReturn.assocFragment.data = (CsrUint8 *)(&returnParameters+1); /*only to keep the copy happpy in packing */

            call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                                 &returnParameters);
        }
        else
        {
            /* The command goes to DM anyways. So forward it straight rather taking a route through LM Link FSM*/
            hci_forward_cmd(context,getSmeContext(context)->palDmFsmInstance, (FsmEvent *)cmd, HCI_STATUS_CODE_SUCCESS);
        }
    }
}

static void ready__hci_read_local_amp_info_cmd(FsmContext* context, const HciReadLocalAmpInfoCmd *cmd)
{
    hci_forward_cmd(context,getSmeContext(context)->palDmFsmInstance, (FsmEvent *)cmd, HCI_STATUS_CODE_SUCCESS);
}

static void ready__hci_read_loopback_mode_cmd(FsmContext* context, const HciReadLoopbackModeCmd *cmd)
{
    hci_unsupported_cmd(context,(FsmEvent *)cmd);
}

static void ready__hci_read_location_data_cmd(FsmContext* context, const HciReadLocationDataCmd *cmd)
{
    hci_unsupported_cmd(context,(FsmEvent *)cmd);
}

static void ready__hci_write_location_data_cmd(FsmContext* context, const HciWriteLocationDataCmd *cmd)
{
    hci_unsupported_cmd(context,(FsmEvent *)cmd);
}


static void ready__hci_write_loopback_mode_cmd(FsmContext* context, const HciWriteLoopbackModeCmd *cmd)
{
    hci_unsupported_cmd(context,(FsmEvent *)cmd);
}

static void ready__hci_set_event_mask_cmd(FsmContext* context, const HciSetEventMaskCmd *cmd)
{
    CsrBool qosViolationEvent= IS_BIT_SET(getHciBottomContext(context)->eventMask.m, EVENT_MASK_HCI_QOS_VIOLATION_EVENT_BIT_POS);
    ReturnParameters retParams;

    sme_trace_entry((TR_PAL_MGR_FSM,"ready__hci_set_event_mask_cmd(): "));

    retParams.commandCode = HCI_SET_EVENT_MASK_CODE;
    
    if (HCI_COMMAND_ALLOWED(cmd->common.id))
    {
        retParams.hciReturnParam.setEventMaskStatus = HCI_STATUS_CODE_SUCCESS;

        getHciBottomContext(context)->eventMask = cmd->eventMask;
    
        if (IS_BIT_SET(getHciBottomContext(context)->eventMask.m, EVENT_MASK_HCI_QOS_VIOLATION_EVENT_BIT_POS) 
            != 
            qosViolationEvent)
        {
            CsrUint32 indMask=0;

            sme_trace_entry((TR_PAL_MGR_FSM,"ready__hci_set_event_mask_cmd(): status changed for qos violation 0ld-%d, new-%d",
                qosViolationEvent, 
                IS_BIT_SET(getHciBottomContext(context)->eventMask.m, EVENT_MASK_HCI_QOS_VIOLATION_EVENT_BIT_POS)));

            indMask |= IS_BIT_SET(getHciBottomContext(context)->eventMask.m, EVENT_MASK_HCI_QOS_VIOLATION_EVENT_BIT_POS)?unifi_PalDataIndQosViolation:0;
            indMask |= IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_NUMBER_OF_COMPLETED_DATA_BLOCKS_EVENT_BIT_POS)?unifi_PalDataIndCompletedDataBlocks:0;
            call_pal_ctrl_event_mask_set_req(context, 
                                             getPalMgrContext(context)->appHandle,
                                             indMask);
        }
    }
    else
    {
        retParams.hciReturnParam.setEventMaskStatus = HCI_STATUS_CODE_COMMAND_DISALLOWED;
    }
    
    call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &retParams);

}

static void ready__hci_set_event_mask_page2_cmd(FsmContext* context, const HciSetEventMaskPage2Cmd *cmd)
{
    CsrBool numCompletedDataBlocksEvent= IS_BIT_SET(getHciBottomContext(context)->eventMask.m, EVENT_MASK_HCI_NUMBER_OF_COMPLETED_DATA_BLOCKS_EVENT_BIT_POS);
    ReturnParameters retParams;

    sme_trace_entry((TR_PAL_MGR_FSM,"ready__hci_set_event_mask_page2_cmd(): "));

    retParams.commandCode = HCI_SET_EVENT_MASK_PAGE2_CODE;
    
    if (HCI_COMMAND_ALLOWED(cmd->common.id))
    {
        retParams.hciReturnParam.setEventMaskPage2Status = HCI_STATUS_CODE_SUCCESS;

        getHciBottomContext(context)->eventMaskPage2 = cmd->eventMaskPage2;
    
        if (IS_BIT_SET(getHciBottomContext(context)->eventMask.m, EVENT_MASK_HCI_NUMBER_OF_COMPLETED_DATA_BLOCKS_EVENT_BIT_POS)
            != 
            numCompletedDataBlocksEvent)
        {
            CsrUint32 indMask=0;
            
            sme_trace_entry((TR_PAL_MGR_FSM,"ready__hci_set_event_mask_page2_cmd(): status changed for number of completed blocks 0ld-%d, new-%d",
                numCompletedDataBlocksEvent, IS_BIT_SET(getHciBottomContext(context)->eventMask.m, EVENT_MASK_HCI_NUMBER_OF_COMPLETED_DATA_BLOCKS_EVENT_BIT_POS)));
            
            indMask |= IS_BIT_SET(getHciBottomContext(context)->eventMask.m, EVENT_MASK_HCI_QOS_VIOLATION_EVENT_BIT_POS)?unifi_PalDataIndQosViolation:0;
            indMask |= IS_BIT_SET(getHciBottomContext(context)->eventMaskPage2.m, EVENT_MASK_HCI_NUMBER_OF_COMPLETED_DATA_BLOCKS_EVENT_BIT_POS)?unifi_PalDataIndCompletedDataBlocks:0;
            call_pal_ctrl_event_mask_set_req(context, 
                                             getPalMgrContext(context)->appHandle,
                                             indMask);
        }
    }
    else
    {
        retParams.hciReturnParam.setEventMaskPage2Status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
    }
    
    call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &retParams);

}

/* LM Proxy functions */
static void ready__pal_ctrl_link_create_cfm(FsmContext* context, const PalCtrlLinkCreateCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_link_create_cfm(): logical link handle=%d", cfm->logicalLinkHandle));
    pal_ctrl_forward_on_logical_link_handle(context, (FsmEvent *)cfm, cfm->logicalLinkHandle);
}

static void ready__pal_ctrl_link_delete_cfm(FsmContext* context, const PalCtrlLinkDeleteCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_link_delete_cfm(): logical link handle=%d", cfm->logicalLinkHandle));
    pal_ctrl_forward_on_physical_link_handle(context, (FsmEvent *)cfm, cfm->physicalLinkHandle);
}

static void ready__pal_ctrl_link_modify_cfm(FsmContext* context, const PalCtrlLinkModifyCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_link_modify_cfm(): logical link handle=%d", cfm->logicalLinkHandle));
    pal_ctrl_forward_on_logical_link_handle(context, (FsmEvent *)cfm, cfm->logicalLinkHandle);
}

static void ready__pal_ctrl_link_flush_cfm(FsmContext* context, const PalCtrlLinkFlushCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_link_flush_cfm(): logical link handle=%d", cfm->logicalLinkHandle));
    pal_ctrl_forward_on_logical_link_handle(context, (FsmEvent *)cfm, cfm->logicalLinkHandle);
}

static void ready__pal_ctrl_failed_contact_counter_read_cfm(FsmContext* context, const PalCtrlFailedContactCounterReadCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_failed_contact_counter_read_cfm(): logical link handle=%d", cfm->logicalLinkHandle));
    pal_ctrl_forward_on_logical_link_handle(context, (FsmEvent *)cfm, cfm->logicalLinkHandle);
}

static void ready__pal_ctrl_failed_contact_counter_reset_cfm(FsmContext* context, const PalCtrlFailedContactCounterResetCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_failed_contact_counter_reset_cfm(): logical link handle=%d", cfm->logicalLinkHandle));
    pal_ctrl_forward_on_logical_link_handle(context, (FsmEvent *)cfm, cfm->logicalLinkHandle);
}

static void ready__pal_ctrl_early_link_loss_ind(FsmContext* context, const PalCtrlEarlyLinkLossInd_Evt *ind)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_early_link_loss_ind():physical link handle=%d", ind->physicalLinkHandle));

    pal_ctrl_forward_on_physical_link_handle(context,(FsmEvent *)ind, ind->physicalLinkHandle);
}

static void ready__pal_ctrl_link_lost_ind(FsmContext* context, const PalCtrlLinkLostInd_Evt *ind)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_link_lost_ind():physical link handle=%d", ind->physicalLinkHandle));

    pal_ctrl_forward_on_physical_link_handle(context,(FsmEvent *)ind, ind->physicalLinkHandle);
}

static void ready__pal_ctrl_link_supervision_timeout_set_cfm(FsmContext* context, const PalCtrlLinkSupervisionTimeoutSetCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_link_supervision_timeout_set_cfm():physical link handle=%d", cfm->physicalLinkHandle));

    pal_ctrl_forward_on_physical_link_handle(context,(FsmEvent *)cfm, cfm->physicalLinkHandle);
}

static void ready__pal_ctrl_link_supervision_timeout_modify_cfm(FsmContext* context, const PalCtrlLinkSupervisionTimeoutModifyCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_link_supervision_timeout_modify_cfm():physical link handle=%d", cfm->physicalLinkHandle));

    pal_ctrl_forward_on_physical_link_handle(context,(FsmEvent *)cfm, cfm->physicalLinkHandle);
}

static void ready__pal_ctrl_link_supervision_timeout_delete_cfm(FsmContext* context, const PalCtrlLinkSupervisionTimeoutDeleteCfm_Evt *cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__pal_ctrl_link_supervision_timeout_delete_cfm():physical link handle=%d", cfm->physicalLinkHandle));

    pal_ctrl_forward_on_physical_link_handle(context,(FsmEvent *)cfm, cfm->physicalLinkHandle);
}

static void forward_on_remote_mac_address(FsmContext* context, const FsmEvent *evt, const unifi_MACAddress *peerStaAddress)
{
    CsrUint16 hipFsmPid;
    sme_trace_entry((TR_PAL_MGR_FSM,"forward_on_remote_mac_address(): 0x%2.2x-0x%2.2x-0x%2.2x-0x%2.2x-0x%2.2x-0x%2.2x",
        peerStaAddress->data[0],peerStaAddress->data[1],peerStaAddress->data[2],peerStaAddress->data[3],peerStaAddress->data[4],peerStaAddress->data[5]));

    hipFsmPid = pal_get_hip_fsm_pid_from_remote_mac_address(context, peerStaAddress);

    if (VALID_PID(hipFsmPid))
    {
        fsm_forward_event(context, hipFsmPid, (FsmEvent *)evt);
    }
    else
    {
        /*  payload if any will be freed with this call */
        fsm_ignore_event(context,evt);
    }
}

static void ready__mlme_associate_ind(FsmContext* context, const MlmeAssociateInd_Evt *ind)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__mlme_associate_ind():"));
    forward_on_remote_mac_address(context, (FsmEvent *)ind, &ind->peerStaAddress);
}

static void ready__mlme_authenticate_ind(FsmContext* context, const MlmeAuthenticateInd_Evt *ind)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__mlme_authenticate_ind():"));
    forward_on_remote_mac_address(context, (FsmEvent *)ind, &ind->peerStaAddress);
}

static void ready__mlme_deauthenticate_ind(FsmContext* context, const MlmeDeauthenticateInd_Evt *ind)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__mlme_deauthenticate_ind():"));
    forward_on_remote_mac_address(context, (FsmEvent *)ind, &ind->peerStaAddress);
}

static void ready__mlme_connected_ind(FsmContext* context, const MlmeConnectedInd_Evt *ind)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__mlme_connected_ind():REMOVE ME. Not used"));
}

static void ready__unifi_sys_ma_unitdata_ind(FsmContext* context, const UnifiSysMaUnitdataInd_Evt *ind)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__unifi_sys_ma_unitdata_ind():"));
    forward_on_remote_mac_address(context, (FsmEvent *)ind, (unifi_MACAddress *)(ind->frame+6));
    call_unifi_sys_ma_unitdata_rsp(context, ind->subscriptionHandle, unifi_Success);
}

static void stopped__core_start_req(FsmContext* context, const CoreStartReq_Evt* req)
{
    /* Start with DM and COEX*/
    send_core_start_req(context, getSmeContext(context)->palDmFsmInstance, req->resume);

    fsm_next_state(context, FSMSTATE_starting);
}

static void ready__hci_read_data_block_size_cmd(FsmContext *context, const HciReadDataBlockSizeCmd *cmd)
{
    ReturnParameters retParams;
    retParams.commandCode = HCI_READ_DATA_BLOCK_SIZE_CODE;

    if (HCI_COMMAND_ALLOWED(cmd->common.id))
    {
        retParams.hciReturnParam.readDataBlockSizeReturn.status = HCI_STATUS_CODE_SUCCESS;
        retParams.hciReturnParam.readDataBlockSizeReturn.dataBlockLength = PAL_MAX_PDU_SIZE; /* same as ACL data length */
        retParams.hciReturnParam.readDataBlockSizeReturn.maxAclDataPacketLength = PAL_MAX_PDU_SIZE;
        retParams.hciReturnParam.readDataBlockSizeReturn.totalNumberOfBlocks = getPalMgrContext(context)->phyLinkCommonAttrib.numTotalDataBlocks;
    }
    else
    {
        retParams.hciReturnParam.readDataBlockSizeReturn.status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
    }

    call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                         &retParams);
}

static void starting__core_start_cfm(FsmContext* context, const CoreStartCfm_Evt* cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"starting__core_start_cfm() : ProcessId - %d",cfm->common.sender_));

    /* Wait until Coex manager is started before initialisting data manager */
    if (cfm->common.sender_ == getSmeContext(context)->palCoexFsmInstance)
    {
        /* Bring up the data manager */
        call_pal_ctrl_activate_req(context,getPalMgrContext(context)->appHandle);

        fsm_next_state(context, FSMSTATE_starting);
    }
    else
    {
        verify(TR_PAL_MGR_FSM,cfm->common.sender_ == getSmeContext(context)->palDmFsmInstance);
        sme_trace_entry((TR_PAL_MGR_FSM,"starting__core_start_cfm() : DM is just initialised. Now intialise COEX"));
        send_core_start_req(context, getSmeContext(context)->palCoexFsmInstance, FALSE);
    }
}

static void starting__pal_ctrl_activate_cfm(FsmContext* context, const PalCtrlActivateCfm_Evt* cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"starting__pal_ctrl_activate_cfm() : numDataBlocks - %d",cfm->numDataBlocks));

    verify(TR_PAL_MGR_FSM, cfm->numDataBlocks>0);
    getPalMgrContext(context)->phyLinkCommonAttrib.numTotalDataBlocks = cfm->numDataBlocks;

    pal_hci_started(context);
    send_core_start_cfm(context, getSmeContext(context)->smeCoreInstance, unifi_Success);

    fsm_next_state(context, FSMSTATE_ready);
}

static void ready__core_stop_req(FsmContext* context, const CoreStopReq_Evt* req)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"ready__core_stop_req() : suspend - %d",req->suspend));

    if (!hip_auto_cfm_is_hip_sap_open(getSmeContext(context)->hipSapAuto))
    {
        /* if its a shutdown because of firmware crash, then clean data manager first so that it cleans up itself
        * and unlocks any link manager instances waiting for a reponse. 
        */
        sme_trace_info((TR_PAL_MGR_FSM,"ready__core_stop_req() : shutdown due to firmware crash. So clean data manager first"));
        call_pal_ctrl_deactivate_req(context, getPalMgrContext(context)->appHandle);
    }
    else
    {
        CsrUint16 lmFsmPid = get_next_lm_instance(context);
        /* stop lm instances one by one */
        if (VALID_PID(lmFsmPid))
        {
            sme_trace_info((TR_PAL_MGR_FSM,"ready__core_stop_req(): stop lm instance - $d",lmFsmPid));
            send_core_stop_req(context, lmFsmPid, FALSE);
        }
        else
        {
            sme_trace_info((TR_PAL_MGR_FSM,"ready__core_stop_req(): No LM instances to stop. So stop DM"));
            send_core_stop_req(context, getSmeContext(context)->palDmFsmInstance, FALSE);
        }
    }
    fsm_next_state(context, FSMSTATE_stopping);
}

static void stopping__core_stop_cfm(FsmContext* context, const CoreStopCfm_Evt* cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"stopping__core_stop_cfm() "));
    if (cfm->common.sender_ == getSmeContext(context)->palCoexFsmInstance)
    {
        sme_trace_info((TR_PAL_MGR_FSM,"stopping__core_stop_cfm(): Coex instace stopped. Stop Data Manager"));
        if (!hip_auto_cfm_is_hip_sap_open(getSmeContext(context)->hipSapAuto))
        {
            pal_hci_stopped(context);
            send_core_stop_cfm(context, getSmeContext(context)->smeCoreInstance, unifi_Success);
            fsm_next_state(context, FSMSTATE_stopped);
        }
        else
        {
            call_pal_ctrl_deactivate_req(context,getPalMgrContext(context)->appHandle);
        }
    }
    else if (cfm->common.sender_ == getSmeContext(context)->palDmFsmInstance)
    {
        sme_trace_info((TR_PAL_MGR_FSM,"stopping__core_stop_cfm(): DM instace stopped. Stop Data Manager"));
        send_core_stop_req(context, getSmeContext(context)->palCoexFsmInstance, FALSE);
    }
    else
    {
        CsrUint16 lmFsmPid;

        sme_trace_info((TR_PAL_MGR_FSM,"stopping__core_stop_cfm(): LM instace stopped. Stop LM instances if any left"));
        /* Delete the LM instance that stopped*/
        pal_delete_lm_instance(context, cfm->common.sender_);
        lmFsmPid = get_next_lm_instance(context);
        /* stop lm instances one by one */
        if (VALID_PID(lmFsmPid))
        {
            sme_trace_info((TR_PAL_MGR_FSM,"stopping__core_stop_cfm(): stop lm instance - $d",lmFsmPid));
            send_core_stop_req(context, lmFsmPid, FALSE);
        }
        else
        {
            sme_trace_info((TR_PAL_MGR_FSM,"stopping__core_stop_cfm(): All LM instaces stopped. stop DM"));
            send_core_stop_req(context, getSmeContext(context)->palDmFsmInstance, FALSE);
        }
    }
}

static void stopping__pal_ctrl_deactivate_cfm(FsmContext* context, const PalCtrlDeactivateCfm_Evt* cfm)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"stopping__pal_ctrl_deactivate_cfm() "));

    if (!hip_auto_cfm_is_hip_sap_open(getSmeContext(context)->hipSapAuto))
    {
        CsrUint16 lmFsmPid = get_next_lm_instance(context);

        /* stop lm instances one by one */
        if (VALID_PID(lmFsmPid))
        {
            sme_trace_info((TR_PAL_MGR_FSM,"ready__core_stop_req(): stop lm instance - $d",lmFsmPid));
            send_core_stop_req(context, lmFsmPid, FALSE);
        }
        else
        {
            sme_trace_info((TR_PAL_MGR_FSM,"ready__core_stop_req(): No LM instances to stop. So stop DM"));
            send_core_stop_req(context, getSmeContext(context)->palDmFsmInstance, FALSE);
        }
    }
    else
    {
        pal_hci_stopped(context);
        send_core_stop_cfm(context, getSmeContext(context)->smeCoreInstance, unifi_Success);
        fsm_next_state(context, FSMSTATE_stopped);
    }
}

/* functions called by PAL Core to send events */
static void resetting__hci_command_complete_evt(FsmContext *context, const HciCommandCompleteEvt *evt)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "resetting__hci_command_complete_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));

    if (HCI_RESET_CODE == evt->returnParameters.commandCode)
    {
        sme_trace_entry((TR_PAL_MGR_FSM,"resetting__hci_command_complete_evt(): command complete for hci-reset "));
        if (evt->common.sender_ == getSmeContext(context)->palDmFsmInstance)
        {
            sme_trace_info((TR_PAL_MGR_FSM,"resetting__hci_command_complete_evt(): hci-reset completed"));
            pal_call_hci_reset_complete(context);
            fsm_next_state(context, FSMSTATE_ready);
        }
        else
        {
            CsrUint16 lmFsmPid;

            sme_trace_info((TR_PAL_MGR_FSM,"resetting__hci_command_complete_evt(): LM instace reset complete reset LM instances if any left"));
            /* Delete the LM instance that stopped*/
            pal_delete_lm_instance(context, evt->common.sender_);
            lmFsmPid = get_next_lm_instance(context);
            /* stop lm instances one by one */
            if (VALID_PID(lmFsmPid))
            {
                sme_trace_info((TR_PAL_MGR_FSM,"resetting__hci_command_complete_evt(): stop lm instance - $d",lmFsmPid));
                send_hci_reset_cmd(context, lmFsmPid);
            }
            else
            {
                sme_trace_info((TR_PAL_MGR_FSM,"resetting__hci_command_complete_evt(): reset complete for all LM instances. reset DM"));
                send_hci_reset_cmd(context, getSmeContext(context)->palDmFsmInstance);
            }
        }
    }
    else
    {
        sme_trace_info((TR_PAL_MGR_FSM, "resetting__hci_command_complete_evt(): command compele for 0x%x opcode",evt->returnParameters.commandCode));
        call_hci_command_complete_evt(context, getHciBottomContext(context)->numCommandsAllowed,
                                             &evt->returnParameters);

        if (HCI_READ_LOCAL_AMP_ASSOC_CODE == evt->returnParameters.commandCode &&
            evt->returnParameters.hciReturnParam.readLocalAmpAssocReturn.assocFragment.dataLen > 0)
        {
            sme_trace_info((TR_PAL_MGR_FSM, "resetting__hci_command_complete_evt(): freeing the assoc fragment"));
            CsrPfree(evt->returnParameters.hciReturnParam.readLocalAmpAssocReturn.assocFragment.data);
        }
    }
}

static void resetting__hci_command_status_evt(FsmContext *context, const HciCommandStatusEvt *evt)
{
    /* currently used only to let host know that PAL is available  */
    sme_trace_entry((TR_PAL_MGR_FSM, "resetting__hci_command_status_evt(): status-%d,num allowed command-%d",evt->status, getHciBottomContext(context)->numCommandsAllowed));

    if (HCI_CREATE_PHYSICAL_LINK_CODE == evt->commandOpcode && evt->status != HCI_STATUS_CODE_SUCCESS)
    {
        sme_trace_info((TR_PAL_MGR_FSM, "ready__hci_command_status_evt(): delete lm instance as the link creation failed"));
        pal_delete_lm_instance(context, evt->common.sender_);
    }

    call_hci_command_status_evt(context, evt->status,
                                       getHciBottomContext(context)->numCommandsAllowed,evt->common.id);
}

static void fsm_reject_hci_cmd(FsmContext* context, const FsmEvent *cmd)
{
    call_hci_command_status_evt(context, HCI_STATUS_CODE_COMMAND_DISALLOWED,
                                       getHciBottomContext(context)->numCommandsAllowed,
                                       cmd->id);
}

static const FsmEventEntry stoppedTransitions[] =
{
    fsm_event_table_entry(CORE_START_REQ_ID,                         stopped__core_start_req),
    /* Handle these commands even if Wifi is off. Especially for AMPM during system startup*/
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_INFO_CODE,              ready__hci_read_local_amp_info_cmd),
    fsm_event_table_entry(HCI_COMMAND_COMPLETE_CODE,                 stopped__hci_command_complete_evt),
};

static const FsmEventEntry startingTransitions[] =
{
    fsm_event_table_entry(CORE_START_CFM_ID,                         starting__core_start_cfm),
    fsm_event_table_entry(PAL_CTRL_ACTIVATE_CFM_ID,                  starting__pal_ctrl_activate_cfm),

    /* Handle command complete. Just in case the wifi on is triggered before completing the command processing in stopped state. */
    fsm_event_table_entry(HCI_COMMAND_COMPLETE_CODE,                 stopped__hci_command_complete_evt),

    /* Save these commands when the wifi is starting */
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_INFO_CODE,              fsm_saved_event),
};

static const FsmEventEntry readyTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT_CODE,  ready__hci_write_connection_accept_timeout_cmd),
    fsm_event_table_entry(HCI_READ_CONNECTION_ACCEPT_TIMEOUT_CODE,   ready__hci_read_connection_accept_timeout_cmd),
    fsm_event_table_entry(HCI_WRITE_LOGICAL_LINK_ACCEPT_TIMEOUT_CODE,ready__hci_write_logical_link_accept_timeout_cmd),
    fsm_event_table_entry(HCI_READ_LOGICAL_LINK_ACCEPT_TIMEOUT_CODE, ready__hci_read_logical_link_accept_timeout_cmd),
    fsm_event_table_entry(HCI_READ_LOCAL_VERSION_INFORMATION_CODE,   ready__hci_read_local_version_information_cmd),
    fsm_event_table_entry(HCI_READ_LOCAL_SUPPORTED_COMMANDS_CODE,    ready__hci_read_local_supported_commands_cmd),

    fsm_event_table_entry(HCI_CREATE_PHYSICAL_LINK_CODE,    ready__hci_create_physical_link_cmd),
    fsm_event_table_entry(HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CODE,    ready__hci_accept_physical_link_request_cmd),

    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_CODE,    ready__hci_disconnect_physical_link_cmd),
    fsm_event_table_entry(HCI_CREATE_LOGICAL_LINK_CODE,    ready__hci_create_logical_link_cmd),
    fsm_event_table_entry(HCI_ACCEPT_LOGICAL_LINK_CODE,    ready__hci_accept_logical_link_cmd),
    fsm_event_table_entry(HCI_LOGICAL_LINK_CANCEL_CODE,    ready__hci_logical_link_cancel_cmd),
    fsm_event_table_entry(HCI_WRITE_REMOTE_AMP_ASSOC_CODE,    ready__hci_write_remote_amp_assoc_cmd),
    fsm_event_table_entry(HCI_READ_LINK_QUALITY_CODE,    ready__hci_read_link_quality_cmd),
    fsm_event_table_entry(HCI_READ_RSSI_CODE,    ready__hci_read_rssi_cmd),
    fsm_event_table_entry(HCI_SHORT_RANGE_MODE_CODE,    ready__hci_short_range_mode_cmd),

    fsm_event_table_entry(HCI_DISCONNECT_LOGICAL_LINK_CODE,    ready__hci_disconnect_logical_link_cmd),
    fsm_event_table_entry(HCI_FLOW_SPEC_MODIFY_CODE,    ready__hci_flow_spec_modify_cmd),
    fsm_event_table_entry(HCI_READ_BEST_EFFORT_FLUSH_TIMEOUT_CODE,    ready__hci_read_best_effort_flush_timeout_cmd),
    fsm_event_table_entry(HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CODE,    ready__hci_write_best_effort_flush_timeout_cmd),
    fsm_event_table_entry(HCI_ENHANCED_FLUSH_CODE,    ready__hci_enhanced_flush_cmd),
    fsm_event_table_entry(HCI_READ_FAILED_CONTACT_COUNTER_CODE,    ready__hci_read_failed_contact_counter_cmd),
    fsm_event_table_entry(HCI_RESET_FAILED_CONTACT_COUNTER_CODE,    ready__hci_reset_failed_contact_counter_cmd),

    fsm_event_table_entry(HCI_READ_LOCAL_AMP_ASSOC_CODE,    ready__hci_read_local_amp_assoc_cmd),

    fsm_event_table_entry(HCI_RESET_CODE,    ready__hci_reset_cmd),
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_INFO_CODE,    ready__hci_read_local_amp_info_cmd),
    fsm_event_table_entry(HCI_READ_LINK_SUPERVISION_TIMEOUT_CODE,    ready__hci_read_link_supervision_timeout_cmd),
    fsm_event_table_entry(HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CODE,    ready__hci_write_link_supervision_timeout_cmd),
    fsm_event_table_entry(HCI_READ_DATA_BLOCK_SIZE_CODE,    ready__hci_read_data_block_size_cmd),

    fsm_event_table_entry(HCI_READ_LOOPBACK_MODE_CODE,    ready__hci_read_loopback_mode_cmd),
    fsm_event_table_entry(HCI_WRITE_LOOPBACK_MODE_CODE,    ready__hci_write_loopback_mode_cmd),
    fsm_event_table_entry(HCI_SET_EVENT_MASK_CODE,    ready__hci_set_event_mask_cmd),
    fsm_event_table_entry(HCI_SET_EVENT_MASK_PAGE2_CODE,    ready__hci_set_event_mask_page2_cmd),
    fsm_event_table_entry(HCI_READ_DATA_BLOCK_SIZE_CODE,           ready__hci_read_data_block_size_cmd),

    fsm_event_table_entry(HCI_READ_LOCATION_DATA_CODE,    ready__hci_read_location_data_cmd),
    fsm_event_table_entry(HCI_WRITE_LOCATION_DATA_CODE,    ready__hci_write_location_data_cmd),


    /* events from LM and DM */
    fsm_event_table_entry(HCI_COMMAND_STATUS_CODE,    ready__hci_command_status_evt),
    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_COMPLETE_CODE,    ready__hci_disconnect_physical_link_complete_evt),
    fsm_event_table_entry(HCI_COMMAND_COMPLETE_CODE,    ready__hci_command_complete_evt),
    fsm_event_table_entry(HCI_CHANNEL_SELECT_CODE,    ready__hci_channel_select_evt),
    fsm_event_table_entry(HCI_PHYSICAL_LINK_COMPLETE_CODE,    ready__hci_physical_link_complete_evt),
    fsm_event_table_entry(HCI_DISCONNECT_LOGICAL_LINK_COMPLETE_CODE,    ready__hci_disconnect_logical_link_complete_evt),
    fsm_event_table_entry(HCI_LOGICAL_LINK_COMPLETE_CODE,    ready__hci_logical_link_complete_evt),
    fsm_event_table_entry(HCI_FLOW_SPEC_MODIFY_COMPLETE_CODE,    ready__hci_flow_spec_modify_complete_evt),
    fsm_event_table_entry(HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE_CODE,    ready__hci_short_range_mode_change_complete_evt),
    fsm_event_table_entry(HCI_ENHANCED_FLUSH_COMPLETE_CODE,    ready__hci_enhanced_flush_complete_evt),


    /* Proxy for PAL CTRL Messages. */
    fsm_event_table_entry(PAL_CTRL_LINK_CREATE_CFM_ID,    ready__pal_ctrl_link_create_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_DELETE_CFM_ID,    ready__pal_ctrl_link_delete_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_MODIFY_CFM_ID,    ready__pal_ctrl_link_modify_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_FLUSH_CFM_ID,    ready__pal_ctrl_link_flush_cfm),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_READ_CFM_ID,    ready__pal_ctrl_failed_contact_counter_read_cfm),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_RESET_CFM_ID,    ready__pal_ctrl_failed_contact_counter_reset_cfm),
    fsm_event_table_entry(PAL_CTRL_EARLY_LINK_LOSS_IND_ID,    ready__pal_ctrl_early_link_loss_ind),
    fsm_event_table_entry(PAL_CTRL_LINK_LOST_IND_ID,    ready__pal_ctrl_link_lost_ind),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_SET_CFM_ID,    ready__pal_ctrl_link_supervision_timeout_set_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_MODIFY_CFM_ID,    ready__pal_ctrl_link_supervision_timeout_modify_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_CFM_ID,    ready__pal_ctrl_link_supervision_timeout_delete_cfm),

    /* Proxy for HIP messages */
    fsm_event_table_entry(MLME_ASSOCIATE_IND_ID,    ready__mlme_associate_ind),
    fsm_event_table_entry(MLME_AUTHENTICATE_IND_ID,    ready__mlme_authenticate_ind),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,    ready__mlme_deauthenticate_ind),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,    ready__mlme_connected_ind),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_IND_ID,    ready__unifi_sys_ma_unitdata_ind),

    fsm_event_table_entry(CORE_STOP_REQ_ID,                         ready__core_stop_req),
};

static const FsmEventEntry resettingTransitions[] =
{
    fsm_event_table_entry(CORE_STOP_REQ_ID,                          fsm_saved_event),
    fsm_event_table_entry(HCI_COMMAND_COMPLETE_CODE,                         resetting__hci_command_complete_evt),

    /* handle the events just in case there were in the middle of being processed */
    fsm_event_table_entry(HCI_COMMAND_STATUS_CODE,    resetting__hci_command_status_evt),

    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_COMPLETE_CODE,    ready__hci_disconnect_physical_link_complete_evt),
    fsm_event_table_entry(HCI_CHANNEL_SELECT_CODE,    ready__hci_channel_select_evt),
    fsm_event_table_entry(HCI_PHYSICAL_LINK_COMPLETE_CODE,    ready__hci_physical_link_complete_evt),
    fsm_event_table_entry(HCI_DISCONNECT_LOGICAL_LINK_COMPLETE_CODE,    ready__hci_disconnect_logical_link_complete_evt),
    fsm_event_table_entry(HCI_LOGICAL_LINK_COMPLETE_CODE,    ready__hci_logical_link_complete_evt),
    fsm_event_table_entry(HCI_FLOW_SPEC_MODIFY_COMPLETE_CODE,    ready__hci_flow_spec_modify_complete_evt),
    fsm_event_table_entry(HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE_CODE,    ready__hci_short_range_mode_change_complete_evt),
    fsm_event_table_entry(HCI_ENHANCED_FLUSH_COMPLETE_CODE, ready__hci_enhanced_flush_complete_evt),


    /* Forward these ctrl events in case link manager is blocked */
    fsm_event_table_entry(PAL_CTRL_LINK_CREATE_CFM_ID,    ready__pal_ctrl_link_create_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_DELETE_CFM_ID,    ready__pal_ctrl_link_delete_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_MODIFY_CFM_ID,    ready__pal_ctrl_link_modify_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_FLUSH_CFM_ID,    ready__pal_ctrl_link_flush_cfm),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_READ_CFM_ID,    ready__pal_ctrl_failed_contact_counter_read_cfm),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_RESET_CFM_ID,    ready__pal_ctrl_failed_contact_counter_reset_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_SET_CFM_ID,    ready__pal_ctrl_link_supervision_timeout_set_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_MODIFY_CFM_ID,    ready__pal_ctrl_link_supervision_timeout_modify_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_CFM_ID,    ready__pal_ctrl_link_supervision_timeout_delete_cfm),
};

static const FsmEventEntry stoppingTransitions[] =
{
    /* Save these commands when the wifi is stopping */
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_INFO_CODE,    fsm_saved_event),

    fsm_event_table_entry(CORE_STOP_CFM_ID,                         stopping__core_stop_cfm),
    fsm_event_table_entry(PAL_CTRL_DEACTIVATE_CFM_ID,               stopping__pal_ctrl_deactivate_cfm),

    /* handle the events just in case there were in the middle of being processed */
    fsm_event_table_entry(HCI_COMMAND_COMPLETE_CODE,                resetting__hci_command_complete_evt),
    fsm_event_table_entry(HCI_COMMAND_STATUS_CODE,                  resetting__hci_command_status_evt),

    fsm_event_table_entry(HCI_COMMAND_STATUS_CODE,    ready__hci_command_status_evt),
    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_COMPLETE_CODE,    ready__hci_disconnect_physical_link_complete_evt),
    fsm_event_table_entry(HCI_COMMAND_COMPLETE_CODE,    ready__hci_command_complete_evt),
    fsm_event_table_entry(HCI_CHANNEL_SELECT_CODE,    ready__hci_channel_select_evt),
    fsm_event_table_entry(HCI_PHYSICAL_LINK_COMPLETE_CODE,    ready__hci_physical_link_complete_evt),
    fsm_event_table_entry(HCI_DISCONNECT_LOGICAL_LINK_COMPLETE_CODE,    ready__hci_disconnect_logical_link_complete_evt),
    fsm_event_table_entry(HCI_LOGICAL_LINK_COMPLETE_CODE,    ready__hci_logical_link_complete_evt),
    fsm_event_table_entry(HCI_FLOW_SPEC_MODIFY_COMPLETE_CODE,    ready__hci_flow_spec_modify_complete_evt),
    fsm_event_table_entry(HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE_CODE,    ready__hci_short_range_mode_change_complete_evt),
    fsm_event_table_entry(HCI_ENHANCED_FLUSH_COMPLETE_CODE, ready__hci_enhanced_flush_complete_evt),


    /* Forward these ctrl events in case link manager is blocked */
    fsm_event_table_entry(PAL_CTRL_LINK_CREATE_CFM_ID,    ready__pal_ctrl_link_create_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_DELETE_CFM_ID,    ready__pal_ctrl_link_delete_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_MODIFY_CFM_ID,    ready__pal_ctrl_link_modify_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_FLUSH_CFM_ID,    ready__pal_ctrl_link_flush_cfm),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_READ_CFM_ID,    ready__pal_ctrl_failed_contact_counter_read_cfm),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_RESET_CFM_ID,    ready__pal_ctrl_failed_contact_counter_reset_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_SET_CFM_ID,    ready__pal_ctrl_link_supervision_timeout_set_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_MODIFY_CFM_ID,    ready__pal_ctrl_link_supervision_timeout_modify_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_CFM_ID,    ready__pal_ctrl_link_supervision_timeout_delete_cfm),

};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT_CODE,  fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_CONNECTION_ACCEPT_TIMEOUT_CODE,   fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_WRITE_LOGICAL_LINK_ACCEPT_TIMEOUT_CODE,fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_LOGICAL_LINK_ACCEPT_TIMEOUT_CODE, fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_LOCAL_VERSION_INFORMATION_CODE,   fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_LOCAL_SUPPORTED_COMMANDS_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_CREATE_PHYSICAL_LINK_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_CREATE_LOGICAL_LINK_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_ACCEPT_LOGICAL_LINK_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_LOGICAL_LINK_CANCEL_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_WRITE_REMOTE_AMP_ASSOC_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_LINK_QUALITY_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_RSSI_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_SHORT_RANGE_MODE_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_DISCONNECT_LOGICAL_LINK_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_FLOW_SPEC_MODIFY_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_BEST_EFFORT_FLUSH_TIMEOUT_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_FLUSH_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_FAILED_CONTACT_COUNTER_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_RESET_FAILED_CONTACT_COUNTER_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_ASSOC_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_RESET_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_INFO_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_LINK_SUPERVISION_TIMEOUT_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_DATA_BLOCK_SIZE_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_READ_LOOPBACK_MODE_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_WRITE_LOOPBACK_MODE_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_SET_EVENT_MASK_CODE,    fsm_reject_hci_cmd),
    fsm_event_table_entry(HCI_SET_EVENT_MASK_PAGE2_CODE,    fsm_reject_hci_cmd),

    /* Proxy for PAL CTRL Messages. */
    fsm_event_table_entry(PAL_CTRL_EARLY_LINK_LOSS_IND_ID,    fsm_ignore_event),
    fsm_event_table_entry(PAL_CTRL_LINK_LOST_IND_ID,    fsm_ignore_event),
    fsm_event_table_entry(PAL_CTRL_EVENT_MASK_SET_CFM_ID,    fsm_ignore_event),


    /* Proxy for HIP messages */
    fsm_event_table_entry(MLME_ASSOCIATE_IND_ID,    fsm_ignore_event),
    fsm_event_table_entry(MLME_AUTHENTICATE_IND_ID,    fsm_ignore_event),
    fsm_event_table_entry(MLME_DEAUTHENTICATE_IND_ID,    fsm_ignore_event),
    fsm_event_table_entry(MLME_CONNECTED_IND_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_IND_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_CFM_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_EAPOL_CFM_ID,    fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_CFM_ID,    fsm_ignore_event),
};

/** Profile Storage state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                       State                   State                               Save     */
   /*                       Name                    Transitions                          *       */
   fsm_state_table_entry(FSMSTATE_stopped, stoppedTransitions,   FALSE),
   fsm_state_table_entry(FSMSTATE_starting, startingTransitions,   FALSE),
   fsm_state_table_entry(FSMSTATE_ready, readyTransitions,   FALSE),
   fsm_state_table_entry(FSMSTATE_resetting, resettingTransitions,   FALSE),
   fsm_state_table_entry(FSMSTATE_stopping, stoppingTransitions,   FALSE),
};

const FsmProcessStateMachine pal_mgr_fsm =
{
#ifdef FSM_DEBUG
       "PAL MGR",                                  /* Process Name       */
#endif
       PAL_HCI_PROCESS,                                /* Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                         /* Transition Tables  */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE),   /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                    /* ignore event handers */
       pal_mgr_init,                                    /* Entry Function     */
       lm_mgr_reset,                                                               /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       lm_mgr_dump                                                             /* Trace Dump Function   */
#endif
};

/*
 * FSM Scripts Config for this FSM
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/* PUBLIC FUNCTION DEFINITIONS *********************************************/

void* pal_hci_bottom_init(CsrUint16 maxHciCommandsAllowed)
{
    hciBottomContext *hciBottom = CsrPmalloc(sizeof(hciBottomContext));

    sme_trace_entry((TR_PAL_MGR_FSM, "pal_hci_bottom_configure(): max allowed command-%d",maxHciCommandsAllowed));
    hciBottom->maxCommandsAllowed = (CsrUint8)maxHciCommandsAllowed;
    hciBottom->numCommandsAllowed = hciBottom->maxCommandsAllowed;

    CsrMemSet(hciBottom->eventMaskPage2.m, 0x00, sizeof(hciBottom->eventMaskPage2.m));
    CsrMemSet(hciBottom->eventMask.m, 0x00, sizeof(hciBottom->eventMask.m));

    /* BT HCI Spec section 5.2 says about endianess of the octet stream:
    * "Unless noted otherwise, all parameter values are sent and received in Little
    * Endian format (i.e. for multi-octet parameters the rightmost (Least Signification
    * Octet) is transmitted first)"
    */
    /* by default enable all events for page 2. There is no default specified in spec*/
    hciBottom->eventMaskPage2.m[0] = 0xFF;
    hciBottom->eventMaskPage2.m[1] = 0x3F;

    /* set default as per the spec in section 7.3.1 */
    hciBottom->eventMask.m[0] = 0xFF;
    hciBottom->eventMask.m[1] = 0xFF;
    hciBottom->eventMask.m[2] = 0xFF;
    hciBottom->eventMask.m[3] = 0xFF;
    hciBottom->eventMask.m[4] = 0xFF;
    hciBottom->eventMask.m[5] = 0x1F;
    hciBottom->eventMask.m[6] = 0x00;
    hciBottom->eventMask.m[7] = 0x01;

    CsrMemSet(hciBottom->supportedCommands.m,
               0,
               sizeof(hciBottom->supportedCommands.m));
    /* Update the supported commands. Make sure this bit flag is updated if you add support for more commands. 
    * Refer BT3.0+HS spec Vol 4 part E 6.26 for the bitmap 
    */
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_SET_EVENT_MASK_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_RESET_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_CONNECTION_ACCEPT_TIMEOUT_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_LINK_SUPERVISION_TIMEOUT_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_LOCAL_VERSION_INFORMATION_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_FAILED_CONTACT_COUNTER_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_RESET_FAILED_CONTACT_COUNTER_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_LINK_QUALITY_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_RSSI_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_ENHANCED_FLUSH_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_CREATE_PHYSICAL_LINK_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_DISCONNECT_PHYSICAL_LINK_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_CREATE_LOGICAL_LINK_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_ACCEPT_LOGICAL_LINK_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_DISCONNECT_LOGICAL_LINK_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_LOGICAL_LINK_CANCEL_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_FLOW_SPEC_MODIFY_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_LOGICAL_LINK_ACCEPT_TIMEOUT_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_WRITE_LOGICAL_LINK_ACCEPT_TIMEOUT_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_SET_EVENT_MASK_PAGE2_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_LOCAL_AMP_INFO_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_LOCAL_AMP_ASSOC_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_WRITE_REMOTE_AMP_ASSOC_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_DATA_BLOCK_SIZE_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_READ_BEST_EFFORT_FLUSH_TIMEOUT_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CMD_BIT_POS);
    SET_BIT(hciBottom->supportedCommands.m, SUPPORTED_COMMANDS_HCI_SHORT_RANGE_MODE_CMD_BIT_POS);

    sme_trace_debug_code(
        {
            CsrUint32 i;
            sme_trace_info((TR_PAL_LM_LINK_FSM, "pal_hci_bottom_init: Supported Command bitfield. Refer BT3.0+HS spec Vol 4 part E 6.26 for the bitmap "));
            for (i=0;i<sizeof(hciBottom->supportedCommands.m); i++)
            {
                sme_trace_info((TR_PAL_LM_LINK_FSM, "Pos-%d , Value- 0x%2.2x",i,hciBottom->supportedCommands.m[i]));
            }
        }
    )
    return (void *)hciBottom;
}

void pal_hci_bottom_deinit(FsmContext *context)
{
    CsrPfree(getHciBottomContext(context));
}

void pal_hci_send_hci_amp_status_change_evt(FsmContext *context, HciStatusCode status, AmpStatus ampStatus)
{
    sme_trace_entry((TR_PAL_MGR_FSM, "pal_hci_send_hci_amp_status_change_evt(): num allowed command-%d",getHciBottomContext(context)->numCommandsAllowed));
    if (hci_event_enabled(context, HCI_AMP_STATUS_CHANGE_CODE))
    {
        call_hci_amp_status_change_evt(context,status,ampStatus);
    }
}

/** @}
 */
