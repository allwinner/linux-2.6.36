/** @file leap.h
 *
 * Definitions for LEAP method.
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
 *   This provides an implementation of a LEAP authentication method.
 *   LEAP (Light Extensible Authentication Protocol) is a Cisco-proprietary
 *   authentication protocol based around the EAP standard. It functions as
 *   an EAP plug-in and can sit alongside other plug-ins running under the
 *   same EAP state machine.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/leap/leap.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_LEAP_H
#define SECURITY_METHOD_LEAP_H

#include "plugins/security_method.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is the method interface for use by the EAP state machine. */
extern void LeapEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method);


#ifdef __cplusplus
}
#endif

#endif /* SECURITY_METHOD_LEAP_H */
