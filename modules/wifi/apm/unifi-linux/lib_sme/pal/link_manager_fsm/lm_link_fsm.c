/** @file lm_link_fsm.c
 *
 * PAL Link Manager Link FSM
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
 *   This FSM is tasked handling the link maintainance (create , disconnect physical
 *    and logical links ).
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/lm_link_fsm.c#3 $
 *
 ****************************************************************************/

/** @{
 * @ingroup link_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "link_manager_fsm/lm_link_fsm.h"
#include "pal_hci_sap/pal_hci_sap_signals.h"
#include "pal_manager/pal_manager.h"
#include "link_manager_fsm/amp_assoc_endec.h"
#include "pal_ctrl_sap/pal_ctrl_sap_from_sme_interface.h"


/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, physicalLinkAttrib))
#define FSMDATA_PHYLINK FSMDATA

/* macros for send messages */
#define SEND_PAL_LINK_START_REQ(hipPid,handle,linkKey) send_pal_link_start_req(context,hipPid, handle,linkKey)
#define SEND_PAL_LINK_STOP_REQ(hipPid) send_pal_link_stop_req(context,hipPid)
#define SEND_PAL_LINK_CONNECT_REQ(hipPid,role) send_pal_link_connect_req(context,hipPid, role)
#define SEND_PAL_LINK_DISCONNECT_REQ(hipPid) send_pal_link_disconnect_req(context,hipPid)
#define SEND_PAL_LINK_ACTIVITY_REPORT_REQ(hipPid,ar) send_pal_link_activity_report_req(context, hipPid, ar)
#define SEND_PAL_LINK_AUTHENTICATE_REQ(hipPid) send_pal_link_authenticate_req(context, hipPid)
#define SEND_PAL_LINK_LINK_SUPERVISION_REQUEST_REQ(hipPid) send_pal_link_link_supervision_request_req(context, hipPid)

#define SEND_PAL_DM_CHANNEL_LIST_REQ(handle, channelList,scanningNotNeeded) send_pal_dm_channel_list_req(context,getSmeContext(context)->palDmFsmInstance,handle, channelList,scanningNotNeeded)
#define SEND_PAL_DM_START_REQ(handle, scanningNotNeeded, channelList) send_pal_dm_start_req(context,getSmeContext(context)->palDmFsmInstance,handle, scanningNotNeeded, channelList)
#define SEND_PAL_DM_STOP_REQ(handle) send_pal_dm_stop_req(context,getSmeContext(context)->palDmFsmInstance,handle)
#define SEND_PAL_DM_REGISTER_REQ(phyLinkHandle) send_pal_dm_register_req(context,getSmeContext(context)->palDmFsmInstance, phyLinkHandle)
#define SEND_PAL_DM_DEREGISTER_REQ(handle) send_pal_dm_deregister_req(context,getSmeContext(context)->palDmFsmInstance, handle)
#define SEND_PAL_DM_MLME_ACTIVITY_START_REQ(handle) send_pal_dm_mlme_activity_start_req(context,getSmeContext(context)->palDmFsmInstance, handle)
#define SEND_PAL_DM_MLME_ACTIVITY_COMPLETE_REQ(handle) send_pal_dm_mlme_activity_complete_req(context,getSmeContext(context)->palDmFsmInstance, handle, FALSE)
#define SEND_PAL_DM_ENABLE_RTS_CTS_REQ(handle) send_pal_dm_enable_rts_cts_req(context, getSmeContext(context)->palDmFsmInstance,handle)
#define SEND_PAL_DM_DISABLE_RTS_CTS_REQ(handle) send_pal_dm_disable_rts_cts_req(context, getSmeContext(context)->palDmFsmInstance,handle)
#define SEND_PAL_DM_ABORT_LINK_RSP(handle) send_pal_dm_abort_link_rsp(context,getSmeContext(context)->palDmFsmInstance,handle)


#define SEND_PAL_CTRL_LINK_CREATE_REQ(logHandle,phyHandle,prio,remoteMac,localMac,flowSpec) call_pal_ctrl_link_create_req(context,getPalMgrContext(context)->appHandle,logHandle,phyHandle,prio,remoteMac,localMac,flowSpec)
#define SEND_PAL_CTRL_LINK_DELETE_REQ(logHandle,phyHandle) \
{   /* pretend an immediate response if hip sap is down. which means firmware has crashed */\
    if (!hip_auto_cfm_is_hip_sap_open(getSmeContext(context)->hipSapAuto)) \
    {\
        pal_ctrl_link_delete_cfm(context,getPalMgrContext(context)->appHandle,logHandle,phyHandle);\
    }\
    else\
    {\
        call_pal_ctrl_link_delete_req(context,getPalMgrContext(context)->appHandle, logHandle,phyHandle); \
    }\
}
#define SEND_PAL_CTRL_LINK_MODIFY_REQ(logHandle,flowSpec) call_pal_ctrl_link_modify_req(context,getPalMgrContext(context)->appHandle,logHandle,flowSpec)
#define SEND_PAL_CTRL_LINK_FLUSH_REQ(logHandle) call_pal_ctrl_link_flush_req(context,getPalMgrContext(context)->appHandle,logHandle)
#define SEND_PAL_CTRL_FAILED_CONTACT_COUNTER_READ_REQ(logHandle) call_pal_ctrl_failed_contact_counter_read_req(context,getPalMgrContext(context)->appHandle,logHandle)
#define SEND_PAL_CTRL_FAILED_CONTACT_COUNTER_RESET_REQ(logHandle) call_pal_ctrl_failed_contact_counter_reset_req(context,getPalMgrContext(context)->appHandle,logHandle)
#define SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_SET_REQ(handle,lsTimeout) call_pal_ctrl_link_supervision_timeout_set_req(context,getPalMgrContext(context)->appHandle,handle,lsTimeout)
#define SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_MODIFY_REQ(handle,lsTimeout) call_pal_ctrl_link_supervision_timeout_modify_req(context,getPalMgrContext(context)->appHandle,handle,lsTimeout)
#define SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_REQ(handle) \
{   /* pretend an immediate response if hip sap is down. which means firmware has crashed */\
    if (!hip_auto_cfm_is_hip_sap_open(getSmeContext(context)->hipSapAuto)) \
    {\
        pal_ctrl_link_supervision_timeout_delete_cfm(context,getPalMgrContext(context)->appHandle,handle);\
    }\
    else\
    {\
        call_pal_ctrl_link_supervision_timeout_delete_req(context,getPalMgrContext(context)->appHandle,handle);\
    }\
}

#define SEND_PAL_CTRL_LINK_ALIVE_REQ(handle) call_pal_ctrl_link_alive_req(context,getPalMgrContext(context)->appHandle,handle)

#define SEND_PAL_COEX_LINK_CONNECTED_REQ(handle) send_pal_coex_link_connected_req(context,getSmeContext(context)->palCoexFsmInstance,handle)
#define SEND_PAL_COEX_LINK_DISCONNECTED_REQ(handle) send_pal_coex_link_disconnected_req(context,getSmeContext(context)->palCoexFsmInstance, handle)

#define PAL_GET_ROLE_STRING(role) (PalRole_PalRoleInitiator==role?"Initiator":"Receiver")

/*Timer related macros*/
#define PAL_START_TIMER(timer,id) {PAL_Timer *tim=timer; if (tim->enabled) send_pal_link_timer(context,id, tim->value, 100);}
#define PAL_STOP_TIMER(timer,timerId) {PAL_Timer *tim=timer;  if (tim->enabled && timerId.id!=0) fsm_remove_timer(context, timerId); timerId.id=0;}

#define DEFAULT_LINK_SUPERVISION_TIMEOUT (PAL_CALCULATE_LS_TIMEOUT_VALUE(0x7D00)) /* default N as per spec is 0x7D00 which is 20sec*/

/* Get timeout in milliseconds */
#define PAL_CALCULATE_LS_TIMEOUT_VALUE(timeout) ((timeout*LINK_SUPERVISION_TIMEOUT_UNIT_IN_MICROS)/1000)
/* range check for link supervision timer */
#define PAL_LS_TIMEOUT_IN_VALID_RANGE(timeout) (((timeout>=LINK_SUPERVISION_TIMEOUT_MANDATORY_RANGE_MIN)||(timeout==0))?TRUE:FALSE)

#define PAL_GET_LS_TIMEOUT_VALUE(timeout) ((timeout*1000)/LINK_SUPERVISION_TIMEOUT_UNIT_IN_MICROS)


#define PAL_SEND_HCI_COMMAND_STATUS(context,status, commandOpcode) send_hci_command_status_evt(context,getSmeContext(context)->palMgrFsmInstance,status,0, commandOpcode)
#define PAL_SEND_HCI_DISCONNECT_PHYSICAL_LINK_COMPLETE(context,status,phyLinkHandle, reason) send_hci_disconnect_physical_link_complete_evt(context,getSmeContext(context)->palMgrFsmInstance,status,phyLinkHandle, reason)
#define PAL_SEND_HCI_COMMAND_COMPLETE(context,returnParameters, pendingCmd) send_hci_command_complete_evt(context,getSmeContext(context)->palMgrFsmInstance,0,returnParameters)
#define PAL_SEND_HCI_CHANNEL_SELECT(context,phyLinkHandle) send_hci_channel_select_evt(context,getSmeContext(context)->palMgrFsmInstance,phyLinkHandle)
#define PAL_SEND_HCI_PHYSICAL_LINK_COMPLETE(context,status, phyLinkHandle) send_hci_physical_link_complete_evt(context,getSmeContext(context)->palMgrFsmInstance,status, phyLinkHandle)
#define PAL_SEND_HCI_DISCONNECT_LOGICAL_LINK_COMPLETE(context,status,logicalLinkHandle, reason) send_hci_disconnect_logical_link_complete_evt(context,getSmeContext(context)->palMgrFsmInstance,status,logicalLinkHandle, reason)
#define PAL_SEND_HCI_LOGICAL_LINK_COMPLETE(context,status,logicalLinkHandle, phyLinkHandle, txFlowSpecId) send_hci_logical_link_complete_evt(context,getSmeContext(context)->palMgrFsmInstance,status,logicalLinkHandle, phyLinkHandle, txFlowSpecId)
#define PAL_SEND_HCI_MODIFY_FLOW_SPEC_COMPLETE(context, status,logicalLinkHandle) send_hci_flow_spec_modify_complete_evt(context,getSmeContext(context)->palMgrFsmInstance, status,logicalLinkHandle)
#define PAL_SEND_HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE(context, status,phyLinkHandle, shortRangeModeState) send_hci_short_range_mode_change_complete_evt(context,getSmeContext(context)->palMgrFsmInstance, status,phyLinkHandle, shortRangeModeState)

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/
/**
 * @brief
 *   FSM States
 *
 * @par Description
 *   Enum defining the FSM States for this FSM process
 */
