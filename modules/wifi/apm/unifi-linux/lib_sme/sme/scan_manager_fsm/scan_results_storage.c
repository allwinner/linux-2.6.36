/** @file scan_results_storage.c
 *
 * The scan result storage entity.
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
 *   This file encapsulates the scan results handling and storage controls
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/scan_manager_fsm/scan_results_storage.c#8 $
 *
 ****************************************************************************/
#include "scan_manager_fsm/scan_results_storage.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "scan_manager_fsm/scan_manager_fsm.h"
#include "scan_manager_fsm/roaming_channel_lists.h"
#include "link_quality_fsm/link_quality_fsm.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "payload_manager/payload_manager.h"
#include "ie_access/ie_access.h"
#include "security_manager_fsm/security_8021x.h"
#include "csr_cstl/csr_wifi_list.h"

#include "network_selector_fsm/network_selector_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "mgt_sap/mgt_sap_from_sme_interface.h"

#include "smeio/smeio_trace_types.h"

#ifdef CCX_VARIANT
#include "ccx_enhanced_roaming/ccx_enhanced_roaming.h"
#endif

/* MACROS *******************************************************************/

#define getScanData(context) (getSmeContext(context)->scanData)

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

typedef struct csr_list_cloaked_scan_result
{
    /** list structure */
    csr_list_node node;

    /* Address of the scan result */
    unifi_MACAddress bssid;

    /* Channel AP seen on */
    CsrUint8 channelNumber;

    /** Does the Firmware Own this scan or should we expire it */
    CsrUint16     firmwareOwned;
    /** Time last updated */
    CsrUint32     lastUpdateTime;

} csr_list_cloaked_scan_result;


/**
 * @brief defines scan results global data structure
 *
 * @par Description
 *   defines scan results global data structure
 */
struct ScanResults
{
    /** The primary scan results list */
    CsrUint8 listCount;
    csr_list rankedList;

    /** The results of a Join scan result */
    srs_scan_data* joinScanResult;

    csr_list currentCloakedAddresses;

    /** Current pmkid canditates */
    CsrUint8 pmkidCandidatesCount;
    unifi_PmkidCandidate* pmkidCandidates;
};

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

static void insert_into_sr_list(FsmContext* context,
                    csr_list *prankedList,
                    srs_csr_list_scan_result *pScanResultNode);

static CsrBool is_ssid_cloaked(
                    const unifi_SSID *ssid);

static void update_scan_result_rank(FsmContext* context, srs_csr_list_scan_result *pSRelement);

static srs_csr_list_scan_result* findScanResultNode(FsmContext* context,
                    srs_csr_list_scan_result* pScanResultNode,
                    const unifi_SSID* ssid,
                    const unifi_MACAddress* bssid);

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/*---------------------------------------------------------------------------*/

/**
 * @brief prints all IEs from the Scan Indication
 *
 * @par Description:
 * prints all IEs from the Scan Indication
 *
 * @ingroup scan_manager
 *
 * @param[in] informationElements : Information Elements to be printed
 *
 * @return
 *      void
 */
/*---------------------------------------------------------------------------*/
sme_trace_info_code(
static void print_ie_elements(FsmContext* context,
                    DataReference informationElements)
{
    void *buf = NULL;
    CsrUint16 length;

    if (informationElements.dataLength == 0)
    {
        return;
    }

    pld_access(getPldContext(context), (PldHdl)informationElements.slotNumber, &buf, &length);
    ie_printIEInfo(buf, length);
}
)

/**
 * @brief determines if a SSID is from a cloaked AP
 *
 * @par Description:
 * During a fullscan, a mlme scan Indication can be recieved from a cloaked AP
 * the SSID must be checked to see if it is cloaked.  This can be
 * determined in 2 ways
 *
 * 1. The length field is 0
 * 2. The length field is not 0 but the field is entirely 0
 *
 * @ingroup scanmanager
 *
 * @param[in]     pSsid : The Ssid to be checked
 *
 * @return
 *      TRUE:  Cloaked
 *      FALSE: Not Cloaked
 */
static CsrBool is_ssid_cloaked(
                    const unifi_SSID *pSsid)
{
    CsrUint8 count = 0;

    /* check the length of the SSID */
    if (pSsid->length == 0)
    {
        return TRUE;
    }
    else
    {
        /* check the contents */
        while( (count < 32)
             &&(count < pSsid->length) )
        {
            /* any other value other than 0 then it must not be cloaked */
            if(pSsid->ssid[count] !=0)
            {
                return FALSE;
            }
            count++;
        }
        /* did not find a valid value */
        return TRUE;
    }
}

/**
 * @brief extracts the SSID from the Scan Indication
 *
 * @par Description:
 * the passed mlme scan indication points to an Information Element that
 * contains an SSID.
 *
 * @ingroup scanmanager
 *
 * @param[in]  ieRef : IE to be processed
 * @param[out] pSsid : structure to be populated.
 *
 * @return
 *      void
 */
/*---------------------------------------------------------------------------*/
void srs_get_unifi_SSID(FsmContext* context,
                    const DataReference ieRef,
                    unifi_SSID *pSsid)
{
    void *buf = NULL;
    CsrUint16 length;
    CsrUint8* element = NULL;
    ie_result ieCode;

    require(TR_SCAN_STORAGE, pSsid != NULL);

    /* Null the SSID */
    CsrMemSet(pSsid,0, sizeof(unifi_SSID));

    if (ieRef.dataLength == 0)
    {
        return;
    }

    pld_access(getPldContext(context), (PldHdl)ieRef.slotNumber, &buf, &length);

    ieCode = ie_find(IE_ID_SSID, buf, length, &element);
    if (ieCode != ie_success)
    {
        sme_trace_error((TR_SCAN_STORAGE, "get_unifi_SSID()::ie_find(IE_ID_SSID) Failed :: %d", ieCode));
        return;
    }
    else
    {
        if(ie_success != ie_getSSID(element, pSsid))
        {
            /* if the function fails reset the length to 0 */
            pSsid->length = 0;
        }
        /* just to be safe null terminate the string,
         * but only if there is space
         * */
        if (pSsid->length < UNIFI_SSID_MAX_OCTETS)
        {
            pSsid->ssid[pSsid->length] = 0;
        }
    }
}


/**
 * @brief Gives a Basic Usability summary for RSSI/SNR values
 *
 * @par Description:
 *  This function maps the given RSSI and SNR values to an overall summary
 *  as a unifi_BasicUsability type.
 *
 * NOTE: these values are based on empirical data and will need to be tuned.
 *
 * @ingroup scanmanager
 *
 * @param[in]  rssi    : Received Signal Strength Indication value
 * @param[in]  snr     : Signal to Noise value
 *
 * @return
 *      unifi_Unusable
 *      unifi_Poor
 *      unifi_Satisfactory
 */
/*---------------------------------------------------------------------------*/
unifi_BasicUsability srs_get_basic_usability(FsmContext* context,
                    CsrInt16 rssi,
                    CsrInt16 snr)
{
    unifi_BasicUsability rssiUsability = unifi_Unusable;
    unifi_BasicUsability snrUsability = unifi_Unusable;
    unifi_BasicUsability combinedUsability;


    /* Decide the likely reliability of the signal based on strength.  */
    if (rssi > RSSI_THRESHOLD_SATISFACTORY)
    {
        rssiUsability = unifi_Satisfactory;
    }
    else if (rssi > RSSI_THRESHOLD_POOR)
    {
        rssiUsability = unifi_Poor;
    }
    /* else leave at unifi_Unusable */


    /* Decide the likely usability of the signal based on signal to noise
     * ratio
    */
    if (snr > SNR_THRESHOLD_SATISFACTORY)
    {
        snrUsability = unifi_Satisfactory;
    }
    else if (snr > SNR_THRESHOLD_POOR)
    {
        snrUsability = unifi_Poor;
    }
    /* else leave at unifi_Unusable */


    /* Return the worse of the usabilities to the caller */
    if ((snrUsability == unifi_Unusable) ||
        (rssiUsability == unifi_Unusable))
    {
        combinedUsability = unifi_Unusable;
    }
    else if ((snrUsability == unifi_Poor) ||
        (rssiUsability == unifi_Poor))
    {
        combinedUsability = unifi_Poor;
    }
    else
    {
        combinedUsability = unifi_Satisfactory;
    }

    sme_trace_info((TR_SCAN_STORAGE, "srs_get_basic_usability():: RSSI %d, SNR %d: %s", rssi, snr, trace_unifi_BasicUsability(combinedUsability)));


    return(combinedUsability);
}

/**
 * @brief reorders the profile list
 *
 * @par Description:
 * This function reorders the profile list
 *
 * @ingroup scanmanager
 *
 * @return
 *      None
 */
/*---------------------------------------------------------------------------*/
void srs_rank_scan_results(FsmContext* context)
{
    ScanResults* scanData = getScanData(context);
    srs_csr_list_scan_result* pCurrentSRNode = NULL;
    csr_list newrankedList;

    sme_trace_entry((TR_SCAN_STORAGE, "srs_rank_scan_results()"));

    csr_list_init(&newrankedList);

    /* take each entry in turn and locate it as required */
    pCurrentSRNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);

    while(NULL != pCurrentSRNode)
    {
        srs_csr_list_scan_result* pNextSRNode = NULL;

        pNextSRNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pCurrentSRNode->rankedNode);

        /* remove the node from the original list */
        (void)csr_list_remove(&scanData->rankedList, &pCurrentSRNode->rankedNode);
        /* remove it from the list count so it is not recounted */
        scanData->listCount--;
        /* and re-insert it into the new list */
        insert_into_sr_list(context, &newrankedList, pCurrentSRNode);

        /* move onto the next one */
        pCurrentSRNode = pNextSRNode;
    }

    scanData->rankedList = newrankedList;

    /* build a new preauth list ready */
    srs_build_pre_auth_candidate_list(context);

    /* Check for a roam and inform the network selector if needed */
    (void) check_if_we_should_roam(context);
}

/**
 * @brief reorders the profile list
 *
 * @par Description:
 * This function reorders the profile list
 *
 * @ingroup scanmanager
 *
 * @return
 *      None
 */
