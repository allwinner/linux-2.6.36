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
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/main.c#4 $
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/select.h>

#include "fsm/fsm_debug.h"
#include "fsm/fsm_internal.h"

#include "sme_top_level_fsm/sme.h"
#include "hostio/hip_fsm_events.h"
#include "smeio/smeio_fsm_events.h"
#include "smeio/smeio_trace_types.h"
#include "smeio/smeio_fsm_trace_events.h"

#ifdef CSR_AMP_ENABLE
#include "paldata_top_level_fsm/paldata.h"

#include "paldata_acl_sap/paldata_acl_sap_remote_sme_interface.h"
#include "paldata_sys_sap/paldata_sys_sap_remote_sme_interface.h"
#include "pal_hci_sap/pal_hci_sap_unpack.h"

#include "palio/palio_fsm_trace_events.h"

#ifdef IPC_CHARDEVICE
#include <sys/ioctl.h>
#include "ipc/linux/unifiio.h"
#endif

#endif /* CSR_AMP_ENABLE */

#include "sme_trace/sme_trace.h"
#include "ipc/ipc.h"
#ifdef IPC_IP
#include "ipc/linux/ipc_ip.h"
#endif
#ifdef IPC_CHARDEVICE
#include "ipc/linux/ipc_chardevice.h"
#endif

#include "linux_sap_platform.h"
#include "event_pack_unpack/event_pack_unpack.h"


#include "sys_sap/sys_sap_remote_sme_interface.h"
#include "mgt_sap/mgt_sap_remote_sme_interface.h"
#include "dbg_sap/dbg_sap_remote_sme_interface.h"
#include "bt_sap/bt_sap_remote_sme_interface.h"

#include "version/hip_version.h"
#include "version/version.h"

#ifdef CSR_WIFI_NME_ENABLE
#include "nme_top_level_fsm/nme.h"
#include "csr_wifi_nme_sap/csr_wifi_nme_sap_remote_sme_interface.h"
#include "csr_wifi_nme_sys_sap/csr_wifi_nme_sys_sap_remote_sme_interface.h"
#include "nmeio/nmeio_fsm_trace_events.h"
#endif

#define PORT_NUMBER_Q       10101
#define HCI_PORT_NUMBER_Q       10103
#define ACL_PORT_NUMBER_Q       10104
#define CHAR_DEVICE_Q       "/dev/unifi0"
#ifdef CSR_AMP_ENABLE
#define PALDATA_CHAR_DEVICE_Q       "/dev/unifiudi0"
#endif
#define MAX_MIB_FILES 3
/* Save the cal data every 30 minutes and 1 minute after startup */
#define CALDATA_SAVE_INTERVAL_MS (60000 * 30)
#define CALDATA_INITIAL_SAVE_INTERVAL_MS (60000)

#define EXIT_CODE_ERROR 		(-1)
#define EXIT_CODE_NORMAL_EXIT   (0)
#define EXIT_CODE_WIFION_ERROR  (1)

typedef struct MainData {
#ifdef IPC_IP
    ipcConnection*  ipIpcCon;
#ifdef CSR_AMP_ENABLE
    ipcConnection*  ipHciIpcCon;
    /* Use seperate IPC for data SAPs */
    ipcConnection*  ipAclIpcCon;
#endif
#endif
#ifdef IPC_CHARDEVICE
    ipcConnection*  charIpcCon;
#endif

    CsrBool exitOnError;
    CsrBool stopOnError;

    unifi_MACAddress address;
    unifi_DataBlockList mibfiles;
    char* calibrationDataFile;
    unifi_DataBlock calibrationData;
    CsrUint32 nextCaldataSaveTime;

    CsrBool externalEvent;

#ifdef SME_TRACE_ENABLE
    char traceMacAddressBuffer[19];
#endif
} MainData;

/* Only so the signal handler can see it */
static LinuxUserSpaceContext* linuxContext = NULL;
#define getMainData(context) ((MainData*)(context)->mainData)

LinuxUserSpaceContext* get_linux_context()
{
    return linuxContext;
}

#ifdef REMOTE_CSR_WIFI_NME_SAP
ipcConnection* get_csr_wifi_nme_ipc_connection(void* context)
{
    /* only used on the host, so no char device option */
    return getMainData(linuxContext)->ipIpcCon;
}
#endif

#ifdef REMOTE_MGT_SAP
ipcConnection* get_mgt_ipc_connection(void* context)
{
#ifdef MGT_SAP_IPC_CHARDEVICE
    return getMainData(linuxContext)->charIpcCon;
#else
    return getMainData(linuxContext)->ipIpcCon;
#endif
}
#endif

#ifdef REMOTE_SYS_SAP
ipcConnection* get_sys_ipc_connection(void* context)
{
#ifdef SYS_SAP_IPC_CHARDEVICE
    return getMainData(linuxContext)->charIpcCon;
#else
    return getMainData(linuxContext)->ipIpcCon;
#endif
}
#endif

#ifdef REMOTE_BT_SAP
ipcConnection* get_bt_ipc_connection(void* context)
{
#ifdef BT_SAP_IPC_CHARDEVICE
    return getMainData(linuxContext)->charIpcCon;
#else
    return getMainData(linuxContext)->ipIpcCon;
#endif
}
#endif

#ifdef REMOTE_DBG_SAP
ipcConnection* get_dbg_ipc_connection(void* context)
{
#ifdef DBG_SAP_IPC_CHARDEVICE
    return getMainData(linuxContext)->charIpcCon;
#else
    return getMainData(linuxContext)->ipIpcCon;
#endif
}
#endif

#ifdef REMOTE_SYS_SAP
ipcConnection* get_hip_ipc_connection(void* context)  { return get_sys_ipc_connection(context); }
#endif

#ifdef CUSTOM_UNIFI_NME_SYS_EAPOL_REQ
extern void csr_wpa_loopback_deinit();
#endif

#ifdef CSR_AMP_ENABLE
#ifdef REMOTE_PAL_HCI_SAP

ipcConnection* get_pal_hci_ipc_connection(FsmContext* context)
{
    return getMainData(linuxContext)->ipHciIpcCon;
}
#endif

#ifdef REMOTE_PALDATA_SYS_SAP
ipcConnection* get_paldata_sys_ipc_connection(void* context)
{
#ifdef IPC_CHARDEVICE
    return getMainData(linuxContext)->charIpcCon;
#else
    return getMainData(linuxContext)->ipIpcCon;
#endif
}
#endif


#ifdef REMOTE_PAL_CTRL_SAP
ipcConnection* get_pal_ctrl_ipc_connection(void* context)
{
    return getMainData(linuxContext)->ipIpcCon;
}
#endif

