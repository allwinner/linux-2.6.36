
/** @file csr_handshake.c
 *
 * Implementation of the WPA Personal library
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
 *   This provides an implementation of WPA Personal library
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_handshake/csr_handshake.c#10 $
 *
 ****************************************************************************/

#include "csr_types.h"
#include "csr_pmalloc.h"
#include "csr_util.h"

#include "csr_handshake.h"
#include "csr_crypto.h"
#include "csr_sha1/csr_sha1.h"
#include "sme_trace/sme_trace.h"

/* PRIVATE CONSTANTS ********************************************************/

/* PRIVATE MACROS ***********************************************************/

#define CSR_WPA_PMK_LENGTH 32
#define CSR_WPA_NONCE_LENGTH 32
#define CSR_WPA_MAC_LENGTH 6
#define CSR_WPA_KEK_LENGTH 16
#define CSR_WPA_KCK_LENGTH 16
#define CSR_WPA_MIC_LENGTH 16
#define CSR_WPA_KEY_IV_LENGTH 16
#define CSR_WPA_KEY_RSC_LENGTH 8
#define CSR_WPA_KEY_RESERVED_LENGTH 8
#define CSR_WPA_KEY_REPLAY_COUNTER_LENGTH 8
#define CSR_WPA_MAX_EAPOL_LENGTH 256
#define CSR_WPA_MAX_KEY_DATA_LENGTH 128

#define CSR_WPA_CCMP_PTK_LENGTH 48
#define CSR_WPA_CCMP_TK_LENGTH 16
#define CSR_WPA_CCMP_GTK_LENGTH 16

#define CSR_WPA_TKIP_PTK_LENGTH 64
#define CSR_WPA_TKIP_TK_LENGTH 32
#define CSR_WPA_TKIP_GTK_LENGTH 32

#define CSR_WPA_MAX_PTK_LENGTH CSR_WPA_TKIP_PTK_LENGTH
#define CSR_WPA_MAX_TK_LENGTH CSR_WPA_TKIP_TK_LENGTH
#define CSR_WPA_MAX_GTK_LENGTH CSR_WPA_TKIP_GTK_LENGTH

#define CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4 1
#define CSR_WPA_KEY_DESCRIPTOR_SHA1_AES 2


#define KCK(ptk) (ptk)
#define KEK(ptk) (ptk + CSR_WPA_KCK_LENGTH)
#define TK(ptk)  (ptk + CSR_WPA_KCK_LENGTH + CSR_WPA_KEK_LENGTH)


/* PRIVATE TYPE DEFINITIONS ***********************************************************/
struct CsrWpaCryptoCookie
{
    struct CsrWpaPersonalCtx *wpaPersonalContext;
    struct eapol_packet *wpaEapolPacket;
    void  *externalContext;
};
typedef struct CsrWpaCryptoCookie CsrWpaCryptoCookie;


struct CsrWpaPersonalCtx
{
    CsrWifiSecurityContext* securityContext;

   CsrUint8 nonce[CSR_WPA_NONCE_LENGTH];

    CsrUint8 ptk[CSR_WPA_MAX_PTK_LENGTH];
    CsrUint8 gtk[CSR_WPA_MAX_GTK_LENGTH];      /* Group transient key */
    CsrUint16 gtkLength;
    CsrBool gtkAvailable;
    CsrUint8 keyReplayCounter[CSR_WPA_KEY_REPLAY_COUNTER_LENGTH];

    CsrCryptoCallbackContext wpaCryptoCallbackContext;  /* Call back context as expected by Crypto module */
    CsrWpaCryptoCookie wpaCryptoCookie;               /* WPA personal specific cookie that is stored
                                                           inside wpaCryptoCallbackContext */

    CsrUint8 keyInfo;

    CsrUint8 cryptoResultBuffer[CSR_WPA_MAX_KEY_DATA_LENGTH];

    /* Temporary variables stored across invocations of hmac_sha1 when processing M 1 */
    CsrUint32 hmacSha1Loop;
    CsrUint32 hmacSha1TotalLoops;
    CsrUint8 seed[128];
    CsrUint32 seedLength;

    /* Temporary variables stored across invocation to compute MIC when processing M 3 */
    CsrUint8 mic[CSR_WPA_MIC_LENGTH];

    /* Temporary variable to store RSC for group handshake */
    CsrUint8 rsc[CSR_WPA_KEY_RSC_LENGTH];

#ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
    /* Temporary variables stored across invocations when processing M 2 */
    CsrUint8 *pPlainKey;
#endif
};

/* PRIVATE VARIABLE DEFINITIONS ***********************************************************/

/******************* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Pack parameters into a CsrCryptoCallbackContext structure
 *
 * @par Description
 *   Prepare cookie that is sent to the crypto library
 *
 * @param
 *  CsrWpaPersonalCtx *: WPA handshake module Context
 *  eapol_packet*      : Received EAPOL Key
 *  void*              : Client context, context of the caller that will be used when making
 *                        call backs
 *  CryptoCallBack     : Function that needs to be called after crypto functionality is completed
 *  CsrUint8*             : Pointer where result of the crypto operation shall be stored
 *
 * @return
 *  CsrwpaCryptoCookie* : Prepared cookie
 */
CsrCryptoCallbackContext* prepare_crypto_context(CsrWpaPersonalCtx *pCtx, eapol_packet *eapolPacket, void *externalContext, CryptoCallBack cryptoCallBack, CsrUint8 *resultBuffer)
{
    CsrCryptoCallbackContext *wpaCryptoCallbackContext = &pCtx->wpaCryptoCallbackContext;
    CsrWpaCryptoCookie       *wpaCryptoCookie          = &pCtx->wpaCryptoCookie;

    wpaCryptoCookie->wpaEapolPacket     = eapolPacket;
    wpaCryptoCookie->wpaPersonalContext = pCtx;
    wpaCryptoCookie->externalContext    = externalContext;

    wpaCryptoCallbackContext->context         = wpaCryptoCookie;
    wpaCryptoCallbackContext->cryptoCallBack  = cryptoCallBack;
    wpaCryptoCallbackContext->resultBuffer   =  resultBuffer;

    return wpaCryptoCallbackContext;
}

/******************* UTILITY FUNCTIONS *************************/

/**
 * @brief
 *   Increment a replay counter
 *
 * @par Description
 *   Increment a replay counter
 *
 * @param
 *  CsrUint8*    : replay counter
 *  CsrUint32    : Length of the replay counter
 *
 * @return
 *  void
 */
static void increment(CsrUint8 *pArray, CsrUint32 length)
{
    CsrUint8 *pBuffer;

    pBuffer = pArray + (length - 1);

    while (pBuffer >= pArray)
    {
        (*pBuffer) += 1;
        if (*pBuffer != 0)
        {
            break;
        }
        pBuffer--;
    }
}

#ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE
/**
 * @brief
 *   Check if one replay counter is greater than another
 *
 * @par Description
 *   Check if one replay counter is greater than another
 *   Used when accepting packets from the authenticator
 *
 * @param
 *  CsrUint8*    : First replay counter
 *  CsrUint8*    : Second replay counter
 *
 * @return
 *  CsrBool : TRUE if replay counter one greater than replay counter two otherwise FALSE
 */
static CsrBool isreplaygreater(CsrUint8 *pArrayOne, CsrUint8 *pArrayTwo)
{
    CsrInt32 i = CSR_WPA_KEY_REPLAY_COUNTER_LENGTH - 1;
    CsrBool result = FALSE;

    while (i >= 0)
    {
        if (pArrayOne[i] > pArrayTwo[i])
        {
            result = TRUE;
            break;
        }
        else
        if (pArrayOne[i] < pArrayTwo[i])
        {
            break;
        }

        i--;
    }

    return result;
}
#endif

/**
 * @brief
 *   Debug function to dump EAPOL key packet
 *
 * @par Description
 *   Debug function to dump EAPOL key packet
 *
 * @param
 *  eapol_packet*    : EAPOL Key
 *
 * @return
 *   void
 */
static void dump_eapol_key(eapol_packet *eapolPacket)
{
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "EAPOL_KEY", &(eapolPacket->u.key), (CsrUint32) EAPOL_LENGTH_FIELD(eapolPacket)));
    sme_trace_info((TR_SECURITY_LIB, "Descriptor type = %d", eapolPacket->u.key.type));
    sme_trace_info((TR_SECURITY_LIB, "Key Information = %04x", EAPOL_KEY_INFO_FIELD(eapolPacket)));

    switch(eapolPacket->u.key.type)
    {
    case 254:
        sme_trace_info((TR_SECURITY_LIB, "Reserved (b12-b15) = %d", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0xF000) >> 12));
        break;
    case 2:
        sme_trace_info((TR_SECURITY_LIB, "Reserved (b13-b15) = %d", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0xE000) >> 13));
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Key Data (b12)= %s", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x1000) ? "true" : "false"));
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unable to decode b12-b15 for Descriptor Type"));
        break;
    }

    sme_trace_info((TR_SECURITY_LIB, "Request (b11) = %s", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0800) ? "true" : "false"));
    sme_trace_info((TR_SECURITY_LIB, "Error (b10)= %s", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0400) ? "true" : "false"));
    sme_trace_info((TR_SECURITY_LIB, "Secure (b9) = %s", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0200) ? "true" : "false"));
    sme_trace_info((TR_SECURITY_LIB, "MIC present (b8) = %s", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0100) ? "true" : "false"));
    sme_trace_info((TR_SECURITY_LIB, "Key Ack (b7) = %s", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0080) ? "true" : "false"));
    sme_trace_info((TR_SECURITY_LIB, "Install (b6) = %s", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0040) ? "true" : "false"));

    switch(eapolPacket->u.key.type)
    {
    case 254:
        sme_trace_info((TR_SECURITY_LIB, "Key Index (b4-b5) = %d", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0030) >> 4));
        break;
    case 2:
        sme_trace_info((TR_SECURITY_LIB, "Reserved (b4-b5) = %d", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0030) >> 4));
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unable to decode b4-b5 for Descriptor Type"));
        break;
    }

    sme_trace_info((TR_SECURITY_LIB, "Key Type (b3) = %s", (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0008) ? "Pairwise" : "Group"));
    sme_trace_info((TR_SECURITY_LIB, "Key Descriptor Version (b0-b2) = %d", EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007));
    switch (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007)
    {
    case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
        sme_trace_info((TR_SECURITY_LIB, "(HMAC-MD5/ARC4)"));
        break;
    case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
        sme_trace_info((TR_SECURITY_LIB, "(HMAC-SHA1-128/AES)"));
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007));
        break;
    }
    sme_trace_info((TR_SECURITY_LIB, "Key length = %d", EAPOL_KEY_LENGTH_FIELD(eapolPacket)));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Key Replay Counter", eapolPacket->u.key.replay_counter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Nonce", eapolPacket->u.key.nonce, CSR_WPA_NONCE_LENGTH));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Key IV", eapolPacket->u.key.iv, CSR_WPA_KEY_IV_LENGTH));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Key RSC", eapolPacket->u.key.rsc, CSR_WPA_KEY_RSC_LENGTH));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Reserved", eapolPacket->u.key.reserved, CSR_WPA_KEY_RESERVED_LENGTH));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "MIC", eapolPacket->u.key.mic, CSR_WPA_MIC_LENGTH));
    sme_trace_info((TR_SECURITY_LIB, "Key Data Length = %d", EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket)));

    if (EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket))
    {
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "keyData", &(eapolPacket->u.key.key_data), EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket)));
    }

}

