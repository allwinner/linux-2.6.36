/** @file csr_security.c
 *
 * Implementation of the security module.
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
 *   This provides the core implementation of the security module
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_security.c#4 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/
/* PROJECT INCLUDES *********************************************************/

#include "csr_types.h"
#include "csr_pmalloc.h"
#include "csr_random.h"
#include "csr_panic.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "csr_cstl/csr_wifi_list.h"

#ifdef CSR_WIFI_SECURITY_PMK_CACHING_ENABLE
#include "csr_crypto.h"
#endif

#include "csr_security_private.h"

#ifdef CSR_WIFI_SECURITY_EAP_ENABLE
#include "csr_eap/csr_eap.h"
#endif

#if defined CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE || defined CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
#include "csr_handshake/csr_handshake.h"
#endif

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
#include "csr_wapi/csr_wapi.h"
#endif

#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
#include "csr_wps/csr_wps.h"
#endif

/* MACROS *******************************************************************/
/* GLOBAL VARIABLE DEFINITIONS **********************************************/
/* PRIVATE TYPES DEFINITIONS ************************************************/
/* PRIVATE CONSTANT DEFINITIONS *********************************************/
const CsrUint8 eapolSnapHeader[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e};
const CsrUint8 preauthSnapHeader[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0xc7};
const CsrUint8 wapiSnapHeader[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0xb4};
const CsrUint8 btAmpSnapHeader[] = {0xaa, 0xaa, 0x03, 0x00, 0x19, 0x58, 0x00, 0x03};

/* PRIVATE VARIABLE DEFINITIONS *********************************************/
/* PRIVATE FUNCTION PROTOTYPES **********************************************/
/* PRIVATE FUNCTION DEFINITIONS *********************************************/

CsrUint8 convertSecurityTypeToEapID(CsrWifiSecurityType securityType)
{
    switch (securityType)
    {
    case CSR_WIFI_SECURITY_TYPE_NONE:       return EAP_TYPE_NONE;
    case CSR_WIFI_SECURITY_TYPE_TLS:        return EAP_TYPE_TLS;
    case CSR_WIFI_SECURITY_TYPE_TTLS:       return EAP_TYPE_TTLS;
    case CSR_WIFI_SECURITY_TYPE_LEAP:       return EAP_TYPE_LEAP;
    case CSR_WIFI_SECURITY_TYPE_FAST:       return EAP_TYPE_EAPFAST;
    case CSR_WIFI_SECURITY_TYPE_PEAP:       return EAP_TYPE_PEAP;
    case CSR_WIFI_SECURITY_TYPE_GTC:        return EAP_TYPE_GTC;
    case CSR_WIFI_SECURITY_TYPE_MSCHAPV2:   return EAP_TYPE_MSCHAPV2;
    case CSR_WIFI_SECURITY_TYPE_SIM:        return EAP_TYPE_SIM;
    case CSR_WIFI_SECURITY_TYPE_WPS:
    default: return EAP_TYPE_NONE;
    }
}

