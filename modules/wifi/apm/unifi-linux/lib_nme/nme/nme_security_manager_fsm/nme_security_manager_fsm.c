/** @file nme_security_manager_fsm.c
 *
 * NME Security Manager FSM Implementation
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009-2010. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   NME Security Manager Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_security_manager_fsm/nme_security_manager_fsm.c#8 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_security
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "nme_top_level_fsm/nme_top_level_fsm.h"
#include "nme_security_manager_fsm/nme_pmk_cache.h"
#include "nme_security.h"

#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
#include "csr_passphrase_hashing.h"
#endif
/* MACROS *******************************************************************/

/**
 * @brief
 *   Simple accessor for this Processes Custom data
 *
 * @par Description
 *   see brief
 */
#define FSMDATA (fsm_get_params(context, FsmData))

/**
 * @brief
 *   Manipulating the security bit flags
 *
 * @par Description
 *   see brief
 */
#define SEC_SET_FLAG(fsmData, value) (fsmData->secFlags |= value)
#define SEC_CLEAR_FLAG(fsmData, value) (fsmData->secFlags &= ~value)
#define SEC_IS_SET(fsmData, value) ((fsmData->secFlags & value) != 0)

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/
/**
 * @brief
 *   FSM States
 *
 * @par Description
 *   Enum defining the FSM States for this FSM process
 */
typedef enum FsmState
{
    FSMSTATE_idle,
    FSMSTATE_waiting_for_subscription,
    FSMSTATE_waiting_for_keys,
    FSMSTATE_ready,
    FSMSTATE_started,
    FSMSTATE_waiting_for_terminate,
    FSMSTATE_terminating,
    FSMSTATE_MAX_STATE
} FsmState;

/**
 * @brief
 *   Security Flags bit indexes
 *
 * @par Description
 *   Enum defining the indexes into secFlags
 */
typedef enum SecFlags
{
    SecFlag_terminateReceived               = 0x0001,
    SecFlag_SettingPairWiseKey              = 0x0002,
    SecFlag_HavePairWiseKey                 = 0x0004,
    SecFlag_SettingGroupKey                 = 0x0008,
    SecFlag_HaveGroupKey                    = 0x0010,
    SecFlag_SettingWepKeys                  = 0x0020,
    SecFlag_HaveWepKeys                     = 0x0040,
    SecFlag_FlushingWepKeys                 = 0x0080,
    SecFlag_MichealMicEapolErrorSent        = 0x0100,
    SecFlag_EapolPathOpen                   = 0x0200,
    SecFlag_EapolPathRequestedClosure       = 0x0400,

    /* We have no credentials at the start of a WPS procedure as we
     * are using WPS to try and generate some, so we have to know
     * when we are attempting WPS.
     */
    SecFlag_Wps                             = 0x0800,

    /* Need to report when all of the actions associated with been "fully
     * connected" are complete. For example on an "open system" this will
     * will be straight away as there is nothing to do, whilst for WPA we
     * will have to authenticate. So for the first time after associating
     * with a new AP the complete ind has to be sent once either one success
     * or failure.
     */
    SecFlag_ReportedComplete                = 0x1000,

    /* Need to track whether there are any updates to the security information
     * that need to be pushed out by updating the profile.
     */
    SecFlag_PacUpdated                      = 0x2000,
    SecFlag_SessionUpdated                  = 0x4000

} SecFlags;

/**
 * @brief
 *   FSM Data
 *
 * @par Description
 *   Struct defining the FSM data for this FSM process
 */
typedef struct FsmData {
    /* security control Block */
    CsrBool isCtrlBlkInitialised;
    nmeSecurityCtrlBlk securityCtrlBlk;

    /** flags used with SecFlags */
    CsrUint16 secFlags;

    /** Used to track the number of outstanding confirms during installing wep
     * keys
     */
    CsrUint16 wepKeyCfmCount;

    /** Security information, separate to the security control block as this
     * info is not required by the sec library, but is supplied back to the
     * connection manager for use in the MGT CONNECT REQ
     */
    unifi_80211PrivacyMode  privacyMode;
    CsrUint16               authModeMask;
    CsrUint16               encryptionModeMask;

    /* This is a Dynamic FSM so it needs to record owner and assigned
     * instance numbers as this aids signal routing.
     */
    CsrUint16 ownerInstance;
    CsrUint16 ourInstance;

    /* Unique msg id reference to ensure that the response to the
     * nme certificate validate ind is the correct one and not
     * from an earlier indication.
     */
    CsrUint8 certificateValidationId;

} FsmData;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/** protocol definition for the subscribe request */
static const CsrUint16 wpaProtocol  = 0x888E;
static const CsrUint16 wapiProtocol = 0x88B4;
/** oui definition for the subscribe request */
static const CsrUint32 wpaOui=0x00000000;
static const CsrUint32 wapiOui=0x00000000;

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   NME Security Manager Process FSM Entry Function
 *
 * @par Description
 *   Called on NME Security Manager Process startup to initialize
 *   the NME security manager data
 *
 * @param[in] context : FSM context
 *
 * @return
 *   void
 */
static void nme_sec_init_data(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    CsrUint8 iTimerId = 0;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_init_data"));

    fsmData->isCtrlBlkInitialised = FALSE;
    fsmData->secFlags = 0;
    fsmData->wepKeyCfmCount = 0;
    fsmData->certificateValidationId = 0;

    /* The NME security module initialises the control block.
     * however, this FSM handle the timer and relies on it being set to zero
     */
    for (iTimerId = 0; iTimerId < CSR_WIFI_SECURITY_TIMEOUT_MAX_TIMEOUTS; iTimerId++)
    {
        fsmData->securityCtrlBlk.secLibTimerId[iTimerId].uniqueid = 0;
    }

    fsmData->ownerInstance = FSM_TERMINATE;
    fsmData->ourInstance = FSM_TERMINATE;
}

/**
 * @brief
 *   NME Security Manager Process FSM Entry Function
 *
 * @par Description
 *   Called on Security Manager Process startup to initialise
 *   the NME security manager data
 *
 * @param[in] context : FSM context
 *
 * @return
 *   void
 */
static void nme_sec_init(FsmContext* context)
{
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_init"));

    fsm_create_params(context, FsmData);
    nme_sec_init_data(context);
    fsm_next_state(context, FSMSTATE_idle);
}

/**
 * @brief
 *   FSM Reset Function
 *
 * @par Description
 *   Called on reset/shutdown to cleanout any memory in use
 *
 * @param[in] context : FSM context
 *
 * @return
 *   void
 */
static void nme_sec_reset(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_reset"));

    if (fsmData->isCtrlBlkInitialised)
    {
        /* reset the security control block */
        nme_security_deinit(context, &fsmData->securityCtrlBlk);
        fsmData->isCtrlBlkInitialised = FALSE;
    }

    fsm_next_state(context, FSM_TERMINATE);
}


/**
 * @brief
 *   FSM Reset Function
 *
 * @par Description
 *   Called on reset/shutdown to:
 *    1. free any memory
 *    2. Flush WEP keys
 *    3. Close EAPOL Path
 *    4. Cancel EAPOL timer
 *    5. De-initialize  Security module
 *
 *    This function will process one step at a time before finally closing down.
 *
 * @param[in] context : FSM context
 *
 * @return
 *   void
 */