/**
 * @brief
 *   Function to send EAPOL key packet
 *
 * @par Description
 *   Function to send EAPOL key packet
 *
 * @param
 *  CsrWifiSecurityContext* : Pointer to current securityContext
 *
 * @return
 *   void
 */

static void send_packet(CsrWifiSecurityContext* securityContext)
{
    sme_trace_info((TR_SECURITY_LIB, "Sending EAPOL key packet"));
    dump_eapol_key((eapol_packet*)securityContext->buffer);

    securityContext->callbacks.sendPacket(securityContext->externalContext,
                                          securityContext->buffer,
                                          securityContext->bufferLength,
                                          securityContext->setupData.localMacAddress,
                                          securityContext->setupData.peerMacAddress);
}

#if defined (CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE) || defined (CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE)
/**
 * @brief
 *   Compute MIC on the packet using Key confirmation key
 *
 * @par Description
 *   Invoke the right crypto function to compute MIC using Key confirmation key
 *
 * @param
 *  CsrWpaPersonalCtx *: WPA handshake module Context
 *  eapol_packet*      : Received EAPOL Key
 *  void*              : Client context, context of the caller that will be used when making
 *                        call backs
 *  CsrUint8*             : Buffer on which to compute MIC
 *  CsrUint32             : Length of the buffer
 *  CryptoCallBack     : Function that needs to be called after crypto functionality is completed
 *
 * @return
 *   void
 */
static void compute_mic(CsrWpaPersonalCtx *pCtx, eapol_packet *eapolPacket,
       CsrUint8 *pBuffer, CsrUint32 bufferLength, CryptoCallBack pCryptoCallBack)
{
    CsrUint8 *hash;
    CsrCryptoCallbackContext *wpaCryptoCallbackContext;

    hash = pCtx->cryptoResultBuffer;

    /* Compute MIC and add to the message */
    switch (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007)
    {
    case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
        {
            sme_trace_info((TR_SECURITY_LIB, "compute_mic:CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4"));

            wpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, pCryptoCallBack, hash);
            CsrCryptoCallHmacMd5((CsrCryptoContext *)pCtx->securityContext->cryptoContext, wpaCryptoCallbackContext, KCK(pCtx->ptk), CSR_WPA_KCK_LENGTH, pBuffer, bufferLength, hash);
        }
        break;
    case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
        {
            sme_trace_info((TR_SECURITY_LIB, "compute_mic:CSR_WPA_KEY_DESCRIPTOR_SHA1_AES"));

            wpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, pCryptoCallBack, hash);
            CsrCryptoCallHmacSha1((CsrCryptoContext *)pCtx->securityContext->cryptoContext, wpaCryptoCallbackContext, KCK(pCtx->ptk), CSR_WPA_KCK_LENGTH, pBuffer, bufferLength, hash);
        }
        break;
    default:
        CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
        sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007));
        break;
    }
}

/**
 * @brief
 *   Derive Pairwise keys from M 1 and M 2 (in case of authenticator)
 *
 * @par Description
 *   Use HMAC SHA1 loops to derive unicast keys
 *
 * @param
*   CsrWpaPersonalCtx *: WPA handshake module Context
 *  eapol_packet*      : Received EAPOL Key
 *  void*              : Client context, context of the caller that will be used when making
 *                        call backs
 *  CryptoCallBack     : Function that needs to be called after crypto functionality is completed
 *
 * @return
 *   void
 */
static void derive_keys(CsrWpaPersonalCtx *pCtx, eapol_packet *eapolPacket, CryptoCallBack pCallBack)
{
    CsrUint8 *pSeed;
    CsrUint8 *pCount;
    CsrUint32 ptkLength;
    CsrCryptoCallbackContext *wpaCryptoCallbackContext;
    const char personalisation[] = "Pairwise key expansion";

    sme_trace_info((TR_SECURITY_LIB, "Entering derive_keys"));

    /* Compute HMAC-SHA1 of seed data keyed with PMK */
    pSeed = pCtx->seed;
    CsrMemCpy( pSeed, personalisation, CsrStrLen( personalisation ) + 1 );
    pSeed += CsrStrLen( personalisation ) + 1;
    if (CsrMemCmp( pCtx->securityContext->setupData.peerMacAddress, pCtx->securityContext->setupData.localMacAddress, CSR_WPA_MAC_LENGTH ) > 0)
    {
        CsrMemCpy( pSeed, pCtx->securityContext->setupData.localMacAddress, CSR_WPA_MAC_LENGTH );
        pSeed += CSR_WPA_MAC_LENGTH;
        CsrMemCpy( pSeed, pCtx->securityContext->setupData.peerMacAddress, CSR_WPA_MAC_LENGTH );
        pSeed += CSR_WPA_MAC_LENGTH;
    }
    else
    {
        CsrMemCpy( pSeed, pCtx->securityContext->setupData.peerMacAddress, CSR_WPA_MAC_LENGTH );
        pSeed += CSR_WPA_MAC_LENGTH;
        CsrMemCpy( pSeed, pCtx->securityContext->setupData.localMacAddress, CSR_WPA_MAC_LENGTH );
        pSeed += CSR_WPA_MAC_LENGTH;
    }
    if (CsrMemCmp( eapolPacket->u.key.nonce, pCtx->nonce, CSR_WPA_NONCE_LENGTH ) > 0)
    {
        CsrMemCpy( pSeed, pCtx->nonce, CSR_WPA_NONCE_LENGTH );
        pSeed += CSR_WPA_NONCE_LENGTH;
        CsrMemCpy( pSeed, eapolPacket->u.key.nonce, CSR_WPA_NONCE_LENGTH );
        pSeed += CSR_WPA_NONCE_LENGTH;
    }
    else
    {
        CsrMemCpy( pSeed, eapolPacket->u.key.nonce, CSR_WPA_NONCE_LENGTH );
        pSeed += CSR_WPA_NONCE_LENGTH;
        CsrMemCpy( pSeed, pCtx->nonce, CSR_WPA_NONCE_LENGTH );
        pSeed += CSR_WPA_NONCE_LENGTH;
    }

    switch (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007)
    {
    case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
        ptkLength = CSR_WPA_TKIP_PTK_LENGTH;
        break;
    case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
        ptkLength = CSR_WPA_CCMP_PTK_LENGTH;
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007));
        ptkLength = CSR_WPA_MAX_PTK_LENGTH;
        break;
    }

    pCount = pSeed++;
    *pCount = 0;
    pCtx->seedLength = (CsrUint32) (pSeed - pCtx->seed);

    pCtx->hmacSha1Loop = 0;
    pCtx->hmacSha1TotalLoops = ((ptkLength * 8) + 159) / 160;

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Seed data", pCtx->seed, pCtx->seedLength));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "PMK", pCtx->securityContext->setupData.pmk, CSR_WPA_PMK_LENGTH));

    /* Implementation assumes that there are at least two calls to hmac_sha1
     * One call happens here, all subsequent calls happen in continue_hmac_sha1 */
    wpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, pCallBack, pCtx->ptk);
    CsrCryptoCallHmacSha1((CsrCryptoContext *)pCtx->securityContext->cryptoContext, wpaCryptoCallbackContext, pCtx->securityContext->setupData.pmk, CSR_WPA_PMK_LENGTH, pCtx->seed, pCtx->seedLength, pCtx->ptk);
}
#endif

#ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE
/******* SUPPLICANT SPECIFIC FUNCTIONS ***********/
/******************* FUNCTIONS TO PROCESS PAIRWISE HANDSHAKE 1 *************************/

/**
 * @brief
 *   Part 6 of processing M 1
 *
 * @par Description
 *   Send M 2
 *
 * @param
 *  void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void send_pairwise_handshake_2 (CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;
    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket;

    sme_trace_info((TR_SECURITY_LIB, "Entering send_pairwise_handshake_2"));

    /* Copy MIC from the result buffer */
    eapolPacket = (eapol_packet*)pCtx->securityContext->buffer;
    CsrMemCpy((CsrUint8*)eapolPacket->u.key.mic, wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH);

    /* Reset Retransmission parameters */
    pCtx->securityContext->callbacks.startTimer(pCtx->securityContext->externalContext,
                                                pCtx->securityContext->setupData.responseTimeout,
                                                CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);
    pCtx->securityContext->retransmissionCount = 1;

    sme_trace_info((TR_SECURITY_LIB, "Trying to send packet len %d, send %x:%x:%x:%x:%x:%x\n", pCtx->securityContext->bufferLength,
      pCtx->securityContext->setupData.peerMacAddress[0], pCtx->securityContext->setupData.peerMacAddress[1], pCtx->securityContext->setupData.peerMacAddress[2], pCtx->securityContext->setupData.peerMacAddress[3], pCtx->securityContext->setupData.peerMacAddress[4], pCtx->securityContext->setupData.peerMacAddress[5]));

    send_packet(pCtx->securityContext);
    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}

/**
 * @brief
 *   Part 5 of processing M 1
 *
 * @par Description
 *   Build M 2
 *
 * @param
*   CsrWpaPersonalCtx *: WPA handshake module Context
 *  eapol_packet*      : Received EAPOL Key
 *  void*              : Client context, context of the caller that will be used when making
 *                        call backs
 *
 * @return
 *   void
 */
