/** @file nme_security.c
 *
 * Network Management Entity security
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
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_security_manager_fsm/nme_security.c#16 $
 *
 ****************************************************************************/

/** @{
 * @ingroup nme_security
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/csr_wifi_fsm.h"
#include "nme_top_level_fsm/nme_top_level_fsm.h"
#include "nme_security_manager_fsm/nme_pmk_cache.h"
#include "lib_info_element.h"
#include "nme_security.h"
#include "csr_security.h"

#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
#include "csr_passphrase_hashing.h"
#endif

/* MACROS *******************************************************************/

static const CsrUint8 eapolllcheader[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8E};
static const CsrUint8 wapillcheader[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x88, 0xB4};

#define CSR_SEC_RETRANSMISSION_TIMEOUT_WPA  (100)   /* milliseconds */
#define CSR_SEC_RETRANSMISSION_TIMEOUT_WAPI (3000)  /* milliseconds */
#define CSR_SEC_RETRANSMISSION_COUNT        (3)

#define NME_SIZE_OF_RSC   (8)

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* declare the call-backs for the call back structure */
/* for functional description, see lib_security.h */
static void callback_send_sec_packet(void *myCtx, const CsrUint8 *pBuffer, const CsrUint32 bufLen, const CsrUint8 *pLocalMac, const CsrUint8 *pPeerMac);
static void callback_install_pairwise_key(void *myCtx, CsrWifiSecurityKeyType keyType, const CsrUint8 *pairwiseKey, const CsrUint32 pairwiseLength, const CsrUint8 *rsc, const CsrUint8 keyIndex);
static void callback_install_group_key( void *myCtx, CsrWifiSecurityKeyType keyType, const CsrUint8 *groupKey, const CsrUint32 groupLength, const CsrUint8* rsc, const CsrUint8 keyIndex);
static void callback_install_pac(void *myCtx, const CsrUint8 *pac, const CsrUint32 pacLength);
static void callback_install_session(void *myCtx, const CsrUint8 *session, const CsrUint32 sessionLength);
static void callback_wps_done(void* myCtx, const CsrUint8 result, const CsrUint32 authenticationType, const CsrUint32 encryptionType, const CsrUint8 *networkKey, const CsrUint16 networkKeyLength, const CsrUint8 networkKeyIndex, const CsrUint8 *ssid);
static void callback_start_timer(void *myCtx, const CsrUint32 duration, const CsrWifiSecurityTimeout timeoutId);
static void callback_stopTimer(void *myCtx, CsrWifiSecurityTimeout timeoutId);
static void callback_abort_procedure(void *myCtx);
static void callback_packet_processing_done(void *myCtx, CsrUint32 appCookie);
static void callback_pmkid_store(void* myCtx, const CsrUint8 bssid[CSR_WIFI_SECURITY_MACADDRESS_LENGTH], const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH], const CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH]);
static CsrBool callback_pmk_get(void* myCtx, const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH], CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH]);

/* Callbacks */
static const CsrWifiSecuritycallbacks secCallbacks = {
    callback_send_sec_packet,
    callback_install_pairwise_key,
    callback_install_group_key,
    callback_install_pac,
    callback_install_session,
    callback_wps_done,
    callback_start_timer,
    callback_stopTimer,
    callback_abort_procedure,
    callback_packet_processing_done,
    callback_pmkid_store,
    callback_pmk_get
};


/* PRIVATE FUNCTION DEFINITIONS *********************************************/
/**
 * @brief
 *  Converts an ASCII string to a byte array the string
 *  must end in 0 and the buffer pointed to by returnValue
 *  must be large enough to hold the converted string.
 *
 * @par Description
 *  See brief
 *
 * @param
 *  const char * : string to convert
 *  CsrUint8 * : point to populate with the result
 *
 * @return
 *  None
 */
static void NmeSecAsciiStrToUint8(const char *string, CsrUint8 *returnValue)
{
    CsrUint16 currentIndex = 0;

    *returnValue = 0;
    while (0 != string[currentIndex] && 0 != string[currentIndex+1])
    {
        *returnValue = 0;
        (void)CsrHexStrToUint8(&string[currentIndex], returnValue);
        currentIndex = currentIndex+2;
        returnValue++;
    }
}

/*
 * Description:
 * See description in csr_security.h
 */
static void callback_stopTimer(
        void *myCtx,
        CsrWifiSecurityTimeout timeoutId)
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;

    sme_trace_entry((TR_NME_SMGR_FSM, "callback_stopTimer(sec timer id: %d)",  timeoutId));
    if (0 != securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId].uniqueid)
    {
        sme_trace_debug((TR_NME_SMGR_FSM, "remove running timer id %d", securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId].uniqueid));
        fsm_remove_timer(securityCtrlBlk->securityFsmContext, securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId]);
        securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId].uniqueid = 0;
    }
}

/*
 * Description:
 * See description in csr_security.h
 */
static void callback_start_timer(
        void *myCtx,
        const CsrUint32 duration,
        const CsrWifiSecurityTimeout timeoutId)
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;

    sme_trace_entry((TR_NME_SMGR_FSM, "callback_start_timer(sec timer id: %d)",  (CsrUint8)timeoutId));
    if (0 != securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId].uniqueid)
    {
        sme_trace_debug((TR_NME_SMGR_FSM, "remove running timer id %d", securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId].uniqueid));
        fsm_remove_timer(securityCtrlBlk->securityFsmContext, securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId]);
        securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId].uniqueid = 0;
    }

    send_nme_security_eap_timer(
            securityCtrlBlk->securityFsmContext,
            securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId],
            duration, 100,
            (CsrUint8)timeoutId);

    sme_trace_entry((TR_NME_SMGR_FSM, "started timer id %d", securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId].uniqueid));
    verify(TR_NME_SMGR_FSM, (0 != securityCtrlBlk->secLibTimerId[(CsrUint8)timeoutId].uniqueid));
}


/*
 * Description:
 * See description in csr_security.h
 */
static void callback_send_sec_packet(
        void *myCtx,
        const CsrUint8 *pBuffer,
        const CsrUint32 bufLen,
        const CsrUint8 *pLocalMac,
        const CsrUint8 *pPeerMac)
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;

    sme_trace_entry((TR_NME_SMGR_FSM, "callback_send_sec_packet" ));

    if ((CsrMemCmp(pBuffer, eapolllcheader, sizeof(eapolllcheader)) == 0) ||
        (CsrMemCmp(pBuffer, wapillcheader, sizeof(wapillcheader)) == 0))
    {
        CsrUint8 *packet = CsrPmalloc(bufLen+12);
        CsrUint8 *pCurrent = packet;
        FsmContext* securityFsmContext = securityCtrlBlk->securityFsmContext;

        CsrMemCpy(pCurrent, pPeerMac, 6);
        pCurrent += 6;
        CsrMemCpy(pCurrent, pLocalMac, 6);
        pCurrent += 6;
        CsrMemCpy(pCurrent, pBuffer, bufLen);

        sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_INFO, "complete packet", packet, bufLen+12));

        nme_routing_store_cfm_prim_internal(
                securityFsmContext,
                UNIFI_NME_SYS_EAPOL_CFM_ID,
                securityCtrlBlk->nmeSecurityMgrFsmInstance);

        call_unifi_nme_sys_eapol_req(
                securityFsmContext,
                NME_APP_HANDLE(securityFsmContext),
                securityCtrlBlk->subscriptionHandle,
                (CsrUint16)bufLen+12,
                packet,
                NULL);

        CsrPfree(packet);
    }
}

