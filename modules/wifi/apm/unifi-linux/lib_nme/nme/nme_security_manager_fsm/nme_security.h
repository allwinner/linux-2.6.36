/** @file nme_security.h
 *
 * Public NME security API
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
 *   Public NME security API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_security_manager_fsm/nme_security.h#2 $
 *
 ****************************************************************************/
#ifndef NME_SECURITY_H_
#define NME_SECURITY_H_
#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup nme_core
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/fsm_types.h"
#include "csr_security.h"
/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

typedef struct nmeSecurityCtrlBlk
{
    /* pointer to parent context */
    FsmContext* securityFsmContext;

    /* security context as required by lib_security */
    CsrWifiSecurityContext* securityContext;

    CsrUint16 nmeSecurityMgrFsmInstance;

    /* Setup structure passed to the security lib at startup.
     * Currently there is some duplication between the contents
     * of the nmeSecurityCtrlBlk and the CsrWifiSecuritySetup.
     * Note:
     * Need to decide on how best to handle this:
     * Is it better to have the data in the correct structure for
     * passing to the security lib.
     * Or should the security lib structure be populated ready for
     * use at the appropriate point that it is actually needed?
     */
    CsrWifiSecuritySetup setupData;

    /* station MacAddress */
    unifi_MACAddress stationMacAddress;
    /* peer MacAddress */
    unifi_MACAddress peerMacAddress;

    /* Need to track the SSID so that if it it changes and the credentials
     * are WPA/WPA2 passphrase, then we can generate a new pmk.
     */
    unifi_SSID peerSsid;

    /* Need to be aware of when we are handling a WPS request as there are
     * no credentials at the start of security.
     */
    CsrBool isWps;

    /* WPS returns the SSID to the network discovered
     */
    unifi_SSID WpsSsid;

    /* credentials */
    unifi_Credentials credentials;

    /* FAST PAC data when a new PAC is provided by the network
     * the PAC will be stored here and then, pushed across
     * to the profile.
     */
    CsrUint8 *pac;
    CsrUint32 pacLength;

    /* When new session data is provided by the network
     * the session will be stored here and then, pushed across
     * to the profile.
     */
    CsrUint8 *session;
    CsrUint32 sessionLength;

    /* If a server side certificate needs to be validated these
     * fields will hold the certificate until it has been sent
     * in a NME CERTIFICATE VALIDATE IND. Once sent there is no need
     * to retain the information as we are only waiting for a valid
     * invalid response.
     */
    CsrUint8 *certificate;
    CsrUint32 certificateLength;

    /* Security lib will request us to run timers for it when required
     * at the moment it will run up to 2 timers, which can overlap
     * so we need to ensure that we match the right internal timer
     * id to that used by the security lib.
     */
    FsmTimerId secLibTimerId[CSR_WIFI_SECURITY_TIMEOUT_MAX_TIMEOUTS];

    CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH];

    /* EAPOL subscription data */
    CsrUint8  subscriptionHandle;
    CsrUint16 allocOffset;

} nmeSecurityCtrlBlk;

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/** @}
 */

/**
 * @brief
 *   initialises the security control block
 *
 * @par Description
 *   See brief
 *
 * @param[in]    pContext : FSM context
 * @param[in]    pSecurityCtrlBlk : Security Control Block
 * @param[in]    CsrBool : WPS
 * @param[in]    CsrUint8 * : WPS Pin (only valid if WPS)
 * @param[in]    pCredentials : Credentials structure (only valid if not WPS)
 * @param[in]    stationMacAddress : station MacAddress
 * @param[in]    peerMacAddress : peer MacAddress
 *
 * @return
 *   void
 */
extern void nme_security_init(
        FsmContext* pContext,
        CsrUint16 nmeSecurityMgrFsmInstance,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        CsrBool isWps,
        CsrUint8 *pWpsPin,
        const unifi_Credentials *pCredentials,
        const unifi_SSID ssid,
        const unifi_MACAddress stationMacAddress);