static void build_pairwise_handshake_2(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrUint32 packetBodyLength;
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;
    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *rxEapolPacket = wpaCryptoCookie->wpaEapolPacket;
    eapol_packet *txEapolPacket = (eapol_packet*) pCtx->securityContext->buffer;

    sme_trace_info((TR_SECURITY_LIB, "Entering build_pairwise_handshake_2"));
    switch (EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0007)
    {
    case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
        EAPOL_KEY_INFO_ASSIGN(txEapolPacket, 0x0109);
        EAPOL_KEY_LENGTH_ASSIGN(txEapolPacket, 0x20); /*lint !e572 */
        break;
    case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
        EAPOL_KEY_INFO_ASSIGN(txEapolPacket, 0x010a);
        EAPOL_KEY_LENGTH_ASSIGN(txEapolPacket, 0x0); /*lint !e572 */
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(txEapolPacket) & 0x0007));
        break;
    }

    /* TODO: When processing message 1 always copy the replay counter, we do not have a way of knowing a replay here? */
    CsrMemCpy(pCtx->keyReplayCounter, rxEapolPacket->u.key.replay_counter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "pCtx->keyReplayCounter", pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH));

    /* Build message 2 */

    CsrMemCpy(&(txEapolPacket->llc_header), pCtx->securityContext->setupData.protocolSnap, CSR_WIFI_SECURITY_SNAP_LENGTH);

    /* Some APs insist on using version 1 (which is the earlier wpa 3.1 spec) wpa_supplicant uses 1 by default */
    txEapolPacket->version = 1;

    txEapolPacket->packet_type = EAPOL_PACKET_TYPE_EAPOL_KEY;
    txEapolPacket->u.key.type = rxEapolPacket->u.key.type;
    CsrMemCpy(txEapolPacket->u.key.replay_counter, pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH);
    CsrMemCpy(txEapolPacket->u.key.nonce, pCtx->nonce, CSR_WPA_NONCE_LENGTH);
    CsrMemSet(txEapolPacket->u.key.iv, 0, CSR_WPA_KEY_IV_LENGTH);
    CsrMemSet(txEapolPacket->u.key.rsc, 0, CSR_WPA_KEY_RSC_LENGTH);
    CsrMemSet(txEapolPacket->u.key.reserved, 0, CSR_WPA_KEY_RESERVED_LENGTH );
    CsrMemSet(txEapolPacket->u.key.mic, 0, CSR_WPA_MIC_LENGTH );
    EAPOL_KEY_DATA_LENGTH_ASSIGN(txEapolPacket, pCtx->securityContext->setupData.secIeLen);
    CsrMemCpy(&(txEapolPacket->u.key.key_data), pCtx->securityContext->setupData.secIe, EAPOL_KEY_DATA_LENGTH_FIELD(txEapolPacket));
    packetBodyLength = (&(txEapolPacket->u.key.key_data) - &(txEapolPacket->u.key.type)) + EAPOL_KEY_DATA_LENGTH_FIELD(txEapolPacket);
    EAPOL_LENGTH_ASSIGN(txEapolPacket, (CsrUint16)packetBodyLength);
    pCtx->securityContext->bufferLength = packetBodyLength + EAPOL_HEADER_LENGTH + CSR_WIFI_SECURITY_SNAP_LENGTH;

    compute_mic(pCtx, rxEapolPacket, pCtx->securityContext->buffer + CSR_WIFI_SECURITY_SNAP_LENGTH, pCtx->securityContext->bufferLength - CSR_WIFI_SECURITY_SNAP_LENGTH, send_pairwise_handshake_2);
}

/**
 * @brief
 *   Part 3 of processing M 1
 *
 * @par Description
 *   Continues the HMAC SHA1 loop to derive the unicast keys
 *
 * @param
 *  void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void continue_hmac_sha1_process_pairwise_handshake_1(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrCryptoCallbackContext *newWpaCryptoCallbackContext;
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;

    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = wpaCryptoCookie->wpaEapolPacket;
    CsrUint8 *pCount, *pPtk = pCtx->ptk;

    sme_trace_info((TR_SECURITY_LIB, "Entering continue_hmac_sha1_process_pairwise_handshake_1"));

    pCtx->hmacSha1Loop++;

    pCount = pCtx->seed + pCtx->seedLength -1;
    *pCount = (CsrUint8) pCtx->hmacSha1Loop;

    if (pCtx->hmacSha1Loop < (pCtx->hmacSha1TotalLoops - 1))
    {
        /* More hmac_sha1 processing required after current call to hmac_sha1 */
        newWpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, continue_hmac_sha1_process_pairwise_handshake_1, pCtx->ptk);
    }
    else
    {
        /* Last call to hmac_sha1 */
        newWpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, build_pairwise_handshake_2, pCtx->ptk);
    }

    CsrCryptoCallHmacSha1((CsrCryptoContext *)pCtx->securityContext->cryptoContext, newWpaCryptoCallbackContext, pCtx->securityContext->setupData.pmk, CSR_WPA_PMK_LENGTH, pCtx->seed, pCtx->seedLength, pPtk + (pCtx->hmacSha1Loop * CSR_SHA1_DIGEST_LENGTH));
}

/**
 * @brief
 *   Part 1 of processing M 1
 *
 * @par Description
 *   Randomize Nonce
 *
 * @param
 *   CsrWpaPersonalCtx *: WPA handshake module Context
 *   eapol_packet*      : Received EAPOL Key
 *   void*              : Client context, context of the caller that will be used when making
 *                        call backs
 *
 * @return
 *   void
 */
static void begin_process_pairwise_handshake_1(CsrWpaPersonalCtx *pCtx, eapol_packet *eapolPacket)
{
    sme_trace_info((TR_SECURITY_LIB, "Entering begin_process_pairwise_handshake_1"));

    CsrWifiSecurityRandom(pCtx->nonce, CSR_WPA_NONCE_LENGTH );
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Local Nonce", pCtx->nonce, CSR_WPA_NONCE_LENGTH));

    pCtx->keyInfo = EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007;

/*lint -save -e420 -e416 */
#ifdef CSR_WIFI_SECURITY_PMK_CACHING_ENABLE
    /* If its WPA2 and AP sent us a PMKID in a PMKID KDE, get the PMK with respect to this PMKID */
    if((pCtx->securityContext->setupData.securityType != CSR_WIFI_SECURITY_TYPE_WPAPSK_SUPPLICANT) /* Dont bother for PSK mode */
            &&  (EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket) == 22)) /* 22 is the KDE length including PMK */
    {
        const CsrUint8 pmkidKdeHeader[] = {0xdd, 0x14, 0x00, 0x0f, 0xac, 0x04};
        /* Look for PMKID KDE */
        if (0 == CsrMemCmp(&eapolPacket->u.key.key_data, pmkidKdeHeader, sizeof(pmkidKdeHeader)))
        {
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "PMKID from AP",
                    (CsrUint8*)(&eapolPacket->u.key.key_data) + sizeof(pmkidKdeHeader), CSR_WIFI_SECURITY_PMKID_LENGTH));
            /* PMKID is present, get the corresponding pmk */
            if (TRUE == pCtx->securityContext->callbacks.pmkGet(pCtx->securityContext->externalContext,
                                                    (CsrUint8*)(&eapolPacket->u.key.key_data) + sizeof(pmkidKdeHeader),
                                                    pCtx->securityContext->setupData.pmk))
            {
                pCtx->securityContext->setupData.pmkValid = TRUE;
            }
            else
            {
                sme_trace_error((TR_SECURITY_LIB, "PMKID sent by AP is not recognized: Aborting handshake"));
                CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
                pCtx->securityContext->callbacks.abortProcedure(pCtx->securityContext->externalContext);
                return;
            }
        }
    }
#endif
/* lint -restore */

    derive_keys(pCtx, eapolPacket, continue_hmac_sha1_process_pairwise_handshake_1);
}

/******************* FUNCTIONS TO PROCESS PAIRWISE HANDSHAKE 3 *************************/

/**
 * @brief
 *   Part 5 of processing M 3
 *
 * @par Description
 *   Add MIC to M 4 and send it out
 *   Install Pairwise keys
 *   Install group keys if available
 *
 * @param
 *  void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void send_pairwise_handshake_4(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;

    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = (eapol_packet*)pCtx->securityContext->buffer;

    sme_trace_info((TR_SECURITY_LIB, "Entering send_pairwise_handshake_4"));

    CsrMemCpy((CsrUint8*)eapolPacket->u.key.mic, wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH);

    /* Reset Retransmission parameters */
    pCtx->securityContext->callbacks.stopTimer(pCtx->securityContext->externalContext,
                                               CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);
    pCtx->securityContext->retransmissionCount = 0;

    send_packet(pCtx->securityContext);

    /* Install the pairwise Temporal Key and GTK */
    pCtx->securityContext->callbacks.installPairwiseKey(pCtx->securityContext->externalContext,
            CSR_WIFI_SECURITY_KEYTYPE_IEEE80211I, TK(pCtx->ptk), EAPOL_KEY_LENGTH_FIELD(eapolPacket), NULL, 0);

    if (pCtx->gtkAvailable)
    {
        pCtx->securityContext->callbacks.installGroupKey(pCtx->securityContext->externalContext,
                                                         CSR_WIFI_SECURITY_KEYTYPE_IEEE80211I, pCtx->gtk, pCtx->gtkLength,
                                                         NULL, 1);

        /* Security association time is counted from association to set protection but here we
         * count from security start to setting group key */
        pCtx->securityContext->callbacks.stopTimer(pCtx->securityContext->externalContext,
                                                   CSR_WIFI_SECURITY_TIMEOUT_SECURITY_ASSOCIATION);
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "GTK not available"));
    }

    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}

/**
 * @brief
 *   Part 4 of processing M 3
 *
 * @par Description
 *   Build M 4
 *
 * @param
 *   CsrWpaPersonalCtx *: WPA handshake module Context
 *   eapol_packet*      : Received EAPOL Key
 *   void*              : Client context, context of the caller that will be used when making
 *                        call backs
 *
 * @return
 *   void
 */
