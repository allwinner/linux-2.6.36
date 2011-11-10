/** @file wps_decode.c
 *
 * Implementation of the Wi-Fi Protected Setup (WPS) decoding.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 * @section DESCRIPTION
 *   This provides an implementation of Wi-Fi Protected Setup (WPS) decoding
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/wps_decode.c#2 $
 *
 ****************************************************************************/

#include "wps_decode.h"
#include "wps_common.h"
#include "wps_crypto.h"
#include "wps_debug.h"

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static unsigned int getDataElement( WpsBuffer *pMessage,
                                    WpsBuffer *pValue )
{
unsigned int type;
unsigned int length;
unsigned int result = 0;
CsrUint8 *pBufferEnd;

    pBufferEnd = pMessage->pStart + pMessage->size;

    do
    {
       type =  *pMessage->pIndex++ << 8;
       if( pMessage->pIndex > pBufferEnd ) break;
       type |= *pMessage->pIndex++;
       if( pMessage->pIndex > pBufferEnd ) break;

       length =  *pMessage->pIndex++ << 8;
       if( pMessage->pIndex > pBufferEnd ) break;
       length |= *pMessage->pIndex++;
       if( pMessage->pIndex > pBufferEnd ) break;

       pValue->pStart = pValue->pIndex = pMessage->pIndex;
       pValue->size = length;

       pMessage->pIndex += length;
       if( pMessage->pIndex > pBufferEnd ) break;

       result = type;
    }
    while( 0 );

    if( !result )
    {
        pValue->pStart = pValue->pIndex = NULL;
        pValue->size = 0;
    }

    return result;
}

