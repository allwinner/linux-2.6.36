/** @file data_manager_fsm.c
 *
 * PAL Data Manager FSM
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
 *   This FSM is tasked handling the data manager events.
 *
 ****************************************************************************
 *
 * @section MODIFICATION HISTORY
 * @verbatim
 *   #1    17:Apr:08 B-36899: Created
 * @endverbatim
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_paldata/paldata/data_manager/data_manager_fsm.c#4 $
 *
 ****************************************************************************/

/** @{
 * @ingroup data_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "paldata_top_level_fsm/paldata_top_level_fsm.h"

#include "data_manager/data_manager_fsm.h"

#include "smeio/smeio_trace_types.h"
#include "paldata_ctrl_sap/paldata_ctrl_sap_from_sme_interface.h"
#include "paldata_acl_sap/paldata_acl_sap_from_sme_interface.h"
#include "paldata_sys_sap/paldata_sys_sap_from_sme_interface.h"

#include "pal_hci_sap/pal_hci_sap_up_pack.h"
#include "event_pack_unpack/event_pack_unpack.h"
#include "data_manager/data_manager.h"
#include "data_manager/data_manager_fsm_types.h"
#include "data_manager/data_manager_fsm_events.h"

/* MACROS *******************************************************************/
#define FSMDATA (fsm_get_params(context, dataManagerFsmContext))

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

typedef struct dataManagerFsmContext
{
    dataManagerContext *damContext;
    void *appHandle;
    CsrUint8 pendingPhysicalLinkHandle;
    CsrUint8 subscriptionHandle;
}dataManagerFsmContext;

typedef enum FsmState
{
    FSMSTATE_wait_for_startup,
    FSMSTATE_ready,
    FSMSTATE_MAX_STATE
} FsmState;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* Define INLINE directive*/
#ifndef INLINE
#define INLINE      inline
#endif

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/


/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Handler for pal_ctrl_start_req event
 *
 * @par Description
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void wait_for_startup__pal_ctrl_activate_req(FsmContext *context, const PalCtrlActivateReq_Evt *req)
{
    if (!FSMDATA->appHandle)
    {
        FSMDATA->appHandle = req->appHandle;
        /* read traffic and command queue sizes from the driver*/
        call_paldata_sys_capabilities_req(context,PALDATA_APP_HANDLE(context));
    }
    else
    {
        call_paldata_pal_ctrl_activate_cfm(context,req->appHandle, 0);
    }
}

static void wait_for_startup__paldata_sys_capabilities_cfm(FsmContext *context, const PaldataSysCapabilitiesCfm_Evt *cfm)
{
    dataManagerContext *damContext = FSMDATA->damContext;
    CsrUint16 numBlocks;

    numBlocks = pal_dam_init_queue_info(damContext,cfm->trafficQueueSize,cfm->commandQueueSize);

    call_paldata_pal_ctrl_activate_cfm(context,FSMDATA->appHandle, numBlocks);
    fsm_next_state(context, FSMSTATE_ready);
}

static void wait_for_startup__pal_acl_data_req(FsmContext *context, const PalAclDataReq_Evt *req)
{
    sme_trace_warn((TR_PAL_DAM,"wait_for_startup__pal_acl_data_req: acl in wrong state. discard"));
    PALDATA_MEM_FREE_DATA_BUFFER(req->freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(req->data));
    PALDATA_MEM_DOWNSTREAM_FREE_DATA_BLOCK(req->data);
}

static void wait_for_startup__paldata_sys_ma_unitdata_ind(FsmContext *context, const PaldataSysMaUnitdataInd_Evt *ind)
{
    sme_trace_warn((TR_PAL_DAM,"wait_for_startup__paldata_sys_ma_unitdata_ind: unitdata in wrong state. discard"));
    PALDATA_MEM_FREE_DATA_BUFFER(ind->freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(ind->frame));
    PALDATA_MEM_UPSTREAM_FREE_DATA_BLOCK(ind->frame);
}