typedef enum FsmState
{
    /** Waiting for CORE_START_REQ */
    FSMSTATE_disconnected,

    /** waiting for amp assoc from host for a link creation */
    FSMSTATE_wait_for_amp_assoc,

    /* wait for mlme start */
    FSMSTATE_starting,

    /** Waiting to access to mlme for connect*/
    FSMSTATE_wait_for_mlme_access_connect,

    /** waiting for peer to join this device */
    FSMSTATE_connecting,

    /** link connected */
    FSMSTATE_connected,

    /** disconnect pending */
    FSMSTATE_disconnecting,

    /* Wait for data manager response on data link management*/
    FSMSTATE_configuring_data_manager,

    /** Last enum in the list */
    FSMSTATE_MAX_STATE
} FsmState;


/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/


/**
 * @brief
 *  logical link callback. Called by the logical link manager when a link is deleted.
 *
 * @par Description
 *    on the callback function will send hci event to denote the completion of the logical
 * link delete procedure and also send a request to data manager to delete its entry for
 * the data link on this handle.
 *
 * @param[in]    context   : FSM Context
 * @param[in]    logicalLinkHandle   : handle for the deleted logical link
 *
 * @return
 *   void
 */
static void logical_link_deleted_callback(FsmContext *context, CsrUint16 logicalLinkHandle)
{
    physicalLinkAttrib* phyLink = FSMDATA;

    sme_trace_entry((TR_PAL_DAM,"logical_link_deleted_callback: llHandle-%d",logicalLinkHandle));
    PAL_SEND_HCI_DISCONNECT_LOGICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_SUCCESS, logicalLinkHandle,phyLink->logicalLinkCompleteStatus);

    SEND_PAL_CTRL_LINK_DELETE_REQ(logicalLinkHandle,phyLink->handle);
    /* This varirable is incremented for each logical lik deleted so that the FSM can track how confirmations are pending
         * from the data manager
         */
    phyLink->numLogicalLinksDeleted++;
}


/**
 * @brief
 *  de-initialising physical link parameters.
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 * @param[in]    phyLink   : pointer to the physical link attributes.
 *
 * @return
 *   void
 */
static void pal_deinit_phy_link(FsmContext *context, physicalLinkAttrib *phyLink)
{
    sme_trace_entry((TR_PAL_LM_LINK_FSM, "pal_deinit_phy_link()"));
    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),phyLink->timerId);
    PAL_STOP_TIMER(pal_get_logical_link_accept_timer(context),phyLink->timerId);
    pal_reset_amp_assoc_info(context, &phyLink->remoteAssoc.writeAssocInfo);
}

/**
 * @brief
 *   Initialisation for physical link data structures.
 *
 * @par Description
 *
 * @param[in]    phyLink : pointer to the phyLink structure
 *
 * @return void
 */
static void pal_init_phy_link(physicalLinkAttrib *phyLink)
{
    CsrMemSet(phyLink,0,sizeof(physicalLinkAttrib));
    phyLink->handle = PAL_INVALID_PHYSICAL_LINK_HANDLE;
    phyLink->hipFsmPid = FSM_TERMINATE;

    phyLink->remoteAssoc.connectedChannelList.numChannels= 0;
    phyLink->remoteAssoc.channelList.numChannels= 0;
    phyLink->remoteAssoc.writeAssocInfo.data = NULL;

    phyLink->linkSupervisionTimeoutDuration = DEFAULT_LINK_SUPERVISION_TIMEOUT;
}

/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   Resets the FSM data back to the initial state
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void init_pal_lm_link_data(FsmContext* context)
{
    physicalLinkAttrib* fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_LM_LINK_FSM, "init_ip_connection_mgr_data()"));

    pal_init_phy_link(fsmData);
}

/**
 * @brief
 *   pal_lm_link FSM Entry Function
 *
 * @par Description
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void pal_lm_link_init(FsmContext* context)
{
    sme_trace_entry((TR_PAL_LM_LINK_FSM, "pal_lm_link_init()"));

    fsm_create_params(context, physicalLinkAttrib);
    init_pal_lm_link_data(context);
    fsm_next_state(context, FSMSTATE_disconnected);
}

/**
 * @brief
 *  function to disconnect all active logical links
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 * @param[in]    phyLink   : pointer to the physical link attributes.
 *
 * @return
 *   void
 */
static void pal_disconnect_all_logical_links(FsmContext *context, physicalLinkAttrib *phyLink)
{
    (void)pal_llm_delete_all_logical_links(context, getLlmContext(context), phyLink->handle, logical_link_deleted_callback);
}

/**
 * @brief
 *   validates physical link handle
 *
 * @par Description
 *
 *
 * @param handle: physical link handle to validate
 *
 * @return TRUE if its a valid handle else FALSE.
 */
static CsrBool validate_physical_link_handle(CsrUint8 handle)
{
    if (handle != PAL_INVALID_PHYSICAL_LINK_HANDLE)
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief
 *   save the link key hci-create-physical-link or hci-accept-physical-link-request
 *
 * @par Description
 *    link key can be saved as straight or swapped depending on the compile time option.
 *
 * @param[in]    fsmData   : pointer to the physical link attributes.
 * @param[in]    linkKey   : pointer to the link key buffer
 * @param[in]    keyLength   : length of the link key
 *
 * @return void
 */
static void save_link_key(physicalLinkAttrib *fsmData, CsrUint8 *linkKey, CsrUint16 keyLength)
{
    sme_trace_hex((TR_PAL_LM_HIP_FSM, TR_LVL_DEBUG, "save_link_key(): Swapped Key",linkKey,keyLength));

#ifdef PAL_SWAP_LINK_KEY
    int i;

    for (i=CSR_WIFI_SECURITY_PMK_LENGTH-1; i<=0; i++)
    {
        fsmData->linkKey[(CSR_WIFI_SECURITY_PMK_LENGTH-1)-i] = linkKey[i];
    }
#else
    CsrMemCpy(fsmData->linkKey,linkKey,keyLength);
#endif
}

/**
 * @brief
 *   validate remote AMP Assoc
 *
 * @par Description
 *
 * @param[in]    role   : role of the link (acceptor or initiator)
 * @param[in]    remoteAssoc   : pointer to the decoded remote AMP Assoc structure
 *
 * @return TRUE if valid else FALSE
 */
static CsrBool validate_remote_amp_assoc(PalRole role, AMP_Assoc *remoteAssoc)
{
    CsrBool status=(remoteAssoc->connectedChannelList.numChannels || remoteAssoc->channelList.numChannels);

    sme_trace_entry((TR_PAL_LM_LINK_FSM, "validate_remote_amp_assoc, num Connected channels-%d, preferred channels-%d",remoteAssoc->connectedChannelList.numChannels,remoteAssoc->channelList.numChannels));

    if (status)
    {
        /* being initiator it only requires to have a non-empty channel list
        * , connected or preferred or both
        */
        if (PalRole_PalRoleResponder == role)
        {
            status = FALSE;

            /* if PAL is the responder, then the initiatory must have only one channel in
            * preferred channel list. section 2.12 of PAL spec says,
            * "Subsequent to the HCI Channel Selected event, the initiating AMP device shall
            * create an AMP_ASSOC containing its MAC address, the 802.11 PAL capabilities,
            * and with only the selected channel in its preferred channel list. The host
            * may obtain this AMP_ASSOC by issuing one or more HCI Read Local 
            * AMP_ASSOC commands."
            */
            if (1 == remoteAssoc->channelList.numChannels)
            {
                status = TRUE;
            }
            else if (1 < remoteAssoc->channelList.numChannels)
            {
                /* only one channel in preferred channels is allowed
                 * reset but do not reject */
                sme_trace_entry((TR_PAL_LM_LINK_FSM, "validate_remote_amp_assoc, preferred channel count exceeded, reseting to 1 %d", remoteAssoc->channelList.numChannels));

                remoteAssoc->channelList.numChannels = 1;
                status = TRUE;
            }
        }
    }
    return status;
}

/**
 * @brief
 *   common function to start the physical link creation procedure. It is called following a
 *   succesfull setup of the remote amp_assoc by the host
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 *
 * @return unifi_Success if physical setup started successfully
 *             unifi_Error if failed to start
 */
static unifi_Status pal_setup_physical_link(FsmContext *context)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;
    unifi_Status status=unifi_Error;

    sme_trace_entry((TR_PAL_LM_LINK_FSM, "pal_setup_physical_link, Role - %s",PAL_GET_ROLE_STRING(fsmData->role)));

    if (validate_physical_link_handle(fsmData->handle) &&
        TRUE == validate_remote_amp_assoc(fsmData->role, &fsmData->remoteAssoc))
    {
        /* scanning is not needed if the role is an responder. Simply get the locally
         * preferred channel list
         */
        CsrBool scanningNotNeeded = (PalRole_PalRoleResponder==fsmData->role)?TRUE:FALSE;
        status = unifi_Success;
        fsmData->handle = fsmData->handle;

        pal_set_remote_mac_address(context, fsmData->handle, &fsmData->remoteAssoc.macAddress);

        /* if there are connected channels in remote and we are the initiator then use those in the connected list. 
        * Otherwise use the preferred list
        */
        if (fsmData->remoteAssoc.connectedChannelList.numChannels && PalRole_PalRoleInitiator==fsmData->role)
        {
            sme_trace_info((TR_PAL_LM_LINK_FSM, "pal_setup_physical_link -non empty connectecd list %d",fsmData->remoteAssoc.connectedChannelList.numChannels));
            SEND_PAL_DM_START_REQ(fsmData->handle, scanningNotNeeded,fsmData->remoteAssoc.connectedChannelList);
        }
        else
        {
            sme_trace_info((TR_PAL_LM_LINK_FSM, "pal_setup_physical_link -non empty preferred list %d",fsmData->remoteAssoc.channelList.numChannels));
            SEND_PAL_DM_START_REQ(fsmData->handle, scanningNotNeeded,fsmData->remoteAssoc.channelList);
        }
        fsm_next_state(context, FSMSTATE_starting);
    }
    return status;
}

/**
 * @brief
 *   function called to complete the stop procedure
 *
 * @par Description
 *    A stop procedure is triggered either by an HCI-Reset command or CORE-STOP-REQ
 * A response is sent depending on what triggered the stop procedure.
 *
 * @param[in]    context   : FSM Context
 *
 * @return void
 */
