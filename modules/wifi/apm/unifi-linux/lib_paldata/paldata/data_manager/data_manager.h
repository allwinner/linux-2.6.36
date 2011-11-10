/** @file data_manager.h
 *
 * PAL Data Manager
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
 *   Public API for data manager.
 *
 ****************************************************************************
 *
 * @section MODIFICATION HISTORY
 * @verbatim
 *   #1    17:Apr:08 B-36899: Created
 * @endverbatim
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_paldata/paldata/data_manager/data_manager.h#4 $
 *
 ****************************************************************************/

#ifndef PAL_DATA_MANAGER_H
#define PAL_DATA_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup data_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "paldata_top_level_fsm/paldata_top_level_fsm.h"
#include "hostio/hip_fsm_events.h"

/* PUBLIC MACROS ************************************************************/
#define PALDATA_SNAP_HEADER_SIZE (8)
#define PALDATA_ACL_HEADER_SIZE (4)
#define PALDATA_MAC_ADDRESS_SIZE (6)

/* ACL PREALLOCATED OFFSET :
  * Called of this function should preallocated PALDATA_ACL_OFFSET bytes
  *  6 -> Destination MAC address
  *  6 -> Source MAC Address
  *  8 -> SNAP Header
  * -4 -> less ACL Header
  */
#define PALDATA_ACL_OFFSET (6+6+8-4)

#define PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,dataPtr) \
do { \
    if (dataPtr) \
    { \
        if (freeFunction) \
        { \
            (*(freeFunction))((void*)(dataPtr)); \
        }\
        else \
        { \
            CsrPfree((void*)(dataPtr)); \
        } \
    } \
}while(0)

/* To get memory stuff portatble */
#define PALDATA_MEM_DOWNSTREAM_ADD_SNAP_HEADER_AND_MAC_ADDRESS(dataBlock, size) pal_push_data_block(dataBlock,size)

/* free data block from L2CAP to PAL-D*/
#define PALDATA_MEM_DOWNSTREAM_FREE_DATA_BLOCK(dataBlock) pal_free_data_block((PAL_DataBlock *)dataBlock)

/* add MA_UNITDATA_REQ to the data block */
#define PALDATA_MEM_DOWNSTREAM_ADD_UNITDATA_SIGNAL(dataBlock, size) pal_push_data_block((PAL_DataBlock *)dataBlock, size)

#define PALDATA_MEM_DOWNSTREAM_REMOVE_ACL_HEADER(dataBlock, size) pal_pull_data_block((PAL_DataBlock *)dataBlock, size)
#define PALDATA_MEM_DOWNSTREAM_GET_ACL_BUFFER(dataBlock, size) pal_pull_data_block((PAL_DataBlock *)dataBlock, size)


/* get the pointer to the first byte in this data block*/
#define PALDATA_MEM_GET_BUFFER(dataBlock) (((PAL_DataBlock *)dataBlock)->pData)
/* Get the length of this data block) */
#define PALDATA_MEM_GET_BUFFER_LENGTH(dataBlock) (((PAL_DataBlock *)dataBlock)->length)

/* Remove MA-Unitdata-Ind signal upto the bulkdata */
#define PALDATA_MEM_UPSTREAM_REMOVE_UNITDATA_SIGNAL(dataBlock,size) pal_pull_data_block((PAL_DataBlock *)dataBlock,size)

/* Remove the SNAP header */
#define PALDATA_MEM_UPSTREAM_REMOVE_SNAP_HEADER(dataBlock,size) pal_pull_data_block((PAL_DataBlock *)dataBlock, size)

/* Remove the MAC Addresses */
#define PALDATA_MEM_UPSTREAM_REMOVE_MAC_ADDRESS(dataBlock,size) pal_pull_data_block((PAL_DataBlock *)dataBlock, size)

/* Add the ACL header */
#define PALDATA_MEM_UPSTREAM_ADD_ACL_HEADER(dataBlock,size) pal_push_data_block((PAL_DataBlock *)dataBlock, size)

/* free data block SYS to PAL-D */
#define PALDATA_MEM_UPSTREAM_FREE_DATA_BLOCK(dataBlock) pal_free_data_block((PAL_DataBlock *)dataBlock)

