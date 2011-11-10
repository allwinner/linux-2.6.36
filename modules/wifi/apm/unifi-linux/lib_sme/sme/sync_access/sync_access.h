/** @file sync_access.h
 *
 *  Allow thread safe syncronous access to the scan results
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
 *   Allow thread safe syncronous access to the scan results
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sync_access/sync_access.h#1 $
 *
 ****************************************************************************/
#ifndef SYNC_ACCESS_H
#define SYNC_ACCESS_H


#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup sync_access
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme.h"

#ifdef SME_SYNC_ACCESS  /* NB: conditional compilation has to go here to avoid an empty include file (otherwise gcc complains) */

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

typedef struct unifi_ScanResultList
{
    CsrUint16 numElements;
    unifi_ScanResult* results;
} unifi_ScanResultList;


/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Protects syncronous access to the SME's data
 *
 * @par Description
 * Lock the SME to allow safe access to internel bufferers
 *
 * @return
 *   void
 */
extern void unifi_mgt_claim_sync_access(FsmContext* context);

/**
 * @brief Unprotects syncronous access
 *
 * @par Description
 * Unlock the SME
 *
 * @return
 *   void
 */
extern void unifi_mgt_release_sync_access(FsmContext* context);

/**
 * @brief Gets scan results
 *
 * @par Description
 * This function fills in the scan results buffer with the scan data available
 *
 * @param[inout]  resultsBuffer : Scan data filled in from the scan list
 *
 * @return
 *        unifi_Success     : if scan data was found
 *        unifi_EndOfList   : if no scan data was found
 */
extern unifi_Status unifi_mgt_scan_results_get(FsmContext* context, unifi_ScanResultList* resultsBuffer);

/**
 * @brief Flushes the scan results
 *
 * @par Description
 * This function clears the SME scan data cache.
 * If we are currently connected to an AP the scan result for the current AP will NOT be deleted
 *
 * @return
 *        unifi_Status error code
 */
extern unifi_Status unifi_mgt_scan_results_flush(FsmContext* context);

/**
 * @brief Gets a sme configuration value
 *
 * @par Description
 * Gets a SME data value the same as unifi_mgt_get_value_req/cfm
 *
 * @param[inout]  appValue : Data to read
 *
 * @return
 *        unifi_Status error code
 */
extern unifi_Status unifi_mgt_get_value(FsmContext* context, unifi_AppValue* appValue);

/**
 * @brief Sets a sme configuration value
 *
 * @par Description
 * Sets a SME data value the same as unifi_mgt_set_value_req/cfm
 *
 * @param[inout]  appValue : Data to write
 *
 * @return
 *        unifi_Status error code
 */
extern unifi_Status unifi_mgt_set_value(FsmContext* context, const unifi_AppValue* appValue);

/**
 * @brief Manage pmkid's
 *
 * @return
 *        unifi_Status error code
 */
extern unifi_Status unifi_mgt_pmkid(FsmContext* context, unifi_ListAction action, unifi_PmkidList *pmkids);

/**
 * @brief Manage blacklist
 *
 * @return
 *        unifi_Status error code
 */
extern unifi_Status unifi_mgt_blacklist(FsmContext* context, unifi_ListAction action, unifi_AddressList *addresses);


/** \@}
 */

#endif /* SME_SYNC_ACCESS */

#ifdef __cplusplus
}
#endif

#endif /*SYNC_ACCESS_H*/
