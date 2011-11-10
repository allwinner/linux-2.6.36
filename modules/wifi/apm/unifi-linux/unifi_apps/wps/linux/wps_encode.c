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
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_encode.c#1 $
 *
 ****************************************************************************/

#include "wps_encode.h"
#include "wps_common.h"
#include "wps_crypto.h"
#include "wps_debug.h"

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static void addDataElementHeader( WpsBuffer *pMessage, unsigned int type, unsigned int length )
{
    /* Attribute Type */
    *pMessage->pIndex++ = (type >> 8) & 0xff;
    *pMessage->pIndex++ = type & 0xff;

    /* Data Length */
    *pMessage->pIndex++ = (length >> 8) & 0xff;
    *pMessage->pIndex++ = length & 0xff;
}

static unsigned int addVersion( WpsBuffer *pMessage )
{
const unsigned int vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_VERSION, vLength ); 

        /* Value */
        *pMessage->pIndex++ = 0x10; /* Version 1.0 */
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addMessageType( WpsBuffer *pMessage, CsrUint8 messageType )
{
const unsigned int vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MESSAGE_TYPE, vLength ); 

        /* Value */
        *pMessage->pIndex++ = messageType;
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int add_UUID_E( WpsBuffer *pMessage )
{
const unsigned int vLength = 16;
unsigned int i;

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

static unsigned int add_MAC_Address( WpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MAC_ADDRESS, sizeof( pCtx->macAddress ) ); 

        /* Value */
        memcpy( pMessage->pIndex, pCtx->macAddress, sizeof( pCtx->macAddress ) );
        pMessage->pIndex += sizeof( pCtx->macAddress );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->macAddress );
}

static unsigned int addEnrolleeNonce( WpsContext *pCtx, WpsBuffer *pMessage  )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_ENROLLEE_NONCE, sizeof( pCtx->eNonce ) ); 

        /* Value */
        memcpy( pMessage->pIndex, pCtx->eNonce, sizeof( pCtx->eNonce ) );
        pMessage->pIndex += sizeof( pCtx->eNonce );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->eNonce );
}

static unsigned int addPublicKey( WpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_PUBLIC_KEY, sizeof( pCtx->ePubKey ) ); 

        /* Value */
        memcpy( pMessage->pIndex, pCtx->ePubKey, sizeof( pCtx->ePubKey ) );
        pMessage->pIndex += sizeof( pCtx->ePubKey );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->ePubKey );
}

static unsigned int addAthenticationTypeFlags( WpsBuffer *pMessage )
{
const unsigned int vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_AUTHENTICATION_TYPE_FLAGS, vLength ); 
    
        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x27; /* WPAPSK | WPA2PSK | Open | Shared */
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addEncryptionTypeFlags( WpsBuffer *pMessage )
{
const unsigned int vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_ENCRYPTION_TYPE_FLAGS, vLength ); 
    
        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x0F; /* AES | TKIP | WEP | None */
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addConnectionTypeFlags( WpsBuffer *pMessage )
{
const unsigned int vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_CONNECTION_TYPE_FLAGS, vLength ); 
    
        /* Value */
        *pMessage->pIndex++ = 0x02; /* IBSS */
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addConfigMethods( WpsBuffer *pMessage )
{
const unsigned int vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_CONFIG_METHODS, vLength ); 
    
        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x88; /* Pushbutton and display */
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addWifiProtectedSetupState( WpsBuffer *pMessage )
{
const unsigned int vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_WIFI_PROTECTED_SETUP_STATE, vLength ); 
    
        /* Value */
        *pMessage->pIndex++ = 0x01; /* Unconfigured */
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addManufacturer( WpsBuffer *pMessage, const char *string )
{
unsigned int vLength = strlen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MANUFACTURER, vLength ); 
    
        /* Value */
        memcpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addModelName( WpsBuffer *pMessage, const char *string )
{
unsigned int vLength = strlen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MODEL_NAME, vLength ); 
    
        /* Value */
        memcpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addModelNumber( WpsBuffer *pMessage, const char *string )
{
unsigned int vLength = strlen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_MODEL_NUMBER, vLength ); 
    
        /* Value */
        memcpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addSerialNumber( WpsBuffer *pMessage, const char *string )
{
unsigned int vLength = strlen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_SERIAL_NUMBER, vLength ); 
    
        /* Value */
        memcpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addPrimaryDeviceType( WpsBuffer *pMessage )
{
const unsigned int vLength = 8;

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

static unsigned int addDeviceName( WpsBuffer *pMessage, const char *string )
{
unsigned int vLength = strlen( string );

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_DEVICE_NAME, vLength ); 
    
        /* Value */
        memcpy( pMessage->pIndex, string, vLength );
        pMessage->pIndex += vLength;
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int add_RF_Bands( WpsBuffer *pMessage )
{
const unsigned int vLength = 1;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_RF_BANDS, vLength ); 
    
        /* Value */
        *pMessage->pIndex++ = 0x01; /* 2.4 GHz */
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addAssociationState( WpsBuffer *pMessage )
{
const unsigned int vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_ASSOCIATION_STATE, vLength ); 
       
        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = 0x00; /* Not Associated */
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int addDevicePasswordId( WpsBuffer *pMessage, char *pin )
{
const unsigned int vLength = 2;

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

static unsigned int addConfigurationError( WpsBuffer *pMessage, unsigned char error )
{
const unsigned int vLength = 2;

    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_CONFIGURATION_ERROR, vLength ); 
    
        /* Value */
        *pMessage->pIndex++ = 0x00;
        *pMessage->pIndex++ = error;
    }

    return WPS_TL_LENGTH + vLength;
}

