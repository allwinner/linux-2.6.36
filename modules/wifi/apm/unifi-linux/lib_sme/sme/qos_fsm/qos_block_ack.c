/** @file qos_block_ack.c
 *
 * Block Acknowledgment support functions
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
 * Block Acknowledgment support functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_block_ack.c#3 $
 *
 ****************************************************************************/

/** @{
 * @ingroup qos
 */

#include "qos_fsm/qos_block_ack.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "smeio/smeio_trace_types.h"


/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Searches within the Block Ack list for a BSSID
 *
 * @par Description
 *   See Brief
 *
 * @param[in] pCtrlBlk : Block Ack control block
 * @param[in] peerAddress : address to search for
 *
 * @return
 *   void
 */
qos_blockAckBssidNode* search_for_bssid_node(
        qos_blockAckCtrlBlk *pCtrlBlk,
        unifi_MACAddress peerAddress)
{
    qos_blockAckBssidNode *pCurrentNode = NULL;

    for (pCurrentNode = csr_list_gethead_t(qos_blockAckBssidNode *, &pCtrlBlk->blockAckBssidList); pCurrentNode != NULL;
         pCurrentNode = csr_list_getnext_t(qos_blockAckBssidNode *, &pCtrlBlk->blockAckBssidList, &pCurrentNode->node))
    {
        if (CsrMemCmp( &peerAddress, &pCurrentNode->peerAddress, sizeof(peerAddress.data)) == 0)
        {
            break;
        }
    }

    return pCurrentNode;
}


/**
 * @brief
 *   SME QOS FSM Dialogue Token
 *
 * @par Description
 *   See Brief
 *
 * @param[in] pCtrlBlk : Block Ack control block
 *
 * @return
 *   DialogToken, generated token
 */
static DialogToken get_ba_dialog_token(qos_blockAckCtrlBlk *pCtrlBlk)
{
    DialogToken dialogToken;

    sme_trace_entry((TR_QOS, "qos_get_dialog_token()"));

    dialogToken = pCtrlBlk->blockAckDialogToken;

    /* advance to the next token */
    pCtrlBlk->blockAckDialogToken++;

    /* 0 is reserved, advance past it */
    if(pCtrlBlk->blockAckDialogToken == 0)
    {
        pCtrlBlk->blockAckDialogToken++;
    }

    return dialogToken;
}

/**
 * @brief
 *   Returns the the supported block ack policy
 *
 * @par Description
 *   See Brief
 *
 * @return
 *   BlockAckPolicy: the supported block ack policy
 */
static BlockAckPolicy get_block_ack_policy(void)
{
    return BlockAckPolicy_Immediate;
}

/**
 * @brief
 *   SME QOS FSM Dialogue Token
 *
 * @par Description
 *   See Brief
 *
 * @param[in] context : FSM context
 * @param[in] pCtrlBlk : Block Ack control block
 * @param[in] peerAddress : pear address
 * @param[in] tid : tid
 *
 * @return
 *   void
 */
void add_block_ack_to_outstanding(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk,
        unifi_MACAddress peerAddress,
        CsrUint8 tid)
{
    qos_blockAckBssidNode *pCurrentNode = search_for_bssid_node(pCtrlBlk, peerAddress);

    /* if it did not exist, add it */
    if(pCurrentNode == NULL)
    {
        pCurrentNode = CsrPmalloc(sizeof(qos_blockAckBssidNode));
        pCurrentNode->tidMask = 0;
        sme_trace_debug((TR_QOS, "tidMask =0x%02x", pCurrentNode->tidMask));

        CsrMemCpy( &pCurrentNode->peerAddress, &peerAddress, sizeof(peerAddress.data) );

        csr_list_insert_head(&pCtrlBlk->blockAckBssidList,  list_owns_neither, &pCurrentNode->node, pCurrentNode);
    }

    /* update the TID flag */
    pCurrentNode->tidMask |= (CsrUint8)(0x01 << tid);
    sme_trace_debug((TR_QOS, "tidMask =0x%02x", pCurrentNode->tidMask));
}



/**
 * @brief
 *   Checks to see if there is an active BA for the Peer address
 *
 * @par Description
 *   See Brief
 *
 * @param[in] context : FSM context
 * @param[in] pCtrlBlk : Block Ack control block
 * @param[in] peerAddress : pear address
 * @param[in] tid : tid
 *
 * @return
 *   CsrBool: TRUE: Present
 *            FALSE: not present
 */
CsrBool check_for_active_ba(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk,
        unifi_MACAddress peerAddress,
        CsrUint8 tid)
{
    qos_blockAckBssidNode *pCurrentNode = NULL;

    sme_trace_entry((TR_QOS, "check_for_active_ba %x", tid));
    pCurrentNode = search_for_bssid_node(pCtrlBlk, peerAddress);

    if (pCurrentNode != NULL)
    {
        sme_trace_debug((TR_QOS, "tidMask present pCurrentNode->tidMask %x  (0x01 << tid) %x", pCurrentNode->tidMask, (0x01 << tid)));
        /* found the BSSID, check the tid */
        if (pCurrentNode->tidMask & (0x01 << tid))
        {
            sme_trace_debug((TR_QOS, "tidMask present"));
            return TRUE;
        }
    }

    sme_trace_debug((TR_QOS, "tidMask not present"));
    return FALSE;
}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in qos_fsm/qos_block_ack.h
 */
/*---------------------------------------------------------------------------*/
void qos_init_block_ack(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk)
{
    pCtrlBlk->blockAckDialogToken = 1;
    csr_list_init(&pCtrlBlk->blockAckBssidList);
}

/*
 * Description:
 * See description in qos_fsm/qos_block_ack.h
 */
