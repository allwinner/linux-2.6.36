/** @file pal_data_common.c
 *
 * PAL Data Common
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
 *  This file contains the routines related to the link data handling shared
 * between various modules.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/pal_data_common.c#2 $
 *
 ****************************************************************************/

/** @{
 * @ingroup pal_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "csr_cstl/csr_wifi_list.h"
#include "smeio/smeio_fsm_types.h"
#include "pal_hci_sap/pal_hci_sap_types.h"
#include "link_manager_fsm/pal_data_common.h"
#include "link_manager_fsm/lm_link_fsm.h"
#include "link_manager_fsm/lm_hip_fsm.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/


/* PRIVATE CONSTANT DEFINITIONS *********************************************/
#define PAL_SET_TIMEOUT_VALUE(timeout,timer) timer.value = (timeout*LINK_ACCEPT_TIMEOUT_UNIT_IN_MICROS)/1000; timer.enabled=(0==timer.value)?FALSE:TRUE

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   create new physical link entry if the handle is unique
 *
 * @par Description
 *
 * @param[in]    palMgrData   :  pointer to the PAL Manager context
 * @param[in]    handle   : physical link handle for the new link
 *
 * @return
 *   pointer to the new link attributes if created or NULL.
 */
static physicalLinkSharedAttrib *create_new_phy_link_entry(palManagerContext *palMgrData, CsrUint8 handle)
{
    int i, freeEntryIndex=-1;
    CsrBool validHandle=TRUE, freeEntryFound=FALSE;

    sme_trace_entry((TR_PAL_MGR_FSM,"create_new_phy_link_entry(): handle-%d",handle));

    if (handle != 0)
    {
        for (i=0; i<palMgrData->maxPhysicalLinks; i++)
        {
            sme_trace_info((TR_PAL_MGR_FSM,"create_new_phy_link_entry(): count-%d, used-%d",i,palMgrData->phyLinkSharedAttrib[i].used));
            if (palMgrData->phyLinkSharedAttrib[i].used &&
                palMgrData->phyLinkSharedAttrib[i].physicalLinkHandle == handle)
            {
                validHandle = FALSE;
                sme_trace_warn((TR_PAL_MGR_FSM,"A link with same handle already exists"));
                break;
            }
            else if (!palMgrData->phyLinkSharedAttrib[i].used &&
                     !freeEntryFound)
            {
                sme_trace_info((TR_PAL_MGR_FSM,"Free entry available at index-%d",i));
                freeEntryIndex = i;
                freeEntryFound = TRUE;
            }
        }

        if (validHandle && freeEntryFound)
        {
            sme_trace_info((TR_PAL_MGR_FSM,"create_new_phy_link_entry():"));
            verify(TR_PAL_DAM,FALSE==palMgrData->phyLinkSharedAttrib[freeEntryIndex].used);
            palMgrData->phyLinkSharedAttrib[freeEntryIndex].used = TRUE;
            palMgrData->phyLinkSharedAttrib[freeEntryIndex].physicalLinkHandle = handle;
            palMgrData->phyLinkSharedAttrib[freeEntryIndex].qosSupported = palMgrData->phyLinkCommonAttrib.guranteedLinkSupported;
            sme_trace_info((TR_PAL_MGR_FSM,"PhyLink entry created for handle- %d",handle));
            return &palMgrData->phyLinkSharedAttrib[freeEntryIndex];
        }
    }
    return NULL;
}

/**
 * @brief
 *   find physical link attributes for a matching handle
 *
 * @par Description
 *
 * @param[in]    palMgrData   :  pointer to the PAL Manager context
 * @param[in]    handle   : physical link handle for the link
 *
 * @return
 *   pointer to the  link attributes if found or NULL.
 */
static physicalLinkSharedAttrib *find_existing_phy_link_entry(palManagerContext *palMgrData, CsrUint8 handle)
{
    int i;

    sme_trace_entry((TR_PAL_MGR_FSM,"find_existing_phy_link_entry() for handle - %d",handle));
    for (i=0; i<palMgrData->maxPhysicalLinks; i++)
    {
        if (palMgrData->phyLinkSharedAttrib[i].used &&
            palMgrData->phyLinkSharedAttrib[i].physicalLinkHandle==handle)
        {
            sme_trace_info((TR_PAL_MGR_FSM,"physical link entry found for handle - %d",handle));
            return &palMgrData->phyLinkSharedAttrib[i];
        }
    }
    return NULL;
}

/**
 * @brief
 *   find physical link attributes for a matching Link FSM pid (process ID)
 *
 * @par Description
 *
 * @param[in]    palMgrData   :  pointer to the PAL Manager context
 * @param[in]    pid   : process ID
 *
 * @return
 *   pointer to the  link attributes if found or NULL.
 */
