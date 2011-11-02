/** @file wps_scan.h
 *
 * Definitions for Wi-Fi Protected Setup (WPS) scan functionality.
 * 
 * @section LEGAL
 *   CONFIDENTIAL
 * 
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 * 
 * @section DESCRIPTION
 *   This provides an implementation of a Wi-Fi Protected Setup (WPS) scan functionality.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_scan.h#1 $
 *
 ****************************************************************************/
#ifndef WPS_SCAN_H
#define WPS_SCAN_H

#include "wps_common.h"

int doScan( WpsContext *pCtx, Network *pCandidate, ScanMode *pScanMode );

#endif /* WPS_SCAN_H */