/* Allocated a downstream data block -- L2CAP-PAL-D ACL data*/
#define PALDATA_MEM_ALLOCATE_DOWNSTREAM_DATA_BLOCK(len,dataBlock,data,blockOnly,releaseData) pal_alloc_data_block(len,(CsrUint8 *)data,blockOnly,releaseData)
/* Allocated a upstream data block -- SYS-PAL-D ACL data*/
#define PALDATA_MEM_UPSTREAM_ALLOCATE_DATA_BLOCK(len,dataBlock,data,blockOnly,releaseData) pal_alloc_data_block(len,(CsrUint8 *)data,blockOnly,releaseData)

#define PALDATA_GET_TOTAL_DATA_BLOCK_LENGTH(dataBlock) (((PAL_DataBlock *)dataBlock)->blockLength)

/* These macros can be used by Synergy function to create a block and reuse the data memory for downstream (L2CAP --> PAL-D)
 * An example use will be
 * PAL_DataBlock *dataBlock = PALDATA_MEM_DOWNSTREAM_SYNERGY_DATA_BLOCK(dataLength+aclOffset, NULL, data, TRUE, NULL);
 */
#define PALDATA_MEM_DOWNSTREAM_SYNERGY_DATA_BLOCK(len,dataBlock,data,blockOnly,releaseData) pal_alloc_data_block(len,dataBlock,(CsrUint8 *)data,blockOnly,releaseData)

/* These macros can be used by Synergy function to create a block and reuse the data memory for  upstream(SYS --> PAL-D)
 * An example use will be
 * PAL_DataBlock *dataBlock = PALDATA_MEM_UPSTREAM_SYNERGY_DATA_BLOCK(frameLength, NULL, frame, TRUE, NULL);
 * Assuming frameLength is SNAP+Payload.
 */
#define PALDATA_MEM_UPSTREAM_SYNERGY_DATA_BLOCK(len,dataBlock,data,blockOnly,releaseData) pal_alloc_data_block(len,dataBlock,(CsrUint8 *)data,blockOnly,releaseData)

/* Get the original data pointer and length to this block when the block was created */
#define PALDATA_MEM_GET_BUFFER_ORIGINAL(dataBlock) (((PAL_DataBlock *)dataBlock)->pDataOrig)
#define PALDATA_MEM_GET_BUFFER_LENGTH_ORIGINAL(dataBlock) (((PAL_DataBlock *)dataBlock)->lengthOrig)


/* PUBLIC TYPES DEFINITIONS *************************************************/

typedef void (*PALData_freeFunction)(void *ptr);

typedef struct dataManagerContext dataManagerContext;

typedef struct llc_snap_hdr_t
{
    CsrUint8    dsap;   /* always 0xAA */
    CsrUint8    ssap;   /* always 0xAA */
    CsrUint8    ctrl;   /* always 0x03 */
    CsrUint8    oui[3];    /* organizational universal id */
    CsrUint16 protocol;
}llc_snap_hdr_t;

/* mBlk structure used in data path by PAL */
typedef struct PAL_DataBlock
{
     CsrUint8*      pData;      /* pointer to start of data */
     void*       pRelease;   /* pointer to address to give to Free() if buffer is to be released. If NULL data need not be released */
     CsrUint8       *pDataOrig; /* Pointer to the original data */
     CsrUint16      lengthOrig; /* Original length pointed by pDataOrig */
     CsrUint16      length;     /* length of data (in bytes) pointed by pData */
} PAL_DataBlock;

/* These  are duplicated from pal_data_common.h inorder to strip data-manager out of pal-sme */
/* AMP Protocol Defines - section 5.1 802.11 AMP DIPD D07r15*/
typedef enum PALDam_DataProtocolId
{
    PALDAM_DATA_PROTO_L2CAP_ID = 0x0001,
    PALDAM_DATA_PROTO_ACTIVITY_REPORT_ID = 0x0002,
    PALDAM_DATA_PROTO_SECURITY_FRAMES_ID = 0x0003
}PALDam_DataProtocolId;

