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
 *   $Id: //depot/dot11/v7.0p/host/lib_paldata/saps/paldata_acl_sap/paldata_acl_signal_from_acl_sap.c#5 $
 *
 ****************************************************************************/

/** @{
 * @ingroup data_manager
 */

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_internal.h"
#include "paldata_top_level_fsm/paldata_top_level_fsm.h"
#include "data_manager/data_manager.h"


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


void pal_acl_data_req(FsmContext* context,
                      CsrUint16 dataLength,
                      const CsrUint8 *data,
                      CsrUint16 logicalLinkHandle,
                      CsrUint16 aclOffset,
                      unifi_FrameFreeFunction freeFunction)
{
#ifndef CSR_WIFI_PALDATA_DIRECT_ACCESS_ENABLE
    CsrUint8 *dataToSend;
#endif
    CsrUint8 *aclData;

    if (dataLength)
    {
#ifndef CSR_WIFI_PALDATA_DIRECT_ACCESS_ENABLE
        PAL_DataBlock *dataBlock;
#endif
        /* Allocate a memory for data block only. Configure such a way that data is not freed when data block is freed.
        * data will need to be explicity freed using freeFunctin.
        * dataLength is the length of the ACL packet only.
        * The offset is defined in /lib_paldata/paldata/data_manager/data_manager.h as PALDATA_ACL_OFFSET which is 16.
        * data is allocated as a seperate memory buffer and will be freed using freeFunction.
        * IMPORTANT: frame needs to be allocated as a seperate memory when calling this function. And a valid freeFunction pointer
        * needs to be passed in. (except for testing).
        */
        /* If offset is not allocated then create a new buffer with offset and copy data */
        if (!aclOffset)
        {
            sme_trace_entry((TR_PAL_ACL_SAP,"pal_acl_data_req: acl offset not allocated. so createing new buffer "));
            aclOffset = PALDATA_ACL_OFFSET;
            aclData = CsrPmalloc(dataLength+aclOffset);
            CsrMemCpy(aclData+aclOffset, data, dataLength);
            PALDATA_MEM_FREE_DATA_BUFFER(freeFunction,data);
        }
        else
        {
            aclData = (CsrUint8 *)data;
        }

        if (!freeFunction) /* For testing only */
        {
            freeFunction = pal_free_data_buffer; /*lint !e546*/ /* Couldn't assign CsrPfree() as its a macro */
        }

#ifndef CSR_WIFI_PALDATA_DIRECT_ACCESS_ENABLE
        dataBlock = PALDATA_MEM_ALLOCATE_DOWNSTREAM_DATA_BLOCK(dataLength+aclOffset, NULL, aclData, TRUE, NULL);
        dataToSend = (CsrUint8 *)dataBlock;
#endif

        sme_trace_entry((TR_PAL_ACL_SAP,"pal_acl_data_req: length-%d,freeFunction-%p",dataLength,freeFunction));
    }
    else
    {
        /* dataLength is zero        */
        sme_trace_warn((TR_PAL_ACL_SAP,"pal_acl_data_req: Zero length buffer.  discarding packet"));
        return;
    }

#ifndef CSR_WIFI_PALDATA_DIRECT_ACCESS_ENABLE
    send_pal_acl_data_req_external(context,
                                   getPalDataContext(context)->palDamFsmInstance,
                                   dataLength,
                                   dataToSend,
                                   logicalLinkHandle,
                                   aclOffset,
                                   freeFunction)
#else
    sme_trace_info((TR_PAL_SYS_SAP,"pal_acl_data_req: Call direct access function to handle the message"));
    (void)paldata_process_acl_data_req(context,dataLength,aclData,logicalLinkHandle,aclOffset,freeFunction);
#endif
}


/*
 * FSM Scripts Config for this FSM
 *
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/** @}
 */
