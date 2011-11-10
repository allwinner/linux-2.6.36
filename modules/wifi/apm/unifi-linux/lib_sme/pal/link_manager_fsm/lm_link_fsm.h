/** @file lm_link_fsm.h
 *
 * Public LM Link FSM API
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
 *   Public LM Link FSM API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/lm_link_fsm.h#1 $
 *
 ****************************************************************************/
#ifndef PAL_LM_LINK_FSM_H
#define PAL_LM_LINK_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup link_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "link_manager_fsm/amp_assoc_endec.h"
#include "csr_security.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/**
 * @brief
 *   Attributes for the physical link.
 *
 * @par Description
 *
 */
typedef struct physicalLinkAttrib
{
    CsrUint8 handle; /* link handle as provided by AMP Manager */
    PalRole role;
    /* Timers for link supervision and connection accept */


/* This flag is used to check if a physical link creation was cancelled while creating
 * If TRUE, the a HCI_Create_Physical_Link_Complete before HCI-Disconnect-Complete with status phyLinkCompleteStatus needs to sent
 * after disconnection is complete
 */
    CsrBool needPhysLinkCompleteEvent;

/* Flag is set to TRUE if there is a physical link disconnect request pending from the HCI.
 * This is primarily used when HCI-Disconnect-Physical-Link is received when there are
 * logical links associated with the link. The logical links needs to cleared first.
 */
    CsrBool discRequested;

    CsrBool needResetCompleteEvent;
    CsrBool needCoreStopCfm;

    AMP_Assoc remoteAssoc; /* remote side Assoc information */

    CsrUint16 hipFsmPid;

    FsmTimerId     timerId;

    /* physical link status connect/disconnect status*/
    HciStatusCode phyLinkCompleteStatus;

    /* only used to store when link disconnect from HIP FSM. */
    HciStatusCode hipLinkDisconnectedReason;

    HciStatusCode logicalLinkCompleteStatus;

    /* Link  supervision timer */
    CsrUint16 linkSupervisionTimeoutDuration;

    CsrUint16 numLogicalLinksDeleted;

    CsrUint8 linkKey[CSR_WIFI_SECURITY_PMK_LENGTH];

    /* used to determine HCI command that triggered the procedure with data manager.
         * so that the right response can be sent
         */
    HciOpcode pendingCommandCode; 

}physicalLinkAttrib;



/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the ip connection manager state machine data
 */
extern const FsmProcessStateMachine pal_lm_link_fsm;

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PAL_LM_LINK_FSM_H */
