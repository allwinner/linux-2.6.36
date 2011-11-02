/** @file roaming_channel_lists.c
 *
 * roaming channel lists source file
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
 *   Functions to manipulate the roaming channel lists
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/scan_manager_fsm/roaming_channel_lists.c#1 $
 *
 ****************************************************************************/
#include "scan_manager_fsm/roaming_channel_lists.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "sme_configuration/sme_configuration_fsm.h"

#include "smeio/smeio_trace_types.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static RoamingChannelSet* findEntry(FsmContext* context, csr_list* roamingChannelLists, const unifi_SSID* ssid)
{
    RoamingChannelSet *currentNode = csr_list_gethead_t(RoamingChannelSet*, roamingChannelLists);
    while (currentNode != NULL)
    {
        if (ssid->length == currentNode->ssid.length &&
            CsrMemCmp(ssid->ssid,currentNode->ssid.ssid, ssid->length) == 0)
        {
            return currentNode;
        }
        currentNode = csr_list_getnext_t(RoamingChannelSet*, roamingChannelLists, &currentNode->listNode);
    }
    return NULL;
}

static RoamingChannelSet* addChannel(FsmContext* context, RoamingChannelSet* node, CsrUint8 channel)
{
    CsrUint16 i;

    /* No Duplicates please */
    for(i = 0; i < node->channelListCount; i++)
    {
        if (node->channelList[i] == channel)
        {
            return NULL;
        }
    }

    /* Reallocate channel list to make more room if needed */
    if (node->channelListCount == node->channelListMax)
    {
        CsrUint8* oldList = node->channelList;
        node->channelListMax *= 2;
        node->channelList = (CsrUint8*)CsrPmalloc(node->channelListMax);
        CsrMemCpy(node->channelList, oldList, node->channelListCount);
        CsrPfree(oldList);
    }

    node->channelList[node->channelListCount] = channel;
    node->channelListCount++;

    sme_trace_entry((TR_SCAN_STORAGE, "roaming channels: network:%s channel:%d", trace_unifi_SSID(&node->ssid, getSSIDBuffer(context)), channel));
    sme_trace_hex((TR_SCAN_STORAGE, TR_LVL_ENTRY, "roaming channels", node->channelList, node->channelListCount));
    return node;
}

/*
 * Description:
 * See description in scan_manager_fsm/roaming_channel_lists.h
 */
/*---------------------------------------------------------------------------*/
RoamingChannelSet* roaming_channel_list_get(FsmContext* context, const unifi_SSID* ssid)
{
    return findEntry(context, &get_sme_config(context)->roamingChannelLists, ssid);
}

/*
 * Description:
 * See description in scan_manager_fsm/roaming_channel_lists.h
 */
/*---------------------------------------------------------------------------*/
RoamingChannelSet* roaming_channel_list_add_channel(FsmContext* context, const unifi_SSID* ssid, CsrUint8 channel)
{
    SmeConfigData* cfg = get_sme_config(context);
    RoamingChannelSet* node = findEntry(context, &cfg->roamingChannelLists, ssid);
    if (node == NULL)
    {
        /* Create New Node */
        node = (RoamingChannelSet*)CsrPmalloc(sizeof(RoamingChannelSet));
        node->ssid = *ssid;
        node->channelListMax = 10;
        node->channelListCount = 1;
        node->channelList = (CsrUint8*)CsrPmalloc(10);
        node->channelList[0] = channel;
        csr_list_insert_head(&cfg->roamingChannelLists, list_owns_value, &node->listNode, node);

        sme_trace_entry((TR_SCAN_STORAGE, "roaming channels: network:%s channel:%d", trace_unifi_SSID(&node->ssid, getSSIDBuffer(context)), channel));
        sme_trace_hex((TR_SCAN_STORAGE, TR_LVL_ENTRY, "roaming channels", node->channelList, node->channelListCount));
        return node;
    }

    return addChannel(context, node, channel);
}

/*
 * Description:
 * See description in scan_manager_fsm/roaming_channel_lists.h
 */
/*---------------------------------------------------------------------------*/
RoamingChannelSet* roaming_channel_list_add_channel_if_exists(FsmContext* context, const unifi_SSID* ssid, CsrUint8 channel)
{
    SmeConfigData* cfg = get_sme_config(context);
    RoamingChannelSet* node = findEntry(context, &cfg->roamingChannelLists, ssid);
    if (node == NULL)
    {
        return NULL;
    }

    return addChannel(context, node, channel);
}

/*
 * Description:
 * See description in scan_manager_fsm/roaming_channel_lists.h
 */
/*---------------------------------------------------------------------------*/
void roaming_channel_list_flush(FsmContext* context)
{
    SmeConfigData* cfg = get_sme_config(context);
    RoamingChannelSet *currentNode = csr_list_gethead_t(RoamingChannelSet*, &cfg->roamingChannelLists);
    while (currentNode != NULL)
    {
        CsrPfree(currentNode->channelList);
        currentNode = csr_list_getnext_t(RoamingChannelSet*, &cfg->roamingChannelLists, &currentNode->listNode);
    }

    csr_list_clear(&cfg->roamingChannelLists);
}
