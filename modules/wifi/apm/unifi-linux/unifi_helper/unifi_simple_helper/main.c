/** @file main.c
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
 *   Entry point and initialisation for the sme example application.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/unifi_simple_helper/main.c#1 $
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "linuxexample.h"

#include "sme_top_level_fsm/sme.h"
#include "sme_trace/sme_trace.h"
#include "event_pack_unpack/event_pack_unpack.h"
#include "sys_sap/sys_sap_remote_sme_interface.h"

/* ---------------------------------- */
/* So we can correctly size udi_msg_t */
/* ---------------------------------- */
#define uint8  CsrUint8
#define uint16 CsrUint16
#include "driver/unifiio.h"
/* -------------------------------------------- */

#define DEFAULT_CHAR_DEVICE "/dev/unifi0"
#define MAX_MIB_FILES 3
#define MAX_MESSAGE_SIZE 2048

CsrBool loadfile(const char* filename, unifi_DataBlock* datablock)
{
    struct stat filestats;
    FILE *fp;
    int readSize;

    if (stat(filename, &filestats) < 0)
    {
        printf("loadfile() : file -> %s Failed to stat : %s\n", filename, strerror(errno));
        return FALSE;
    }

    datablock->length = (CsrUint16)filestats.st_size;
    datablock->data = (CsrUint8*)malloc(datablock->length);

    if (datablock->data == NULL)
    {
        printf("loadfile() : file -> %s Failed to allocate %d bytes\n", filename, datablock->length);
        return FALSE;
    }

    fp = fopen(filename, "r");
    if (!fp)
    {
        printf("loadfile() : file -> %s fopen failed : %s\n", filename, strerror(errno));
        return FALSE;
    }

    readSize = fread(datablock->data, 1, (unsigned int)datablock->length, fp);
    if (readSize != datablock->length)
    {
        printf("loadfile() : file -> %s fread failed returned %d : %s\n", filename, readSize, strerror(errno));
        fclose(fp);
        return FALSE;
    }

    fclose(fp);

    return TRUE;
}

void loadMibfile(const char* filename, unifi_DataBlockList* mibfiles)
{
    if (mibfiles->numElements == MAX_MIB_FILES)
    {
        printf("loadMibfile() : mib file -> %s will not be loaded as %d files already loaded\n", filename, mibfiles->numElements);
        return;
    }

    if(loadfile(filename, &mibfiles->dataList[mibfiles->numElements]))
    {
    	mibfiles->numElements++;
    }
}

static void print_usage()
{
    printf("Usage :\n");
    printf("    -h                          : Print usage infomation\n");
    printf("    -dev=<filename>             : Which device to open. Default: %s\n", DEFAULT_CHAR_DEVICE);
    printf("    -cal=<filename>             : Calibration Data file.\n");
    printf("                                : The file is read at startup and write updates to it\n");
    printf("    -mib=<filename>             : Load a mib file on startup.\n");
    printf("                                : Upto %d files can be passed on commandline\n", MAX_MIB_FILES);
    printf("    -scan                       : perform a full scan and print the results before connecting\n");
    printf("    -ssid=<ssid>                : SSID  to pass to unifi_mgt_connect_req()\n");
    printf("    -bssid=<XX:XX:XX:XX:XX:XX>  : BSSID to pass to unifi_mgt_connect_req()\n");
    printf("    -adhoc                      : Use Adhoc mode\n");
    printf("    -authmode=<open or shared>  : 802.11 Authentication mode <Default = open>\n");
    printf("    -txkey=<0-3>                : Wep Transmit key <Default = 0>\n");
    printf("    -key1=<XX *5 or XX *13>     : Wep40 or Wep128 Key \n");
    printf("    -key2=<XX *5 or XX *13>     : Wep40 or Wep128 Key \n");
    printf("    -key3=<XX *5 or XX *13>     : Wep40 or Wep128 Key \n");
    printf("    -key4=<XX *5 or XX *13>     : Wep40 or Wep128 Key \n");
    printf("    -flightmode                 : Boot into flightmode and exit.\n");
    (void)fflush(stdout);
}

