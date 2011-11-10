/** @file sme_interface_hip_signal_to_sys_sap.c
 *
 * Processes Hip signals from the sme and passes them to the driver
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
 *   Builds a buffer from the signal and and payloads in the Payload Manager
 *   then passes the buffer to the driver
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/saps/sys_sap/sme_interface_hip_signal_to_sys_sap.c#2 $
 *
 ****************************************************************************/

#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "fsm/csr_wifi_fsm.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "sys_sap/sme_interface_hip_signal_to_sys_sap.h"
#include "sys_sap/sme_interface_hip_auto_cfm.h"
#include "sys_sap/hip_signal_header.h"

#include "sys_sap/sys_sap_from_sme_interface.h"
#include "payload_manager/payload_manager.h"

#include "event_pack_unpack/event_pack_unpack.h"

static void build_auto_cfm(FsmContext* context, const FsmEvent* eventHeader)
{
    FsmEvent* autoevt = NULL;
    switch(eventHeader->id)
    {
    case MLME_ASSOCIATE_REQ_ID:
        build_mlme_associate_cfm(autoevt, eventHeader->sender_,
                                 NullDataReference,
                                 NullDataReference,
                                 ResultCode_UnspecifiedFailure,
                                 0, 0, 0, 0, 0, 0);
        break;
    case MLME_REASSOCIATE_REQ_ID:
        build_mlme_reassociate_cfm(autoevt, eventHeader->sender_,
                                 NullDataReference,
                                 NullDataReference,
                                 ResultCode_UnspecifiedFailure,
                                 0, 0, 0, 0, 0, 0);
        break;
    case MLME_AUTHENTICATE_REQ_ID:
        build_mlme_authenticate_cfm(autoevt, eventHeader->sender_,
                                    NullDataReference,
                                    NullBssid,
                                    AuthenticationType_OpenSystem,
                                    ResultCode_UnspecifiedFailure);
        break;
    case MLME_DEAUTHENTICATE_REQ_ID:
        build_mlme_deauthenticate_cfm(autoevt, eventHeader->sender_,
                                      NullBssid,
                                      ResultCode_UnspecifiedFailure);
        break;
    case MLME_JOIN_REQ_ID:
        build_mlme_join_cfm(autoevt, eventHeader->sender_, NullDataReference, ResultCode_UnspecifiedFailure);
        break;
    case MLME_START_REQ_ID:
        build_mlme_start_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure, NullBssid);
        break;
    case MLME_SETKEYS_REQ_ID:
        build_mlme_setkeys_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;
    case MLME_DELETEKEYS_REQ_ID:
        build_mlme_deletekeys_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;
/*  case MLME_EAPOL_REQ_ID:
        build_mlme_eapol_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;*/
    case MLME_GET_REQ_ID:
        build_mlme_get_cfm(autoevt, eventHeader->sender_,
                           NullDataReference, MibStatus_InvalidParameters, 0);
        break;
    case MLME_SET_REQ_ID:
        build_mlme_set_cfm(autoevt, eventHeader->sender_,
                           NullDataReference, MibStatus_InvalidParameters, 0);
        break;
    case MLME_GET_NEXT_REQ_ID:
        build_mlme_get_next_cfm(autoevt, eventHeader->sender_,
                                NullDataReference, MibStatus_InvalidParameters, 0);
        break;

    case MLME_SETPROTECTION_REQ_ID:
        build_mlme_setprotection_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;

    case MLME_SCAN_REQ_ID:
        build_mlme_scan_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;

    case MLME_RESET_REQ_ID:
        build_mlme_reset_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;

    case MLME_POWERMGT_REQ_ID:
        build_mlme_powermgt_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;

    case MLME_ADD_AUTONOMOUS_SCAN_REQ_ID:
        build_mlme_add_autonomous_scan_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure, 1);
        break;

    case MLME_DEL_AUTONOMOUS_SCAN_REQ_ID:
        build_mlme_del_autonomous_scan_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure, 1);
        break;

    case MLME_ADD_PERIODIC_REQ_ID:
        build_mlme_add_periodic_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure, 1);
        break;

    case MLME_DEL_PERIODIC_REQ_ID:
        build_mlme_del_periodic_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure, 1);
        break;

    case MLME_ADD_BLACKOUT_REQ_ID:
        build_mlme_add_blackout_cfm(autoevt, eventHeader->sender_, 1, ResultCode_UnspecifiedFailure);
        break;

    case MLME_DEL_BLACKOUT_REQ_ID:
        build_mlme_del_blackout_cfm(autoevt, eventHeader->sender_, 1, ResultCode_UnspecifiedFailure);
        break;

    case MLME_SET_UNITDATA_FILTER_REQ_ID:
        build_mlme_set_unitdata_filter_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;
    case MLME_ADDTS_REQ_ID:
        build_mlme_addts_cfm(autoevt, eventHeader->sender_,
                             NullDataReference, ResultCode_UnspecifiedFailure, 0);
        break;
    case MLME_DELTS_REQ_ID:
        build_mlme_delts_cfm(autoevt, eventHeader->sender_,
                             ResultCode_UnspecifiedFailure, NullBssid, 0 );
        break;

    case MLME_ADD_TRIGGERED_GET_REQ_ID:
        build_mlme_add_triggered_get_cfm(autoevt, eventHeader->sender_,ResultCode_UnspecifiedFailure, 1);
        break;

    case MLME_DEL_TRIGGERED_GET_REQ_ID:
        build_mlme_del_triggered_get_cfm(autoevt, eventHeader->sender_,ResultCode_UnspecifiedFailure, 1);
        break;

    case MLME_ADDBA_REQ_ID:
        build_mlme_addba_cfm(autoevt, eventHeader->sender_,
                             NullBssid, /* p1_Peer_QSTA_Address */
                             0, /* p2_Dialog_Token */
                             0, /* p3_TID */
                             ResultCode_UnspecifiedFailure,
                             0, /* p5_Block_Ack_Policy */
                             0, /* p6_Buffer_Size */
                             0 /* p7_Block_Ack_Timeout */);

        break;
    case MLME_DELBA_REQ_ID:
        build_mlme_delba_cfm(autoevt, eventHeader->sender_,
                             NullBssid, /* p1_Peer_QSTA_Address */
                             0, /* p2_Direction */
                             0, /* p3_TID */
                             ResultCode_UnspecifiedFailure);

        break;