static CsrBool validate_security_input(const CsrWifiSecuritySetup* setupData,
                                const CsrWifiSecuritycallbacks* callbacks)
{
    CsrBool ret;

    do
    {
        ret = FALSE;

        /* Validate necessary data for all security types */
        if (setupData == NULL)
        {
            sme_trace_error((TR_SECURITY_LIB, "setupData is NULL"));
            break;
        }
        if (callbacks == NULL)
        {
            sme_trace_error((TR_SECURITY_LIB, "callbacks is NULL"));
            break;
        }

        if (callbacks->abortProcedure == NULL)
        {
            sme_trace_error((TR_SECURITY_LIB, "abortProcedure is NULL"));
            break;
        }
        if (callbacks->installGroupKey == NULL)
        {
            sme_trace_error((TR_SECURITY_LIB, "installGroupKey is NULL"));
            break;
        }
        if (callbacks->installPairwiseKey == NULL)
        {
            sme_trace_error((TR_SECURITY_LIB, "installPairwiseKey is NULL"));
            break;
        }
        if (callbacks->sendPacket == NULL)
        {
            sme_trace_error((TR_SECURITY_LIB, "sendPacket is NULL"));
            break;
        }
        if (callbacks->startTimer == NULL)
        {
            sme_trace_error((TR_SECURITY_LIB, "startTimer is NULL"));
            break;
        }
        if (callbacks->stopTimer == NULL)
        {
            sme_trace_error((TR_SECURITY_LIB, "stopTimer is NULL"));
            break;
        }
        if (callbacks->packetProcessingDone == NULL)
        {
            sme_trace_error((TR_SECURITY_LIB, "packetProcessingDone is NULL"));
            break;
        }
        /* General inputs required for all modules */
        if (setupData->protocolSnapValid != TRUE)
        {
            sme_trace_error((TR_SECURITY_LIB, "Protocol SNAP required"));
            break;
        }
        if (setupData->responseTimeout == 0)
        {
            sme_trace_error((TR_SECURITY_LIB, "Non zero response timeout required"));
            break;
        }

        ret = TRUE;

        switch(setupData->securityType)
        {
            case CSR_WIFI_SECURITY_TYPE_TLS:

                if ((setupData->identity == NULL) || (0 == CsrStrLen(setupData->identity)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Identity is required"));
                    break;
                }
                if (callbacks->installSession == NULL)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "installSession is NULL"));
                    break;
                }
                if (setupData->clientCertificateLength == 0)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "clientCertificateLength is 0"));
                    break;
                }
                if (setupData->clientPrivateKeyLength == 0)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "clientPrivateKeyLength is 0"));
                    break;
                }
                break;

            case CSR_WIFI_SECURITY_TYPE_TTLS:
                if ((setupData->identity == NULL) || (0 == CsrStrLen(setupData->identity)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Identity is required"));
                    break;
                }
                if (callbacks->installSession == NULL)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "installSession is NULL"));
                    break;
                }
                if ((setupData->password == NULL) || (0 == CsrStrLen(setupData->password)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Password is required"));
                    break;
                }
                if ((setupData->username == NULL) || (0 == CsrStrLen(setupData->username)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Username is required"));
                    break;
                }
                break;

            case CSR_WIFI_SECURITY_TYPE_LEAP:
                if ((setupData->identity == NULL) || (0 == CsrStrLen(setupData->identity)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Identity is required"));
                    break;
                }
                if ((setupData->password == NULL) || (0 == CsrStrLen(setupData->password)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Password is required"));
                    break;
                }
                if ((setupData->username == NULL) || (0 == CsrStrLen(setupData->username)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Username is required"));
                    break;
                }

                break;
            case CSR_WIFI_SECURITY_TYPE_FAST:
                if (callbacks->installPac == NULL)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "installPac is NULL"));
                    break;
                }
                if ((setupData->identity == NULL) || (0 == CsrStrLen(setupData->identity)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Identity is required"));
                    break;
                }
                if ((setupData->password == NULL) || (0 == CsrStrLen(setupData->password)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Password is required"));
                    break;
                }
                if ((setupData->username == NULL) || (0 == CsrStrLen(setupData->username)))
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "Username is required"));
                    break;
                }
                break;
            case CSR_WIFI_SECURITY_TYPE_WPS:
                if (callbacks->wpsDone == NULL)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "wpsDone is NULL"));
                    break;
                }
                break;

            case CSR_WIFI_SECURITY_TYPE_WPAPSK_AUTHENTICATOR:
            case CSR_WIFI_SECURITY_TYPE_WPAPSK_SUPPLICANT:
            case CSR_WIFI_SECURITY_TYPE_WAPIPSK_AE:
            case CSR_WIFI_SECURITY_TYPE_WAPIPSK_ASUE:
                if (setupData->pmkValid != TRUE)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "PMK is required"));
                    break;
                }
                break;
            case CSR_WIFI_SECURITY_TYPE_WAPI_ASUE:
                if (setupData->clientCertificateLength == 0)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "clientCertificateLength is 0"));
                    break;
                }
                if (setupData->clientPrivateKeyLength == 0)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "clientPrivateKeyLength is 0"));
                    break;
                }
                if (setupData->authenticationServerCertificateLength == 0)
                {
                    ret = FALSE;
                    sme_trace_error((TR_SECURITY_LIB, "authenticationServerCertificateLength is 0"));
                    break;
                }
                break;
            case CSR_WIFI_SECURITY_TYPE_SIM:
                break;

            case CSR_WIFI_SECURITY_TYPE_PEAP:
            case CSR_WIFI_SECURITY_TYPE_GTC:
            case CSR_WIFI_SECURITY_TYPE_MSCHAPV2:
            case CSR_WIFI_SECURITY_TYPE_AKA:
            case CSR_WIFI_SECURITY_TYPE_NONE:
            {
                ret = FALSE;
                sme_trace_error((TR_SECURITY_LIB, "Security type Not implemented"));
                break;
            }
        }

        if (ret == FALSE)
        {
            break;
        }

        /* For LEAP and WPS there is no key handshake procedure and hence no need of secIE */
        if ((setupData->securityType != CSR_WIFI_SECURITY_TYPE_LEAP) &&
            (setupData->securityType != CSR_WIFI_SECURITY_TYPE_WPS) &&
                (setupData->secIeLen == 0))
        {
            ret = FALSE;
            sme_trace_error((TR_SECURITY_LIB, "Security IE is required"));
            break;
        }

    }while (0);

    if (ret != TRUE)
    {
        sme_trace_error((TR_SECURITY_LIB, "Validation of setup data failed"));
    }

    return ret;
}

