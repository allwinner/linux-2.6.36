/** @file nme_routing_manager.h
 *
 * Public NME APPHANDLE MANAGER API
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
 *   Public NME Primitive routing Utils API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_signal_routing_fsm/nme_routing_manager.h#2 $
 *
 ****************************************************************************/
#ifndef NME_ROUTING_MANAGER_H_
#define NME_ROUTING_MANAGER_H_
#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_routing_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "csr_cstl/csr_wifi_list.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/**
 * @brief list base node that for a specified primitive
 *        confirmation Id will contains a list of routing
 *        information for the expected confirmation primitives
 *        of the specified id.
 *
 * @par Description
 *   see Brief
 */
typedef struct CsrListRoutingCfmBaseNode
{
    /** node definition */
    csr_list_node node;
    /** list structure */
    csr_list      queue;
    /** eventId */
    CsrUint16     cfmId;
} CsrListRoutingCfmBaseNode;

/**
 * @brief defines the information that will allow correct routing
 *        of a received confirmation.
 *
 * @par Description
 *   see Brief
 */
typedef struct CsrListRoutingCfmInstNode
{
    /** node definition */
    csr_list_node node;
    /** appHandle */
    void*         appHandle;
    /** should the cfm be consumed internally or forwarded to the NME-SAP? */
    CsrBool       isExternalEvent;
    /** fsm that must receive the cfm */
    CsrUint16     destinationFsm;
} CsrListRoutingCfmInstNode;

/**
 * @brief
 *   Will contain the indication mask for a specific appHandle
 *
 * @par Description
 *   see Brief
 */
typedef struct NmeIndRegistration
{
    void* appHandle;
    unifi_IndicationsMask indMask;
} NmeIndRegistration;

/**
 * @brief defines a routing Control Block
 *
 * @par Description
 *   see Brief
 */
typedef struct RoutingCtrlblk
{
    /** list structure */
    csr_list routingCfmInfoList;

    /* Variables for tracking the appHandles to indications mappings */
    CsrUint16 indRegistrationSize;
    CsrUint16 indRegistrationCount;
    NmeIndRegistration* indRegistrations;
    /* Used to temporarily hold the list of appHandles */
    void** tmpIndList;

} RoutingCtrlblk;


/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

/**
 * @brief
 *   module Initialisation function
 *
 * @par Description
 *   See Brief
 *
 * @param[in]    pContext* : FSM context
 * @param[in]    pCtrlblk* : Control Block pointer
 *
 * @return
 *   void
 */
extern void nme_routing_context_initialize(
            FsmContext* pContext,
            RoutingCtrlblk *pCtrlblk);

/**
 * @brief
 *   module reset function
 *
 * @par Description
 *   See Brief
 *
 * @param[in]    pContext* : FSM context
 * @param[in]    pCtrlblk* : Control Block pointer
 *
 * @return
 *   void
 */
extern void nme_routing_context_reset(
            FsmContext* pContext,
            RoutingCtrlblk *pCtrlblk);

/**
 * @brief
 *   Store external event routing information
 *
 * @par Description
 *   See Brief
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const CsrUint16 : Confirm ID
 * @param[in] const void*: appHandle
 *
 * @return
 *   void
 */
extern void nme_routing_store_cfm_prim_external(
        FsmContext* pContext,
        const CsrUint16 cfmId,
        void* const pAppHandle);

/**
 * @brief
 *   Store internal event routing information
 *
 * @par Description
 *   See Brief
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const CsrUint16 : Confirm ID
 * @param[in] const CsrUint16 : fsm instance
 *
 * @return
 *   None
 */
extern void nme_routing_store_cfm_prim_internal(
        FsmContext* pContext,
        const CsrUint16 cfmId,
        const CsrUint16 fsmInstance);

/**
 * @brief
 *   Attempts to find a list for the confirmation ID specified and
 *   if present will return information for the element at the head
 *   of the queue. The element at the head of the queue will also
 *   be removed.
 *
 * @par Description
 *   See Brief
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const CsrUint16 : confirmation Id
 * @param[out] void** : appHandle
 * @param[out] CsrBool* : should the confirm be forward to the NME-SAP?
 * @param[out] CsrUint16* : fsm instance (if cfm is for internal use)
 *
 * @return
 *   CsrBool - TRUE if an entry is found for the confirmation Id.
 */
extern CsrBool nme_routing_get_cfm_apphandle(
        FsmContext* pContext,
        const CsrUint16 cfmId,
        void** ppAppHandle,
        CsrBool *isExternal,
        CsrUint16 *destinationFsmInstance);

/**
 * @brief
 *   Attempts to create a list of appHandles for the specified
 *   indication mask.
 *
 * @par Description
 *   See Brief
 *
 * @param[in]    pContext* : FSM context
 * @param[in]    unifi_IndicationsMask : bit map of indications to look for associated
 *                                       appHandles
 * @param[in]    void** : used to return the pointer the list of appHandles
 *
 * @return
 *   CsrUint16 count of appHandles in the list
 */
CsrUint16 nme_routing_get_ind_apphandles(
        FsmContext* context,
        unifi_IndicationsMask ind,
        void** ppAppHandles);


#ifdef __cplusplus
}
#endif

#endif /* NME_ROUTING_MANAGER_H_ */
