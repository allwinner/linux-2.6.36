/** @file ie_access_rsn.c
 *
 * Utilities to process RSN information elements
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
 *   Utilities to process RSN information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_rsn.c#3 $
 *
 ****************************************************************************/


#include "ie_access/ie_access.h"
#include "ie_access/ie_access_rsn.h"
/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "abstractions/osa.h"
#include "fsm/csr_wifi_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "smeio/smeio_trace_types.h"

#include "ie_access/ie_access.h"

#include "security_manager_fsm/security_8021x.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "scan_manager_fsm/scan_manager_fsm_types.h"

/* MACROS *******************************************************************/
/** IE ID FIELD SIZE */
#define IE_ID_FIELD_SIZE    0x02
/** IE OUI FIELD SIZE */
#define IE_OUI_FIELD_SIZE   0x04

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/
static ie_result process_rsn_element(
                    ie_rsn_data* rsnData,
                    unifi_AuthMode *authMode8021x,
                    CsrUint8 *rsnElementCurrent,
                    CsrBool allowUnicastUseGroupCipher );

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Updates the cipher capability flag
 *
 * @par Description
 *   Updates the cipher capability flag
 *
 * @param[in] rsnData : the RSN data block
 * @param[in] cipherSuite : cipher suite to be analysised
 * @param[in] keyType : key type.
 *
 * @return
 *   void
 */
