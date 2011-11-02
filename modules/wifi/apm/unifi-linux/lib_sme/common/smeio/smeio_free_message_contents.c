/*******************************************************************************

                (c) Cambridge Silicon Radio Limited 2009

                All rights reserved and confidential information of CSR

                WARNING: This is an auto-generated file! Do NOT edit!

REVISION:       $Revision: #1 $

*******************************************************************************/

#include "smeio/smeio_free_message_contents.h"


void free_unifi_datablock_contents(const unifi_DataBlock* pType)
{
    CsrPfree(pType->data);
}

void free_unifi_datablocklist_contents(const unifi_DataBlockList* pType)
{
    {
    CsrUint16 i1;
        for(i1 = 0; i1 < pType->numElements; i1++)
        {
            free_unifi_datablock_contents(((const unifi_DataBlock*)&((pType->dataList)) + i1));
        }
    }
    CsrPfree(pType->dataList);
}

void free_unifi_pmkidlist_contents(const unifi_PmkidList* pType)
{
    CsrPfree(pType->pmkids);
}

void free_unifi_smeversions_contents(const unifi_SmeVersions* pType)
{
    CsrPfree(pType->smeIdString);
}

void free_unifi_versions_contents(const unifi_Versions* pType)
{
    CsrPfree(pType->smeIdString);
}

void free_unifi_addresslist_contents(const unifi_AddressList* pType)
{
    CsrPfree(pType->addresses);
}

void free_unifi_cloakedssidconfig_contents(const unifi_CloakedSsidConfig* pType)
{
    CsrPfree(pType->cloakedSsids);
}

void free_unifi_connectionconfig_contents(const unifi_ConnectionConfig* pType)
{
    CsrPfree(pType->mlmeAssociateReqInformationElements);
}

void free_unifi_connectioninfo_contents(const unifi_ConnectionInfo* pType)
{
    CsrPfree(pType->beaconFrame);
    CsrPfree(pType->associationReqFrame);
    CsrPfree(pType->associationRspFrame);
    CsrPfree(pType->assocScanInfoElements);
    CsrPfree(pType->assocReqInfoElements);
    CsrPfree(pType->assocRspInfoElements);
}

void free_unifi_scanconfig_contents(const unifi_ScanConfig* pType)
{
    CsrPfree(pType->passiveChannelList);
}

void free_unifi_scanresult_contents(const unifi_ScanResult* pType)
{
    CsrPfree(pType->informationElements);
}

void free_unifi_appvalue_contents(const unifi_AppValue* pType)
{
    switch(pType->id)
    {
    case unifi_CloakedSsidConfigValue:
        free_unifi_cloakedssidconfig_contents((const unifi_CloakedSsidConfig*) &(pType->unifi_Value_union.cloakedSsids));
        break;
    case unifi_ScanConfigValue:
        free_unifi_scanconfig_contents((const unifi_ScanConfig*) &(pType->unifi_Value_union.scanConfig));
        break;
    case unifi_VersionsValue:
        free_unifi_versions_contents((const unifi_Versions*) &(pType->unifi_Value_union.versions));
        break;
    case unifi_ConnectionInfoValue:
        free_unifi_connectioninfo_contents((const unifi_ConnectionInfo*) &(pType->unifi_Value_union.connectionInfo));
        break;
    case unifi_CalibrationDataValue:
        free_unifi_datablock_contents((const unifi_DataBlock*) &(pType->unifi_Value_union.calibrationData));
        break;
    case unifi_ConnectionConfigValue:
        free_unifi_connectionconfig_contents((const unifi_ConnectionConfig*) &(pType->unifi_Value_union.connectionConfig));
        break;
    default:
        break;
    };
}

void free_unifi_dbg_cmd_req_contents(const UnifiDbgCmdReq_Evt* pType)
{
    CsrPfree(pType->command);
}

void free_unifi_mgt_association_complete_ind_contents(const UnifiMgtAssociationCompleteInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
    free_unifi_connectioninfo_contents((const unifi_ConnectionInfo*)&(pType->connectionInfo));
}

void free_unifi_mgt_association_start_ind_contents(const UnifiMgtAssociationStartInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
}