#ifdef REMOTE_PALDATA_ACL_SAP
ipcConnection* get_paldata_acl_ipc_connection(void* context)
{
    return getMainData(linuxContext)->ipAclIpcCon;
}
#endif
#endif

#ifdef REMOTE_CSR_WIFI_NME_SYS_SAP
ipcConnection* get_csr_wifi_nme_sys_ipc_connection(void* context)
{
#ifdef IPC_CHARDEVICE
    return getMainData(linuxContext)->charIpcCon;
#else
    return getMainData(linuxContext)->ipIpcCon;
#endif
}
#endif

#ifdef OSA_MEMORY_PROFILE
extern void stack_profile_report();
#endif

#ifdef CUSTOM_UNIFI_MGT_WIFI_OFF_IND
/* Custom exit code for WEXT builds */
void unifi_mgt_wifi_off_ind(void* context, CsrUint16 appHandlesCount, void* *appHandles, unifi_ControlIndication controlIndication)
{
    switch(controlIndication)
    {
    case unifi_ControlError:
        sme_trace_entry((TR_FSM, "unifi_mgt_wifi_off_ind(unifi_ControlError) Calling unifi_mgt_wifi_on_req"));
        if (getMainData(linuxContext)->exitOnError)
        {
            sme_trace_error((TR_FSM, "unifi_mgt_wifi_off_ind(unifi_ControlError) exiting on error"));
            exit(EXIT_CODE_ERROR);
        }
        else if (getMainData(linuxContext)->stopOnError)
        {
            sme_trace_error((TR_FSM, "unifi_mgt_wifi_off_ind(unifi_ControlError) stopping on error"));
        }
        else
        {
            unifi_mgt_wifi_on_req(linuxContext->fsmContext, NULL,
                                  &getMainData(linuxContext)->address,
                                  getMainData(linuxContext)->mibfiles.numElements,
                                  getMainData(linuxContext)->mibfiles.dataList);
        }
        break;
    case unifi_ControlExit:
        sme_trace_entry((TR_FSM, "unifi_mgt_wifi_off_ind(unifi_ControlExit) Calling exit"));
        exit(EXIT_CODE_NORMAL_EXIT);
        break;
    default:
        break;
    }
}
#endif /* CUSTOM_UNIFI_MGT_WIFI_OFF_IND */

#ifdef CUSTOM_UNIFI_MGT_WIFI_ON_CFM
/* Custom exit code for WEXT builds */
void unifi_mgt_wifi_on_cfm(void* context, void* appHandle, unifi_Status status)
{
    if (status != unifi_Success)
    {
        sme_trace_entry((TR_FSM, "unifi_mgt_wifi_on_cfm(%s) Calling exit", trace_unifi_Status(status)));
        exit(EXIT_CODE_WIFION_ERROR);
    }
}
#endif /* CUSTOM_UNIFI_MGT_WIFI_OFF_CFM */

#ifdef CUSTOM_UNIFI_MGT_WIFI_OFF_CFM
void unifi_mgt_wifi_off_cfm(void* context, void* appHandle, unifi_Status status)
{
    sme_trace_entry((TR_FSM, "unifi_mgt_wifi_off_cfm(%s) Calling exit", trace_unifi_Status(status)));
    exit(EXIT_CODE_NORMAL_EXIT);
}
#endif /* CUSTOM_UNIFI_MGT_WIFI_OFF_CFM */

#ifdef CUSTOM_UNIFI_MGT_WIFI_FLIGHTMODE_CFM
void unifi_mgt_wifi_flightmode_cfm(void* context, void* appHandle, unifi_Status status)
{
    sme_trace_entry((TR_FSM, "unifi_mgt_wifi_flightmode_cfm(%s) Calling exit", trace_unifi_Status(status)));
    exit(EXIT_CODE_NORMAL_EXIT);
}
#endif /* CUSTOM_UNIFI_MGT_WIFI_FLIGHTMODE_CFM */

static void save_calibration_data(LinuxUserSpaceContext* context)
{
    unifi_AppValue value;
    unifi_DataBlock* datablock = &value.unifi_Value_union.calibrationData;

    if(getMainData(context)->calibrationDataFile == NULL)
    {
        return;
    }

    sme_trace_entry((TR_FSM, "save_calibration_data(%s)", getMainData(context)->calibrationDataFile));

    unifi_mgt_claim_sync_access(context->fsmContext);
    value.id = unifi_CalibrationDataValue;
    if (unifi_mgt_get_value(context->fsmContext, &value) != unifi_Success)
    {
        sme_trace_error((TR_FSM, "save_calibration_data() get Calibration Data Failed"));
        unifi_mgt_release_sync_access(context->fsmContext);
        return;
    }
    unifi_mgt_release_sync_access(context->fsmContext);

    if (datablock->length)
    {
    	size_t writeSize;
        FILE *fp = fopen(getMainData(context)->calibrationDataFile, "w");
        if (!fp)
        {
            sme_trace_error((TR_FSM, "save_calibration_data() : file -> %s fopen failed : %s", getMainData(context)->calibrationDataFile, strerror(errno)));
            return;
        }

        writeSize = fwrite(datablock->data, 1, (unsigned int)datablock->length, fp);
        if (writeSize != datablock->length)
        {
            sme_trace_error((TR_FSM, "save_calibration_data() : file -> %s fwrite failed returned %d : %s", getMainData(context)->calibrationDataFile, writeSize, strerror(errno)));
            fclose(fp);
            return;
        }

        fclose(fp);
    }
}

/**
 * @brief
 *   This function is executed at normal process termination.
 *
 * @par Description:
 *   This function is called at normal process termination.
 *
 * @return
 *   void
 */
static CsrBool signalCommon(void)
{
    int i;

    /* --------------------------------------- */
    /* Make sure signalCommon() only runs once */
    /* --------------------------------------- */
    static CsrBool signalCommonCalled = FALSE;
    if (signalCommonCalled == TRUE)
    {
        return FALSE;
    }
    signalCommonCalled = TRUE;
    /* --------------------------------------- */

#ifdef OSA_MEMORY_PROFILE
    stack_profile_report();
#endif

    /* Disable trace as it uses the OSA */
    sme_trace_set_all_levels(TR_LVL_OFF);

#ifdef IPC_IP
    ipc_disconnect(getMainData(linuxContext)->ipIpcCon);
    getMainData(linuxContext)->ipIpcCon = NULL;

#ifdef CSR_AMP_ENABLE
    ipc_disconnect(getMainData(linuxContext)->ipHciIpcCon);
    getMainData(linuxContext)->ipHciIpcCon = NULL;
    ipc_disconnect(getMainData(linuxContext)->ipAclIpcCon);
    getMainData(linuxContext)->ipAclIpcCon = NULL;
#endif
#endif

#ifdef IPC_CHARDEVICE
    ipc_disconnect(getMainData(linuxContext)->charIpcCon);
    getMainData(linuxContext)->charIpcCon = NULL;
#endif

    if (linuxContext->fsmContext)
    {
#ifdef CSR_AMP_ENABLE
        paldata_shutdown(linuxContext->palDataFsmContext);
#endif
#ifdef CSR_WIFI_NME_ENABLE
        csr_wifi_nme_shutdown(linuxContext->nmeFsmContext);
#endif
        sme_shutdown(linuxContext->fsmContext);
    }

    for (i = 0; i < getMainData(linuxContext)->mibfiles.numElements; ++i)
    {
        CsrPfree(getMainData(linuxContext)->mibfiles.dataList[i].data);
    }
    CsrPfree(getMainData(linuxContext)->mibfiles.dataList);
    if (getMainData(linuxContext)->calibrationData.length)
    {
        CsrPfree(getMainData(linuxContext)->calibrationData.data);
    }
    return TRUE;
}

