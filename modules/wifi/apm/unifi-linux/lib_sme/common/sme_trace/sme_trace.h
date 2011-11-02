/** @file sme_trace.h
 *
 *  Trace facilities for the SME
 *
 * @section LEGAL
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Provides prototypes for the SME trace facilities
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/sme_trace/sme_trace.h#1 $
 *
 ****************************************************************************/

#ifndef SME_TRACE_H
#define SME_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

typedef enum sme_trace_level
{
    TR_LVL_ENTRY,
    TR_LVL_DEBUG,
    TR_LVL_INFO,
    TR_LVL_WARN,
    TR_LVL_ERROR,
    TR_LVL_CRIT,
    TR_LVL_OFF
} sme_trace_level;

/*
 * Add New Trace ID's here...
 * Only add to the end before LAST_MODULE_ID
 */
typedef enum sme_trace_module_id
{
    /*  ----------------------------------------- */
    /*              SME TRACE CODE                */
    /*  ----------------------------------------- */
    TR_SME_TRACE,
    /*  ----------------------------------------- */

    /*  ----------------------------------------- */
    /*                  SME LIBS                  */
    /*  ----------------------------------------- */
    TR_CSR_LIST,
    TR_IE_ACCESS,
    TR_IE_PRINT,
    TR_PAYLOAD_MGR,
    TR_OSA,
    TR_IPC,
    /*  ----------------------------------------- */

    /*  ----------------------------------------- */
    /*                  SME SAPS                  */
    /*  ----------------------------------------- */
    TR_STARTUP_SAP,
    TR_SYS_SAP,
    TR_MGT_SAP,
    TR_DBG_SAP,
    TR_BT_SAP,
    /*  ----------------------------------------- */

    /*  ----------------------------------------- */
    /*          SME PROCESSES / MODULES           */
    /*  ----------------------------------------- */
    TR_SCAN_STORAGE,
    TR_CORE,
    TR_CXN_MGR,
    TR_SECURITY,
    TR_MIB_ACCESS,
    TR_SCAN,
    TR_LINK_QUALITY,
    TR_DBG,
    TR_HIP_PROXY,
    TR_MIB_ACCESS_FSM,
    TR_NETWORK_SELECTOR_FSM,
    TR_UNIFI_DRIVER_FSM,
    TR_SME_CONFIGURATION_FSM,
    TR_SME_MEASUREMENTS_FSM,
    TR_POWER_MGR,
    TR_COEX,
    TR_CCX,
    TR_DOT11N,
    TR_REGDOM,
    TR_QOS,
    TR_CRYPTO_LIB,

    /*  ----------------------------------------- */

    /*  ----------------------------------------- */
    /* FSM                                        */
    /*  ----------------------------------------- */
    TR_FSM,
    TR_FSM_DUMP,
    TR_MSC,
    /*  ----------------------------------------- */

    /*  ----------------------------------------- */
    /* SECURITY LIBRARY                           */
    /*  ----------------------------------------- */
    TR_SECURITY_LIB,
    TR_SECURITY_WAPI,

    /*  ----------------------------------------- */
    /* PAL                                        */
    /*  ----------------------------------------- */
#ifdef CSR_AMP_ENABLE
    TR_PAL_HCI_SAP,
    TR_PAL_ACL_SAP,
    TR_PAL_CTRL_SAP,
    TR_PAL_SYS_SAP,
    TR_PAL_MGR_FSM,
    TR_PAL_LM_LINK_FSM,
    TR_PAL_DM_FSM,
    TR_PAL_DAM,
    TR_PAL_LM_HIP_FSM,
    TR_PAL_COEX_FSM,

#endif

    /*  ----------------------------------------- */
    /* NME                                        */
    /*  ----------------------------------------- */
#ifdef CSR_WIFI_NME_ENABLE
    TR_NME_NME_SAP,
    TR_NME_MGT_SAP,
    TR_NME_SIG_FSM,
    TR_NME_CORE_FSM,
    TR_NME_PMGR_FSM,
    TR_NME_CMGR_FSM,
    TR_NME_SMGR_FSM,
    TR_NME_CCX_FSM,
    TR_NME_WPS_FSM,
    TR_NME_NS_FSM,
    TR_NME_SIGR,
#endif

    /*  ----------------------------------------- */

    TR_LAST_MODULE_ID
} sme_trace_module_id;


