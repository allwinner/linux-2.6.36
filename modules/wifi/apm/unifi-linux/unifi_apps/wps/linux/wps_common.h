/** @file wps_common.h
 *
 * Definitions for Wi-Fi Protected Setup common.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of a Wi-Fi Protected Setup common functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_common.h#3 $
 *
 ****************************************************************************/
#ifndef WPS_COMMON_H
#define WPS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <malloc.h>
#include "csr_types.h"

#define WPS_AP_CHANNEL                        0x1001
#define WPS_ASSOCIATION_STATE                 0x1002
#define WPS_AUTHENTICATION_TYPE               0x1003
#define WPS_AUTHENTICATION_TYPE_FLAGS         0x1004
#define WPS_AUTHENTICATOR                     0x1005
#define WPS_CONFIG_METHODS                    0x1008
#define WPS_CONFIGURATION_ERROR               0x1009
#define WPS_CONFIRMATION_URL4                 0x100A
#define WPS_CONFIRMATION_URL6                 0x100B
#define WPS_CONNECTION_TYPE                   0x100C
#define WPS_CONNECTION_TYPE_FLAGS             0x100D
#define WPS_CREDENTIAL                        0x100E
#define WPS_ENCRYPTION_TYPE                   0X100F
#define WPS_ENCRYPTION_TYPE_FLAGS             0x1010
#define WPS_DEVICE_NAME                       0x1011
#define WPS_DEVICE_PASSWORD_ID                0x1012
#define WPS_E_HASH1                           0x1014
#define WPS_E_HASH2                           0x1015
#define WPS_E_SNONCE1                         0x1016
#define WPS_E_SNONCE2                         0x1017
#define WPS_ENCRYPTED_SETTINGS                0x1018
#define WPS_ENROLLEE_NONCE                    0x101A
#define WPS_FEATURE                           0x101B
#define WPS_IDENTITY                          0X101C
#define WPS_IDENTITY_PROOF                    0X101D
#define WPS_KEY_WRAP_AUTHENTICATOR            0x101E
#define WPS_KEY_IDENTIFIER                    0X101F
#define WPS_MAC_ADDRESS                       0x1020
#define WPS_MANUFACTURER                      0x1021
#define WPS_MESSAGE_TYPE                      0x1022
#define WPS_MODEL_NAME                        0x1023
#define WPS_MODEL_NUMBER                      0x1024
#define WPS_NETWORK_INDEX                     0x1026
#define WPS_NETWORK_KEY                       0x1027
#define WPS_NETWORK_KEY_INDEX                 0x1028
#define WPS_NEW_DEVICE_NAME                   0x1029
#define WPS_NEW_PASSWORD                      0x102A
#define WPS_OOB_DEVICE_PASSWORD               0x102C
#define WPS_OS_VERSION_ID                     0x102D
#define WPS_POWER_LEVEL                       0x102F
#define WPS_PSK_CURRENT                       0x1030
#define WPS_PSK_MAX                           0x1031
#define WPS_PUBLIC_KEY                        0x1032
#define WPS_RADIO_ENABLED                     0x1033
#define WPS_REBOOT                            0x1034
#define WPS_REGISTRAR_CURRENT                 0x1035
#define WPS_REGISTRAR_ESTABLISHED             0x1036
#define WPS_REGISTRAR_LIST                    0x1037
#define WPS_REGISTRAR_MAX                     0x1038
#define WPS_REGISTRAR_NONCE                   0x1039
#define WPS_REQUEST_TYPE                      0x103A
#define WPS_RESPONSE_TYPE                     0x103B
#define WPS_RF_BANDS                          0x103C
#define WPS_R_HASH1                           0x103D
#define WPS_R_HASH2                           0x103E
#define WPS_R_SNONCE1                         0x103F
#define WPS_R_SNONCE2                         0x1040
#define WPS_SELECTED_REGISTRAR                0x1041
#define WPS_SERIAL_NUMBER                     0x1042
#define WPS_WIFI_PROTECTED_SETUP_STATE        0x1044
#define WPS_SSID                              0x1045
#define WPS_TOTAL_NETWORKS                    0x1046
#define WPS_UUID_E                            0x1047
#define WPS_UUID_R                            0x1048
#define WPS_VENDOR_EXTENSION                  0x1049
#define WPS_VERSION                           0x104A
#define WPS_X509_CERTIFICATE_REQUEST          0x104B
#define WPS_X509_CERTIFICATE                  0x104C
#define WPS_EAP_IDENTITY                      0x104D
#define WPS_MESSAGE_COUNTER                   0x104E
#define WPS_PUBLIC_KEY_HASH                   0x104F
#define WPS_REKEY_KEY                         0x1050
#define WPS_KEY_LIFETIME                      0x1051
#define WPS_PERMITTED_CONFIG_METHODS          0x1052
#define WPS_SELECTED_REGISTRAR_CONFIG_METHODS 0x1053
#define WPS_PRIMARY_DEVICE_TYPE               0x1054
#define WPS_SECONDARY_DEVICE_TYPE_LIST        0x1055
#define WPS_PORTABLE_DEVICE                   0x1056
#define WPS_AP_SETUP_LOCKED                   0x1057
#define WPS_APPLICATION_EXTENSION             0x1058
#define WPS_EAP_TYPE                          0x1059
#define WPS_INITIALIZATION_VECTOR             0x1060
#define WPS_KEY_PROVIDED_AUTOMATICALLY        0x1061
#define WPS_8021X_ENABLED                     0x1062
#define WPS_APP_SESSION_KEY                   0x1063
#define WPS_WEP_TRANSMIT_KEY                  0x1064