void free_unifi_mgt_blacklist_req_contents(const UnifiMgtBlacklistReq_Evt* pType)
{
    CsrPfree(pType->setAddresses);
}

void free_unifi_mgt_blacklist_cfm_contents(const UnifiMgtBlacklistCfm_Evt* pType)
{
    CsrPfree(pType->getAddresses);
}

void free_unifi_mgt_connect_req_contents(const UnifiMgtConnectReq_Evt* pType)
{
    free_unifi_connectionconfig_contents((const unifi_ConnectionConfig*)&(pType->connectionConfig));
}

void free_unifi_mgt_connection_quality_ind_contents(const UnifiMgtConnectionQualityInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
}

void free_unifi_mgt_get_value_cfm_contents(const UnifiMgtGetValueCfm_Evt* pType)
{
    free_unifi_appvalue_contents((const unifi_AppValue*)&(pType->appValue));
}

void free_unifi_mgt_ibss_station_ind_contents(const UnifiMgtIbssStationInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
}

void free_unifi_mgt_media_status_ind_contents(const UnifiMgtMediaStatusInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
    free_unifi_connectioninfo_contents((const unifi_ConnectionInfo*)&(pType->connectionInfo));
}

void free_unifi_mgt_mib_get_req_contents(const UnifiMgtMibGetReq_Evt* pType)
{
    CsrPfree(pType->mibAttribute);
}

void free_unifi_mgt_mib_get_cfm_contents(const UnifiMgtMibGetCfm_Evt* pType)
{
    CsrPfree(pType->mibAttribute);
}

void free_unifi_mgt_mib_get_next_req_contents(const UnifiMgtMibGetNextReq_Evt* pType)
{
    CsrPfree(pType->mibAttribute);
}

void free_unifi_mgt_mib_get_next_cfm_contents(const UnifiMgtMibGetNextCfm_Evt* pType)
{
    CsrPfree(pType->mibAttribute);
}

void free_unifi_mgt_mib_set_req_contents(const UnifiMgtMibSetReq_Evt* pType)
{
    CsrPfree(pType->mibAttribute);
}

void free_unifi_mgt_mic_failure_ind_contents(const UnifiMgtMicFailureInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
}

void free_unifi_mgt_multicast_address_req_contents(const UnifiMgtMulticastAddressReq_Evt* pType)
{
    CsrPfree(pType->setAddresses);
}

void free_unifi_mgt_multicast_address_cfm_contents(const UnifiMgtMulticastAddressCfm_Evt* pType)
{
    CsrPfree(pType->getAddresses);
}

void free_unifi_mgt_packet_filter_set_req_contents(const UnifiMgtPacketFilterSetReq_Evt* pType)
{
    CsrPfree(pType->filter);
}

void free_unifi_mgt_pmkid_req_contents(const UnifiMgtPmkidReq_Evt* pType)
{
    CsrPfree(pType->setPmkids);
}

void free_unifi_mgt_pmkid_cfm_contents(const UnifiMgtPmkidCfm_Evt* pType)
{
    CsrPfree(pType->getPmkids);
}

void free_unifi_mgt_pmkid_candidate_list_ind_contents(const UnifiMgtPmkidCandidateListInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
    CsrPfree(pType->pmkidCandidates);
}

void free_unifi_mgt_roam_complete_ind_contents(const UnifiMgtRoamCompleteInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
}

void free_unifi_mgt_roam_start_ind_contents(const UnifiMgtRoamStartInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
}

void free_unifi_mgt_scan_full_req_contents(const UnifiMgtScanFullReq_Evt* pType)
{
    CsrPfree(pType->ssid);
    CsrPfree(pType->channelList);
    CsrPfree(pType->probeIe);
}

void free_unifi_mgt_scan_result_ind_contents(const UnifiMgtScanResultInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
    free_unifi_scanresult_contents((const unifi_ScanResult*)&(pType->result));
}

void free_unifi_mgt_scan_results_get_cfm_contents(const UnifiMgtScanResultsGetCfm_Evt* pType)
{
    {
        CsrUint16 i1;
        for(i1 = 0; i1 < pType->scanResultsCount; i1++)
        {
            free_unifi_scanresult_contents(((const unifi_ScanResult*)(pType->scanResults)) + i1);
        }
    }
    CsrPfree(pType->scanResults);
}