static void update_cipher_type(
                    ie_rsn_data* rsnData,
                    CsrUint32 cipherSuite,
                    unifi_KeyType keyType )
{
    if (rsnData->rsntype == IE_RSN_TYPE_WAPI)
    {
        if (keyType == unifi_GroupKey)
        {
            if ((cipherSuite & 255) == IE_WAPI_CIPHER_SMS4_MASK)
            {
                sme_trace_info((TR_IE_ACCESS, "Group SMS4 Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherGroupSms4;
            }
        }
        else if (keyType == unifi_PairwiseKey)
        {
            if ((cipherSuite & 255) == IE_WAPI_CIPHER_SMS4_MASK)
            {
                sme_trace_info((TR_IE_ACCESS, "Pairwise SMS4 Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherPairwiseSms4;
            }
        }
        else
        {
            sme_trace_error((TR_IE_ACCESS, "Key type Unknown"));
            verify(TR_IE_ACCESS, 0); /*lint !e774*/
        }
    }
    else
    {
        if (keyType == unifi_GroupKey)
        {
            /* calculate the group cipher suite */
            switch(cipherSuite & 255)
            {
            case IE_RSN_CIPHER_WEP40:
                sme_trace_info((TR_IE_ACCESS, "Group WEP 40 Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherGroupWep40;
                break;
            case IE_RSN_CIPHER_WEP104:
                sme_trace_info((TR_IE_ACCESS, "Group WEP 104 Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherGroupWep104;
                break;
            case IE_RSN_CIPHER_TKIP:
                sme_trace_info((TR_IE_ACCESS, "Group TKIP Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherGroupTkip;
                break;
            case IE_RSN_CIPHER_CCMP:
                sme_trace_info((TR_IE_ACCESS, "Group AES Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherGroupCcmp;
                break;
            case IE_RSN_CIPHER_VENDOR:
            default:
                sme_trace_error((TR_IE_ACCESS, "No Group Cipher [0x%x] [0x%x]", cipherSuite, (cipherSuite & 255)));
                /* do nothing */
                break;
            }
        }
        else if (keyType == unifi_PairwiseKey)
        {
            switch(cipherSuite & 255)
            {
            case IE_RSN_CIPHER_WEP40:
                sme_trace_info((TR_IE_ACCESS, "Pairwise WEP 40 Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherPairwiseWep40;
                break;
            case IE_RSN_CIPHER_WEP104:
                sme_trace_info((TR_IE_ACCESS, "Pairwise WEP 104 Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherPairwiseWep104;
                break;
            case IE_RSN_CIPHER_TKIP:
                sme_trace_info((TR_IE_ACCESS, "Pairwise TKIP Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherPairwiseTkip;
                break;
            case IE_RSN_CIPHER_CCMP:
                sme_trace_info((TR_IE_ACCESS, "Pairwise AES Cipher Capable"));
                rsnData->cipherCapabilityFlags |= unifi_EncryptionCipherPairwiseCcmp;
                break;
            case IE_RSN_CIPHER_VENDOR:
            default:
                sme_trace_error((TR_IE_ACCESS, "No Pairwise Cipher %x", cipherSuite));
                /* do nothing */
                break;
            }
        }
        else
        {
            sme_trace_error((TR_IE_ACCESS, "Key type Unknown"));
            verify(TR_IE_ACCESS, 0); /*lint !e774*/
        }
    }
}

/**
 * @brief
 *   Updates the suite capability flag
 *
 * @par Description
 *   Updates the suite capability flag
 *
 * @param[in] rsnData : the RSN data block
 * @param[out] authMode8021x : authentication capability flag.
 * @param[in] rsnElement : RSN IE buffer.
 *
 * @return
 *   void
 */
static void update_akm_suite_type(
                    ie_rsn_data* rsnData,
                    unifi_AuthMode *authMode8021x,
                    CsrUint8 *rsnElement )
{
    CsrUint32 akmSuite;

    ie_be_CsrUint32_noshift(rsnElement, akmSuite);

    if (rsnData->rsntype == IE_RSN_TYPE_WPA)
    {
        if (akmSuite == IE_WPA_AKM_PSK)
        {
            *authMode8021x |= unifi_8021xAuthWPAPSK;
        }
        else if (akmSuite == IE_WPA_AKM_PSKSA)
        {
            *authMode8021x |= unifi_8021xAuthWPA;
        }
        else
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::WPA akmSuite Unknown %d", akmSuite));
        }
    }
    else if (rsnData->rsntype == IE_RSN_TYPE_WPA2)
    {
        if (akmSuite == IE_WPA2_AKM_PSK)
        {
            *authMode8021x |= unifi_8021xAuthWPA2PSK;
        }
        else if (akmSuite == IE_WPA2_AKM_PSKSA)
        {
            *authMode8021x |= unifi_8021xAuthWPA2;
        }
        else if ( (akmSuite == IE_WPA2_AKM_PSKSA_FT)
                ||(akmSuite == IE_WPA2_AKM_PSK_FT)
                ||(akmSuite == IE_WPA2_AKM_PSKSA_SHA256)
                ||(akmSuite == IE_WPA2_AKM_PSK_SHA256))
        {
            sme_trace_warn((TR_IE_ACCESS, "ie_access::WPA2 akmSuite unsupported %d", akmSuite));
        }
        else
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::WPA2 akmSuite Unknown %d", akmSuite));
        }
    }
    else if (rsnData->rsntype == IE_RSN_TYPE_WAPI)
    {
        if (akmSuite == IE_WAPI_AKM_PSK)
        {
            *authMode8021x |= unifi_WAPIAuthWAIPSK;
        }
        else if (akmSuite == IE_WAPI_AKM_PSKSA)
        {
            *authMode8021x |= unifi_WAPIAuthWAI;
        }
        else
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::WAPI akmSuite Unknown"));
        }
    }
    else
    {
        sme_trace_error((TR_IE_ACCESS, "rsntype Unknown"));
        verify(TR_IE_ACCESS, 0);    /*lint !e774*/
    }
}

/**
 * @brief
 *   process an RSN IE
 *
 * @par Description
 *   process an RSN IE
 *
 * @param[in] rsnData : the RSN data block
 * @param[out] authMode8021x : authentication capability flag.
 * @param[in] rsnElement : RSN IE buffer.
 *
 * @return
 *        ie_success   : element found
 *        ie_invalid   : IE field length invalid
 *        ie_not_found : IE element not found
 */
static ie_result process_rsn_element(
                    ie_rsn_data* rsnData,
                    unifi_AuthMode *authMode8021x,
                    CsrUint8 *rsnElement,
                    CsrBool allowUnicastUseGroupCipher )
{
    int i = 0;
    CsrUint8* rsnElementCurrent;

    rsnData->cipherCapabilityFlags = unifi_EncryptionCipherNone;
    rsnData->version = 0;
    rsnData->groupCipherSuite = 0;
    rsnData->pairwiseCipherSuiteCount = 0;
    rsnData->akmSuiteCount = 0;
    rsnData->rsmCapabilities = 0;

    if(rsnData->rsntype == IE_RSN_TYPE_WPA)
    {
        sme_trace_debug((TR_IE_ACCESS, "ie_access::rsnData->rsntype == IE_RSN_TYPE_WPA"));
        rsnElementCurrent = rsnElement + IE_ID_FIELD_SIZE + IE_OUI_FIELD_SIZE;
    }
    else if(rsnData->rsntype == IE_RSN_TYPE_WPA2)
    {
        sme_trace_debug((TR_IE_ACCESS, "ie_access::rsnData->rsntype == IE_RSN_TYPE_WPA2"));
        rsnElementCurrent = rsnElement + IE_ID_FIELD_SIZE;
    }
    else
    {
        return ie_invalid;
    }

    /* Extract the RSN info  Only the version is Mandatory all others are optional*/
    ie_le_CsrUint16(rsnElementCurrent, rsnData->version);

    if (ie_bytesleft(rsnElement, rsnElementCurrent) < 4)
        return ie_success; /* Optional Data */

    ie_be_CsrUint32(rsnElementCurrent, rsnData->groupCipherSuite);
    update_cipher_type(rsnData, rsnData->groupCipherSuite, unifi_GroupKey);

    if (ie_bytesleft(rsnElement, rsnElementCurrent) < 2)
        return ie_success; /* Optional Data */

    ie_le_CsrUint16(rsnElementCurrent, rsnData->pairwiseCipherSuiteCount);

    sme_trace_debug((TR_IE_ACCESS, "ie_access::rsnData->pairwiseCipherSuiteCount = %d", rsnData->pairwiseCipherSuiteCount));

    /* ---------------------------------------------------------------------*/
    /* Pairwise Cipher Suites                                               */
    /* ---------------------------------------------------------------------*/
    if (rsnData->pairwiseCipherSuiteCount > IE_MAX_PAIRWISE_CIPHER_SUITES)
    {
        sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() pairwiseCipherSuiteCount is to large for ie_rsn_info. pairwiseCiphersSuites will be lost"));
    }
    for (i = 0; i < rsnData->pairwiseCipherSuiteCount; i++) {

        if (ie_bytesleft(rsnElement, rsnElementCurrent) < 4)
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() rsn ie is not valid. Not enough data"));
            return ie_invalid;
        }

        /* Check for too much data */
        if (i >= IE_MAX_PAIRWISE_CIPHER_SUITES) { rsnElementCurrent+=4; continue; }


        ie_be_CsrUint32(rsnElementCurrent, rsnData->pairwiseCipherSuites[i]);

        if (allowUnicastUseGroupCipher && ((rsnData->pairwiseCipherSuites[i] & 0xFF) == IE_RSN_CIPHER_USE_GROUP))
        {
            sme_trace_info((TR_IE_ACCESS, "Using Group Cipher for Pairwise"));
            rsnData->pairwiseCipherSuites[i] = rsnData->groupCipherSuite;
        }
        else
        if ((rsnData->pairwiseCipherSuites[i] & 0xFF) == IE_RSN_CIPHER_USE_GROUP)
        {
            sme_trace_info((TR_IE_ACCESS, "Disallowed :: Use Group Cipher for Pairwise"));
            continue;
        }

        update_cipher_type(rsnData, rsnData->pairwiseCipherSuites[i], unifi_PairwiseKey);
    }

    if (rsnData->pairwiseCipherSuiteCount >= IE_MAX_PAIRWISE_CIPHER_SUITES)
    {
        rsnData->pairwiseCipherSuiteCount = IE_MAX_PAIRWISE_CIPHER_SUITES;
    }
    /* ---------------------------------------------------------------------*/

    /* ---------------------------------------------------------------------*/
    /* akm Cipher Suites                                                    */
    /* ---------------------------------------------------------------------*/
    if (ie_bytesleft(rsnElement, rsnElementCurrent) < 2) return ie_success; /* Optional Data */
    ie_le_CsrUint16(rsnElementCurrent, rsnData->akmSuiteCount);

    if (rsnData->akmSuiteCount > IE_MAX_AKM_SUITES)
    {
        sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() akmSuiteCount is to large for ie_rsn_info. akmSuites will be lost"));
    }
    for (i = 0; i < rsnData->akmSuiteCount; i++)
    {
        if (ie_bytesleft(rsnElement, rsnElementCurrent) < 4)
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() rsn ie is not valid. Not enough data"));
            return ie_invalid;
        }

        /* Check for too much data */
        if (i >= IE_MAX_AKM_SUITES) { rsnElementCurrent+=4; continue; }

        update_akm_suite_type(rsnData, authMode8021x, rsnElementCurrent);

        ie_be_CsrUint32(rsnElementCurrent, rsnData->akmSuites[i]);
    }
    if (rsnData->akmSuiteCount >= IE_MAX_AKM_SUITES)
    {
        rsnData->akmSuiteCount = IE_MAX_AKM_SUITES;
    }
    /* ---------------------------------------------------------------------*/

    if (ie_bytesleft(rsnElement, rsnElementCurrent) < 2) return ie_success; /* Optional Data */
    ie_le_CsrUint16(rsnElementCurrent, rsnData->rsmCapabilities);

    /* ---------------------------------------------------------------------*/
    /* pmkids                                                               */
    /* ---------------------------------------------------------------------*/
    /* this data ia not used so ignore it */

    /* ---------------------------------------------------------------------*/

    return ie_success;
}

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
/**
 * @brief
 *   process a WAPI IE
 *
 * @par Description
 *   process a WAPI IE
 *
 * @param[in] rsnData : the WAPI data block
 * @param[out] authMode8021x : authentication capability flag.
 * @param[in] rsnElement : WAPI IE buffer.
 *
 * @return
 *        ie_success   : element found
 *        ie_invalid   : IE field length invalid
 *        ie_not_found : IE element not found
 */
static ie_result process_wapi_element(
                    ie_rsn_data* rsnData,
                    unifi_AuthMode *authMode8021x,
                    CsrUint8 *wapiElement )
{
    int i = 0;
    CsrUint8* rsnElementCurrent;

    rsnData->cipherCapabilityFlags = unifi_EncryptionCipherNone;
    rsnData->version = 0;
    rsnData->groupCipherSuite = 0;
    rsnData->pairwiseCipherSuiteCount = 0;
    rsnData->akmSuiteCount = 0;
    rsnData->rsmCapabilities = 0;

    if(rsnData->rsntype == IE_RSN_TYPE_WAPI)
    {
        sme_trace_debug((TR_IE_ACCESS, "ie_access::rsnData->rsntype == IE_RSN_TYPE_WAPI"));
        rsnElementCurrent = wapiElement + IE_ID_FIELD_SIZE;
    }
    else
    {
        return ie_invalid;
    }

    /* Extract the RSN info  Only the version is Mandatory all others are optional*/
    ie_le_CsrUint16(rsnElementCurrent, rsnData->version);

    if (ie_bytesleft(wapiElement, rsnElementCurrent) < 4)
        return ie_invalid; /* Too short */

    /* ---------------------------------------------------------------------*/
    /* akm Cipher Suites                                                    */
    /* ---------------------------------------------------------------------*/
    ie_le_CsrUint16(rsnElementCurrent, rsnData->akmSuiteCount);

    if (rsnData->akmSuiteCount > IE_MAX_AKM_SUITES)
    {
        sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() akmSuiteCount is to large for ie_rsn_info. akmSuites will be lost"));
    }
    for (i = 0; i < rsnData->akmSuiteCount; i++)
    {
        if (ie_bytesleft(wapiElement, rsnElementCurrent) < 4)
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() WAPI ie is not valid. Not enough data"));
            return ie_invalid;
        }

        /* Check for too much data */
        if (i >= IE_MAX_AKM_SUITES) { rsnElementCurrent+=4; continue; }

        update_akm_suite_type(rsnData, authMode8021x, rsnElementCurrent);

        ie_be_CsrUint32(rsnElementCurrent, rsnData->akmSuites[i]);
    }
    if (rsnData->akmSuiteCount >= IE_MAX_AKM_SUITES)
    {
        rsnData->akmSuiteCount = IE_MAX_AKM_SUITES;
    }
    /* ---------------------------------------------------------------------*/

    ie_le_CsrUint16(rsnElementCurrent, rsnData->pairwiseCipherSuiteCount);

    sme_trace_debug((TR_IE_ACCESS, "ie_access::rsnData->pairwiseCipherSuiteCount = %d", rsnData->pairwiseCipherSuiteCount));

    /* ---------------------------------------------------------------------*/
    /* Pairwise Cipher Suites                                               */
    /* ---------------------------------------------------------------------*/
    if (rsnData->pairwiseCipherSuiteCount > IE_MAX_PAIRWISE_CIPHER_SUITES)
    {
        sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() pairwiseCipherSuiteCount is to large for ie_rsn_info. pairwiseCiphersSuites will be lost"));
    }
    for (i = 0; i < rsnData->pairwiseCipherSuiteCount; i++) {

        if (ie_bytesleft(wapiElement, rsnElementCurrent) < 4)
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() WAPI ie is not valid. Not enough data"));
            return ie_invalid;
        }

        /* Check for too much data */
        if (i >= IE_MAX_PAIRWISE_CIPHER_SUITES) { rsnElementCurrent+=4; continue; }


        ie_be_CsrUint32(rsnElementCurrent, rsnData->pairwiseCipherSuites[i]);

        update_cipher_type(rsnData, rsnData->pairwiseCipherSuites[i], unifi_PairwiseKey);
    }

    if (rsnData->pairwiseCipherSuiteCount >= IE_MAX_PAIRWISE_CIPHER_SUITES)
    {
        rsnData->pairwiseCipherSuiteCount = IE_MAX_PAIRWISE_CIPHER_SUITES;
    }
    /* ---------------------------------------------------------------------*/


    ie_be_CsrUint32(rsnElementCurrent, rsnData->groupCipherSuite);
    update_cipher_type(rsnData, rsnData->groupCipherSuite, unifi_GroupKey);


    if (ie_bytesleft(wapiElement, rsnElementCurrent) < 2) return ie_success; /* Optional Data */
    ie_le_CsrUint16(rsnElementCurrent, rsnData->rsmCapabilities);

    /* ---------------------------------------------------------------------*/
    /* pmkids                                                               */
    /* ---------------------------------------------------------------------*/
    /* this data is not used so ignore it */

    /* ---------------------------------------------------------------------*/

    return ie_success;
}
#endif


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in ie_access/ie_access_rsn.h
 */
/*---------------------------------------------------------------------------*/
ie_result ie_getRSNInfo(
                    CsrUint8* elements,
                    CsrUint32 length,
                    ie_rsn_ctrl* rsninfo,
                    CsrBool allowUnicastUseGroupCipher)
{
    CsrUint8* rsnElement;
    ie_result result = ie_error;

    require(TR_IE_ACCESS, elements != NULL);
    require(TR_IE_ACCESS, rsninfo != NULL);

    rsninfo->authMode8021x = unifi_80211AuthOpen;

    rsninfo->rsnDataWPA.rsntype = IE_RSN_TYPE_WPA;
    rsninfo->rsnDataWPA.rsmCapabilities = 0;

    rsninfo->rsnDataWPA2.rsntype = IE_RSN_TYPE_WPA2;
    rsninfo->rsnDataWPA2.rsmCapabilities = 0;

    rsninfo->rsnDataWAPI.rsntype = IE_RSN_TYPE_WAPI;
    rsninfo->rsnDataWAPI.rsmCapabilities = 0;

    /* ------------------------------------------------ */
    /* WPA2 RSN IE */
    /* ------------------------------------------------ */
    if (ie_find(IE_ID_RSN, elements, length, &rsnElement) == ie_success)
    {
        /* Data Size check for Version */
        if (ie_len(rsnElement) < 2)
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() rsn ie is not valid. Not enough data"));
            return ie_invalid;
        }

        result = process_rsn_element(&rsninfo->rsnDataWPA2,
                                     &(rsninfo->authMode8021x),
                                     rsnElement,
                                     allowUnicastUseGroupCipher);
    }

    /* ------------------------------------------------ */
    /* WPA VENDER IE */
    /* ------------------------------------------------ */
    if (ie_find_vendor_specific(ieWpaOui, NULL, 0, elements, length, &rsnElement) == ie_success)
    {
        sme_trace_hex((TR_IE_ACCESS, TR_LVL_DEBUG, "ie_access::ie_getRSNInfo() rsn ie ", rsnElement, ie_len(rsnElement)+2 ));
        /* Data Size check for Version */
        if (ie_len(rsnElement) < 2)
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() WPA ie is not valid. Not enough data"));
            return ie_invalid;
        }

        result = process_rsn_element(&rsninfo->rsnDataWPA,
                                     &(rsninfo->authMode8021x),
                                     rsnElement,
                                     allowUnicastUseGroupCipher);
    }

    /* ------------------------------------------------ */
    /* WAPI IE */
    /* ------------------------------------------------ */
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    if (ie_find(IE_ID_WAPI, elements, length, &rsnElement) == ie_success)
    {
        /* Data Size check for Version */
        if (ie_len(rsnElement) < 2)
        {
            sme_trace_error((TR_IE_ACCESS, "ie_access::ie_getRSNInfo() WAPI ie is not valid. Not enough data"));
            return ie_invalid;
        }

        result = process_wapi_element(&rsninfo->rsnDataWAPI,
                                     &(rsninfo->authMode8021x),
                                     rsnElement);
    }
#endif

    return result;
}


/*
 * Description:
 * See description in ie_access/ie_access_rsn.h
 */
/*---------------------------------------------------------------------------*/
void ie_rsn_update_ap_capabilities(
                    srs_scan_data *pSRelement,
                    CsrUint8 *pbuf,
                    CsrUint16 length,
                    CsrBool allowUnicastUseGroupCipher)
{
    ie_rsn_ctrl rsninfo;

    sme_trace_entry((TR_IE_ACCESS, "ie_rsn_update_ap_capabilities"));

    if(ie_success == ie_getRSNInfo(pbuf, length, &rsninfo, allowUnicastUseGroupCipher))
    {
        pSRelement->securityData.authMode_CapabilityFlags = rsninfo.authMode8021x;
        pSRelement->ieCapabilityFlags &= ~unifi_IECapabilityMask_PreAuth;

        if (rsninfo.authMode8021x & (unifi_8021xAuthWPA | unifi_8021xAuthWPAPSK))
        {
            pSRelement->securityData.wpaCipherCapabilityFlags  = rsninfo.rsnDataWPA.cipherCapabilityFlags;
        }

        if (rsninfo.authMode8021x & (unifi_8021xAuthWPA2 | unifi_8021xAuthWPA2PSK))
        {
            pSRelement->securityData.wpa2CipherCapabilityFlags = rsninfo.rsnDataWPA2.cipherCapabilityFlags;

            /* store the PreAuth capability */
            if ((rsninfo.rsnDataWPA2.rsmCapabilities & IE_RSN_PREAUTH_MASK) == IE_RSN_PREAUTH_MASK)
            {
                pSRelement->ieCapabilityFlags |= unifi_IECapabilityMask_PreAuth;
                sme_trace_info((TR_IE_ACCESS, "unifi_IECapabilityMask_PreAuth"));
            }
        }

        if (rsninfo.authMode8021x & (unifi_WAPIAuthWAI | unifi_WAPIAuthWAIPSK))
        {
            pSRelement->securityData.wapiCipherCapabilityFlags = rsninfo.rsnDataWAPI.cipherCapabilityFlags;

            /* store the PreAuth capability */
            if ((rsninfo.rsnDataWAPI.rsmCapabilities & IE_WAPI_PREAUTH_MASK) == IE_WAPI_PREAUTH_MASK)
            {
                pSRelement->ieCapabilityFlags |= unifi_IECapabilityMask_PreAuth;
                sme_trace_info((TR_IE_ACCESS, "unifi_IECapabilityMask_PreAuth"));
            }
        }

    }
}

/*
 * Description:
 * See description in ie_access/ie_access_rsn.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8 ie_rsn_check_ap_wpa_capability(CsrUint8 *pbuf, CsrUint16 length)
{
    CsrUint8* ie;

    if(ie_success == ie_find_vendor_specific(ieWpaOui, NULL, 0, pbuf, length, &ie))
    {

        return ie_len(ie) + 2;
    }

    return 0;
}

/*
 * Description:
 * See description in ie_access/ie_access_rsn.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8 ie_rsn_check_ap_wpa2_capability(CsrUint8 *pbuf, CsrUint16 length)
{
    CsrUint8     *ie;

    if (ie_success == ie_find(IE_ID_RSN, pbuf, length, &ie))
    {
        return ie_len(ie) + 2;
    }

    return 0;
}

/*
 * Description:
 * See description in ie_access/ie_access_rsn.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8 ie_rsn_check_ap_wapi_capability(CsrUint8 *pbuf, CsrUint16 length)
{
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    CsrUint8     *ie;

    if (ie_success == ie_find(IE_ID_WAPI, pbuf, length, &ie))
    {
        return ie_len(ie) + 2;
    }
#endif

    return 0;
}

/*
 * Description:
 * See description in ie_access/ie_access_rsn.h
 */
/*---------------------------------------------------------------------------*/
CsrBool ie_rsn_validate_ies(
                    CsrUint8 *pbuf,
                    CsrUint16 length,
                    SmeConfigData *cfg)
{

    ie_rsn_ctrl rsnieValue;
    ie_result ieCode = ie_getRSNInfo(pbuf,
                                      length,
                                     &rsnieValue,
                                     cfg->smeConfig.allowUnicastUseGroupCipher );

    if (ieCode == ie_success)
    {
        CsrUint8 i;
        CsrBool pairwiseFound = FALSE;
        CsrBool akmFound = FALSE;
        CsrUint8 rsnDataCount;
        ie_rsn_data *rsnData;
        ie_rsn_data *rsnDataTable[2];

        rsnDataTable[0] = &rsnieValue.rsnDataWPA2;
        rsnDataTable[1] = &rsnieValue.rsnDataWPA;

        /*
         * process all IEs and see is they match all critria of the
         * user defined configure.
         * if each check fails it will move exit the the current loop
         * and process the next IE
         */

        for(rsnDataCount = 0; rsnDataCount < 2; rsnDataCount++)
        {
            rsnData =  rsnDataTable[rsnDataCount];

            pairwiseFound = FALSE;
            akmFound = FALSE;

            sme_trace_debug((TR_IE_ACCESS, "rsnDataCount %d", rsnDataCount  ));

            /* check we are configured to use it. */
            if( (!((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPA2) & (rsnieValue.authMode8021x & unifi_8021xAuthWPA2)))
              &&(!((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPA2PSK) & (rsnieValue.authMode8021x & unifi_8021xAuthWPA2PSK)))
              &&(!((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPA) & (rsnieValue.authMode8021x & unifi_8021xAuthWPA)))
              &&(!((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPAPSK) & (rsnieValue.authMode8021x & unifi_8021xAuthWPAPSK)))
            )
            {
                sme_trace_debug((TR_IE_ACCESS, "WPA2 IE missmatch"  ));
                sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew() :: AuthMode == (0x%.4X) vs rsntype == (0x%.4X)", cfg->connectionConfig.authModeMask, rsnData->rsntype));
                continue;
            }

            /* ---------------------------------------------------------- */
            /* Process the group cipher suite */
            /* ---------------------------------------------------------- */
            if ( (!((cfg->connectionConfig.encryptionModeMask & unifi_EncryptionCipherGroupTkip) && ( (rsnData->groupCipherSuite == IE_WPA_CIPHER_TKIP)
                                                                                    ||(rsnData->groupCipherSuite == IE_WPA2_CIPHER_TKIP)) ))
               &&(!((cfg->connectionConfig.encryptionModeMask & unifi_EncryptionCipherGroupCcmp) && ( (rsnData->groupCipherSuite == IE_WPA_CIPHER_CCMP)
                                                                                          ||(rsnData->groupCipherSuite == IE_WPA2_CIPHER_CCMP)) ))
               )
            {
                sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew() :: group Key missmatch == encryptionStatus %x groupCipherSuite %x ", cfg->connectionConfig.encryptionModeMask, rsnData->groupCipherSuite));
                continue;
            }

            /* ---------------------------------------------------------- */
            /* Sanity check the pairwise cipher suite */
            /* ---------------------------------------------------------- */
            /* we should not have reach this point, but leave it in as a sanity check for now */
            if ( ((cfg->connectionConfig.encryptionModeMask & unifi_EncryptionCipherPairwiseTkip)    != unifi_EncryptionCipherPairwiseTkip)
               &&((cfg->connectionConfig.encryptionModeMask & unifi_EncryptionCipherPairwiseCcmp) != unifi_EncryptionCipherPairwiseCcmp) )
            {
                sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew() :: Cannot connect to WPA AP with Encrytion Status == %s", trace_unifi_EncryptionMode(cfg->connectionConfig.encryptionModeMask)));
                continue;
            }

            /* ---------------------------------------------------------- */
            /* Select the pairwise cipher suite */
            /* ---------------------------------------------------------- */
            if (rsnData->pairwiseCipherSuiteCount == 0)
            {
                sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew() :: No Pairwise Cipher Suite in RSN"));
                continue;
            }
            for (i = 0; i < rsnData->pairwiseCipherSuiteCount; i++)
            {
                /* if CCMP then stop as that is the best */
                if( ((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPA) && (rsnData->pairwiseCipherSuites[i] == IE_WPA_CIPHER_CCMP))
                  ||((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPAPSK) && (rsnData->pairwiseCipherSuites[i] == IE_WPA_CIPHER_CCMP))
                  ||((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPA2) && (rsnData->pairwiseCipherSuites[i] == IE_WPA2_CIPHER_CCMP))
                  ||((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPA2PSK) && (rsnData->pairwiseCipherSuites[i] == IE_WPA2_CIPHER_CCMP))
                  )
                {
                    if ( (cfg->connectionConfig.encryptionModeMask & unifi_EncryptionCipherPairwiseCcmp) == unifi_EncryptionCipherPairwiseCcmp)
                    {
                        pairwiseFound = TRUE;
                        break; /* AES is as good as it can get so stop looking here */
                    }
                }

                if( ((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPA) && (rsnData->pairwiseCipherSuites[i] == IE_WPA_CIPHER_TKIP))
                  ||((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPAPSK) && (rsnData->pairwiseCipherSuites[i] == IE_WPA_CIPHER_TKIP))
                  ||((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPA2) && (rsnData->pairwiseCipherSuites[i] == IE_WPA2_CIPHER_TKIP))
                  ||((cfg->connectionConfig.authModeMask & unifi_8021xAuthWPA2PSK) && (rsnData->pairwiseCipherSuites[i] == IE_WPA2_CIPHER_TKIP))
                  )
                {
                    if ( (cfg->connectionConfig.encryptionModeMask & unifi_EncryptionCipherPairwiseTkip) == unifi_EncryptionCipherPairwiseTkip)
                    {
                        pairwiseFound = TRUE;
                        /*break; Do not break as AES will override this if it exists */
                    }
                }
            }

            /* was a suitable pairwise suite found? */
            if (pairwiseFound == FALSE)
            {
                sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew() :: No usable Pairwise Cipher Suite found in RSN"));
                continue;
            }

            /* ---------------------------------------------------------- */
            /* Select the akm suite */
            /* ---------------------------------------------------------- */
            if (rsnData->akmSuiteCount == 0)
            {
                sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew() :: No akm Cipher Suite in RSN"));
                continue;
            }
            for (i = 0; i < rsnData->akmSuiteCount; i++)
            {
                if (rsnData->rsntype == IE_RSN_TYPE_WPA && rsnData->akmSuites[i] == IE_WPA_AKM_PSK)
                {
                    sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew(WPA, IE_RSN_AKM_PSK)"));
                    akmFound = TRUE;
                }
                else if (rsnData->rsntype == IE_RSN_TYPE_WPA && rsnData->akmSuites[i] == IE_WPA_AKM_PSKSA)
                {
                    sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew(WPA, IE_RSN_AKM_PSK)"));
                    akmFound = TRUE;
                    break;
                }
                else if (rsnData->rsntype == IE_RSN_TYPE_WPA2 && rsnData->akmSuites[i] == IE_WPA2_AKM_PSK)
                {
                    sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew(WPA, IE_RSN_AKM_PSK)"));
                    akmFound = TRUE;
                }
                else if (rsnData->rsntype == IE_RSN_TYPE_WPA2 && rsnData->akmSuites[i] == IE_WPA2_AKM_PSKSA)
                {
                    sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew(WPA, IE_RSN_AKM_PSK)"));
                    akmFound = TRUE;
                    break;
                }
            }
            if (akmFound == FALSE)
            {
                sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew() :: No usable AKM Suite found in RSN"));
                return unifi_Unavailable;
            }
            /* ---------------------------------------------------------- */

            /*
             * at this point we have not failed any of the tests and
             * have matched to the critia of the confiuration to the AP
             */
             break; /*lint !e539*/
        }

        /* if the count has exceeded the maximum number of IEs, we have processed both RSNs and failed */
        if (rsnDataCount >= 2)
        {
            sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew() :: failed to match critia to AP IEs"));
            return FALSE;
        }
        sme_trace_debug((TR_IE_ACCESS, "sec_8021xNew() :: matched to == %x", rsnDataCount));

        return TRUE;
    }

        /* no IE found, failed to validate */
    return FALSE;
}
