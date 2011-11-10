/** @file qos_fsm.c
 *
 * Public SME quality of service FSM API
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
 *   Public SME quality of service FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_fsm.c#7 $
 *
 ****************************************************************************/

/** @{
 * @ingroup qos
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "smeio/smeio_trace_types.h"

#include "qos_fsm/qos_tspec.h"
#include "qos_fsm/qos_tclas.h"
#include "qos_fsm/qos_block_ack.h"
#include "qos_fsm/qos_action_sub_fsm.h"

#include "network_selector_fsm/network_selector_fsm.h"

#include "ie_access/ie_access.h"

#include "mgt_sap/mgt_sap_from_sme_interface.h"
#include "sys_sap/sys_sap_from_sme_interface.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Processes Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, FsmData))

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
    FSMSTATE_stopped,
    FSMSTATE_active,
    FSMSTATE_MAX_STATE
} FsmState;

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
/*lint -esym(754,FsmData::temp)*/
typedef struct FsmData
{
    qos_tspecDataBlk tspecDataBlock;

    qos_blockAckCtrlBlk blkAckCtrlBlk;

    qos_tspecData   *pCurrentActiveTspecData;
    CsrBool         internalRequesterId;
    void*           cfmAppHandle;

} FsmData;


/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/
static void mlme_addts_cfm(FsmContext* context, const MlmeAddtsCfm_Evt* cfm);

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void tspec_req_cleanup(FsmContext* context, const UnifiMgtTspecReq_Evt* req)
{
    sme_trace_entry((TR_QOS, "tspec_req_cleanup:" ));

    /* default behavior, free up any payloads */
    CsrPfree(req->tspec);
    CsrPfree(req->tclas);
}

static void tspec_cfm_cleanup(FsmContext* context, const MlmeAddtsCfm_Evt* cfm)
{
    sme_trace_entry((TR_QOS, "tspec_cfm_cleanup:" ));

    /* default behavior, free up any payloads */
    if (cfm->informationElements.dataLength != 0)
    {
        pld_release(getPldContext(context), cfm->informationElements.slotNumber);
    }

}

/**
 * @brief
 *   FSM data initialisation
 *
 * @par Description
 *   FSM data initialisation
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void init_sme_qos_data(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_QOS, "init_sme_qos_data()"));

    qos_initialise_tspec_handler(&fsmData->tspecDataBlock);
    qos_init_block_ack(context, &fsmData->blkAckCtrlBlk);

    fsmData->internalRequesterId = TRUE;
    fsmData->pCurrentActiveTspecData = NULL;
}

/**
 * @brief
 *   SME configuration FSM Entry Function
 *
 * @par Description
 *   Called on SME configuration Process startup to initialise
 *   the security manager data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void sme_qos_init(FsmContext* context)
{
    sme_trace_entry((TR_QOS, "sme_qos_init()"));

    fsm_create_params(context, FsmData);

    init_sme_qos_data(context);

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   SME QOS FSM Entry Function
 *
 * @par Description
 *   Called on SME QOS Process startup to initialise
 *   the QOS data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void sme_qos_reset(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_QOS, "sme_qos_reset()"));

    /* clean up the lists */
    qos_reset_tspec_handler(&fsmData->tspecDataBlock);
    qos_reset_block_ack(context, &fsmData->blkAckCtrlBlk);

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   SME QOS FSM Entry Function
 *
 * @par Description
 *   Called on SME QOS Process startup to initialise
 *   the QOS data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static unifi_TspecResultCode ValidateRequest(FsmContext* context, const UnifiMgtTspecReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    unifi_TspecResultCode tspecResultCode;
    qos_tspecData *pTspecData;

    /* check the tspec to ensure it is correctly formatted */
    if ((tspecResultCode = validate_tspec_content(context, req->tspecLength, req->tspec)) != unifi_TspecResultSuccess)
    {
        sme_trace_warn((TR_QOS, "tspec rejected, invalid"));
        return tspecResultCode;
    }
    if ((tspecResultCode = validate_tclas(context, req->tclasLength, req->tclas)) != unifi_TspecResultSuccess)
    {
        sme_trace_warn((TR_QOS, "tclas rejected, invalid"));
        return tspecResultCode;
    }

    /* see if this is a direct tspec replacement */
    pTspecData = qos_get_tspec_data_by_transaction_id(&fsmData->tspecDataBlock, req->transactionId);


    if ( (pTspecData == NULL)
       &&((tspecResultCode = validate_tspec_position(context, &fsmData->tspecDataBlock, req->tspecLength, req->tspec)) != unifi_TspecResultSuccess) )
    {
        sme_trace_warn((TR_QOS, "tspec rejected, already installed"));
        return tspecResultCode;
    }

    return tspecResultCode;
}

