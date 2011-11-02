/** @file linuxexample.h
 *
 * Linux Example Context definition 
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
 *   This file implements the sys sap when talking to the linux kernel
 * 
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/unifi_simple_helper/linuxexample.h#1 $
 * 
 ****************************************************************************/

#ifndef LINUXEXAMPLE_H_
#define LINUXEXAMPLE_H_

#include "sme_top_level_fsm/sme.h"

typedef struct LinuxExampleContext {
    
    /* Char device data to talk to the driver using the SYS Sap */
    char* charDevice;
    int sysSapFd;
    
    /* Configuration data */
    CsrBool flightmode;
    unifi_DataBlockList mibfiles;
    char* calibrationDataFile;
    unifi_DataBlock calibrationData;
    unifi_SSID ssid;
    unifi_MACAddress stationAddress;
    unifi_MACAddress bssid;
    CsrBool scan;
    CsrBool adhoc;
    
    unifi_AuthMode authMode;
    unifi_80211PrivacyMode privacy;
    
    CsrUint8 txkey;
    struct {
        CsrUint8 keysize;
        CsrUint8 key[13];
    } keys[4];
    
    /* Sme context */
    FsmContext* fsmContext;
    
} LinuxExampleContext;

#endif /*LINUXEXAMPLE_H_*/
