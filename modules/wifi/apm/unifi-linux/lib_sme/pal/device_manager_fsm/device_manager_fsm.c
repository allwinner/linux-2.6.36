/** @file device_manager_fsm.c
 *
 * PAL Device Manager FSM
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
 *   This FSM is tasked with managing the unifi interactions that are common to
 * all physical links. It perform common operations like get/set mibs, scanning and
 * channel selection, start/stop network, reset unifi etc. The services are provided
 * primarily to Link Manager.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/device_manager_fsm/device_manager_fsm.c#4 $
 *
 ****************************************************************************/

/** @{
 * @ingroup device_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "device_manager_fsm/device_manager_fsm.h"
#include "link_manager_fsm/link_manager_fsm_types.h"
#include "link_manager_fsm/amp_assoc_endec.h"
#include "link_manager_fsm/hip_interface.h"
#include "hip_proxy_fsm/mib_utils.h"
#include "pal_hci_sap/pal_hci_sap_signals.h"
#include "sys_sap/sys_sap_from_sme_interface.h"
#include "pal_ctrl_sap/pal_ctrl_sap_from_sme_interface.h"
#include "payload_manager/payload_manager.h"

#include "hip_proxy_fsm/mib_encoding.h"
#include "regulatory_domain/regulatory_domain.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "coex_manager_fsm/coex_manager_fsm.h"

#include "hip_proxy_fsm/mib_action_sub_fsm.h"
#include "smeio/smeio_trace_types.h"

/* This is to access the channel order that SME uses. It is used during scanning and channel selection. */
extern const CsrUint8 channelScanOrderList[];

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, DmFsmData))

#define PAL_SEND_HCI_COMMAND_COMPLETE(context,returnParameters, pendingCmd) send_hci_command_complete_evt(context,getSmeContext(context)->palMgrFsmInstance,0,returnParameters)
#define PAL_SEND_HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE(context, status,phyLinkHandle, shortRangeModeState) send_hci_short_range_mode_change_complete_evt(context,getSmeContext(context)->palMgrFsmInstance, status,phyLinkHandle, shortRangeModeState)

#define PAL_DAM_DOT11_4B_POWER_LEVEL (2)

#define PAL_DM_NUM_UNITDATA_SUBSCRIPTIONS (4)
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
    /** DM initialised */
    FSMSTATE_stopped,

    /** DM ready for requests from LM */
    FSMSTATE_ready,

    /** waiting for scan results */
    FSMSTATE_scanning,

    /** waiting for mlme start */
    FSMSTATE_starting,

    /** Unifi beaconing */
    FSMSTATE_started,

    /** waiting for mlme reset cfm or del-wds-cfm during a stop*/
    FSMSTATE_stopping,

    /** LM instance busy with an mlme procedure (connect or disconnect)*/
    FSMSTATE_wait_for_lm_activity_complete,

    /** Last enum in the list */
    FSMSTATE_MAX_STATE
} FsmState;

/**
 * @brief
 *   structure to hold scan results
 *
 * @par Description
 *   structure contains parameters to store the scan results
 */
typedef struct PAL_DM_ScanResultChannelList
{
    CsrUint8                  number;
    CsrUint8                          count;
    CsrBool                        highPriority; /* set to TRUE for 1,6 & 11 channels */
    CsrInt16 snr;
    CsrInt16 rssi;
}PAL_DM_ScanResultChannelList;


/**
 * @brief
 *   DM Client information
 *
 * @par Description
 *   Information related to the DM clients.
 */
typedef struct PAL_DM_Client_Info
{
    CsrUint16 numClientsStarted; /* Number of clients started the network */
    CsrUint8 activeDmClientHandle; /* saves the handle of the client that requested the ongoing procedure*/
}PAL_DM_Client_Info;


/**
 * @brief
 *   Unitdata subscription information
 *
 * @par Description
 */
typedef struct PAL_DM_Unitdata_Subscription_Info
{
    CsrBool used;
    CsrUint8 subscriptionHandle; /* Number of clients started the network */
}PAL_DM_Unitdata_Subscription_Info;


/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct DmFsmData
{
    PAL_DM_Client_Info clientInfo;
    unifi_RadioIF ifType;
    PalChannellist requestedChannelList; /* subset of channels requested by client to scan */
    PalChannellist preferredChannelList; /* channel list populated after scanning in the preferred order */
    PAL_DM_ScanResultChannelList scanResults[HIGHEST_80211_b_g_CHANNEL_NUM]; /* scan results */
    /* Local AMP Assoc Info (only used while host reading the assoc data)*/
    assocDataInfo readAssocInfo;
    CsrUint32 pendingEventId; /* event id that is pending to determine the appropriate confirmation */

    /* Option to fix the channel at run time */
    CsrUint8 fixedChannel;
    ShortRangeMode shortRangeModeStatus; /* SHORT_RANGE_MODE_DISABLED by default */

    PAL_DM_Unitdata_Subscription_Info subscriptionInfo[PAL_DM_NUM_UNITDATA_SUBSCRIPTIONS];
    CsrInt32 pendingSubcriptions;
} DmFsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/


/**
 * @brief
 *   get the link quality
 *
 * @par Description
 *   Calculates the link quality from the link quality parameters in MIB.
 *
 * @param[in]    qual   : pointer to link quality MIB Parameters
 *
 * @return
 *   void
 */
static CsrUint8 pal_dm_get_link_quality(unifi_LinkQuality *qual)
{
    /* return SNR for the time being */
    return (CsrUint8)qual->unifiSnr;
}

/**
 * @brief
 *   callback function to handle mlme-set-cfm from sub-fsm routine
 *
 * @par Description
 *   callback function to handle confirm for mlme-set for setting the power level for short range mode enable/disable.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm   : pointer to the confirm message.
 *
 * @return
 *   void
 */
static void configuring_short_range_mode__mlme_set_cfm(FsmContext *context, const MlmeSetCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;

    ShortRangeMode newShortRangeModeStatus = (SHORT_RANGE_MODE_DISABLED==fsmData->shortRangeModeStatus)?SHORT_RANGE_MODE_ENABLED:SHORT_RANGE_MODE_DISABLED;

    if (MibStatus_Successful == cfm->status)
    {
        sme_trace_info((TR_PAL_DM_FSM,"wait_for_get_set_mib_rsp__mlme_set_cfm(): dot11CurrentTxPowerLevel set"));

        PAL_SEND_HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE(context,
                                                      HCI_STATUS_CODE_SUCCESS,
                                                      fsmData->clientInfo.activeDmClientHandle,
                                                      newShortRangeModeStatus);
        fsmData->shortRangeModeStatus = newShortRangeModeStatus;
    }
    else
    {
        PAL_SEND_HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE(context,
                                                      HCI_STATUS_CODE_UNSPECIFIED_ERROR,
                                                      fsmData->clientInfo.activeDmClientHandle,
                                                      newShortRangeModeStatus);
    }

    fsm_next_state(context, FSMSTATE_started);

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }
    verify(TR_PAL_DM_FSM,cfm->dummyDataRef2.dataLength==0);
}


/**
 * @brief
 *   callback function to handle mlme-get-cfm from sub-fsm routine
 *
 * @par Description
 *   callback function to handle confirm for mlme-get for rssi and link-quality hci commands.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm   : pointer to the confirm message.
 *
 * @return
 *   void
 */
static void reading_link_quality_measurements__mlme_get_cfm(FsmContext *context, const MlmeGetCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;
    PAL_MibData *mibData=pal_get_common_mib_data(context);
    ReturnParameters returnParams;
    CsrBool status;

    returnParams.commandCode = (HciOpcode)fsmData->pendingEventId;

    status=mib_util_decode_linkq(context, &cfm->mibAttributeValue, cfm->status, cfm->errorIndex, &mibData->qual);

    if (HCI_READ_RSSI_CODE==fsmData->pendingEventId)
    {
        returnParams.hciReturnParam.readRSSIReturn.handle = fsmData->clientInfo.activeDmClientHandle;
        if (status)
        {
            sme_trace_info((TR_PAL_DM_FSM,"reading_link_quality_measurements__mlme_get_cfm():RSSI Value read-%d, snr-%d",mibData->qual.unifiRssi,mibData->qual.unifiSnr));
            returnParams.hciReturnParam.readRSSIReturn.status = HCI_STATUS_CODE_SUCCESS;
            returnParams.hciReturnParam.readRSSIReturn.rSSI=(CsrUint8)mibData->qual.unifiRssi;
        }
        else
        {
            returnParams.hciReturnParam.readRSSIReturn.status = HCI_STATUS_CODE_UNSPECIFIED_ERROR;
        }
    }
    else
    {
        verify(TR_PAL_DM_FSM,fsmData->pendingEventId==HCI_READ_LINK_QUALITY_CODE);
        returnParams.hciReturnParam.readLinkQualityReturn.handle = fsmData->clientInfo.activeDmClientHandle;
        if (status)
        {
            sme_trace_info((TR_PAL_DM_FSM,"reading_link_quality_measurements__mlme_get_cfm():RSSI Value read-%d, snr-%d",mibData->qual.unifiRssi,mibData->qual.unifiSnr));
            returnParams.hciReturnParam.readLinkQualityReturn.status = HCI_STATUS_CODE_SUCCESS;
            returnParams.hciReturnParam.readLinkQualityReturn.linkQuality=pal_dm_get_link_quality(&mibData->qual);
        }
        else
        {
            returnParams.hciReturnParam.readLinkQualityReturn.status = HCI_STATUS_CODE_UNSPECIFIED_ERROR;
        }
    }
    PAL_SEND_HCI_COMMAND_COMPLETE(context, returnParams, TRUE);

    fsmData->pendingEventId=0;

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }
    verify(TR_PAL_DM_FSM,cfm->dummyDataRef2.dataLength==0);

    fsm_next_state(context, FSMSTATE_started);
}

