/** @file nme_routing_manager.c
 *
 * NME primitive application handle manager
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
 *   Each primitive contains an application handle (appHandle) that in the
 *   case of a request confirm pair have to match. Whilst for indications
 *   applications have to register for the indications that they wish to
 *   receive.
 *
 *   When handling request confirms the NME has to record the appHandle in
 *   requests received from applications and ensure that the confirm sent
 *   in response has the original appHandle.
 *
 *   For indications that NME has to handle NME SET EVENT MASK REQs and
 *   ensure that only requested indications are passed upwards whilst
 *   the NME needs to receive all indications from the SME.
 *
 *   This file contains the function required for the NME to record and
 *   retrieve the appropriate appHandle (or maybe handles in the case
 *   of indications).
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_signal_routing_fsm/nme_routing_manager.c#3 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_routing_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "nme_top_level_fsm/nme_top_level_fsm.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief Gets a required node from the apphandle list structure
 *
 * @par Description:
 * See Brief
 *
 * @ingroup nme_routing_manager
 *
 * @param[in] pCtrlblk* : Control Block pointer
 * @param[in] eventId : Confirmation id to be located
 *
 * @return
 *      CsrListRoutingCfmBaseNode* node in the list or NULL
 */
static CsrListRoutingCfmBaseNode* nme_routing_cfm_base_node_get(
        RoutingCtrlblk *pCtrlblk,
        const CsrUint16 cfmId)
{
    CsrListRoutingCfmBaseNode *pNode;
    csr_list* list;

    require(TR_NME_SIGR, NULL != pCtrlblk);

    sme_trace_entry((TR_NME_SIGR, "nme_routing_cfm_base_node_get(), looking for [0x%x]", cfmId));

    list = &pCtrlblk->routingCfmInfoList;

    for (pNode = csr_list_gethead_t(CsrListRoutingCfmBaseNode *, list); NULL != pNode;
         pNode = csr_list_getnext_t(CsrListRoutingCfmBaseNode *, list, &(pNode->node)))
    {
        if(pNode->cfmId == cfmId)
        {
            return pNode;
        }
    }
    return NULL;
}

/**
 * @brief Stores the supplied information ready for routing
 *        confirm when it is received.
 *
 * @par Description:
 * See Brief
 *
 * @ingroup nme_routing_manager
 *
 * @param[in] FsmContext* : Fsm context
 * @param[in] const CsrUint16 : cfm id
 * @param[in] void* const : message apphandle
 * @param[in] const CsrBool : is the destination external?
 * @param[in] const CsrUint16 : internal destination FSM
 *
 * @return
 *      void
 */