static physicalLinkSharedAttrib *find_phy_link_entry_from_pid(palManagerContext *palMgrData, CsrUint16 pid)
{
    int i;

    sme_trace_entry((TR_PAL_MGR_FSM,"find_phy_link_entry_from_pid() for pid - %d",pid));
    for (i=0; i<palMgrData->maxPhysicalLinks; i++)
    {
        if (palMgrData->phyLinkSharedAttrib[i].used &&
            palMgrData->phyLinkSharedAttrib[i].linkFsmPid==pid)
        {
            sme_trace_info((TR_PAL_MGR_FSM,"physical link entry found for handle - %d",palMgrData->phyLinkSharedAttrib[i].physicalLinkHandle));
            return &palMgrData->phyLinkSharedAttrib[i];
        }
    }
    return NULL;
}

/**
 * @brief
 *   find physical link attributes for a matching remote MAC Address
 *
 * @par Description
 *
 * @param[in]    palMgrData   :  pointer to the PAL Manager context
 * @param[in]    remoteMacAddr   : remote MAC Address
 *
 * @return
 *   pointer to the  link attributes if found or NULL.
 */
static physicalLinkSharedAttrib *find_existing_phy_link_entry_from_remote_mac_addr(palManagerContext *palMgrData, const unifi_MACAddress *remoteMacAddr)
{
    int i;

    sme_trace_entry((TR_PAL_MGR_FSM,"find_existing_phy_link_entry_from_remote_mac_addr(): "));
    sme_trace_hex((TR_PAL_MGR_FSM, TR_LVL_INFO,"find_existing_phy_link_entry_from_remote_mac_addr:Remote Mac Addr",remoteMacAddr->data,6));

    for (i=0; i<palMgrData->maxPhysicalLinks; i++)
    {
        if (palMgrData->phyLinkSharedAttrib[i].used &&
            0 == CsrMemCmp(&palMgrData->phyLinkSharedAttrib[i].remoteMacAddress,
                            remoteMacAddr,
                            sizeof(remoteMacAddr->data)))
        {
            sme_trace_info((TR_PAL_MGR_FSM,"phyLink entry found for remote mac address"));
            return &palMgrData->phyLinkSharedAttrib[i];
        }
    }
    return NULL;
}

/**
 * @brief
 *   PAL_Core FSM Entry Function
 *
 * @par Description
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */

/* PUBLIC FUNCTIONS *********************************************************/

CsrUint8 pal_get_selected_channel_no(FsmContext *context)
{
    return getPalMgrContext(context)->phyLinkCommonAttrib.selectedChannel;
}

void pal_set_selected_channel_no(FsmContext *context, CsrUint8 channelNo)
{
    getPalMgrContext(context)->phyLinkCommonAttrib.selectedChannel = channelNo;
}

PAL_Timer *pal_get_connection_accept_timer(FsmContext *context)
{
    return &getPalMgrContext(context)->phyLinkCommonAttrib.connectionAcceptTimer;
}

PAL_Timer *pal_get_logical_link_accept_timer(FsmContext *context)
{
    return &getPalMgrContext(context)->phyLinkCommonAttrib.logicalLinkAcceptTimer;
}

void pal_set_connection_accept_timer(FsmContext *context, CsrUint32 value)
{
    PAL_SET_TIMEOUT_VALUE(value, getPalMgrContext(context)->phyLinkCommonAttrib.connectionAcceptTimer);
}

void pal_set_logical_link_accept_timer(FsmContext *context, CsrUint32 value)
{
    PAL_SET_TIMEOUT_VALUE(value, getPalMgrContext(context)->phyLinkCommonAttrib.logicalLinkAcceptTimer);
}

unifi_MACAddress *pal_get_local_mac_address(FsmContext *context)
{
    return &getPalMgrContext(context)->phyLinkCommonAttrib.localMacAddress;
}

void pal_set_local_mac_address(FsmContext *context, const unifi_MACAddress *macAddress)
{
    getPalMgrContext(context)->phyLinkCommonAttrib.localMacAddress = *macAddress;
}

PAL_MibData *pal_get_common_mib_data(FsmContext *context)
{
    return &getPalMgrContext(context)->phyLinkCommonAttrib.mibData;
}

void pal_set_common_mib_data(FsmContext *context, PAL_MibData *mibData)
{
    getPalMgrContext(context)->phyLinkCommonAttrib.mibData=*mibData;
}

CsrBool pal_get_link_qos_support(FsmContext *context, CsrUint8 phyLinkHandle)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_existing_phy_link_entry(palMgrData, phyLinkHandle);

    verify(TR_PAL_DAM, linkAttrib!=NULL);
    return linkAttrib->qosSupported;
}