static CsrBool validate_input_packet(CsrWifiSecurityContext* context, const CsrUint8* packetBuffer, const CsrUint32 packetBufferLength)
{
    CsrBool ret = FALSE;
    CsrUint16 length;

    do
    {
        if ((context == NULL) || (packetBuffer == NULL) || (packetBufferLength == 0))
        {
            sme_trace_warn((TR_SECURITY_LIB, "Initial validation failed"));
            break;
        }

        /* Packet buffer size restricted */
        if (packetBufferLength > CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE + CSR_WIFI_SECURITY_SNAP_LENGTH)
        {
            sme_trace_warn((TR_SECURITY_LIB, "Packet buffer length %d too long, max is %d", packetBufferLength,
                    CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE + CSR_WIFI_SECURITY_SNAP_LENGTH));
            break;
        }

        if ((0 == CsrMemCmp(btAmpSnapHeader, packetBuffer, sizeof(btAmpSnapHeader)))
                || (0 == CsrMemCmp(eapolSnapHeader, packetBuffer, sizeof(eapolSnapHeader))))
        {
            length = EAPOL_LENGTH_FIELD((eapol_packet*)packetBuffer);
        }
        #ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
        else
        if (0 == CsrMemCmp(wapiSnapHeader, packetBuffer, sizeof(wapiSnapHeader)))
        {
            length = WAPI_PACKET_LENGTH((wapi_packet*)packetBuffer);
        }
        #endif
        else
        {
            sme_trace_warn((TR_SECURITY_LIB, "Unknown LLC protocol type: Discarding packet"));
            break;
        }

        if (length > packetBufferLength)
        {
            /* Malformed packet */
            sme_trace_warn((TR_SECURITY_LIB, "Packet buffer length input less than length in the EAPOL packet"));
            break;
        }

        ret = TRUE;

    } while (0);

    if (ret == FALSE)
    {
        sme_trace_warn((TR_SECURITY_LIB, "Packet validation failed"));
    }

    return ret;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_security.h
 */
void CsrWifiSecurityRandom(CsrUint8* buff, CsrUint32 length)
{
    CsrUint16 i;
    for (i = 0; i < length; i++)
    {
        buff[i] = (CsrUint8) CsrRandom();
    }
}

/*
 * Description:
 * See description in csr_security.h
 */
CsrWifiSecurityContext* CsrWifiSecurityInit(void* externalContext,
                                      struct CsrCryptoContext* cryptoContext,
                                      const CsrWifiSecuritySetup* setupData,
                                      const CsrWifiSecuritycallbacks* callbacks)
{
    CsrWifiSecurityContext* securityContext;

    sme_trace_entry((TR_SECURITY_LIB, "CsrWifiSecurityInit()"));

    if (FALSE == validate_security_input(setupData, callbacks))
    {
        return NULL;
    }

    securityContext = (CsrWifiSecurityContext*) CsrPmalloc(sizeof(CsrWifiSecurityContext));
    CsrMemSet(securityContext, 0, sizeof(CsrWifiSecurityContext));
    csr_list_init(&securityContext->inputQueue);

    securityContext->externalContext = externalContext;
    securityContext->cryptoContext = cryptoContext;
    securityContext->callbacks = *callbacks;

    /* Deep copy data */
    securityContext->setupData = *setupData;
    if (setupData->secIeLen) {
        securityContext->setupData.secIe = (CsrUint8*) CsrPmalloc(setupData->secIeLen);
        CsrMemCpy(securityContext->setupData.secIe, setupData->secIe, setupData->secIeLen);
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "SEC IE", setupData->secIe, setupData->secIeLen));
    }

    if (setupData->pmkValid == TRUE)
    {
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "pmk", setupData->pmk, CSR_WIFI_SECURITY_PMK_LENGTH));
    }

    if (setupData->identity)
    {
        securityContext->setupData.identity = (char *) CsrPmalloc(CsrStrLen(setupData->identity) + 1);
        (void) CsrStrCpy(securityContext->setupData.identity, setupData->identity);
    }
    if (setupData->username)
    {
        securityContext->setupData.username = (char *) CsrPmalloc(CsrStrLen(setupData->username) + 1);
        (void)CsrStrCpy(securityContext->setupData.username, setupData->username);
    }
    if (setupData->password)
    {
        securityContext->setupData.password = (char *) CsrPmalloc(CsrStrLen(setupData->password) + 1);
        (void)CsrStrCpy(securityContext->setupData.password, setupData->password);
    }

    if (setupData->fast_pac_length != 0)
    {
        securityContext->setupData.fast_pac = (CsrUint8 *) CsrPmalloc(setupData->fast_pac_length);
        CsrMemCpy(securityContext->setupData.fast_pac, setupData->fast_pac, setupData->fast_pac_length);
    }

    if (setupData->clientCertificateLength != 0)
    {
        securityContext->setupData.clientCertificate = (CsrUint8 *) CsrPmalloc(setupData->clientCertificateLength);
        CsrMemCpy(securityContext->setupData.clientCertificate, setupData->clientCertificate, setupData->clientCertificateLength);
    }

    if (setupData->clientPrivateKeyLength != 0)
    {
        securityContext->setupData.clientPrivateKey = (CsrUint8 *) CsrPmalloc(setupData->clientPrivateKeyLength);
        CsrMemCpy(securityContext->setupData.clientPrivateKey, setupData->clientPrivateKey, setupData->clientPrivateKeyLength);
    }

    if (setupData->authenticationServerCertificateLength != 0)
    {
        securityContext->setupData.authenticationServerCertificate = (CsrUint8 *) CsrPmalloc((CsrUint32)setupData->authenticationServerCertificateLength);
        CsrMemCpy(securityContext->setupData.authenticationServerCertificate, setupData->authenticationServerCertificate, (CsrUint32)setupData->authenticationServerCertificateLength);
    }