/*---------------------------------------------------------------------------*/
void srs_print_scan_results(FsmContext* context)
{
    sme_trace_debug_code(
    CsrUint32 i = 1;
    ScanResults* scanData = getScanData(context);
    srs_csr_list_scan_result* pCurrentNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);

    sme_trace_entry((TR_SCAN_STORAGE, "srs_print_scan_results()"));

    while(NULL != pCurrentNode)
    {
        sme_trace_debug((TR_SCAN_STORAGE, "%2d) rank %2d channel %2d BSSID %s SSID %-20s RSSI %d SNR %d",
            i++,
            pCurrentNode->rankValue,
            pCurrentNode->scanParameters.scanResult.channelNumber,
            trace_unifi_MACAddress(pCurrentNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
            trace_unifi_SSID(&pCurrentNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)),
            pCurrentNode->scanParameters.scanResult.rssi,
            pCurrentNode->scanParameters.scanResult.snr));

        pCurrentNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pCurrentNode->rankedNode);
    }
    )
}

/**
 * @brief recalculates the rank value for this profile element
 *
 * @par Description:
 * This function reorders the profile list
 *
 * @ingroup scanmanager
 *
 * @return
 *      None
 */
/*---------------------------------------------------------------------------*/
CsrBool rerank_single_entry(FsmContext* context, const srs_csr_list_scan_result *pScanResultNode)
{
    /* find teh previous node */
    ScanResults* scanData = getScanData(context);
    srs_csr_list_scan_result *pPreviousNode = NULL;
    srs_csr_list_scan_result *pCurrentNode = NULL;
    srs_csr_list_scan_result *pNextNode = NULL;
    CsrBool bReinsert = FALSE;

    sme_trace_entry((TR_SCAN_STORAGE, "rerank_single_entry()"));

    pCurrentNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);

    while (pCurrentNode != pScanResultNode)
    {
        pPreviousNode = pCurrentNode;
        pCurrentNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pCurrentNode->rankedNode);

        if (pCurrentNode == NULL)
        {
            return FALSE;
        }
    }
    sme_trace_debug((TR_SCAN_STORAGE, "Current RankValue: rank %d: BSSID %s SSID %s",
        pCurrentNode->rankValue,
        trace_unifi_MACAddress(pCurrentNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
        trace_unifi_SSID(&pCurrentNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)) ));

    /* Now compare */

    if(  pPreviousNode != NULL)
     {
         sme_trace_debug((TR_SCAN_STORAGE, "Previous RankValue: rank %d: BSSID %s SSID %s",
             pPreviousNode->rankValue,
             trace_unifi_MACAddress(pPreviousNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
             trace_unifi_SSID(&pPreviousNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)) ));
     }
    if(  (pPreviousNode != NULL)
      && (pCurrentNode->rankValue > pPreviousNode->rankValue) )
    {
        bReinsert = TRUE;
    }
    else
    {

        pNextNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pCurrentNode->rankedNode);

        /* ok so far */
        if(  pNextNode != NULL)
        {
            sme_trace_debug((TR_SCAN_STORAGE, "Next RankValue: rank %d: BSSID %s SSID %s",
                pNextNode->rankValue,
                trace_unifi_MACAddress(pNextNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
                trace_unifi_SSID(&pNextNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)) ));
        }

        if(  (pNextNode != NULL)
          && (pCurrentNode->rankValue <= pNextNode->rankValue) )
        {
            bReinsert = TRUE;
        }
    }

    if(bReinsert)
    {
        /* failed the tests so reinsert */
        sme_trace_debug((TR_SCAN_STORAGE, "entry re-ranked"));

        (void)csr_list_remove(&scanData->rankedList, &pCurrentNode->rankedNode);

        /* The list need to be reduced. */
         scanData->listCount--;

        insert_into_sr_list(context, &scanData->rankedList, pCurrentNode);

        /* Check for a roam and inform the network selector if needed */
        (void) check_if_we_should_roam(context);
    }

    return TRUE;
}

/**
 * @brief calculates a join/roam metric value based on RSSI, SNR and Admission Capacity
 *
 * @par Description
 * calculates a join/roam metric value based on RSSI, SNR and Admission Capacity
 * The admission capacity is only taken into account if CCX is enabled
 * Otherwise it will only use RSSI and SNR (same as before)
 *
 * @param[in] pSRelement containing the necessary input data
 *
 * @return
 *        unit8 : the calculated metric value
 */
CsrUint16 calculate_ranking_metric( srs_scan_data *pScanData )
{
    CsrUint16 metricValue;
#ifdef CCX_VARIANT
    CsrUint16 admissionCap;
#endif

    /* The valid RSSI range in the firmware is around -152dBm to +8dBm */
    /* the reported rx sensitivty in the data sheet, -92dBm to -4dBm */

    /* The valid SNR range from the hardware is -127 to +160 */
    /* offset the values and then scale */
    CsrUint16 rssi = (CsrUint16)pScanData->scanResult.rssi + 152;
    CsrUint16 snr  = (CsrUint16)pScanData->scanResult.snr + 152;

    /* Always Include signal quality contribution */
    metricValue = (rssi + snr)/10;

#ifdef CCX_VARIANT
    /* If CCX: Add Admission Capacity contribution (normalised in the range [0-WEIGHT_AP_ADMIN_CAPACITY] ) */
    /* But exclude APs which have not reported a QBSS LOAD IE */

    admissionCap= pScanData->ap_currentAdmitCapacity;
    if ( admissionCap != 0xffff )
    {
        metricValue += (CsrUint16) (( WEIGHT_AP_ADMIN_CAPACITY * admissionCap ) / LARGEST_AP_ADMIN_CAPACITY);
    }
    else
    {
        admissionCap = 0;
    }

    sme_trace_debug(( TR_SCAN_STORAGE, "calculate_ranking_metric(): rssi %d snr %d, Admission Capacity %d :: Resulting Metric Value %d",
                      pScanData->scanResult.rssi,
                      pScanData->scanResult.snr,
                      admissionCap,
                      metricValue ));
#else

    sme_trace_debug(( TR_SCAN_STORAGE, "calculate_ranking_metric(): rssi %d snr %d :: Resulting Metric Value %d",
                      pScanData->scanResult.rssi,
                      pScanData->scanResult.snr,
                      metricValue ));
#endif

    return metricValue;
}


/**
 * @brief recalculates the rank value for this profile element
 *
 * @par Description:
 * This function reorders the profile list
 *
 * @ingroup scanmanager
 *
 * @return
 *      None
 */
/*---------------------------------------------------------------------------*/
static void update_scan_result_rank(FsmContext* context, srs_csr_list_scan_result *pSRelement)
{
    srs_csr_list_scan_result *pSRelementLocal = pSRelement; /* Rubbish for lint */
    sme_trace_entry((TR_SCAN_STORAGE, ">> update_profile_rank()"));

    pSRelement->rankValue = calculate_ranking_metric(&pSRelementLocal->scanParameters);

    sme_trace_entry(( TR_SCAN_STORAGE, "<< update_scan_result_rank(): rank value: %d: SSID: %s, BSSID: %s",
                      pSRelement->rankValue,
                      trace_unifi_SSID(&pSRelement->scanParameters.scanResult.ssid, getSSIDBuffer(context)),
                      trace_unifi_MACAddress(pSRelement->scanParameters.scanResult.bssid, getMacAddressBuffer(context)) ));

}


/**
 * @brief detemines the APs capabilities
 *
 * @par Description:
 * detemines the APs capabilities
 *
 * @ingroup scanmanager
 *
 * @return
 *      None
 */
/*---------------------------------------------------------------------------*/
static void update_ap_capability(
                        FsmContext* context,
                        srs_scan_data *pSRelement,
                        CapabilityInformation capabilityInformation,
                        DataReference informationElements)
{
    void* ieBuf = NULL;
    CsrUint16 ieLength;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_SCAN_STORAGE, "update_ap_capability"));

    /* reset the values */
    pSRelement->ieCapabilityFlags = 0;
    pSRelement->securityData.wpaCipherCapabilityFlags = unifi_EncryptionCipherNone;
    pSRelement->securityData.wpa2CipherCapabilityFlags = unifi_EncryptionCipherNone;
    pSRelement->securityData.wapiCipherCapabilityFlags = unifi_EncryptionCipherNone;
    pSRelement->securityData.authMode_CapabilityFlags = unifi_80211AuthOpen;


    if((capabilityInformation & 0x0010) != 0)
    {
        pSRelement->securityData.privacyMode = unifi_80211PrivacyEnabled;
        pSRelement->securityData.authMode_CapabilityFlags |= unifi_80211AuthShared;
    }
    else
    {
        pSRelement->securityData.privacyMode = unifi_80211PrivacyDisabled;
    }

    if (informationElements.dataLength == 0)
    {
        return;
    }

    pld_access(getPldContext(context), informationElements.slotNumber, &ieBuf, &ieLength);

    /*
     * now look for all capability indications
     */

    /* WMM */
    if (ie_wmm_check_ap_wmm_capability(ieBuf, ieLength))
    {
        pSRelement->ieCapabilityFlags |= WMM_Capable;
        if (ie_isWMMUAPSD(ieBuf, ieLength))
        {
            pSRelement->ieCapabilityFlags |= WMMUAPSD_Capable;
        }
        else
        {
            pSRelement->ieCapabilityFlags &= ~WMMUAPSD_Capable;
        }
    }
    else
    {
        pSRelement->ieCapabilityFlags &= ~WMM_Capable;
        pSRelement->ieCapabilityFlags &= ~WMMUAPSD_Capable;
    }

    /* WPS */
    if (ie_wps_check_ap_wps_capability(ieBuf, ieLength))
    {
        pSRelement->ieCapabilityFlags |= WPS_Capable;
    }
    else
    {
        pSRelement->ieCapabilityFlags &= ~WPS_Capable;
    }

    /* RSN */
    ie_rsn_update_ap_capabilities(pSRelement, ieBuf, ieLength, cfg->smeConfig.allowUnicastUseGroupCipher);

#ifdef CCX_VARIANT
    if (ie_ccx_check_ap_ccx_capability(ieBuf, ieLength))
    {
        pSRelement->ieCapabilityFlags |= CCX_Capable;
    }
    else
    {
        pSRelement->ieCapabilityFlags &= ~CCX_Capable;
    }
