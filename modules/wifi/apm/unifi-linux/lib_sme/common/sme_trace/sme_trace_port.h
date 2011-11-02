/** @file sme_trace_port.h
 *
 *  Trace facilities for the SME
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
 *   Provides prototypes for the port specific SME trace facilities.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/sme_trace/sme_trace_port.h#1 $
 *
 ****************************************************************************/

#ifndef SME_TRACE_PORT_H
#define SME_TRACE_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"

#ifdef SME_TRACE_ENABLE
    typedef struct moduleconfig {
        const char* const name;
        sme_trace_level level;
    } sme_trace_moduleconfig;

    extern sme_trace_moduleconfig modulecfg[TR_LAST_MODULE_ID];

    extern void sme_trace_initialise_port(CsrUint32 argc, char **argv);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SME_TRACE_PORT_H */