#if defined CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE || defined CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
    /* New 4-way handshake */
    if (setupData->mode == CSR_WIFI_SECURITY_WPAPSK_MODE_AUTHENTICATOR)
    {
        securityContext->wpaContext = csr_handshake_init(securityContext);
    }
#endif

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    if ((setupData->mode == CSR_WIFI_SECURITY_WAPIPSK_MODE_AE) ||
        (setupData->mode == CSR_WIFI_SECURITY_WAPIPSK_MODE_ASUE) ||
        (setupData->mode == CSR_WIFI_SECURITY_WAPI_MODE_ASUE))
    {
        securityContext->wapiContext = csr_wapi_init(securityContext);
    }
#endif

#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
    if (setupData->mode == CSR_WIFI_SECURITY_WPS_MODE_SUPPLICANT)
    {
        securityContext->wpsContext = csr_wps_init(securityContext);
    }
#endif

    return securityContext;
}

/*
 * Description:
 * See description in csr_security.h
 */
void CsrWifiSecurityDeInit(CsrWifiSecurityContext* context)
{
    packetNode    *packet;

    sme_trace_entry((TR_SECURITY_LIB, "CsrWifiSecurityDeInit()"));

    require(TR_SECURITY_LIB, context != NULL);

    if (context->setupData.secIe)
    {
        CsrPfree(context->setupData.secIe);
    }

#if defined CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE || defined CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
    if (context->wpaContext)
    {
        csr_handshake_deinit(context->wpaContext);
    }
#endif

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    if (context->wapiContext)
    {
        csr_wapi_deinit(context->wapiContext);
    }
#endif

    if (context->setupData.identity)
    {
        CsrPfree(context->setupData.identity);
    }
    if (context->setupData.username)
    {
        CsrPfree(context->setupData.username);
    }
    if (context->setupData.password)
    {
        CsrPfree(context->setupData.password);
    }

    if (context->setupData.fast_pac)
    {
        CsrPfree(context->setupData.fast_pac);
    }

    if (context->setupData.clientCertificate)
    {
        CsrPfree(context->setupData.clientCertificate);
    }

    if (context->setupData.clientPrivateKey)
    {
        CsrPfree(context->setupData.clientPrivateKey);
    }

    if (context->setupData.authenticationServerCertificate)
    {
        CsrPfree(context->setupData.authenticationServerCertificate);
    }


#ifdef CSR_WIFI_SECURITY_EAP_ENABLE
    if (context->eapContext)
    {
        eap_deinit(context->eapContext);
    }
#endif

#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
    if (context->wpsContext)
    {
        csr_wps_deinit(context->wpsContext);
    }
#endif

    /* Empty the list of packets queued inside the security library */
    packet = csr_list_gethead_t(packetNode*, &context->inputQueue);

    while(packet != NULL)
    {
        context->callbacks.packetProcessingDone(context->externalContext, packet->appCookie);
        csr_list_removehead(&context->inputQueue);
        packet = csr_list_gethead_t(packetNode*, &context->inputQueue);
    }

    CsrPfree(context);
}

