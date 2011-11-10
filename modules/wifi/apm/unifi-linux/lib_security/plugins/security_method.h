/** @file security_method.h
 *
 * Definitions for the EAP state machine plug-in interface.
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
 *   This file defines the interface between the EAP state machine and its
 *   plug-in methods. This is almost entirely governed by RFC:4137.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/security_method.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_H
#define SECURITY_METHOD_H

#include "csr_security_private.h"
#include "csr_eap/csr_eap.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Each plug-in method exposes the following states to the EAP state machine. */
typedef enum
{
    NONE,
    INIT,
    CONT,
    MAY_CONT,
    DONE
} eap_method_state;

/* What EAP machine terminal state to go into (success or fail) if the
 * authentication is concluded now (either my us or by the AS). */
typedef enum
{
    FAIL,       /* FAIL, or FAIL if we get a SUCCESS/FAILURE from AS */
    COND_SUCC,  /* succeed authentication only if we get a SUCCESS from the AS */
    UNCOND_SUCC /* succeed authentication, even if we get a FAILURE from AS */
} eap_decision;

typedef enum
{
    KeyType_Pairwise,
    KeyType_Group,
    KeyType_PAC,
    KeyType_Session
}KeyType;

typedef CsrInt32 CipherSuiteSelector;

/* A structure for holding all the relevant keying data. */
typedef struct eapKey
{
    CsrUint32 length; /* length in bytes */
    KeyType keytype;
    CipherSuiteSelector css;
    CsrUint8 keydata[128];
    CsrUint8 index;
} eapKey;

/* This structure defines the interface between the EAP state machine and a
 * plug-in method. Every method has only one object of this type. */
typedef struct eapMethod
{
    void*            methodContext;

    eap_method_state methodState;
    eap_decision     decision;
    CsrBool          allowNotifications;
    CsrBool          isKeyAvailable;

    /* This is a hint from the EAP state machine to the method to indicate that
     * the current request is an EAPOL-KEY message (additional to RFC:4137,
     * which doesn't differentiate between EAP requests and key messages).
     */
    CsrBool          isEapolKeyHint;

    /**
    * @brief Cleanup the method.
    */
    void (*deinit)(struct eapMethod* context);


/**
 * @brief Check the message from the Authentication Server for validity.
 *
 * @par Description
 *   Checks the message received for validity given the state of the method.
 *
 * @param[in] packet : the EAP packet from the AS.
 *
 * @return
 *   TRUE  = response checks out such that we need to further process it,
 *   FALSE = response should be discarded without further processing.
 */
    CsrBool (*check)(struct eapMethod* context, eap_packet *eapReqData, CsrUint16 size);

/**
 * @brief Parse and processing an incoming message for the method.
 *
 * @par Description
 *   Does any necessary processing depending on the contents of the message and
 *   the state of the method. Does NOT formulate a response packet, but may
 *   assemble data ready for a response later.
 *
 * @param[in] packet : the EAP packet from the AP/AS.
 */
    void    (*process)(struct eapMethod* context, eap_packet *eapReqData, CsrUint8 reqId);

/**
 * @brief Create a response to send to the AP (if necessary).
 *
 * @par Description
 *   This function doesn't actually send the message, just creates it.
 *   The method should allocate enough space for an entire EAPOL packet
 *   including LLC SNAP header. The EAP state machine will fill out the SNAP
 *   header, but the method must make sure sufficient space is allocated and
 *   fill out everything else, including eapol header fields.
 *
 * @param[in] reqId : the EAP request ID in the last message from the AP.
 *
 * @return
 *      The queued EAPOL packet ready to be sent to the AP. If zero length then
 *      there is no packet.
 */
     DataBuffer (*buildResp)(struct eapMethod* context, CsrUint8 reqId);

/**
 * @brief Pass back a key ready for use.
 *
 * @pre check to see if isKeyAvailable is set or the return value is undefined.
 *
 * @return
 *      A key to be installed into the firmware.
 */
    eapKey * (*getKey)(struct eapMethod* context);

} eapMethod;


#ifdef __cplusplus
}
#endif

#endif /*SECURITY_METHOD_H*/