static void process_commandline(LinuxExampleContext* context, int argc, char **argv)
{
    int i;

    context->charDevice = DEFAULT_CHAR_DEVICE;

    if (argc == 1)
    {
        print_usage();
    }

    for (i = 1; i < argc; i++) {

        if (strncmp(argv[i], "-flightmode", 11) == 0)
        {
            context->flightmode = TRUE;
            printf("process_commandline() : flightmode\n");
            continue;
        }

        if (strncmp(argv[i], "-dev=", 5) == 0)
        {
            context->charDevice = &argv[i][5];
            printf("process_commandline() : device -> %s\n", context->charDevice);
            continue;
        }

        if (strncmp(argv[i], "-mib=", 5) == 0)
        {
            loadMibfile(&argv[i][5], &context->mibfiles);
            printf("process_commandline() : mib -> %s\n", &argv[i][5]);
            continue;
        }

        if (strncmp(argv[i], "-cal=", 5) == 0)
        {
            context->calibrationDataFile = &argv[i][5];
            loadfile(context->calibrationDataFile, &context->calibrationData);
            printf("process_commandline() : calibration data -> %s\n", context->calibrationDataFile);
            continue;
        }

        if (strncmp(argv[i], "-ssid=", 6) == 0)
        {
            char* ssid = &argv[i][6];
            context->ssid.length = strlen(ssid);
            strncpy((char*)&context->ssid.ssid, ssid, 32);
            printf("process_commandline() : ssid -> %s\n", ssid);
            continue;
        }

        if (strncmp(argv[i], "-bssid=", 7) == 0)
        {
            char* bssid = &argv[i][7];
            unsigned int macaddr_reader[6];
            int scanfResult;
            int j;

            scanfResult = sscanf(bssid, "%x:%x:%x:%x:%x:%x", &macaddr_reader[0], &macaddr_reader[1],
                                                             &macaddr_reader[2], &macaddr_reader[3],
                                                             &macaddr_reader[4], &macaddr_reader[5]);
            if (scanfResult != 6)
            {
                print_usage();
                printf("process_commandline() : %s invalid\n", argv[i]);
                exit(-1);
            }

            for (j = 0; j < 6; j++)
            {
                context->bssid.data[j] = (CsrUint8) macaddr_reader[j];
            }
            continue;
        }

        if (strncmp(argv[i], "-scan", 5) == 0)
        {
            context->scan = TRUE;
            printf("process_commandline() : scan\n");
            continue;
        }

        if (strncmp(argv[i], "-adhoc", 6) == 0)
        {
            context->adhoc = TRUE;
            printf("process_commandline() : adhoc\n");
            continue;
        }

        if (strncmp(argv[i], "-adhoc", 6) == 0)
        {
            context->adhoc = TRUE;
            printf("process_commandline() : adhoc\n");
            continue;
        }

        if (strncmp(argv[i], "-key", 4) == 0)
        {
            char* keynumstr = &argv[i][4];
            char* keystr = &argv[i][6];
            unsigned int keynumber;
            unsigned int readkey[13];
            CsrUint8 key[13];
            int scanfResult;

            scanfResult = sscanf(keynumstr, "%1d", (int*)&keynumber);
            if (scanfResult != 1 || keynumber < 1 || keynumber > 4)
            {
                print_usage();
                printf("process_commandline() : key -> %s invalid\n", argv[i]);
                exit(-1);
            }

            scanfResult = sscanf(keystr, "%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X",
                    &readkey[0], &readkey[1], &readkey[2], &readkey[3], &readkey[4],  &readkey[5],
                    &readkey[6], &readkey[7], &readkey[8], &readkey[9], &readkey[10], &readkey[11], &readkey[12]);
            key[0]  = (CsrUint8)readkey[0];
            key[1]  = (CsrUint8)readkey[1];
            key[2]  = (CsrUint8)readkey[2];
            key[3]  = (CsrUint8)readkey[3];
            key[4]  = (CsrUint8)readkey[4];
            key[5]  = (CsrUint8)readkey[5];
            key[6]  = (CsrUint8)readkey[6];
            key[7]  = (CsrUint8)readkey[7];
            key[8]  = (CsrUint8)readkey[8];
            key[9]  = (CsrUint8)readkey[9];
            key[10] = (CsrUint8)readkey[10];
            key[11] = (CsrUint8)readkey[11];
            key[12] = (CsrUint8)readkey[12];


            if (scanfResult != 5 && scanfResult != 13)
            {
                print_usage();
                printf("process_commandline() : %s invalid\n", argv[i]);
                exit(-1);
            }

            context->privacy = unifi_80211PrivacyEnabled;
            context->keys[keynumber-1].keysize = scanfResult;
            memcpy(context->keys[keynumber-1].key, key, 13);
            printf("process_commandline() : key -> %d :: %s\n", keynumber, keystr);

            continue;
        }

        if (strncmp(argv[i], "-txkey=", 7) == 0)
        {
            char* txkey = &argv[i][7];
            unsigned int reader;
            int scanfResult;

            scanfResult = sscanf(txkey, "%d", (int*)&reader);
            if (scanfResult != 1)
            {
                print_usage();
                printf("process_commandline() : %s invalid\n", argv[i]);
                exit(-1);
            }

            context->txkey = (CsrUint8)reader;
            printf("process_commandline() : txkey -> %d\n", context->txkey);
            continue;
        }

        if (strncmp(argv[i], "-authmode=", 10) == 0)
        {
            char* authmode = &argv[i][10];

            if (strcmp(authmode, "open") == 0)
            {
                context->authMode = unifi_80211AuthOpen;
                printf("process_commandline() : authmode -> open\n");
                continue;
            }

            if (strcmp(authmode, "shared") == 0)
            {
                context->authMode = unifi_80211AuthShared;
                printf("process_commandline() : authmode -> shared\n");
                continue;
            }
            print_usage();
            printf("process_commandline() : %s invalid\n", argv[i]);
            exit(-1);
            continue;
        }

        if (strncmp(argv[i], "-sme_trace:", 11) == 0)
        {
            continue;
        }

        if (strcmp(argv[i], "-h") == 0)
        {
            print_usage();
            exit(0);
        }

        printf("process_commandline() : Unknown option %s\n", argv[i]);
        print_usage();
        exit(0);
    }
}
static void wakeup_handler(int a)
{
/*    printf("wakeup_handler()\n"); */
    /* Install the signal handler to allow the select to be interrupted */
    signal(SIGALRM, wakeup_handler);
}