typedef struct PalAclheader
{
    CsrUint16 handlePlusFlags;
    CsrUint16 length;
}PalAclheader;

/***************** END ***************************/



/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/


/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   process acl data from the host
 *
 * @par Description
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    dataBlock   : Client context
 * @param[in]    aclHeader   : Client context
 * @param[in]    freeFunction   : function pointer to be called to free the data memory
 *
 * @return
 *   returns the next timeout value in microseconds if there was timer started during the call. Useful to take action by caller
 * if there was a timer started.
 */
extern CsrUint32 pal_acl_process_downstream_data(FsmContext *context,
                                                    dataManagerContext *damContext,
                                                    PAL_DataBlock *dataBlock,
                                                    PalAclheader *aclHeader,
                                                    unifi_FrameFreeFunction freeFunction);

/**
 * @brief
 *    process the ma-unitdata-ind from HIP
 *
 * @par Description
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    dataBlock   : data block (points to the bulkdata)
 * @param[in]    freeFunction   : pointer to free the data
 *
 * @return
 *   void
 */
extern void pal_acl_process_upstream_data(FsmContext *context,
                                                dataManagerContext *damContext,
                                                PAL_DataBlock *dataBlock,
                                                unifi_FrameFreeFunction freeFunction);

/**
 * @brief
 *    process ma-unitdata-cfm
 *
 * @par Description
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    result  :  result from the router
 * @param[in]    transmissionStatus  :  transmission status
 * @param[in]    providedPriority  :  priority requested on transmission of this packet
 * @param[in]    providedServiceClass  :  service class requested on transmission of this packet
 * @param[in]    reqIdentifier  :  identifier requested on transmission of this packet

 *
 * @return
 *   returns the next timeout value if there was timer started during the call. Useful to take action by caller
 * if there was a timer started.
 */
extern CsrUint32 pal_dam_process_ma_unitdata_cfm(FsmContext *context,
                                              dataManagerContext *damContext,
                                              unifi_Status result,
                                              unifi_TransmissionStatus transmissionStatus,
                                              unifi_Priority providedPriority,
                                              unifi_ServiceClass providedServiceClass,
                                              CsrUint32 reqIdentifier);

/**
 * @brief
 *    update the queue parameter during initialisation.
 *
 * @par Description
 *    These parameters are updated during the data manager initialisation.  The function will determine
 * the size it will use for the traffic queue based on the supplied parameters.
 *
 * @param[in]    damContext   : data manager Context
 * @param[in]    trafficQueueSize   : traffic queue size in the driver
 * @param[in]    commandQueueSize   : command queue size in the driver
 *
 * @return
 *   the size of the traffic queue selected.
 */
extern CsrUint16 pal_dam_init_queue_info(dataManagerContext *damContext,
                                           CsrUint16 trafficQueueSize,
                                           CsrUint16 commandQueueSize);

/**
 * @brief
 *     create a data link in the data manager.
 *
 * @par Description
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    phyLinkHandle   :physical link handle
 * @param[in]    txFlowSpec   : Tx flow spec
 * @param[in]    logicalLinkHandle   : logical link handle
 * @param[in]    userPriority   : user priority
 * @param[in]    remoteMacAddress   : remote MAC Address
 * @param[in]    localMacAddress   : local MAC Address
 *
 * @return
 *   void
 */
extern void pal_dam_create_logical_link_entry(FsmContext *context,
                                                   dataManagerContext *damContext,
                                                   CsrUint8 phyLinkHandle,
                                                   const unifi_AmpFlowSpec *txFlowSpec,
                                                   CsrUint16 logicalLinkHandle,
                                                   unifi_Priority userPriority,
                                                   const unifi_MACAddress *remoteMacAddress,
                                                   const unifi_MACAddress *localMacAddress);

/**
 * @brief
 *    Modify an existing data link
 *
 * @par Description
 *
 * @param[in]    context      : FSM Context
 * @param[in]    damContext   : data manager Context
 * @param[in]    logicalLinkHandle   : logical link handle
 * @param[in]    modifiedTxFlowSpec   : Tx flow spec modified
 *
 * @return
 *   void
 */
