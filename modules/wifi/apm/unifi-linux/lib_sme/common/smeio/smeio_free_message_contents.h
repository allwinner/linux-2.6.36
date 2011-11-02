/*******************************************************************************

                (c) Cambridge Silicon Radio Limited 2009

                All rights reserved and confidential information of CSR

                WARNING: This is an auto-generated file! Do NOT edit!

REVISION:       $Revision: #1 $

*******************************************************************************/
#ifndef SMEIO_FREE_MESSAGE_CONTENTS_H
#define SMEIO_FREE_MESSAGE_CONTENTS_H

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void free_unifi_datablock_contents(const unifi_DataBlock* pType);
extern void free_unifi_datablocklist_contents(const unifi_DataBlockList* pType);
extern void free_unifi_pmkidlist_contents(const unifi_PmkidList* pType);
extern void free_unifi_smeversions_contents(const unifi_SmeVersions* pType);
extern void free_unifi_versions_contents(const unifi_Versions* pType);
extern void free_unifi_addresslist_contents(const unifi_AddressList* pType);
extern void free_unifi_cloakedssidconfig_contents(const unifi_CloakedSsidConfig* pType);
extern void free_unifi_connectionconfig_contents(const unifi_ConnectionConfig* pType);
extern void free_unifi_connectioninfo_contents(const unifi_ConnectionInfo* pType);
extern void free_unifi_scanconfig_contents(const unifi_ScanConfig* pType);
extern void free_unifi_scanresult_contents(const unifi_ScanResult* pType);
extern void free_unifi_appvalue_contents(const unifi_AppValue* pType);
extern void free_unifi_dbg_cmd_req_contents(const UnifiDbgCmdReq_Evt* evt);
extern void free_unifi_mgt_association_complete_ind_contents(const UnifiMgtAssociationCompleteInd_Evt* evt);
extern void free_unifi_mgt_association_start_ind_contents(const UnifiMgtAssociationStartInd_Evt* evt);
extern void free_unifi_mgt_blacklist_req_contents(const UnifiMgtBlacklistReq_Evt* evt);
extern void free_unifi_mgt_blacklist_cfm_contents(const UnifiMgtBlacklistCfm_Evt* evt);
extern void free_unifi_mgt_connect_req_contents(const UnifiMgtConnectReq_Evt* evt);
extern void free_unifi_mgt_connection_quality_ind_contents(const UnifiMgtConnectionQualityInd_Evt* evt);
extern void free_unifi_mgt_get_value_cfm_contents(const UnifiMgtGetValueCfm_Evt* evt);
extern void free_unifi_mgt_ibss_station_ind_contents(const UnifiMgtIbssStationInd_Evt* evt);
extern void free_unifi_mgt_media_status_ind_contents(const UnifiMgtMediaStatusInd_Evt* evt);
extern void free_unifi_mgt_mib_get_req_contents(const UnifiMgtMibGetReq_Evt* evt);
extern void free_unifi_mgt_mib_get_cfm_contents(const UnifiMgtMibGetCfm_Evt* evt);
extern void free_unifi_mgt_mib_get_next_req_contents(const UnifiMgtMibGetNextReq_Evt* evt);
extern void free_unifi_mgt_mib_get_next_cfm_contents(const UnifiMgtMibGetNextCfm_Evt* evt);
extern void free_unifi_mgt_mib_set_req_contents(const UnifiMgtMibSetReq_Evt* evt);
extern void free_unifi_mgt_mic_failure_ind_contents(const UnifiMgtMicFailureInd_Evt* evt);
extern void free_unifi_mgt_multicast_address_req_contents(const UnifiMgtMulticastAddressReq_Evt* evt);
extern void free_unifi_mgt_multicast_address_cfm_contents(const UnifiMgtMulticastAddressCfm_Evt* evt);
extern void free_unifi_mgt_packet_filter_set_req_contents(const UnifiMgtPacketFilterSetReq_Evt* evt);
extern void free_unifi_mgt_pmkid_req_contents(const UnifiMgtPmkidReq_Evt* evt);
extern void free_unifi_mgt_pmkid_cfm_contents(const UnifiMgtPmkidCfm_Evt* evt);
extern void free_unifi_mgt_pmkid_candidate_list_ind_contents(const UnifiMgtPmkidCandidateListInd_Evt* evt);
extern void free_unifi_mgt_roam_complete_ind_contents(const UnifiMgtRoamCompleteInd_Evt* evt);
extern void free_unifi_mgt_roam_start_ind_contents(const UnifiMgtRoamStartInd_Evt* evt);
extern void free_unifi_mgt_scan_full_req_contents(const UnifiMgtScanFullReq_Evt* evt);
extern void free_unifi_mgt_scan_result_ind_contents(const UnifiMgtScanResultInd_Evt* evt);
extern void free_unifi_mgt_scan_results_get_cfm_contents(const UnifiMgtScanResultsGetCfm_Evt* evt);
extern void free_unifi_mgt_set_value_req_contents(const UnifiMgtSetValueReq_Evt* evt);
extern void free_unifi_mgt_tspec_req_contents(const UnifiMgtTspecReq_Evt* evt);
extern void free_unifi_mgt_tspec_ind_contents(const UnifiMgtTspecInd_Evt* evt);
extern void free_unifi_mgt_tspec_cfm_contents(const UnifiMgtTspecCfm_Evt* evt);
extern void free_unifi_mgt_wifi_flightmode_req_contents(const UnifiMgtWifiFlightmodeReq_Evt* evt);
extern void free_unifi_mgt_wifi_off_ind_contents(const UnifiMgtWifiOffInd_Evt* evt);
extern void free_unifi_mgt_wifi_on_req_contents(const UnifiMgtWifiOnReq_Evt* evt);
extern void free_unifi_sys_eapol_req_contents(const UnifiSysEapolReq_Evt* evt);
extern void free_unifi_sys_hip_req_contents(const UnifiSysHipReq_Evt* evt);
extern void free_unifi_sys_hip_ind_contents(const UnifiSysHipInd_Evt* evt);
extern void free_unifi_sys_ma_unitdata_req_contents(const UnifiSysMaUnitdataReq_Evt* evt);
extern void free_unifi_sys_ma_unitdata_ind_contents(const UnifiSysMaUnitdataInd_Evt* evt);
extern void free_unifi_sys_multicast_address_ind_contents(const UnifiSysMulticastAddressInd_Evt* evt);
extern void free_unifi_sys_multicast_address_rsp_contents(const UnifiSysMulticastAddressRsp_Evt* evt);
extern void free_unifi_sys_tclas_add_req_contents(const UnifiSysTclasAddReq_Evt* evt);
extern void free_unifi_sys_tclas_del_req_contents(const UnifiSysTclasDelReq_Evt* evt);
extern void free_unifi_sys_wifi_on_rsp_contents(const UnifiSysWifiOnRsp_Evt* evt);
extern void smeio_free_message_contents(const FsmEvent* evt);


#ifdef __cplusplus
}
#endif

#endif