#endif

    if (ie_dot11n_check_ap_capability(ieBuf, ieLength))
    {
        pSRelement->ieCapabilityFlags |= DOT11N_Capable;
    }
    else
    {
        pSRelement->ieCapabilityFlags &= ~DOT11N_Capable;
    }


    sme_trace_info_code(
        sme_trace_info((TR_SCAN_STORAGE, "======================================================"));
        sme_trace_info((TR_SCAN_STORAGE, "ieCapabilityFlags %d authMode_CapabilityFlags %x",
                        pSRelement->ieCapabilityFlags,
                        pSRelement->securityData.authMode_CapabilityFlags));

        if ((pSRelement->ieCapabilityFlags & WMM_Capable) == WMM_Capable)
            sme_trace_info((TR_SCAN_STORAGE, "WMM CAPABLE"));
        if ((pSRelement->ieCapabilityFlags & WPS_Capable) == WPS_Capable)
            sme_trace_info((TR_SCAN_STORAGE, "WPS CAPABLE"));
    )

#ifdef CCX_VARIANT
    sme_trace_info_code(
        if ((pSRelement->ieCapabilityFlags & CCX_Capable) == CCX_Capable)
            sme_trace_info((TR_SCAN_STORAGE, "CCX CAPABLE"));
    )
#endif

    sme_trace_info_code(
        if ((pSRelement->ieCapabilityFlags & DOT11N_Capable) == DOT11N_Capable)
            sme_trace_info((TR_SCAN_STORAGE, "802.11n CAPABLE"));
    )

    sme_trace_info_code(
        if ((pSRelement->securityData.authMode_CapabilityFlags & unifi_8021xAuthWPAPSK) == unifi_8021xAuthWPAPSK)
            sme_trace_info((TR_SCAN_STORAGE, "WPAPSK CAPABLE"));
        if ((pSRelement->securityData.authMode_CapabilityFlags & unifi_8021xAuthWPA) == unifi_8021xAuthWPA)
            sme_trace_info((TR_SCAN_STORAGE, "WPA CAPABLE"));
        if ((pSRelement->securityData.authMode_CapabilityFlags & unifi_8021xAuthWPA2PSK) == unifi_8021xAuthWPA2PSK)
            sme_trace_info((TR_SCAN_STORAGE, "WPA2PSK CAPABLE"));
        if ((pSRelement->securityData.authMode_CapabilityFlags & unifi_8021xAuthWPA2) == unifi_8021xAuthWPA2)
            sme_trace_info((TR_SCAN_STORAGE, "WPA2 CAPABLE"));
        sme_trace_info((TR_SCAN_STORAGE, "privacyMode %s", trace_unifi_80211PrivacyMode(pSRelement->securityData.privacyMode)));
        sme_trace_info((TR_SCAN_STORAGE, "======================================================"));

    )
}


/**
 * @brief populates the scan parameter with new data.
 *
 * @par Description:
 * this function populates a the scan parameter with new data.
 *
 * @ingroup scanmanager
 *
 * @param[in] ssid : The current SSID
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
 * @param[in] pScanParameters : Structure to be populated
 *
 * @return
 *      void
 */
/*---------------------------------------------------------------------------*/
static void populate_scan_parameters(FsmContext* context,
                                     unifi_SSID* ssid,
                                     DataReference informationElements,
                                     Interface ifIndex,
                                     BssType bssType,
                                     unifi_MACAddress bSSID,
                                     TimeUnits beaconPeriodTu,
                                     TsfTime timestamp,
                                     TsfTime localTime,
                                     ChannelNumber channel,
                                     Megahertz channelFrequency,
                                     CapabilityInformation capabilityInformation,
                                     Decibels rSSI,
                                     Decibels sNR,
                                     srs_scan_data *pScanParameters)
{
    unifi_ScanResult *pScanResult = NULL;
    void* ieBuf = NULL;
    CsrUint16 ieLength;

    require(TR_SCAN_STORAGE, pScanParameters != NULL);

    sme_trace_info_code( print_ie_elements(context, informationElements) );

    pScanResult = &pScanParameters->scanResult;
    pScanResult->bssid = bSSID;
    pScanResult->ssid = *ssid;

    update_ap_capability(context, pScanParameters,
                         capabilityInformation,
                         informationElements);

    /* note: there will always be IEs to process */
    pld_access(getPldContext(context), informationElements.slotNumber, &ieBuf, &ieLength);

#ifdef CCX_VARIANT
    if(pScanParameters->ieCapabilityFlags & CCX_Capable)
    {
        process_ccx_ies(ieBuf, ieLength, &pScanParameters->ccxCtrlBlk);
    }

    /* Process QBSS Load IE (Useful for Roaming) */
    pScanParameters->ap_currentAdmitCapacity = ccx_reh_process_qbss_load_ie( context, bssType, informationElements );
#endif

    pScanResult->usability = srs_get_basic_usability(context, rSSI, sNR);
    pScanResult->rssi = rSSI;
    pScanResult->snr = sNR;
    pScanResult->channelNumber = (CsrUint8)channel; /* down cast the hip type */
    pScanResult->bssType = bssType; /* These Types are compatable */

    pScanResult->ifIndex = ifIndex;
    pScanResult->beaconPeriodTu = beaconPeriodTu;
    pScanResult->channelFrequency = channelFrequency;
    pScanResult->capabilityInformation = capabilityInformation;

    /* These types are compatable */
    pScanResult->timeStamp = *((unifi_TsfTime*)&timestamp);
    pScanResult->localTime = *((unifi_TsfTime*)&localTime);

    pScanParameters->informationElements = informationElements;
    pScanResult->informationElements = ieBuf;
    pScanResult->informationElementsLength = ieLength;
}

/**
 * @brief places the given scanresult structure in the scan list
 *
 * @par Description:
 * this function takes a scan result and places it on the scan in accordance
 * the current ranking procedure
 *
 * @ingroup scanmanager
 *
 * @param[in] prankedList : Pointer to the ranked list
 * @param[in] pScanResultNode : Scan node to be added to list
 *
 * @return
 *      void
 */
/*---------------------------------------------------------------------------*/
static void insert_into_sr_list(
                    FsmContext* context,
                    csr_list *prankedList,
                    srs_csr_list_scan_result *pScanResultNode)
{
    ScanResults* scanData = getScanData(context);
    srs_csr_list_scan_result* pWPCurrentNode = NULL;
    srs_csr_list_scan_result* pWPPreviousNode = NULL;

    sme_trace_entry((TR_SCAN_STORAGE, "insert_into_sr_list(%s)", trace_unifi_SSID(&pScanResultNode->scanParameters.scanResult.ssid, getSSIDBuffer(context))));

    scanData->listCount++;

    /*  if the list is just empty just add it */
    if(csr_list_isempty(prankedList))
    {
        sme_trace_debug((TR_SCAN_STORAGE, "Empty list: insert at the head of the list"));
        csr_list_insert_head(prankedList,  list_owns_neither, &pScanResultNode->rankedNode, pScanResultNode);
    }
    else
    {
        sme_trace_debug((TR_SCAN_STORAGE, "pScanResultNode->rankValue %d", pScanResultNode->rankValue));

        for (pWPCurrentNode = csr_list_gethead_t(srs_csr_list_scan_result *, prankedList); pWPCurrentNode != NULL;
             pWPCurrentNode = csr_list_getnext_t(srs_csr_list_scan_result *, prankedList, &pWPCurrentNode->rankedNode))
        {
            /* check if the new ranking is better than the current value
             * if it is then it will need to go infront
            sme_trace_debug((TR_SCAN_STORAGE, "new  %d vs current %d", pScanResultNode->rankValue, pWPCurrentNode->rankValue));
             */

            if ( pScanResultNode->rankValue > pWPCurrentNode->rankValue)
            {
                /* use the previous pointer to see if e are still at
                 * the head of the   list.
                 */
                if (NULL == pWPPreviousNode)
                {
                    sme_trace_debug((TR_SCAN_STORAGE, "insert at the head of the list"));
                    csr_list_insert_head(prankedList,
                                         list_owns_neither,
                                         &pScanResultNode->rankedNode,
                                         pScanResultNode);
                }
                else
                {
                    sme_trace_debug((TR_SCAN_STORAGE, "insert_into_sr_list:: add after "));
                    csr_list_insert_after(prankedList,
                                          list_owns_neither,
                                          &pWPPreviousNode->rankedNode,
                                          &pScanResultNode->rankedNode,
                                          pScanResultNode);
                }

                return;
            }

            pWPPreviousNode = pWPCurrentNode;
        }


        /* must have reach the end of the list */
        csr_list_insert_tail(prankedList,
                             list_owns_neither,
                             &pScanResultNode->rankedNode,
                             pScanResultNode);
    }
}

/**
 * @brief deletes a scan result
 *
 * @return
 *      void
 */
/*---------------------------------------------------------------------------*/
static void remove_scan_node(
                    FsmContext* context,
                    srs_csr_list_scan_result* deleteNode,
                    CsrBool sendDeleteInd,
                    CsrBool forceDelete)
{
    SmeConfigData* cfg = get_sme_config(context);
    ScanResults* scanData = getScanData(context);

    sme_trace_entry((TR_SCAN_STORAGE, "remove_scan_node: %s, %s",
                trace_unifi_SSID(&deleteNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)),
                trace_unifi_MACAddress(deleteNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context))));

    /* NEVER DELETE THE CURRENTLY CONNECTED SCAN DATA */
    if ( ( (!forceDelete)
         &&(ns_get_connection_status(context) != ns_ConnectionStatus_Disconnected)
         &&(CsrMemCmp(&deleteNode->scanParameters.scanResult.bssid, &cfg->connectionInfo.bssid, sizeof(cfg->connectionInfo.bssid.data)) == 0))
       ||( (!forceDelete)
         &&(isInRoamingList(context,&deleteNode->scanParameters.scanResult.bssid)) ))
    {
        /* the firmware has release ownership, its the SMEs now. */
        sme_trace_info((TR_SCAN_STORAGE, "remove_scan_node: SME owns result for %s: not deleting because it is connected AP",
            trace_unifi_MACAddress(deleteNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context))));
        deleteNode->firmwareOwned = 0;
        return;
    }

    if (sendDeleteInd)
    {
        void* appHandles;
        CsrUint16 appHandleCount = get_appHandles(context, unifi_IndScanResult, &appHandles);
        unifi_MACAddress bssid = deleteNode->scanParameters.scanResult.bssid;

        sme_trace_info((TR_SCAN_STORAGE, "remove_scan_node: sending deletion unifi_mmi_scan_result_ind for  BSSID %s SSID %s (owned by %s)",
            trace_unifi_MACAddress(bssid, getMacAddressBuffer(context)),
            trace_unifi_SSID(&deleteNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)),
            (deleteNode->firmwareOwned != 0) ? "firmware" : "SME" ));

        /* 0 The scan result memory and restore the bssid for passing to the mgt api */
        CsrMemSet(&deleteNode->scanParameters.scanResult, 0, sizeof(unifi_ScanResult));
        deleteNode->scanParameters.scanResult.bssid = bssid;

        if (appHandleCount)
        {
            call_unifi_mgt_scan_result_ind(context, appHandleCount, appHandles, &deleteNode->scanParameters.scanResult);
        }
    }

    if (deleteNode->scanParameters.informationElements.dataLength != 0)
    {
        pld_release(getPldContext(context), deleteNode->scanParameters.informationElements.slotNumber);
    }

    /* If the Scan data for the join scan is removed then set the join data to NULL */
    if (scanData->joinScanResult == &deleteNode->scanParameters)
    {
        scanData->joinScanResult = NULL;
    }

    (void)csr_list_remove(&scanData->rankedList, &deleteNode->rankedNode);
    CsrPfree(deleteNode);
    scanData->listCount--;
}


