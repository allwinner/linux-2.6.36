/** @file data_manager.c
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
 *   This module provides the library for PAL data manager. The module does not
 * consider reentrancy. It is upto the caller to ensure that calls are thread safe.
 *
 ****************************************************************************
 *
 * @section MODIFICATION HISTORY
 * @verbatim
 *   #1    17:Apr:08 B-36899: Created
 * @endverbatim
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_paldata/paldata/data_manager/data_manager.c#10 $
 *
 ****************************************************************************/

/** @{
 * @ingroup data_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "paldata_top_level_fsm/paldata_top_level_fsm.h"

#include "fsm/csr_wifi_fsm.h"
#include "csr_cstl/csr_wifi_list.h"

#include "fsm/fsm_internal.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "smeio/smeio_trace_types.h"

#include "pal_hci_sap/pal_hci_sap_up_pack.h"
#include "event_pack_unpack/event_pack_unpack.h"
#include "data_manager/data_manager.h"
#include "data_manager/data_manager_fsm_types.h"
#include "data_manager/data_manager_fsm_events.h"
#include "paldata_sys_sap/paldata_sys_sap_from_sme_interface.h"
#include "paldata_acl_sap/paldata_acl_sap_from_sme_interface.h"


/* MACROS *******************************************************************/

        /* Section 5.4.2 of HCI Spec:
        * BC is bits 00 : Point to point
        * PB is bits 11 : A complete L2CAP PDU. Automatically flushable.Shall be used with an AMP Controller.
        */
        /* pack according to section 5.2: "Where bit strings are specified, the low order bit is the right hand bit, e.g. 0 is
        * the low order bit in 10."
        */
#define PALDATA_HANDLE_PLUS_FLAGS(handle) (0x3000|(handle))

/* Max physical links supports. This value must be the same as what is defined for the PAL control side
 * (in pal_data_common.h). Any change in the control side should be reflected here.
 */
#define PAL_DAM_MAX_PHYSICAL_LINKS (2)
/* supports 2 Access Classes for  AC0 & AC4/7
 */
#define PAL_DAM_MAX_NUM_OF_QOS (2)
#define PAL_DAM_GET_HANDLE(handlePlusFlag) (handlePlusFlag&0x0FFF)
#define PAL_DAM_GET_QOS_INDEX(usrp) ((unifi_PriorityQosUp4==usrp || unifi_PriorityQosUp7==usrp)?0:1) /* only one user priority supported now */
#define GET_FLUSH_TIMEOUT(llink) (llink->txFlowSpec.flushTimeout)
#define GET_SERVICE_TYPE_STR(type)     ((type==unifi_PalServicetypeNoTraffic)?"No Traffic":((type==unifi_PalServicetypeBestEffort)?"Best Effort":"Guaranteed"))

/* Subtract two time values.*/
#define GET_CURRENT_TIME_IN_MICROS() CsrGetTime()
#define TIME_DIFF(startTime,endTime) (CsrUint32)CsrTimeSub(endTime,startTime)
#define TIME_ADD(time1,time2) CsrTimeAdd(time1, time2)

#define TX_UPDATE_CREDIT_INFO(crdInfo)  (crdInfo)->numFreeDataBlocks--;
/* Oldest is head of queue */
#define TX_PKT_DELETE_OLDEST_PACKET_IN_LIST(logicalLink) csr_list_removehead(&logicalLink->txPacketList)
/*oldest is the head of queue */
#define TX_PKT_GET_OLDEST_PACKET_IN_LIST(logicalLink) ((logicalLink)?(PAL_DamTxPacketInfo *)(csr_list_gethead_void(&logicalLink->txPacketList)->value):NULL)
#define TX_PKT_INFO_VALID(pkt) (((pkt)&&(pkt)->logicalLink)?TRUE:FALSE)
#define TX_PKT_START_TIME(txPktInfo) (CsrTimeSub((txPktInfo)->expiryTime,GET_FLUSH_TIMEOUT((txPktInfo)->logicalLink)))
#define TX_PKT_TIME_TAKEN(txPktInfo,timeNow) (TIME_DIFF(TX_PKT_START_TIME(txPktInfo),timeNow))
#define TX_PKT_NOT_EXPIRED(txPktInfo, timeNow) CsrTimeGt((txPktInfo)->expiryTime, (timeNow))
/* max tag is twice the credit size to avoid any overlap */
#define TX_PKT_MAX_PACKET_TAG_NUMBER(damContext,logicalLink) (damContext->QosCreditInfo[PAL_DAM_GET_QOS_INDEX(logicalLink->userPriority)].totalDataBlocks*2)

/* These flags are duplicated from pal_data_common.h inorder to strip data-manager out of pal-sme */
#define PAL_INVALID_LOGICAL_LINK_HANDLE (0xFFF)
#define PAL_INVALID_PHYSICAL_LINK_HANDLE (0)
#define PAL_MAX_PDU_SIZE (1492) /*as per clause 6 of PAL spec */

#define MIN_EARLY_LINK_LOSS_TIMEOUT_DURATION (1000000) /*in microseconds.*/
#define EARLY_LINK_LOSS_FRACTION (2) /*a fraction of remaining duration for link loss. 2 means 1/2 times the remaining duration for link loss*/
/* link loss time in microseconds */
#define GET_EARLY_LINK_LOSS_TIMEOUT_VALUE(linkLossRemainingDuration) ((linkLossRemainingDuration)/EARLY_LINK_LOSS_FRACTION)

/* Req Id is 32bits in size. Below describes how it is used in PAL-D
  * request Identifier consists 16 bits of logical link handle in least 2 bytes and 16 bits of packetTag in most significant 2 bytes.
  * 31                    27                                              15                                                              0
  * |--App handle--|--Logical Link Handle (12 bits)--|-----------Packet Tag (16 bits)---------|
  * Combination of the two will make the identifier a unique number across data links
  */
#define GENERATE_REQ_IDENTIFIER_FOR_TX_PAKCET(txPktInfo) ((UNIFI_PALDATA_APPHANDLE << 28) | (txPktInfo->logicalLink->logicalLinkHandle << 16) | txPktInfo->packetTag)
#define GET_PACKET_TAG_FROM_REQ_IDENTIFIER(reqIdentifier) (CsrUint16)(reqIdentifier & 0x0000FFFF)
#define GET_LINK_HANDLE_FROM_REQ_IDENTIFIER(reqIdentifier) (CsrUint16)((reqIdentifier & 0x0FFF0000) >> 16)
#define GET_APPHANDLE_FROM_REQ_IDENTIFIER(reqIdentifier) ((reqIdentifier & 0xF0000000) >> 28)

/* Set this macro to appropriate value to enable batching of hci-number-of-completed-blocks.  */
#ifndef CSR_WIFI_PALDATA_BLOCK_ACK_COUNT_MAX
#define CSR_WIFI_PALDATA_BLOCK_ACK_COUNT_MAX (1)
#endif

/********** END ***********************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

typedef enum deleteLinkType
{
    DELETE_ALL_LINKS,
    DELETE_MATCHING_LINK
}deleteLinkType;

typedef struct linkSupervisionTimerEntry
{
    CsrUint32 timeoutDuration; /* in microseconds */
    CsrUint8 physicalLinkHandle;
    /* Timer running with a % duration of full LSTO.
     * On expriy will generate LINK SUPERVISION REQUEST
     */
    FsmTimerId  earlyLinkLossTimer;

    /* Timer running with a % duration of full LSTO.
     * On expriy will generate LINK LOST indication to PAL Ctrl
     */
    FsmTimerId  linkLossTimer;

    /* link loss expiry time */
    CsrUint32 expiryTime;

    /* flag set to TRUE if the timer needs to be restarted after expriy. 
    * It is set to TRUE when ma-unitdata-ind is received so that it avoids stopping and
    * starting the timer
    */
    CsrBool linkAlive;
}linkSupervisionTimerEntry;

/* Structure contains information regarding the usage of the
 * traffic queues. This is used to inform host about the availability
 * of free blocks in the AMP Controller
 */
typedef struct creditInfo
{
    CsrUint16 totalDataBlocks;
    CsrUint16 numFreeDataBlocks; /* Free blocks. Maps to free Slots in traffic queue. For BE each ma-unitdata-status-ind will denote a free slot in driver queue   */
}creditInfo;

typedef struct pdamLogicalLinkAttrib
{
    CsrUint16 logicalLinkHandle; /* link handle created by PAL */
    CsrUint8 physicalLinkHandle; /* link handle created by PAL */
    unifi_MACAddress remoteMacAddress;
    unifi_MACAddress localMacAddress;
    unifi_Priority userPriority;
    unifi_AmpFlowSpec txFlowSpec;
    CsrUint16 failedContactCounter;
    CsrUint16 flushPendingCount; /* count of number of packets pending to be flushed. This could be due to flush request or automatic flush */
    CsrBool flushAllPacketsPending; /* TRUE if all packets needs to be flushed. Set on receiving flush_req or delete_link_req*/

    /* linear list of packets pending the result of transmission for this logical link.
        * The list is FIFO. new packets go into the tail. Acknowledged packets are removed
        * from the head.
        */
    csr_list txPacketList;
    CsrBool flushOccured; /* TRUE if atleast one packet was flushed */
    void(*cbFlushOccuredFinalResponse)( FsmContext *, CsrUint16, CsrBool); /*lint !e955    cbFlushOccuredFinalResponse */
    FsmTimerId  flushTimerId;
    void (* cbLogicalLinkDeleted)(FsmContext *, CsrUint16,CsrUint8); /* called just before the link is deleted. NULL by default*/ /*lint !e955    cbLogicalLinkDeleted */

    /* points to the memory where details of link supervision timer is stored for this physical link.
         * Saves another looping to get this info when a packet is received from peer. This pointer is always non-null
         * physical links are the last ones to be deleted, hence the link supervision timeouts. The pointer is assinged
         * while creating a logical link. At this point, the link supervision timeout is already configured.
         */
    linkSupervisionTimerEntry *lsTableEntry;
    CsrUint16 receivedAckCount;
    CsrUint16 blockAckCount;

    CsrUint16 nextPacketTag;
}pdamLogicalLinkAttrib;

typedef struct PAL_DamTxPacketInfo
{
    pdamLogicalLinkAttrib *logicalLink;
    CsrUint32 expiryTime; /*expiry time (now+flushtimeout) used for Qos Violation checks & starting flush timer */
    CsrBool flushPending; /* Set to TRUE if flush is pending on this packet */

    /* A unique tag to identify this packet. The tag must be unique for packets pending tx with in a logical link  */
    CsrUint16 packetTag;
}PAL_DamTxPacketInfo;

typedef struct PAL_DamCommandQ
{
    CsrInt32 qSize; /* free slots in this access class traffic queue*/
    CsrInt32 freeSlots; /* free slots in this access class traffic queue*/

} PAL_DamCommandQ;

/**
 * @brief
 *   logical link information.
 *
 * @par Description
 *
 */
typedef struct logicalLinkList
{
     csr_list logicalLinkList;    /* a list of logical links */
}logicalLinkList;

/**
 * @brief
 *   Hci Event masks. If TRUE the event is enabled else it will be suppressed.
 *
 * @par Description
 *
 */
typedef struct HciEventMasks
{
    CsrBool qosViolationEventEnabled;
    CsrBool numCompletedDataBlocksEventEnabled;
}HciEventMasks;

/**
 * @brief
 *   the main structure that holds the information for data manager
 *
 * @par Description
 *
 */
struct dataManagerContext
{
    creditInfo QosCreditInfo[PAL_DAM_MAX_NUM_OF_QOS];

    /* command queue is mapped to the command Queue in the hip lib.
     * It is used by PAL-D while sending ma-unitdata-cancel-req to make sure
     * that the queue does not overflow
     */
    PAL_DamCommandQ cmdQueue;

    logicalLinkList *dataLinkList;

    linkSupervisionTimerEntry linkSupervisionTable[PAL_DAM_MAX_PHYSICAL_LINKS];
    CsrInt32 numActivePhyLinks;

    HciEventMasks eventMask;

    CsrUint8 subscriptionHandle;
    CsrUint16 allocOffset;

#ifdef SME_TRACE_ENABLE
    /* Statistics */
    CsrUint32 avgInterPacketArrivalTime;
    CsrUint32 totalTxPktCount;
    CsrUint32 highestInterPacketArrivalTime;
    CsrUint32 pktsWithGreaterThan5msArrTime;
    CsrUint32 totalTimePktsWithHighLatency;
    CsrUint32 totalTxPktSize;

    CsrUint32 avgStatusIndTime;
    CsrUint32 totalStatusInds;
    CsrUint32 totalSuccessfullTx;
    CsrUint32 totalFailedTx;

    CsrUint32 totalUnitdataInds;
    CsrUint32 totalUnitdataIndSize;
    CsrUint32 avgInterUnitdataIndTime;
    CsrUint32 unitdataIndsWithGreaterThan5msArrTime;
    CsrUint32 unitdataIndsTotalTimePktsWithHighLatency;
    CsrUint32 unitdataIndsHighestInterPacketArrivalTime;

    CsrUint32 numFlushRequests;
    CsrUint32 numFlushTimerExpiries;
    CsrUint32 numTotalPacketsFlushTriggered;
    CsrUint32 numTotalPacketsFlushedSuccessfully;
#endif
};

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* Define INLINE directive*/
#ifndef INLINE
#define INLINE      inline
#endif

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/