static void nme_sec_security_manager_cleanup(FsmContext* context)
{
    FsmData* fsmData = FSMDATA;
    CsrUint8 iTimerId = 0;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_security_manager_cleanup()"));

    /* cancel any outstanding EAPOL timers */
    for (iTimerId = 0; iTimerId < CSR_WIFI_SECURITY_TIMEOUT_MAX_TIMEOUTS; iTimerId++)
    {
        sme_trace_debug((TR_NME_SMGR_FSM, "checking timer for sec lib id %d", iTimerId));
        if(fsmData->securityCtrlBlk.secLibTimerId[iTimerId].uniqueid != 0)
        {
            sme_trace_debug((TR_NME_SMGR_FSM, "removing timer id %d", fsmData->securityCtrlBlk.secLibTimerId[iTimerId].uniqueid));
            fsm_remove_timer(context, fsmData->securityCtrlBlk.secLibTimerId[iTimerId]);
            fsmData->securityCtrlBlk.secLibTimerId[iTimerId].uniqueid = 0;
        }
    }

    if (SEC_IS_SET(fsmData, SecFlag_HaveWepKeys))
    {
        if (!SEC_IS_SET(fsmData, SecFlag_FlushingWepKeys))
        {
            unifi_Key key;
            sme_trace_debug((TR_NME_SMGR_FSM, "flushing WEP keys()"));

            SEC_SET_FLAG(fsmData, SecFlag_FlushingWepKeys);
            nme_routing_store_cfm_prim_internal(context, UNIFI_NME_MGT_KEY_CFM_ID, fsmData->ourInstance);
            /* Eventhough we're flushing the keys, still need to supply something in the keys field
             * that is not used by the SME.
             */
            CsrMemSet(&key, 0, sizeof(unifi_Key));
            call_unifi_nme_mgt_key_req(context, NME_APP_HANDLE(context), unifi_ListActionFlush, &key);
        }
        else
        {
            sme_trace_debug((TR_NME_SMGR_FSM, "waiting for WEP keys to flush"));
        }
        fsm_next_state(context, FSMSTATE_terminating);

        /* Continue the cleanup on the reception of key flush confirmation */
        return;
    }

    if (SEC_IS_SET(fsmData, SecFlag_EapolPathOpen))
    {
        if (!SEC_IS_SET(fsmData, SecFlag_EapolPathRequestedClosure))
        {
            sme_trace_debug((TR_NME_SMGR_FSM, "unsubscribe EAPOL()"));
            call_unifi_nme_sys_ma_unitdata_unsubscribe_req(context, NME_APP_HANDLE(context), fsmData->securityCtrlBlk.subscriptionHandle);
            SEC_SET_FLAG(fsmData, SecFlag_EapolPathRequestedClosure);
        }
        fsm_next_state(context, FSMSTATE_terminating);

        /* Continue the cleanup on the reception of unsubscribe confirmation */
        return;
    }

    /* if running, shut down the security Lib */
    if (fsmData->isCtrlBlkInitialised)
    {
        /* reset the security control block */
        nme_security_deinit(context, &fsmData->securityCtrlBlk);
    }

    /* send confirmation if required */
    if (SEC_IS_SET(fsmData, SecFlag_terminateReceived))
    {
        sme_trace_debug((TR_NME_SMGR_FSM, "send security_stop_cfm() and TERMINATE"));
        send_nme_security_terminate_cfm(context, fsmData->ownerInstance, unifi_Success);
        fsm_next_state(context, FSM_TERMINATE);
    }
}


/**
 * @brief
 *   Processes credentials to generate corresponding security flags
 *
 * @par Description
 *   This function process the credentials structure to produce the correct
 *   security argument required by the SME.
 *
 * @param[in] context : FSM context
 * @param[in] credentials : populated credential structure.
 *
 * @return
 *   nme_SecurityResultCode : nme_SecurityResultCodeSuccess
 *                            nme_SecurityResultCodeUnsupportedCredentialType
 *                            nme_SecurityResultCodeUnknownCredentialType
 */
static nme_SecurityResultCode nme_sec_generate_security_flags(
        FsmContext* context,
        unifi_Credentials *credentials)
{
    FsmData* fsmData = FSMDATA;
    nme_SecurityResultCode resultCode = nme_SecurityResultCodeSuccess;

    switch(credentials->credentialType)
    {
    case unifi_CredentialTypeOpenSystem:
    {
        fsmData->privacyMode = unifi_80211PrivacyDisabled,
        fsmData->authModeMask = unifi_80211AuthOpen,
        fsmData->encryptionModeMask = unifi_EncryptionCipherNone;
        break;
    }
    case unifi_CredentialTypeWep64:
    {
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = credentials->credential.wep64Key.wepAuthType;
        fsmData->encryptionModeMask = unifi_EncryptionCipherGroupWep40;
        break;
    }
    case unifi_CredentialTypeWep128:
    {
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = credentials->credential.wep128Key.wepAuthType;
        fsmData->encryptionModeMask = unifi_EncryptionCipherGroupWep104;
        break;
    }
    case unifi_CredentialTypeWpaPassphrase:
    {
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = unifi_8021xAuthWPAPSK;
        fsmData->encryptionModeMask = credentials->credential.wpaPsk.encryptionMode;
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
        break;
    }
    case unifi_CredentialTypeWpaPsk:
    {
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = unifi_8021xAuthWPAPSK;
        fsmData->encryptionModeMask = credentials->credential.wpaPassphrase.encryptionMode;
        break;
    }
    case unifi_CredentialTypeWpa2Passphrase:
    {
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = unifi_8021xAuthWPA2PSK;
        fsmData->encryptionModeMask = credentials->credential.wpa2Passphrase.encryptionMode;
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
        break;
    }
    case unifi_CredentialTypeWpa2Psk:
    {
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = unifi_8021xAuthWPA2PSK;
        fsmData->encryptionModeMask = credentials->credential.wpa2Psk.encryptionMode;
        break;
    }
    case unifi_CredentialType8021xLeap:
    {
#if defined CSR_WIFI_SECURITY_LEAP_ENABLE
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = unifi_80211AuthOpen | unifi_8021xAuthWPA2| unifi_8021xAuthWPA;
        fsmData->encryptionModeMask = credentials->credential.leap.encryptionMode;
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
        break;
    }
    case unifi_CredentialType8021xFast:
    {
#if defined CSR_WIFI_SECURITY_FAST_ENABLE
        if (0 < credentials->credential.fast.pacLength ||
            credentials->credential.fast.allowPacProvisioning)
        {
            fsmData->privacyMode = unifi_80211PrivacyEnabled;
            fsmData->authModeMask = credentials->credential.fast.eapCredentials.authMode;
            fsmData->encryptionModeMask  = credentials->credential.fast.eapCredentials.encryptionMode;
        }
        else
        {
            /* Need a PAC or we need to automatically provision one
             * as neither of these is possible it is an error.
             */
            resultCode = nme_SecurityResultCodeNoPacSupplied;
        }
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
        break;
    }
    case unifi_CredentialType8021xTtls:
    {
#if defined CSR_WIFI_SECURITY_TTLS_ENABLE
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = credentials->credential.ttls.authMode;
        fsmData->encryptionModeMask  = credentials->credential.ttls.encryptionMode;
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
        break;
    }
    case unifi_CredentialType8021xTls:
    {
#if defined CSR_WIFI_SECURITY_TLS_ENABLE
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = credentials->credential.tls.authMode;
        fsmData->encryptionModeMask  = credentials->credential.ttls.encryptionMode;
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
        break;
    }
    case unifi_CredentialTypeWapiPassphrase:
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
#ifdef CSR_CRYPTO_KDHMACSHA256_ENABLE
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = unifi_WAPIAuthWAIPSK;
        fsmData->encryptionModeMask = unifi_EncryptionCipherPairwiseSms4 | unifi_EncryptionCipherGroupSms4;
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
        break;

    case unifi_CredentialTypeWapiPsk:
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = unifi_WAPIAuthWAIPSK;
        fsmData->encryptionModeMask = unifi_EncryptionCipherPairwiseSms4 | unifi_EncryptionCipherGroupSms4;
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
        break;
    case unifi_CredentialTypeWapi:
#if defined(CSR_WIFI_SECURITY_WAPI_ENABLE)
        fsmData->privacyMode = unifi_80211PrivacyEnabled;
        fsmData->authModeMask = unifi_WAPIAuthWAI;
        fsmData->encryptionModeMask = unifi_EncryptionCipherPairwiseSms4 | unifi_EncryptionCipherGroupSms4;
#else
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
#endif
        break;

    case unifi_CredentialType8021xPeapGtc:
    case unifi_CredentialType8021xPeapMschapv2:
    {
        resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
        break;
    }
    default:
    {
        resultCode = nme_SecurityResultCodeUnknownCredentialType;
        break;
    }
    }
    return resultCode;
}

/**
 * @brief
 *   Generates the appropriate indication to inform the owner FSM
 *   of the security result
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] nme_SecurityResultCode : result
 *
 * @return
 *   void
 */
