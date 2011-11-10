/** @file ie_access_scan_req.h
 *
 * Utilities to populate scan req information elements
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
 *   Utilities to populate scan req information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_message_handling/ie_access_scan_req.h#1 $
 *
 ****************************************************************************/
#ifndef SME_IE_ACCESS_SCAN_REQ_H
#define SME_IE_ACCESS_SCAN_REQ_H

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

#include "ie_access/ie_access.h"


/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Creates the informational Element buffer for the Scan req
 *
 * @par Description
 * Creates the informational Element buffer for the Scan req
 *
 * @return
 *        DataReference
 */
extern DataReference ie_create_scan_req_ies(
        FsmContext* context,
        CsrUint8 ssidCount,
        const unifi_SSID * const pSsid,
        CsrUint16 probeIeLength,
        const CsrUint8* const probeIe);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_IE_ACCESS_scan_REQ_H */
