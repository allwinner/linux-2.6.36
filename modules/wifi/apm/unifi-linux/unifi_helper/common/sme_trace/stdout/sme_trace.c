/** @file sme_trace.c
 *
 * Provides a port for the SME trace prototypes
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
 *   stdout implementation for the sme_trace.h prototypes
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/sme_trace/stdout/sme_trace.c#1 $
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_trace_port.h"
#include "sme_trace/stdout/trace_time.h"

#ifdef SME_TRACE_ENABLE

/* -------------------------------------------------------
 VT100 Colour codes....

 <ESC>[{attr};{fg};{bg}m

 {attr} is one of following

 0   Reset All Attributes (return to normal mode)
 1   Bright (Usually turns on BOLD)
 2   Dim
 3   Underline
 5   Blink
 7   Reverse
 8   Hidden

 {fg} is one of the following

 30  Black
 31  Red
 32  Green
 33  Yellow
 34  Blue
 35  Magenta
 36  Cyan
 37  White

 {bg} is one of the following

 40  Black
 41  Red
 42  Green
 43  Yellow
 44  Blue
 45  Magenta
 46  Cyan
 47  White
 ------------------------------------------------------- */

#define VT100_CANCEL "\033[0m"

#define VT100_BLUE "\033[1;34m"
#define VT100_RED_UNDERLINE "\033[1;4;31m"
#define VT100_REVERSE_RED "\033[37;41m"
#define VT100_MAGENTA_UNDERLINE "\033[1;4;35m"

/* Unused ------
 #define VT100_BLACK "\033[1;30m"
 #define VT100_RED "\033[1;31m"
 #define VT100_GREEN "\033[1;32m"
 #define VT100_YELLOW "\033[1;33m"
 #define VT100_MAGENTA "\033[1;35m"
 #define VT100_CYAN "\033[1;36m"
 #define VT100_WHITE "\033[1;37m"

 #define VT100_BLACK_UNDERLINE "\033[1;4;30m"
 #define VT100_GREEN_UNDERLINE "\033[1;4;32m"
 #define VT100_YELLOW_UNDERLINE "\033[1;4;33m"
 #define VT100_BLUE_UNDERLINE "\033[1;4;34m"
 #define VT100_CYAN_UNDERLINE "\033[1;4;36m"
 #define VT100_WHITE_UNDERLINE "\033[1;4;37m"
 */

typedef struct levelconfig
{
    const char* const name;
    const char* colour;
} levelconfig;

/* These static's need to use a control block */
static const char* vt100Cancel = "";
static levelconfig levelcfg[TR_LVL_OFF + 1] = { { "ENTRY", "" },
                                                { "DEBUG", "" },
                                                { "INFO ",  "" },
                                                { "WARN ",  "" },
                                                { "ERROR", "" },
                                                { "CRIT ",  "" },
                                                { "OFF  ",   "" } };

const char* sme_trace_module_to_str(sme_trace_module_id id)
{
    if (id >= TR_LAST_MODULE_ID) return "UNKNOWN MODULE";
    return modulecfg[id].name;
}

static const char* level_to_color(sme_trace_level level)
{
    if (level > TR_LVL_OFF)
    {
        sme_trace_error((TR_SME_TRACE, "level_to_color(%d) :: Unknown trace level", level));
        return "";
    }
    return levelcfg[level].colour;
}

static const char* level_to_str(sme_trace_level level)
{
    if (level > TR_LVL_OFF)
    {
        sme_trace_error((TR_SME_TRACE, "level_to_str(%d) :: Unknown trace level", level));
        return "UNKNOWN LEVEL";
    }
    return levelcfg[level].name;
}

void sme_trace_initialise_port(CsrUint32 argc, char **argv)
{
    CsrUint32 i;
    char* parsestring= NULL;

    for (i = 1; i < argc; i++)
    {
        if (CsrStrNCmp(argv[i], "-sme_trace:", 11) == 0)
        {
            parsestring = &argv[i][11];

            if (CsrStrNCmp(parsestring, "vt100", 5) == 0)
            {
                vt100Cancel = VT100_CANCEL;
                levelcfg[TR_LVL_INFO].colour = VT100_BLUE;
                levelcfg[TR_LVL_WARN].colour = VT100_MAGENTA_UNDERLINE;
                levelcfg[TR_LVL_ERROR].colour = VT100_RED_UNDERLINE;
                levelcfg[TR_LVL_CRIT].colour = VT100_REVERSE_RED;
                continue;
            }
        }
    }
}

