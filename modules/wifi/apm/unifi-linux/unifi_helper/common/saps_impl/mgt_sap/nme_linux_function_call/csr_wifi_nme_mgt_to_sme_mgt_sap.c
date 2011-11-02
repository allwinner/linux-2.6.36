/** @file csr_wifi_nme_mgt_to_sme_mgt_sap.c
 *
 * NME Top Level
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009-2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Maps between the NME MGT SAP and the SME MGT SAP
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/saps_impl/mgt_sap/nme_linux_function_call/csr_wifi_nme_mgt_to_sme_mgt_sap.c#2 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_top_level_fsm
 */


/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/csr_wifi_fsm.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "csr_wifi_nme_mgt_sap/csr_wifi_nme_mgt_sap.h"
#include "mgt_sap/mgt_sap.h"
#include "linux_sap_platform.h"


/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC CONSTANT DEFINITIONS **********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/* From NME MGT to SME MGT SAP */

void unifi_nme_mgt_restricted_access_enable_req(void* context, void* appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_restricted_access_enable_req(linuxContext->fsmContext, appHandle);
}


void unifi_nme_mgt_restricted_access_disable_req(void* context, void* appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_restricted_access_disable_req(linuxContext->fsmContext, appHandle);
}

void unifi_nme_mgt_wifi_on_req(void* context,
                               void* appHandle,
                               const unifi_MACAddress *address,
                               CsrUint16 mibFilesCount,
                               const unifi_DataBlock *mibFiles)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_wifi_on_req(linuxContext->fsmContext, appHandle, address, mibFilesCount, mibFiles);
}

void unifi_nme_mgt_wifi_off_req(void* context,
                                void* appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_wifi_off_req(linuxContext->fsmContext, appHandle);
}

void unifi_nme_mgt_wifi_flightmode_req(void* context,
                                       void* appHandle,
                                       const unifi_MACAddress *address,
                                       CsrUint16 mibFilesCount,
                                       const unifi_DataBlock *mibFiles)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_wifi_flightmode_req(linuxContext->fsmContext, appHandle, address, mibFilesCount, mibFiles);
}

void unifi_nme_mgt_set_value_req(void* context,
                                 void* appHandle,
                                 const unifi_AppValue *appValue)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_set_value_req(linuxContext->fsmContext, appHandle, appValue);
}

void unifi_nme_mgt_get_value_req(void* context,
                                 void* appHandle,
                                 unifi_AppValueId appValueId)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_get_value_req(linuxContext->fsmContext, appHandle, appValueId);
}

void unifi_nme_mgt_mib_set_req(void* context,
                               void* appHandle,
                               CsrUint16 mibAttributeLength,
                               const CsrUint8 *mibAttribute)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_mib_set_req(linuxContext->fsmContext, appHandle, mibAttributeLength, mibAttribute);
}

void unifi_nme_mgt_mib_get_req(void* context,
                               void* appHandle,
                               CsrUint16 mibAttributeLength,
                               const CsrUint8 *mibAttribute)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_mib_get_req(linuxContext->fsmContext, appHandle, mibAttributeLength, mibAttribute);
}

void unifi_nme_mgt_mib_get_next_req(void* context,
                                    void* appHandle,
                                    CsrUint16 mibAttributeLength,
                                    const CsrUint8 *mibAttribute)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_mib_get_next_req(linuxContext->fsmContext, appHandle, mibAttributeLength, mibAttribute);
}

void unifi_nme_mgt_scan_full_req(void* context,
                                 void* appHandle,
                                 CsrUint8 ssidCount,
                                 const unifi_SSID *ssid,
                                 const unifi_MACAddress *bssid,
                                 CsrBool forceScan,
                                 unifi_BSSType bssType,
                                 unifi_ScanType scanType,
                                 CsrUint16 channelListCount,
                                 const CsrUint8 *channelList,
                                 CsrUint16 probeIeLength,
                                 const CsrUint8 *probeIe)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_scan_full_req(linuxContext->fsmContext,
                            appHandle,
                            ssidCount,
                            ssid,
                            bssid,
                            forceScan,
                            bssType,
                            scanType,
                            channelListCount,
                            channelList,
                            probeIeLength,
                            probeIe);
}

void unifi_nme_mgt_scan_results_get_req(void* context,
                                        void* appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_scan_results_get_req(linuxContext->fsmContext, appHandle);
}