/*****************************************************************************
 *                       STOPPED TRANSITIONS
 *****************************************************************************/

/**
 * @brief
 *   Process has been ordered to start.
 *
 * @par Description
 *   A CORE_START_REQ has been received.  Ensure data is set to known values
 *
 * @param[in]    context : FSM context
 * @param[in]    req     : content of the CoreStopReq
 *
 * @return
 *  Void
 */
static void core_start(FsmContext *context, const CoreStartReq_Evt *req)
{
    sme_trace_entry((TR_QOS, "core_start()"));
    send_core_start_cfm(context, req->common.sender_, unifi_Success);

    /* flush all tspecs */
    init_sme_qos_data(context);

    fsm_next_state(context, FSMSTATE_active);
}

/*****************************************************************************
 *                       ACTIVE TRANSITIONS
 *****************************************************************************/
/**
 * @brief
 *   A CORE_STOP_REQ was received while disconnected, nothing to do other
 *   than change state.
 *
 * @param[in]    context            : FSM context
 * @param[in]    request            : content of the CoreStopReq
 *
 * @return
 *  Void
 */
static void core_stop(FsmContext *context, const CoreStopReq_Evt *req)
{
    sme_trace_entry((TR_QOS, "core_stop()"));

    send_core_stop_cfm(context, req->common.sender_, unifi_Success);

    /* flush all tspecs */
    sme_qos_reset(context);

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   "*" State MaUnitdata transition
 *
 * @par Description
 *   Fallback for spurious / missed MaUnitdata's
 *   Makes sure the payload is deleted.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : CoreStopReq_Evt
 *
 * @return
 *   void
 */
static void qos_disassociated_ind(FsmContext* context, const QosDisassociatedInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    SmeConfigData* cfg = get_sme_config(context);
    qos_tspecData *pCurrentTspecData;
    void* appHandles;
    CsrUint16 appHandleCount = get_appHandles(context, unifi_IndTspec, &appHandles);

    sme_trace_entry((TR_QOS, "qos_disassociated_ind"));
    pCurrentTspecData = qos_get_first_active_tspec(&fsmData->tspecDataBlock, TSPEC_REJECTED_ANY);

    if (appHandleCount && pCurrentTspecData != NULL)
    {
        do
        {
            call_unifi_mgt_tspec_ind(context, appHandleCount, appHandles, pCurrentTspecData->transactionId, pCurrentTspecData->resultCode, 0, NULL);

            pCurrentTspecData = qos_get_next_active_tspec(&fsmData->tspecDataBlock);
        }
        while(pCurrentTspecData != NULL);
    }

    /* Always request the block Acknowledgment to be disabled */
    qos_process_del_block_ack_req(context, &fsmData->blkAckCtrlBlk, cfg->connectionInfo.bssid, 0, ReasonCode_EndBa);

    /* clear the active list */
    clear_active_tspec_list(&fsmData->tspecDataBlock);
}

static void tclas_add_cfm(FsmContext* context, const UnifiSysTclasAddCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    void* appHandles;
    CsrUint16 appHandleCount = get_appHandles(context, unifi_IndTspec, &appHandles);

    sme_trace_entry((TR_QOS, "tclas_add_cfm: %d ", cfm->status));
    if(cfm->status == unifi_Success)
    {
#if 0
        SmeConfigData* cfg = get_sme_config(context);
        /* do not tie in the block ack to the TSPECs */
        qos_process_add_block_ack_req(context, &fsmData->blkAckCtrlBlk, cfg->connectionInfo.bssid, fsmData->pCurrentActiveTspecData->tid);
#endif

        if (appHandleCount)
        {
            call_unifi_mgt_tspec_ind(context, appHandleCount, appHandles, fsmData->pCurrentActiveTspecData->transactionId, unifi_TspecResultInstalled, 0, NULL);
        }
    }
    else if (appHandleCount)
    {
        call_unifi_mgt_tspec_ind(context, appHandleCount, appHandles, fsmData->pCurrentActiveTspecData->transactionId, unifi_TspecResultInvalidTclasParameters, 0, NULL);
    }

    fsmData->pCurrentActiveTspecData = qos_get_next_active_tspec(&fsmData->tspecDataBlock);
    if (fsmData->pCurrentActiveTspecData != NULL)
    {
        DataReference tspec_DR = {0, IE_WMM_TSPEC__TOTAL_SIZE};

        pld_store(getPldContext(context),
                fsmData->pCurrentActiveTspecData->tspec,
                IE_WMM_TSPEC__TOTAL_SIZE,
                &tspec_DR.slotNumber);

        qos_add_tspec_fsm_start(context, mlme_addts_cfm, tspec_DR, fsmData->pCurrentActiveTspecData->dialogToken);
        /* The FSM will never return here */
    }

}

/**
 * @brief
 *   "*" State MaUnitdata transition
 *
 * @par Description
 *   Fallback for spurious / missed MaUnitdata's
 *   Makes sure the payload is deleted.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : CoreStopReq_Evt
 *
 * @return
 *   void
 */
static void mlme_addts_cfm(FsmContext* context, const MlmeAddtsCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    void* appHandles;
    CsrUint16 appHandleCount = get_appHandles(context, unifi_IndTspec, &appHandles);

    sme_trace_entry((TR_QOS, "mlme_addts_cfm: %d %s",
                                            cfm->resultCode,
                                            trace_ResultCode(cfm->resultCode)));

    if (cfm->informationElements.dataLength != 0)
    {
        sme_trace_debug((TR_QOS, "payload present"));
    }

    switch (cfm->resultCode)
    {
    case ResultCode_Success:
    {

        qos_tspec_update_tspec_status(&fsmData->tspecDataBlock, fsmData->pCurrentActiveTspecData->tid, TSPEC_ACTIVE);

        if (fsmData->pCurrentActiveTspecData->tclasLength == 0)
        {
#if 0
            /* do not tie in the block ack to the TSPECs */
            SmeConfigData* cfg = get_sme_config(context);
            qos_process_add_block_ack_req(context, &fsmData->blkAckCtrlBlk, cfg->connectionInfo.bssid, fsmData->pCurrentActiveTspecData->tid);
#endif

            if (appHandleCount)
            {
                call_unifi_mgt_tspec_ind(context, appHandleCount, appHandles, fsmData->pCurrentActiveTspecData->transactionId, unifi_TspecResultInstalled, 0, NULL);
            }
        }
        else
        {
            /* Clean message before entering the sub FSM */
            tspec_cfm_cleanup(context, cfm);

            qos_add_tclas_fsm_start(context, tclas_add_cfm,
                                    fsmData->pCurrentActiveTspecData->tclasLength,
                                    fsmData->pCurrentActiveTspecData->tclas);
            /* The FSM will never return here */
            return;
        }

        break;
    }
    case ResultCode_Timeout:
    case ResultCode_RejectedForDelayPeriod:
    {
        qos_tspec_update_tspec_status(&fsmData->tspecDataBlock, fsmData->pCurrentActiveTspecData->tid, TSPEC_IDLE);

        if (appHandleCount)
        {
            call_unifi_mgt_tspec_ind(context, appHandleCount, appHandles, fsmData->pCurrentActiveTspecData->transactionId, unifi_TspecResultRejectedForDelayPeriod, 0, NULL);
        }
        break;
    }
    case ResultCode_RejectedWithSuggestedTspecChanges:
    {
        unifi_DataBlock newTspec;
        CsrUint8 *pBuf;

        /* access the suggested data reference, this is in the wrong format */
        pld_access(getPldContext(context), cfm->informationElements.slotNumber, (void**)&pBuf, &newTspec.length);

        /* allocate a new buffer */
        newTspec.data = CsrPmalloc(newTspec.length);
        /* and copy the data into it.. */
        CsrMemCpy(newTspec.data, pBuf, newTspec.length);


        sme_trace_info( (TR_QOS, "newTspec.length= %d", newTspec.length ) );
        sme_trace_hex( (TR_QOS, TR_LVL_INFO, "pBuf :: ", pBuf, newTspec.length) );

        sme_trace_hex( (TR_QOS, TR_LVL_INFO, "newTspec.data :: ", newTspec.data, newTspec.length) );

        /* has the SME been given free reign to retry? */
        if( (fsmData->pCurrentActiveTspecData->strict)
          ||(fsmData->pCurrentActiveTspecData->suggestTspecCount > 0))
        {
            qos_tspec_update_tspec_status(&fsmData->tspecDataBlock, fsmData->pCurrentActiveTspecData->tid, TSPEC_IDLE);
            fsmData->pCurrentActiveTspecData->suggestTspecCount = 0;

            if (appHandleCount)
            {
                call_unifi_mgt_tspec_ind(context, appHandleCount, appHandles, fsmData->pCurrentActiveTspecData->transactionId, unifi_TspecResultRejectedWithSuggestedChanges, newTspec.length, newTspec.data);
            }

            /* free the tspec data, it has been sent.. */
            CsrPfree(newTspec.data);
        }
        else
        {
            if (appHandleCount)
            {
                call_unifi_mgt_tspec_ind(context, appHandleCount, appHandles, fsmData->pCurrentActiveTspecData->transactionId, unifi_TspecResultRejectedWithSuggestedChanges, newTspec.length, newTspec.data);
            }

            sme_trace_debug((TR_QOS, "suggested tspec sent"));

            /* remove the outstanding tspec */
            CsrPfree(fsmData->pCurrentActiveTspecData->tspec);

            /* allocate a new buffer */
            fsmData->pCurrentActiveTspecData->tspec = CsrPmalloc(newTspec.length);
            /* and copy the data into it.. */
            CsrMemCpy(fsmData->pCurrentActiveTspecData->tspec, newTspec.data, newTspec.length);
            /* free the tspec data */
            CsrPfree(newTspec.data);

            /* this is not a new additions so locate a new dialogtoken manually */
            fsmData->pCurrentActiveTspecData->dialogToken = qos_get_dialog_token(&fsmData->tspecDataBlock);
            fsmData->pCurrentActiveTspecData->suggestTspecCount++;

            qos_add_tspec_fsm_start(context, mlme_addts_cfm, cfm->informationElements, fsmData->pCurrentActiveTspecData->dialogToken);
            /* The FSM will never return here */

            return;
        }

        break;
    }
    default:
    {
        qos_tspec_update_tspec_status(&fsmData->tspecDataBlock, fsmData->pCurrentActiveTspecData->tid, TSPEC_IDLE);
        if (appHandleCount)
        {
            call_unifi_mgt_tspec_ind(context, appHandleCount, appHandles, fsmData->pCurrentActiveTspecData->transactionId, unifi_TspecResultUnspecifiedFailure, 0, NULL);
        }
        break;
    }
    }

    /* always free up the message */
    tspec_cfm_cleanup(context, cfm);

    fsmData->pCurrentActiveTspecData = qos_get_next_active_tspec(&fsmData->tspecDataBlock);
    if (fsmData->pCurrentActiveTspecData != NULL)
    {
        DataReference tspec_DR = {0, IE_WMM_TSPEC__TOTAL_SIZE};

        pld_store(getPldContext(context),
                fsmData->pCurrentActiveTspecData->tspec,
                IE_WMM_TSPEC__TOTAL_SIZE,
                &tspec_DR.slotNumber);

        qos_add_tspec_fsm_start(context, mlme_addts_cfm, tspec_DR, fsmData->pCurrentActiveTspecData->dialogToken);
        /* The FSM will never return here */
    }
}

static void mlme_delts_cfm(FsmContext* context, const MlmeDeltsCfm_Evt* cfm)
{
    /*
    SmeConfigData* cfg = get_sme_config(context);
    */

    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_QOS, "mlme_delts_cfm: %d %s",
                                            cfm->resultCode,
                                            trace_ResultCode(cfm->resultCode)));

    if (fsmData->internalRequesterId != TRUE)
    {
        if(cfm->resultCode == ResultCode_Success)
        {
            call_unifi_mgt_tspec_cfm(context, fsmData->cfmAppHandle, unifi_Success, fsmData->pCurrentActiveTspecData->transactionId, unifi_TspecResultSuccess, 0, NULL);
        }
        else
        {
            /* note, at this point any error will be blindly passed back. */
            call_unifi_mgt_tspec_cfm(context, fsmData->cfmAppHandle, cfm->resultCode, fsmData->pCurrentActiveTspecData->transactionId, unifi_TspecResultFailure, 0, NULL);
        }
    }

    (void) qos_get_tspec_data_by_transaction_id(&fsmData->tspecDataBlock, fsmData->pCurrentActiveTspecData->transactionId);