static void build_pairwise_handshake_4(CsrWpaPersonalCtx *pCtx, eapol_packet *rxEapolPacket)
{

    eapol_packet *txEapolPacket = (eapol_packet *) pCtx->securityContext->buffer;
    CsrUint32 packetBodyLength;

    sme_trace_info((TR_SECURITY_LIB, "Entering build_pairwise_handshake_4"));

    /* Build message 4 */

    switch (EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0007)
    {
    case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
        /* Reflect Secure bit received from Authenticator */
        if((EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0200))
        {
            EAPOL_KEY_INFO_ASSIGN(txEapolPacket, 0x0309);
        }
        else
        {
            EAPOL_KEY_INFO_ASSIGN(txEapolPacket, 0x0109);
        }
        EAPOL_KEY_LENGTH_ASSIGN(txEapolPacket, 0x20); /*lint !e572 */
        break;
    case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
        /* Reflect Secure bit received from Authenticator */
        if((EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0200))
        {
            EAPOL_KEY_INFO_ASSIGN(txEapolPacket, 0x030a);
        }
        else
        {
            EAPOL_KEY_INFO_ASSIGN(txEapolPacket, 0x010a);
        }
        EAPOL_KEY_LENGTH_ASSIGN(txEapolPacket, 0x10); /*lint !e572 */
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(txEapolPacket) & 0x0007));
        break;
    }

    CsrMemCpy(&(txEapolPacket->llc_header), pCtx->securityContext->setupData.protocolSnap, CSR_WIFI_SECURITY_SNAP_LENGTH);

    /* Some APs insist on using version 1 (which is the earlier wpa 3.1 spec) wpa_supplicant uses 1 by default */
    txEapolPacket->version = 1;

    txEapolPacket->packet_type = EAPOL_PACKET_TYPE_EAPOL_KEY;
    txEapolPacket->u.key.type = rxEapolPacket->u.key.type;
    CsrMemCpy(txEapolPacket->u.key.replay_counter, pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH);
    CsrMemSet(txEapolPacket->u.key.nonce, 0, CSR_WPA_NONCE_LENGTH);
    CsrMemSet(txEapolPacket->u.key.iv, 0, CSR_WPA_KEY_IV_LENGTH);
    CsrMemSet(txEapolPacket->u.key.rsc, 0, CSR_WPA_KEY_RSC_LENGTH);
    CsrMemSet(txEapolPacket->u.key.reserved, 0, CSR_WPA_KEY_RESERVED_LENGTH );
    CsrMemSet(txEapolPacket->u.key.mic, 0, CSR_WPA_MIC_LENGTH );
    EAPOL_KEY_DATA_LENGTH_ASSIGN(txEapolPacket, 0); /*lint !e572 */
    packetBodyLength = (&(txEapolPacket->u.key.key_data) - &(txEapolPacket->u.key.type)) + EAPOL_KEY_DATA_LENGTH_FIELD(txEapolPacket);
    EAPOL_LENGTH_ASSIGN(txEapolPacket, (CsrUint16)packetBodyLength);
    pCtx->securityContext->bufferLength = packetBodyLength + EAPOL_HEADER_LENGTH + CSR_WIFI_SECURITY_SNAP_LENGTH;

    compute_mic(pCtx, rxEapolPacket, pCtx->securityContext->buffer + CSR_WIFI_SECURITY_SNAP_LENGTH, pCtx->securityContext->bufferLength - CSR_WIFI_SECURITY_SNAP_LENGTH, send_pairwise_handshake_4);
}

/**
 * @brief
 *   Part 3 of processing M 3
 *
 * @par Description
 *   Store Group keys
 *
 * @param
 *  void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void store_group_key_process_pairwise_handshake_3(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrUint8 type;
    CsrBool parseError = TRUE;
    CsrUint8 length;
    CsrUint8 *pBuffer;
    CsrUint8 gtkkde[] = { 0x00, 0x0F, 0xAC, 0x01 };
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;
    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = wpaCryptoCookie->wpaEapolPacket;
    CsrUint16 keyDataLength = (CsrUint16)EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket);

    sme_trace_entry((TR_SECURITY_LIB, "store_group_key_pairwise_handshake_3"));

    do
    {
        /* Safety checks, somewhat paranoid */
        if ((keyDataLength == 0) || (keyDataLength > EAPOL_LENGTH_FIELD(eapolPacket)))
        {
            sme_trace_warn((TR_SECURITY_LIB, "Key info bit says encrypted key data is present, but key data length "
                    "is incorrect"));
            break;
        }

        if (((EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007) == CSR_WPA_KEY_DESCRIPTOR_SHA1_AES))
        {
            if (!(keyDataLength % 8))
            {
                keyDataLength -= 8;
            }
            else
            {
                /* Wrong encoding of key data, do no decode key */
                break;
            }
        }

        pBuffer = wpaCryptoCallbackContext->resultBuffer;
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "plain", pBuffer, EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket) - 8));
        sme_trace_info((TR_SECURITY_LIB, "Result buffer = %x", pBuffer));

        while( keyDataLength != 0 )
        {
            if((*pBuffer == 0xdd) && ((keyDataLength == 1 ) || (*(pBuffer + 1) == 0)))
            {
                /* Padding */
                break;
            }

            type = *pBuffer++;
            length = *pBuffer++;

            if ((length + 2) > keyDataLength)
            {
                break;
            }

            sme_trace_info((TR_SECURITY_LIB, "IE type = %02x, IE length = %02x", type, length ));
            if( type == 0xdd && !CsrMemCmp( pBuffer, gtkkde, sizeof( gtkkde ) ))
            {
                sme_trace_info((TR_SECURITY_LIB, "GTK KDE"));
                pBuffer += 6;
                pCtx->gtkLength = length - 6;
                CsrMemCpy( pCtx->gtk, pBuffer, pCtx->gtkLength );
                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "GTK", pCtx->gtk, pCtx->gtkLength));
                pCtx->gtkAvailable = TRUE;
                parseError = FALSE;
                break;
            }
            pBuffer += length;

            keyDataLength -= (2 + length);
        }
    } while (0);

    if (parseError == FALSE)
    {
        build_pairwise_handshake_4(pCtx, eapolPacket);
    }
}

/**
 * @brief
 *   Part 2 of processing M 3
 *
 * @par Description
 *   Verify MIC
 *
 * @param
 *  void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void continue_process_pairwise_handshake_3(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrUint8 micPass = FALSE;
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;
    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = wpaCryptoCookie->wpaEapolPacket;

    CsrCryptoCallbackContext *newWpaCryptoCallbackContext;

    sme_trace_info((TR_SECURITY_LIB, "Entering continue_process_pairwise_handshake_3"));

    CsrMemCpy((CsrUint8*)eapolPacket->u.key.mic, pCtx->mic, CSR_WPA_MIC_LENGTH);

    if (CsrMemCmp((CsrUint8*)eapolPacket->u.key.mic, wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH ) == 0)
    {
        sme_trace_info((TR_SECURITY_LIB, "MIC verified OK"));
        micPass = TRUE;
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "MIC error"));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Received MIC", eapolPacket->u.key.mic, CSR_WPA_MIC_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Computed MIC", wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH));
    }

    if (micPass)
    {
        if (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x1000) /* Encrypted Key Data */
        {
            switch (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007)
            {
            case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
                {
                    CsrUint8 key[32];
                    CsrUint8 *plain;

                    plain = pCtx->cryptoResultBuffer;

                    sme_trace_info((TR_SECURITY_LIB, "using RC4"));

                    CsrMemCpy( key, eapolPacket->u.key.iv, 16 );
                    CsrMemCpy( key + 16, KEK(pCtx->ptk), 16 );

                    newWpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, store_group_key_process_pairwise_handshake_3, plain);
                    /* This RC4 function sets the key and discards first 256 bytes of keystream  */
                    CsrCryptoCallRc4Discard256((CsrCryptoContext *)pCtx->securityContext->cryptoContext, newWpaCryptoCallbackContext, 32, key, (CsrUint16)EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket), &(eapolPacket->u.key.key_data), plain);
                    return;
                }
            case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
                {
                    CsrUint8 *plain;

                    sme_trace_info((TR_SECURITY_LIB, "using AES"));

                    plain = pCtx->cryptoResultBuffer;
                    newWpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, store_group_key_process_pairwise_handshake_3, plain);
                    CsrCryptoCallAes128Unwrap((CsrCryptoContext *)pCtx->securityContext->cryptoContext, newWpaCryptoCallbackContext, KEK(pCtx->ptk), (EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket) / 8) - 1, &(eapolPacket->u.key.key_data), plain);
                    return;
                }
            default:
                sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007));
                break;
            }
        }
        else
        {
            sme_trace_info((TR_SECURITY_LIB, "Key Data is not encrypted"));
        }

        build_pairwise_handshake_4(pCtx, eapolPacket);
    }
    else
    {
        CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
    }
}

/**
 * @brief
 *   Part 1 of processing M 3
 *
 * @par Description
 *   Check Replay, compute MIC for verification
 *
 * @param
 *   CsrWpaPersonalCtx *: WPA handshake module Context
 *   eapol_packet*      : Received EAPOL Key
 *   void*              : Client context, context of the caller that will be used when making
 *                        call backs
 *
 * @return
 *   void
 */
static void begin_process_pairwise_handshake_3(CsrWpaPersonalCtx *pCtx, eapol_packet *eapolPacket)
{
    CsrUint32 length;

    sme_trace_entry((TR_SECURITY_LIB, "begin_process_pairwise_handshake_3"));

    /* Check that the received replay counter is greater than what we received before */
    if (isreplaygreater((CsrUint8*)eapolPacket->u.key.replay_counter, (CsrUint8*)pCtx->keyReplayCounter))
    {
        /* Copy to use later */
        CsrMemCpy(pCtx->keyReplayCounter, eapolPacket->u.key.replay_counter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH);
    }
    else
    {
        CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
        return;
    }

    sme_trace_info((TR_SECURITY_LIB, "begin_process_pairwise_handshake_3:"));

    length = (CsrUint32) (EAPOL_KEY_PACKET_LENGTH(eapolPacket) - CSR_WIFI_SECURITY_SNAP_LENGTH);

    /* To verify MIC, mic field should contain 0 */
    CsrMemCpy(pCtx->mic, eapolPacket->u.key.mic, CSR_WPA_MIC_LENGTH);
    CsrMemSet((CsrUint8*)eapolPacket->u.key.mic, 0, CSR_WPA_MIC_LENGTH);

    compute_mic(pCtx, eapolPacket, (CsrUint8*)&(eapolPacket->version), length, continue_process_pairwise_handshake_3);
}

/******************* FUNCTIONS TO PROCESS GROUP HANDSHAKE 1 *************************/

