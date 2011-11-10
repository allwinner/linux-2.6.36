/** @file nme_core_fsm.h
 *
 * Public NME Core FSM API
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
 *   Public NME Core FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_core_fsm/nme_core_fsm.h#3 $
 *
 ****************************************************************************/
#ifndef CSR_WIFI_NME_CORE_PROCESS_H
#define CSR_WIFI_NME_CORE_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"

#include "csr_cstl/csr_wifi_list.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/**
 * @brief
 *   NME Configuration data
 *
 * @par Description
 *   Struct defining the configuration data for the NME
 */
typedef struct NmeConfigData
{
    /* -------------------------------------------- */
    /* Restricted Access Data */
    /* -------------------------------------------- */
    /* Sniffed to match the setting in the SME, if enabled then
     * restricted features are only allowed after activation
     */
    CsrBool enableRestrictedAccess;

    /* If true then the recorded app handle is the only one allowed
     * to request restricted features.
     */
    CsrBool restrictedAccessActivated;
    void* restrictedAccessAppHandle;

    /* Station MAC address */
    unifi_MACAddress stationMacAddress;

    /* PmkId Cache */
    csr_list pmkCache;

    /* Cloaked Ssid's Track the user sets and add any
     * cloaked profile ssid's to the list */
    CsrUint8    cloakedSsidsCount;
    unifi_SSID* cloakedSsids;

    /* Need to track whether we have reported that we connected as we
     * should only forward MEDIA STATUS IND (disconnected) if we reported
     * that we connected successfully.
     */
    CsrBool reportedConnected;

} NmeConfigData;


/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the state machine data
 */
extern const FsmProcessStateMachine nme_core_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Accessor for the NME config data
 *
 * @par Description
 *   see brief
 *
 * @param[in]  FsmContext*     : FSM context
 *
 * @return
 *   NmeConfigData* - Configuration data
 */
extern NmeConfigData* nme_core_get_nme_config(FsmContext* pContext);

/**
 * @brief
 *   Returns TRUE if the specified appHandle is restricted access
 *
 * @par Description
 *   see brief
 *
 * @param[in]  FsmContext* : FSM context
 * @param[in]  void*       : app handle
 *
 * @return
 *   CsrBool
 */
extern CsrBool nme_core_is_access_restricted(
        FsmContext* pContext,
        void* appHandle);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_NME_CORE_PROCESS_H */