extern void pal_dam_modify_logical_link_entry(FsmContext *context,
                                              dataManagerContext *damContext,
                                              CsrUint16 logicalLinkHandle,
                                              const unifi_AmpFlowSpec *modifiedTxFlowSpec);

/**
 * @brief
 *    Delete a matching data link
 *
 * @par Description
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    logicalLinkHandle   : logical link handle
 * @param[in]    pal_dam_logical_link_deleted_callback   : callback once deletion is complete
 *
 * @return
 *   void
 */
extern CsrBool pal_dam_delete_matching_logical_link(FsmContext *context,
                                                          dataManagerContext *damContext,
                                                          CsrUint16 logicalLinkHandle,
                                                          void (*pal_dam_logical_link_deleted_callback)(FsmContext *,CsrUint16,CsrUint8)); /*lint !e955    pal_dam_logical_link_deleted_callback */

/**
 * @brief
 *    reset failed contact counter
 *
 * @par Description
 *
 * @param[in]    damContext   : data manager Context
 * @param[in]    logicalLinkHandle   : logical link handle
 *
 * @return
 *   return TRUE if successful else returns FALSE
 */
extern CsrBool pal_dam_reset_failed_contact_counter(dataManagerContext *damContext, CsrUint16 logicalLinkHandle);

/**
 * @brief
 *    read failed contact counter
 *
 * @par Description
 *
 * @param[in]    damContext   : data manager Context
 * @param[in]    logicalLinkHandle   : logical link handle
 * @param[out]    failedContactCounter   : variable to save the current value of failed contact counter
 *
 * @return
 *   return TRUE if successful else returns FALSE
 */
extern CsrBool pal_dam_read_failed_contact_counter(dataManagerContext *damContext,
                                                          CsrUint16 logicalLinkHandle,
                                                          CsrUint16 *failedContactCounter);

/**
 * @brief
 *    function to set the hci event masks.
 *
 * @par Description
 *
 * @param[in]    damContext   : data manager Context
 * @param[in]    qosViolationEventEnabled   : boolean to set event status for HCI_QOS_VIOLATION
 * @param[in]    numCompletedDataBlocksEventEnabled   : boolean to set event status for HCI_NUMBER_OF_COMPLETED_DATA_BLOCKS
 *
 * @return
 *   void
 */
extern void pal_dam_set_event_mask(dataManagerContext *damContext,
                                          CsrBool qosViolationEventEnabled,
                                          CsrBool numCompletedDataBlocksEventEnabled);


/**
 * @brief
 *    process a flush request from the PAL control.
 *
 * @par Description
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    logicalLinkHandle   : logical link handle
 * @param[in]    cbFlushOccuredFinalResponse   : callback when flush is completed
 *
 * @return
 *   void
 */
extern void pal_dam_process_flush_req(FsmContext *context,
                                            dataManagerContext *damContext,
                                            CsrUint16 logicalLinkHandle,
                                            void(*cbFlushOccuredFinalResponse)( FsmContext *, CsrUint16, CsrBool)); /*lint !e955    cbFlushOccuredFinalResponse */

/**
 * @brief
 *    process flush timer expiry
 *
 * @par Description
 *   Flush all packets that are expired on this logical link handle
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    logicalLinkHandle   : logical link handle
 * @param[in]    cbFlushOccuredFinalResponse   : callback when flush is completed
 *
 * @return
 *   void
 */
extern void pal_dam_process_flush_timeout(FsmContext *context,
                                                 dataManagerContext *damContext,
                                                 CsrUint16 logicalLinkHandle,
                                                 void(*cbFlushOccuredFinalResponse)( FsmContext *, CsrUint16, CsrBool)); /*lint !e955    cbFlushOccuredFinalResponse */

/**
 * @brief
 *    set link supervision timeout value for a physical link.
 *
 * @par Description
 *    If the value set is zero then the timer will not be started. This is to provide a mechansim
 * to disable the timer for test purposes.
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    phyLinkHandle   : phsyical link handle
 * @param[in]    linkSupervisionTimeoutDuration   : timeout value
 * @param[out]    firstPhyLink   : TRUE if this is the first physical link in the data manager. else FALSE.
 *
 * @return
 *   void
 */