/*
 * Description:
 * See description in csr_security.h
 */
void CsrWifiSecurityStart(CsrWifiSecurityContext* context)
{
    sme_trace_info((TR_SECURITY_LIB, "CsrWifiSecurityStart()"));

    require(TR_SECURITY_LIB, context != NULL);

    if (context->setupData.protocolSnapValid != TRUE)
    {
        sme_trace_error((TR_SECURITY_LIB, "CsrWifiSecurityStart() Cannot send initial message as the SNAP is not set"));
        context->callbacks.abortProcedure(context->externalContext);
        return;
    }

    /* Register Config SA timeout to put a cap on the amount of time the security association can take */
    context->callbacks.startTimer(context->externalContext,
                                  CSR_WIFI_SECURITY_CONFIGSA_TIMEOUT,
                                  CSR_WIFI_SECURITY_TIMEOUT_SECURITY_ASSOCIATION);

    switch(context->setupData.mode)
    {
#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
    case CSR_WIFI_SECURITY_WPS_MODE_SUPPLICANT:
        csr_wps_start(context->wpsContext);
        break;
#endif

    case CSR_WIFI_SECURITY_WPAPSK_MODE_SUPPLICANT:
    case CSR_WIFI_SECURITY_WPA_MODE_SUPPLICANT:
        context->callbacks.startTimer(context->externalContext,
                                      CSR_WIFI_SECURITY_START_FIRST,
                                      CSR_WIFI_SECURITY_TIMEOUT_SECURITY_START);
        context->startTimerOn = TRUE;
        break;

#if defined CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE || defined CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
    case CSR_WIFI_SECURITY_WPAPSK_MODE_AUTHENTICATOR:
        csr_handshake_start(context->wpaContext);
        break;
#endif

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    case CSR_WIFI_SECURITY_WAPIPSK_MODE_ASUE:
    case CSR_WIFI_SECURITY_WAPI_MODE_ASUE:
    case CSR_WIFI_SECURITY_WAPIPSK_MODE_AE:
        csr_wapi_start(context->wapiContext);
        break;
#endif
    default:
        sme_trace_error((TR_SME_TRACE, "CsrWifiSecurityStart::not implemented" ));
        break;
    }
}

/*
 * Description:
 * See description in csr_security.h
 */

void CsrWifiSecuritySendEapolError(CsrWifiSecurityContext* context, CsrBool pairwise, CsrUint8 *tsc)
{
    sme_trace_entry((TR_SECURITY_LIB, "CsrWifiSecuritySendEapolError"));

    require(TR_SECURITY_LIB, context != NULL);

    sme_trace_entry((TR_SECURITY_LIB, "pairwise %d", pairwise));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_DEBUG, "tsc", tsc, 8));

#if defined CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE || defined CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
    csr_handshake_send_eapol_error (context->wpaContext, pairwise, tsc);
#endif
}

/*
 * Description:
 * See description in csr_security.h
 */