/**
 * @brief
 *   This function handles asynchronous signals to indicate to the main
 *   loop that a shutdown has been requested.
 *
 * @par Description:
 *   This function is a handler for caught signals that indicate the process
 *   should be shut down.  Pre-shutdown preparation is done and then exit() is
 *   called to terminate.
 *
 * @param[in]     signum: The number of the signal to handle
 *
 * @return
 *   void
 *
 * @todo Exit should not be called from an asynchronous signal
 * handler, rather it should set a flag that calls exit from the main
 * thread
 */
static void handleSignal(int signum)
{
    (void)signalCommon();
    exit(EXIT_CODE_NORMAL_EXIT);
}

/**
 * @brief
 *   This function handles asynchronous signals to indicate to the main
 *   loop that a shutdown has been requested.
 *
 * @par Description:
 *   This function is a handler for caught signals that indicate the process
 *   should be shut down.  Pre-shutdown preparation is done and then exit() is
 *   called to terminate.
 *
 * @param[in]     signum: The number of the signal to handle
 *
 * @return
 *   void
 *
 * @todo Exit should not be called from an asynchronous signal
 * handler, rather it should set a flag that calls exit from the main
 * thread
 */
static void handleExit(void)
{
    if (!linuxContext) return;

#ifdef CUSTOM_UNIFI_NME_SYS_EAPOL_REQ
    csr_wpa_loopback_deinit();
#endif

    (void) signalCommon();

    CsrPfree(getMainData(linuxContext));
    CsrPfree(linuxContext);
}

/**
 * @brief
 *   This function installs signal handlers
 *
 * @par Description:
 *   This function installs signal handlers to enable graceful termination
 *   of the SME process.
 *
 * @return
 *   void
 */
static void registerSignalHandlers()
{
    sme_trace_entry((TR_FSM, "registerSignalHandlers()"));
    (void) signal(SIGHUP, handleSignal);   /* HANGUP - terminal dissapears  */
    (void) signal(SIGQUIT, handleSignal);  /* CTRL-\                        */
    (void) signal(SIGINT, handleSignal);   /* CTRL-C                        */
    (void) signal(SIGABRT, handleSignal);  /* incase someone calls abort()  */
    (void) signal(SIGTERM, handleSignal);  /* test sys terminates with this */
    (void) signal(SIGPIPE, handleSignal);
    (void) atexit(handleExit);             /* normal program termination    */
}

static void fsm_wakeup_callback(void* extContext)
{
    LinuxUserSpaceContext* myLinuxContext  = (LinuxUserSpaceContext*)extContext;
    /*sme_trace_entry((TR_FSM, "fsm_wakeup_callback()"));*/
    /* Only need a flag as this is a single threaded app */
    getMainData(myLinuxContext)->externalEvent = TRUE;
}
#ifdef FSM_DEBUG
#ifndef SME_TRACE_NO_MSC
void trace_fsm_event(const char* text, const FsmContext* context, const FsmEvent *event)
{
    if (trace_smeio_fsm_event(text, context, event))
    {
    }
#ifdef CSR_WIFI_NME_ENABLE
    else if (trace_nmeio_fsm_event(text, context, event))
    {
    }
#endif
#ifdef CSR_AMP_ENABLE
    else if (trace_palio_fsm_event(text, context, event))
    {
    }
#endif
}

#define TRACE_TRANSITION_FUNCTION_DEFINE(tag,fnname,fncontext) \
static void fnname(void* extContext, const FsmEventEntry* eventEntryArray, const FsmEvent* event) \
{ \
    trace_fsm_event(tag, ((LinuxUserSpaceContext*)extContext)->fncontext, event); \
}

#define TRACE_EVENT_FUNCTION_DEFINE(tag,fnname,fncontext) \
static void fnname(void* extContext, const FsmEvent* event) \
{ \
    trace_fsm_event(tag, ((LinuxUserSpaceContext*)extContext)->fncontext, event); \
}

TRACE_TRANSITION_FUNCTION_DEFINE("",  fsm_on_transition_trace_callback, fsmContext)
TRACE_TRANSITION_FUNCTION_DEFINE("U", fsm_on_unhandled_trace_callback,  fsmContext)
TRACE_EVENT_FUNCTION_DEFINE("S", fsm_on_saved_trace_callback,   fsmContext)
TRACE_EVENT_FUNCTION_DEFINE("I", fsm_on_invalid_trace_callback, fsmContext)
TRACE_EVENT_FUNCTION_DEFINE("IG",fsm_on_ignored_trace_callback, fsmContext)

#ifdef CSR_WIFI_NME_ENABLE
TRACE_TRANSITION_FUNCTION_DEFINE("",  nme_on_transition_trace_callback, nmeFsmContext)
TRACE_TRANSITION_FUNCTION_DEFINE("U", nme_on_unhandled_trace_callback,  nmeFsmContext)
TRACE_EVENT_FUNCTION_DEFINE("S", nme_on_saved_trace_callback,   nmeFsmContext)
TRACE_EVENT_FUNCTION_DEFINE("I", nme_on_invalid_trace_callback, nmeFsmContext)
TRACE_EVENT_FUNCTION_DEFINE("IG",nme_on_ignored_trace_callback, nmeFsmContext)
#endif

#endif
#endif

#if defined IPC_IP || defined IPC_CHARDEVICE
/**
 * @brief
 *   Handles UNIFI DBG CMD REQ
 *
 * @par Description:
 *   This function is a handler for "exit" or "reset" UNIFI DBG CMD REQ
 *   commands.
 *
 * @param[in]  FsmContext*: FSM context
 * @param[in]  CsrUint8*: buffer to check for the DBG CMD REQ
 * @param[in]  ipcConnection*: IPC Connection info
 *
 * @return
 *   CsrBool - TRUE if the signal as handled, otherwise FALSE
 */