static void sme_trace_line_port(sme_trace_module_id id, sme_trace_level level, const char* const format, va_list args)
{

    char timestr[64];
    sme_trace_time(64, timestr);

    printf("%s%s, %s, %s, ", level_to_color(level), timestr, sme_trace_module_to_str(id), level_to_str(level));
    (void)vprintf(format, args); /*lint !e64 */
    printf("%s\n", vt100Cancel);
    va_end(args);
    (void)fflush(stdout);
}

static void sme_trace_hex_port(sme_trace_module_id id, sme_trace_level level, const char* const message, const void* address, CsrUint32 length)
{
    char timestr[64];
    CsrUint32 i = 0;
    CsrUint8* bytes;
#define TR_MAX_DUMP_LENGTH 1024
    CsrUint32 safelength = length;

    sme_trace_time(64, timestr);

    printf("%s%s, %s, %s, %s\n", level_to_color(level), timestr, sme_trace_module_to_str(id), level_to_str(level), message);

    if (length > TR_MAX_DUMP_LENGTH)
    {
        safelength = TR_MAX_DUMP_LENGTH;
        printf("Printing only first %ld bytes of %ld requested.\n", safelength,
                length);
    }

    bytes = (CsrUint8*)address;
    for (; i < safelength; i++)
    {
        printf("%.2X ", *bytes);
        bytes++;
        if (i % 24== 23)
        {
            printf("\n");
        }
    }
    printf("%s\n", vt100Cancel);
    (void)fflush(stdout);
}

void sme_trace_msc_fn(sme_trace_module_id id, const char* const format, ...)
{
    va_list args;
    if (id >= TR_LAST_MODULE_ID) return;
    if (modulecfg[id].level > TR_LVL_INFO) return;
    va_start(args, format);
    sme_trace_line_port(id, TR_LVL_INFO, format, args);
}

void sme_trace_entry_fn(sme_trace_module_id id, const char* const format, ...)
{
    va_list args;
    if (id >= TR_LAST_MODULE_ID) return;
    if (modulecfg[id].level > TR_LVL_ENTRY) return;
    va_start(args, format);
    sme_trace_line_port(id, TR_LVL_ENTRY, format, args);
}

void sme_trace_debug_fn(sme_trace_module_id id, const char* const format, ...)
{
    va_list args;
    if (id >= TR_LAST_MODULE_ID) return;
    if (modulecfg[id].level > TR_LVL_DEBUG) return;
    va_start(args, format);
    sme_trace_line_port(id, TR_LVL_DEBUG, format, args);
}

void sme_trace_info_fn(sme_trace_module_id id, const char* const format, ...)
{
    va_list args;
    if (id >= TR_LAST_MODULE_ID) return;
    if (modulecfg[id].level > TR_LVL_INFO) return;
    va_start(args, format);
    sme_trace_line_port(id, TR_LVL_INFO, format, args);
}

void sme_trace_warn_fn(sme_trace_module_id id, const char* const format, ...)
{
    va_list args;
    if (id >= TR_LAST_MODULE_ID) return;
    if (modulecfg[id].level > TR_LVL_WARN) return;
    va_start(args, format);
    sme_trace_line_port(id, TR_LVL_WARN, format, args);
}

void sme_trace_error_fn(sme_trace_module_id id, const char* const format, ...)
{
    va_list args;
    if (id >= TR_LAST_MODULE_ID) return;
    if (modulecfg[id].level > TR_LVL_ERROR) return;
    va_start(args, format);
    sme_trace_line_port(id, TR_LVL_ERROR, format, args);
}

void sme_trace_crit_fn(sme_trace_module_id id, const char* const format, ...)
{
    va_list args;
    if (id >= TR_LAST_MODULE_ID) return;
    if (modulecfg[id].level > TR_LVL_CRIT) return;
    va_start(args, format);
    sme_trace_line_port(id, TR_LVL_CRIT, format, args);
}

void sme_trace_hex_fn(sme_trace_module_id id, sme_trace_level level, const char* const message, const void* address, CsrUint32 length)
{
    if (id >= TR_LAST_MODULE_ID) return;
    if (modulecfg[id].level > level) return;
    sme_trace_hex_port(id, level, message, address, length);
}

#endif

