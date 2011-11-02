/*
 * ---------------------------------------------------------------------------
 * FILE: unifi_xbv.h
 *
 * PURPOSE:
 *      Definitions for test entry point around XBV parser in lib_hip
 *      
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */

#ifndef _UNIFI_XBV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "csr_types.h"
#include "csr_framework_ext.h"
#include "csr_util.h"

#define CSR_SLT_END 0
#define DBG_TAG(t)  printf("[%c%c%c%c]\n", t[0], t[1], t[2], t[3])

#define UDBG1   1

/* Simplified card structure for the test harness */
typedef struct card {
    CsrUint32 build_id;
    void *ospriv;
} card_t;

/* Structure of an entry in the Symbol Look Up Table (SLUT). */
typedef struct _symbol {
    CsrUint16 id;
    CsrUint32 obj;
} symbol_t;

void unifi_error(void* ospriv, const char *fmt, ...);
void *unifi_malloc(void *priv, size_t size);
void unifi_free(void *priv, void *buf);
void unifi_trace(void* ospriv, int lvl, const char *fmt, ...);

#endif /* _UNIFI_XBV_H */
