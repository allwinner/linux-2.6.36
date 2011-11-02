/** @file sme_interface_hip_signal_from_sys_sap.c
 *
 * Processes Hip signals from the driver and injects them into the sdl runtime
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
 *   Extracts any payloads and passes them to the Payload Manager then
 *   constructs a signal and injects it into the SDL
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/saps/sys_sap/sme_interface_hip_signal_from_sys_sap.c#3 $
 *
 ****************************************************************************/

#include "abstractions/osa.h"
#include "fsm/csr_wifi_fsm.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "sys_sap/sys_sap.h"
#include "sys_sap/hip_signal_header.h"
#include "hostio/hip_fsm_events.h"

#include "payload_manager/payload_manager.h"

#ifdef CCX_VARIANT
#include "ccx_ie_access/ccx_ie_access.h"
#endif

static CsrBool storeDataRefs(FsmContext* context,
                             CsrUint16 mlmeCommandLength,
                             const CsrUint8 *mlmeCommand,
                             CsrUint16 dataRef1Length,
                             const CsrUint8 *dataRef1,
                             CsrUint16 dataRef2Length,
                             const CsrUint8 *dataRef2,
                             DataReference* dataRefResult1,
                             DataReference* dataRefResult2)
{
    sme_trace_debug_code(
        const CsrUint8* tracebuffer = mlmeCommand;
        CsrUint16 id = event_unpack_CsrUint16((CsrUint8 **)&tracebuffer);
    )

    const CsrUint8* buffer = mlmeCommand;
    buffer += 8; /* Skip the 4 bytes for id, dest, sender and 1st slotnumber */
    dataRefResult1->dataLength = event_unpack_CsrUint16((CsrUint8 **)&buffer);
    buffer += 2; /* Skip the slot number */
    dataRefResult2->dataLength = event_unpack_CsrUint16((CsrUint8 **)&buffer);

    /* Check the bulk data for consistency */
    verify(TR_SYS_SAP, dataRefResult1->dataLength == dataRef1Length);
    verify(TR_SYS_SAP, dataRefResult2->dataLength == dataRef2Length);

    if (dataRefResult1->dataLength != dataRef1Length)
    {
        sme_trace_error((TR_SYS_SAP, "storeDataRef() dataRefResult1->dataLength != dataRef1Length"));
        return FALSE;
    }

    if (dataRefResult2->dataLength != dataRef2Length)
    {
        sme_trace_error((TR_SYS_SAP, "storeDataRef() dataRefResult2->dataLength != dataRef2Length"));
        return FALSE;
    }

    if (dataRefResult1->dataLength != 0)
    {
        pld_store_extra(getPldContext(context), (void *)dataRef1, dataRefResult1->dataLength, &dataRefResult1->slotNumber, id);
        sme_trace_debug((TR_SYS_SAP, "storeDataRefs(1) :: HIP MSG(0x%.4X) :: handle:0x%.4x length:%d", id, dataRefResult1->slotNumber, dataRefResult1->dataLength));
    }

    if (dataRefResult2->dataLength != 0)
    {
        pld_store_extra(getPldContext(context), (void *)dataRef2, dataRefResult2->dataLength, &dataRefResult2->slotNumber, id);
        sme_trace_debug((TR_SYS_SAP, "storeDataRefs(2) :: HIP MSG(0x%.4X) :: handle:0x%.4x length:%d", id, dataRefResult2->slotNumber, dataRefResult2->dataLength));
    }

    return TRUE;
}
/* -------------------------------------------------- */

