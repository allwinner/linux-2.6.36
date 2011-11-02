/** @file csr_loopback_wpa.c
 *
 * Implementation of a loop back test
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of a loop back test between WPA authenticator and supplicant
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_helper/common/csr_loopback_wpa.c#6 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

/* PROJECT INCLUDES *********************************************************/
#include "linux_sap_platform.h"
#include "nme_top_level_fsm/nme_top_level_fsm.h"
#include "sme_top_level_fsm/sme.h"
#include "csr_security.h"
#include "csr_passphrase_hashing.h"
#include "sme_trace/sme_trace.h"
#include "lib_info_element.h"

/* MACROS *******************************************************************/
/* GLOBAL VARIABLE DEFINITIONS **********************************************/
/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/
static LinuxUserSpaceContext* linuxContext;
static CsrUint8 authMac[6];
static CsrUint8 suppMac[6];
static CsrUint16 secielength;
static CsrUint8 secie[64];
static const CsrUint8 eapolllcheader[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8E};
static CsrUint8 localPsk[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\0' };
static CsrUint8 localSsid[32];
static CsrUint8 localSsidLength;

static void* savedAppHandle;
static CsrUint8 savedSubscriptionHandle;

static CsrUint8 localPmk[CSR_WIFI_SECURITY_PMK_LENGTH];

static CsrUint8 sendPacket[1024];


static CsrWifiSecurityContext *pAuthCtx = NULL;

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/
void authSend(void *myCtx, const CsrUint8 *pBuffer, const CsrUint32 bufLen, const CsrUint8 *pLocalMac, const CsrUint8 *pPeerMac)
{
    sme_trace_info((TR_SME_TRACE, "Authenticator Send EAPOL-KEY" ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "Source MAC", pLocalMac, CSR_WIFI_SECURITY_MACADDRESS_LENGTH ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "Destination MAC", pPeerMac, CSR_WIFI_SECURITY_MACADDRESS_LENGTH ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "Data", pBuffer, bufLen ));

    if( 0x03 == pBuffer[9] )
    {
        CsrMemCpy(sendPacket,     pPeerMac,  6);
        CsrMemCpy(&sendPacket[6], pLocalMac, 6);
        CsrMemCpy(&sendPacket[12],pBuffer,   bufLen);

        unifi_nme_sys_ma_unitdata_ind(linuxContext->nmeFsmContext,
                                      savedAppHandle,
                                      savedSubscriptionHandle,
                                      (CsrUint16)bufLen+12,
                                      sendPacket,
                                      NULL,
                                      unifi_ReceptionStatusRxSuccess,
                                      unifi_PriorityQosUp0,
                                      unifi_ServiceClassStrictlyOrdered);
    }
    else
    {
        sme_trace_hex((TR_SME_TRACE, TR_LVL_ERROR, "authSend :: EAPOL", pBuffer, bufLen ));
    }
}

void authInstallPairwise (void* clientContext, CsrWifiSecurityKeyType keyType, const CsrUint8* key,  const CsrUint32 keyLength, const CsrUint8* rsc, const CsrUint8 keyIndex)
{
    sme_trace_info((TR_SME_TRACE, "Authenticator Install Keys" ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "Pairwise", key, keyLength ));
}

void authInstallGroup (void* clientContext, CsrWifiSecurityKeyType keyType, const CsrUint8 *key, const CsrUint32 keyLength, const CsrUint8* rsc, const CsrUint8 keyIndex)
{
    sme_trace_info((TR_SME_TRACE, "Authenticator Install Keys" ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "Group", key, keyLength ));
}

void authInstallPac( void *myCtx, const CsrUint8 *pac, const CsrUint32 pacLength )
{
    sme_trace_info((TR_SME_TRACE, "Authenticator Install PAC" ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "pac", pac, pacLength ));
}

void suppInstallPairwise (void* clientContext, CsrWifiSecurityKeyType keyType, const CsrUint8* key,  const CsrUint32 keyLength, const CsrUint8* rsc, const CsrUint8 keyIndex)
{
    sme_trace_info((TR_SME_TRACE, "Supplicant Install Keys" ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "Pairwise", key, keyLength ));
}

void suppInstallGroup (void* clientContext, CsrWifiSecurityKeyType keyType, const CsrUint8 *key, const CsrUint32 keyLength, const CsrUint8* rsc, const CsrUint8 keyIndex)
{
    sme_trace_info((TR_SME_TRACE, "Supplicant Install Keys" ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "Group", key, keyLength ));
}

void suppInstallPac( void *myCtx, const CsrUint8 *pac, const CsrUint32 pacLength )
{
    sme_trace_info((TR_SME_TRACE, "Supplicant Install PAC" ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "pac", pac, pacLength ));
}

void suppInstallSession( void *myCtx, const CsrUint8 *session, const CsrUint32 sessionLength )
{
    sme_trace_info((TR_SME_TRACE, "Supplicant Install Session" ));
    sme_trace_hex((TR_SME_TRACE, TR_LVL_INFO, "session", session, sessionLength ));
}

void wpsDone(void* myCtx, const CsrUint8 result, const CsrUint32 authenticationType,
                    const CsrUint32 encryptionType, const CsrUint8 *networkKey, const CsrUint16 networkKeyLength,
                    const CsrUint8 networkKeyIndex, const CsrUint8 *ssid)
{
    sme_trace_info((TR_SME_TRACE, "WPS Done" ));
}

void timerStart ( void *pClientCtx, const CsrUint32 duration, CsrWifiSecurityTimeout timeoutId)
{
    sme_trace_info((TR_SME_TRACE, "Timer start %d, id %d", duration, timeoutId));
}

void timerStop (void *pClientCtx, CsrWifiSecurityTimeout timeoutId)
{
    sme_trace_info((TR_SME_TRACE, "Timer stop id %d", timeoutId ));
}

void abortProcedure (void *pClientCtx)
{
    sme_trace_info((TR_SME_TRACE, "Abort procedure" ));
}

void packetProcessingDone(void *myCtx, CsrUint32 appCookie)
{
    sme_trace_entry((TR_SME_TRACE, "packet processing done"));
}

static void pmkid_store(void* myCtx, const CsrUint8 bssid[CSR_WIFI_SECURITY_MACADDRESS_LENGTH], const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH], const CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH])
{
}