static void stop_completed(FsmContext *context)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    if (fsmData->needResetCompleteEvent)
    {
        ReturnParameters returnParams;

        sme_trace_info((TR_PAL_LM_LINK_FSM, "pal_process_physical_link_cmd(): reset complete. sending command complete"));
        returnParams.commandCode = HCI_RESET_CODE;
        returnParams.hciReturnParam.resetStatus = HCI_STATUS_CODE_SUCCESS;
        PAL_SEND_HCI_COMMAND_COMPLETE(context, returnParams, TRUE);
    }
    else if (fsmData->needCoreStopCfm)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM, "pal_process_physical_link_cmd(): core-stop complete. sending confirm"));
        send_core_stop_cfm(context, getSmeContext(context)->palMgrFsmInstance, unifi_Success);
    }
}

/**
 * @brief
 *   function called to clean up the link
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 *
 * @return void
 */
static void pal_link_cleanup(FsmContext *context)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    verify(TR_PAL_LM_LINK_FSM, fsmData->handle>0);
    if (fsmData->hipFsmPid != FSM_TERMINATE)
    {
        SEND_PAL_LINK_STOP_REQ(fsmData->hipFsmPid);
    }
    stop_completed(context);
    pal_deinit_phy_link(context, fsmData);
    fsm_next_state(context, FSM_TERMINATE);
}

/**
 * @brief
 *   comon function called to process hci-create/accept-physical-link
 *
 * @par Description
 *    If command is successfully processed, then function will proceed with the link creation. Otherwise it will terminate
 * the connection immediately.
 *
 * @param[in]    context   : FSM Context
 * @param[in]    handle   : physical link handle
 * @param[in]    role   : Role of the link
 * @param[in]    linkKey   : link key buffer
 * @param[in]    linkKeyLength   : length of link key
 * @param[in]    linkKeyType   : type of link key
 *
 * @return void
 */
static void pal_process_physical_link_cmd(FsmContext *context,
                                                CsrUint8 handle,
                                                PalRole role,
                                                CsrUint8 *linkKey,
                                                CsrUint8 linkKeyLength,
                                                LinkKeyType linkKeyType)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;
    HciStatusCode status=HCI_STATUS_CODE_UNSPECIFIED_ERROR;

    if (validate_physical_link_handle(handle) &&
        (LINK_KEY_TYPE_DEBUG_COMBINATION_KEY == linkKeyType ||
        LINK_KEY_TYPE_UNAUTHENTICATED_COMBINATION_KEY == linkKeyType ||
        LINK_KEY_TYPE_AUTHENTICATED_COMBINATION_KEY == linkKeyType) &&
        CSR_WIFI_SECURITY_PMK_LENGTH == linkKeyLength)
    {
        status = HCI_STATUS_CODE_SUCCESS;
    }
    PAL_SEND_HCI_COMMAND_STATUS(context, status,
                                (role==PalRole_PalRoleInitiator?HCI_CREATE_PHYSICAL_LINK_CODE:HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CODE));

    sme_trace_info((TR_PAL_LM_LINK_FSM, "pal_process_physical_link_cmd(): handle-%d,role-%d,linkKeyLength-%d,linkKeyType-%d",
                    handle, role, linkKeyLength, linkKeyType));

    /* This field is required while cleaning up incase the command fails. So populate it early */

    fsmData->handle = handle;

    if (HCI_STATUS_CODE_SUCCESS == status)
    {
        fsmData->role = role;
        save_link_key(fsmData, linkKey, linkKeyLength);
        /* start connection accept timer after tje link params are setup */
        PAL_START_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);

        fsm_next_state(context, FSMSTATE_wait_for_amp_assoc);
    }
    else
    {
        /* exit the FSM and reject the link request. send a command status */
        pal_link_cleanup(context);
    }
}


/************************************** TRANSITION FUNCTIONS *******************************************/


static void disconnected__hci_create_physical_link_cmd(FsmContext *context, const HciCreatePhysicalLinkCmd *cmd)
{
    pal_process_physical_link_cmd(context,cmd->physicalLinkHandle,PalRole_PalRoleInitiator, cmd->dedicatedAmpKey,cmd->dedicatedAmpKeyLength,cmd->dedicatedAmpKeyType);
}

static void disconnected__hci_accept_physical_link_cmd(FsmContext *context, const HciAcceptPhysicalLinkRequestCmd *cmd)
{
    pal_process_physical_link_cmd(context,cmd->physicalLinkHandle,PalRole_PalRoleResponder, cmd->dedicatedAmpKey,cmd->dedicatedAmpKeyLength,cmd->dedicatedAmpKeyType);
}

static void wait_for_amp_assoc__hci_write_remote_amp_assoc_cmd(FsmContext *context, const HciWriteRemoteAmpAssocCmd *cmd)
{
    unifi_Status status=unifi_Success;
    ReturnParameters returnParams;
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    sme_trace_entry((TR_PAL_LM_LINK_FSM, "wait_for_amp_assoc__hci_write_remote_amp_assoc_cmd:"
                                      "recveived handle-%d,lsfar-%d,rlen-%d,len-%d",
                                      cmd->physicalLinkHandle,cmd->lengthSoFar,cmd->remainingLength,cmd->assocFragment.dataLen));
    if (!cmd->remainingLength || !cmd->assocFragment.dataLen)
    {
        sme_trace_warn((TR_PAL_LM_LINK_FSM, "Inappropriate write-assoc command"));
        status=unifi_Error;
    }
    else
    {
        if (!cmd->lengthSoFar)
        {
            if (!fsmData->remoteAssoc.writeAssocInfo.totalLen &&
                cmd->remainingLength <= PAL_AMP_ASSOC_MAX_TOTAL_LENGTH)
            {
                sme_trace_info((TR_PAL_LM_LINK_FSM, "First write-assoc command of total length - %d",cmd->remainingLength));
                fsmData->remoteAssoc.writeAssocInfo.data = CsrPmalloc(cmd->remainingLength);
                fsmData->remoteAssoc.writeAssocInfo.totalLen = cmd->remainingLength;
            }
            else
            {
                sme_trace_error((TR_PAL_LM_LINK_FSM, "Remote AMP Assoc is bigger than PAL can support - (lengthReceived-%d, ,maxSupported-%d)",cmd->remainingLength,PAL_AMP_ASSOC_MAX_TOTAL_LENGTH));
                status = unifi_Error;
            }
        }
        if (unifi_Success == status &&
            ((fsmData->remoteAssoc.writeAssocInfo.totalLen-fsmData->remoteAssoc.writeAssocInfo.currentLen)
             == cmd->remainingLength) &&
            (cmd->assocFragment.dataLen <= cmd->remainingLength) &&
            (cmd->lengthSoFar == fsmData->remoteAssoc.writeAssocInfo.currentLen))
        {
            CsrMemCpy(fsmData->remoteAssoc.writeAssocInfo.data+fsmData->remoteAssoc.writeAssocInfo.currentLen,
                       cmd->assocFragment.data,
                       cmd->assocFragment.dataLen);
            fsmData->remoteAssoc.writeAssocInfo.currentLen+=cmd->assocFragment.dataLen;
        }
        else
        {
            sme_trace_warn((TR_PAL_LM_LINK_FSM, "Length fields mismatch in write-assoc command"));
            status = unifi_Error;
        }
    }

    /* Send the command complete first */
    returnParams.commandCode = HCI_WRITE_REMOTE_AMP_ASSOC_CODE;
    returnParams.hciReturnParam.writeRemoteAmpAssocReturn.physicalLinkHandle=cmd->physicalLinkHandle;
    if (unifi_Error == status)
    {
        sme_trace_error((TR_PAL_LM_LINK_FSM,"physical link handle with the write-amp-assoc command not recognised"));
        returnParams.hciReturnParam.writeRemoteAmpAssocReturn.status = HCI_STATUS_CODE_INVALID_HCI_COMMAND_PARAMETERS;
    }
    else
    {
        /* check to see if assoc is written in full and see if decode is success */
        if (!(fsmData->remoteAssoc.writeAssocInfo.totalLen-fsmData->remoteAssoc.writeAssocInfo.currentLen) &&
            FALSE == pal_decode_amp_assoc(context,
                                         fsmData->remoteAssoc.writeAssocInfo.data,
                                         fsmData->remoteAssoc.writeAssocInfo.totalLen,
                                         &fsmData->remoteAssoc))
        {
            sme_trace_error((TR_PAL_LM_LINK_FSM,"AMP Assoc decoding failed. Rejecting the connection"));
            status = unifi_Error;
            returnParams.hciReturnParam.writeRemoteAmpAssocReturn.status = HCI_STATUS_CODE_INVALID_HCI_COMMAND_PARAMETERS;
        }
        else
        {
            sme_trace_info((TR_PAL_LM_LINK_FSM,"AMP Assoc decoding Success. Continue with connection"));
            returnParams.hciReturnParam.writeRemoteAmpAssocReturn.status = HCI_STATUS_CODE_SUCCESS;
        }
    }

    PAL_SEND_HCI_COMMAND_COMPLETE(context, returnParams, TRUE);

    /* Kick off the link creation if this was the last fragment */
    if (unifi_Success == status &&
       !(fsmData->remoteAssoc.writeAssocInfo.totalLen-fsmData->remoteAssoc.writeAssocInfo.currentLen))
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM, "End of write-assoc command-start link creation"));
        status = pal_setup_physical_link(context);
        if (unifi_Success == status)
        {
            sme_trace_info((TR_PAL_LM_LINK_FSM, "Link creation successfully started"));
            pal_reset_amp_assoc_info(context, &fsmData->remoteAssoc.writeAssocInfo); /* if its a failure then assoc is reset below */
        }
    }

    /* Send physical_link_complete failure if write amp assoc or link creation failed */
    if (unifi_Error == status)
    {
        /* write assoc failed, free resources if there was anything allocated and send a complete with failure*/
        pal_reset_amp_assoc_info(context, &fsmData->remoteAssoc.writeAssocInfo);
        /* Also reject create-link- request*/
        PAL_SEND_HCI_PHYSICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_INVALID_HCI_COMMAND_PARAMETERS,
                                            fsmData->handle);
        pal_link_cleanup(context);
    }
}

static void wait_for_amp_assoc__hci_disconnect_physical_link_cmd(FsmContext *context, const HciDisconnectPhysicalLinkCmd *cmd)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_SUCCESS, HCI_DISCONNECT_PHYSICAL_LINK_CODE);

    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);
    /* As per HCI Spec section 7.1.39:
     * "If the cancellation was successful, the Physical Link Complete event
     *  shall be generated with the error code Unknown Connection Identifier (0x02)."
     */
    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;

    PAL_SEND_HCI_PHYSICAL_LINK_COMPLETE(context, fsmData->phyLinkCompleteStatus, fsmData->handle);

    PAL_SEND_HCI_DISCONNECT_PHYSICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_SUCCESS, fsmData->handle,
                                                   HCI_STATUS_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST);
    pal_link_cleanup(context);
}