void unifi_nme_mgt_connect_req(void* context,
                               void* appHandle,
                               const unifi_ConnectionConfig *connectionConfig)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_connect_req(linuxContext->fsmContext, appHandle, connectionConfig);
}

void unifi_nme_mgt_disconnect_req(void* context,
                                  void* appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_disconnect_req(linuxContext->fsmContext, appHandle);
}

void unifi_nme_mgt_multicast_address_req(void* context,
                                         void* appHandle,
                                         unifi_ListAction action,
                                         CsrUint8 setAddressesCount,
                                         const unifi_MACAddress *setAddresses)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_multicast_address_req(linuxContext->fsmContext,
                                    appHandle,
                                    action,
                                    setAddressesCount,
                                    setAddresses);
}

void unifi_nme_mgt_pmkid_req(void* context,
                             void* appHandle,
                             unifi_ListAction action,
                             CsrUint8 setPmkidsCount,
                             const unifi_Pmkid *setPmkids)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_pmkid_req(linuxContext->fsmContext,
                        appHandle,
                        action,
                        setPmkidsCount,
                        setPmkids);
}

void unifi_nme_mgt_key_req(void* context,
                           void* appHandle,
                           unifi_ListAction action,
                           const unifi_Key *key)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_key_req(linuxContext->fsmContext, appHandle, action, key);
}

void unifi_nme_mgt_packet_filter_set_req(void* context,
                                         void* appHandle,
                                         CsrUint16 filterLength,
                                         const CsrUint8 *filter,
                                         unifi_PacketFilterMode mode,
                                         const unifi_IPV4Address* arpFilterAddress)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_packet_filter_set_req(linuxContext->fsmContext,
                                    appHandle,
                                    filterLength,
                                    filter,
                                    mode,
                                    arpFilterAddress);
}

void unifi_nme_mgt_tspec_req(void* context,
                             void* appHandle,
                             unifi_ListAction action,
                             CsrUint32 transactionId,
                             CsrBool strict,
                             CsrUint8 ctrlMask,
                             CsrUint16 tspecLength,
                             const CsrUint8 *tspec,
                             CsrUint16 tclasLength,
                             const CsrUint8 *tclas)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_tspec_req(linuxContext->fsmContext,
                        appHandle,
                        action,
                        transactionId,
                        strict,
                        ctrlMask,
                        tspecLength,
                        tspec,
                        tclasLength,
                        tclas);
}

void unifi_nme_mgt_scan_results_flush_req(void* context,
                                          void* appHandle)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_scan_results_flush_req(linuxContext->fsmContext, appHandle);
}

void unifi_nme_mgt_blacklist_req(void* context,
                                 void* appHandle,
                                 unifi_ListAction action,
                                 CsrUint8 setAddressCount,
                                 const unifi_MACAddress *setAddresses)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_blacklist_req(linuxContext->fsmContext,
                            appHandle,
                            action,
                            setAddressCount,
                            setAddresses);
}

void unifi_nme_mgt_event_mask_set_req(void* context,
                                      void* appHandle,
                                      CsrUint32 indMask)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_mgt_event_mask_set_req(linuxContext->fsmContext, appHandle, indMask);
}

/* Coming from SME MGT to NME MGT */

void unifi_mgt_wifi_on_cfm(void* context,
                           void* appHandle,
                           unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_wifi_on_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_wifi_off_cfm(void* context,
                            void* appHandle,
                            unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_wifi_off_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_wifi_off_ind(void* context,
                            CsrUint16 appHandlesCount,
                            void* *appHandles,
                            unifi_ControlIndication controlIndication)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_wifi_off_ind(linuxContext->nmeFsmContext, appHandlesCount, appHandles, controlIndication);
}

void unifi_mgt_wifi_flightmode_cfm(void* context,
                                   void* appHandle,
                                   unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_wifi_flightmode_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_set_value_cfm(void* context,
                             void* appHandle,
                             unifi_Status status,
                             unifi_AppValueId appValueId)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_set_value_cfm(linuxContext->nmeFsmContext, appHandle, status, appValueId);
}

void unifi_mgt_get_value_cfm(void* context,
                             void* appHandle,
                             unifi_Status status,
                             const unifi_AppValue *appValue)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_get_value_cfm(linuxContext->nmeFsmContext, appHandle, status, appValue);
}