/*
 * Description:
 * See description in csr_security.h
 */
static void callback_install_pairwise_key(
        void *myCtx,
        CsrWifiSecurityKeyType keyType,
        const CsrUint8 *pairwiseKey,
        const CsrUint32 pairwiseLength,
        const CsrUint8 *rsc,
        const CsrUint8 keyIndex)
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;
    unifi_Key key;
    sme_trace_info((TR_NME_SMGR_FSM, "callback_install_pairwise_key" ));

    key.address = securityCtrlBlk->peerMacAddress;
    key.authenticator = FALSE;

    CsrMemCpy(key.key, pairwiseKey, pairwiseLength);
    key.keyLength = (CsrUint8)pairwiseLength;

    if(rsc != NULL) {

        CsrUint8 i;

        /* CsrMemCpy((CsrUint8*)key.keyRsc, rsc, 16); */
        if (keyType == CSR_WIFI_SECURITY_KEYTYPE_IEEE80211I)
        {
            for (i = 0; i < 8; i += 2)
            {
                key.keyRsc[i/2] = rsc[i+1] << 8 | rsc[i];
            }
        }
        else
        if (keyType == CSR_WIFI_SECURITY_KEYTYPE_WAPI)
        {
            for (i = 0; i < 16; i += 2)
            {
                key.keyRsc[i/2] = rsc[i+1] << 8 | rsc[i];
            }
        }

    } else {
        CsrMemSet((CsrUint8*)key.keyRsc, 0, 16);
    }

    key.keyType = unifi_PairwiseKey;
    key.wepTxKey = 0;
    key.keyIndex = keyIndex;

    /* *********************  TEST CODE ****************** */

    /*corrupt bit 128 -> 191*/
    /* key.key[17] = 0;
    key.key[18] = 0x45; */
    /* key.key[29] = 0;
    key.key[30] = 0x45; */

    send_nme_security_libsetkey_ind(
            securityCtrlBlk->securityFsmContext,
            securityCtrlBlk->nmeSecurityMgrFsmInstance,
            key);
}

/*
 * Description:
 * See description in csr_security.h
 */
static void callback_install_group_key(
        void *myCtx,
        CsrWifiSecurityKeyType keyType,
        const CsrUint8 *groupKey,
        const CsrUint32 groupLength,
        const CsrUint8* rsc,
        const CsrUint8 keyIndex)
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;
    unifi_Key key;
    sme_trace_info((TR_NME_SMGR_FSM, "callback_install_group_key" ));

    key.address = securityCtrlBlk->peerMacAddress;
    key.authenticator = FALSE;

    CsrMemCpy(key.key, groupKey, groupLength);
    key.keyLength = (CsrUint8)groupLength;

    /* Although the keyRsc is an array of Uint16[8] only the first 4 are actually used. */
    CsrMemSet((CsrUint8*)key.keyRsc, 0, sizeof(key.keyRsc));

    if(rsc != NULL) {

        CsrUint8 i;

        /* CsrMemCpy((CsrUint8*)key.keyRsc, rsc, NME_SIZE_OF_RSC); */
        if (keyType == CSR_WIFI_SECURITY_KEYTYPE_IEEE80211I)
        {
            for (i = 0; i < 8; i += 2)
            {
                key.keyRsc[i/2] = rsc[i+1] << 8 | rsc[i];
            }
        }
        else
        if (keyType == CSR_WIFI_SECURITY_KEYTYPE_WAPI)
        {
            for (i = 0; i < 16; i += 2)
            {
                key.keyRsc[i/2] = rsc[i+1] << 8 | rsc[i];
            }
        }
    }

    key.keyType = unifi_GroupKey;
    key.wepTxKey = 0;
    key.keyIndex = keyIndex;

    /* *********************  TEST CODE ****************** */

    /* corrupt bit 128 -> 191*/
    /*  key.key[17] = 0;
    key.key[18] = 0x45; */

    send_nme_security_libsetkey_ind(
            securityCtrlBlk->securityFsmContext,
            securityCtrlBlk->nmeSecurityMgrFsmInstance,
            key);
}

/*
 * Description:
 * See description in csr_security.h
 */
static void callback_install_pac(
        void *myCtx, const CsrUint8 *pac, const CsrUint32 pacLength)
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;

    require(TR_NME_SMGR_FSM, ((0 == pacLength && NULL == pac) || (0 < pacLength && NULL != pac)));

    sme_trace_entry((TR_NME_SMGR_FSM, "callback_install_pac" ));

    /* Store the PAC in the security control block rather
     * than pass it around as part of the FSM event.
     */
    if (0 < securityCtrlBlk->pacLength)
    {
        CsrPfree(securityCtrlBlk->pac);
        securityCtrlBlk->pacLength = 0;
    }
    if (0 < pacLength)
    {
        securityCtrlBlk->pac = CsrPmalloc(pacLength);
        CsrMemCpy(securityCtrlBlk->pac, pac, pacLength);
        securityCtrlBlk->pacLength = pacLength;
    }

    /*
     * save it in the setupdata
     */
    if (0 < securityCtrlBlk->setupData.fast_pac_length)
    {
        CsrPfree(securityCtrlBlk->setupData.fast_pac);
        securityCtrlBlk->setupData.fast_pac_length = 0;
    }
    if (0 < pacLength)
    {
        securityCtrlBlk->setupData.fast_pac = CsrPmalloc(pacLength);
        CsrMemCpy(securityCtrlBlk->setupData.fast_pac, pac, pacLength);
        securityCtrlBlk->setupData.fast_pac_length = pacLength;
    }

    /* Send the event to inform the security FSM about the PAC */
    send_nme_security_libsetpac_ind(
            securityCtrlBlk->securityFsmContext,
            securityCtrlBlk->nmeSecurityMgrFsmInstance);
}

/**
 * @brief
 *  Install a fast re-authentication session
 *
 * @par Description
 *  Called to save fast re-authentication session
 *
 * @param
 *  void*      : Global client context
 *  CsrUint8*  : session
 *  CsrUint32  : session length
 *
 * @return
 *  void
 */
static void callback_install_session(void *myCtx, const CsrUint8 *session, const CsrUint32 sessionLength)
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;

    require(TR_NME_SMGR_FSM, ((0 == sessionLength && NULL == session) || (0 < sessionLength && NULL != session)));