static unsigned int add_OS_Version( WpsBuffer *pMessage )
{
const unsigned int vLength = 4;

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

static unsigned int addRegistrarNonce( WpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_REGISTRAR_NONCE, sizeof( pCtx->rNonce ) ); 

        /* Value */
        memcpy( pMessage->pIndex, pCtx->rNonce, sizeof( pCtx->rNonce ) );
        pMessage->pIndex += sizeof( pCtx->rNonce );
    }

    return WPS_TL_LENGTH + sizeof( pCtx->rNonce );
}

static unsigned int addEHash1( WpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_E_HASH1, WPS_SHA256_DIGEST_LENGTH ); 

        /* E-Hash1 is the HMAC-SHA256 of the concatenation of E-S1 (Enrollee Secret
           Nonce 1), PSK1, the Enrollee Public Key and the Registrar Public Key,
           keyed with AuthKey. */

        /* Value */
        computeEHash( pCtx, pCtx->eSnonce1, pCtx->psk1, pMessage->pIndex );
        WPS_DBG_PRINTHEX(( "WPS E-HASH1", pMessage->pIndex, WPS_SHA256_DIGEST_LENGTH ));
        pMessage->pIndex += WPS_SHA256_DIGEST_LENGTH;
    }

    return WPS_TL_LENGTH + WPS_SHA256_DIGEST_LENGTH;
}

static unsigned int addEHash2( WpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_E_HASH2, WPS_SHA256_DIGEST_LENGTH ); 

        /* E-Hash1 is the HMAC-SHA256 of the concatenation of E-S1 (Enrollee Secret
           Nonce 1), PSK1, the Enrollee Public Key and the Registrar Public Key,
           keyed with AuthKey. */

        /* Value */
        computeEHash( pCtx, pCtx->eSnonce2, pCtx->psk2, pMessage->pIndex );
        WPS_DBG_PRINTHEX(( "WPS E-HASH2", pMessage->pIndex, WPS_SHA256_DIGEST_LENGTH ));
        pMessage->pIndex += WPS_SHA256_DIGEST_LENGTH;
    }

    return WPS_TL_LENGTH + WPS_SHA256_DIGEST_LENGTH;
}

static unsigned int addAuthenticator( WpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_AUTHENTICATOR, WPS_AUTHENTICATOR_LENGTH ); 

        /* Value */
        computeAuthenticator( pCtx, &pCtx->in, &pCtx->out, pMessage->pIndex );
        pMessage->pIndex += WPS_AUTHENTICATOR_LENGTH;
    }

    return WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;
}

static unsigned int addKeyWrapAuthenticator( WpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_KEY_WRAP_AUTHENTICATOR, WPS_AUTHENTICATOR_LENGTH ); 

        /* Value */
        computeKeyWrapAuthenticator( pCtx, pMessage, pMessage->pIndex );
        pMessage->pIndex += WPS_AUTHENTICATOR_LENGTH;
    }

    return WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;
}

static unsigned int addEncryptedSettings( WpsBuffer *pMessage, WpsBuffer *encryptedSettings )
{
    if( pMessage )
    {
        addDataElementHeader( pMessage, WPS_ENCRYPTED_SETTINGS, encryptedSettings->size ); 

        /* Value */
        memcpy( pMessage->pIndex, encryptedSettings->pStart, encryptedSettings->size );
        pMessage->pIndex += encryptedSettings->size;
    }

    return WPS_TL_LENGTH + encryptedSettings->size;
}

static unsigned int addEnrolleeSecretNonce1( WpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        WPS_DBG_PRINTHEX(( "WPS Enrollee Secret Nonce 1", pCtx->eSnonce1, sizeof( pCtx->eSnonce1 ) ));
        
        addDataElementHeader( pMessage, WPS_E_SNONCE1, sizeof( pCtx->eSnonce1 )  );
        
        /* Value */
        memcpy( pMessage->pIndex, pCtx->eSnonce1, sizeof( pCtx->eSnonce1 ) );
        pMessage->pIndex += sizeof( pCtx->eSnonce1 );
    }
    
    return WPS_TL_LENGTH + sizeof( pCtx->eSnonce1 );
}

