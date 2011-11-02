/** @file mib_utils.c
 *
 * mib access support function header file
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
 *   Provides the external function declarions for mib access process
 *   support functions.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mib_utils.c#2 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "hip_proxy_fsm/mib_utils.h"
#include "hip_proxy_fsm/mib_encoding.h"
#include "hip_proxy_fsm/hip_signal_proxy_fsm.h"

#include "sme_configuration/sme_configuration_fsm.h"
#include "security_manager_fsm/security_manager_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "smeio/smeio_trace_types.h"

void mib_util_send_get(FsmContext* context, mib_ids id, CsrUint8 index1, CsrUint8 index2)
{
    DataReference mibSetDataRef = mib_encode_create_get(context, 1);
    (void)mib_encode_add_get(context, &mibSetDataRef, id, index1, index2);
    send_mlme_get_req_internal(context, getSmeContext(context)->mibAccessInstance, mibSetDataRef);
}

void mib_util_send_set_int(FsmContext* context, mib_ids id, CsrInt32 value, CsrUint8 index1, CsrUint8 index2)
{
    DataReference mibSetDataRef = mib_encode_create_set(context, 1, 0);
    (void)mib_encode_add_set_int(context, &mibSetDataRef, id, value, index1, index2);
    send_mlme_set_req_internal(context, getSmeContext(context)->mibAccessInstance, mibSetDataRef);
}

void mib_util_send_set_os(FsmContext* context, mib_ids id, const CsrUint8* data, CsrUint8 datalength, CsrUint8 index1, CsrUint8 index2)
{
    DataReference mibSetDataRef = mib_encode_create_set(context, 0, 1);
    (void)mib_encode_add_set_os(context, &mibSetDataRef, id, data, datalength, index1, index2);
    send_mlme_set_req_internal(context, getSmeContext(context)->mibAccessInstance, mibSetDataRef);
}


void mib_util_encode_stats_req(FsmContext* context, DataReference* mibGetDataRef, unifi_RadioIF ifIndex)
{
    *mibGetDataRef = mib_encode_create_get(context, 27);

    (void)mib_encode_add_get(context, mibGetDataRef, dot11RetryCount,                       (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11MultipleRetryCount,               (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11ACKFailureCount,                  (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11FrameDuplicateCount,              (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11FCSErrorCount,                    (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, unifiTxDataRate,                       (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11FailedCount,                      (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11TransmittedFragmentCount,         (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11TransmittedFrameCount,            (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11WEPExcludedCount,                 (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11WEPICVErrorCount,                 (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11WEPUndecryptableCount,            (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11MulticastReceivedFrameCount,      (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11MulticastTransmittedFrameCount,   (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11ReceivedFragmentCount,            (CsrUint8)ifIndex, 0);

    /*(void)mib_encode_add_get(context, mibGetDataRef, dot11RSNA4WayHandshakeFailures,        (CsrUint8)ifIndex, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNATKIPCounterMeasuresInvoked,    (CsrUint8)ifIndex, 0);*/

    if(check_have_set_keys(context, hip_proxy_get_security_manager(context)))
    {
        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsTKIPLocalMICFailures, (CsrUint8)ifIndex, 1);
        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsTKIPLocalMICFailures, (CsrUint8)ifIndex, 2);

        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsTKIPReplays,          (CsrUint8)ifIndex, 1);
        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsTKIPReplays,          (CsrUint8)ifIndex, 2);

        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsTKIPICVErrors,        (CsrUint8)ifIndex, 1);
        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsTKIPICVErrors,        (CsrUint8)ifIndex, 2);

        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsCCMPReplays,          (CsrUint8)ifIndex, 1);
        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsCCMPReplays,          (CsrUint8)ifIndex, 2);

        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsCCMPDecryptErrors,    (CsrUint8)ifIndex, 1);
        (void)mib_encode_add_get(context, mibGetDataRef, dot11RSNAStatsCCMPDecryptErrors,    (CsrUint8)ifIndex, 2);
    }
    if (get_sme_config(context)->mibConfig.dot11RtsThreshold < 3000)
    {
        (void)mib_encode_add_get(context, mibGetDataRef, dot11RTSSuccessCount,               (CsrUint8)ifIndex, 0);
        (void)mib_encode_add_get(context, mibGetDataRef, dot11RTSFailureCount,               (CsrUint8)ifIndex, 0);
    }
}

