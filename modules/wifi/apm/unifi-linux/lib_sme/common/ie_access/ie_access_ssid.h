/** @file ie_access_ssid.h
 *
 * Utilities to read and write SSID elements
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
 *   Utilities to read and write SSID elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_ssid.h#1 $
 *
 ****************************************************************************/
#ifndef SME_IE_ACCESS_SSID_H
#define SME_IE_ACCESS_SSID_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup ie_access
 */

/* STANDARD INCLUDES ********************************************************/
#include "abstractions/osa.h"

/* PROJECT INCLUDES *********************************************************/

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Copies the SSID into the provided result buffer.
 *
 * @par Description
 *  Copies the SSID into the provided result buffer.
 *  The result buffer MUST be at least 33 bytes.
 *  The result from this call will be a null terminated string
 *      See Doc P802.11-REVma/D7.0 -> 7.3.2.1
 *
 * @param[in] element : A pointer to the element buffer.
 * @param[out] result : A pointer to the output buffer.
 *
 * @return
 *        ie_success   : element found
 *        ie_not_found : IE element not found
 */
extern ie_result ie_getSSIDStr(
        CsrUint8* element,
        char* result);

/**
 * @brief Copies the SSID into the provided unifi_SSID.
 *
 * @par Description
 * Copies the SSID into the provided unifi_SSID
 *   See Doc P802.11-REVma/D7.0 -> 7.3.2.1
 *
 * @param[in] element : A pointer to the element buffer.
 * @param[out] result : A pointer to the output buffer.
 *
 * @return
 *        ie_success   : element found
 *        ie_error     : The SSID is greater than 32
 *        ie_not_found : IE element not found
 */
extern ie_result ie_getSSID(
        CsrUint8* element,
        unifi_SSID *result);

/**
 * @brief Gets the SSID Length.
 *
 * @par Description
 *   Gets the SSID Length
 *
 * @param[in] element : A pointer to the element buffer.
 * @param[out] result : A pointer to the output buffer.
 *
 * @return
 *        ie_success   : element found
 *        ie_error     : The SSID is greater than 32
 *        ie_not_found : IE element not found
 */
extern CsrUint8 ie_get_ssid_length(
        const unifi_SSID * const pSsid);

/**
 * @brief Copies the SSID into the provided buffer starting at element[0]
 *
 * @par Description
 *  Copies the SSID into the provided buffer starting at element[0]
 *      See Doc P802.11-REVma/D7.0 -> 7.3.2.1
 *
 * @param[out] element : Pointer to the IE buffer.
 * @param[in] result : pointer to the SSID buffer.
 *
 * @return
 *        ie_success   : always returned
 */
extern ie_result ie_setSSID(
        CsrUint8* element,
        const unifi_SSID* const result);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_IE_ACCESS_SSID_H */
