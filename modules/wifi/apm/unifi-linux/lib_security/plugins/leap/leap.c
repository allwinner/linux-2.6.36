/** @file leap.c
 *
 * Implementation of LEAP method.
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
 *   This provides an implementation of the LEAP authentication protocol.
 *   LEAP (Light Extensible Authentication Protocol) is a Cisco-proprietary
 *   authentication protocol based around the EAP standard. It functions as
 *   an EAP plug-in and can sit alongside other plug-ins running under the
 *   same EAP state machine.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/leap/leap.c#1 $
 *
 ****************************************************************************/

#include "csr_types.h"
#include "csr_pmalloc.h"
#include "leap.h"
#include "sme_trace/sme_trace.h"
#include "csr_crypto.h"

/* PRIVATE MACROS ***********************************************************/

#define CHALLENGE_LENGTH           8
#define CHALLENGE_RESPONSE_LENGTH 24

/* The length of the output from an MD4/MD5 hash operation */
#define MD_OUT 16

/* PRIVATE MACROS ***********************************************************/

/* All LEAP packets follow this structure */
typedef struct leap_packet
{
    /* LEAP header (length defined below) */
    CsrUint8 version;
    CsrUint8 not_used;
    CsrUint8 length;

    /* Variable content */
    union
    {
        struct challenge
        {
            CsrUint8 plaintext[CHALLENGE_LENGTH];
            CsrUint8 username; /* variable length data */
        } challenge;
        struct response
        {
            CsrUint8 cyphertext[CHALLENGE_RESPONSE_LENGTH];
            CsrUint8 username; /* variable length data */
        } response;
    }u;
} leap_packet;

#define LEAP_HEADER_LENGTH (3)

typedef enum leap_state
{
    LEAP_INIT,               /* awaiting first challenge */
    LEAP_CHALLENGED,         /* awaiting reply to our challenge response */
    LEAP_CHALLENGING,        /* awaiting reply to our challenge back to the AS */
    LEAP_WAIT_FOR_KEY_1,     /* waiting for the first EAPOL-Key message */
    LEAP_WAIT_FOR_KEY_2      /* waiting for the second EAPOL-Key message */
} leap_state;

/* This is the control structure for the LEAP state machine. */
typedef struct LeapContext
{
    CsrWifiSecurityContext* context;

    leap_state state;

    /* Temporary place for storing key information to pass back up to the EAP
     * state machine. Used for both session and group keys. */
    eapKey key;

    /* Need to store the challenges, responses and password hashes for
     * key derivation. */
    /* Note: The order of storage here is important. These need to be
     * consecutive so that we can use them as a combined buffer for
     * calculating the session key. */
    CsrUint8 pwd_hash[MD_OUT];
    CsrUint8 network_challenge[CHALLENGE_LENGTH];
    CsrUint8 network_challenge_resp[CHALLENGE_RESPONSE_LENGTH];
    CsrUint8 peer_challenge[CHALLENGE_LENGTH];
    CsrUint8 peer_challenge_resp[CHALLENGE_RESPONSE_LENGTH];

    /* We need to actually store the session key data (the other (group) key is
     * stored for us in an incoming EAPOL-Key message). */
    CsrUint8 session_key[MD_OUT];
} LeapContext;

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief Processes a a LEAP challenge from an Authentication Server.
 *
 * @par Description
 *   The AS has challenged us with some data. We need to DES encrypt it using
 *   profile data as a key.
 *
 * @param[in] packet : MAC EAPOL packet containing a LEAP challenge.
 *
 * @return
 *   TRUE = challenge processsed ok, FALSE = something wrong.
 */
static CsrBool process_leap_challenge(eapMethod* context, eap_packet *packet)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
/*  This array is nice to have for debugging. It corresponds to the example
 *  challenge in the LEAP specification. See LEAP spec for corresponding
 *  password and expected results.
 *  CsrUint8 fixed_challenge[8] = {0x5e, 0x2a, 0x84, 0x2a, 0xa6, 0x4b, 0xdd, 0x27};
 */
    sme_trace_info((TR_SECURITY_LIB, "process_leap_challenge"));

    /* We need to store the challenge text for later key derivation. */
    CsrMemCpy(leapContext->peer_challenge,
           ((leap_packet *)(&packet->u.resp.u.data))->u.challenge.plaintext,
           CHALLENGE_LENGTH);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "peer_challenge", leapContext->peer_challenge, CHALLENGE_LENGTH));
    CsrCryptoCallMschapNtPasswordHash(leapContext->context->cryptoContext, NULL,
                                      (CsrUint8*)leapContext->context->setupData.password,
                                      leapContext->pwd_hash);

    CsrCryptoCallMschapChallengeResponse(leapContext->context->cryptoContext, NULL,
                                         leapContext->peer_challenge, leapContext->pwd_hash,
                                         leapContext->peer_challenge_resp);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "peer_challenge_resp", leapContext->peer_challenge_resp, CHALLENGE_RESPONSE_LENGTH));

    return TRUE;
}