static void ready__pal_ctrl_deactivate_req(FsmContext *context, const PalCtrlDeactivateReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;

        /* This tells us either the system is shutting down or there is something wrong with lower layers
            * (for eg: firmware crash). So cleanup everything locally. Send any response thats pending and
            * move to startup state.
            */
        pal_dam_deinit(context, damContext, FALSE);
        FSMDATA->appHandle = NULL;
        call_paldata_pal_ctrl_deactivate_cfm(context, req->appHandle);
        fsm_next_state(context, FSMSTATE_wait_for_startup);
    }
    else
    {
        call_paldata_pal_ctrl_deactivate_cfm(context, req->appHandle);
    }
}

static void ready__pal_ctrl_event_mask_set_req(FsmContext *context, const PalCtrlEventMaskSetReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;
        CsrBool qosViolationEventEnabled = (CsrBool)req->indMask&unifi_PalDataIndQosViolation;
        CsrBool numCompletedDataBlocksEventEnabled = (CsrBool)req->indMask&unifi_PalDataIndCompletedDataBlocks;

        pal_dam_set_event_mask(damContext, qosViolationEventEnabled, numCompletedDataBlocksEventEnabled);
    }
    call_paldata_pal_ctrl_event_mask_set_cfm(context, req->appHandle);
}

static void pal_dam_flush_occured_on_flush_req_callback(FsmContext *context, CsrUint16 logicalLinkHandle, CsrBool flushOccured)
{
    call_paldata_pal_ctrl_link_flush_cfm(context, FSMDATA->appHandle, logicalLinkHandle, flushOccured);
}

static void ready__pal_ctrl_link_flush_req(FsmContext *context, const PalCtrlLinkFlushReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;

        pal_dam_process_flush_req(context, damContext, req->logicalLinkHandle,
                                  pal_dam_flush_occured_on_flush_req_callback);
    }
    else
    {
        call_paldata_pal_ctrl_link_flush_cfm(context, req->appHandle, req->logicalLinkHandle, FALSE);
    }
}

static void ready__pal_dam_flush_timer_expired(FsmContext *context, const PalDamFlushTimer_Evt *timer)
{
    dataManagerContext *damContext = FSMDATA->damContext;
    sme_trace_entry((TR_PAL_DAM, "pal_dam_flush_timer_expired(): handle-%d",timer->logicalLinkHandle));

    pal_dam_process_flush_timeout(context, damContext, timer->logicalLinkHandle, NULL);
}

static void ready__pal_dam_early_link_loss_timer_expired(FsmContext *context, const PalDamEarlyLinkLossTimer_Evt *timer)
{
    dataManagerContext *damContext = FSMDATA->damContext;
    sme_trace_entry((TR_PAL_DAM, "pal_dam_early_link_loss_timer_expired(): handle-%d ",timer->physicalLinkHandle));

    if (!pal_dam_early_link_loss_timer_expired(context, damContext, timer->physicalLinkHandle))
    {
        call_paldata_pal_ctrl_early_link_loss_ind(context, FSMDATA->appHandle, timer->physicalLinkHandle);
    }
}

static void ready__pal_dam_link_loss_timer_expired(FsmContext *context, const PalDamLinkLossTimer_Evt *timer)
{
    dataManagerContext *damContext = FSMDATA->damContext;
    sme_trace_entry((TR_PAL_DAM, "pal_dam_link_loss_timer_expired(): handle-%d ",timer->physicalLinkHandle));

    if (!pal_dam_link_loss_timer_expired(context, damContext, timer->physicalLinkHandle))
    {
        call_paldata_pal_ctrl_link_lost_ind(context, FSMDATA->appHandle, timer->physicalLinkHandle);
    }
}

static void ready__paldata_sys_ma_unitdata_subscribe_cfm(FsmContext *context, const PaldataSysMaUnitdataSubscribeCfm_Evt *cfm)
{
    FSMDATA->subscriptionHandle = cfm->subscriptionHandle;
    pal_dam_set_subscription_handle_and_offset(FSMDATA->damContext, cfm->subscriptionHandle,cfm->allocOffset);
    call_paldata_pal_ctrl_link_supervision_timeout_set_cfm(context, FSMDATA->appHandle, FSMDATA->pendingPhysicalLinkHandle);
}