/**
 * @brief
 *   Stop any security related timers that are running
 *
 * @par Description
 *   See brief
 *
 * @param[in]    FsmContext* : FSM context
 * @param[in]    nmeSecurityCtrlBlk* : Security Control Block
 *
 * @return
 *   void
 */
extern void nme_security_stop_all_timers(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk);


/**
 * @brief
 *   Deinitialises the security control block
 *
 * @par Description
 *   See brief
 *
 * @param[in]    pContext : FSM context
 * @param[in]    pSecurityCtrlBlk : Security Control Block
 *
 * @return
 *   void
 */
extern void nme_security_deinit(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk);

/**
 * @brief
 *   start the security process
 *
 * @par Description
 *   The basic authentication has been successful and further 802.1x
 *   authentication my be required. has been established and now
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] nmeSecurityCtrlBlk* : Security Control Block
 * @param[in] CsrUint16 : IE length
 * @param[in] CsrUint8* : Association request IE
 * @param[in] const unifi_SSID : peer SSID
 * @param[in] const unifi_MACAddress : peer MAC Address
 *
 * @return
 *   void
 */
extern void nme_security_start(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        CsrUint16 ieLength,
        CsrUint8 *ie,
        const unifi_SSID peerSsid,
        const unifi_MACAddress peerMacAddress);

/**
 * @brief
 *   restart the security process
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] nmeSecurityCtrlBlk* : Security Control Block
 * @param[in] CsrUint16 : IE length
 * @param[in] CsrUint8* : Association request IE
 * @param[in] const unifi_SSID : peer SSID
 * @param[in] const unifi_MACAddress : peer MAC Address
 *
 * @return
 *   void
 */
extern void nme_security_restart(
        FsmContext* pContext,
        nmeSecurityCtrlBlk* pSecurityCtrlBlk,
        CsrUint16 ieLength,
        CsrUint8* pIe,
        const unifi_SSID peerSsid,
        const unifi_MACAddress peerMacAddress);

/**
 * @brief
 *   stops the security process
 *
 * @par Description
 *   See brief.
 *
 * @param[in] FsmContext* : FSM context
 * @param[in] nmeSecurityCtrlBlk* : Security Control Block
 *
 * @return
 *   void
 */
extern void nme_security_stop(
        FsmContext* pContext,
        nmeSecurityCtrlBlk* pSecurityCtrlBlk);

/**
 * @brief
 *   Processes and forwards a packet to lib_security as required
 *
 * @par Description
 *   See brief
 *
 * @param[in]    pContext : FSM context
 * @param[in]    pSecurityCtrlBlk : Security Control Block
 * @param[in]    packetBuffer : Pointer to packet
 * @param[in]    packetBufferLength : packet length
 *
 * @return
 *   void
 */
extern void nme_security_process_ma_unitdata_packet(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        CsrUint8* packetBuffer,
        const CsrUint16 packetBufferLength);

/**
 * @brief
 *   inform lib_security that an eap timer out has occurred
 *
 * @par Description
 *   See brief
 *
 * @param[in]    pContext : FSM context
 * @param[in]    pSecurityCtrlBlk : Security Control Block
 * @param[in]    timeoutIndex : timeout type
 *
 * @return
 *   void
 */
extern void nme_security_timer_expired(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        CsrUint8 timeoutIndex);

/**
 * @brief
 *   Indicate to lib_security that a michael mic failure has occurred
 *
 * @par Description
 *   See brief
 *
 * @param[in]    pContext : FSM context
 * @param[in]    pSecurityCtrlBlk : Security Control Block
 * @param[in]    unifi_KeyType : keyType
 * @param[in]    CsrUint8* : TSC
 *
 * @return
 *   void
 */
extern void nme_handle_michael_mic_failure(
        FsmContext* pContext,
        nmeSecurityCtrlBlk *pSecurityCtrlBlk,
        unifi_KeyType keyType,
        CsrUint8 *tsc);


#ifdef __cplusplus
}
#endif

#endif /* NME_SECURITY_H_ */