static CsrBool custom_handler_dbg_exit_req(FsmContext* fsmContext, CsrUint8* buffer, ipcConnection* connection)
{
    CsrUint8* tempbuffer = buffer;
    CsrUint16 id = event_unpack_CsrUint16(&tempbuffer);
    CsrBool result = FALSE;

    if (id == UNIFI_DBG_CMD_REQ_ID)
    {
        char* command;
        tempbuffer = &buffer[6];

        command = event_unpack_string(&tempbuffer);

        if(CsrStrNCmp(command, "exit", 4) == 0)
        {
            result = TRUE;
            CsrPfree(command);
            /* As about to exit need to free the messages as well as the command str */
            ipc_message_free(connection, buffer);
            fsm_debug_dump(fsmContext);
#ifdef CSR_WIFI_NME_ENABLE
            fsm_debug_dump(linuxContext->nmeFsmContext);
#endif
            exit(EXIT_CODE_NORMAL_EXIT);
        }
        else if(CsrStrNCmp(command, "reset", 5) == 0)
        {
            /* Need to reset in the same order as start up */
            linuxContext->fsmContext = sme_reset(linuxContext->fsmContext);
            sme_install_wakeup_callback(linuxContext->fsmContext, fsm_wakeup_callback);
#ifdef CSR_AMP_ENABLE
            linuxContext->palDataFsmContext = paldata_reset(linuxContext->palDataFsmContext);
            sme_install_wakeup_callback(linuxContext->palDataFsmContext, fsm_wakeup_callback);
#endif
#ifdef CSR_WIFI_NME_ENABLE
            linuxContext->nmeFsmContext = csr_wifi_nme_reset(linuxContext->nmeFsmContext);
            csr_wifi_nme_install_wakeup_callback(linuxContext->nmeFsmContext, fsm_wakeup_callback);
#endif
            result = TRUE;
        }
        /* Only free the command string as the message buffer is freed by the calling function. */
        CsrPfree(command);
    }
    return result;
}
#endif

static void loadMacAddress(const char* mac_file, unifi_MACAddress* address)
{
    unsigned int macaddr_reader[6];
    int readbytes;
    FILE *fp = fopen(mac_file, "r");

    if (!fp)
    {
        sme_trace_error((TR_IPC, "loadMacAddress() : mac file -> %s fopen failed MAC address in MIB will be used", mac_file));
        return;
    }

    /* The MAC address may either be provided by a MAC file or
     * ROM assigned. Thus if there is a MAC file there must be a
     * MAC address, if not, we assume at this point the MAC address
     * is in the ROM. Checks in MIB access will ensure ROM assigned
     * MAC addresses are not default.
     */
    readbytes = fscanf(fp, " %x:%x:%x:%x:%x:%x", &macaddr_reader[0], &macaddr_reader[1],
                                                 &macaddr_reader[2], &macaddr_reader[3],
                                                 &macaddr_reader[4], &macaddr_reader[5]);
    if (readbytes != 6)
    {
        sme_trace_error((TR_FSM, "Only read %d/6 bytes from MAC address file '%s'", readbytes, mac_file));
    }
    else
    {
        int i;
        for (i = 0; i < 6; i++)
        {
            address->data[i] = (CsrUint8) macaddr_reader[i];
        }
    }
    fclose(fp);
}

static CsrBool loadfile(const char* filename, unifi_DataBlock* datablock)
{
    struct stat filestats;
    FILE *fp;
    size_t readSize;

    sme_trace_entry((TR_FSM, "loadfile(%s)", filename));

    if (stat(filename, &filestats) < 0)
    {
        sme_trace_error((TR_FSM, "loadfile() : file -> %s Failed to stat : %s", filename, strerror(errno)));
        return FALSE;
    }

    datablock->length = (CsrUint16)filestats.st_size;
    datablock->data = (CsrUint8*)CsrPmalloc(datablock->length);

    fp = fopen(filename, "r");
    if (!fp)
    {
        sme_trace_error((TR_FSM, "loadfile() : file -> %s fopen failed : %s", filename, strerror(errno)));
        return FALSE;
    }

    readSize = fread(datablock->data, 1, (unsigned int)datablock->length, fp);
    if (readSize != datablock->length)
    {
        sme_trace_error((TR_FSM, "loadfile() : file -> %s fread failed returned %d : %s", filename, readSize, strerror(errno)));
        fclose(fp);
        return FALSE;
    }

    fclose(fp);

    return TRUE;
}

static void loadMibfile(const char* filename, unifi_DataBlockList* mibfiles)
{
    sme_trace_entry((TR_FSM, "loadMibfile()"));
    if (mibfiles->numElements == MAX_MIB_FILES)
    {
        sme_trace_error((TR_FSM, "loadMibfile() : mib file -> %s will not be loaded as %d files already loaded", filename, mibfiles->numElements));
        return;
    }

    if (loadfile(filename, &mibfiles->dataList[mibfiles->numElements]))
    {
        mibfiles->numElements++;
    }
}

static void print_versions()
{
    printf("Sme version           : %s\n", getSmeVersion());
    printf("Sme Interface version : 0x%.2X%.2X\n", UNIFI_SAP_API_VERSION_MAJOR, UNIFI_SAP_API_VERSION_MINOR);
    printf("Hip Interface version : 0x%.4X\n", SME_SUPPORTED_HIP_VERSION);
    (void)fflush(stdout);
}

static void print_usage()
{
    print_versions();
    printf("Usage :\n");
    printf("    -h                          : Print usage infomation\n");
    printf("    -v                          : Print version infomation\n");
#ifdef IPC_IP
    printf("    -ipc_port=<filename>        : Socket port to listen on. Default: %d\n", PORT_NUMBER_Q);
#endif
#ifdef IPC_CHARDEVICE
    printf("    -dev=<filename>     : Char device to open. Default: %s\n", CHAR_DEVICE_Q);
#ifdef CSR_AMP_ENABLE
    printf("    -paldatadev=<filename>      : Char device to open. Default: %s\n", PALDATA_CHAR_DEVICE_Q);
    printf("    -paldisablesecurity >       : disable security handshake if already enabled\n");
#endif
#endif

#ifdef CSR_AMP_ENABLE
    printf("    -palselectchannel=<number>  : channel to select\n");
    printf("    -paldisableqos >            : disable qos support in PAL\n");
#ifdef IPC_IP
        printf("    -ipc_hciport=<portnumber>        : Socket port for AMP HCI to listen on. Default: %d\n", HCI_PORT_NUMBER_Q);
        printf("    -ipc_aclport=<portnumber>        : Socket port for AMP ACL to listen on. Default: %d\n", ACL_PORT_NUMBER_Q);
#endif
#endif
    printf("    -cal=<filename>             : Calibration Data file.\n");
    printf("                                : The file is read at startup and write updates to it\n");
    printf("    -mib=<filename>             : Load a mib file on startup.\n");
    printf("                                : Upto %d files can be passed on commandline\n", MAX_MIB_FILES);
    printf("    -mac=<filename>             : Load a file containing a MAC address with the format XX:XX:XX:XX:XX:XX.\n");
    printf("    -wifion                     : Automatically start the SME.\n");
    printf("    -flightmode                 : Boot into flightmode and exit.\n");
    printf("    -exitOnError                : Development option. Exit rather than restart on error\n");
    printf("    -stopOnError                : Development option. Do not restart on error\n");
    (void)fflush(stdout);
}

