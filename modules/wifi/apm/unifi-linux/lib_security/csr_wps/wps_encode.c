/** @file wps_encode.c
 *
 * Implementation of the Wi-Fi Protected Setup (WPS) encoding.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) encoding
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_encode.c#1 $
 *
 ****************************************************************************/

#include "wps_encode.h"
#include "wps_common.h"
#include "wps_crypto.h"
#include "wps_eap.h"
#include "sme_trace/sme_trace.h"
#include "csr_hmac/csr_hmac.h"
#include "csr_aes/csr_aes.h"


/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void addDataElementHeader( WpsBuffer *pMessage, CsrUint32 type, CsrUint32 length )
{
    /* Attribute Type */
    *pMessage->pIndex++ = (type >> 8) & 0xff;
    *pMessage->pIndex++ = type & 0xff;

    /* Data Length */
    *pMessage->pIndex++ = (length >> 8) & 0xff;
    *pMessage->pIndex++ = length & 0xff;
}

static CsrUint32 addVersion( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_VERSION, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x10; /* Version 1.0 */
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addMessageType( WpsBuffer *pMessage, CsrUint8 messageType )
{
    const CsrUint32 vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MESSAGE_TYPE, vLength );

        /* Value */
        *pMessage->pIndex++ = messageType;
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 add_UUID_E( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 16;
    CsrUint32 i;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_UUID_E, vLength );

        /* Value */
        for( i = 0; i < vLength; i++ )
        {
            *pMessage->pIndex++ = 0x04;
        }
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 add_MAC_Address( CsrWpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MAC_ADDRESS, sizeof( pCtx->securityContext->setupData.localMacAddress ) );

        /* Value */
        CsrMemCpy( pMessage->pIndex, pCtx->securityContext->setupData.localMacAddress, sizeof( pCtx->securityContext->setupData.localMacAddress ) );
        pMessage->pIndex += sizeof( pCtx->securityContext->setupData.localMacAddress );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->securityContext->setupData.localMacAddress );
}

static CsrUint32 addEnrolleeNonce( CsrWpsContext *pCtx, WpsBuffer *pMessage  )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_ENROLLEE_NONCE, sizeof( pCtx->eNonce ) );

        /* Value */
        CsrMemCpy( pMessage->pIndex, pCtx->eNonce, sizeof( pCtx->eNonce ) );
        pMessage->pIndex += sizeof( pCtx->eNonce );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->eNonce );
}

static CsrUint32 addPublicKey( CsrWpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_PUBLIC_KEY, sizeof( pCtx->ePubKey ) );

        /* Value */
        CsrMemCpy( pMessage->pIndex, pCtx->ePubKey, sizeof( pCtx->ePubKey ) );
        pMessage->pIndex += sizeof( pCtx->ePubKey );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->ePubKey );
}