/**
 * @brief Assemble a challenge response message to send to the AP.
 *
 * @par Description
 *   This function assembles a response to the AS challenge from previously
 *   prepared data.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
static DataBuffer build_leap_challenge_response(eapMethod* context, CsrUint8 reqId)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
    eapol_packet *eapol = (eapol_packet *)leapContext->context->buffer;
    CsrUint16 length;
    DataBuffer dataref = {CSR_WIFI_SECURITY_SNAP_LENGTH
                          + EAPOL_HEADER_LENGTH
                          + EAP_REQ_RESP_HEADER_LENGTH
                          + LEAP_HEADER_LENGTH
                          + CHALLENGE_RESPONSE_LENGTH,
                          NULL};
    dataref.buf = (CsrUint8 *)eapol;

    dataref.dataLength += (CsrUint16)CsrStrLen(leapContext->context->setupData.username);
    /* 0-7 = space for the LLC SNAP header */

    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    length = EAP_REQ_RESP_HEADER_LENGTH + LEAP_HEADER_LENGTH + CHALLENGE_RESPONSE_LENGTH
             + (CsrUint16)CsrStrLen(leapContext->context->setupData.username);
    EAPOL_LENGTH_ASSIGN(eapol, length);


    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    eapol->u.eap.u.resp.type = EAP_TYPE_LEAP;
    ((leap_packet *)(&eapol->u.eap.u.resp.u.data))->version = 1;
    ((leap_packet *)(&eapol->u.eap.u.resp.u.data))->not_used = 0;
    ((leap_packet *)(&eapol->u.eap.u.resp.u.data))->length = CHALLENGE_RESPONSE_LENGTH;

    CsrMemCpy(((leap_packet *)(&eapol->u.eap.u.resp.u.data))->u.response.cyphertext,
           leapContext->peer_challenge_resp,
           CHALLENGE_RESPONSE_LENGTH);

    CsrMemCpy(&((leap_packet *)(&eapol->u.eap.u.resp.u.data))->u.response.username,
               leapContext->context->setupData.username,
               CsrStrLen(leapContext->context->setupData.username));

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "build_leap_challenge_response", dataref.buf, dataref.dataLength));
    sme_trace_info((TR_SECURITY_LIB, "dataref.dataLength = %d", dataref.dataLength ));
    sme_trace_info((TR_SECURITY_LIB, "dataref.buf = %p", dataref.buf ));

    return dataref;
}

/**
 * @brief Process a LEAP challenge success response from an Access Point.
 *
 * @par Description
 *   LEAP dictates that we respond to a success from the Access Point with a
 *   challenge of our own (mutual authentication). This function prepares some
 *   challenge data and calculates the expected response.
 */
static void process_leap_challenge_success(eapMethod* context)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
/*    Ip8021x_pkt* out_pkt;
      leap_challenge *challenge; */
    CsrUint8 pwd_hash_hash[MD_OUT];

    /* Generate random challenge */
    CsrWifiSecurityRandom(leapContext->network_challenge, CHALLENGE_LENGTH);

    /* Rehash the password as per LEAP requirements */
    CsrCryptoCallMd4(leapContext->context->cryptoContext, NULL,
                     leapContext->pwd_hash, MD_OUT, pwd_hash_hash);
    CsrMemCpy(leapContext->pwd_hash, pwd_hash_hash, MD_OUT);

    /* Calculate expected response to random challenge so we can compare it
     * later when the AS responds. */

    CsrCryptoCallMschapChallengeResponse(leapContext->context->cryptoContext, NULL,
                                         leapContext->network_challenge, leapContext->pwd_hash,
                                         leapContext->network_challenge_resp);
}

/**
 * @brief Assemble a network challenge message to send to the AS.
 *
 * @par Description
 *   This function assembles a challenge to the AS from previously assembled
 *   data.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet.
 */