static void wait_for_amp_assoc__stop(FsmContext *context, const FsmEvent *req)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"starting__stop:"));

    if (HCI_RESET_CODE == req->id)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"starting__stop: HCI_RESET"));
        fsmData->needResetCompleteEvent = TRUE;
    }
    else
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"starting__stop: CORE_STOP_REQ"));
        fsmData->needCoreStopCfm = TRUE;
    }

    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);
    /* As per HCI Spec section 7.1.39:
     * "If the cancellation was successful, the Physical Link Complete event
     *  shall be generated with the error code Unknown Connection Identifier (0x02)."
     */
    PAL_SEND_HCI_PHYSICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER, fsmData->handle);
    pal_link_cleanup(context);
}

static void wait_for_amp_assoc__pal_connection_accept_timeout_ind(FsmContext *context, const PalLinkTimer_Evt *timer)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    PAL_SEND_HCI_PHYSICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_CONNECTION_TIMEOUT, fsmData->handle);
    pal_link_cleanup(context);
}

static void connecting__pal_link_connect_cfm(FsmContext *context, const PalLinkConnectCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);
    if (unifi_Success == cfm->connectStatus)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM, "connecting__pal_link_connect_cfm():Connection succeeded."));

        SEND_PAL_DM_MLME_ACTIVITY_COMPLETE_REQ(fsmData->handle); /* No need for a confirm */
        SEND_PAL_COEX_LINK_CONNECTED_REQ(fsmData->handle); /* No need for a confirm */

        SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_SET_REQ(fsmData->handle, fsmData->linkSupervisionTimeoutDuration);
        fsm_next_state(context, FSMSTATE_configuring_data_manager);
    }
    else
    {
        sme_trace_warn((TR_PAL_LM_LINK_FSM, "Authentication failed."));
        fsmData->needPhysLinkCompleteEvent = TRUE;
        fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_UNSPECIFIED_ERROR;

        SEND_PAL_COEX_LINK_DISCONNECTED_REQ(fsmData->handle);
        fsm_next_state(context, FSMSTATE_disconnecting);
    }
}

static void connected__pal_link_disconnect_ind(FsmContext *context, const PalLinkDisconnectInd_Evt *ind)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    fsmData->hipLinkDisconnectedReason = (unifi_TimedOut==ind->disconnectStatus)?HCI_STATUS_CODE_CONNECTION_TIMEOUT:HCI_STATUS_CODE_REMOTE_USER_TERMINATED_CONNECTION;
    fsmData->logicalLinkCompleteStatus = fsmData->hipLinkDisconnectedReason;

    /* Disconnect all the logical links first if there is any before disconnecting the physical link */
    pal_disconnect_all_logical_links(context, fsmData);
    if (!fsmData->numLogicalLinksDeleted)
    {
        sme_trace_entry((TR_PAL_LM_LINK_FSM, "connected__pal_link_disconnect_ind(): No logical to delete. So delete link supervision timeout in data manager"));
        SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_REQ(fsmData->handle);
    }
    /* If logical link were present then wait for the data links to be deleted from data manager
     * where all packets will be flushed as well.
     */
    fsm_next_state(context, FSMSTATE_disconnecting);
}

static void connected__pal_link_link_supervision_response_ind(FsmContext *context, const PalLinkLinkSupervisionResponseInd_Evt *ind)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    SEND_PAL_CTRL_LINK_ALIVE_REQ(fsmData->handle);
}

static void connected__pal_ctrl_early_link_loss_ind(FsmContext *context, const PalCtrlEarlyLinkLossInd_Evt *ind)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    SEND_PAL_LINK_LINK_SUPERVISION_REQUEST_REQ(fsmData->hipFsmPid);
}

static void connected__pal_ctrl_link_lost_ind(FsmContext *context, const PalCtrlLinkLostInd_Evt *ind)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    fsmData->logicalLinkCompleteStatus = HCI_STATUS_CODE_CONNECTION_TIMEOUT;
    fsmData->hipLinkDisconnectedReason = HCI_STATUS_CODE_CONNECTION_TIMEOUT;
    /* Disconnect all the logical links first if there is any before disconnecting the physical link */
    pal_disconnect_all_logical_links(context, fsmData);

    if (!fsmData->numLogicalLinksDeleted)
    {
        sme_trace_entry((TR_PAL_LM_LINK_FSM, "connected__pal_ctrl_link_lost_ind(): No logical to delete. So delete link supervision timeout in data manager"));
        SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_REQ(fsmData->handle);
    }
    /* If logical link were present then wait for the data links to be deleted from data manager
     * where all packets will be flushed as well.
     */
    fsm_next_state(context, FSMSTATE_disconnecting);
}

static void connected__hci_disconnect_physical_link_cmd(FsmContext *context, const HciDisconnectPhysicalLinkCmd *cmd)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_SUCCESS, HCI_DISCONNECT_PHYSICAL_LINK_CODE);

    fsmData->discRequested = TRUE;

    fsmData->logicalLinkCompleteStatus = HCI_STATUS_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST;
    /* Disconnect all the logical links first if there is any before disconnecting the physical link */
    pal_disconnect_all_logical_links(context, fsmData);

    if (!fsmData->numLogicalLinksDeleted)
    {
        sme_trace_entry((TR_PAL_LM_LINK_FSM, "connected__hci_disconnect_physical_link_cmd(): No logical to delete. So delete link supervision timeout in data manager"));
        SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_REQ(fsmData->handle);
    }
    /* If logical link were present then wait for the data links to be deleted from data manager
     * where all packets will be flushed as well.
     */
    fsm_next_state(context, FSMSTATE_disconnecting);
}

static void disconnecting__hci_disconnect_physical_link_cmd(FsmContext *context, const HciDisconnectPhysicalLinkCmd *cmd)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_SUCCESS, HCI_DISCONNECT_PHYSICAL_LINK_CODE);

    fsmData->discRequested = TRUE;
    /* Wait for the disconnect procedure to finish */
}

static void disconnecting__pal_link_disconnect_cfm(FsmContext *context, const PalLinkDisconnectCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    SEND_PAL_DM_MLME_ACTIVITY_COMPLETE_REQ(fsmData->handle);
    SEND_PAL_DM_STOP_REQ(fsmData->handle);
}

static void disconnecting__pal_link_disconnect_ind(FsmContext *context, const PalLinkDisconnectInd_Evt *ind)
{
    disconnecting__pal_link_disconnect_cfm(context, (PalLinkDisconnectCfm_Evt *)ind);
}

static void disconnecting__pal_dm_stop_cfm(FsmContext *context, const PalDmStopCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    /* Disconnect must not fail.*/
    if (fsmData->needPhysLinkCompleteEvent)
    {
        PAL_SEND_HCI_PHYSICAL_LINK_COMPLETE(context, fsmData->phyLinkCompleteStatus, fsmData->handle);
    }

    if (fsmData->discRequested)
    {
        PAL_SEND_HCI_DISCONNECT_PHYSICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_SUCCESS, fsmData->handle,
                                                       HCI_STATUS_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST);
    }
    else if (!fsmData->needPhysLinkCompleteEvent)
    {
        /* If it comes here, it means that remote side has disconnected
         * the link for some reason or there is a link loss
         * while the connection was in a CONNECTED state.
         */
        PAL_SEND_HCI_DISCONNECT_PHYSICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_SUCCESS, fsmData->handle,
                                                       fsmData->hipLinkDisconnectedReason);
    }

    pal_link_cleanup(context);
}

static void disconnecting__pal_ctrl_link_supervision_timeout_delete_cfm(FsmContext *context, const PalCtrlLinkSupervisionTimeoutDeleteCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"disconnecting__pal_ctrl_link_supervision_timeout_delete_cfm:"));
    SEND_PAL_COEX_LINK_DISCONNECTED_REQ(fsmData->handle);
}

static void disconnecting__pal_ctrl_link_delete_cfm(FsmContext *context, const PalCtrlLinkDeleteCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"disconnecting__pal_ctrl_link_delete_cfm: num data links deleted-%d",fsmData->numLogicalLinksDeleted));
    verify(TR_PAL_LM_LINK_FSM, fsmData->numLogicalLinksDeleted>0);
    fsmData->numLogicalLinksDeleted--;

    if (!fsmData->numLogicalLinksDeleted)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"disconnecting__pal_ctrl_link_delete_cfm:No more data links to be deleted. Delete link supervision timeout"));
        SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_REQ(fsmData->handle);
    }
    /* else remain in the same state until all data links confirms are received */
}

static void disconnecting__pal_coex_link_disconnected_cfm(FsmContext *context, const PalCoexLinkDisconnectedCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"disconnecting__pal_coex_link_disconnected_cfm:"));

    /* Disconnect HIP fsm if there is one. otherwise continue with device manager*/
    if (fsmData->hipFsmPid != FSM_TERMINATE)
    {
        SEND_PAL_LINK_DISCONNECT_REQ(fsmData->hipFsmPid);
    }
    else
    {
        SEND_PAL_DM_MLME_ACTIVITY_COMPLETE_REQ(fsmData->handle);
        SEND_PAL_DM_STOP_REQ(fsmData->handle);
    }
}

static void disconnecting__stop(FsmContext *context, const FsmEvent *req)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"save stop req: id-0x%d",req->id));

    if (HCI_RESET_CODE == req->id)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"save stop req: HCI_RESET"));
        fsmData->needResetCompleteEvent = TRUE;
    }
    else
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"save stop req: CORE_STOP_REQ"));
        fsmData->needCoreStopCfm = TRUE;
    }
}

static void connecting__hci_disconnect_physical_link_cmd(FsmContext *context, const HciDisconnectPhysicalLinkCmd *cmd)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_SUCCESS, HCI_DISCONNECT_PHYSICAL_LINK_CODE);

    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);

    /* HCI_Create_Physical_Link_Complete needs to go when disconnect procedure is completed */
    fsmData->needPhysLinkCompleteEvent = TRUE;
    fsmData->discRequested = TRUE;

    /* As per HCI Spec section 7.1.39:
     * "If the cancellation was successful, the Physical Link Complete event
     *  shall be generated with the error code Unknown Connection Identifier (0x02)."
     */
    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;

    /* start disconnect procedure starting with coex */
    SEND_PAL_COEX_LINK_DISCONNECTED_REQ(fsmData->handle);
    fsm_next_state(context, FSMSTATE_disconnecting);
}