/**
 * @brief places the given scanresult structure in the scan list
 *
 * @par Description:
 * this function takes a scan result and places it on the scan in accordance
 * the current ranking procedure
 *
 * @ingroup scanmanager
 *
 * @param[in] rankedList :
 * @param[in] pScanResultNode : Scan node to be xxxx
 * @param[in] searchData :
 *
 * @return
 *      srs_csr_list_scan_result
 */
/*---------------------------------------------------------------------------*/
static srs_csr_list_scan_result* findScanResultNode(
                    FsmContext* context,
                    srs_csr_list_scan_result* pScanResultNode,
                    const unifi_SSID* ssid,
                    const unifi_MACAddress* bssid)
{
    ScanResults* scanData = getScanData(context);
    csr_list* list = &scanData->rankedList;

    if (pScanResultNode == NULL)
    {
        pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, list);
    }
    else
    {
        pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, list, &pScanResultNode->rankedNode);
    }


    while (pScanResultNode != NULL)
    {
        CsrBool match = TRUE;
        if (ssid != NULL && ssid->length != 0)
        {
            match = srs_ssid_compare(&pScanResultNode->scanParameters.scanResult.ssid, ssid);

#ifdef CCX_VARIANT
            /* did not find the ssid look in the SSIDL IE */
            if (match == FALSE)
            {
                CsrUint8  *ieBuf = NULL;
                CsrUint16 ieLength;
                srs_csr_list_scan_result* pTempNode = pScanResultNode;

                /* just make sure that is does not exist already */
                while (pTempNode != NULL)
                {
                    if (srs_ssid_compare(&pTempNode->scanParameters.scanResult.ssid, ssid))
                    {
                        sme_trace_debug((TR_SCAN_STORAGE, " found a match later in the list "));
                        break;
                    }
                    pTempNode = csr_list_getnext_t(srs_csr_list_scan_result *, list, &pTempNode->rankedNode);
                }

                if(pTempNode == NULL)
                {
                    sme_trace_debug((TR_SCAN_STORAGE, " search the SSIDL IE"));

                    if (pScanResultNode->scanParameters.informationElements.dataLength != 0)
                    {
/*                        struct ccx_ssidl_hiddenSsid currentSSID;
 *
 */
                        pld_access(getPldContext(context), pScanResultNode->scanParameters.informationElements.slotNumber, (void*)&ieBuf, &ieLength);

                        if(ccx_ie_ssidl_get_ssid(context, ieBuf, ieLength, ssid) != NULL)
                        {
                            match = TRUE;
                        }
                    }
                }
            }
#endif
        }


        if (match && bssid != NULL)
        {
            match = FALSE;
            if (CsrMemCmp( &BroadcastBssid, bssid, sizeof(BroadcastBssid.data)) == 0)
            {
                match = TRUE;
            }
            if (CsrMemCmp( &(pScanResultNode->scanParameters.scanResult.bssid), bssid, sizeof(bssid->data)) == 0)
            {
                match = TRUE;
            }
        }


        if (match)
        {
            return pScanResultNode;
        }

        pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, list, &pScanResultNode->rankedNode);
    }

    return NULL;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
ScanResults* srs_initialise_scan_result_storage()
{
    ScanResults* results = (ScanResults*) CsrPmalloc(sizeof(ScanResults));
    csr_list_init(&results->rankedList);
    csr_list_init(&results->currentCloakedAddresses);

    results->listCount = 0;
    results->joinScanResult = NULL;
    results->pmkidCandidatesCount = 0;
    results->pmkidCandidates = NULL;

    return results;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_destroy_scan_result_storage(ScanResults* context)
{
    sme_trace_entry((TR_SCAN_STORAGE, "srs_destroy_scan_result_storage()"));

    csr_list_clear(&context->currentCloakedAddresses);
    CsrPfree(context->pmkidCandidates);

    CsrPfree(context);
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_reset_scan_result_storage(FsmContext* context)
{
    ScanResults* scanData = getScanData(context);
    srs_csr_list_scan_result *pScanResultNode = NULL;
    sme_trace_entry((TR_SCAN_STORAGE, "srs_reset_scan_result_storage()"));

    pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);

    while (NULL != pScanResultNode)
    {
        sme_trace_debug((TR_SCAN_STORAGE, "srs_reset_scan_result_storage(%s) Delete", trace_unifi_MACAddress(pScanResultNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context))));
        remove_scan_node(context, pScanResultNode, FALSE, TRUE);
        pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);
    }

    csr_list_init(&scanData->rankedList);
    csr_list_clear(&scanData->currentCloakedAddresses);

    scanData->listCount = 0;
    scanData->joinScanResult = NULL;

    if (scanData->pmkidCandidates)
    {
        CsrPfree(scanData->pmkidCandidates);
        scanData->pmkidCandidates = NULL;
    }

    scanData->pmkidCandidatesCount = 0;
    scanData->pmkidCandidates = NULL;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