static DataBuffer build_leap_network_challenge(eapMethod* context, CsrUint8 reqId)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
    eapol_packet *eapol = (eapol_packet *)leapContext->context->buffer;
    CsrUint16 length;
    DataBuffer dataref = {CSR_WIFI_SECURITY_SNAP_LENGTH
                          + EAPOL_HEADER_LENGTH
                          + EAP_REQ_RESP_HEADER_LENGTH
                          + LEAP_HEADER_LENGTH
                          + CHALLENGE_LENGTH,
                          NULL};

    dataref.buf = (CsrUint8 *)eapol;
    dataref.dataLength += (CsrUint16)CsrStrLen(leapContext->context->setupData.username);
    /* 0-7 = space for the LLC SNAP header */

    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    length = EAP_REQ_RESP_HEADER_LENGTH + LEAP_HEADER_LENGTH + CHALLENGE_LENGTH
             + (CsrUint16)CsrStrLen(leapContext->context->setupData.username);
    EAPOL_LENGTH_ASSIGN(eapol, length);

    eapol->u.eap.code = EAP_CODE_REQ;
    eapol->u.eap.id = reqId + 1;
    eapol->u.eap.eap_length_hi  = eapol->eapol_length_hi;
    eapol->u.eap.eap_length_low = eapol->eapol_length_low;

    eapol->u.eap.u.resp.type = EAP_TYPE_LEAP;
    ((leap_packet *)(&eapol->u.eap.u.resp.u.data))->version = 1;
    ((leap_packet *)(&eapol->u.eap.u.resp.u.data))->not_used = 0;
    ((leap_packet *)(&eapol->u.eap.u.resp.u.data))->length = CHALLENGE_LENGTH;

    CsrMemCpy(((leap_packet *)(&eapol->u.eap.u.resp.u.data))->u.challenge.plaintext,
           leapContext->network_challenge,
           CHALLENGE_LENGTH);

    CsrMemCpy(&((leap_packet *)(&eapol->u.eap.u.resp.u.data))->u.challenge.username,
               leapContext->context->setupData.username,
               CsrStrLen(leapContext->context->setupData.username));

    return dataref;
}

/**
 * @brief Check the challenge response from the Authentication Server.
 *
 * @par Description
 *   The AS should have DES encoded our challenge and sent us the correct
 *   cyphertext back. Check it for validity.
 *
 * @param[in] packet : the EAP response from the AS containing a LEAP challenge response.
 *
 * @return
 *   TRUE = response checks out ok, FALSE = response wrong.
 */
static CsrBool check_as_response(eapMethod* context, eap_packet *eap)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
    sme_trace_entry((TR_SECURITY_LIB, "process_as_response"));

    if (CsrMemCmp(leapContext->network_challenge_resp,
               ((leap_packet *)(&eap->u.resp.u.data))->u.response.cyphertext,
               CHALLENGE_RESPONSE_LENGTH)
        == 0)
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief process an incoming EAPOL-Key message.
 *
 * @par Description
 *   Under LEAP, EAPOL-Key message processing depends upon whether we have a
 *   session key or a group (multicast) key message.
 *   A group session key is simply extracted from the incoming message.
 *   A session key needs to be calculated from the contents of previous
 *   challenge messages. The actual session EAPOL-Key message contains no key
 *   and only acts as a trigger to enable the session key.
 *
 * @param[in]   key : an EAPOL-Key message received from the AP.
 */
static void processEapolKey(eapMethod* context, eapol_key_leap *key)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
    CsrUint8 css[4] = {0, 0x0f, 0xac, 4};

    CsrMemCpy(&leapContext->key.css, css, 4);
    leapContext->key.index = key->flag_and_index & 0x7f;

    if (key->flag_and_index & 0x80)
    {
        /* Calculate session key */
        CsrCryptoCallMd5(leapContext->context->cryptoContext, NULL,
                         leapContext->pwd_hash,
                         MD_OUT + CHALLENGE_LENGTH * 2 + CHALLENGE_RESPONSE_LENGTH * 2,
                         leapContext->session_key);
        CsrMemCpy(leapContext->key.keydata, leapContext->session_key, MD_OUT);
        leapContext->key.length = 104/8; /* 104 bit WEP */
        sme_trace_info((TR_SECURITY_LIB, "Installing KeyType Pairwise"));
        /* FixMe Pass index etc ? */
        (leapContext->context->callbacks.installPairwiseKey)(leapContext->context->externalContext,
                                                             CSR_WIFI_SECURITY_KEYTYPE_LEAPWEP,
                                                             leapContext->key.keydata,
                                                             leapContext->key.length,
                                                             NULL,
                                                             1);
    }
    else
    {
        CsrUint16 keyLength = KEY_CCX_LENGTH_FIELD(key);

        sme_trace_info((TR_SECURITY_LIB, "processEapolKey() :: keyLength = %d", keyLength));
        if(keyLength > 104/8) keyLength = 104/8;
        CsrMemCpy(leapContext->key.keydata, &key->keydata, keyLength); /*lint !e670*/
        leapContext->key.length = keyLength;
        (leapContext->context->callbacks.installGroupKey)(leapContext->context->externalContext,
                                                          CSR_WIFI_SECURITY_KEYTYPE_LEAPWEP,
                                                          leapContext->key.keydata,
                                                          leapContext->key.length,
                                                          NULL,
                                                          1);
    }
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/**
 * @brief Check an incoming EAP meassage from the AP.
 *
 * @par Description
 *   This function checks an incoming EAP message for validity and relevance to
 *   LEAP.
 *
 * @param[in]   eapReqData : the EAP message received from the AP.
 * @param[in]   size : the size of the incoming packet. Maybe be larger than the actual EAP message.
 *
 * @return
 *   TRUE - LEAP message ok, FALSE - bad or irrelevant message.
 */
