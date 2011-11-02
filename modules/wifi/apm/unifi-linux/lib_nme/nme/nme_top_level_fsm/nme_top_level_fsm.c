/** @file nme_top_level_fsm.c
 *
 * NME Top Level
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
 *   Creates and initialises Network Management Entity
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_top_level_fsm/nme_top_level_fsm.c#2 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_top_level_fsm
 */


/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "nme_top_level_fsm/nme.h"
#include "nme_top_level_fsm/nme_top_level_fsm.h"
#include "nmeio/nmeio_free_internal_fsm_message_contents.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/**
 * See description in sme_top_level_fsm/sme_top_level_fsm.h
 */
const unifi_MACAddress NmeNullBssid = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

/**
 * See description in sme_top_level_fsm/sme_top_level_fsm.h
 */
const unifi_MACAddress NmeBroadcastBssid = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Based on the event id will call any appropriate message
 *   content free function.
 *
 * @par Description
 *   See brief.
 *
 * @param[in] void* : app Context
 * @param[in] const FsmEvent* : event
 *
 * @return
 *   void
 */
static void nme_free_ignored_event(void* appContext, const FsmEvent* pEvt)
{
    nme_free_event(pEvt);
}


/* PUBLIC CONSTANT DEFINITIONS **********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/
/**
 * See description in nme_top_level_fsm/nme.h
 */
void nme_free_event(const FsmEvent* pEvt)
{
    if (0xa000 <= pEvt->id &&  0xb000 > pEvt->id)
    {
        /* The NME-SAP primitives specific range, excludes renamed MGT-SAP primitives */
        nmeio_free_message_contents(pEvt);
    }
    else if (0x6100 <= pEvt->id &&  0x6600 > pEvt->id)
    {
        /* Events from the NME internal FSMs */
        nmeio_free_internal_fsm_message_contents(pEvt);
    }
    else if (UNIFI_NME_SYS_MA_UNITDATA_IND_ID == pEvt->id)
    {
        UnifiNmeSysMaUnitdataInd_Evt *pInd = (UnifiNmeSysMaUnitdataInd_Evt *)pEvt;
        if (pInd->freeFunction)
        {
            (*pInd->freeFunction)(pInd->frame);
        }
        else
        {
            CsrPfree(pInd->frame);
        }
    }
    else
    {
        /* Everything else should be covered by the SME free function */
        smeio_free_message_contents(pEvt);
    }
}

/**
 * See description in nme_top_level_fsm/nme.h
 */
#ifdef SME_TRACE_ENABLE
void nme_trace_credentials(const unifi_Credentials *pCredentials)
{
    sme_trace_debug((TR_NME_CORE_FSM, "credential type %s", trace_unifi_CredentialType(pCredentials->credentialType)));
    switch(pCredentials->credentialType)
    {
    case unifi_CredentialTypeOpenSystem:
        break;
    case unifi_CredentialTypeWep64:
        sme_trace_debug((TR_NME_CORE_FSM, "auth mode %s", trace_unifi_AuthMode(pCredentials->credential.wep64Key.wepAuthType)));
        break;
    case unifi_CredentialTypeWep128:
        sme_trace_debug((TR_NME_CORE_FSM, "auth mode %s", trace_unifi_AuthMode(pCredentials->credential.wep128Key.wepAuthType)));
        break;
    case unifi_CredentialTypeWpaPsk:
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.wpaPsk.encryptionMode));
        break;
    case unifi_CredentialTypeWpaPassphrase:
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.wpaPassphrase.encryptionMode));
        break;
    case unifi_CredentialTypeWpa2Psk:
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.wpa2Psk.encryptionMode));
        break;
    case unifi_CredentialTypeWpa2Passphrase:
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.wpa2Passphrase.encryptionMode));
        break;
    case unifi_CredentialType8021xTls:
        sme_trace_debug((TR_NME_CORE_FSM, "auth mode %s", trace_unifi_AuthMode(pCredentials->credential.tls.authMode)));
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.tls.encryptionMode));
        break;
    case unifi_CredentialType8021xTtls:
        sme_trace_debug((TR_NME_CORE_FSM, "auth mode %s", trace_unifi_AuthMode(pCredentials->credential.ttls.authMode)));
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.ttls.encryptionMode));
        break;
    case unifi_CredentialType8021xPeapGtc:
        sme_trace_debug((TR_NME_CORE_FSM, "auth mode %s", trace_unifi_AuthMode(pCredentials->credential.peapGtc.authMode)));
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.peapGtc.encryptionMode));
        break;
    case unifi_CredentialType8021xPeapMschapv2:
        sme_trace_debug((TR_NME_CORE_FSM, "auth mode %s", trace_unifi_AuthMode(pCredentials->credential.peapMsChapV2.authMode)));
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.peapMsChapV2.encryptionMode));
        break;
    case unifi_CredentialType8021xLeap:
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.leap.encryptionMode));
        break;
    case unifi_CredentialType8021xFast:
        sme_trace_debug((TR_NME_CORE_FSM, "auth mode %s", trace_unifi_AuthMode(pCredentials->credential.fast.eapCredentials.authMode)));
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.fast.eapCredentials.encryptionMode));
        break;
    case unifi_CredentialTypeWapiPsk:
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.wapiPsk.encryptionMode));
        break;
    case unifi_CredentialTypeWapiPassphrase:
        sme_trace_debug((TR_NME_CORE_FSM, "encryption mode 0x%x", pCredentials->credential.wapiPassphrase.encryptionMode));
        break;
    case unifi_CredentialTypeWapi:
        break;
    default:
        break;
    }
}
#endif