extern void pal_dam_set_link_supervision_timeout(FsmContext *context,
                                                        dataManagerContext *damContext,
                                                        CsrUint8 phyLinkHandle,
                                                        CsrUint16 linkSupervisionTimeoutDuration,
                                                        CsrBool *firstPhyLink);

/**
 * @brief
 *    modify link supervision timeout value for a physical link.
 *
 * @par Description
 *    If the value set is zero then the timer will not be started. This is to provide a mechansim
 * to disable the timer for test purposes.
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    phyLinkHandle   : phsyical link handle
 * @param[in]    linkSupervisionTimeoutDuration   : modified timeout value
 *
 * @return
 *   void
 */
extern void pal_dam_modify_link_supervision_timeout(FsmContext *context,
                                                            dataManagerContext *damContext,
                                                            CsrUint8 phyLinkHandle,
                                                            CsrUint16 linkSupervisionTimeoutDuration);

/**
 * @brief
 *    delete link supervision timeout value for a physical link.
 *
 * @par Description
 *    If the value set is zero then the timer will not be started. This is to provide a mechansim
 * to disable the timer for test purposes.
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    phyLinkHandle   : phsyical link handle
 * @param[out]    lastPhyLink   : TRUE if this is the last physical link in the data manager. else FALSE.
 *
 * @return
 *   void
 */
extern void pal_dam_delete_link_supervision_timeout(FsmContext *context,
                                                           dataManagerContext *damContext,
                                                           CsrUint8 phyLinkHandle,
                                                           CsrBool *lastPhyLink);

/**
 * @brief
 *    function to tell data manager that the physical link is still alive.
 *
 * @par Description
 * This API is intended for the control side to the data manager that link is still alive
 * as there had been interaction in the control side with the peer.
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    phyLinkHandle   : phsyical link handle
 *
 * @return
 *   void
 */
extern void pal_dam_link_alive(FsmContext *context,
                                  dataManagerContext *damContext,
                                  CsrUint8 phyLinkHandle);

/**
 * @brief
 *   handle early link supervision timer expiry
 *
 * @par Description
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    phyLinkHandle   : phsyical link handle
 *
 * @return
 *   returns FALSE means signals data manager has not heard from the remote device in this time.
 * Caller generates signal to let pal-c know about early link loss. TRUE means everything is fine, link is
 * still alive
 * 
 */
extern CsrBool pal_dam_early_link_loss_timer_expired(FsmContext *context,
                                                           dataManagerContext *damContext,
                                                           CsrUint8 phyLinkHandle);

/**
 * @brief
 *    handle link supervision timer expiry.
 *
 * @par Description
 *
 * @param[in]    context   : Client context
 * @param[in]    damContext   : data manager Context
 * @param[in]    phyLinkHandle   : phsyical link handle
 *
 * @return
 *   returns FALSE means signals data manager has not heard from the remote device in this time.
 * Caller generates signal to let pal-c know about early link loss. TRUE means everything is fine, link is
 * still alive

 */
extern CsrBool pal_dam_link_loss_timer_expired(FsmContext *context,
                                                     dataManagerContext *damContext,
                                                     CsrUint8 phyLinkHandle);

/**
 * @brief
 *    data manager initialisation
 *
 * @par Description
 *
 * @param[in]    damContext   : data manager Context
 * @param[in]    cbFlushOccuredEvt   : callback for HCI-flush-Occurred event
 * @param[in]    cbQosViolationEvt   : callback for HCI-Qos-Violation event
 * @param[in]    cbNumCompletedDataBlocksEvt   : callback for HCI-Number-Completed-Data-Blocks event
 *
 * @return
 *   void
 */
extern void pal_dam_data_init(dataManagerContext **damContext);

/**
 * @brief
 *    deinitialise data manager
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 * @param[in]    damContext   : data manager Context
 * @param[in]    shutdown   : boolean if set to TRUE will free the context itself.  Otherwise will clear up all
 *                                            data links, timers and reset variables. But retain the context memory.
 *
 * @return
 *   void
 */
