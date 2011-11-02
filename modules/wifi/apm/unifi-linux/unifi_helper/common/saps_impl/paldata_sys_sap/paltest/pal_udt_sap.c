/** @file pal_udt_sap.c
 *
 * Test implementation for the UDT sap
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
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/saps_impl/paldata_sys_sap/paltest/pal_udt_sap.c#1 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "paldata_top_level_fsm/paldata.h"
#include "pal_udt_sap/pal_udt_sap_remote_output_interface.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/


/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/************************************** PUBLIC FUNCTIONS *******************************************/

/* FIXME: Change to pal_udt_ma_unitdata_ind */
void pal_udt_send_downstream_acl_data(FsmContext *context, PAL_DataBlock *dataBlock, CsrUint16 length)
{
   /* Message is packed and ready to be sent */
   (void)ipc_message_send(get_pal_udt_ipc_connection(context), dataBlock->pData, length);

   /* Memory needs to be freed */
   pal_free_data_block(dataBlock);
}
