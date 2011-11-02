/** @file mib_access_fsm.c
 *
 * MIB Access FSM Implementation
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
 *   MIB Access Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mib_access_fsm.c#7 $
 ****************************************************************************/

/** @{
 * @ingroup mib_access
 */

/* STANDARD INCLUDES ********************************************************/


/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "hip_proxy_fsm/mib_access_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "sme_core_fsm/sme_core_fsm.h"
#include "mgt_sap/mgt_sap_from_sme_interface.h"

#include "hip_proxy_fsm/mib_encoding.h"
#include "hip_proxy_fsm/mib_action_sub_fsm.h"

#include "smeio/smeio_trace_types.h"


/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Processes Custom data
 *
 * @par Description
 *   see brief
 */
#define MIBACCESSDATA (fsm_get_params(context, MibAccessData))

/* Radio calibration setting for initial mib setup */
#define RADIO_CAL_AUTO 0
#define RADIO_CAL_NONE 2

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
    FSMSTATE_wait_for_core_start,
    FSMSTATE_wait_for_mib_init,
    FSMSTATE_wait_for_mib_file_set,
    FSMSTATE_idle,
    FSMSTATE_MAX_STATE
} FsmState;


/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct MibAccessData {

    /** Store for the last request call */
    CsrUint16             reqSender;
    CsrUint16             reqEventId;

    CsrUint16             mibInitCurrentFile;
    CsrUint16             mibInitCurrentIndexStart;
    CsrUint16             mibInitCurrentIndexEnd;

    CsrUint16             flightmodeIndex;

    CsrUint16             calibrationCurrentIndex;
    void*              cfmAppHandle;
} MibAccessData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/
static void send_next_mib_mlme_set(FsmContext* context, CsrUint16 elementOffset);
static void send_initial_mib_get(FsmContext* context);
static void send_initial_mib_get_scheduled_interrupt(FsmContext* context);
static void send_initial_mib_set(FsmContext* context);
static void generic_get_cfm(FsmContext* context, const MlmeGetCfm_Evt* cfm);
static void wait_for_calibration_data_set__mlme_set_cfm(FsmContext* context, const MlmeSetCfm_Evt* cfm);

/* PUBLIC FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   MIB Access Process FSM Entry Function
 *
 * @par Description
 *   Called on MIB AccessProcess startup to initialise
 *   the MIB Accesor data
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void mib_access_init(FsmContext* context)
{
    sme_trace_entry((TR_MIB_ACCESS_FSM, "mib_access_init()"));
    fsm_create_params(context, MibAccessData);
    fsm_next_state(context, FSMSTATE_wait_for_core_start);
}

/**
 * @brief
 *   Fail a MIB get request
 *
 * @par Description
 *   Invalid MIB get request handler
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void bounce_mgt_mib_get_req(FsmContext* context, const UnifiMgtMibGetReq_Evt* req)
{
   sme_trace_warn((TR_SME_CONFIGURATION_FSM, "bounce_mgt_mib_get_req()"));
   call_unifi_mgt_mib_get_cfm(context, req->appHandle, unifi_Unavailable, 0, NULL);

   /* Free the copy of the data */
   CsrPfree(req->mibAttribute);
}

/**
 * @brief
 *   Fail a MIB get request
 *
 * @par Description
 *   Invalid MIB get request handler
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void bounce_mgt_mib_set_req(FsmContext* context, const UnifiMgtMibSetReq_Evt* req)
{
   sme_trace_warn((TR_SME_CONFIGURATION_FSM, "bounce_mgt_mib_set_req()"));
   call_unifi_mgt_mib_set_cfm(context, req->appHandle, unifi_Unavailable);

   /* Free the copy of the data */
   CsrPfree(req->mibAttribute);
}

/**
 * @brief
 *   Fail a MIB 'get next' request
 *
 * @par Description
 *   Invalid MIB 'get next' request handler
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void bounce_mgt_mib_get_next_req(FsmContext* context, const UnifiMgtMibGetNextReq_Evt* req)
{
    sme_trace_warn((TR_SME_CONFIGURATION_FSM, "bounce_mgt_mib_get_next_req()"));
    call_unifi_mgt_mib_get_next_cfm(context, req->appHandle, unifi_Unavailable, 0 , NULL);

    /* Free the copy of the data */
    CsrPfree(req->mibAttribute);
}


#define GET_CALIBRATION_DATA_COUNT 8
static void get_next_calibration_chunk(FsmContext* context)
{
    MibAccessData* mibData = MIBACCESSDATA;
    DataReference mibSetDataRef = mib_encode_create_get(context, GET_CALIBRATION_DATA_COUNT);
    CsrUint8 i;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "get_next_calibration_chunk(%d -> %d)", mibData->calibrationCurrentIndex + 1, mibData->calibrationCurrentIndex + GET_CALIBRATION_DATA_COUNT + 1));

    for (i = 0; i < GET_CALIBRATION_DATA_COUNT; i++)
    {
        /* 127 is the largest index allowed */
        if (mibData->calibrationCurrentIndex + i + 1 > 127)
        {
            break;
        }
        (void)mib_encode_add_get(context, &mibSetDataRef, unifiRadioTrimCache, (CsrUint8)(mibData->calibrationCurrentIndex + i + 1), 0);
    }
    mibData->calibrationCurrentIndex += GET_CALIBRATION_DATA_COUNT;

    mib_get_sub_fsm_start(context, *generic_get_cfm, &mibSetDataRef, TRUE);
}

static void store_calibration_chunk(FsmContext* context, const MlmeGetCfm_Evt* cfm)
{
    SmeConfigData* cfg = get_sme_config(context);
    unifi_DataBlock currentCalData = {0, NULL};
    unifi_DataBlock newCalData = {0, NULL};
    PldHdl newPldHdl;
    CsrUint8* newBuffer;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "store_calibration_chunk"));

    /* No Data to store*/
    if (cfm->status != MibStatus_Successful && cfm->errorIndex == 0)
    {
        return;
    }

    pld_access(getPldContext(context), cfm->mibAttributeValue.slotNumber, (void **)&newCalData.data, &newCalData.length);
    /*sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_ERROR, "New Calibration Data", newCalData.data, newCalData.length));*/

    /* First Chunk of Calibration data */
    if (cfg->calibrationData.dataLength == 0)
    {
        cfg->calibrationData = cfm->mibAttributeValue;
        return;
    }

    /* Merge the 2 buffers */
    pld_access(getPldContext(context), cfg->calibrationData.slotNumber, (void **)&currentCalData.data, &currentCalData.length);
    /*sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_ERROR, "Current Calibration Data", currentCalData.data, currentCalData.length));*/

    pld_create(getPldContext(context), (CsrUint16)(cfm->mibAttributeValue.dataLength + cfg->calibrationData.dataLength), (void **)&newBuffer, &newPldHdl);
    CsrMemCpy(newBuffer, currentCalData.data, cfg->calibrationData.dataLength);
    pld_release(getPldContext(context), cfg->calibrationData.slotNumber);

    /* Adjust the Length to remove error data */
    if (cfm->errorIndex != GET_CALIBRATION_DATA_COUNT)
    {
        CsrUint16 newDataLength = 0;

        CsrUint8* readBuffer = newCalData.data + 4;
        CsrUint16 i;
        for (i = 0; i < cfm->errorIndex; i++)
        {
            CsrUint16 len = readBuffer[1];
            if(readBuffer[0] != 0x30)
            {
                sme_trace_error((TR_MIB_ACCESS_FSM, "store_calibration_chunk(buffer[0] != 0x30)"));
                return;
            }

            if (len == 0x81)
            {
                len = readBuffer[2];
                len ++;
            }
            if (len == 0x82)
            {
                len = readBuffer[2] << 8 | readBuffer[3];
                len += 2;
            }

            newDataLength += len + 2;
            readBuffer += len + 2;
        }
        newCalData.data[2] = newDataLength >> 8;
        newCalData.data[3] = newDataLength & 0xFF;
        newCalData.length = 4 + newDataLength;
        /*sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_ERROR, "Adjusted Calibration Data", newCalData.data, newCalData.length));*/
    }

    CsrMemCpy(&newBuffer[cfg->calibrationData.dataLength], newCalData.data, newCalData.length);
    pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    cfg->calibrationData.dataLength = newCalData.length + cfg->calibrationData.dataLength;
    cfg->calibrationData.slotNumber = newPldHdl;
}