static void nme_routing_store_cfm_primitive_info(
        FsmContext* pContext,
        const CsrUint16 cfmId,
        void* const appHandle,
        const CsrBool isExternalEvent,
        const CsrUint16 fsmInstance)
{
    RoutingCtrlblk *pCtrlblk;
    CsrListRoutingCfmInstNode *pCfmInstNode;
    CsrListRoutingCfmBaseNode *pBaseNode;

    require_trace(TR_NME_SIGR, NULL != pContext, (TR_NME_SIGR, "pContext %p ", pContext));

    sme_trace_entry((TR_NME_SIGR, "nme_routing_store_cfm_primitive_info(ID %x handle: %x isExt: %x FSM: %x)", cfmId, appHandle, isExternalEvent, fsmInstance));

    /* Try to find an existing base node for the confirmation id or create a
     * new one if one doesn't exist, so that we can add the information for
     * the particular confirmation instance to a queue.
     */
    pCtrlblk = nme_signal_routing_get_ctrlblk_context(pContext);
    pBaseNode = nme_routing_cfm_base_node_get(pCtrlblk, cfmId);
    if (pBaseNode == NULL)
    {
        pBaseNode = CsrPmalloc(sizeof(*pBaseNode));
        csr_list_init(&(pBaseNode->queue));
        pBaseNode->cfmId = cfmId;
        csr_list_insert_tail(&(pCtrlblk->routingCfmInfoList), list_owns_value, &(pBaseNode->node), pBaseNode);
    }
    /* Add the actual info for the expected confirm to the queue. */
    pCfmInstNode = CsrPmalloc(sizeof(*pCfmInstNode));
    pCfmInstNode->isExternalEvent = isExternalEvent;
    pCfmInstNode->appHandle = appHandle;
    pCfmInstNode->destinationFsm = fsmInstance;
    csr_list_insert_tail(&(pBaseNode->queue), list_owns_value, &(pCfmInstNode->node), pCfmInstNode);
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in nme_core_fsm/nme_routing_manager.h
 */
/*---------------------------------------------------------------------------*/
void nme_routing_context_initialize(
        FsmContext* pContext,
        RoutingCtrlblk *pCtrlblk)
{
    require_trace(TR_NME_SIGR, NULL != pContext, (TR_NME_SIGR, "pContext %p ", pContext));
    require_trace(TR_NME_SIGR, NULL != pCtrlblk, (TR_NME_SIGR, "pCtrlblk %p ", pCtrlblk));

    csr_list_init(&(pCtrlblk->routingCfmInfoList));
    pCtrlblk->indRegistrationSize = 0;
    pCtrlblk->indRegistrationCount = 0;
    pCtrlblk->indRegistrations = NULL;
    pCtrlblk->tmpIndList = NULL;
}

/*
 * Description:
 * See description in nme_core_fsm/nme_apphandle_manager.h
 */
/*---------------------------------------------------------------------------*/
void nme_routing_context_reset(
        FsmContext* pContext,
        RoutingCtrlblk *pCtrlblk)
{
    require_trace(TR_NME_SIGR, NULL != pContext, (TR_NME_SIGR, "pContext %p ", pContext));
    require_trace(TR_NME_SIGR, NULL != pCtrlblk, (TR_NME_SIGR, "pCtrlblk %p ", pCtrlblk));

    while (!csr_list_isempty(&(pCtrlblk->routingCfmInfoList)))
    {
        CsrListRoutingCfmBaseNode *pBaseNode = csr_list_gethead_t(CsrListRoutingCfmBaseNode *, &(pCtrlblk->routingCfmInfoList));

        while (!csr_list_isempty(&(pBaseNode->queue)))
        {
            csr_list_removehead(&(pBaseNode->queue));
        }
        csr_list_removehead(&(pCtrlblk->routingCfmInfoList));
    }
    if (0 < pCtrlblk->indRegistrationCount)
    {
        CsrPfree(pCtrlblk->indRegistrations);
    }
    pCtrlblk->indRegistrationSize = 0;
    pCtrlblk->indRegistrationCount = 0;
    if (NULL != pCtrlblk->tmpIndList)
    {
        CsrPfree(pCtrlblk->tmpIndList);
    }
    pCtrlblk->tmpIndList = NULL;
}

/*
 * Description:
 * See description in nme_core_fsm/nme_routing_manager.h
 */
/*---------------------------------------------------------------------------*/
void nme_routing_store_cfm_prim_internal(
        FsmContext* pContext,
        const CsrUint16 cfmId,
        const CsrUint16 fsmInstance)
{
    require_trace(TR_NME_SIGR, NULL != pContext, (TR_NME_SIGR, "pContext %p ", pContext));
    nme_routing_store_cfm_primitive_info(pContext, cfmId, NULL, FALSE, fsmInstance);
}

/*
 * Description:
 * See description in nme_core_fsm/nme_routing_manager.h
 */
/*---------------------------------------------------------------------------*/
void nme_routing_store_cfm_prim_external(
        FsmContext* pContext,
        const CsrUint16 cfmId,
        void* const appHandle)
{
    require_trace(TR_NME_SIGR, NULL != pContext, (TR_NME_SIGR, "pContext %p ", pContext));
    require_trace(TR_NME_SIGR, NULL != pContext, (TR_NME_SIGR, "appHandle %p ", appHandle));
    nme_routing_store_cfm_primitive_info(pContext, cfmId, appHandle, TRUE, 0);
}


/*
 * Description:
 * See description in nme_core_fsm/nme_routing_manager.h
 */
/*---------------------------------------------------------------------------*/
CsrBool nme_routing_get_cfm_apphandle(
        FsmContext* pContext,
        const CsrUint16 cfmId,
        void** appHandle,
        CsrBool *externalEvent,
        CsrUint16 *fsmInstance)
{
    RoutingCtrlblk *pCtrlblk = NULL;
    CsrListRoutingCfmBaseNode *pCurrentBaseNode = NULL;

    require_trace(TR_NME_SIGR, NULL != pContext, (TR_NME_SIGR, "pContext %p ", pContext));

    sme_trace_entry((TR_NME_SIGR, "nme_routing_get_cfm_apphandle()"));

    pCtrlblk = nme_signal_routing_get_ctrlblk_context(pContext);
    pCurrentBaseNode = nme_routing_cfm_base_node_get(pCtrlblk, cfmId);

    /* if is does not exist, add as a new base node */
    if (NULL == pCurrentBaseNode)
    {
        sme_trace_debug((TR_NME_SIGR, "node not found"));
        return FALSE;
    }
    else
    {
        /* get the head */
        CsrListRoutingCfmInstNode *pNode = csr_list_gethead_t(CsrListRoutingCfmInstNode *, &(pCurrentBaseNode->queue));

        *appHandle = pNode->appHandle;
        *externalEvent = pNode->isExternalEvent;
        *fsmInstance = pNode->destinationFsm;

        /* delete node */
        csr_list_removehead(&(pCurrentBaseNode->queue));

        /* finally if the queue is empty, delete the base node. */
        if (csr_list_isempty(&(pCurrentBaseNode->queue)))
        {
            csr_list_result removeResult=csr_list_not_found;
            removeResult = csr_list_remove(&(pCtrlblk->routingCfmInfoList), &(pCurrentBaseNode->node));
            if (csr_list_success != removeResult)
            {
                sme_trace_warn((TR_NME_SIGR, "Failed to remove base node"));
            }
            ensure(TR_NME_SIGR, csr_list_success == removeResult);
        }
    }
    return TRUE;
}


/*
 * Description:
 * See description in nme_core_fsm/nme_routing_manager.h
 */
/*---------------------------------------------------------------------------*/
CsrUint16 nme_routing_get_ind_apphandles(
        FsmContext* pContext,
        unifi_IndicationsMask ind,
        void** ppAppHandles)
{
    RoutingCtrlblk *pCtrlblk = NULL;
    CsrUint16 count = 0;
    CsrUint8 i = 0;

    require_trace(TR_NME_SIGR, NULL != pContext, (TR_NME_SIGR, "pContext %p ", pContext));
    require_trace(TR_NME_SIGR, NULL != ppAppHandles, (TR_NME_SIGR, "ppAppHandles %p ", ppAppHandles));

    sme_trace_entry((TR_NME_SIGR, "nme_routing_get_ind_apphandles(%s)", trace_unifi_IndicationsMask(ind)));

    pCtrlblk = nme_signal_routing_get_ctrlblk_context(pContext);
    if (pCtrlblk->indRegistrationCount == 0)
    {
        sme_trace_info((TR_NME_SIGR, "0 registered app handles"));
        return 0;
    }

    /* Allocate the max size possible */
    CsrPfree(pCtrlblk->tmpIndList);
    pCtrlblk->tmpIndList = CsrPmalloc(pCtrlblk->indRegistrationCount * sizeof(void*));
    *ppAppHandles = pCtrlblk->tmpIndList;

    for (i = 0; i < pCtrlblk->indRegistrationCount; ++i)
    {
        if (pCtrlblk->indRegistrations[i].indMask & ind)
        {
            pCtrlblk->tmpIndList[count] = pCtrlblk->indRegistrations[i].appHandle;
            count++;
        }
    }
    sme_trace_info((TR_NME_SIGR, "%d registered app handles", count));
    return count;
}