static CsrBool checkVersion( WpsBuffer *value )
{
CsrBool result = FALSE;

    if( value->size == 1 && value->pStart[0] == WPS_SUPPORTED_VERSION )
    {
        result = TRUE;
    }

    return result;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/
CsrUint8 getMessageType( WpsBuffer *pIn )
{
CsrUint8 result = 0;
WpsBuffer value;

    do
    {
       if( WPS_VERSION != getDataElement( pIn, &value ) )
       {
           WPS_DBG_PRINT(( "getDataElement(): Version missing" ));
           break;
       }
       if( !checkVersion( &value ) )
       {
           WPS_DBG_PRINT(( "checkVersion(): version not supported" ));
           break;
       }
       if( WPS_MESSAGE_TYPE != getDataElement( pIn, &value ) )
       {
           WPS_DBG_PRINT(( "getDataElement(): Message Type missing" ));
           break;
       }

       /* Everything OK if no break by this point. Finally check the length of the
        * Message Type data element before setting return value */
       if( value.size == 1 )
       {
           result = value.pStart[0];
       }
    }
    while( 0 );

    return result;
}

CsrBool parseM2( WpsContext *pCtx )
{
WpsBuffer value;
unsigned int id;
CsrBool result = FALSE;

    do
    {
       if( WPS_ENROLLEE_NONCE != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Enrollee Nonce missing" ));
           break;
       }

       if( WPS_REGISTRAR_NONCE != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Registrar Nonce missing" ));
           break;
       }
       /* Check incoming length against expected length? */
       memcpy( pCtx->rNonce, value.pStart, sizeof( pCtx->rNonce ) );

       if( WPS_UUID_R != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required UUID-R missing" ));
           break;
       }

       if( WPS_PUBLIC_KEY != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Public Key missing" ));
           break;
       }
       /* Check incoming length against expected length? */
       memcpy( pCtx->rPubKey, value.pStart, sizeof( pCtx->rPubKey ) );

       if( WPS_AUTHENTICATION_TYPE_FLAGS != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Authentication Type Flags missing" ));
           break;
       }

       if( WPS_ENCRYPTION_TYPE_FLAGS != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Encryption Type Flags missing" ));
           break;
       }

       if( WPS_CONNECTION_TYPE_FLAGS != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Connection Type Flags missing" ));
           break;
       }

       if( WPS_CONFIG_METHODS != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Config Methods missing" ));
           break;
       }

       if( WPS_MANUFACTURER != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Manufacturer missing" ));
           break;
       }

       if( WPS_MODEL_NAME != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Model Name missing" ));
           break;
       }

       if( WPS_MODEL_NUMBER != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Model Number missing" ));
           break;
       }

       if( WPS_SERIAL_NUMBER != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Serial Number missing" ));
           break;
       }

       if( WPS_PRIMARY_DEVICE_TYPE != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Primary Device Type missing" ));
           break;
       }

       if( WPS_DEVICE_NAME != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Device Name missing" ));
           break;
       }

       if( WPS_RF_BANDS != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required RF Bands missing" ));
           break;
       }

       if( WPS_ASSOCIATION_STATE != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Association State missing" ));
           break;
       }

       if( WPS_CONFIGURATION_ERROR != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Configuration Error missing" ));
           break;
       }

       if( WPS_DEVICE_PASSWORD_ID != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required Device Password ID missing" ));
           break;
       }

       if( WPS_OS_VERSION_ID != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: required OS Version ID missing" ));
           break;
       }

       /* Check for any non-required optional data elements before final
          Authenticator data element */
       for( id = getDataElement( &pCtx->in, &value );
            id != 0 && id != WPS_AUTHENTICATOR;
            id = getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M2: Non-required optional (%04x)", id ));
       }

       if( WPS_AUTHENTICATOR != id )
       {
           WPS_DBG_PRINT(( "M2: Authenticator missing" ));
           break;
       }

       /* Session keys must be derived before AuthKey is used to verify authenticator */
       computeSessionKeys( pCtx );

       if( !verifyAuthenticator( pCtx, &pCtx->out, &pCtx->in, value.pStart ) )
       {
           WPS_DBG_PRINT(( "WPS M2 Authenticator BAD" ));
           break;
       }

       /* Everything present and correct if no break by this point */
       result = TRUE;
    }
    while( 0 );

    return result;
}

CsrBool parseM4( WpsContext *pCtx )
{
WpsBuffer value;
unsigned int id;
CsrBool result = FALSE;
WpsBuffer rHash1;
WpsBuffer encryptedSettings;

    do
    {
       if( WPS_ENROLLEE_NONCE != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M4: required Enrollee Nonce missing" ));
           break;
       }
       if( WPS_R_HASH1 != getDataElement( &pCtx->in, &rHash1 ) )
       {
           WPS_DBG_PRINT(( "M4: required R-HASH1 missing" ));
           break;
       }

       if( WPS_R_HASH2 != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M4: required R-HASH2 missing" ));
           break;
       }
       memcpy( pCtx->rHash2, value.pStart, sizeof( pCtx->rHash2 ) );

       if( WPS_ENCRYPTED_SETTINGS != getDataElement( &pCtx->in, &encryptedSettings ) )
       {
           WPS_DBG_PRINT(( "M4: required Encrypted Settings missing" ));
           break;
       }

       /* Check for any non-required optional data elements before final
          Authenticator data element */
       for( id = getDataElement( &pCtx->in, &value );
            id != 0 && id != WPS_AUTHENTICATOR;
            id = getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M4: Non-required optional (%04x)", id ));
       }

       if( WPS_AUTHENTICATOR != id )
       {
           WPS_DBG_PRINT(( "M4: Authenticator missing" ));
           break;
       }

       if( !verifyAuthenticator( pCtx, &pCtx->out, &pCtx->in, value.pStart ) )
       {
           WPS_DBG_PRINT(( "WPS M4 Authenticator BAD" ));
           break;
       }

       if( !parseEncryptedSettingsM4( pCtx, &encryptedSettings, &rHash1 ) )
       {
           WPS_DBG_PRINT(( "WPS M4 Encrypted Settings parse failed" ));
           break;
       }

       /* Everything present and correct if no break by this point */
       result = TRUE;
    }
    while( 0 );

    return result;
}

CsrBool parseEncryptedSettingsM4( WpsContext *pCtx, WpsBuffer *pEncryptedSettings, WpsBuffer *rHash1 )
{
WpsBuffer value;
unsigned int id;
CsrBool result = FALSE;
WpsBuffer rSNonce1;
WpsBuffer decryptedSettings;

    decryptEncryptedSettings( pCtx, pEncryptedSettings, &decryptedSettings );

    do
    {
       if( WPS_R_SNONCE1 != getDataElement( &decryptedSettings, &rSNonce1 ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: required R-SNONCE1 missing" ));
           break;
       }

       /* Check for any non-required optional data elements before final
          Authenticator data element */
       for( id = getDataElement( &decryptedSettings, &value );
            id != 0 && id != WPS_KEY_WRAP_AUTHENTICATOR;
            id = getDataElement( &decryptedSettings, &value ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Non-required optional (%04x)", id ));
       }

       if( WPS_KEY_WRAP_AUTHENTICATOR != id )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Key Wrap Authenticator missing" ));
           break;
       }

       if( !verifyKeyWrapAuthenticator( pCtx, &decryptedSettings, value.pStart ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Key Wrap Authenticator bad" ));
           break;
       }

       /* The R-SNonce1 is now decoded, so R-HASH1 can be checked */
       if( !verifyEHash( pCtx, rSNonce1.pStart, pCtx->psk1, rHash1->pStart ) )
       {
           WPS_DBG_PRINT(( "R-HASH1 verification failed" ));
           break;
       }

       /* Everything present and correct if no break by this point */
       result = TRUE;
    }
    while( 0 );

    return result;
}

CsrBool parseM6( WpsContext *pCtx )
{
WpsBuffer value;
unsigned int id;
CsrBool result = FALSE;
WpsBuffer encryptedSettings;

    do
    {
       if( WPS_ENROLLEE_NONCE != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M6: required Enrollee Nonce missing" ));
           break;
       }
       if( WPS_ENCRYPTED_SETTINGS != getDataElement( &pCtx->in, &encryptedSettings ) )
       {
           WPS_DBG_PRINT(( "M6: required Encrypted Settings missing" ));
           break;
       }

       /* Check for any non-required optional data elements before final
          Authenticator data element */
       for( id = getDataElement( &pCtx->in, &value );
            id != 0 && id != WPS_AUTHENTICATOR;
            id = getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M6: Non-required optional (%04x)", id ));
       }

       if( WPS_AUTHENTICATOR != id )
       {
           WPS_DBG_PRINT(( "M6: Authenticator missing" ));
           break;
       }

       if( !verifyAuthenticator( pCtx, &pCtx->out, &pCtx->in, value.pStart ) )
       {
           WPS_DBG_PRINT(( "WPS M6 Authenticator BAD" ));
           break;
       }

       if( !parseEncryptedSettingsM6( pCtx, &encryptedSettings ) )
       {
           WPS_DBG_PRINT(( "WPS Encrypted Settings parse failed" ));
           break;
       }

       /* Everything present and correct if no break by this point */
       result = TRUE;
    }
    while( 0 );

    return result;
}

CsrBool parseEncryptedSettingsM6( WpsContext *pCtx, WpsBuffer *pEncryptedSettings )
{
WpsBuffer value;
unsigned int id;
CsrBool result = FALSE;
WpsBuffer rSNonce2;
WpsBuffer decryptedSettings;

    decryptEncryptedSettings( pCtx, pEncryptedSettings, &decryptedSettings );

    do
    {
       if( WPS_R_SNONCE2 != getDataElement( &decryptedSettings, &rSNonce2 ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: required R-SNONCE2 missing" ));
           break;
       }

       /* Check for any non-required optional data elements before final
          Authenticator data element */
       for( id = getDataElement( &decryptedSettings, &value );
            id != 0 && id != WPS_KEY_WRAP_AUTHENTICATOR;
            id = getDataElement( &decryptedSettings, &value ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Non-required optional (%04x)", id ));
       }

       if( WPS_KEY_WRAP_AUTHENTICATOR != id )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Key Wrap Authenticator missing" ));
           break;
       }

       if( !verifyKeyWrapAuthenticator( pCtx, &decryptedSettings, value.pStart ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Key Wrap Authenticator bad" ));
           break;
       }

       /* The R-SNonce2 is now decoded, so R-HASH2 can be checked */
       if( !verifyEHash( pCtx, rSNonce2.pStart, pCtx->psk2, pCtx->rHash2 ) )
       {
           WPS_DBG_PRINT(( "R-HASH2 verification failed" ));
           break;
       }

       /* Everything present and correct if no break by this point */
       result = TRUE;
    }
    while( 0 );

    return result;
}

CsrBool parseM8( WpsContext *pCtx )
{
WpsBuffer value;
unsigned int id;
CsrBool result = FALSE;
WpsBuffer encryptedSettings;

    do
    {
       if( WPS_ENROLLEE_NONCE != getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M8: required Enrollee Nonce missing" ));
           break;
       }
       if( WPS_ENCRYPTED_SETTINGS != getDataElement( &pCtx->in, &encryptedSettings ) )
       {
           WPS_DBG_PRINT(( "M8: required Encrypted Settings missing" ));
           break;
       }

       /* Check for any non-required optional data elements before final
          Authenticator data element */
       for( id = getDataElement( &pCtx->in, &value );
            id != 0 && id != WPS_AUTHENTICATOR;
            id = getDataElement( &pCtx->in, &value ) )
       {
           WPS_DBG_PRINT(( "M8: Non-required optional (%04x)", id ));
       }

       if( WPS_AUTHENTICATOR != id )
       {
           WPS_DBG_PRINT(( "M8: Authenticator missing" ));
           break;
       }

       if( !verifyAuthenticator( pCtx, &pCtx->out, &pCtx->in, value.pStart ) )
       {
           WPS_DBG_PRINT(( "WPS M8 Authenticator BAD" ));
           break;
       }

       if( !parseEncryptedSettingsM8( pCtx, &encryptedSettings ) )
       {
           WPS_DBG_PRINT(( "WPS Encrypted Settings parse failed" ));
           break;
       }

       /* Everything present and correct if no break by this point */
       result = TRUE;
    }
    while( 0 );

    return result;
}

CsrBool parseEncryptedSettingsM8( WpsContext *pCtx, WpsBuffer *pEncryptedSettings )
{
WpsBuffer value;
unsigned int id;
CsrBool result = FALSE;
WpsBuffer decryptedSettings;
WpsBuffer credential;
int credentialCount = 0;

    decryptEncryptedSettings( pCtx, pEncryptedSettings, &decryptedSettings );

    do
    {
       if( WPS_CREDENTIAL != getDataElement( &decryptedSettings, &value ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: required Credential missing" ));
           break;
       }

       if( !parseCredential( pCtx, &value, credentialCount ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Credential parse bad" ));
       }
       credentialCount++;

       /* Check for any non-required optional data elements before final
          Authenticator data element */
       id = getDataElement( &decryptedSettings, &value );

       while( WPS_CREDENTIAL == id )
       {
           if( !parseCredential( pCtx, &value, credentialCount ) )
           {
               WPS_DBG_PRINT(( "Encrypted Settings Data: Credential parse bad" ));
           }
           credentialCount++;
           id = getDataElement( &decryptedSettings, &value );
       }

       while( id != 0 && id != WPS_KEY_WRAP_AUTHENTICATOR )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Non-required optional (%04x)", id ));
           id = getDataElement( &decryptedSettings, &value );
       }

       if( WPS_KEY_WRAP_AUTHENTICATOR != id )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Key Wrap Authenticator missing" ));
           break;
       }

       if( !verifyKeyWrapAuthenticator( pCtx, &decryptedSettings, value.pStart ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Key Wrap Authenticator bad" ));
           break;
       }

       /* Everything present and correct if no break by this point */
       result = TRUE;
    }
    while( 0 );

    return result;
}

CsrBool parseCredential( WpsContext *pCtx, WpsBuffer *pCredential, int credentialCount )
{
WpsBuffer value;
unsigned int id;
CsrBool result = FALSE;

    do
    {
       if( WPS_NETWORK_INDEX != getDataElement( pCredential, &value ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: required Network Index missing" ));
           break;
       }
       if( WPS_SSID != getDataElement( pCredential, &value ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: required SSID missing" ));
           break;
       }

       if( credentialCount != 0 && memcmp( value.pStart, pCtx->pCandidate->ssid, value.size ) )
       {
    	   /* Terminate the parse early successfully if the SSID does not match */
    	   result = TRUE;
    	   break;
       }

       /* Remember the SSID for the final profile generation */
       pCtx->ssid.pStart = value.pStart;
       pCtx->ssid.pIndex = value.pIndex;
       pCtx->ssid.size = value.size;
	   WPS_DBG_PRINT(( "Match found for SSID %s", pCtx->pCandidate->ssid ));

       if( WPS_AUTHENTICATION_TYPE != getDataElement( pCredential, &value ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: required Authentication Type missing" ));
           break;
       }

       WPS_DBG_PRINTHEX(( "Authentication type", value.pStart, value.size ));
       if( value.size == 2 )
       {
           pCtx->authenticationType = value.pStart[0] << 8;
           pCtx->authenticationType |= value.pStart[1];
       }
       else
       {
           WPS_DBG_PRINT(( "Authentication Type incorrect size" ));
           pCtx->authenticationType = 0;
       }

       if( WPS_ENCRYPTION_TYPE != getDataElement( pCredential, &value ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: required Encryption Type missing" ));
           break;
       }
       WPS_DBG_PRINTHEX(( "Encryption type", value.pStart, value.size ));
       if( value.size == 2 )
       {
           pCtx->encryptionType = value.pStart[0] << 8;
           pCtx->encryptionType |= value.pStart[1];
       }
       else
       {
           WPS_DBG_PRINT(( "Encryption Type incorrect size" ));
           pCtx->encryptionType = 0;
       }

       id = getDataElement( pCredential, &value );
       if( WPS_NETWORK_KEY_INDEX != id )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: optional Network Key Index missing" ));
           /* Default to network key index 1 */
           pCtx->networkKeyIndex = 1;
       }
       else
       {
           if(value.size == 1 && value.pStart[0] >= 1 && value.pStart[0] <= 4)
           {
               pCtx->networkKeyIndex = value.pStart[0] - 1;
           }
    	   id = getDataElement( pCredential, &value );
       }

       if( WPS_NETWORK_KEY != id )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: required Network Key missing (ignored)" ));
           pCtx->networkKey.size = 0;
       }
       else
       {
    	   pCtx->networkKey.pStart = value.pStart;
    	   pCtx->networkKey.size = value.size;
    	   id = getDataElement( pCredential, &value );
       }

       if( WPS_MAC_ADDRESS != id )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: required MAC Address missing" ));
           break;
       }

       /* Check for any optional data elements */
       for( id = getDataElement( pCredential, &value );
            id != 0;
            id = getDataElement( pCredential, &value ) )
       {
           WPS_DBG_PRINT(( "Encrypted Settings Data: Optional (%04x)", id ));
       }

       /* Everything present and correct if no break by this point */
       result = TRUE;
    }
    while( 0 );

    return result;
}