static void sme_schedule_init(LinuxUserSpaceContext* context, int argc, char **argv)
{
    CsrBool wifion = FALSE;
    CsrBool flightmode = FALSE;
    int i;
#ifdef IPC_IP
    CsrUint32 ipc_portNumber = PORT_NUMBER_Q;
#ifdef CSR_AMP_ENABLE
    CsrUint32 ipc_hciPortNumber = HCI_PORT_NUMBER_Q;
    CsrUint32 ipc_aclPortNumber = ACL_PORT_NUMBER_Q;
#endif
#endif

#ifdef IPC_CHARDEVICE
    const char* connectStr = CHAR_DEVICE_Q;
#endif

    sme_trace_entry((TR_FSM, "sme_schedule_init()"));


    CsrMemSet(&getMainData(linuxContext)->address, 0xFF, sizeof(unifi_MACAddress));
    getMainData(linuxContext)->mibfiles.numElements = 0;
    getMainData(linuxContext)->mibfiles.dataList = (unifi_DataBlock*)CsrPmalloc(sizeof(unifi_DataBlock) * MAX_MIB_FILES);
    getMainData(linuxContext)->calibrationDataFile = NULL;
    getMainData(linuxContext)->calibrationData.length = 0;
    getMainData(linuxContext)->calibrationData.data = NULL;
    getMainData(linuxContext)->nextCaldataSaveTime = fsm_get_time_of_day_ms(context->fsmContext) + CALDATA_SAVE_INTERVAL_MS;
    getMainData(linuxContext)->exitOnError = FALSE;
    getMainData(linuxContext)->stopOnError = FALSE;

#ifdef CSR_AMP_ENABLE
    context->palDataFsmContext = paldata_init(linuxContext);
    sme_install_wakeup_callback(context->palDataFsmContext, fsm_wakeup_callback);
#endif

    /* Initialise basic constructs used by the SME */
    context->fsmContext = sme_init(context, NULL);
    sme_install_wakeup_callback(context->fsmContext, fsm_wakeup_callback);

#ifdef CSR_WIFI_NME_ENABLE
    context->nmeFsmContext = csr_wifi_nme_init(linuxContext, NULL);
    csr_wifi_nme_install_wakeup_callback(context->nmeFsmContext, fsm_wakeup_callback);
#endif

#ifdef FSM_DEBUG
    fsm_install_on_transition_callback(context->fsmContext, fsm_on_transition_trace_callback);
    fsm_install_unhandled_event_callback(context->fsmContext, fsm_on_unhandled_trace_callback);
    fsm_install_save_event_callback(context->fsmContext, fsm_on_saved_trace_callback);
    fsm_install_invalid_event_callback(context->fsmContext, fsm_on_invalid_trace_callback);
    fsm_install_ignore_event_callback(context->fsmContext, fsm_on_ignored_trace_callback);

#ifdef CSR_WIFI_NME_ENABLE
    fsm_install_on_transition_callback(context->nmeFsmContext, nme_on_transition_trace_callback);
    fsm_install_unhandled_event_callback(context->nmeFsmContext, nme_on_unhandled_trace_callback);
    fsm_install_save_event_callback(context->nmeFsmContext, nme_on_saved_trace_callback);
    fsm_install_invalid_event_callback(context->nmeFsmContext, nme_on_invalid_trace_callback);
    fsm_install_ignore_event_callback(context->nmeFsmContext, nme_on_ignored_trace_callback);
#endif

#endif

    registerSignalHandlers();

    /* If no args print the usage incase the user does not know what the help option is */
    if (argc == 1)
    {
        print_usage();
    }

    for (i = 1; i < argc; i++) {

        if (CsrStrNCmp(argv[i], "-ipc_port:", 10) == 0)
        {
#ifdef IPC_IP
            ipc_portNumber = (CsrUint32)atoi(&argv[i][10]);
            sme_trace_info((TR_IPC, "sme_schedule_init() : IPC Port override -> %s :: %d", argv[i], ipc_portNumber));
#endif
            continue;
        }

#ifdef CSR_AMP_ENABLE
        if (CsrStrNCmp(argv[i], "-ipc_hciport:", 13) == 0)
        {
#ifdef IPC_IP
            ipc_hciPortNumber = (CsrUint32)atoi(&argv[i][13]);
            sme_trace_info((TR_IPC, "sme_schedule_init() : HCI IPC Port override -> %s :: %d", argv[i], ipc_hciPortNumber));
#endif
            continue;
        }

        if (CsrStrNCmp(argv[i], "-ipc_aclport:", 13) == 0)
        {
#ifdef IPC_IP
            ipc_aclPortNumber = (CsrUint32)atoi(&argv[i][13]);
            sme_trace_info((TR_IPC, "sme_schedule_init() : ACL IPC Port override -> %s :: %d", argv[i], ipc_aclPortNumber));
#endif
            continue;
        }
#endif

        /* TODO :: This is depricated... Remove! */
        if (CsrStrNCmp(argv[i], "-ipc_connect=", 13) == 0)
        {
#ifdef IPC_CHARDEVICE
            connectStr = &argv[i][13];
            sme_trace_crit((TR_IPC, "sme_schedule_init() : -ipc_connect option is depricated. DO NOT USE!"));
            sme_trace_info((TR_IPC, "sme_schedule_init() : IPC Connect String -> %s :: %s", argv[i], connectStr));
#endif
            continue;
        }

        if (CsrStrNCmp(argv[i], "-dev=", 5) == 0)
        {
#ifdef IPC_CHARDEVICE
            connectStr = &argv[i][5];
            sme_trace_info((TR_IPC, "sme_schedule_init() : char device -> %s :: %s", argv[i], connectStr));
#endif
            continue;
        }
        if (CsrStrNCmp(argv[i], "-paldatadev=", 12) == 0)
        {
            continue;
        }
#ifdef CSR_AMP_ENABLE
        if (CsrStrNCmp(argv[i], "-palselectchannel=", 18) == 0)
        {
            int len = CsrStrLen(argv[i]);
            char* str = (char*)CsrPmalloc(len);
            CsrMemCpy(str,&(argv[i][1]), len-1);
            str[len-1] = '\0';

            sme_trace_info((TR_IPC, "sme_schedule_init() : pal channel to select -> %s :: %d , len-%d,str-%s", argv[i], (CsrUint8)atoi(&argv[i][18]),len,str));
            unifi_dbg_cmd_req(context->fsmContext, str);
            continue;
        }

        if (CsrStrNCmp(argv[i], "-paldisableqos", 18) == 0)
        {
            int len = CsrStrLen(argv[i]);
            char* str = (char*)CsrPmalloc(len);
            CsrMemCpy(str,&(argv[i][1]), len-1);
            str[len-1] = '\0';

            sme_trace_info((TR_IPC, "sme_schedule_init() : disable qos to select -%s , len-%d,str-%s", argv[i],len,str));
            unifi_dbg_cmd_req(context->fsmContext, str);

            continue;
        }

        if (CsrStrNCmp(argv[i], "-paldisablesecurity", 18) == 0)
        {
            int len = CsrStrLen(argv[i]);
            char* str = (char*)CsrPmalloc(len);
            CsrMemCpy(str,&(argv[i][1]), len-1);
            str[len-1] = '\0';

            sme_trace_info((TR_IPC, "sme_schedule_init() : disable qos to select -%s", argv[i]));
            unifi_dbg_cmd_req(context->fsmContext, str);

            continue;
        }
#endif
        if (CsrStrNCmp(argv[i], "-flightmode", 11) == 0)
        {
            flightmode = TRUE;
            sme_trace_info((TR_IPC, "sme_schedule_init() : unifi_mgt_wifi_flightmode_req will be sent on connect -> %s", argv[i]));
            continue;
        }

        if (CsrStrNCmp(argv[i], "-wifion", 7) == 0)
        {
            wifion = TRUE;
            sme_trace_info((TR_IPC, "sme_schedule_init() : unifi_mgt_wifi_on_req will be sent on connect -> %s", argv[i]));
            continue;
        }

        if (CsrStrNCmp(argv[i], "-mac=", 5) == 0)
        {
            loadMacAddress(&argv[i][5], &getMainData(linuxContext)->address);
            sme_trace_info((TR_IPC, "sme_schedule_init() : macAddress file -> %s", argv[i]));
            sme_trace_info((TR_IPC, "sme_schedule_init() : macAddress -> %s", trace_unifi_MACAddress(getMainData(linuxContext)->address, getMainData(linuxContext)->traceMacAddressBuffer)));
            continue;
        }
        if (CsrStrNCmp(argv[i], "-mib=", 5) == 0)
        {
            loadMibfile(&argv[i][5], &getMainData(linuxContext)->mibfiles);
            sme_trace_info((TR_IPC, "sme_schedule_init() : mib file -> %s", argv[i]));
            continue;
        }
        if (CsrStrNCmp(argv[i], "-cal=", 5) == 0)
        {
            getMainData(linuxContext)->calibrationDataFile = &argv[i][5];
            (void)loadfile(getMainData(linuxContext)->calibrationDataFile, &getMainData(linuxContext)->calibrationData);
            sme_trace_info((TR_IPC, "sme_schedule_init() : cal file -> %s", getMainData(linuxContext)->calibrationDataFile));
            continue;
        }
        /* Skip -sme_trace:... as the sme trace module will handle these */
        if (CsrStrNCmp(argv[i], "-sme_trace:", 11) == 0)
        {
            continue;
        }

        if (CsrStrNCmp(argv[i], "-v", CsrStrLen(argv[i])) == 0)
        {
            print_versions();
            exit(EXIT_CODE_NORMAL_EXIT);
        }

        if (CsrStrNCmp(argv[i], "-exitOnError", CsrStrLen(argv[i])) == 0)
        {
            getMainData(linuxContext)->exitOnError = TRUE;
            continue;
        }

        if (CsrStrNCmp(argv[i], "-stopOnError", CsrStrLen(argv[i])) == 0)
        {
            getMainData(linuxContext)->stopOnError = TRUE;
            continue;
        }

        if (CsrStrNCmp(argv[i], "-h", CsrStrLen(argv[i])) == 0)
        {
            print_usage();
            exit(EXIT_CODE_WIFION_ERROR);
        }

        print_usage();
        sme_trace_error((TR_IPC, "error : Unknown commandline option : %s", argv[i]));
        exit(EXIT_CODE_WIFION_ERROR);
    }

#ifdef IPC_CHARDEVICE
    if (connectStr)
    {
        getMainData(context)->charIpcCon = ipc_chardevice_connect(connectStr, NULL, NULL, NULL, NULL);
        if (getMainData(context)->charIpcCon == NULL)
        {
            sme_trace_crit((TR_IPC, "sme_schedule_init() : char device connect to %s failed", connectStr));
            exit(EXIT_CODE_WIFION_ERROR);
        }
    }
#endif

#ifdef IPC_IP
    getMainData(context)->ipIpcCon = ipc_ip_create((int)ipc_portNumber, NULL, NULL, NULL, NULL);
    if (getMainData(context)->ipIpcCon == NULL)
    {
        sme_trace_crit((TR_IPC, "sme_schedule_init() : ipc_create(port:%d) failed", ipc_portNumber));
        exit(EXIT_CODE_WIFION_ERROR);
    }

#ifdef CSR_AMP_ENABLE
    getMainData(context)->ipHciIpcCon = ipc_ip_create((int)ipc_hciPortNumber, NULL, NULL, NULL, NULL);
    if (getMainData(context)->ipHciIpcCon == NULL)
    {
        sme_trace_crit((TR_IPC, "sme_schedule_init() : ipc_create(port:%d) failed", ipc_hciPortNumber));
        exit(EXIT_CODE_WIFION_ERROR);
    }
    getMainData(context)->ipAclIpcCon = ipc_ip_create((int)ipc_aclPortNumber, NULL, NULL, NULL, NULL);
    if (getMainData(context)->ipAclIpcCon == NULL)
    {
        sme_trace_crit((TR_IPC, "sme_schedule_init() : ipc_create(port:%d) failed", ipc_aclPortNumber));
        exit(EXIT_CODE_WIFION_ERROR);
    }
#endif

#endif

    if (getMainData(linuxContext)->calibrationData.length)
    {
        unifi_AppValue appValue;
        appValue.id = unifi_CalibrationDataValue;
        appValue.unifi_Value_union.calibrationData = getMainData(linuxContext)->calibrationData;
        unifi_mgt_claim_sync_access(linuxContext->fsmContext);
        (void)unifi_mgt_set_value(linuxContext->fsmContext, &appValue);
        unifi_mgt_release_sync_access(linuxContext->fsmContext);
    }

    if (flightmode)
    {
        unifi_mgt_wifi_flightmode_req(linuxContext->fsmContext, NULL,
                                      &getMainData(context)->address,
                                      getMainData(linuxContext)->mibfiles.numElements,
                                      getMainData(linuxContext)->mibfiles.dataList);

    }

    if (wifion)
    {
        /* Set the nextCaldataSaveTime to 1 minute from now */
        getMainData(linuxContext)->nextCaldataSaveTime = fsm_get_time_of_day_ms(context->fsmContext) + CALDATA_INITIAL_SAVE_INTERVAL_MS;
        unifi_mgt_wifi_on_req(linuxContext->fsmContext, NULL,
                              &getMainData(context)->address,
                              getMainData(linuxContext)->mibfiles.numElements,
                              getMainData(linuxContext)->mibfiles.dataList);
    }
}