static void ready__pal_dam_link_create_req(FsmContext *context, const PalCtrlLinkCreateReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;

        sme_trace_entry((TR_PAL_DAM, "pal_dam_disconnect_all_links_req(): userPrio-%d",req->userPriority));
        pal_dam_create_logical_link_entry(context,damContext,
                                  req->physicalLinkHandle,
                                  &req->txFlowSpec,
                                  req->logicalLinkHandle,
                                  req->userPriority,
                                  &req->remoteMacAddress,
                                  &req->localMacAddress);

#ifdef FSM_DEBUG_DUMP
        pal_dm_dump(damContext);
#endif
    }
    call_paldata_pal_ctrl_link_create_cfm(context, req->appHandle, req->logicalLinkHandle);
}

static void ready__pal_dam_link_modify_req(FsmContext *context, const PalCtrlLinkModifyReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;

        pal_dam_modify_logical_link_entry(context, damContext,
                                          req->logicalLinkHandle,
                                          &req->txFlowSpec);
#ifdef FSM_DEBUG_DUMP
        pal_dm_dump(damContext);
#endif
    }
    call_paldata_pal_ctrl_link_modify_cfm(context, req->appHandle, req->logicalLinkHandle);
}

static void pal_dam_link_delete_callback(FsmContext *context, CsrUint16 logicalLinkHandle, CsrUint8 physicalLinkHandle)
{
#ifdef FSM_DEBUG_DUMP
    dataManagerContext *damContext = FSMDATA->damContext;
    pal_dm_dump(damContext);
#endif
    call_paldata_pal_ctrl_link_delete_cfm(context, FSMDATA->appHandle, logicalLinkHandle,physicalLinkHandle);

}

static void ready__pal_dam_link_delete_req(FsmContext *context, const PalCtrlLinkDeleteReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;

        (void)pal_dam_delete_matching_logical_link(context, damContext,req->logicalLinkHandle, pal_dam_link_delete_callback);
    }
    else
    {
        call_paldata_pal_ctrl_link_delete_cfm(context, req->appHandle, req->logicalLinkHandle, req->physicalLinkHandle);
    }
}

static void ready__pal_ctrl_failed_contact_counter_reset_req(FsmContext *context, const PalCtrlFailedContactCounterResetReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;

        (void)pal_dam_reset_failed_contact_counter(damContext,req->logicalLinkHandle);
#ifdef FSM_DEBUG_DUMP
        pal_dm_dump(damContext);
#endif
    }
    call_paldata_pal_ctrl_failed_contact_counter_reset_cfm(context, req->appHandle, req->logicalLinkHandle);
}

static void ready__pal_ctrl_failed_contact_counter_read_req(FsmContext *context, const PalCtrlFailedContactCounterReadReq_Evt *req)
{
    CsrUint16 failedContactCounter=0;
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;

        (void)pal_dam_read_failed_contact_counter(damContext,req->logicalLinkHandle, &failedContactCounter);
#ifdef FSM_DEBUG_DUMP
        pal_dm_dump(damContext);
#endif
    }
    call_paldata_pal_ctrl_failed_contact_counter_read_cfm(context, req->appHandle, req->logicalLinkHandle,failedContactCounter);
}

static void ready__pal_ctrl_link_supervision_timeout_set_req(FsmContext *context, const PalCtrlLinkSupervisionTimeoutSetReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;
        CsrBool firstPhyLink;

        sme_trace_entry((TR_PAL_DAM, "ready__pal_ctrl_set_link_supervision_timeout()"));
        pal_dam_set_link_supervision_timeout(context, damContext, req->physicalLinkHandle, req->linkSupervisionTimeout, &firstPhyLink);

        /* if this is the first link subscribe to ma unitdata*/
        if (TRUE == firstPhyLink)
        {
            CsrUint32 oui = 0x001958;
            call_paldata_sys_ma_unitdata_subscribe_req(context,PALDATA_APP_HANDLE(context),unifi_Llc_Snap,PALDAM_DATA_PROTO_L2CAP_ID,oui);
            FSMDATA->pendingPhysicalLinkHandle = req->physicalLinkHandle;
        }
        else
        {
            call_paldata_pal_ctrl_link_supervision_timeout_set_cfm(context, req->appHandle, req->physicalLinkHandle);
        }

    }
    else
    {
        call_paldata_pal_ctrl_link_supervision_timeout_set_cfm(context, req->appHandle, req->physicalLinkHandle);
    }
}

