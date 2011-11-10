/** @file pal_manager.c
 *
 * PAL Manager
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
 *   PAL Manager implements all global procedures required by PAL.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/pal_manager/pal_manager.c#2 $
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

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "link_manager_fsm/hip_interface.h"
#include "device_manager_fsm/device_manager_fsm.h"
#include "coex_manager_fsm/coex_manager_fsm.h"


/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */

/* PRIVATE TYPES DEFINITIONS ************************************************/
/* control structure containing parameters used by PAL Manager */


/* GLOBAL VARIABLE DEFINITIONS **********************************************/


/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS *********************************************/

palManagerContext *pal_manager_init(CsrUint16 maxPhysicalLinks, CsrUint16 maxHciCommandsAllowed)
{
    palManagerContext *palMgrData = CsrPmalloc(sizeof(palManagerContext));

    if (maxPhysicalLinks > PAL_MAX_PHYSICAL_LINKS)
    {
        maxPhysicalLinks = PAL_MAX_PHYSICAL_LINKS;
    }

    palMgrData->maxPhysicalLinks = maxPhysicalLinks;
    palMgrData->phyLinkSharedAttrib = CsrPmalloc(sizeof(physicalLinkSharedAttrib)*maxPhysicalLinks);
    CsrMemSet(palMgrData->phyLinkSharedAttrib,0,sizeof(physicalLinkSharedAttrib)*maxPhysicalLinks);

    PAL_ENABLE_TIMER(palMgrData->phyLinkCommonAttrib.connectionAcceptTimer);
    PAL_ENABLE_TIMER(palMgrData->phyLinkCommonAttrib.logicalLinkAcceptTimer);
    palMgrData->phyLinkCommonAttrib.connectionAcceptTimer.value = LINK_ACCEPT_TIMEOUT_VALUE_DEFAULT_IN_MILLIS;
    palMgrData->phyLinkCommonAttrib.logicalLinkAcceptTimer.value = LINK_ACCEPT_TIMEOUT_VALUE_DEFAULT_IN_MILLIS;
    palMgrData->phyLinkCommonAttrib.selectedChannel = PAL_INVALID_CHANNEL_NUMBER;
    palMgrData->phyLinkCommonAttrib.numTotalDataBlocks = 0;
    CsrMemSet(&palMgrData->phyLinkCommonAttrib.mibData,0,sizeof(PAL_MibData));

    /* set the country string 0xFFFFFF as per IEEE 2007 (refer dot11CountryString)
         * spec so that station is a non-country entity
         */
    palMgrData->phyLinkCommonAttrib.mibData.dot11CountryString[0]=palMgrData->phyLinkCommonAttrib.mibData.dot11CountryString[1]=palMgrData->phyLinkCommonAttrib.mibData.dot11CountryString[2]='X';
    palMgrData->phyLinkCommonAttrib.mibData.dot11HCCWmin = PAL_HIP_DSSS_DEFAULT_CW_MIN;
    palMgrData->phyLinkCommonAttrib.mibData.cipherSuite[0] = 0x00;
    palMgrData->phyLinkCommonAttrib.mibData.cipherSuite[1] = 0x0F;
    palMgrData->phyLinkCommonAttrib.mibData.cipherSuite[2] = 0xAC;
    palMgrData->phyLinkCommonAttrib.mibData.cipherSuite[3] = 0x04;
#ifdef PAL_GUARANTEED_LINK_DISABLED
    palMgrData->phyLinkCommonAttrib.guranteedLinkSupported = FALSE;
#else
    palMgrData->phyLinkCommonAttrib.guranteedLinkSupported = TRUE;
#endif

#ifdef PAL_SECURITY_DISABLED
    palMgrData->phyLinkCommonAttrib.securityEnabled = FALSE;
#else
    palMgrData->phyLinkCommonAttrib.securityEnabled = TRUE;
#endif

    palMgrData->initialLocalAmpInfoRead = FALSE;
    palMgrData->ampStatus = AMP_STATUS_AVAILABLE_BUT_PHYSICALLY_POWERED_DOWN;
    palMgrData->appHandle = (void *)palMgrData; /* some value to keep it unique*/

    palMgrData->hciBottom = pal_hci_bottom_init(maxHciCommandsAllowed);
    return palMgrData;
}

void pal_manager_deinit(FsmContext *context)
{
    palManagerContext *palMgrData = getPalMgrContext(context);

    pal_hci_bottom_deinit(context);
    CsrPfree(palMgrData->phyLinkSharedAttrib);
    CsrPfree(palMgrData);
}

void pal_set_security_support(FsmContext *context, CsrBool state)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"pal_set_security_support: state-%d",state));
#ifdef PAL_SECURITY_ENABLED
    if (TRUE == state || FALSE==state)
    {
        palManagerContext *palMgrData = getPalMgrContext(context);
        sme_trace_entry((TR_PAL_MGR_FSM,"pal_set_security_support: changing securityEnable state from %d to %d",
                         palMgrData->phyLinkCommonAttrib.securityEnabled,state));
        palMgrData->phyLinkCommonAttrib.securityEnabled = state;
    }
#endif
}

