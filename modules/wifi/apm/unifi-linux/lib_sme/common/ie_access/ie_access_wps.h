/** @file ie_access_wps.h
 *
 * Utilities to process WPS information elements
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
 *   Utilities to process WPS information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_wps.h#1 $
 *
 ****************************************************************************/
#ifndef SME_IE_ACCESS_WPS_H
#define SME_IE_ACCESS_WPS_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup ie_access
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Check to see if the IE convey WPS capability
 *
 * @par Description
 * Check to see if the IE convey WPS capability
 *
 * @param[in] pbuf : Pointer to the IE buffer.
 * @param[in] length : buffer length
 *
 * @return
 *        TRUE  : element found
 *        FALSE : IE element not found
 */
extern CsrBool ie_wps_check_ap_wps_capability(
                    CsrUint8 *pbuf,
                    CsrUint16 length);


/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_IE_ACCESS_WPS_H */