#if 0
    /* do not tie in the block ack to the TSPECs */
    qos_process_del_block_ack_req(context, &fsmData->blkAckCtrlBlk, cfg->connectionInfo.bssid, pTspecData->tid, ReasonCode_EndBa);
#endif

    /* need to look up stored TSPEC */
    qos_delete_tspec_data_by_transaction_id(&fsmData->tspecDataBlock, fsmData->pCurrentActiveTspecData->transactionId);
}

static void tspec_not_present(FsmContext* context, const UnifiMgtTspecReq_Evt* req)
{
    unifi_Status  unifiStatus = unifi_Unavailable;
    unifi_TspecResultCode TspecResultCode = unifi_TspecResultNotPresent;

    sme_trace_info((TR_QOS, "tspec_not_present: transID 0x%x action %s",  req->transactionId, trace_unifi_ListAction(req->action)));

    /* Cleanup the Message */
    CsrPfree(req->tclas);
    CsrPfree(req->tspec);

    call_unifi_mgt_tspec_cfm(context, req->appHandle, unifiStatus, req->transactionId, TspecResultCode, 0, NULL);
}

static void tspec_req(FsmContext* context, const UnifiMgtTspecReq_Evt* req)
{
    SmeConfigData* cfg = get_sme_config(context);
    FsmData* fsmData = FSMDATA;

    unifi_Status  unifiStatus = unifi_Success;
    unifi_TspecResultCode TspecResultCode = unifi_TspecResultSuccess;
    CsrBool confirmRequired = TRUE;
    CsrBool cleanupRequired = TRUE;

    sme_trace_info((TR_QOS, "tspec_req: transID 0x%x action %s",  req->transactionId, trace_unifi_ListAction(req->action)));

    if (isAccessRestricted(context, req->appHandle))
    {
        unifiStatus = unifi_Restricted;
        TspecResultCode = unifi_TspecResultUnspecifiedFailure;
    }
    else
    {
        switch(req->action)
        {
        case unifi_ListActionAdd:
        {
            qos_tspecData *pTspecData;

            /* first check the tspecs is properly formed and not already allocated */
            TspecResultCode = ValidateRequest(context, req);

            sme_trace_info((TR_QOS, "tspec_req: VALIDATE unifiStatus, %d TspecResultCode %d , req->transactionId %d", unifiStatus, TspecResultCode, req->transactionId));

            if (TspecResultCode == unifi_TspecResultSuccess)
            {
                pTspecData = qos_get_tspec_data_by_transaction_id(&fsmData->tspecDataBlock, req->transactionId);
                cleanupRequired = FALSE;

                /* check if there is already one present */
                if(pTspecData != NULL)
                {
                    if(ns_get_connection_status(context) == ns_ConnectionStatus_Connected)
                    {
                        fsmData->pCurrentActiveTspecData = pTspecData;
                        fsmData->internalRequesterId = TRUE;

                        /* This Saves the event for later processing */
                        fsm_saved_event(context, (const FsmEvent*)req);

                        /* remove the current tspec from active use,
                         * the cfm function will negotiate the removal of the from the active list and
                         * delete the stored copy */
                        qos_del_tspec_fsm_start(context, mlme_delts_cfm, pTspecData->tsInfo);
                        /* The FSM will never return here */

                        return;
                    }
                    else
                    {
                        /* delete the old TSPEC, the replacement will be added */
                        qos_delete_tspec_data_by_transaction_id(&fsmData->tspecDataBlock, pTspecData->transactionId);
                    }
                }

                sme_trace_info((TR_QOS, "tspec_req: valid request"));

                pTspecData = qos_add_tspec_data(&fsmData->tspecDataBlock, req);

                if(ns_get_connection_status(context) == ns_ConnectionStatus_Connected)
                {
                    DataReference tspec_DR = {0, IE_WMM_TSPEC__TOTAL_SIZE};

                    /* indicate the active tspec */
                    fsmData->pCurrentActiveTspecData = pTspecData;

                    pld_store(getPldContext(context),
                            fsmData->pCurrentActiveTspecData->tspec,
                            IE_WMM_TSPEC__TOTAL_SIZE,
                            &tspec_DR.slotNumber);

                    qos_tspec_update_tspec_status(&fsmData->tspecDataBlock, pTspecData->tid, TSPEC_PENDING);

                    qos_add_tspec_fsm_start(context, mlme_addts_cfm, tspec_DR, pTspecData->dialogToken);
                    /* The FSM will never return here */
                }
                else
                {
                    qos_tspec_update_tspec_status(&fsmData->tspecDataBlock, pTspecData->tid, TSPEC_IDLE);
                }
            }
            else
            {
                unifiStatus = unifi_Error;
            }
            break;
        }
        case unifi_ListActionRemove:
        {

            qos_tspecData *pTspecData = qos_get_tspec_data_by_transaction_id(&fsmData->tspecDataBlock, req->transactionId);

            if(pTspecData == NULL)
            {
                sme_trace_warn((TR_QOS, "Unrecognized transaction ID"));
                unifiStatus = unifi_Error;
                TspecResultCode = unifi_TspecResultInvalidTransactionID;
            }
            else
            {
                /* if we are connected then delete */
                if( (ns_get_connection_status(context) == ns_ConnectionStatus_Connected)
                  &&(cfg->WMMAssociation == TRUE))
                {
                    fsmData->pCurrentActiveTspecData = pTspecData;
                    fsmData->internalRequesterId = FALSE;
                    confirmRequired = FALSE;
                    cleanupRequired = FALSE;

                    fsmData->cfmAppHandle = req->appHandle;

                    /* clean up before SUB FSM is called */
                    tspec_req_cleanup(context, req);

                    qos_del_tspec_fsm_start(context, mlme_delts_cfm, pTspecData->tsInfo);
                    /* The FSM will never return here */

                }
                else
                {
                    /* need to look up stored TSPEC */
                    qos_delete_tspec_data_by_transaction_id(&fsmData->tspecDataBlock, pTspecData->transactionId);
                }
            }

            break;
        }
        case unifi_ListActionFlush:
        case unifi_ListActionGet:
        {
            unifiStatus = unifi_Unsupported;
            TspecResultCode = unifi_TspecResultFailure;
            break;
        }
        default:
        {
            unifiStatus = unifi_Error;
            TspecResultCode = unifi_TspecResultFailure;
            break;
        }

        }
    }

    if (confirmRequired == TRUE)
    {
        call_unifi_mgt_tspec_cfm(context, req->appHandle, unifiStatus, req->transactionId, TspecResultCode, 0, NULL);
    }

    if (cleanupRequired == TRUE)
    {
        tspec_req_cleanup(context, req);
    }
}

