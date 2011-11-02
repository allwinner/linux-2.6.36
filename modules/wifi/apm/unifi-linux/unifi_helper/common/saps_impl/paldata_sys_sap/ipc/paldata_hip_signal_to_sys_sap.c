/** @file paldata_hip_signal_to_sys_sap.c
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
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/saps_impl/paldata_sys_sap/ipc/paldata_hip_signal_to_sys_sap.c#1 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "paldata_top_level_fsm/paldata.h"
#include "data_manager/data_manager.h"
#include "paldata_sys_sap/paldata_sys_sap_remote_sme_interface.h"
#include "paldata_sys_sap_serialise.h"
#include "ipc/ipc.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/


/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/************************************** PUBLIC FUNCTIONS *******************************************/

#ifdef CUSTOM_PALDATA_SYS_MA_UNITDATA_REQ
void paldata_sys_ma_unitdata_req(void* context, void* appHandle, CsrUint8 subscriptionHandle, CsrUint16 frameLength, const CsrUint8 *frame, unifi_FrameFreeFunction freeFunction, unifi_Priority priority, unifi_ServiceClass serviceClass, CsrUint32 reqIdentifier)

{
    CsrUint8* evt;
    /* freeFunction is given as zero for ease of testing */
    CsrUint16 packedLength = serialise_paldata_sys_ma_unitdata_req(&evt, appHandle, subscriptionHandle, frameLength, frame, freeFunction, priority, serviceClass, reqIdentifier);

    (void)ipc_message_send(get_paldata_sys_ipc_connection(context), evt, packedLength);
    CsrPfree(evt);
    PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,frame);
}
#endif

#ifdef CUSTOM_PALDATA_SYS_HIP_REQ
void paldata_sys_hip_req(void* context, CsrUint16 mlmeCommandLength, const CsrUint8 *mlmeCommand, CsrUint16 dataRef1Length, const CsrUint8 *dataRef1, CsrUint16 dataRef2Length, const CsrUint8 *dataRef2)
{
    (void)ipc_message_send_3(get_paldata_sys_ipc_connection(context),
                           mlmeCommand, mlmeCommandLength,
                           dataRef1, dataRef1Length,
                           dataRef2, dataRef2Length);
}
#endif /* CUSTOM_PALDATA_SYS_HIP_REQ */