#ifdef CSR_AMP_ENABLE
static void fsm_pal_hci_recieve_message(FsmContext* fsmContext, ipcConnection* connection)
{
    CsrUint8* buffer;
    CsrUint32 length;

    (void)ipc_message_recv(connection, (CsrUint32)-1 , &buffer, &length);
    if (length != 0)
    {
        /* check for dbg_exit_req and handle outside of the FSM. This is because
         * the rules are we can't called sme_shutdown from inside sme_execute
         */
        if (custom_handler_dbg_exit_req(fsmContext, buffer, connection)) {} /*lint !e774*/
        else if (unpack_pal_hci_signalDown_received(linuxContext->fsmContext, buffer, (CsrUint16)length)) {} /*lint !e774*/
        else
        {
            sme_trace_error((TR_IPC, "Unhandled HCI ipc message"));
        }
        ipc_message_free(connection, buffer);
    }
}

static void fsm_pal_acl_recieve_message(FsmContext* fsmContext, ipcConnection* connection)
{
    CsrUint8* buffer;
    CsrUint32 length;

    (void)ipc_message_recv(connection, (CsrUint32)-1 , &buffer, &length);
    if (length != 0)
    {
        sme_trace_info((TR_IPC, "fsm_pal_acl_recieve_message: message of lenght -%d recvd",length));
        if (remote_paldata_acl_signal_receive(fsmContext, buffer, (CsrUint16)length)) {} /*lint !e774*/
        else
        {
            sme_trace_error((TR_IPC, "fsm_pal_acl_recieve_message: Unhandled ipc message"));
        }
    }
    ipc_message_free(connection, buffer);
}
#endif