void unifi_mgt_mib_set_cfm(void* context,
                           void* appHandle,
                           unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_mib_set_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_mib_get_cfm(void* context,
                           void* appHandle,
                           unifi_Status status,
                           CsrUint16 mibAttributeLength,
                           const CsrUint8 *mibAttribute)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_mib_get_cfm(linuxContext->nmeFsmContext, appHandle, status, mibAttributeLength, mibAttribute);
}

void unifi_mgt_mib_get_next_cfm(void* context,
                                void* appHandle,
                                unifi_Status status,
                                CsrUint16 mibAttributeLength,
                                const CsrUint8 *mibAttribute)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_mib_get_next_cfm(linuxContext->nmeFsmContext, appHandle, status, mibAttributeLength, mibAttribute);
}

void unifi_mgt_scan_full_cfm(void* context,
                             void* appHandle,
                             unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_scan_full_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_scan_results_get_cfm(void* context,
                                    void* appHandle,
                                    unifi_Status status,
                                    CsrUint16 scanResultsCount,
                                    const unifi_ScanResult *scanResults)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_scan_results_get_cfm(linuxContext->nmeFsmContext, appHandle, status, scanResultsCount, scanResults);
}

void unifi_mgt_scan_result_ind(void* context,
                               CsrUint16 appHandlesCount,
                               void* *appHandles,
                               const unifi_ScanResult *result)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_scan_result_ind(linuxContext->nmeFsmContext, appHandlesCount, appHandles, result);
}

void unifi_mgt_connect_cfm(void* context,
                           void* appHandle,
                           unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_connect_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_media_status_ind(void* context,
                                CsrUint16 appHandlesCount,
                                void* *appHandles,
                                unifi_MediaStatus mediaStatus,
                                const unifi_ConnectionInfo *connectionInfo,
                                unifi_IEEE80211Reason disassocReason,
                                unifi_IEEE80211Reason deauthReason)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_media_status_ind(linuxContext->nmeFsmContext,
                                   appHandlesCount,
                                   appHandles,
                                   mediaStatus,
                                   connectionInfo,
                                   disassocReason,
                                   deauthReason);
}

void unifi_mgt_connection_quality_ind(void* context,
                                      CsrUint16 appHandlesCount,
                                      void* *appHandles,
                                      const unifi_LinkQuality *linkQuality)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_connection_quality_ind(linuxContext->nmeFsmContext, appHandlesCount, appHandles, linkQuality);
}

void unifi_mgt_disconnect_cfm(void* context,
                              void* appHandle,
                              unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_disconnect_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_multicast_address_cfm(void* context,
                                     void* appHandle,
                                     unifi_Status status,
                                     unifi_ListAction action,
                                     CsrUint8 getAddressesCount,
                                     const unifi_MACAddress *getAddresses)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_multicast_address_cfm(linuxContext->nmeFsmContext, appHandle, status, action, getAddressesCount, getAddresses);
}

void unifi_mgt_mic_failure_ind(void* context,
                               CsrUint16 appHandlesCount,
                               void* *appHandles,
                               CsrBool secondFailure,
                               CsrUint16 count,
                               const unifi_MACAddress *address,
                               unifi_KeyType keyType,
                               CsrUint16 keyId,
                               const CsrUint16 *tsc)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_mic_failure_ind(linuxContext->nmeFsmContext,
                                  appHandlesCount,
                                  appHandles,
                                  secondFailure,
                                  count,
                                  address,
                                  keyType,
                                  keyId,
                                  tsc);
}

void unifi_mgt_pmkid_candidate_list_ind(void* context,
                                        CsrUint16 appHandlesCount,
                                        void* *appHandles,
                                        CsrUint8 pmkidCandidatesCount,
                                        const unifi_PmkidCandidate *pmkidCandidates)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_pmkid_candidate_list_ind(linuxContext->nmeFsmContext, appHandlesCount, appHandles, pmkidCandidatesCount, pmkidCandidates);
}

void unifi_mgt_pmkid_cfm(void* context,
                         void* appHandle,
                         unifi_Status status,
                         unifi_ListAction action,
                         CsrUint8 getPmkidsCount,
                         const unifi_Pmkid *getPmkids)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_pmkid_cfm(linuxContext->nmeFsmContext,
                            appHandle,
                            status,
                            action,
                            getPmkidsCount,
                            getPmkids);
}