static void ready__pal_ctrl_link_supervision_timeout_modify_req(FsmContext *context, const PalCtrlLinkSupervisionTimeoutModifyReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;
        sme_trace_entry((TR_PAL_DAM, "ready__pal_ctrl_modify_link_supervision_timeout()"));
        pal_dam_modify_link_supervision_timeout(context, damContext, req->physicalLinkHandle, req->linkSupervisionTimeout);
    }
    call_paldata_pal_ctrl_link_supervision_timeout_modify_cfm(context, req->appHandle, req->physicalLinkHandle);
}

static void ready__pal_ctrl_link_supervision_timeout_delete_req(FsmContext *context, const PalCtrlLinkSupervisionTimeoutDeleteReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;
        CsrBool lastPhyLink;

        sme_trace_entry((TR_PAL_DAM, "ready__pal_ctrl_delete_link_supervision_timeout()"));
        pal_dam_delete_link_supervision_timeout(context, damContext, req->physicalLinkHandle, &lastPhyLink);

        /* if this is the last link unsubscribe ma unitdata*/
        if (TRUE == lastPhyLink)
        {
            call_paldata_sys_ma_unitdata_unsubscribe_req(context,PALDATA_APP_HANDLE(context),FSMDATA->subscriptionHandle);
        }
    }
    call_paldata_pal_ctrl_link_supervision_timeout_delete_cfm(context, req->appHandle, req->physicalLinkHandle);
}

static void ready__pal_ctrl_link_alive_req(FsmContext *context, const PalCtrlLinkAliveReq_Evt *req)
{
    if (req->appHandle == FSMDATA->appHandle)
    {
        dataManagerContext *damContext = FSMDATA->damContext;
        sme_trace_entry((TR_PAL_DAM, "pal_ctrl_link_alive()"));
        pal_dam_link_alive(context, damContext, req->physicalLinkHandle);
    }
}

static void ready__pal_acl_data_req(FsmContext *context, const PalAclDataReq_Evt *req)
{
    /* The length needs to be atleast ACL Header+1 byte of data */
    if (req->dataLength > PALDATA_ACL_HEADER_SIZE &&
        req->aclOffset == PALDATA_ACL_OFFSET)
    {
        dataManagerContext *damContext = FSMDATA->damContext;
        PalAclheader aclHeader;
        PAL_DataBlock *dataBlock = (PAL_DataBlock *)req->data;
        CsrUint8 *acldata = PALDATA_MEM_DOWNSTREAM_GET_ACL_BUFFER(req->data, PALDATA_ACL_OFFSET);

        aclHeader.handlePlusFlags = event_unpack_CsrUint16(&acldata);
        aclHeader.length = event_unpack_CsrUint16(&acldata);

        /* skip acl header from the buffer */
        acldata = PALDATA_MEM_DOWNSTREAM_REMOVE_ACL_HEADER(dataBlock,PALDATA_ACL_HEADER_SIZE);
        (void)pal_acl_process_downstream_data(context, damContext, dataBlock, &aclHeader, req->freeFunction);
    }
    else
    {
        sme_trace_warn((TR_PAL_DAM,"ready__pal_dam_acl_data_req: zero length buffer for PAL-DAM-ACL-DATA-IND"));
        PALDATA_MEM_FREE_DATA_BUFFER(req->freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(req->data));
        PALDATA_MEM_DOWNSTREAM_FREE_DATA_BLOCK(req->data);

    }
}

static void ready__paldata_sys_ma_unitdata_ind(FsmContext *context, const PaldataSysMaUnitdataInd_Evt *ind)
{

    sme_trace_info((TR_PAL_DAM, "ready__paldata_sys_ma_unitdata_ind():"));

    if (ind->frame && unifi_ReceptionStatusRxSuccess == ind->receptionStatus)
    {
        dataManagerContext *damContext = FSMDATA->damContext;

        if (unifi_ReceptionStatusRxSuccess == ind->receptionStatus)
        {
            pal_acl_process_upstream_data(context,
                                          damContext,
                                          (PAL_DataBlock *)ind->frame,
                                          ind->freeFunction);
        }
        else
        {
            PALDATA_MEM_FREE_DATA_BUFFER(ind->freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(ind->frame));
            PALDATA_MEM_UPSTREAM_FREE_DATA_BLOCK(ind->frame);
        }
    }
    call_paldata_sys_ma_unitdata_rsp(context,ind->subscriptionHandle,unifi_Success);
}