static CsrBool send_hip_message(FsmContext* context, const FsmEvent* eventHeader, const CsrUint8* hipEvent, CsrUint16 hip_event_size, const DataReference* dataRef1, const DataReference* dataRef2 )
{
    CsrBool messageOk = TRUE;
    CsrBool hasMatchingCfm = TRUE;
    CsrBool optionalMatchingCfm = FALSE;
    sme_trace_debug((TR_SYS_SAP, "send_hip_message()"));
    switch(eventHeader->id)
    {
    case MLME_DISASSOCIATE_IND_ID:
    case MLME_DEAUTHENTICATE_IND_ID:
    case MA_UNITDATA_IND_ID:
    case MA_UNITDATA_CFM_ID:
    case MLME_PROTECTEDFRAMEDROPPED_IND_ID:
    case MLME_MICHAELMICFAILURE_IND_ID:
    case MLME_SCAN_IND_ID:
    case MLME_AUTONOMOUS_SCAN_IND_ID:
    case DEBUG_STRING_IND_ID:
    case DEBUG_WORD16_IND_ID:
    case DEBUG_GENERIC_IND_ID:
    case MLME_TRIGGERED_GET_IND_ID:
    case MLME_DELTS_IND_ID:
    case MLME_ADDTS_IND_ID:
    case MLME_DELBA_IND_ID:
    case MLME_ADDBA_IND_ID:

#ifdef CSR_AMP_ENABLE
    case MLME_AUTHENTICATE_IND_ID:
    case MLME_ASSOCIATE_IND_ID:
#endif
/*
    case MLME_SCHEDULE_IND_ID:
    case MLME_ASSOCIATE_IND_ID:
    case DS_UNITDATA_IND_ID:
    case MLME_REASSOCIATE_IND_ID:
    case MLME_NEIGHBORREP_IND_ID:
    case MLME_MREQUEST_IND_ID:
    case MLME_DLS_IND_ID:
    case MLME_AUTHENTICATE_IND_ID:
    case MLME_MREPORT_IND_ID:
    case MA_SNIFFDATA_IND_ID:
    case MLME_HL_SYNC_IND_ID:
    case MLME_DLSTEARDOWN_IND_ID:
    case MLME_STAKEYESTABLISHED_IND_ID:
    case MLME_AUTONOMOUS_SCAN_RESULTS_IND_ID:
*/
        hasMatchingCfm = FALSE;
        break;

    case MLME_CONNECTED_IND_ID:
        optionalMatchingCfm = TRUE;
        break;

    default:
        messageOk = hip_auto_cfm_has_matching_cfm(getSmeContext(context)->hipSapAuto, eventHeader->id);
        break;
    }

    if (messageOk)
    {
        switch(eventHeader->id)
        {
        case MLME_ASSOCIATE_CFM_ID:
            send_mlme_associate_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
#ifdef CSR_AMP_ENABLE
        case MLME_ASSOCIATE_IND_ID:
            send_mlme_associate_ind_from_hip(context, getSmeContext(context)->palMgrFsmInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
#endif
        case MLME_REASSOCIATE_CFM_ID:
            send_mlme_reassociate_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_AUTHENTICATE_CFM_ID:
            send_mlme_authenticate_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
#ifdef CSR_AMP_ENABLE
        case MLME_AUTHENTICATE_IND_ID:
            send_mlme_authenticate_ind_from_hip(context, getSmeContext(context)->palMgrFsmInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
#endif
        case MLME_DEAUTHENTICATE_CFM_ID:
            send_mlme_deauthenticate_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_JOIN_CFM_ID:
            send_mlme_join_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_START_CFM_ID:
            send_mlme_start_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_CONNECTED_IND_ID:
            send_mlme_connected_ind_from_hip(context, getSmeContext(context)->hipProxyInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_DEAUTHENTICATE_IND_ID:
            send_mlme_deauthenticate_ind_from_hip(context, getSmeContext(context)->hipProxyInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_DISASSOCIATE_IND_ID:
            send_mlme_disassociate_ind_from_hip(context, getSmeContext(context)->hipProxyInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MA_UNITDATA_IND_ID:
            send_ma_unitdata_ind_from_hip(context, getSmeContext(context)->hipProxyInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
#ifdef CSR_AMP_ENABLE
        case MA_UNITDATA_CFM_ID:
            send_ma_unitdata_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_ADD_WDS_CFM_ID:
            send_mlme_add_wds_cfm_from_hip(context, getSmeContext(context)->palDmFsmInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_DEL_WDS_CFM_ID:
            send_mlme_del_wds_cfm_from_hip(context, getSmeContext(context)->palDmFsmInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
#endif
        case MLME_SETKEYS_CFM_ID:
            send_mlme_setkeys_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_DELETEKEYS_CFM_ID:
            send_mlme_deletekeys_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_PROTECTEDFRAMEDROPPED_IND_ID:
            send_mlme_protectedframedropped_ind_from_hip(context, getSmeContext(context)->hipProxyInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_MICHAELMICFAILURE_IND_ID:
            send_mlme_michaelmicfailure_ind_from_hip(context, getSmeContext(context)->hipProxyInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_GET_CFM_ID:
            send_mlme_get_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_SET_CFM_ID:
            send_mlme_set_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_GET_NEXT_CFM_ID:
            send_mlme_get_next_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_SETPROTECTION_CFM_ID:
            send_mlme_setprotection_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_SCAN_CFM_ID:
            send_mlme_scan_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_RESET_CFM_ID:
            send_mlme_reset_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_SCAN_IND_ID:
            send_mlme_scan_ind_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_AUTONOMOUS_SCAN_IND_ID:
            send_mlme_autonomous_scan_ind_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_ADD_AUTONOMOUS_SCAN_CFM_ID:
           send_mlme_add_autonomous_scan_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_DEL_AUTONOMOUS_SCAN_CFM_ID:
            send_mlme_del_autonomous_scan_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_ADD_PERIODIC_CFM_ID:
            send_mlme_add_periodic_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_DEL_PERIODIC_CFM_ID:
            send_mlme_del_periodic_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_ADD_BLACKOUT_CFM_ID:
            send_mlme_add_blackout_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_DEL_BLACKOUT_CFM_ID:
            send_mlme_del_blackout_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case DEBUG_STRING_IND_ID:
            send_debug_string_ind_from_hip(context, getSmeContext(context)->dbgInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case DEBUG_WORD16_IND_ID:
            send_debug_word16_ind_from_hip(context, getSmeContext(context)->dbgInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case DEBUG_GENERIC_IND_ID:
            send_debug_generic_ind_from_hip(context, getSmeContext(context)->dbgInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case DEBUG_GENERIC_CFM_ID:
            send_debug_generic_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_POWERMGT_CFM_ID:
            send_mlme_powermgt_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_SET_UNITDATA_FILTER_CFM_ID:
            send_mlme_set_unitdata_filter_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_ADDTS_CFM_ID:
            send_mlme_addts_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_DELTS_CFM_ID:
            send_mlme_delts_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_DELTS_IND_ID:
            send_mlme_delts_ind_from_hip(context, getSmeContext(context)->qosInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_ADDBA_CFM_ID:
            send_mlme_addba_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_DELBA_CFM_ID:
            send_mlme_delba_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_DELBA_IND_ID:
            send_mlme_delba_ind_from_hip(context, getSmeContext(context)->qosInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_ADDBA_IND_ID:
            send_mlme_addba_ind_from_hip(context, getSmeContext(context)->qosInstance, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;

        case MLME_ADD_TRIGGERED_GET_CFM_ID:
            send_mlme_add_triggered_get_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_DEL_TRIGGERED_GET_CFM_ID:
            send_mlme_del_triggered_get_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_TRIGGERED_GET_IND_ID:
            send_mlme_triggered_get_ind_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        case MLME_MEASURE_CFM_ID:
            send_mlme_measure_cfm_from_hip(context, eventHeader->destination, hipEvent, hip_event_size, (*dataRef1), (*dataRef2), messageOk);
            break;
        default:
            sme_trace_crit((TR_SYS_SAP, "send_hip_message() : Error unhandled HIP CFM Message 0x%.4X", eventHeader->id));
            messageOk = FALSE;
            break;
        }

#ifdef CCX_VARIANT
        {
            CsrUint8 *pBuf;
            CsrUint8 *pIeStart;
            CsrUint16 length;

            switch(eventHeader->id)
            {
            case MLME_ASSOCIATE_CFM_ID:
            case MLME_ASSOCIATE_IND_ID:
            case MLME_REASSOCIATE_CFM_ID:
            case MLME_AUTHENTICATE_CFM_ID:
            case MLME_AUTHENTICATE_IND_ID:
            case MLME_DEAUTHENTICATE_CFM_ID:
            case MLME_CONNECTED_IND_ID:
            case MLME_DEAUTHENTICATE_IND_ID:
            case MLME_DISASSOCIATE_IND_ID:
            case MA_UNITDATA_IND_ID:
            case MA_UNITDATA_CFM_ID:
            case MLME_SETKEYS_CFM_ID:
            case MLME_DELETEKEYS_CFM_ID:
            case MLME_PROTECTEDFRAMEDROPPED_IND_ID:
            case MLME_MICHAELMICFAILURE_IND_ID:
            case MLME_SETPROTECTION_CFM_ID:
            case MLME_SCAN_IND_ID:
            case MLME_AUTONOMOUS_SCAN_IND_ID:
            case MLME_ADD_PERIODIC_CFM_ID:
            case MLME_DEL_PERIODIC_CFM_ID:
            case MLME_ADD_BLACKOUT_CFM_ID:
            case MLME_DEL_BLACKOUT_CFM_ID:
            case MLME_POWERMGT_CFM_ID:
            case MLME_SET_UNITDATA_FILTER_CFM_ID:
            case MLME_ADDTS_CFM_ID:
            case MLME_DELTS_CFM_ID:
/*          ignored to prevent multiple indications
 *          case MLME_DELTS_IND_ID:
 */
            case MLME_DELBA_IND_ID:
            case MLME_ADDBA_IND_ID:
            case MLME_ADD_TRIGGERED_GET_CFM_ID:
            case MLME_DEL_TRIGGERED_GET_CFM_ID:
            case MLME_TRIGGERED_GET_IND_ID:
            case MLME_MEASURE_CFM_ID:
            {
                if(dataRef1->dataLength != 0)
                {
                    pld_access(getPldContext(context), (PldHdl)dataRef1->slotNumber, (void**)&pBuf, &length);

                    /* process IEs */

                    /* ts_metrics - ALL management messages*/
                    pIeStart = ccx_ie_tsm_check_if_ie_present(pBuf, length);
                    if (pIeStart != NULL)
                    {
                        send_ts_metric_ie_to_sme(context, pIeStart);
                    }
                }
                break;
            }
            default:
            {
                break;
            }
            }

        }
#endif

    }
    if (!messageOk)
    {
        sme_trace_crit((TR_SYS_SAP, "send_hip_message() : Error unhandled HIP Message 0x%.4X", eventHeader->id));
        if (dataRef1->dataLength)
        {
            pld_release(getPldContext(context), dataRef1->slotNumber);
        }
        if (dataRef2->dataLength)
        {
            pld_release(getPldContext(context), dataRef2->slotNumber);
        }
    }
    else if (hasMatchingCfm)
    {
        /* Remove the queued error cfm
         * If the message is not ok we do not remove as the
         * unifi_sys_wifi_off_ind will flush the message.
         */
        (void)hip_auto_cfm_remove_matching_cfm(getSmeContext(context)->hipSapAuto, eventHeader->id, optionalMatchingCfm);
    }


    return messageOk;
}

void unifi_sys_hip_ind(FsmContext* context,
                       CsrUint16 mlmeCommandLength,
                       const CsrUint8 *mlmeCommand,
                       CsrUint16 dataRef1Length,
                       const CsrUint8 *dataRef1,
                       CsrUint16 dataRef2Length,
                       const CsrUint8 *dataRef2)
{
    FsmEvent eventHeader;
    CsrUint8* buffer = (CsrUint8*)mlmeCommand;
    CsrUint8* hip_event = (CsrUint8*)mlmeCommand;
    DataReference dataRefResult1 = {0, 0};
    DataReference dataRefResult2 = {0, 0};

    if (!hip_auto_cfm_is_hip_sap_open(getSmeContext(context)->hipSapAuto))
    {
        sme_trace_error((TR_SYS_SAP, "unifi_sys_hip_ind() called when sme is not accepting messages"));
        return;
    }

    eventHeader.id = event_unpack_CsrUint16(&buffer);
    eventHeader.destination = event_unpack_CsrUint16(&buffer) & 0x00FF; /* Mask out the top byte for local pid */
    eventHeader.sender_ = event_unpack_CsrUint16(&buffer);

    /*sme_trace_error((TR_SYS_SAP, "unifi_sys_hip_ind() Msg : 0x%.4X To   : 0x%.4X",eventHeader.id, eventHeader.destination));*/

    if (!storeDataRefs(context, mlmeCommandLength, (CsrUint8*)mlmeCommand, dataRef1Length, (CsrUint8*)dataRef1, dataRef2Length, (CsrUint8*)dataRef2, &dataRefResult1, &dataRefResult2) )
    {
        sme_trace_crit((TR_SYS_SAP, "unifi_sys_hip_ind() : Bulkdata Error :: Ignoring message"));
        return;
    }

    sme_trace_hex_code(
        sme_trace_hex((TR_SYS_SAP, TR_LVL_DEBUG, "unifi_sys_hip_ind(Event)", mlmeCommand, mlmeCommandLength));
        if (dataRef1Length != 0)
        {
            sme_trace_hex((TR_SYS_SAP, TR_LVL_DEBUG, "unifi_sys_hip_ind(dataRef1)", dataRef1, dataRef1Length));
        }
        if (dataRef2Length != 0)
        {
            sme_trace_hex((TR_SYS_SAP, TR_LVL_DEBUG, "unifi_sys_hip_ind(dataRef2)", dataRef2, dataRef2Length));
        }
    );

    if (!send_hip_message(context, &eventHeader, hip_event, mlmeCommandLength, &dataRefResult1, &dataRefResult2))
    {
        sme_trace_crit((TR_SYS_SAP, "send_hip_message() : Message send Failed"));
        return;
    }
}
