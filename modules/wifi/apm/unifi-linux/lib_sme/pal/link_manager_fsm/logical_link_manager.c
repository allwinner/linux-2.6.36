/** @file logical_link_manager.c
 *
 * Logical link manager
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
 *  This file contains the library for logical link management primarily used by
 * the link manager FSMs.
 *
 ****************************************************************************
 *
 * @section MODIFICATION HISTORY
 * @verbatim
 *   #1   18:apr:08 B-36899 :created.
 * @endverbatim
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/logical_link_manager.c#1 $
 *
 ****************************************************************************/

/** @{
 * @ingroup link_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "pal_manager/pal_manager.h"
#include "link_manager_fsm/hip_interface.h"
#include "link_manager_fsm/logical_link_manager.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */


#define PAL_FIRST_LOGICAL_LINK_HANDLE (0x000)
#define PAL_LAST_LOGICAL_LINK_HANDLE (0xFFE)
#define PAL_DOT11_MAX_SHORT_AND_LONG_RETRY_LIMIT (255)

/*
 * This is an assumed value for guaranteed latency link where the SDU Interarriver
 * time is not specified.
 * Expressed in Mbps
 */
#define PAL_ASSUMED_BW_FOR_GUARANTEED_LATENCY_LINK (2)

#define GET_GUARANTEED_LINK_BANDWIDTH(flowSpec) ((0xFFFFFFFF==(flowSpec)->sduInterArrivalTime)?PAL_ASSUMED_BW_FOR_GUARANTEED_LATENCY_LINK:((((flowSpec)->maximumSduSize*8)/(flowSpec)->sduInterArrivalTime) * (1000 *1000)) / (1024*1024)+1)

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/
typedef enum deleteLinkType
{
    DELETE_ALL_LINKS,
    DELETE_MATCHING_LINK
}deleteLinkType;


/**
 * @brief
 *   Attributes for the logical link.
 *
 * @par Description
 *
 */
typedef struct logicalLinkAttrib
{
    CsrUint16 logicalLinkHandle; /* link handle created by PAL */
    CsrUint8 physicalLinkHandle; /* link handle created by PAL */
    Priority userPriority;
    AmpFlowSpec txFlowSpec;
    AmpFlowSpec rxFlowSpec;
    CsrUint32 bestEfforFlushTimeout; /* in microseconds */
}logicalLinkAttrib;


/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/


/* PRIVATE FUNCTION DEFINITIONS *********************************************/


/**
 * @brief
 *   get User Priority from the service type in the flow specifications given by the Host
 *
 * @par Description
 *   If the PAL supports guranteed links then UserPriority will be based on the service type in
 * flow spec. Otherwise the UP will always be Priority_Contention.
 *
 * @param[in]    flowSpec   : Pointeer to the flow spec from host
 * @param[in]    qosSupported   : boolean for guaranteed link support locally.
 *
 * @return
 *   void
 */
static Priority pal_get_user_priority_from_flow_spec(const AmpFlowSpec *flowSpec, CsrBool qosSupported)
{
    Priority userPriority=Priority_Contention;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_get_user_priority_from_flow_spec: serviceType-%d",flowSpec->serviceType));
    if (qosSupported)
    {
        if (HCI_SERVICE_TYPE_BEST_EFFORT == flowSpec->serviceType)
        {
            userPriority = Priority_QoSUP0;
        }
        else if (HCI_SERVICE_TYPE_GUARANTEED == flowSpec->serviceType)
        {
            userPriority = Priority_QoSUP4;
        }
    }
    sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_get_user_priority_from_flow_spec: userPriority-%d",userPriority));
    return userPriority;
}

/**
 * @brief
 *   allocate a new logical link handle
 *
 * @par Description
 * Function identifies the new link handle along with the postion to insert in the linked list
 * It returns the node after which the new node should be inserted along with the new logical
 * link handle. If the new node is to be inserted is the first node then the insertAfterNode
 * pointer will be NULL
 *
 * @param[in]    logicalLinkList   : Pointer to list anchor
 * @param[out]    insertAfterNode   : pointer the node after which teh new node can be inserted.
 *                                                   NULL if no new handle can be allocated.
 *
 * @return
 *   handle if new handle is allocated.
 */