/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   get the link supervision entry for a physical link handle
 *
 * @par Description
 *
 * @param[in]    damContext   : DAM Manager context
 * @param[in]    phyLinkHandle   : physical link handle
 *
 * @return
 *   returns pointer to the structure if the entry exists else return NULL.
 */
static linkSupervisionTimerEntry *pal_dam_get_link_supervision_entry(dataManagerContext *damContext, CsrUint8 phyLinkHandle)
{
    CsrInt32 i;
    sme_trace_entry((TR_PAL_DAM,"pal_dam_get_link_supervision_entry: phyHandle-%d",phyLinkHandle));

    for (i=0; i<PAL_DAM_MAX_PHYSICAL_LINKS; i++)
    {
        if (damContext->linkSupervisionTable[i].physicalLinkHandle == phyLinkHandle)
        {
            sme_trace_info((TR_PAL_DAM,"pal_dam_get_link_supervision_entry: entry found"));

            return &damContext->linkSupervisionTable[i];
        }
    }
    return NULL;
}

/**
 * @brief
 *   populate and send the MA-UNITDATA-CANCEL-REQ
 *
 * @par Description
 *
 * @param[in]    context   : FSM context
 * @param[in]    reqIdentifier   : tag of the ma-unitdata-req to cancel
 *
 * @return
 *   void
 */
static void populate_and_send_ma_unitdata_cancel_req(FsmContext *context, CsrUint32 reqIdentifier)
{
    MaUnitdataCancelReq_Evt evt;
    CsrUint16 packedLength = 0;
    CsrUint8* buffer = (CsrUint8*)&evt;

    packedLength += event_pack_CsrUint16(&buffer, MA_UNITDATA_CANCEL_REQ_ID);
    packedLength += event_pack_CsrUint16(&buffer, 0); /* destination id - not relevant */
    packedLength += event_pack_CsrUint16(&buffer, context->currentInstance->instanceId);
    packedLength += event_pack_CsrUint32(&buffer, 0); /* Data_Reference : DummyDataRef1 */
    packedLength += event_pack_CsrUint32(&buffer, 0); /* Data_Reference : DummyDataRef2 */
    packedLength += event_pack_CsrUint32(&buffer, reqIdentifier); /* p1_Host_Tag */
    sme_trace_info((TR_PAL_DAM, "populate_and_send_ma_unitdata_cancel_req: Event(MA_UNITDATA_CANCEL_REQ_ID) To(env)"));

    call_paldata_sys_hip_req(context,packedLength,(CsrUint8*)&evt,0,NULL,0,NULL);
}

/**
 * @brief
 *   get the data link attribute for a logical link handle
 *
 * @par Description
 *
 * @param[in]    dataLinkList   : data link list
 * @param[in]    logicalLinkHandle   : data block with the bulkdata
 *
 * @return
 *   return the pointer to the structure if success else return NULL
 */
static pdamLogicalLinkAttrib *pal_dam_get_link_attrib(logicalLinkList *dataLinkList, CsrUint16 logicalLinkHandle)
{
    csr_list_node *node;

    sme_trace_entry((TR_PAL_DAM,"pal_dam_get_link_attrib: handle-%d",logicalLinkHandle));
    node = csr_list_gethead_void(&dataLinkList->logicalLinkList);
    while (node)
    {
        pdamLogicalLinkAttrib *logicalLink = (pdamLogicalLinkAttrib *)node->value;

        if (logicalLinkHandle==logicalLink->logicalLinkHandle)
        {
            return logicalLink;
        }

        node = csr_list_getnext_void(&dataLinkList->logicalLinkList,node);
    }
    return NULL;
}

/**
 * @brief
 *   get the data link attribute from remote mac address
 *
 * @par Description
 *
 * @param[in]    dataLinkList   : data link list
 * @param[in]    remoteMacAddress   : remote mac address
 *
 * @return
 *   return the pointer to the structure if success else return NULL
 */
static pdamLogicalLinkAttrib *pal_dam_get_link_attrib_from_remote_mac_address(logicalLinkList *dataLinkList, unifi_MACAddress *remoteMacAddress)
{
    csr_list_node *node;

    sme_trace_entry((TR_PAL_DAM,"pal_dam_get_link_attrib_from_remote_mac_address:"));
    node = csr_list_gethead_void(&dataLinkList->logicalLinkList);
    while (node)
    {
        pdamLogicalLinkAttrib *logicalLink = (pdamLogicalLinkAttrib *)node->value;

        if (0 == CsrMemCmp(remoteMacAddress, &logicalLink->remoteMacAddress,6))
        {
            return logicalLink;
        }

        node = csr_list_getnext_void(&dataLinkList->logicalLinkList,node);
    }
    return NULL;
}

/**
 * @brief
 *   start the flush timer
 *
 * @par Description
 *
 * @param[in]    context   : data link list
 * @param[in]    logicalLink   : link attributes
 * @param[in]    timeoutValue   : timeout value
 *
 * @return
 *   void
 */
static void pal_dam_start_flush_timer(FsmContext *context,
                                          pdamLogicalLinkAttrib *logicalLink,
                                          CsrUint32 timeoutValue)
{
    sme_trace_entry((TR_PAL_DAM, "pal_dam_start_flush_timer(): timeout-%dmicroS",timeoutValue));
    send_pal_dam_flush_timer(context, logicalLink->flushTimerId, timeoutValue/1000, 10, logicalLink->logicalLinkHandle);
}

/**
 * @brief
 *   stop the flush timer
 *
 * @par Description
 *
 * @param[in]    context   : data link list
 * @param[in]    logicalLink   : link attributes
 *
 * @return
 *   void
 */
static void pal_dam_stop_flush_timer(FsmContext *context,
                                     pdamLogicalLinkAttrib *logicalLink)
{
    sme_trace_entry((TR_PAL_DAM, "pal_dam_stop_flush_timer()"));
    if (logicalLink->flushTimerId.id)
    {
        fsm_remove_timer(context, logicalLink->flushTimerId);
        logicalLink->flushTimerId.id = 0;
    }
}

/**
 * @brief
 *   restart the flush timer
 *
 * @par Description
 *
 * @param[in]    context   : data link list
 * @param[in]    logicalLink   : link attributes
 * @param[in]    timeoutValue   : timeout value
 *
 * @return
 *   void
 */
static void pal_dam_restart_flush_timer(FsmContext *context,
                                            pdamLogicalLinkAttrib *logicalLink,
                                            CsrUint32 timeoutValue)
{
    sme_trace_entry((TR_PAL_DAM, "pal_dam_restart_flush_timer(): timeout-%dmicroS",timeoutValue));
    fsm_remove_timer(context, logicalLink->flushTimerId);
    send_pal_dam_flush_timer(context, logicalLink->flushTimerId, timeoutValue/1000, 10, logicalLink->logicalLinkHandle);
}


/**
 * @brief
 *   next flush timeout value
 *
 * @par Description
 * This function will loop through all outstanding packets for QOS links to figure next flush timeout value.
 * The first packet that has no flush already pending on it will be used to calculate next flush timeout.
 * Return flush timeout value in microsecs Or zero if no packets left to start a flush timer.
 * For eg: all packets are being flushed. or No packets in queue.
 *
 *
 * @param[in]    context   : data link list
 * @param[in]    logicalLink   : link attributes
 * @param[in]    timeNow   : current timestamp in microseconds
 *
 * @return
 *   return flush timeout from now in microseconds. Zero if no timers required 
 */
static CsrUint32 get_next_flush_timer_value(FsmContext *context, pdamLogicalLinkAttrib *logicalLink, CsrUint32 timeNow)
{
    PAL_DamTxPacketInfo *txPktInfo;
    CsrUint32 leastValForFlushTimeout=0;
    csr_list_node *pktNode;

    pktNode = csr_list_gethead_void(&logicalLink->txPacketList);

    sme_trace_info((TR_PAL_DAM, "get_next_flush_timer_value(): numPktsInQueue-%d,now-%u micros. Zero means no packets in queue",csr_list_size(&logicalLink->txPacketList),timeNow));

    /* Loop through the packet until a valid packet is found for a flush timer to be started.
     * A valid packet will be the first from the head of queue on which no flush is pending
     */
    while (pktNode)
    {
        txPktInfo = (PAL_DamTxPacketInfo *)(pktNode->value);

        if (!txPktInfo->flushPending)
        {
            sme_trace_info((TR_PAL_DAM, "get_next_flush_timer_value(): next packet found to expire at -%u",txPktInfo->expiryTime));
            leastValForFlushTimeout = TIME_DIFF(timeNow,txPktInfo->expiryTime);
            break;
        }
        else
        {
            sme_trace_info((TR_PAL_DAM, "get_next_flush_timer_value(): flush pending on this packet. so skip to next one"));
            pktNode = csr_list_getnext_void(&logicalLink->logicalLinkList,pktNode);
        }
    }

    return leastValForFlushTimeout;
}

/**
 * @brief
 *   flush the tx queue
 *
 * @par Description
 * This function will loop through all outstanding packets for a paritcular data link and trigger
 * flush (send ma-unitdata-cancel-req) for all packets that is waiting to be transmitted. Those
 * packets that already have a flush pending will be skipped. If flushAllPacketsPending is set, then
 * all packets in the list will be flushed. Otherwise it will flush the expired packets.
 *
 * @param[in]    context   : FSM Context
 * @param[in]    damContext   :  data manager context
 * @param[in]    logicalLink   : link attributes
 * @param[in]    timeNow   : current timestamp in microseconds
 *
 * @return
 *   number of packets on which flush is triggered. Caller can take further action based on the return value.
 * Zero is returned if no packets flushed.
 */
static CsrUint16 pal_dam_flush_queue_handler(FsmContext *context,
                                                   dataManagerContext *damContext,
                                                   pdamLogicalLinkAttrib *logicalLink,
                                                   CsrUint32 timeNow)
{
    CsrUint16 numPacketsFlushed=0;
    csr_list_node *node;
    PAL_DamTxPacketInfo *txPktInfo;

    sme_trace_entry((TR_PAL_DAM, "pal_dam_flush_queue_handler(): handle-%d, flushAllPacketsPending=%d",logicalLink->logicalLinkHandle,logicalLink->flushAllPacketsPending));

    node = csr_list_gethead_void(&logicalLink->txPacketList);

    sme_trace_info((TR_PAL_DAM, "pal_dam_flush_queue_handler(): %d packets already have flush pending. TimeNow-%u",logicalLink->flushPendingCount,timeNow));
    while (node && damContext->cmdQueue.freeSlots)
    {
        txPktInfo = (PAL_DamTxPacketInfo *)(node->value);

        sme_trace_info((TR_PAL_DAM, "pal_dam_flush_queue_handler(): Expiry time for this packet is %u",txPktInfo->expiryTime));

        /* Stop the flushing if the this packet is NOT expired and the caller is NOT flushing all packets */
        if (FALSE == logicalLink->flushAllPacketsPending && 
            TX_PKT_NOT_EXPIRED(txPktInfo,timeNow))
        {
            sme_trace_info((TR_PAL_DAM, "pal_dam_flush_queue_handler(): No more packets to flush with in the boundary specified"));
            break;
        }
        /* trigger the flush only if the flush not already pending on that packet */
        if (!txPktInfo->flushPending)
        {
            damContext->cmdQueue.freeSlots--;
            populate_and_send_ma_unitdata_cancel_req(context, GENERATE_REQ_IDENTIFIER_FOR_TX_PAKCET(txPktInfo));
            txPktInfo->flushPending=TRUE;
            numPacketsFlushed++;
            sme_trace_crit((TR_PAL_DAM, "pal_dam_flush_queue_handler(): flush this packet. total pending-%d,this flush-%d",logicalLink->flushPendingCount,numPacketsFlushed));
#ifdef SME_TRACE_ENABLE
            damContext->numTotalPacketsFlushTriggered++;
#endif
        }
        node = csr_list_getnext_void(&logicalLink->txPacketList,node);
    }

    if (numPacketsFlushed)
    {
        verify(TR_PAL_DAM,damContext->cmdQueue.freeSlots<=damContext->cmdQueue.qSize);

        logicalLink->flushPendingCount+=numPacketsFlushed;
        sme_trace_info((TR_PAL_DAM, "pal_dam_flush_queue_handler(): Flush triggered for %d packets in this handle. %d packets being flushed in total",numPacketsFlushed,logicalLink->flushPendingCount));

        sme_trace_info((TR_PAL_DAM, "pal_dam_flush_queue_handler(): Stop the flush timer"));
        pal_dam_stop_flush_timer(context,logicalLink);
    }
#ifdef SME_TRACE_ENABLE
    else
    {
        sme_trace_info((TR_PAL_DAM, "pal_dam_flush_queue_handler(): flush procedure completed as no packets to flush."));
    }
#endif
    return logicalLink->flushPendingCount;
}


/**
 * @brief
 *   delete a logical link entry
 *
 * @par Description
 *
 * @param[in]    context   : data link list
 * @param[in]    dataLinkList   : data links list
 * @param[in]    logicalLinkHandle   : logic link handle to be deleted.
 *
 * @return
 *   TRUE if deletion success else FALSE
 */