/**
 * @brief
 *   callback function to handle mlme-set-cfm from sub-fsm routine
 *
 * @par Description
 *   callback function to handle confirm for mlme-set to enable and disable RTS/CTS.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm   : pointer to the confirm message.
 *
 * @return
 *   void
 */
static void configuring_rts_cts__mlme_set_cfm(FsmContext *context, const MlmeSetCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;
    PAL_MibData *mibData=pal_get_common_mib_data(context);

    if (MibStatus_Successful==cfm->status)
    {
        /* No response required if pending pending was to enable/disable RTS/CTS */
        if (PAL_DM_DISABLE_RTS_CTS_REQ_ID == fsmData->pendingEventId)
        {
            mibData->dot11RTSThreshold = PAL_MAX_PDU_SIZE+1;
        }
        else if (PAL_DM_ENABLE_RTS_CTS_REQ_ID == fsmData->pendingEventId)
        {
            mibData->dot11RTSThreshold = 0;
        }
    }

    fsmData->pendingEventId = 0;
    fsm_next_state(context, FSMSTATE_started);

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }
    verify(TR_PAL_DM_FSM,cfm->dummyDataRef2.dataLength==0);
}


static void pal_dm_init_channel_list(FsmContext* context)
{
    CsrUint8 i;
    DmFsmData *fsmData = FSMDATA;
    PAL_DM_ScanResultChannelList *channelListResult = fsmData->scanResults;

    /* Initialise the structure */
    for (i=0;i<HIGHEST_80211_b_g_CHANNEL_NUM;i++)
    {
        channelListResult[i].number=i+1;
        channelListResult[i].count=0;
        channelListResult[i].snr = 0;
        channelListResult[i].rssi = 0;

        /* Set the priority to high if channel numbers are 1,6 or 11.
        * This will put these channels on a higher priority than all other channels
        */
        if (1 == channelListResult[i].number ||
            6 == channelListResult[i].number ||
            11 == channelListResult[i].number)
        {
            channelListResult[i].highPriority=TRUE;
        }
        else
        {
            channelListResult[i].highPriority=FALSE;
        }
    }

    fsmData->preferredChannelList.numChannels=0;
    if (!fsmData->fixedChannel)
    {
        for (i=0 ; i < HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
        {
            /* FIXME:Add support for more channels when regulatory stuff is sorted
             * For the time being stick 1-11 channels
             */
            if (channelScanOrderList[i] <= PAL_MAX_CHANNEL_NUMBER)
            {
                fsmData->preferredChannelList.number[fsmData->preferredChannelList.numChannels] = channelScanOrderList[i];
                fsmData->preferredChannelList.numChannels++;
            }
        }
    }
    else
    {
        fsmData->preferredChannelList.number[0] = fsmData->fixedChannel;
        fsmData->preferredChannelList.numChannels=1;
    }

    CsrMemSet(&fsmData->requestedChannelList,0,sizeof(PalChannellist));
}

/**
 * @brief
 *   callback function to handle mlme-set-cfm.
 *
 * @par Description
 *   This function is called to process confirm for mlme-set during the start
 * procedure.
 *
 * @param[in]    context   : FSM context
 * @param[in]    palMibValues   : pointer to existing default MIB attributes
 *
 * @return
 *   void
 */
static void wait_for_mib_write__mlme_set_cfm(FsmContext *context, const MlmeSetCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;

    if (MibStatus_Successful==cfm->status)
    {
        unifi_MACAddress localMacAddress = *pal_get_local_mac_address(context);
        sme_trace_info((TR_PAL_DM_FSM,"mib defaults set successfully, start the mlme-start now"));

        /* do a reset to set the new changes in espcially the mib WMMEnabled.
        * SME interface doesn't support the setDefaultMib flag yet. If supported move this to
        * send_unifi_reset_req
        */
        send_mlme_reset_req(context,
                            localMacAddress,
                            FALSE);

        /* Wait for the confirm before mlme-start-req is sent to avoid any raise conditions */
        fsm_next_state(context, FSMSTATE_starting);
    }
    else
    {
        fsmData->clientInfo.numClientsStarted=0;
        sme_trace_warn((TR_PAL_DM_FSM,"Mlme-set-cfm failed"));

        pal_set_selected_channel_no(context, PAL_INVALID_CHANNEL_NUMBER);
        send_unifi_reset_req(context, getSmeContext(context)->unifiDriverInstance);

        fsm_next_state(context, FSMSTATE_stopping);
    }

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }
    verify(TR_PAL_DM_FSM,cfm->dummyDataRef2.dataLength==0);
}


/**
 * @brief
 *   set the default MIB values before starting an AMP network
 *
 * @par Description
 *   Funciton sets the default MIB values for AMP before an AMP network is started
 *
 * @param[in]    context   : FSM context
 * @param[in]    palMibValues   : pointer to existing default MIB attributes
 *
 * @return
 *   void
 */
void pal_dm_send_set_defaults(FsmContext* context,const PAL_MibData* palMibValues)
{
    DataReference mibSetDataRef = mib_encode_create_set(context, 10, 1);

    (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiCoexScheme, unifi_CoexSchemePTA, 0, 0);

    /* Set the retry count to a maximum for both short and long */
    /* This will help us to set a maximum flush timeout for logical links
     * Before a physical link is created, the dot11RTSThreshold is the default value (3000).
     * Hence the shortRetryLimit is used for retry attempts.
     * Once the link is created the RTS/CTS is enabled by default. henceforth LongRetryLimit
     * will be used for retry attempts. But if there is no SCO activity on the other side the RTS/CTS
     * needs to be disabled and retry attempts will start using shortRetryLimit again. So it is
     * important that both values are the same.
     */
    (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11LongRetryLimit, PAL_MAX_DOT11_RETRY_LIMIT, unifi_GHZ_2_4, 0);
    (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11ShortRetryLimit, PAL_MAX_DOT11_RETRY_LIMIT, unifi_GHZ_2_4, 0);
    (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiWMMEnabled, FALSE, unifi_GHZ_2_4, 0);
    (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiCSROnlyHighThroughputOptionEnabled, FALSE, unifi_GHZ_2_4, 0);

    if (FALSE==palMibValues->dot11ShortSlotTimeOptionEnabled)
    {
        (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11ShortSlotTimeOptionEnabled, TRUE, unifi_GHZ_2_4, 0);
    }

    if (pal_security_enabled(context))
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM, "pal_dm_send_set_defaults():Security enabled. set security MIBs"));
        (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11PrivacyInvoked, TRUE, unifi_GHZ_2_4, 0);
        (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11RSNAEnabled, TRUE, unifi_GHZ_2_4, 0);

        {
            (void)mib_encode_add_set_os(context, &mibSetDataRef, dot11RSNAConfigGroupCipher,palMibValues->cipherSuite, 4, unifi_GHZ_2_4, 0);
        }
    }

    mib_set_sub_fsm_start(context, *wait_for_mib_write__mlme_set_cfm, &mibSetDataRef, FALSE);
}


/**
 * @brief
 *   Start the AMP network
 *
 * @par Description
 *   Funciton implement the procedure to start the creation of AMP network. Start off with setting the
 * default MIB attributes that is relevant for AMP.
 *
 * @param[in]    context   : FSM context
 * @param[in]    channelNumber   : channel number on which the network needs to be started
 *
 * @return
 *   void
 */