static CsrUint16 pal_allocate_logical_link_handle(csr_list *logicalLinkList,csr_list_node **insertAfterNode)
{
    csr_list_node *currentNode,*previousNode=NULL; /* if previous node remains null, insert as the first node*/
    logicalLinkAttrib *logicalLink;
    CsrUint16 newHandle = PAL_FIRST_LOGICAL_LINK_HANDLE;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_allocate_logical_link"));
    currentNode = csr_list_gethead_void(logicalLinkList);
    while (currentNode)
    {
        logicalLink = (logicalLinkAttrib *)currentNode->value;
        verify(TR_PAL_LM_LINK_FSM, logicalLink!=NULL);
        sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_allocate_logical_link: new handle-%d,current handle-%d",newHandle,logicalLink->logicalLinkHandle));
        if (newHandle < logicalLink->logicalLinkHandle)
        {
            /* insert the new node between the current one and previous one
             * and the handle should be 1 greater than the previous handle
             */
            sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_allocate_logical_link: new handle found-%d!!",newHandle));
            break;
        }
        newHandle += 1; /* increment the handle */
        previousNode = currentNode;
        currentNode = csr_list_getnext_void(logicalLinkList,currentNode);
    }

    *insertAfterNode = previousNode;
    return newHandle;
}

/**
 * @brief
 *   delete logical link entry
 *
 * @par Description
 * Function to delete logical link entry in the list for a physical link. Depending on the parameter the function will delete
 * just one linke provided a valid logical link handle is given or all links matching the supplied phy link handle. deleteType
 * determines the action to be taken.
 *
 * @param[in]    context   : FSM Context
 * @param[in]    llmContext   : pointer to the logical link manager context
 * @param[in]    logicalLinkHandle   : logical link handle
 * @param[in]    phyLinkHandle   : physical link handle
 * @param[in]    deleteType   :  Type of deletion ->delete all or delete matching link only.
 * @param[in]    callback   : callback -> NULL or function pointer to call on deletion.
 *
 * @return
 *   TRUE if operation is success else FALSE.
 */
static CsrBool pal_delete_logical_link_attrib(FsmContext *context,
                                                logicalLinkManagerContext *llmContext,
                                                CsrUint16 logicalLinkHandle,
                                                CsrUint8 phyLinkHandle,
                                                deleteLinkType deleteType,
                                                pal_llm_logical_link_deleted_callback callback)
{
    CsrBool status=FALSE;
    csr_list_node *node,*currentNode;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_delete_logical_link_attrib: llHandle-%d,delType-%d,phyLinkHdl-%d",logicalLinkHandle,deleteType,phyLinkHandle));
    node = csr_list_gethead_void(&llmContext->logicalLinkList);

    while (node)
    {
        logicalLinkAttrib *logicalLink = (logicalLinkAttrib *)node->value;

        currentNode = node;
        node = csr_list_getnext_void(&llmContext->logicalLinkList,currentNode);
        if (logicalLink &&
            ((DELETE_MATCHING_LINK == deleteType && logicalLinkHandle == logicalLink->logicalLinkHandle) ||
             (DELETE_ALL_LINKS == deleteType && logicalLink->physicalLinkHandle==phyLinkHandle)))
        {
            sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_delete_logical_link_attrib: deleting link node with handle %d, current list size-%d",logicalLink->logicalLinkHandle,csr_list_size(&llmContext->logicalLinkList)));
            if (callback)
            {
                sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_delete_logical_link_attrib: Invoking callback"));
                callback(context, logicalLink->logicalLinkHandle);
            }
            /* if its a guranteed link, replenish the available bandwidth */
            if (HCI_SERVICE_TYPE_GUARANTEED == logicalLink->txFlowSpec.serviceType)
            {
                sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_delete_logical_link_attrib: Increase available guaranteed bandwidth from %d", llmContext->availableGuaranteedBandwidth));
                llmContext->availableGuaranteedBandwidth += (CsrUint8)GET_GUARANTEED_LINK_BANDWIDTH(&logicalLink->txFlowSpec);
                sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_delete_logical_link_attrib:  to %d", llmContext->availableGuaranteedBandwidth));
            }

            (void)csr_list_remove(&llmContext->logicalLinkList,currentNode);

            sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_delete_logical_link_attrib: List size after deleting link-%d",csr_list_size(&llmContext->logicalLinkList)));

            status=TRUE;

            /* We have just deleted one logical link. Skip the loop
                       * if this was only one requested to be deleted. Otherwise continue the loop until all links are deleted.
                       */
            if (DELETE_MATCHING_LINK == deleteType)
            {
                break;
            }
        }
    }
    return status;
}