static void nme_sec_report_security_result(FsmContext* context, nme_SecurityResultCode resultCode)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_report_security_result(%d)", resultCode));

    if (!SEC_IS_SET(fsmData, SecFlag_ReportedComplete) ||
        nme_SecurityResultCodeSuccess == resultCode)
    {
        SEC_SET_FLAG(fsmData, SecFlag_ReportedComplete);
        send_nme_security_complete_ind(context, fsmData->ownerInstance, resultCode);
    }
    else
    {
        send_nme_security_abort_ind(context, fsmData->ownerInstance, resultCode);
    }
}

/**
 * @brief
 *   idle - preconnect transition
 *
 * @par Description
 *   Perform initial checks and any time sensitive pre-processing
 *   such as hashing
 *
 * @param[in] context : FSM context
 * @param[in] req     : Preconnect request
 *
 * @return
 *   void
 */
static void nme_sec_idle_preconnect(FsmContext* context, const NmeSecurityPreconnectReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    nme_SecurityResultCode resultCode;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_idle_preconnect()"));
    sme_trace_debug((TR_NME_SMGR_FSM, "credential type %s", trace_unifi_CredentialType(req->credentials.credentialType) ));

    fsmData->ownerInstance = req->common.sender_;
    fsmData->ourInstance = req->common.destination;

    sme_trace_debug((TR_NME_SMGR_FSM, "ownerInstance %d ourInstance %d", fsmData->ownerInstance, fsmData->ourInstance));

    resultCode = nme_sec_generate_security_flags(context, (unifi_Credentials*)&req->credentials);

    if (resultCode == nme_SecurityResultCodeSuccess)
    {
        /* switch on the security type required */
        switch(req->credentials.credentialType)
        {
        case unifi_CredentialTypeOpenSystem:
        {
            fsm_next_state(context, FSMSTATE_ready);

            send_nme_security_preconnect_cfm(
                    context,
                    fsmData->ownerInstance,
                    resultCode,
                    fsmData->privacyMode,
                    fsmData->authModeMask,
                    fsmData->encryptionModeMask);
            break;
        }
        case unifi_CredentialTypeWep64:
        {
            unifi_Key key;
            CsrUint8 i;
            const CsrUint8 *currentKey = req->credentials.credential.wep64Key.key1;
            /* send all 4 keys down individually */

            /* clear all fields */
            CsrMemSet(&key, 0, sizeof(unifi_Key));

            sme_trace_info((TR_NME_SMGR_FSM, "key 0x%x:0x%x:0x%x:0x%x:0x%x",
                    req->credentials.credential.wep64Key.key1[0],
                    req->credentials.credential.wep64Key.key1[1],
                    req->credentials.credential.wep64Key.key1[2],
                    req->credentials.credential.wep64Key.key1[3],
                    req->credentials.credential.wep64Key.key1[4]));

            /* and only set the ones that we are interested in */
            key.keyLength = 5;

            sme_trace_info((TR_NME_SMGR_FSM, "TxKey:%d", req->credentials.credential.wep64Key.selectedWepKey));

            for (i=0; i< 4; i++)
            {
                CsrMemCpy(key.key, currentKey, 5);
                currentKey += 5;
                key.keyIndex = (i); /* indexes 1 tp 4 */
                key.wepTxKey = FALSE;
                if((i+1) == req->credentials.credential.wep64Key.selectedWepKey)
                {
                    key.wepTxKey = TRUE;
                }

                sme_trace_info((TR_NME_SMGR_FSM, "Index:%d, TxKey:%s, Length:%d", key.keyIndex, key.wepTxKey?"TRUE":"FALSE", key.keyLength));
                sme_trace_info((TR_NME_SMGR_FSM, "key 0x%x:0x%x:0x%x:0x%x:0x%x", key.key[0],key.key[1],key.key[2],key.key[3],key.key[4]));

                nme_routing_store_cfm_prim_internal(context, UNIFI_NME_MGT_KEY_CFM_ID, fsmData->ourInstance);
                call_unifi_nme_mgt_key_req(context, NME_APP_HANDLE(context), unifi_ListActionAdd, &key);
            }

            fsmData->wepKeyCfmCount = 4;

            SEC_SET_FLAG(fsmData, SecFlag_SettingWepKeys);

            fsm_next_state(context, FSMSTATE_waiting_for_keys);

            break;
        }
        case unifi_CredentialTypeWep128:
        {
            unifi_Key key;
            CsrUint8 i;
            const CsrUint8 *currentKey = req->credentials.credential.wep128Key.key1;
            /* send all 4 keys down individually */

            /* clear all fields */
            CsrMemSet(&key, 0, sizeof(unifi_Key));

            /* and only set the ones that we are interested in */
            key.keyLength = 13;

            for (i=0; i< 4; i++)
            {
                CsrMemCpy(key.key, currentKey, 13);
                currentKey += 13;
                key.keyIndex = (i); /* indexes 1 tp 4 */
                key.wepTxKey = FALSE;
                if((i+1) == req->credentials.credential.wep128Key.selectedWepKey)
                {
                    key.wepTxKey = TRUE;
                }
                nme_routing_store_cfm_prim_internal(context, UNIFI_NME_MGT_KEY_CFM_ID, fsmData->ourInstance);
                call_unifi_nme_mgt_key_req(context, NME_APP_HANDLE(context), unifi_ListActionAdd, &key);
            }

            fsmData->wepKeyCfmCount = 4;

            SEC_SET_FLAG(fsmData, SecFlag_SettingWepKeys);

            fsm_next_state(context, FSMSTATE_waiting_for_keys);

            break;
        }
        case unifi_CredentialTypeWpaPassphrase:
        {
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
            /* Might not actually have the SSID yet */
            if (0 < req->ssid.length)
            {
                CsrCryptoWpaPskHash(req->credentials.credential.wpaPassphrase.passphrase,
                                    (CsrUint8*)req->ssid.ssid,
                                    req->ssid.length,
                                    fsmData->securityCtrlBlk.pmk);
            }
#endif

            /* request transmission of data packets */
            call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wpaProtocol, wpaOui);

            fsm_next_state(context, FSMSTATE_waiting_for_subscription);
            break;
        }
        case unifi_CredentialTypeWpaPsk:
        {
            /* request transmission of data packets */
            call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wpaProtocol, wpaOui);

            CsrMemCpy(&(fsmData->securityCtrlBlk.pmk[0]), &(req->credentials.credential.wpaPsk.psk[0]), CSR_WIFI_SECURITY_PMK_LENGTH);
            fsm_next_state(context, FSMSTATE_waiting_for_subscription);
            break;
        }

        case unifi_CredentialTypeWpa2Passphrase:
        {
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
            CsrCryptoWpaPskHash(req->credentials.credential.wpa2Passphrase.passphrase,
                                (CsrUint8*)req->ssid.ssid,
                                req->ssid.length,
                                fsmData->securityCtrlBlk.pmk);
#endif

            /* request transmission of data packets */
            call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wpaProtocol, wpaOui);

            fsm_next_state(context, FSMSTATE_waiting_for_subscription);
            break;
        }
        case unifi_CredentialTypeWpa2Psk:
        {
            /* request transmission of data packets */
            call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wpaProtocol, wpaOui);

            CsrMemCpy(&(fsmData->securityCtrlBlk.pmk[0]), &(req->credentials.credential.wpa2Psk.psk[0]), CSR_WIFI_SECURITY_PMK_LENGTH);
            fsm_next_state(context, FSMSTATE_waiting_for_subscription);
            break;
        }
        case unifi_CredentialType8021xLeap:
        {
#if defined CSR_WIFI_SECURITY_LEAP_ENABLE
            sme_trace_debug((TR_NME_SMGR_FSM, "username(%s) password(%s)", req->credentials.credential.leap.username, req->credentials.credential.leap.password ));
#endif

            /* request transmission of data packets */
            call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wpaProtocol, wpaOui);

            fsm_next_state(context, FSMSTATE_waiting_for_subscription);
            break;
        }
        case unifi_CredentialType8021xFast:
        {
#if defined CSR_WIFI_SECURITY_FAST_ENABLE
            sme_trace_debug((TR_NME_SMGR_FSM, "username(%s) password(%s) authServerUserIdentity(%s)", req->credentials.credential.fast.eapCredentials.username, req->credentials.credential.fast.eapCredentials.password , req->credentials.credential.fast.eapCredentials.authServerUserIdentity ));
            sme_trace_hex((TR_NME_CMGR_FSM, TR_LVL_DEBUG, "pacfile:", req->credentials.credential.fast.pac, req->credentials.credential.fast.pacLength));
#endif
            /* request transmission of data packets */
            call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wpaProtocol, wpaOui);

            fsm_next_state(context, FSMSTATE_waiting_for_subscription);
            break;
        }
        case unifi_CredentialType8021xTtls:
        {
#if defined CSR_WIFI_SECURITY_TTLS_ENABLE
            sme_trace_debug((TR_NME_SMGR_FSM, "username(%s) password(%s) authServerUserIdentity(%s)", req->credentials.credential.ttls.username, req->credentials.credential.ttls.password , req->credentials.credential.ttls.authServerUserIdentity ));
            sme_trace_hex((TR_NME_CMGR_FSM, TR_LVL_DEBUG, "session:", req->credentials.credential.ttls.session, req->credentials.credential.ttls.sessionLength));
#endif

            /* request transmission of data packets */
            call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wpaProtocol, wpaOui);

            fsm_next_state(context, FSMSTATE_waiting_for_subscription);
            break;
        }
        case unifi_CredentialType8021xTls:
        {
#if defined CSR_WIFI_SECURITY_TLS_ENABLE
            sme_trace_debug((TR_NME_SMGR_FSM, "username(%s) certificateLength(%d) privateKeyLength(%d)",
                    req->credentials.credential.tls.username,
                    req->credentials.credential.tls.certificateLength ,
                    req->credentials.credential.tls.privateKeyLength ));

            sme_trace_hex((TR_NME_CMGR_FSM, TR_LVL_DEBUG, "certificate", req->credentials.credential.tls.certificate, req->credentials.credential.tls.certificateLength));
            sme_trace_hex((TR_NME_CMGR_FSM, TR_LVL_DEBUG, "privateKey", req->credentials.credential.tls.privateKey, req->credentials.credential.tls.privateKeyLength));
            sme_trace_hex((TR_NME_CMGR_FSM, TR_LVL_DEBUG, "session:", req->credentials.credential.tls.session, req->credentials.credential.tls.sessionLength));
#endif

            /* request transmission of data packets */
            call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wpaProtocol, wpaOui);

            fsm_next_state(context, FSMSTATE_waiting_for_subscription);
            break;
        }
        case unifi_CredentialTypeWapi:
        case unifi_CredentialTypeWapiPsk:
        case unifi_CredentialTypeWapiPassphrase:
        {
            if (req->credentials.credentialType == unifi_CredentialTypeWapiPassphrase)
            {
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
#ifdef CSR_CRYPTO_KDHMACSHA256_ENABLE
                CsrCryptoWapiPskHash(req->credentials.credential.wapiPassphrase.passphrase,
                                     CsrStrLen(req->credentials.credential.wapiPassphrase.passphrase),
                                     fsmData->securityCtrlBlk.pmk, 16);
#endif
#endif
            }

            if (req->credentials.credentialType == unifi_CredentialTypeWapiPsk)
            {
                CsrMemCpy(&(fsmData->securityCtrlBlk.pmk[0]), &(req->credentials.credential.wapiPsk.psk[0]), CSR_WIFI_SECURITY_PMK_LENGTH);
            }

            /* request transmission of data packets */
            call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wapiProtocol, wapiOui);
            fsm_next_state(context, FSMSTATE_waiting_for_subscription);
            break;
        }
        case unifi_CredentialType8021xPeapGtc:
        case unifi_CredentialType8021xPeapMschapv2:
        {
            /* nothing to do here for Release 1.0*/
            send_nme_security_preconnect_cfm(
                    context,
                    fsmData->ownerInstance,
                    resultCode,
                    fsmData->privacyMode,
                    fsmData->authModeMask,
                    fsmData->encryptionModeMask);
            break;
        }

        }

        nme_security_init(context,
                          fsmData->ourInstance,
                          &fsmData->securityCtrlBlk,
                          FALSE, /* WPS */
                          NULL, /* WPS PIN */
                          &req->credentials,
                          req->ssid,
                          req->stationMacAddress);
    }
    else
    {
        /* connection failed */
        send_nme_security_preconnect_cfm(
                context,
                fsmData->ownerInstance,
                resultCode,
                fsmData->privacyMode,
                fsmData->authModeMask,
                fsmData->encryptionModeMask);
    }
}