static void start_amp_network(FsmContext *context, CsrUint8 channelNumber)
{
    DmFsmData *fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    if ((PAL_INVALID_CHANNEL_NUMBER == pal_get_selected_channel_no(context)||
        (pal_get_selected_channel_no(context)== channelNumber))) /* case where channel is already selected by SME */
    {
        PAL_MibData *mibData=pal_get_common_mib_data(context);
        mibData->dot11RTSThreshold = cfg->mibConfig.dot11RtsThreshold;
        mibData->dot11LongRetryLimit = cfg->dot11LongRetryLimit;
        mibData->dot11ShortRetryLimit = cfg->dot11ShortRetryLimit;
        mibData->dot11ShortSlotTimeOptionImplemented = cfg->dot11ShortSlotTimeOptionImplemented;
        mibData->dot11ShortSlotTimeOptionEnabled = cfg->dot11ShortSlotTimeOptionEnabled;

        pal_set_selected_channel_no(context, channelNumber);

        fsmData->pendingEventId = PAL_DM_START_REQ_ID;

         /* Pause Scan Manager */
         send_sm_pause_req(context, getSmeContext(context)->scanManagerInstance);

        /*  Tell COEX link is Connecting so that it can generate a channel avoidance message to BT */
        send_pal_coex_link_connecting_req(context,getSmeContext(context)->palCoexFsmInstance,channelNumber);

        /*We support short slot time. Otherwise PAL can enable it. Do it only if it is necessary */
        verify(TR_PAL_DM_FSM,mibData->dot11ShortSlotTimeOptionImplemented==TRUE);

        pal_dm_send_set_defaults(context, mibData);
    }
    else
    {
        sme_trace_error((TR_PAL_DM_FSM,"ready__pal_dm_start_req: invalid handle "));
    }
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
static void init_pal_dm_data(FsmContext* context, CsrBool reset)
{
    DmFsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_PAL_DM_FSM, "init_pal_dm_data()"));
    CsrMemSet(&fsmData->clientInfo,0,sizeof(PAL_DM_Client_Info));
    fsmData->fixedChannel = 0;
    pal_dm_init_channel_list(context);
    if (reset && fsmData->readAssocInfo.data)
    {
        CsrPfree(fsmData->readAssocInfo.data);
    }
    CsrMemSet(&fsmData->readAssocInfo,0,sizeof(assocDataInfo));
    fsmData->pendingEventId=0;
    pal_set_selected_channel_no(context, PAL_INVALID_CHANNEL_NUMBER);
    fsmData->shortRangeModeStatus = SHORT_RANGE_MODE_DISABLED;
    fsmData->pendingSubcriptions = 0;
}

/**
 * @brief
 *   Comparison function used to sort channels in the scan results
 *
 * @par Description
 *   Function implements the sorting logic for sorting the channels in the scan results. The function is supplied
 * to the qsort() as a parameter that is called for each channel in the scan results. There are 2 parameters
 * passed into the function. The function return a value (0 or 1) depending on which channel entry is better.
 *
 * @param[in]    a   : entry a in an array of scan results
 * @param[in]    b   : entry b in an array of scan results
 *
 * @return
 *   return 0 if a is higher priority
 *   return 1 if b is higher priority
 */
static CsrInt32 pal_dm_scan_result_compare(const void *a, const void *b)
{
    const PAL_DM_ScanResultChannelList *entrya = (PAL_DM_ScanResultChannelList *)a;
    const PAL_DM_ScanResultChannelList *entryb = (PAL_DM_ScanResultChannelList *)b;

    CsrInt32 ret=0;

    sme_trace_entry((TR_PAL_DM_FSM,"pal_dm_scan_result_compare entry a - channel-%d count-%d priority-%d",
                     entrya->number,entrya->count,entrya->highPriority));
    sme_trace_entry((TR_PAL_DM_FSM,"pal_dm_scan_result_compare entry b - channel-%d count-%d priority-%d",
                     entryb->number,entryb->count,entryb->highPriority));

    if (entrya->highPriority && entryb->highPriority)
    {
        /* Proceed as usual */
        sme_trace_entry((TR_PAL_DM_FSM,"pal_dm_scan_result_compare:  a&b are high priority channels"));
    }
    else if (entrya->highPriority)
    {
        sme_trace_entry((TR_PAL_DM_FSM, "pal_dm_scan_result_compare: a is high priority channel than b"));
        return 0;
    }
    else if (entryb->highPriority)
    {
        sme_trace_entry((TR_PAL_DM_FSM, "pal_dm_scan_result_compare: b is high priority channel than a"));
        return 1;
    }
 
    /* select the channel that is least used*/
    if (entryb->count < entrya->count)
    {
        sme_trace_entry((TR_PAL_DM_FSM, "pal_dm_scan_result_compare:"
                         "b is preferred over a because it is less used"));
        ret = 1;
    }
    else if (entrya->count < entryb->count)
    {
        sme_trace_entry((TR_PAL_DM_FSM, "pal_dm_scan_result_compare:"
                         "a is preferred over b because it is less used"));
        ret = 0;
    }
    /* If both channels appear to be in used in same numbers then
     * select the channel with the least average rssi. Not ideal but some information to be 
     * used to arrive at a conclusion
     */
    else if (entrya->rssi >= entryb->rssi)
    {
        sme_trace_entry((TR_PAL_DM_FSM, "pal_dm_scan_result_compare:"
                         "b is preferred over a because of weaker average rssi"));
        ret = 1;
    }
    return ret;
}

/**
 * @brief
 *   function to sort received scan results in the way PAL needs it
 *
 * @par Description
 *   Sorts the channel in descending order (1st entry is most preferred) based on the logic
 * implemented in pal_dm_scan_result_compare() function.
 *
 * @param[in/out]    scanResults   : scanResults as from SME. Sorted results is stored in the same memory
 * @param[in]    numEntries   : number of entries in the scan results.
 *
 * @return
 *   void
 */
static void pal_dm_sort_scan_results(PAL_DM_ScanResultChannelList *scanResults,CsrInt32 numEntries)
{
    CsrInt32 i,j,result;
    PAL_DM_ScanResultChannelList entry;

    sme_trace_entry((TR_PAL_DM_FSM, "pal_dm_sort_scan_results(): num entries - %d",numEntries));

    for (i=0; i<numEntries; i++)
    {
        for (j=i+1; j<numEntries; j++)
        {
            result = pal_dm_scan_result_compare(scanResults+i,scanResults+j);
            if (result)
            {
                entry = *(scanResults+i);
                *(scanResults+i) = *(scanResults+j);
                *(scanResults+j) = entry;
            }
        }
    }

#ifdef SME_TRACE_ENABLE
    sme_trace_info((TR_PAL_DM_FSM, "pal_dm_sort_scan_results(): Final result in order of preference"));

    for (i=0;i<HIGHEST_80211_b_g_CHANNEL_NUM;i++)
    {
        sme_trace_info((TR_PAL_DM_FSM, "pal_dm_sort_scan_results(): [%d]:Channel%d, Count-%d, Average(Rssi-%d,Snr-%d)",
                        i+1,scanResults[i].number,scanResults[i].count,
                        scanResults[i].rssi,scanResults[i].snr));
    }
#endif
}


 /**
     * @brief
     *   Selects the best channel from the preferred channel list
     *
     * @par Description
     *   The best channel is always the first channel in the preferred channel list
     *
     * @param[in]    fsmData   : pointer to DM fsm data
     * @param[out]    selectedChannel   : pointer to the selected channel determined by the function
     *
     * @return
     *   return unifi_Success if best channel is found
     *   return unifi_Error if channel is not found.
     */
static unifi_Status pal_select_best_channel(DmFsmData *fsmData, CsrUint8 *selectedChannel)
{
    unifi_Status status=unifi_Error;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_select_best_channel:"));

    if (fsmData->preferredChannelList.numChannels)
    {
        *selectedChannel = fsmData->preferredChannelList.number[0];
        sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_select_best_channel: selected channel is %d",*selectedChannel));

        status = unifi_Success;
    }
    return status;
}

 /**
  * @brief
  *   update and save the channel list requested by LM for channel selection
  *
  * @par Description
  *   The requested channel list is saved in the same priority order if the channel is acceptable
  * to DM. A channel is acceptable if the requested channel is present in the preferred channel list.
  *
  * @param[in]    context   : Fsm Context
  * @param[in]    requestedChannelList   : requested channel list
  *
  * @return
  *   void
  */
static void pal_dm_update_requested_channel_list(FsmContext *context, const PalChannellist *requestedChannelList)
{
    DmFsmData *fsmData = FSMDATA;
    CsrInt32 i,j;

    /* initialise the channel selection parameters*/
    pal_dm_init_channel_list(context);

    /* Accept the channel number only if the channel is in the preferred list */
    fsmData->requestedChannelList.numChannels=0;
    for (i=0; i<requestedChannelList->numChannels; i++)
    {
        for (j=0; j<fsmData->preferredChannelList.numChannels; j++)
        {
            if (fsmData->preferredChannelList.number[j] == requestedChannelList->number[i])
            {
                fsmData->requestedChannelList.number[fsmData->requestedChannelList.numChannels++]
                            = requestedChannelList->number[i];
            }
        }
    }
}


 /**
  * @brief
  *   populate local amp info
  *
  * @par Description
  *   Populates the local amp information
  *
  * @param[in]    context   : Fsm Context
  * @param[out]    info   : pointer to the structure to populate the info
  * @param[in]    ampStatus: amp status
  *
  * @return
  *   void
  */