#if defined IPC_IP || defined IPC_CHARDEVICE

/* Extract the appHandle only. last bits is the instance Id */
#define GET_APP_HANDLE(appHandle) ((appHandle&0x000000C0)>>6)
static CsrBool remote_sys_data_signal(CsrUint8* buffer, CsrUint16 size)
{
    CsrUint8* tempbuffer = buffer;
    CsrUint16 id = event_unpack_CsrUint16(&tempbuffer);
    CsrBool status=FALSE;

    if (id >= 0x8020 &&
        id <= 0x802D)
    {
        tempbuffer += 4; /* Skip the 4 bytes sender and 1st slotnumber */
        CsrUint32 appHandle = event_unpack_CsrUint32(&tempbuffer);

        sme_trace_entry((TR_IPC, "remote_sys_data_signal: sys data signal id-0x%0x with appHandle-0x%x",id,appHandle));

        switch (GET_APP_HANDLE(appHandle))
        {
            case UNIFI_SME_APPHANDLE:
                status = remote_sys_signal_receive(linuxContext->fsmContext, buffer, size);
                break;

#ifdef CSR_AMP_ENABLE
            case UNIFI_PALDATA_APPHANDLE:
                status = remote_paldata_sys_signal_receive(linuxContext->palDataFsmContext, buffer, size);
                break;
#endif
#ifdef CSR_WIFI_NME_ENABLE
            case UNIFI_NME_APPHANDLE:
                status = remote_csr_wifi_nme_sys_signal_receive(linuxContext->nmeFsmContext, buffer, size);
                break;
#endif
            default:
                sme_trace_error((TR_IPC, "Invalid App Handle - %d",GET_APP_HANDLE(appHandle)));
                break;
        }
    }
    return status;
}
static void fsm_recieve_message(FsmContext* fsmContext, ipcConnection* connection)
{
    CsrUint8* buffer;
    CsrUint32 length;

    (void)ipc_message_recv(connection, (CsrUint32)-1 , &buffer, &length);
    if (length != 0)
    {
        /* check for dbg_exit_req and handle outside of the FSM. This is because
         * the rules are we can't called sme_shutdown from inside sme_execute
         */
        if (custom_handler_dbg_exit_req(fsmContext, buffer, connection)) {} /*lint !e774*/
        else if (remote_sys_data_signal(buffer, (CsrUint16)length))  {} /*lint !e774*/
#ifdef CSR_WIFI_NME_ENABLE
        else if (remote_csr_wifi_nme_signal_receive(linuxContext->nmeFsmContext, buffer, (CsrUint16)length)) {} /*lint !e774*/
#else
        else if (remote_mgt_signal_receive(fsmContext, buffer, (CsrUint16)length)) {} /*lint !e774*/
#endif
        else if (remote_sys_signal_receive(fsmContext, buffer, (CsrUint16)length)) {} /*lint !e774*/
        else if (remote_dbg_signal_receive(fsmContext, buffer, (CsrUint16)length)) {} /*lint !e774*/
        else if (remote_bt_signal_receive(fsmContext, buffer, (CsrUint16)length)) {} /*lint !e774*/
        else
        {
            sme_trace_error((TR_IPC, "Unhandled ipc message"));
        }
        ipc_message_free(connection, buffer);
    }
}
#endif

