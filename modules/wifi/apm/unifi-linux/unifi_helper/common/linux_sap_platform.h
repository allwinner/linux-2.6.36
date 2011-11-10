/** @file linux_sap_platform.h
 *
 * Linux userspace sap configuration definition
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
 *   Startup SME wrapper
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/linux_sap_platform.h#2 $
 *
 ****************************************************************************/
#ifndef LINUX_SAP_PLATFORM_H
#define LINUX_SAP_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/csr_wifi_fsm.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/
typedef struct LinuxUserSpaceContext {

    FsmContext* fsmContext;
#ifdef CSR_AMP_ENABLE
    FsmContext* palDataFsmContext;
#endif
#ifdef CSR_WIFI_NME_ENABLE
    FsmContext* nmeFsmContext;
#endif
#ifdef BT_SAP_FD_SUPPORT
    int btsapfd;
#endif
    void* mainData;
} LinuxUserSpaceContext;

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

#ifdef BT_SAP_FD_SUPPORT
extern void bt_fd_triggered(LinuxUserSpaceContext* linuxContext);
#endif

#ifdef __cplusplus
}
#endif

#endif /*LINUX_SAP_PLATFORM_H*/
