/** @file qos_block_ack.h
 *
 * Public SME QOS Block Acknowledgment API
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
 *   Public SME QOS Block Acknowledgment API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_block_ack.h#2 $
 *
 ****************************************************************************/
#ifndef QOS_BLOCK_ACK_H
#define QOS_BLOCK_ACK_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup qos
 */

/* STANDARD INCLUDES ********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "csr_cstl/csr_wifi_list.h"

/* PROJECT INCLUDES *********************************************************/

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/
typedef struct qos_blockAckCtrlBlk
{
    DialogToken blockAckDialogToken;

    csr_list    blockAckBssidList;

}qos_blockAckCtrlBlk;

typedef struct qos_blockAckBssidNode
{
    csr_list_node node;

    unifi_MACAddress peerAddress;
/*    csr_list blockAckTidList;
 */
    CsrUint8 tidMask;

}qos_blockAckBssidNode;

typedef struct qos_blockAckTidNode
{
    csr_list_node node;

    CsrUint8 tid;

}qos_blockAckTidNode;

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

extern void qos_process_add_block_ack_req(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk,
        unifi_MACAddress peerAddress,
        CsrUint8 tid);

extern void qos_process_del_block_ack_req(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk,
        unifi_MACAddress peerAddress,
        CsrUint8 tid,
        ReasonCode reasonCode);

extern void qos_init_block_ack(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk);

extern void qos_reset_block_ack(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk);

extern void remove_block_ack_from_outstanding(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk,
        unifi_MACAddress peerAddress,
        CsrUint8 tid);

/** \@}
 */

#ifdef __cplusplus
}
#endif


#endif /* QOS_BLOCK_ACK_H */