/**
 * @brief
 *   get number of logical links created in a particular service type.
 *
 * @par Description
 *
 * @param[in]    llmContext   :  pointer to the logical link manager context
 * @param[in]    serviceType   : type of service.
 *
 * @return
 *   number of logical links
 */
static CsrUint16 pal_llm_get_existing_link_count(logicalLinkManagerContext *llmContext, HciServiceType serviceType)
{
    csr_list_node *node,*currentNode;
    CsrUint16 count=0;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_get_existing_link_count"));
    node = csr_list_gethead_void(&llmContext->logicalLinkList);
    while (node)
    {
        logicalLinkAttrib *logicalLink = (logicalLinkAttrib *)node->value;

        if (serviceType==logicalLink->txFlowSpec.serviceType )
        {
            count++;
        }

        currentNode = node;
        node = csr_list_getnext_void(&llmContext->logicalLinkList,currentNode);
    }
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_get_existing_link_count: count- %d",count));
    return count;
}


/**
 * @brief
 *   get attributes for the link with matching handle
 *
 * @par Description
 *
 * @param[in]    llmContext   :  pointer to the logical link manager context
 * @param[in]    logicalLinkHandle   : handle for which the attributes needs to be retrieved
 *
 * @return
 *   pointer to the attributes if found or NULL.
 */
static logicalLinkAttrib *pal_llm_get_link_attrib(logicalLinkManagerContext *llmContext, CsrUint16 logicalLinkHandle)
{
    csr_list_node *node,*currentNode;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_get_link_attrib"));
    node = csr_list_gethead_void(&llmContext->logicalLinkList);
    while (node)
    {
        logicalLinkAttrib *logicalLink = (logicalLinkAttrib *)node->value;

        if (logicalLinkHandle==logicalLink->logicalLinkHandle)
        {
            return logicalLink;
        }

        currentNode = node;
        node = csr_list_getnext_void(&llmContext->logicalLinkList,currentNode);
    }
    return NULL;
}

/**
 * @brief
 *   get attributes for the link with matching flow spec id
 *
 * @par Description
 *
 * @param[in]    llmContext   :  pointer to the logical link manager context
 * @param[in]    flowSpecId   : flow spec id for which the attributes needs to be retrieved
 *
 * @return
 *   pointer to the attributes if found or NULL.
 */
static logicalLinkAttrib *pal_llm_get_link_attrib_from_flow_spec_id(logicalLinkManagerContext *llmContext, CsrUint16 flowSpecId)
{
    csr_list_node *node,*currentNode;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_get_link_attrib_from_flow_spec_id: id -%d",flowSpecId));
    node = csr_list_gethead_void(&llmContext->logicalLinkList);
    while (node)
    {
        logicalLinkAttrib *logicalLink = (logicalLinkAttrib *)node->value;

        if (flowSpecId==logicalLink->txFlowSpec.id)
        {
            sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_get_link_attrib_from_flow_spec_id: Found logicalLinkHandle -%d",logicalLink->logicalLinkHandle));
            return logicalLink;
        }

        currentNode = node;
        node = csr_list_getnext_void(&llmContext->logicalLinkList,currentNode);
    }
    return NULL;
}


/* PUBLIC FUNCTIONS *********************************************************/