void pal_set_link_qos_support(FsmContext *context, CsrUint8 phyLinkHandle, CsrBool qosSupported)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_existing_phy_link_entry(palMgrData, phyLinkHandle);

    verify(TR_PAL_DAM, linkAttrib!=NULL);
    linkAttrib->qosSupported = qosSupported;
}

unifi_MACAddress *pal_get_remote_mac_address(FsmContext *context, CsrUint8 phyLinkHandle)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_existing_phy_link_entry(palMgrData, phyLinkHandle);

    if (linkAttrib)
    {
        return &linkAttrib->remoteMacAddress;
    }
    else
    {
        return NULL;
    }
}

unifi_Status pal_get_phy_link_handle_from_remote_mac_address(FsmContext *context,
                                                                         const unifi_MACAddress *remoteMacAddr,
                                                                         CsrUint8 *phyLinkHandle)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_existing_phy_link_entry_from_remote_mac_addr(palMgrData, remoteMacAddr);

    if (linkAttrib)
    {
        *phyLinkHandle = linkAttrib->physicalLinkHandle;
        return unifi_Success;
    }
    else
    {
        return unifi_Error;
    }
}

void pal_set_remote_mac_address(FsmContext *context, CsrUint8 phyLinkHandle, const unifi_MACAddress *macAddress)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_existing_phy_link_entry(palMgrData, phyLinkHandle);

    if (linkAttrib)
    {
        linkAttrib->remoteMacAddress=*macAddress;
    }
}

CsrUint16 pal_lm_create_instance(FsmContext *context, CsrUint8 handle)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = create_new_phy_link_entry(palMgrData, handle);

    if (linkAttrib)
    {
        linkAttrib->linkFsmPid = fsm_add_instance(context, &pal_lm_link_fsm, TRUE);
        return linkAttrib->linkFsmPid;
    }
    else
    {
        return FSM_TERMINATE;
    }
}

CsrUint16 pal_lm_create_hip_instance(FsmContext *context, CsrUint8 handle)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_existing_phy_link_entry(palMgrData, handle);

    if (linkAttrib)
    {
        linkAttrib->hipFsmPid = fsm_add_instance(context, &pal_lm_hip_fsm, TRUE);
        return linkAttrib->hipFsmPid;
    }
    else
    {
        return FSM_TERMINATE;
    }
}

void pal_delete_lm_instance(FsmContext *context, CsrUint16 pid)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_phy_link_entry_from_pid(palMgrData,pid);

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_delete_lm_instance: pid-%d",pid));
    if (linkAttrib)
    {
        sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_delete_lm_instance: Instance deleted"));
        CsrMemSet(linkAttrib, 0, sizeof(physicalLinkSharedAttrib));
    }
}

CsrBool pal_lm_instance_exits(FsmContext *context)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    int i;

    for (i=0; i<palMgrData->maxPhysicalLinks; i++)
    {
        if (palMgrData->phyLinkSharedAttrib[i].used)
        {
            return TRUE;
        }
    }
    return FALSE;
}

CsrUint16 pal_get_link_fsm_pid_from_handle(FsmContext *context, CsrUint8 handle)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_existing_phy_link_entry(palMgrData, handle);
    return (linkAttrib)?linkAttrib->linkFsmPid:FSM_TERMINATE;
}

CsrUint16 pal_get_hip_fsm_pid_from_handle(FsmContext *context, CsrUint8 handle)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_existing_phy_link_entry(palMgrData, handle);
    return (linkAttrib)?linkAttrib->hipFsmPid:FSM_TERMINATE;
}

CsrUint16 pal_get_hip_fsm_pid_from_remote_mac_address(FsmContext *context, const unifi_MACAddress *remoteMacAddress)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    physicalLinkSharedAttrib *linkAttrib = find_existing_phy_link_entry_from_remote_mac_addr(palMgrData, remoteMacAddress);
    return (linkAttrib)?linkAttrib->hipFsmPid:FSM_TERMINATE;
}

CsrBool pal_guranteed_link_supported(FsmContext *context)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_guranteed_link_supported: state-%d",palMgrData->phyLinkCommonAttrib.guranteedLinkSupported));
    return palMgrData->phyLinkCommonAttrib.guranteedLinkSupported;
}

CsrBool pal_security_enabled(FsmContext *context)
{
    palManagerContext *palMgrData = getPalMgrContext(context);
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_security_enabled: state-%d",palMgrData->phyLinkCommonAttrib.securityEnabled));
    return palMgrData->phyLinkCommonAttrib.securityEnabled;
}

/** @}
 */
