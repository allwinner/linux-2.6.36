/** @file sme_measurements_fsm.c
 *
 * Public SME Measurements FSM API
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2007-2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   Public SME Measurements FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_measurements/sme_measurements_fsm.c#2 $
 *
 ****************************************************************************/

/** @{
 * @ingroup sme_measurements
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "fsm/csr_wifi_fsm.h"

#include "dbg_sap/dbg_sap_from_sme_interface.h"
#include "sme_measurements/sme_measurements.h"

#include "ie_access/ie_access.h"
#include "hip_proxy_fsm/mib_utils.h"

#include "smeio/smeio_trace_types.h"


#ifdef CCX_VARIANT
    #include "ccx_iapp_msg_handler/ccx_iapp_msg_handler.h"
    #include "ccx_measurements/ccx_measurements.h"
#endif


/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Processes Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, FsmData))

/*
static unifi_MACAddress peerAddress = {{0x00, 0x1a, 0x30, 0x8F, 0x0b, 0xF0}};
*/
/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/*lint -e749 */
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
    FSMSTATE_idle,
    FSMSTATE_inProgress,
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
    smh_measurement_data_block measurementDataBlock;

    /** stores the PID of the start or stop initiator */
    CsrUint16 corePid;

    DialogToken currentDialogToken;

} FsmData;


/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

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
static void init_sme_measurements_data(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "init_sme_measurements_data()"));

    fsmData->currentDialogToken = 0;

    smh_initialise_measurement_handler(&fsmData->measurementDataBlock);
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
static void sme_measurements_init(FsmContext* context)
{
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "sme_measurements_init()"));

    fsm_create_params(context, FsmData);

    init_sme_measurements_data(context);

    fsm_next_state(context, FSMSTATE_stopped);
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
static void sme_measurements_reset(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "sme_measurements_reset()"));

    smh_reset_measurement_handler(&fsmData->measurementDataBlock);

    init_sme_measurements_data(context);

    fsm_next_state(context, FSMSTATE_stopped);
}

/**
 * @brief
 *   Measurement Dialogue Token generator
 *
 * @par Description
 *   Allocates a dialogue Token.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   DialogToken, allocated dialogue token
 */
/*---------------------------------------------------------------------------*/
#ifdef CCX_VARIANT
static DialogToken get_dialog_token(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    DialogToken dialogToken;

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "get_dialog_token()"));

    dialogToken = fsmData->currentDialogToken;

    /* advance to the next token */
    fsmData->currentDialogToken++;

    return dialogToken;
}
#endif

/*****************************************************************************
 *                       STOPPED TRANSITIONS
 *****************************************************************************/

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
static void stopped__core_start_req(FsmContext* context, const CoreStartReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "stopped__core_start_req"));

    /* Store the PID of the sender of the CoreStartReq_Evt */
    fsmData->corePid = req->common.sender_;

    /*
    {
        DataReference mibSetDataRef;
        mibSetDataRef = ccx_mib_intialization(context);

        mib_set_sub_fsm_start(context, initializing_mib_set_cfm, &mibSetDataRef, FALSE);
    }
    */
    send_core_start_cfm(context, fsmData->corePid, unifi_Success);
    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   stopped State MaUnitdata transition
 *
 * @par Description
 *   A cross over has occurred and the message has been most likely been saved.
 *   consume and release the payload.
 *
 * @param[in]    context   : FSM context
 * @param[in]    ind       : MaUnitdataInd_Evt
 *
 * @return
 *   void
 */
static void stopped__ma_unitdata_ind(FsmContext* context, const MaUnitdataInd_Evt* ind)
{
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "stopped__ma_unitdata_ind:"));

    /* free the original message */
    pld_release(getPldContext(context), ind->data.slotNumber);
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
static void stopped__mlme_measure_cfm(FsmContext* context, const MlmeMeasureCfm_Evt* cfm)
{
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "stopped__mlme_measure_cfm: %d(%s) %x",
                                              cfm->resultCode,
                                              trace_ResultCode(cfm->resultCode),
                                              cfm->dialogToken));

    /* free any payloads */
    if (cfm->measurementReportSet.dataLength >0)
    {
        pld_release(getPldContext(context), cfm->measurementReportSet.slotNumber);
    }
}
/*****************************************************************************
 *                       IDLE TRANSITIONS
 *****************************************************************************/

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
static void idle__core_stop_req(FsmContext* context, const CoreStopReq_Evt* req)
{
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "idle__core_stop_req"));

    /* nothing currently to cleardown */

    send_core_stop_cfm(context, req->common.sender_, unifi_Success);

    fsm_next_state(context, FSMSTATE_stopped);
}

