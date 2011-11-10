/** @file sme_pbc.h
 *
 * programming by contract facilities for the SME
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
 *   programming by contract facilities for the SME
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/sme_trace/sme_pbc.h#1 $
 *
 ****************************************************************************/
#ifndef __SME_PBC_H__
#define __SME_PBC_H__

/* ------------------------------------ */
/* Configuration Options */
/* ------------------------------------ */
/*
    #define SME_PBC_NO_ASSERTS      == ALL OFF
    #define SME_PBC_NO_REQUIRE      == Turn off requires
    #define SME_PBC_NO_VERIFY       == Turn off verifys
    #define SME_PBC_NO_ENSURE       == Turn off ensures
    #define SME_PBC_NO_INVARIANT    == Turn off invariants
*/
/* ------------------------------------ */

#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SME_PBC_NO_ASSERTS
    #define SME_PBC_NO_REQUIRE
    #define SME_PBC_NO_VERIFY
    #define SME_PBC_NO_ENSURE
    #define SME_PBC_NO_INVARIANT
#endif

/* ------------------------------------ */
/* Require PBC Macros */
/* ------------------------------------ */
#define PBCSTRINGIFY(x) #x
#define PBCTOSTRING(x) PBCSTRINGIFY(x)

#ifdef SME_PBC_NO_REQUIRE
    #define require_code(args)
    #define require(id,condition)
    #define require_trace(id,condition,args)
#else
    #define require_code(args) args
    #define require(id,condition) \
        if (!(condition)) { \
            CsrPanic(CSR_TECH_WIFI, CSR_PANIC_FW_ASSERTION_FAIL,  __FILE__ "(" PBCTOSTRING(__LINE__) ") : require failure. " #condition); \
        }

    #define require_trace(id,condition,args) \
        if (!(condition)) { \
            sme_trace_crit(args); \
            CsrPanic(CSR_TECH_WIFI, CSR_PANIC_FW_ASSERTION_FAIL,  __FILE__ "(" PBCTOSTRING(__LINE__) ") : require failure. " #condition); \
        }
#endif
/* ------------------------------------ */

/* ------------------------------------ */
/* Verify PBC Macros */
/* ------------------------------------ */
#ifdef SME_PBC_NO_VERIFY
    #define verify_code(args)
    #define verify(id,condition)
    #define verify_trace(id,condition,args)
#else
    #define verify_code(args) args
    #define verify(id,condition) \
        if (!(condition)) { \
            CsrPanic(CSR_TECH_WIFI, CSR_PANIC_FW_ASSERTION_FAIL,  __FILE__ "(" PBCTOSTRING(__LINE__) ") : verify failure. " #condition); \
        }

    #define verify_trace(id,condition,args) \
        if (!(condition)) { \
            sme_trace_crit(args); \
            CsrPanic(CSR_TECH_WIFI, CSR_PANIC_FW_ASSERTION_FAIL,  __FILE__ "(" PBCTOSTRING(__LINE__) ") : verify failure. " #condition); \
        }
#endif
/* ------------------------------------ */

/* ------------------------------------ */
/* Ensure PBC Macros */
/* ------------------------------------ */
#ifdef SME_PBC_NO_ENSURE
    #define ensure_code(args)
    #define ensure(id,condition)
    #define ensure_trace(id,condition,args)
#else
    #define ensure_code(args) args
    #define ensure(id,condition) \
        if (!(condition)) { \
            CsrPanic(CSR_TECH_WIFI, CSR_PANIC_FW_ASSERTION_FAIL,  __FILE__ "(" PBCTOSTRING(__LINE__) ") : ensure failure. " #condition); \
        }

    #define ensure_trace(id,condition,args) \
        if (!(condition)) { \
            sme_trace_crit(args); \
            CsrPanic(CSR_TECH_WIFI, CSR_PANIC_FW_ASSERTION_FAIL,  __FILE__ "(" PBCTOSTRING(__LINE__) ") : ensure failure. " #condition); \
        }
#endif
/* ------------------------------------ */

/* ------------------------------------ */
/* Invariant PBC Macros */
/* ------------------------------------ */
#ifdef SME_PBC_NO_INVARIANT
    #define invariant_code(args)
    #define invariant(id,condition)
#else
    #define invariant_code(args) args
    #define invariant(id,condition) \
        if (!(condition)) { \
            CsrPanic(CSR_TECH_WIFI, CSR_PANIC_FW_ASSERTION_FAIL,  __FILE__ "(" PBCTOSTRING(__LINE__) ") : invariant failure. " #condition); \
        }
#endif
/* ------------------------------------ */

#ifdef __cplusplus
}
#endif

#endif /* __SME_PBC_H__ */