/**
 * @brief
 *   idle - preconnect transition
 *
 * @par Description
 *   Perform initial checks and any time sensitive pre-processing
 *   such as hashing
 *
 * @param[in] context : FSM context
 * @param[in] req     : Preconnect request
 *
 * @return
 *   void
 */
static void nme_sec_idle_prewps(FsmContext* context, const NmeSecurityPrewpsReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_idle_prewps()"));

    fsmData->ownerInstance = req->common.sender_;
    fsmData->ourInstance = req->common.destination;

    sme_trace_debug((TR_NME_SMGR_FSM, "ownerInstance %d ourInstance %d", fsmData->ownerInstance, fsmData->ourInstance));

    SEC_SET_FLAG(fsmData, SecFlag_Wps);
    nme_security_init(context,
                      fsmData->ourInstance,
                      &fsmData->securityCtrlBlk,
                      TRUE, /* WPS */
                      (CsrUint8 *)&(req->pin[0]), /* WPS PIN */
                      NULL, /* credentials - WPS so we don't have any yet */
                      req->ssid,
                      req->stationMacAddress);

    /* request transmission of data packets */
    call_unifi_nme_sys_ma_unitdata_subscribe_req(context, NME_APP_HANDLE(context), unifi_Llc_Snap, wpaProtocol, wpaOui);
    fsm_next_state(context, FSMSTATE_waiting_for_subscription);
}


/**
 * @brief
 *   set keys event
 *
 * @par Description   CsrUint8 iTimerId = 0;
 *
 *   The security Lib has request keys to be install
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityLibsetkeyInd_Evt*: set key request
 *
 * @return
 *   void
 */
static void nme_sec_setkey_ind(FsmContext* context, const NmeSecurityLibsetkeyInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_setkey_ind(%s)", trace_KeyType(ind->key.keyType) ));

    if(ind->key.keyType == unifi_PairwiseKey)
    {
        SEC_SET_FLAG(fsmData, SecFlag_SettingPairWiseKey);
    }
    else if(ind->key.keyType == unifi_GroupKey)
    {
        SEC_SET_FLAG(fsmData, SecFlag_SettingGroupKey);
    }
    else
    {
        /* ignore */
        sme_trace_entry((TR_NME_SMGR_FSM, "Unknown Key Type(%d)", ind->key.keyType ));
        return;
    }

    nme_routing_store_cfm_prim_internal(context, UNIFI_NME_MGT_KEY_CFM_ID, fsmData->ourInstance);
    call_unifi_nme_mgt_key_req(context, NME_APP_HANDLE(context), unifi_ListActionAdd, &ind->key);
}

/**
 * @brief
 *   set PAC event
 *
 * @par Description
 *   The security Lib has requested a FAST PAC to be install
 *   this new PAC has to be stored in the profile for future use
 *   but only if the current security procedure is successful.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityLibsetpacInd_Evt* : set pac request
 *
 * @return
 *   void
 */
static void nme_sec_setpac_ind(FsmContext* context, const NmeSecurityLibsetpacInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_setpac_ind()"));
    SEC_SET_FLAG(fsmData, SecFlag_PacUpdated);
}

/**
 * @brief
 *   sec session id event
 *
 * @par Description
 *   The security Lib has requested a session id has to be
 *   stored in the profile for future use but only if the
 *   current security procedure is successful.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityLibsetsessionInd_Evt* : set session id request
 *
 * @return
 *   void
 */
static void nme_sec_setsession_ind(FsmContext* context, const NmeSecurityLibsetsessionInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_setsession_ind()"));
    SEC_SET_FLAG(fsmData, SecFlag_SessionUpdated);
}


/**
 * @brief
 *   set wps done id event
 *
 * @par Description
 *   The security Lib has indicated the completion of the WPS process.
 *   The Security FSM will need to create and store a profile with the returned data
 *   stored in relation to the profile for future use.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityLibsetsessionInd_Evt* : set session id request
 *
 * @return
 *   void
 */