/*****************************************************************************
 *                       IDLE TRANSITIONS
 *****************************************************************************/
static void idle__mlme_mrequest_ind(FsmContext* context, const MlmeMrequestInd_Evt* ind)
{
    /*
    FsmData* fsmData = FSMDATA;
    smh_measurement_data  measurementData;
*/
    DataReference measureReqDataRef = {0, 0};

/*    SmeConfigData* cfg = get_sme_config(context);
    PldHdl hnd = (PldHdl)ind->data.slotNumber;
    CsrUint8 *pBuf;
    CsrUint16 length = (CsrUint16) ind->data.dataLength;
*/
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "idle__mlme_mrequest_ind:"));




    /* check to see if the dislogue token is already in use */




    /* gain access to the data */
/*    pld_access(hnd, (void**)&pBuf, &length);

    sme_trace_error( (TR_SME_MEASUREMENTS_FSM, "DATALENGTH= %d", length ) );
    sme_trace_hex( (TR_SME_MEASUREMENTS_FSM, TR_LVL_ERROR, "dataref :: ", pBuf, length) );
*/

/*    encode_ccx_measurement_request(pBuf, length, &measurementData, &measureReqDataRef);
 */

    /* forward the request to the FW */
/*    sme_trace_info((TR_SME_MEASUREMENTS_FSM, "send a measure request"));
 */

    /* add it to the measurements list */
/*    measurementData.dialogToken = dialogToken;
    measurementData.measurementCategory = ind->measurementCategory;
    measurementData.originatorMacAddress = ind->peerMacAddress;

    smh_add_measurement_data(&fsmData->measurementDataBlock, &measurementData);
*/

    send_mlme_measure_req(context, measureReqDataRef, 0);

    /*
     * NOTE: Do not change state, the Firmware is responsible for timing
     */
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
 * @param[in]    ind       : MaUnitdataInd_Evt
 *
 * @return
 *   void
 */
static void idle__ma_unitdata_ind(FsmContext* context, const MaUnitdataInd_Evt* ind)
{
    CsrUint16 length;
    CsrUint8 *pBuf;

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "idle__ma_unitdata_ind:"));

    sme_trace_debug((TR_SME_MEASUREMENTS_FSM, "idle__ma_unitdata_ind:  da [%s]  sa [%s] Ri[%d] Rs[%d] P[%d] Sc[%d]",
                                               trace_unifi_MACAddress(ind->da, getMacAddressBuffer(context)),
                                               trace_unifi_MACAddress(ind->sa, getMacAddressBuffer(context)),
                                               ind->routingInformation,
                                               ind->receptionStatus,
                                               ind->priority,
                                               ind->serviceClass));
    /* gain access to the data */
    pld_access(getPldContext(context), (PldHdl)ind->data.slotNumber, (void**)&pBuf, &length);

    sme_trace_debug( (TR_SME_MEASUREMENTS_FSM, "DATALENGTH= %d", length ) );
    sme_trace_hex( (TR_SME_MEASUREMENTS_FSM, TR_LVL_DEBUG, "dataref :: ", pBuf, length) );
#ifdef CCX_VARIANT

    /*if ((ccx_iapp_get_type(pBuf) == IAPP_Type_RadioMeasurement) && (ccx_iapp_get_subtype(pBuf) == IAPP_SubType_mr_request))*/
    if ((ccx_iapp_get_type(pBuf) == IAPP_Type_RadioMeasurement) && (ccx_iapp_get_subtype(pBuf) == IAPP_SubType_cntl_request))
    {
        DataReference measureReqDataRef = {0, 0};
        smh_measurement_data  measurementData;
        FsmData* fsmData = FSMDATA;

        ccx_mr_convert_measurement_frame_to_mlme_req(context, ind->data, &measureReqDataRef);

        /* add it to the measurements list */
        measurementData.dialogToken = get_dialog_token(context);
        measurementData.measurementCategory = MeasurementCategory_CcxMeasurement;
        measurementData.originatorMacAddress = ind->sa;

        smh_add_measurement_data(&fsmData->measurementDataBlock, &measurementData);

        /* forward the request to the FW */
        sme_trace_info((TR_SME_MEASUREMENTS_FSM, "send a measure request"));
        send_mlme_measure_req(context, measureReqDataRef, measurementData.dialogToken);

        /* only one measurement is allowed at any one time */
        fsm_next_state(context, FSMSTATE_inProgress);
    }
