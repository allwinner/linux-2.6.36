/** @file security_method_null.h
 *
 * Definitions for a null EAP method.
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
 *   This provides an implementation of a null EAP authentication method.
 *   This is the default plug-in method supplied to the EAP state machine
 *   until a proper one has been negotiated with an Authentication Server (AS).
 *   This is also useful as a skeleton for adding more methods.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/null/security_method_null.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_NULL_H
#define SECURITY_METHOD_NULL_H

#include "plugins/security_method.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is the method interface for use by the EAP state machine. */
extern void NullEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_METHOD_NULL_H */
