/** @file wps_event.h
 *
 * Definitions for Wi-Fi Protected Setup (WPS) event handling.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of a Wi-Fi Protected Setup (WPS) event handling.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_event.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_EVENT_H
#define WPS_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wps_common.h"

typedef enum
{
    EV_IGNORE,
    EV_EAP_IDENTITY_REQUEST,
    EV_WSC_START,
    EV_WSC_NACK,
    EV_M2,
    EV_M2D,
    EV_M4,
    EV_M6,
    EV_M8,
    EV_EAP_FAIL
}WpsEvent;

typedef enum
{
    WPS_RESULT_VOID,
    WPS_RESULT_SUCCESS,
    WPS_RESULT_TIMEOUT,
    WPS_RESULT_NACK,
    WPS_RESULT_M2_ERROR,
    WPS_RESULT_M2D,
    WPS_RESULT_M4_ERROR,
    WPS_RESULT_M6_ERROR,
    WPS_RESULT_M8_ERROR,
    WPS_RESULT_EAP_FAIL
}WpsEventResult;

WpsEvent getEvent( CsrInt32 fd, CsrWpsContext *pCtx, CsrInt32 *pEapId );
void HandleEvent( CsrUint8 eapId, CsrInt32 event, CsrWpsContext *pCtx);

#ifdef __cplusplus
}
#endif

#endif /* WPS_EVENT_H */