static CsrUint32 addAthenticationTypeFlags( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_AUTHENTICATION_TYPE_FLAGS, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x27; /* WPAPSK | WPA2PSK | Open | Shared */
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addEncryptionTypeFlags( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_ENCRYPTION_TYPE_FLAGS, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x0F; /* AES | TKIP | WEP | None */
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addConnectionTypeFlags( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_CONNECTION_TYPE_FLAGS, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x02; /* IBSS */
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addConfigMethods( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_CONFIG_METHODS, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x88; /* Pushbutton and display */
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addWifiProtectedSetupState( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_WIFI_PROTECTED_SETUP_STATE, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x01; /* Unconfigured */
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addManufacturer( WpsBuffer *pMessage, const char *string )
{
    CsrUint32 vLength = CsrStrLen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MANUFACTURER, vLength );

        /* Value */
        CsrMemCpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addModelName( WpsBuffer *pMessage, const char *string )
{
    CsrUint32 vLength = CsrStrLen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MODEL_NAME, vLength );

        /* Value */
        CsrMemCpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addModelNumber( WpsBuffer *pMessage, const char *string )
{
    CsrUint32 vLength = CsrStrLen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MODEL_NUMBER, vLength );

        /* Value */
        CsrMemCpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addSerialNumber( WpsBuffer *pMessage, const char *string )
{
    CsrUint32 vLength = CsrStrLen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_SERIAL_NUMBER, vLength );

        /* Value */
        CsrMemCpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addPrimaryDeviceType( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 8;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_PRIMARY_DEVICE_TYPE, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x01; /* Category: Computer */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x50;
        *pMessage->pIndex++ = 0xf2;
        *pMessage->pIndex++ = 0x04;
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x01; /* Sub-category: PC */
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addDeviceName( WpsBuffer *pMessage, const char *string )
{
    CsrUint32 vLength = CsrStrLen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_DEVICE_NAME, vLength );

        /* Value */
        CsrMemCpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 add_RF_Bands( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_RF_BANDS, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x01; /* 2.4 GHz */
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addAssociationState( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_ASSOCIATION_STATE, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x00; /* Not Associated */
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addDevicePasswordId( WpsBuffer *pMessage, char *pin )
{
    const CsrUint32 vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_DEVICE_PASSWORD_ID, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x00;
        if( strcmp( pin, WPS_PUSHBUTTON_PIN ) )
        {
            *pMessage->pIndex++ = 0x00; /* default PIN */
        }
        else
        {
            *pMessage->pIndex++ = 0x04; /* pushbutton */
        }
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addConfigurationError( WpsBuffer *pMessage, unsigned char error )
{
    const CsrUint32 vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_CONFIGURATION_ERROR, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = error;
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 add_OS_Version( WpsBuffer *pMessage )
{
    const CsrUint32 vLength = 4;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_OS_VERSION_ID, vLength );

        /* Value */
        *pMessage->pIndex++ = 0x80;
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x00;
    }

    return WPS_TL_LENGTH + vLength;
}

static CsrUint32 addRegistrarNonce( CsrWpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_REGISTRAR_NONCE, sizeof( pCtx->rNonce ) );

        /* Value */
        CsrMemCpy( pMessage->pIndex, pCtx->rNonce, sizeof( pCtx->rNonce ) );
        pMessage->pIndex += sizeof( pCtx->rNonce );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->rNonce );
}

static CsrUint32 addEncryptedSettings( WpsBuffer *pMessage, WpsBuffer *encryptedSettings )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_ENCRYPTED_SETTINGS, encryptedSettings->size );

        /* Value */
        CsrMemCpy( pMessage->pIndex, encryptedSettings->pStart, encryptedSettings->size );
        pMessage->pIndex += encryptedSettings->size;
    }

    return WPS_TL_LENGTH + encryptedSettings->size;
}

static CsrUint32 addEnrolleeSecretNonce1( CsrWpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Enrollee Secret Nonce 1", pCtx->eSnonce1, sizeof( pCtx->eSnonce1 ) ));

        addDataElementHeader( pMessage, WPS_E_SNONCE1, sizeof( pCtx->eSnonce1 )  );

        /* Value */
        CsrMemCpy( pMessage->pIndex, pCtx->eSnonce1, sizeof( pCtx->eSnonce1 ) );
        pMessage->pIndex += sizeof( pCtx->eSnonce1 );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->eSnonce1 );
}

static CsrUint32 addEnrolleeSecretNonce2( CsrWpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Enrollee Secret Nonce 2", pCtx->eSnonce2, sizeof( pCtx->eSnonce2 ) ));

        addDataElementHeader( pMessage, WPS_E_SNONCE2, sizeof( pCtx->eSnonce2 )  );

        /* Value */
        CsrMemCpy( pMessage->pIndex, pCtx->eSnonce2, sizeof( pCtx->eSnonce2 ) );
        pMessage->pIndex += sizeof( pCtx->eSnonce2 );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->eSnonce2 );
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

void sendM1(CsrWpsContext *pCtx)
{
    WpsBuffer *pMessage[2];
    CsrInt32 i;
    CsrUint32 length;

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Enrollee MAC", pCtx->securityContext->setupData.localMacAddress, sizeof(pCtx->securityContext->setupData.localMacAddress)));

    /* initialise Enrollee Nonce */
    CsrWifiSecurityRandom(pCtx->eNonce, sizeof(pCtx->eNonce));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Enrollee Nonce", pCtx->eNonce, sizeof(pCtx->eNonce)));

    initKeyPair(pCtx);

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addVersion(pMessage[i]);
        length += addMessageType(pMessage[i], WPS_M1);
        length += add_UUID_E(pMessage[i]);
        length += add_MAC_Address(pCtx, pMessage[i]);
        length += addEnrolleeNonce(pCtx, pMessage[i]);
        length += addPublicKey(pCtx, pMessage[i]);
        length += addAthenticationTypeFlags(pMessage[i]);
        length += addEncryptionTypeFlags(pMessage[i]);
        length += addConnectionTypeFlags(pMessage[i]);
        length += addConfigMethods(pMessage[i]);
        length += addWifiProtectedSetupState(pMessage[i]);
        length += addManufacturer(pMessage[i], "CSR");
        length += addModelName(pMessage[i], "Unifi");
        length += addModelNumber(pMessage[i], "1234");
        length += addSerialNumber(pMessage[i], "5678");
        length += addPrimaryDeviceType(pMessage[i]);
        length += addDeviceName(pMessage[i], "Unifi");
        length += add_RF_Bands(pMessage[i]);
        length += addAssociationState(pMessage[i]);
        length += addDevicePasswordId(pMessage[i], (char*)pCtx->securityContext->setupData.pin);
        length += addConfigurationError(pMessage[i], WPS_NO_ERROR);
        length += add_OS_Version(pMessage[i]);

        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if(i == 0)
        {
            initBuffer(pMessage[1], (CsrUint8 *)CsrPmalloc(length), length);
        }
    }

    sme_trace_info((TR_SECURITY_LIB, "Sending M1"));
    SendResponse(pCtx->eapId, WPS_OPCODE_MSG, pCtx->securityContext->setupData.bssid,
                 pCtx->securityContext->setupData.localMacAddress, pCtx);

    pCtx->state = WPS_STATE_WAIT_M2;
}

void sendM3(CsrWpsContext *pCtx)
{
    CsrUint32 length;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    initEhash(pCtx);
    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    length =  addVersion(NULL);
    length += addMessageType(NULL, WPS_M3);
    length += addRegistrarNonce(pCtx, NULL);
    length +=  2 * (WPS_TL_LENGTH + WPS_SHA256_DIGEST_LENGTH);
    /* The length is now known to initialise the buffer to build the message */
    /* Need to allocate an extra 12 bytes for Authenticator to be added */
    initBuffer(&pCtx->out, (CsrUint8 *)CsrPmalloc(length + 12), length);

    length =  addVersion(&pCtx->out);
    length += addMessageType(&pCtx->out, WPS_M3);
    length += addRegistrarNonce( pCtx, &pCtx->out);

    addDataElementHeader(&pCtx->out, WPS_E_HASH1, WPS_SHA256_DIGEST_LENGTH);

    /* E-Hash1 is the HMAC-SHA256 of the concatenation of E-S1 (Enrollee Secret
       Nonce 1), PSK1, the Enrollee Public Key and the Registrar Public Key,
       keyed with AuthKey. */

    /* Value */

    /* The E-Hash is the HMAC-SHA256 of the concatenation of a secret
       nonce, PSK, the Enrollee Public Key and the Registrar Public Key,
       keyed with AuthKey. */

    pCtx->message = (CsrUint8 *)CsrPmalloc(2 * WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey) + sizeof(pCtx->rPubKey));
    CsrMemCpy(pCtx->message, pCtx->eSnonce1, WPS_NONCE_LENGTH);
    CsrMemCpy(pCtx->message + WPS_NONCE_LENGTH, pCtx->psk1, WPS_NONCE_LENGTH);
    CsrMemCpy(pCtx->message + 2 * WPS_NONCE_LENGTH, pCtx->ePubKey, sizeof(pCtx->ePubKey));
    CsrMemCpy(pCtx->message + 2 * WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey), pCtx->rPubKey, sizeof(pCtx->rPubKey));

    CsrCryptoHmacSha256(pCtx->authKey, 32,
                        pCtx->message, 2 * 16 + sizeof(pCtx->ePubKey) + sizeof(pCtx->rPubKey),
                        pCtx->out.pIndex);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS E-HASH1", pCtx->out.pIndex, WPS_SHA256_DIGEST_LENGTH));
    pCtx->out.pIndex += WPS_SHA256_DIGEST_LENGTH;

    addDataElementHeader(&pCtx->out, WPS_E_HASH2, WPS_SHA256_DIGEST_LENGTH);

    /* E-Hash1 is the HMAC-SHA256 of the concatenation of E-S1 (Enrollee Secret
       Nonce 1), PSK1, the Enrollee Public Key and the Registrar Public Key,
       keyed with AuthKey. */

    /* Value */

    /* The E-Hash is the HMAC-SHA256 of the concatenation of a secret
       nonce, PSK, the Enrollee Public Key and the Registrar Public Key,
       keyed with AuthKey. */

    CsrMemCpy(pCtx->message, pCtx->eSnonce2, WPS_NONCE_LENGTH);
    CsrMemCpy(pCtx->message + WPS_NONCE_LENGTH, pCtx->psk2, WPS_NONCE_LENGTH);

    CsrCryptoHmacSha256(pCtx->authKey, 32,
                        pCtx->message, 2 * 16 + sizeof(pCtx->ePubKey) + sizeof(pCtx->rPubKey),
                        pCtx->out.pIndex);
    CsrPfree(pCtx->message);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS E-HASH2", pCtx->out.pIndex, WPS_SHA256_DIGEST_LENGTH));
    pCtx->out.pIndex += WPS_SHA256_DIGEST_LENGTH;

    /* Authenticator is computed over M2 and M3 */
    addDataElementHeader( &pCtx->out, WPS_AUTHENTICATOR, WPS_AUTHENTICATOR_LENGTH);

    /* Value */
    pCtx->message = (CsrUint8 *) CsrPmalloc(pCtx->in.size + pCtx->out.size);
    CsrMemCpy(pCtx->message, pCtx->in.pStart, pCtx->in.size);
    CsrMemCpy(pCtx->message + pCtx->in.size, pCtx->out.pStart, pCtx->out.size);

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->message, pCtx->in.size + pCtx->out.size,
                        digest);

    CsrPfree(pCtx->message);
    CsrMemCpy(pCtx->out.pIndex, digest, WPS_AUTHENTICATOR_LENGTH);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", pCtx->out.pIndex, WPS_AUTHENTICATOR_LENGTH));

    pCtx->out.pIndex += WPS_AUTHENTICATOR_LENGTH;

    pCtx->out.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    sme_trace_info((TR_SECURITY_LIB, "Sending M3"));

    SendResponse(pCtx->eapId, WPS_OPCODE_MSG, pCtx->securityContext->setupData.bssid,
                 pCtx->securityContext->setupData.localMacAddress, pCtx );

    pCtx->state = WPS_STATE_WAIT_M4;

    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}