/**
 * @brief
 *   WaitingForMibGetSignalStats State MibGetSignalStats_confirm transition
 *
 * @par Description
 *   Waits for MLME confirm, clears bailout timer before and
 * sends result of request to resuestor
 * returning
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Get SignalStats Confirmation Parameters
 *
 * @return
 *   void
 */
static void generic_get_cfm(FsmContext* context, const MlmeGetCfm_Evt* cfm)
{
    MibAccessData* mibData = MIBACCESSDATA;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "generic_get_cfm(0x%.4X)", mibData->reqEventId));

    if (mibData->reqEventId == MLME_GET_REQ_ID)
    {
        fsm_forward_event(context, mibData->reqSender, (const FsmEvent*)cfm);
        fsm_next_state(context, FSMSTATE_idle);
        return;
    }

    if (mibData->reqEventId == UNIFI_MGT_MIB_GET_REQ_ID)
    {
        unifi_DataBlock dataBlock = {0, NULL};
        unifi_Status status = unifi_Success;

        if (cfm->status != MibStatus_Successful && cfm->status != MibStatus_ReadOnly)
        {
            sme_trace_error((TR_MIB_ACCESS_FSM, "generic_get_cfm failed (%s)", trace_MibStatus(cfm->status)));
            status = unifi_Error;
        }
        else
        {
            pld_access(getPldContext(context), cfm->mibAttributeValue.slotNumber, (void **)&dataBlock.data, &dataBlock.length);
            dataBlock.length = cfm->mibAttributeValue.dataLength;
            sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_DEBUG, "generic_get_cfm() Read Data", dataBlock.data, dataBlock.length));
        }
        call_unifi_mgt_mib_get_cfm(context, mibData->cfmAppHandle, status, dataBlock.length, dataBlock.data);
        if (cfm->mibAttributeValue.dataLength != 0)
        {
            pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
        }
        fsm_next_state(context, FSMSTATE_idle);
        return;
    }

    if (mibData->reqEventId == MIB_SAVE_CALIBRATION_DATA_REQ_ID)
    {
        store_calibration_chunk(context, cfm);

        if (cfm->status == MibStatus_Successful)
        {
            /* Keep reading the calibration table while there is more data */
            get_next_calibration_chunk(context);
            return;
        }

        fsm_next_state(context, FSMSTATE_idle);
        return;
    }

    /* We should NEVER be able to get here */
    fsm_error_event(context, (FsmEvent*)cfm);
    fsm_next_state(context, FSMSTATE_idle);
}


/**
 * @brief
 *  Send a chunk of calibration data to the FW
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   CsrBool : Did we send a mib set to the FW
 */
static CsrBool set_next_calibration_chunk(FsmContext* context)
{
    MibAccessData* mibData = MIBACCESSDATA;
    SmeConfigData *cfg = get_sme_config(context);
    unifi_DataBlock calData = {0, NULL};
    DataReference calibrationData;
    CsrUint8 headerSize = 2;
    CsrUint16 len = 0;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "set_next_calibration_chunk(%d)", mibData->calibrationCurrentIndex));

    if (cfg->calibrationData.dataLength == 0)
    {
        return FALSE;
    }


    pld_access(getPldContext(context), cfg->calibrationData.slotNumber, (void **)&calData.data, &calData.length);

    len = calData.data[1];
    if (calData.data[1] == 0x82)
    {
        len = calData.data[2] << 8 | calData.data[3];
        headerSize = 4;
    }
    else if (calData.data[1] == 0x81)
    {
        len = calData.data[2];
        headerSize = 3;
    }

    if (mibData->calibrationCurrentIndex == 0)
    {
        calibrationData = cfg->calibrationData;
        calibrationData.dataLength = len + headerSize;
        pld_add_ref(getPldContext(context), cfg->calibrationData.slotNumber);
        /*sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_ERROR, "Set Calibration Data", calData.data, calibrationData.dataLength));*/
    }
    else
    {
        CsrUint16 i;
        CsrUint8* buff = calData.data;
        for (i = 0; i < mibData->calibrationCurrentIndex; ++i) {
            buff += (len + headerSize);

            /* No more data to process */
            if (buff >= calData.data + cfg->calibrationData.dataLength)
            {
                return FALSE;
            }

            len = buff[1];
            if (buff[1] == 0x82)
            {
                len = buff[2] << 8 | buff[3];
                headerSize = 4;
            }
            else if (buff[1] == 0x81)
            {
                len = buff[2];
                headerSize = 3;
            }
        }
        calibrationData.dataLength = (len + headerSize);
        pld_store(getPldContext(context), buff, calibrationData.dataLength, &calibrationData.slotNumber);
        /*sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_ERROR, "Set Calibration Data", buff, calibrationData.dataLength));*/
    }
    mibData->calibrationCurrentIndex++;

    /* Use a Sub FSM to handle this event */
    mib_set_sub_fsm_start(context, *wait_for_calibration_data_set__mlme_set_cfm, &calibrationData, TRUE);

    return TRUE;
}
/**
 * @brief
 *   WaitingForMibSet State MibSet_confirm transition
 *
 * @par Description
 *  Sends a cfm to the requestor
 * returning
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : event
 *
 * @return
 *   void
 */
static void generic_set_cfm(FsmContext* context, const MlmeSetCfm_Evt* cfm)
{
    MibAccessData* mibData = MIBACCESSDATA;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "generic_set_cfm()"));
    sme_trace_entry((TR_MIB_ACCESS_FSM, "generic_set_cfm(STATE == %s)", fsm_current_state_name(context)));

    if (mibData->reqSender == FSM_ENV)
    {
        if (cfm->mibAttributeValue.dataLength != 0)
        {
            pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
        }
        call_unifi_mgt_mib_set_cfm(context, mibData->cfmAppHandle, cfm->status == MibStatus_Successful?unifi_Success:unifi_Error);
    }
    else
    {
        fsm_forward_event(context, mibData->reqSender, (const FsmEvent*)cfm);
    }
    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   Handles MIB 'get next' confirmations in response to a 'get next' request
 *
 * @par Description
 *   The firmware will (in error-free operation) be sending us the next OID
 *   in the MIB after the one specified in the request. The OID will be BER
 *   encoded.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Get SignalStats Confirmation Parameters
 *
 * @return
 *   void
 */