static void nme_sec_libwpsdone_ind(FsmContext* context, const NmeSecurityLibwpsdoneInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_libwpsdone_ind()"));

    /* take the credentials */
    send_nme_security_complete_ind(context, fsmData->ownerInstance, nme_SecurityResultCodeSuccess);

}
/**
 * @brief
 *   set certificate validate event
 *
 * @par Description
 *   The security Lib has requested validation of the supplied
 *   server side certificate.
 *
 * @param[in] context : FSM context
 * @param[in] const NmeSecurityLibsetcertificatevalidationInd_Evt* : indication from sec lib
 *
 * @return
 *   void
 */
static void nme_sec_setcertvalidate_ind(
        FsmContext* context,
        const NmeSecurityLibsetcertificatevalidationInd_Evt* ind)
{
    void* appHandles;
    CsrUint16 appHandleCount = 0;
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_setcertvalidate_ind()"));
    require(TR_NME_SMGR_FSM, 0 < fsmData->securityCtrlBlk.certificateLength && NULL != fsmData->securityCtrlBlk.certificate);

    appHandleCount = nme_routing_get_ind_apphandles(context, unifi_IndCertificateValidate, &appHandles);
    if (appHandleCount)
    {
        fsmData->certificateValidationId = nme_signal_routing_get_dyn_fsm_msg_id(context, NME_DYN_SECURITY_MANAGER_FSM);
        call_unifi_nme_certificate_validate_ind(
                context,
                1, /* Only ever send one indication as we only require one response */
                appHandles,
                fsmData->certificateValidationId,
                fsmData->securityCtrlBlk.certificateLength,
                fsmData->securityCtrlBlk.certificate);
    }
    else
    {
        sme_trace_warn((TR_NME_SMGR_FSM, "No appHandles registered to receive nme_certificate_validate_ind"));
        /* @TODO inform security lib that the certificate is not valid.
         * The interface into security for this has to be written.
         */
    }
    /* No longer need the certificate */
    CsrPfree(fsmData->securityCtrlBlk.certificate);
    fsmData->securityCtrlBlk.certificateLength = 0;
}

/**
 * @brief
 *   Start transition
 *
 * @par Description
 *   Starts the Security Library
 *
 * @param[in]    context   : FSM context
 * @param[in]    req       : Start request
 *
 * @return
 *   void
 */
static void nme_sec_start_req(FsmContext* context, const NmeSecurityStartReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_start_req()"));

    fsmData->isCtrlBlkInitialised = TRUE;

    sme_trace_hex((TR_NME_CMGR_FSM, TR_LVL_DEBUG, "ready_start", req->ie, req->ieLength));

    nme_security_start(context, &fsmData->securityCtrlBlk, req->ieLength, req->ie, req->peerSsid, req->peerMacAddress);
    send_nme_security_start_cfm(context, fsmData->ownerInstance, unifi_Success);

    if (!SEC_IS_SET(fsmData, SecFlag_Wps))
    {
        /* For some credential types there are no further actions required */
        if (unifi_CredentialTypeOpenSystem == fsmData->securityCtrlBlk.credentials.credentialType ||
            unifi_CredentialTypeWep64 == fsmData->securityCtrlBlk.credentials.credentialType ||
            unifi_CredentialTypeWep128 == fsmData->securityCtrlBlk.credentials.credentialType)
        {
            SEC_SET_FLAG(fsmData, SecFlag_ReportedComplete);
            send_nme_security_complete_ind(context, fsmData->ownerInstance, nme_SecurityResultCodeSuccess);
        }
    }
    fsm_next_state(context, FSMSTATE_started);
}

/**
 * @brief
 *   Started stop transition
 *
 * @par Description
 *   Stops the Security Library but doesn't terminate the security
 *   manager FSM, as it will possibly be required to restart with the
 *   same credentials.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityStopReq_Evt* : stop request
 *
 * @return
 *   void
 */
static void nme_sec_started_stop_req(
        FsmContext* context,
        const NmeSecurityStopReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_started_stop_req()"));

    nme_security_stop(context, &fsmData->securityCtrlBlk);
    send_nme_security_stop_cfm(context, fsmData->ownerInstance, unifi_Success);
    fsm_next_state(context, FSMSTATE_ready);
}

/**
 * @brief
 *   Not started stop transition
 *
 * @par Description
 *   Shouldn't really get a stop req if the FSM is not started.
 *   Return success as the FSM is not in the started state.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityStopReq_Evt* : stop request
 *
 * @return
 *   void
 */
static void nme_sec_not_started_stop_req(
        FsmContext* context,
        const NmeSecurityStopReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_not_started_stop_req()"));
    send_nme_security_stop_cfm(context, fsmData->ownerInstance, unifi_Success);
}

/**
 * @brief
 *   Restart transition
 *
 * @par Description
 *   Restarts the Security Library
 *
 * @param[in]  FsmContext* : FSM context
 * @param[in]  const NmeSecurityRestartReq_Evt* : restart request
 *
 * @return
 *   void
 */
static void nme_sec_restart_req(FsmContext* context, const NmeSecurityRestartReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_restart_req()"));

    sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "started_started", req->ie, req->ieLength));

    nme_security_restart(context, &fsmData->securityCtrlBlk, req->ieLength, req->ie, req->peerSsid, req->peerMacAddress);
    send_nme_security_restart_cfm(context, fsmData->ownerInstance, unifi_Success);

    /* For some credential types there are no further actions required */
    if (unifi_CredentialTypeOpenSystem == fsmData->securityCtrlBlk.credentials.credentialType ||
        unifi_CredentialTypeWep64 == fsmData->securityCtrlBlk.credentials.credentialType ||
        unifi_CredentialTypeWep128 == fsmData->securityCtrlBlk.credentials.credentialType)
    {
        nme_sec_report_security_result(context, nme_SecurityResultCodeSuccess);
    }
    else
    {
        /* For the others we have to report when have completed so that connection
         * manager can move from authenticating to connected.
         */
        SEC_CLEAR_FLAG(fsmData, SecFlag_ReportedComplete);
    }
    fsm_next_state(context, FSMSTATE_started);
}

/**
 * @brief
 *   MaUnitdata transition
 *
 * @par Description
 *   passes the 4 way hand shake message to the security Library
*
 * @param[in] context : FSM context
 * @param[in] ind     : MaUnitdata
 *
 * @return
 *   void
 */
static void nme_sec_ma_unitdata_ind(FsmContext* context, const UnifiNmeSysMaUnitdataInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    const FsmEvent *pSavedMaUnitDataInd = NULL;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_ma_unitdata_ind"));
    sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "MA_UNITDATA_IND frame", ind->frame, ind->frameLength));

    /* Check that this is not an out of date packet by ensuring that there are
     * no further ma unitdata inds in the saved queue. If there is then just
     * free the current one.
     */
    pSavedMaUnitDataInd = fsm_sniff_saved_event(context, UNIFI_NME_SYS_MA_UNITDATA_IND_ID);
    if (NULL != pSavedMaUnitDataInd)
    {
        if (ind->freeFunction)
        {
            (*ind->freeFunction)(ind->frame);
        }
        else
        {
            CsrPfree(ind->frame);
        }
    }
    else
    {
        /* If there is a free function defined with this message then we
         * need to take a copy of the buffer and then call the free function.
         * This is because the free function could be different on a per
         * packet basis and we don't want to have to try and track the free function
         * for packets that we've passed in to the security library and need to
         * free when the security lib has finished with them.
         */
        if (NULL == ind->freeFunction)
        {
            /* No free function so we can just pass the buffer into security and call
             * CsrPfree when it is finished.
             */
            nme_security_process_ma_unitdata_packet(context, &fsmData->securityCtrlBlk, ind->frame,  ind->frameLength);
        }
        else
        {
            CsrUint8 *pFrame = CsrPmalloc(ind->frameLength);
            CsrMemCpy(pFrame, ind->frame, ind->frameLength);
            nme_security_process_ma_unitdata_packet(context, &fsmData->securityCtrlBlk, pFrame, ind->frameLength);
            (*ind->freeFunction)(ind->frame);
        }
    }
}

/**
 * @brief
 *   NmeSysEapolCfm transition
 *
 * @par Description
 *   Response from to a Security Lib requested EAPOL req.
 *   If the transmission failed due to Michael MIC failure
 *   then security has failed and the connection has to be
 *   dropped.
 *   If there was just a problem with the EAPOL transmission
 *   then it can either be left to the security library EAPOL
 *   transmission error handling. Or by enabling the NME_ABORT_SEC_ON_EAPOL_PROBLEM
 *   define at compilation time it will be treated as an error
 *   and the connection will be terminated.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeSysEapolCfm_Evt * : NmeSysEapolCfm
 *
 * @return
 *   void
 */