#define WPS_SUPPORTED_VERSION 0x10

/* EAP Op-codes for Wi-Fi Protected Setup */
#define WPS_OPCODE_START     0x01
#define WPS_OPCODE_ACK       0x02
#define WPS_OPCODE_NACK      0x03
#define WPS_OPCODE_MSG       0x04
#define WPS_OPCODE_DONE      0x05
#define WPS_OPCODE_FRAG_ACK  0x06

/* Wi-Fi Protected Setup message types */
#define WPS_M1   0x04
#define WPS_M2   0x05
#define WPS_M2D  0x06
#define WPS_M3   0x07
#define WPS_M4   0x08
#define WPS_M5   0x09
#define WPS_M6   0x0a
#define WPS_M7   0x0b
#define WPS_M8   0x0c
#define WPS_ACK  0x0d
#define WPS_NACK 0x0e
#define WPS_DONE 0x0f

#define WPS_NO_ERROR 0
#define DECRYPTION_CRC_FAILURE 2

typedef struct
{
    CsrUint8 *pStart;
    CsrUint8 *pIndex;
    unsigned int size;
} WpsBuffer;

#define WPS_TL_LENGTH 4
#define WPS_MAC_ADDRESS_LENGTH 6
#define WPS_AUTHENTICATOR_LENGTH 8
#define WPS_NONCE_LENGTH 16
#define WPS_PSK_LENGTH 16
#define WPS_KEY_WRAP_KEY_LENGTH 16
#define WPS_IV_LENGTH 16
#define WPS_SHA256_DIGEST_LENGTH 32
#define WPS_AUTH_KEY_LENGTH 32
#define WPS_DIFFIE_HELLMAN_KEY_LENGTH 192
#define WPS_PUSHBUTTON_PIN "00000000"
#define WPS_WALK_TIME 120

#define WPS_AUTHENTICATION_TYPE_OPEN    0x0001
#define WPS_AUTHENTICATION_TYPE_WPAPSK  0x0002
#define WPS_AUTHENTICATION_TYPE_SHARED  0x0004
#define WPS_AUTHENTICATION_TYPE_WPA     0x0008
#define WPS_AUTHENTICATION_TYPE_WPA2    0x0010
#define WPS_AUTHENTICATION_TYPE_WPA2PSK 0x0020

#define WPS_ENCRYPTION_TYPE_NONE 0x0001
#define WPS_ENCRYPTION_TYPE_WEP  0x0002
#define WPS_ENCRYPTION_TYPE_TKIP 0x0004
#define WPS_ENCRYPTION_TYPE_AES  0x0008

/* Cipher suites ordered in increasing strength */
#define WPS_CIPHER_SUITE_UNKNOWN 0
#define WPS_CIPHER_SUITE_WEP_40  1
#define WPS_CIPHER_SUITE_WEP_104 2
#define WPS_CIPHER_SUITE_TKIP    3
#define WPS_CIPHER_SUITE_CCMP    4

typedef struct
{
    unsigned char bssid[6];
    char ssid[32];
    CsrBool privacy;
    CsrBool wpsEnabled;
    CsrBool wpsActivated;
    CsrBool wpaEnabled;
    CsrBool wpa2Enabled;
    int signal;
    int groupCipherSuite;
    int pairwiseCipherSuite;
} Network;

typedef struct
{
    WpsBuffer in;
    WpsBuffer out;
    WpsBuffer lastTx;
    int txCount;
    WpsBuffer ssid;
    WpsBuffer networkKey;
    CsrUint8 networkKeyIndex;
    unsigned int authenticationType;
    unsigned int encryptionType;
    char pin[9]; /* Only 8 digit PINs supported */
    Network *pCandidate;
    CsrUint8 psk1[WPS_PSK_LENGTH];
    CsrUint8 psk2[WPS_PSK_LENGTH];
    CsrUint8 macAddress[WPS_MAC_ADDRESS_LENGTH];
    CsrUint8 eNonce[WPS_NONCE_LENGTH];
    CsrUint8 rNonce[WPS_NONCE_LENGTH];
    CsrUint8 ePrvKey[WPS_DIFFIE_HELLMAN_KEY_LENGTH];
    CsrUint8 ePubKey[WPS_DIFFIE_HELLMAN_KEY_LENGTH];
    CsrUint8 rPubKey[WPS_DIFFIE_HELLMAN_KEY_LENGTH];
    CsrUint8 eSnonce1[WPS_NONCE_LENGTH];
    CsrUint8 eSnonce2[WPS_NONCE_LENGTH];
    CsrUint8 authKey[WPS_AUTH_KEY_LENGTH];
    CsrUint8 keyWrapKey[WPS_KEY_WRAP_KEY_LENGTH];
    CsrUint8 rHash2[WPS_SHA256_DIGEST_LENGTH];
} WpsContext;

typedef enum
{
	WPS_SCAN_MODE_ACTIVATED,
	WPS_SCAN_MODE_ENABLED
}ScanMode;

void initBuffer( WpsBuffer *pBuf, CsrUint8 *pData, unsigned int size );
void freeBuffer( WpsBuffer *pBuf );

#ifdef __cplusplus
}
#endif

#endif /* WPS_COMMON_H */