static void generic_getnext_cfm(FsmContext* context, const MlmeGetNextCfm_Evt* cfm)
{
    MibAccessData* mibData = MIBACCESSDATA;
    unifi_DataBlock dataBlock = {0, NULL};
    unifi_Status status = unifi_Success;

    sme_trace_entry((TR_MIB_ACCESS_FSM, "generic_getnext_cfm()"));

    if (cfm->status != MibStatus_Successful && cfm->status != MibStatus_ReadOnly)
    {
        sme_trace_error((TR_MIB_ACCESS_FSM, "generic_getnext_cfm failed (%s)", trace_MibStatus(cfm->status)));
        status = unifi_Error;
    }
    else
    {
        pld_access(getPldContext(context), cfm->mibAttributeValue.slotNumber, (void **)&dataBlock.data, &dataBlock.length);
        dataBlock.length = cfm->mibAttributeValue.dataLength;
        sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_DEBUG, "generic_getnext_cfm() Read Data", dataBlock.data, dataBlock.length));
    }

    call_unifi_mgt_mib_get_next_cfm(context, mibData->cfmAppHandle, status, dataBlock.length, dataBlock.data);

    if (cfm->mibAttributeValue.dataLength != 0)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }

    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   Perform a MIB 'GET' request on behalf of the upper layer
 *
 * @par Description
 *   Accepts a BER-encoded OID from the upper layer/environment and issues a
 *   MLME_GET_REQ to the firmware (via the HIP proxy process). In error free
 *   operation, this will result in the firmware sending us a MLME_GET_CFM
 *   with the associated BER-encoded value.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void idle_mgt_mib_get_req(FsmContext* context, const UnifiMgtMibGetReq_Evt* req)
{
    MibAccessData* mibData = MIBACCESSDATA;
    DataReference   dataRef;
    PldHdl          hdl;

    sme_trace_entry((TR_MIB_ACCESS_FSM , ">> idle_mgt_mib_get_req"));

    /*
     * Store the sender ID and the request type as a generic handler will deal
     * with the confirmation and the behavior on receipt of this message
     * varies according to the signal that initiated the MIB-GET.
     */
    mibData->reqSender  = req->common.sender_;
    mibData->reqEventId = req->common.id;

    /*
     * The memory used for input OID contained in the request is now 'owned'
     * by us. Place this memory under the control of the payload manager so
     * that we can then send it down to the firmware via a DataReference. No
     * need to worry about releasing the memory, the DD SHIM will take care
     * of this for us.
     */
    pld_store(getPldContext(context), req->mibAttribute, req->mibAttributeLength, &hdl);
    dataRef.slotNumber = hdl;
    dataRef.dataLength = req->mibAttributeLength;

    /* Free the copy of the data */
    CsrPfree(req->mibAttribute);

    sme_trace_debug((TR_MIB_ACCESS_FSM, "%s: issuing MIB GET REQ with %d byte BER-encoded OID", fsm_current_state_name(context), dataRef.dataLength));
    sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_DEBUG, "BER Encoded OID", req->mibAttribute, req->mibAttributeLength));

    /* Use a Sub FSM to handle this event */
    mibData->cfmAppHandle = req->appHandle;
    mib_get_sub_fsm_start(context, *generic_get_cfm, &dataRef, TRUE);

    sme_trace_entry((TR_MIB_ACCESS_FSM , "<< idle_mgt_mib_get_req"));
}

/**
 * @brief
 *   Perform a MIB 'GET-NEXT' request on behalf of the upper layer
 *
 * @par Description
 *   Accepts a BER-encoded OID from the upper layer/environment and issues a
 *   MLME_GET_NEXT_REQ to the firmware (via the HIP proxy process). In error
 *   free operation, this will result in the firmware sending us a confirm
 *   with the next MIB OID (BER-encoded).
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void idle_mgt_mib_get_next_req(FsmContext* context, const UnifiMgtMibGetNextReq_Evt* req)
{
    MibAccessData* mibData = MIBACCESSDATA;
    DataReference   dataRef;
    PldHdl          hdl;

    sme_trace_entry((TR_MIB_ACCESS_FSM , ">> idle_mgt_mib_get_next_req"));

    /*
     * Store the sender ID and the request type as a generic handler will deal
     * with the confirmation and the behavior on receipt of this message
     * varies according to the signal that initiated the MIB-GET-NEXT.
     */
    mibData->reqSender  = req->common.sender_;
    mibData->reqEventId = req->common.id;

    /*
     * The memory used for input OID contained in the request is now 'owned'
     * by us. Place this memory under the control of the payload manager so
     * that we can then send it down to the firmware via a DataReference. No
     * need to worry about releasing the memory, the DD SHIM will take care
     * of this for us.
     */
    pld_store(getPldContext(context), req->mibAttribute, req->mibAttributeLength, &hdl);
    dataRef.slotNumber = hdl;
    dataRef.dataLength = req->mibAttributeLength;

    /* Free the copy of the data */
    CsrPfree(req->mibAttribute);

    /* Use a Sub FSM to handle this event */
    mibData->cfmAppHandle = req->appHandle;
    mib_getnext_sub_fsm_start(context, *generic_getnext_cfm, &dataRef, TRUE);

    sme_trace_entry((TR_MIB_ACCESS_FSM , "<< idle_mgt_mib_get_next_req"));
}

/**
 * @brief
 *   Perform a MIB 'GET' request on behalf of the upper layer
 *
 * @par Description
 *   Accepts a BER-encoded OID from the upper layer/environment and issues a
 *   MLME_GET_REQ to the firmware (via the HIP proxy process). In error free
 *   operation, this will result in the firmware sending us a MLME_GET_CFM
 *   with the associated BER-encoded value.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void idle_mgt_mib_set_req(FsmContext* context, const UnifiMgtMibSetReq_Evt* req)
{
    MibAccessData* mibData = MIBACCESSDATA;
    DataReference   dataRef;
    PldHdl          hdl;

    sme_trace_entry((TR_MIB_ACCESS_FSM , ">> idle_mgt_mib_set_req"));

    if (isAccessRestricted(context, req->appHandle))
    {
        call_unifi_mgt_mib_set_cfm(context, req->appHandle, unifi_Restricted);
        CsrPfree(req->mibAttribute);
        return;
    }

    /*
     * Store the sender ID and the request type as a generic handler will deal
     * with the confirmation and the behavior on receipt of this message
     * varies according to the signal that initiated the MIB-GET.
     */
    mibData->reqSender  = req->common.sender_;
    mibData->reqEventId = req->common.id;

    /*
     * The memory used for input OID contained in the request is now 'owned'
     * by us. Place this memory under the control of the payload manager so
     * that we can then send it down to the firmware via a DataReference. No
     * need to worry about releasing the memory, the DD SHIM will take care
     * of this for us.
     */
    pld_store(getPldContext(context), req->mibAttribute, req->mibAttributeLength, &hdl);
    dataRef.slotNumber = hdl;
    dataRef.dataLength = req->mibAttributeLength;

    sme_trace_debug((TR_MIB_ACCESS_FSM, "%s: issuing MIB SET REQ with %d byte BER-encoded OID", fsm_current_state_name(context), dataRef.dataLength));
    sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_DEBUG, "BER Encoded OID", req->mibAttribute, req->mibAttributeLength));

    /* Free the copy of the data */
    CsrPfree(req->mibAttribute);

    /* Use a Sub FSM to handle this event */
    mibData->cfmAppHandle = req->appHandle;
    mib_set_sub_fsm_start(context, *generic_set_cfm, &dataRef, TRUE);

    sme_trace_entry((TR_MIB_ACCESS_FSM , "<< idle_mgt_mib_set_req"));
}
/**
 * @brief
 *  Core Start Request received in initial state as anticipated
 *
 * @par Description
 *  The Core Start request has arrived.  This is automatically acknowledged
 *  as successful and we move into a state where we wait to be told to
 *  initialise the UniFi MIB from configuration data.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Start request
 *
 * @return
 *   void
 */
