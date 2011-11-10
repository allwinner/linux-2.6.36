/** @file ap_validation.c
 *
 * Public AP Validation Implementation
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
 *   This file implements the AP Validation functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ap_utils/ap_validation.c#6 $
 *
 ****************************************************************************/

/** @{
 * @ingroup sme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "ap_utils/ap_validation.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "smeio/smeio_trace_types.h"
#include "ie_access/ie_access.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/*
 * Description:
 * See description in network_selector_fsm/ap_validation.h
 */
/*---------------------------------------------------------------------------*/
CsrBool validate_ap(FsmContext* context, srs_scan_data *pScanData, SmeConfigData *cfg)
{
    CsrBool ret = TRUE;
    srs_security_data securityData;

    sme_trace_debug_code(
        char bssidBuffer[20];
        char ssidBuffer[33];
    );


    sme_trace_entry((TR_NETWORK_SELECTOR_FSM, "validate_ap()"));
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() (value)     : (scan) : (cfg)"));
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() ssid        : %s : %s", trace_unifi_SSID(&pScanData->scanResult.ssid, getSSIDBuffer(context)), trace_unifi_SSID(&cfg->connectionConfig.ssid, ssidBuffer)));
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() bssid       : %s : %s", trace_unifi_MACAddress(pScanData->scanResult.bssid, getMacAddressBuffer(context)), trace_unifi_MACAddress(cfg->connectionConfig.bssid, bssidBuffer)));
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() ifIndex     : %s : %s", trace_unifi_RadioIF(pScanData->scanResult.ifIndex),  trace_unifi_RadioIF(cfg->connectionConfig.ifIndex)));
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() bsstype     : %s : %s", trace_unifi_BSSType(pScanData->scanResult.bssType),  trace_unifi_BSSType(cfg->connectionConfig.bssType)));

    if (cfg->connectionConfig.ifIndex != unifi_GHZ_Both && cfg->connectionConfig.ifIndex != pScanData->scanResult.ifIndex)
    {
        sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed ifindex check"));
        return FALSE;
    }

    if (CsrMemCmp(&cfg->connectionConfig.bssid, &BroadcastBssid, sizeof(cfg->connectionConfig.bssid.data)) != 0 &&
        CsrMemCmp(&cfg->connectionConfig.bssid, &pScanData->scanResult.bssid, sizeof(cfg->connectionConfig.bssid.data)) != 0 )
    {
        sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() %s failed bssid check", trace_unifi_MACAddress(pScanData->scanResult.bssid, getMacAddressBuffer(context))));
        return FALSE;
    }

    if (black_list_is_blacklisted(context, &pScanData->scanResult.bssid))
    {
        sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() %s failed blacklist", trace_unifi_MACAddress(pScanData->scanResult.bssid, getMacAddressBuffer(context))));
        return FALSE;
    }

    if(pScanData->scanResult.bssType != cfg->connectionConfig.bssType)
    {
        sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed infrastructureMode"));
        return FALSE;
    }

    /* *********************************************************************
            Process Security Parameters
       ***********************************************************************/
    if(srs_get_security_data(context, pScanData, &cfg->connectionConfig.bssid, &cfg->connectionConfig.ssid, &securityData) == FALSE)
    {
        /* failed to find matching security parameters */
        return FALSE;
    }

    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() authMode    : 0x%.4X : 0x%.4X", securityData.authMode_CapabilityFlags,  cfg->connectionConfig.authModeMask));
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() WPACiphers  : 0x%.4X : 0x%.4X", securityData.wpaCipherCapabilityFlags,  cfg->connectionConfig.encryptionModeMask));
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() WPA2Ciphers : 0x%.4X : 0x%.4X", securityData.wpa2CipherCapabilityFlags,  cfg->connectionConfig.encryptionModeMask));
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() WAPICiphers : 0x%.4X : 0x%.4X", securityData.wapiCipherCapabilityFlags,  cfg->connectionConfig.encryptionModeMask));
    sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() privacy     : %s : %s", trace_unifi_80211PrivacyMode(securityData.privacyMode),  trace_unifi_80211PrivacyMode(cfg->connectionConfig.privacyMode)));

    if(cfg->connectionConfig.privacyMode != securityData.privacyMode)
    {
        sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed Privacy Check"));
        return FALSE;
    }

    /* check if the authmode is set to none or does not match the configuration*/
    /* The AP could be in automatic (both open and shared set) rather than explicitly
     * stating open or shared so, have to allow for this case by not just doing
     * a straight comparison and assuming that they msut match exactly.
     */
    /* The != check handles the case where both values are 0 */
    if( cfg->connectionConfig.authModeMask != (cfg->connectionConfig.authModeMask & securityData.authMode_CapabilityFlags) &&
       !(cfg->connectionConfig.authModeMask & securityData.authMode_CapabilityFlags) &&
       !(cfg->connectionConfig.authModeMask & unifi_8021xAuthOther1x && securityData.authMode_CapabilityFlags & unifi_80211AuthOpen))
    {
        sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed authMode_8021x_CapabilityFlags cfg[0x%x] ap[0x%x] ",cfg->connectionConfig.authModeMask, securityData.authMode_CapabilityFlags));
        return FALSE;
    }

#if 0

    /* The configuration must support either WPA or WPA2 */
    if( (cfg->authMode & unifi_8021xAuthWPA)
      ||(cfg->authMode & unifi_8021xAuthWPAPSK)
      ||(cfg->authMode & unifi_8021xAuthWPA2)
      ||(cfg->authMode & unifi_8021xAuthWPA2PSK))
    {
        if (pScanData->probeScanParams.informationElements.dataLength != 0)
        {
            void *buf = NULL;
            CsrUint16 length;

            pld_access((PldHdl)pScanData->probeScanParams.informationElements.slotNumber, &buf, &length);

            if (FALSE == ie_rsn_validate_ies(buf, length, cfg))
            {
                /* failed to match the AP */
            return FALSE;
        }
    }
    }
#endif

    /* check if encryption status is configured correctly it must support
     * Need to check for WAPI, WPA and WPA2 support */
    if((securityData.authMode_CapabilityFlags & (unifi_8021xAuthWPA  | unifi_8021xAuthWPAPSK |
                                               unifi_8021xAuthWPA2 | unifi_8021xAuthWPA2PSK |
                                               unifi_WAPIAuthWAI   | unifi_WAPIAuthWAIPSK )))
    {
        CsrUint16 groupCiphers = 0;
        CsrUint16 pairwiseCiphers = 0;

        ret = FALSE;

        if((cfg->connectionConfig.authModeMask & (unifi_8021xAuthWPA | unifi_8021xAuthWPAPSK)))
        {
            groupCiphers = securityData.wpaCipherCapabilityFlags & GROUP_CIPHER_MASK;
            pairwiseCiphers = securityData.wpaCipherCapabilityFlags & PAIRWISE_CIPHER_MASK;
            if ((cfg->connectionConfig.encryptionModeMask & groupCiphers) && (cfg->connectionConfig.encryptionModeMask & pairwiseCiphers))
            {
                ret = TRUE;
            }
            else
            {
                sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed WPA encryptionMode [0x%x] [0x%x] ",cfg->connectionConfig.encryptionModeMask, securityData.wpaCipherCapabilityFlags));
            }
        }

        if((cfg->connectionConfig.authModeMask & (unifi_8021xAuthWPA2 | unifi_8021xAuthWPA2PSK)))
        {
            groupCiphers = securityData.wpa2CipherCapabilityFlags & GROUP_CIPHER_MASK;
            pairwiseCiphers = securityData.wpa2CipherCapabilityFlags & PAIRWISE_CIPHER_MASK;
            if ((cfg->connectionConfig.encryptionModeMask & groupCiphers) && (cfg->connectionConfig.encryptionModeMask & pairwiseCiphers))
            {
                ret = TRUE;
            }
            else
            {
                sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed WPS2 encryptionMode [0x%x] [0x%x] ",cfg->connectionConfig.encryptionModeMask, securityData.wpa2CipherCapabilityFlags));
            }
        }

        if(cfg->wapiOptionImplemented &&
                (cfg->connectionConfig.authModeMask & (unifi_WAPIAuthWAI | unifi_WAPIAuthWAIPSK)))
        {
            groupCiphers = securityData.wapiCipherCapabilityFlags & GROUP_CIPHER_MASK;
            pairwiseCiphers = securityData.wapiCipherCapabilityFlags & PAIRWISE_CIPHER_MASK;
            if ((cfg->connectionConfig.encryptionModeMask & groupCiphers) && (cfg->connectionConfig.encryptionModeMask & pairwiseCiphers))
            {
                ret = TRUE;
            }
            else
            {
                sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed WAPI encryptionMode [0x%x] [0x%x] ",cfg->connectionConfig.encryptionModeMask, securityData.wapiCipherCapabilityFlags));
            }
        }

        sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap: strict-draft-n mode = %s, high-throughput-option = %s %s",
                        cfg->smeConfig.enableStrictDraftN?"ENABLED":"DISABLED",
                        cfg->highThroughputOptionEnabled?"ENABLED":"DISABLED",
                        (cfg->smeConfig.enableStrictDraftN && cfg->highThroughputOptionEnabled)?"(extra checks required)":""));

        if ( (cfg->smeConfig.enableStrictDraftN)
           &&(cfg->highThroughputOptionEnabled)
           &&(ret == TRUE))
        {
            /* Check the draft N capabilities */
            sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap: checking APs IE capabilities (0x%x) for N-capability (0x%x). RESULT=%s", 
                            pScanData->ieCapabilityFlags, DOT11N_Capable,
                            ((pScanData->ieCapabilityFlags & DOT11N_Capable)==DOT11N_Capable)?"YES":"NO"));

            if ((pScanData->ieCapabilityFlags & DOT11N_Capable) == DOT11N_Capable )
            {
                sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap: checking AP ciphers are not restricted to TKIP. AP supports 0x%x, TKIP is 0x%x. RESULT=%s",
                                pairwiseCiphers, unifi_EncryptionCipherPairwiseTkip,
                                ((pairwiseCiphers & (~unifi_EncryptionCipherPairwiseTkip)) !=  0)?"OK":"ERROR, ONLY TKIP SUPPORTED"));

                /* check the pairwise cipher is not TKIP only */
                if((pairwiseCiphers & (~unifi_EncryptionCipherPairwiseTkip)) ==  0)
                {
                    sme_trace_info((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed: Strict Draft N enabled - DraftN AP beaconing TKIP only"));
                    ret = FALSE;
                }

                /* check the SMEs configuration allows more that just TKIP */
                sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap: checking STA ciphers are not restricted to TKIP. STA supports 0x%x, TKIP is 0x%x. RESULT=%s",
                                cfg->connectionConfig.encryptionModeMask, unifi_EncryptionCipherPairwiseTkip,
                                ((cfg->connectionConfig.encryptionModeMask & (~unifi_EncryptionCipherPairwiseTkip)) !=  0)?"OK":"ERROR, ONLY TKIP SUPPORTED"));

                if(((cfg->connectionConfig.encryptionModeMask & PAIRWISE_CIPHER_MASK )& (~unifi_EncryptionCipherPairwiseTkip)) ==  0)
                {
                    sme_trace_info((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed: Strict Draft N enabled - SME configured for TKIP only "));
                    ret = FALSE;
                }
            }
        }
    }


    sme_trace_debug_code(
        if (ret)
        {
            sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() PASSED ALL CHECKS"));
        }
        else
        {
            sme_trace_debug((TR_NETWORK_SELECTOR_FSM, "validate_ap() failed"));
        }
    )
    /* we have passed all of the tests */
    return ret;
}

/** @}
 */
