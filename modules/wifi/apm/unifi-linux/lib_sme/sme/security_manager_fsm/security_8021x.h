/** @file security_8021x.h
 *
 * 801x abstraction API header
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
 *   Provides abstraction API for 801x security subsystem.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/security_manager_fsm/security_8021x.h#3 $
 *
 ****************************************************************************/
#ifndef SECURITY_8021X_H
#define SECURITY_8021X_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/csr_wifi_fsm.h"
#include "ie_access/ie_access.h"

/* PUBLIC MACROS ************************************************************/
/* WPA Cipher Suites */
#define IE_WPA_CIPHER_WEP40   (0x0050F201UL)
#define IE_WPA_CIPHER_TKIP    (0x0050F202UL)
#define IE_WPA_CIPHER_CCMP    (0x0050F204UL)
#define IE_WPA_CIPHER_WEP104  (0x0050F205UL)

/* WPA AKM Suites */
#define IE_WPA_AKM_PSKSA      (0x0050F201UL)
#define IE_WPA_AKM_PSK        (0x0050F202UL)


/* WPA2 Cipher Suites */
#define IE_WPA2_CIPHER_WEP40  (0x000FAC01UL)
#define IE_WPA2_CIPHER_TKIP   (0x000FAC02UL)
#define IE_WPA2_CIPHER_CCMP   (0x000FAC04UL)
#define IE_WPA2_CIPHER_WEP104 (0x000FAC05UL)

/* WPA2 AKM Suites */
#define IE_WPA2_AKM_PSKSA           (0x000FAC01UL)
#define IE_WPA2_AKM_PSK             (0x000FAC02UL)
#define IE_WPA2_AKM_PSKSA_FT        (0x000FAC03UL)
#define IE_WPA2_AKM_PSK_FT          (0x000FAC04UL)
#define IE_WPA2_AKM_PSKSA_SHA256    (0x000FAC05UL)
#define IE_WPA2_AKM_PSK_SHA256      (0x000FAC06UL)


/* WAPI Cipher Suites */
#define IE_WAPI_CIPHER_SMS4   (0x00147201UL)

/* WAPI AKM Suites */
#define IE_WAPI_AKM_PSKSA     (0x00147201UL)
#define IE_WAPI_AKM_PSK       (0x00147202UL)

/* PUBLIC TYPES DEFINITIONS *************************************************/


/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_8021X_H */

