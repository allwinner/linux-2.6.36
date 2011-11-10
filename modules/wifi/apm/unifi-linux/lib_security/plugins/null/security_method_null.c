/** @file security_method_null.c
 *
 * Implementation of a null EAP method.
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
 *   This provides an implementation of a null EAP authentiacation method.
 *   This is the default plug-in method supplied to the EAP state machine
 *   until a proper one has been negotiated with an Authentication Server (AS).
 *   This is also useful as a skeleton for adding more methods.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/null/security_method_null.c#1 $
 *
 ****************************************************************************/

#include "security_method_null.h"

static void NullDeinit(eapMethod* context)
{
}

/**
 * @brief Check the validity of an incoming EAP message.
 *
 * @param[in]   eapReqData : the EAP message received from the AP.
 * @param[in]   size : the size of the incoming packet. Maybe be larger than the actual EAP message.
 *
 * @return
 *   TRUE - message ok, FALSE - bad or irrelevant message.
 */
static CsrBool NullCheck(eapMethod* context, eap_packet *eapReqData, CsrUint16 size)
{
    return FALSE;
}

/**
 * @brief Process a message from the AP.
 *
 * @pre NullCheck() has been called to ensure the message's validity.
 *
 * @post processing has been done ready for NullBuild() to assemble a response.
 *
 * @param[in]   eapReqData : the EAP message received from the AP.
 */
static void NullProcess(eapMethod* context, eap_packet *eapReqData, CsrUint8 reqId)
{
}

/**
 * @brief Assemble a message, if necessary, to send to the AP.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet (if length = zero then
 *   no packet).
 */
static DataBuffer NullBuildResp(eapMethod* context, CsrUint8 reqId)
{
    DataBuffer ret = {0,NULL};
    return ret;
}

/**
 * @brief Return a key to the caller.
 *
 * @pre
 *   The caller must check isKeyAvailable before calling this function.
 *   isKeyAvailable must be TRUE or the return value is undefined.
 *
 * @return encryption key.
 */
static eapKey* NullGetKey(eapMethod* context)
{
    return NULL;
}

/* Our one and only NULL plug-in method.
 * This is the method interface for use by the EAP state machine. */
static const eapMethod null_method =
{
    NULL,
    INIT,  /* state */
    FAIL,  /* decision */
    FALSE, /* allow notifications */
    FALSE, /* isKeyAvaiable */
    FALSE, /* isEapolKeyHint */
    NullDeinit,
    NullCheck,
    NullProcess,
    NullBuildResp,
    NullGetKey
};

/**
 * @brief Initialise the NULL method state machine.
 */
void NullEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method)
{
    CsrMemCpy(method, &null_method, sizeof(eapMethod));
}

