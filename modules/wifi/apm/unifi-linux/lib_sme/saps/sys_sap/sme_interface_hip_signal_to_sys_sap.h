/** @file sme_interface_hip_signal_to_sys_sap.h
 *
 * Macro definition for output signal creation of hip signals
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
 *   These macros are used to map the sdl output of a hip signal to a call to
 *   hip_signal_to_sys_sap()
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/saps/sys_sap/sme_interface_hip_signal_to_sys_sap.h#2 $
 *
 ****************************************************************************/

#ifndef __SME_INTERFACE_HIP_TO_SYS_SAP_H__
#define __SME_INTERFACE_HIP_TO_SYS_SAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fsm/csr_wifi_fsm.h"

/* ------------------------------------------------ */
/* Added the DataRefs to the end of the Signal and  */
/* sends the signal to the dd_sap interface         */
/* ------------------------------------------------ */
extern void hip_signal_to_sys_sap(FsmContext* context, FsmEvent *evt, CsrUint16 evtSize);

#ifdef __cplusplus
}
#endif

#endif /* __SME_INTERFACE_HIP_TO_SYS_SAP_H__ */