/*    sme_trace_error((TR_NME_SMGR_FSM, "callback_install_session()"));
    sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_ERROR, "session", session, sessionLength));
*/
    /* Store the session in the security control block rather
     * than pass it around as part of the FSM event.
     */
    if (0 < securityCtrlBlk->sessionLength)
    {
        CsrPfree(securityCtrlBlk->session);
        securityCtrlBlk->sessionLength = 0;
    }
    if (0 < sessionLength)
    {
        securityCtrlBlk->session = CsrPmalloc(sessionLength);
        CsrMemCpy(securityCtrlBlk->session, session, sessionLength);
        securityCtrlBlk->sessionLength = sessionLength;
    }
    if (0 < securityCtrlBlk->setupData.session_length)
    {
        CsrPfree(securityCtrlBlk->setupData.session);
        securityCtrlBlk->setupData.session_length = 0;
    }
    if (0 < sessionLength)
    {
        securityCtrlBlk->setupData.session = CsrPmalloc(sessionLength);
        CsrMemCpy(securityCtrlBlk->setupData.session, session, sessionLength);
        securityCtrlBlk->setupData.session_length = sessionLength;
    }

    /* Send the event to inform the security FSM about the session */
    send_nme_security_libsetsession_ind(
            securityCtrlBlk->securityFsmContext,
            securityCtrlBlk->nmeSecurityMgrFsmInstance);
}

/**
 * @brief
 *  Need to convert the encryption mode supplied by the
 *  security library to values suitable for use in the
 *  credentials of a profile.
 *
 * @par Description
 *  See brief
 *
 * @param CsrUint32  : security lib encryption mode
 *
 * @return
 *  CsrUint16 encryption mode mask
 */
static CsrUint16 nme_security_map_wps_encryption_mode(CsrUint32 wpsEncryptionMode)
{
    CsrUint16 encrpModeMask = 0;

    switch(wpsEncryptionMode)
    {
    case CSR_WIFI_SECURITY_WPS_ENC_TYPE_WEP:
        encrpModeMask = unifi_EncryptionCipherPairwiseWep104 | unifi_EncryptionCipherGroupWep104;
        break;
    case CSR_WIFI_SECURITY_WPS_ENC_TYPE_TKIP:
        encrpModeMask = unifi_EncryptionCipherPairwiseTkip | unifi_EncryptionCipherGroupTkip;
        break;
    case CSR_WIFI_SECURITY_WPS_ENC_TYPE_AES:
        encrpModeMask = unifi_EncryptionCipherPairwiseCcmp | unifi_EncryptionCipherGroupCcmp;
        break;
    default:
        sme_trace_warn((TR_NME_SMGR_FSM, "map_encryptionmode, unrecognised Encryption Mode %d", wpsEncryptionMode));
    }

    return encrpModeMask;
}
/*
 * Description:
 * See description in csr_security.h
 */
static void callback_wps_done(
        void* myCtx,
        const CsrUint8 result,
        const CsrUint32 authenticationType,
        const CsrUint32 encryptionType,
        const CsrUint8 *networkKey,
        const CsrUint16 networkKeyLength,
        const CsrUint8 networkKeyIndex,
        const CsrUint8 *ssid)
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;
    CsrUint8 ssidLen = 0;
    nme_SecurityResultCode resultCode = nme_SecurityResultCodeSuccess;

    sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done, key len %d", networkKeyLength));
    sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_ERROR, "WPS networkKey", networkKey, networkKeyLength ));
    sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_ERROR, "WPS ssid", ssid, 12 ));

    CsrMemSet(&securityCtrlBlk->credentials, 0, sizeof(unifi_Credentials));
    while (ssid[ssidLen] != 0){ssidLen++;}
    securityCtrlBlk->WpsSsid.length = ssidLen;
    CsrMemCpy(securityCtrlBlk->WpsSsid.ssid, ssid, ssidLen);

    /* WPS has failed if its a 0 key */
    if (networkKeyLength == 0)
    {
        /* Must be an open system */
        securityCtrlBlk->credentials.credentialType = unifi_CredentialTypeOpenSystem;
    }
    else
    {
        char *passphrase = CsrPmalloc(networkKeyLength+1);
        CsrMemCpy(passphrase, networkKey, networkKeyLength);
        passphrase[networkKeyLength]= 0;

        sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done, passphrase %s", passphrase ));

        switch (authenticationType)
        {
        case CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA :
        {
            sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done, CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA"));
            resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
            break;
        }
        case CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA2:
        {
            sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA2"));
            resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
            break;
        }
        case CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPAPSK:
        {
            sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA"));
            securityCtrlBlk->credentials.credentialType = unifi_CredentialTypeWpaPsk;
            securityCtrlBlk->credentials.credential.wpaPassphrase.encryptionMode = nme_security_map_wps_encryption_mode(encryptionType);

            if (64 == networkKeyLength)
            {
                NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wpaPsk.psk);
            }
            else
            {
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
                /*hash the passphrase directly into the correct location */
                CsrCryptoWpaPskHash(passphrase,
                                    (CsrUint8*)ssid,
                                    ssidLen,
                                    securityCtrlBlk->credentials.credential.wpaPsk.psk);
#endif
            }
            break;
        }
        case CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA2PSK:
        {
            sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA2" ));
            securityCtrlBlk->credentials.credentialType = unifi_CredentialTypeWpa2Psk;
            securityCtrlBlk->credentials.credential.wpa2Passphrase.encryptionMode = nme_security_map_wps_encryption_mode(encryptionType);

            if (64 == networkKeyLength)
            {
                NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wpa2Psk.psk);
            }
            else
            {
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
                /*hash the passphrase directly into the correct location */
                CsrCryptoWpaPskHash(passphrase,
                                    (CsrUint8*)ssid,
                                    ssidLen,
                                    securityCtrlBlk->credentials.credential.wpa2Psk.psk);
#endif
            }
            break;
        }
        default:
        {
            sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done CSR_WIFI_SECURITY_WPS_AUTH_TYPE_OPEN" ));
            /* Default is OPEN but this could still mean WEP */
            if (CSR_WIFI_SECURITY_WPS_ENC_TYPE_WEP == encryptionType)
            {
                sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done CSR_WIFI_SECURITY_WPS_ENC_TYPE_WEP keyIndex %d", networkKeyIndex ));
                if (networkKeyIndex < 1 || networkKeyIndex > 4)
                {
                    sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done, Unsupported key index"));
                    resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
                    break;
                }

                if (5 == networkKeyLength)
                {
                    sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done WEP 64 Ascii" ));
                    securityCtrlBlk->credentials.credentialType = unifi_CredentialTypeWep64;
                    /* WEP auth type set to open as during a connection attempt
                     * we'll try shared if we failed with open anyway.
                     */
                    securityCtrlBlk->credentials.credential.wep64Key.wepAuthType = unifi_80211AuthOpen;
                    securityCtrlBlk->credentials.credential.wep64Key.selectedWepKey = networkKeyIndex;

                    switch(networkKeyIndex)
                    {
                    case 1:
                    {
                        CsrMemCpy(securityCtrlBlk->credentials.credential.wep64Key.key1, passphrase, 5);
                        break;
                    }
                    case 2:
                    {
                        CsrMemCpy(securityCtrlBlk->credentials.credential.wep64Key.key2, passphrase, 5);
                        break;
                    }
                    case 3:
                    {
                        CsrMemCpy(securityCtrlBlk->credentials.credential.wep64Key.key3, passphrase, 5);
                        break;
                    }
                    default:
                    {
                        CsrMemCpy(securityCtrlBlk->credentials.credential.wep64Key.key4, passphrase, 5);
                        break;
                    }
                    }
                }
                else if (13 == networkKeyLength)
                {
                    sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done assuming WEP 128 Ascii" ));
                    securityCtrlBlk->credentials.credentialType = unifi_CredentialTypeWep128;
                    /* WEP auth type set to open as during a connection attempt
                     * we'll try shared if we failed with open anyway.
                     */
                    securityCtrlBlk->credentials.credential.wep128Key.wepAuthType = unifi_80211AuthOpen;
                    securityCtrlBlk->credentials.credential.wep128Key.selectedWepKey = networkKeyIndex;

                    switch(networkKeyIndex)
                    {
                    case 1:
                    {
                        CsrMemCpy(securityCtrlBlk->credentials.credential.wep128Key.key1, passphrase, 13);
                        break;
                    }
                    case 2:
                    {
                        CsrMemCpy(securityCtrlBlk->credentials.credential.wep128Key.key2, passphrase, 13);
                        break;
                    }
                    case 3:
                    {
                        CsrMemCpy(securityCtrlBlk->credentials.credential.wep128Key.key3, passphrase, 13);
                        break;
                    }
                    default:
                    {
                        CsrMemCpy(securityCtrlBlk->credentials.credential.wep128Key.key4, passphrase, 13);
                        break;
                    }
                    }
                }
                else if (10 == networkKeyLength)
                {
                    sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done WEP 64 hex" ));
                    securityCtrlBlk->credentials.credentialType = unifi_CredentialTypeWep64;
                    /* WEP auth type set to open as during a connection attempt
                     * we'll try shared if we failed with open anyway.
                     */
                    securityCtrlBlk->credentials.credential.wep64Key.wepAuthType = unifi_80211AuthOpen;
                    securityCtrlBlk->credentials.credential.wep64Key.selectedWepKey = networkKeyIndex;
                    switch(networkKeyIndex)
                    {
                    case 1:
                    {
                        NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wep64Key.key1);
                        break;
                    }
                    case 2:
                    {
                        NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wep64Key.key2);
                        break;
                    }
                    case 3:
                    {
                        NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wep64Key.key3);
                        break;
                    }
                    default:
                    {
                        NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wep64Key.key4);
                        break;
                    }
                    }
                }
                else if (26 == networkKeyLength)
                {
                    sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done assuming WEP 128 hex" ));
                    securityCtrlBlk->credentials.credentialType = unifi_CredentialTypeWep128;
                    /* WEP auth type set to open as during a connection attempt
                     * we'll try shared if we failed with open anyway.
                     */
                    securityCtrlBlk->credentials.credential.wep128Key.wepAuthType = unifi_80211AuthOpen;
                    securityCtrlBlk->credentials.credential.wep128Key.selectedWepKey = networkKeyIndex;
                    switch(networkKeyIndex)
                    {
                    case 1:
                    {
                        NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wep128Key.key1);
                        break;
                    }
                    case 2:
                    {
                        NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wep128Key.key2);
                        break;
                    }
                    case 3:
                    {
                        NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wep128Key.key3);
                        break;
                    }
                    default:
                    {
                        NmeSecAsciiStrToUint8(passphrase, securityCtrlBlk->credentials.credential.wep128Key.key4);
                        break;
                    }
                    }
                }
                else
                {
                    sme_trace_info((TR_NME_SMGR_FSM, "callback_wps_done, Unsupported key length"));
                    resultCode = nme_SecurityResultCodeUnsupportedCredentialType;
                    break;
                }
            }
            break;
        }
        }
        CsrPfree((CsrUint8*) passphrase);
    }

    send_nme_security_libwpsdone_ind(
            securityCtrlBlk->securityFsmContext,
            securityCtrlBlk->nmeSecurityMgrFsmInstance,
            resultCode);

    /* the security FSM will clean up. */
}