static CsrBool LeapCheck(eapMethod* context, eap_packet *eapReqData, CsrUint16 size)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
    sme_trace_info((TR_SECURITY_LIB, "LeapCheck"));

    /* EAP state machine assumes key messages are normal responses,
     * but under EAPOL they aren't. */
    if (context->isEapolKeyHint == FALSE)
    {
        if ((eapReqData->code == EAP_CODE_REQ) || (eapReqData->code == EAP_CODE_RESP))
        {
            if ( ((leap_packet *)(&eapReqData->u.resp.u.data))->version != 1 )
                return FALSE;
        }
    }

    switch(leapContext->state)
    {
    case LEAP_INIT:
        sme_trace_info((TR_SECURITY_LIB, "leapContext->state = LEAP_INIT"));
        if ( (size < LEAP_HEADER_LENGTH + CHALLENGE_LENGTH) ||
             (context->isEapolKeyHint == TRUE) )
             return FALSE;
        break;
    case LEAP_CHALLENGED:
        sme_trace_info((TR_SECURITY_LIB, "leapContext->state = LEAP_CHALLENGED"));
        /* We don't expect a LEAP packet in this state, just an EAP success. */
        if ( (size < EAPOL_HEADER_LENGTH)||
             (context->isEapolKeyHint == TRUE) )
            return FALSE;
        if (EAP_LENGTH_FIELD(*eapReqData) != EAPOL_HEADER_LENGTH)
            return FALSE;
        if (eapReqData->code != EAP_CODE_SUCCESS)
            return FALSE;
        break;
    case LEAP_CHALLENGING:
        sme_trace_info((TR_SECURITY_LIB, "leapContext->state = LEAP_CHALLENGING"));
        if ( (size < CHALLENGE_RESPONSE_LENGTH + 3/*leap header*/ + 5/*eap header*/) ||
             (context->isEapolKeyHint == TRUE) )
             return FALSE;
        break;
    case LEAP_WAIT_FOR_KEY_1:
        sme_trace_info((TR_SECURITY_LIB, "leapContext->state = LEAP_WAIT_FOR_KEY_1"));
        /* HERE - more sanity checks on the key message. */
        break;
    case LEAP_WAIT_FOR_KEY_2:
        sme_trace_info((TR_SECURITY_LIB, "leapContext->state = LEAP_WAIT_FOR_KEY_2"));
        /* HERE - more sanity checks on the key message. */
        break;
    }

    return TRUE;
}

/**
 * @brief Process a recieved message from the AP.
 *
 * @pre LeapCheck() has been called to ensure the message's validity.
 *
 * @post processing has been done ready for LeapBuild() to assemble a response.
 *
 * @param[in]   eapReqData : the EAP packet arrived from the AP.
 */