void CsrWifiSecurityTimerExpired(CsrWifiSecurityContext* context, CsrWifiSecurityTimeout timeoutIndex)
{
    sme_trace_entry((TR_SECURITY_LIB, "CsrWifiSecurityTimerExpired()"));

    require(TR_SECURITY_LIB, context != NULL);

    if (timeoutIndex == CSR_WIFI_SECURITY_TIMEOUT_SECURITY_START)
    {
        CsrUint8 eapolStart[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00 };

        /* Send Eapol Start Message */
        sme_trace_info((TR_SECURITY_LIB, "CsrWifiSecurityStart() :: Sending EAPOL start"));
        CsrMemCpy((CsrUint8*)eapolStart, context->setupData.protocolSnap, CSR_WIFI_SECURITY_SNAP_LENGTH);
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "CsrWifiSecurityStart() :: EAPOL start", eapolStart, sizeof(eapolStart)));
        context->callbacks.sendPacket(context->externalContext,
                                      eapolStart,
                                      sizeof(eapolStart),
                                      context->setupData.localMacAddress,
                                      context->setupData.peerMacAddress);

        context->retransmissionCount++;
        if (context->retransmissionCount <= context->setupData.retransmissionAttempts)
        {
            context->callbacks.startTimer(context->externalContext,
                                          CSR_WIFI_SECURITY_START_SUBSEQUENT,
                                          CSR_WIFI_SECURITY_TIMEOUT_SECURITY_START);
        }
        else
        {
            context->startTimerOn = FALSE;
            sme_trace_error((TR_SECURITY_LIB, "Havent received first packet from AP for long: Aborting handshake"));
            context->callbacks.abortProcedure(context->externalContext);
        }
        return;
    }

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    if (context->wapiContext)
    {
        csr_wapi_timeout(context->wapiContext, timeoutIndex);
        return;
    }
#endif

#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
    if (context->wpsContext)
    {
        csr_wps_timeout(context->wpsContext, timeoutIndex);
        return;
    }
#endif

#if defined CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE || defined CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
    if (context->wpaContext)
    {
        csr_handshake_timeout(context->wpaContext, timeoutIndex);
        return;
    }
#endif

#ifdef CSR_WIFI_SECURITY_EAP_ENABLE
    if (context->eapContext)
    {
        eap_timeout(context->eapContext, timeoutIndex);
        return;
    }
#endif

    sme_trace_error((TR_SECURITY_LIB, "CsrWifiSecurityTimerExpired() : Unexpected timout"));
}

/*
 * Description:
 * See description in csr_security.h
 */