/*
 * Description:
 * See description in csr_security.h
 */
static void callback_abort_procedure(
        void *myCtx)
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;
    sme_trace_entry((TR_NME_SMGR_FSM, "callback_abort_procedure" ));

    send_nme_security_libabort_ind(
            securityCtrlBlk->securityFsmContext,
            securityCtrlBlk->nmeSecurityMgrFsmInstance);

    /* the security FSM will clean up. */
}

/*
 * Description:
 * See description in csr_security.h
 */
static void callback_packet_processing_done(
        void *myCtx,
        CsrUint32 appCookie)
{
    sme_trace_entry((TR_NME_SMGR_FSM, "callback_packet_processing_done"));

    /* free the message sent */
    CsrPfree((CsrUint8*) appCookie);
}

/*
 * Description:
 * See description in csr_security.h
 */
static void callback_pmkid_store(void* myCtx, const CsrUint8 bssid[CSR_WIFI_SECURITY_MACADDRESS_LENGTH], const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH], const CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH])
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;
    NmeConfigData* cfg = nme_core_get_nme_config(securityCtrlBlk->securityFsmContext);
    unifi_Pmkid setPmkid;
    sme_trace_entry((TR_NME_SMGR_FSM, "callback_pmkid_store"));

    /* don't save the PMK for WPA only WPA2 */
    if (nme_sec_get_auth_mode_mask(securityCtrlBlk->securityFsmContext) & (unifi_8021xAuthWPA2|unifi_8021xAuthWPA2PSK))
    {
        CsrMemCpy(setPmkid.bssid.data, bssid, sizeof(setPmkid.bssid.data));
        CsrMemCpy(setPmkid.pmkid, pmkid, sizeof(setPmkid.pmkid));

        nme_pmk_cache_set(&cfg->pmkCache, &setPmkid.bssid, pmkid, pmk);

        /* Send to the SME as well (Use FSM_TERMINATE with the routing as we do not care about the cfm */
        nme_routing_store_cfm_prim_internal(securityCtrlBlk->securityFsmContext, UNIFI_NME_MGT_PMKID_CFM_ID, FSM_TERMINATE);
        call_unifi_nme_mgt_pmkid_req(securityCtrlBlk->securityFsmContext,
                                     NME_APP_HANDLE(securityCtrlBlk->securityFsmContext),
                                     unifi_ListActionAdd,
                                     1,
                                     &setPmkid);
    }
}

/*
 * Description:
 * See description in csr_security.h
 */
static CsrBool callback_pmk_get(void* myCtx, const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH], CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH])
{
    nmeSecurityCtrlBlk* securityCtrlBlk = (nmeSecurityCtrlBlk*)myCtx;
    NmeConfigData* cfg = nme_core_get_nme_config(securityCtrlBlk->securityFsmContext);
    csr_list_pmk*  node = nme_pmk_cache_get_pmk_from_pmkid(&cfg->pmkCache, pmkid);

    sme_trace_entry((TR_NME_SMGR_FSM, "callback_pmk_get"));

    if (node)
    {
        CsrMemCpy(pmk, node->pmk, CSR_WIFI_SECURITY_PMK_LENGTH);
        return TRUE;
    }
    return FALSE;

}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in nme_security_fsm/nme_security.h
 */