void free_unifi_mgt_set_value_req_contents(const UnifiMgtSetValueReq_Evt* pType)
{
    free_unifi_appvalue_contents((const unifi_AppValue*)&(pType->appValue));
}

void free_unifi_mgt_tspec_req_contents(const UnifiMgtTspecReq_Evt* pType)
{
    CsrPfree(pType->tspec);
    CsrPfree(pType->tclas);
}

void free_unifi_mgt_tspec_ind_contents(const UnifiMgtTspecInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
    CsrPfree(pType->tspec);
}

void free_unifi_mgt_tspec_cfm_contents(const UnifiMgtTspecCfm_Evt* pType)
{
    CsrPfree(pType->tspec);
}

void free_unifi_mgt_wifi_flightmode_req_contents(const UnifiMgtWifiFlightmodeReq_Evt* pType)
{
    {
        CsrUint16 i1;
        for(i1 = 0; i1 < pType->mibFilesCount; i1++)
        {
            free_unifi_datablock_contents(((const unifi_DataBlock*)(pType->mibFiles)) + i1);
        }
    }
    CsrPfree(pType->mibFiles);
}

void free_unifi_mgt_wifi_off_ind_contents(const UnifiMgtWifiOffInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
}

void free_unifi_mgt_wifi_on_req_contents(const UnifiMgtWifiOnReq_Evt* pType)
{
    {
        CsrUint16 i1;
        for(i1 = 0; i1 < pType->mibFilesCount; i1++)
        {
            free_unifi_datablock_contents(((const unifi_DataBlock*)(pType->mibFiles)) + i1);
        }
    }
    CsrPfree(pType->mibFiles);
}

void free_unifi_sys_eapol_req_contents(const UnifiSysEapolReq_Evt* pType)
{
    CsrPfree(pType->frame);
}

void free_unifi_sys_hip_req_contents(const UnifiSysHipReq_Evt* pType)
{
    CsrPfree(pType->mlmeCommand);
    CsrPfree(pType->dataRef1);
    CsrPfree(pType->dataRef2);
}

void free_unifi_sys_hip_ind_contents(const UnifiSysHipInd_Evt* pType)
{
    CsrPfree(pType->mlmeCommand);
    CsrPfree(pType->dataRef1);
    CsrPfree(pType->dataRef2);
}

void free_unifi_sys_ma_unitdata_req_contents(const UnifiSysMaUnitdataReq_Evt* pType)
{
    CsrPfree(pType->frame);
}

void free_unifi_sys_ma_unitdata_ind_contents(const UnifiSysMaUnitdataInd_Evt* pType)
{
    CsrPfree(pType->frame);
}

void free_unifi_sys_multicast_address_ind_contents(const UnifiSysMulticastAddressInd_Evt* pType)
{
    CsrPfree(pType->setAddresses);
}

void free_unifi_sys_multicast_address_rsp_contents(const UnifiSysMulticastAddressRsp_Evt* pType)
{
    CsrPfree(pType->getAddresses);
}

void free_unifi_sys_tclas_add_req_contents(const UnifiSysTclasAddReq_Evt* pType)
{
    CsrPfree(pType->tclas);
}

void free_unifi_sys_tclas_del_req_contents(const UnifiSysTclasDelReq_Evt* pType)
{
    CsrPfree(pType->tclas);
}

void free_unifi_sys_wifi_on_rsp_contents(const UnifiSysWifiOnRsp_Evt* pType)
{
    free_unifi_smeversions_contents((const unifi_SmeVersions*)&(pType->smeVersions));
}