/**
 * @brief
 *   Part 4 of processing Group M 1
 *
 * @par Description
 *   Add MIC to Group M 2, Send Group M 2 and install the key
 *
 * @param
 *   void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void send_group_handshake_2(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;
    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = (eapol_packet*)pCtx->securityContext->buffer;

    sme_trace_entry((TR_SECURITY_LIB, "send_group_handshake_2"));

    CsrMemCpy((CsrUint8*)eapolPacket->u.key.mic, wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH);

    send_packet(pCtx->securityContext);

    if (pCtx->gtkAvailable)
    {
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "RSC", pCtx->rsc, CSR_WPA_KEY_RSC_LENGTH));
        pCtx->securityContext->callbacks.installGroupKey(pCtx->securityContext->externalContext,
                                                         CSR_WIFI_SECURITY_KEYTYPE_IEEE80211I, pCtx->gtk, pCtx->gtkLength,
                                                         pCtx->rsc, 1);
        /* Delete Config SA timeout */
        pCtx->securityContext->callbacks.stopTimer(pCtx->securityContext->externalContext,
                                                   CSR_WIFI_SECURITY_TIMEOUT_SECURITY_ASSOCIATION);
    }

    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}

/**
 * @brief
 *   Part 3 of processing Group M 1
 *
 * @par Description
 *   Store group key and build Group M 2
 *
 * @param
 *   void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void build_group_handshake_2(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrUint8 *pBuffer;

    CsrUint32 packetBodyLength;
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;
    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *rxEapolPacket = wpaCryptoCookie->wpaEapolPacket;
    eapol_packet *txEapolPacket = (eapol_packet*) pCtx->securityContext->buffer;
    CsrUint16 keyInfo;

    sme_trace_entry((TR_SECURITY_LIB, "build_group_handshake_2"));

    pBuffer = wpaCryptoCallbackContext->resultBuffer;

    if (rxEapolPacket->u.key.type == 254)
    {
        pCtx->gtkLength = EAPOL_KEY_DATA_LENGTH_FIELD(rxEapolPacket);
        if ((EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0007) == CSR_WPA_KEY_DESCRIPTOR_SHA1_AES)
        {
            pCtx->gtkLength -= 8;
        }
        pCtx->gtkAvailable = TRUE;
        CsrMemCpy( pCtx->gtk, pBuffer, pCtx->gtkLength );

        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "GTK", pCtx->gtk, pCtx->gtkLength));
    }
    else
    if (rxEapolPacket->u.key.type == 2)
    {
        CsrUint8 type, length;
        CsrUint8 gtkkde[] = { 0x00, 0x0F, 0xAC, 0x01 };

        do
        {
            type = *pBuffer++;
            length = *pBuffer++;
            sme_trace_info((TR_SECURITY_LIB, "IE type = %02x, IE length = %02x", type, length ));
            if( type == 0xdd && !CsrMemCmp( pBuffer, gtkkde, sizeof( gtkkde ) ) )
            {
                sme_trace_info((TR_SECURITY_LIB, "GTK KDE"));
                pBuffer += 6;
                pCtx->gtkLength = length - 6;
                CsrMemCpy( pCtx->gtk, pBuffer, pCtx->gtkLength );
                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "GTK", pCtx->gtk, pCtx->gtkLength));
                pCtx->gtkAvailable = TRUE;
            }
            pBuffer += length;
        }
        while( length != 0 );
    }

    switch (EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0007)
    {
    case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
        keyInfo = 0x0301;
        keyInfo |= EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0030;
        EAPOL_KEY_INFO_ASSIGN(txEapolPacket, keyInfo);
        EAPOL_KEY_LENGTH_ASSIGN(txEapolPacket, 0x20); /*lint !e572 */
        break;
    case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
        keyInfo = 0x0302;
        keyInfo |= EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0030;
        EAPOL_KEY_INFO_ASSIGN(txEapolPacket, keyInfo);
        EAPOL_KEY_LENGTH_ASSIGN(txEapolPacket, 0x10); /*lint !e572 */
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(txEapolPacket) & 0x0007));
        break;
    }

    sme_trace_info((TR_SECURITY_LIB, "replay counter (%d)", pCtx->keyReplayCounter[CSR_WPA_KEY_REPLAY_COUNTER_LENGTH - 1]));

    CsrMemCpy(&(txEapolPacket->llc_header), pCtx->securityContext->setupData.protocolSnap, CSR_WIFI_SECURITY_SNAP_LENGTH);

    /* Some APs insist on using version 1 (which is the earlier wpa 3.1 spec) wpa_supplicant uses 1 by default */
    txEapolPacket->version = 1;

    txEapolPacket->packet_type = EAPOL_PACKET_TYPE_EAPOL_KEY;
    txEapolPacket->u.key.type = rxEapolPacket->u.key.type;
    CsrMemCpy(txEapolPacket->u.key.replay_counter, pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH);
    CsrMemSet(txEapolPacket->u.key.nonce, 0, CSR_WPA_NONCE_LENGTH);
    CsrMemSet(txEapolPacket->u.key.iv, 0, CSR_WPA_KEY_IV_LENGTH);
    CsrMemSet(txEapolPacket->u.key.rsc, 0, CSR_WPA_KEY_RSC_LENGTH);
    CsrMemSet(txEapolPacket->u.key.reserved, 0, CSR_WPA_KEY_RESERVED_LENGTH );
    CsrMemSet(txEapolPacket->u.key.mic, 0, CSR_WPA_MIC_LENGTH );
    EAPOL_KEY_DATA_LENGTH_ASSIGN(txEapolPacket, 0); /*lint !e572 */

    packetBodyLength = (&(txEapolPacket->u.key.key_data) - &(txEapolPacket->u.key.type)) + EAPOL_KEY_DATA_LENGTH_FIELD(txEapolPacket);
    EAPOL_LENGTH_ASSIGN(txEapolPacket, (CsrUint16)packetBodyLength);

    pCtx->securityContext->bufferLength = packetBodyLength + EAPOL_HEADER_LENGTH + CSR_WIFI_SECURITY_SNAP_LENGTH;

    compute_mic(pCtx, rxEapolPacket, pCtx->securityContext->buffer + CSR_WIFI_SECURITY_SNAP_LENGTH, pCtx->securityContext->bufferLength - CSR_WIFI_SECURITY_SNAP_LENGTH, send_group_handshake_2);
}

/**
 * @brief
 *   Part 2 of processing Group M 1
 *
 * @par Description
 *   Verify MIC and extract the group key
 *
 * @param
 *   void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void continue_process_group_handshake_1(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrUint8 micPass = FALSE;
    CsrCryptoCallbackContext *newWpaCryptoCallbackContext;
    CsrUint8 *plain;
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;

    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = wpaCryptoCookie->wpaEapolPacket;

    sme_trace_info((TR_SECURITY_LIB, "Entering continue_process_group_handshake_1"));

    CsrMemCpy((CsrUint8*)eapolPacket->u.key.mic, pCtx->mic, CSR_WPA_MIC_LENGTH);

    if (CsrMemCmp((CsrUint8*)eapolPacket->u.key.mic, wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH ) == 0)
    {
        sme_trace_info((TR_SECURITY_LIB, "MIC verified OK"));
        micPass = TRUE;
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "MIC error"));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Received MIC", eapolPacket->u.key.mic, CSR_WPA_MIC_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Computed MIC", wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH));
    }

    if (micPass)
    {
        plain = pCtx->cryptoResultBuffer;

        switch (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007)
        {
        case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
            {
                CsrUint8 key[32];
                CsrMemCpy( key, eapolPacket->u.key.iv, 16 );
                CsrMemCpy( key + 16, KEK(pCtx->ptk), 16 );
                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "key", key, 32));

                newWpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, build_group_handshake_2, plain);
                /* This RC4 function sets the key and discards first 256 bytes of keystream  */
                CsrCryptoCallRc4Discard256((CsrCryptoContext *)pCtx->securityContext->cryptoContext, newWpaCryptoCallbackContext, 32, key, (CsrUint16)EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket), &(eapolPacket->u.key.key_data), plain);

            }
            break;
        case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
            {
                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "KeyData", &(eapolPacket->u.key.key_data), EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket)));
                newWpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, build_group_handshake_2, plain);
                CsrCryptoCallAes128Unwrap((CsrCryptoContext *)pCtx->securityContext->cryptoContext, newWpaCryptoCallbackContext, KEK(pCtx->ptk), (EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket) / 8) - 1, &(eapolPacket->u.key.key_data), plain);
            }
            break;
        default:
            CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
            sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007));
            break;
        }
    }
    else
    {
        CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
    }
}

/**
 * @brief
 *   Part 1 of processing Group M 1
 *
 * @par Description
 *   Check replay and MIC
 *
 * @param
 *   CsrWpaPersonalCtx *: WPA handshake module Context
 *   eapol_packet*      : Received EAPOL Key
 *   void*              : Client context, context of the caller that will be used when making
 *                        call backs
 * @return
 *   void
 */
static void begin_process_group_handshake_1(CsrWpaPersonalCtx *pCtx, eapol_packet *eapolPacket)
{
    CsrUint32 length;

    sme_trace_entry((TR_SECURITY_LIB, "begin_process_group_handshake_1"));

    /* Check that the received replay counter is greater than what we received before */
    if (isreplaygreater((CsrUint8*)eapolPacket->u.key.replay_counter, (CsrUint8*)pCtx->keyReplayCounter))
    {
        /* Copy to use later */
        CsrMemCpy(pCtx->keyReplayCounter, eapolPacket->u.key.replay_counter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH);
    }
    else
    {
        CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
        return;
    }

    length = (CsrUint32) (EAPOL_KEY_PACKET_LENGTH(eapolPacket) - CSR_WIFI_SECURITY_SNAP_LENGTH);

    /* To verify MIC, mic field should contain 0 */
    CsrMemCpy(pCtx->mic, eapolPacket->u.key.mic, CSR_WPA_MIC_LENGTH);
    CsrMemSet((CsrUint8*)eapolPacket->u.key.mic, 0, CSR_WPA_MIC_LENGTH);

    /* Save RSC from group M1 */
    CsrMemCpy(pCtx->rsc, eapolPacket->u.key.rsc, CSR_WPA_KEY_RSC_LENGTH);

    compute_mic(pCtx, eapolPacket, (CsrUint8*)&(eapolPacket->version), length, continue_process_group_handshake_1);
}
#endif


#ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
              /******* AUTHENTICATOR SPECIFIC FUNCTIONS ***********/
/******************* FUNCTIONS TO PROCESS PAIRWISE HANDSHAKE 2 *************************/

/**
 * @brief
 *   Part 7 of processing M 2
 *
 * @par Description
 *   Send the completely prepared packet
 *
 * @param
 *   void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void send_pairwise_handshake_3(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;
    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket;

    sme_trace_entry((TR_SECURITY_LIB, "send_pairwise_handshake_3"));

    eapolPacket = (eapol_packet *)pCtx->securityContext->buffer;
    CsrMemCpy((CsrUint8*)eapolPacket->u.key.mic, wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Message 3", pCtx->securityContext->buffer, pCtx->securityContext->bufferLength));

    /* Reset retransmission parameters */
    pCtx->securityContext->callbacks.stopTimer(pCtx->securityContext->externalContext,
                                               CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);
    pCtx->securityContext->callbacks.startTimer(pCtx->securityContext->externalContext, pCtx->securityContext->setupData.responseTimeout, CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);
    pCtx->securityContext->retransmissionCount = 1;

    send_packet(pCtx->securityContext);

    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}