static void populate_local_amp_info(FsmContext *context, HciReadLocalAmpInfoReturn *info, AmpStatus ampStatus)
{
    PAL_MibData *mibData=pal_get_common_mib_data(context);

    sme_trace_entry((TR_PAL_DM_FSM,"populate_local_amp_info: ampStatus-%d",ampStatus));

    info->ampAssocLength = PAL_AMP_ASSOC_MAX_TOTAL_LENGTH;
    info->controllerType = AMP_CONTROLLER_TYPE_802_11;
    info->status = HCI_STATUS_CODE_SUCCESS;
    info->totalBandwidth = PAL_TOTAL_BANDWIDTH*1024; /* value in kbps*/
    info->maxGuaranteedBandwidth = PAL_MAX_GUARANTEED_BANDWIDTH*1024; /* value in kbps*/

    info->maxPduSize = PAL_MAX_PDU_SIZE;

    if (pal_guranteed_link_supported(context))
    {
        sme_trace_entry((TR_PAL_DM_FSM,"populate_local_amp_info: Guranteed Links supported "));
        info->palCapabilities = PAL_CAPABILITIES;
        info->palCapabilities |= PAL_CAPABILITY_GURANTEED_LINK_SUPPORTED;
    }
    else
    {
        sme_trace_entry((TR_PAL_DM_FSM,"populate_local_amp_info: Guranteed Links NOT supported "));
        info->palCapabilities = PAL_CAPABILITIES;
    }
    if (fsm_current_state(context) != FSMSTATE_stopped)
    {
        info->minLatency = pal_hip_get_min_latency(mibData->dot11HCCWmin);
        info->maxFlushTimeout = pal_hip_get_max_flush_timeout(mibData->dot11HCCWmin);
    }
    else
    {
        /* Fill in default values as MIB is not read yet. */
        info->minLatency = PAL_MIN_LATENCY_DEFAULT;
        info->maxFlushTimeout = PAL_MAX_FLUSH_TIMEOUT_DEFAULT;
    }
    info->bestEffortFlushTimeout = info->maxFlushTimeout;
    info->ampStatus = ampStatus;
}

/**
 * @brief
 *   populate and send local amp info response
 *
 * @par Description
 *   Populates the local amp information and sends it to the host
 *
 * @param[in]    context   : Fsm Context
 * @param[in]    ampStatus: amp status
 *
 * @return
 *   void
 */
static void send_hci_read_local_amp_info_response(FsmContext *context)
{
    ReturnParameters params;
    HciReadLocalAmpInfoReturn *info = &params.hciReturnParam.readLocalAmpInfoReturn;

    params.commandCode = HCI_READ_LOCAL_AMP_INFO_CODE;
    populate_local_amp_info(context, info, getPalMgrContext(context)->ampStatus);
    PAL_SEND_HCI_COMMAND_COMPLETE(context, params, TRUE);
}


/**
 * @brief
 *   ip_connection_ Process FSM Entry Function
 *
 * @par Description
 *   Called on Security Manager Process startup to initialise
 *   the security manager data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void pal_dm_init(FsmContext* context)
{
    sme_trace_entry((TR_PAL_DM_FSM, "pal_dm_init()"));

    fsm_create_params(context, DmFsmData);
    init_pal_dm_data(context, FALSE);
    fsm_next_state(context, FSMSTATE_stopped);
}

/* PUBLIC FUNCTION DEFINITIONS *********************************************/

/* FSM DEFINITION **********************************************/


/******************** TRANSITION FUNCTIONS ***********************************************************/

static void stopped__core_start_req(FsmContext* context, const CoreStartReq_Evt* req)
{
    DmFsmData *fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);

    pal_set_local_mac_address(context, &cfg->stationMacAddress);
    fsmData->ifType = unifi_GHZ_2_4;

    send_core_start_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_ready);
}

static void ready_stop(FsmContext* context, const CoreStopReq_Evt* req)
{
    send_core_stop_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_stopped);
}

static void started_stop(FsmContext* context, const CoreStopReq_Evt* req)
{
    send_core_stop_cfm(context, req->common.sender_, unifi_Success);

    /* FIXME: Shutdown the connection */
    /*fsm_next_state(context, FSMSTATE_stopped); ??? */

}

static void scanning__scan_cfm(FsmContext *context, const SmScanCfm_Evt * cfm)
{
    DmFsmData *fsmData = FSMDATA;
    srs_scan_data *currentScan;
    CsrInt32 i,j;
    CsrUint8 selectedChannel;
    CsrBool result=FALSE;

    sme_trace_info((TR_PAL_DM_FSM, "scanning__scan_cfm(): result-%d",cfm->scanResult));

    if (ResultCode_Success == cfm->scanResult)
    {
        /* Loop through All the scan results and build the list */
        currentScan = srs_get_scan_parameters_first(context, NULL, &BroadcastBssid);
        while (currentScan)
        {
            sme_trace_info((TR_PAL_DM_FSM, "scanning__scan_cfm(): channel %d",currentScan->scanResult.channelNumber));

            if (currentScan->scanResult.channelNumber <= HIGHEST_80211_b_g_CHANNEL_NUM)
            {
                 fsmData->scanResults[currentScan->scanResult.channelNumber-1].count++;
    
                 if (0==fsmData->scanResults[currentScan->scanResult.channelNumber-1].rssi)
                 {
                     fsmData->scanResults[currentScan->scanResult.channelNumber-1].rssi = currentScan->scanResult.rssi;
                 }
                 else
                 {
                     fsmData->scanResults[currentScan->scanResult.channelNumber-1].rssi = (currentScan->scanResult.rssi+fsmData->scanResults[currentScan->scanResult.channelNumber-1].rssi)/2;
                 }

                 if (0==fsmData->scanResults[currentScan->scanResult.channelNumber-1].snr)
                 {
                     fsmData->scanResults[currentScan->scanResult.channelNumber-1].snr = currentScan->scanResult.snr;
                 }
                 else
                 {
                     fsmData->scanResults[currentScan->scanResult.channelNumber-1].snr = (currentScan->scanResult.snr+fsmData->scanResults[currentScan->scanResult.channelNumber-1].snr)/2;
                 }

                 sme_trace_info((TR_PAL_DM_FSM, "scanning__scan_cfm(): channel %d:count-%d,this(Rssi-%d,Snr-%d), avg(Rssi-%d,Snr-%d)",
                                 currentScan->scanResult.channelNumber,
                                 fsmData->scanResults[currentScan->scanResult.channelNumber-1].count,
                                 currentScan->scanResult.rssi,currentScan->scanResult.snr,
                                 fsmData->scanResults[currentScan->scanResult.channelNumber-1].rssi,
                                 fsmData->scanResults[currentScan->scanResult.channelNumber-1].snr));

            }
            currentScan = srs_get_scan_parameters_next(context, &currentScan->scanResult.bssid,
                                                       NULL,
                                                       &BroadcastBssid);
        }

        /* now sort the channel numbers in descending order for quality - more count means poor quality */
        pal_dm_sort_scan_results(fsmData->scanResults, HIGHEST_80211_b_g_CHANNEL_NUM);

        sme_trace_info((TR_PAL_DM_FSM, "scanning__scan_cfm(): sorting complete"));

        verify(TR_PAL_DM_FSM,fsmData->requestedChannelList.numChannels<=HIGHEST_80211_b_g_CHANNEL_NUM);
        fsmData->preferredChannelList.numChannels = 0;
        /* Accept the channel only if its in the channel list subset requested by client*/
        for (i=0;i<HIGHEST_80211_b_g_CHANNEL_NUM;i++)
        {
            for (j=0;j<fsmData->requestedChannelList.numChannels;j++)
            {
                if (fsmData->requestedChannelList.number[j] == fsmData->scanResults[i].number)
                {
                    sme_trace_info((TR_PAL_DM_FSM,"scanning__scan_cfm(): channel-%d,pos-%d,rssi-%d,snr-%d",
                                    fsmData->scanResults[i].number,fsmData->preferredChannelList.numChannels,
                                    fsmData->scanResults[i].rssi,fsmData->scanResults[i].snr));
                    fsmData->preferredChannelList.number[fsmData->preferredChannelList.numChannels] = fsmData->scanResults[i].number;
                    fsmData->preferredChannelList.numChannels++;
                    break;
                }
            }
        }

        if (unifi_Success == pal_select_best_channel(fsmData,&selectedChannel))
        {
            start_amp_network(context,selectedChannel);
            result=TRUE;
        }
    }

    if (FALSE == result)
    {
        send_pal_dm_start_cfm(context,
                              pal_get_link_fsm_pid_from_handle(context,fsmData->clientInfo.activeDmClientHandle),
                              PaldmStatus_NoSuitableChannelFound,
                              fsmData->clientInfo.activeDmClientHandle);
        fsm_next_state(context, FSMSTATE_ready);
    }
}

static void start_scanning(FsmContext *context, const PalChannellist *channelList)
{
    CsrUint32 crntTime = fsm_get_time_of_day_ms(context);

    sme_trace_info((TR_PAL_DM_FSM,"start_scanning:scanning started: num channels-%d",channelList->numChannels));
    /* cancel any other Scans as this takes precedence */
    send_sm_scan_cancel_ind(context, getSmeContext(context)->scanManagerInstance, crntTime, TRUE);

    /* FIXME: Scan only the channels requested. API needs upgrading*/
    send_sm_scan_req(context,
                     getSmeContext(context)->scanManagerInstance,
                     crntTime,
                     0, /* channel 0 = All channels */
                     0,
                     NULL,
                     BroadcastBssid,
                     FALSE,
                     FALSE,
                     FALSE,
                     unifi_ScanStopAllResults,
                     BssType_AnyBss,
                     0);
    fsm_next_state(context, FSMSTATE_scanning);
}