void nme_security_init(
        FsmContext* pContext,
        CsrUint16 nmeSecurityMgrFsmInstance,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        CsrBool isWps,
        CsrUint8 *pWpsPin,
        const unifi_Credentials *pCredentials,
        const unifi_SSID ssid,
        const unifi_MACAddress stationMacAddress)
{
    require(TR_NME_SMGR_FSM, NULL != pContext);
    require(TR_NME_SMGR_FSM, NULL != pSecurityCtrlBlk);
    require(TR_NME_SMGR_FSM, (isWps && (NULL != pWpsPin)) || (!isWps && (NULL != pCredentials)));

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_security_init(isWps: %d)", isWps));

    pSecurityCtrlBlk->isWps = isWps;
    pSecurityCtrlBlk->WpsSsid.length = 0;
    pSecurityCtrlBlk->securityFsmContext = pContext;
    pSecurityCtrlBlk->nmeSecurityMgrFsmInstance = nmeSecurityMgrFsmInstance;
    pSecurityCtrlBlk->securityContext = NULL;
    pSecurityCtrlBlk->stationMacAddress = stationMacAddress;

    /* Common Security setup values */
    pSecurityCtrlBlk->subscriptionHandle = 0;
    pSecurityCtrlBlk->allocOffset = 0;

    pSecurityCtrlBlk->pac = NULL;
    pSecurityCtrlBlk->pacLength = 0;
    pSecurityCtrlBlk->session = NULL;
    pSecurityCtrlBlk->sessionLength = 0;

    /* The security Lib advises an initial memset */
    CsrMemSet(&pSecurityCtrlBlk->setupData, 0x00, sizeof(CsrWifiSecuritySetup));

    if (isWps)
    {
#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
        CsrMemCpy(pSecurityCtrlBlk->setupData.localMacAddress, &pSecurityCtrlBlk->stationMacAddress, 6);
        pSecurityCtrlBlk->setupData.pmkValid = FALSE;
        pSecurityCtrlBlk->setupData.protocolSnapValid = TRUE;
        pSecurityCtrlBlk->setupData.responseTimeout = 3000;
        CsrMemCpy(pSecurityCtrlBlk->setupData.ssid, ssid.ssid, ssid.length);
        CsrMemCpy(pSecurityCtrlBlk->setupData.protocolSnap, eapolllcheader, CSR_WIFI_SECURITY_SNAP_LENGTH);
        pSecurityCtrlBlk->setupData.mode = CSR_WIFI_SECURITY_WPS_MODE_SUPPLICANT;
        pSecurityCtrlBlk->setupData.securityType = CSR_WIFI_SECURITY_TYPE_WPS;
        CsrMemCpy(pSecurityCtrlBlk->setupData.pin, pWpsPin, 8);
#endif
    }
    else
    {
        sme_trace_debug((TR_NME_SMGR_FSM, "credentialType %s", trace_unifi_CredentialType(pCredentials->credentialType)));

        /* Take a copy of the credentials */
        pSecurityCtrlBlk->credentials = *pCredentials;

        /* Specific Security setup values */
        if (unifi_CredentialTypeOpenSystem != pSecurityCtrlBlk->credentials.credentialType &&
            unifi_CredentialTypeWep64 != pSecurityCtrlBlk->credentials.credentialType &&
            unifi_CredentialTypeWep128 != pSecurityCtrlBlk->credentials.credentialType)
        {
            CsrMemCpy(pSecurityCtrlBlk->setupData.localMacAddress, &pSecurityCtrlBlk->stationMacAddress, 6);

            /* Common defaults that might be overridden later by credential specific values */
            pSecurityCtrlBlk->setupData.responseTimeout = CSR_SEC_RETRANSMISSION_TIMEOUT_WPA;
            pSecurityCtrlBlk->setupData.retransmissionAttempts = CSR_SEC_RETRANSMISSION_COUNT;
            CsrMemCpy(pSecurityCtrlBlk->setupData.protocolSnap, eapolllcheader, CSR_WIFI_SECURITY_SNAP_LENGTH);
            pSecurityCtrlBlk->setupData.mode = CSR_WIFI_SECURITY_WPAPSK_MODE_SUPPLICANT;
            pSecurityCtrlBlk->setupData.protocolSnapValid = TRUE;
            pSecurityCtrlBlk->setupData.pmkValid = FALSE;

            switch (pSecurityCtrlBlk->credentials.credentialType)
            {
            case unifi_CredentialTypeWpaPassphrase:
            case unifi_CredentialTypeWpa2Passphrase:
            case unifi_CredentialTypeWpaPsk:
            case unifi_CredentialTypeWpa2Psk:
            {
                pSecurityCtrlBlk->setupData.securityType = CSR_WIFI_SECURITY_TYPE_WPAPSK_SUPPLICANT;
                CsrMemCpy(pSecurityCtrlBlk->setupData.pmk, pSecurityCtrlBlk->pmk, CSR_WIFI_SECURITY_PMK_LENGTH);
                pSecurityCtrlBlk->setupData.pmkValid = TRUE;
                break;
            }
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
            case unifi_CredentialTypeWapiPassphrase:
            case unifi_CredentialTypeWapiPsk:
            {
                pSecurityCtrlBlk->setupData.securityType = CSR_WIFI_SECURITY_TYPE_WAPIPSK_ASUE;
                pSecurityCtrlBlk->setupData.mode = CSR_WIFI_SECURITY_WAPIPSK_MODE_ASUE;

                pSecurityCtrlBlk->setupData.responseTimeout = CSR_SEC_RETRANSMISSION_TIMEOUT_WAPI;
                pSecurityCtrlBlk->setupData.retransmissionAttempts = CSR_SEC_RETRANSMISSION_COUNT;
                CsrMemCpy(pSecurityCtrlBlk->setupData.protocolSnap, wapillcheader, CSR_WIFI_SECURITY_SNAP_LENGTH);
                pSecurityCtrlBlk->setupData.protocolSnapValid = TRUE;

                CsrMemCpy(pSecurityCtrlBlk->setupData.pmk, pSecurityCtrlBlk->pmk, CSR_WIFI_SECURITY_PMK_LENGTH);
                pSecurityCtrlBlk->setupData.pmkValid = TRUE;
                break;
            }

            case unifi_CredentialTypeWapi:
            {
                pSecurityCtrlBlk->setupData.securityType = CSR_WIFI_SECURITY_TYPE_WAPI_ASUE;
                pSecurityCtrlBlk->setupData.mode = CSR_WIFI_SECURITY_WAPI_MODE_ASUE;

                pSecurityCtrlBlk->setupData.responseTimeout = CSR_SEC_RETRANSMISSION_TIMEOUT_WAPI;
                pSecurityCtrlBlk->setupData.retransmissionAttempts = CSR_SEC_RETRANSMISSION_COUNT;
                CsrMemCpy(pSecurityCtrlBlk->setupData.protocolSnap, wapillcheader, CSR_WIFI_SECURITY_SNAP_LENGTH);
                pSecurityCtrlBlk->setupData.protocolSnapValid = TRUE;

                pSecurityCtrlBlk->setupData.clientCertificateLength = pSecurityCtrlBlk->credentials.credential.wapi.certificateLength;
                pSecurityCtrlBlk->setupData.clientCertificate = CsrPmalloc(pSecurityCtrlBlk->credentials.credential.wapi.certificateLength);
                CsrMemCpy(pSecurityCtrlBlk->setupData.clientCertificate,
                          pSecurityCtrlBlk->credentials.credential.wapi.certificate,
                          pSecurityCtrlBlk->credentials.credential.wapi.certificateLength);

                pSecurityCtrlBlk->setupData.clientPrivateKeyLength = pSecurityCtrlBlk->credentials.credential.wapi.privateKeyLength;
                pSecurityCtrlBlk->setupData.clientPrivateKey = CsrPmalloc(pSecurityCtrlBlk->credentials.credential.wapi.privateKeyLength);
                CsrMemCpy(pSecurityCtrlBlk->setupData.clientPrivateKey,
                          pSecurityCtrlBlk->credentials.credential.wapi.privateKey,
                          pSecurityCtrlBlk->credentials.credential.wapi.privateKeyLength);

                pSecurityCtrlBlk->setupData.authenticationServerCertificateLength = pSecurityCtrlBlk->credentials.credential.wapi.caCertificateLength;
                pSecurityCtrlBlk->setupData.authenticationServerCertificate = CsrPmalloc(pSecurityCtrlBlk->credentials.credential.wapi.caCertificateLength);
                CsrMemCpy(pSecurityCtrlBlk->setupData.authenticationServerCertificate,
                          pSecurityCtrlBlk->credentials.credential.wapi.caCertificate,
                          pSecurityCtrlBlk->credentials.credential.wapi.caCertificateLength);

                break;
            }
#endif

#ifdef CSR_WIFI_SECURITY_EAP_ENABLE

            case unifi_CredentialType8021xLeap:
            {
#if defined CSR_WIFI_SECURITY_LEAP_ENABLE
                pSecurityCtrlBlk->setupData.securityType = CSR_WIFI_SECURITY_TYPE_LEAP;
                pSecurityCtrlBlk->setupData.identity = pSecurityCtrlBlk->credentials.credential.leap.username;
                pSecurityCtrlBlk->setupData.username = pSecurityCtrlBlk->credentials.credential.leap.username;
                pSecurityCtrlBlk->setupData.password = pSecurityCtrlBlk->credentials.credential.leap.password;
#endif
                break;
            }
            case unifi_CredentialType8021xFast:
            {
#if defined CSR_WIFI_SECURITY_FAST_ENABLE
                pSecurityCtrlBlk->setupData.pmkValid = TRUE;
                pSecurityCtrlBlk->setupData.securityType = CSR_WIFI_SECURITY_TYPE_FAST;
                pSecurityCtrlBlk->setupData.identity = pSecurityCtrlBlk->credentials.credential.fast.eapCredentials.authServerUserIdentity;
                pSecurityCtrlBlk->setupData.username = pSecurityCtrlBlk->credentials.credential.fast.eapCredentials.username;
                pSecurityCtrlBlk->setupData.password = pSecurityCtrlBlk->credentials.credential.fast.eapCredentials.password;

                if (0 < pSecurityCtrlBlk->credentials.credential.fast.pacLength)
                {
                    pSecurityCtrlBlk->setupData.fast_pac = CsrPmalloc(pSecurityCtrlBlk->credentials.credential.fast.pacLength);
                    CsrMemCpy(pSecurityCtrlBlk->setupData.fast_pac, pSecurityCtrlBlk->credentials.credential.fast.pac,
                    pSecurityCtrlBlk->credentials.credential.fast.pacLength);
                    pSecurityCtrlBlk->setupData.fast_pac_length = pSecurityCtrlBlk->credentials.credential.fast.pacLength;
                }
#endif
                break;
            }
            case unifi_CredentialType8021xTtls:
            {
#if defined CSR_WIFI_SECURITY_TTLS_ENABLE
                pSecurityCtrlBlk->setupData.securityType = CSR_WIFI_SECURITY_TYPE_TTLS;
                pSecurityCtrlBlk->setupData.pmkValid = TRUE;
                pSecurityCtrlBlk->setupData.identity = pSecurityCtrlBlk->credentials.credential.ttls.authServerUserIdentity;
                pSecurityCtrlBlk->setupData.username = pSecurityCtrlBlk->credentials.credential.ttls.username;
                pSecurityCtrlBlk->setupData.password = pSecurityCtrlBlk->credentials.credential.ttls.password;

                if (0 < pSecurityCtrlBlk->credentials.credential.ttls.sessionLength)
                {
                    pSecurityCtrlBlk->setupData.session = CsrPmalloc(pSecurityCtrlBlk->credentials.credential.ttls.sessionLength);
                    CsrMemCpy(pSecurityCtrlBlk->setupData.session, pSecurityCtrlBlk->credentials.credential.ttls.session,
                    pSecurityCtrlBlk->credentials.credential.ttls.sessionLength);
                    pSecurityCtrlBlk->setupData.session_length = pSecurityCtrlBlk->credentials.credential.ttls.sessionLength;
                }
#endif
                break;
            }
            case unifi_CredentialType8021xTls:
            {
#if defined CSR_WIFI_SECURITY_TLS_ENABLE
                pSecurityCtrlBlk->setupData.securityType = CSR_WIFI_SECURITY_TYPE_TLS;
                pSecurityCtrlBlk->setupData.pmkValid = TRUE;
                pSecurityCtrlBlk->setupData.identity = pSecurityCtrlBlk->credentials.credential.tls.username;

                pSecurityCtrlBlk->setupData.clientCertificate = CsrPmalloc(pSecurityCtrlBlk->credentials.credential.tls.certificateLength);
                CsrMemCpy(pSecurityCtrlBlk->setupData.clientCertificate, pSecurityCtrlBlk->credentials.credential.tls.certificate,
                        pSecurityCtrlBlk->credentials.credential.tls.certificateLength);
                pSecurityCtrlBlk->setupData.clientCertificateLength = pSecurityCtrlBlk->credentials.credential.tls.certificateLength;

                pSecurityCtrlBlk->setupData.clientPrivateKey = CsrPmalloc(pSecurityCtrlBlk->credentials.credential.tls.privateKeyLength);
                CsrMemCpy(pSecurityCtrlBlk->setupData.clientPrivateKey, pSecurityCtrlBlk->credentials.credential.tls.privateKey,
                        pSecurityCtrlBlk->credentials.credential.tls.privateKeyLength);
                pSecurityCtrlBlk->setupData.clientPrivateKeyLength = pSecurityCtrlBlk->credentials.credential.tls.privateKeyLength;

                if (0 < pSecurityCtrlBlk->credentials.credential.tls.sessionLength)
                {
                    pSecurityCtrlBlk->setupData.session = CsrPmalloc(pSecurityCtrlBlk->credentials.credential.tls.sessionLength);
                    CsrMemCpy(pSecurityCtrlBlk->setupData.session, pSecurityCtrlBlk->credentials.credential.tls.session,
                    pSecurityCtrlBlk->credentials.credential.tls.sessionLength);
                    pSecurityCtrlBlk->setupData.session_length = pSecurityCtrlBlk->credentials.credential.tls.sessionLength;
                }
#endif
                break;
            }

            case unifi_CredentialType8021xPeapGtc:
            case unifi_CredentialType8021xPeapMschapv2:
            {
                break;
            }
#endif /* CSR_WIFI_SECURITY_EAP_ENABLE */
            default:
            {
                break;
            }
            }
        }
        else
        {
            sme_trace_info((TR_NME_SMGR_FSM, "Not a sec connection"));
        }
    }
}
/*
 * Description:
 * See description in nme_security_fsm/nme_security.h
 */
