/** @file ap_validation.h
 *
 * Public AP Validation API
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
 *   Public AP Validation API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ap_utils/ap_validation.h#2 $
 *
 ****************************************************************************/
#ifndef AP_VALIDATION_H
#define AP_VALIDATION_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup networkselector
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "fsm/fsm_types.h"
#include "sme_configuration/sme_configuration_fsm.h"
#include "ie_access/ie_access.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Function to validate an Access Point
 *
 * @par Description
 *   A function to validate an Access Point against the current configuration.
 *
 * @param[in]  context   : FSM context
 * @param[in]  pScanData : Scan data to be check
 * @param[in]  cfg       : Current configuration data
 *
 * @return
 *   unifi_NetworkStatusMMI current status
 */
extern CsrBool validate_ap(
                    FsmContext* context,
                    srs_scan_data *pScanData,
                    SmeConfigData *cfg);

/**
 * @brief
 *   Function to check if a draft N connection is required
 *
 * @par Description
 *   Function to check if a draft N connection is required
 *
 * @param[in]  context   : FSM context
 * @param[in]  pScanData : Scan data to be check
 * @param[in]  cfg       : Current configuration data
 *
 * @return
 *   unifi_NetworkStatusMMI current status
 */
extern CsrBool check_if_draft_n_required(
        FsmContext* context,
        ie_rsn_data *rsnData,
        SmeConfigData *cfg);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* AP_VALIDATION_H */