#ifdef SME_TRACE_ENABLE

    extern void sme_trace_initialise(CsrUint32 argc, char **argv);

    extern void sme_trace_set_all_levels(sme_trace_level level);
    extern void sme_trace_set_module_level(sme_trace_module_id id, sme_trace_level level);
    extern sme_trace_level sme_trace_get_module_level(sme_trace_module_id id);
    extern const char* sme_trace_module_to_str(sme_trace_module_id id);

    /*
    extern void sme_trace_fn(const char* name, const char* const format, ...);
    #define sme_trace_msc(tracedata, args) if (tracedata->level > TR_LVL_INFO) { sme_trace_fn(tracedata->name, args); }
    */

    extern void sme_trace_msc_fn(sme_trace_module_id id, const char* const format, ...);
    extern void sme_trace_entry_fn(sme_trace_module_id id, const char* const format, ...);
    extern void sme_trace_debug_fn(sme_trace_module_id id, const char* const format, ...);
    extern void sme_trace_info_fn(sme_trace_module_id id, const char* const format, ...);
    extern void sme_trace_warn_fn(sme_trace_module_id id, const char* const format, ...);
    extern void sme_trace_error_fn(sme_trace_module_id id, const char* const format, ...);
    extern void sme_trace_crit_fn(sme_trace_module_id id, const char* const format, ...);
    extern void sme_trace_hex_fn(sme_trace_module_id id, sme_trace_level level, const char* const message, const void* address, CsrUint32 length);

#else

    #define sme_trace_initialise(argc, argv)

    #define sme_trace_set_all_levels(level)
    #define sme_trace_get_module_level(id) TR_LVL_OFF
    #define sme_trace_set_module_level(id,level)
    #define sme_trace_module_to_str(id) ""

    #define SME_TRACE_NO_MSC
    #define SME_TRACE_NO_ENTRY
    #define SME_TRACE_NO_DEBUG
    #define SME_TRACE_NO_INFO
    #define SME_TRACE_NO_WARN
    #define SME_TRACE_NO_ERROR
    #define SME_TRACE_NO_CRIT
    #define SME_TRACE_NO_HEX
#endif


#ifdef SME_TRACE_NO_MSC
   #define sme_trace_msc(args)
   #define sme_trace_msc_code(args)
#else
   #define sme_trace_msc(args) sme_trace_msc_fn args
   #define sme_trace_msc_code(args) args
#endif

#ifdef SME_TRACE_NO_ENTRY
   #define sme_trace_entry(args)
   #define sme_trace_entry_code(args)
#else
   #define sme_trace_entry(args) sme_trace_entry_fn args
   #define sme_trace_entry_code(args) args
#endif

#ifdef SME_TRACE_NO_DEBUG
   #define sme_trace_debug(args)
   #define sme_trace_debug_code(args)
#else
   #define sme_trace_debug(args) sme_trace_debug_fn args
   #define sme_trace_debug_code(args) args
#endif

#ifdef SME_TRACE_NO_INFO
   #define sme_trace_info(args)
   #define sme_trace_info_code(args)
#else
   #define sme_trace_info(args) sme_trace_info_fn args
   #define sme_trace_info_code(args) args
#endif

#ifdef SME_TRACE_NO_WARN
   #define sme_trace_warn(args)
   #define sme_trace_warn_code(args)
#else
   #define sme_trace_warn(args) sme_trace_warn_fn args
   #define sme_trace_warn_code(args) args
#endif

#ifdef SME_TRACE_NO_ERROR
   #define sme_trace_error(args)
   #define sme_trace_error_code(args)
#else
   #define sme_trace_error(args) sme_trace_error_fn args
   #define sme_trace_error_code(args) args
#endif

#ifdef SME_TRACE_NO_CRIT
   #define sme_trace_crit(args)
   #define sme_trace_crit_code(args)
#else
   #define sme_trace_crit(args) sme_trace_crit_fn args
   #define sme_trace_crit_code(args) args
#endif

#ifdef SME_TRACE_NO_HEX
   #define sme_trace_hex(args)
   #define sme_trace_hex_code(args)
#else
   #define sme_trace_hex(args) sme_trace_hex_fn args
   #define sme_trace_hex_code(args) args
#endif

#ifdef __cplusplus
}
#endif

#endif /* SME_TRACE_H */