extern void pal_dam_deinit(FsmContext *context, dataManagerContext *damContext, CsrBool shutdown);

/**
 * @brief
 *    encode snap header.
 *
 * @par Description
 *
 * @param[in]    buf   : buffer to save encoded header. memory allocated by caller
 * @param[in]    snap   : snap header information
 *
 * @return
 *   void
 */
extern void pal_encode_snap(CsrUint8 *buf, llc_snap_hdr_t *snap);

/**
 * @brief
 *    decode snap header.
 *
 * @par Description
 *
 * @param[in]    buf   : buffer with encode snap header
 * @param[in]    snap   : pointer to structure to save decoded snap header. Memory allocated by calller
 *
 * @return
 *   void
 */
extern void pal_decode_snap(CsrUint8 *buf, llc_snap_hdr_t *snap);


/**
 * @brief
 *    save subscription handle.
 *
 * @par Description
 *
 * @param[in]    damContext   : Data manager context
 * @param[in]    subscriptionHandle   : subscription to be used in Paldata-Sys-Ma-Unitdata-Req/Paldata-sys-ma-unitdata-rsp
 * @param[in]    allocOffset   : offset in the frame to the payload
 *
 * @return
 *   void
 */
extern void pal_dam_set_subscription_handle_and_offset(dataManagerContext *damContext,
                                                               CsrUint8 subscriptionHandle,
                                                               CsrUint16 allocOffset);

#ifdef FSM_DEBUG_DUMP

/**
 * @brief
 *
 *
 * @par Description
 *
 * @param[in]    damContext   : data manager Context
 *
 * @return
 *   void
 */
extern void pal_dm_dump(dataManagerContext *damContext);
#endif

/* Data block management routines */


/**
 * @brief
 *    allocate data block for a downstream ACL packet
 *
 * @par Description
 *
 * @param[in]    len   : length of data to allocate
 * @param[in]    data   : pointer to data if pre-allocated. If blockOnly if TRUE then data from *data is copied. Else the pointer is stored.
 * @param[in]    blockOnly   : TRUE means only datablock is allocated. FALSE means a data memory of length len will be allocated in pData and *data is copied if provided
 * @param[in]    releaseData   : if supplied means CsrPfree() will be called to free releaseData pointer. Else ignored. Valid only if blockOnly is TRUE.
 *
 * Note: Memory for the dataBlock structure and the actual data of length 'len' is allocated
 * by the function.
 *
 * @return
 *   return pointer to the head data block which will be the new block.
 */
extern PAL_DataBlock *pal_alloc_data_block(CsrUint16 len, CsrUint8 *data, CsrBool blockOnly, CsrUint8 *releaseData);

/**
 * @brief
 *    push data block by length bytes.
 *
 * @par Description
 *
 * @param[in/out]    dataBlock   : data block to pushed. 
 * @param[in]    length   : offset to be pushed.
 *
 * @return
 *   returns the pointer to the data after the push operation
 */
extern CsrUint8 *pal_push_data_block(PAL_DataBlock *dataBlock, CsrUint16 length);

/**
 * @brief
 *    pull data block by length bytes.
 *
 * @par Description
 *
 * @param[in/out]    dataBlock   : data block to pulled.
 * @param[in]    length   : offset to be pulled.
 *
 * @return
 *   returns the pointer to the data after the pull operation
 */
extern CsrUint8 *pal_pull_data_block(PAL_DataBlock *dataBlock, CsrUint16 length);

/**
 * @brief
 *    free the data block
 *
 * @par Description
 *    The actual data memory is freed only if pRelease flag is set to TRUE.
 * This gives flexibility to the caller on the memory in use.
 *
 * @param[in]    dataBlock   : data block to be freed
 *
 * @return
 *   void
 */
extern void pal_free_data_block(PAL_DataBlock *dataBlock);

/**
 * @brief
 *    function that can be assigned to freeFucnction pointer in data path. Primarily for testing
 *
 * @par Description
 *
 *
 * @param[in]    ptr   : pointer to be freed
 *
 * @return
 *   void
 */
extern void pal_free_data_buffer(void* ptr);
/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PAL_DATA_MANAGER_H */