void smeio_free_message_contents(const FsmEvent* evt)
{
    switch(evt->id)
    {
    case UNIFI_DBG_CMD_REQ_ID:
        free_unifi_dbg_cmd_req_contents((UnifiDbgCmdReq_Evt*)evt);
        break;
    case UNIFI_MGT_ASSOCIATION_COMPLETE_IND_ID:
        free_unifi_mgt_association_complete_ind_contents((UnifiMgtAssociationCompleteInd_Evt*)evt);
        break;
    case UNIFI_MGT_ASSOCIATION_START_IND_ID:
        free_unifi_mgt_association_start_ind_contents((UnifiMgtAssociationStartInd_Evt*)evt);
        break;
    case UNIFI_MGT_BLACKLIST_REQ_ID:
        free_unifi_mgt_blacklist_req_contents((UnifiMgtBlacklistReq_Evt*)evt);
        break;
    case UNIFI_MGT_BLACKLIST_CFM_ID:
        free_unifi_mgt_blacklist_cfm_contents((UnifiMgtBlacklistCfm_Evt*)evt);
        break;
    case UNIFI_MGT_CONNECT_REQ_ID:
        free_unifi_mgt_connect_req_contents((UnifiMgtConnectReq_Evt*)evt);
        break;
    case UNIFI_MGT_CONNECTION_QUALITY_IND_ID:
        free_unifi_mgt_connection_quality_ind_contents((UnifiMgtConnectionQualityInd_Evt*)evt);
        break;
    case UNIFI_MGT_GET_VALUE_CFM_ID:
        free_unifi_mgt_get_value_cfm_contents((UnifiMgtGetValueCfm_Evt*)evt);
        break;
    case UNIFI_MGT_IBSS_STATION_IND_ID:
        free_unifi_mgt_ibss_station_ind_contents((UnifiMgtIbssStationInd_Evt*)evt);
        break;
    case UNIFI_MGT_MEDIA_STATUS_IND_ID:
        free_unifi_mgt_media_status_ind_contents((UnifiMgtMediaStatusInd_Evt*)evt);
        break;
    case UNIFI_MGT_MIB_GET_REQ_ID:
        free_unifi_mgt_mib_get_req_contents((UnifiMgtMibGetReq_Evt*)evt);
        break;
    case UNIFI_MGT_MIB_GET_CFM_ID:
        free_unifi_mgt_mib_get_cfm_contents((UnifiMgtMibGetCfm_Evt*)evt);
        break;
    case UNIFI_MGT_MIB_GET_NEXT_REQ_ID:
        free_unifi_mgt_mib_get_next_req_contents((UnifiMgtMibGetNextReq_Evt*)evt);
        break;
    case UNIFI_MGT_MIB_GET_NEXT_CFM_ID:
        free_unifi_mgt_mib_get_next_cfm_contents((UnifiMgtMibGetNextCfm_Evt*)evt);
        break;
    case UNIFI_MGT_MIB_SET_REQ_ID:
        free_unifi_mgt_mib_set_req_contents((UnifiMgtMibSetReq_Evt*)evt);
        break;
    case UNIFI_MGT_MIC_FAILURE_IND_ID:
        free_unifi_mgt_mic_failure_ind_contents((UnifiMgtMicFailureInd_Evt*)evt);
        break;
    case UNIFI_MGT_MULTICAST_ADDRESS_REQ_ID:
        free_unifi_mgt_multicast_address_req_contents((UnifiMgtMulticastAddressReq_Evt*)evt);
        break;
    case UNIFI_MGT_MULTICAST_ADDRESS_CFM_ID:
        free_unifi_mgt_multicast_address_cfm_contents((UnifiMgtMulticastAddressCfm_Evt*)evt);
        break;
    case UNIFI_MGT_PACKET_FILTER_SET_REQ_ID:
        free_unifi_mgt_packet_filter_set_req_contents((UnifiMgtPacketFilterSetReq_Evt*)evt);
        break;
    case UNIFI_MGT_PMKID_REQ_ID:
        free_unifi_mgt_pmkid_req_contents((UnifiMgtPmkidReq_Evt*)evt);
        break;
    case UNIFI_MGT_PMKID_CFM_ID:
        free_unifi_mgt_pmkid_cfm_contents((UnifiMgtPmkidCfm_Evt*)evt);
        break;
    case UNIFI_MGT_PMKID_CANDIDATE_LIST_IND_ID:
        free_unifi_mgt_pmkid_candidate_list_ind_contents((UnifiMgtPmkidCandidateListInd_Evt*)evt);
        break;
    case UNIFI_MGT_ROAM_COMPLETE_IND_ID:
        free_unifi_mgt_roam_complete_ind_contents((UnifiMgtRoamCompleteInd_Evt*)evt);
        break;
    case UNIFI_MGT_ROAM_START_IND_ID:
        free_unifi_mgt_roam_start_ind_contents((UnifiMgtRoamStartInd_Evt*)evt);
        break;
    case UNIFI_MGT_SCAN_FULL_REQ_ID:
        free_unifi_mgt_scan_full_req_contents((UnifiMgtScanFullReq_Evt*)evt);
        break;
    case UNIFI_MGT_SCAN_RESULT_IND_ID:
        free_unifi_mgt_scan_result_ind_contents((UnifiMgtScanResultInd_Evt*)evt);
        break;
    case UNIFI_MGT_SCAN_RESULTS_GET_CFM_ID:
        free_unifi_mgt_scan_results_get_cfm_contents((UnifiMgtScanResultsGetCfm_Evt*)evt);
        break;
    case UNIFI_MGT_SET_VALUE_REQ_ID:
        free_unifi_mgt_set_value_req_contents((UnifiMgtSetValueReq_Evt*)evt);
        break;
    case UNIFI_MGT_TSPEC_REQ_ID:
        free_unifi_mgt_tspec_req_contents((UnifiMgtTspecReq_Evt*)evt);
        break;
    case UNIFI_MGT_TSPEC_IND_ID:
        free_unifi_mgt_tspec_ind_contents((UnifiMgtTspecInd_Evt*)evt);
        break;
    case UNIFI_MGT_TSPEC_CFM_ID:
        free_unifi_mgt_tspec_cfm_contents((UnifiMgtTspecCfm_Evt*)evt);
        break;
    case UNIFI_MGT_WIFI_FLIGHTMODE_REQ_ID:
        free_unifi_mgt_wifi_flightmode_req_contents((UnifiMgtWifiFlightmodeReq_Evt*)evt);
        break;
    case UNIFI_MGT_WIFI_OFF_IND_ID:
        free_unifi_mgt_wifi_off_ind_contents((UnifiMgtWifiOffInd_Evt*)evt);
        break;
    case UNIFI_MGT_WIFI_ON_REQ_ID:
        free_unifi_mgt_wifi_on_req_contents((UnifiMgtWifiOnReq_Evt*)evt);
        break;
    case UNIFI_SYS_EAPOL_REQ_ID:
        free_unifi_sys_eapol_req_contents((UnifiSysEapolReq_Evt*)evt);
        break;
    case UNIFI_SYS_HIP_REQ_ID:
        free_unifi_sys_hip_req_contents((UnifiSysHipReq_Evt*)evt);
        break;
    case UNIFI_SYS_HIP_IND_ID:
        free_unifi_sys_hip_ind_contents((UnifiSysHipInd_Evt*)evt);
        break;
    case UNIFI_SYS_MA_UNITDATA_REQ_ID:
        free_unifi_sys_ma_unitdata_req_contents((UnifiSysMaUnitdataReq_Evt*)evt);
        break;
    case UNIFI_SYS_MA_UNITDATA_IND_ID:
        free_unifi_sys_ma_unitdata_ind_contents((UnifiSysMaUnitdataInd_Evt*)evt);
        break;
    case UNIFI_SYS_MULTICAST_ADDRESS_IND_ID:
        free_unifi_sys_multicast_address_ind_contents((UnifiSysMulticastAddressInd_Evt*)evt);
        break;
    case UNIFI_SYS_MULTICAST_ADDRESS_RSP_ID:
        free_unifi_sys_multicast_address_rsp_contents((UnifiSysMulticastAddressRsp_Evt*)evt);
        break;
    case UNIFI_SYS_TCLAS_ADD_REQ_ID:
        free_unifi_sys_tclas_add_req_contents((UnifiSysTclasAddReq_Evt*)evt);
        break;
    case UNIFI_SYS_TCLAS_DEL_REQ_ID:
        free_unifi_sys_tclas_del_req_contents((UnifiSysTclasDelReq_Evt*)evt);
        break;
    case UNIFI_SYS_WIFI_ON_RSP_ID:
        free_unifi_sys_wifi_on_rsp_contents((UnifiSysWifiOnRsp_Evt*)evt);
        break;
    default:
        break;
    }
}



