/** @file osa.h
 *
 * Operating System Abstraction main header file
 *
 * @section LEGAL
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Provides abstraction API for typical OS-related functions such as
 *   malloc, free, etc.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/abstractions/osa.h#1 $
 *
 ****************************************************************************/
#ifndef OSA_H
#   define OSA_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup abstractions
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"
#include "csr_util.h"
#include "csr_pmalloc.h"
#include "csr_panic.h"
#include "csr_sched.h"
#ifdef FSM_MUTEX_ENABLE
#include "csr_framework_ext.h"
#endif
/* PUBLIC MACROS ************************************************************/

/** @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* OSA_H */