void pal_set_qos_support(FsmContext *context, CsrBool state)
{

    sme_trace_entry((TR_PAL_MGR_FSM,"pal_set_qos_support: state-%d",state));

    if (TRUE == state || FALSE==state)
    {
        palManagerContext *palMgrData = getPalMgrContext(context);

        sme_trace_entry((TR_PAL_MGR_FSM,"pal_set_qos_support: changing qos support state from %d to %d",
                         palMgrData->phyLinkCommonAttrib.guranteedLinkSupported,state));
        palMgrData->phyLinkCommonAttrib.guranteedLinkSupported = state;
    }
}

void pal_fix_channel(FsmContext* context, CsrUint8 channel)
{
    pal_dm_set_channel_to_select(context, channel);
}

void pal_generate_ar(FsmContext* context,
                        CsrBool scheduleKnown,
                        CsrUint32 startTime,
                        CsrUint32 duration,
                        CsrUint32 periodicity)
{
    sme_trace_entry((TR_PAL_MGR_FSM,"pal_generate_ar(): scheduleKnown-%d, startTime-%d, duration-%d, periodicity-%d",
        scheduleKnown, startTime, duration, periodicity));

    pal_coex_generate_ar(context, scheduleKnown, startTime, duration, periodicity);
}

CsrBool pal_amp_connection_in_progress(FsmContext* context)
{
    /* AMP Connection is in progress if there is atleast one client */
    sme_trace_entry((TR_PAL_MGR_FSM,"pal_amp_connection_in_progress():"));
    return pal_lm_instance_exits(context);
}

void pal_mgr_update_sme_connection_status(FsmContext* context, PAL_SmeConnectionStatus connectionStatus)
{
    AmpStatus newAmpStatus = getPalMgrContext(context)->ampStatus;

    sme_trace_entry((TR_PAL_MGR_FSM,"pal_mgr_update_sme_connection_status(): newStatus-%d, oldStatus-%d",connectionStatus, getPalMgrContext(context)->ampStatus));

    switch (connectionStatus)
    {
        case PAL_SME_CONNECTION_STATUS_POWER_OFF:
            newAmpStatus = AMP_STATUS_AVAILABLE_BUT_PHYSICALLY_POWERED_DOWN;
            break;
        case PAL_SME_CONNECTION_STATUS_DISABLED:
            newAmpStatus = AMP_STATUS_ONLY_USED_BY_BLUETOOTH_TECHNOLOGY;
            break;
        case PAL_SME_CONNECTION_STATUS_CONNECTED_FULL_CAPACITY:
            newAmpStatus = AMP_STATUS_NO_CAPACITY_AVAILABLE_FOR_BLUETOOTH_OPERATION;
            break;
        case PAL_SME_CONNECTION_STATUS_CONNECTED_HIGH_CAPICITY:
            newAmpStatus = AMP_STATUS_LOW_CAPACITY_AVAILABLE_FOR_BLUETOOTH_OPERATION;
            break;
        case PAL_SME_CONNECTION_STATUS_CONNECTED_MEDIUM_CAPICITY:
            newAmpStatus = AMP_STATUS_MEDIUM_CAPACITY_AVAILABLE_FOR_BLUETOOTH_OPERATION;
            break;
        case PAL_SME_CONNECTION_STATUS_CONNECTED_LOW_CAPICITY:
            newAmpStatus = AMP_STATUS_HIGH_CAPACITY_AVAILABLE_FOR_BLUETOOTH_OPERATION;
            break;
        case PAL_SME_CONNECTION_STATUS_DISCONNECTED:
            newAmpStatus = AMP_STATUS_FULL_CAPACITY_AVAILABLE_FOR_BLUETOOTH_OPERATION;
            break;
        default:
            break;
    }

    if (newAmpStatus != getPalMgrContext(context)->ampStatus)
    {
        getPalMgrContext(context)->ampStatus = newAmpStatus;
        sme_trace_info((TR_PAL_DM_FSM,"pal_mgr_update_sme_connection_status():updated new Status-%d,Initial LocalAmpInfoRead-%d",getPalMgrContext(context)->ampStatus,getPalMgrContext(context)->initialLocalAmpInfoRead));
        if (getPalMgrContext(context)->initialLocalAmpInfoRead)
        {
            pal_hci_send_hci_amp_status_change_evt(context,HCI_STATUS_CODE_SUCCESS,getPalMgrContext(context)->ampStatus);
        }
    }
}


void pal_sme_connection_status_change_request(FsmContext* context, PAL_SmeConnectionStatus connectionStatus)
{
    /* AMP Connection is in progress if there is atleast one client */
    sme_trace_entry((TR_PAL_MGR_FSM,"pal_sme_connection_status_change_request(): %d",connectionStatus));

    pal_mgr_update_sme_connection_status(context, connectionStatus);
}

CsrBool pal_mgr_amp_connections_allowed(FsmContext* context)
{
    return ((PAL_SME_CONNECTION_STATUS_POWER_OFF==getPalMgrContext(context)->ampStatus)||
            (AMP_STATUS_NO_CAPACITY_AVAILABLE_FOR_BLUETOOTH_OPERATION==getPalMgrContext(context)->ampStatus)
            )?FALSE:TRUE;
}


/* FSM DEFINITION **********************************************/
/*
 * FSM Scripts Config for this FSM
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/** @}
 */