/**
 * See description in nme_top_level_fsm/nme.h
 */
FsmContext* csr_wifi_nme_init(void* pExternalContext, void* cryptoContext)
{
    NmeContext* pNmeContext = (NmeContext*)CsrPmalloc(sizeof(*pNmeContext));

    pNmeContext->fsmContext = fsm_init_context(pNmeContext, pExternalContext, NME_NUM_OF_PROCESSES);

    fsm_install_app_ignore_event_callback(pNmeContext->fsmContext, nme_free_ignored_event);

    /* Initialise any other FSM instances that we need */
    pNmeContext->nmeSignalRoutingFsmInstance = fsm_add_instance(pNmeContext->fsmContext, &nme_signal_routing_fsm, FALSE);
    pNmeContext->nmeCoreFsmInstance = fsm_add_instance(pNmeContext->fsmContext, &nme_core_fsm, FALSE);
    pNmeContext->nmeProfileMgrFsmInstance = fsm_add_instance(pNmeContext->fsmContext, &nme_profile_manager_fsm, FALSE);
    pNmeContext->nmeNetworkSelectorFsmInstance = fsm_add_instance(pNmeContext->fsmContext, &nme_network_selector_fsm, FALSE);

    pNmeContext->cryptoContext = cryptoContext;

    fsm_call_entry_functions(pNmeContext->fsmContext);
    return(pNmeContext->fsmContext);
}

/**
 * See description in nme_top_level_fsm/nme.h
 */
FsmContext* csr_wifi_nme_reset(FsmContext* pContext)
{
    void* pExtContext = pContext->externalContext;
    /* remeber the crypto instance */
    void* cryptoContext = getNmeContext(pContext)->cryptoContext;;

    csr_wifi_nme_shutdown(pContext);
    return csr_wifi_nme_init(pExtContext, cryptoContext);
}

/**
 * See description in nme_top_level_fsm/nme.h
 */
void csr_wifi_nme_shutdown(FsmContext* pContext)
{
    NmeContext* pNmeContext = getNmeContext(pContext);
    fsm_shutdown(pContext);
    CsrPfree(pNmeContext);
}

/**
 * See description in nme_top_level_fsm/nme.h
 */
FsmTimerData csr_wifi_nme_execute(FsmContext* pContext)
{
    return fsm_execute(pContext);
}

/**
 * See description in nme_top_level_fsm/nme.h
 */
void csr_wifi_nme_install_wakeup_callback(FsmContext* pContext, FsmExternalWakupCallbackPtr callback)
{
    fsm_install_wakeup_callback(pContext, callback);
}

/** @}
 */