static void qos_associated_ind(FsmContext* context, const QosAssociatedInd_Evt* ind)
{
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_QOS, "qos_associated_ind"));

    /* new association, check the BSSID */
    sme_trace_debug((TR_QOS, "%s",
                    trace_unifi_MACAddress(cfg->connectionInfo.bssid, getMacAddressBuffer(context))));

    if(cfg->WMMAssociation == TRUE)
    {
        FsmData* fsmData = FSMDATA;
        qos_tspec_update_associate_tspec_status(&fsmData->tspecDataBlock, TSPEC_ACTIVE);

        fsmData->pCurrentActiveTspecData = qos_get_first_active_tspec(&fsmData->tspecDataBlock, TSPEC_IDLE);

        if (fsmData->pCurrentActiveTspecData != NULL)
        {
            DataReference tspec_DR = {0, IE_WMM_TSPEC__TOTAL_SIZE};

            sme_trace_debug((TR_QOS, "Kick off the SUB FSM" ));

            pld_store(getPldContext(context),
                    fsmData->pCurrentActiveTspecData->tspec,
                    IE_WMM_TSPEC__TOTAL_SIZE,
                    &tspec_DR.slotNumber);

            qos_add_tspec_fsm_start(context, mlme_addts_cfm, tspec_DR, fsmData->pCurrentActiveTspecData->dialogToken);
            /* The FSM will never return here */
        }

        /* Always request the block Acknowledgment to be enabled on BestEffort */
        qos_process_add_block_ack_req(context, &fsmData->blkAckCtrlBlk, cfg->connectionInfo.bssid, 0);
    }
    else
    {
        sme_trace_debug((TR_QOS, " Not a WMM Connection, do nothing" ));
    }
}