static void ready__pal_dm_start_req(FsmContext *context, const PalDmStartReq_Evt *req)
{
    DmFsmData *fsmData = FSMDATA;

    pal_dm_update_requested_channel_list(context,&req->channelList);

    /* proceed only if the channelList has got some acceptable channels */
    if (fsmData->requestedChannelList.numChannels)
    {
        fsmData->clientInfo.activeDmClientHandle = req->handle;
        if (req->scanningNotNeeded)
        {
            start_amp_network(context,fsmData->requestedChannelList.number[0]);
        }
        else
        {
            start_scanning(context, &fsmData->requestedChannelList);
        }
    }
    else
    {
        send_pal_dm_start_cfm(context,
                              req->common.sender_,
                              PaldmStatus_NoSuitableChannelFound,
                              req->handle);
    }
}

static void ready__pal_dm_stop_req(FsmContext *context, const PalDmStopReq_Evt *req)
{
    send_pal_dm_stop_cfm(context,
                         req->common.sender_,
                         unifi_Success);
}

static void ready__hci_reset_cmd(FsmContext *context, const HciResetCmd *cmd)
{
    ReturnParameters returnParams;

    init_pal_dm_data(context,TRUE);

    returnParams.commandCode = HCI_RESET_CODE;
    returnParams.hciReturnParam.resetStatus = HCI_STATUS_CODE_SUCCESS;

    PAL_SEND_HCI_COMMAND_COMPLETE(context, returnParams, TRUE);
}

static void started__pal_dm_stop_req(FsmContext *context, const PalDmStopReq_Evt *req)
{
    DmFsmData *fsmData = FSMDATA;
    unifi_MACAddress remoteMacAddress = *pal_get_remote_mac_address(context, req->handle);

    sme_trace_entry((TR_PAL_DM_FSM,"started__pal_dm_stop_req: %d clients already started. New client with handle %d requesting to stop ",fsmData->clientInfo.numClientsStarted,req->handle));
    verify(TR_PAL_DM_FSM,fsmData->clientInfo.numClientsStarted>0);

    fsmData->clientInfo.activeDmClientHandle = req->handle;


    /* save the request id if this is the last client. This is used when unifi-reset-cfm is processed to determine if reset
    was triggered due to a stop or start failure
    */
    if (1 == fsmData->clientInfo.numClientsStarted)
    {
        fsmData->pendingEventId = PAL_DM_STOP_REQ_ID;
    }
    send_mlme_del_wds_req(context, remoteMacAddress);
    call_unifi_sys_port_configure_req(context,unifi_8021xPortClosedDiscard,unifi_8021xPortClosedDiscard,&remoteMacAddress);
    fsm_next_state(context, FSMSTATE_stopping);
}

static void started__pal_dm_mlme_activity_start_req(FsmContext *context, const PalDmMlmeActivityStartReq_Evt *req)
{
    DmFsmData *fsmData = FSMDATA;

    fsmData->clientInfo.activeDmClientHandle = req->handle;
    send_pal_dm_mlme_activity_start_cfm(context,
                                        pal_get_link_fsm_pid_from_handle(context,req->handle),
                                        PaldmStatus_Success, req->handle);
    fsm_next_state(context, FSMSTATE_wait_for_lm_activity_complete);
}

static void started__pal_dm_start_req(FsmContext *context, const PalDmStartReq_Evt *req)
{
    DmFsmData *fsmData = FSMDATA;
    CsrInt32 i;
    CsrBool validChannelList=FALSE;

    sme_trace_entry((TR_PAL_DM_FSM,"started__pal_dm_start_req: %d clients already started. New client with handle %d requesting to start ",fsmData->clientInfo.numClientsStarted,req->handle));

    /* verify if the selected channel is in the requested channel list. If not, we cannot serve the
     * request
     */
    for (i=0; i<req->channelList.numChannels; i++)
    {
        if (pal_get_selected_channel_no(context) == req->channelList.number[i])
        {
            sme_trace_info((TR_PAL_DM_FSM,"started__pal_dm_start_req: selected channel %d is in the requested list",req->channelList.number[i]));
            validChannelList = TRUE;
            break;
        }
    }

    if (validChannelList)
    {
        unifi_MACAddress remoteMacAddress = *pal_get_remote_mac_address(context, req->handle);

        fsmData->clientInfo.activeDmClientHandle = req->handle;
        send_mlme_add_wds_req(context, remoteMacAddress);
        call_unifi_sys_port_configure_req(context,unifi_8021xPortOpen,unifi_8021xPortOpen,&remoteMacAddress);
        fsm_next_state(context, FSMSTATE_starting);
    }
    else
    {
        sme_trace_warn((TR_PAL_DM_FSM,"started__pal_dm_start_req: selected channel %d is not found in requested channel list",pal_get_selected_channel_no(context)));
        send_pal_dm_start_cfm(context,
                              req->common.sender_,
                              PaldmStatus_NoSuitableChannelFound,
                              req->handle);
    }
}

static void started__pal_dm_disable_rts_cts_req(FsmContext *context, const PalDmDisableRtsCtsReq_Evt *req)
{
    DmFsmData *fsmData = FSMDATA;

    PAL_MibData *mibData=pal_get_common_mib_data(context);

    /* FIXME: If there is atleast one link that needs RTS/CTS enabled , we need to keep it
     * as enabled. Not doing that check for now.
     */
    sme_trace_info((TR_PAL_DM_FSM,"dot11RTSThreshold-%d",mibData->dot11RTSThreshold));
    if (mibData->dot11RTSThreshold<PAL_MAX_PDU_SIZE)
    {
        DataReference mibSetDataRef = mib_encode_create_set(context, 1, 0);
        (void)mib_encode_add_set_int(context, &mibSetDataRef,
                                     dot11RTSThreshold,
                                     3000, unifi_GHZ_2_4, 0);


        fsmData->pendingEventId = PAL_DM_DISABLE_RTS_CTS_REQ_ID;
        mib_set_sub_fsm_start(context, *configuring_rts_cts__mlme_set_cfm, &mibSetDataRef, FALSE);
    }
}

static void started__pal_dm_enable_rts_cts_req(FsmContext *context, const PalDmEnableRtsCtsReq_Evt *req)
{
    DmFsmData *fsmData = FSMDATA;

    PAL_MibData *mibData=pal_get_common_mib_data(context);

    /* Enable RTS/CTS even if this the only link that needs RTS/CTS enabled.*/
    sme_trace_info((TR_PAL_DM_FSM,"dot11RTSThreshold-%d",mibData->dot11RTSThreshold));
    if (mibData->dot11RTSThreshold!=0)
    {
        DataReference mibSetDataRef = mib_encode_create_set(context, 1, 0);
        (void)mib_encode_add_set_int(context, &mibSetDataRef,
                                     dot11RTSThreshold,
                                     0, unifi_GHZ_2_4, 0);


        fsmData->pendingEventId = PAL_DM_ENABLE_RTS_CTS_REQ_ID;
        mib_set_sub_fsm_start(context, *configuring_rts_cts__mlme_set_cfm, &mibSetDataRef, FALSE);
    }
}

static void started__hci_read_rssi_cmd(FsmContext *context, const HciReadRssiCmd *cmd)
{
    DmFsmData *fsmData = FSMDATA;
    DataReference mibGetDataRef;

    fsmData->clientInfo.activeDmClientHandle = (CsrUint8)cmd->handle; /* save the phy link handle for now so that it can be used in hci response*/
    fsmData->pendingEventId = HCI_READ_RSSI_CODE;
    mib_util_encode_stats_req(context, &mibGetDataRef, unifi_GHZ_2_4);
    mib_get_sub_fsm_start(context, *reading_link_quality_measurements__mlme_get_cfm, &mibGetDataRef, FALSE);
}

static void started__hci_read_link_quality_cmd(FsmContext *context, const HciReadLinkQualityCmd *cmd)
{
    DmFsmData *fsmData = FSMDATA;
    DataReference mibGetDataRef;

    fsmData->clientInfo.activeDmClientHandle = (CsrUint8)cmd->handle; /* save the phy link handle for now so that it can be used in hci response*/
    fsmData->pendingEventId = HCI_READ_LINK_QUALITY_CODE;
    mib_util_encode_stats_req(context, &mibGetDataRef, unifi_GHZ_2_4);
    mib_get_sub_fsm_start(context, *reading_link_quality_measurements__mlme_get_cfm, &mibGetDataRef, FALSE);
}

static void started__hci_short_range_mode_cmd(FsmContext *context, const HciShortRangeModeCmd *cmd)
{
    DmFsmData *fsmData = FSMDATA;

    if (fsmData->shortRangeModeStatus != cmd->shortRangeMode)
    {
        SmeConfigData* cfg = get_sme_config(context);
        CsrUint16 newDot11CurrentTxPowerLevel = (SHORT_RANGE_MODE_ENABLED==cmd->shortRangeMode)?PAL_DAM_DOT11_4B_POWER_LEVEL:cfg->mibConfig.dot11CurrentTxPowerLevel;
        DataReference mibSetDataRef = mib_encode_create_set(context, 1, 0);
        (void)mib_encode_add_set_int(context, &mibSetDataRef, dot11CurrentTxPowerLevel, (CsrInt32)newDot11CurrentTxPowerLevel, 0, 0);

        fsmData->clientInfo.activeDmClientHandle = cmd->physicalLinkHandle; /* save the phy link handle for now so that it can be used in hci response*/

         mib_set_sub_fsm_start(context, *configuring_short_range_mode__mlme_set_cfm, &mibSetDataRef, FALSE);
    }
    else
    {
        PAL_SEND_HCI_SHORT_RANGE_MODE_CHANGE_COMPLETE(context,
                                                      HCI_STATUS_CODE_SUCCESS,
                                                      cmd->physicalLinkHandle,
                                                      cmd->shortRangeMode);
    }
}

