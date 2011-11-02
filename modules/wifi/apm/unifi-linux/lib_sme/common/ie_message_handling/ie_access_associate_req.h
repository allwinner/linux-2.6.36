/** @file ie_access_associate_req.h
 *
 * Utilities to process WPS information elements
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
 *   Utilities to process WPS information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_message_handling/ie_access_associate_req.h#1 $
 *
 ****************************************************************************/
#ifndef SME_IE_ACCESS_ASSOCIATE_REQ_H
#define SME_IE_ACCESS_ASSOCIATE_REQ_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup ie_access
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"

#include "ie_access/ie_access.h"


/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief Creates the informational Element buffer for the Associate req
 *
 * @par Description
 * Creates the informational Element buffer for the Associate req
 *
 * @param[in] securityIeDataRef : Pointer to the IE buffer.
 * @param[in] joinData : pointer tot teh join data
 * @param[in] cfg : pointer to the configuration manager data
 *
 * @return
 *        void
 */
extern void ie_create_associated_req_ies(FsmContext* context,
                    DataReference securityIeDataRef,
                    SmeConfigData* cfg);

/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_IE_ACCESS_ASSOCIATE_REQ_H */
