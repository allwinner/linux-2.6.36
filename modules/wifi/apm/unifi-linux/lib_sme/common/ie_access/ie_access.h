/** @file ie_access.h
 *
 * Utilities to read and write information elements
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
 *   Utilities to read and write information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access.h#1 $
 *
 ****************************************************************************/
#ifndef SME_IE_ACCESS_H
#define SME_IE_ACCESS_H

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

#include "lib_info_element.h"
#include "lib_info_element_trace.h"
#include "lib_info_element_subsidairy_fields.h"

#include "ie_access/ie_access_ssid.h"
#include "ie_access/ie_access_wmm.h"
#include "ie_access/ie_access_rsn.h"
#include "ie_access/ie_access_wps.h"
#include "ie_access/ie_access_dot11n.h"
#ifdef CCX_VARIANT
#include "ccx_ie_access/ccx_ie_access.h"
#endif

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

/**
 * @brief prints all IEs presentin a buffer
 *
 * @par Description
 *  prints all IEs presentin a buffer
 *
 * @param[in] elements : Pointer to the IE buffer.
 * @param[in] length   : Length of the IE buffer.
 *
 * @return
 *        void
 */
extern void ie_printIEInfo(
                    CsrUint8* elements,
                    CsrUint32 length);

/**
 * @brief Checks for G Rates in the info elements
 *
 * @param[in] elements : Pointer to the IE buffer.
 * @param[in] length   : Length of the IE buffer.
 *
 * @return
 *        CsrBool
 */
CsrBool ie_hasGRates(
                    CsrUint8* elements,
                    CsrUint32 length);


/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_IE_ACCESS_H */
