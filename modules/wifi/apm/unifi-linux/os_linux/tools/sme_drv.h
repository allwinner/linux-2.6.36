/*
 * ---------------------------------------------------------------------------
 * FILE:     sme_drv.h
 * 
 * Copyright (C) 2006-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#define UNION union
#define STRUCT struct
#define PACKING

#include "driver/sigs.h"

#ifndef IFNAMSIZ
#define IFNAMSIZ        16
#endif


#define MAX_GUID_LENGTH     32
#define MAX_MIB_FILES        4
#define MAX_MIB_PATH_LEN   128

typedef struct MANAGER_PRIV_DATA {
    int Fd;
    int drv_connection;
    int kill_event_thread;
    int event_thread_running;
    char guid[MAX_GUID_LENGTH];
    unsigned char macaddress[6];
    unsigned char unifi_macaddress[6];
    unsigned int mib_files_num;
    char mib_files[MAX_MIB_FILES][MAX_MIB_PATH_LEN];
} MANAGER_PRIV_DATA;



#define DD_IND_ERROR            0x01
#define DD_IND_EXIT             0x02
#define DD_COMM_ERROR           0x04
#define DD_SME_NOT_PRESENT      0x10
#define DD_SME_PRESENT          0x20

#define ERROR_IO_ERROR          -1

#define INVALID_SIGNAL          0x0000

int drv_set_mac_address(MANAGER_PRIV_DATA *priv);
int drv_set_reset_state(MANAGER_PRIV_DATA *priv);
int drv_download_mib(MANAGER_PRIV_DATA *priv, char *mib_file);

int wait_for_event(MANAGER_PRIV_DATA *priv, int signal_id);


int MibEncodeOID(const char *oid_str, unsigned char *buf);
int MibEncodeOctetString(unsigned char *aStr, int aStrLength, unsigned char *aData);
int MibEncodeNull(unsigned char *aData);
int MibEncodeInteger(int aVal, unsigned char *aData);
int MibEncodeBoolean(int aVal, unsigned char *aData);

int MibDecodeInteger(unsigned char *aData, int *aVal);
int MibDecodeOctetString(unsigned char *aData, int aStrLength, unsigned char *aStr);
int MibDecodeBoolean(unsigned char *aData, CsrUint32 *aVal);
