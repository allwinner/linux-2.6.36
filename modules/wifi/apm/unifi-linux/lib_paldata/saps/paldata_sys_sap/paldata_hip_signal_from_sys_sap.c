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
 *   $Id: //depot/dot11/v7.0p/host/lib_paldata/saps/paldata_sys_sap/paldata_hip_signal_from_sys_sap.c#3 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "paldata_top_level_fsm/paldata.h"
#include "data_manager/data_manager.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/


/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/************************************** PUBLIC FUNCTIONS *******************************************/
void paldata_sys_hip_ind(FsmContext* context,

                             CsrUint16 mlmeCommandLength,
                             const CsrUint8 *mlmeCommand,
                             CsrUint16 dataRef1Length,
                             const CsrUint8 *dataRef1,
                             CsrUint16 dataRef2Length,
                             const CsrUint8 *dataRef2)
{
}

void paldata_sys_ma_unitdata_cfm(FsmContext* context,
                                 void* appHandle,
                                 unifi_Status result,
                                 unifi_TransmissionStatus transmissionStatus,
                                 unifi_Priority providedPriority,
                                 unifi_ServiceClass providedServiceClass,
                                 CsrUint32 reqIdentifier)
{
#ifndef CSR_WIFI_PALDATA_DIRECT_ACCESS_ENABLE
    sme_trace_entry((TR_PAL_SYS_SAP,"paldata_sys_ma_unitdata_cfm: Send signal to PAL-D"));
    send_paldata_sys_ma_unitdata_cfm_external(context,
                                              getPalDataContext(context)->palDamFsmInstance,
                                              appHandle,
                                              result,
                                              transmissionStatus,
                                              providedPriority,
                                              providedServiceClass,
                                              reqIdentifier);
#else
    sme_trace_entry((TR_PAL_SYS_SAP,"paldata_sys_ma_unitdata_cfm: Call direct access function to handle the message"));
    (void)paldata_process_unitdata_cfm(context, result, transmissionStatus, providedPriority, providedServiceClass, reqIdentifier);
#endif
}


void paldata_sys_ma_unitdata_ind(FsmContext* context,
                                       void* appHandle,
                                       CsrUint8 subscriptionHandle,
                                       CsrUint16 frameLength,
                                       const CsrUint8 *frame,
                                       unifi_FrameFreeFunction freeFunction,
                                       unifi_ReceptionStatus receptionStatus,
                                       unifi_Priority priority,
                                       unifi_ServiceClass serviceClass)
{
#ifndef CSR_WIFI_PALDATA_DIRECT_ACCESS_ENABLE
    CsrUint8 *frameToSend = (CsrUint8 *)frame;

    sme_trace_entry((TR_PAL_SYS_SAP,"paldata_sys_ma_unitdata_ind: Send signal to PAL-D: frameLength-%d, frame-%p,freeFunction-%",frameLength,frame,freeFunction));
#endif

    if (frameLength)
    {
#ifndef CSR_WIFI_PALDATA_DIRECT_ACCESS_ENABLE
        /* Allocate a memory for data block only. Configure such a way that frame is not freed when data block is freed.
        * frame will need to be explicity freed using freeFunctin.
        * IMPORTANT: frame needs to be allocated as a seperate memory when calling this function. And a valid freeFunction pointer 
        * needs to be passed in.(except for unit/system testing).
        */
        PAL_DataBlock *dataBlock = PALDATA_MEM_UPSTREAM_ALLOCATE_DATA_BLOCK(frameLength, NULL, frame, TRUE, NULL);
        frameToSend = (CsrUint8*)dataBlock;
#endif
        if (!freeFunction) /* For testing only */
        {
            freeFunction = (unifi_FrameFreeFunction)&pal_free_data_buffer; /*lint !e546*/ /* Couldn't assign CsrPfree() as its a macro */
        }
    }

#ifndef CSR_WIFI_PALDATA_DIRECT_ACCESS_ENABLE
    send_paldata_sys_ma_unitdata_ind_external(context,
                                              getPalDataContext(context)->palDamFsmInstance,
                                              appHandle,
                                              subscriptionHandle,
                                              frameLength,
                                              frameToSend,
                                              freeFunction,
                                              receptionStatus,
                                              priority,
                                              serviceClass);
#else

    sme_trace_info((TR_PAL_SYS_SAP,"paldata_sys_ma_unitdata_ind: Call direct access function to handle the message"));
    paldata_process_unitdata_ind(context,subscriptionHandle,frameLength,frame,freeFunction,receptionStatus,priority,serviceClass);
#endif

}