static void connecting__stop(FsmContext *context, const FsmEvent *req)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"connecting__stop:"));

    if (HCI_RESET_CODE == req->id)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"connecting__stop: HCI_RESET"));
        fsmData->needResetCompleteEvent = TRUE;
    }
    else
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"connecting__stop: CORE_STOP_REQ"));
        fsmData->needCoreStopCfm = TRUE;
    }

    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);

    /* HCI_Create_Physical_Link_Complete needs to go when disconnect procedure is completed */
    fsmData->needPhysLinkCompleteEvent = TRUE;

    /* As per HCI Spec section 7.1.39:
     * "If the cancellation was successful, the Physical Link Complete event
     *  shall be generated with the error code Unknown Connection Identifier (0x02)."
     */
    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;

     /* start with disconnecting coex */
     SEND_PAL_COEX_LINK_DISCONNECTED_REQ(fsmData->handle);
     fsm_next_state(context, FSMSTATE_disconnecting);
}

static void connecting__pal_connection_accept_timeout_ind(FsmContext *context, const PalLinkTimer_Evt *timer)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_CONNECTION_TIMEOUT;
    fsmData->needPhysLinkCompleteEvent = TRUE;
    /* start disconnect procedure starting with coex */
    SEND_PAL_COEX_LINK_DISCONNECTED_REQ(fsmData->handle);
    fsm_next_state(context, FSMSTATE_disconnecting);

}

static void connected__hci_read_rssi_cmd(FsmContext *context, const HciReadRssiCmd *cmd)
{
    fsm_forward_event(context, getSmeContext(context)->palDmFsmInstance, (FsmEvent*)cmd);
}

static void connected__hci_read_link_quality_cmd(FsmContext *context, const HciReadLinkQualityCmd *cmd)
{
    fsm_forward_event(context, getSmeContext(context)->palDmFsmInstance, (FsmEvent*)cmd);
}

static void connected__hci_create_logical_link_cmd(FsmContext *context, const HciCreateLogicalLinkCmd *cmd)
{
    HciStatusCode status;

    if (pal_llm_new_logical_link_acceptable(context, getLlmContext(context), &cmd->txFlowSpec,pal_get_link_qos_support(context, cmd->physicalLinkHandle)))
    {
        status=HCI_STATUS_CODE_SUCCESS;
    }
    else
    {
        status=HCI_STATUS_CODE_QOS_REJECTED;
    }
    PAL_SEND_HCI_COMMAND_STATUS(context, status,
                                cmd->common.id);

    if (HCI_STATUS_CODE_SUCCESS == status)
    {
        /* physical link requested is valid so start the logical link creation */
        CsrUint16 logicalLinkHandle;
        unifi_AmpFlowSpec damFlowSpec;
        Priority userPriority;

        pal_llm_create_logical_link(getLlmContext(context),
                                    cmd->physicalLinkHandle,
                                    &cmd->txFlowSpec,
                                    &cmd->rxFlowSpec,
                                    pal_get_link_qos_support(context, cmd->physicalLinkHandle),
                                    &logicalLinkHandle, &userPriority, &damFlowSpec.flushTimeout);

        sme_trace_info((TR_PAL_LM_LINK_FSM, "flushtimeout - %d",damFlowSpec.flushTimeout));
        damFlowSpec.accessLatency = cmd->txFlowSpec.accessLatency;
        damFlowSpec.serviceType = cmd->txFlowSpec.serviceType;
        damFlowSpec.maximumSduSize = cmd->txFlowSpec.maximumSduSize;
        damFlowSpec.sduInterArrivalTime = cmd->txFlowSpec.sduInterArrivalTime;

        SEND_PAL_CTRL_LINK_CREATE_REQ(logicalLinkHandle, cmd->physicalLinkHandle,
                                          userPriority,
                                          pal_get_remote_mac_address(context, cmd->physicalLinkHandle),
                                          pal_get_local_mac_address(context),
                                          &damFlowSpec);
        fsm_next_state(context, FSMSTATE_configuring_data_manager);
    }
}

static void connected__hci_accept_logical_link_cmd(FsmContext *context, const HciAcceptLogicalLinkCmd *cmd)
{
    connected__hci_create_logical_link_cmd(context, (HciCreateLogicalLinkCmd *)cmd);
}

static void connected__hci_flow_spec_modify_cmd(FsmContext *context, const HciFlowSpecModifyCmd *cmd)
{
    HciStatusCode status=HCI_STATUS_CODE_QOS_REJECTED;
    unifi_AmpFlowSpec damFlowSpec;

    if (pal_llm_flow_spec_modify_acceptable(context, getLlmContext(context),
                                            cmd->handle,
                                            &cmd->txFlowSpec) &&
        pal_llm_update_link_with_modified_flow_spec(getLlmContext(context),
                                                    cmd->handle,
                                                    &cmd->txFlowSpec,
                                                    &damFlowSpec.flushTimeout))
    {
        status = HCI_STATUS_CODE_SUCCESS;
    }
    PAL_SEND_HCI_COMMAND_STATUS(context, status, HCI_FLOW_SPEC_MODIFY_CODE);


    if (HCI_STATUS_CODE_SUCCESS == status)
    {
        physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

        damFlowSpec.accessLatency = cmd->txFlowSpec.accessLatency;
        damFlowSpec.serviceType = cmd->txFlowSpec.serviceType;
        damFlowSpec.maximumSduSize = cmd->txFlowSpec.maximumSduSize;
        damFlowSpec.sduInterArrivalTime = cmd->txFlowSpec.sduInterArrivalTime;
        fsmData->pendingCommandCode = HCI_FLOW_SPEC_MODIFY_CODE;
        SEND_PAL_CTRL_LINK_MODIFY_REQ(cmd->handle, &damFlowSpec);
        fsm_next_state(context, FSMSTATE_configuring_data_manager);
    }
}

static void connected__hci_logical_link_cancel_cmd(FsmContext *context, const HciLogicalLinkCancelCmd *cmd)
{
    ReturnParameters params;

    params.commandCode = HCI_LOGICAL_LINK_CANCEL_CODE;
    params.hciReturnParam.logicalLinkCancelReturn.physicalLinkHandle = cmd->physicalLinkHandle;
    params.hciReturnParam.logicalLinkCancelReturn.txFlowSpecId = cmd->txFlowSpecId;

    if (pal_llm_get_logical_link_handle_from_flow_spec_id(getLlmContext(context), cmd->txFlowSpecId)
        != PAL_INVALID_LOGICAL_LINK_HANDLE)
    {
        /* Link is already created so return the failure */
        params.hciReturnParam.logicalLinkCancelReturn.status = HCI_STATUS_CODE_ACL_CONNECTION_ALREADY_EXISTS;
    }
    else
    {
        params.hciReturnParam.logicalLinkCancelReturn.status = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
    }

    PAL_SEND_HCI_COMMAND_COMPLETE(context, params, TRUE);
}

static void connected__hci_disconnect_logical_link_cmd(FsmContext *context, const HciDisconnectLogicalLinkCmd *cmd)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;
    HciStatusCode status=HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;

    if (pal_llm_link_exits(getLlmContext(context),cmd->logicalLinkHandle))
    {
        status = HCI_STATUS_CODE_SUCCESS;
    }
    PAL_SEND_HCI_COMMAND_STATUS(context, status, HCI_DISCONNECT_LOGICAL_LINK_CODE);

    if (HCI_STATUS_CODE_SUCCESS == status)
    {
        SEND_PAL_CTRL_LINK_DELETE_REQ(cmd->logicalLinkHandle, fsmData->handle);
        fsm_next_state(context, FSMSTATE_configuring_data_manager);
    }
}

static void connected__hci_read_best_effort_flush_timeout_cmd(FsmContext *context, const HciReadBestEffortFlushTimeoutCmd *cmd)
{
    ReturnParameters retParams;

    retParams.commandCode = HCI_READ_BEST_EFFORT_FLUSH_TIMEOUT_CODE;

    if (unifi_Success == pal_llm_read_best_effor_flush_timeout(getLlmContext(context),cmd->logicalLinkHandle,
                                                             &retParams.hciReturnParam.readBEFlushTimeoutReturn.bestEffortFlushTimeout))
    {
        retParams.hciReturnParam.writeBEFlushTimeoutReturn = HCI_STATUS_CODE_SUCCESS;
    }
    else
    {
        retParams.hciReturnParam.writeBEFlushTimeoutReturn = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
    }
    PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
}

static void connected__hci_write_best_effort_flush_timeout_cmd(FsmContext *context, const HciWriteBestEffortFlushTimeoutCmd *cmd)
{
    AmpFlowSpec currentTxFlowSpec;

    if (unifi_Success == pal_llm_write_best_effor_flush_timeout(context, 
                                                                getLlmContext(context),
                                                                cmd->logicalLinkHandle,
                                                                cmd->bestEffortFlushTimeout,
                                                                &currentTxFlowSpec))
    {
        physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;
        unifi_AmpFlowSpec damFlowSpec;

        damFlowSpec.accessLatency = currentTxFlowSpec.accessLatency;
        damFlowSpec.serviceType = currentTxFlowSpec.serviceType;
        damFlowSpec.maximumSduSize = currentTxFlowSpec.maximumSduSize;
        damFlowSpec.sduInterArrivalTime = currentTxFlowSpec.sduInterArrivalTime;
        damFlowSpec.flushTimeout = currentTxFlowSpec.flushTimeout;
        fsmData->pendingCommandCode = HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CODE;
        SEND_PAL_CTRL_LINK_MODIFY_REQ(cmd->logicalLinkHandle, &damFlowSpec);
        fsm_next_state(context, FSMSTATE_configuring_data_manager);
    }
    else
    {
        ReturnParameters retParams;

        retParams.commandCode = HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CODE;
        /* handle is already validated by the Manager. so the failure should be with the parameter itself */
        retParams.hciReturnParam.readBEFlushTimeoutReturn.status = HCI_STATUS_CODE_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE;
        PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
    }
}

static void connected__hci_enhanced_flush_cmd(FsmContext *context, const HciFlushCmd *cmd)
{
    PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_SUCCESS, HCI_ENHANCED_FLUSH_CODE);
    SEND_PAL_CTRL_LINK_FLUSH_REQ(cmd->handle);
    fsm_next_state(context, FSMSTATE_configuring_data_manager);
}

static void connected__hci_reset_failed_contact_counter_cmd(FsmContext *context, const HciResetFailedContactCounterCmd *cmd)
{
    SEND_PAL_CTRL_FAILED_CONTACT_COUNTER_RESET_REQ(cmd->handle);
    fsm_next_state(context, FSMSTATE_configuring_data_manager);
}

static void connected__hci_read_failed_contact_counter_cmd(FsmContext *context, const HciReadFailedContactCounterCmd *cmd)
{
    SEND_PAL_CTRL_FAILED_CONTACT_COUNTER_READ_REQ(cmd->handle);
    fsm_next_state(context, FSMSTATE_configuring_data_manager);
}

