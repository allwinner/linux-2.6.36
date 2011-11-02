/** @file ie_access_rsn.h
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_rsn.h#1 $
 *
 ****************************************************************************/
#ifndef SME_IE_ACCESS_RSN_H
#define SME_IE_ACCESS_RSN_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup ie_access
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"

#include "scan_manager_fsm/scan_results_storage.h"
#include "sme_configuration/sme_configuration_fsm.h"

/* MACROS *******************************************************************/

/** pairwise cipher suites */
#define IE_MAX_PAIRWISE_CIPHER_SUITES 10
/** Maximun AX AKM suites */
#define IE_MAX_AKM_SUITES 10
/** Maximum PMKIDS */
#define IE_MAX_PMKIDS 3


/**
 * @brief defines the ie result codes
 *
 * @par Description
 *   the ie result codes
 */
typedef enum ie_rsn_type
{
    IE_RSN_TYPE_WPA,
    IE_RSN_TYPE_WPA2,
    IE_RSN_TYPE_WAPI,
    IE_RSN_TYPE_UNDEFINED
} ie_rsn_type;


/* Cipher field mask */
/** Use Group mask */
#define IE_RSN_CIPHER_USE_GROUP        (CsrUint32)0x00000000
/** wep 40 mask */
#define IE_RSN_CIPHER_WEP40            (CsrUint32)0x00000001
/** tkip mask */
#define IE_RSN_CIPHER_TKIP             (CsrUint32)0x00000002
/** ccmp mask */
#define IE_RSN_CIPHER_CCMP             (CsrUint32)0x00000004
/** wep 104 mask */
#define IE_RSN_CIPHER_WEP104           (CsrUint32)0x00000005
/** vendor specific mask */
#define IE_RSN_CIPHER_VENDOR           (CsrUint32)0x00000006

#define IE_WAPI_CIPHER_SMS4_MASK       (CsrUint32)0x00000001

/* Capability field mask */
/** pre-auth mask */
#define IE_RSN_PREAUTH_MASK                 (CsrUint16)0x0001
/** no  pairwise mask */
#define IE_RSN_NO_PAIRWISE_MASK             (CsrUint16)0x0002
/** ptksa replay counter mask */
#define IE_RSN_PTKSA_REPLAY_COUNTER_MASK    (CsrUint16)0x000C
/** gtksa replay counter mask */
#define IE_RSN_GTKSA REPLAY_COUNTER_MASK    (CsrUint16)0x0030
/** peerkey enabled mask */
#define IE_RSN_PEERKEY_ENABLED_MASK         (CsrUint16)0x0200

/** pre-auth mask */
#define IE_WAPI_PREAUTH_MASK                (CsrUint16)0x0001


#define GROUP_CIPHER_MASK    (unifi_EncryptionCipherGroupWep40  |   \
                              unifi_EncryptionCipherGroupWep104 |   \
                              unifi_EncryptionCipherGroupTkip   |   \
                              unifi_EncryptionCipherGroupCcmp   |   \
                              unifi_EncryptionCipherGroupSms4)

#define PAIRWISE_CIPHER_MASK (unifi_EncryptionCipherPairwiseWep40  |    \
                              unifi_EncryptionCipherPairwiseWep104 |    \
                              unifi_EncryptionCipherPairwiseTkip   |    \
                              unifi_EncryptionCipherPairwiseCcmp   |    \
                              unifi_EncryptionCipherPairwiseSms4)




/* TYPES DEFINITIONS ********************************************************/
/**
 * @brief defines the ie RSN info block
 *
 * @par Description
 *   defines the ie RSN info block
 *
 * TO BE PHASED OUT..
 *
 */
typedef struct ie_rsn_info
{
    /** RSN type */
    CsrUint16 rsntype;
    /** Version */
    CsrUint16 version;
    /** Group Cipher Suite */
    CsrUint32 groupCipherSuite;
    /** PairWise Cipher Suites count */
    CsrUint16 pairwiseCipherSuiteCount;
    /** PairWise Cipher Suites Array */
    CsrUint32 pairwiseCipherSuites[IE_MAX_PAIRWISE_CIPHER_SUITES];
    /** AKM Suite count */
    CsrUint16 akmSuiteCount;
    /** AKM Suite array */
    CsrUint32 akmSuites[IE_MAX_AKM_SUITES];
    /** RSM capabilities */
    CsrUint16 rsmCapabilities;
} ie_rsn_info;



/**
 * @brief defines the ie RSN data block
 *
 * @par Description
 *   defines a single RSN data block
 */
