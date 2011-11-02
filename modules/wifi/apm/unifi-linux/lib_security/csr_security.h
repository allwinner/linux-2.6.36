/** @file csr_security.h
 *
 * Definitions for Security library.
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
 *   This provides an implementation of Security library
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_security.h#2 $
 *
 ****************************************************************************/

#ifndef CSR_WIFI_SECURITY_H
#define CSR_WIFI_SECURITY_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"

/* DEFINE DEPENDANCIES ******************************************************/
#if defined (CSR_WIFI_SECURITY_FAST_ENABLE) || defined (CSR_WIFI_SECURITY_LEAP_ENABLE) || defined (CSR_WIFI_SECURITY_TLS_ENABLE) || defined (CSR_WIFI_SECURITY_TTLS_ENABLE) || defined (CSR_WIFI_SECURITY_SIM_ENABLE)
#ifndef CSR_WIFI_SECURITY_EAP_ENABLE
#define CSR_WIFI_SECURITY_EAP_ENABLE
#endif
#ifndef CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE
#define CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE
#endif
#endif

#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
#define CSR_WIFI_SECURITY_WPS_KEYLEN 128
#endif

#ifdef CSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE
#ifndef CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
#define CSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
#endif
#endif



/* MACROS *******************************************************************/
#define CSR_WIFI_SECURITY_MACADDRESS_LENGTH 6
#define CSR_WIFI_SECURITY_SSID_LENGTH 32
#define CSR_WIFI_SECURITY_PMKID_LENGTH 16
#define CSR_WIFI_SECURITY_PMK_LENGTH 32
#define CSR_WIFI_SECURITY_SNAP_LENGTH (8)

#define CSR_WIFI_SECURITY_MAX_PAC_LENGTH 500
#define CSR_WIFI_SECURITY_MAX_CERTIFICATE_LENGTH 2048
#define CSR_WIFI_SECURITY_MAX_SESSION_LENGTH 80

/* Timeout limiting total security process from call to security_start to setting keys */
#define CSR_WIFI_SECURITY_CONFIGSA_TIMEOUT (60 * 1000) /* in milliseconds */

/* When we do not receive first message from AP, these timeouts indicate the interval to send EAPOL start */
#define CSR_WIFI_SECURITY_START_FIRST (100) /* in milliseconds */
#define CSR_WIFI_SECURITY_START_SUBSEQUENT (500) /* in milliseconds */

#define CSR_WIFI_SECURITY_MAX_FRAGMENT_SIZE 1500    /* Maximum fragment size that can be transmitted including SNAP header */

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PUBLIC TYPE DEFINITIONS ************************************************/
struct CsrCryptoContext;
typedef struct CsrWifiSecurityContext CsrWifiSecurityContext;

typedef enum {

    CSR_WIFI_SECURITY_WPS_AUTH_TYPE_OPEN    = 0x0001,
    CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPAPSK  = 0x0002,
    CSR_WIFI_SECURITY_WPS_AUTH_TYPE_SHARED  = 0x0004,
    CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA     = 0x0008,
    CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA2    = 0x0010,
    CSR_WIFI_SECURITY_WPS_AUTH_TYPE_WPA2PSK = 0x0020

} CsrWifiSecurityWPSAuthMode;

typedef enum {

    CSR_WIFI_SECURITY_WPS_ENC_TYPE_NONE = 0x0001,
    CSR_WIFI_SECURITY_WPS_ENC_TYPE_WEP  = 0x0002,
    CSR_WIFI_SECURITY_WPS_ENC_TYPE_TKIP = 0x0004,
    CSR_WIFI_SECURITY_WPS_ENC_TYPE_AES  = 0x0008

} CsrWifiSecurityWPSEncMode;

/* TODO: Merge this along with CsrWifiSecurityWPSEncMode, eliminate this */
/* Cipher suites ordered in increasing strength */
typedef enum {

    CSR_WIFI_SECURITY_WPS_CIPHER_SUITE_UNKNOWN = 0,
    CSR_WIFI_SECURITY_WPS_CIPHER_SUITE_WEP_40  = 1,
    CSR_WIFI_SECURITY_WPS_CIPHER_SUITE_WEP_104 = 2,
    CSR_WIFI_SECURITY_WPS_CIPHER_SUITE_TKIP    = 3,
    CSR_WIFI_SECURITY_WPS_CIPHER_SUITE_CCMP    = 4

} CsrWifiSecurityWPSCipherSuite;

/* TODO: Get rid of this */
#define WPS_PUSHBUTTON_PIN "00000000"

