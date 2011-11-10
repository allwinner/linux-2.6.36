/** @file scan_results_storage.h
 *
 * scan results header file
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
 *   This file encasulate the scan result handling and storage controls
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/scan_manager_fsm/scan_results_storage.h#3 $
 *
 ****************************************************************************/
#ifndef SCAN_RESULTS_STORAGE_H
#define SCAN_RESULTS_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup scan_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_internal.h"
#include "hostio/hip_fsm_types.h"
#include "hostio/hip_fsm_events.h"

#include "smeio/smeio_fsm_types.h"

#include "csr_cstl/csr_wifi_list.h"

#ifdef CCX_VARIANT
  #include "ccx_fsm/ccx_private_types.h"
#endif

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

typedef struct ScanResults ScanResults;

/**
 * @brief: flags indicating IEs present in beacons that should be
 *         placed in asscoiate request
 */
typedef enum srs_IECapabilityMask
{
    NONE_Capable             =    0x0000,
    WMM_Capable              =    0x0001,
    WMMUAPSD_Capable         =    0x0002,
    CCX_Capable              =    0x0004,
    WPS_Capable              =    0x0008,
    PRE_AUTH_Capable         =    0x0010,
    DOT11N_Capable           =    0x0020
} srs_IECapabilityMask;

/**
 * @brief ScanResult security data
 *
 * @par Description
 *   See brief
 */
typedef struct srs_security_data
{
    /** WPA Cipher capabilities */
    unifi_EncryptionMode        wpaCipherCapabilityFlags;
    /** WPA2 Cipher capabilities */
    unifi_EncryptionMode        wpa2CipherCapabilityFlags;
    /** WAPI Cipher capabilities */
    unifi_EncryptionMode        wapiCipherCapabilityFlags;
    /** Authentication Mode */
    unifi_AuthMode              authMode_CapabilityFlags;
    /** Privacy Mode */
    unifi_80211PrivacyMode      privacyMode;

}srs_security_data;

/**
 * @brief stores IEs from beacons to be used in associate
 *
 * @par Description
 *   used to store IEs from beacons to be used in associate
 */
typedef struct srs_scan_data
{
    /** Scan result */
    unifi_ScanResult            scanResult;

    DataReference               informationElements;

    /** IE Capabilities */
    srs_IECapabilityMask        ieCapabilityFlags;

    /** Security Capabilities */
    srs_security_data           securityData;

#ifdef CCX_VARIANT
    /** CCX Control Block */
    ccx_ctrl_blk                ccxCtrlBlk;

   /* Access Point Available Admision Capacity */
   CsrUint16 ap_currentAdmitCapacity;
#endif

}srs_scan_data;

/**
 * @brief defines the type to be used in the scan results storage list
 *
 * @par Description
 *   defines the type to be used in the scan results storage list
 */
typedef struct csr_list_scan_result
{
    /** list structure */
    csr_list_node rankedNode;
    /** scan parameter data */
    srs_scan_data scanParameters;
    /** Does the Firmware Own this scan or should we expire it */
    CsrUint16     firmwareOwned;
    /** Time last updated */
    CsrUint32     lastUpdateTime;
    /** the ranking value for this element */
    CsrUint16     rankValue;
} srs_csr_list_scan_result;

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Initialize the wirelessprofile tree object.
 *
 * @par Description
 * This function Initialize the scan results module.
 *
 * the follow entities are initialized
 *    1. primaryScanResultsList
 *    2. newScanResultsList
 *
 * @return
 *        void
 */
extern ScanResults* srs_initialise_scan_result_storage(void);


/**
 * @brief Frees the scan results store
 *
 * @return
 *        void
 */
extern void srs_destroy_scan_result_storage(ScanResults* context);

/**
 * @brief Removes all scan results except the one to which the STA is
 *        currently connected
 *
 * @par Description
 *        Removes all scan results except the one to which the STA is
 *        currently connected
 *
 * @return
 *        void
 */
extern void srs_delete_unconnected_scan_results(FsmContext* context);