static void mlme_delts_ind(FsmContext* context, const MlmeDeltsInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    CsrUint8 tid;
    qos_tspecData* pTspecData;
    sme_trace_entry((TR_QOS, "mlme_delts_ind: ind->tsInfo %d", ind->tsInfo ));


    /* the tspec being deleted is identified via the tid in the tsifo */
    tid = (CsrUint8)IE_WMM_TS_INFO__TID_GET(ind->tsInfo);
    sme_trace_debug((TR_QOS, "mlme_delts_ind: tid[%d]",  tid));

    pTspecData = qos_get_tspec_data_by_tid(&fsmData->tspecDataBlock, tid);

    if (pTspecData != NULL)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndTspec, &appHandles);
        if (appHandleCount)
        {
            call_unifi_mgt_tspec_ind(context, appHandleCount, appHandles, pTspecData->transactionId, unifi_TspecResultTspecRemotelyDeleted, 0, NULL);
        }

        /* need to look up stored TSPEC */
        qos_delete_tspec_data_by_tid(&fsmData->tspecDataBlock, tid);

#ifdef CCX_VARIANT
        /* only inform the CCX fsm if there is an active tspec */
        send_ccx_ts_metrics_delts_ind(context, getSmeContext(context)->ccxInstance, tid);
#endif
    }
    else
    {
        sme_trace_debug((TR_QOS, "mlme_delts_ind: no tspec found, ignore"));
        /* ignore the request, no one to inform */
    }

}