static CsrBool pmk_get(void* myCtx, const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH], CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH])
{
    return FALSE;
}

/* Callbacks */
static const CsrWifiSecuritycallbacks authSecCallbacks = {
    authSend,
    authInstallPairwise,
    authInstallGroup,
    suppInstallPac,
    suppInstallSession,
    wpsDone,
    timerStart,
    timerStop,
    abortProcedure,
    packetProcessingDone,
    pmkid_store,
    pmk_get
};


static void auth_init()
{
    CsrWifiSecuritySetup setupData;
    CsrMemSet(&setupData, 0x00, sizeof(CsrWifiSecuritySetup));

    if (pAuthCtx)
    {
        CsrWifiSecurityDeInit(pAuthCtx);
        pAuthCtx = NULL;
    }

    setupData.secIeLen = secielength;
    setupData.secIe = (CsrUint8*) CsrPmalloc(setupData.secIeLen);
    CsrMemCpy(setupData.secIe, secie, setupData.secIeLen);
    CsrMemCpy(setupData.peerMacAddress, suppMac, 6);
    CsrMemCpy(setupData.localMacAddress, authMac, 6);
    setupData.mode = CSR_WIFI_SECURITY_WPAPSK_MODE_AUTHENTICATOR;
    setupData.securityType = CSR_WIFI_SECURITY_TYPE_WPAPSK_AUTHENTICATOR;

    setupData.responseTimeout = 100;
    setupData.retransmissionAttempts = 3;
    setupData.protocolSnapValid = TRUE;
    CsrMemCpy(setupData.protocolSnap, eapolllcheader, CSR_WIFI_SECURITY_SNAP_LENGTH);

    if (strlen((const char *)localPsk) && localSsidLength)
    {
        CsrCryptoWpaPskHash((char *)localPsk, localSsid, localSsidLength, localPmk);
        CsrMemCpy(setupData.pmk, localPmk, CSR_WIFI_SECURITY_PMK_LENGTH);
        setupData.pmkValid = TRUE;
    }

    #if 0
    if (strlen((const char *)identity))
    {
        setupData.identity = identity;
    }
    if (strlen((const char *)username))
    {
        setupData.username = username;
    }
    if (strlen((const char *)password))
    {
        setupData.password = password;
    }
    #endif

    pAuthCtx = CsrWifiSecurityInit(NULL, NULL, &setupData, &authSecCallbacks);
    CsrPfree(setupData.secIe);
}