/**
 * @brief
 *   Part 6 of processing M 2
 *
 * @par Description
 *   Adds MIC to M 3
 *
 * @param
 *   void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void add_mic_process_pairwise_handshake_2(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;

    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *rxEapolPacket = wpaCryptoCookie->wpaEapolPacket;
    CsrUint32 packetBodyLength;
    CsrUint16 cipherLength;
    CsrUint8 *pMic;
    CsrCryptoCallbackContext *newWpaCryptoCallbackContext;
    eapol_packet *txEapolPacket = (eapol_packet *)pCtx->securityContext->buffer;

    sme_trace_entry((TR_SECURITY_LIB, "add_mic_process_pairwise_handshake_2"));

    CsrPfree(pCtx->pPlainKey);

    /* Extract cipherLength */
    cipherLength = EAPOL_KEY_DATA_LENGTH_FIELD(txEapolPacket);
    CsrMemCpy(&(txEapolPacket->u.key.key_data), wpaCryptoCallbackContext->resultBuffer, cipherLength);
    packetBodyLength = (&(txEapolPacket->u.key.key_data) - &(txEapolPacket->u.key.type)) + EAPOL_KEY_DATA_LENGTH_FIELD(txEapolPacket);
    EAPOL_LENGTH_ASSIGN(txEapolPacket, (CsrUint16)packetBodyLength);

    pCtx->securityContext->bufferLength = packetBodyLength + EAPOL_HEADER_LENGTH + CSR_WIFI_SECURITY_SNAP_LENGTH;

    pMic = pCtx->cryptoResultBuffer;

    /* Compute MIC and add to the message */
    newWpaCryptoCallbackContext = prepare_crypto_context(pCtx, rxEapolPacket, pCtx->securityContext->externalContext, send_pairwise_handshake_3, pMic);
    CsrCryptoCallHmacSha1((CsrCryptoContext *)pCtx->securityContext->cryptoContext, newWpaCryptoCallbackContext, KCK(pCtx->ptk), CSR_WPA_KCK_LENGTH, pCtx->securityContext->buffer + CSR_WIFI_SECURITY_SNAP_LENGTH, pCtx->securityContext->bufferLength - CSR_WIFI_SECURITY_SNAP_LENGTH, pMic);
}

/**
 * @brief
 *   Part 5 of processing M 2
 *
 * @par Description
 *   Builds M 3
 *
 * @param
 *   CsrWpaPersonalCtx *: WPA handshake module Context
 *   eapol_packet*      : Received EAPOL Key
 *   void*              : Client context, context of the caller that will be used when making
 *                        call backs
 * @return
 *   void
 */
static void build_pairwise_handshake_3(CsrWpaPersonalCtx *pCtx, eapol_packet *rxEapolPacket)
{
    CsrUint8 *pPlain;
    CsrUint32 cipherLength, plainLength;
    CsrCryptoCallbackContext *wpaCryptoCallbackContext;
    CsrUint8 gtkkde[] = { 0x00, 0x0F, 0xAC, 0x01 };
    CsrUint8 padLength;
    eapol_packet *txEapolPacket;

    sme_trace_info((TR_SECURITY_LIB, "Enterning send_pairwise_handshake_3"));

    switch (EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0007)
    {
    case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
        pCtx->gtkLength = CSR_WPA_TKIP_GTK_LENGTH;
        break;
    case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
        pCtx->gtkLength = CSR_WPA_CCMP_GTK_LENGTH;
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(rxEapolPacket) & 0x0007));
        pCtx->gtkLength = CSR_WPA_MAX_GTK_LENGTH;
        break;
    }

    CsrWifiSecurityRandom(pCtx->gtk, pCtx->gtkLength);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "GTK", pCtx->gtk, pCtx->gtkLength));

    increment(pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH );

    /* Build message 3 */

    txEapolPacket = (eapol_packet*) pCtx->securityContext->buffer;

    CsrMemCpy(&(txEapolPacket->llc_header), pCtx->securityContext->setupData.protocolSnap, CSR_WIFI_SECURITY_SNAP_LENGTH);

    txEapolPacket->version = 0x02;
    txEapolPacket->packet_type = EAPOL_PACKET_TYPE_EAPOL_KEY;

    txEapolPacket->u.key.type         = rxEapolPacket->u.key.type;
    txEapolPacket->u.key.key_info_hi  = 0x13;
    txEapolPacket->u.key.key_info_low = 0xca;

    EAPOL_KEY_LENGTH_ASSIGN(txEapolPacket, 0x10); /*lint !e572 */
    CsrMemCpy(txEapolPacket->u.key.replay_counter, pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH);
    CsrMemCpy(txEapolPacket->u.key.nonce, pCtx->nonce, CSR_WPA_NONCE_LENGTH);
    CsrMemSet(txEapolPacket->u.key.iv, 0, CSR_WPA_KEY_IV_LENGTH);
    CsrMemSet(txEapolPacket->u.key.rsc, 0, CSR_WPA_KEY_RSC_LENGTH);
    CsrMemSet(txEapolPacket->u.key.reserved, 0, CSR_WPA_KEY_RESERVED_LENGTH );
    CsrMemSet(txEapolPacket->u.key.mic, 0, CSR_WPA_MIC_LENGTH );

    pCtx->pPlainKey = (CsrUint8*) CsrPmalloc(pCtx->securityContext->setupData.secIeLen + sizeof(gtkkde) + pCtx->gtkLength
            + 20);

    /* Create hardwired plaintext buffer with GTK */
    pPlain = pCtx->pPlainKey;

    CsrMemCpy(pPlain, pCtx->securityContext->setupData.secIe, pCtx->securityContext->setupData.secIeLen);
    pPlain += pCtx->securityContext->setupData.secIeLen;

    *pPlain++ = 0xdd;
    *pPlain++ = (CsrUint8) pCtx->gtkLength + 6; /* Length */

    CsrMemCpy(pPlain, gtkkde, sizeof(gtkkde));
    pPlain += sizeof(gtkkde);
    *pPlain++ = 0; /* Key ID and Tx bits set to 0 currently */
    *pPlain++ = 0;
    CsrMemCpy( pPlain, pCtx->gtk, pCtx->gtkLength );
    pPlain += pCtx->gtkLength;

    plainLength = (CsrUint32) (pPlain - pCtx->pPlainKey);

    cipherLength = plainLength;
    padLength = (CsrUint8)(cipherLength % 8);

    if (padLength)
    {
        padLength = 8 - padLength;
        *pPlain++ = 0xdd;
        *pPlain++ = 0x0;
    }
    cipherLength += padLength + 8;

    EAPOL_KEY_DATA_LENGTH_ASSIGN(txEapolPacket, (CsrUint16)cipherLength);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Plain key data", pCtx->pPlainKey, plainLength));

    wpaCryptoCallbackContext = prepare_crypto_context(pCtx, rxEapolPacket, pCtx->securityContext->externalContext, add_mic_process_pairwise_handshake_2, pCtx->cryptoResultBuffer);
    CsrCryptoCallAes128Wrap((CsrCryptoContext *)pCtx->securityContext->cryptoContext, wpaCryptoCallbackContext, KEK(pCtx->ptk), (cipherLength - 8)/ 8, pCtx->pPlainKey, pCtx->cryptoResultBuffer);
}

/**
 * @brief
 *   Part 4 of processing M 2
 *
 * @par Description
 *   Verify the MIC
 *
 * @param
 *   void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void verify_mic_process_pairwise_handshake_2(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrUint8 micPass = FALSE;
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;

    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = wpaCryptoCookie->wpaEapolPacket;

    sme_trace_entry((TR_SECURITY_LIB, "verify_mic_process_pairwise_handshake_2"));

    /* Restore the MIC */
    CsrMemCpy((CsrUint8*)eapolPacket->u.key.mic, pCtx->mic, CSR_WPA_MIC_LENGTH);

    if (CsrMemCmp((CsrUint8*)eapolPacket->u.key.mic, wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH ) == 0)
    {
        sme_trace_info((TR_SECURITY_LIB, "MIC verified OK"));
        micPass = TRUE;
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "MIC error"));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Received MIC", eapolPacket->u.key.mic, CSR_WPA_MIC_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Computed MIC", wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH));
    }

    if (micPass)
    {
        build_pairwise_handshake_3(pCtx, eapolPacket);
    }
    else
    {
        CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
    }
}

/**
 * @brief
 *   Part 3 of processing M 2
 *
 * @par Description
 *   Store they keys and compute MIC
 *
 * @param
 *   void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void continue_process_pairwise_handshake_2(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;

    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = wpaCryptoCookie->wpaEapolPacket;
    CsrUint32 length = EAPOL_KEY_PACKET_LENGTH(eapolPacket) - CSR_WIFI_SECURITY_SNAP_LENGTH;

    sme_trace_info((TR_SECURITY_LIB, "Entering continue_process_pairwise_handshake_2"));

    switch (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007)
    {
    case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "PTK", pCtx->ptk, CSR_WPA_TKIP_PTK_LENGTH));
        break;
    case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "PTK", pCtx->ptk, CSR_WPA_CCMP_PTK_LENGTH));
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unknown Key Descriptor Version (%d)", EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0007));
        break;
    }

    /* To verify MIC, mic field should contain 0 */
    CsrMemCpy(pCtx->mic, eapolPacket->u.key.mic, CSR_WPA_MIC_LENGTH);
    CsrMemSet((CsrUint8*)eapolPacket->u.key.mic, 0, CSR_WPA_MIC_LENGTH);

    compute_mic(pCtx, eapolPacket, (CsrUint8*)&(eapolPacket->version), length, verify_mic_process_pairwise_handshake_2);
}

/**
 * @brief
 *   Part 2 of processing M 2
 *
 * @par Description
 *   Derive the unicast keys from the packet
 *
 * @param
 *   void*              : Contains WPA handshake module context
 *
 * @return
 *   void
 */