static void connected__hci_read_link_supervision_timeout_cmd(FsmContext *context, const HciReadLinkSupervisionTimeoutCmd *cmd)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;
    ReturnParameters retParams;

    retParams.commandCode = HCI_READ_LINK_SUPERVISION_TIMEOUT_CODE;
    retParams.hciReturnParam.readTimeoutReturn.status = HCI_STATUS_CODE_SUCCESS;
    retParams.hciReturnParam.readTimeoutReturn.handle = cmd->handle;
    retParams.hciReturnParam.readTimeoutReturn.linkSupervisionTimeout = (CsrUint16)PAL_GET_LS_TIMEOUT_VALUE(fsmData->linkSupervisionTimeoutDuration);
    PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
}

static void connected__hci_write_link_supervision_timeout_cmd(FsmContext *context, const HciWriteLinkSupervisionTimeoutCmd *cmd)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    if (PAL_LS_TIMEOUT_IN_VALID_RANGE(cmd->linkSupervisionTimeout))
    {
        fsmData->linkSupervisionTimeoutDuration = (CsrUint16)PAL_CALCULATE_LS_TIMEOUT_VALUE(cmd->linkSupervisionTimeout);

        sme_trace_info((TR_PAL_LM_LINK_FSM, "connected__hci_write_link_supervision_timeout_cmd(): new supervision timeout set - %d",fsmData->linkSupervisionTimeoutDuration));

        SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_MODIFY_REQ(fsmData->handle, fsmData->linkSupervisionTimeoutDuration);
        fsm_next_state(context, FSMSTATE_configuring_data_manager);
    }
    else
    {
        ReturnParameters retParams;

        retParams.commandCode = HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CODE;
        retParams.hciReturnParam.writeTimeoutReturn.status = HCI_STATUS_CODE_INVALID_HCI_COMMAND_PARAMETERS;
        retParams.hciReturnParam.writeTimeoutReturn.handle = cmd->handle;
        PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
    }
}

static void connected__hci_short_range_mode_cmd(FsmContext *context, const HciShortRangeModeCmd *cmd)
{
    PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_SUCCESS, cmd->common.id);
    fsm_forward_event(context, getSmeContext(context)->palDmFsmInstance, (FsmEvent*)cmd);
}

static void connected__stop(FsmContext *context, const FsmEvent *req)
{
    physicalLinkAttrib *phyLink = FSMDATA_PHYLINK;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"connected__stop: id-0x%d",req->id));

    if (HCI_RESET_CODE == req->id)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"connected__stop: HCI_RESET"));
        phyLink->needResetCompleteEvent = TRUE;
    }
    else
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"connected__stop: CORE_STOP_REQ"));
        phyLink->needCoreStopCfm = TRUE;
    }
    phyLink->discRequested = TRUE;
    phyLink->logicalLinkCompleteStatus = HCI_STATUS_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST;

    /* Disconnect all the logical links first if there is any before disconnecting the physical link */
    pal_disconnect_all_logical_links(context, phyLink);

    if (!phyLink->numLogicalLinksDeleted)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM, "connected__stop(): No logical to delete. So delete link supervision timeout in data manager"));
        SEND_PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_REQ(phyLink->handle);
    }
    /* If logical link were present then wait for the data links to be deleted from data manager
         * where all packets will be flushed as well.
         */
    fsm_next_state(context, FSMSTATE_disconnecting);
}

static void configuring_data_manager__pal_ctrl_failed_contact_counter_read_cfm(FsmContext *context, const PalCtrlFailedContactCounterReadCfm_Evt *cfm)
{
    ReturnParameters retParams;

    retParams.commandCode = HCI_READ_FAILED_CONTACT_COUNTER_CODE;
    retParams.hciReturnParam.readFailedContactCounterReturn.handle = cfm->logicalLinkHandle;
    retParams.hciReturnParam.readFailedContactCounterReturn.failedContactCounter = cfm->failedContactCounter;
    retParams.hciReturnParam.resetFailedContactCounterReturn.status = HCI_STATUS_CODE_SUCCESS;
    PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
    fsm_next_state(context, FSMSTATE_connected);
}

static void configuring_data_manager__pal_ctrl_failed_contact_counter_reset_cfm(FsmContext *context, const PalCtrlFailedContactCounterResetCfm_Evt *cfm)
{
    ReturnParameters retParams;

    retParams.commandCode = HCI_RESET_FAILED_CONTACT_COUNTER_CODE;
    retParams.hciReturnParam.resetFailedContactCounterReturn.handle = cfm->logicalLinkHandle;
    retParams.hciReturnParam.resetFailedContactCounterReturn.status = HCI_STATUS_CODE_SUCCESS;
    PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
    fsm_next_state(context, FSMSTATE_connected);
}

static void configuring_data_manager__pal_ctrl_link_flush_cfm(FsmContext *context, const PalCtrlLinkFlushCfm_Evt *cfm)
{
    sme_trace_info((TR_PAL_LM_LINK_FSM,"configuring_data_manager__pal_ctrl_link_flush_cfm:loigical handl-%d",cfm->logicalLinkHandle));

    send_hci_enhanced_flush_complete_evt(context,getSmeContext(context)->palMgrFsmInstance,cfm->logicalLinkHandle);
    fsm_next_state(context, FSMSTATE_connected);
}

static void configuring_data_manager__pal_ctrl_link_create_cfm(FsmContext *context, const PalCtrlLinkCreateCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    sme_trace_info((TR_PAL_LM_LINK_FSM,"configuring_data_manager__pal_ctrl_link_create_cfm:loigical handl-%d",cfm->logicalLinkHandle));
    PAL_SEND_HCI_LOGICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_SUCCESS, 
                                       cfm->logicalLinkHandle, 
                                       fsmData->handle,
                                       pal_llm_get_tx_flow_spec_from_logical_link_handle(getLlmContext(context),cfm->logicalLinkHandle)->id);

    fsm_next_state(context, FSMSTATE_connected);
}

static void configuring_data_manager__pal_ctrl_link_modify_cfm(FsmContext *context, const PalCtrlLinkModifyCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;
    
    if (HCI_FLOW_SPEC_MODIFY_CODE == fsmData->pendingCommandCode)
    {
        PAL_SEND_HCI_MODIFY_FLOW_SPEC_COMPLETE(context, HCI_STATUS_CODE_SUCCESS, cfm->logicalLinkHandle);
    }
    else if (HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CODE == fsmData->pendingCommandCode)
    {
        ReturnParameters retParams;
        
        retParams.commandCode = HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CODE;
        retParams.hciReturnParam.readBEFlushTimeoutReturn.status = HCI_STATUS_CODE_SUCCESS;
        PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
    }
    fsmData->pendingCommandCode = 0;

    fsm_next_state(context, FSMSTATE_connected);
}

static void configuring_data_manager__pal_ctrl_link_delete_cfm(FsmContext *context, const PalCtrlLinkDeleteCfm_Evt *cfm)
{
    (void)pal_llm_delete_matching_logical_link(context, getLlmContext(context),cfm->logicalLinkHandle);

    PAL_SEND_HCI_DISCONNECT_LOGICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_SUCCESS,
                                                  cfm->logicalLinkHandle,
                                                  HCI_STATUS_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST);

    fsm_next_state(context, FSMSTATE_connected);
}

static void configuring_data_manager__pal_ctrl_link_supervision_modify_cfm(FsmContext *context, const PalCtrlLinkSupervisionTimeoutModifyCfm_Evt *cfm)
{
    ReturnParameters retParams;

    retParams.commandCode = HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CODE;
    retParams.hciReturnParam.writeTimeoutReturn.status = HCI_STATUS_CODE_SUCCESS;
    retParams.hciReturnParam.writeTimeoutReturn.handle = cfm->physicalLinkHandle;
    PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
    fsm_next_state(context, FSMSTATE_connected);
}

static void configuring_data_manager__pal_ctrl_link_supervision_set_cfm(FsmContext *context, const PalCtrlLinkSupervisionTimeoutSetCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;
    /* link creation is success */
    PAL_SEND_HCI_PHYSICAL_LINK_COMPLETE(context, HCI_STATUS_CODE_SUCCESS,
                                        fsmData->handle);

#ifdef REMOVE_CODE
    /* Enable RTS/CTS by default for all packets. Clause 5.2.2. PAL Spec */
    SEND_PAL_DM_ENABLE_RTS_CTS_REQ(fsmData->handle);

    /* send an acivity report to peer*/
    pal_send_activity_report(context, fsmData);
#endif

    fsm_next_state(context, FSMSTATE_connected);
}

static void starting__pal_dm_start_cfm(FsmContext *context, const PalDmStartCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    /* process disconnect first if it was pending */
    if (fsmData->discRequested || fsmData->needPhysLinkCompleteEvent)
    {
        SEND_PAL_COEX_LINK_DISCONNECTED_REQ(fsmData->handle);
        fsm_next_state(context, FSMSTATE_disconnecting);
    }
    else if (PaldmStatus_Success == cfm->startStatus)
    {
        if (PalRole_PalRoleInitiator == fsmData->role)
        {
            /* Send the intermediate response to the AMP manager */
            PAL_SEND_HCI_CHANNEL_SELECT(context, fsmData->handle);

            sme_trace_info((TR_PAL_LM_LINK_FSM,"starting__pal_dm_start_cfm:Initiator:Waiting for receiver to kick off connect procedure"));
        }
        else
        {
            sme_trace_info((TR_PAL_LM_LINK_FSM,"starting__pal_dm_start_cfm:Receiver:Start the connect procedure (probe,authenticate,associate)"));
        }

        SEND_PAL_DM_MLME_ACTIVITY_START_REQ(fsmData->handle);
        fsm_next_state(context, FSMSTATE_wait_for_mlme_access_connect);
    }
    else
    {
        PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);
        fsmData->needPhysLinkCompleteEvent = TRUE;
        if (PaldmStatus_NoSuitableChannelFound  == cfm->startStatus)
        {
            fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_CONNECTION_REJECTED_DUE_TO_NO_SUITABLE_CHANNEL_FOUND;
        }
        else if (PaldmStatus_StartFailed  == cfm->startStatus)
        {
            fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_CONNECTION_LIMIT_EXCEEDED;
        }
        else
        {
            fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_UNSPECIFIED_ERROR;
        }

        SEND_PAL_COEX_LINK_DISCONNECTED_REQ(fsmData->handle);
        fsm_next_state(context, FSMSTATE_disconnecting);

    }
}

static void starting__hci_disconnect_physical_link_cmd(FsmContext *context, const HciDisconnectPhysicalLinkCmd *cmd)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_SUCCESS, HCI_DISCONNECT_PHYSICAL_LINK_CODE);

    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);

    /* As per HCI Spec section 7.1.39:
     * "If the cancellation was successful, the Physical Link Complete event
     *  shall be generated with the error code Unknown Connection Identifier (0x02)."
     */
    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;

    /* HCI_Create_Physical_Link_Complete needs to go when disconnect procedure is completed */
    fsmData->needPhysLinkCompleteEvent = TRUE;
    fsmData->discRequested = TRUE;
}