CsrBool pal_llm_new_logical_link_acceptable(FsmContext *context,
                                                 logicalLinkManagerContext *llmContext,
                                                 const AmpFlowSpec *txFlowSpec,
                                                 CsrBool qosSupported)
{
    CsrBool status=FALSE;

    sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_new_logical_link_acceptable:"));
    sme_trace_info((TR_PAL_LM_LINK_FSM,"TX FlowSpec->maxSduSize -%d, flushTO-%d, accLat-%d, id-%d, interArrTim-%d, servType-%s",
                                        txFlowSpec->maximumSduSize,
                                        txFlowSpec->flushTimeout,
                                        txFlowSpec->accessLatency,
                                        txFlowSpec->id,
                                        txFlowSpec->sduInterArrivalTime,
                                        GET_SERVICE_TYPE_STR(txFlowSpec->serviceType)));
    sme_trace_info((TR_PAL_LM_LINK_FSM,"linkQosSuuport-%d, localQosSupport-%d,currentLinks-%d,availabeGurntdBW-%d,maxLinksAllowed-%d,maxQosLinksAllowed-%d, requestedBW-%d",
                    qosSupported,pal_guranteed_link_supported(context),
                    csr_list_size(&llmContext->logicalLinkList),
                    llmContext->availableGuaranteedBandwidth,
                    llmContext->maxLogicalLinks,llmContext->maxGuranteedLogicalLinks,
                    GET_GUARANTEED_LINK_BANDWIDTH(txFlowSpec)));

    if (csr_list_size(&llmContext->logicalLinkList) < llmContext->maxLogicalLinks)
    {
        PAL_MibData *mibData=pal_get_common_mib_data(context);
        /* Allow it if its best effort */
        if (HCI_SERVICE_TYPE_NO_TRAFFIC == txFlowSpec->serviceType)
        {
            sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_new_logical_link_acceptable: no traffic link acceptable"));
            status = TRUE;
        }
        else if (HCI_SERVICE_TYPE_BEST_EFFORT == txFlowSpec->serviceType)
        {
            /* According to AMP Mgr FIPD D05r14 section 5.6
             * "If 'Best effort' is selected then Access Latency and Flush Timeout shall both be set
             * to 0xFFFFFFFF."
             * "Values of 0xFFFFFFFF for Maximum SDU size and SDU Inter-arrival time are used when
             * the actual values are not known. If one value is set to 0xFFFFFFFF then the other value
             * shall also be set to 0xFFFFFFFF."
             */
            if ((0xFFFFFFFF==txFlowSpec->flushTimeout && 0xFFFFFFFF==txFlowSpec->accessLatency) &&
                ((0xFFFF == txFlowSpec->maximumSduSize && 0xFFFFFFFF == txFlowSpec->sduInterArrivalTime) ||
                (txFlowSpec->maximumSduSize <= PAL_MAX_PDU_SIZE && txFlowSpec->sduInterArrivalTime <= PAL_MIN_SDU_INTER_ARRIVAL_TIME)))
            {
                sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_new_logical_link_acceptable: best effort link acceptable"));
                status = TRUE;
            }
        }
        else
        {
            if (qosSupported && /* if link doesn't support Qos then guranteed logical link cannot be created. */
                txFlowSpec->maximumSduSize <= PAL_MAX_PDU_SIZE && /* Must be specified for guaranteed links */
                pal_llm_get_existing_link_count(llmContext,HCI_SERVICE_TYPE_GUARANTEED) < llmContext->maxGuranteedLogicalLinks &&
                txFlowSpec->flushTimeout <= pal_hip_get_max_flush_timeout(mibData->dot11HCCWmin) &&
                txFlowSpec->flushTimeout >= pal_hip_get_min_flush_timeout(mibData->dot11HCCWmin) && /* to ensure atleast one retry is possible */
                /* if AccessLatency is specified , check its validity. */
                (txFlowSpec->accessLatency >= pal_hip_get_min_latency(mibData->dot11HCCWmin) ||
                 0xFFFFFFFF == txFlowSpec->accessLatency) &&
                /* No point checking SDU InterArrival Time as it is checked during bandwidth calculation */
                /*txFlowSpec->sduInterArrivalTime >= PAL_MIN_SDU_INTER_ARRIVAL_TIME => it could be 0xFFFFFFFF for guaranteed latency links s*/
                GET_GUARANTEED_LINK_BANDWIDTH(txFlowSpec) <= llmContext->availableGuaranteedBandwidth
                )

            {
                sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_new_logical_link_acceptable: guaranteed link acceptable."));
                status = TRUE;
            }
        }
    }

    sme_trace_debug_code(
        {
            if (!status)
            {
                sme_trace_debug((TR_PAL_LM_LINK_FSM, "pal_llm_new_logical_link_acceptable: New logical link creation failed"));
            }
        }
    )

    return status;
}