static void wait_for_lm_activity_complete__pal_dm_mlme_activity_complete_req(FsmContext *context, const PalDmMlmeActivityCompleteReq_Evt *req)
{
    DmFsmData *fsmData = FSMDATA;

    if (req->replyRequired)
    {
        send_pal_dm_mlme_activity_complete_cfm(context,
                                            pal_get_link_fsm_pid_from_handle(context,req->handle),
                                            unifi_Success, req->handle);
    }
    fsmData->clientInfo.activeDmClientHandle=PAL_INVALID_PHYSICAL_LINK_HANDLE;
    fsm_next_state(context, FSMSTATE_started);
}

static void stopping__unifi_reset_cfm(FsmContext *context, const UnifiResetCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_DM_FSM,"stopping__unifi_reset_cfm:"));
    if (PAL_DM_STOP_REQ_ID == fsmData->pendingEventId)
    {
        sme_trace_info((TR_PAL_DM_FSM,"stopping__unifi_reset_cfm:sending dm-stop-cfm"));
        send_pal_dm_stop_cfm(context,
                              pal_get_link_fsm_pid_from_handle(context,fsmData->clientInfo.activeDmClientHandle),
                              PaldmStatus_Success);

    }
    else
    {
        verify(TR_PAL_DM_FSM,PAL_DM_START_REQ_ID == fsmData->pendingEventId);
        sme_trace_info((TR_PAL_DM_FSM,"stopping__unifi_reset_cfm:sending dm-star-cfm failure"));
        send_pal_dm_start_cfm(context,
                              pal_get_link_fsm_pid_from_handle(context,fsmData->clientInfo.activeDmClientHandle),
                              PaldmStatus_StartFailed,
                              fsmData->clientInfo.activeDmClientHandle);
    }
    fsmData->pendingEventId=0;
    fsmData->clientInfo.activeDmClientHandle = PAL_INVALID_PHYSICAL_LINK_HANDLE;

    /* unpause Scan Manager */
    send_sm_unpause_req(context, getSmeContext(context)->scanManagerInstance);

    fsm_next_state(context, FSMSTATE_ready);
}

static void stopping__mlme_del_wds_cfm(FsmContext *context, const MlmeDelWdsCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_DM_FSM,"stopping__mlme_del_wds_cfm: result-%d",cfm->resultCode));
    fsmData->clientInfo.numClientsStarted--;
    if (!fsmData->clientInfo.numClientsStarted)
    {
        sme_trace_info((TR_PAL_DM_FSM,"stopping__mlme_del_wds_cfm: Last client stopped. so trigger reset"));

        /* send mediaDisconnect if last client is disconnected */
        call_unifi_sys_media_status_req(context, unifi_MediaDisconnected, unifi_MediaTypeAmp);

        pal_set_selected_channel_no(context, PAL_INVALID_CHANNEL_NUMBER);
        pal_dm_init_channel_list(context);

        /* continue with reset only if result of this operation was success */
        if (ResultCode_Success == cfm->resultCode)
        {
            /* unsubscribe the ma-unitdata-inds first */
            CsrInt32 i;
            for (i=0; i<PAL_DM_NUM_UNITDATA_SUBSCRIPTIONS; i++)
            {
                verify(TR_PAL_DM_FSM, fsmData->subscriptionInfo[i].used == TRUE);
                call_unifi_sys_ma_unitdata_unsubscribe_req(context,PAL_SYS_APP_HANDLE(context),fsmData->subscriptionInfo[i].subscriptionHandle);
            }

            fsmData->pendingSubcriptions = 0;
            CsrMemSet(fsmData->subscriptionInfo, 0, PAL_DM_NUM_UNITDATA_SUBSCRIPTIONS);
            send_unifi_reset_req(context, getSmeContext(context)->unifiDriverInstance);
        }
        else
        {
            sme_trace_warn((TR_PAL_DM_FSM,"stopping__mlme_del_wds_cfm: failed"));
            fsmData->pendingEventId=0;
            /* unpause Scan Manager */
            send_sm_unpause_req(context, getSmeContext(context)->scanManagerInstance);
            send_pal_dm_stop_cfm(context,
                                 pal_get_link_fsm_pid_from_handle(context,fsmData->clientInfo.activeDmClientHandle),
                                 PaldmStatus_UnspecifiedError);
            fsmData->clientInfo.activeDmClientHandle = PAL_INVALID_PHYSICAL_LINK_HANDLE;
            fsm_next_state(context, FSMSTATE_ready);
        }
    }
    else
    {
        sme_trace_info((TR_PAL_DM_FSM,"stopping__mlme_del_wds_cfm: %d clients remain started.",fsmData->clientInfo.numClientsStarted));
        send_pal_dm_stop_cfm(context,
                             pal_get_link_fsm_pid_from_handle(context,fsmData->clientInfo.activeDmClientHandle),
                             PaldmStatus_Success);
        fsm_next_state(context, FSMSTATE_started);
    }
}

static void starting__mlme_reset_cfm(FsmContext *context, const MlmeResetCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;

    sme_trace_entry((TR_PAL_DM_FSM,"starting__mlme_reset_cfm: status-%d",cfm->resultCode));

    if (ResultCode_Success == cfm->resultCode)
    {
        pal_hip_send_mlme_start_req(context,
                                    fsmData->ifType,
                                    pal_get_selected_channel_no(context));
    }
    else
    {
        fsmData->clientInfo.numClientsStarted=0;
        sme_trace_warn((TR_PAL_DM_FSM,"starting__mlme_reset_cfm: reset failed"));

        pal_set_selected_channel_no(context, PAL_INVALID_CHANNEL_NUMBER);
        send_unifi_reset_req(context, getSmeContext(context)->unifiDriverInstance);

        fsm_next_state(context, FSMSTATE_stopping);
    }
    verify(TR_PAL_DM_FSM,cfm->dummyDataRef2.dataLength==0 && cfm->dummyDataRef1.dataLength==0);
}

static void starting__mlme_start_cfm(FsmContext *context, const MlmeStartCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;

    if (ResultCode_Success == cfm->resultCode)
    {
        unifi_MACAddress remoteMacAddress = *pal_get_remote_mac_address(context, fsmData->clientInfo.activeDmClientHandle);
        send_mlme_add_wds_req(context, remoteMacAddress);
        call_unifi_sys_port_configure_req(context,unifi_8021xPortOpen,unifi_8021xPortOpen,&remoteMacAddress);
    }
    else
    {
        fsmData->clientInfo.numClientsStarted=0;
        sme_trace_warn((TR_PAL_DM_FSM,"starting__mlme_start_cfm: start failed"));

        pal_set_selected_channel_no(context, PAL_INVALID_CHANNEL_NUMBER);
        send_unifi_reset_req(context, getSmeContext(context)->unifiDriverInstance);

        fsm_next_state(context, FSMSTATE_stopping);
    }
    verify(TR_PAL_DM_FSM,cfm->dummyDataRef2.dataLength==0 && cfm->dummyDataRef1.dataLength==0);
}


static void fsm_ignore_subscription_cfm_event(FsmContext *context, const UnifiSysMaUnitdataSubscribeCfm_Evt *cfm)
{
    /* simply unsubscribe in case we have left the state to handle all subscriptions */
    if (unifi_SubscriptionResultSuccess == cfm->status)
    {
        call_unifi_sys_ma_unitdata_unsubscribe_req(context,PAL_SYS_APP_HANDLE(context),cfm->subscriptionHandle);
    }
}

