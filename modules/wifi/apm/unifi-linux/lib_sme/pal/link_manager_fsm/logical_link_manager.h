/** @file logical_link_manager.h
 *
 * PAL logical link manager API
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
 *   Public logical link manager API
 *
 ****************************************************************************
 *
 * @section MODIFICATION HISTORY
 * @verbatim
 * @endverbatim
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/logical_link_manager.h#2 $
 *
 ****************************************************************************/
#ifndef PAL_LOGICAL_LINK_MANAGER_H
#define PAL_LOGICAL_LINK_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "csr_cstl/csr_wifi_list.h"
#include "smeio/smeio_fsm_types.h"
#include "pal_hci_sap/pal_hci_sap_types.h"


/* PUBLIC MACROS ************************************************************/
#define GET_SERVICE_TYPE_STR(type)     ((type==HCI_SERVICE_TYPE_NO_TRAFFIC)?"No Traffic":((type==HCI_SERVICE_TYPE_BEST_EFFORT)?"Best Effort":"Guaranteed"))


/* PUBLIC TYPES DEFINITIONS *************************************************/

/* Call back supplied by the caller to call when a logical link is deleted */
typedef void (* pal_llm_logical_link_deleted_callback)(FsmContext *context, CsrUint16 logicalLinkHandle);

typedef struct logicalLinkManagerContext logicalLinkManagerContext;

/**
 * @brief
 *   logical link mgr data.
 *
 * @par Description
 *
 */
struct logicalLinkManagerContext
{
    CsrUint16 maxLogicalLinks; /* Total num of logical including both BE and Guranteed */
    CsrUint16 maxGuranteedLogicalLinks; /* Max no of guranteed links. Limit set during compilation */
    CsrUint8 availableGuaranteedBandwidth;
    csr_list logicalLinkList;    /* a list of logical links */

};

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the ip connection manager state machine data
 */

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */
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
extern CsrBool pal_llm_new_logical_link_acceptable(FsmContext *context,
                                                        logicalLinkManagerContext *llmContext,
                                                        const AmpFlowSpec *txFlowSpec,
                                                        CsrBool qosSupported);

/**
  * @brief
  *   create a logical link called when hci-create-logical-link is processed.
  *
  * @par Description
  *
  * @param[in]    llmContext   :  pointer to the logical link manager context
  * @param[in]    phyLinkHandle   : physical link handle
  * @param[in]    txFlowSpec   : Tx flow specification as received from host
  * @param[in]    rxFlowSpec   : Rx flow specification as received from host
  * @param[in]    qosSupported   : boolean that tells if PAL supports guaranteed links
  * @param[in]    logicalLinkHandle   : Valid logical link handle
  * @param[out]    userPriority   : user priority
  * @param[out]    flushTimeout   : flush Timeout

  *
  * @return
  *   number of logical links
  */
extern void pal_llm_create_logical_link(logicalLinkManagerContext *llmContext,
                                          CsrUint8 phyLinkHandle,
                                          const AmpFlowSpec *txFlowSpec,
                                          const AmpFlowSpec *rxFlowSpec,
                                          CsrBool qosSupported,
                                          CsrUint16 *logicalLinkHandle,
                                          Priority *userPriority,
                                          CsrUint32 *flushTimeout);

/**
  * @brief
  *   delete a matching logical links.
  *
  * @par Description
  *
  * @param[in]    context   :  FSM Context
  * @param[in]    llmContext   :  pointer to the logical link manager context
  * @param[in]    logicalLinkHandle   : handle.
  *
  * @return
  *   TRUE if logical link deleed. else FALSE.
  */
extern CsrBool pal_llm_delete_matching_logical_link(FsmContext *context,
                                                        logicalLinkManagerContext *llmContext,
                                                        CsrUint16 logicalLinkHandle);

/**
  * @brief
  *   delete all logical links on a matchingp physical link.
  *
  * @par Description
  *
  * @param[in]    context   :  FSM Context
  * @param[in]    llmContext   :  pointer to the logical link manager context
  * @param[in]    phyLinkHandle   : phy link handle.
  * @param[in]    callback   : callback pointer on a successfull deletion. Call only if its not NULL.
  *
  * @return
  *   TRUE if logical link deleed. else FALSE.
  */
extern CsrBool pal_llm_delete_all_logical_links(FsmContext *context,
                                                  logicalLinkManagerContext *llmContext,
                                                  CsrUint8 phyLinkHandle,
                                                  pal_llm_logical_link_deleted_callback callback);

/**
  * @brief
  *   initialise logical link manager.
  *
  * @par Description
  *
  * @param[in]    maxLogicalLinks   :  maximum number of logical links PAL needs to support
  * @param[in]    maxGuaranteedLogicalLinks   :  Maximum number of guaranteed links PAL needs to support
  *
  * @return
  *   returns the context of the logical link manager
  */
extern logicalLinkManagerContext * pal_llm_init(CsrUint16 maxLogicalLinks, CsrUint16 maxGuaranteedLogicalLinks);

/**
  * @brief
  *   deinit logical link manager
  *
  * @par Description
  *
  * @param[in]    llmContext   :  logical link manager Context
  *
  * @return
  *   TRUE if logical link deleed. else FALSE.
  */
