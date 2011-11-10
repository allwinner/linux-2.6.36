/** @file pal_ctrl_sap.c
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/saps_impl/pal_ctrl_sap/linuxfunctioncall/pal_ctrl_sap.c#1 $
 *
 ****************************************************************************/

/* paldata lib builds version of the pal_ctrl */
#include "paldata_top_level_fsm/paldata.h"
#include "pal_ctrl_sap/pal_ctrl_sap.h"
#include "paldata_ctrl_sap/paldata_ctrl_sap.h"

#include "linux_sap_platform.h"

void paldata_pal_ctrl_link_create_cfm(void* context, void *appHandle, CsrUint16 logicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_link_create_cfm(linuxContext->fsmContext, appHandle, logicalLinkHandle);
}

void paldata_pal_ctrl_link_delete_cfm(void* context, void *appHandle, CsrUint16 logicalLinkHandle, CsrUint8 physicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_link_delete_cfm(linuxContext->fsmContext, appHandle, logicalLinkHandle,physicalLinkHandle);
}

void paldata_pal_ctrl_link_modify_cfm(void* context, void *appHandle, CsrUint16 logicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_link_modify_cfm(linuxContext->fsmContext, appHandle, logicalLinkHandle);
}

void paldata_pal_ctrl_event_mask_set_cfm(void* context, void *appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_event_mask_set_cfm(linuxContext->fsmContext, appHandle);
}

void paldata_pal_ctrl_link_flush_cfm(void* context, void *appHandle, CsrUint16 logicalLinkHandle, CsrBool flushOccured)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_link_flush_cfm(linuxContext->fsmContext, appHandle, logicalLinkHandle, flushOccured);
}

void paldata_pal_ctrl_failed_contact_counter_read_cfm(void* context, void *appHandle, CsrUint16 logicalLinkHandle, CsrUint16 failedContactCounter)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_failed_contact_counter_read_cfm(linuxContext->fsmContext, appHandle, logicalLinkHandle, failedContactCounter);
}

void paldata_pal_ctrl_failed_contact_counter_reset_cfm(void* context, void *appHandle, CsrUint16 logicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_failed_contact_counter_reset_cfm(linuxContext->fsmContext, appHandle, logicalLinkHandle);
}

void paldata_pal_ctrl_activate_cfm(void* context, void *appHandle, CsrUint16 numDataBlocks)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_activate_cfm(linuxContext->fsmContext, appHandle, numDataBlocks);
}

void paldata_pal_ctrl_deactivate_cfm(void* context, void *appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_deactivate_cfm(linuxContext->fsmContext, appHandle);
}

void paldata_pal_ctrl_link_supervision_timeout_set_cfm(void* context, void *appHandle, CsrUint8 physicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_link_supervision_timeout_set_cfm(linuxContext->fsmContext, appHandle, physicalLinkHandle);
}

void paldata_pal_ctrl_link_supervision_timeout_modify_cfm(void* context, void *appHandle, CsrUint8 physicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_link_supervision_timeout_modify_cfm(linuxContext->fsmContext, appHandle, physicalLinkHandle);
}

void paldata_pal_ctrl_link_supervision_timeout_delete_cfm(void* context, void *appHandle, CsrUint8 physicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_link_supervision_timeout_delete_cfm(linuxContext->fsmContext, appHandle, physicalLinkHandle);
}

void paldata_pal_ctrl_early_link_loss_ind(void* context, void *appHandle, CsrUint8 physicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_early_link_loss_ind(linuxContext->fsmContext, appHandle, physicalLinkHandle);
}

void paldata_pal_ctrl_link_lost_ind(void* context, void *appHandle, CsrUint8 physicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    pal_ctrl_link_lost_ind(linuxContext->fsmContext, appHandle, physicalLinkHandle);
}

/* sme lib builds version of the pal_ctrl */
void pal_ctrl_link_create_req(void* context, void *appHandle, CsrUint16 logicalLinkHandle, CsrUint8 physicalLinkHandle,
                     CsrUint16 userPriority,
                     const unifi_MACAddress* remoteMacAddress,
                     const unifi_MACAddress* localMacAddress,
                     const unifi_AmpFlowSpec* txFlowSpec)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_link_create_req(linuxContext->palDataFsmContext, 
                                     appHandle,
                                     logicalLinkHandle,
                                     physicalLinkHandle,
                                     userPriority,
                                     remoteMacAddress,
                                     localMacAddress,
                                     txFlowSpec);
}

void pal_ctrl_link_delete_req(void* context, void *appHandle, CsrUint16 logicalLinkHandle, CsrUint8 physicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_link_delete_req(linuxContext->palDataFsmContext, appHandle, logicalLinkHandle,physicalLinkHandle);
}

void pal_ctrl_link_modify_req(void* context, void *appHandle, CsrUint16 logicalLinkHandle, const unifi_AmpFlowSpec* txFlowSpec)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_link_modify_req(linuxContext->palDataFsmContext, appHandle, logicalLinkHandle, txFlowSpec);
}

void pal_ctrl_event_mask_set_req(void* context, void *appHandle, CsrUint32 indMask)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_event_mask_set_req(linuxContext->palDataFsmContext, appHandle, indMask);
}

void pal_ctrl_link_flush_req(void* context, void *appHandle, CsrUint16 logicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_link_flush_req(linuxContext->palDataFsmContext, appHandle, logicalLinkHandle);
}

void pal_ctrl_failed_contact_counter_read_req(void* context, void *appHandle, CsrUint16 logicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_failed_contact_counter_read_req(linuxContext->palDataFsmContext, appHandle, logicalLinkHandle);
}

void pal_ctrl_failed_contact_counter_reset_req(void* context, void *appHandle, CsrUint16 logicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_failed_contact_counter_reset_req(linuxContext->palDataFsmContext, appHandle, logicalLinkHandle);
}

void pal_ctrl_activate_req(void* context, void *appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_activate_req(linuxContext->palDataFsmContext, appHandle);
}

void pal_ctrl_deactivate_req(void* context, void *appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_deactivate_req(linuxContext->palDataFsmContext, appHandle);
}

void pal_ctrl_link_supervision_timeout_set_req(void* context, void *appHandle, CsrUint8 physicalLinkHandle, CsrUint16 linkSupervisionTimeout)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_link_supervision_timeout_set_req(linuxContext->palDataFsmContext, appHandle, physicalLinkHandle, linkSupervisionTimeout);
}

void pal_ctrl_link_supervision_timeout_modify_req(void* context, void *appHandle, CsrUint8 physicalLinkHandle, CsrUint16 linkSupervisionTimeout)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_link_supervision_timeout_modify_req(linuxContext->palDataFsmContext, appHandle, physicalLinkHandle, linkSupervisionTimeout);
}

void pal_ctrl_link_supervision_timeout_delete_req(void* context, void *appHandle, CsrUint8 physicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_link_supervision_timeout_delete_req(linuxContext->palDataFsmContext, appHandle, physicalLinkHandle);
}

void pal_ctrl_link_alive_req(void* context, void *appHandle, CsrUint8 physicalLinkHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    paldata_pal_ctrl_link_alive_req(linuxContext->palDataFsmContext, appHandle, physicalLinkHandle);
}