static void sme_wakeup_callback(void* context)
{
/*    printf("sme_wakeup_callback()\n"); */
    /* Interrupt the select as the sme wants to run */
    raise(SIGALRM);
}


static void sme_schedule(LinuxExampleContext* context)
{
    fd_set readfds;
    int r;
    struct timeval selectTimeout;
    CsrUint8 *buffer;

    buffer = (CsrUint8 *)malloc(MAX_MESSAGE_SIZE);
    if (buffer == NULL) {
        printf("sme_schedule(): Failed to malloc buffer: %s\n", strerror(errno));
        exit(errno);
    }


    for (;;)
    {
        FsmTimerData timeout = sme_execute(context->fsmContext);

        /* Initialise the fd_set for select on the char device */
        FD_ZERO(&readfds);
        FD_SET((unsigned int) context->sysSapFd, &readfds);

        selectTimeout.tv_sec  = (long) (timeout.timeoutTimeMs / (CsrUint32) COAL_MILLISECOND);
        selectTimeout.tv_usec = (timeout.timeoutTimeMs % COAL_MILLISECOND) * COAL_MILLISECOND /* for microsecs */;

        /* Sit on the char device until :
         * 1) The chardevice has data to read
         * 2) The timeout time elapses
         * 3) The sme_wakeup_callback is called by the sme to indicate it wants to run
         */
        r = select(context->sysSapFd + 1, &readfds, NULL, NULL, &selectTimeout);

        if ((r < 0) && (errno != EINTR))
        {
            printf("sme_schedule(): select() failed: %s\n", strerror(errno));
            exit(errno);
        }

        /* Check for data on the Sys Sap Char Device */
        if (FD_ISSET(context->sysSapFd, &readfds))
        {
            udi_msg_t *msg;

            r = read(context->sysSapFd, buffer, MAX_MESSAGE_SIZE);
            if (r <= 0)
            {
                printf("sme_schedule(): read() failed: %s\n", strerror(errno));
                exit(errno);
            }

            /* Pass the message to the SME
             * This function decodes the data an calls the correct SME function
             * NOTE : buffer contains a udi_msg_t header followed by the message
             */
            msg = (udi_msg_t *)buffer;
            if (!remote_sys_signal_receive(context->fsmContext,
                                           (CsrUint8*)(msg + 1),
                                           msg->signal_length))
            {
                printf("Unhandled ipc message\n");
            }
        }
    }
    free(buffer);
}