static void starting__stop(FsmContext *context, const FsmEvent *req)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"starting__stop:"));

    if (HCI_RESET_CODE == req->id)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"starting__stop: HCI_RESET"));
        fsmData->needResetCompleteEvent = TRUE;
    }
    else
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"starting__stop: CORE_STOP_REQ"));
        fsmData->needCoreStopCfm = TRUE;
    }

    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);

    /* As per HCI Spec section 7.1.39:
     * "If the cancellation was successful, the Physical Link Complete event
     *  shall be generated with the error code Unknown Connection Identifier (0x02)."
     */
    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;

    /* HCI_Create_Physical_Link_Complete needs to go when disconnect procedure is completed */
    fsmData->needPhysLinkCompleteEvent = TRUE;
}

static void starting__pal_connection_accept_timeout_ind(FsmContext *context, const PalLinkTimer_Evt *timer)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_CONNECTION_TIMEOUT;
    fsmData->needPhysLinkCompleteEvent = TRUE;
}

static void wait_for_mlme_access_connect__pal_dm_mlme_activity_start_cfm(FsmContext *context, const PalDmMlmeActivityStartCfm_Evt *cfm)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    if (fsmData->discRequested || fsmData->needPhysLinkCompleteEvent)
    {
        SEND_PAL_COEX_LINK_DISCONNECTED_REQ(fsmData->handle);
        fsm_next_state(context, FSMSTATE_disconnecting);
    }
    else
    {
        if (PaldmStatus_Success == cfm->mlmeActivityStartStatus)
        {
            fsmData->hipFsmPid = pal_lm_create_hip_instance(context, fsmData->handle);
            SEND_PAL_LINK_CONNECT_REQ(fsmData->hipFsmPid, fsmData->role);
            fsm_next_state(context, FSMSTATE_connecting);
        }
        else
        {
            /* mlme access failed. It shouldn't happen*/
            sme_trace_error((TR_PAL_LM_LINK_FSM,"wait_for_mlme_access_connect__pal_dm_mlme_activity_start_cfm: Unrecoverable error"));
        }
    }
}

static void wait_for_mlme_access_connect__hci_disconnect_physical_link_cmd(FsmContext *context, const HciDisconnectPhysicalLinkCmd *cmd)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);

    /* HCI_Create_Physical_Link_Complete needs to go when disconnect procedure is completed */
    fsmData->needPhysLinkCompleteEvent = TRUE;
    fsmData->discRequested = TRUE;

    /* As per HCI Spec section 7.1.39:
     * "If the cancellation was successful, the Physical Link Complete event
     *  shall be generated with the error code Unknown Connection Identifier (0x02)."
     */
    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
    PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_SUCCESS,
                                HCI_DISCONNECT_PHYSICAL_LINK_CODE);
}

static void wait_for_mlme_access_connect__pal_connection_accept_timeout_ind(FsmContext *context, const PalLinkTimer_Evt *timer)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_CONNECTION_TIMEOUT;
    fsmData->needPhysLinkCompleteEvent = TRUE;
}

static void wait_for_mlme_access_connect__stop(FsmContext *context, const FsmEvent *req)
{
    physicalLinkAttrib *fsmData = FSMDATA_PHYLINK;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"wait_for_mlme_access_connect__stop:"));

    if (HCI_RESET_CODE == req->id)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"wait_for_mlme_access_connect__stop: HCI_RESET"));
        fsmData->needResetCompleteEvent = TRUE;
    }
    else
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"wait_for_mlme_access_connect__stop: CORE_STOP_REQ"));
        fsmData->needCoreStopCfm = TRUE;
    }

    PAL_STOP_TIMER(pal_get_connection_accept_timer(context),fsmData->timerId);

    /* HCI_Create_Physical_Link_Complete needs to go when disconnect procedure is completed */
    fsmData->needPhysLinkCompleteEvent = TRUE;

    /* As per HCI Spec section 7.1.39:
     * "If the cancellation was successful, the Physical Link Complete event
     *  shall be generated with the error code Unknown Connection Identifier (0x02)."
     */
    fsmData->phyLinkCompleteStatus = HCI_STATUS_CODE_UNKNOWN_CONNECTION_IDENTIFIER;

}

static void hci_write_remote_amp_assoc_cmd_reject(FsmContext *context, const HciWriteRemoteAmpAssocCmd *cmd)
{
   ReturnParameters retParams;

   retParams.commandCode = HCI_WRITE_REMOTE_AMP_ASSOC_CODE;
   retParams.hciReturnParam.writeRemoteAmpAssocReturn.status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
   retParams.hciReturnParam.writeRemoteAmpAssocReturn.physicalLinkHandle = cmd->physicalLinkHandle;

   PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
}

static void hci_create_logical_link_cmd_reject(FsmContext *context, const HciCreateLogicalLinkCmd *cmd)
{
   PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_COMMAND_DISALLOWED,
                               cmd->common.id);
}

static void hci_disconnect_logical_link_cmd_reject(FsmContext *context, const HciDisconnectLogicalLinkCmd *cmd)
{
   PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_COMMAND_DISALLOWED,
                               cmd->common.id);
}

static void hci_read_rssi_cmd_common_handler(FsmContext *context, const HciReadRssiCmd *cmd)
{
    ReturnParameters returnParams;

    returnParams.commandCode = HCI_READ_RSSI_CODE;
    returnParams.hciReturnParam.readRSSIReturn.status = HCI_STATUS_CODE_SUCCESS;
    returnParams.hciReturnParam.readRSSIReturn.rSSI=PAL_RSSI_INDICATION_UNAVAILABLE;
    PAL_SEND_HCI_COMMAND_COMPLETE(context, returnParams, TRUE);
}

static void hci_read_link_quality_cmd_common_handler(FsmContext *context, const HciReadLinkQualityCmd *cmd)
{
    ReturnParameters returnParams;

    returnParams.commandCode = HCI_READ_LINK_QUALITY_CODE;
    returnParams.hciReturnParam.readLinkQualityReturn.status = HCI_STATUS_CODE_SUCCESS;
    returnParams.hciReturnParam.readLinkQualityReturn.linkQuality=PAL_LINK_QUALITY_INDICATION_UNAVAILABLE;
    PAL_SEND_HCI_COMMAND_COMPLETE(context, returnParams, TRUE);
}

static void hci_create_physical_link_cmd_reject(FsmContext *context, const HciCreatePhysicalLinkCmd *cmd)
{
   PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_COMMAND_DISALLOWED,
                               cmd->common.id);
}

static void hci_accept_physical_link_cmd_reject(FsmContext *context, const HciAcceptPhysicalLinkRequestCmd *cmd)
{
   PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_COMMAND_DISALLOWED,
                               cmd->common.id);
}

static void hci_disconnect_physical_link_cmd_reject(FsmContext *context, const HciDisconnectPhysicalLinkCmd *cmd)
{
   PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_COMMAND_DISALLOWED,
                               cmd->common.id);
}

static void hci_logical_link_cancel_cmd_reject(FsmContext *context, const HciLogicalLinkCancelCmd *cmd)
{
   ReturnParameters retParams;

   retParams.commandCode = HCI_LOGICAL_LINK_CANCEL_CODE;
   retParams.hciReturnParam.logicalLinkCancelReturn.status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
   retParams.hciReturnParam.logicalLinkCancelReturn.physicalLinkHandle = cmd->physicalLinkHandle;
   retParams.hciReturnParam.logicalLinkCancelReturn.txFlowSpecId = cmd->txFlowSpecId;

   PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
}

static void hci_short_range_mode_cmd_reject(FsmContext *context, const HciShortRangeModeCmd *cmd)
{
    PAL_SEND_HCI_COMMAND_STATUS(context, HCI_STATUS_CODE_COMMAND_DISALLOWED, cmd->common.id);

}

static void hci_read_link_supervision_timeout_cmd_reject(FsmContext *context, const HciReadLinkSupervisionTimeoutCmd *cmd)
{
    ReturnParameters retParams;

    retParams.commandCode = HCI_READ_LINK_SUPERVISION_TIMEOUT_CODE;
    retParams.hciReturnParam.readTimeoutReturn.status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
    retParams.hciReturnParam.readTimeoutReturn.handle = cmd->handle;
    retParams.hciReturnParam.readTimeoutReturn.linkSupervisionTimeout = 0;
    PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
}

static void hci_write_link_supervision_timeout_cmd_reject(FsmContext *context, const HciWriteLinkSupervisionTimeoutCmd *cmd)
{
    ReturnParameters retParams;

    retParams.commandCode = HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CODE;
    retParams.hciReturnParam.writeTimeoutReturn.status = HCI_STATUS_CODE_COMMAND_DISALLOWED;
    retParams.hciReturnParam.writeTimeoutReturn.handle = cmd->handle;
    PAL_SEND_HCI_COMMAND_COMPLETE(context, retParams, TRUE);
}

static void unhandled_transitions(FsmContext *context, const FsmEvent *evt)
{
   sme_trace_warn((TR_PAL_LM_LINK_FSM,"Event %d not handled in this state %s, ",evt->id, fsm_current_state_name(context)));
}

#ifdef FSM_DEBUG_DUMP
static void lm_link_fsm_dump(FsmContext* context, const CsrUint16 id)
{

}
#endif

static void lm_link_fsm_reset(FsmContext* context)
{

}

/* FSM DEFINITION **********************************************/

/** State joining transitions */
static const FsmEventEntry disconnectedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(HCI_CREATE_PHYSICAL_LINK_CODE,             disconnected__hci_create_physical_link_cmd),
    fsm_event_table_entry(HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CODE,     disconnected__hci_accept_physical_link_cmd),
    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_CODE,         hci_disconnect_physical_link_cmd_reject),
};

/** State waitForAmpAssoc transitions */
static const FsmEventEntry waitForAmpAssocTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(HCI_WRITE_REMOTE_AMP_ASSOC_CODE,           wait_for_amp_assoc__hci_write_remote_amp_assoc_cmd),
    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_CODE,         wait_for_amp_assoc__hci_disconnect_physical_link_cmd),
    fsm_event_table_entry(PAL_LINK_TIMER_ID,                         wait_for_amp_assoc__pal_connection_accept_timeout_ind),

    fsm_event_table_entry(HCI_RESET_CODE,                            wait_for_amp_assoc__stop),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                          wait_for_amp_assoc__stop),
};