void sendM5(CsrWpsContext *pCtx)
{
    CsrUint8 iv[WPS_IV_LENGTH];
    CsrInt32 i;
    CsrUint32 length;
    WpsBuffer *pMessage[2];
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    encodeDecryptedSettingsM5(pCtx);
    /* Encrypt decrypted settings */
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Decrypted Settings", pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size));

    /* Create random IV */
    CsrWifiSecurityRandom(iv, sizeof(iv));

    /* TODO: remove hardcoding */
    initBuffer(&pCtx->encryptedSettings, (CsrUint8*)CsrPmalloc(1000), 1000);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings IV", iv, sizeof(iv)));
    CsrMemCpy(pCtx->encryptedSettings.pStart, iv, sizeof(iv));

    CsrCryptoAes128CbcEncrypt(pCtx->keyWrapKey, iv,
                              pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size,
                              pCtx->encryptedSettings.pStart + WPS_IV_LENGTH,
                              &length,
                              PAD_MODE_PKCS7);
    pCtx->encryptedSettings.size = WPS_IV_LENGTH + length;

    sme_trace_info((TR_SECURITY_LIB, "WPS Encrypted Settings size = %d", pCtx->encryptedSettings.size));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings", pCtx->encryptedSettings.pStart, pCtx->encryptedSettings.size));

    freeBuffer(&pCtx->decryptedSettings);

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for(i = 0; i < 2; i++)
    {
        length =  addVersion(pMessage[i]);
        length += addMessageType(pMessage[i], WPS_M5);
        length += addRegistrarNonce(pCtx, pMessage[i]);
        length += addEncryptedSettings(pMessage[i], &pCtx->encryptedSettings);

        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if(i == 0)
        {
            /* Need to allocate an extra 12 bytes for Authenticator to be added */
            initBuffer(pMessage[1], (CsrUint8 *)CsrPmalloc(length + 12), length);
        }
    }

    freeBuffer(&pCtx->encryptedSettings);

    /* Authenticator is computed over M4 and M5 */
    addDataElementHeader(&pCtx->out, WPS_AUTHENTICATOR, WPS_AUTHENTICATOR_LENGTH);

    /* Value */
    pCtx->message = (CsrUint8 *)CsrPmalloc(pCtx->in.size + pCtx->out.size);
    CsrMemCpy(pCtx->message, pCtx->in.pStart, pCtx->in.size);
    CsrMemCpy(pCtx->message + pCtx->in.size, pCtx->out.pStart, pCtx->out.size);

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->message, pCtx->in.size + pCtx->out.size,
                        digest);
    CsrPfree(pCtx->message);

    CsrMemCpy(pCtx->out.pIndex, digest, WPS_AUTHENTICATOR_LENGTH);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", pCtx->out.pIndex, WPS_AUTHENTICATOR_LENGTH));

    pCtx->out.pIndex += WPS_AUTHENTICATOR_LENGTH;

    pCtx->out.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    sme_trace_info((TR_SECURITY_LIB, "Sending M5"));
    SendResponse(pCtx->eapId, WPS_OPCODE_MSG, pCtx->securityContext->setupData.bssid,
                 pCtx->securityContext->setupData.localMacAddress, pCtx);
    pCtx->state = WPS_STATE_WAIT_M6;

    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}