CsrBool srs_ssid_compare(
                    const unifi_SSID *pPrimarySSID,
                    const unifi_SSID *pSecondarySSID)
{
    return (pPrimarySSID->length == pSecondarySSID->length) &&
        (CsrMemCmp(pPrimarySSID->ssid,pSecondarySSID->ssid, pPrimarySSID->length) == 0);
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_delete_unconnected_scan_results(FsmContext* context)
{
    ScanResults* scanData = getScanData(context);
    srs_csr_list_scan_result *pScanResultNode = NULL, *pScanResultNextNode;
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_SCAN_STORAGE, "srs_delete_unconnected_scan_results()"));

    pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);

    while (NULL != pScanResultNode)
    {
        if (ns_get_connection_status(context) != ns_ConnectionStatus_Disconnected &&
            CsrMemCmp(&pScanResultNode->scanParameters.scanResult.bssid, &cfg->connectionInfo.bssid, 6) == 0)
        {
            pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode);
            continue;
        }

        pScanResultNextNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode);

        remove_scan_node(context, pScanResultNode, TRUE, FALSE);

        pScanResultNode = pScanResultNextNode;
    }

    /* Clear out the Cloaked Results */
    csr_list_clear(&scanData->currentCloakedAddresses);
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_reset_join_scan_data(FsmContext* context)
{
    ScanResults* scanData = getScanData(context);
    if(scanData->joinScanResult)
    {
        scanData->joinScanResult = NULL;
    }
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
void srs_take_scan_results_ownership(FsmContext* context, CsrUint32 timeAdjust)
{
    ScanResults* scanData = getScanData(context);
    CsrUint32 now = fsm_get_time_of_day_ms(context);
    srs_csr_list_scan_result *pScanResultNode = NULL;
    csr_list_cloaked_scan_result* node = csr_list_gethead_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses);
    sme_trace_entry((TR_SCAN_STORAGE, "srs_take_scan_results_ownership()"));

    while (NULL != node)
    {
        if (node->firmwareOwned)
        {
            node->firmwareOwned = 0;
            node->lastUpdateTime = now;
        }
        else
        {
            node->lastUpdateTime = (CsrUint32)CsrTimeAdd(node->lastUpdateTime, timeAdjust);
        }

        node = csr_list_getnext_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses, &node->node);
    }

    /* scan the complete list */
    pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);
    while (NULL != pScanResultNode)
    {
        /* check each scan result fot ownership */
        if (pScanResultNode->firmwareOwned)
        {
            pScanResultNode->firmwareOwned = 0;
            pScanResultNode->lastUpdateTime = now;
        }
        else
        {
            pScanResultNode->lastUpdateTime = (CsrUint32)CsrTimeAdd(pScanResultNode->lastUpdateTime, timeAdjust);
        }

        pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode);
    }
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
CsrUint8 srs_scan_cloaked_ssids(FsmContext* context)
{
    ScanResults* scanData = getScanData(context);
    SmeConfigData* cfg = get_sme_config(context);
    csr_list_cloaked_scan_result* node = csr_list_gethead_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses);
    sme_trace_entry((TR_SCAN_STORAGE, "srs_rescan_cloaked_ssids()"));

    if (cfg->cloakedSsids.cloakedSsidsCount == 0)
    {
        sme_trace_debug((TR_SCAN_STORAGE, "No Cloaked ssids configured. Reset list"));
        csr_list_clear(&scanData->currentCloakedAddresses);
        return 0;
    }

    while (NULL != node)
    {
        /* Take a copy of the SSIDs */
    	unifi_SSID* ssids = (unifi_SSID*)CsrPmalloc(sizeof(unifi_SSID) * cfg->cloakedSsids.cloakedSsidsCount);
		CsrMemCpy(ssids->ssid, cfg->cloakedSsids.cloakedSsids, sizeof(unifi_SSID) * cfg->cloakedSsids.cloakedSsidsCount);

		/* Queue a scan for this AP (This is called from the Scan Manager context) */
        send_sm_scan_req(context,
                         getSmeContext(context)->scanManagerInstance,
                         fsm_get_time_of_day_ms(context),
                         node->channelNumber,
                         cfg->cloakedSsids.cloakedSsidsCount,
                         ssids,
                         node->bssid,
                         FALSE,
                         TRUE,
                         TRUE,
                         unifi_ScanStopFirstResult,
                         BssType_AnyBss,
                         0);

        node = csr_list_getnext_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses, &node->node);
    }
    return (CsrUint8)csr_list_size(&scanData->currentCloakedAddresses);
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
srs_scan_data* srs_create_scan_result(FsmContext* context,
                                      CsrUint16 firmwareOwned,
                                      DataReference informationElements,
                                      Interface ifIndex,
                                      BssType bssType,
                                      unifi_MACAddress bSSID,
                                      TimeUnits beaconPeriodTu,
                                      TsfTime timestamp,
                                      TsfTime localTime,
                                      ChannelNumber channel,
                                      Megahertz channelFrequency,
                                      CapabilityInformation capabilityInformation,
                                      Decibels rSSI,
                                      Decibels sNR)
{
    ScanResults* scanData = getScanData(context);
    SmeConfigData* cfg = get_sme_config(context);
    srs_csr_list_scan_result *pScanResultNode = findScanResultNode(context, NULL, NULL, &bSSID);
    unifi_SSID ssid;
    CsrBool isCloaked;
    srs_get_unifi_SSID(context, informationElements, &ssid);
    isCloaked = is_ssid_cloaked(&ssid);

    sme_trace_info((TR_SCAN_STORAGE, "srs_create_scan_result(%s, %s)", trace_unifi_SSID(&ssid, getSSIDBuffer(context)), trace_unifi_MACAddress(bSSID, getMacAddressBuffer(context))));

    /* When in driver mode and a cloaked AP is received chuck it*/
    if (!pScanResultNode && is_ssid_cloaked(&ssid))
    {
        csr_list_cloaked_scan_result* node = csr_list_gethead_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses);
        CsrBool cloakedAddressKnown = FALSE;
        sme_trace_debug((TR_SCAN_STORAGE, "srs_create_scan_result(%s) Cloaked", trace_unifi_MACAddress(bSSID, getMacAddressBuffer(context))));

        while (NULL != node)
        {
            if (CsrMemCmp(node->bssid.data, bSSID.data, sizeof(bSSID.data)) == 0)
            {
                cloakedAddressKnown = TRUE;
                break;
            }

            node = csr_list_getnext_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses, &node->node);
        }

        /* If we have not seen this cloaked AP before then attempt to scan for it using the cloaked ssid list */
        if (!cloakedAddressKnown && cfg->cloakedSsids.cloakedSsidsCount && cfg->cloakedSsids.cloakedSsidsCount < 0xFF)
        {
            /* Keep Track of this address */
            node = (csr_list_cloaked_scan_result*)CsrPmalloc(sizeof(csr_list_cloaked_scan_result));
            node->bssid = bSSID;
            node->channelNumber = (CsrUint8)channel;
            node->lastUpdateTime = fsm_get_time_of_day_ms(context);
            node->firmwareOwned = firmwareOwned;
            csr_list_insert_head(&scanData->currentCloakedAddresses, list_owns_value, &node->node, node);

            /* Do not scan for cloaked AP's before the initial scan is done. */
            if (cloaked_scanning_enabled(context))
            {
                /* Take a copy of the SSIDs */
            	unifi_SSID* ssids = (unifi_SSID*)CsrPmalloc(sizeof(unifi_SSID) * cfg->cloakedSsids.cloakedSsidsCount);
        		CsrMemCpy(ssids->ssid, cfg->cloakedSsids.cloakedSsids, sizeof(unifi_SSID) * cfg->cloakedSsids.cloakedSsidsCount);

        		sme_trace_debug((TR_SCAN_STORAGE, "srs_create_scan_result(%s) Cloaked First time scan", trace_unifi_MACAddress(bSSID, getMacAddressBuffer(context))));
                /* Queue a scan for this AP (This is called from the Scan Manager context) */
                send_sm_scan_req(context,
                                 getSmeContext(context)->scanManagerInstance,
                                 fsm_get_time_of_day_ms(context),
                                 (CsrUint8)channel,
                                 cfg->cloakedSsids.cloakedSsidsCount,
                                 ssids,
                                 bSSID,
                                 FALSE,
                                 TRUE,
                                 TRUE,
                                 unifi_ScanStopFirstResult,
                                 bssType,
                                 0);
            }
            else
            {
                sme_trace_debug((TR_SCAN_STORAGE, "srs_create_scan_result(%s) Cloaked First time scan deferred", trace_unifi_MACAddress(bSSID, getMacAddressBuffer(context))));
            }
        }

        pld_release(getPldContext(context), (PldHdl)informationElements.slotNumber);
        return NULL;
    }

    /* Fill in the cloaked ssid with the existing Value from the scan result */
    if (isCloaked)
    {
        ssid = pScanResultNode->scanParameters.scanResult.ssid;
        /*sme_trace_debug((TR_SCAN_STORAGE, "srs_create_scan_result(%s, %s) Cloaked fill in SSID", trace_unifi_SSID(&ssid, getSSIDBuffer(context)), trace_unifi_MACAddress(bSSID, getMacAddressBuffer(context))));*/
    }

    /* check the restricted list to see if the SSID is allowed
     * the function call will check it see if the list restriction flag is set
     */
    {
        CsrBool created = FALSE;
        /* see if the BSSID exists */
        pScanResultNode = findScanResultNode(context, NULL, NULL, &bSSID);

        /* the BSSID was not found */
        if(pScanResultNode == NULL)
        {
            sme_trace_debug((TR_SCAN_STORAGE, "srs_create_scan_result(%s, %s)", trace_unifi_SSID(&ssid, getSSIDBuffer(context)), trace_unifi_MACAddress(bSSID, getMacAddressBuffer(context))));

            created = TRUE;
            pScanResultNode = CsrPmalloc(sizeof(srs_csr_list_scan_result));

            pScanResultNode->rankValue = 0;
            pScanResultNode->scanParameters.informationElements.dataLength = 0;
        }

        if (pScanResultNode->scanParameters.informationElements.dataLength != 0)
        {
            pld_release(getPldContext(context), pScanResultNode->scanParameters.informationElements.slotNumber);
        }

        if (firmwareOwned || created)
        {
            pScanResultNode->firmwareOwned = firmwareOwned;
        }
        pScanResultNode->lastUpdateTime = fsm_get_time_of_day_ms(context);

        /* Ignore the Scan result RSSI and SNR when this is the currently connected AP */
        if (scanData->joinScanResult && &pScanResultNode->scanParameters == scanData->joinScanResult)
        {
            sme_trace_debug((TR_SCAN_STORAGE, "srs_create_scan_result(%s, %s, %3d, %3d) Connected AP Leave RSSI at %3d, %3d",
                                              trace_unifi_SSID(&ssid, getSSIDBuffer(context)),
                                              trace_unifi_MACAddress(bSSID, getMacAddressBuffer(context)),
                                              rSSI,
                                              sNR,
                                              pScanResultNode->scanParameters.scanResult.rssi,
                                              pScanResultNode->scanParameters.scanResult.snr));
            rSSI = pScanResultNode->scanParameters.scanResult.rssi;
            sNR = pScanResultNode->scanParameters.scanResult.snr;
        }

        /* fill in / Update the Scan Data */
        populate_scan_parameters(context,
                                 &ssid,
                                 informationElements,
                                 ifIndex,
                                 bssType,
                                 bSSID,
                                 beaconPeriodTu,
                                 timestamp,
                                 localTime,
                                 channel,
                                 channelFrequency,
                                 capabilityInformation,
                                 rSSI,
                                 sNR,
                                 &pScanResultNode->scanParameters);

        /* update the ranking value */
        update_scan_result_rank(context, pScanResultNode);

        /* Update Roaming Network lists if needed */
        if (cfg->roamingConfig.disableRoamScans == FALSE)
        {
            CsrBool updateRoamScan = FALSE;
            /* Only create an entry IF more that 1 AP with same SSID is seen */
            if (created)
            {
                srs_csr_list_scan_result* otherScanResult = findScanResultNode(context, NULL, &ssid, NULL);
                if (otherScanResult)
                {
                    if(roaming_channel_list_add_channel(context, &ssid, otherScanResult->scanParameters.scanResult.channelNumber))
                    {
                        updateRoamScan = TRUE;
                    }
                }
            }

            if (roaming_channel_list_add_channel_if_exists(context, &ssid, (CsrUint8)channel))
            {
                updateRoamScan = TRUE;
            }
            if (updateRoamScan)
            {
                /* Start / Restart the roaming scan */
                install_roaming_scan(context);
            }
        }

        if (created)
        {
            /* Add new entry into the ranked list */
            insert_into_sr_list(context, &scanData->rankedList, pScanResultNode);

            /* build a new preauth list ready */
            srs_build_pre_auth_candidate_list(context);

            /* Check for a roam and inform the network selector if needed */
            (void) check_if_we_should_roam(context);

        }
        else
        {
            /* check if rerank is required */
            if(rerank_single_entry(context, pScanResultNode))
            {
                /* send update the preauth list ready */
                srs_build_pre_auth_candidate_list(context);
            }
        }
    }

    return pScanResultNode == NULL?NULL:&pScanResultNode->scanParameters;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_update_join_scan_result(FsmContext* context, srs_scan_data* scanResult)
{
    ScanResults* scanData = getScanData(context);

    require(TR_SCAN_STORAGE, scanResult != NULL);

    scanData->joinScanResult = scanResult;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
srs_scan_data* srs_get_scan_parameters_first(FsmContext* context, const unifi_SSID* ssid, const unifi_MACAddress* bssid)
{
    srs_csr_list_scan_result *pScanResultNode = NULL;

    sme_trace_entry((TR_SCAN_STORAGE, "srs_get_scan_parameters_first()"));

    pScanResultNode = findScanResultNode(context, NULL, ssid, bssid);
    if (pScanResultNode != NULL)
    {
        return &pScanResultNode->scanParameters;
    }
    return NULL;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
srs_scan_data* srs_get_scan_parameters_next(FsmContext* context, const unifi_MACAddress* currentBssid, const unifi_SSID* ssid, const unifi_MACAddress* bssid)
{
    srs_csr_list_scan_result *pScanResultNode = NULL;

    sme_trace_entry((TR_SCAN_STORAGE, "srs_get_scan_parameters_next()"));

    pScanResultNode = findScanResultNode(context, NULL, ssid, currentBssid);
    if (pScanResultNode == NULL)
    {
        return NULL;
    }

    pScanResultNode = findScanResultNode(context, pScanResultNode, ssid, bssid);

    if (pScanResultNode != NULL)
    {
        return &pScanResultNode->scanParameters;
    }
    return NULL;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
srs_scan_data* srs_get_join_scan_parameters(FsmContext* context)
{
    ScanResults* scanData = getScanData(context);
    sme_trace_entry((TR_SCAN_STORAGE, "srs_get_join_scan_parameters()"));

    return scanData->joinScanResult;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
unifi_Status srs_delete_scan_result_profile(FsmContext* context, const unifi_MACAddress* bssid, CsrBool forceDelete)
{
    ScanResults* scanData = getScanData(context);
    srs_csr_list_scan_result* pScanResultNode = NULL;
    csr_list_cloaked_scan_result* node = csr_list_gethead_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses);

    sme_trace_entry((TR_SCAN_STORAGE, "srs_delete_scan_result_profile()"));

    /* Remove from cloaked list */
    while (NULL != node)
    {
        if (CsrMemCmp(node->bssid.data, bssid->data, sizeof(bssid->data)) == 0)
        {
            (void)csr_list_remove(&scanData->currentCloakedAddresses, &node->node);
            break;
        }

        node = csr_list_getnext_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses, &node->node);
    }

    pScanResultNode = findScanResultNode(context, NULL, NULL, bssid);

    while (pScanResultNode != NULL)
    {
        srs_csr_list_scan_result* pNextScanResultNode;
        sme_trace_debug((TR_SCAN_STORAGE, "srs_delete_scan_result_profile(BSSID %s, SSID %s) Delete",
            trace_unifi_MACAddress(pScanResultNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
            trace_unifi_SSID(&pScanResultNode->scanParameters.scanResult.ssid, getSSIDBuffer(context))));
        remove_scan_node(context, pScanResultNode, TRUE, forceDelete);

        pNextScanResultNode = findScanResultNode(context, NULL, NULL, bssid);
        if (pScanResultNode == pNextScanResultNode)
        {
            /* Handle case where remove_scan_node() does not delete the node */
            break;
        }
        pScanResultNode = pNextScanResultNode;
    }

    return unifi_Success;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_scan_result_update(FsmContext* context, const unifi_MACAddress *pBssid)
{
    ScanResults* scanData = getScanData(context);
    srs_csr_list_scan_result* pScanResultNode = NULL;

    sme_trace_entry((TR_SCAN_STORAGE, "srs_scan_result_update:"));

    for (pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList); pScanResultNode != NULL;
         pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode))
    {
        if(NULL == pBssid)
        {
            update_scan_result_rank(context, pScanResultNode);
        }
        else
        {
            if (CsrMemCmp( &(pScanResultNode->scanParameters.scanResult.bssid), pBssid, sizeof(pBssid->data)) == 0)
            {
                /* update a single result */
                update_scan_result_rank(context, pScanResultNode);
            }
        }
    }

    srs_rank_scan_results(context);
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
srs_scan_data* srs_scan_result_quality_update(FsmContext* context,
                    const unifi_MACAddress* bssid,
                    CsrInt16 rssi,
                    CsrInt16 snr)
{
    srs_csr_list_scan_result* pScanResultNode = NULL;

    pScanResultNode = findScanResultNode(context, NULL, NULL, bssid);

    sme_trace_entry((TR_SCAN_STORAGE, "srs_scan_result_quality_update:rssi %d snr %d", rssi, snr));

    if(pScanResultNode != NULL)
    {
        unifi_ScanResult *pScanResult = &pScanResultNode->scanParameters.scanResult;

        if(pScanResult->rssi != rssi || pScanResult->snr  != snr)
        {
            pScanResult->rssi = rssi;
            pScanResult->snr  = snr;
            pScanResult->usability = srs_get_basic_usability(context, rssi, snr);

            /* update the ranking value */
            update_scan_result_rank(context, pScanResultNode);

            (void)rerank_single_entry(context, pScanResultNode);

            sme_trace_debug((TR_SCAN_STORAGE, "srs_scan_result_quality_update: UPDATED"));

            return &pScanResultNode->scanParameters;
        }

        sme_trace_debug((TR_SCAN_STORAGE, "srs_scan_result_quality_update: no change to RSSI or SNR"));
    }
    else
    {
        sme_trace_warn((TR_SCAN_STORAGE, "srs_scan_result_quality_update: failed to update"));
    }

    return NULL;
}

/**
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_expire_old_scan_data(FsmContext* context)
{
    ScanResults* scanData = getScanData(context);
    SmeConfigData* cfg = get_sme_config(context);
    srs_csr_list_scan_result* pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);
    csr_list_cloaked_scan_result* node = csr_list_gethead_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses);
    unifi_ScanConfigData* roamScanData;
    CsrUint16 basicValiditySec = get_current_scan_data(context, &roamScanData)->validitySeconds;
    CsrUint16 roamingValiditySec = 0;
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    if (roamScanData)
    {
        roamingValiditySec = roamScanData->validitySeconds;
    }

    sme_trace_entry((TR_SCAN_STORAGE, "expire_old_scan_data()"));

    /* Do not expire scan results if no validity period */
    if (basicValiditySec == 0 && roamingValiditySec == 0)
    {
        return;
    }

    /* Remove from cloaked list if timed out */
    while (NULL != node)
    {
        csr_list_cloaked_scan_result* thisnode = node;
        node = csr_list_getnext_t(csr_list_cloaked_scan_result *, &scanData->currentCloakedAddresses, &node->node);

        if (node && !node->firmwareOwned && CsrTimeLe(thisnode->lastUpdateTime, CsrTimeSub(now, basicValiditySec * 1000)))
        {
            (void)csr_list_remove(&scanData->currentCloakedAddresses, &thisnode->node);
        }
    }

    /* Check if scanning is paused; if so, do NOT expire any cached scan data */
    while(pScanResultNode != NULL)
    {
        srs_csr_list_scan_result* nextNode;

        CsrUint16 validitySec = basicValiditySec;
        if (roamingValiditySec && srs_ssid_compare(&pScanResultNode->scanParameters.scanResult.ssid, &cfg->connectionInfo.ssid))
        {
            validitySec = roamingValiditySec;
        }

        if (validitySec != 0)
        {
            /* expire any scan results not owned by the firmware */
            if (CsrTimeLe(pScanResultNode->lastUpdateTime, CsrTimeSub(now, validitySec * 1000)))
            {
                if (!pScanResultNode->firmwareOwned)
                {
                    srs_csr_list_scan_result* deleteNode = pScanResultNode;
                    pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode);
                    sme_trace_debug((TR_SCAN_STORAGE, "expire_old_scan_data: expiring BSSID %s SSID %s owned by SME",
                        trace_unifi_MACAddress(deleteNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
                        trace_unifi_SSID(&deleteNode->scanParameters.scanResult.ssid, getSSIDBuffer(context))
                        ));
                    remove_scan_node(context, deleteNode, TRUE, FALSE);
                    continue;
                }
                else
                {
                    sme_trace_debug((TR_SCAN_STORAGE, "expire_old_scan_data: would have expired BSSID %s SSID %s but it is owned by firmware",
                        trace_unifi_MACAddress(pScanResultNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
                        trace_unifi_SSID(&pScanResultNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)) ));
                }
            }
            else
            {
                if (!pScanResultNode->firmwareOwned)
                {
                    sme_trace_debug((TR_SCAN_STORAGE, "expire_old_scan_data: not expiring BSSID %s SSID %s owned by SME: last update time %lums, times out at %lums (validity %ds)",
                        trace_unifi_MACAddress(pScanResultNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
                        trace_unifi_SSID(&pScanResultNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)),
                        pScanResultNode->lastUpdateTime,
                        pScanResultNode->lastUpdateTime + (validitySec * 1000),
                        validitySec));
                }
                else
                {
                    sme_trace_debug((TR_SCAN_STORAGE, "expire_old_scan_data: not expiring BSSID %s SSID %s owned by firmware: last update time %lums, times out at %lums (validity %ds)",
                        trace_unifi_MACAddress(pScanResultNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
                        trace_unifi_SSID(&pScanResultNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)),
                        pScanResultNode->lastUpdateTime,
                        pScanResultNode->lastUpdateTime + (validitySec * 1000),
                        validitySec));
                }
            }
        }

        nextNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode);

        pScanResultNode = nextNode;
    }
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_check_for_expiry(FsmContext* context, const unifi_MACAddress* bssid, CsrUint16 validitySec)
{
    srs_csr_list_scan_result *pScanResultNode = NULL;
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    sme_trace_entry((TR_SCAN_STORAGE, "srs_check_for_expiry(%s)", trace_unifi_MACAddress(*bssid, getMacAddressBuffer(context))));

    /* Do not expire scan results if no validity period */
    if (validitySec == 0)
    {
        return;
    }

    pScanResultNode = findScanResultNode(context, NULL, NULL, bssid);
    if (pScanResultNode != NULL)
    {
        sme_trace_debug((TR_SCAN_STORAGE, "srs_check_for_expiry(%s): owned by %s, last updated %d, expiry at %d in %d ticks",
            trace_unifi_MACAddress(*bssid, getMacAddressBuffer(context)),
            pScanResultNode->firmwareOwned ? "firmware" : "SME",
            pScanResultNode->lastUpdateTime, pScanResultNode->lastUpdateTime + (validitySec * 1000),
            (pScanResultNode->lastUpdateTime + (validitySec * 1000) - now)));

        if ((!pScanResultNode->firmwareOwned) &&
               CsrTimeLe(pScanResultNode->lastUpdateTime, now - (validitySec * 1000)))
        {
            remove_scan_node(context, pScanResultNode, TRUE, FALSE);
        }
    }
    else
    {
        sme_trace_info((TR_SCAN_STORAGE, "srs_check_for_expiry: %s not found in scan result nodes",
                    trace_unifi_MACAddress(*bssid, getMacAddressBuffer(context))));
    }
}


CsrBool validate_candidate_list_change(FsmContext* context,
                            CsrUint8 oldListCount,
                            const unifi_PmkidCandidate *pOldList,
                            CsrUint8 newListCount,
                            const unifi_PmkidCandidate *pNewList)
{

    require(TR_SCAN_STORAGE, context != NULL);

    sme_trace_entry((TR_SCAN_STORAGE, "validate_candidate_list_change"));
    sme_trace_entry((TR_SCAN_STORAGE, "OldList %d  newlist %d", oldListCount, newListCount));

    sme_trace_debug_code({
        CsrUint8 i;
        if(oldListCount !=0)
        {
            const unifi_PmkidCandidate *pmkidCandidate = pOldList;
            sme_trace_entry((TR_SCAN_STORAGE, "old list"));
            for(i=0; i<oldListCount; i++)
            {
                sme_trace_debug((TR_SCAN_STORAGE, "%d: BSSID %s ",i, trace_unifi_MACAddress((pmkidCandidate->bssid), getMacAddressBuffer(context)) ));
                pmkidCandidate++;
            }
        }

        if(newListCount !=0)
        {
            const unifi_PmkidCandidate *pmkidCandidate = pNewList;

            sme_trace_entry((TR_SCAN_STORAGE, "new list"));
            for(i=0; i<newListCount; i++)
            {
                sme_trace_debug((TR_SCAN_STORAGE, "%d: BSSID %s ",i, trace_unifi_MACAddress((pmkidCandidate->bssid), getMacAddressBuffer(context)) ));
                pmkidCandidate++;
            }
        }
    })
    /* simple case, the list has changed size */
    if(oldListCount != newListCount)
    {
        return TRUE;
    }

    /* if the list has not changes and both ar not zero (can not CsrMemCmp) */
    if( ((oldListCount != 0) && (newListCount != 0)) )
    {
        /*
         * this structure has multiple fields and therefore will most likely contain
         * padding.  these fields will not be initialized and therefore we will
         * need to loop and check each entry individually
         *   in other words NO CsrMemCmp on the complete structure
         */
        const unifi_PmkidCandidate *newPmkidCandidate = pNewList;
        const unifi_PmkidCandidate *oldPmkidCandidate = pOldList;
        CsrUint8 i;

        for (i=0; i<newListCount ; i++)
        {
            /* therefore the list size must be the same check the list has not reordered */
            if( (CsrMemCmp (&oldPmkidCandidate->bssid, &newPmkidCandidate->bssid, sizeof(newPmkidCandidate->bssid.data)) != 0)
              ||(oldPmkidCandidate->preAuthAllowed != newPmkidCandidate->preAuthAllowed) )
            {
                /* something did not match */
                return TRUE;
            }
            oldPmkidCandidate++;
            newPmkidCandidate++;
        }
    }

    return FALSE;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_build_pre_auth_candidate_list(FsmContext* context)
{
    ScanResults* scanData = getScanData(context);
    SmeConfigData* cfg = get_sme_config(context);
    srs_csr_list_scan_result* pScanResultNode = NULL;
    void* appHandles;
    CsrUint16 appHandleCount = get_appHandles(context, unifi_IndPmkidCandidateList, &appHandles);

    CsrUint8 pmkidCandidateCount = 0;
    unifi_PmkidCandidate* pmkidCandidates = NULL;

    sme_trace_entry((TR_SCAN_STORAGE, "srs_build_pre_auth_candidate_list >>"));

    /* make sure that the list is required. */
    if(appHandleCount == 0)
    {
        return;
    }

    /*         Condition 1 */
    /* only deliver a candidate list for the ssid currently connected */
    if (ns_get_connection_status(context) != ns_ConnectionStatus_Connected)
    {
        sme_trace_debug((TR_SCAN_STORAGE, "srs_build_pre_auth_candidate_list: No Connections" ));
        return;
    }

    /*         Condition 2
     * WPA2 or WAPI keys must have been installed */
    if( ((cfg->connectionInfo.authMode & unifi_8021xAuthWPA2PSK) != unifi_8021xAuthWPA2PSK)
      &&((cfg->connectionInfo.authMode & unifi_8021xAuthWPA2) != unifi_8021xAuthWPA2) &&
      ((cfg->connectionInfo.authMode & unifi_WAPIAuthWAIPSK) != unifi_WAPIAuthWAIPSK)
      &&((cfg->connectionInfo.authMode & unifi_WAPIAuthWAI) != unifi_WAPIAuthWAI))
    {
        sme_trace_debug((TR_SCAN_STORAGE, "srs_build_pre_auth_candidate_list: AuthMode incorrect 0x%.2x", cfg->connectionInfo.authMode));
        return;
    }

    pmkidCandidates = (unifi_PmkidCandidate*)CsrPmalloc(sizeof(unifi_PmkidCandidate) * UNIFI_PMKID_LIST_MAX);

    /* scan through tthe list */
    for (pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList); pScanResultNode != NULL;
         pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode))
    {
        sme_trace_debug((TR_SCAN_STORAGE, "srs_build_pre_auth_candidate_list: : Checking BSSID %s SSID %s",
            trace_unifi_MACAddress(pScanResultNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
            trace_unifi_SSID(&pScanResultNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)) ));

        /* the AP must be WPA2/WPA2PSK WAI/WAIPSK Capable */
        if( ((pScanResultNode->scanParameters.securityData.authMode_CapabilityFlags & unifi_8021xAuthWPA2PSK) == unifi_8021xAuthWPA2PSK)
          ||((pScanResultNode->scanParameters.securityData.authMode_CapabilityFlags & unifi_8021xAuthWPA2) == unifi_8021xAuthWPA2) ||
          ((pScanResultNode->scanParameters.securityData.authMode_CapabilityFlags & unifi_WAPIAuthWAIPSK) == unifi_WAPIAuthWAIPSK)
          ||((pScanResultNode->scanParameters.securityData.authMode_CapabilityFlags & unifi_WAPIAuthWAI) == unifi_WAPIAuthWAI))
        {
            /* only add it to the list if it is the same SSID */
            if(   (srs_ssid_compare(&pScanResultNode->scanParameters.scanResult.ssid, &cfg->connectionInfo.ssid))
                &&(CsrMemCmp  (&pScanResultNode->scanParameters.scanResult.bssid, &cfg->connectionInfo.bssid, sizeof(cfg->connectionInfo.bssid.data)) != 0) )
            {
                sme_trace_debug((TR_SCAN_STORAGE, "srs_build_pre_auth_candidate_list: : matched SSID BSSID %s SSID %s",
                    trace_unifi_MACAddress(pScanResultNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
                    trace_unifi_SSID(&pScanResultNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)) ));

                pmkidCandidates[pmkidCandidateCount].bssid = pScanResultNode->scanParameters.scanResult.bssid;
                pmkidCandidates[pmkidCandidateCount].preAuthAllowed = ((pScanResultNode->scanParameters.ieCapabilityFlags & PRE_AUTH_Capable) == 0 ? FALSE:TRUE);
                pmkidCandidateCount++;

                /* only allow a preauthlist size of */
                if (pmkidCandidateCount == UNIFI_PMKID_LIST_MAX)
                {
                    sme_trace_debug((TR_SCAN_STORAGE, "srs_build_pre_auth_candidate_list: : AP list count exceeds UNIFI_PMKID_LIST_MAX, quiting" ));
                    break;
                }
            }
        }
    }

    /* make the decision if we should send up a unifi_mgt_pmkid_candidate_list_ind */
    if (validate_candidate_list_change(context, scanData->pmkidCandidatesCount,
                                                scanData->pmkidCandidates,
                                                pmkidCandidateCount,
                                                pmkidCandidates) )
    {
        appHandleCount = get_appHandles(context, unifi_IndPmkidCandidateList, &appHandles);
        sme_trace_debug((TR_SCAN_STORAGE, "srs_build_pre_auth_candidate_list: : valid list" ));

        /* Free the old list */
        if (scanData->pmkidCandidates)
        {
            CsrPfree(scanData->pmkidCandidates);
            scanData->pmkidCandidates = NULL;
        }
        /* update current list */
        scanData->pmkidCandidatesCount = pmkidCandidateCount;
        scanData->pmkidCandidates = pmkidCandidates;
        if (appHandleCount)
        {
            call_unifi_mgt_pmkid_candidate_list_ind(context,
                                                    appHandleCount,
                                                    appHandles,
                                                    pmkidCandidateCount,
                                                    pmkidCandidates);
        }
    }
    else
    {
        sme_trace_debug((TR_SCAN_STORAGE, "srs_build_pre_auth_candidate_list: : invalid list" ));
        CsrPfree(pmkidCandidates);
    }

}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
CsrUint16 srs_scanresults_count(FsmContext* context)
{
    return getScanData(context)->listCount;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
CsrUint16 srs_copy_scanresults_bytes_required(FsmContext* context)
{
    ScanResults* scanData = getScanData(context);
    CsrUint16 result = 0;

    sme_trace_entry((TR_SCAN_STORAGE, "srs_copy_scanresults_bytes_required()"));
#if 0
    srs_csr_list_scan_result* pScanResultNode = NULL;
    for (pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList); pScanResultNode != NULL;
         pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode))
    {
        unifi_ScanResult* scan = &pScanResultNode->scanParameters.scanResult;
        result += sizeof(unifi_ScanResult) + scan->informationElements.length + 4; /* + 3 for potential padding */
    }
#else
    result = sizeof(unifi_ScanResult) * scanData->listCount;
#endif

    sme_trace_debug((TR_SCAN_STORAGE, "srs_copy_scanresults() == %d bytes", result));
    return result;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
unifi_Status srs_copy_scanresults(FsmContext* context, CsrUint16* numElements, unifi_ScanResult* results, CsrUint16 buffsize, CsrBool fullCopy)
{
    ScanResults* scanData = getScanData(context);
    srs_csr_list_scan_result* pScanResultNode = NULL;
    CsrUint16 infoElementBytes = 0;
    sme_trace_entry((TR_SCAN_STORAGE, "srs_copy_scanresults(%d : FullCopy: %s)", buffsize, fullCopy?"TRUE":"FALSE"));

    *numElements = 0;

    for (pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList); pScanResultNode != NULL;
         pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode))
    {
        unifi_ScanResult* result = &pScanResultNode->scanParameters.scanResult;

        sme_trace_debug((TR_SCAN_STORAGE, "srs_copy_scanresults: : SSID %s", trace_unifi_SSID(&result->ssid, getSSIDBuffer(context))));
        /* Make sure there is enough room in the buffer for more Data */
        if ((*numElements * sizeof(unifi_ScanResult)) + infoElementBytes > buffsize)
        {
            sme_trace_error((TR_SCAN_STORAGE, "srs_copy_scanresults() unifi_NoRoom :: %d Elements written", *numElements));
            break;
        }

        results[*numElements] = *result;
        (*numElements)++;
        if (fullCopy)
        {
            infoElementBytes += result->informationElementsLength;
        }
    }

    if (fullCopy)
    {
        /* Copy the Info Elements into the buffer AND update the scan result pointer in the buffer */
        CsrUint8* current_buff = (CsrUint8*)&results[*numElements];
        CsrUint16 i;
        for (i = 0; i < *numElements; ++i)
        {
            unifi_ScanResult* result = &results[i];
            CsrMemCpy(current_buff, result->informationElements, result->informationElementsLength);
            result->informationElements = current_buff;
            current_buff += result->informationElementsLength;
        }
    }

    /* memcheck reports uninitialised memory due to padding in the unifi_ScanResult struct*/
    /*
    sme_trace_hex((TR_SCAN_STORAGE, TR_LVL_DEBUG, "srs_copy_scanresults(Full Buffer)",
                                                  results,
                                                  (*numElements * sizeof(unifi_ScanResult)) + infoElementBytes));
    */
    return unifi_Success;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
