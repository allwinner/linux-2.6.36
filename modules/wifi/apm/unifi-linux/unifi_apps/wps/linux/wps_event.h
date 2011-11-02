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
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_event.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_EVENT_H
#define WPS_EVENT_H

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
    EV_TIMEOUT,
    EV_EAP_FAIL
}WpsEvent;

typedef enum
{
	WPS_STATE_INIT,
	WPS_STATE_WAIT_START,
	WPS_STATE_WAIT_M2,
	WPS_STATE_WAIT_M4,
	WPS_STATE_WAIT_M6,
	WPS_STATE_WAIT_M8
}WpsState;

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

WpsEvent getEvent( int fd, WpsContext *pCtx, int *pEapId );
int HandleEvent( int fd, int eapId, int event, WpsState *pState, WpsContext *pCtx, unsigned char *supp_mac );
#endif /* WPS_EVENT_H */