static void mlme_addba_cfm(FsmContext* context, const MlmeAddbaCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_QOS, "mlme_addba_cfm: "));

    if(cfm->resultCode == ResultCode_Success)
    {
        /* there should never be a contention value.. but just incase */
        if((cfm->tid & 0x8000) == 0)
        {
            /* success and no errors */
            return;
        }
        sme_trace_error((TR_QOS, "mlme_addba_cfm: received a tid with the contention bit set" ));
    }

    /* addba failed remove the added block ack */
    remove_block_ack_from_outstanding(
            context,
            &fsmData->blkAckCtrlBlk,
            cfm->peerQstaAddress,
            (CsrUint8)cfm->tid);
}

static void mlme_delba_cfm(FsmContext* context, const MlmeDelbaCfm_Evt* cfm)
{
    sme_trace_entry((TR_QOS, "mlme_delba_cfm: " ));

    /* no nothing, consume */
}

static void mlme_delba_ind(FsmContext* context, const MlmeDelbaInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_QOS, "mlme_addba_cfm: " ));

    /* there should never be a contention value.. but just incase */
    if(ind->tid & 0x8000)
    {
        sme_trace_error((TR_QOS, "mlme_addba_cfm: received a tid with the contention bit set" ));
        return;
    }

    /* addba failed remove the added block ack */
    remove_block_ack_from_outstanding(
            context,
            &fsmData->blkAckCtrlBlk,
            ind->peerQstaAddress,
            (CsrUint8)ind->tid);
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in ccx/ccx_ie_access_qos.h
 */