static void processPacket(CsrWifiSecurityContext* context, const CsrUint8* packetBuffer, const CsrUint32 packetBufferLength)
{

    eapol_packet* pkt = (eapol_packet*)(packetBuffer);
    CsrUint16 length = (CsrUint16)packetBufferLength; /* Length should be validated by validate_input_packet function previously */

    sme_trace_entry((TR_SECURITY_LIB, "processPacket()"));

    if ((0 == CsrMemCmp(btAmpSnapHeader, packetBuffer, sizeof(btAmpSnapHeader)))
            || (0 == CsrMemCmp(eapolSnapHeader, packetBuffer, sizeof(eapolSnapHeader))))
    {
        length = EAPOL_LENGTH_FIELD(pkt);
        /* length is only the EAPOL packet body length, so need to allow for LLC SNAP and EAPOL header */
        sme_trace_info((TR_SECURITY_LIB, "packet length = %d", length + CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_DEBUG, "pkt dump", packetBuffer, length + CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH));
    }
    #ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    else
    if (0 == CsrMemCmp(wapiSnapHeader, packetBuffer, sizeof(wapiSnapHeader)))
    {
        length = WAPI_PACKET_LENGTH((wapi_packet*)packetBuffer);
        sme_trace_info((TR_SECURITY_LIB, "packet length = %d", length + CSR_WIFI_SECURITY_SNAP_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_DEBUG, "pkt dump", packetBuffer, length + CSR_WIFI_SECURITY_SNAP_LENGTH));
    }
    #endif

    /* Save the SNAP for later inclusion in outgoing packets. */
    context->setupData.protocolSnapValid = TRUE;
    CsrMemCpy(context->setupData.protocolSnap, packetBuffer, CSR_WIFI_SECURITY_SNAP_LENGTH);

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    if (0 == CsrMemCmp(wapiSnapHeader, packetBuffer, sizeof(wapiSnapHeader)))
    {
        context->processingInProgress = TRUE;
        csr_wapi_pkt_input(context->wapiContext, (wapi_packet*)packetBuffer, length);
        return;
    }
#endif

#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
    if (context->setupData.mode == CSR_WIFI_SECURITY_WPS_MODE_SUPPLICANT)
    {
        context->processingInProgress = TRUE;
        csr_wps_pkt_input(context->wpsContext, (CsrUint8*) packetBuffer, length + CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH);
        return;
    }
#endif

#ifdef CSR_WIFI_SECURITY_EAP_ENABLE
    /* If we are running an eap instance then pass the message on */
    if(context->eapContext != NULL)
    {
        /* FIXME : EAPOL KEY HINT? */
        context->processingInProgress = TRUE;
        if(eap_pkt_input(context->eapContext, pkt, length) && pkt->packet_type == EAPOL_PACKET_TYPE_EAPOL_KEY)
        {
#if defined CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE || defined CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
            sme_trace_info((TR_SECURITY_LIB, "CsrWifiSecurityProcessPacket() :: forwarding packet to handshake module"));
            if(context->wpaContext == NULL)
            {
                /* New 4-way handshake */
                context->wpaContext = csr_handshake_init(context);
            }

            if(context->wpaContext != NULL)
            {
                context->processingInProgress = TRUE;
                csr_handshake_pkt_input(context->wpaContext, (eapol_packet *) packetBuffer, length);
            }
#else
            sme_trace_error((TR_SECURITY_LIB, "Unexpected EAPOL KEY with eapContext"));
#endif
        }
        return;
    }
#endif

    /* EapolKey messages when not in eap negotiations are for a 4 way handshake */
    if (pkt->packet_type == EAPOL_PACKET_TYPE_EAPOL_KEY)
    {
#if defined CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE || defined CSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
        if(context->wpaContext == NULL)
        {
            /* New 4-way handshake */
            context->wpaContext = csr_handshake_init(context);
        }

        if(context->wpaContext != NULL)
        {
            context->processingInProgress = TRUE;
            csr_handshake_pkt_input(context->wpaContext, (eapol_packet *) packetBuffer, length);
        }
#else
        sme_trace_error((TR_SECURITY_LIB, "Unexpected EAPOL KEY"));
#endif
        return;
    }
    else
    {
#ifdef CSR_WIFI_SECURITY_EAP_ENABLE
        /* New eap negotiation */
        if (context->setupData.identity != NULL)
        {
            CsrBool result; /*lint -e550*/
            context->eapContext = eap_init(context);
            context->processingInProgress = TRUE;
            result = eap_pkt_input(context->eapContext, pkt, length);
            sme_trace_info((TR_SECURITY_LIB, "CsrWifiSecurityProcessPacket() :: eap_pkt_input returns %s",
                                result?"TRUE":"FALSE"));
            return;
        }
#else
        sme_trace_error((TR_SECURITY_LIB, "Unexpected EAP Message 0x%X", pkt->packet_type ));
#endif
    }

    sme_trace_error((TR_SECURITY_LIB, "Unhandled packet!, type - 0x%X", pkt->packet_type ));
}

/*
 * Description:
 * See description in csr_security.h
 */
