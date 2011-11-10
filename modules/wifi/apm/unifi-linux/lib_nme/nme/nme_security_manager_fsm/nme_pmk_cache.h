/** @file pmk_cache.h
 *
 * PMK Cache store header file
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Handles storing of PMK's against Pmk IDs and the BssId
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_security_manager_fsm/nme_pmk_cache.h#2 $
 *
 ****************************************************************************/
#ifndef NME_PMK_CACHE_H
#define NME_PMK_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup security
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "nme_top_level_fsm/nme_top_level_fsm.h"

#include "csr_cstl/csr_wifi_list.h"
#include "csr_security.h"

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

/**
 * @brief defines the type to be used in the wireless profile storage list
 *
 * @par Description
 *   defines the type to be used in the wireless profile storage list
 */
typedef struct csr_list_pmk
{
    csr_list_node node;
    unifi_MACAddress bssid;
    CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH];
    CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH];
} csr_list_pmk;

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Gets key cached key data
 *
 * @par Description
 * Access cached key data for an AP based on the supplied BssId
 *
 * @param[in] csr_list* : PMK List
 * @param[in] const unifi_MACAddress*  : BssId of associated AP with the key
 *
 * @return csr_list_pmk* Cached data or NULL
 */
extern csr_list_pmk* nme_pmk_cache_get_pmk(csr_list* pmkList, const unifi_MACAddress* bssid);

/**
 * @brief Gets key cached key data
 *
 * @par Description
 * Access cached key data for an AP based on the supplied PmkId
 *
 * @param[in] csr_list* : PMK List
 * @param[in] const CsrUint8 [CSR_WIFI_SECURITY_PMKID_LENGTH]: pmkid associated with the key
 *
 * @return csr_list_pmk* Cached data or NULL
 */
extern csr_list_pmk* nme_pmk_cache_get_pmk_from_pmkid(csr_list* pmkList, const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH]);

/**
 * @brief Sets key cached key data
 *
 * @par Description
 *  Sets cached key data for an AP
 *
 * @param[in] csr_list* : PMK List
 * @param[in] const unifi_MACAddress*  : BssId of associated AP with the key
 * @param[in] const CsrUint8 [CSR_WIFI_SECURITY_PMKID_LENGTH]: PMKID Data to Cache
 * @param[in] const CsrUint8 [CSR_WIFI_SECURITY_PMK_LENGTH]: PMK Data to Cache
 *
 * @return void
 */
extern void nme_pmk_cache_set(csr_list* pmkList,
                              const unifi_MACAddress* bssid,
                              const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH],
                              const CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH]);

/**
 * @brief Delete key cached key data
 *
 * @par Description
 *  Remove cached key data (if it exists) for an AP
 *
 * @param[in] csr_list* : PMK List
 * @param[in] const unifi_MACAddress*  : BssId of associated AP with the key
 *
 * @return CsrBool - indicates if the keyData was deleted or not
 */
extern CsrBool nme_pmk_cache_delete(csr_list* pmkList, const unifi_MACAddress* bssid);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /*NME_PMK_CACHE_H*/