/*---------------------------------------------------------------------------*/
void qos_reset_block_ack(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk)
{
    qos_blockAckBssidNode *pCurrentNode;

    sme_trace_entry((TR_QOS, "qos_reset_block_ack"));

    pCtrlBlk->blockAckDialogToken = 1;

    pCurrentNode = csr_list_gethead_t(qos_blockAckBssidNode *, &pCtrlBlk->blockAckBssidList);

    while (NULL != pCurrentNode)
    {
        sme_trace_debug((TR_QOS, "qos_reset_block_ack(%s) Delete", trace_unifi_MACAddress(pCurrentNode->peerAddress, getMacAddressBuffer(context))));

        (void)csr_list_remove(&pCtrlBlk->blockAckBssidList, &pCurrentNode->node);
        CsrPfree(pCurrentNode);

        pCurrentNode = csr_list_gethead_t(qos_blockAckBssidNode *, &pCtrlBlk->blockAckBssidList);
    }

    /* free any memory */
}

/*
 * Description:
 * See description in qos_fsm/qos_block_ack.h
 */
/*---------------------------------------------------------------------------*/
void qos_process_add_block_ack_req(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk,
        unifi_MACAddress peerAddress,
        CsrUint8 tid)
{
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_debug((TR_QOS, "qos_process_add_block_ack_req(%s) tid %d", trace_unifi_MACAddress(peerAddress, getMacAddressBuffer(context)), tid));

    /* check that the current connection has Draft N capability */
    if( (cfg->joinIECapabilities & DOT11N_Capable)
        &&(cfg->highThroughputOptionEnabled))
    {
        /* if there is an out standing request, ignore it */
        if (!check_for_active_ba(context, pCtrlBlk, peerAddress, tid))
        {
            send_mlme_addba_req(context,
                                peerAddress,                    /* peerQstaAddress */
                                get_ba_dialog_token(pCtrlBlk),  /* dialogToken */
                                tid,                            /* tID */
                                get_block_ack_policy(),         /* blockAckPolicy */
                                16,                             /* bufferSize */
                                0,                              /* blockAckTimeout */
                                100,                            /* addbaFailureTimeout */
                                0);                             /* blockAckStartingSequenceControl */

            /* register the sending of the block ack */
            add_block_ack_to_outstanding(context, pCtrlBlk, peerAddress, tid);
        }
    }
    else
    {
        if (!(cfg->joinIECapabilities & DOT11N_Capable))
        {
            sme_trace_debug((TR_QOS, "AP not HT capable"));
        }
        if (!cfg->highThroughputOptionEnabled)
        {
            sme_trace_debug((TR_QOS, "Station not HT capable"));
        }
    }
}


/*
 * Description:
 * See description in qos_fsm/qos_block_ack.h
 */
/*---------------------------------------------------------------------------*/
void qos_process_del_block_ack_req(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk,
        unifi_MACAddress peerAddress,
        CsrUint8 tid,
        ReasonCode reasonCode)
{
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_debug((TR_QOS, "qos_process_del_block_ack_req(%s) tid %d", trace_unifi_MACAddress(peerAddress, getMacAddressBuffer(context)), tid));

    /* check that the current connection has Draft N capability */
    if( (cfg->joinIECapabilities & DOT11N_Capable)
      &&(cfg->highThroughputOptionEnabled))
    {
        /* if there is an out standing request, ignore it */
        if (check_for_active_ba(context, pCtrlBlk, peerAddress, tid))
        {
            send_mlme_delba_req(context,
                                peerAddress,                    /* peerQstaAddress */
                                Initiator_Originator,           /* Initiator */
                                tid,                            /* tID */
                                reasonCode);                    /* ReasonCode */

            /* register the sending of the block ack */
            remove_block_ack_from_outstanding(context, pCtrlBlk, peerAddress, tid);
        }
        else
        {
            sme_trace_debug((TR_QOS, "not active, ignore"));
        }
    }
    else
    {
        if (!(cfg->joinIECapabilities & DOT11N_Capable))
        {
            sme_trace_debug((TR_QOS, "AP not HT capable"));
        }
        if (!cfg->highThroughputOptionEnabled)
        {
            sme_trace_debug((TR_QOS, "Station not HT capable"));
        }
    }
}

/*
 * Description:
 * See description in qos_fsm/qos_block_ack.h
 */
/*---------------------------------------------------------------------------*/
void remove_block_ack_from_outstanding(
        FsmContext* context,
        qos_blockAckCtrlBlk *pCtrlBlk,
        unifi_MACAddress peerAddress,
        CsrUint8 tid)
{
    qos_blockAckBssidNode *pCurrentNode = search_for_bssid_node(pCtrlBlk, peerAddress);

    sme_trace_entry((TR_QOS, "remove_block_ack_from_outstanding"));

    /* if it did not exist, add it */
    if(pCurrentNode != NULL)
    {
        sme_trace_debug((TR_QOS, "remove_block_ack_from_outstanding(%s) found", trace_unifi_MACAddress(pCurrentNode->peerAddress, getMacAddressBuffer(context))));

        /* update the tid mask */
        pCurrentNode->tidMask &= ~((CsrUint8)(0x01 << tid));
        sme_trace_debug((TR_QOS, "tidMask =0x%02x", pCurrentNode->tidMask));


        /* remove the node if its the last one */
        if(pCurrentNode->tidMask == 0)
        {
            sme_trace_debug((TR_QOS, "remove_block_ack_from_outstanding: node removed"));

            (void)csr_list_remove(&pCtrlBlk->blockAckBssidList, &pCurrentNode->node);
            CsrPfree(pCurrentNode);
        }
    }
}
