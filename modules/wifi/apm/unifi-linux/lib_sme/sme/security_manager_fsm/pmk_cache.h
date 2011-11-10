/** @file pmk_cache.h
 *
 * PMK Cache store header file
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
 *   Handles storing of PMK's
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/security_manager_fsm/pmk_cache.h#2 $
 *
 ****************************************************************************/
#ifndef PMK_CACHE_H
#define PMK_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup security
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"

#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"

#include "csr_cstl/csr_wifi_list.h"

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

typedef struct PmkCacheContext PmkCacheContext;

/**
 * @brief defines the type to be used in the wireless profile storage list
 *
 * @par Description
 *   defines the type to be used in the wireless profile storage list
 */
typedef struct csr_list_pmk
{
    csr_list_node node;
    unifi_SSID ssid;
    unifi_Pmkid pmkid;
} csr_list_pmk;

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Initialize the pmk cache
 *
 * @par Description
 * Sets up the Key storage
 *
 * @return
 *        PmkCacheContext*
 */
extern PmkCacheContext* sec_initialise_pmk_cache(void);

/**
 * @brief Shutdown the pmk cache
 *
 * @par Description
 *  frees the cache
 *
 * @return
 *        void
 */
extern void sec_shutdown_pmk_cache(PmkCacheContext* context);

/**
 * @brief Deletes all cache entries
 *
 * @par Description
 * clears the Key storage
 *
 * @return
 *        void
 */
extern void sec_clear_pmk_cache(PmkCacheContext* context);

/**
 * @brief Gets key cached key data
 *
 * @par Description
 * Access cached key data for an AP
 *
 * @param[in] bssid: AP associated with the key
 *
 * @return pmk_key_data* Cached data or NULL
 */
extern unifi_Status sec_get_pmk_cached_data(PmkCacheContext* context, const unifi_MACAddress* bssid, unifi_Pmkid** pmkid);

/**
 * @brief Sets key cached key data
 *
 * @par Description
 *  Access cached key data for an AP
 *
 * @param[in] bssid  : AP associated with the key
 * @param[in] keyData: Data to Cache
 *
 * @return void
 */
extern void sec_set_pmk_cached_data(PmkCacheContext* context, const unifi_SSID* ssid, const unifi_Pmkid* pmkid);

/**
 * @brief Delete key cached key data
 *
 * @par Description
 *  Remove cached key data for an AP
 *
 * @param[in] bssid: AP associated with the key
 *
 * @return void
 */
extern void sec_delete_pmk_cached_data(PmkCacheContext* context, const unifi_MACAddress* bssid);

/**
 * @brief Delete key cached key data
 *
 * @par Description
 *  Remove cached key data for an AP
 *
 * @param[in] bssid: AP associated with the key
 *
 * @return void
 */
extern csr_list* sec_get_pmk_list_handle(PmkCacheContext* context);

/**
 * @brief Delete key cached key data
 *
 * @par Description
 *  Remove cached key data for an AP
 *
 * @param[in] bssid: AP associated with the key
 *
 * @return void
 */
extern CsrUint8 sec_get_pmk_list_count(PmkCacheContext* context);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /*PMK_CACHE_H*/