static void wait_for_core_start__core_start_req(FsmContext* context, const CoreStartReq_Evt* req)
{
    sme_trace_entry((TR_MIB_ACCESS_FSM, ">>mib_access::wait_for_core_start__core_start_req()"));

    /* Check if UniFi was fully powered prior to the request to start (resume)
     * If so we must move to idle state because the unifi_driver_fsm will not
     * perform a MIB initialisation
     */
    if (core_get_startupPowerMaintainedFlag(context, getSmeContext(context)->smeCoreInstance))
    {
        sme_trace_info((TR_MIB_ACCESS_FSM,
                            "power was maintained since last shutdown, "
                            "engaging quick startup"));
        send_core_start_cfm(context, req->common.sender_, unifi_Success);
        fsm_next_state(context, FSMSTATE_idle);
    }
    else
    {

        send_core_start_cfm(context, req->common.sender_, unifi_Success);

        fsm_next_state(context, FSMSTATE_wait_for_mib_init);
    }

    sme_trace_entry((TR_MIB_ACCESS_FSM, "<<mib_access::wait_for_core_start__core_start_req()"));
}

/**
 * @brief
 *  Init Mib Request received, as anticipated
 *
 * @par Description
 *  The Initialise MIB request has arrived.  We start a sequence where the
 *  UniFi MIB is initialised from configuration data provided by the DD SAP.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Start request
 *
 * @return
 *   void
 */
static void wait_for_mib_init__mib_init_req(FsmContext* context, const MibInitReq_Evt* req)
{
    MibAccessData* mibAccessData = MIBACCESSDATA;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_MIB_ACCESS_FSM, ">>mib_access::wait_for_mib_init__mib_init_req()"));

    if (cfg->mibFiles.numElements > 0)
    {
        /* Start the MIB write sequence */
        sme_trace_debug((TR_MIB_ACCESS_FSM, "Starting MIB configuration download (1/%d)", cfg->mibFiles.numElements));
        mibAccessData->mibInitCurrentFile = 0;
        mibAccessData->mibInitCurrentIndexStart = 0;
        mibAccessData->mibInitCurrentIndexEnd = 0;
        send_next_mib_mlme_set(context, 0);
    }
    else
    {
        sme_trace_info((TR_MIB_ACCESS_FSM, "No MIB configuration to download - assuming all set in ROM"));

        /* There is no MIB configuration try and read the Mac Address */
        send_initial_mib_get(context);
    }

    sme_trace_entry((TR_MIB_ACCESS_FSM, "<<mib_access::wait_for_mib_init__mib_init_req()"));
}

/**
 * @brief
 *  MLME_SET_CFM arrived in response to a MLME_SET_REQ made for initialisation
 *
 * @par Description
 *  If the set_cfm is successful, send the next one, otherwise report
 *  failure to the higher level and abort the initialisation.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Content of MLME_SET_CFM
 *
 * @return
 *   void
 */
static void wait_for_mib_init_mlme_set__mlme_set_cfm(FsmContext* context, const MlmeSetCfm_Evt *cfm)
{
    MibAccessData *mibAccessData = MIBACCESSDATA;
    sme_trace_entry((TR_MIB_ACCESS_FSM, ">> wait_for_mib_init_mlme_set__mlme_set_cfm"));
    sme_trace_debug((TR_MIB_ACCESS_FSM, "__mlme_set_cfm status was %s", trace_MibStatus(cfm->status)));

    if (cfm->mibAttributeValue.dataLength != 0)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }

    if (cfm->status == MibStatus_Successful)
    {
        /* Send the next MIB element */
        send_next_mib_mlme_set(context, mibAccessData->mibInitCurrentIndexEnd);
    }
    else
    {
        /* Tell unifi_driver_fsm that something went wrong */
        sme_trace_warn((TR_MIB_ACCESS_FSM, "mlme_set_cfm(%s, idx=%d)", trace_MibStatus(cfm->status),cfm->errorIndex));
        send_next_mib_mlme_set(context, (CsrUint16)(mibAccessData->mibInitCurrentIndexStart + cfm->errorIndex+1));
    }
}

/**
 * @brief
 *  Send the next MIB initialisation element to UniFi
 *
 * @par Description
 *  This function is used to step through the MIB file(s) stored in memory
 *  in the configuration area.  If there is more than one file, they are
 *  stored consecutively, with their UDMI headers still in place.
 *
 *  Indexes are kept in the MIBACCESS data structure so that we know how far
 *  we have gone so far:
 *
 *  - mibCurrentIndex always points at the next SET to be sent, or a UDMI
 *    header
 *  - mibInitEndIndex is the end of the current MIB file in memory, so that
 *    we know to expect another UDMI header (or the end, but that is checked
 *    against the length of the data block)
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void send_next_mib_mlme_set(FsmContext* context, CsrUint16 elementOffset)
{
#define MIB_UDMI_HEADER_LEN 10
    MibAccessData *mibAccessData = MIBACCESSDATA;
    SmeConfigData *cfg = get_sme_config(context);
    CsrUint8 *mibPtr;
    CsrUint32 mibFileLength;
    DataReference mibEncodeDataRef;

    sme_trace_entry((TR_MIB_ACCESS_FSM, ">> send_next_mib_mlme_set"));

    /* We have finished downloading this MIB configuration data block.
     * See if there are any more to download.*/
    if (mibAccessData->mibInitCurrentFile >= cfg->mibFiles.numElements)
    {
        /* There are no more MIB data blocks to send. Move on to the Mac Address Get. */
        sme_trace_debug((TR_MIB_ACCESS_FSM,  "send_next_mib_mlme_set: MIB configuration completed (%d file%s)",
                                             mibAccessData->mibInitCurrentFile + 1,
                                             (mibAccessData->mibInitCurrentFile + 1) == 1 ? "" : "s"));
        send_initial_mib_get(context);
        return;
    }

    sme_trace_debug((TR_MIB_ACCESS_FSM, "Continuing MIB configuration download (%d/%d)",
                     mibAccessData->mibInitCurrentFile + 1, cfg->mibFiles.numElements));

    mibPtr = cfg->mibFiles.dataList[mibAccessData->mibInitCurrentFile].data;
    mibAccessData->mibInitCurrentIndexStart = elementOffset;

    /* Check the header is valid */
    if (CsrMemCmp(mibPtr, "UDMI", 4) != 0)
    {
        sme_trace_error((TR_MIB_ACCESS_FSM, "MIB configuration header not 'UDMI' in file %d",
                                            mibAccessData->mibInitCurrentFile + 1 ));

        /* This is considered to be a fatal error */
        send_mib_init_cfm(context, getSmeContext(context)->unifiDriverInstance, MibStatus_InvalidParameters);
        fsm_next_state(context, FSMSTATE_wait_for_mib_init);
        return;
    }

    mibFileLength = (mibPtr[9] << 24) |
                    (mibPtr[8] << 16) |
                    (mibPtr[7] << 8) |
                     mibPtr[6];

    sme_trace_info((TR_MIB_ACCESS_FSM, "send_next_mib_mlme_set: MIB file version %#x, length %ld",
                                       ((mibPtr[5] << 8) | mibPtr[4]), mibFileLength));

    mibPtr += MIB_UDMI_HEADER_LEN;

    mibEncodeDataRef = mib_encode_file(context, mibPtr, mibFileLength, mibAccessData->mibInitCurrentIndexStart, &mibAccessData->mibInitCurrentIndexEnd);
    if (mibEncodeDataRef.dataLength == 0)
    {
        /* Try next file */
        mibAccessData->mibInitCurrentFile++;
        send_next_mib_mlme_set(context, 0);
        return;
    }
    else
    {
        send_mlme_set_req(context, mibEncodeDataRef);
    }

    fsm_next_state(context, FSMSTATE_wait_for_mib_file_set);

    sme_trace_entry((TR_MIB_ACCESS_FSM, "<< send_next_mib_mlme_set"));
}