static void continue_hmac_sha1_process_pairwise_handshake_2(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;
    CsrCryptoCallbackContext *newWpaCryptoCallbackContext;
    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = wpaCryptoCookie->wpaEapolPacket;
    CsrUint8 *pCount, *pPtk = pCtx->ptk;

    sme_trace_info((TR_SECURITY_LIB, "Entering continue_hmac_sha1_pairwise_handshake_2"));

    pCtx->hmacSha1Loop++;

    pCount = pCtx->seed + pCtx->seedLength -1;
    *pCount = (CsrUint8) pCtx->hmacSha1Loop;

    if (pCtx->hmacSha1Loop < (pCtx->hmacSha1TotalLoops - 1))
    {
        /* More hmac_sha1 processing required after current call to hmac_sha1 */
        newWpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, continue_hmac_sha1_process_pairwise_handshake_2, NULL);
    }
    else
    {
        /* Last call to hmac_sha1 */
        newWpaCryptoCallbackContext = prepare_crypto_context(pCtx, eapolPacket, pCtx->securityContext->externalContext, continue_process_pairwise_handshake_2, NULL);
    }

    CsrCryptoCallHmacSha1((CsrCryptoContext *)pCtx->securityContext->cryptoContext, newWpaCryptoCallbackContext, pCtx->securityContext->setupData.pmk, CSR_WPA_PMK_LENGTH, pCtx->seed, pCtx->seedLength, pPtk + (pCtx->hmacSha1Loop * CSR_SHA1_DIGEST_LENGTH));
}

/**
 * @brief
 *   Part 1 of processing M 2
 *
 * @par Description
 *   Derive the unicast keys from the packet
 *
 * @param
 *   CsrWpaPersonalCtx *: WPA handshake module Context
 *   eapol_packet*      : Received EAPOL Key
 *   void*              : Client context, context of the caller that will be used when making
 *                        call backs
 * @return
 *   void
 */
static void begin_process_pairwise_handshake_2(CsrWpaPersonalCtx *pCtx, eapol_packet *eapolPacket)
{
    sme_trace_info((TR_SECURITY_LIB, "Entering process_pairwise_handshake_2"));

    derive_keys(pCtx, eapolPacket, continue_hmac_sha1_process_pairwise_handshake_2);
}

/******************* FUNCTIONS TO PROCESS PAIRWISE HANDSHAKE 4 *************************/

/**
 * @brief
 *   Part 2 of processing M 4
 *
 * @par Description
 *   Verify MIC and set the keys
 *
 * @param
 *   void *             : Contains WPA handshake module Context
 *
 * @return
 *   void
 */
static void continue_process_pairwise_handshake_4(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrUint8 micPass = FALSE;
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;

    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = wpaCryptoCookie->wpaEapolPacket;

    sme_trace_info((TR_SECURITY_LIB, "Entering continue_process_pairwise_handshake_4"));

    CsrMemCpy((CsrUint8*)eapolPacket->u.key.mic, pCtx->mic, CSR_WPA_MIC_LENGTH);

    if (CsrMemCmp((CsrUint8*)eapolPacket->u.key.mic, wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH ) == 0)
    {
        sme_trace_info((TR_SECURITY_LIB, "MIC verified OK"));
        micPass = TRUE;
    }
    else
    {
        sme_trace_info((TR_SECURITY_LIB, "MIC error"));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Received MIC", eapolPacket->u.key.mic, CSR_WPA_MIC_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Computed MIC", wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH));
    }

    if (micPass)
    {
        /* Reset retransmission parameters */
        pCtx->securityContext->callbacks.stopTimer(pCtx->securityContext->externalContext,
                                                   CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);
        pCtx->securityContext->retransmissionCount = 0;

        /* Install the pairwise Temporal Key and GTK */
        pCtx->securityContext->callbacks.installPairwiseKey(pCtx->securityContext->externalContext,
                                                            CSR_WIFI_SECURITY_KEYTYPE_IEEE80211I, TK(pCtx->ptk), 16,
                                                            NULL, 0);
        pCtx->securityContext->callbacks.installGroupKey(pCtx->securityContext->externalContext,
                                                         CSR_WIFI_SECURITY_KEYTYPE_IEEE80211I, pCtx->gtk, 16, NULL, 1);

        /* Delete Config SA timeout */
        pCtx->securityContext->callbacks.stopTimer(pCtx->securityContext->externalContext,
                                                   CSR_WIFI_SECURITY_TIMEOUT_SECURITY_ASSOCIATION);
    }

    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}

/**
 * @brief
 *   Part 1 of processing M 4
 *
 * @par Description
 *   Computes MIC on the packet
 *
 * @param
 *   CsrWpaPersonalCtx *: WPA handshake module Context
 *   eapol_packet*      : Received EAPOL Key
 *   void*              : Client context, context of the caller that will be used when making
 *                        call backs
 * @return
 *   void
 */
static void begin_process_pairwise_handshake_4(CsrWpaPersonalCtx *pCtx, eapol_packet *eapolPacket)
{
    CsrUint32 length;

    sme_trace_info((TR_SECURITY_LIB, "Entering begin_process_pairwise_handshake_4"));

    length = (CsrUint32) (EAPOL_KEY_PACKET_LENGTH(eapolPacket) - CSR_WIFI_SECURITY_SNAP_LENGTH);

    /* To verify MIC, mic field should contain 0 */
    CsrMemCpy(pCtx->mic, eapolPacket->u.key.mic, CSR_WPA_MIC_LENGTH);
    CsrMemSet((CsrUint8*)eapolPacket->u.key.mic, 0, CSR_WPA_MIC_LENGTH);

    compute_mic(pCtx, eapolPacket, (CsrUint8*)&(eapolPacket->version), length, continue_process_pairwise_handshake_4);
}

/**
 * @brief
 *   Form M 1 of 4 and send the packet
 *
 * @par Description
 *   Form M 1 of 4 and send the packet
 *
 * @param
 *   CsrWpaPersonalCtx *: WPA handshake module Context
 *   CsrUint8*             : MAC address of the peer device
 *   void*              : Client context, context of the caller that will be used when making
 *                        call backs
 * @return
 *   void
 */
static void send_pairwise_handshake_1(CsrWpaPersonalCtx *pCtx, const CsrUint8 *pPeerMac)
{
    eapol_packet *eapolPacket;
    CsrUint32 packetBodyLength;

    sme_trace_info((TR_SECURITY_LIB, "Entering send_pairwise_handshake_1"));

    CsrWifiSecurityRandom(pCtx->nonce, CSR_WPA_NONCE_LENGTH );
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Local Nonce", pCtx->nonce, CSR_WPA_NONCE_LENGTH));

    increment(pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH );

    /* Build message 1 */
    eapolPacket = (eapol_packet*) pCtx->securityContext->buffer;

    CsrMemCpy(&(eapolPacket->llc_header), pCtx->securityContext->setupData.protocolSnap, CSR_WIFI_SECURITY_SNAP_LENGTH);

    eapolPacket->version = 0x02;
    eapolPacket->packet_type = EAPOL_PACKET_TYPE_EAPOL_KEY;

    eapolPacket->u.key.type         = 0x02;
    eapolPacket->u.key.key_info_hi  = 0x00;
    eapolPacket->u.key.key_info_low = 0x8a;

    EAPOL_KEY_LENGTH_ASSIGN(eapolPacket, 0x10); /*lint !e572 */
    CsrMemCpy(eapolPacket->u.key.replay_counter, pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH);
    CsrMemCpy(eapolPacket->u.key.nonce, pCtx->nonce, CSR_WPA_NONCE_LENGTH);
    CsrMemSet(eapolPacket->u.key.iv, 0, CSR_WPA_KEY_IV_LENGTH);
    CsrMemSet(eapolPacket->u.key.rsc, 0, CSR_WPA_KEY_RSC_LENGTH);
    CsrMemSet(eapolPacket->u.key.reserved, 0, CSR_WPA_KEY_RESERVED_LENGTH );
    CsrMemSet(eapolPacket->u.key.mic, 0, CSR_WPA_MIC_LENGTH );
    EAPOL_KEY_DATA_LENGTH_ASSIGN(eapolPacket, 0); /*lint !e572 */
    packetBodyLength = (&(eapolPacket->u.key.key_data) - &(eapolPacket->u.key.type)) + EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket);
    EAPOL_LENGTH_ASSIGN(eapolPacket, (CsrUint16)packetBodyLength);
    pCtx->securityContext->bufferLength = packetBodyLength + EAPOL_HEADER_LENGTH + CSR_WIFI_SECURITY_SNAP_LENGTH;

    /* start timer */
    pCtx->securityContext->callbacks.startTimer(pCtx->securityContext->externalContext, pCtx->securityContext->setupData.responseTimeout, CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);
    pCtx->securityContext->retransmissionCount = 1;

    send_packet(pCtx->securityContext);
}

#endif /* CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE */

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_handshake.h
 */
/*---------------------------------------------------------------------------*/

CsrWpaPersonalCtx* csr_handshake_init(CsrWifiSecurityContext* context)
{
    CsrWpaPersonalCtx *pCtx;

    sme_trace_entry((TR_SECURITY_LIB, "csr_handshake_init"));

    sme_trace_info((TR_SECURITY_LIB, "CsrWpaPersonalCtx = %d bytes", sizeof(CsrWpaPersonalCtx)));

    pCtx =  (CsrWpaPersonalCtx*) CsrPmalloc(sizeof(CsrWpaPersonalCtx));
    pCtx->securityContext = context;

    switch (context->setupData.mode)
    {
    case CSR_WIFI_SECURITY_WPAPSK_MODE_SUPPLICANT:
        sme_trace_info((TR_SECURITY_LIB, "Supplicant mode"));
        break;
    case CSR_WIFI_SECURITY_WPAPSK_MODE_AUTHENTICATOR:
        sme_trace_info((TR_SECURITY_LIB, "Authenticator mode"));
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unknown mode (%d)", context->setupData.mode));
        break;
    }

    sme_trace_info((TR_SECURITY_LIB, "psendPacket = %p", pCtx->securityContext->callbacks.sendPacket));
    sme_trace_info((TR_SECURITY_LIB, "pInstallPairwiseKey = %p", pCtx->securityContext->callbacks.installPairwiseKey));
    sme_trace_info((TR_SECURITY_LIB, "pInstallGroupKey = %p", pCtx->securityContext->callbacks.installGroupKey));
    sme_trace_info((TR_SECURITY_LIB, "pStartTimer = %p", pCtx->securityContext->callbacks.startTimer));
    sme_trace_info((TR_SECURITY_LIB, "pStopTimer = %p", pCtx->securityContext->callbacks.stopTimer));
    sme_trace_info((TR_SECURITY_LIB, "pAbortProcedure = %p", pCtx->securityContext->callbacks.abortProcedure));

    /* Message length of last transmitted handshake */
    pCtx->securityContext->retransmissionCount = 0;

    CsrMemSet(pCtx->keyReplayCounter, 0, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH );

    pCtx->gtkAvailable = FALSE;

    return pCtx;
}

