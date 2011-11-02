/** @file ie_access_wmm.h
 *
 * Utilities to process WMM information elements
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
 *   Utilities to process WMM information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_wmm.h#4 $
 *
 ****************************************************************************/
#ifndef SME_IE_ACCESS_WMM_H
#define SME_IE_ACCESS_WMM_H

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
#include "fsm/csr_wifi_fsm.h"

#include "qos_fsm/qos_tspec.h"

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Check to see if the IE convey WMM capability
 *
 * @par Description
 * Check to see if the IE convey WMM capability
 *
 * @param[in] pbuf : Pointer to the IE buffer.
 * @param[in] length : buffer length
 *
 * @return
 *        TRUE  : element found
 *        FALSE : IE element not found
 */
extern CsrBool ie_wmm_check_ap_wmm_capability(CsrUint8 *pbuf, CsrUint16 length);

/**
 * @brief Check to see if the IE contains a WMM IE
 *
 * @par Description
 * Check to see if the IE contains a WMM Parameter IE
 *
 * @param[in] pbuf : Pointer to the IE buffer.
 * @param[in] length : buffer length
 *
 * @return
 *        TRUE  : element found
 *        FALSE : IE element not found
 */
extern CsrUint8* ie_wmm_check_for_wmm_parameter_ie(CsrUint8 *pbuf, CsrUint16 length);

/**
 * @brief Check to see if the IE contains a WMM IE
 *
 * @par Description
 * Check to see if the IE contains a WMM Parameter IE
 *
 * @param[in] pbuf : Pointer to the IE buffer.
 * @param[in] length : buffer length
 *
 * @return
 *        CsrUint8 : start of ie or NULL
 */
extern CsrUint8* ie_get_wmm_ie(const CsrUint8 *pbuf, const CsrUint16 length);

/**
 * @brief Checks if the AP supports UAPSD
 *
 * @param[in]  beaconIe     : The beacon's information elements
 *
 * @return
 *        CsrBool
 */
extern CsrBool ie_isWMMUAPSD(CsrUint8 *pbuf, CsrUint16 length);

/**
 * @brief Checks if the AP supports UAPSD
 *
 * @param[in]  beaconIe     : The beacon's information elements
 *
 * @return
 *        CsrBool
 */
extern CsrBool ie_wmm_sme_uapsd_enable(
        CsrUint8 QoSInfoField);

/**
 * @brief Returns Total length of a WMM Associate Req Frame
 *
 * @par Description
 * Total length of a WMM Associate Req Frame
 *
 * @return
 *    CsrUint8 : Total length of a WMM Associate request IE
 */
extern CsrUint8 ie_wmm_get_associate_req_ie_length(void);

/**
 * @brief populates H85 IE
 *
 * @par Description
 * populates H85 IE
 *
 * @param[in] pbuf : buffer (large enough) to contain H85 IE
 * @param[in] QoSInfoField : buffer to the stored data.
 *
 * @return
 *    CsrUint8 : pointer to the next free element
 */
extern CsrUint8* ie_wmm_generate_associate_req_ie(
        CsrUint8 *pbuf,
        CsrUint8 *apWmmIe,
        CsrUint8 userQoSInfoField,
        CsrBool wmmPSAllowed,
        CsrBool btEnabled);

/**
 * @brief builds an acm mask
 *
 * @par Description:
 * builds an acm mask from an IE buffer
 *
 * @ingroup scanmanager
 *
 * @param[in]  informationElements : buffer to be parsed
 * @param[in]  informationElements : buffer to be parsed
 *
 * @return
 *      qos_ac_mask
 */
/*---------------------------------------------------------------------------*/
extern qos_ac_mask build_acm_mask(
        CsrUint8 *pbuf,
        CsrUint16 length);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_IE_ACCESS_WMM_H */