static unsigned int addEnrolleeSecretNonce2( WpsContext *pCtx, WpsBuffer *pMessage )
{
    if( pMessage )
    {
        WPS_DBG_PRINTHEX(( "WPS Enrollee Secret Nonce 2", pCtx->eSnonce2, sizeof( pCtx->eSnonce2 ) ));
        
        addDataElementHeader( pMessage, WPS_E_SNONCE2, sizeof( pCtx->eSnonce2 )  );
        
        /* Value */
        memcpy( pMessage->pIndex, pCtx->eSnonce2, sizeof( pCtx->eSnonce2 ) );
        pMessage->pIndex += sizeof( pCtx->eSnonce2 );
    }
    
    return WPS_TL_LENGTH + sizeof( pCtx->eSnonce2 );
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

void encodeM1( WpsContext *pCtx )
{
WpsBuffer *pMessage[2];
int i;
unsigned int length;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    WPS_DBG_PRINTHEX(( "WPS Enrollee MAC", pCtx->macAddress, sizeof( pCtx->macAddress ) ));
    initEnrolleeNonce( pCtx );
    initKeyPair( pCtx );

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addVersion( pMessage[i] );
        length += addMessageType( pMessage[i], WPS_M1 );
        length += add_UUID_E( pMessage[i] );
        length += add_MAC_Address( pCtx, pMessage[i] );
        length += addEnrolleeNonce( pCtx, pMessage[i] );
        length += addPublicKey( pCtx, pMessage[i] );
        length += addAthenticationTypeFlags( pMessage[i] );
        length += addEncryptionTypeFlags( pMessage[i] );
        length += addConnectionTypeFlags( pMessage[i] );
        length += addConfigMethods( pMessage[i] );
        length += addWifiProtectedSetupState( pMessage[i] );
        length += addManufacturer( pMessage[i], "CSR" );
        length += addModelName( pMessage[i], "Unifi" );
        length += addModelNumber( pMessage[i], "1234" );
        length += addSerialNumber( pMessage[i], "5678" );
        length += addPrimaryDeviceType( pMessage[i] );
        length += addDeviceName( pMessage[i], "Unifi" );
        length += add_RF_Bands( pMessage[i] );
        length += addAssociationState( pMessage[i] );
        length += addDevicePasswordId( pMessage[i], pCtx->pin );
        length += addConfigurationError( pMessage[i], WPS_NO_ERROR );
        length += add_OS_Version( pMessage[i] );

        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if( i == 0 )
        {
            initBuffer( pMessage[1], (CsrUint8 *)malloc(length), length );
        }
    }
}

void encodeM3( WpsContext *pCtx )
{
WpsBuffer *pMessage[2];
int i;
unsigned int length;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    initEhash( pCtx );

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addVersion( pMessage[i] );
        length += addMessageType( pMessage[i], WPS_M3 );
        length += addRegistrarNonce( pCtx, pMessage[i] );
        length += addEHash1( pCtx, pMessage[i] );
        length += addEHash2( pCtx, pMessage[i] );
 
        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if( i == 0 )
        {
            /* Need to allocate an extra 12 bytes for Authenticator to be added */
            initBuffer( pMessage[1], (CsrUint8 *)malloc(length + 12), length );
        }
    }

    /* Authenticator is computed over M2 and M3 */
    pCtx->out.size += addAuthenticator( pCtx, &pCtx->out );
}

void encodeM5( WpsContext *pCtx )
{
WpsBuffer *pMessage[2];
int i;
unsigned int length;
WpsBuffer decryptedSettings;
WpsBuffer encryptedSettings;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    encodeDecryptedSettingsM5( pCtx, &decryptedSettings );
    encryptDecryptedSettings( pCtx, &decryptedSettings, &encryptedSettings );

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addVersion( pMessage[i] );
        length += addMessageType( pMessage[i], WPS_M5 );
        length += addRegistrarNonce( pCtx, pMessage[i] );
        length += addEncryptedSettings( pMessage[i], &encryptedSettings );
 
        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if( i == 0 )
        {
            /* Need to allocate an extra 12 bytes for Authenticator to be added */
            initBuffer( pMessage[1], (CsrUint8 *)malloc(length + 12), length );
        }
    }

    /* Authenticator is computed over M4 and M5 */
    pCtx->out.size += addAuthenticator( pCtx, &pCtx->out );
}

