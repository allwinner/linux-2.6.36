/** @file roaming_channel_lists.h
 *
 * roaming channel lists header file
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/scan_manager_fsm/roaming_channel_lists.h#2 $
 *
 ****************************************************************************/
#ifndef ROAMING_CHANNEL_LISTS_H
#define ROAMING_CHANNEL_LISTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_internal.h"
#include "hostio/hip_fsm_types.h"
#include "hostio/hip_fsm_events.h"
#include "smeio/smeio_fsm_types.h"

#include "csr_cstl/csr_wifi_list.h"

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

typedef struct RoamingChannelSet
{
    csr_list_node listNode;
    unifi_SSID    ssid;
    CsrUint16     channelListMax;      /* allocated size of channelList */
    CsrUint16     channelListCount;    /* currently used channelList    */
    CsrUint8*     channelList;
} RoamingChannelSet;

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Get the roaming list for a Network
 *
 * @return
 *        RoamingChannelSet* Returns NULL if no entry exists
 */
extern RoamingChannelSet* roaming_channel_list_get(FsmContext* context, const unifi_SSID* ssid);

/**
 * @brief Create or add a channel to a roaming list
 *
 * @par Description
 * If an entry does not exist the create a new one
 *
 * @return
 *        void
 */
extern RoamingChannelSet* roaming_channel_list_add_channel(FsmContext* context, const unifi_SSID* ssid, CsrUint8 channel);

/**
 * @brief Add a channel to a roaming list if a record already exists
 *
 * @return
 *        void
 */
extern RoamingChannelSet* roaming_channel_list_add_channel_if_exists(FsmContext* context, const unifi_SSID* ssid, CsrUint8 channel);

/**
 * @brief Clear out the roaming channel lists
 *
 * @return
 *        void
 */
extern void roaming_channel_list_flush(FsmContext* context);


#ifdef __cplusplus
}
#endif

#endif /*ROAMING_CHANNEL_LISTS_H*/