static void LeapProcess(eapMethod* context, eap_packet *eapReqData, CsrUint8 reqId)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
    sme_trace_info((TR_SECURITY_LIB, "LeapProcess"));
    context->isKeyAvailable = FALSE;

    /* No integrity check needed here, as LeapCheck() has already been called. */
    switch(leapContext->state)
    {
    case LEAP_INIT:
        context->decision = FAIL;
        if (process_leap_challenge(context, eapReqData))
        {
            context->methodState = CONT;
        }
        else
        {
            /* Something went wrong. Just fail the authentication. */
            context->methodState = DONE;
        }
        break;
    case LEAP_CHALLENGED:
        process_leap_challenge_success(context);

        /* If we get a success or failure response to this challenge we want to
         * fail because we are looking for a challenge response. */
        context->decision = FAIL;

        context->methodState = CONT;
        break;
    case LEAP_CHALLENGING:
        if (check_as_response(context, eapReqData))
        {
            /* Checked out ok. */
            context->methodState = CONT;
        }
        else
        {
            /* AS response invalid. Fail authentication. */
            context->decision = FAIL;
            context->methodState = DONE;
        }
        break;
    case LEAP_WAIT_FOR_KEY_1:
        processEapolKey(context, (eapol_key_leap *)eapReqData);
        context->methodState = CONT;
        break;
    case LEAP_WAIT_FOR_KEY_2:
        processEapolKey(context, (eapol_key_leap *)eapReqData);
        context->decision = UNCOND_SUCC;
        context->methodState = DONE;
        break;
    }/* switch */
}

/**
 * @brief Assemble a message, if necessary, to send to the AP.
 *
 * @par Description
 *   This function's name contains "Resp" as a reference to RFC:4137 (EAP
 *   state machines) which assumes anything a supplicant sends out is a
 *   response to a request from an AP. This isn't strictly true and this
 *   function may assemble a response, a request, or nothing depending upon
 *   the current LEAP state.
 *   A message to send to the AP is assembled here. Not actually sent.
 *   This function, being the last function called to process the state
 *   (as per EAP check/process/build/send rules) also updates the state ready
 *   for the next message.
 *
 * @param[in]   reqId : the EAP id of the last message received from the AP.
 *
 * @return
 *   a packet manager reference to a queued packet (if length = zero then
 *   no packet).
 */
static DataBuffer LeapBuildResp(eapMethod* context, CsrUint8 reqId)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
    DataBuffer ret = {0, NULL};

    sme_trace_info((TR_SECURITY_LIB, "LeapBuildResp"));

    switch(leapContext->state)
    {
    case LEAP_INIT:
        ret = build_leap_challenge_response(context, reqId);
        leapContext->state = LEAP_CHALLENGED;
        break;
    case LEAP_CHALLENGED:
        ret = build_leap_network_challenge(context, reqId);
        leapContext->state = LEAP_CHALLENGING;
        break;
    case LEAP_CHALLENGING:
        /* we don't need to send a response in this state */
        leapContext->state = LEAP_WAIT_FOR_KEY_1;
        break;
    case LEAP_WAIT_FOR_KEY_1:
        /* we don't need to send a response in this state */
        leapContext->state = LEAP_WAIT_FOR_KEY_2;
        break;
    case LEAP_WAIT_FOR_KEY_2:
        /* we don't need to send a response in this state */
        leapContext->state = LEAP_INIT;
        break;
    }

    return ret;
}

/**
 * @brief Return a key to the caller.
 *
 * @par Description
 *   This function returns the key associated with the last EAPOL-Key
 *   message. In the case of the session key, this will have been calculated
 *   by us and triggered by a minimal EAPOL-Key message. In the case of the
 *   multicast key, this will have been put out on air by the AP.
 *
 * @pre
 *   The caller must check isKeyAvailable before calling this function.
 *   isKeyAvailable must be TRUE or the return value is undefined.
 *
 * @return
 *   session or group key - whichever was most recently obtained.
 */
static eapKey *LeapGetKey(eapMethod* context)
{
    LeapContext* leapContext = (LeapContext*)context->methodContext;
    return &leapContext->key;
}

static void LeapDeinit(eapMethod* context)
{
    CsrPfree(context->methodContext);
    context->methodContext = NULL;
}

/* Our one and only LEAP plug-in method.
 * This is the method interface for use by the EAP state machine. */
static const eapMethod leap_method =
{
    NULL,  /* Pointer to local Context */
    NONE,  /* state */
    FAIL,  /* decision */
    FALSE, /* allow notifications */
    FALSE, /* isKeyAvaiable */
    FALSE, /* isEapolKeyHint */
    LeapDeinit,
    LeapCheck,
    LeapProcess,
    LeapBuildResp,
    LeapGetKey
};


/**
 * @brief Initialize the NULL method state machine.
 */
void LeapEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method)
{
    LeapContext* leapContext = (LeapContext*) CsrPmalloc(sizeof(LeapContext));

    CsrMemCpy(method, &leap_method, sizeof(leap_method));
    method->methodContext = leapContext;

    leapContext->context = context;
    leapContext->state = LEAP_INIT;
}