void unifi_mgt_key_cfm(void* context,
                       void* appHandle,
                       unifi_Status status,
                       unifi_ListAction action)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_key_cfm(linuxContext->nmeFsmContext, appHandle, status, action);
}

void unifi_mgt_packet_filter_set_cfm(void* context,
                                     void* appHandle,
                                     unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_packet_filter_set_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_tspec_cfm(void* context,
                         void* appHandle,
                         unifi_Status status,
                         CsrUint32 transactionId,
                         unifi_TspecResultCode tspecResultCode,
                         CsrUint16 tspecLength,
                         const CsrUint8 *tspec)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_tspec_cfm(linuxContext->nmeFsmContext,
                            appHandle,
                            status,
                            transactionId,
                            tspecResultCode,
                            tspecLength,
                            tspec);
}

void unifi_mgt_tspec_ind(void* context,
                         CsrUint16 appHandlesCount,
                         void* *appHandles,
                         CsrUint32 transactionId,
                         unifi_TspecResultCode tspecResultCode,
                         CsrUint16 tspecLength,
                         const CsrUint8 *tspec)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_tspec_ind(linuxContext->nmeFsmContext,
                            appHandlesCount,
                            appHandles,
                            transactionId,
                            tspecResultCode,
                            tspecLength,
                            tspec);
}

void unifi_mgt_scan_results_flush_cfm(void* context,
                                      void* appHandle,
                                      unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_scan_results_flush_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_blacklist_cfm(void* context,
                             void* appHandle,
                             unifi_Status status,
                             unifi_ListAction action,
                             CsrUint8 getAddressCount,
                             const unifi_MACAddress *getAddresses)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_blacklist_cfm(linuxContext->nmeFsmContext,
                                appHandle,
                                status,
                                action,
                                getAddressCount,
                                getAddresses);
}


void unifi_mgt_roam_start_ind(void* context,
                              CsrUint16 appHandlesCount,
                              void* *appHandles,
                              unifi_RoamReason reason,
                              unifi_IEEE80211Reason reason80211)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_roam_start_ind(linuxContext->nmeFsmContext, appHandlesCount, appHandles, reason, reason80211);
}

void unifi_mgt_roam_complete_ind(void* context,
                                 CsrUint16 appHandlesCount,
                                 void* *appHandles,
                                 unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_roam_complete_ind(linuxContext->nmeFsmContext, appHandlesCount, appHandles, status);
}

void unifi_mgt_association_start_ind(void* context,
                                     CsrUint16 appHandlesCount,
                                     void* *appHandles,
                                     const unifi_MACAddress *address,
                                     const unifi_SSID *ssid)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_association_start_ind(linuxContext->nmeFsmContext, appHandlesCount, appHandles, address, ssid);
}

void unifi_mgt_association_complete_ind(void* context,
                                        CsrUint16 appHandlesCount,
                                        void* *appHandles,
                                        unifi_Status status,
                                        const unifi_ConnectionInfo *connectionInfo,
                                        unifi_IEEE80211Reason deauthReason)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_association_complete_ind(linuxContext->nmeFsmContext, appHandlesCount, appHandles, status, connectionInfo, deauthReason);
}

void unifi_mgt_ibss_station_ind(void* context,
                                CsrUint16 appHandlesCount,
                                void* *appHandles,
                                const unifi_MACAddress *address,
                                CsrBool isconnected)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_ibss_station_ind(linuxContext->nmeFsmContext, appHandlesCount, appHandles, address, isconnected);
}

void unifi_mgt_event_mask_set_cfm(void* context,
                                  void* appHandle,
                                  unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_event_mask_set_cfm(linuxContext->nmeFsmContext, appHandle, status);
}


void unifi_mgt_restricted_access_enable_cfm(void* context, void* appHandle, unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_restricted_access_enable_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

void unifi_mgt_restricted_access_disable_cfm(void* context, void* appHandle, unifi_Status status)
{
    LinuxUserSpaceContext* linuxContext = (LinuxUserSpaceContext*)context;
    unifi_nme_mgt_restricted_access_disable_cfm(linuxContext->nmeFsmContext, appHandle, status);
}