CsrBool pal_llm_flow_spec_modify_acceptable(FsmContext *context,
                                                   logicalLinkManagerContext *llmContext,
                                                   CsrUint16 logicalLinkHandle,
                                                   const AmpFlowSpec *newTxFlowSpec)
{
    CsrBool status=FALSE;
    logicalLinkAttrib *logicalLink = pal_llm_get_link_attrib(llmContext, logicalLinkHandle);

    sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_flow_spec_modify_acceptable:"));
    sme_trace_info((TR_PAL_LM_LINK_FSM,"TX FlowSpec->maxSduSize -%d, flushTO-%d, accLat-%d, id-%d, interArrTim-%d, servType-%d",
                                        newTxFlowSpec->maximumSduSize,
                                        newTxFlowSpec->flushTimeout,
                                        newTxFlowSpec->accessLatency,
                                        newTxFlowSpec->id,
                                        newTxFlowSpec->sduInterArrivalTime,
                                        GET_SERVICE_TYPE_STR(newTxFlowSpec->serviceType)));
    sme_trace_info((TR_PAL_LM_LINK_FSM,"QosSuuport-%d, requested bandwidth-%d",
                     pal_guranteed_link_supported(context),GET_GUARANTEED_LINK_BANDWIDTH(newTxFlowSpec)));

    /* Service type cannot be changed */
    if (logicalLink && logicalLink->txFlowSpec.serviceType==newTxFlowSpec->serviceType)
    {
        PAL_MibData *mibData=pal_get_common_mib_data(context);

        /* Allow it if its no traffic */
        if (HCI_SERVICE_TYPE_NO_TRAFFIC == newTxFlowSpec->serviceType)
        {
            sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_flow_spec_modify_acceptable: no traffic link acceptable"));
            status = TRUE;
        }
        else if (HCI_SERVICE_TYPE_BEST_EFFORT == newTxFlowSpec->serviceType)
        {
            /* According to AMP Mgr FIPD D05r14 section 5.6
             * "If 'Best effort' is selected then Access Latency and Flush Timeout shall both be set
             * to 0xFFFFFFFF."
             * "Values of 0xFFFFFFFF for Maximum SDU size and SDU Inter-arrival time are used when
             * the actual values are not known. If one value is set to 0xFFFFFFFF then the other value
             * shall also be set to 0xFFFFFFFF."
             */
            if ((0xFFFFFFFF==newTxFlowSpec->flushTimeout && 0xFFFFFFFF==newTxFlowSpec->accessLatency) &&
                ((0xFFFF == newTxFlowSpec->maximumSduSize && 0xFFFFFFFF == newTxFlowSpec->sduInterArrivalTime) ||
                (newTxFlowSpec->maximumSduSize <= PAL_MAX_PDU_SIZE && newTxFlowSpec->sduInterArrivalTime <= PAL_MIN_SDU_INTER_ARRIVAL_TIME)))
            {
                sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_flow_spec_modify_acceptable: best effort link acceptable"));
                status = TRUE;
            }
        }
        else
        {
            /* get new available guranteed bandwidth by decrementing its old requested bandwidth */
            CsrUint8 availableBW = (CsrUint8)(llmContext->availableGuaranteedBandwidth+GET_GUARANTEED_LINK_BANDWIDTH(&logicalLink->txFlowSpec));

            if (newTxFlowSpec->maximumSduSize <= PAL_MAX_PDU_SIZE && /* Must be specified for guaranteed links */
                newTxFlowSpec->flushTimeout <= pal_hip_get_max_flush_timeout(mibData->dot11HCCWmin) &&
                newTxFlowSpec->flushTimeout >= pal_hip_get_min_flush_timeout(mibData->dot11HCCWmin) && /* to ensure atleast one retry is possible */
                (newTxFlowSpec->accessLatency >= pal_hip_get_min_latency(mibData->dot11HCCWmin) ||
                0xFFFFFFFF == newTxFlowSpec->accessLatency) &&
                /* No point checking SDU InterArrival Time as it is checked during bandwidth calculation */
                /*txFlowSpec->sduInterArrivalTime >= PAL_MIN_SDU_INTER_ARRIVAL_TIME => it could be 0xFFFFFFFF for guaranteed latency links s*/
                GET_GUARANTEED_LINK_BANDWIDTH(newTxFlowSpec) <= availableBW
                )
            {
                sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_flow_spec_modify_acceptable: guaranteed link acceptable. Available guranteed bw reduced to-%d",llmContext->availableGuaranteedBandwidth));
                status = TRUE;
            }
        }
    }

    sme_trace_debug_code(
        {
            if (!status)
            {
                sme_trace_debug((TR_PAL_LM_LINK_FSM, "pal_llm_flow_spec_modify_acceptable: Modify flow spec failed"));
            }
        }
    )

    return status;
}