static CsrBool delete_logical_link_entry(FsmContext *context,
                                           logicalLinkList *dataLinkList,
                                           CsrUint16 logicalLinkHandle)
{
    csr_list_node *node,*currentNode;

    sme_trace_entry((TR_PAL_DAM,"delete_logical_link_entry: llHandle-%d",logicalLinkHandle));
    node = csr_list_gethead_void(&dataLinkList->logicalLinkList);

    while (node)
    {
        pdamLogicalLinkAttrib *logicalLink = (pdamLogicalLinkAttrib *)node->value;

        currentNode = node;
        node = csr_list_getnext_void(&dataLinkList->logicalLinkList,currentNode);
        if (logicalLink && logicalLinkHandle == logicalLink->logicalLinkHandle)
        {
            sme_trace_info((TR_PAL_DAM,"delete_logical_link_entry: deleting link node with handle %d, current list size-%d",logicalLink->logicalLinkHandle,csr_list_size(&dataLinkList->logicalLinkList)));

            verify(TR_PAL_DAM,0==csr_list_size(&logicalLink->txPacketList)); /*lint !e666    csr_list_size */
            csr_list_clear(&logicalLink->txPacketList);

            (void)csr_list_remove(&dataLinkList->logicalLinkList,currentNode);

            sme_trace_info((TR_PAL_DAM,"delete_logical_link_entry: List size after deleting link-%d",csr_list_size(&dataLinkList->logicalLinkList)));
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief
 *    delete a logical link entry with callback
 *
 * @par Description
 * Function to delete logical link entry in the list for a physical link. Depending on the parameter the function will delete
 * just one linke provided a valid logical link handle is given or all links matching the supplied phy link handle. deleteType
 * determines the action to be taken.
 *
 * @param[in]    context   : FSM Context
 * @param[in]    dataLinkList   : data links list
 * @param[in]    logicalLinkHandle   : logical link handle
 * @param[in]    phyLinkHandle   : physical link handle
 * @param[in]    deleteType   :  Type of deletion ->delete all or delete matching link only.
 * @param[in]    cbLogicalLinkDeleted   : callback -> NULL or function pointer to call on deletion.
 *
 * @return
 *   TRUE if deletion success else FALSE
 */
static CsrBool delete_logical_link_entry_with_callback(FsmContext *context,
                                                           dataManagerContext *damContext,
                                                           CsrUint16 logicalLinkHandle,
                                                           CsrUint8 phyLinkHandle,
                                                           deleteLinkType deleteType,
                                                           void (* cbLogicalLinkDeleted)(FsmContext *, CsrUint16,CsrUint8))  /*lint !e955    cbLogicalLinkDeleted */
{
    csr_list_node *node,*currentNode;
    logicalLinkList *dataLinkList = damContext->dataLinkList;

    sme_trace_entry((TR_PAL_DAM,"delete_logical_link_entry_with_callback: llHandle-%d,delType-%d,phyLinkHdl-%d",logicalLinkHandle,deleteType,phyLinkHandle));
    node = csr_list_gethead_void(&dataLinkList->logicalLinkList);

    while (node)
    {
        pdamLogicalLinkAttrib *logicalLink = (pdamLogicalLinkAttrib *)node->value;

        currentNode = node;
        node = csr_list_getnext_void(&dataLinkList->logicalLinkList,currentNode);
        if (logicalLink &&
            (logicalLinkHandle == logicalLink->logicalLinkHandle || PAL_INVALID_LOGICAL_LINK_HANDLE == logicalLinkHandle))
        {
            sme_trace_info((TR_PAL_DAM,"delete_logical_link_entry_with_callback: deleting link node with handle %d, current list size-%d",logicalLink->logicalLinkHandle,csr_list_size(&dataLinkList->logicalLinkList)));

            /* set this flag incase there are packets to be flushed before responding with a confirm*/
            logicalLink->flushAllPacketsPending = TRUE;

            if (cbLogicalLinkDeleted)
            {
                CsrUint32 timeNow = GET_CURRENT_TIME_IN_MICROS();
                if (!pal_dam_flush_queue_handler(context, damContext, logicalLink, timeNow))
                {
                    sme_trace_info((TR_PAL_DAM,"delete_logical_link_entry_with_callback: no packets to flush so do a calll back straight"));

                    cbLogicalLinkDeleted(context,logicalLinkHandle,logicalLink->physicalLinkHandle);
                    (void)csr_list_remove(&dataLinkList->logicalLinkList,currentNode);
                }
                else
                {
                    sme_trace_info((TR_PAL_DAM,"delete_logical_link_entry_with_callback: Flush triggered,saving callback to call after flush completed"));
                    verify(TR_PAL_DAM, NULL==logicalLink->cbLogicalLinkDeleted);
                    logicalLink->cbLogicalLinkDeleted = cbLogicalLinkDeleted;
                }
            }
            else if (DELETE_ALL_LINKS == deleteType && logicalLink->physicalLinkHandle==phyLinkHandle)
            {
                (void)csr_list_remove(&dataLinkList->logicalLinkList,currentNode);
            }
            /* return only if only one logical link needs to be deleted */
            if (DELETE_MATCHING_LINK == deleteType)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/**
 * @brief
 *   allocate TX packet information entry in the tx packet list
 *
 * @par Description
 *
 * @param[in]    damContext   : pal-d context
 * @param[in]    logicalLink   : link attributes
 * @param[in]    size   : size of the data to tx
 *
 * @return
 *   pointer to the new packet of type PAL_DamTxPacketInfo
 */
static PAL_DamTxPacketInfo * pal_llm_allocate_tx_packet_info(dataManagerContext *damContext,
                                                  pdamLogicalLinkAttrib *logicalLink,
                                                  CsrUint32 expiryTime)
{
    csr_list_node *node;
    PAL_DamTxPacketInfo *newPktInfo;

    sme_trace_entry((TR_PAL_DAM,"pal_llm_allocate_tx_packet_info"));
    /* insert the node onto the list in the appropriate location */
    node = (csr_list_node *)CsrPmalloc(sizeof(csr_list_node));
    newPktInfo = CsrPmalloc(sizeof(PAL_DamTxPacketInfo));
    csr_list_insert_tail(&logicalLink->txPacketList,list_owns_both,node,(void*)newPktInfo);

    newPktInfo->logicalLink=logicalLink;
    newPktInfo->expiryTime=expiryTime;
    newPktInfo->flushPending=FALSE;
    newPktInfo->packetTag=logicalLink->nextPacketTag;

    logicalLink->nextPacketTag++;
    if (!(logicalLink->nextPacketTag%TX_PKT_MAX_PACKET_TAG_NUMBER(damContext,logicalLink)))
    {
        logicalLink->nextPacketTag = 0;
    }

    return newPktInfo;
}


/**
 * @brief
 *   update credit information for the host when tx confirmation is received from the Unifi
 *
 * @par Description
 *
 * @param[in]    context   : Client Context
 * @param[in]    damContext   : data manager context
 * @param[in]    crdInfo   : pointer to the credit informaton for this packet.
 * @param[in]    numCompletedBlocks   : number of packets that are completed transmission
 * @param[in]    logicalLinkHandle   :  logical link for which the packet is transmitted
 *
 * @return
 *   void
 */
static void update_credit_info(FsmContext *context,
                                 dataManagerContext *damContext,
                                 creditInfo *crdInfo,
                                 CsrUint16 numCompletedBlocks,
                                 CsrUint16 logicalLinkHandle)
{
    sme_trace_entry((TR_PAL_DAM, "update_credit_info(): freeBlocks-%d, completedBlocks-%d,llHdl-%d",crdInfo->numFreeDataBlocks,numCompletedBlocks,logicalLinkHandle));
    if (crdInfo->numFreeDataBlocks < crdInfo->totalDataBlocks)
    {
        CsrUint16 free_credits=(crdInfo->numFreeDataBlocks+numCompletedBlocks)>crdInfo->totalDataBlocks?
                            crdInfo->totalDataBlocks-crdInfo->numFreeDataBlocks:numCompletedBlocks ;
        /* Set the remaining free credits as appropriate */
        crdInfo->numFreeDataBlocks += free_credits;
        verify(TR_PAL_DAM, crdInfo->numFreeDataBlocks<=crdInfo->totalDataBlocks);

        /* Align it with the avaialable slots */
        sme_trace_info((TR_PAL_DAM, "update_credit_info(): freeCredits-%d",free_credits));

        if (damContext->eventMask.numCompletedDataBlocksEventEnabled)
        {
            call_hci_number_of_completed_data_blocks_evt(context,
                                                         (CsrUint16)crdInfo->totalDataBlocks,
                                                         1,
                                                         logicalLinkHandle,
                                                         free_credits,free_credits);
        }
    }
}


/**
 * @brief
 *   start early link loss timer
 *
 * @par Description
 *   The function will start the early link loss timer only if it expriry time for link loss timer (if expired
 * denotes termination of link) is greater than a minimum duration. The minimum duration of early link
 * loss is set to MIN_EARLY_LINK_LOSS_TIMEOUT_DURATION.
 *
 * @param[in]    context   : Client Context
 * @param[in]    lsEntry   : pointer to the link supervision entry
 * @param[in]    timeNow   : timestamp now in microseconds
 *
 * @return
 *   void
 */
static void start_early_link_loss_timer(FsmContext *context,
                                          linkSupervisionTimerEntry *lsEntry,
                                          CsrUint32 timeNow)
{
    CsrUint32 earlyLinkLossTimeout; /* in micro seconds */

    earlyLinkLossTimeout = GET_EARLY_LINK_LOSS_TIMEOUT_VALUE(TIME_DIFF(timeNow,lsEntry->expiryTime));

    sme_trace_info((TR_PAL_DAM,"start_early_link_loss_timer: entry found. next timeout - %d microseconds",earlyLinkLossTimeout));

    lsEntry->linkAlive = FALSE;
    if (earlyLinkLossTimeout >= MIN_EARLY_LINK_LOSS_TIMEOUT_DURATION)
    {
        sme_trace_info((TR_PAL_DAM,"start_early_link_loss_timer: restart early link loss again for %d microSecs",earlyLinkLossTimeout));
        lsEntry->earlyLinkLossTimer.id = 0;
        send_pal_dam_early_link_loss_timer(context, lsEntry->earlyLinkLossTimer,
                                           earlyLinkLossTimeout/1000, 10,
                                           lsEntry->physicalLinkHandle);
    }
    else
    {
        sme_trace_info((TR_PAL_DAM,"start_early_link_loss_timer: final leg, start link loss timer for %d microSecs",earlyLinkLossTimeout));
        lsEntry->linkLossTimer.id = 0;
        send_pal_dam_link_loss_timer(context, lsEntry->linkLossTimer,
                             earlyLinkLossTimeout/1000, 10,
                             lsEntry->physicalLinkHandle);
    }
}

/**
 * @brief
 *   start link loss timer
 *
 * @par Description
 *
 * @param[in]    context   : Client Context
 * @param[in]    lsEntry   : pointer to the link supervision entry
 *
 * @return
 *   void
 */
static void pal_dam_start_link_loss_timer(FsmContext *context, linkSupervisionTimerEntry *lsEntry)
{
    sme_trace_entry((TR_PAL_DAM, "pal_dam_start_link_loss_timer():handl-%d timeout-%dmicroS",lsEntry->physicalLinkHandle,lsEntry->timeoutDuration));

    verify(TR_PAL_DAM,lsEntry!=NULL);
    if (lsEntry->timeoutDuration)
    {
        /* Start with the early loss timer and then the link loss timer in the final leg to get away with only one
        * timer at a time
        */
        CsrUint32 timeNow = GET_CURRENT_TIME_IN_MICROS();
        lsEntry->expiryTime = TIME_ADD(timeNow,lsEntry->timeoutDuration);
        start_early_link_loss_timer(context,lsEntry,timeNow);
    }
}

/**
 * @brief
 *   stop link loss timer
 *
 * @par Description
 *
 * @param[in]    context   : Client Context
 * @param[in]    lsEntry   : pointer to the link supervision entry
 *
 * @return
 *   void
 */
static void pal_dam_stop_link_loss_timer(FsmContext *context,
                                              linkSupervisionTimerEntry *lsEntry)
{
    sme_trace_entry((TR_PAL_DAM, "pal_dam_stop_link_loss_timer():handl-%d timeout-%dmicroS",lsEntry->physicalLinkHandle,lsEntry->timeoutDuration));
    sme_trace_info((TR_PAL_DAM, "pal_dam_stop_link_loss_timer():earlyLLTimerid-%d LLTimerid-%d ",lsEntry->earlyLinkLossTimer.id,lsEntry->linkLossTimer.id));

    verify(TR_PAL_DAM,lsEntry!=NULL);
    if (lsEntry->earlyLinkLossTimer.id)
    {
        sme_trace_entry((TR_PAL_DAM, "pal_dam_stop_link_loss_timer():stopping earlyLinkLoss timer"));
        fsm_remove_timer(context, lsEntry->earlyLinkLossTimer);
        lsEntry->earlyLinkLossTimer.id = 0;
    }
    if (lsEntry->linkLossTimer.id)
    {
        sme_trace_entry((TR_PAL_DAM, "pal_dam_stop_link_loss_timer():stopping linkLoss timer"));
        fsm_remove_timer(context, lsEntry->linkLossTimer);
        lsEntry->linkLossTimer.id = 0;
    }    
}

/**
 * @brief
 *   restart link loss timer
 *
 * @par Description
 *
 * @param[in]    context   : Client Context
 * @param[in]    lsEntry   : pointer to the link supervision entry
 *
 * @return
 *   void
 */
static void pal_dam_restart_link_loss_timer(FsmContext *context,
                                                linkSupervisionTimerEntry *lsEntry)
{
    sme_trace_entry((TR_PAL_DAM, "pal_dam_restart_link_loss_timer():handl-%d timeout-%dmicroS",lsEntry->physicalLinkHandle,lsEntry->timeoutDuration));

    verify(TR_PAL_DAM,lsEntry!=NULL);
    pal_dam_stop_link_loss_timer(context,lsEntry);
    pal_dam_start_link_loss_timer(context,lsEntry);
}

/**
 * @brief
 *    process the flush timeout
 *
 * @par Description
 *    Flush all packets that are expired. Call the callback function if set immediately if no packets were available
 * to flush. Otherwise save it to call when flush is completed.
 *
 * @param[in]    context   : Client Context
 * @param[in]    damContext   : data manager context
 * @param[in]    logicalLinkHandle   : logical link handle
 * @param[in]    cbFlushOccuredFinalResponse   : callback to call if flush procedure is completed.
 *
 * @return
 *   void
 */
void pal_dam_process_flush_timeout(FsmContext *context,
                                          dataManagerContext *damContext,
                                          CsrUint16 logicalLinkHandle,
                                          void(*cbFlushOccuredFinalResponse)( FsmContext *, CsrUint16, CsrBool)) /*lint !e955    cbFlushOccuredFinalResponse */
{
    pdamLogicalLinkAttrib *logicalLink = pal_dam_get_link_attrib(damContext->dataLinkList, logicalLinkHandle);

    sme_trace_info((TR_PAL_DAM, "pal_dam_process_flush_timeout(): handle-%d",logicalLinkHandle));
    if (logicalLink)
    {
        CsrUint32 nextFlushTimeoutVal;
        CsrUint32 timeNow = GET_CURRENT_TIME_IN_MICROS();

#ifdef SME_TRACE_ENABLE
         damContext->numFlushTimerExpiries++;
#endif
        logicalLink->cbFlushOccuredFinalResponse = cbFlushOccuredFinalResponse;
        if (!pal_dam_flush_queue_handler(context,
                                          damContext,
                                          logicalLink,
                                          timeNow))
         {
            /* No packets to flush */
            if (logicalLink->cbFlushOccuredFinalResponse)
            {
                logicalLink->cbFlushOccuredFinalResponse(context, logicalLink->logicalLinkHandle,logicalLink->flushOccured);
                logicalLink->cbFlushOccuredFinalResponse = NULL;
            }
         }

        nextFlushTimeoutVal = get_next_flush_timer_value(context, logicalLink,timeNow);
        if (nextFlushTimeoutVal > 0)
        {
            sme_trace_crit((TR_PAL_DAM, "pal_dam_process_flush_timeout():start new timer: expiry after %d micros",nextFlushTimeoutVal));
            pal_dam_start_flush_timer(context,
                                      logicalLink,
                                      nextFlushTimeoutVal);
        }
        else
        {
            sme_trace_crit((TR_PAL_DAM, "pal_dam_process_flush_timeout(): no timer to start"));
            pal_dam_stop_flush_timer(context,logicalLink);
        }
    }
    else
    {
        sme_trace_crit((TR_PAL_DAM, "pal_dam_process_flush_timeout(): Link not found-%d",logicalLinkHandle));
    }
}

/**
 * @brief
 *    initialise data (logical) links list
 *
 * @par Description
 *
 * @param
 *
 * @return
 *   pointer to the list structure
 */
static logicalLinkList * pal_dam_logical_links_init(void)
{
    logicalLinkList *dataLinkList = CsrPmalloc(sizeof(logicalLinkList));

    csr_list_init(&dataLinkList->logicalLinkList);
    return dataLinkList;
}


/**
 * @brief
 *    de-initialise data (logical) links list
 *
 * @par Description
 *
 * @param[in]    context  : FSM Context
 * @param[in]    dataLinkList   : data link list
 *
 * @return
 *   void
 */
static void pal_dam_logical_links_deinit(FsmContext *context, dataManagerContext *damContext)
{
    csr_list_node *node,*pktNode;
    pdamLogicalLinkAttrib *logicalLink;
    logicalLinkList *dataLinkList = damContext->dataLinkList;
    CsrBool lastPhyLink;

    sme_trace_entry((TR_PAL_DAM, "pal_dam_logical_links_deinit(): %d",dataLinkList));
    node = csr_list_gethead_void(&dataLinkList->logicalLinkList);
    sme_trace_entry((TR_PAL_DAM, "pal_dam_logical_links_deinit(): node-%d",node));

    while (node)
    {
        logicalLink = (pdamLogicalLinkAttrib *)node->value;

        sme_trace_info((TR_PAL_DAM, "pal_dam_logical_links_deinit(): Delete logical link node llHandle-%d, phyHandle-%d",logicalLink->logicalLinkHandle,logicalLink->physicalLinkHandle));
        verify(TR_PAL_DAM,logicalLink!=NULL);

        /* stop flush timer if running */
        pal_dam_stop_flush_timer(context,logicalLink);

        /* stop link supervision timer if running */
        pal_dam_delete_link_supervision_timeout(context, damContext, logicalLink->physicalLinkHandle, &lastPhyLink);

        /* delete packets pending acks */
        pktNode = csr_list_gethead_void(&logicalLink->txPacketList);
        while (pktNode)
        {
            sme_trace_info((TR_PAL_DAM, "pal_dam_logical_links_deinit(): This packet is to be acknowledged. Discard it"));
            (void)csr_list_remove(&logicalLink->txPacketList,pktNode);
            pktNode = csr_list_gethead_void(&logicalLink->txPacketList);
        }

        /* send any responses if pending for flush or logical link deletion */
        if (logicalLink->cbFlushOccuredFinalResponse)
        {
            sme_trace_info((TR_PAL_DAM, "pal_dam_logical_links_deinit(): flushing complete. sending final response"));
            logicalLink->cbFlushOccuredFinalResponse(context, logicalLink->logicalLinkHandle,FALSE);
        }

        if (logicalLink->cbLogicalLinkDeleted)
        {
             sme_trace_info((TR_PAL_DAM,"pal_dam_logical_links_deinit(): delete Logical link callback set. so call it"));
             logicalLink->cbLogicalLinkDeleted(context,logicalLink->logicalLinkHandle,logicalLink->physicalLinkHandle);
        }
        sme_trace_entry((TR_PAL_DAM, "pal_dam_logical_links_deinit(): 2 node-%d",node));

        (void)csr_list_remove(&dataLinkList->logicalLinkList,node);
        node = csr_list_gethead_void(&dataLinkList->logicalLinkList);

    }
}


/**
 * @brief
 *    populate snap header and mac addresses
 *
 * @par Description
 *
 * @param[in]    context   : client context
 * @param[in]    dataBlock   : data block where snap header will go
 * @param[in]    logicalLink   : link info
 *
 * Note:
 * Ensure that enough buffer is allocated in the dataBlock before calling this function as it will
 * try to push the pointer to populate the snap header
 *
 *
 * @return
 *   void
 */
static CsrUint16 pal_populate_snap_header_and_mac_address(FsmContext *context,
                                                                     PAL_DataBlock **dataBlock,
                                                                     pdamLogicalLinkAttrib *logicalLink)
{
    CsrUint16 totalLength = PALDATA_SNAP_HEADER_SIZE+(PALDATA_MAC_ADDRESS_SIZE*2);
    llc_snap_hdr_t snap;
    CsrUint8 bt_oui[] = {0x00, 0x19, 0x58};
    CsrUint8 *buffer = PALDATA_MEM_DOWNSTREAM_ADD_SNAP_HEADER_AND_MAC_ADDRESS(*dataBlock, totalLength);

    (void)event_pack_buffer(&buffer, (CsrUint8*)&logicalLink->remoteMacAddress, PALDATA_MAC_ADDRESS_SIZE); /*lint !e545 !e725    p2_DA */
    (void)event_pack_buffer(&buffer, (CsrUint8*)&logicalLink->localMacAddress, PALDATA_MAC_ADDRESS_SIZE); /*lint !e545 !e725    p3_SA */

    /* tack on SNAP */
    snap.dsap = snap.ssap = 0xAA;
    snap.ctrl = 0x03;
    CsrMemCpy(snap.oui, bt_oui, 3);
    snap.protocol = PALDAM_DATA_PROTO_L2CAP_ID;
    pal_encode_snap(buffer, &snap);
    return totalLength;
}


/**
 * @brief
 *    send acl data to Unifi
 *
 * @par Description
 *
 * @param[in]    context   : client context
 * @param[in]    damContext   : data manager context
 * @param[in]    dataBlock   : data block to send
 * @param[in]    length   : length of data
 * @param[in]    logicalLink   : link attributes
 * @param[in]    freeFunction   : function pointer to free the data memory
 *
 * Note:
 * Ensure data block has got enough memory to add ma-unitdata-req message and snap header
 *
 *
 * @return
 *   void
 */
static CsrUint32 pal_send_downstream_acl_data(FsmContext *context,
                                         dataManagerContext *damContext,
                                         PAL_DataBlock *dataBlock,
                                         CsrUint16 length,
                                         pdamLogicalLinkAttrib *logicalLink,
                                         unifi_FrameFreeFunction freeFunction)
{
    CsrUint32 reqIdentifier;
    CsrUint32 nextTimeoutValue=0;
    sme_trace_entry((TR_PAL_DAM, "pal_sys_send_downstream_acl_data(): "));

    if (unifi_PalServicetypeGuaranteed==logicalLink->txFlowSpec.serviceType ||
        unifi_PalServicetypeBestEffort==logicalLink->txFlowSpec.serviceType)
    {
        CsrInt32 qNo;
        PAL_DamTxPacketInfo *newPktInfo;
        CsrUint32 flushTime;
        CsrUint32 timeNow = GET_CURRENT_TIME_IN_MICROS();
        unifi_ServiceClass serviceClass = (unifi_PriorityContention == logicalLink->userPriority)?unifi_ServiceClassReorderableMulticast:unifi_ServiceClassQosAck;

        qNo = PAL_DAM_GET_QOS_INDEX(logicalLink->userPriority);
        flushTime = GET_FLUSH_TIMEOUT(logicalLink);

        newPktInfo = pal_llm_allocate_tx_packet_info(damContext, logicalLink, TIME_ADD(timeNow,flushTime));
        reqIdentifier = GENERATE_REQ_IDENTIFIER_FOR_TX_PAKCET(newPktInfo);

        sme_trace_info((TR_PAL_DAM,"packet for %s link of length-%d, reqIdentifier-%d",
                       GET_SERVICE_TYPE_STR(logicalLink->txFlowSpec.serviceType),length, reqIdentifier));

        sme_trace_info((TR_PAL_DAM, "pal_send_downstream_acl_data(): flushTimeoutIndMiilliSec-%d, timeNow-%u\n",flushTime,timeNow));

        /* Start the flush timer only if there isn't one already running
         * and flush timer is enabled. A best effort link may have no flush
         * timer running
         */
        if (!logicalLink->flushTimerId.id && flushTime > 0)
        {
            sme_trace_info((TR_PAL_DAM, "pal_send_downstream_acl_data(): No timer running now for this handle. so start flush timer."));
            pal_dam_start_flush_timer(context,
                                      logicalLink,
                                      flushTime);
            nextTimeoutValue = flushTime;
        }

        call_paldata_sys_ma_unitdata_req(context,
                                         PALDATA_APP_HANDLE(context),
                                         damContext->subscriptionHandle,
                                         PALDATA_MEM_GET_BUFFER_LENGTH(dataBlock),
                                         PALDATA_MEM_GET_BUFFER_ORIGINAL(dataBlock),
                                         (unifi_FrameFreeFunction)freeFunction,
                                         logicalLink->userPriority,
                                         serviceClass,
                                         reqIdentifier);
        PALDATA_MEM_DOWNSTREAM_FREE_DATA_BLOCK(dataBlock);

        TX_UPDATE_CREDIT_INFO(&damContext->QosCreditInfo[qNo]);
    }
    else if (unifi_PalServicetypeNoTraffic==logicalLink->txFlowSpec.serviceType)
    {
        sme_trace_info((TR_PAL_DAM,"packet for no traffic link of length - %d",length));
        PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(dataBlock));
        PALDATA_MEM_DOWNSTREAM_FREE_DATA_BLOCK(dataBlock);
    }
    else
    {
        sme_trace_error((TR_PAL_DAM, "pal_sys_send_downstream_acl_data(): unrecognised service type - %d",logicalLink->txFlowSpec.serviceType));
        PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(dataBlock));
        PALDATA_MEM_DOWNSTREAM_FREE_DATA_BLOCK(dataBlock);
    }
    return nextTimeoutValue;
}

/************************************** PUBLIC FUNCTIONS *******************************************/

void pal_dam_set_event_mask(dataManagerContext *damContext,
                                   CsrBool qosViolationEventEnabled,
                                   CsrBool numCompletedDataBlocksEventEnabled)
{
    damContext->eventMask.qosViolationEventEnabled = qosViolationEventEnabled;
    damContext->eventMask.numCompletedDataBlocksEventEnabled = numCompletedDataBlocksEventEnabled;
}


void pal_dam_process_flush_req(FsmContext *context,
                                     dataManagerContext *damContext,
                                     CsrUint16 logicalLinkHandle,
                                     void(*cbFlushOccuredFinalResponse)( FsmContext *, CsrUint16, CsrBool)) /*lint !e955    cbFlushOccuredFinalResponse */
{
    pdamLogicalLinkAttrib *logicalLink = pal_dam_get_link_attrib(damContext->dataLinkList, logicalLinkHandle);
    CsrUint32 timeNow = GET_CURRENT_TIME_IN_MICROS();

    if (logicalLink)
    {
#ifdef SME_TRACE_ENABLE
        damContext->numFlushRequests++;
#endif

        logicalLink->flushAllPacketsPending = TRUE;
        logicalLink->cbFlushOccuredFinalResponse = cbFlushOccuredFinalResponse;
        if (!pal_dam_flush_queue_handler(context,
                                          damContext,
                                          logicalLink,
                                          timeNow))
        {
           /* No packets to flush */
           if (logicalLink->cbFlushOccuredFinalResponse)
           {
               logicalLink->cbFlushOccuredFinalResponse(context, logicalLink->logicalLinkHandle,logicalLink->flushOccured);
               logicalLink->cbFlushOccuredFinalResponse = NULL;
           }
           logicalLink->flushAllPacketsPending = FALSE;
        }

    }
    else
    {
        sme_trace_crit((TR_PAL_DAM, "pal_ctrl_flush_data_link_req_common_handler(): Link not found-%d",logicalLinkHandle));
    }
}

CsrBool pal_dam_delete_matching_logical_link(FsmContext *context,
                                                   dataManagerContext *damContext,
                                                   CsrUint16 logicalLinkHandle,
                                                   void (*pal_dam_logical_link_deleted_callback)(FsmContext *,CsrUint16,CsrUint8)) /*lint !e955    pal_dam_logical_link_deleted_callback */
{
    sme_trace_entry((TR_PAL_DAM,"pal_dam_delete_matching_logical_link: llHandle-%d",logicalLinkHandle));
    return  delete_logical_link_entry_with_callback(context, damContext,
                                      logicalLinkHandle,
                                      PAL_INVALID_PHYSICAL_LINK_HANDLE,
                                      DELETE_MATCHING_LINK,
                                      pal_dam_logical_link_deleted_callback);
}

void pal_dam_create_logical_link_entry(FsmContext *context,
                                            dataManagerContext *damContext,
                                            CsrUint8 phyLinkHandle,
                                            const unifi_AmpFlowSpec *txFlowSpec,
                                            CsrUint16 logicalLinkHandle,
                                            unifi_Priority userPriority,
                                            const unifi_MACAddress *remoteMacAddress,
                                            const unifi_MACAddress *localMacAddress)
{
    logicalLinkList *dataLinkList = damContext->dataLinkList;
    csr_list_node *node;
    pdamLogicalLinkAttrib *logicalLink;

    logicalLink = CsrPmalloc(sizeof(pdamLogicalLinkAttrib));
    CsrMemSet(logicalLink, 0, sizeof(pdamLogicalLinkAttrib));
    logicalLink->userPriority = userPriority;
    logicalLink->txFlowSpec = *txFlowSpec;
    logicalLink->txFlowSpec.flushTimeout = logicalLink->txFlowSpec.flushTimeout;
    logicalLink->logicalLinkHandle = logicalLinkHandle;
    logicalLink->physicalLinkHandle = phyLinkHandle;
    logicalLink->flushOccured = FALSE;
    csr_list_init(&logicalLink->txPacketList); /* initialise the packet info list */
    logicalLink->remoteMacAddress = *remoteMacAddress;
    logicalLink->localMacAddress = *localMacAddress;
    logicalLink->lsTableEntry = pal_dam_get_link_supervision_entry(damContext,phyLinkHandle);
    logicalLink->blockAckCount = CSR_WIFI_PALDATA_BLOCK_ACK_COUNT_MAX;
    verify(TR_PAL_DAM, logicalLink->lsTableEntry!=NULL);

    sme_trace_entry((TR_PAL_DAM,"pal_dam_create_logical_link_entry: FlushTO-%d,servType-%d, accessLat-%d",
                     txFlowSpec->flushTimeout, txFlowSpec->serviceType,txFlowSpec->accessLatency,userPriority,sizeof(userPriority)));
    sme_trace_info((TR_PAL_DAM,"pal_dam_create_logical_link_entry: userPrio-%d,sizeofVariable-%d",userPriority,sizeof(userPriority)));
    /* insert the node onto the list in the appropriate location */
    node = (csr_list_node *)CsrPmalloc(sizeof(csr_list_node));
    csr_list_insert_tail(&dataLinkList->logicalLinkList,list_owns_both,node,logicalLink);
    sme_trace_info((TR_PAL_DAM,"pal_dam_create_logical_link_entry(): created handle -%d",logicalLink->logicalLinkHandle));
}

void pal_dam_modify_logical_link_entry(FsmContext *context,
                                       dataManagerContext *damContext,
                                       CsrUint16 logicalLinkHandle,
                                       const unifi_AmpFlowSpec *modifiedTxFlowSpec)
{
    logicalLinkList *dataLinkList = damContext->dataLinkList;
    pdamLogicalLinkAttrib *logicalLink = pal_dam_get_link_attrib(dataLinkList, logicalLinkHandle);

    sme_trace_entry((TR_PAL_DAM,"pal_dam_modify_logical_link_entry:"));
       /* Now overwrite the old flow spec */
    logicalLink->txFlowSpec = *modifiedTxFlowSpec;
    logicalLink->txFlowSpec.flushTimeout = logicalLink->txFlowSpec.flushTimeout;

    /* stop the flush timeout if its running */
    if (logicalLink->txFlowSpec.flushTimeout == 0)
    {
        pal_dam_stop_flush_timer(context, logicalLink);
    }
}


CsrBool pal_dam_reset_failed_contact_counter(dataManagerContext *damContext, CsrUint16 logicalLinkHandle)
{
    logicalLinkList *dataLinkList = damContext->dataLinkList;
    CsrBool status=FALSE;

    pdamLogicalLinkAttrib *logicalLink = pal_dam_get_link_attrib(dataLinkList, logicalLinkHandle);

    if (logicalLink && logicalLinkHandle == logicalLink->logicalLinkHandle)
    {
        status = TRUE;
        logicalLink->failedContactCounter = 0;
    }
    return status;
}

CsrBool pal_dam_read_failed_contact_counter(dataManagerContext *damContext,
                                                        CsrUint16 logicalLinkHandle,
                                                        CsrUint16 *failedContactCounter)
{
    logicalLinkList *dataLinkList = damContext->dataLinkList;
    CsrBool status=FALSE;

    pdamLogicalLinkAttrib *logicalLink = pal_dam_get_link_attrib(dataLinkList, logicalLinkHandle);

    if (logicalLink && logicalLinkHandle == logicalLink->logicalLinkHandle)
    {
        status = TRUE;
        *failedContactCounter = logicalLink->failedContactCounter;
    }
    return status;
}

void pal_dam_deinit(FsmContext *context, dataManagerContext *damContext, CsrBool shutdown)
{
    sme_trace_entry((TR_PAL_DAM, "pal_dam_deinit(): %d", damContext->dataLinkList));

    sme_trace_entry((TR_PAL_DAM, "pal_dam_deinit(): %s triggered", shutdown?"Shutdown":"Reset"));

    pal_dam_logical_links_deinit(context, damContext);

    /* if its a software reset then hold onto the memory. For eg: a firmware crash.
        * If its a shutdown then free it
        */
    if (shutdown)
    {
        CsrPfree(damContext->dataLinkList);
        CsrPfree(damContext);
    }
    else
    {
        /* reset credit info as this will be read again from the driver once reset is complete */
        CsrMemSet(damContext->QosCreditInfo, 0, sizeof(creditInfo));
        CsrMemSet(&damContext->cmdQueue, 0, sizeof(PAL_DamCommandQ));

#ifdef SME_TRACE_ENABLE
        damContext->avgInterPacketArrivalTime=0;
        damContext->totalTxPktCount=0;
        damContext->highestInterPacketArrivalTime=0;
        damContext->pktsWithGreaterThan5msArrTime=0;
        damContext->totalTimePktsWithHighLatency=0;
        damContext->totalTxPktSize=0;
        damContext->avgStatusIndTime=0;
        damContext->totalStatusInds=0;
        damContext->totalSuccessfullTx=0;
        damContext->totalFailedTx=0;
        damContext->totalUnitdataInds=0;
        damContext->totalUnitdataIndSize=0;
        damContext->avgInterUnitdataIndTime=0;
        damContext->unitdataIndsWithGreaterThan5msArrTime=0;
        damContext->unitdataIndsTotalTimePktsWithHighLatency=0;
        damContext->unitdataIndsHighestInterPacketArrivalTime=0;
        damContext->numFlushRequests=0;
        damContext->numFlushTimerExpiries=0;
        damContext->numTotalPacketsFlushTriggered=0;
        damContext->numTotalPacketsFlushedSuccessfully=0;
#endif

    }
    sme_trace_entry((TR_PAL_DAM, "pal_dam_deinit(): %s complete", shutdown?"Shutdown":"Reset"));

}

void pal_dam_data_init(dataManagerContext **damContext)
{
    sme_trace_entry((TR_PAL_DAM, "pal_dam_data_init()"));

    *damContext = CsrPmalloc(sizeof(dataManagerContext));
    CsrMemSet(*damContext, 0, sizeof(dataManagerContext));

    (*damContext)->dataLinkList = pal_dam_logical_links_init();

    /* Enabled HCI events by default */
    (*damContext)->eventMask.qosViolationEventEnabled = TRUE;
    (*damContext)->eventMask.numCompletedDataBlocksEventEnabled = TRUE;

    sme_trace_entry((TR_PAL_DAM, "pal_dam_data_init(): %d", (*damContext)->dataLinkList));

}

CsrUint16 pal_dam_init_queue_info(dataManagerContext *damContext,
                                    CsrUint16 trafficQueueSize,
                                    CsrUint16 commandQueueSize)
{
    CsrInt32 i;

    sme_trace_entry((TR_PAL_DAM, "pal_dam_init_queue_info(): trafficQSize-%d, commandQSize-%d",trafficQueueSize,commandQueueSize));

    damContext->cmdQueue.qSize = commandQueueSize?commandQueueSize-1:commandQueueSize;
    damContext->cmdQueue.freeSlots = damContext->cmdQueue.qSize;

    for (i = 0; i < PAL_DAM_MAX_NUM_OF_QOS; i++)
    {
        damContext->QosCreditInfo[i].totalDataBlocks = trafficQueueSize?trafficQueueSize-1:trafficQueueSize;
        damContext->QosCreditInfo[i].numFreeDataBlocks = damContext->QosCreditInfo[i].totalDataBlocks;
    }
    return damContext->QosCreditInfo[0].totalDataBlocks;
}

CsrUint32 pal_dam_process_ma_unitdata_cfm(FsmContext *context,
                                              dataManagerContext *damContext,
                                              unifi_Status result,
                                              unifi_TransmissionStatus transmissionStatus,
                                              unifi_Priority providedPriority,
                                              unifi_ServiceClass providedServiceClass,
                                              CsrUint32 reqIdentifier)
{
    CsrUint32 nextTimeoutValue=0;
#ifdef SME_TRACE_ENABLE
    {
        CsrUint32 now = GET_CURRENT_TIME_IN_MICROS();
        static CsrUint32 prevTime=0;
        CsrUint32 timediff;
        timediff = TIME_DIFF(prevTime,now);
        prevTime=now;

        if (damContext->totalStatusInds>0)
        {
            if (damContext->avgStatusIndTime == 0)
            {
                damContext->avgStatusIndTime = timediff;
            }
            else
            {
                damContext->avgStatusIndTime = (damContext->avgStatusIndTime+timediff)/2;
            }
        }
        damContext->totalStatusInds++;
    }
#endif
    sme_trace_info((TR_PAL_DAM, "pal_dam_process_ma_unitdata_cfm(): result-%d, transmissionStatus-%d, reqId-0x%x\n",result,transmissionStatus,reqIdentifier));

    /* Veirfy if the the confirm is meant for paldata by checking the app handle. */
    if (UNIFI_PALDATA_APPHANDLE == GET_APPHANDLE_FROM_REQ_IDENTIFIER(reqIdentifier))
    {
        pdamLogicalLinkAttrib *logicalLink = pal_dam_get_link_attrib(damContext->dataLinkList, GET_LINK_HANDLE_FROM_REQ_IDENTIFIER(reqIdentifier));
        CsrUint32 timeNow = GET_CURRENT_TIME_IN_MICROS();

        if (logicalLink &&
            (unifi_PalServicetypeGuaranteed == logicalLink->txFlowSpec.serviceType ||
            unifi_PalServicetypeBestEffort == logicalLink->txFlowSpec.serviceType) &&
            logicalLink->userPriority == providedPriority)
        {
            PAL_DamTxPacketInfo *txPktInfo = TX_PKT_GET_OLDEST_PACKET_IN_LIST(logicalLink);

            sme_trace_info((TR_PAL_DAM, "pal_dam_process_ma_unitdata_cfm(): serviceType-%s, link handle-%d\n",
                           GET_SERVICE_TYPE_STR(logicalLink->txFlowSpec.serviceType), logicalLink->logicalLinkHandle));

            /* Ensure if the recieved ACK is the expected ack. ACks must come in same sequence as the packets transmitted.
            * So verify it against the expected packet tag
            */
            if (TX_PKT_INFO_VALID(txPktInfo) &&
                txPktInfo->packetTag == GET_PACKET_TAG_FROM_REQ_IDENTIFIER(reqIdentifier))
            {
                CsrUint32 nextFlushTimeoutVal;
                CsrBool flushPendingOnThisPacket = txPktInfo->flushPending;
                CsrBool queueFlushed = FALSE;

                sme_trace_info((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm():packet acked for link handle - %d",logicalLink->logicalLinkHandle));

                /* Check if there is a qos violation */
                if (unifi_PalServicetypeGuaranteed == logicalLink->txFlowSpec.serviceType) /* it must be a guaranteed link */
                {
                    sme_trace_info((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm():timeNow-%u,expTime-%u,accessLatency-%d in MicroSec",timeNow,txPktInfo->expiryTime,logicalLink->txFlowSpec.accessLatency));

                    if (TX_PKT_TIME_TAKEN(txPktInfo,timeNow) > logicalLink->txFlowSpec.accessLatency &&
                        !logicalLink->flushPendingCount && /* send violation only if no flush pending */
                        damContext->eventMask.qosViolationEventEnabled) /* and event should be enabled*/
                    {
                        sme_trace_info((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm():Qos violation-tx expTime-%u,lat-%d,timeNow-%u",txPktInfo->expiryTime,logicalLink->txFlowSpec.accessLatency, timeNow));
                        call_hci_qos_violation_evt(context, logicalLink->logicalLinkHandle);
                    }
                }
                sme_trace_info((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm():totalFlushPendingCount-%d,flushPendingOnThisPacket-%d",
                 logicalLink->flushPendingCount,txPktInfo->flushPending));

                /* delete the node with the packet info before caculating next flush timeout*/
                TX_PKT_DELETE_OLDEST_PACKET_IN_LIST(logicalLink);

                logicalLink->receivedAckCount++;
                /* Update the credits and generate event when we have accumultated as many confirms to generate a block ack OR
                * There are no packets pending transmission
                */
                if (logicalLink->receivedAckCount == logicalLink->blockAckCount ||
                    (0 == csr_list_size(&logicalLink->txPacketList)))
                {
                    /* update credit info. Do this before flush occured callback is called so that all compleed-block event
                     * goes before flush-occurred event if any
                     */
                    update_credit_info(context,
                                       damContext,
                                       &damContext->QosCreditInfo[PAL_DAM_GET_QOS_INDEX(logicalLink->userPriority)],
                                       logicalLink->receivedAckCount,
                                       logicalLink->logicalLinkHandle);

                    logicalLink->receivedAckCount = 0;
                }

                /* update the commandQ available slots if flush was pending on this packet. flushPending TRUE means
                * ma-unitdata-cancel-req was sent and used up a slot in the command queue
                */
                if (flushPendingOnThisPacket)
                {
                    damContext->cmdQueue.freeSlots++;
                    sme_trace_info((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm():Flush was pending on this packet. Incremented the free command q slots - %d",damContext->cmdQueue.freeSlots));
                    verify(TR_PAL_DAM,damContext->cmdQueue.freeSlots<=damContext->cmdQueue.qSize);

                    /* Flush the queue. This call is to ensure 2 things:
                     * 1. that we have no expired packets in the queue. It is not inefficient because it will quit looping through packet if it finds
                     *     that first packet in head of queue is not expired. But if it is expired, then this is safe way of getting rid of expired packets.
                     * 2. To flush the next packet if there is any because we have slot available in the command queue.
                     */
                    (void)pal_dam_flush_queue_handler(context,
                                                      damContext,
                                                      logicalLink,
                                                      timeNow);
                    queueFlushed = TRUE;
                }

                /* Update the failed-contact-counter */
                /*As per HCI spec section 6.15 , reset the failed-contact-counter:
                                "When the Failed_Contact_Counter is > zero and an L2CAP PDUpacket is
                                 acknowledged for that connection"
                                 */
                if (TransmissionStatus_Successful == transmissionStatus &&
                    unifi_Success == result)
                {
                    if (logicalLink->failedContactCounter > 0)
                    {
                        sme_trace_info((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm():PDU ackd, failed contact counter reset"));
                        logicalLink->failedContactCounter = 0;
                    }
                }
                /* HCI Spec section 6.15 says:
                                 "The Failed_Contact_Counter records the number of consecutive incidents in
                                  which either the slave or master local or remote device did not respond after
                                  the flush timeout had expired, and the L2CAP PDUpacket that was currently
                                  being transmitted was automatically flushed. When this occurs, the
                                  Failed_Contact_Counter is incremented by 1."*/
                /*increment the counter if flush is pending on this packet and there is no flush request (means automatic flush) */
                else if (flushPendingOnThisPacket && FALSE == logicalLink->flushAllPacketsPending)
                {
                    logicalLink->failedContactCounter++;
                    sme_trace_info((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm():PDU flushed, failed contact counter incremented to - %d",logicalLink->failedContactCounter));
                }

                /* If flush is not pending then go ahead with flush timer. If flush is pending
                 * then timer need not be restarted because the timer that is running (if any, in case of automatic
                 * flush. or no timer running if any all packets are being flushed) will be for the packet that
                 * needs to be transmitted.
                 */
                if (logicalLink->flushPendingCount)
                {
                    sme_trace_crit((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm():flush pending-%d packets",logicalLink->flushPendingCount));

                    /* if flush is pending on this particular packet then do the processing. otherwise
                     * leave it
                     */
                    if (flushPendingOnThisPacket)
                    {
#ifdef SME_TRACE_ENABLE
                        if (transmissionStatus != TransmissionStatus_Successful || result != unifi_Success )
                        {
                            damContext->numTotalPacketsFlushedSuccessfully++;
                        }
                        else
                        {
                            damContext->totalSuccessfullTx++;
                        }
#endif
                        if ((transmissionStatus != TransmissionStatus_Successful || result != unifi_Success) &&
                            !logicalLink->flushOccured)
                        {
                            sme_trace_crit((TR_PAL_DAM, "pal_dam_process_ma_unitdata_cfm(): Atleast one packet is flushed"));
                            logicalLink->flushOccured = TRUE;
                        }

                        logicalLink->flushPendingCount--;
                        if (!logicalLink->flushPendingCount)
                        {
                            if (logicalLink->flushOccured)
                            {
                                sme_trace_crit((TR_PAL_DAM, "pal_dam_process_ma_unitdata_cfm(): flushing complete. sending hci-flush-occured event"));
                                call_hci_flush_occurred_evt(context, logicalLink->logicalLinkHandle);
                            }
                            if (logicalLink->cbFlushOccuredFinalResponse)
                            {
                                sme_trace_info((TR_PAL_DAM, "pal_dam_process_ma_unitdata_cfm(): flushing complete. sending final response"));
                                logicalLink->cbFlushOccuredFinalResponse(context, logicalLink->logicalLinkHandle,logicalLink->flushOccured);
                            }

                            logicalLink->flushOccured = FALSE;
                            logicalLink->flushAllPacketsPending= FALSE;

                            if (logicalLink->cbLogicalLinkDeleted)
                            {
                                 /* Save these three parameters as the link entry will be deleted before doing a callback. */
                                 CsrUint16 logicalLinkHandle = logicalLink->logicalLinkHandle;
                                 CsrUint8 physicalLinkHandle = logicalLink->physicalLinkHandle;
                                 void (* cbLogicalLinkDeleted)(FsmContext *context, CsrUint16 logLinkHandle,CsrUint8 phyLinkHandle) = logicalLink->cbLogicalLinkDeleted;

                                 sme_trace_info((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm(): delete Logical link callback set. so call it"));
                                 (void)delete_logical_link_entry(context, damContext->dataLinkList, logicalLink->logicalLinkHandle);
                                 sme_trace_info((TR_PAL_DAM,"delete_logical_link_entry_with_callback: no packets to flush so do a calll back straight"));

                                 cbLogicalLinkDeleted(context,logicalLinkHandle,physicalLinkHandle);
                            }
                        }
                    }
#ifdef SME_TRACE_ENABLE
                    else
                    {
                        if (transmissionStatus != TransmissionStatus_Successful || result != unifi_Success)
                        {
                            sme_trace_crit((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm():flushPending: TX failed without requesting flush on this pkt. TxStatus-%d",transmissionStatus));
                            damContext->totalFailedTx++;
                        }
                        else
                        {
                            damContext->totalSuccessfullTx++;
                        }
                    }
#endif
                }
                else
                {
#ifdef SME_TRACE_ENABLE
                    if (transmissionStatus != TransmissionStatus_Successful || result != unifi_Success)
                    {
                        /* report error only if a flush timer is running for the link. A BE link may have no flush timer */
                        if (logicalLink->txFlowSpec.flushTimeout > 0)
                        {
                            sme_trace_crit((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm(): TX failed without requesting flush  on this pkt. TxStatus-%d",transmissionStatus));
                        }
                        damContext->totalFailedTx++;
                    }
                    else
                    {
                        damContext->totalSuccessfullTx++;
                    }
#endif

                   if (logicalLink->txFlowSpec.flushTimeout > 0)
                   {
                        if (FALSE == queueFlushed)
                        {
                            (void)pal_dam_flush_queue_handler(context,
                                                              damContext,
                                                              logicalLink,
                                                              timeNow);
                        }
                        nextFlushTimeoutVal = get_next_flush_timer_value(context, logicalLink, timeNow);
                        if (nextFlushTimeoutVal > 0)
                        {
                            pal_dam_restart_flush_timer(context,
                                                        logicalLink,
                                                        nextFlushTimeoutVal);
                            nextTimeoutValue = nextFlushTimeoutVal;
                        }
                        else
                        {
                            pal_dam_stop_flush_timer(context,logicalLink);
                        }
                    }
                }
            }
            else
            {
#ifdef SME_TRACE_ENABLE
                if (txPktInfo)
                {
                    sme_trace_error((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm(): unexpected packet tag received-0x%x, expected-0x%x",GET_PACKET_TAG_FROM_REQ_IDENTIFIER(reqIdentifier),txPktInfo->packetTag));
                }
                else
                {
                    sme_trace_error((TR_PAL_DAM,"pal_dam_process_ma_unitdata_cfm(): No packet pending tx"));
                }
#endif
            }
        }
        else
        {
            sme_trace_error((TR_PAL_DAM, "pal_dam_process_ma_unitdata_cfm(): Invalid link handle or parameters in the reqIdentifier"));
        }
    }
    else
    {
        sme_trace_error((TR_PAL_DAM, "pal_dam_process_ma_unitdata_cfm(): Unexpected app handle in reqIdentifier. (expected app handle-%d, expected-%d)", GET_APPHANDLE_FROM_REQ_IDENTIFIER(reqIdentifier),UNIFI_PALDATA_APPHANDLE));
    }
    return nextTimeoutValue;
}

CsrUint32 pal_acl_process_downstream_data(FsmContext *context,
                                             dataManagerContext *damContext,
                                             PAL_DataBlock *dataBlock,
                                             PalAclheader *aclHeader,
                                             unifi_FrameFreeFunction freeFunction)
{
    pdamLogicalLinkAttrib *logicalLink;
    CsrUint16 length = aclHeader->length;
    CsrBool validPacket=TRUE;
    CsrUint32 nextTimeoutValue=0;

#ifdef SME_TRACE_ENABLE
    static CsrUint32 prevTime=0;
    CsrUint32 now = GET_CURRENT_TIME_IN_MICROS();
    CsrUint32 timediff;
    timediff = TIME_DIFF(prevTime,now);
    prevTime=now;

    /* Skip info on first packet */
    if (damContext->totalTxPktCount>0)
    {
        if (timediff > 5000)
        {
            damContext->pktsWithGreaterThan5msArrTime++;
            damContext->totalTimePktsWithHighLatency+=timediff;
        }
        if (damContext->avgInterPacketArrivalTime == 0)
        {
            damContext->avgInterPacketArrivalTime = timediff;
        }
        else
        {
            damContext->avgInterPacketArrivalTime = (damContext->avgInterPacketArrivalTime+timediff)/2;
        }
        if (timediff > damContext->highestInterPacketArrivalTime)
        {
            damContext->highestInterPacketArrivalTime = timediff;
        }
    }
    damContext->totalTxPktCount++;
    damContext->totalTxPktSize+=length;

#endif

    sme_trace_entry((TR_PAL_DAM, "pal_acl_process_downstream_data():length-%d",length));
    logicalLink = pal_dam_get_link_attrib(damContext->dataLinkList,
                                          PAL_DAM_GET_HANDLE(aclHeader->handlePlusFlags));

    if (NULL == logicalLink ||
        aclHeader->length > logicalLink->txFlowSpec.maximumSduSize ||
        aclHeader->length > PAL_MAX_PDU_SIZE)
    {
        sme_trace_crit((TR_PAL_DAM, "pal_acl_process_downstream_data(): invalid logical link handle-%d,%d",PAL_DAM_GET_HANDLE(aclHeader->handlePlusFlags),aclHeader->handlePlusFlags));
        validPacket = FALSE;
    }
    else if (logicalLink->flushAllPacketsPending)
    {
        /* Either Flush_Req or a Delete_Data_Link_Req is penidng.
        * Don't let it through. Generate hci-number-of-completed-data-blocks
        * to keep host going with its credit count
        */
        sme_trace_error((TR_PAL_DAM, "pal_acl_process_downstream_data(): Flush pending on handle %d for %d packets",PAL_DAM_GET_HANDLE(aclHeader->handlePlusFlags),logicalLink->flushPendingCount));
        validPacket = FALSE;
    }
    else
    {
        CsrInt32 qNo = PAL_DAM_GET_QOS_INDEX(logicalLink->userPriority);

        /* Let the packet through only if there is free slots available on tx */
        if (!damContext->QosCreditInfo[qNo].numFreeDataBlocks)
        {
            sme_trace_crit((TR_PAL_DAM, "pal_acl_process_downstream_data(): No Free slots available. packet discarded"));
            validPacket = FALSE;
        }
    }

    if (validPacket)
    {
        /* Everything looks good. So just forward the data */
        /* populate snap header */
        length += pal_populate_snap_header_and_mac_address(context, &dataBlock,logicalLink);

        nextTimeoutValue = pal_send_downstream_acl_data(context,damContext, dataBlock, length, logicalLink,freeFunction);

    }
    else
    {
        /* Invalid or unexpected packet. Free the buffer */
        PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(dataBlock));
        PALDATA_MEM_DOWNSTREAM_FREE_DATA_BLOCK(dataBlock);
    }
    return nextTimeoutValue;
}

void pal_acl_process_upstream_data(FsmContext *context,
                                          dataManagerContext *damContext,
                                          PAL_DataBlock *dataBlock,
                                          unifi_FrameFreeFunction freeFunction)
{
    pdamLogicalLinkAttrib *logicalLink;
    CsrUint8 *unitdata = PALDATA_MEM_GET_BUFFER(dataBlock);
    unifi_MACAddress                da;
    unifi_MACAddress                sa;

    /* Get MAC Addresses from the frame */
    /*lint -save -e545 -e725 */
    event_unpack_buffer(&unitdata, (CsrUint8*)&da, PALDATA_MAC_ADDRESS_SIZE); /*lint !e545 !e725  MAC_Address : dA */
    /*lint -save -e545 -e725 */
    event_unpack_buffer(&unitdata, (CsrUint8*)&sa, PALDATA_MAC_ADDRESS_SIZE); /*lint !e545 !e725  MAC_Address : sA */
    unitdata = PALDATA_MEM_UPSTREAM_REMOVE_MAC_ADDRESS(dataBlock,(PALDATA_MAC_ADDRESS_SIZE*2));

    logicalLink = pal_dam_get_link_attrib_from_remote_mac_address(damContext->dataLinkList,(unifi_MACAddress *)&sa);

    if (logicalLink &&
        0 == CsrMemCmp(logicalLink->localMacAddress.data, da.data, sizeof(da)))
    {
        /* strip SNAP header */
        CsrUint8 *buf;

        unitdata = PALDATA_MEM_UPSTREAM_REMOVE_SNAP_HEADER(dataBlock, PALDATA_SNAP_HEADER_SIZE);

#ifdef SME_TRACE_ENABLE
        {
            CsrUint32 timenow = GET_CURRENT_TIME_IN_MICROS();
            static CsrUint32 prevTime=0;
            CsrUint32 timediff = TIME_DIFF(prevTime,timenow);
            prevTime=timenow;

            /* Skip info on first packet */
            if (damContext->totalUnitdataInds>0)
            {
                if (timediff > 5000)
                {
                    damContext->unitdataIndsWithGreaterThan5msArrTime++;
                    damContext->unitdataIndsTotalTimePktsWithHighLatency+=timediff;
                }
                if (damContext->avgInterUnitdataIndTime == 0)
                {
                    damContext->avgInterUnitdataIndTime = timediff;
                }
                else
                {
                    damContext->avgInterPacketArrivalTime = (damContext->avgInterPacketArrivalTime+timediff)/2;
                }
                if (timediff > damContext->unitdataIndsHighestInterPacketArrivalTime)
                {
                    damContext->unitdataIndsHighestInterPacketArrivalTime = timediff;
                }
            }
            damContext->totalUnitdataInds++;
            damContext->totalUnitdataIndSize+=PALDATA_MEM_GET_BUFFER_LENGTH(dataBlock);
        }
#endif

        buf = PALDATA_MEM_UPSTREAM_ADD_ACL_HEADER(dataBlock, PALDATA_ACL_HEADER_SIZE);

       /* Set  physical link handle for upstream ACL packets. Refer section 5.4.2 of AMP HCI Spec D09r12 in the description for handle */
       (void)event_pack_CsrUint16(&buf,PALDATA_HANDLE_PLUS_FLAGS(logicalLink->physicalLinkHandle));
       (void)event_pack_CsrUint16(&buf, PALDATA_MEM_GET_BUFFER_LENGTH(dataBlock)-PALDATA_ACL_HEADER_SIZE);

       verify(TR_PAL_DAM,(PALDATA_MEM_GET_BUFFER_LENGTH_ORIGINAL(dataBlock)-PALDATA_MEM_GET_BUFFER_LENGTH(dataBlock)) == PALDATA_ACL_OFFSET);

        /* Now send it to the host */
        call_pal_acl_data_ind(context,
                              PALDATA_MEM_GET_BUFFER_LENGTH(dataBlock),
                              PALDATA_MEM_GET_BUFFER_ORIGINAL(dataBlock),
                              logicalLink->physicalLinkHandle,
                              PALDATA_ACL_OFFSET,
                              freeFunction);

        PALDATA_MEM_DOWNSTREAM_FREE_DATA_BLOCK(dataBlock);

        /* We know the remote link is alive.  so extend the expiryTime for link supervision timer */
        logicalLink->lsTableEntry->linkAlive = TRUE;
        logicalLink->lsTableEntry->expiryTime = TIME_ADD(GET_CURRENT_TIME_IN_MICROS(),logicalLink->lsTableEntry->timeoutDuration);
    }
    else
    {
        sme_trace_warn((TR_PAL_DAM, "pal_sys_hip_ind() : ma-unitdata-ind with invalid local/remote mac address or snap"));
        PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,PALDATA_MEM_GET_BUFFER_ORIGINAL(dataBlock));
        PALDATA_MEM_UPSTREAM_FREE_DATA_BLOCK(dataBlock);
    }
}

void pal_dam_set_link_supervision_timeout(FsmContext *context,
                                                dataManagerContext *damContext,
                                                CsrUint8 phyLinkHandle,
                                                CsrUint16 linkSupervisionTimeoutDuration,
                                                CsrBool *firstPhyLink)
{
    linkSupervisionTimerEntry * lsEntry = pal_dam_get_link_supervision_entry(damContext,phyLinkHandle);

    sme_trace_entry((TR_PAL_DAM,"pal_dam_set_link_supervision_timeout: phyHandle-%d, timeout-%d",phyLinkHandle,linkSupervisionTimeoutDuration));

    *firstPhyLink = FALSE;
    if (!lsEntry)
    {
        CsrInt32 i;
        sme_trace_info((TR_PAL_DAM,"pal_dam_set_link_supervision_timeout: no entry with same handle, find unused entry now"));

        for (i=0; i<PAL_DAM_MAX_PHYSICAL_LINKS; i++)
        {
            sme_trace_info((TR_PAL_DAM,"pal_dam_get_link_supervision_entry: ununsed entry %d, handle-%d",i,damContext->linkSupervisionTable[i].physicalLinkHandle));
            if (0 == damContext->linkSupervisionTable[i].physicalLinkHandle)
            {
                sme_trace_info((TR_PAL_DAM,"pal_dam_get_link_supervision_entry: ununsed entry found"));
                damContext->linkSupervisionTable[i].physicalLinkHandle = phyLinkHandle;
                damContext->linkSupervisionTable[i].timeoutDuration = linkSupervisionTimeoutDuration*1000;
                damContext->numActivePhyLinks++;

                if (1 == damContext->numActivePhyLinks)
                {
                    *firstPhyLink = TRUE;
                }

                pal_dam_start_link_loss_timer(context, &damContext->linkSupervisionTable[i]);
                break;
            }
        }
    }
}

void pal_dam_modify_link_supervision_timeout(FsmContext *context,
                                                     dataManagerContext *damContext,
                                                     CsrUint8 phyLinkHandle,
                                                     CsrUint16 linkSupervisionTimeoutDuration)
{
    linkSupervisionTimerEntry * lsEntry = pal_dam_get_link_supervision_entry(damContext,phyLinkHandle);

    sme_trace_entry((TR_PAL_DAM,"pal_dam_modify_link_supervision_timeout: phyHandle-%d, timeout-%d",phyLinkHandle,linkSupervisionTimeoutDuration));

    verify(TR_PAL_DAM,lsEntry!=NULL);
    if (lsEntry)
    {
        if (lsEntry->timeoutDuration != linkSupervisionTimeoutDuration)
        {
            sme_trace_info((TR_PAL_DAM,"pal_dam_modify_link_supervision_timeout: entry found"));
            lsEntry->timeoutDuration = linkSupervisionTimeoutDuration*1000;
            pal_dam_restart_link_loss_timer(context, lsEntry);
        }
    }
}

void pal_dam_delete_link_supervision_timeout(FsmContext *context,
                                                    dataManagerContext *damContext,
                                                    CsrUint8 phyLinkHandle,
                                                    CsrBool *lastPhyLink)
{
    linkSupervisionTimerEntry * lsEntry = pal_dam_get_link_supervision_entry(damContext,phyLinkHandle);

    sme_trace_entry((TR_PAL_DAM,"pal_dam_delete_link_supervision_timeout: phyHandle-%d",phyLinkHandle));

    *lastPhyLink = FALSE;
    if (lsEntry)
    {
        sme_trace_info((TR_PAL_DAM,"pal_dam_delete_link_supervision_timeout: entry found"));
        pal_dam_stop_link_loss_timer(context, lsEntry);
        lsEntry->physicalLinkHandle = 0;
        lsEntry->timeoutDuration = 0;
        damContext->numActivePhyLinks--;

        if (!damContext->numActivePhyLinks)
        {
            *lastPhyLink = TRUE;
        }
    }
}

void pal_dam_link_alive(FsmContext *context,
                           dataManagerContext *damContext,
                           CsrUint8 phyLinkHandle)
{
    linkSupervisionTimerEntry * lsEntry = pal_dam_get_link_supervision_entry(damContext,phyLinkHandle);

    sme_trace_entry((TR_PAL_DAM,"pal_dam_link_alive: phyHandle-%d",phyLinkHandle));

    verify(TR_PAL_DAM,lsEntry!=NULL);
    if (lsEntry)
    {
        sme_trace_info((TR_PAL_DAM,"pal_dam_link_alive: entry found"));
        pal_dam_restart_link_loss_timer(context, lsEntry);
    }
}

CsrBool pal_dam_early_link_loss_timer_expired(FsmContext *context,
                                                    dataManagerContext *damContext,
                                                    CsrUint8 phyLinkHandle)
{
    linkSupervisionTimerEntry * lsEntry = pal_dam_get_link_supervision_entry(damContext,phyLinkHandle);
    CsrBool linkAlive = FALSE;

    sme_trace_entry((TR_PAL_DAM,"pal_dam_early_link_loss_timer_expired: phyHandle-%d",phyLinkHandle));

    verify(TR_PAL_DAM,lsEntry!=NULL);
    if (lsEntry)
    {
        /* If PAL-D have not heard from the remote for more than a second then consider the link as drowsy */
        CsrUint32 timeNow = GET_CURRENT_TIME_IN_MICROS();
        if (lsEntry->linkAlive && (lsEntry->timeoutDuration - TIME_DIFF(timeNow, lsEntry->expiryTime)) <= MIN_EARLY_LINK_LOSS_TIMEOUT_DURATION)
        {
            sme_trace_entry((TR_PAL_DAM,"pal_dam_early_link_loss_timer_expired: link is still alive. %d microsecs to go for link loss (expiry at %u, timeNow-%u)",TIME_DIFF(timeNow, lsEntry->expiryTime),lsEntry->expiryTime,timeNow));
            linkAlive = TRUE;
        }
        start_early_link_loss_timer(context,lsEntry,timeNow);
    }
    return linkAlive;
}

CsrBool pal_dam_link_loss_timer_expired(FsmContext *context,
                                             dataManagerContext *damContext,
                                             CsrUint8 phyLinkHandle)
{
    linkSupervisionTimerEntry * lsEntry = pal_dam_get_link_supervision_entry(damContext,phyLinkHandle);
    CsrBool linkAlive = FALSE;

    sme_trace_entry((TR_PAL_DAM,"pal_dam_link_loss_timer_expired: phyHandle-%d",phyLinkHandle));

    verify(TR_PAL_DAM,lsEntry!=NULL);
    if (lsEntry)
    {
        CsrUint32 timeNow = GET_CURRENT_TIME_IN_MICROS();

        sme_trace_info((TR_PAL_DAM,"pal_dam_link_loss_timer_expired: entry found"));

        /* If PAL-D have not heard from the remote for more than a second then consider the link as dead */
        if (lsEntry->linkAlive && (lsEntry->timeoutDuration - TIME_DIFF(timeNow, lsEntry->expiryTime)) <= MIN_EARLY_LINK_LOSS_TIMEOUT_DURATION)
        {
            sme_trace_entry((TR_PAL_DAM,"pal_dam_link_loss_timer_expired: link is still alive. %d microsecs to go for link loss (expiry at %u, timeNow-%u)",TIME_DIFF(timeNow, lsEntry->expiryTime),lsEntry->expiryTime,timeNow));
            linkAlive = TRUE;
        }

        if (linkAlive)
        {
            sme_trace_info((TR_PAL_DAM,"pal_dam_link_loss_timer_expired: restart the time as link is still alive"));
            start_early_link_loss_timer(context,lsEntry,GET_CURRENT_TIME_IN_MICROS());
        }
        else
        {
            sme_trace_info((TR_PAL_DAM,"pal_dam_link_loss_timer_expired: we have lost the link"));
            lsEntry->linkLossTimer.id = 0;
        }
    }
    return linkAlive;
}

void pal_encode_snap(CsrUint8 *buf, llc_snap_hdr_t *snap)
{
    (void)event_pack_CsrUint8(&buf,snap->dsap);
    (void)event_pack_CsrUint8(&buf,snap->ssap);
    (void)event_pack_CsrUint8(&buf,snap->ctrl);
    (void)event_pack_buffer(&buf, snap->oui, 3);

    /* SNAP Protocol is encoded in big endian (network byte order) - reference ?? */
    buf[0] =  (snap->protocol >>  8) & 0xFF;
    buf[1] = snap->protocol        & 0xFF;
}

void pal_decode_snap(CsrUint8 *buf, llc_snap_hdr_t *snap)
{
    snap->dsap = event_unpack_CsrUint8(&buf);
    snap->ssap = event_unpack_CsrUint8(&buf);
    snap->ctrl = event_unpack_CsrUint8(&buf);
    event_unpack_buffer(&buf, snap->oui, 3);

    /* SNAP Protocol is decoded in big endian (network byte order) - reference ?? */
    snap->protocol = (buf[0] << 8) | buf[1];
}

void pal_dam_set_subscription_handle_and_offset(dataManagerContext *damContext,
                                                         CsrUint8 subscriptionHandle,
                                                         CsrUint16 allocOffset)
{
    damContext->subscriptionHandle = subscriptionHandle;
    damContext->allocOffset = allocOffset;
}

#ifdef FSM_DEBUG_DUMP
void pal_dm_dump(dataManagerContext *damContext)
{
    CsrInt32 i;
    csr_list_node *node;
    logicalLinkList *dataLinkList=damContext->dataLinkList;

    sme_trace_crit((TR_FSM_DUMP, "####### DATA MANAGER DUMP ########"));

    sme_trace_crit((TR_FSM_DUMP, "HCI Event Masks HCI_Qos_Violation:%s, HCI_Num_Completed_Data_Blocks-%s",
         damContext->eventMask.qosViolationEventEnabled?"Enabled":"Disabled",
         damContext->eventMask.numCompletedDataBlocksEventEnabled?"Enabled":"Disabled"));

    sme_trace_crit((TR_FSM_DUMP, "Link Supervision Timeout table:"));
    for (i=0;i<PAL_DAM_MAX_PHYSICAL_LINKS;i++)
    {
        sme_trace_crit((TR_FSM_DUMP, "handle: %d, timeout-%d,",
              damContext->linkSupervisionTable[i].physicalLinkHandle,
              damContext->linkSupervisionTable[i].timeoutDuration));
        sme_trace_crit((TR_FSM_DUMP, "Timers: earlyLinkLossRunning:%s, linkLossRunning-%s,",
              (damContext->linkSupervisionTable[i].earlyLinkLossTimer.id?"TRUE":"FALSE"),
              (damContext->linkSupervisionTable[i].linkLossTimer.id?"TRUE":"FALSE")));
    }

    sme_trace_crit((TR_FSM_DUMP, "Command Queue size-%d, freeSlots-%d",damContext->cmdQueue.qSize, damContext->cmdQueue.freeSlots));

    for (i=0;i<PAL_DAM_MAX_NUM_OF_QOS;i++)
    {
        sme_trace_crit((TR_FSM_DUMP, "Credit Info For %s queue",i==0?"Qos4_7(Guaranteed)":"Qos0(Best Effort)"));
        sme_trace_crit((TR_FSM_DUMP, "Credit Info: total-%d,freeBlocks-%d",
                        damContext->QosCreditInfo[i].totalDataBlocks,
                        damContext->QosCreditInfo[i].numFreeDataBlocks));
    }

    sme_trace_crit((TR_FSM_DUMP,"Packet info for Logical Links: Num link present: %d",csr_list_size(&dataLinkList->logicalLinkList)));
    node = csr_list_gethead_void(&dataLinkList->logicalLinkList);

    while (node)
    {
        pdamLogicalLinkAttrib *logicalLink = (pdamLogicalLinkAttrib *)node->value;

        sme_trace_crit((TR_FSM_DUMP,"Packet info for Logical Links: Pointer: 0x%p",logicalLink));
        if (logicalLink)
        {
            csr_list_node *pktNode;

            sme_trace_crit((TR_FSM_DUMP,"LL Handle %d, PL Hdl-%d,priority-%d,failedContactCounter-%d",
                            logicalLink->logicalLinkHandle,logicalLink->physicalLinkHandle,logicalLink->userPriority,
                            logicalLink->failedContactCounter));
            sme_trace_crit((TR_FSM_DUMP,"flushPendingCount-%d,id-%d",logicalLink->flushPendingCount,
                            logicalLink->flushTimerId.id));
            sme_trace_crit((TR_FSM_DUMP,"TxFlowSpec:"));
            sme_trace_crit((TR_FSM_DUMP,"serviceType:%s,maxSDUSize-%d,sduInterArrTime-%d,accessLat-%d,flushTO-%d",
                            GET_SERVICE_TYPE_STR(logicalLink->txFlowSpec.serviceType),
                            logicalLink->txFlowSpec.maximumSduSize,logicalLink->txFlowSpec.sduInterArrivalTime,
                            logicalLink->txFlowSpec.accessLatency,
                            logicalLink->txFlowSpec.flushTimeout));

            sme_trace_crit((TR_FSM_DUMP,"LocalMacAddress: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
                            logicalLink->localMacAddress.data[0],logicalLink->localMacAddress.data[1],
                            logicalLink->localMacAddress.data[2],logicalLink->localMacAddress.data[3],
                            logicalLink->localMacAddress.data[4],logicalLink->localMacAddress.data[5]));
            sme_trace_crit((TR_FSM_DUMP,"RemoteMacAddress: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
                            logicalLink->remoteMacAddress.data[0],logicalLink->remoteMacAddress.data[1],
                            logicalLink->remoteMacAddress.data[2],logicalLink->remoteMacAddress.data[3],
                            logicalLink->remoteMacAddress.data[4],logicalLink->remoteMacAddress.data[5]));

            sme_trace_crit((TR_FSM_DUMP,"Packet List : NumPacketsInList-%d",csr_list_size(&logicalLink->txPacketList)));

            pktNode = csr_list_gethead_void(&logicalLink->txPacketList);

            while (pktNode)
            {
                PAL_DamTxPacketInfo *newPktInfo = (PAL_DamTxPacketInfo *)pktNode->value;

                sme_trace_crit((TR_FSM_DUMP,"ExpiryTime :%u micros, LogicalLink Pointer-0x%p",newPktInfo->expiryTime,newPktInfo->logicalLink));
                sme_trace_crit((TR_FSM_DUMP,"LL Handle :%d, PL Handle-%d",newPktInfo->logicalLink->logicalLinkHandle,newPktInfo->logicalLink->physicalLinkHandle));
                pktNode = csr_list_getnext_void(&logicalLink->txPacketList,pktNode);
            }
        }
        node = csr_list_getnext_void(&dataLinkList->logicalLinkList,node);
    }
    sme_trace_crit((TR_FSM_DUMP,"Statistics - TX"));
    sme_trace_crit((TR_FSM_DUMP,"total Tx Packets: Count-%d, size-%d",damContext->totalTxPktCount,damContext->totalTxPktSize));
    sme_trace_crit((TR_FSM_DUMP,"total Tx Packets: avgInterPktArrivalTime-%d, highestInterArrivalTime-%d",damContext->avgInterPacketArrivalTime,damContext->highestInterPacketArrivalTime));
    sme_trace_crit((TR_FSM_DUMP,"total Tx Packets: BigArrivalDifferenceCount(Greater than 5ms)-%d ",damContext->pktsWithGreaterThan5msArrTime));
    sme_trace_crit((TR_FSM_DUMP,"total Tx Packets: total time lost in big inter-arrv-time-%d ",damContext->totalTimePktsWithHighLatency));
    sme_trace_crit((TR_FSM_DUMP,"Statistics - Unitdata Cfms "));
    sme_trace_crit((TR_FSM_DUMP,"total UnitData_Cfms: count-%d, avgTime-%d",damContext->totalStatusInds,damContext->avgStatusIndTime));
    sme_trace_crit((TR_FSM_DUMP,"total UnitData_Cfms: Packets successfullu Txd-%d, Packets failed to tx-%d(This should never be>0 if flushTimer was running)",damContext->totalSuccessfullTx,damContext->totalFailedTx));

    sme_trace_crit((TR_FSM_DUMP,"Statistics - RX"));
    sme_trace_crit((TR_FSM_DUMP,"total Rx Packets: Count-%d, size-%d",damContext->totalUnitdataInds,damContext->totalUnitdataIndSize));
    sme_trace_crit((TR_FSM_DUMP,"total Rx Packets: avgInterPktArrivalTime-%d, highestInterArrivalTime-%d",damContext->avgInterUnitdataIndTime,damContext->unitdataIndsHighestInterPacketArrivalTime));
    sme_trace_crit((TR_FSM_DUMP,"total Rx Packets: BigArrivalDifferenceCount(Greater than 5ms)-%d ",damContext->unitdataIndsWithGreaterThan5msArrTime));
    sme_trace_crit((TR_FSM_DUMP,"total Rx Packets: total time lost in big inter-arrv-time-%d ",damContext->totalTimePktsWithHighLatency));

    sme_trace_crit((TR_FSM_DUMP,"Statistics - FLUSH"));
    sme_trace_crit((TR_FSM_DUMP,"total : Flush Requests-%d, Flush Timeouts-%d",damContext->numFlushRequests,damContext->numFlushTimerExpiries));
    sme_trace_crit((TR_FSM_DUMP,"total : Flush Triggered-%d, Successfully Flushed-%d",damContext->numTotalPacketsFlushTriggered, damContext->numTotalPacketsFlushedSuccessfully));

}
#endif

/* PAL memory routines for data path . This needs to be moved to mode specific when needed .
 * For eg: for kernel mode, the function will allocate using unifi_net_data_malloc for downstream
 * packets  FIXME:
 */

PAL_DataBlock *pal_alloc_data_block(CsrUint16 len, CsrUint8 *data, CsrBool blockOnly, CsrUint8 *releaseData)
{
    PAL_DataBlock *dataBlock = CsrPmalloc(sizeof(PAL_DataBlock));
    
    sme_trace_entry((TR_PAL_DAM,"pal_alloc_data_block:blockOnly-%d releaseData-%p if not NULL will release %p, data ptr-%p, new dataBlock is %p",blockOnly,releaseData,releaseData,data,dataBlock));

    dataBlock->length = 0;
    if (blockOnly)
    {
        /* we are just pointing to a data memory hoping to resuse */
        dataBlock->length = len;
        dataBlock->pData = data;
        dataBlock->pRelease = releaseData;
    }
    else
    {
        dataBlock->pData = CsrPmalloc(len);
        /* Copy the data if requested and a valid memory is available */
        if (data)
        {
            CsrMemCpy(dataBlock->pData, data, len);
            dataBlock->length = len;
        }
        dataBlock->pRelease = dataBlock->pData;
    }

    dataBlock->lengthOrig = len;
    dataBlock->pDataOrig = dataBlock->pData;
        
    return dataBlock;

}


CsrUint8 *pal_push_data_block(PAL_DataBlock *dataBlock, CsrUint16 length)
{
    sme_trace_entry((TR_PAL_DAM,"pal_push_data_block:head dataBlock-%p, length-%d",dataBlock, length));
    
    verify(TR_PAL_DAM, dataBlock!=NULL && length+dataBlock->length <= dataBlock->lengthOrig);
    
    dataBlock->pData -= length;
    dataBlock->length += length;
    return dataBlock->pData;
}

CsrUint8 *pal_pull_data_block(PAL_DataBlock *dataBlock, CsrUint16 length)
{
    sme_trace_entry((TR_PAL_DAM,"pal_pull_data_block:head dataBlock-%p, length-%d, current total dataBlock length - %d",dataBlock,length,dataBlock->length));

    verify(TR_PAL_DAM, dataBlock!=NULL && length <= dataBlock->length);

    dataBlock->pData += length;
    dataBlock->length -= length;
    return dataBlock->pData;
}

void pal_free_data_block(PAL_DataBlock *dataBlock)
{
    verify(TR_PAL_DAM, dataBlock!=NULL);
    sme_trace_entry((TR_PAL_DAM,"pal_free_data_block:"));
    
    if (dataBlock->pRelease)
    {
        sme_trace_entry((TR_PAL_DAM,"pal_free_data_block: freeing data - %p",dataBlock->pRelease));
        CsrPfree(dataBlock->pRelease);
    }
    CsrPfree(dataBlock);
}

void pal_free_data_buffer(void* ptr)
{
    sme_trace_entry((TR_PAL_DAM,"pal_free_data_buffer: free pointer-%p", ptr));
    CsrPfree(ptr);
}

/** @}
 */