typedef enum {
    CSR_WIFI_SECURITY_TYPE_NONE,
    CSR_WIFI_SECURITY_TYPE_TLS,
    CSR_WIFI_SECURITY_TYPE_TTLS,
    CSR_WIFI_SECURITY_TYPE_LEAP,
    CSR_WIFI_SECURITY_TYPE_FAST,
    CSR_WIFI_SECURITY_TYPE_PEAP,
    CSR_WIFI_SECURITY_TYPE_SIM,
    CSR_WIFI_SECURITY_TYPE_AKA,
    CSR_WIFI_SECURITY_TYPE_GTC,
    CSR_WIFI_SECURITY_TYPE_MSCHAPV2,
    CSR_WIFI_SECURITY_TYPE_WPS,
    CSR_WIFI_SECURITY_TYPE_WPAPSK_AUTHENTICATOR,
    CSR_WIFI_SECURITY_TYPE_WPAPSK_SUPPLICANT,
    CSR_WIFI_SECURITY_TYPE_WAPIPSK_AE,
    CSR_WIFI_SECURITY_TYPE_WAPIPSK_ASUE,
    CSR_WIFI_SECURITY_TYPE_WAPI_ASUE
} CsrWifiSecurityType;

typedef enum {
    CSR_WIFI_SECURITY_ABORTREASON_RETRY_LIMIT,
    CSR_WIFI_SECURITY_ABORTREASON_ASSOCIATION_TIMEOUT,
    CSR_WIFI_SECURITY_ABORTREASON_INVALID_INPUT
} CsrWifiSecurityAbortReason;

typedef enum {
    CSR_WIFI_SECURITY_TIMEOUT_MESSAGE_RETRY,
    CSR_WIFI_SECURITY_TIMEOUT_SECURITY_START,
    CSR_WIFI_SECURITY_TIMEOUT_SECURITY_ASSOCIATION,
    CSR_WIFI_SECURITY_TIMEOUT_MAX_TIMEOUTS
} CsrWifiSecurityTimeout;

typedef enum {
    CSR_WIFI_SECURITY_KEYTYPE_IEEE80211I,
    CSR_WIFI_SECURITY_KEYTYPE_LEAPWEP,
    CSR_WIFI_SECURITY_KEYTYPE_WAPI
} CsrWifiSecurityKeyType;

typedef struct CsrWifiSecuritycallbacks
{
    void (*sendPacket)           (void* clientContext,
                                  const CsrUint8* buffer,
                                  const CsrUint32 bufferLength,
                                  const CsrUint8* localMacAddress,
                                  const CsrUint8* peerMacAddress);

    void (*installPairwiseKey)   (void* clientContext,
                                  CsrWifiSecurityKeyType keyType,
                                  const CsrUint8* key,
                                  const CsrUint32 keyLength,
                                  const CsrUint8* rsc,
                                  const CsrUint8 keyIndex);

    void (*installGroupKey)      (void* clientContext,
                                  CsrWifiSecurityKeyType keyType,
                                  const CsrUint8 *key,
                                  const CsrUint32 keyLength,
                                  const CsrUint8* rsc,
                                  const CsrUint8 keyIndex);

    void (*installPac)           (void* clientContext,
                                  const CsrUint8 *pac,
                                  const CsrUint32 pacLength );

    void (*installSession)       (void* clientContext,
                                  const CsrUint8 *session,
                                  const CsrUint32 sessionLength );

    void (*wpsDone)              (void* clientContext,
                                  const CsrUint8 result,
                                  const CsrUint32 authType,
                                  const CsrUint32 encType,
                                  const CsrUint8 *networkKey,
                                  const CsrUint16 networkKeyLength,
                                  const CsrUint8 networkKeyIndex,
                                  const CsrUint8 *ssid);

    void (*startTimer)           (void* clientContext,
                                  const CsrUint32 durationMs,
                                  const CsrWifiSecurityTimeout timeoutIndex);

    void (*stopTimer)            (void* clientContext,
                                  const CsrWifiSecurityTimeout timeoutIndex);

    void (*abortProcedure)       (void* clientContext);

    void (*packetProcessingDone) (void* clientContext,
                                  CsrUint32 appCookie);

    void (*pmkidStore)           (void* clientContext,
                                  const CsrUint8 bssid[CSR_WIFI_SECURITY_MACADDRESS_LENGTH],
                                  const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH],
                                  const CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH]);

    CsrBool (*pmkGet)            (void* clientContext,
                                  const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH],
                                  CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH]);
} CsrWifiSecuritycallbacks;


typedef enum
{
    CSR_WIFI_SECURITY_WPAPSK_MODE_AUTHENTICATOR,
    CSR_WIFI_SECURITY_WPAPSK_MODE_SUPPLICANT,
    CSR_WIFI_SECURITY_WPA_MODE_SUPPLICANT
#ifdef CSR_WIFI_SECURITY_WPS_ENABLE
    ,
    CSR_WIFI_SECURITY_WPS_MODE_SUPPLICANT
#endif
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    ,
    CSR_WIFI_SECURITY_WAPIPSK_MODE_AE,
    CSR_WIFI_SECURITY_WAPIPSK_MODE_ASUE,
    CSR_WIFI_SECURITY_WAPI_MODE_ASUE
#endif
} CsrWifiSecurityMode;