CsrBool pal_llm_update_link_with_modified_flow_spec(logicalLinkManagerContext *llmContext,
                                                            CsrUint16 logicalLinkHandle,
                                                            const AmpFlowSpec *modifiedTxFlowSpec,
                                                            CsrUint32 *flushTimeout)
{
    logicalLinkAttrib *logicalLink = pal_llm_get_link_attrib(llmContext, logicalLinkHandle);

    verify(TR_PAL_LM_LINK_FSM, logicalLink && logicalLink->txFlowSpec.serviceType==modifiedTxFlowSpec->serviceType);

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_update_link_with_modified_flow_spec: allocating mem for old flow spec"));

     *flushTimeout = modifiedTxFlowSpec->flushTimeout;
    if (HCI_SERVICE_TYPE_GUARANTEED == modifiedTxFlowSpec->serviceType)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_update_link_with_modified_flow_spec: Available guranteed BW-%d, Modified BW-%d",
                        llmContext->availableGuaranteedBandwidth,GET_GUARANTEED_LINK_BANDWIDTH(modifiedTxFlowSpec)));
        /* First add the old BW to the total BW available */
        llmContext->availableGuaranteedBandwidth += (CsrUint8)GET_GUARANTEED_LINK_BANDWIDTH(&logicalLink->txFlowSpec);
        llmContext->availableGuaranteedBandwidth -= (CsrUint8)GET_GUARANTEED_LINK_BANDWIDTH(modifiedTxFlowSpec);
    }
    else if (HCI_SERVICE_TYPE_BEST_EFFORT == modifiedTxFlowSpec->serviceType)
    {
        *flushTimeout = logicalLink->bestEfforFlushTimeout; /* in microseconds */
    }
    /* Now overwrite the old flow spec */
    logicalLink->txFlowSpec = *modifiedTxFlowSpec;

    return TRUE;
}


/* function creates the new logical ink entry in the list and saves all the information about
 * the logical link supplied. It also increments the numLogicalLinks parameter by one for the
 * particular physical link
 */
void pal_llm_create_logical_link(logicalLinkManagerContext *llmContext,
                                   CsrUint8 phyLinkHandle,
                                   const AmpFlowSpec *txFlowSpec,
                                   const AmpFlowSpec *rxFlowSpec,
                                   CsrBool qosSupported,
                                   CsrUint16 *logicalLinkHandle,
                                   Priority *userPriority,
                                   CsrUint32 *flushTimeout)
{
    csr_list_node *node,*insertAfterNode;
    logicalLinkAttrib *logicalLink;

    *userPriority = pal_get_user_priority_from_flow_spec(txFlowSpec, qosSupported);

    logicalLink = CsrPmalloc(sizeof(logicalLinkAttrib));
    CsrMemSet(logicalLink, 0, sizeof(logicalLinkAttrib));
    logicalLink->userPriority = *userPriority;
    logicalLink->txFlowSpec = *txFlowSpec;
    logicalLink->rxFlowSpec = *rxFlowSpec;
    logicalLink->logicalLinkHandle = pal_allocate_logical_link_handle(&llmContext->logicalLinkList,&insertAfterNode);
    logicalLink->physicalLinkHandle = phyLinkHandle;

    *flushTimeout = txFlowSpec->flushTimeout;
    /* Update available guranteed bandwidth */
    if (HCI_SERVICE_TYPE_GUARANTEED == txFlowSpec->serviceType)
    {
        sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_create_logical_link: Available guranteed BW-%d, Requested BW-%d",
                        llmContext->availableGuaranteedBandwidth,GET_GUARANTEED_LINK_BANDWIDTH(txFlowSpec)));
        llmContext->availableGuaranteedBandwidth -= (CsrUint8)GET_GUARANTEED_LINK_BANDWIDTH(txFlowSpec);
    }
    else if (HCI_SERVICE_TYPE_BEST_EFFORT == txFlowSpec->serviceType)
    {
        logicalLink->bestEfforFlushTimeout = 0;
        *flushTimeout = logicalLink->bestEfforFlushTimeout; /* in microseconds */
    }

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_create_logical_link"));
    /* insert the node onto the list in the appropriate location */
    node = (csr_list_node *)CsrPmalloc(sizeof(csr_list_node));
    if (NULL==insertAfterNode)
    {
        /* this means either this is the first node in the list or first node have a
         * handle value greater than fist handle value PAL_FIRST_LOGICAL_LINK_HANDLE.
         * So insert as head
         */
        csr_list_insert_tail(&llmContext->logicalLinkList,list_owns_both,node,logicalLink);
    }
    else
    {
        /* otherwise insert it after this node */
        csr_list_insert_after(&llmContext->logicalLinkList,list_owns_both,insertAfterNode,node,(void *)logicalLink);
    }
    sme_trace_info((TR_PAL_LM_LINK_FSM,"pal_llm_create_logical_link(): created handle -%d",logicalLink->logicalLinkHandle));
    *logicalLinkHandle = logicalLink->logicalLinkHandle;
}