static void nme_sec_eapol_cfm(FsmContext* context, const UnifiNmeSysEapolCfm_Evt *cfm)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_eapol_cfm(0x%.4X:%s)", cfm->result, trace_unifi_EapolRc(cfm->result)));

    /* Only interested if the Michael mic flag is set, otherwise it is ignored (no message contents to free) */
    if (SEC_IS_SET(fsmData, SecFlag_MichealMicEapolErrorSent))
    {
        sme_trace_warn((TR_NME_SMGR_FSM, "Terminating security due to mic failure"));
        nme_sec_report_security_result(context, nme_SecurityResultCodeSecondMicFailure);
        nme_sec_security_manager_cleanup(context);
    }
    else if (cfm->result != unifi_EapolRcSuccess)
    {
        sme_trace_warn((TR_NME_SMGR_FSM, "result code %d != success", cfm->result));
        /* Enable to following define to terminate the security lib and fail the
         * connection.
         * If not enabled the security library will handle the failure with it's
         * own internal EAPOL transmission error handling.
         */
#ifdef NME_ABORT_SEC_ON_EAPOL_PROBLEM
        nme_sec_report_security_result(context, nme_SecurityResultCodeFailed);
        nme_sec_security_manager_cleanup(context);
#endif
    }
}

/**
 * @brief
 *   UnifiNmeSysMaUnitdataSubscribeCfm transition
 *
 * @par Description
 *   Event from the driver to convey subscription status
 *
 * @param[in] context : FSM context
 * @param[in] cfm     : UnifiNmeSysMaUnitdataSubscribeCfm
 *
 * @return
 *   void
 */
static void nme_sec_ma_unitdata_subscribe_cfm(FsmContext* context, const UnifiNmeSysMaUnitdataSubscribeCfm_Evt *cfm)
{
    FsmData* fsmData = FSMDATA;
    nme_SecurityResultCode result = nme_SecurityResultCodeSubscriptionFailed;
    sme_trace_entry((TR_NME_SMGR_FSM, "ma_unitdata_subscribe_cfm status:%s", trace_unifi_SubscriptionResult(cfm->status) ));

    if(cfm->status == unifi_SubscriptionResultSuccess )
    {
        /* eapol path now open */
        sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_ma_unitdata_subscribe_cfm status: subscriptionHandle %d, allocOffset %d", cfm->subscriptionHandle, cfm->allocOffset ));

        SEC_SET_FLAG(fsmData, SecFlag_EapolPathOpen);
        result = nme_SecurityResultCodeSuccess;
        fsmData->securityCtrlBlk.subscriptionHandle = cfm->subscriptionHandle;
        fsmData->securityCtrlBlk.allocOffset = cfm->allocOffset;
        fsm_next_state(context, FSMSTATE_ready);
    }
    else
    {
        /* fail and disconnect */
        fsm_next_state(context, FSMSTATE_idle);
    }
    /* Inform the security manager FSM */
    if (SEC_IS_SET(fsmData, SecFlag_Wps))
    {
        send_nme_security_prewps_cfm(
                context,
                fsmData->ownerInstance,
                result);
    }
    else
    {
        send_nme_security_preconnect_cfm(
                context,
                fsmData->ownerInstance,
                result,
                fsmData->privacyMode,
                fsmData->authModeMask,
                fsmData->encryptionModeMask);
    }
}

/**
 * @brief
 *   UnifiNmeSysMaUnitdataUnSubscribeCfm transition
 *
 * @par Description
 *   Event from the driver to convey subscription status
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeSysMaUnitdataUnsubscribeCfm_Evt* : cfm
 *
 * @return
 *   void
 */
static void nme_sec_ma_unitdata_unsubscribe_cfm(FsmContext* context, const UnifiNmeSysMaUnitdataUnsubscribeCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_ma_unitdata_unsubscribe_cfm status:%s", trace_unifi_SubscriptionResult(cfm->status) ));

    /* Just assume that it was successful */
    SEC_CLEAR_FLAG(fsmData, SecFlag_EapolPathOpen);
    nme_sec_security_manager_cleanup(context);
}

/**
 * @brief
 *   NmeMgtSetkeys cfm transition
 *
 * @par Description
 *   Confirmation that the keys have been set
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeMgtKeyCfm_Evt* : NmeMgtSetkeys
 *
 * @return
 *   void
 */
static void nme_sec_set_key_cfm(FsmContext* context, const UnifiNmeMgtKeyCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;
    CsrBool sendConfirmation = FALSE;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_set_key_cfm status:%s", trace_unifi_Status(cfm->status) ));

    if (SEC_IS_SET(fsmData, SecFlag_SettingWepKeys))
    {
        fsmData->wepKeyCfmCount--;
        sme_trace_entry((TR_NME_SMGR_FSM, "SecFlag_SettingWepKeys"));

        /* if confirmation for all keys has been received, inform the connection manager */
        if(fsmData->wepKeyCfmCount == 0)
        {
            SEC_CLEAR_FLAG(fsmData, SecFlag_SettingWepKeys);

            if (cfm->status == unifi_Success)
            {
                SEC_SET_FLAG(fsmData, SecFlag_HaveWepKeys);
            }
        }
    }
    if (SEC_IS_SET(fsmData, SecFlag_SettingPairWiseKey))
    {
        sme_trace_entry((TR_NME_SMGR_FSM, "SecFlag_SettingPairWiseKey"));
        SEC_CLEAR_FLAG(fsmData, SecFlag_SettingPairWiseKey);

        if (cfm->status == unifi_Success)
        {
            sme_trace_entry((TR_NME_SMGR_FSM, "SecFlag_HavePairWiseKey"));
            SEC_SET_FLAG(fsmData, SecFlag_HavePairWiseKey);
            sendConfirmation = TRUE;
        }
    }
    if (SEC_IS_SET(fsmData, SecFlag_SettingGroupKey))
    {
        sme_trace_entry((TR_NME_SMGR_FSM, "SecFlag_SettingGroupKey"));
        SEC_CLEAR_FLAG(fsmData, SecFlag_SettingGroupKey);

        if (cfm->status == unifi_Success)
        {
            sme_trace_entry((TR_NME_SMGR_FSM, "SecFlag_HaveGroupKey"));
            SEC_SET_FLAG(fsmData, SecFlag_HaveGroupKey);
            sendConfirmation = TRUE;
        }
    }

    if (SEC_IS_SET(fsmData, SecFlag_HaveWepKeys)  && (!(fsmData->securityCtrlBlk.credentials.credentialType == unifi_CredentialType8021xLeap)))
    {
        sme_trace_entry((TR_NME_SMGR_FSM, "SecFlag_HaveWepKeys"));
        /*all keys installed*/
        send_nme_security_preconnect_cfm(
                context,
                fsmData->ownerInstance,
                nme_SecurityResultCodeSuccess,
                fsmData->privacyMode,
                fsmData->authModeMask,
                fsmData->encryptionModeMask);

        fsm_next_state(context, FSMSTATE_ready);
    }
    /* 1 or both keys have changed */
    else if (sendConfirmation &&
             (SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey)) && (SEC_IS_SET(fsmData, SecFlag_HaveGroupKey)))
    {
        unifi_ProfileIdentity profileIdentity;

        sme_trace_entry((TR_NME_SMGR_FSM, "SecFlag_HavePairWiseKey && SecFlag_HaveGroupKey"));
        send_nme_security_keys_installed_ind(
                context,
                fsmData->ownerInstance);

        /* Pass on any PAC or session update to profile manager */
        if (SEC_IS_SET(fsmData, SecFlag_PacUpdated))
        {
            profileIdentity = nme_connection_manager_get_profile_identity(context);
            nme_profile_manager_update_profile_pac(
                    context,
                    &profileIdentity,
                    fsmData->securityCtrlBlk.pac,
                    fsmData->securityCtrlBlk.pacLength);

            SEC_CLEAR_FLAG(fsmData, SecFlag_PacUpdated);
        }
        if (SEC_IS_SET(fsmData, SecFlag_SessionUpdated))
        {
            profileIdentity = nme_connection_manager_get_profile_identity(context);
            nme_profile_manager_update_profile_session(
                    context,
                    &profileIdentity,
                    fsmData->securityCtrlBlk.session,
                    fsmData->securityCtrlBlk.sessionLength);

            SEC_CLEAR_FLAG(fsmData, SecFlag_SessionUpdated);
        }
    }

    if (!SEC_IS_SET(fsmData, SecFlag_ReportedComplete) &&
        (SEC_IS_SET(fsmData, SecFlag_HavePairWiseKey)) && (SEC_IS_SET(fsmData, SecFlag_HaveGroupKey)))
    {
        SEC_SET_FLAG(fsmData, SecFlag_ReportedComplete);
        send_nme_security_complete_ind(context, fsmData->ownerInstance, nme_SecurityResultCodeSuccess);

        if(fsmData->securityCtrlBlk.credentials.credentialType == unifi_CredentialType8021xLeap)
        {
            SEC_SET_FLAG(fsmData, SecFlag_HaveWepKeys);
        }

    }
}