/*
 * Description:
 * See description in csr_handshake.h
 */
/*---------------------------------------------------------------------------*/
void csr_handshake_start(CsrWpaPersonalCtx *pCtx)
{
    sme_trace_entry((TR_SECURITY_LIB, "csr_handshake_start"));

    switch (pCtx->securityContext->setupData.mode)
    {
    case CSR_WIFI_SECURITY_WPAPSK_MODE_SUPPLICANT:
        sme_trace_info((TR_SECURITY_LIB, "Supplicant start"));
        break;
    case CSR_WIFI_SECURITY_WPAPSK_MODE_AUTHENTICATOR:
        #ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
        sme_trace_info((TR_SECURITY_LIB, "Authenticator start"));
        send_pairwise_handshake_1(pCtx, pCtx->securityContext->setupData.peerMacAddress);
        #else
        sme_trace_error((TR_SECURITY_LIB, "AUTHENTICATOR functionality not compiled in the build"));
        #endif
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unknown mode (%d)", pCtx->securityContext->setupData.mode));
        break;
    }
}

/*
 * Description:
 * See description in csr_handshake.h
 */
/*---------------------------------------------------------------------------*/
void csr_handshake_pkt_input(CsrWpaPersonalCtx *pCtx, eapol_packet *eapolPacket, CsrUint16 length)
{
    sme_trace_entry((TR_SECURITY_LIB, "csr_wpa_process_eapol_key"));

    dump_eapol_key(eapolPacket);

    if (!(EAPOL_KEY_INFO_FIELD((eapolPacket)) & 0x0008))
    {
        sme_trace_info((TR_SECURITY_LIB, "Group M1 received"));
        #ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE
        begin_process_group_handshake_1(pCtx, eapolPacket);
        #else
        sme_trace_error((TR_SECURITY_LIB, "Supplicant functionality not present in the build"));
        #endif
    }
    else
    {
        if (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0100) /* MIC present */
        {
            if (EAPOL_KEY_INFO_FIELD(eapolPacket) & 0x0080) /* Ack present */
            {
                sme_trace_info((TR_SECURITY_LIB, "M3 received"));
                #ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE
                begin_process_pairwise_handshake_3(pCtx, eapolPacket);
                #else
                sme_trace_error((TR_SECURITY_LIB, "Supplicant functionality not present in the build"));
                #endif
            }
            else
            {
                /* Either M2 or M4. M2 has key data, M4 does not */
                if (EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket))
                {
                    sme_trace_info((TR_SECURITY_LIB, "M2 received"));
                    #ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
                    begin_process_pairwise_handshake_2(pCtx, eapolPacket);
                    #else
                    sme_trace_error((TR_SECURITY_LIB, "AUTHENTICATOR functionality not present in the build"));
                    #endif
                }
                else
                {
                    sme_trace_info((TR_SECURITY_LIB, "M4 received"));
                    #ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
                    begin_process_pairwise_handshake_4(pCtx, eapolPacket);
                    #else
                    sme_trace_error((TR_SECURITY_LIB, "AUTHENTICATOR functionality not present in the build"));
                    #endif
                }
            }
        }
        else
        {
            sme_trace_info((TR_SECURITY_LIB, "M1 received"));
            #ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE
            begin_process_pairwise_handshake_1(pCtx, eapolPacket);
            #else
            sme_trace_error((TR_SECURITY_LIB, "Supplicant functionality not present in the build"));
            #endif
        }
    }
}

/*
 * Description:
 * See description in csr_handshake.h
 */
/*---------------------------------------------------------------------------*/
void csr_handshake_timeout(CsrWpaPersonalCtx *pCtx, CsrWifiSecurityTimeout timeoutIndex)
{
    sme_trace_info((TR_SECURITY_LIB, "csr_wpa_timer_expired():timer expiry notification"));

    if (timeoutIndex == CSR_WIFI_SECURITY_TIMEOUT_SECURITY_ASSOCIATION)
    {
        sme_trace_error((TR_SECURITY_LIB, "Config SA Timeout: Aborting handshake"));
        pCtx->securityContext->callbacks.abortProcedure(pCtx->securityContext->externalContext);
    }
    else if (timeoutIndex == CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY)
    {
        if (!pCtx->securityContext->retransmissionCount)
        {
            sme_trace_info((TR_SECURITY_LIB, "csr_wpa_timer_expired():Unexpected timer expiry notification"));
        }
        else
        {
            sme_trace_info((TR_SECURITY_LIB, "csr_wpa_timer_expired():retransmission count - %d", pCtx->securityContext->retransmissionCount));
            if (pCtx->securityContext->retransmissionCount > pCtx->securityContext->setupData.retransmissionAttempts)
            {
                sme_trace_info((TR_SECURITY_LIB, "csr_wpa_timer_expired():MAX Retry count reached. Abort the procedure"));
                pCtx->securityContext->callbacks.abortProcedure(pCtx->securityContext->externalContext);
            }
            else
            {
                sme_trace_info((TR_SECURITY_LIB, "csr_wpa_timer_expired():Retransmitting handshake message"));
                /* start timer */
                pCtx->securityContext->callbacks.startTimer(pCtx->securityContext->externalContext,
                                                            pCtx->securityContext->setupData.responseTimeout,
                                                            CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY);
                pCtx->securityContext->retransmissionCount++;

                send_packet(pCtx->securityContext);
            }
        }
    }
}

/**
 * @brief
 *   Form M 1 of 4 and send the packet
 *
 * @par Description
 *   Form M 1 of 4 and send the packet
 *
 * @param
 *   CsrWpaPersonalCtx *: WPA handshake module Context
 *   CsrUint8*             : MAC address of the peer device
 *   void*              : Client context, context of the caller that will be used when making
 *                        call backs
 * @return
 *   void
 */
static void send_mic_failure_error(CsrCryptoCallbackContext *wpaCryptoCallbackContext)
{
    CsrWpaCryptoCookie *wpaCryptoCookie = wpaCryptoCallbackContext->context;
    CsrWpaPersonalCtx *pCtx = wpaCryptoCookie->wpaPersonalContext;
    eapol_packet *eapolPacket = wpaCryptoCookie->wpaEapolPacket;

    /* Copy MIC from the result buffer */
    CsrMemCpy((CsrUint8*)eapolPacket->u.key.mic, wpaCryptoCallbackContext->resultBuffer, CSR_WPA_MIC_LENGTH);

    sme_trace_info((TR_SECURITY_LIB, "Trying to send packet len %d, send %x:%x:%x:%x:%x:%x\n", pCtx->securityContext->bufferLength,
      pCtx->securityContext->setupData.peerMacAddress[0], pCtx->securityContext->setupData.peerMacAddress[1], pCtx->securityContext->setupData.peerMacAddress[2], pCtx->securityContext->setupData.peerMacAddress[3], pCtx->securityContext->setupData.peerMacAddress[4], pCtx->securityContext->setupData.peerMacAddress[5]));

    send_packet(pCtx->securityContext);
}

/*
 * Description:
 * See description in csr_handshake.h
 */
/*---------------------------------------------------------------------------*/
void csr_handshake_send_eapol_error (CsrWpaPersonalCtx *pCtx,  CsrBool pairwise, CsrUint8 *tsc)
{
    eapol_packet *eapolPacket;
    CsrUint32 packetBodyLength;
    CsrUint16 keyInfo;

    sme_trace_info((TR_SECURITY_LIB, "Entering csr_handshake_send_eapol_error"));

    eapolPacket = (eapol_packet*) pCtx->securityContext->buffer;

    CsrMemSet(eapolPacket, 0, sizeof(eapol_packet));

    CsrMemCpy(&(eapolPacket->llc_header), pCtx->securityContext->setupData.protocolSnap, CSR_WIFI_SECURITY_SNAP_LENGTH);

    eapolPacket->version = 0x02;
    eapolPacket->packet_type = EAPOL_PACKET_TYPE_EAPOL_KEY;

    eapolPacket->u.key.type = 0x02;

    switch (pCtx->keyInfo)
    {
    case CSR_WPA_KEY_DESCRIPTOR_MD5_ARC4:
        keyInfo = 0x0f01;
        break;
    case CSR_WPA_KEY_DESCRIPTOR_SHA1_AES:
        keyInfo = 0x0f02;
        break;
    default:
        sme_trace_error((TR_SECURITY_LIB, "!!!!!!!Unknown Key info, default used = %d!!!!!!!", pCtx->keyInfo));
        keyInfo = 0x0f01;
        break;
    }

    if (pairwise)
    {
        keyInfo |= 0x8;
    }
    EAPOL_KEY_INFO_ASSIGN(eapolPacket, keyInfo);
    EAPOL_KEY_LENGTH_ASSIGN(eapolPacket, 0x0); /*lint !e572 */

    increment(pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH );
    CsrMemCpy(eapolPacket->u.key.replay_counter, pCtx->keyReplayCounter, CSR_WPA_KEY_REPLAY_COUNTER_LENGTH);
    CsrMemCpy(eapolPacket->u.key.rsc, tsc, CSR_WPA_KEY_RSC_LENGTH);
    EAPOL_KEY_DATA_LENGTH_ASSIGN(eapolPacket, 0); /*lint !e572 */
    packetBodyLength = (&(eapolPacket->u.key.key_data) - &(eapolPacket->u.key.type)) + EAPOL_KEY_DATA_LENGTH_FIELD(eapolPacket);
    EAPOL_LENGTH_ASSIGN(eapolPacket, (CsrUint16)packetBodyLength);
    pCtx->securityContext->bufferLength = packetBodyLength + EAPOL_HEADER_LENGTH + CSR_WIFI_SECURITY_SNAP_LENGTH;

    compute_mic(pCtx, eapolPacket, pCtx->securityContext->buffer + CSR_WIFI_SECURITY_SNAP_LENGTH, pCtx->securityContext->bufferLength - CSR_WIFI_SECURITY_SNAP_LENGTH, send_mic_failure_error);
}

/*
 * Description:
 * See description in csr_handshake.h
 */
/*---------------------------------------------------------------------------*/

void csr_handshake_deinit(CsrWpaPersonalCtx *pCtx)
{
    sme_trace_entry((TR_SECURITY_LIB, "csr_handshake_deinit"));
    CsrPfree(pCtx);
}