/**
 * @brief
 *  MLME Set Confirm arrived as expected while waiting for Calibration data set
 *
 * @par Description
 *  The expected MLME Set Confirm has arrived which should be for the
 *  Calibration data set earlier.  If successful, go on to set the mac address
 *  If a failure, report it directly to unifi_driver_fsm.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Content of the MLME Set Confirm
 *
 * @return
 *   void
 */
static void wait_for_calibration_data_set__mlme_set_cfm(FsmContext* context, const MlmeSetCfm_Evt* cfm)
{
    sme_trace_entry((TR_MIB_ACCESS_FSM, "wait_for_calibration_data_set__mlme_set_cfm: status was %s", trace_MibStatus(cfm->status)));

    if (cfm->mibAttributeValue.dataLength != 0)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }

    /* Send the next block of calibration data */
    if (set_next_calibration_chunk(context) == TRUE)
    {
        return;
    }

    send_initial_mib_set(context);
    if (cfm->status != MibStatus_Successful)
    {
        /* This can happen but it is not a show stopper.
         * If the data we read is not complete then errors will occur */
        sme_trace_warn((TR_MIB_ACCESS_FSM, "wait_for_calibration_data_set__mlme_set_cfm: MLME_SET_CFM status was %s", trace_MibStatus(cfm->status)));
    }
}

/**
 * @brief
 *  MLME Set Confirm arrived as expected while wait for MAC Address set
 *
 * @par Description
 *  The expected MLME Set Confirm has arrived which should be for the MAC
 *  Address set earlier.  If successful, go on to readback data from the MIB
 *  into the data cache.  If a failure, report it directly to
 *  unifi_driver_fsm.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Content of the MLME Set Confirm
 *
 * @return
 *   void
 */
static void wait_for_init_mib_get__mlme_get_cfm(FsmContext* context, const MlmeGetCfm_Evt* cfm)
{
    SmeConfigData* cfg = get_sme_config(context);
    CsrUint8 unifiTxPowerAdjustIndex = 14;
    MibStatus status = cfm->status;

    static const unifi_MACAddress csrDefaultAddress = {{0x00, 0x02, 0x5B, 0x00, 0xA5, 0xA5}};
    CsrBool stationMacAddressSet = CsrMemCmp(&cfg->stationMacAddress, &BroadcastBssid,  sizeof(BroadcastBssid.data)) != 0;

    sme_trace_entry((TR_MIB_ACCESS_FSM, "wait_for_init_mib_get__mlme_get_cfm: status was %s :: stationMacAddressSet = %d", trace_MibStatus(cfm->status), stationMacAddressSet));
#ifdef CCX_VARIANT
    unifiTxPowerAdjustIndex++;
#endif
#ifdef CSR_AMP_ENABLE
    unifiTxPowerAdjustIndex += 4;  /* Update this value if number of MIB reads for AMP (see below) changes */
#endif

    if (cfm->status == MibStatus_Successful || cfm->errorIndex >= unifiTxPowerAdjustIndex)
    {
        CsrInt32 readInt;
        unifi_MACAddress readAddress;
        CsrUint8 mibAttributeValueIndex = 0;
        status = MibStatus_Successful;

        (void)mib_decode_get_os(context,  &cfm->mibAttributeValue, mibAttributeValueIndex++, (CsrUint8*)&readAddress, 6);

        if (!stationMacAddressSet && CsrMemCmp(&csrDefaultAddress, &readAddress, sizeof(readAddress.data)) == 0)
        {
            sme_trace_crit((TR_MIB_ACCESS_FSM, "wait_for_init_mib_get__mlme_get_cfm() :Default CSR MACAddress 00:02:5B:00:A5:A5 Found"));
            status = MibStatus_InvalidParameters;
        }
        else if (!stationMacAddressSet && CsrMemCmp(&NullBssid, &readAddress, sizeof(readAddress.data)) == 0)
        {
            sme_trace_crit((TR_MIB_ACCESS_FSM, "wait_for_init_mib_get__mlme_get_cfm() :Invalid MACAddress 00:00:00:00:00:00 Found"));
            status = MibStatus_InvalidParameters;
        }
        else
        {
            CsrMemCpy(&cfg->permanentMacAddress, &readAddress, sizeof(readAddress.data));
            /* If the station Mac address not set then use the value read out of the MIB */
            if (!stationMacAddressSet)
            {
                stationMacAddressSet = TRUE;
                cfg->stationMacAddress = cfg->permanentMacAddress;
            }
        }

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->mibConfig.dot11RtsThreshold = (CsrUint16)readInt;

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->mibConfig.dot11FragmentationThreshold = (CsrUint16)readInt;

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->mibConfig.unifiFixMaxTxDataRate = (readInt==1); /* 1 = TRUE, 2 = FALSE */

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->mibConfig.unifiFixTxDataRate = (CsrUint8)readInt;

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->mibConfig.dot11CurrentTxPowerLevel = (CsrUint8)readInt;

        /* Note: In F/W: 1 means True and 2 means False, so % operation maps 1 to 1 (True) and 2 to 0 (False) */
        (void)mib_decode_get_CsrBool(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &cfg->regDomInfo.dot11MultiDomainCapabilityImplemented);
        (void)mib_decode_get_CsrBool(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &cfg->regDomInfo.dot11MultiDomainCapabilityEnabled);

        /* Force dot11MultiDomainCapabilityEnabled OFF when 802,11a in use */
        if (cfg->smeConfig.ifIndex & unifi_GHZ_5_0)
        {
            cfg->regDomInfo.dot11MultiDomainCapabilityEnabled = FALSE;
        }

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->dot11CurrentRegDomain                  = (CsrUint16) readInt;
        cfg->regdomReadFromMib                      = TRUE;

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->unifiCoexScheme = (unifi_CoexScheme)readInt;

        (void)mib_decode_get_CsrBool(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &cfg->highThroughputOptionEnabled);
        (void)mib_decode_get_CsrBool(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &cfg->wapiOptionImplemented);

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->versions.firmwarePatch = (CsrUint32)readInt;
        sme_trace_info((TR_MIB_ACCESS_FSM, "Firmware patch build ID = %d", readInt));

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->unifiKeepAliveTime = (CsrUint16)readInt;

#ifdef CCX_VARIANT
        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->unifiCCXVersionImplemented                  = (CsrUint8) readInt;
#endif

#ifdef CSR_AMP_ENABLE
        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->dot11LongRetryLimit = (CsrUint8)readInt;

        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &readInt);
        cfg->dot11ShortRetryLimit = (CsrUint8)readInt;

        (void)mib_decode_get_CsrBool(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &cfg->dot11ShortSlotTimeOptionImplemented);
        (void)mib_decode_get_CsrBool(context, &cfm->mibAttributeValue, mibAttributeValueIndex++, &cfg->dot11ShortSlotTimeOptionEnabled);
/* Note: Please update value of 'unifiTxPowerAdjustIndex' defined above if number of MIB reads for AMP changes */
#endif

        if (cfm->errorIndex > (unifiTxPowerAdjustIndex + 1))
        {
            (void)mib_decode_get_int(context, &cfm->mibAttributeValue, unifiTxPowerAdjustIndex, &readInt);
            cfg->unifiTxPowerAdjustment_tpo = (CsrInt8) readInt;

            (void)mib_decode_get_int(context, &cfm->mibAttributeValue, (CsrUint16)(unifiTxPowerAdjustIndex+1), &readInt);
            cfg->unifiTxPowerAdjustment_eirp = (CsrInt8) readInt;

            /* keep the index up to date */
            mibAttributeValueIndex += 2;
        }
        else if (cfg->regDomInfo.dot11MultiDomainCapabilityEnabled)
        {
            /* BIG Problem.... Cannot use 802.11d without unifiTxPowerAdjustment correctly set*/
            sme_trace_crit((TR_MIB_ACCESS_FSM, "unifi is not correctly configured. 802.11d is enabled BUT unifiTxPowerAdjustment is not set"));
            status = MibStatus_InvalidParameters;
        }

        /* Save the default values so we only update those that are different */
        cfg->defaultMibConfig = cfg->mibConfig;
    }
    else
    {
        sme_trace_crit((TR_MIB_ACCESS_FSM, "wait_for_init_mib_get__mlme_get_cfm() :Failed to read MIB, error index %d", cfm->errorIndex));
    }

    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }

    sme_trace_info((TR_MIB_ACCESS_FSM, "wait_for_init_mib_get__mlme_get_cfm() permanentMacAddress = %s", trace_unifi_MACAddress(cfg->permanentMacAddress, getMacAddressBuffer(context))));
    sme_trace_info((TR_MIB_ACCESS_FSM, "wait_for_init_mib_get__mlme_get_cfm() stationMacAddress = %s", trace_unifi_MACAddress(cfg->stationMacAddress, getMacAddressBuffer(context))));

    if (status != MibStatus_Successful)
    {
        sme_trace_crit((TR_MIB_ACCESS_FSM, "wait_for_init_mib_get__mlme_get_cfm() :Error on Startup Mib Read"));
        /* Error reading the MACAddress fail the startup */
        send_mib_init_cfm(context, getSmeContext(context)->unifiDriverInstance, status);
        fsm_next_state(context, FSMSTATE_wait_for_mib_init);
        return;
    }

    send_initial_mib_get_scheduled_interrupt(context);
}