void mib_util_send_stats_req(FsmContext* context, unifi_RadioIF ifIndex)
{
    DataReference mibGetDataRef;

    mib_util_encode_stats_req(context, &mibGetDataRef, ifIndex);

    send_mlme_get_req_internal(context, getSmeContext(context)->mibAccessInstance, mibGetDataRef);
}

CsrBool mib_util_decode_stats(FsmContext* context, const DataReference* mibData, MibStatus status, CsrUint16 errorIndex, unifi_ConnectionStats* stats)
{
    CsrInt32 readInt;
    CsrUint8 mibIndex = 0;
    CsrUint32    counter1, counter2;

    /* make sure all values have been defaulted */
    CsrMemSet(stats, 0, sizeof(unifi_ConnectionStats));

    if (status != MibStatus_Successful)
    {
        return FALSE;
    }

    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11RetryCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11MultipleRetryCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11AckFailureCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11FrameDuplicateCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11FcsErrorCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, &readInt);
    stats->unifiTxDataRate = (CsrUint8)readInt;
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11FailedCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11TransmittedFragmentCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11TransmittedFrameCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11WEPExcludedCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11WEPICVErrorCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11WEPUndecryptableCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11MulticastReceivedFrameCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11MulticastTransmittedFrameCount);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11ReceivedFragmentCount);

    /*(void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11RSNA4WayHandshakeFailures);
    (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11RSNATKIPCounterMeasuresInvoked);*/

    if(check_have_set_keys(context, hip_proxy_get_security_manager(context)))
    {
        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter1);
        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter2);
        stats->dot11RSNAStatsTKIPLocalMICFailures = counter1 + counter2;

        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter1);
        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter2);
        stats->dot11RSNAStatsTKIPReplays = counter1 + counter2;

        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter1);
        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter2);
        stats->dot11RSNAStatsTKIPICVErrors = counter1 + counter2;

        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter1);
        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter2);
        stats->dot11RSNAStatsCCMPReplays = counter1 + counter2;

        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter1);
        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&counter2);
        stats->dot11RSNAStatsCCMPDecryptErrors = counter1 + counter2;
    }
    else
    {
        stats->dot11RSNAStatsTKIPLocalMICFailures = 0;
        stats->dot11RSNAStatsTKIPReplays = 0;
        stats->dot11RSNAStatsTKIPICVErrors = 0;
        stats->dot11RSNAStatsCCMPReplays = 0;
        stats->dot11RSNAStatsCCMPDecryptErrors = 0;
    }
    if (errorIndex > mibIndex)
    {
        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11RtsSuccessCount);
        (void)mib_decode_get_int(context, mibData, mibIndex++, (CsrInt32*)&stats->dot11RtsFailureCount);
    }
    else
    {
        stats->dot11RtsSuccessCount = 0;
        stats->dot11RtsFailureCount = 0;
    }

    return TRUE;

}


void mib_util_encode_linkq_req(FsmContext* context, DataReference* mibGetDataRef, unifi_RadioIF ifIndex)
{
    *mibGetDataRef = mib_encode_create_get(context, 2);

    (void)mib_encode_add_get(context, mibGetDataRef, unifiRSSI,    0, 0);
    (void)mib_encode_add_get(context, mibGetDataRef, unifiSNR,    0, 0);
}

void mib_util_send_linkq_req(FsmContext* context, unifi_RadioIF ifIndex)
{
    DataReference mibGetDataRef;

    mib_util_encode_linkq_req(context, &mibGetDataRef, ifIndex);

    send_mlme_get_req_internal(context, getSmeContext(context)->mibAccessInstance, mibGetDataRef);
}

CsrBool mib_util_decode_linkq(FsmContext* context, const DataReference* mibData, MibStatus status, CsrUint16 errorIndex, unifi_LinkQuality* linkq)
{
    CsrInt32 readInt;

    if (status != MibStatus_Successful)
    {
        return FALSE;
    }

    (void)mib_decode_get_int(context, mibData, 0, &readInt);
    linkq->unifiRssi = (CsrInt16)readInt;
    (void)mib_decode_get_int(context, mibData, 1, &readInt);
    linkq->unifiSnr = (CsrInt16)readInt;
    return TRUE;

}
