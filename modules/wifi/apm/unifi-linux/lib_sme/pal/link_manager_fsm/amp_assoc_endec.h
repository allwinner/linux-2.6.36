/** @file amp_assoc_endec.h
 *
 * AMP Assoc API
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
 *   AMP Assoc API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/amp_assoc_endec.h#1 $
 *
 ****************************************************************************/
#ifndef AMP_ASSOC_ENDEC_H
#define AMP_ASSOC_ENDEC_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup ip_connection_mgr
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

/* PUBLIC MACROS ************************************************************/
#define AMP_ASSOC_MAX_LENGTH_PER_COMMAND (248)

/* PUBLIC TYPES DEFINITIONS *************************************************/

/**
 * @brief
 *   Information regarding read & write assoc data.
 *
 * @par Description
 *
 */
typedef struct assocDataInfo
{
    CsrUint16 totalLen;
    CsrUint16 currentLen;
    CsrUint8 *data;
}assocDataInfo;

/**
 * @brief
 *   Peer to Peer AMP Assoc structure
 *
 * @par Description
 *   AMP Assoc packet follows TLV format. The type and length fields values are defined
 *   in the macro section The packet format for AMP Assoc is defined in PAL Spec section 7.1
 *
 */
typedef struct AMP_Assoc
{
    PalChannellist channelList;
    unifi_MACAddress macAddress;
    /* PAL Capability field and Vendor private info needs to be added. Ignored for now */
/*    PAL_ExternalConnection extConnection; */
    PalChannellist connectedChannelList;
    PAL_CoexCapabilities capability;

    /* Variable to store Assoc fragment if the received command
     * did not contains a full assoc
     */
    assocDataInfo writeAssocInfo;
}AMP_Assoc;

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
     *   encode amp assoc packet that goes in HCI_Channel_Select or HCI_Read_Local_AMP_Info
     *
     * @par Description
     *
     *
     * @param[in]    context : FSM Context
     * @param[in]    connectedChannelList : connection channel list
     * @param[in]    preferredChannelList : preferred channel list
     * @param[in]    macAddress : Local MAC Address
     * @param[in]    capabilities : PAL Capabilities
     * @param[out]    data : pointer to the AMP Assoc packet. Allocated here. but freed by caller.
     * @param[out]    len : acutal length of the encoded AMP Assoc filled in by the function
     *
     * @return void
     */
extern void pal_encode_amp_assoc(FsmContext *context,
                                 PalChannellist *connectedChannelList,
                                 PalChannellist *preferredChannelList,
                                 unifi_MACAddress *macAddress,
                                 PAL_CoexCapabilities *capabilities,
                                 CsrUint8 **data,
                                 CsrUint16 *len);


/**
 * @brief
 *   decode amp assoc packet that comes in HCI_Create_Physical_Link or HCI_Accept_Physical_Link
 *
 * @par Description
 *
 *
 * @param[in]    context : FSM Context
 * @param[in]    data : pointer to the AMP Assoc packet
 * @param[in]    len : length of the packet
 * @param[out]    assoc:  decoded structure of AMP Assoc to be filled in
 *
 * @return void
 */
extern CsrBool pal_decode_amp_assoc(FsmContext *context,
                                           const CsrUint8 *data,
                                           CsrUint16 len,
                                           AMP_Assoc *assoc);

/**
 * @brief
 *   reset amp assoc
 *
 * @par Description
 *   Resets amp assoc by freeing any resources allocated
 *
 * @param[in]    context : FSM Context
 * @param[out]    assocInfo:  pointer to the Assoc that needs to be reset
 *
 * @return void
 */
extern void pal_reset_amp_assoc_info(FsmContext *context, assocDataInfo *assocInfo);

#ifdef __cplusplus
}
#endif

#endif /* AMP_ASSOC_ENDEC_H */
