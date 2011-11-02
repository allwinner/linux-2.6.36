/** @file trace_time.c
 *
 * Time-of-day utility for trace and MSC trace functions
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
 *   Provides a mechanism for obtaining the date and time in string format
 *   for the MSC trace and sme trace functions.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/sme_trace/stdout/trace_time.c#1 $
 *
 ****************************************************************************/

/** @{
 * @ingroup sme_trace
 */

/* STANDARD INCLUDES ********************************************************/
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/stdout/trace_time.h"

/* MACROS *******************************************************************/
/* GLOBAL VARIABLE DEFINITIONS **********************************************/
/* PRIVATE TYPES DEFINITIONS ************************************************/
/* PRIVATE CONSTANT DEFINITIONS *********************************************/
/* PRIVATE VARIABLE DEFINITIONS *********************************************/
/* PRIVATE FUNCTION DEFINITIONS *********************************************/
/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/* --------------------------------------------------------------------------
 * Place the time and date in string format into the supplied buffer
 */
void sme_trace_time(size_t buffersize, char* outTime)
{
    struct timeval  tv;
    struct tm   tnow;

    if (gettimeofday(&tv, NULL) < 0)
    {
        return;
    }

    CsrMemSet(outTime, 0, buffersize);
    /*
     * localtime_r is not ANSI compliant but as this is part of the a
     * target-specific directory we can get away with using it (we stick to
     * ANSI to aid with porting - this code is non-portable anyway).
     */
    /*lint --e(58,718,746) */
    if (localtime_r(&tv.tv_sec, &tnow) == NULL) /*lint --e(58,718,746) */
        return;

    (void)strftime(outTime, buffersize, "%Y/%m/%d-%H:%M:%S", &tnow);
    sprintf(&outTime[strlen(outTime)], ".%.3d", (int)(tv.tv_usec/1000));
    /*(void)strftime(outTime, buffersize, "%d/%m/%Y-%H:%M:%S.%.3d", &tnow, tv.tv_usec/1000);*/
}