static void fsm_schedule(LinuxUserSpaceContext* context)
{
    struct timeval selectTimeout;
    fd_set readfds;
    int r;
#ifdef IPC_IP
    CsrInt32 ipfd;
#endif
#ifdef IPC_CHARDEVICE
    CsrInt32 charfd;
#endif
    sme_trace_entry((TR_FSM, "fsm_schedule()"));

    for(;;)
    {
        CsrInt32 highestfd = 0;
        FsmTimerData timeout;

        /* To handle messages between the 2 FSM's we loop
         * until no message passes between the 2 */
        do {
#ifdef CSR_AMP_ENABLE
            FsmTimerData amptimeout;
#endif
#ifdef CSR_WIFI_NME_ENABLE
            FsmTimerData nmeTimeout;
#endif

            getMainData(linuxContext)->externalEvent = FALSE;

            timeout = sme_execute(context->fsmContext);

#ifdef CSR_AMP_ENABLE
            amptimeout = paldata_execute(context->palDataFsmContext);
            /* Wake up on the lowest timeout */
            if (amptimeout.timeoutTimeMs < timeout.timeoutTimeMs)
            {
                timeout = amptimeout;
            }
#endif
#ifdef CSR_WIFI_NME_ENABLE
            /* Need to execute the NME after the SME and PAL as these have to handle far more events
             * than the NME
             */
            nmeTimeout = csr_wifi_nme_execute(context->nmeFsmContext);
            /* Wake up on the lowest timeout */
            if (nmeTimeout.timeoutTimeMs < timeout.timeoutTimeMs)
            {
                timeout = nmeTimeout;
            }
#endif

        } while(getMainData(linuxContext)->externalEvent);

        selectTimeout.tv_sec  = (long) (timeout.timeoutTimeMs / (CsrUint32) COAL_MILLISECOND);
        selectTimeout.tv_usec = (timeout.timeoutTimeMs % COAL_MILLISECOND) * COAL_MILLISECOND /* for microsecs */;

        /* Initialise the fd_set for select on the char device */
        FD_ZERO(&readfds);

#ifdef IPC_IP
        ipfd = ipc_getFd(getMainData(linuxContext)->ipIpcCon);
        if (ipfd != IPC_INVALID_FD)
        {
            highestfd = ipfd;
            FD_SET((unsigned int) ipfd, &readfds);
        }

#ifdef CSR_AMP_ENABLE
        ipfd = ipc_getFd(getMainData(linuxContext)->ipHciIpcCon);
        if (ipfd != IPC_INVALID_FD)
        {
            if (ipfd > highestfd)
            {
                highestfd = ipfd;
            }
            FD_SET((unsigned int) ipfd, &readfds);
        }
        ipfd = ipc_getFd(getMainData(linuxContext)->ipAclIpcCon);
        if (ipfd != IPC_INVALID_FD)
        {
            if (ipfd > highestfd)
            {
                highestfd = ipfd;
            }
            FD_SET((unsigned int) ipfd, &readfds);
        }
#endif
#endif

#ifdef IPC_CHARDEVICE
        charfd = ipc_getFd(getMainData(linuxContext)->charIpcCon);
        if (charfd != IPC_INVALID_FD)
        {
#ifdef IPC_IP
            if (charfd > ipfd)
            {
                highestfd = charfd;
            }
#else
            highestfd = charfd;
#endif
            if (charfd != IPC_INVALID_FD)
            {
                FD_SET((unsigned int) charfd, &readfds);
            }
        }

#ifdef BT_SAP_FD_SUPPORT
        if (linuxContext->btsapfd > 0)
        {
            FD_SET(linuxContext->btsapfd, &readfds);
            if (linuxContext->btsapfd > highestfd)
            {
                highestfd = linuxContext->btsapfd;
            }
        }
#endif

#endif
        r = select(highestfd + 1, &readfds, NULL, NULL, &selectTimeout);
        if ((r < 0) && (errno != EINTR))
        {
            sme_trace_crit((TR_IPC, "sme_schedule(): select() failed: %s\n", strerror(errno)));
            exit(errno);
        }

#ifdef IPC_IP
        if (ipc_getFd(getMainData(linuxContext)->ipIpcCon) != IPC_INVALID_FD &&
            FD_ISSET(ipc_getFd(getMainData(linuxContext)->ipIpcCon), &readfds)) /*lint !e573 !e737 !e666*/
        {
            fsm_recieve_message(linuxContext->fsmContext, getMainData(linuxContext)->ipIpcCon);
        }
#ifdef CSR_AMP_ENABLE
        if (ipc_getFd(getMainData(linuxContext)->ipHciIpcCon) != IPC_INVALID_FD &&
            FD_ISSET(ipc_getFd(getMainData(linuxContext)->ipHciIpcCon), &readfds)) /*lint !e573 !e737*/
        {
            fsm_pal_hci_recieve_message(linuxContext->fsmContext, getMainData(linuxContext)->ipHciIpcCon);
        }
        if (ipc_getFd(getMainData(linuxContext)->ipAclIpcCon) != IPC_INVALID_FD &&
            FD_ISSET(ipc_getFd(getMainData(linuxContext)->ipAclIpcCon), &readfds)) /*lint !e573 !e737*/
        {
            fsm_pal_acl_recieve_message(linuxContext->palDataFsmContext, getMainData(linuxContext)->ipAclIpcCon);
        }
#endif
#endif

#ifdef IPC_CHARDEVICE
        if (charfd != IPC_INVALID_FD && FD_ISSET(charfd, &readfds)) /*lint !e573 !e737*/
        {
            fsm_recieve_message(linuxContext->fsmContext, getMainData(linuxContext)->charIpcCon);
        }

#ifdef BT_SAP_FD_SUPPORT
        if (linuxContext->btsapfd > 0 && FD_ISSET(linuxContext->btsapfd, &readfds))
        {
            bt_fd_triggered(linuxContext);
        }
#endif

#endif
        /* Check the signal type and save the cal data on connect cfm and wifi on cfm */
        if (CsrTimeLe(getMainData(linuxContext)->nextCaldataSaveTime, fsm_get_time_of_day_ms(context->fsmContext)))
        {
            getMainData(linuxContext)->nextCaldataSaveTime = fsm_get_time_of_day_ms(context->fsmContext) + CALDATA_SAVE_INTERVAL_MS;
            save_calibration_data(linuxContext);
        }
    }
}

int main(int argc, char **argv)
{
    /* Create the Application Context */
    linuxContext = (LinuxUserSpaceContext*) CsrPmalloc(sizeof(LinuxUserSpaceContext));
    CsrMemSet(linuxContext, 0x00, sizeof(LinuxUserSpaceContext));

    linuxContext->mainData = (MainData*) CsrPmalloc(sizeof(MainData));
#ifdef IPC_IP
    getMainData(linuxContext)->ipIpcCon = NULL;
#endif
#ifdef IPC_CHARDEVICE
    getMainData(linuxContext)->charIpcCon = NULL;
#endif

    /* Set Trace levels from the command line */
    sme_trace_initialise((CsrUint32)argc, argv);

    /* Send the first trace message */
    sme_trace_entry((TR_STARTUP_SAP, "%s(%d)", __FUNCTION__, argc));

    /* Initialise SME and scheduling */
    sme_schedule_init(linuxContext, argc, argv);

    fsm_schedule(linuxContext);
    return 0;
}