/**
 * @brief
 *  Get the MAC address, stored in ROM or initial Mib
 *
 * @par Description
 *  Trys to read the Mac Address from ROM / InitialMib
 *  before we override it from the configuration
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void send_initial_mib_get(FsmContext* context)
{
    MibAccessData* mibData = MIBACCESSDATA;
    DataReference mibGetDataRef;

    sme_trace_entry((TR_MIB_ACCESS_FSM, ">> send_initial_mib_get"));

    mibData->reqSender = getSmeContext(context)->mibAccessInstance;
    mibData->reqEventId = 0;

    /* request a block big enough for all possible combinations */
    mibGetDataRef = mib_encode_create_get(context, 19);

    (void)mib_encode_add_get(context, &mibGetDataRef, dot11MACAddress,             unifi_GHZ_2_4, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11RTSThreshold,           unifi_GHZ_2_4, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11FragmentationThreshold, unifi_GHZ_2_4, 0);

    (void)mib_encode_add_get(context, &mibGetDataRef, unifiFixMaxTxDataRate,       0, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, unifiFixTxDataRate,          0, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11CurrentTxPowerLevel,    unifi_GHZ_2_4, 0);

    /* 802.11d Attributes */
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11MultiDomainCapabilityImplemented,  unifi_GHZ_2_4, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11MultiDomainCapabilityEnabled,      unifi_GHZ_2_4, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11CurrentRegDomain,                  unifi_GHZ_2_4, 0);

    (void)mib_encode_add_get(context, &mibGetDataRef, unifiCoexScheme,                        0, 0);

    /* 802.11n Attributes */
    (void)mib_encode_add_get(context, &mibGetDataRef, unifiCSROnlyHighThroughputOptionEnabled,    unifi_GHZ_2_4, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, gb15629dot11wapiOptionImplemented, unifi_GHZ_2_4, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, unifiFirmwarePatchBuildID, 0, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, unifiKeepAliveTime, 0, 0);

#ifdef CCX_VARIANT
    (void)mib_encode_add_get(context, &mibGetDataRef, unifiCCXVersionImplemented,   unifi_GHZ_2_4, 0);
#endif

#ifdef CSR_AMP_ENABLE
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11LongRetryLimit,                    unifi_GHZ_2_4, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11ShortRetryLimit,                   unifi_GHZ_2_4, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11ShortSlotTimeOptionImplemented,    unifi_GHZ_2_4, 0);
    (void)mib_encode_add_get(context, &mibGetDataRef, dot11ShortSlotTimeOptionEnabled,        unifi_GHZ_2_4, 0);
#endif

    /* These MUST be read last */
    (void)mib_encode_add_get(context, &mibGetDataRef, unifiTxPowerAdjustment,                 unifi_GHZ_2_4, 1); /* TPO */
    (void)mib_encode_add_get(context, &mibGetDataRef, unifiTxPowerAdjustment,                 unifi_GHZ_2_4, 2); /* EIRP */

    /* Use a Sub FSM to handle this event */
    mib_get_sub_fsm_start(context, *wait_for_init_mib_get__mlme_get_cfm, &mibGetDataRef, TRUE);

    sme_trace_entry((TR_MIB_ACCESS_FSM, "<< send_initial_mib_get"));
}

static void wait_for_init_mib_get_scheduled_interrupt__mlme_get_cfm(FsmContext* context, const MlmeGetCfm_Evt* cfm)
{
    SmeConfigData* cfg = get_sme_config(context);
    MibAccessData* mibData = MIBACCESSDATA;

    sme_trace_entry((TR_MIB_ACCESS_FSM, "wait_for_init_mib_get__mlme_get_cfm: status was %s", trace_MibStatus(cfm->status)));

    cfg->scheduledInterrupt = FALSE;

    if (cfm->status == MibStatus_Successful)
    {
        CsrInt32 readInt;
        (void)mib_decode_get_int(context, &cfm->mibAttributeValue, 0, &readInt);
        if (readInt != 0)
        {
            cfg->scheduledInterrupt = TRUE;
        }
    }


    if (cfm->mibAttributeValue.dataLength)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }

    /* Address Read complete Load Cal data */
    mibData->calibrationCurrentIndex = 0;
    if (set_next_calibration_chunk(context) == FALSE)
    {
        /* No Cal Data so try the Mac Address */
        send_initial_mib_set(context);
    }
}