static void ready__paldata_sys_ma_unitdata_cfm(FsmContext *context, const PaldataSysMaUnitdataCfm_Evt *cfm)
{
    dataManagerContext *damContext = FSMDATA->damContext;

    sme_trace_info((TR_PAL_DAM, "ready__paldata_sys_ma_unitdata_cfm(): status ind with status-%d, hosttag-%d,priority-%d,serClass-%d\n",cfm->transmissionStatus,cfm->reqIdentifier,cfm->providedPriority,cfm->providedServiceClass));
    (void)pal_dam_process_ma_unitdata_cfm(context, 
                                    damContext, 
                                    cfm->result, 
                                    cfm->transmissionStatus, 
                                    cfm->providedPriority, 
                                    cfm->providedServiceClass, 
                                    cfm->reqIdentifier);
}

static void pal_dam_init(FsmContext* context)
{
    dataManagerFsmContext *fsmData;
    sme_trace_entry((TR_PAL_MGR_FSM, "pal_dam_init()"));
    fsm_create_params(context, dataManagerFsmContext);

    fsmData = FSMDATA;

    pal_dam_data_init(&fsmData->damContext);
    fsmData->appHandle = NULL;
    fsm_next_state(context, FSMSTATE_wait_for_startup);
}

#ifdef FSM_DEBUG_DUMP
static void pal_dam_dump(FsmContext* context, const CsrUint16 id)
{
    dataManagerFsmContext *fsmData = fsm_get_params_by_id(context, id, dataManagerFsmContext);

    pal_dm_dump(fsmData->damContext);
}
#endif

static void pal_dam_reset(FsmContext* context)
{
    dataManagerContext *damContext = FSMDATA->damContext;

#ifdef FSM_DEBUG_DUMP
    pal_dm_dump(damContext);
#endif
    pal_dam_deinit(context, damContext, TRUE);
}

static const FsmEventEntry readyTransitions[] =
{
    /* Signal Id,                                                 Function */
    /* Keep this events in the begining to save lookup time */
    fsm_event_table_entry(PAL_ACL_DATA_REQ_ID, ready__pal_acl_data_req),
    fsm_event_table_entry(PALDATA_SYS_MA_UNITDATA_CFM_ID, ready__paldata_sys_ma_unitdata_cfm),
    fsm_event_table_entry(PALDATA_SYS_MA_UNITDATA_IND_ID, ready__paldata_sys_ma_unitdata_ind), /*event carries ma-unitdata-ind */

    /* Internal events for Flush timer handling */
    fsm_event_table_entry(PAL_DAM_EARLY_LINK_LOSS_TIMER_ID, ready__pal_dam_early_link_loss_timer_expired),
    fsm_event_table_entry(PAL_DAM_LINK_LOSS_TIMER_ID, ready__pal_dam_link_loss_timer_expired),
    fsm_event_table_entry(PAL_DAM_FLUSH_TIMER_ID, ready__pal_dam_flush_timer_expired),

    fsm_event_table_entry(PAL_CTRL_LINK_CREATE_REQ_ID,           ready__pal_dam_link_create_req),
    fsm_event_table_entry(PAL_CTRL_LINK_DELETE_REQ_ID,           ready__pal_dam_link_delete_req),
    fsm_event_table_entry(PAL_CTRL_LINK_MODIFY_REQ_ID,           ready__pal_dam_link_modify_req),
    fsm_event_table_entry(PAL_CTRL_LINK_FLUSH_REQ_ID,           ready__pal_ctrl_link_flush_req),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_READ_REQ_ID, ready__pal_ctrl_failed_contact_counter_read_req),
    fsm_event_table_entry(PAL_CTRL_FAILED_CONTACT_COUNTER_RESET_REQ_ID, ready__pal_ctrl_failed_contact_counter_reset_req),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_SET_REQ_ID, ready__pal_ctrl_link_supervision_timeout_set_req),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_MODIFY_REQ_ID, ready__pal_ctrl_link_supervision_timeout_modify_req),
    fsm_event_table_entry(PAL_CTRL_LINK_SUPERVISION_TIMEOUT_DELETE_REQ_ID, ready__pal_ctrl_link_supervision_timeout_delete_req),
    fsm_event_table_entry(PAL_CTRL_LINK_ALIVE_REQ_ID, ready__pal_ctrl_link_alive_req),
    fsm_event_table_entry(PAL_CTRL_DEACTIVATE_REQ_ID, ready__pal_ctrl_deactivate_req),
    fsm_event_table_entry(PAL_CTRL_EVENT_MASK_SET_REQ_ID, ready__pal_ctrl_event_mask_set_req),

    fsm_event_table_entry(PALDATA_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID, ready__paldata_sys_ma_unitdata_subscribe_cfm),
};

