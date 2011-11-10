/** @file pal_acl_data_to_acl_sap.c
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
 *   Function to send acl packet to the test environment.
 *
 ****************************************************************************
 *
 * @section MODIFICATION HISTORY
 * @verbatim
 *   #1    17:Apr:08 B-36899: Created
 * @endverbatim
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/saps_impl/paldata_acl_sap/ipc/paldata_acl_signal_to_acl_sap.c#2 $
 *
 ****************************************************************************/

/** @{
 * @ingroup data_manager
 */

/* STANDARD INCLUDES ********************************************************/
#include <stdio.h>

/* PROJECT INCLUDES *********************************************************/

#include "paldata_top_level_fsm/paldata.h"
#include "paldata_acl_sap/paldata_acl_sap_from_sme_interface.h"
#include "data_manager/data_manager.h"
#include "event_pack_unpack/event_pack_unpack.h"
#include "paldata_acl_sap/paldata_acl_sap_remote_sme_interface.h"
#include "paldata_acl_sap_serialise.h"
#include "ipc/ipc.h"


/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/


/* PRIVATE FUNCTION DEFINITIONS *********************************************/
/**
 * @brief
 *   PAL data manager Entry Function
 *
 * @par Description
 *
 * @param[in]    context   : FSM context
 *
 * @return
 *   void
 */

/************************************** PUBLIC FUNCTIONS *******************************************/

#ifdef CUSTOM_PAL_ACL_DATA_IND
void pal_acl_data_ind(void* context, CsrUint16 dataLength, const CsrUint8 *data, CsrUint8 physicalLinkHandle, CsrUint16 aclOffset, unifi_FrameFreeFunction freeFunction)
{
/* Just send the raw ACL data.
 * 1. For the IOP. primary to fit the omnicli traffic generator format
 * 2. To work the L2CAP when running with sockets
 * 3. For customers who use socket interface and BT SIG packed format to communicate with AMP PAL
 */
#ifdef PAL_RAW_ACL_DATA_ENABLED
    sme_trace_msc((TR_MSC, "MSC MESSAGE :: From(%s) Event(%s:ACL Indication: Handle_Plus_Flags = 0x%04x Data_Length = %d Data = 0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x...) To(%s)",
        "PAL_DAM",
        "",
        physicalLinkHandle,
        dataLength,
        (data+aclOffset)[0+4],(data+aclOffset)[1+4],(data+aclOffset)[2+4],(data+aclOffset)[3+4],
        (data+aclOffset)[4+4],(data+aclOffset)[5+4],(data+aclOffset)[6+4],(data+aclOffset)[7+4],
        (data+aclOffset)[8+4],(data+aclOffset)[9+4],
        "ENV"
    ));
    (void)ipc_message_send(get_paldata_acl_ipc_connection(context), data+aclOffset, dataLength);

#else
    CsrUint8* evt;

    CsrUint16 packedLength = serialise_pal_acl_data_ind(&evt, dataLength, data+aclOffset, physicalLinkHandle, 0, 0);

    sme_trace_msc((TR_MSC, "MSC MESSAGE :: From(%s) Event(%s:ACL Indication: Handle_Plus_Flags = 0x%04x Data_Length = %d Data = 0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x...) To(%s)",
        "PAL_DAM",
        "",
        physicalLinkHandle,
        dataLength,
        (data+aclOffset)[0+4],(data+aclOffset)[1+4],(data+aclOffset)[2+4],(data+aclOffset)[3+4],
        (data+aclOffset)[4+4],(data+aclOffset)[5+4],(data+aclOffset)[6+4],(data+aclOffset)[7+4],
        (data+aclOffset)[8+4],(data+aclOffset)[9+4],
        "ENV"
    ));

    /* For test purposes skip the PALDATA_ACL_OFFSET part */
    (void)ipc_message_send(get_paldata_acl_ipc_connection(context), evt, packedLength);
    CsrPfree(evt);
#endif
    PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,data);
}
#endif

/*
 * FSM Scripts Config for this FSM
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/** @}
 */
