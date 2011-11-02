/** @file eaptls.h
 *
 * Definitions for a EAP-TLS method.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   This provides an implementation of a TLS authentication method.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/tls/eaptls.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_EAPTLS_H
#define SECURITY_METHOD_EAPTLS_H

#include "plugins/security_method.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is the method interface for use by the EAP state machine. */
extern void TlsEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_METHOD_EAPTLS_H */