static void starting__unifi_sys_ma_unitdata_subscribe_cfm(FsmContext *context, const UnifiSysMaUnitdataSubscribeCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;

    sme_trace_info((TR_PAL_DM_FSM,"starting__unifi_sys_ma_unitdata_subscribe_cfm: status-%d,handle-%d, pendingSubscriptions-%d",cfm->status,cfm->subscriptionHandle,fsmData->pendingSubcriptions));

    if (unifi_SubscriptionResultSuccess == cfm->status)
    {
        fsmData->subscriptionInfo[fsmData->pendingSubcriptions].subscriptionHandle = cfm->subscriptionHandle;
        fsmData->subscriptionInfo[fsmData->pendingSubcriptions].used = TRUE;
        fsmData->pendingSubcriptions++;

        /* complete the procedure if all subscriptions are confirmed */
        if (PAL_DM_NUM_UNITDATA_SUBSCRIPTIONS == fsmData->pendingSubcriptions)
        {
            sme_trace_info((TR_PAL_DM_FSM,"starting__unifi_sys_ma_unitdata_subscribe_cfm: start success"));

            /* send mediaConnect is sent on first connection. subscription happens only for the first connection */
            call_unifi_sys_media_status_req(context, unifi_MediaConnected, unifi_MediaTypeAmp);

            fsmData->clientInfo.numClientsStarted++;
            fsmData->preferredChannelList.numChannels=1;
            fsmData->preferredChannelList.number[0] = pal_get_selected_channel_no(context);
            send_pal_dm_start_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context,fsmData->clientInfo.activeDmClientHandle),
                                  PaldmStatus_Success,
                                  fsmData->clientInfo.activeDmClientHandle);
            fsm_next_state(context, FSMSTATE_started);
        }
    }
    else
    {
        /* should we check if there any subscriptions succeeded.. It is unlikely that will be the case. So avoid the checking and complications */
        sme_trace_warn((TR_PAL_DM_FSM,"starting__unifi_sys_ma_unitdata_subscribe_cfm: start failed"));

        if (0 == fsmData->clientInfo.numClientsStarted)
        {
            /* unsubscribe any successful subscriptions */
            CsrInt32 i;
            for (i=0; i<fsmData->pendingSubcriptions; i++)
            {
                verify(TR_PAL_DM_FSM, fsmData->subscriptionInfo[i].used == TRUE);
                call_unifi_sys_ma_unitdata_unsubscribe_req(context,PAL_SYS_APP_HANDLE(context),fsmData->subscriptionInfo[i].subscriptionHandle);
            }
            CsrMemSet(fsmData->subscriptionInfo, 0, PAL_DM_NUM_UNITDATA_SUBSCRIPTIONS);
            fsmData->pendingSubcriptions = 0;

            pal_set_selected_channel_no(context, PAL_INVALID_CHANNEL_NUMBER);
            send_unifi_reset_req(context, getSmeContext(context)->unifiDriverInstance);

            fsm_next_state(context, FSMSTATE_stopping);
        }
        else
        {
            send_pal_dm_start_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context,fsmData->clientInfo.activeDmClientHandle),
                                  PaldmStatus_UnspecifiedError,
                                  fsmData->clientInfo.activeDmClientHandle);
            fsm_next_state(context, FSMSTATE_started);
        }
    }
}

static void starting__mlme_add_wds_cfm(FsmContext *context, const MlmeAddWdsCfm_Evt *cfm)
{
    DmFsmData *fsmData = FSMDATA;

    if (ResultCode_Success == cfm->resultCode)
    {
        CsrUint32 oui = 0x001958;

        /* subscribe for ma-unitdata-inds if this is the first cilent */
        if (0 == fsmData->clientInfo.numClientsStarted)
        {
            call_unifi_sys_ma_unitdata_subscribe_req(context,
                                                     PAL_SYS_APP_HANDLE(context),
                                                     unifi_Llc_Snap,
                                                     PAL_DATA_PROTO_ACTIVITY_REPORT_ID,
                                                     oui);

            call_unifi_sys_ma_unitdata_subscribe_req(context,
                                                     PAL_SYS_APP_HANDLE(context),
                                                     unifi_Llc_Snap,
                                                     PAL_DATA_PROTO_SECURITY_FRAMES_ID,
                                                     oui);

            call_unifi_sys_ma_unitdata_subscribe_req(context,
                                                     PAL_SYS_APP_HANDLE(context),
                                                     unifi_Llc_Snap,
                                                     PAL_DATA_PROTO_LINK_SUPERVISION_REQUEST_ID,
                                                     oui)

            call_unifi_sys_ma_unitdata_subscribe_req(context,
                                                     PAL_SYS_APP_HANDLE(context),
                                                     unifi_Llc_Snap,
                                                     PAL_DATA_PROTO_LINK_SUPERVISION_RESPONSE_ID,
                                                     oui)
            fsmData->pendingSubcriptions = 0;
        }
        else
        {
            sme_trace_info((TR_PAL_DM_FSM,"starting__unifi_sys_ma_unitdata_subscribe_cfm: start success"));

            fsmData->clientInfo.numClientsStarted++;
            fsmData->preferredChannelList.numChannels=1;
            fsmData->preferredChannelList.number[0] = pal_get_selected_channel_no(context);
            send_pal_dm_start_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context,fsmData->clientInfo.activeDmClientHandle),
                                  PaldmStatus_Success,
                                  fsmData->clientInfo.activeDmClientHandle);
            fsm_next_state(context, FSMSTATE_started);
        }
    }
    else
    {
        sme_trace_warn((TR_PAL_DM_FSM,"starting__mlme_add_wds_cfm: start failed"));

        if (0 == fsmData->clientInfo.numClientsStarted)
        {
            pal_set_selected_channel_no(context, PAL_INVALID_CHANNEL_NUMBER);
            send_unifi_reset_req(context, getSmeContext(context)->unifiDriverInstance);

            fsm_next_state(context, FSMSTATE_stopping);
        }
        else
        {
            send_pal_dm_start_cfm(context,
                                  pal_get_link_fsm_pid_from_handle(context,fsmData->clientInfo.activeDmClientHandle),
                                  PaldmStatus_UnspecifiedError,
                                  fsmData->clientInfo.activeDmClientHandle);
            fsm_next_state(context, FSMSTATE_started);
        }
    }
    verify(TR_PAL_DM_FSM,cfm->dummyDataRef2.dataLength==0 && cfm->dummyDataRef1.dataLength==0);
}

static void ready_hci_read_local_amp_info_cmd(FsmContext *context, const HciReadLocalAmpInfoCmd *cmd)
{
    send_hci_read_local_amp_info_response(context);
}

static void hci_read_local_amp_assoc_cmd_common_handler(FsmContext *context, const HciReadLocalAmpAssocCmd *cmd)
{
    DmFsmData *fsmData = FSMDATA;
    unifi_Status status = unifi_Success;

    sme_trace_entry((TR_PAL_DM_FSM,"hci_read_local_amp_assoc_cmd_common_handler - lengthSoFar - %d",cmd->lengthSoFar));
    if (!cmd->lengthSoFar)
    {
        if (!fsmData->readAssocInfo.data)
        {
            PAL_CoexCapabilities cap;

            sme_trace_info((TR_PAL_DM_FSM,"First read local amp assoc command. num preferred channels-s%d",fsmData->preferredChannelList.numChannels));

            pal_get_local_coex_capability(context,&cap);
            pal_encode_amp_assoc(context,
                                 NULL,
                                 &fsmData->preferredChannelList,
                                 pal_get_local_mac_address(context),
                                 &cap,
                                 &fsmData->readAssocInfo.data,
                                 &fsmData->readAssocInfo.totalLen);
        }
        else
        {
            sme_trace_warn((TR_PAL_DM_FSM,"Unexpected value 'zero' for lengthSoFar field"));
            status = unifi_Error;
        }
    }

    if (unifi_Success == status &&
        cmd->lengthSoFar == fsmData->readAssocInfo.currentLen &&
        cmd->lengthSoFar < fsmData->readAssocInfo.totalLen &&
        fsmData->readAssocInfo.totalLen <= cmd->maxRemoteAmpAssocLength &&
        cmd->maxRemoteAmpAssocLength <= PAL_AMP_ASSOC_MAX_TOTAL_LENGTH)
    {
        ReturnParameters *returnParams;
        CsrUint16 remainingLength = fsmData->readAssocInfo.totalLen-fsmData->readAssocInfo.currentLen;
        CsrUint8 lengthToSend=remainingLength%AMP_ASSOC_MAX_LENGTH_PER_COMMAND;

        verify(TR_PAL_DM_FSM,fsmData->readAssocInfo.currentLen<=fsmData->readAssocInfo.totalLen);
        /* No choice other than to allocate a new chunk because of the autogenerated code expected whole
         * object to be passed
         */
        returnParams = (ReturnParameters *)CsrPmalloc(sizeof(*returnParams)+lengthToSend);
        returnParams->hciReturnParam.readLocalAmpAssocReturn.assocFragment.data = CsrPmalloc(lengthToSend);

        CsrMemCpy(returnParams->hciReturnParam.readLocalAmpAssocReturn.assocFragment.data,
                   fsmData->readAssocInfo.data+fsmData->readAssocInfo.currentLen,
                   lengthToSend);
        fsmData->readAssocInfo.currentLen += lengthToSend;

        returnParams->hciReturnParam.readLocalAmpAssocReturn.assocFragment.dataLen = lengthToSend;
        returnParams->hciReturnParam.readLocalAmpAssocReturn.remainingLength = remainingLength;
        returnParams->commandCode = HCI_READ_LOCAL_AMP_ASSOC_CODE;
        returnParams->hciReturnParam.readLocalAmpAssocReturn.status = HCI_STATUS_CODE_SUCCESS;
        returnParams->hciReturnParam.readLocalAmpAssocReturn.physicalLinkHandle = cmd->physicalLinkHandle;
        PAL_SEND_HCI_COMMAND_COMPLETE(context, *returnParams, TRUE);
        CsrPfree(returnParams);
        /* data to be freed by the PAL Manager */

        /* free up the resources if last fragment was sent*/
        if (fsmData->readAssocInfo.currentLen == fsmData->readAssocInfo.totalLen)
        {
            sme_trace_info((TR_PAL_DM_FSM,"Last fragment of amp assoc sent. Freeing up resources"));
            pal_reset_amp_assoc_info(context, &fsmData->readAssocInfo);
        }
    }
    else
    {
        ReturnParameters returnParams;
        sme_trace_warn((TR_PAL_DM_FSM,"Some problem with the lengths - "
                                        "recvd lsFar-%d",
                                        cmd->lengthSoFar));
        returnParams.commandCode = HCI_READ_LOCAL_AMP_ASSOC_CODE;
        returnParams.hciReturnParam.readLocalAmpAssocReturn.status = HCI_STATUS_CODE_INVALID_HCI_COMMAND_PARAMETERS;
        returnParams.hciReturnParam.readLocalAmpAssocReturn.physicalLinkHandle = cmd->physicalLinkHandle;
        returnParams.hciReturnParam.readLocalAmpAssocReturn.remainingLength=0;
        returnParams.hciReturnParam.readLocalAmpAssocReturn.assocFragment.dataLen=0;
        returnParams.hciReturnParam.readLocalAmpAssocReturn.assocFragment.data = (CsrUint8 *)(&returnParams+1); /*only to keep the copy happpy in packing */
        PAL_SEND_HCI_COMMAND_COMPLETE(context, returnParams, TRUE);
    }
}