#endif

    /* free the original message */
    pld_release(getPldContext(context), ind->data.slotNumber);
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
static void idle__mlme_mreport_cfm(FsmContext* context, const MlmeMreportCfm_Evt* cfm)
{
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "idle__mlme_mreport_cfm: %d %s",
                                            cfm->resultCode,
                                            trace_ResultCode(cfm->resultCode)));
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
static void idle__mlme_measure_cfm(FsmContext* context, const MlmeMeasureCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    smh_measurement_data* pMeasData = NULL;

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "idle__mlme_measure_cfm: %d(%s) %x",
                                              cfm->resultCode,
                                              trace_ResultCode(cfm->resultCode),
                                              cfm->dialogToken));

    pMeasData = smh_get_measurement_data(&fsmData->measurementDataBlock, cfm->dialogToken);

    if(pMeasData != NULL)
    {
        if(cfm->resultCode == ResultCode_Success)
        {
            sme_trace_error((TR_SME_MEASUREMENTS_FSM, "The measurement FSM originated this, just consume "));

            sme_trace_info((TR_SME_MEASUREMENTS_FSM, "pMeasData->originatorMacAddress = %s", trace_unifi_MACAddress(pMeasData->originatorMacAddress, getMacAddressBuffer(context))));
            sme_trace_info((TR_SME_MEASUREMENTS_FSM, "NullBssid = %s", trace_unifi_MACAddress(NullBssid, getMacAddressBuffer(context))));

            if(CsrMemCmp(&NullBssid, &pMeasData->originatorMacAddress, sizeof(pMeasData->originatorMacAddress.data)) == 0)
            {
                sme_trace_error((TR_SME_MEASUREMENTS_FSM, "The measurement FSM originated this, just consume "));
                /* free any payloads */
                pld_release(getPldContext(context), cfm->measurementReportSet.slotNumber);

                /* measurement complete, erase measurement data */
                smh_delete_measurement_data(&fsmData->measurementDataBlock, cfm->dialogToken);

                fsm_next_state(context, FSMSTATE_idle);

                return;
            }
#ifdef CCX_VARIANT
            if(pMeasData->measurementCategory == MeasurementCategory_CcxMeasurement)
            {
                DataReference dataref;
                SmeConfigData* cfg = get_sme_config(context);

                sme_trace_info((TR_SME_MEASUREMENTS_FSM, "matching measurement CCX"));

                encode_ccx_measure_report_frame(context,
                                             cfm->measurementReportSet,
                                             &dataref,
                                             pMeasData,
                                             &cfg->stationMacAddress);

                /* free the original message */
                pld_release(getPldContext(context), cfm->measurementReportSet.slotNumber);

                ccx_iapp_send_ma_unitdata_req(context, dataref);
            }
            else
#endif
            {
                sme_trace_info((TR_SME_MEASUREMENTS_FSM, "matching measurement 80211"));

                send_mlme_mreport_req(context,
                                      cfm->measurementReportSet,
                                      NullDataReference,
                                      pMeasData->originatorMacAddress,
                                      pMeasData->dialogToken,
                                      pMeasData->measurementCategory);
            }

            /* measurement complete, erase measurement data */
            smh_delete_measurement_data(&fsmData->measurementDataBlock, cfm->dialogToken);
        }
        else
        {
            sme_trace_info((TR_SME_MEASUREMENTS_FSM, "measurement failed to succeed "));

            /* report a failure */
        }
    }
    else
    {
        sme_trace_error((TR_SME_MEASUREMENTS_FSM, "measurement not found in list "));
        /* free any payloads */
        pld_release(getPldContext(context), cfm->measurementReportSet.slotNumber);
    }

    fsm_next_state(context, FSMSTATE_idle);
}

/*****************************************************************************
 *                       INPROGRESS TRANSITIONS
 *****************************************************************************/

static void inprogress__ma_unitdata_ind(FsmContext* context, const MaUnitdataInd_Evt* ind)
{
#ifdef CCX_VARIANT
    FsmData* fsmData = FSMDATA;
    CsrUint16 length;
    CsrUint8 *pBuf;
    smh_measurement_data *pCurrentMeasurementFrame;
    ccx_mr_measurement_frame_data measurementReqFrameData;
#endif

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "inprogress__ma_unitdata_ind:"));

    sme_trace_debug((TR_SME_MEASUREMENTS_FSM, "inprogress__ma_unitdata_ind:  da [%s]  sa [%s] Ri[%d] Rs[%d] P[%d] Sc[%d]",
                                               trace_unifi_MACAddress(ind->da, getMacAddressBuffer(context)),
                                               trace_unifi_MACAddress(ind->sa, getMacAddressBuffer(context)),
                                               ind->routingInformation,
                                               ind->receptionStatus,
                                               ind->priority,
                                               ind->serviceClass));