/*---------------------------------------------------------------------------*/
qos_tspecDataBlk* qos_get_tspecdatablk_context(
        FsmContext* context)
{
    FsmData *fsmData = (fsm_get_params_by_id(context, getSmeContext(context)->qosInstance, FsmData));

    sme_trace_entry((TR_QOS, "qos_get_tspecdatablk_context() "));

    return &fsmData->tspecDataBlock;
}


/* FSM DEFINITION **********************************************/

/** State idle transitions */
static const FsmEventEntry stoppedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                      core_start),
    fsm_event_table_entry(QOS_DISASSOCIATED_IND_ID,               fsm_ignore_event),
    fsm_event_table_entry(UNIFI_MGT_TSPEC_REQ_ID,                 tspec_not_present),
};

/** State idle transitions */
static const FsmEventEntry activeTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(UNIFI_MGT_TSPEC_REQ_ID,                 tspec_req),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                       core_stop),
    fsm_event_table_entry(QOS_ASSOCIATED_IND_ID,                  qos_associated_ind),
    fsm_event_table_entry(QOS_DISASSOCIATED_IND_ID,               qos_disassociated_ind),
    fsm_event_table_entry(MLME_DELTS_IND_ID,                      mlme_delts_ind),

    fsm_event_table_entry(MLME_ADDBA_CFM_ID,                      mlme_addba_cfm),
    fsm_event_table_entry(MLME_DELBA_CFM_ID,                      mlme_delba_cfm),
    fsm_event_table_entry(MLME_ADDBA_IND_ID,                      fsm_ignore_event),
    fsm_event_table_entry(MLME_DELBA_IND_ID,                      mlme_delba_ind),
};


/** SME Measurement state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                       State                           State                       Save     */
   /*                       Name                            Transitions                  *       */
   fsm_state_table_entry(FSMSTATE_stopped,                  stoppedTransitions,                FALSE ),
   fsm_state_table_entry(FSMSTATE_active,                   activeTransitions,                 FALSE ),
};

const FsmProcessStateMachine qos_fsm =
{
#ifdef FSM_DEBUG
       "SME QOS",                                                          /* FSM Process Name */
#endif
       QOS_PROCESS,                                                        /* FSM Process ID */
       {FSMSTATE_MAX_STATE, stateTable},                                   /* FSM Transition Tables*/
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),   /* FSM Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),              /* FSM Ignore Event handers */
       sme_qos_init,                                                       /* FSM Entry Function */
       sme_qos_reset,                                                      /* FSM Reset Function */
#ifdef FSM_DEBUG_DUMP
       NULL                                                                /* FSM Trace Dump Function */
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