void nme_security_stop_all_timers(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk)
{
    CsrUint8 iTimerId = 0;
    require(TR_NME_SMGR_FSM, NULL != pContext);
    require(TR_NME_SMGR_FSM, NULL != pSecurityCtrlBlk);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_security_stop_all_timers"));

    for (iTimerId = 0; iTimerId < CSR_WIFI_SECURITY_TIMEOUT_MAX_TIMEOUTS; iTimerId++)
    {
        if (0 != pSecurityCtrlBlk->secLibTimerId[iTimerId].uniqueid)
        {
            fsm_remove_timer(pContext, pSecurityCtrlBlk->secLibTimerId[iTimerId]);
            pSecurityCtrlBlk->secLibTimerId[iTimerId].uniqueid = 0;
        }
    }
}

/*
 * Description:
 * See description in nme_security_fsm/nme_security.h
 */
void nme_security_deinit(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk)
{
    require(TR_NME_SMGR_FSM, NULL != pContext);
    require(TR_NME_SMGR_FSM, NULL != pSecurityCtrlBlk);

    nme_security_stop_all_timers(pContext, pSecurityCtrlBlk);

    pSecurityCtrlBlk->securityFsmContext = NULL;
    pSecurityCtrlBlk->credentials.credentialType = unifi_CredentialTypeOpenSystem;
    pSecurityCtrlBlk->stationMacAddress = NmeNullBssid;
    pSecurityCtrlBlk->peerMacAddress = NmeNullBssid;

    pSecurityCtrlBlk->subscriptionHandle = 0;
    pSecurityCtrlBlk->allocOffset = 0;
    pSecurityCtrlBlk->isWps = FALSE;
    pSecurityCtrlBlk->WpsSsid.length = 0;

    if (pSecurityCtrlBlk->setupData.clientCertificate != NULL)
    {
        CsrPfree(pSecurityCtrlBlk->setupData.clientCertificate);
        pSecurityCtrlBlk->setupData.clientCertificate = NULL;
    }

    if (pSecurityCtrlBlk->setupData.clientPrivateKey != NULL)
    {
        CsrPfree(pSecurityCtrlBlk->setupData.clientPrivateKey);
        pSecurityCtrlBlk->setupData.clientPrivateKey = NULL;
    }

    if (pSecurityCtrlBlk->setupData.authenticationServerCertificate != NULL)
    {
        CsrPfree(pSecurityCtrlBlk->setupData.authenticationServerCertificate);
        pSecurityCtrlBlk->setupData.authenticationServerCertificate = NULL;
    }

    if (pSecurityCtrlBlk->setupData.fast_pac != NULL)
    {
        CsrPfree(pSecurityCtrlBlk->setupData.fast_pac);
        pSecurityCtrlBlk->setupData.fast_pac = NULL;
    }

    if (pSecurityCtrlBlk->setupData.session != NULL)
    {
        CsrPfree(pSecurityCtrlBlk->setupData.session);
        pSecurityCtrlBlk->setupData.session = NULL;
    }

    if (NULL != pSecurityCtrlBlk->securityContext)
    {
        CsrWifiSecurityDeInit(pSecurityCtrlBlk->securityContext);
        pSecurityCtrlBlk->securityContext = NULL;
    }

}