/**
 *  Check if scheduledInterrupt is enabled
 */
static void send_initial_mib_get_scheduled_interrupt(FsmContext* context)
{
    MibAccessData* mibData = MIBACCESSDATA;
    DataReference mibGetDataRef;

    sme_trace_entry((TR_MIB_ACCESS_FSM, ">> send_initial_mib_get_scheduled_interrupt"));

    mibData->reqSender = getSmeContext(context)->mibAccessInstance;
    mibData->reqEventId = 0;

    mibGetDataRef = mib_encode_create_get(context, 1);

    (void)mib_encode_add_get(context, &mibGetDataRef, unifiScheduledInterruptPeriod, 0, 0);

    /* Use a Sub FSM to handle this event */
    mib_get_sub_fsm_start(context, *wait_for_init_mib_get_scheduled_interrupt__mlme_get_cfm, &mibGetDataRef, TRUE);
}

/**
 * @brief
 *  MLME Set Confirm arrived as expected while wait for MAC Address set
 *
 * @par Description
 *  The expected MLME Set Confirm has arrived which should be for the MAC
 *  Address set earlier.  If successful, go on to readback data from the MIB
 *  into the data cache.  If a failure, report it directly to
 *  unifi_driver_fsm.
 *
 * @param[in]    context   : FSM context
 * @param[in]    cfm       : Content of the MLME Set Confirm
 *
 * @return
 *   void
 */
static void wait_for_init_mib_set__mlme_set_cfm(FsmContext* context, const MlmeSetCfm_Evt* cfm)
{
    sme_trace_entry((TR_MIB_ACCESS_FSM, "wait_for_init_mib_set__mlme_set_cfm(%s: %d)", trace_MibStatus(cfm->status), cfm->errorIndex));

    if (cfm->mibAttributeValue.dataLength != 0)
    {
        pld_release(getPldContext(context), cfm->mibAttributeValue.slotNumber);
    }

    if (cfm->status == MibStatus_Successful)
    {
        send_mib_init_cfm(context, getSmeContext(context)->unifiDriverInstance, cfm->status);
        fsm_next_state(context, FSMSTATE_idle);
    }
    else if (cfm->errorIndex == MIBACCESSDATA->flightmodeIndex && !core_get_flightmodeBootFlag(context))
    {
        /* Flightmode mib value set failed Just ignore */
        sme_trace_warn((TR_MIB_ACCESS_FSM, "wait_for_init_mib_set__mlme_set_cfm() Failed to set unifiRadioCalibrationMode"));
        send_mib_init_cfm(context, getSmeContext(context)->unifiDriverInstance, MibStatus_Successful);
        fsm_next_state(context, FSMSTATE_idle);
    }
    else
    {
        sme_trace_error((TR_MIB_ACCESS_FSM, "wait_for_init_mib_set__mlme_set_cfm: MLME_SET_CFM status was %s", trace_MibStatus(cfm->status)));
        /* Tell unifi_driver_fsm that something went wrong */
        send_mib_init_cfm(context, getSmeContext(context)->unifiDriverInstance, cfm->status);
        fsm_next_state(context, FSMSTATE_wait_for_core_start);
    }
    sme_trace_entry((TR_MIB_ACCESS_FSM, "wait_for_init_mib_set__mlme_set_cfm()"));
}

/**
 * @brief
 *  Set the MAC address, stored in the configuration area, into the UniFi MIB
 *
 * @par Description
 *  If the MAC address stored in the configuration area is not all-zeros or
 *  all-ffffs, send an MLME_SET_REQ for it to the UniFi.
 *
 *  Note: this will only work in the setup phase of the UniFi;
 *
 *  Otherwise, skip this step and initiate the readback of MIB data into
 *  this FSM's local cache.
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
static void send_initial_mib_set(FsmContext* context)
{
    MibAccessData* fsmData = MIBACCESSDATA;
    SmeConfigData* cfg = get_sme_config(context);
    DataReference mibSetDataRef = mib_encode_create_set(context, 5, 1);
    unifi_RadioIF ifIndex;

    sme_trace_entry((TR_MIB_ACCESS_FSM, ">>send_initial_mib_set"));

    fsmData->flightmodeIndex = 0;

    /* Write the MAC address down into the MIB if it is not all-zeros or all-ffffs.*/
    if (CsrMemCmp(&cfg->stationMacAddress, &NullBssid, sizeof(cfg->stationMacAddress.data)) != 0)
    {
        /* Not all zeros... */
        if (CsrMemCmp(&cfg->stationMacAddress, &BroadcastBssid, sizeof(cfg->stationMacAddress.data)) != 0)
        {
            fsmData->flightmodeIndex = 1;
            for(ifIndex = unifi_GHZ_2_4; ifIndex <= unifi_GHZ_5_0; ifIndex++)
            {
                (void)mib_encode_add_set_os(context,  &mibSetDataRef, dot11MACAddress, (CsrUint8*)&cfg->stationMacAddress, 6, (CsrUint8)ifIndex, 0);
            }
        }
    }

    /* Flight Mode support.... Set calibration mode to none when flightmode boot
     * We do NOT need to set this on shutdown as it only affects the FIRST mlme reset! */
    (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiRadioCalibrationMode,
                                 (core_get_flightmodeBootFlag(context)?RADIO_CAL_NONE:RADIO_CAL_AUTO), 0, 0);

    /* Force the use of Scan averaging */
    (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiScanAverageType, 1, 0, 0);

    /* This is to configure the data plane interface */
    (void)mib_encode_add_set_int(context, &mibSetDataRef, unifiFirmwareDriverInterface, cfg->smeConfig.firmwareDriverInterface, 0, 0);

    /* Force dot11MultiDomainCapabilityEnabled OFF when 802,11a in use */
    if (cfg->smeConfig.ifIndex & unifi_GHZ_5_0)
    {
        for(ifIndex = unifi_GHZ_2_4; ifIndex <= unifi_GHZ_5_0; ifIndex++)
        {
            (void)mib_encode_add_set_boolean(context, &mibSetDataRef, dot11MultiDomainCapabilityEnabled, FALSE, (CsrUint8)ifIndex, 0);

        }
    }

#ifdef CCX_VARIANT

    ifIndex = cfg->smeConfig.ifIndex;

    if (cfg->ccxConfig.ccxRadioMgtEnabled == TRUE)
    {
        (void)mib_encode_add_set_boolean(context, &mibSetDataRef, unifiCCXCiscoRadioMeasurementCapabilityEnabled, TRUE, ifIndex, 0);
        sme_trace_debug((TR_MIB_ACCESS_FSM, "Setting unifiCCXCiscoRadioMeasurementCapabilityEnabled to TRUE"));
    }
    else
    {
        (void)mib_encode_add_set_boolean(context, &mibSetDataRef, unifiCCXCiscoRadioMeasurementCapabilityEnabled, FALSE, ifIndex, 0);
        sme_trace_debug((TR_MIB_ACCESS_FSM, "Setting unifiCCXCiscoRadioMeasurementCapabilityEnabled to FALSE"));
    }
#endif

    /* Use a Sub FSM to handle this event */
    mib_set_sub_fsm_start(context, *wait_for_init_mib_set__mlme_set_cfm, &mibSetDataRef, TRUE);
    sme_trace_entry((TR_MIB_ACCESS_FSM, "<<send_initial_mib_set"));
}


