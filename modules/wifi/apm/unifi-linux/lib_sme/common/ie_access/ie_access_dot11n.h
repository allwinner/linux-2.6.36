/** @file ie_access_dot11n.h
 *
 * Utilities to process 802.11n information elements
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
 *   Utilities to process 802.11n information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_dot11n.h#2 $
 *
 ****************************************************************************/
#ifndef SME_IE_ACCESS_DOT11N_H
#define SME_IE_ACCESS_DOT11N_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup ie_access
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

/**
 * @brief defines the ie RSN info block
 *
 * @par Description
 *   defines the ie RSN info block
 */
typedef enum dot11n_ie_elementid
{
   IE_DOT11N_ID_HT_CAPABILITIES            = (CsrUint8)0x2D, /* 45*/
   IE_DOT11N_ID_HT_INFO                    = (CsrUint8)0x3D, /* 61*/
   IE_DOT11N_ID_MAX                        = (CsrUint8)0xFF  /* FF*/
} dot11n_ie_elementid;

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Returns Total length of a WMM Associate Req Frame
 *
 * @par Description
 * Total length of a WMM Associate Req Frame
 *
 * @return
 *    CsrUint8 : Total length of a WMM Associate request IE
 */
extern CsrUint8 ie_dot11n_get_ht_cap_length(void);

/**
 * @brief Generate a HT cap IE
 *
 * @par Description
 * Generate a HT capabilities IE
 *
 * @param[in] pbuf : buffer (large enough) to contain HT Cap IE
 *
 * @return
 *    CsrUint8 : pointer to the next free element
 */
extern CsrUint8* ie_dot11n_generate_ht_cap_ie(
        CsrUint8 *pbuf);

/**
 * @brief Check for AP HT capabilities
 *
 * @par Description
 * Checks for AP HT capabilities and information IEs
 *
 * @param[in] pbuf : IE buffer
 * @param[in] length : buffer length
 *
 * @return
 *    CsrBool : TRUE  - capable
 *            : FALSE - not capable
 */
extern CsrBool ie_dot11n_check_ap_capability(
        CsrUint8 *pbuf,
        CsrUint16 length);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_IE_ACCESS_DOT11N_H */