void unifi_nme_sys_eapol_req(void* context, void* appHandle, CsrUint8 subscriptionHandle, CsrUint16 frameLength, const CsrUint8 *frame, unifi_FrameFreeFunction freeFunction)
{
    sme_trace_hex((TR_SME_TRACE, TR_LVL_ERROR, "unifi_nme_sys_eapol_req", frame, frameLength));
    /* Init the Authenticator if this is an Eapol start */
    if (frameLength == 24 &&
        frame[20] == 0x02 &&
        frame[21] == 0x01 &&
        frame[22] == 0x00 &&
        frame[23] == 0x00)
    {
        unifi_Status status;
        ie_result result;
        CsrUint8* rsnptr;
        unifi_AppValue appValue;
        /* 31 31 31 31 31 31 00 01 02 03 04 05 AA AA 03 00 00 00 88 8E 02 01 00 00 ---- Eapol Start*/
        /* Fill in the Mac Addresses */
        CsrMemCpy(authMac, frame, 6);
        CsrMemCpy(suppMac, &frame[6], 6);

        linuxContext = (LinuxUserSpaceContext*)context;
        savedAppHandle = appHandle;
        savedSubscriptionHandle = subscriptionHandle;

        /* Fill in the Rsn And Psk */
        unifi_mgt_claim_sync_access(linuxContext->fsmContext);
        appValue.id = unifi_ConnectionInfoValue;
        status = unifi_mgt_get_value(linuxContext->fsmContext, &appValue);

        if (status != unifi_Success)
        {
            sme_trace_error((TR_SME_TRACE, "unifi_mgt_get_value(unifi_ConnectionInfoValue) Failed" ));
            unifi_mgt_release_sync_access(linuxContext->fsmContext);
            return;
        }
        if (appValue.unifi_Value_union.connectionInfo.assocReqInfoElementsLength == 0)
        {
            sme_trace_error((TR_SME_TRACE, "assocReqInfoElementsLength == 0" ));
            unifi_mgt_release_sync_access(linuxContext->fsmContext);
            return;
        }

        sme_trace_hex((TR_SME_TRACE, TR_LVL_ERROR, "unifi_nme_sys_eapol_req() ACCOC IE", appValue.unifi_Value_union.connectionInfo.assocReqInfoElements, appValue.unifi_Value_union.connectionInfo.assocReqInfoElementsLength));

        result = ie_find(IE_ID_RSN,
                         appValue.unifi_Value_union.connectionInfo.assocReqInfoElements,
                         appValue.unifi_Value_union.connectionInfo.assocReqInfoElementsLength,
                         &rsnptr);

        if (result != ie_success)
        {
            result = ie_find_vendor_specific(ieWpaOui,
                                             ieWpaOuiSubtype,
                                             0,
                                             appValue.unifi_Value_union.connectionInfo.assocReqInfoElements,
                                             appValue.unifi_Value_union.connectionInfo.assocReqInfoElementsLength,
                                             &rsnptr);

            if (result != ie_success)
            {
                sme_trace_error((TR_SME_TRACE, "ie_find(ieWpaOui, ieWpaOuiSubtype, 1) Failed" ));
                unifi_mgt_release_sync_access(linuxContext->fsmContext);
                return;
            }
        }

        secielength = ie_len(rsnptr) + 2;
        CsrMemCpy(secie, rsnptr, secielength);

        localSsidLength = appValue.unifi_Value_union.connectionInfo.ssid.length;
        CsrMemCpy(localSsid, appValue.unifi_Value_union.connectionInfo.ssid.ssid, sizeof(localSsid));

        sme_trace_hex((TR_SME_TRACE, TR_LVL_ERROR, "unifi_nme_sys_eapol_req() RSN", secie, secielength));

        unifi_mgt_release_sync_access(linuxContext->fsmContext);

        auth_init();
        CsrWifiSecurityStart(pAuthCtx);
        return;
    }

    if (pAuthCtx == NULL)
    {
        sme_trace_error((TR_SME_TRACE, "pAuthCtx == NULL" ));
        return;
    }

    CsrWifiSecurityProcessPacket(pAuthCtx, (CsrUint32)NULL, &frame[12], frameLength - 12);
}

void csr_wpa_loopback_deinit()
{
    if (pAuthCtx)
    {
        CsrWifiSecurityDeInit(pAuthCtx);
        pAuthCtx = NULL;
    }
}


//int main(int argc, char *argv[])
//{
//    sme_trace_initialise(argc, argv);
//
//    supplicant_init();
//    auth_init();
//
//    CsrWifiSecurityStart(pAuthCtx);
//
//    CsrWifiSecurityDeInit(pAuthCtx);
//    CsrWifiSecurityDeInit(pSuppCtx);
//
//    return 0;
//}