extern void pal_llm_deinit(logicalLinkManagerContext *llmContext);

/**
  * @brief
  *   retrieve logical link handle from flow spec id
  *
  * @par Description
  *
  * @param[in]    llmContext   :  logical link manager Context
  * @param[in]    txFlowSpecId   :  flow spec ID
  *
  * @return
  *   logical link handle
  */
extern CsrUint16 pal_llm_get_logical_link_handle_from_flow_spec_id(logicalLinkManagerContext *llmContext,
                                                                        CsrUint8 txFlowSpecId);

/**
  * @brief
  *   retrieve physical link handle from logical link handle
  *
  * @par Description
  *
  * @param[in]    llmContext   :  logical link manager Context
  * @param[in]    logicalLinkHandle   :  logical link handle
  *
  * @return
  *   physical link handle
  */
extern CsrUint8 pal_llm_get_phy_link_handle_from_logical_link_handle(logicalLinkManagerContext *llmContext,
                                                                          CsrUint16 logicalLinkHandle);

/**
  * @brief
  *   checks if a modify flow spec request from host is acceptable or not
  *
  * @par Description
  *
  * @param[in]    context   :  FSM Context
  * @param[in]    llmContext   :  logical link manager Context
  * @param[in]    logicalLinkHandle   :  logical link handle
  * @param[in]    newTxFlowSpec   :  new flow spec to validate
  *
  * @return
  *   TRUE if new flow spec is acceptable else FALSE.
  */
extern CsrBool pal_llm_flow_spec_modify_acceptable(FsmContext *context,
                                                          logicalLinkManagerContext *llmContext,
                                                          CsrUint16 logicalLinkHandle,
                                                          const AmpFlowSpec *newTxFlowSpec);

/**
  * @brief
  *   update the logical link attribute with the new modified flow spec
  *
  * @par Description
  *
  * @param[in]    llmContext   :  logical link manager Context
  * @param[in]    logicalLinkHandle   :  logical link handle
  * @param[in]    modifiedTxFlowSpec   :  new flow spec to validate
  * @param[out]    flushTimeout   :  new flush timeout based on the flow spec
  *
  * @return
  *   TRUE if new flow spec is acceptable else FALSE.
  */
extern CsrBool pal_llm_update_link_with_modified_flow_spec(logicalLinkManagerContext *llmContext,
                                                                   CsrUint16 logicalLinkHandle,
                                                                   const AmpFlowSpec *modifiedTxFlowSpec,
                                                                   CsrUint32 *flushTimeout);

/**
  * @brief
  *   checks if a link with matching handle exists
  *
  * @par Description
  *
  * @param[in]    llmContext   :  logical link manager Context
  * @param[in]    logicalLinkHandle   :  logical link handle
  *
  * @return
  *   TRUE if link exists else FALSE.
  */
extern CsrBool pal_llm_link_exits(logicalLinkManagerContext *llmContext, CsrUint16 logicalLinkHandle);


/**
  * @brief
  *   read best effort flush timeout for link
  *
  * @par Description
  *
  * @param[in]    llmContext   :  logical link manager Context
  * @param[in]    logicalLinkHandle   :  logical link handle
  * @param[out]    bestEfforFlushTimeout   : pointer flush timeout value
  *
  * @return
  *   unifi_Success if link exists else unifi_Error.
  */
extern unifi_Status pal_llm_read_best_effor_flush_timeout(logicalLinkManagerContext *llmContext,
                                                          CsrUint16 logicalLinkHandle,
                                                          CsrUint32 *bestEfforFlushTimeout);

/**
  * @brief
  *   validates and saves best effort flush timeout for link. The function will return the current flow spec if the new timeout
  * value is acceptable. The flow spec may be used by LM to configure data manager with the new flushtimeout for BE links.
  *
  * @par Description
  *
  * @param[in]    context   :  FSM Context
  * @param[in]    llmContext   :  logical link manager Context
  * @param[in]    logicalLinkHandle   :  logical link handle
  * @param[in]    bestEfforFlushTimeout   :  flush timeout value
  * @param[out]    currentTxFlowSpec   :  tx flow spec for the link configured to be returned. It can be used to configure data manager
  *
  * @return
  *   unifi_Success if link exists else unifi_Error.
  */
extern unifi_Status pal_llm_write_best_effor_flush_timeout(FsmContext *context,
                                                                logicalLinkManagerContext *llmContext, 
                                                                CsrUint16 logicalLinkHandle, 
                                                                CsrUint32 bestEfforFlushTimeout,
                                                                AmpFlowSpec *currentTxFlowSpec);


/**
  * @brief
  *   get tx flow spec from logical link handle
  *
  * @par Description
  *
  * @param[in]    llmContext   :  logical link manager Context
  * @param[in]    logicalLinkHandle   :  logical link handle
  *
  * @return
  *  pointer to tx flow spec if link exists else NULL.
  */
extern AmpFlowSpec *pal_llm_get_tx_flow_spec_from_logical_link_handle(logicalLinkManagerContext *llmContext, 
                                                                              CsrUint16 logicalLinkHandle);

#ifdef __cplusplus
}
#endif

#endif /* PAL_LOGICAL_LINK_MANAGER_H */