/**
 * @brief
 *   Idle State MibSaveCalibrationDataReq_Evt transition
 *
 * @par Description
 *   Gets the cal data from the mib for suspend and resume
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void save_calibration_data_req(FsmContext* context, const MibSaveCalibrationDataReq_Evt* req)
{
    MibAccessData* mibData = MIBACCESSDATA;
    SmeConfigData* cfg = get_sme_config(context);
    sme_trace_entry((TR_MIB_ACCESS_FSM, "save_calibration_data_req()"));

    mibData->reqSender = req->common.sender_;
    mibData->reqEventId = req->common.id;

    mibData->calibrationCurrentIndex = 0;

    if (cfg->calibrationData.dataLength != 0)
    {
        pld_release(getPldContext(context), cfg->calibrationData.slotNumber);
        cfg->calibrationData.dataLength = 0;
    }

    get_next_calibration_chunk(context);
}

static void idle_get_req(FsmContext* context, const MlmeGetReq_Evt* req)
{
    MibAccessData* mibData = MIBACCESSDATA;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "idle_get_req()"));

    mibData->reqSender = req->common.sender_;
    mibData->reqEventId = req->common.id;

    /* Use a Sub FSM to handle this event */
    mib_get_sub_fsm_start(context, *generic_get_cfm, &req->mibAttribute, TRUE);
}

/**
 * @brief
 *   idle State MibSet transition
 *
 * @par Description
 *   Makes a Set request to the MLME and moves to the
 *   waiting for cfm state
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : event
 *
 * @return
 *   void
 */
static void idle_set_req(FsmContext* context, const MlmeSetReq_Evt* req)
{
    unifi_DataBlock dataBlock = {0, NULL};
    MibAccessData* mibData = MIBACCESSDATA;
    sme_trace_entry((TR_MIB_ACCESS_FSM, "idle_set_req()"));
    mibData->reqSender = req->common.sender_;
    mibData->reqEventId = req->common.id;

    pld_access(getPldContext(context), req->mibAttributeValue.slotNumber, (void **)&dataBlock.data, &dataBlock.length);
    dataBlock.length = req->mibAttributeValue.dataLength;
    sme_trace_hex((TR_MIB_ACCESS_FSM, TR_LVL_DEBUG, "idle_set_req() Read Data", dataBlock.data, dataBlock.length));

    /* Use a Sub FSM to handle this event */
    mib_set_sub_fsm_start(context, *generic_set_cfm, &req->mibAttributeValue, TRUE);
}

/**
 * @brief
 *   Responds to a CORE_STOP_REQ from the higher layer
 *
 * @par Description
 *   If a CORE_STOP_REQ is received, we jump immediately to the stopped
 *   state.  Any stray MLME_[S/G]ET_CFMs that arrive afterwards have to be
 *   mopped up in that state, and we assume that no process is sitting
 *   waiting on them, by virtue of the fact that other processes are stopped
 *   before this one.
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Core Stop Request
 *
 * @return
 *   void
 */
static void core_stop(FsmContext* context, const CoreStopReq_Evt* req)
{
    sme_trace_entry((TR_MIB_ACCESS_FSM, "core_stop()"));
    send_core_stop_cfm(context, req->common.sender_, unifi_Success);
    fsm_next_state(context, FSMSTATE_wait_for_core_start);
}

/** State wait_for_core_start transitions */
static const FsmEventEntry waitForCoreStartTransitions[] =
{
                          /* Signal Id,                          Function */
    fsm_event_table_entry(CORE_START_REQ_ID,                     wait_for_core_start__core_start_req ),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                      core_stop),
    /* These should only be strays from aborted initialisation */
    fsm_event_table_entry(MLME_GET_CFM_ID,                       fsm_ignore_event ),
    fsm_event_table_entry(MLME_SET_CFM_ID,                       fsm_ignore_event ),
};

/** State wait_for_mib_init transitions */
static const FsmEventEntry waitForMibInitTransitions[] =
{
                          /* Signal Id,                          Function */
    fsm_event_table_entry(MIB_INIT_REQ_ID,                       wait_for_mib_init__mib_init_req ),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                      core_stop),
    /* These should only be strays from aborted initialisation */
    fsm_event_table_entry(MLME_GET_CFM_ID,                       fsm_ignore_event ),
    fsm_event_table_entry(MLME_SET_CFM_ID,                       fsm_ignore_event ),
};

static const FsmEventEntry waitForMibFileSetTransitions[] =
{
                          /* Signal Id,                          Function */
    fsm_event_table_entry(MLME_SET_CFM_ID,                       wait_for_mib_init_mlme_set__mlme_set_cfm ),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                      core_stop),
};

/** State idle transitions */
static const FsmEventEntry idleTransitions[] =
{
                          /* Signal Id,                          Function */
    fsm_event_table_entry(MLME_GET_REQ_ID,                       idle_get_req),
    fsm_event_table_entry(MLME_SET_REQ_ID,                       idle_set_req),
    fsm_event_table_entry(CORE_STOP_REQ_ID,                      core_stop),
    fsm_event_table_entry(MIB_SAVE_CALIBRATION_DATA_REQ_ID,      save_calibration_data_req),
    fsm_event_table_entry(UNIFI_MGT_MIB_GET_REQ_ID,              idle_mgt_mib_get_req),
    fsm_event_table_entry(UNIFI_MGT_MIB_GET_NEXT_REQ_ID,         idle_mgt_mib_get_next_req),
    fsm_event_table_entry(UNIFI_MGT_MIB_SET_REQ_ID,              idle_mgt_mib_set_req),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /*                    Signal Id,                             Function */
    fsm_event_table_entry(CORE_STOP_REQ_ID,                      fsm_saved_event),
    fsm_event_table_entry(UNIFI_MGT_MIB_GET_REQ_ID,              bounce_mgt_mib_get_req),
    fsm_event_table_entry(UNIFI_MGT_MIB_GET_NEXT_REQ_ID,         bounce_mgt_mib_get_next_req),
    fsm_event_table_entry(UNIFI_MGT_MIB_SET_REQ_ID,              bounce_mgt_mib_set_req),
};
/** Mib Access state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                    State                                              State                                     Save    */
   /*                    Name                                               Transitions                                *       */
   fsm_state_table_entry(FSMSTATE_wait_for_core_start,                      waitForCoreStartTransitions,              FALSE),
   fsm_state_table_entry(FSMSTATE_wait_for_mib_init,                        waitForMibInitTransitions,                FALSE),
   fsm_state_table_entry(FSMSTATE_wait_for_mib_file_set,                    waitForMibFileSetTransitions,             FALSE),
   fsm_state_table_entry(FSMSTATE_idle,                                     idleTransitions,                          FALSE),

};


const FsmProcessStateMachine mib_access_fsm = {
#ifdef FSM_DEBUG
       "MIB Access",                                                           /* SM Process Name       */
#endif
       MIB_ACCESS_PROCESS,                                                     /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                                       /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions,FALSE),  /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL,FALSE),                  /* ignore event handers */
       mib_access_init,                                                        /* Entry Function        */
       NULL,                                                                   /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                          /* Trace Dump Function   */
#endif
};

/*
 * FSM Scripts Config for this FSM
 *                              state                  nextstate                  event
 *                              -----                  ---------                  -----
 * fsm::remove_transition       idle                   idle                       .*
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/** @}
 */