typedef struct ie_rsn_data
{
    /** RSN Type identifier*/
    ie_rsn_type rsntype;
    /** cipherCapability */
    unifi_EncryptionMode cipherCapabilityFlags;
    /** Version */
    CsrUint16 version;
    /** Group Cipher Suite */
    CsrUint32 groupCipherSuite;
    /** PairWise Cipher Suites count */
    CsrUint16 pairwiseCipherSuiteCount;
    /** PairWise Cipher Suites Array */
    CsrUint32 pairwiseCipherSuites[IE_MAX_PAIRWISE_CIPHER_SUITES];
    /** AKM Suite count */
    CsrUint16 akmSuiteCount;
    /** AKM Suite array */
    CsrUint32 akmSuites[IE_MAX_AKM_SUITES];
    /** RSM capabilities */
    CsrUint16 rsmCapabilities;
} ie_rsn_data;

/**
 * @brief defines the ie RSN Ctrl block
 *
 * @par Description
 *   The IE can contain both WPA and WPA2 RSN block. This is encapsulates both.
 */
typedef struct ie_rsn_ctrl
{
    /** 802.1x Authentication bitmask */
    unifi_AuthMode authMode8021x;

    /** WPA RSN data block*/
    ie_rsn_data rsnDataWPA;

    /** WPA2 RSN data block*/
    ie_rsn_data rsnDataWPA2;

    /** WAPI data block*/
    ie_rsn_data rsnDataWAPI;

} ie_rsn_ctrl;


/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Gets the RSN information
 *
 * @par Description
 *  Gets the RSN information
 *      See Doc P802.11-REVma/D7.0 -> 7.3.2.25
 *
 * @param[in] elements : Pointer to the IE buffer.
 * @param[in] length   : Length of the IE buffer.
 * @param[out] rsnCtrl : pointer to the output buffer.
 *
 * @return
 *        ie_success   : element found
 *        ie_invalid   : IE field length invalid
 *        ie_not_found : IE element not found
 */
extern ie_result ie_getRSNInfo2(
                    CsrUint8* elements,
                    CsrUint32 length,
                    ie_rsn_ctrl* rsnCtrl,
                    CsrBool allowUnicastUseGroupCipher);

extern ie_result ie_getRSNInfo(
                    CsrUint8* elements,
                    CsrUint32 length,
                    ie_rsn_ctrl* rsnCtrl,
                    CsrBool allowUnicastUseGroupCipher);

/**
 * @brief Updates the AP Capabilities
 *
 * @par Description
 * Updates the AP Capabilities
 *
 * @param[out] pSRelement : Pointer to the current scanresult.
 * @param[in]  pbuf : Pointer to the IE buffer.
 * @param[in]  length : buffer length
 *
 * @return
 *        void
 */
extern void ie_rsn_update_ap_capabilities(
                    srs_scan_data *pSRelement,
                    CsrUint8 *pbuf,
                    CsrUint16 length,
                    CsrBool allowUnicastUseGroupCipher);

/**
 * @brief Check to see if the IE convey WPA capability
 *
 * @par Description
 * Check to see if the IE convey WPA capability
 *
 * @param[in] pbuf : Pointer to the IE buffer.
 * @param[in] length : buffer length
 *
 * @return
 *        > 0  : element found of length X
 *        0    : IE element not found
 */
extern CsrUint8 ie_rsn_check_ap_wpa_capability(
                    CsrUint8 *pbuf,
                    CsrUint16 length);

/**
 * @brief Check to see if the IE convey WPA2 capability
 *
 * @par Description
 * Check to see if the IE convey WPA2 capability
 *
 * @param[in] pbuf : Pointer to the IE buffer.
 * @param[in] length : buffer length
 *
 * @return
 *        > 0  : element found of length X
 *        0    : IE element not found
 */
extern CsrUint8 ie_rsn_check_ap_wpa2_capability(
                    CsrUint8 *pbuf,
                    CsrUint16 length);

/**
 * @brief Check to see if the IE convey WAPI capability
 *
 * @par Description
 * Check to see if the IE convey WAPI capability
 *
 * @param[in] pbuf : Pointer to the IE buffer.
 * @param[in] length : buffer length
 *
 * @return
 *        > 0  : element found of length X
 *        0    : IE element not found
 */
extern CsrUint8 ie_rsn_check_ap_wapi_capability(
                    CsrUint8 *pbuf,
                    CsrUint16 length);

/**
 * @brief Validate the IE again Configuration settings
 *
 * @par Description
 * Validate the IE again Configuration settings
 *
 * @param[in] pbuf : Pointer to the IE buffer.
 * @param[in] length : buffer length
 *
 * @return
 *        TRUE  : element found
 *        FALSE : IE element not found
 */
extern CsrBool ie_rsn_validate_ies(
                    CsrUint8 *pbuf,
                    CsrUint16 length,
                    SmeConfigData *cfg);


/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_IE_ACCESS_RSN_H */