/**
 * @brief Performs a complete reset to initial values
 *
 * @par Description
 * this function performs a complete reset on the scan storage module.
 *
 * the following procedure is performed entities are initialized
 *    1. primaryScanResultsList cleared
 *    2. newScanResultsList cleared
 *
 * @return
 *        void
 */
extern void srs_reset_scan_result_storage(FsmContext* context);

/**
 * @brief resets the locally storage join scan.
 *
 * @par Description
 * Resets the locally storage join scan
 *
 * @return
 *        void
 */
extern void srs_reset_join_scan_data(FsmContext* context);

/**
 * @brief Creates a scan result record from a MlmeScanInd.
 *
 * @par Description
 * This function accepts a MlmeScanInd and processes it to create a scan result
 * record.
 *
 * The function will extract the information directly from the mlme signal and
 * also attached Information element.  the IE is then freed
 *
 * @param[in] firmwareOwned : Indicates that the firmware has control
 * @param[in] informationElements : The received Informational Elements
 * @param[in] ifIndex : IF Index
 * @param[in] bssType : bss Type
 * @param[in] bSSID : BSSID
 * @param[in] beaconPeriod : Beacon Period
 * @param[in] timestamp : Timestamp
 * @param[in] localTime : Local Time
 * @param[in] channel : Channel
 * @param[in] channelFrequency : Channel Frequency
 * @param[in] capabilityInformation : Capability Information
 * @param[in] rSSI : RSSI Value
 * @param[in] sNR : SNR Value
 *
 * @return
 *        srs_scan_data* : node created or NULL
 */
extern srs_scan_data* srs_create_scan_result(FsmContext* context,
                    CsrUint16 firmwareOwned,
                    DataReference informationElements,
                    Interface ifIndex,
                    BssType bssType,
                    unifi_MACAddress bSSID,
                    TimeUnits beaconPeriodMs,
                    TsfTime timestamp,
                    TsfTime localTime,
                    ChannelNumber channel,
                    Megahertz channelFrequency,
                    CapabilityInformation capabilityInformation,
                    Decibels rSSI,
                    Decibels sNR);

/**
 * @brief Takes ownership of scan results from the firmware
 *
 * @par Description
 * We will expire scan data that is not owned by the firmware
 *
 * @param[in]  timeAdjust  : Time to add to SME Owned scan results
 *
 * @return
 *        void
 */
extern void srs_take_scan_results_ownership(FsmContext* context, CsrUint32 timeAdjust);

/**
 * @brief Updates the Local scan result.
 *
 * @par Description
 * After the Connection Manager indicates the begin of the join process the
 * Wireless Storage Process will recieve a MlmeScanInd.  This is passed to
 * this function that update the local scan result ready for collection.
 *
 * @param[in] ind : The MlmeScanInd message to be processed.
 *
 * @return
 *        void
 */
extern void srs_update_join_scan_result(FsmContext* context, srs_scan_data* scanResult);

/**
 * @brief Gets a specified scan result with parameters.
 *
 * @par Description
 * Returns the scan result as specifed in supplied searchData from the
 * primary Scan Result list.
 *
 * The scan result uses the Sap type srs_scan_data as required by the
 * core modules.
 *
 * @param[in]  searchData  : The Data to use in the search
 *
 * @return
 *        PmScanData*: NULL = no record found
 *                     else a pointer to the requested profile
 */
extern srs_scan_data* srs_get_scan_parameters_first(FsmContext* context, const unifi_SSID* ssid, const unifi_MACAddress* bssid);

/**
 * @brief Gets the next specified scan result with parameters.
 *
 * @par Description
 * Returns the scan result as specifed in supplied searchData from the primary
 * Scan Result list.
 *
 * @param[in]  handle      : Handle of the start point of the search
 * @param[in]  searchData  : The Data to use in the search
 *
 * @return
 *        PmScanData*: NULL = no record found
 *                     else a pointer to the requested profile
 */