CsrBool pal_llm_delete_all_logical_links(FsmContext *context,
                                           logicalLinkManagerContext *llmContext,
                                           CsrUint8 phyLinkHandle,
                                           pal_llm_logical_link_deleted_callback callback)
{
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_delete_all_logical_links: phyHandle-%d",phyLinkHandle));
    return  pal_delete_logical_link_attrib(context, llmContext,
                                           PAL_INVALID_LOGICAL_LINK_HANDLE,
                                           phyLinkHandle,
                                           DELETE_ALL_LINKS,
                                           callback);
}

CsrBool pal_llm_delete_matching_logical_link(FsmContext *context,
                                                 logicalLinkManagerContext *llmContext,
                                                 CsrUint16 logicalLinkHandle)
{
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_delete_matching_logical_link: llHandle-%d",logicalLinkHandle));
    return  pal_delete_logical_link_attrib(context, llmContext,
                                           logicalLinkHandle,
                                           PAL_INVALID_PHYSICAL_LINK_HANDLE,
                                           DELETE_MATCHING_LINK,
                                           NULL);
}

logicalLinkManagerContext * pal_llm_init(CsrUint16 maxLogicalLinks, CsrUint16 maxGuaranteedLogicalLinks)
{
    logicalLinkManagerContext *llmContext = CsrPmalloc(sizeof(logicalLinkManagerContext));
    CsrMemSet(llmContext, 0, sizeof(logicalLinkManagerContext));

    csr_list_init(&llmContext->logicalLinkList);

    if (maxLogicalLinks > PAL_MAX_LOGICAL_LINKS)
    {
        maxLogicalLinks = PAL_MAX_LOGICAL_LINKS;
    }

    if (maxGuaranteedLogicalLinks > PAL_MAX_GUARANTEED_LOGICAL_LINKS)
    {
        maxGuaranteedLogicalLinks = PAL_MAX_GUARANTEED_LOGICAL_LINKS;
    }

    llmContext->maxLogicalLinks = maxLogicalLinks;
    llmContext->maxGuranteedLogicalLinks = maxGuaranteedLogicalLinks;
    llmContext->availableGuaranteedBandwidth = PAL_MAX_GUARANTEED_BANDWIDTH;
    return llmContext;
}

void pal_llm_deinit(logicalLinkManagerContext *llmContext)
{
    csr_list_node *node,*currentNode;

    node = csr_list_gethead_void(&llmContext->logicalLinkList);
    while (node)
    {
        currentNode = node;
        node = csr_list_getnext_void(&llmContext->logicalLinkList,currentNode);
        (void)csr_list_remove(&llmContext->logicalLinkList,currentNode);
    }
    CsrPfree(llmContext);
}