static const FsmEventEntry startingTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PAL_DM_START_CFM_ID,                   starting__pal_dm_start_cfm),
    fsm_event_table_entry(PAL_LINK_TIMER_ID,                     starting__pal_connection_accept_timeout_ind),
    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_CODE,     starting__hci_disconnect_physical_link_cmd),

    fsm_event_table_entry(HCI_RESET_CODE,                            starting__stop),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                          starting__stop),
};

static const FsmEventEntry waitForMlmeAccessConnectTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PAL_DM_MLME_ACTIVITY_START_CFM_ID,        wait_for_mlme_access_connect__pal_dm_mlme_activity_start_cfm),
    fsm_event_table_entry(PAL_LINK_TIMER_ID,                        wait_for_mlme_access_connect__pal_connection_accept_timeout_ind),
    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_CODE,        wait_for_mlme_access_connect__hci_disconnect_physical_link_cmd),

    fsm_event_table_entry(HCI_RESET_CODE,                           wait_for_mlme_access_connect__stop),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                         wait_for_mlme_access_connect__stop),
};

static const FsmEventEntry connectingTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PAL_LINK_CONNECT_CFM_ID,                   connecting__pal_link_connect_cfm),
    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_CODE,         connecting__hci_disconnect_physical_link_cmd),
    fsm_event_table_entry(PAL_LINK_TIMER_ID,                         connecting__pal_connection_accept_timeout_ind),

    fsm_event_table_entry(HCI_RESET_CODE,                            connecting__stop),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                          connecting__stop),
};

/** State connected transitions */
static const FsmEventEntry connectedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PAL_LINK_DISCONNECT_IND_ID,                connected__pal_link_disconnect_ind),
    fsm_event_table_entry(PAL_LINK_LINK_SUPERVISION_RESPONSE_IND_ID, connected__pal_link_link_supervision_response_ind),

    fsm_event_table_entry(PAL_CTRL_EARLY_LINK_LOSS_IND_ID,           connected__pal_ctrl_early_link_loss_ind),
    fsm_event_table_entry(PAL_CTRL_LINK_LOST_IND_ID,                 connected__pal_ctrl_link_lost_ind),

    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_CODE,         connected__hci_disconnect_physical_link_cmd),
    fsm_event_table_entry(HCI_READ_RSSI_CODE,                        connected__hci_read_rssi_cmd),
    fsm_event_table_entry(HCI_READ_LINK_QUALITY_CODE,                connected__hci_read_link_quality_cmd),

    fsm_event_table_entry(HCI_CREATE_LOGICAL_LINK_CODE,              connected__hci_create_logical_link_cmd),
    fsm_event_table_entry(HCI_ACCEPT_LOGICAL_LINK_CODE,              connected__hci_accept_logical_link_cmd),
    fsm_event_table_entry(HCI_DISCONNECT_LOGICAL_LINK_CODE,          connected__hci_disconnect_logical_link_cmd),
    fsm_event_table_entry(HCI_FLOW_SPEC_MODIFY_CODE,                 connected__hci_flow_spec_modify_cmd),
    fsm_event_table_entry(HCI_LOGICAL_LINK_CANCEL_CODE,              connected__hci_logical_link_cancel_cmd),
    fsm_event_table_entry(HCI_READ_BEST_EFFORT_FLUSH_TIMEOUT_CODE,   connected__hci_read_best_effort_flush_timeout_cmd),
    fsm_event_table_entry(HCI_WRITE_BEST_EFFORT_FLUSH_TIMEOUT_CODE,  connected__hci_write_best_effort_flush_timeout_cmd),
    fsm_event_table_entry(HCI_ENHANCED_FLUSH_CODE,                   connected__hci_enhanced_flush_cmd),
    fsm_event_table_entry(HCI_RESET_FAILED_CONTACT_COUNTER_CODE,     connected__hci_reset_failed_contact_counter_cmd),
    fsm_event_table_entry(HCI_READ_FAILED_CONTACT_COUNTER_CODE,      connected__hci_read_failed_contact_counter_cmd),
    fsm_event_table_entry(HCI_READ_LINK_SUPERVISION_TIMEOUT_CODE,    connected__hci_read_link_supervision_timeout_cmd),
    fsm_event_table_entry(HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CODE,   connected__hci_write_link_supervision_timeout_cmd),
    fsm_event_table_entry(HCI_SHORT_RANGE_MODE_CODE,                 connected__hci_short_range_mode_cmd),

    fsm_event_table_entry(HCI_RESET_CODE,                  connected__stop),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                  connected__stop),
};

static const FsmEventEntry disconnectingTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PAL_LINK_DISCONNECT_CFM_ID,                       disconnecting__pal_link_disconnect_cfm),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_IND_ID,                       disconnecting__pal_link_disconnect_ind),
    fsm_event_table_entry(HCI_DISCONNECT_PHYSICAL_LINK_CODE,                disconnecting__hci_disconnect_physical_link_cmd),
    fsm_event_table_entry(PAL_DM_STOP_CFM_ID,                               disconnecting__pal_dm_stop_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_CFM_ID,  disconnecting__pal_ctrl_link_supervision_timeout_delete_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_DELETE_CFM_ID,                      disconnecting__pal_ctrl_link_delete_cfm),
    fsm_event_table_entry(PAL_COEX_LINK_DISCONNECTED_CFM_ID,                disconnecting__pal_coex_link_disconnected_cfm),
    fsm_event_table_entry(PAL_CTRL_EARLY_LINK_LOSS_IND_ID,                  fsm_ignore_event),
    fsm_event_table_entry(PAL_CTRL_LINK_LOST_IND_ID,                        fsm_ignore_event),
    fsm_event_table_entry(PAL_LINK_LINK_SUPERVISION_RESPONSE_IND_ID,        fsm_ignore_event),

    fsm_event_table_entry(HCI_RESET_CODE,                            disconnecting__stop),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                          disconnecting__stop),
};

static const FsmEventEntry configuringDataManagerTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PAL_CTRL_LINK_CREATE_CFM_ID,           configuring_data_manager__pal_ctrl_link_create_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_MODIFY_CFM_ID,           configuring_data_manager__pal_ctrl_link_modify_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_DELETE_CFM_ID,           configuring_data_manager__pal_ctrl_link_delete_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_FLUSH_CFM_ID,            configuring_data_manager__pal_ctrl_link_flush_cfm),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_READ_CFM_ID,            configuring_data_manager__pal_ctrl_failed_contact_counter_read_cfm),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_RESET_CFM_ID,           configuring_data_manager__pal_ctrl_failed_contact_counter_reset_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_MODIFY_CFM_ID,        configuring_data_manager__pal_ctrl_link_supervision_modify_cfm),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_SET_CFM_ID,           configuring_data_manager__pal_ctrl_link_supervision_set_cfm),
    fsm_event_table_entry(HCI_SHORT_RANGE_MODE_CODE,                 fsm_saved_event),

    /* save this event and reject or process it from the next state*/
    fsm_event_table_entry(HCI_LOGICAL_LINK_CANCEL_CODE,              fsm_saved_event),
};


/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /* Signal Id,                                                 Function */
     /* DM SAP */
    fsm_event_table_entry(PAL_DM_MLME_ACTIVITY_START_CFM_ID,         unhandled_transitions),
    fsm_event_table_entry(PAL_DM_START_CFM_ID,                       unhandled_transitions),

    /* LINK SAP */
    fsm_event_table_entry(PAL_LINK_CONNECT_CFM_ID,                   unhandled_transitions),
    fsm_event_table_entry(PAL_LINK_AUTHENTICATE_CFM_ID,              unhandled_transitions),
    fsm_event_table_entry(PAL_LINK_DISCONNECT_CFM_ID,                unhandled_transitions),

    /* HCI SAP */
    fsm_event_table_entry(HCI_WRITE_REMOTE_AMP_ASSOC_CODE,           hci_write_remote_amp_assoc_cmd_reject),
    fsm_event_table_entry(HCI_CREATE_PHYSICAL_LINK_CODE,             hci_create_physical_link_cmd_reject),
    fsm_event_table_entry(HCI_ACCEPT_PHYSICAL_LINK_REQUEST_CODE,     hci_accept_physical_link_cmd_reject),
    fsm_event_table_entry(HCI_CREATE_LOGICAL_LINK_CODE,              hci_create_logical_link_cmd_reject),
    fsm_event_table_entry(HCI_DISCONNECT_LOGICAL_LINK_CODE,          hci_disconnect_logical_link_cmd_reject),
    fsm_event_table_entry(HCI_READ_RSSI_CODE,                        hci_read_rssi_cmd_common_handler),
    fsm_event_table_entry(HCI_READ_LINK_QUALITY_CODE,                hci_read_link_quality_cmd_common_handler),
    fsm_event_table_entry(HCI_LOGICAL_LINK_CANCEL_CODE,              hci_logical_link_cancel_cmd_reject),
    fsm_event_table_entry(HCI_SHORT_RANGE_MODE_CODE,                 hci_short_range_mode_cmd_reject),
    fsm_event_table_entry(HCI_READ_LINK_SUPERVISION_TIMEOUT_CODE,    hci_read_link_supervision_timeout_cmd_reject),
    fsm_event_table_entry(HCI_WRITE_LINK_SUPERVISION_TIMEOUT_CODE,   hci_write_link_supervision_timeout_cmd_reject),
};

/** Profile Storage state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
    /*                       State                   State                               Save     */
    /*                       Name                    Transitions                          *       */
    fsm_state_table_entry(FSMSTATE_disconnected, disconnectedTransitions,   FALSE),
    fsm_state_table_entry(FSMSTATE_wait_for_amp_assoc, waitForAmpAssocTransitions,   FALSE),
    fsm_state_table_entry(FSMSTATE_starting, startingTransitions,   FALSE),
    fsm_state_table_entry(FSMSTATE_wait_for_mlme_access_connect,  waitForMlmeAccessConnectTransitions, TRUE),
    fsm_state_table_entry(FSMSTATE_connecting,  connectingTransitions,  TRUE),
    fsm_state_table_entry(FSMSTATE_connected,   connectedTransitions,           FALSE),
    fsm_state_table_entry(FSMSTATE_disconnecting,   disconnectingTransitions,           FALSE),
    fsm_state_table_entry(FSMSTATE_configuring_data_manager,   configuringDataManagerTransitions,           TRUE),

};

const FsmProcessStateMachine pal_lm_link_fsm =
{
#ifdef FSM_DEBUG
       "PAL LM Link",                                  /* Process Name       */
#endif
       PAL_LM_LINK_PROCESS,                                /* Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                         /* Transition Tables  */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE),   /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                    /* ignore event handers */
       pal_lm_link_init,                                    /* Entry Function     */
       lm_link_fsm_reset,                                                               /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       lm_link_fsm_dump                                                             /* Trace Dump Function   */
#endif
};

/*
 * FSM Scripts Config for this FSM
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/** @}
 */