extern srs_scan_data* srs_get_scan_parameters_next(FsmContext* context, const unifi_MACAddress* currentBssid, const unifi_SSID* ssid, const unifi_MACAddress* bssid);

/**
 * @brief Gets the local scan result.
 *
 * @par Description
 * Returns the local scan result.
 *
 * The scan result uses the Sap type srs_scan_data as required by the
 * core modules.
 *
 * @return
 *        PmJoinScanParameters*: NULL = no record found
 *                               else a pointer to the requested profile
 */
extern srs_scan_data* srs_get_join_scan_parameters(FsmContext* context);

/**
 * @brief delete the scan result with the specifed network handle.
 *
 * @par Description
 * locals and Deletes the specified scan result.
 *
 * @param[in]  searchData : reference for the scan data
 *
 * @return
 *        unifi_Status_unifi_endoflist    no match in the list
 *        unifi_Status_unifi_success      delete completed
 *        unifi_Status_unifi_error        a major error has occured
 */

extern unifi_Status srs_delete_scan_result_profile(FsmContext* context, const unifi_MACAddress* bssid, CsrBool forceDelete);
/**
 * @brief gets a scan result for a given BSSID
 *
 * @par Description
 * gets a scan result for a given BSSID
 *
 * @param[in] pBssid : The BSSID to be checked for.
 *
 * @return
 */
extern void srs_scan_result_update(FsmContext* context, const unifi_MACAddress* bssid);

/**
 * @brief gets a scan result for a given BSSID
 *
 * @par Description
 * gets a scan result for a given BSSID
 *
 * @param[in] searchData : Search Data.
 * @param[in] rssi : New RSSI.
 * @param[in] snr : New SNR.
 *
 * @return
 */
extern srs_scan_data* srs_scan_result_quality_update(FsmContext* context, const unifi_MACAddress* bssid, CsrInt16 rssi, CsrInt16 snr);

/**
 * @brief Kicks the scan result ranking process
 *
 * @par Description
 * gets a scan result for a given BSSID
 *
 * @return
 *        void
 *
 */
extern void srs_rank_scan_results(FsmContext* context);

/**
 * @brief Deletes any scan results that are too old
 *
 * @par Description:
 *  This function cycles through the list of cached scan results and examines
 *  the age of each result. Any scan results whose age exceeds the configured
 *  validity period is deleted.
 *
 * @ingroup scanmanager
 *
 * @param[in]  context : SME context
 *
 * @return
 *      void
 */
void srs_expire_old_scan_data(FsmContext* context);

/**
 * @brief Gives a Basic Usability summary for RSSI/SNR values
 *
 * @par Description:
 *  This function maps the given RSSI and SNR values to an overall summary
 *  as a unifi_BasicUsability type.
 *
 * NOTE: these values are based on empirical data and will need to be tuned.
 *
 * @param[in]  rssi    : Received Signal Strength Indication value
 * @param[in]  snr     : Signal to Noise value
 *
 * @return
 *      unifi_Unusable
 *      unifi_Poor
 *      unifi_Satisfactory
 */
extern unifi_BasicUsability srs_get_basic_usability(FsmContext* context, CsrInt16 rssi, CsrInt16 snr);

/**
 * @brief Checks and expires a scan result if needed
 *
 * @par Description
 * Checks and expires a scan result if needed
 *
 * @param[in] bssid : bssid of the scan result
 * @param[in] validitySec : lifetime in seconds
 *
 * @return
 *        void
 */
extern void srs_check_for_expiry(FsmContext* context, const unifi_MACAddress* bssid, CsrUint16 validitySec);

/**
 * @brief Build and send a pre auth candidate list if required.
 *
 * @par Description
 * Build and send a pre auth candidate list if required.
 *
 * @param[in] joinReq : flag to indicate a new join.
 *
 * @return
 *        void
 */
extern void srs_build_pre_auth_candidate_list(FsmContext* context);

/**
 * @brief get the current number of scan results
 *
 * @return
 *    number of scan results
 */
