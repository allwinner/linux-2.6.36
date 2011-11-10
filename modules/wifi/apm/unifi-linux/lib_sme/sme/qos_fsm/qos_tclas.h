/** @file qos_tclas.h
 *
 * Public SME QOS TCLAS API
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
 *   Public SME QOS TCLAS API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_tclas.h#1 $
 *
 ****************************************************************************/
#ifndef QOS_TCLAS_H
#define QOS_TCLAS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup qos
 */

/* STANDARD INCLUDES ********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

/* PROJECT INCLUDES *********************************************************/

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

extern unifi_TspecResultCode validate_tclas(
            FsmContext* context,
            CsrUint16 tclasLength,
            CsrUint8 *tclas);

/** \@}
 */

#ifdef __cplusplus
}
#endif


#endif /* QOS_TCLAS_H */
