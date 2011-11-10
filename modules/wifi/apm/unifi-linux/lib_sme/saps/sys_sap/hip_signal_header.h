/** @file hip_signal_header.h
 *
 * Type definition of the hip signal header
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
 *   All Hip signals have the same header structure and this definition is
 *   used to map that header on to buffers
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/saps/sys_sap/hip_signal_header.h#2 $
 *
 ****************************************************************************/

#ifndef HIP_SIGNAL_HEADER_H_
#define HIP_SIGNAL_HEADER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "abstractions/osa.h"
#include "fsm/csr_wifi_fsm.h"
#include "hostio/hip_fsm_types.h"

typedef struct hip_signal_header
{
    FsmEvent common;
    DataReference dataref1;
    DataReference dataref2;
} hip_signal_header;

#ifdef __cplusplus
}
#endif

#endif /* HIP_SIGNAL_HEADER_H_ */