void encodeDecryptedSettingsM5(CsrWpsContext *pCtx)
{
    CsrUint32 length;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    /* First with NULL to compute length of buffer required. Second to build message */
    length =  addEnrolleeSecretNonce1(pCtx, NULL);
    initBuffer(&pCtx->decryptedSettings, (CsrUint8 *)CsrPmalloc(length + 12), length);
    length =  addEnrolleeSecretNonce1(pCtx, &pCtx->decryptedSettings);

    /* Finally, add the Key Wrap Authenticator to Encrypted Settings Data */
    addDataElementHeader(&pCtx->decryptedSettings, WPS_KEY_WRAP_AUTHENTICATOR, WPS_AUTHENTICATOR_LENGTH);

    /* Value */
    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size,
                        digest);
    CsrMemCpy(pCtx->decryptedSettings.pIndex, digest, WPS_AUTHENTICATOR_LENGTH);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", pCtx->decryptedSettings.pIndex, WPS_AUTHENTICATOR_LENGTH));

    pCtx->decryptedSettings.pIndex += WPS_AUTHENTICATOR_LENGTH;

    pCtx->decryptedSettings.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;
}

void sendM7(CsrWpsContext *pCtx)
{
    CsrUint8 iv[WPS_IV_LENGTH];
    WpsBuffer *pMessage[2];
    CsrInt32 i;
    CsrUint32 length;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    encodeDecryptedSettingsM7(pCtx);

    /* Encrypt decrypted settings */
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Decrypted Settings", pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size));

    /* TODO: remove hardcoding */
    initBuffer(&pCtx->encryptedSettings, (CsrUint8*)CsrPmalloc(1000), 1000);

    /* Create random IV */
    CsrWifiSecurityRandom(pCtx->encryptedSettings.pStart, WPS_IV_LENGTH);

    CsrMemCpy(iv, pCtx->encryptedSettings.pStart, WPS_IV_LENGTH);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings IV", pCtx->encryptedSettings.pStart, WPS_IV_LENGTH));

    CsrCryptoAes128CbcEncrypt(pCtx->keyWrapKey, iv,
                              pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size,
                              pCtx->encryptedSettings.pStart + WPS_IV_LENGTH,
                              &length,
                              PAD_MODE_PKCS7);
    pCtx->encryptedSettings.size = WPS_IV_LENGTH + length;

    sme_trace_info((TR_SECURITY_LIB, "WPS Encrypted Settings size = %d", pCtx->encryptedSettings.size));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings", pCtx->encryptedSettings.pStart, pCtx->encryptedSettings.size));

    freeBuffer(&pCtx->decryptedSettings);

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for(i = 0; i < 2; i++)
    {
        length =  addVersion(pMessage[i]);
        length += addMessageType(pMessage[i], WPS_M7);
        length += addRegistrarNonce(pCtx, pMessage[i]);
        length += addEncryptedSettings(pMessage[i], &pCtx->encryptedSettings);

        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if(i == 0)
        {
            /* Need to allocate an extra 12 bytes for Authenticator to be added */
            initBuffer(pMessage[1], (CsrUint8 *)CsrPmalloc(length + 12), length);
        }
    }

    freeBuffer(&pCtx->encryptedSettings);

    /* Authenticator is computed over M6 and M7 */
    addDataElementHeader(&pCtx->out, WPS_AUTHENTICATOR, WPS_AUTHENTICATOR_LENGTH);

    /* Value */
    pCtx->message = (CsrUint8 *)CsrPmalloc(pCtx->in.size + pCtx->out.size);
    CsrMemCpy(pCtx->message, pCtx->in.pStart, pCtx->in.size);
    CsrMemCpy(pCtx->message + pCtx->in.size, pCtx->out.pStart, pCtx->out.size);

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->message, pCtx->in.size + pCtx->out.size,
                        digest);
    CsrPfree(pCtx->message);

    CsrMemCpy(pCtx->out.pIndex, digest, WPS_AUTHENTICATOR_LENGTH);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", pCtx->out.pIndex, WPS_AUTHENTICATOR_LENGTH));

    pCtx->out.pIndex += WPS_AUTHENTICATOR_LENGTH;

    pCtx->out.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    sme_trace_info((TR_SECURITY_LIB, "Sending M7"));
    SendResponse(pCtx->eapId, WPS_OPCODE_MSG, pCtx->securityContext->setupData.bssid,
                 pCtx->securityContext->setupData.localMacAddress, pCtx );
    pCtx->state = WPS_STATE_WAIT_M8;

    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}