#ifdef CCX_VARIANT

    pCurrentMeasurementFrame = smh_get_current_measurement_data(&fsmData->measurementDataBlock);

    extract_measurement_frame_info(context, ind->data, &measurementReqFrameData);

    /* decide if we should replace the current active measurement */
    if (smh_replace_current_measurement(pCurrentMeasurementFrame, ind->da))
    {
        DataReference measureRequestFrame;
        smh_measurement_data  measurementData;

        ccx_mr_build_mlme_measure_req_clear(context, &measureRequestFrame);

        /* add it to the measurements list */
        measurementData.dialogToken = get_dialog_token(context);
        measurementData.measurementCategory = MeasurementCategory_CcxMeasurement;
        measurementData.originatorMacAddress = NullBssid; /* generated by the SME */

        smh_add_measurement_data(&fsmData->measurementDataBlock, &measurementData);

        /* forward the request to the FW */
        sme_trace_info((TR_SME_MEASUREMENTS_FSM, "send a measure request"));
        send_mlme_measure_req(context, measureRequestFrame, measurementData.dialogToken);

        /* This Saves the event for later processing  (After we disconnect)*/
        fsm_saved_event(context, (const FsmEvent*)ind);

    }
    else
    {
        /* send a rejection */
        DataReference dataref = {0, 0};
        DataReference rejectReport = {0, 0};
        SmeConfigData* cfg = get_sme_config(context);

        encode_ccx_measure_report_element(context, &rejectReport, &measurementReqFrameData);

        encode_ccx_measure_report_frame(context,
                                     NullDataReference,
                                     &dataref,
                                     pCurrentMeasurementFrame,
                                     &cfg->stationMacAddress);

        /* free the original message */
        pld_release(getPldContext(context), ind->data.slotNumber);

        ccx_iapp_send_ma_unitdata_req(context, dataref);
    }


    /* gain access to the data */
    pld_access(getPldContext(context), (PldHdl)ind->data.slotNumber, (void**)&pBuf, &length);

    sme_trace_debug( (TR_SME_MEASUREMENTS_FSM, "DATALENGTH= %d", length ) );
    sme_trace_hex( (TR_SME_MEASUREMENTS_FSM, TR_LVL_DEBUG, "dataref :: ", pBuf, length) );
#else
    /* free the original message */
    pld_release(getPldContext(context), ind->data.slotNumber);
#endif

}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/* FSM DEFINITION **********************************************/

/** State idle transitions */
static const FsmEventEntry stoppedTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                      stopped__core_start_req),
    fsm_event_table_entry(MA_UNITDATA_IND_ID,                     stopped__ma_unitdata_ind),
    fsm_event_table_entry(MLME_MEASURE_CFM_ID,                    stopped__mlme_measure_cfm),
};

/** State idle transitions */
static const FsmEventEntry idleTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_MREQUEST_IND_ID,                   idle__mlme_mrequest_ind),
    fsm_event_table_entry(MLME_MREPORT_CFM_ID,                    idle__mlme_mreport_cfm),
    fsm_event_table_entry(MA_UNITDATA_IND_ID,                     idle__ma_unitdata_ind),
};

/** State idle transitions */
static const FsmEventEntry inProgressTransitions[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(MLME_MEASURE_CFM_ID,                    idle__mlme_measure_cfm),
    fsm_event_table_entry(MA_UNITDATA_IND_ID,                     inprogress__ma_unitdata_ind),
};


/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry defaultHandlers[] =
{
    /* Signal Id,                                                 Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                       idle__core_stop_req),
};


/** SME Measurement state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                       State                           State                       Save     */
   /*                       Name                            Transitions                  *       */
   fsm_state_table_entry(FSMSTATE_stopped,                  stoppedTransitions,         FALSE ),
   fsm_state_table_entry(FSMSTATE_idle,                     idleTransitions,            FALSE ),
   fsm_state_table_entry(FSMSTATE_inProgress,               inProgressTransitions,      TRUE ),
};

const FsmProcessStateMachine sme_measurements_fsm =
{
#ifdef FSM_DEBUG
       "SME measurements",                                       /* SM Process Name       */
#endif
       SME_MEASUREMENT_PROCESS,                                  /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                         /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, defaultHandlers, FALSE),  /* FSM Default Event Handlers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),    /* Ignore Event handers */
       sme_measurements_init,                                    /* Entry Function        */
       sme_measurements_reset,                                   /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                      /* Trace Dump Function   */
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