/*
 * Description:
 * See description in nme_security_fsm/nme_security.h
 */
void nme_security_start(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        CsrUint16 ieLength,
        CsrUint8 *ie,
        const unifi_SSID peerSsid,
        const unifi_MACAddress peerMacAddress)
{
    /* Assume by default that the security library will be started */
    CsrBool securityLibRequired = TRUE;

    require(TR_NME_SMGR_FSM, NULL != pContext);
    require(TR_NME_SMGR_FSM, NULL != pSecurityCtrlBlk);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_security_start(wps: %d)", pSecurityCtrlBlk->isWps));

    /* update peer address */
    pSecurityCtrlBlk->peerMacAddress = peerMacAddress;
    CsrMemCpy(pSecurityCtrlBlk->setupData.peerMacAddress, &pSecurityCtrlBlk->peerMacAddress, 6);

    if (!pSecurityCtrlBlk->isWps)
    {
    switch (pSecurityCtrlBlk->credentials.credentialType)
    {
    case unifi_CredentialTypeOpenSystem:
    case unifi_CredentialTypeWep64:
    case unifi_CredentialTypeWep128:
    {
        sme_trace_info((TR_NME_SMGR_FSM, "Not starting sec lib"));
        securityLibRequired = FALSE;
        break;
    }
    case unifi_CredentialTypeWpaPassphrase:
    case unifi_CredentialTypeWpa2Passphrase:
    case unifi_CredentialTypeWpaPsk:
    case unifi_CredentialTypeWpa2Psk:
    case unifi_CredentialType8021xTtls:
    case unifi_CredentialType8021xTls:
    case unifi_CredentialType8021xFast:
    {
        CsrUint8* a2ie;

        /* Process the Info Elements for RSN or WPA info */
        if(ie_success == ie_find_vendor_specific(ieWpaOui, NULL, 0, ie, ieLength, &a2ie))
        {
            pSecurityCtrlBlk->setupData.secIe = a2ie;
            pSecurityCtrlBlk->setupData.secIeLen = ie_len(a2ie) + 2;

            sme_trace_debug((TR_NME_SMGR_FSM, "WPA association"));
            sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "WPA IE", a2ie, ie_len(a2ie) + 2));
        }
        else if (ie_success == ie_find(IE_ID_RSN, ie, ieLength, &a2ie))
        {
            pSecurityCtrlBlk->setupData.secIe = a2ie;
            pSecurityCtrlBlk->setupData.secIeLen = ie_len(a2ie) + 2;
            sme_trace_debug((TR_NME_SMGR_FSM, "RSN association"));
            sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "RSN IE", a2ie, ie_len(a2ie) + 2));
        }
        else
        {
            sme_trace_info((TR_NME_SMGR_FSM, "credential "));
        }
        break;
    }
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    case unifi_CredentialTypeWapiPassphrase:
    case unifi_CredentialTypeWapiPsk:
    case unifi_CredentialTypeWapi:
    {
        CsrUint8* a2ie;
        if (ie_success == ie_find(IE_ID_WAPI, ie, ieLength, &a2ie))
        {
            pSecurityCtrlBlk->setupData.secIe = a2ie;
            pSecurityCtrlBlk->setupData.secIeLen = ie_len(a2ie) + 2;

            sme_trace_debug((TR_NME_SMGR_FSM, "WAPI association"));
            sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "WAPI IE", a2ie, ie_len(a2ie) + 2));
        }
        else
        {
            sme_trace_error((TR_NME_SMGR_FSM, "Wapi IE not found"));
        }
        break;
    }
#endif

    /* @TODO need to get the use of compile time switch right with regard
     * the calls to the security lib, because if we haven't setup the security
     * data should we be calling the lib?
     */
#ifdef CSR_WIFI_SECURITY_EAP_ENABLE
    case unifi_CredentialType8021xLeap:
    case unifi_CredentialType8021xPeapGtc:
    case unifi_CredentialType8021xPeapMschapv2:
