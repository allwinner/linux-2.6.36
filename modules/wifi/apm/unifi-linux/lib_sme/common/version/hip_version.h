/** @file hip_version.h
 *
 *   Host Interface Protocol version supported by the SME
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
 *   This file declares the Host Interface Protocol (HIP) version against
 *   which this version of the SME has been developed, tested and released.
 *   It can be compared against the HIP versions declared by the driver and
 *   the firmware to determine likely compatibility
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/version/hip_version.h#1 $
 *
 ****************************************************************************/

#ifndef HIP_VERSION_H
#define HIP_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup version
 */

#define SME_SUPPORTED_HIP_VERSION   0x0800

/* Macros to help with access to a HIP version */
#define HIP_VERSION_MAJOR_NUMBER(num) ((num >> 8) & 0xff)
#define HIP_VERSION_MINOR_NUMBER(num) ((num     ) & 0xff)

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* HIP_VERSION_H */

