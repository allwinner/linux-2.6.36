/** @file trace_time.h
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
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/sme_trace/stdout/trace_time.h#1 $
 *
 ****************************************************************************/
#ifndef TRACE_TIME_H_
#define TRACE_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup sme_trace
 */
 /* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include <stddef.h>

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   Obtains the time of day in DD/MM/YYYY-HH:MM:SS format
 *
 * @par Description
 *   This function is only used by the trace (debugging) software. For targets
 *   that have the necessary facilities, a null-terminated string containing
 *   the current time of day in DD/MM/YYYY-HH:MM:SS format is returned.
 * @par
 *   For targets that lack the necessary facilities, any suitable
 *   null-terminated string of less than buffersize characters can be returned.
 *
 * @param[in]   buffersize: The size of the buffer the function can write into
 * @param[out]  outTime   : The buffer this function writes the time into
 *
 * @return
 *   void
 *
 * @pre  buffersize >= 20
 * @pre  outTime must point to a valid area of memory of size greater or equal
 *       to that specified in buffersize.
 *
 * @post The buffer will be populated with the time of day and date and will
 *       not overflow the buffer. If the buffer is of insufficient size, the
 *       date and time string will be truncated.
 */
void sme_trace_time(size_t buffersize, char* outTime);

/** @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /*TRACE_TIME_H_*/