/**
 * @brief
 *   NmeMgtDeletekeys cfm transition
 *
 * @par Description
 *   Confirmation that the keys have been deleted
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeMgtKeyCfm_Evt* : UnifiNmeMgtKeyCfm
 *
 * @return
 *   void
 */
static void nme_sec_delete_key_cfm(FsmContext* context, const UnifiNmeMgtKeyCfm_Evt* cfm)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_delete_key_cfm status:%s", trace_unifi_Status(cfm->status) ));

    SEC_CLEAR_FLAG(fsmData, SecFlag_HaveWepKeys);
    nme_sec_security_manager_cleanup(context);
}

/**
 * @brief
 *   NmeSecurityEapTimer transition
 *
 * @par Description
 *   The EAPOL Timer has fired, inform the security lib
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityEapTimer_Evt* : NmeSecurityEapTimer
 *
 * @return
 *   void
 */
static void nme_sec_eap_timer_ind(FsmContext* context, const NmeSecurityEapTimer_Evt* timer)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_eap_timer_ind(id: %d)", timer->common.timerid.uniqueid));

    nme_security_timer_expired(context, &fsmData->securityCtrlBlk, timer->timeoutIndex);
}

/**
 * @brief
 *   NmeSecurityLibAbortInd transition
 *
 * @par Description
 *   indication from the security Library that an abort has been issued.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityLibabortInd_Evt* : NmeSecurityAbortInd
 *
 * @return
 *   void
 */
static void nme_sec_security_abort_ind(FsmContext* context, const NmeSecurityLibabortInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    NmeConfigData* cfg = nme_core_get_nme_config(context);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_security_abort_ind" ));

    /* Remove any pmkid */
    if (nme_pmk_cache_delete(&(cfg->pmkCache), &(fsmData->securityCtrlBlk.peerMacAddress)))
    {
        unifi_Pmkid *pRemovePmkids = CsrPmalloc(sizeof(*pRemovePmkids));
        CsrMemSet(pRemovePmkids, 0, sizeof(*pRemovePmkids));
        CsrMemCpy(pRemovePmkids->bssid.data, fsmData->securityCtrlBlk.peerMacAddress.data, sizeof(fsmData->securityCtrlBlk.peerMacAddress.data));

        /* Send to the SME as well (Use FSM_TERMINATE with the routing as we do not care about the cfm */
        nme_routing_store_cfm_prim_internal(context, UNIFI_NME_PMKID_CFM_ID, FSM_TERMINATE);
        call_unifi_nme_mgt_pmkid_req(context,
                                     NME_APP_HANDLE(context),
                                     unifi_ListActionRemove,
                                     1,
                                     pRemovePmkids);
        CsrPfree(pRemovePmkids);
    }
    /* Report the failure to the "owner" FSM */
    /* Ensure that any outstanding profile contents are not passed to Profile
     * Manager as the security procedure has failed.
     */
    SEC_CLEAR_FLAG(fsmData, SecFlag_PacUpdated);
    SEC_CLEAR_FLAG(fsmData, SecFlag_SessionUpdated);

    nme_sec_report_security_result(context, nme_SecurityResultCodeFailed);
    fsm_next_state(context, FSMSTATE_waiting_for_terminate);
}

/**
 * @brief
 *   NmeSecurityTerminateReq transition
 *
 * @par Description
 *   The Security Manager has been requested to terminate.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const NmeSecurityTerminateReq_Evt* : NmeSecurityTerminateReq
 *
 * @return
 *   void
 */
static void nme_sec_terminate_req(FsmContext* context, const NmeSecurityTerminateReq_Evt* req)
{
    FsmData* fsmData = FSMDATA;

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_terminate_req"));

    SEC_SET_FLAG(fsmData, SecFlag_terminateReceived);
    nme_sec_security_manager_cleanup(context);
}

/**
 * @brief
 *   UnifiNmeMgtMicFailureInd transition
 *
 * @par Description
 *   Message from the SME to indicate that a Michael Mic failure has been detected.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeMgtMicFailureInd_Evt* : MlmeMichaelmicfailure
 *
 * @return
 *   void
 */
static void nme_sec_mic_failure_ind(FsmContext* context, const UnifiNmeMgtMicFailureInd_Evt* ind)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_mic_failure_ind :count %d, secondfailure %d", ind->count, ind->secondFailure));

    nme_handle_michael_mic_failure(context, &fsmData->securityCtrlBlk, ind->keyType, (CsrUint8*)ind->tsc);
    /* we need to wait till we have seen a count of 2 */
    if (ind->secondFailure)
    {
        SEC_SET_FLAG(fsmData, SecFlag_MichealMicEapolErrorSent);
    }
}

/**
 * @brief
 *   UnifiNmeCertificateValidateRsp transition
 *
 * @par Description
 *   Message from outside the NME returning the response to a
 *   certificate validate ind. If this response matches the
 *   current msg id expected then it is forwarded to security
 *   otherwise the msg is ignored as it must related to a
 *   previous certificate validate ind which we are no longer
 *   interested in.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] const UnifiNmeCertificateValidateRsp_Evt* : certificate validate rsp
 *
 * @return
 *   void
 */
static void nme_sec_certificate_validate_rsp(
        FsmContext* context,
        const UnifiNmeCertificateValidateRsp_Evt* rsp)
{
    FsmData* fsmData = FSMDATA;
    sme_trace_entry((TR_NME_SMGR_FSM, "nme_sec_certificate_validate_rsp(status=%d,result=%d)", rsp->status, rsp->result));

    if (rsp->validationId == fsmData->certificateValidationId)
    {
        /* @TODO: Pass the result onto the security library
         * No interface into the security lib for this yet
         */
        sme_trace_debug((TR_NME_SMGR_FSM, "Passing to security"));
    }
    else
    {
        sme_trace_debug((TR_NME_SMGR_FSM, "ignored id=%d, waiting for %d", rsp->validationId, fsmData->certificateValidationId));
    }
}
/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in nme_security_manager_fsm/nme_security_manager_fsm.h
 */
/*---------------------------------------------------------------------------*/
unifi_Credentials *nme_sec_get_credentials(FsmContext* context)
{
    unifi_Credentials *pCredentials = NULL;
    CsrUint16 securityInstance = nme_ntw_selector_get_security_fsm_instance(context);

    if (FSM_TERMINATE != securityInstance)
    {
        FsmData *fsmData = fsm_get_params_by_id(context, securityInstance, FsmData);

        if (fsmData->isCtrlBlkInitialised)
        {
            pCredentials = CsrPmalloc(sizeof(*pCredentials));
            CsrMemCpy(pCredentials, &(fsmData->securityCtrlBlk.credentials), sizeof(*pCredentials));
        }
    }

    return(pCredentials);
}

/*
 * Description:
 * See description in nme_security_manager_fsm/nme_security_manager_fsm.h
 */
/*---------------------------------------------------------------------------*/
unifi_SSID nme_sec_get_wps_ssid(FsmContext* context)
{
    unifi_SSID ssid;
    CsrUint16 securityInstance = nme_ntw_selector_get_security_fsm_instance(context);

    if (FSM_TERMINATE != securityInstance)
    {
        FsmData *fsmData = fsm_get_params_by_id(context, securityInstance, FsmData);

        if (fsmData->isCtrlBlkInitialised)
        {
            return fsmData->securityCtrlBlk.WpsSsid;
        }
    }
    ssid.length = 0;
    return ssid;
}