void encodeDecryptedSettingsM5( WpsContext *pCtx, WpsBuffer *pDecryptedSettings )
{
WpsBuffer *pMessage[2];
int i;
unsigned int length;

    pMessage[0] = NULL;
    pMessage[1] = pDecryptedSettings;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addEnrolleeSecretNonce1( pCtx, pMessage[i] );
 
        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if( i == 0 )
        {
            /* Need to allocate an extra 12 bytes for Key Wrap Authenticator to be added */
            initBuffer( pMessage[1], (CsrUint8 *)malloc(length + 12), length );
        }
    }
 
    /* Finally, add the Key Wrap Authenticator to Encrypted Settings Data */
    pMessage[1]->size += addKeyWrapAuthenticator( pCtx, pMessage[1] );
}

void encodeM7( WpsContext *pCtx )
{
WpsBuffer *pMessage[2];
int i;
unsigned int length;
WpsBuffer decryptedSettings;
WpsBuffer encryptedSettings;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    encodeDecryptedSettingsM7( pCtx, &decryptedSettings );
    encryptDecryptedSettings( pCtx, &decryptedSettings, &encryptedSettings );
        
    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addVersion( pMessage[i] );
        length += addMessageType( pMessage[i], WPS_M7 );
        length += addRegistrarNonce( pCtx, pMessage[i] );
        length += addEncryptedSettings( pMessage[i], &encryptedSettings );
 
        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if( i == 0 )
        {
            /* Need to allocate an extra 12 bytes for Authenticator to be added */
            initBuffer( pMessage[1], (CsrUint8 *)malloc(length + 12), length );
        }
    }

    /* Authenticator is computed over M6 and M7 */
    pCtx->out.size += addAuthenticator( pCtx, &pCtx->out );
}

void encodeDecryptedSettingsM7( WpsContext *pCtx, WpsBuffer *pDecryptedSettings )
{
WpsBuffer *pMessage[2];
int i;
unsigned int length;

    pMessage[0] = NULL;
    pMessage[1] = pDecryptedSettings;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addEnrolleeSecretNonce2( pCtx, pMessage[i] );
 
        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if( i == 0 )
        {
            /* Need to allocate an extra 12 bytes for Key Wrap Authenticator to be added */
            initBuffer( pDecryptedSettings, (CsrUint8 *)malloc(length + 12), length );
        }
    }

    /* Finally, add the Key Wrap Authenticator to Decrypted Settings Data */
    pDecryptedSettings->size += addKeyWrapAuthenticator( pCtx, pDecryptedSettings );
}

void encodeDone( WpsContext *pCtx )
{
WpsBuffer *pMessage[2];
int i;
unsigned int length;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addVersion( pMessage[i] );
        length += addMessageType( pMessage[i], WPS_DONE );
        length += addEnrolleeNonce( pCtx, pMessage[i] );
        length += addRegistrarNonce( pCtx, pMessage[i] );
 
        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if( i == 0 )
        {
            /* No additional space required for Authenticator */
            initBuffer( pMessage[1], (CsrUint8 *)malloc(length), length );
        }
    }

    WPS_DBG_PRINT(( "WSC DONE length = %d", pCtx->out.size ));
}

void encodeAck( WpsContext *pCtx )
{
WpsBuffer *pMessage[2];
int i;
unsigned int length;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addVersion( pMessage[i] );
        length += addMessageType( pMessage[i], WPS_ACK );
        length += addEnrolleeNonce( pCtx, pMessage[i] );
        length += addRegistrarNonce( pCtx, pMessage[i] );
 
        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if( i == 0 )
        {
            /* No additional space required for Authenticator */
            initBuffer( pMessage[1], (CsrUint8 *)malloc(length), length );
        }
    }

    WPS_DBG_PRINT(( "WSC ACK length = %d", pCtx->out.size ));
}

void encodeNack( WpsContext *pCtx, unsigned char error )
{
WpsBuffer *pMessage[2];
int i;
unsigned int length;

    pMessage[0] = NULL;
    pMessage[1] = &pCtx->out;

    /* First iteration with pMessage[0] of NULL to compute length of buffer
       required. Second iteration with pMessage[1] to build message */
    for( i = 0; i < 2; i++ )
    {
        length =  addVersion( pMessage[i] );
        length += addMessageType( pMessage[i], WPS_NACK );
        length += addEnrolleeNonce( pCtx, pMessage[i] );
        length += addRegistrarNonce( pCtx, pMessage[i] );
        length += addConfigurationError( pMessage[i], error );
 
        /* At end of first iteration, the length is known to
           initialise the buffer to build the message in the
           second iteration */
        if( i == 0 )
        {
            /* No additional space required for Authenticator */
            initBuffer( pMessage[1], (CsrUint8 *)malloc(length), length );
        }
    }

    WPS_DBG_PRINT(( "WSC NACK length = %d", pCtx->out.size ));
}