#endif /* CSR_WIFI_SECURITY_EAP_ENABLE */
    default:
    {
        break;
    }
    }
    }
    /* Check if we need to generate a new PMK in the case of WPA/WPA2
     * passphrase where the SSID has changed.
     */
    if ((unifi_CredentialTypeWpaPassphrase == pSecurityCtrlBlk->credentials.credentialType ||
         unifi_CredentialTypeWpa2Passphrase == pSecurityCtrlBlk->credentials.credentialType) &&
        (pSecurityCtrlBlk->peerSsid.length != peerSsid.length ||
         0 != CsrMemCmp(pSecurityCtrlBlk->peerSsid.ssid, peerSsid.ssid, peerSsid.length)))
    {
        sme_trace_debug((TR_NME_SMGR_FSM, "WPA/WPA2 passphrase hashing due to different SSID"));
#ifdef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
        if (unifi_CredentialTypeWpaPassphrase == pSecurityCtrlBlk->credentials.credentialType)
        {
            CsrCryptoWpaPskHash(pSecurityCtrlBlk->credentials.credential.wpaPassphrase.passphrase,
                                (CsrUint8*)peerSsid.ssid,
                                peerSsid.length,
                                pSecurityCtrlBlk->pmk);
        }
        else if (unifi_CredentialTypeWpa2Passphrase == pSecurityCtrlBlk->credentials.credentialType)
        {
            CsrCryptoWpaPskHash(pSecurityCtrlBlk->credentials.credential.wpa2Passphrase.passphrase,
                                (CsrUint8*)peerSsid.ssid,
                                peerSsid.length,
                                pSecurityCtrlBlk->pmk);
        }
        CsrMemCpy(pSecurityCtrlBlk->setupData.pmk, pSecurityCtrlBlk->pmk, CSR_WIFI_SECURITY_PMK_LENGTH);
        pSecurityCtrlBlk->setupData.pmkValid = TRUE;
#endif
        pSecurityCtrlBlk->peerSsid = peerSsid;
    }

    if (securityLibRequired)
    {
        pSecurityCtrlBlk->securityContext = CsrWifiSecurityInit(pSecurityCtrlBlk, getNmeSecurityContext(pContext), &pSecurityCtrlBlk->setupData, &secCallbacks);

        /* The security library could have failed to initialise if the information
         * passed in is incorrect.
         */
        if (NULL != pSecurityCtrlBlk->securityContext)
        {
            CsrWifiSecurityStart(pSecurityCtrlBlk->securityContext);
        }
        else
        {
            send_nme_security_libabort_ind(
                    pSecurityCtrlBlk->securityFsmContext,
                    pSecurityCtrlBlk->nmeSecurityMgrFsmInstance);
        }
    }

    /* the IE has been copied and is no longer needed */
    CsrPfree(ie);
}

/*
 * Description:
 * See description in nme_security_fsm/nme_security.h
 */
void nme_security_restart(
        FsmContext* pContext,
        nmeSecurityCtrlBlk* pSecurityCtrlBlk,
        CsrUint16 ieLength,
        CsrUint8* pIe,
        const unifi_SSID peerSsid,
        const unifi_MACAddress peerMacAddress)
{
    require(TR_NME_SMGR_FSM, NULL != pContext);
    require(TR_NME_SMGR_FSM, NULL != pSecurityCtrlBlk);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_security_restart : %s", trace_unifi_CredentialType(pSecurityCtrlBlk->credentials.credentialType)));

    nme_security_stop_all_timers(pContext, pSecurityCtrlBlk);
    if (NULL != pSecurityCtrlBlk->securityContext)
    {
        CsrWifiSecurityDeInit(pSecurityCtrlBlk->securityContext);
        pSecurityCtrlBlk->securityContext = NULL;
    }

    /* Can now restart the security lib (if required)*/
    nme_security_start(pContext, pSecurityCtrlBlk, ieLength, pIe, peerSsid, peerMacAddress);
}

/*
 * Description:
 * See description in nme_security_fsm/nme_security.h
 */
void nme_security_stop(
        FsmContext* pContext,
        nmeSecurityCtrlBlk* pSecurityCtrlBlk)
{
    require(TR_NME_SMGR_FSM, NULL != pContext);
    require(TR_NME_SMGR_FSM, NULL != pSecurityCtrlBlk);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_security_stop : %s", trace_unifi_CredentialType(pSecurityCtrlBlk->credentials.credentialType)));

    nme_security_stop_all_timers(pContext, pSecurityCtrlBlk);
    if (NULL != pSecurityCtrlBlk->securityContext)
    {
        CsrWifiSecurityDeInit(pSecurityCtrlBlk->securityContext);
        pSecurityCtrlBlk->securityContext = NULL;
    }
}

/*
 * Description:
 * See description in nme_security_fsm/nme_security.h
 */
void nme_security_process_ma_unitdata_packet(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        CsrUint8* packetBuffer,
        const CsrUint16 packetBufferLength)
{
    CsrUint8 *localPtr = (CsrUint8*)packetBuffer+12; /* skip the mac address's */

    require(TR_NME_SMGR_FSM, NULL != pContext);
    require(TR_NME_SMGR_FSM, NULL != pSecurityCtrlBlk);
    require(TR_NME_SMGR_FSM, NULL != packetBuffer);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_security_process_ma_unitdata_packet" ));

    if (NULL != pSecurityCtrlBlk->securityContext)
    {
        if ( (CsrMemCmp(localPtr, eapolllcheader, sizeof(eapolllcheader)) == 0) ||
             (CsrMemCmp(localPtr, wapillcheader, sizeof(wapillcheader)) == 0))
        {
            CsrWifiSecurityProcessPacket(pSecurityCtrlBlk->securityContext, (CsrUint32)packetBuffer, packetBuffer+12,  packetBufferLength-12);
        }
        else
        {
            sme_trace_warn((TR_NME_SMGR_FSM, "Unknown Packet type. ignore and free"));
            CsrPfree(packetBuffer);
        }
    }
    else
    {
        sme_trace_warn((TR_NME_SMGR_FSM, "Security Lib is not active: ignore and free"));
        CsrPfree(packetBuffer);
    }
}

/*
 * Description:
 * See description in nme_security_fsm/nme_security.h
 */
void nme_security_timer_expired(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        CsrUint8 timeoutIndex)
{
    require(TR_NME_SMGR_FSM, NULL != pContext);
    require(TR_NME_SMGR_FSM, NULL != pSecurityCtrlBlk);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_security_timer_expired(sec lib id: %d) - timer id: %d", timeoutIndex, pSecurityCtrlBlk->secLibTimerId[timeoutIndex].uniqueid));
    pSecurityCtrlBlk->secLibTimerId[timeoutIndex].uniqueid = 0;
    CsrWifiSecurityTimerExpired(pSecurityCtrlBlk->securityContext, (CsrWifiSecurityTimeout) timeoutIndex);

}

/*
 * Description:
 * See description in nme_security_fsm/nme_security.h
 */
void nme_handle_michael_mic_failure(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        unifi_KeyType keyType,
        CsrUint8 *tsc)
{
    require(TR_NME_SMGR_FSM, NULL != pContext);
    require(TR_NME_SMGR_FSM, NULL != pSecurityCtrlBlk);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_handle_michael_mic_failure" ));

    CsrWifiSecuritySendEapolError(pSecurityCtrlBlk->securityContext, (CsrBool)(keyType==unifi_PairwiseKey), tsc );
}