static const FsmEventEntry waitForStartupTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PAL_CTRL_ACTIVATE_REQ_ID, wait_for_startup__pal_ctrl_activate_req),
    fsm_event_table_entry(PALDATA_SYS_CAPABILITIES_CFM_ID, wait_for_startup__paldata_sys_capabilities_cfm),
    fsm_event_table_entry(PAL_ACL_DATA_REQ_ID, wait_for_startup__pal_acl_data_req),
    fsm_event_table_entry(PALDATA_SYS_MA_UNITDATA_IND_ID, wait_for_startup__paldata_sys_ma_unitdata_ind),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(PALDATA_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID, fsm_ignore_event),
};

/** Profile Storage state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                       Name                    Transitions                          *       */
   fsm_state_table_entry(FSMSTATE_wait_for_startup, waitForStartupTransitions,   FALSE),
   fsm_state_table_entry(FSMSTATE_ready, readyTransitions,   FALSE),
};

const FsmProcessStateMachine pal_dam_fsm =
{
#ifdef FSM_DEBUG
    "PAL DAM",                                  /* Process Name       */
#endif
    PAL_DAM_PROCESS,                                /* Process ID         */
    {FSMSTATE_MAX_STATE, stateTable},                         /* Transition Tables  */
    fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE),   /* Handled event handers */
    fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                    /* ignore event handers */
    pal_dam_init,                                    /* Entry Function     */
    pal_dam_reset,                                                               /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
    pal_dam_dump                                                             /* Trace Dump Function   */
#endif
};


/************************************** PUBLIC FUNCTIONS *******************************************/
/**
 * @brief
 *   PAL data manager Entry Function
 *
 * @par Description
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */


/* see paldata.h for description */
void paldata_process_unitdata_ind(FsmContext* context,
                                       CsrUint8 subscriptionHandle,
                                       CsrUint16 frameLength,
                                       const CsrUint8 *frame,
                                       unifi_FrameFreeFunction freeFunction,
                                       unifi_ReceptionStatus receptionStatus,
                                       unifi_Priority priority,
                                       unifi_ServiceClass serviceClass)
{
#ifdef CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE
#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexLock(context->transitionLock);
#endif
#endif
#endif
    {
        FsmInstanceEntry* currentInstance = fsm_save_current_instance_by_id(context, getPalDataContext(context)->palDamFsmInstance);

        sme_trace_entry((TR_PAL_DAM, "paldata_process_unitdata_ind():"));
    
        if (frame)
        {
            dataManagerContext *damContext = FSMDATA->damContext;
            PAL_DataBlock *dataBlock = PALDATA_MEM_UPSTREAM_ALLOCATE_DATA_BLOCK(frameLength, NULL, frame, TRUE, NULL);

            if (unifi_ReceptionStatusRxSuccess == receptionStatus)
            {
                pal_acl_process_upstream_data(context,
                                              damContext,
                                              dataBlock,
                                              freeFunction);
            }
            else
            {
                PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(dataBlock));
                PALDATA_MEM_UPSTREAM_FREE_DATA_BLOCK(dataBlock);
            }
        }

        call_paldata_sys_ma_unitdata_rsp(context,subscriptionHandle,unifi_Success);

        fsm_restore_current_instance(context, currentInstance);
    }