extern CsrUint16 srs_scanresults_count(FsmContext* context);

/**
 * @brief Calculate the number of bytes required to fill in the scan results buffer
 *
 * @return
 *    number of bytes required
 */
extern CsrUint16 srs_copy_scanresults_bytes_required(FsmContext* context);


/**
 * @brief Copy the scan data into the buffer for the MGT SAP
 *
 * @return
 *    unifi_Success or unifi_NoRoom
 */
extern unifi_Status srs_copy_scanresults(FsmContext* context, CsrUint16* numElements, unifi_ScanResult* results, CsrUint16 buffsize, CsrBool fullCopy);

/**
 * @brief chops the list down to given number of scan results
 *
 * @par Description
 * chops the list down to given number of scan results
 *
 * @param[in] context : fsm context.
 * @param[in] preJoinSSID : SSID of a current prejoin .
 * @param[in] pCurrentScanResult : current scan result being added.
 * @param[out] pResultDeleted : flag to indicate is current SR has ben deleted.
 *
 * @return
 */
extern void srs_crop_scan_result_list(
                    FsmContext* const context,
                    const unifi_SSID * preJoinSSID,
                    const srs_scan_data *pCurrentScanResult,
                    CsrBool * const pResultDeleted);

extern void srs_get_unifi_SSID(FsmContext* context,
                    const DataReference ieRef,
                    unifi_SSID *pSsid);

/**
 * @brief Copy the scan data into the buffer for the MGT SAP
 *
 * @return
 *    unifi_Success or unifi_NoRoom
 */
/* extern CsrUint8 calculate_signal_values(unifi_ScanResult* pScanResult); */
extern CsrUint16 calculate_ranking_metric( srs_scan_data *pScanData );

/**
 * @brief Tries to give a channel that is least noisy/most free by utilizing scan results
 *
 * @par Description:
 *  This function uses the previously obtained scan results to find out a free channel. It
 *  may later be used to start an Adhoc network. Currently there is no support for Band
 *  selection and the algorithm is simple.
 *
 * NOTE: Currently 802.11a is not supported by this function
 *
 * @ingroup scanmanager
 *
 * @param[in]  networkType80211    : Band in which the resulting channel number is required
 *
 * @return
 *      Channel Number
 */
extern CsrUint8 srs_get_free_channel(FsmContext* context, unifi_80211NetworkType networkType80211);


/**
 * @brief
 *   Get the current scan interval
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   CsrUint16 current scan interval
 */
extern CsrBool srs_ssid_compare(
        const unifi_SSID *pPrimarySSID,
        const unifi_SSID *pSecondarySSID);


/**
 * @brief
 *   Get the current scan interval
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   CsrUint16 current scan interval
 */
extern CsrBool srs_scan_result_contains_ssid(
        FsmContext* context,
        srs_scan_data* pScanResult,
        const unifi_SSID *pSSID);


/**
 * @brief
 *   extracts security data from a scanresult
 *
 * @par Description
 *   extracts security data from a scanresult, however, this may be
 *   in an SSIDL IE and therefore this will need to be searched.
 *
 * @param[in]  context : FSM context
 * @param[in]  pScanData : Scanresult to process
 * @param[in]  pBSSID : bssid to match
 * @param[in]  pSSID : ssid to match
 * @param[in]  securityData : FSM context
 *
 * @return
 *   CsrUint16 current scan interval
 */
extern CsrBool srs_get_security_data(
        FsmContext* context,
        srs_scan_data* pScanData,
        const unifi_MACAddress *pBSSID,
        const unifi_SSID *pSSID,
        srs_security_data *securityData);

/**
 * @brief
 *   Rescan cloaked ssids as the list has been updated
 *
 * @param[in]  context : FSM context
 *
 * @return
 *   CsrUint8 count of the scans initiated
 */
CsrUint8 srs_scan_cloaked_ssids(FsmContext* context);

/** \@}
 */


#ifdef __cplusplus
}
#endif

#endif /*SCAN_RESULTS_STORAGE_H_*/