void CsrWifiSecurityProcessPacket(CsrWifiSecurityContext* context, const CsrUint32 appCookie, const CsrUint8* packetBuffer, const CsrUint32 packetBufferLength)
{
    csr_list_node *node;
    packetNode    *packet;

    sme_trace_entry((TR_SECURITY_LIB, "CsrWifiSecurityProcessPacket()"));

    if (FALSE == validate_input_packet(context, packetBuffer, packetBufferLength))
    {
        context->callbacks.packetProcessingDone(context->externalContext, appCookie);
        return;
    }

    /* If we have already queued several packets, stop queueing and drop the packet */
    if (csr_list_size(&context->inputQueue) > 10)
    {
        sme_trace_error((TR_SECURITY_LIB, "Too many packets queued %d, dropping packet",
                csr_list_size(&context->inputQueue)));
        context->callbacks.packetProcessingDone(context, appCookie);
        return;
    }

    packet = csr_list_gethead_t(packetNode*, &context->inputQueue);
    if ((context->processingInProgress == FALSE) && (packet != NULL))
    {
        sme_trace_error((TR_SECURITY_LIB, "No packet being processed(?) but packet packet in queue"));
    }

    node = (csr_list_node *)CsrPmalloc(sizeof(csr_list_node));
    packet = (packetNode*)CsrPmalloc(sizeof(packetNode));
    packet->appCookie = (CsrUint32)appCookie;
    packet->packet = (CsrUint8*)packetBuffer;
    packet->packetLength = (CsrUint16) packetBufferLength;
    csr_list_insert_tail(&context->inputQueue, list_owns_both, node, packet);

    if (context->processingInProgress == FALSE)
    {
        require (TR_SECURITY_LIB, (1 == csr_list_size(&context->inputQueue)));

        if (context->startTimerOn == TRUE)
        {
            context->callbacks.stopTimer(context->externalContext,
                                         CSR_WIFI_SECURITY_TIMEOUT_SECURITY_START);
            context->startTimerOn = FALSE;
        }

        processPacket(context, packetBuffer, packetBufferLength);
        /* Remember processPacket may call CsrWifiSecurityPacketProcessingDone which removes the packet
         * from queue */
    }
}

/*
 * Description:
 * See description in csr_security_private.h
 */
void CsrWifiSecurityPacketProcessingDone(CsrWifiSecurityContext* context)
{
    packetNode *packet;

    sme_trace_entry((TR_SECURITY_LIB, "CsrWifiSecurityPacketProcessingDone()"));

    context->processingInProgress = FALSE;
    packet = csr_list_gethead_t(packetNode*, &context->inputQueue);

    if (packet != NULL)
    {
        context->callbacks.packetProcessingDone(context->externalContext, packet->appCookie);
        csr_list_removehead(&context->inputQueue);
    }
    else
    {
        sme_trace_warn((TR_SECURITY_LIB, "CsrWifiSecurityPacketProcessingDone() "
                "called when there is nothing in the queue"));
    }

    /* Look for any queued messages and feed one to the module */
    packet = csr_list_gethead_t(packetNode*, &context->inputQueue);
    if (packet != NULL)
    {
        processPacket(context, packet->packet, packet->packetLength);
    }
}

/*
 * Description:
 * See description in csr_security_private.h
 */
void CsrWifiSecurityStorePmk(CsrWifiSecurityContext* securityContext, CsrUint8* pmk)
{
#ifdef CSR_WIFI_SECURITY_PMK_CACHING_ENABLE
    CsrUint8 pmkid[CSR_SHA1_DIGEST_LENGTH];
    const char *pmkString = "PMK Name";
    CsrUint8 pmkidInput[8 + 2 * CSR_WIFI_SECURITY_MACADDRESS_LENGTH];
#endif

    sme_trace_entry((TR_SECURITY_LIB, "CsrWifiSecurityStorePmk"));

    CsrMemCpy(securityContext->setupData.pmk, pmk, CSR_WIFI_SECURITY_PMK_LENGTH);
    securityContext->setupData.pmkValid = TRUE;

#ifdef CSR_WIFI_SECURITY_PMK_CACHING_ENABLE
     /* Derive PMK ID from PMK */
    CsrMemCpy(pmkidInput, pmkString, CsrStrLen(pmkString));
    CsrMemCpy(pmkidInput + CsrStrLen(pmkString), securityContext->setupData.peerMacAddress, CSR_WIFI_SECURITY_MACADDRESS_LENGTH);
    CsrMemCpy(pmkidInput + CsrStrLen(pmkString) + CSR_WIFI_SECURITY_MACADDRESS_LENGTH,
              securityContext->setupData.localMacAddress, CSR_WIFI_SECURITY_MACADDRESS_LENGTH);

    CsrCryptoCallHmacSha1(securityContext->cryptoContext, NULL,
            pmk, CSR_WIFI_SECURITY_PMK_LENGTH, pmkidInput, sizeof(pmkidInput), pmkid);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "PMK", pmk, CSR_WIFI_SECURITY_PMK_LENGTH));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Storing PMKID", pmkid, CSR_WIFI_SECURITY_PMKID_LENGTH));

    securityContext->callbacks.pmkidStore(securityContext->externalContext, securityContext->setupData.peerMacAddress,
                pmkid, pmk);
#endif
}