#ifdef CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE
#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexUnlock(context->transitionLock);
#endif
#endif
#endif
}

/* see paldata.h for description */
CsrUint32 paldata_process_acl_data_req(FsmContext *context, 
                                        CsrUint16 dataLength, 
                                        CsrUint8  *data,
                                        CsrUint16 logicalLinkHandle,
                                        CsrUint16 aclOffset,
                                        unifi_FrameFreeFunction freeFunction)
{
    CsrUint32 nextTimeoutValue = 0;

#ifdef CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE
#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexLock(context->transitionLock);
#endif
#endif
#endif
    {
        PAL_DataBlock *dataBlock = PALDATA_MEM_ALLOCATE_DOWNSTREAM_DATA_BLOCK(dataLength+aclOffset, NULL, data, TRUE, NULL);
        FsmInstanceEntry* currentInstance = fsm_save_current_instance_by_id(context, getPalDataContext(context)->palDamFsmInstance);

        /* The length needs to be atleast ACL Header+1 byte of data */
        if (dataLength > PALDATA_ACL_HEADER_SIZE &&
            aclOffset == PALDATA_ACL_OFFSET)
        {
            dataManagerContext *damContext = FSMDATA->damContext;
            PalAclheader aclHeader;
            CsrUint8 *acldata = PALDATA_MEM_DOWNSTREAM_GET_ACL_BUFFER(dataBlock, PALDATA_ACL_OFFSET);
    
            aclHeader.handlePlusFlags = event_unpack_CsrUint16(&acldata);
            aclHeader.length = event_unpack_CsrUint16(&acldata);
    
            /* skip acl header from the buffer */
            acldata = PALDATA_MEM_DOWNSTREAM_REMOVE_ACL_HEADER(dataBlock,PALDATA_ACL_HEADER_SIZE);
            nextTimeoutValue = pal_acl_process_downstream_data(context, damContext, dataBlock, &aclHeader, freeFunction);
        }
        else
        {
            sme_trace_warn((TR_PAL_DAM,"paldata_process_acl_data_req: zero length buffer for PAL-DAM-ACL-DATA-IND"));
            PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(dataBlock));
            PALDATA_MEM_DOWNSTREAM_FREE_DATA_BLOCK(dataBlock);
    
        }
        fsm_restore_current_instance(context, currentInstance);
    }
#ifdef CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE
#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexUnlock(context->transitionLock);
#endif
#endif
#endif
    return nextTimeoutValue;
}

/* see paldata.h for description */
CsrUint32 paldata_process_unitdata_cfm(FsmContext *context,
                                        unifi_Status result,
                                        unifi_TransmissionStatus transmissionStatus,
                                        unifi_Priority providedPriority,
                                        unifi_ServiceClass providedServiceClass,
                                        CsrUint32 reqIdentifier)
{
    CsrUint32 nextTimeoutValue = 0;

#ifdef CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE
#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexLock(context->transitionLock);
#endif
#endif
#endif
    {
        dataManagerContext *damContext;
        FsmInstanceEntry* currentInstance = fsm_save_current_instance_by_id(context, getPalDataContext(context)->palDamFsmInstance);
        
        damContext = FSMDATA->damContext;
        sme_trace_info((TR_PAL_DAM, "paldata_process_unitdata_cfm(): status ind with status-%d, hosttag-%d,priority-%d,serClass-%d\n",transmissionStatus,reqIdentifier,providedPriority,providedServiceClass));
        nextTimeoutValue = pal_dam_process_ma_unitdata_cfm(context, 
                                        damContext, 
                                        result, 
                                        transmissionStatus, 
                                        providedPriority, 
                                        providedServiceClass, 
                                        reqIdentifier);

        fsm_restore_current_instance(context, currentInstance);
    }
#ifdef CSR_WIFI_PALDATA_SYNC_ACCESS_ENABLE
#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexUnlock(context->transitionLock);
#endif
#endif
#endif
    return nextTimeoutValue;
}

/** @}
 */