#ifdef CSR_AMP_ENABLE
    case MLME_ADD_WDS_REQ_ID:
        build_mlme_add_wds_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;

    case MLME_DEL_WDS_REQ_ID:
        build_mlme_del_wds_cfm(autoevt, eventHeader->sender_, ResultCode_UnspecifiedFailure);
        break;
#endif

    case MLME_MEASURE_REQ_ID:
        build_mlme_measure_cfm(autoevt, eventHeader->sender_,
                               NullDataReference, ResultCode_UnspecifiedFailure, 0);
        break;

    case DEBUG_GENERIC_REQ_ID:
    case MLME_SNIFFJOIN_REQ_ID:
    case MLME_CHANNELSWITCH_REQ_ID:
    case MLME_MREQUEST_REQ_ID:
    case MLME_DLSTEARDOWN_REQ_ID:
    case MLME_SCHEDULE_REQ_ID:
    case MLME_HL_SYNC_REQ_ID:
    case MLME_DISASSOCIATE_REQ_ID:
    case MLME_TPCADAPT_REQ_ID:
    case MLME_LINKMEASURE_REQ_ID:
    case MLME_HL_SYNC_CANCEL_REQ_ID:
    case MLME_NEIGHBORREPREQ_REQ_ID:
    case MLME_AUTONOMOUS_SCAN_RESULTS_REQ_ID:
    case DS_UNITDATA_REQ_ID:
        sme_trace_crit((TR_SYS_SAP, "build_auto_cfm() : Error unhandled HIP Message 0x%.4X", eventHeader->id));
        break;
    default:
        break;
    }
    if (autoevt != NULL)
    {
        hip_auto_cfm_add(getSmeContext(context)->hipSapAuto, autoevt);
    }
}


void hip_signal_to_sys_sap(FsmContext* context, FsmEvent *evt, CsrUint16 evtSize)
{
    FsmEvent eventHeader;
    unifi_DataBlock mlmeCommand;
    unifi_DataBlock dataRef1;
    unifi_DataBlock dataRef2;
    PldHdl pldHnd1;
    PldHdl pldHnd2;
    PldContext* pldContext = getPldContext(context);
    CsrUint8* buffer = (CsrUint8*)evt;

    /* Decode the header info */
    eventHeader.id = event_unpack_CsrUint16(&buffer);
    eventHeader.destination = event_unpack_CsrUint16(&buffer);
    eventHeader.sender_ = event_unpack_CsrUint16(&buffer);
    pldHnd1 = event_unpack_CsrUint16(&buffer);
    dataRef1.length = event_unpack_CsrUint16(&buffer);
    pldHnd2 = event_unpack_CsrUint16(&buffer);
    dataRef2.length = event_unpack_CsrUint16(&buffer);

    mlmeCommand.length = evtSize;
    mlmeCommand.data = (CsrUint8*)evt;

    sme_trace_hex((TR_SYS_SAP, TR_LVL_DEBUG, "unifi_dd_hip_req(Event)", mlmeCommand.data, mlmeCommand.length));

    dataRef1.data = NULL;
    dataRef2.data = NULL;

    if (dataRef1.length != 0)
    {
        CsrUint16 length;
        pld_access(pldContext, pldHnd1, (void **)&dataRef1.data, &length);
        sme_trace_hex((TR_SYS_SAP, TR_LVL_DEBUG, "unifi_dd_hip_req(dataRef1)", dataRef1.data, dataRef1.length));
    }

    if (dataRef2.length != 0)
    {
        CsrUint16 length;
        pld_access(pldContext, pldHnd2, (void **)&dataRef2.data, &length);
        sme_trace_hex((TR_SYS_SAP, TR_LVL_DEBUG, "unifi_dd_hip_req(dataref2)", dataRef2.data, dataRef2.length));
    }

    build_auto_cfm(context, &eventHeader);

    /*sme_trace_error((TR_SYS_SAP, "unifi_sys_hip_req() Msg : 0x%.4X From : 0x%.4X",eventHeader.id, eventHeader.sender_));*/

    if (hip_auto_cfm_is_hip_sap_open(getSmeContext(context)->hipSapAuto))
    {
        call_unifi_sys_hip_req(context,
                               mlmeCommand.length,
                               mlmeCommand.data,
                               dataRef1.length,
                               dataRef1.data,
                               dataRef2.length,
                               dataRef2.data);
    }

    if (dataRef1.length != 0)
    {
        pld_release(pldContext, pldHnd1);
    }
    if (dataRef2.length != 0)
    {
        pld_release(pldContext, pldHnd2);
    }
}


