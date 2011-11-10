/** @file qos_tspec.c
 *
 * TSPEC support functions
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
 * TSPEC support functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_tspec.c#1 $
 *
 ****************************************************************************/

/** @{
 * @ingroup qos
 */

#include "qos_fsm/qos_tspec.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "ie_access/ie_access.h"


/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief extracts the tsinfo from a tspec
 *
 * @par Description:
 * See Brief
 *
 * @ingroup qos
 *
 * @param[in] tspec : tspec to be processed
 *
 * @return
 *      CsrUint32 : extracted tsinfo
 */
/*---------------------------------------------------------------------------*/
static CsrUint32 get_tsinfo(
        CsrUint8 *tspec,
        CsrUint16 tspecLength)
{
    CsrUint32 tsinfo = 0;
    ie_wmm_tspec *pIeTspec;


    ie_result result = ie_get_wmm_tspec(tspec, tspecLength, pIeTspec);

    ie_trace_wmm_tspec(1, tspec);


    if (result ==ie_success)
    {
        ie_le_CsrUint24_noshift(pIeTspec->wmmTspecBodytsInfo, tsinfo);
    }
    sme_trace_debug((TR_QOS, "get_tsinfo() 0x%x.04", tsinfo));
    return tsinfo;
}

/**
 * @brief extracts the tid from a tspec
 *
 * @par Description:
 * See Brief
 *
 * @ingroup qos
 *
 * @param[in] tspec : tspec to be processed
 *
 * @return
 *      CsrUint8 : extracted tid
 */
/*---------------------------------------------------------------------------*/
static CsrUint8 get_tid(
        CsrUint8 *tspec,
        CsrUint16 tspecLength)
{
    CsrUint32 tsinfo = get_tsinfo(tspec, tspecLength);
    CsrUint8 tid = (CsrUint8)IE_WMM_TS_INFO__TID_GET(tsinfo);

    sme_trace_debug((TR_QOS, "get_tid() %d", tid));
    return tid;
}

/**
 * @brief extracts the dir from a tspec
 *
 * @par Description:
 * See Brief
 *
 * @ingroup qos
 *
 * @param[in] tspec : tspec to be processed
 *
 * @return
 *      tspecDirection : extracted direction
 */
/*---------------------------------------------------------------------------*/
static tspecDirection get_dir(
        CsrUint8 *tspec,
        CsrUint16 tspecLength)
{
    CsrUint32 tsinfo = get_tsinfo(tspec, tspecLength);
    tspecDirection tspecDir = IE_WMM_TS_INFO__DIRECTION_GET(tsinfo);

    sme_trace_debug((TR_QOS, "get_dir() %d", tspecDir));
    return tspecDir;
}


/**
 * @brief converts the tid to AC
 *
 * @par Description:
 * See Brief
 *
 * @ingroup qos
 *
 * @param[in] tid : tid to be converted
 *
 * @return
 *      qos_ac : converted ac
 */
/*---------------------------------------------------------------------------*/
static qos_ac tid_to_ac(
        CsrUint8 tid)
{
    qos_ac qosAc = AC_NONE;
    sme_trace_entry((TR_QOS, "tid_to_ac() tid %x", tid));

    /* top bit is masked */
    switch(tid)
    {
    case 0x01: /* 1 0001 */
    case 0x02: /* 2 0010 */
    case 0x09: /* 1 1001 */
    case 0x0A: /* 2 1010 */
        qosAc = AC_1;
        break;
    case 0x00: /* 0 0000 */
    case 0x03: /* 3 0011 */
    case 0x08: /* 0 1000 */
    case 0x0B: /* 3 1011 */
        qosAc = AC_2;
        break;
    case 0x04: /* 4 0100 */
    case 0x05: /* 5 0101 */
    case 0x0C: /* 4 1100 */
    case 0x0D: /* 5 1101 */
        qosAc = AC_3;
        break;
    case 0x06: /* 6 0110 */
    case 0x07: /* 7 0111 */
    case 0x0E: /* 6 1110 */
    case 0x0F: /* 7 1111 */
        qosAc = AC_4;
        break;
    default:
    {
        sme_trace_error((TR_QOS, "tid_to_ac() invalid "));
    }
    }
    return qosAc;
}