void srs_crop_scan_result_list(
                    FsmContext* const context,
                    const unifi_SSID *preJoinSSID,
                    const srs_scan_data *pCurrentScanResult,
                    CsrBool * const pResultDeleted)
{
    SmeConfigData* cfg = get_sme_config(context);
    ScanResults* scanData = getScanData(context);

    sme_trace_debug((TR_SCAN_STORAGE, "crop_scan_result_list: : preJoinSSID %s", trace_unifi_SSID(preJoinSSID, getSSIDBuffer(context)) ));

    *pResultDeleted = FALSE;

    /* scan to the max capacity and then remove from there.
     * we must not remove the current join scanresult
     *
     * This will have the effect of possibly creating a list of Max +1
     */
    if ((cfg->scanConfig.maxResults > 0) && (scanData->listCount > cfg->scanConfig.maxResults))
    {
        /* force an expiry of old data */
        srs_expire_old_scan_data(context);

        /* expiry might have reduced the list */
        if (scanData->listCount > cfg->scanConfig.maxResults)
        {
            CsrUint8 tailCount;
            srs_csr_list_scan_result* pScanResultNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);

            /* move to the first item to be removed. */
            for(tailCount = 0; tailCount < cfg->scanConfig.maxResults; tailCount++)
            {
                pScanResultNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode);
            }


            /* crop the list to its maximum running size */
            while(pScanResultNode != NULL)
            {
                srs_csr_list_scan_result *pNextNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pScanResultNode->rankedNode);

                     /* keep the join scan result */
                if( (&(pScanResultNode->scanParameters) != scanData->joinScanResult)
                  &&(pScanResultNode->scanParameters.scanResult.ssid.length != preJoinSSID->length)
                  &&(CsrMemCmp(&pScanResultNode->scanParameters.scanResult.ssid, preJoinSSID, preJoinSSID->length) == 0) )
                {
                    sme_trace_debug((TR_SCAN_STORAGE, "crop_scan_result_list: : AP list count %d exceeds %d, throwing away existing BSSID %s SSID %s",
                        scanData->listCount,
                        cfg->scanConfig.maxResults,
                        trace_unifi_MACAddress(pScanResultNode->scanParameters.scanResult.bssid, getMacAddressBuffer(context)),
                        trace_unifi_SSID(&pScanResultNode->scanParameters.scanResult.ssid, getSSIDBuffer(context)) ));

                    if( &(pScanResultNode->scanParameters) == pCurrentScanResult)
                    {
                        *pResultDeleted = TRUE;
                        remove_scan_node(context, pScanResultNode, FALSE, FALSE);
                    }
                    else
                    {
                        remove_scan_node(context, pScanResultNode, TRUE, FALSE);
                    }
                }
                pScanResultNode = pNextNode;
            }
        }
    }
}

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
 * @ingroup scan
 *
 * @param[in]  networkType80211    : Band in which the resulting channel number is required
 *
 * @return
 *      Channel Number
 */