void encodeDecryptedSettingsM7(CsrWpsContext *pCtx)
{
    CsrUint32 length;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    /* Find the length */
    length =  addEnrolleeSecretNonce2(pCtx, NULL);
    /* Need to allocate an extra 12 bytes for Key Wrap Authenticator to be added */
    initBuffer(&pCtx->decryptedSettings, (CsrUint8 *)CsrPmalloc(length + 12), length);
    length =  addEnrolleeSecretNonce2(pCtx, &pCtx->decryptedSettings);

    /* Finally, add the Key Wrap Authenticator to Decrypted Settings Data */
    addDataElementHeader(&pCtx->decryptedSettings, WPS_KEY_WRAP_AUTHENTICATOR, WPS_AUTHENTICATOR_LENGTH);

    /* Value */
    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size,
                        digest);
    CsrMemCpy(pCtx->decryptedSettings.pIndex, digest, WPS_AUTHENTICATOR_LENGTH);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", pCtx->decryptedSettings.pIndex, WPS_AUTHENTICATOR_LENGTH));

    pCtx->decryptedSettings.pIndex += WPS_AUTHENTICATOR_LENGTH;

    pCtx->decryptedSettings.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;
}

void sendDone(CsrWpsContext *pCtx)
{
    WpsBuffer *pMessage[2];
    CsrInt32 i;
    CsrUint32 length;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for(i = 0; i < 2; i++)
    {
        length =  addVersion(pMessage[i]);
        length += addMessageType(pMessage[i], WPS_DONE);
        length += addEnrolleeNonce(pCtx, pMessage[i]);
        length += addRegistrarNonce( pCtx, pMessage[i] );

        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if(i == 0)
        {
            /* No additional space required for Authenticator */
            initBuffer(pMessage[1], (CsrUint8 *)CsrPmalloc(length), length);
        }
    }

    sme_trace_info((TR_SECURITY_LIB, "WSC DONE length = %d", pCtx->out.size));

    sme_trace_info((TR_SECURITY_LIB, "Sending Done"));
    SendResponse(pCtx->eapId, WPS_OPCODE_DONE, pCtx->securityContext->setupData.bssid,
                 pCtx->securityContext->setupData.localMacAddress, pCtx);

    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}

