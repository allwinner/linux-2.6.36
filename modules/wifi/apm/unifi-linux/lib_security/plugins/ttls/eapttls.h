/** @file eapttls.h
 *
 * TTLS functions.
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
 *   TTLS function.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/ttls/eapttls.h#1 $
 *
 ****************************************************************************/
#ifndef EAPTTLS_H
#define EAPTTLS_H

#include "plugins/security_method.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is the method interface for use by the EAP state machine. */
extern void TtlsEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method);


#ifdef __cplusplus
}
#endif
#endif /* EAPTTLS_H */