/*
 * Description:
 * See description in nme_security_manager_fsm/nme_security_manager_fsm.h
 */
/*---------------------------------------------------------------------------*/
unifi_AuthMode nme_sec_get_auth_mode_mask(FsmContext* context)
{
    CsrUint16 securityInstance = nme_ntw_selector_get_security_fsm_instance(context);

    if (FSM_TERMINATE != securityInstance)
    {
        FsmData *fsmData = fsm_get_params_by_id(context, securityInstance, FsmData);

        if (fsmData->isCtrlBlkInitialised)
        {
            return fsmData->authModeMask;
        }
    }
    return unifi_80211AuthOpen;
}

/* FSM DEFINITION **********************************************/

static const FsmEventEntry idleTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(NME_SECURITY_PRECONNECT_REQ_ID,               nme_sec_idle_preconnect ),
    fsm_event_table_entry(NME_SECURITY_PREWPS_REQ_ID,                   nme_sec_idle_prewps ),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,             fsm_ignore_event ),
};

static const FsmEventEntry waitingForSubscriptionTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_SUBSCRIBE_CFM_ID,   nme_sec_ma_unitdata_subscribe_cfm ),
};

static const FsmEventEntry waitingForKeysTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_NME_MGT_KEY_CFM_ID,                     nme_sec_set_key_cfm ),
};

static const FsmEventEntry readyTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(NME_SECURITY_START_REQ_ID,                    nme_sec_start_req ),
    fsm_event_table_entry(NME_SECURITY_RESTART_REQ_ID,                  nme_sec_restart_req ),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,             fsm_saved_event ),
};

static const FsmEventEntry startedTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,             nme_sec_ma_unitdata_ind ),
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                   nme_sec_eapol_cfm ),
    fsm_event_table_entry(UNIFI_NME_MGT_KEY_CFM_ID,                     nme_sec_set_key_cfm ),
    fsm_event_table_entry(NME_SECURITY_RESTART_REQ_ID,                  nme_sec_restart_req ),
    fsm_event_table_entry(NME_SECURITY_STOP_REQ_ID,                     nme_sec_started_stop_req ),
    fsm_event_table_entry(NME_SECURITY_LIBSETCERTIFICATEVALIDATION_IND_ID, nme_sec_setcertvalidate_ind),
    fsm_event_table_entry(UNIFI_NME_CERTIFICATE_VALIDATE_RSP_ID,        nme_sec_certificate_validate_rsp),
};

static const FsmEventEntry waitingForTerminateTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(NME_SECURITY_RESTART_REQ_ID,                  nme_sec_restart_req ),
    fsm_event_table_entry(NME_SECURITY_TERMINATE_REQ_ID,                nme_sec_terminate_req ),
    /* Ignore the following events as we are waiting for terminate event. */
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                   fsm_ignore_event ),
    fsm_event_table_entry(NME_SECURITY_EAP_TIMER_ID,                    fsm_ignore_event ),
    fsm_event_table_entry(NME_SECURITY_LIBSETKEY_IND_ID,                fsm_ignore_event ),
    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,             fsm_ignore_event ),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,             fsm_ignore_event ),
};

static const FsmEventEntry terminatingTransitions[] =
{
    /*                    Signal Id,                                    Function */
    /* Once in the terminating state several actions may need to be performed before
     * the FSM can actually be terminated. The required actions are based on the flags
     * that are set. Only once all the required actions are complete will the terminating cfm
     * be sent.
     */
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_UNSUBSCRIBE_CFM_ID, nme_sec_ma_unitdata_unsubscribe_cfm ),
    fsm_event_table_entry(UNIFI_NME_MGT_KEY_CFM_ID,                     nme_sec_delete_key_cfm ),

    /* Ignore the following events as we are terminating. */
    fsm_event_table_entry(UNIFI_NME_SYS_EAPOL_CFM_ID,                   fsm_ignore_event ),
    fsm_event_table_entry(NME_SECURITY_EAP_TIMER_ID,                    fsm_ignore_event ),
    fsm_event_table_entry(NME_SECURITY_LIBABORT_IND_ID,                 fsm_ignore_event ),
    fsm_event_table_entry(NME_SECURITY_LIBSETKEY_IND_ID,                fsm_ignore_event ),
    fsm_event_table_entry(NME_SECURITY_LIBSETCERTIFICATEVALIDATION_IND_ID, fsm_ignore_event),
    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,             fsm_ignore_event ),
    fsm_event_table_entry(UNIFI_NME_SYS_MA_UNITDATA_IND_ID,             fsm_ignore_event ),
};

/** Standard handlers if the event is not handled in the current state */
static const FsmEventEntry unhandledTransitions[] =
{
    /*                    Signal Id,                                    Function */
    fsm_event_table_entry(NME_SECURITY_TERMINATE_REQ_ID,                nme_sec_terminate_req ),
    fsm_event_table_entry(NME_SECURITY_EAP_TIMER_ID,                    nme_sec_eap_timer_ind ),
    fsm_event_table_entry(NME_SECURITY_LIBABORT_IND_ID,                 nme_sec_security_abort_ind ),
    fsm_event_table_entry(NME_SECURITY_LIBSETKEY_IND_ID,                nme_sec_setkey_ind ),
    fsm_event_table_entry(NME_SECURITY_LIBSETPAC_IND_ID,                nme_sec_setpac_ind ),
    fsm_event_table_entry(NME_SECURITY_LIBSETSESSION_IND_ID,            nme_sec_setsession_ind ),
    fsm_event_table_entry(NME_SECURITY_LIBWPSDONE_IND_ID,               nme_sec_libwpsdone_ind ),
    fsm_event_table_entry(UNIFI_NME_MGT_MIC_FAILURE_IND_ID,             nme_sec_mic_failure_ind ),

    /* As we have no control over when the RSP to the NME CERTIFICATE VALIDATE IND is
     * received, it could arrive too late and the response has to be ignored.
     */
    fsm_event_table_entry(UNIFI_NME_CERTIFICATE_VALIDATE_RSP_ID,        fsm_ignore_event ),

    /* Stop requests are only handled in the started state */
    fsm_event_table_entry(NME_SECURITY_STOP_REQ_ID,                     nme_sec_not_started_stop_req ),
};


/** Security Manager state table definition */
static const FsmTableEntry stateTable[FSMSTATE_MAX_STATE] =
{
   /*                         State                         State                               Save     */
   /*                         Name                          Transitions                         *       */
   fsm_state_table_entry(FSMSTATE_idle,                     idleTransitions,                    FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_for_subscription, waitingForSubscriptionTransitions,  FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_for_keys,         waitingForKeysTransitions,          FALSE),
   fsm_state_table_entry(FSMSTATE_ready,                    readyTransitions,                   FALSE),
   fsm_state_table_entry(FSMSTATE_started,                  startedTransitions,                 FALSE),
   fsm_state_table_entry(FSMSTATE_waiting_for_terminate,    waitingForTerminateTransitions,     TRUE),
   fsm_state_table_entry(FSMSTATE_terminating,              terminatingTransitions,             FALSE)
};

const FsmProcessStateMachine nme_security_manager_fsm = {
#ifdef FSM_DEBUG
       "Nme_Security Mgr",                                          /* SM Process Name       */
#endif
       NME_SECURITY_MANAGER_PROCESS,                                /* SM Process ID         */
       {FSMSTATE_MAX_STATE, stateTable},                            /* Transition Tables     */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, unhandledTransitions, FALSE), /* Handled event handers */
       fsm_state_table_entry(FSMSTATE_MAX_STATE, NULL, FALSE),      /* Ignore Event handers */
       nme_sec_init,                                                /* Entry Function        */
       nme_sec_reset,                                               /* Reset Function        */
#ifdef FSM_DEBUG_DUMP
       NULL                                                         /* Trace Dump Function   */
#endif
};

/*
 * FSM Scripts Config for this FSM
 *                              state                                       nextstate                  event
 *                              -----                                       ---------                  -----
 * fsm_to_dot config
 * fsm::fsm_to_dot graph_attributes rankdir=LR;
 *
 */

/** @}
 */