int main(int argc, char **argv)
{
    int i;

    /* Create and initialise the Application Context */
    LinuxExampleContext* context = (LinuxExampleContext*) malloc(sizeof(LinuxExampleContext));
    context->charDevice = NULL;
    context->sysSapFd = -1;
    context->flightmode = FALSE;
    context->mibfiles.numElements = 0;
    context->mibfiles.dataList = (unifi_DataBlock*)malloc(sizeof(unifi_DataBlock) * MAX_MIB_FILES);
    context->calibrationDataFile = NULL;
    context->calibrationData.length = 0;
    context->calibrationData.data = NULL;
    context->ssid.length = 0;
    memset(&context->stationAddress, 0xFF, sizeof(context->stationAddress));
    memset(&context->bssid, 0xFF, sizeof(context->bssid));
    context->scan = FALSE;
    context->adhoc = FALSE;
    context->authMode = unifi_80211AuthOpen;
    context->privacy = unifi_80211PrivacyDisabled;
    context->txkey = 0;
    for (i = 0; i < 4; ++i)
    {
        context->keys[i].keysize = 0;
    }
    context->fsmContext = NULL;

    /* Get the commandline options */
    process_commandline(context, argc, argv);

    /* Open the char device used for the Sys Sap */
    context->sysSapFd = open(context->charDevice, O_RDWR);
    if (context->sysSapFd < 0)
    {
        printf("main() : Failed to open %s : %s\n", context->charDevice, strerror(errno));
        exit(1);
    }

    /* Install the signal handler to allow the select to be interrupted */
    signal(SIGALRM, wakeup_handler);

    /* Set Trace levels from the command line */
    sme_trace_initialise(argc, argv);

    /* Initialise basic constructs used by the SME */
    context->fsmContext = sme_init(context, NULL);

    /* Install a callback to wakeup the SME whenever it needs to run */
    sme_install_wakeup_callback(context->fsmContext, sme_wakeup_callback);

    /* if we have some calibration data then write it to the SME before wifi on */
    if (context->calibrationData.length)
    {
        unifi_Status status;
        unifi_AppValue value;
        value.id = unifi_CalibrationDataValue;
        value.unifi_Value_union.calibrationData = context->calibrationData;
        printf("unifi_mgt_set_value()\n");
        status = unifi_mgt_set_value(context->fsmContext, &value);
        if (status != unifi_Success)
        {
            exit(status);
        }
    }

    if (context->flightmode)
    {
        printf("unifi_mgt_wifi_flightmode_req()\n");
        (void)fflush(stdout);
        unifi_mgt_wifi_flightmode_req(context->fsmContext, NULL, &context->stationAddress, context->mibfiles.numElements, context->mibfiles.dataList);
    }
    else
    {
        /* Kick off the SME by calling unifi_mgt_wifi_on_req()
         * See mgt_sap.c for the rest of the mgt processing */
        printf("unifi_mgt_wifi_on_req()\n");
        (void)fflush(stdout);
        unifi_mgt_wifi_on_req(context->fsmContext, NULL, &context->stationAddress, context->mibfiles.numElements, context->mibfiles.dataList);
    }

    /* Run the scheduling loop for the SME
     * THIS DOES NOT RETURN! */
    sme_schedule(context);

    return 0;
}