void sendAck(CsrWpsContext *pCtx)
{
    WpsBuffer *pMessage[2];
    CsrInt32 i;
    CsrUint32 length;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for(i = 0; i < 2; i++)
    {
        length =  addVersion(pMessage[i]);
        length += addMessageType(pMessage[i], WPS_ACK);
        length += addEnrolleeNonce(pCtx, pMessage[i]);
        length += addRegistrarNonce(pCtx, pMessage[i]);

        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if(i == 0)
        {
            /* No additional space required for Authenticator */
            initBuffer(pMessage[1], (CsrUint8 *)CsrPmalloc(length), length);
        }
    }

    sme_trace_info((TR_SECURITY_LIB, "WSC ACK length = %d", pCtx->out.size));

    sme_trace_info((TR_SECURITY_LIB, "Sending Ack"));
    SendResponse(pCtx->eapId, WPS_OPCODE_ACK, pCtx->securityContext->setupData.bssid,
                 pCtx->securityContext->setupData.localMacAddress, pCtx);
}

void sendNack(CsrWpsContext *pCtx, unsigned char error)
{
    WpsBuffer *pMessage[2];
    CsrInt32 i;
    CsrUint32 length;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for(i = 0; i < 2; i++)
    {
        length =  addVersion(pMessage[i]);
        length += addMessageType(pMessage[i], WPS_NACK);
        length += addEnrolleeNonce(pCtx, pMessage[i]);
        length += addRegistrarNonce( pCtx, pMessage[i] );
        length += addConfigurationError(pMessage[i], error);

        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if(i == 0)
        {
            /* No additional space required for Authenticator */
            initBuffer(pMessage[1], (CsrUint8 *)CsrPmalloc(length), length);
        }
    }

    sme_trace_info((TR_SECURITY_LIB, "WSC NACK length = %d", pCtx->out.size));

    sme_trace_info((TR_SECURITY_LIB, "Sending Nack"));
    SendResponse(pCtx->eapId, WPS_OPCODE_NACK, pCtx->securityContext->setupData.bssid,
                 pCtx->securityContext->setupData.localMacAddress, pCtx);

    CsrWifiSecurityPacketProcessingDone(pCtx->securityContext);
}