typedef struct CsrWifiSecuritySetup
{
    CsrWifiSecurityType securityType;
    CsrWifiSecurityMode mode;

    CsrUint8      localMacAddress[CSR_WIFI_SECURITY_MACADDRESS_LENGTH];
    CsrUint8      peerMacAddress[CSR_WIFI_SECURITY_MACADDRESS_LENGTH];
    CsrBool       pmkValid;
    CsrUint8      pmk[CSR_WIFI_SECURITY_PMK_LENGTH];
    CsrUint8      *secIe;
    CsrUint16     secIeLen;


    /* Which Snap To use with this Authentication */
    /* 0xAA 0xAA 0x03 0x00 0x00 0x00 0x88 0x8E for WPA */
    /* 0xAA 0xAA 0x03 0x00 0x00 0x00 0x88 0xC7 for WPA Preauthentication*/
    /* 0xAA 0xAA 0x03 0x00 0x00 0x00 0x?? 0x?? for Amp handshake*/
    /* 0xAA 0xAA 0x03 0x00 0x00 0x00 0x88 0xB4 for WAPI */
    CsrBool        protocolSnapValid;
    CsrUint8       protocolSnap[CSR_WIFI_SECURITY_SNAP_LENGTH];

    char*          identity;
    char*          username;
    char*          password;

    CsrUint32      session_length;
    CsrUint8       *session;


    CsrUint32      fast_pac_length;
    CsrUint8       *fast_pac;


    CsrUint32      clientCertificateLength;
    CsrUint8       *clientCertificate;

    CsrUint32      clientPrivateKeyLength;
    CsrUint8       *clientPrivateKey;


    CsrUint32       authenticationServerCertificateLength;
    CsrUint8       *authenticationServerCertificate;


    /* Required for WPS */
    CsrUint8       bssid[CSR_WIFI_SECURITY_MACADDRESS_LENGTH];
    CsrUint8       ssid[CSR_WIFI_SECURITY_SSID_LENGTH];
    CsrUint8       pin[9];    /* Only 8 digit PINs supported */

    /* Retransmission parameters */
    CsrUint16      responseTimeout;
    CsrUint8       retransmissionAttempts;

} CsrWifiSecuritySetup;

/* PUBLIC CONSTANT DEFINITIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/
/**
 * @brief
 *   Init function for the handshake library
 *
 * @par Description
 *   Called during init, this function initializes module data structures
 * @param
 *   void*                 : External context to pass to all callbacks
 *   CsrCryptoContext*     : CryptoContext to pass to all crypto calls
 *   CsrWifiSecuritySetup*     : Setup data for the Security (Credentials etc)
 *   CsrWifiSecuritycallbacks* : callbacks for use by the security implementation

 * @return
 *   CsrWifiSecurityContext* : Context or NULL on error
 */
CsrWifiSecurityContext* CsrWifiSecurityInit(void* externalContext,
                                      struct CsrCryptoContext* cryptoContext,
                                      const CsrWifiSecuritySetup* setupData,
                                      const CsrWifiSecuritycallbacks* callbacks);

/**
 * @brief
 *   Deinit function for the handshake library
 *
 * @par Description
 *   Called to cleanup the security library
 *
 * @param
 *   CsrWifiSecurityContext*: Context obtained during init
 * @return
 *   void
 */
void CsrWifiSecurityDeInit(CsrWifiSecurityContext* context);

/**
 * @brief
 *   Function to initiate the security procedure
 *
 * @par Description
 *   Called after init, this function starts handshake procedure by sending a
 *   EAPOL start if mode is supplicant or M 1 of 4 way handshake if mode is
 *   authenticator
 *
 * @param
 *   CsrWifiSecurityContext*: Context obtained during init
 * @return
 *   void
 */
void CsrWifiSecurityStart(CsrWifiSecurityContext* context);

/**
 * @brief
 *   Send an eapol error (Probably due to a michael mic failure)
 *
 * @par Description
 *   Need to send this message before initiating counter measures after a mic failure
 *
 * @param
 *   CsrWifiSecurityContext*: Context obtained during init
 * @return
 *   void
 */
void CsrWifiSecuritySendEapolError(CsrWifiSecurityContext* context, CsrBool pairwise, CsrUint8 *tsc);

/**
 * @brief
 *   Entry point for EAPOL packets
 *
 * @par Description
 *   Called to process EAPOL packets
 *
 * @param
 *   CsrWifiSecurityContext*: Context obtained during init
 *   CsrUint8*             : EAPOL packet (with the SNAP header)
 *   CsrUint32             : Length of the EAPOL packet
 * @return
 *   void
 */
void CsrWifiSecurityProcessPacket(CsrWifiSecurityContext* context, const CsrUint32 appCookie, const CsrUint8* packetBuffer, const CsrUint32 packetBufferLength);


/**
 * @brief
 *      Handle a timeout
 *
 * @par Description
 *   Called when a registered timeout has expired. Retransmission or abort happens.
 *
 * @param
 *   context: Context obtained during init

 * @return
 *   void
 */
void CsrWifiSecurityTimerExpired(CsrWifiSecurityContext* context, CsrWifiSecurityTimeout timeoutIndex);

#ifdef __cplusplus
}
#endif

#endif /* CSR_WIFI_SECURITY_H */
