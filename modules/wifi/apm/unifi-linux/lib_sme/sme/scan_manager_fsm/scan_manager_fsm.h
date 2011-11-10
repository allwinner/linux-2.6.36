/** @file scan_manager_fsm.h
 *
 * Public Scan Manager FSM API
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
 *   The scan manager sits idle waiting for any scan requests from the rest
 *   of the SME. It currently provides two scanning methods: active-full, and
 *   active-probe.
 * @par Active Full Scan
 *   This scans all the channels passed down in the channel-list data reference
 *   (an argument of the request) in a single MLME call. Note that this method
 *   utilises the air-interface for a considerable period of time. Because of
 *   this, the scan manager examines the MMI Network state - if it is in a
 *   connected, or similar, state then the scan is broken up into a series of
 *   single-channel MLME scan requests. If the MMI network state is disconnected
 *   (or similar) then a single MLME scan request is performed that scans all
 *   the channels in a single SME<->MLME request/confirm transation.
 * @par Active Probe Scan (AKA directected scan)
 *   Prior to joining a network it is advisable to perform a quick directed
 *   scan to detect that the network really exists (this mechanism allows a
 *   join to a stored profile to occur without having to do a full scan first).
 *   These scans are directed to a probe for a particular 'host' on a particular
 *   channel.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/scan_manager_fsm/scan_manager_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef SCAN_MANAGER_PROCESS_H
#define SCAN_MANAGER_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup scanmanager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "regulatory_domain/regulatory_domain.h"

/* PUBLIC MACROS ************************************************************/

/**
 * Generic probe delay time (in usecs) - medium is held idle for at least this
 * duration prior to transmitting a probe request frame.
 * This value should be 0 unless a VERY long (5000+) value is used. (to long to be practical)
 *
 */
#define SCANMGR_TMPFIXED_PROBE_DELAY_USEC            60     /* microseconds */

/**
 * The time to spend when performing a scan on an individual channel.
 */
#define SCANMGR_TMPFIXED_SINGLESCAN_ACTIVE_TU       500     /* TimeUnits */
#define SCANMGR_TMPFIXED_SINGLESCAN_PASSIVE_TU      210     /* TimeUnits */
#define SCANMGR_TMPFIXED_SINGLESCAN_ADHOC_TU       1000     /* TimeUnits */


/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the connection manager state machine data
 */
extern const FsmProcessStateMachine scanManagerFsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Get the current scan interval
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   CsrUint16 current scan interval
 */
extern unifi_ScanConfigData* get_current_scan_data(FsmContext* context, unifi_ScanConfigData** roamScanData);


/**
 * @brief
 *   Resets autonomous scan when a mlme reset is sent to the FW
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   void
 */
extern void reset_autonomous_scan(FsmContext* context);

/**
 * @brief
 *   returns a pointer to the regulatory domain information structure
 *
 * @par Description
 *   see brief
 *
 * @param[in]  context     : FSM context
 *
 * @return
 *   RegDomData *
 */
extern RegDomData* get_regulatory_data(FsmContext* context);

/**
 * @brief
 *   Install a roaming scan
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
extern void install_roaming_scan(FsmContext* context);

/**
 * @brief
 *   Install a roaming scan
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */
extern CsrBool cloaked_scanning_enabled(FsmContext* context);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SCAN_MANAGER_PROCESS_H */