/*---------------------------------------------------------------------------*/
CsrUint8 srs_get_free_channel(FsmContext* context, unifi_80211NetworkType networkType80211)
{
    /* find the previous node */
    ScanResults*             scanData = getScanData(context);
    srs_csr_list_scan_result *pCurrentNode = NULL;
    CsrUint8                    apPresence[HIGHEST_80211_b_g_CHANNEL_NUM + 1];
    CsrUint8                    selectedChannel = 0, channelIndex = 0, leastNumberOfDevices = 255;

    sme_trace_entry((TR_SCAN_STORAGE, "srs_get_most_free_channel()"));

    CsrMemSet(apPresence, 0x00, sizeof(apPresence));

    pCurrentNode = csr_list_gethead_t(srs_csr_list_scan_result *, &scanData->rankedList);

    while (pCurrentNode != NULL)
    {
        sme_trace_info((TR_SCAN_STORAGE, "channel number %d, RSSI %d\n",
                pCurrentNode->scanParameters.scanResult.channelNumber,
                pCurrentNode->scanParameters.scanResult.rssi));

        /* Increment the apPresence counter for this particular scan result if the signal strength
         * seems strong enough to affect us */
        if (pCurrentNode->scanParameters.scanResult.rssi > RSSI_THRESHOLD_POOR)
        {
            apPresence[pCurrentNode->scanParameters.scanResult.channelNumber]++;
        }

        /* Get the next scan result */
        pCurrentNode = csr_list_getnext_t(srs_csr_list_scan_result *, &scanData->rankedList, &pCurrentNode->rankedNode);
    }

    /* Now get the least used channel
     * TODO: Currently a very simple algorithm is used, can be enhanced */
    do
    {
        /* First try the preferred channels 1, 6 and 11 */
        if ((apPresence[1] == 0) && (regdom_is_channel_actively_usable(context, 1) == TRUE))
        {
            selectedChannel = 1;
            break;
        }

        if ((apPresence[6] == 0) && (regdom_is_channel_actively_usable(context, 6) == TRUE))
        {
            selectedChannel = 6;
            break;
        }

        if ((apPresence[11] == 0) && (regdom_is_channel_actively_usable(context, 11) == TRUE))
        {
            selectedChannel = 11;
            break;
        }

        /* Pick any channel where there are no devices or least number of devices */
        while (channelIndex < HIGHEST_80211_b_g_CHANNEL_NUM)
        {
            channelIndex++;
            if (regdom_is_channel_actively_usable(context, channelIndex) == FALSE)
            {
                continue;
            }

            sme_trace_info((TR_SCAN_STORAGE, "Considering channel number %d\n",channelIndex));

            if (apPresence[channelIndex] == 0)
            {
                selectedChannel = channelIndex;
                break;
            }

            if (apPresence[channelIndex] < leastNumberOfDevices)
            {
                selectedChannel = channelIndex;
                leastNumberOfDevices = apPresence[channelIndex];
            }
        }
    } while (0);

    sme_trace_info((TR_SCAN_STORAGE, "selected channel number %d\n",channelIndex));

    return selectedChannel;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
CsrBool srs_scan_result_contains_ssid(
        FsmContext* context,
        srs_scan_data* pScanData,
        const unifi_SSID *pSSID)
{
    /* first check the primary SSID */
    if (srs_ssid_compare(&pScanData->scanResult.ssid, pSSID))
    {
        return TRUE;
    }

 #ifdef CCX_VARIANT
    {
        /* did not find the ssid look in the SSIDL IE */
        CsrUint8  *ieBuf = NULL;
        CsrUint16 ieLength;

        pld_access(getPldContext(context), pScanData->informationElements.slotNumber, (void*)&ieBuf, &ieLength);

        if (ccx_ie_ssidl_get_ssid(context, ieBuf, ieLength, pSSID)!= NULL)
        {
            return TRUE;
        }
    }
#endif

    return FALSE;
}

/*
 * Description:
 * See description in scan_manager_fsm/scan_results_storage.h
 */
/*---------------------------------------------------------------------------*/
CsrBool srs_get_security_data(
        FsmContext* context,
        srs_scan_data* pScanData,
        const unifi_MACAddress *pBSSID,
        const unifi_SSID *pSSID,
        srs_security_data *securityData)
{
    sme_trace_entry((TR_SCAN_STORAGE, "srs_get_security_data\n"));

    /*
    sme_trace_debug((TR_SCAN_STORAGE, "ssid to match(%s)",
                     trace_unifi_SSID(pSSID, getSSIDBuffer(context)) ));
    sme_trace_debug((TR_SCAN_STORAGE, "scanResult supplied (%s)",
                     trace_unifi_SSID(&pScanData->scanResult.ssid, getSSIDBuffer(context)) ));
     */

    /* first check the primary SSID */
    if ( (pSSID == NULL)
       ||(is_ssid_cloaked(pSSID))
       ||(srs_ssid_compare(&pScanData->scanResult.ssid, pSSID)) )
    {
        *securityData = pScanData->securityData;

        sme_trace_info((TR_SCAN_STORAGE, "pSSID = NULL or cloaked" ));

        sme_trace_debug((TR_SCAN_STORAGE, "securityData() authMode    : 0x%.4X ", securityData->authMode_CapabilityFlags ));
        sme_trace_debug((TR_SCAN_STORAGE, "securityData() WPACiphers  : 0x%.4X ", securityData->wpaCipherCapabilityFlags ));
        sme_trace_debug((TR_SCAN_STORAGE, "securityData() WPA2Ciphers : 0x%.4X ", securityData->wpa2CipherCapabilityFlags ));
        sme_trace_debug((TR_SCAN_STORAGE, "securityData() WAPICiphers : 0x%.4X ", securityData->wapiCipherCapabilityFlags ));
        sme_trace_debug((TR_SCAN_STORAGE, "securityData() privacy     : %s ", trace_unifi_80211PrivacyMode(securityData->privacyMode) ));

        return TRUE;
    }

    /* secondary, check for a SSIDL */
#ifdef CCX_VARIANT
    {
        /* did not find the ssid look in the SSIDL IE */
        CsrUint8  *ieBuf = NULL;
        CsrUint16 ieLength;
        ccx_ssidl_hidden_ssid* HiddenSsid;

        pld_access(getPldContext(context), pScanData->informationElements.slotNumber, (void*)&ieBuf, &ieLength);

        HiddenSsid = ccx_ie_ssidl_get_ssid(context, ieBuf, ieLength, pSSID);
        if (HiddenSsid != NULL)
        {
            ccx_ie_convert_security_data(context, HiddenSsid, securityData);
            return TRUE;
        }
    }
#endif

    return FALSE;
}