#ifdef FSM_DEBUG_DUMP
static void dm_fsm_dump(FsmContext* context, const CsrUint16 id)
{
}
#endif

static void dm_fsm_reset(FsmContext* context)
{

}

/** State idle transitions */
static const FsmEventEntry stoppedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                      stopped__core_start_req),
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_INFO_CODE,           ready_hci_read_local_amp_info_cmd),
};

/** State idle transitions */
static const FsmEventEntry readyTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                        ready_stop),
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_INFO_CODE,            ready_hci_read_local_amp_info_cmd),
    fsm_event_table_entry(PAL_DM_START_REQ_ID,                     ready__pal_dm_start_req),
    fsm_event_table_entry(PAL_DM_STOP_REQ_ID,                      ready__pal_dm_stop_req),
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_ASSOC_CODE,           hci_read_local_amp_assoc_cmd_common_handler),
    fsm_event_table_entry(HCI_RESET_CODE,                          ready__hci_reset_cmd),
};

/** State idle transitions */
static const FsmEventEntry scanningTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(SM_SCAN_CFM_ID,                          scanning__scan_cfm),
};

/** State idle transitions */
static const FsmEventEntry stoppingTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(UNIFI_RESET_CFM_ID,                     stopping__unifi_reset_cfm),
    fsm_event_table_entry(MLME_DEL_WDS_CFM_ID,                    stopping__mlme_del_wds_cfm),
};

/** State idle transitions */
static const FsmEventEntry startingTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_RESET_CFM_ID,                      starting__mlme_reset_cfm),
    fsm_event_table_entry(MLME_START_CFM_ID,                             starting__mlme_start_cfm),
    fsm_event_table_entry(MLME_ADD_WDS_CFM_ID,                           starting__mlme_add_wds_cfm),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,        starting__unifi_sys_ma_unitdata_subscribe_cfm),
};

/** State idle transitions */
static const FsmEventEntry startedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                        started_stop),

    fsm_event_table_entry(PAL_DM_MLME_ACTIVITY_START_REQ_ID,       started__pal_dm_mlme_activity_start_req),
    fsm_event_table_entry(PAL_DM_STOP_REQ_ID,                      started__pal_dm_stop_req),
    fsm_event_table_entry(PAL_DM_START_REQ_ID,                     started__pal_dm_start_req),
    fsm_event_table_entry(PAL_DM_DISABLE_RTS_CTS_REQ_ID,           started__pal_dm_disable_rts_cts_req),
    fsm_event_table_entry(PAL_DM_ENABLE_RTS_CTS_REQ_ID,            started__pal_dm_enable_rts_cts_req),
    fsm_event_table_entry(HCI_READ_RSSI_CODE,                      started__hci_read_rssi_cmd),
    fsm_event_table_entry(HCI_READ_LINK_QUALITY_CODE,              started__hci_read_link_quality_cmd),
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_ASSOC_CODE,           hci_read_local_amp_assoc_cmd_common_handler),
    fsm_event_table_entry(HCI_SHORT_RANGE_MODE_CODE,               started__hci_short_range_mode_cmd),
};

/** State idle transitions */
static const FsmEventEntry waitForLmActivityCompleteTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PAL_DM_MLME_ACTIVITY_COMPLETE_REQ_ID,     wait_for_lm_activity_complete__pal_dm_mlme_activity_complete_req),
    fsm_event_table_entry(HCI_READ_LOCAL_AMP_ASSOC_CODE,            hci_read_local_amp_assoc_cmd_common_handler),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_START_CFM_ID,                            fsm_ignore_event),
    fsm_event_table_entry(MLME_SET_CFM_ID,                              fsm_ignore_event),
    fsm_event_table_entry(MLME_DEL_WDS_CFM_ID,                          fsm_ignore_event),
    fsm_event_table_entry(MLME_ADD_WDS_CFM_ID,                          fsm_ignore_event),
    fsm_event_table_entry(MLME_RESET_CFM_ID,                            fsm_ignore_event),

    fsm_event_table_entry(SM_PAUSE_CFM_ID,                              fsm_ignore_event),
    fsm_event_table_entry(SM_UNPAUSE_CFM_ID,                            fsm_ignore_event),
    fsm_event_table_entry(PAL_DM_MLME_ACTIVITY_COMPLETE_REQ_ID,         fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_PORT_CONFIGURE_CFM_ID,              fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID,     fsm_ignore_event),
    fsm_event_table_entry(UNIFI_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,       fsm_ignore_subscription_cfm_event),
};

/** Profile Storage state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                       State                   State                               Save     */
   /*                       Name                    Transitions                          *       */
   fsm_state_table_entry(FSMSTATE_stopped, stoppedTransitions, TRUE),
   fsm_state_table_entry(FSMSTATE_ready, readyTransitions, TRUE),
   fsm_state_table_entry(FSMSTATE_scanning, scanningTransitions, TRUE),
   fsm_state_table_entry(FSMSTATE_starting, startingTransitions, TRUE),
   fsm_state_table_entry(FSMSTATE_started, startedTransitions, TRUE),

   fsm_state_table_entry(FSMSTATE_stopping, stoppingTransitions, TRUE),
   fsm_state_table_entry(FSMSTATE_wait_for_lm_activity_complete, waitForLmActivityCompleteTransitions, TRUE),
};

const FsmProcessStateMachine pal_dm_fsm =
{
#ifdef FSM_DEBUG
       "PAL DM",                                  /* Process Name       */
#endif
       PAL_DM_PROCESS,                                /* Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                         /* Transition Tables  */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE),   /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                    /* ignore event handers */
       pal_dm_init,                                    /* Entry Function     */
       dm_fsm_reset,                                                               /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       dm_fsm_dump                                                             /* Trace Dump Function   */
#endif
};

/*
 * FSM Scripts Config for this FSM
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */


/* PUBLIC FUNCTIONS *********************************************************/

void pal_dm_set_channel_to_select(FsmContext* context, CsrUint8 channel)
{
    DmFsmData *fsmData = fsm_get_params_by_id(context, getSmeContext(context)->palDmFsmInstance, DmFsmData);

    sme_trace_entry((TR_PAL_DM_FSM,"pal_set_channel_to_select():channel-%d,id-%d",channel,getSmeContext(context)->palDmFsmInstance));
    if (channel > 0 && channel <= PAL_MAX_CHANNEL_NUMBER)
    {
        fsmData->fixedChannel = channel;
        fsmData->preferredChannelList.number[0] = fsmData->fixedChannel;
        fsmData->preferredChannelList.numChannels=1;
    }
}

CsrUint8 pal_dm_get_subscription_handle(FsmContext* context, PAL_DataProtocolId prot)
{
    DmFsmData *fsmData = fsm_get_params_by_id(context, getSmeContext(context)->palDmFsmInstance, DmFsmData);
    CsrUint8 subHandle=0xFF;
    switch (prot)
    {
        case PAL_DATA_PROTO_ACTIVITY_REPORT_ID:
            subHandle = fsmData->subscriptionInfo[PAL_DATA_PROTO_ACTIVITY_REPORT_ID-PAL_DATA_PROTO_ACTIVITY_REPORT_ID].subscriptionHandle;
            break;

        case PAL_DATA_PROTO_SECURITY_FRAMES_ID:
            subHandle = fsmData->subscriptionInfo[PAL_DATA_PROTO_SECURITY_FRAMES_ID-PAL_DATA_PROTO_ACTIVITY_REPORT_ID].subscriptionHandle;
            break;

        case PAL_DATA_PROTO_LINK_SUPERVISION_REQUEST_ID:
            subHandle = fsmData->subscriptionInfo[PAL_DATA_PROTO_LINK_SUPERVISION_REQUEST_ID-PAL_DATA_PROTO_ACTIVITY_REPORT_ID].subscriptionHandle;
            break;

        case PAL_DATA_PROTO_LINK_SUPERVISION_RESPONSE_ID:
            subHandle = fsmData->subscriptionInfo[PAL_DATA_PROTO_LINK_SUPERVISION_RESPONSE_ID-PAL_DATA_PROTO_ACTIVITY_REPORT_ID].subscriptionHandle;
            break;

        default:
        break;
    }
    return subHandle;
}
/** @}
 */