CsrUint16 pal_llm_get_logical_link_handle_from_flow_spec_id(logicalLinkManagerContext *llmContext, CsrUint8 txFlowSpecId)
{
    logicalLinkAttrib *logicalLink = pal_llm_get_link_attrib_from_flow_spec_id(llmContext, txFlowSpecId);
    CsrUint16 logicalLinkHandle=PAL_INVALID_LOGICAL_LINK_HANDLE;

    if (logicalLink)
    {
        logicalLinkHandle = logicalLink->logicalLinkHandle;
    }
    return logicalLinkHandle;
}

CsrUint8 pal_llm_get_phy_link_handle_from_logical_link_handle(logicalLinkManagerContext *llmContext, CsrUint16 logicalLinkHandle)
{
    logicalLinkAttrib *logicalLink = pal_llm_get_link_attrib(llmContext, logicalLinkHandle);

    if (logicalLink && logicalLinkHandle == logicalLink->logicalLinkHandle)
    {
        return logicalLink->physicalLinkHandle;
    }
    return PAL_INVALID_PHYSICAL_LINK_HANDLE;
}

CsrBool pal_llm_link_exits(logicalLinkManagerContext *llmContext, CsrUint16 logicalLinkHandle)
{
    return (pal_llm_get_link_attrib(llmContext, logicalLinkHandle)?TRUE:FALSE);
}

unifi_Status pal_llm_read_best_effor_flush_timeout(logicalLinkManagerContext *llmContext, CsrUint16 logicalLinkHandle, CsrUint32 *bestEfforFlushTimeout)
{
    unifi_Status status=unifi_Error;

    logicalLinkAttrib *logicalLink = pal_llm_get_link_attrib(llmContext, logicalLinkHandle);

    if (logicalLink && logicalLinkHandle == logicalLink->logicalLinkHandle)
    {
        status = unifi_Success;
        *bestEfforFlushTimeout = 0==logicalLink->bestEfforFlushTimeout?0xFFFFFFFF:logicalLink->bestEfforFlushTimeout;
    }
    return status;
}

unifi_Status pal_llm_write_best_effor_flush_timeout(FsmContext *context,
                                                         logicalLinkManagerContext *llmContext, 
                                                         CsrUint16 logicalLinkHandle, 
                                                         CsrUint32 bestEfforFlushTimeout,
                                                         AmpFlowSpec *currentTxFlowSpec)
{
    unifi_Status status=unifi_Error;

    logicalLinkAttrib *logicalLink = pal_llm_get_link_attrib(llmContext, logicalLinkHandle);
    PAL_MibData *mibData=pal_get_common_mib_data(context);

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"pal_llm_write_best_effor_flush_timeout: llHandle-%d, new timeout-%d",logicalLinkHandle,bestEfforFlushTimeout));

    /* Accept the new flusht timeout only its within the acceptable range and the link requested is a BE */
    if (logicalLink && logicalLinkHandle == logicalLink->logicalLinkHandle &&
        HCI_SERVICE_TYPE_BEST_EFFORT == logicalLink->txFlowSpec.serviceType &&
        /* Assume default value (which is 0 or flush timeout not used) if the 
             * flushtTimeout given is 0xFFFFFFFF. Refer 802.11 PAL Spec section 2.8
             */
        (0xFFFFFFFF == bestEfforFlushTimeout ||
        (bestEfforFlushTimeout <= pal_hip_get_max_flush_timeout(mibData->dot11HCCWmin) &&
        bestEfforFlushTimeout >= pal_hip_get_min_flush_timeout(mibData->dot11HCCWmin)))) /* to ensure atleast one retry is possible */
    {
        status = unifi_Success;
        logicalLink->bestEfforFlushTimeout = 0xFFFFFFFF==bestEfforFlushTimeout?0:bestEfforFlushTimeout;
        *currentTxFlowSpec = logicalLink->txFlowSpec;
        currentTxFlowSpec->flushTimeout = logicalLink->bestEfforFlushTimeout;
    }
    return status;
}

AmpFlowSpec *pal_llm_get_tx_flow_spec_from_logical_link_handle(logicalLinkManagerContext *llmContext, 
                                                                       CsrUint16 logicalLinkHandle)
{
    logicalLinkAttrib *logicalLink = pal_llm_get_link_attrib(llmContext, logicalLinkHandle);

    return logicalLink?&logicalLink->txFlowSpec:NULL;
}


/** @}
 */