/**
 * @brief calculates a channel index from the ac and direction
 *
 * @par Description:
 * See Brief
 *
 * @ingroup qos
 *
 * @param[in] ac : tspec ac
 * @param[in] dir : tspec direction
 * @param[out] pChannelIndex : calculated channel
 *
 * @return
 *      CsrBool : TRUE:  successful
 *                FALSE: not successful
 */
/*---------------------------------------------------------------------------*/
static CsrBool channel_index(
        CsrUint8 ac,
        tspecDirection dir,
        CsrUint8 *pChannelIndex)
{
    CsrBool rtn = TRUE;

    switch(dir)
    {
    /* if it bi-direction assign it to the first channel slot. */
    case tspecDirection_bidirectional:
        *pChannelIndex = ac;
        break;
    case tspecDirection_uplink:
        *pChannelIndex = ac;
        break;
    case tspecDirection_downlink:
        *pChannelIndex = ac + 4;
        break;
    default:
        rtn = FALSE;
    }

    return rtn;
}



/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
void qos_initialise_tspec_handler(
        qos_tspecDataBlk *pDataBlk)
{
    CsrUint8 i;

    sme_trace_entry((TR_QOS, "qos_initialise_tspec_handler"));

    pDataBlk->currentDialogToken = 0;

    pDataBlk->tidMap = TID_NONE;

    pDataBlk->searchStatus = TSPEC_IDLE;
    pDataBlk->tspecSearchIndex = 0;

    pDataBlk->activeTspecs[0].ptspecsData = NULL;
    pDataBlk->activeTspecs[0].status = TSPEC_RESERVED;
    for (i=0; i<NUMBER_OF_ACTIVE_TSPECS; i++)
    {
        pDataBlk->activeTspecs[i+1].ptspecsData = NULL;
        pDataBlk->activeTspecs[i+1].status = TSPEC_INACTIVE;
    }
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
void qos_reset_tspec_handler(
        qos_tspecDataBlk *pDataBlk)
{
    CsrUint8 i;
    sme_trace_entry((TR_QOS, "qos_reset_tspec_handler()"));

    for (i=1; i<NUMBER_OF_ACTIVE_TSPECS+1; i++)
    {
        if(pDataBlk->activeTspecs[i].ptspecsData != NULL)
        {
            CsrPfree(pDataBlk->activeTspecs[i].ptspecsData->tspec);
            CsrPfree(pDataBlk->activeTspecs[i].ptspecsData->tclas);
            CsrPfree(pDataBlk->activeTspecs[i].ptspecsData);

            pDataBlk->activeTspecs[i].ptspecsData = NULL;
            pDataBlk->activeTspecs[i].status = TSPEC_INACTIVE;
        }
    }
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
unifi_TspecResultCode validate_tspec_content(
        FsmContext* context,
        CsrUint16 tspecLength,
        CsrUint8 *tspec)
{
    CsrUint8 tid;
    qos_ac ac = AC_NONE;

    sme_trace_entry((TR_QOS, "validate_tspec()"));

    /* make sure the length is correct */
    if(tspecLength != IE_WMM_TSPEC__TOTAL_SIZE)
    {
        sme_trace_error((TR_QOS, "iesize %d should be %d", tspecLength, IE_WMM_TSPEC__TOTAL_SIZE));
        return unifi_TspecResultIeLengthIncorrect;
    }

    tid = get_tid(tspec, tspecLength);
    ac = tid_to_ac(tid);

    if (ac == AC_NONE)
    {
        return unifi_TspecResultInvalidTspecParameters;
    }

    return unifi_TspecResultSuccess;
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
unifi_TspecResultCode validate_tspec_position(
        FsmContext* context,
        qos_tspecDataBlk *pDataBlk,
        CsrUint16 tspecLength,
        CsrUint8 *tspec)
{
    CsrUint8 tid = get_tid(tspec, tspecLength);

    sme_trace_entry((TR_QOS, "pDataBlk->activeTspecs[%d].status() %d", tid, pDataBlk->activeTspecs[tid].status));

    if(pDataBlk->activeTspecs[tid].status != TSPEC_INACTIVE)
    {
        sme_trace_warn((TR_QOS, "tspec already install."));
        return unifi_TspecResultTidAlreadyInstalled;
    }
    return unifi_TspecResultSuccess;
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
DialogToken qos_get_dialog_token(
        qos_tspecDataBlk *pDataBlk)
{
    DialogToken dialogToken;

    sme_trace_entry((TR_QOS, "qos_get_dialog_token()"));

    dialogToken = pDataBlk->currentDialogToken;

    /* advance to the next token */
    pDataBlk->currentDialogToken++;

    return dialogToken;
}


/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
qos_tspecData* qos_add_tspec_data(
        qos_tspecDataBlk *pDataBlk,
        const UnifiMgtTspecReq_Evt* req)
{
    qos_ac ac = AC_NONE;
    CsrUint8 tid;
    CsrUint8 channelIndex;
    CsrUint32 tsInfo;
    tspecDirection dir;
    sme_trace_entry((TR_QOS, "qos_add_tspec_data"));

    tid = get_tid(req->tspec, req->tspecLength);
    dir = get_dir(req->tspec, req->tspecLength);
    tsInfo = get_tsinfo(req->tspec, req->tspecLength);

    ac = tid_to_ac(tid);

    if(channel_index(ac, dir, &channelIndex))
    {
        qos_tspecData *tspecData = CsrPmalloc(sizeof(qos_tspecData));

        sme_trace_debug((TR_QOS, "qos_add_tspec_data: creating new node"));

        /* get the tid to determine calculate where to place it. */
        tspecData->tspec = req->tspec;
        tspecData->tspecLength = req->tspecLength;
        tspecData->tclas = req->tclas;
        tspecData->tclasLength = req->tclasLength;
        tspecData->tsInfo = tsInfo;
        tspecData->tid = tid;
        tspecData->transactionId = req->transactionId;
        tspecData->strict = req->strict;
        tspecData->ctrlMask = req->ctrlMask;
        tspecData->dialogToken = qos_get_dialog_token(pDataBlk);
        tspecData->resultCode = unifi_TspecResultSuccess;
        tspecData->tspecOrigin = TSPEC_INITALIZED;
        tspecData->suggestTspecCount = 0;


        sme_trace_debug((TR_QOS, "tspec         %d", tspecData->tspec));
        sme_trace_debug((TR_QOS, "tsInfo        %d", tspecData->tsInfo));
        sme_trace_debug((TR_QOS, "tid           %d", tspecData->tid));
        sme_trace_debug((TR_QOS, "transactionId %d", tspecData->transactionId));
        sme_trace_debug((TR_QOS, "dialogToken   %d", tspecData->dialogToken));

        pDataBlk->activeTspecs[tid].ptspecsData = tspecData;

        return tspecData;
    }
    else
    {
        return NULL;
    }
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
void qos_delete_tspec_data_by_tid(
        qos_tspecDataBlk *pDataBlk,
        CsrUint8 tid)
{
    sme_trace_entry((TR_QOS, "qos_delete_tspec_data_by_tid"));

    CsrPfree(pDataBlk->activeTspecs[tid].ptspecsData->tspec);
    CsrPfree(pDataBlk->activeTspecs[tid].ptspecsData->tclas);
    CsrPfree(pDataBlk->activeTspecs[tid].ptspecsData);

    pDataBlk->activeTspecs[tid].ptspecsData = NULL;
    pDataBlk->activeTspecs[tid].status = TSPEC_INACTIVE;
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
void qos_delete_tspec_data_by_transaction_id(
        qos_tspecDataBlk *pDataBlk,
        CsrUint32 transactionId)
{
    CsrUint8 i;
    sme_trace_entry((TR_QOS, "qos_delete_tspec_data_by_transaction_id %d", transactionId));

    for (i=1; i<NUMBER_OF_ACTIVE_TSPECS+1; i++)
    {
        if(pDataBlk->activeTspecs[i].ptspecsData != NULL)
        {
            if(pDataBlk->activeTspecs[i].ptspecsData->transactionId == transactionId)
            {
                qos_delete_tspec_data_by_tid(pDataBlk, pDataBlk->activeTspecs[i].ptspecsData->tid);
            }
        }
    }
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
qos_tspecData* qos_get_tspec_data_by_tid(
        qos_tspecDataBlk *pDataBlk,
        CsrUint8 tid)
{
    return(pDataBlk->activeTspecs[tid].ptspecsData);
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
qos_tspecData* qos_get_tspec_data_by_transaction_id(
        qos_tspecDataBlk *pDataBlk,
        CsrUint32 transactionId)
{
    CsrUint8 i;

    for (i=1; i<NUMBER_OF_ACTIVE_TSPECS+1; i++)
    {
        if(pDataBlk->activeTspecs[i].ptspecsData != NULL)
        {
            if(pDataBlk->activeTspecs[i].ptspecsData->transactionId == transactionId)
            {
                return qos_get_tspec_data_by_tid(pDataBlk, pDataBlk->activeTspecs[i].ptspecsData->tid);
            }
        }
    }
    return NULL;
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
void clear_active_tspec_list(
        qos_tspecDataBlk *pDataBlk)
{
    CsrUint8 i;
    sme_trace_entry((TR_QOS, "clear_active_tspec_list"));

    /* zero out the list */
    for (i=1; i<NUMBER_OF_ACTIVE_TSPECS+1; i++)
    {
        if (pDataBlk->activeTspecs[i].status != TSPEC_INACTIVE)
        {
            pDataBlk->activeTspecs[i].status = TSPEC_IDLE;
        }
    }
}

static qos_tspecData* find_active_tspec(
        qos_tspecDataBlk *pDataBlk,
        CsrUint8 startIndex,
        qos_tspec_status status)
{
    sme_trace_entry((TR_QOS, "qos_get_first_active_tspec, startIndex %d, status %d",startIndex, status));

    while (startIndex <= NUMBER_OF_ACTIVE_TSPECS)
    {
        if (pDataBlk->activeTspecs[startIndex].status & status)
        {
            return pDataBlk->activeTspecs[startIndex].ptspecsData;
        }

        startIndex++;
    }
    return NULL;
}
/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
qos_tspecData* qos_get_first_active_tspec(
        qos_tspecDataBlk *pDataBlk,
        qos_tspec_status status)
{
    qos_tspecData *pTspecData;
    sme_trace_entry((TR_QOS, "qos_get_first_active_tspec, %d", status));

    pDataBlk->tspecSearchIndex= 1;
    pDataBlk->searchStatus = status;

    pTspecData = find_active_tspec(pDataBlk, pDataBlk->tspecSearchIndex, pDataBlk->searchStatus);

    if (pTspecData != NULL)
    {
        pDataBlk->tspecSearchIndex = pTspecData->tid;
        return pTspecData;
    }

    return NULL;
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
qos_tspecData* qos_get_next_active_tspec(
        qos_tspecDataBlk *pDataBlk)
{
    qos_tspecData *pTspecData;

    sme_trace_entry((TR_QOS, "qos_get_next_active_tspec"));
    pDataBlk->tspecSearchIndex++;

    pTspecData = find_active_tspec(pDataBlk, pDataBlk->tspecSearchIndex, pDataBlk->searchStatus);

    if (pTspecData != NULL)
    {
        pDataBlk->tspecSearchIndex = pTspecData->tid;
        return pTspecData;
    }

    return NULL;
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
CsrBool qos_tspec_associate_req_tspec_signalling(
        qos_tspecDataBlk *pDataBlk,
        CsrUint8 **ppTspecBuf,
        CsrUint16 *pTspecLength)
{
    sme_trace_entry((TR_QOS, "qos_tspec_associate_req_tspec_signalling"));

    if( pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_SIGNALLING_INDEX].ptspecsData == NULL)
    {
        sme_trace_debug((TR_QOS, "Signaling TSPEC  = NULL"));
        *ppTspecBuf = NULL;
        *pTspecLength = 0;
        return FALSE;
    }

    *ppTspecBuf = pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_SIGNALLING_INDEX].ptspecsData->tspec;
    *pTspecLength = pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_SIGNALLING_INDEX].ptspecsData->tspecLength;
    return TRUE;
 }

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
CsrBool qos_tspec_get_associate_req_tspec_voice(
        qos_tspecDataBlk *pDataBlk,
        CsrUint8 **ppTspecBuf,
        CsrUint16 *pTspecLength)
{
    sme_trace_entry((TR_QOS, "qos_tspec_get_associate_req_tspec_voice"));

    if( pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_VOICE_INDEX].ptspecsData == NULL)
    {
        sme_trace_debug((TR_QOS, "Voice TSPEC  = NULL"));
        *ppTspecBuf = NULL;
        *pTspecLength = 0;
        return FALSE;
    }

    *ppTspecBuf = pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_VOICE_INDEX].ptspecsData->tspec;
    *pTspecLength = pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_VOICE_INDEX].ptspecsData->tspecLength;
    return TRUE;
}

/*
 * Description:
 * See description in ccx/ccx_ie_access_qos.h
 */
/*---------------------------------------------------------------------------*/
CsrUint16 qos_tspec_associate_tspec_length(
        qos_tspecDataBlk *pTspecDataBlk)
{
    CsrUint16 totalLength = 0;
    CsrUint8 *pTspecBuf;
    CsrUint16 tspecBufLength;

    sme_trace_entry((TR_QOS, "qos_tspec_associate_tspec_length()"));

    (void)qos_tspec_associate_req_tspec_signalling(pTspecDataBlk, &pTspecBuf, &tspecBufLength);
    totalLength += tspecBufLength;

    (void)qos_tspec_get_associate_req_tspec_voice(pTspecDataBlk, &pTspecBuf, &tspecBufLength);
    totalLength += tspecBufLength;

    sme_trace_debug((TR_QOS, "qos_associate_tspec_length() totalLength %d", totalLength));

    return( totalLength );
}

/*
 * Description:
 * See description in ccx/ccx_ie_access_qos.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8* qos_tspec_get_associate_tspec_ies(
        qos_tspecDataBlk *pTspecDataBlk,
        CsrUint8 *pbuf)
{
    CsrUint8 *pTspecBuf;
    CsrUint16 tspecBufLength;

    sme_trace_entry((TR_QOS, "qos_tspec_get_associate_tspec_ies() "));

    if (qos_tspec_associate_req_tspec_signalling(pTspecDataBlk, &pTspecBuf, &tspecBufLength))
    {
        CsrMemCpy(pbuf, pTspecBuf, tspecBufLength);
        pbuf += tspecBufLength;
    }

    if (qos_tspec_get_associate_req_tspec_voice(pTspecDataBlk, &pTspecBuf, &tspecBufLength))
    {
        CsrMemCpy(pbuf, pTspecBuf, tspecBufLength);
        pbuf += tspecBufLength;
    }

    return pbuf;
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
void qos_tspec_update_associate_tspec_status(
        qos_tspecDataBlk *pTspecDataBlk,
        qos_tspec_status status)
{
    sme_trace_entry((TR_QOS, "qos_tspec_update_associate_tspec_status() status %d",status));

    if (pTspecDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_SIGNALLING_INDEX].ptspecsData != NULL)
    {
        pTspecDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_SIGNALLING_INDEX].status = status;
    }
    if (pTspecDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_VOICE_INDEX].ptspecsData != NULL)
    {
        pTspecDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_VOICE_INDEX].status = status;
    }
}

/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
void qos_tspec_update_tspec_status(
        qos_tspecDataBlk *pTspecDataBlk,
        CsrUint8 tid,
        qos_tspec_status status)
{
    sme_trace_entry((TR_QOS, "qos_tspec_update_associate_tspec_status() status %d",status));

    if (pTspecDataBlk->activeTspecs[tid].ptspecsData != NULL)
    {
        pTspecDataBlk->activeTspecs[tid].status = status;
    }
    else
    {
        sme_trace_error((TR_QOS, "qos_tspec_update_associate_tspec_status() trying to set the status of a non active TS: status %d",status));
    }

}


/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
CsrBool qos_tspec_process_associate_cfm_ie(
        qos_tspecDataBlk *pDataBlk,
        CsrUint8 *pTspecBuf,
        CsrUint16 tspecLength,
        ResultCode resultCode)
{
    unifi_TspecResultCode tspecResultCode;

    sme_trace_entry((TR_QOS, "qos_tspec_validate_associate_cfm_ie"));

    switch (resultCode)
    {
    case ResultCode_Success:                { tspecResultCode = unifi_TspecResultSuccess;                   break; }
    case ResultCode_UnspecifiedQosFailure:  { tspecResultCode = unifi_TspecResultUnspecifiedFailure;        break; }
    case ResultCode_WrongPolicy:            { tspecResultCode = unifi_TspecResultWrongPolicy;               break; }
    case ResultCode_InsufficientBandwidth:  { tspecResultCode = unifi_TspecResultInsufficientBandwidth;     break; }
    case ResultCode_InvalidTspecParameters: { tspecResultCode = unifi_TspecResultInvalidTspecParameters;    break; }
    default:                                { tspecResultCode = unifi_TspecResultUnspecifiedFailure;        break; }
    }

    if (pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_SIGNALLING_INDEX].ptspecsData != NULL) {
        pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_SIGNALLING_INDEX].ptspecsData->resultCode = tspecResultCode;
    }
    if (pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_VOICE_INDEX].ptspecsData != NULL) {
       pDataBlk->activeTspecs[ASSOCIATE_REQ_TSPEC_VOICE_INDEX].ptspecsData->resultCode = tspecResultCode;
    }

    return TRUE;
}


/*
 * Description:
 * See description in qos_fsm/qos_tspec.h
 */
/*---------------------------------------------------------------------------*/
CsrBool qos_tspec_check_tspec_active(
        qos_tspecDataBlk *pTspecDataBlk,
        CsrUint8 tid)
{
    sme_trace_entry((TR_QOS, "qos_tspec_get_tspec_status() tid%d", tid));

    /*
    for (i=1; i<NUMBER_OF_ACTIVE_TSPECS; i++)
    {
        if (pTspecDataBlk->activeTspecs[i].ptspecsData != NULL)
        {
            sme_trace_debug((TR_QOS, "qos_tspec_get_tspec_status(%d) status %d",i, pTspecDataBlk->activeTspecs[i].status));
        }
        else
        {
            sme_trace_entry((TR_QOS, "qos_tspec_get_tspec_status(%d) INACTIVE", i ));
        }
    }
*/
    if (pTspecDataBlk->activeTspecs[tid].ptspecsData != NULL)
    {
        sme_trace_debug((TR_QOS, "qos_tspec_get_tspec_status() status %d",pTspecDataBlk->activeTspecs[tid].status));
        return TRUE;
    }
    else
    {
        sme_trace_entry((TR_QOS, "qos_tspec_get_tspec_status() INACTIVE"));
        return FALSE;
    }
}
