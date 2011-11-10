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
 *   $Id: //depot/dot11/v7.0p/host/lib_security/csr_wps/wps_decode.c#4 $
 *
 ****************************************************************************/

#include "sme_trace/sme_trace.h"
#include "wps_decode.h"
#include "wps_common.h"
#include "wps_crypto.h"
#include "csr_aes/csr_aes.h"
#include "csr_hmac/csr_hmac.h"

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static CsrUint32 getDataElement( WpsBuffer *pMessage,
                                    WpsBuffer *pValue )
{
    CsrUint32 type;
    CsrUint32 length;
    CsrUint32 result = 0;
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
           sme_trace_info((TR_SECURITY_LIB, "getDataElement(): Version missing" ));
           break;
       }
       if( !checkVersion( &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "checkVersion(): version not supported" ));
           break;
       }
       if( WPS_MESSAGE_TYPE != getDataElement( pIn, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "getDataElement(): Message Type missing" ));
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

void parseM2( CsrWpsContext *pCtx )
{
    WpsBuffer value;
    CsrUint32 id;
    CsrBool result = FALSE;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    do
    {
       if( WPS_ENROLLEE_NONCE != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Enrollee Nonce missing" ));
           break;
       }

       if( WPS_REGISTRAR_NONCE != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Registrar Nonce missing" ));
           break;
       }
       /* Check incoming length against expected length? */
       CsrMemCpy( pCtx->rNonce, value.pStart, sizeof( pCtx->rNonce ) );

       if( WPS_UUID_R != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required UUID-R missing" ));
           break;
       }

       if( WPS_PUBLIC_KEY != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Public Key missing" ));
           break;
       }
       /* Check incoming length against expected length? */
       CsrMemCpy( pCtx->rPubKey, value.pStart, sizeof( pCtx->rPubKey ) );

       if( WPS_AUTHENTICATION_TYPE_FLAGS != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Authentication Type Flags missing" ));
           break;
       }

       if( WPS_ENCRYPTION_TYPE_FLAGS != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Encryption Type Flags missing" ));
           break;
       }

       if( WPS_CONNECTION_TYPE_FLAGS != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Connection Type Flags missing" ));
           break;
       }

       if( WPS_CONFIG_METHODS != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Config Methods missing" ));
           break;
       }

       if( WPS_MANUFACTURER != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Manufacturer missing" ));
           break;
       }

       if( WPS_MODEL_NAME != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Model Name missing" ));
           break;
       }

       if( WPS_MODEL_NUMBER != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Model Number missing" ));
           break;
       }

       if( WPS_SERIAL_NUMBER != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Serial Number missing" ));
           break;
       }

       if( WPS_PRIMARY_DEVICE_TYPE != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Primary Device Type missing" ));
           break;
       }

       if( WPS_DEVICE_NAME != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Device Name missing" ));
           break;
       }

       if( WPS_RF_BANDS != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required RF Bands missing" ));
           break;
       }

       if( WPS_ASSOCIATION_STATE != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Association State missing" ));
           break;
       }

       if( WPS_CONFIGURATION_ERROR != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Configuration Error missing" ));
           break;
       }

       if( WPS_DEVICE_PASSWORD_ID != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required Device Password ID missing" ));
           break;
       }

       if( WPS_OS_VERSION_ID != getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: required OS Version ID missing" ));
           break;
       }

       /* Check for any non-required optional data elements before final
          Authenticator data element */
       for( id = getDataElement( &pCtx->in, &value );
            id != 0 && id != WPS_AUTHENTICATOR;
            id = getDataElement( &pCtx->in, &value ) )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: Non-required optional (%04x)", id ));
       }

       if( WPS_AUTHENTICATOR != id )
       {
           sme_trace_info((TR_SECURITY_LIB, "M2: Authenticator missing" ));
           break;
       }

       pCtx->rxAuthenticator = value.pStart;
       result = TRUE;

    } while (0);

    if (result == FALSE)
    {
        pCtx->result = FALSE;
        return;
    }

    computeSessionKeys (pCtx);
    /* Session keys must be derived before AuthKey is used to verify authenticator */
    /* Do not include the 12 byte authenticator element in the current message */
    pCtx->in.size -= WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    pCtx->message = (CsrUint8 *) CsrPmalloc(pCtx->out.size + pCtx->in.size);
    CsrMemCpy(pCtx->message, pCtx->out.pStart, pCtx->out.size);
    CsrMemCpy(pCtx->message + pCtx->out.size, pCtx->in.pStart, pCtx->in.size);

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->message, pCtx->out.size + pCtx->in.size,
                        digest);

    CsrPfree(pCtx->message);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", digest, WPS_AUTHENTICATOR_LENGTH));

    pCtx->in.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    if(CsrMemCmp(digest, pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH))
    {
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Authenticator error", pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS computed authenticator", digest, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_info((TR_SECURITY_LIB, "WPS M2 Authenticator BAD"));
        pCtx->result = FALSE;
    }
    else
    {
        /* Everything present and correct at this point */
        pCtx->result = TRUE;
    }
}

void parseM4( CsrWpsContext *pCtx )
{
    WpsBuffer value;
    CsrUint32 id;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    do
    {
        if( WPS_ENROLLEE_NONCE != getDataElement( &pCtx->in, &value ) )
        {
            sme_trace_info((TR_SECURITY_LIB, "M4: required Enrollee Nonce missing" ));
            pCtx->result = FALSE;
            break;
        }
        if( WPS_R_HASH1 != getDataElement( &pCtx->in, &pCtx->rHash1 ) )
        {
            sme_trace_info((TR_SECURITY_LIB, "M4: required R-HASH1 missing" ));
            pCtx->result = FALSE;
            break;
        }
        if( WPS_R_HASH2 != getDataElement( &pCtx->in, &value ) )
        {
            sme_trace_info((TR_SECURITY_LIB, "M4: required R-HASH2 missing" ));
            pCtx->result = FALSE;
            break;
        }
        CsrMemCpy( pCtx->rHash2, value.pStart, sizeof( pCtx->rHash2 ) );

        if( WPS_ENCRYPTED_SETTINGS != getDataElement( &pCtx->in, &pCtx->encryptedSettings ) )
        {
            sme_trace_info((TR_SECURITY_LIB, "M4: required Encrypted Settings missing" ));
            pCtx->result = FALSE;
            break;
        }

        /* Check for any non-required optional data elements before final
           Authenticator data element */
        for(id = getDataElement(&pCtx->in, &value); id != 0 && id != WPS_AUTHENTICATOR; id = getDataElement(&pCtx->in, &value))
        {
            sme_trace_info((TR_SECURITY_LIB, "M4: Non-required optional (%04x)", id));
        }

        if(WPS_AUTHENTICATOR != id)
        {
            sme_trace_info((TR_SECURITY_LIB, "M4: Authenticator missing"));
            pCtx->result = FALSE;
            break;
        }
    }
    while (0);

    if (pCtx->result == FALSE)
    {
        return;
    }

    pCtx->rxAuthenticator = value.pStart;

    /* Do not include the 12 byte authenticator element in the current message */
    pCtx->in.size -= WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;
    pCtx->message = (CsrUint8 *)CsrPmalloc(pCtx->out.size + pCtx->in.size);
    CsrMemCpy(pCtx->message, pCtx->out.pStart, pCtx->out.size);
    CsrMemCpy(pCtx->message + pCtx->out.size, pCtx->in.pStart, pCtx->in.size);

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->message, pCtx->out.size + pCtx->in.size,
                        digest);
    CsrPfree(pCtx->message);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", digest, WPS_AUTHENTICATOR_LENGTH));

    pCtx->in.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    if(CsrMemCmp(digest, pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH))
    {
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Authenticator error", pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS computed authenticator", digest, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_info((TR_SECURITY_LIB, "WPS M4 Authenticator BAD"));
        pCtx->result = FALSE;
        return;
    }

    parseEncryptedSettingsM4(pCtx);
    if(pCtx->result == FALSE)
    {
        sme_trace_info((TR_SECURITY_LIB, "WPS M4 Encrypted Settings parse failed"));
    }
}

void parseEncryptedSettingsM4(CsrWpsContext *pCtx)
{
    CsrUint8 iv[WPS_IV_LENGTH];
    WpsBuffer value;
    CsrUint32 id;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings IV", pCtx->encryptedSettings.pStart, WPS_IV_LENGTH));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings", &pCtx->encryptedSettings.pStart[WPS_IV_LENGTH], pCtx->encryptedSettings.size - WPS_IV_LENGTH));

    /* IV gets modified by Aes128CbcDecrypt but is later used in MAC computation, so save it */
    CsrMemCpy(iv, pCtx->encryptedSettings.pStart, WPS_IV_LENGTH);
    initBuffer(&pCtx->decryptedSettings, (CsrUint8 *)CsrPmalloc(1000), 1000);
    CsrCryptoAes128CbcDecrypt(pCtx->keyWrapKey, iv,
                              &pCtx->encryptedSettings.pStart[WPS_IV_LENGTH], pCtx->encryptedSettings.size - WPS_IV_LENGTH,
                              pCtx->decryptedSettings.pStart,
                              &pCtx->decryptedSettings.size,
                              PAD_MODE_PKCS7);
    sme_trace_info((TR_SECURITY_LIB, "Decrypted %d bytes from Encrypted Settings", pCtx->decryptedSettings.size));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Decrypted Settings", pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size));

    if(WPS_R_SNONCE1 != getDataElement(&pCtx->decryptedSettings, &pCtx->rSNonce1))
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: required R-SNONCE1 missing"));
        pCtx->result = FALSE;
        freeBuffer(&pCtx->decryptedSettings);
        return;
    }

    /* Check for any non-required optional data elements before final
       Authenticator data element */
    for(id = getDataElement( &pCtx->decryptedSettings, &value); id != 0 && id != WPS_KEY_WRAP_AUTHENTICATOR; id = getDataElement(&pCtx->decryptedSettings, &value))
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Non-required optional (%04x)", id));
    }

    if(WPS_KEY_WRAP_AUTHENTICATOR != id)
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Key Wrap Authenticator missing"));
        pCtx->result = FALSE;
        freeBuffer(&pCtx->decryptedSettings);
        return;
    }

    pCtx->rxAuthenticator = value.pStart;

    /* Do not include the 12 byte key wrap authenticator element in the calculation */
    pCtx->decryptedSettings.size -= WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size,
                        digest);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", digest, WPS_AUTHENTICATOR_LENGTH));

    pCtx->decryptedSettings.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    if(CsrMemCmp(digest, pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH))
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Key Wrap Authenticator bad"));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS computed authenticator", digest, WPS_AUTHENTICATOR_LENGTH));
        pCtx->result = FALSE;
        freeBuffer(&pCtx->decryptedSettings);
        return;
    }

    /* The R-SNonce1 is now decoded, so R-HASH1 can be checked */

    /* The E-Hash is the HMAC-SHA256 of the concatenation of a secret
       nonce, PSK, the Enrollee Public Key and the Registrar Public Key,
       keyed with AuthKey. */

    pCtx->message = (CsrUint8 *) CsrPmalloc(2 * WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey) + sizeof(pCtx->rPubKey));
    CsrMemCpy(pCtx->message, pCtx->rSNonce1.pStart, WPS_NONCE_LENGTH);
    CsrMemCpy(pCtx->message + WPS_NONCE_LENGTH, pCtx->psk1, WPS_NONCE_LENGTH);
    CsrMemCpy(pCtx->message + 2 * WPS_NONCE_LENGTH, pCtx->ePubKey, sizeof(pCtx->ePubKey));
    CsrMemCpy(pCtx->message + 2 * WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey), pCtx->rPubKey, sizeof(pCtx->rPubKey));

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->message, 2 * WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey) + sizeof(pCtx->rPubKey),
                        digest);
    CsrPfree(pCtx->message);

    if(CsrMemCmp(digest, pCtx->rHash1.pStart, WPS_SHA256_DIGEST_LENGTH))
    {
        sme_trace_info((TR_SECURITY_LIB, "R-HASH1 verification failed"));
        pCtx->result = FALSE;
    }

    freeBuffer(&pCtx->decryptedSettings);

    /* Everything present and correct at this point */
}

void parseM6( CsrWpsContext *pCtx )
{
    WpsBuffer value;
    CsrUint32 id;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    do
    {
        if(WPS_ENROLLEE_NONCE != getDataElement(&pCtx->in, &value))
        {
            sme_trace_info((TR_SECURITY_LIB, "M6: required Enrollee Nonce missing"));
            pCtx->result = FALSE;
            break;
        }
        if(WPS_ENCRYPTED_SETTINGS != getDataElement(&pCtx->in, &pCtx->encryptedSettings))
        {
            sme_trace_info((TR_SECURITY_LIB, "M6: required Encrypted Settings missing"));
            pCtx->result = FALSE;
            break;
        }

        /* Check for any non-required optional data elements before final
           Authenticator data element */
        for(id = getDataElement(&pCtx->in, &value); id != 0 && id != WPS_AUTHENTICATOR; id = getDataElement(&pCtx->in, &value))
        {
            sme_trace_info((TR_SECURITY_LIB, "M6: Non-required optional (%04x)", id));
        }

        if(WPS_AUTHENTICATOR != id)
        {
            sme_trace_info((TR_SECURITY_LIB, "M6: Authenticator missing"));
            pCtx->result = FALSE;
            break;
        }
    }
    while (0);

    if(pCtx->result == FALSE)
    {
        return;
    }

    pCtx->rxAuthenticator = value.pStart;

    /* Do not include the 12 byte authenticator element in the current message */
    pCtx->in.size -= WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;
    pCtx->message = (CsrUint8 *)CsrPmalloc(pCtx->out.size + pCtx->in.size);
    CsrMemCpy(pCtx->message, pCtx->out.pStart, pCtx->out.size);
    CsrMemCpy(pCtx->message + pCtx->out.size, pCtx->in.pStart, pCtx->in.size);

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->message, pCtx->out.size + pCtx->in.size,
                        digest);
    CsrPfree(pCtx->message);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", digest, WPS_AUTHENTICATOR_LENGTH));

    pCtx->in.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    if(CsrMemCmp(digest, pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH))
    {
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Authenticator error", pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS computed authenticator", digest, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_info((TR_SECURITY_LIB, "WPS M6 Authenticator BAD"));
        pCtx->result = FALSE;
        return;
    }
    parseEncryptedSettingsM6(pCtx);
    if(pCtx->result == FALSE)
    {
        sme_trace_info((TR_SECURITY_LIB, "WPS Encrypted Settings parse failed"));
    }
}

void parseEncryptedSettingsM6(CsrWpsContext *pCtx)
{
    CsrUint8 iv[WPS_IV_LENGTH];
    WpsBuffer value;
    CsrUint32 id;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings IV", pCtx->encryptedSettings.pStart, WPS_IV_LENGTH));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings", &pCtx->encryptedSettings.pStart[WPS_IV_LENGTH],
                  pCtx->encryptedSettings.size - WPS_IV_LENGTH));

    CsrMemCpy(iv, pCtx->encryptedSettings.pStart, WPS_IV_LENGTH);
    initBuffer(&pCtx->decryptedSettings, (CsrUint8 *)CsrPmalloc(1000), 1000);
    CsrCryptoAes128CbcDecrypt(pCtx->keyWrapKey, iv,
                              &pCtx->encryptedSettings.pStart[WPS_IV_LENGTH], pCtx->encryptedSettings.size - WPS_IV_LENGTH,
                              pCtx->decryptedSettings.pStart,
                              &pCtx->decryptedSettings.size,
                              PAD_MODE_PKCS7);
    sme_trace_info((TR_SECURITY_LIB, "Decrypted %d bytes from Encrypted Settings", pCtx->decryptedSettings.size));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Decrypted Settings", pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size));

    if(WPS_R_SNONCE2 != getDataElement( &pCtx->decryptedSettings, &pCtx->rSNonce2))
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: required R-SNONCE2 missing"));
        pCtx->result = FALSE;
        freeBuffer(&pCtx->decryptedSettings);
        return;
    }

    /* Check for any non-required optional data elements before final authenticator data element */
    for(id = getDataElement(&pCtx->decryptedSettings, &value); id != 0 && id != WPS_KEY_WRAP_AUTHENTICATOR; id = getDataElement(&pCtx->decryptedSettings, &value))
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Non-required optional (%04x)", id));
    }

    if(WPS_KEY_WRAP_AUTHENTICATOR != id)
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Key Wrap Authenticator missing"));
        pCtx->result = FALSE;
        freeBuffer(&pCtx->decryptedSettings);
        return;
    }

    pCtx->rxAuthenticator = value.pStart;

    /* Do not include the 12 byte key wrap authenticator element in the calculation */
    pCtx->decryptedSettings.size -= WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size,
                        digest);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", digest, WPS_AUTHENTICATOR_LENGTH));


    pCtx->decryptedSettings.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    if(CsrMemCmp(digest, pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH))
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Key Wrap Authenticator bad"));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS computed authenticator", digest, WPS_AUTHENTICATOR_LENGTH));
        pCtx->result = FALSE;
        freeBuffer(&pCtx->decryptedSettings);
        return;
    }

    /* The R-SNonce2 is now decoded, so R-HASH2 can be checked */

    /* The E-Hash is the HMAC-SHA256 of the concatenation of a secret
       nonce, PSK, the Enrollee Public Key and the Registrar Public Key,
       keyed with AuthKey. */

    pCtx->message = (CsrUint8 *)CsrPmalloc(2 * WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey) + sizeof(pCtx->rPubKey));
    CsrMemCpy(pCtx->message, pCtx->rSNonce2.pStart, WPS_NONCE_LENGTH);
    CsrMemCpy(pCtx->message + WPS_NONCE_LENGTH, pCtx->psk2, WPS_NONCE_LENGTH);
    CsrMemCpy(pCtx->message + 2 * WPS_NONCE_LENGTH, pCtx->ePubKey, sizeof(pCtx->ePubKey));
    CsrMemCpy(pCtx->message + 2 * WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey), pCtx->rPubKey, sizeof(pCtx->rPubKey));

    CsrCryptoHmacSha256(pCtx->authKey, WPS_SHA256_DIGEST_LENGTH,
                        pCtx->message, 2 * WPS_NONCE_LENGTH + sizeof(pCtx->ePubKey) + sizeof(pCtx->rPubKey),
                        digest);
    CsrPfree(pCtx->message);

    if(CsrMemCmp(digest, pCtx->rHash2, WPS_SHA256_DIGEST_LENGTH))
    {
        sme_trace_info((TR_SECURITY_LIB, "R-HASH2 verification failed"));
        pCtx->result = FALSE;
    }

    freeBuffer(&pCtx->decryptedSettings);

    /* Everything present and correct if no break by this point */
}

void parseM8( CsrWpsContext *pCtx )
{
    WpsBuffer value;
    CsrUint32 id;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    do
    {
        if(WPS_ENROLLEE_NONCE != getDataElement(&pCtx->in, &value))
        {
            sme_trace_info((TR_SECURITY_LIB, "M8: required Enrollee Nonce missing"));
            pCtx->result = FALSE;
            break;
        }
        if(WPS_ENCRYPTED_SETTINGS != getDataElement(&pCtx->in, &pCtx->encryptedSettings))
        {
            sme_trace_info((TR_SECURITY_LIB, "M8: required Encrypted Settings missing"));
            pCtx->result = FALSE;
            break;
        }

        /* Check for any non-required optional data elements before final
           Authenticator data element */
        for( id = getDataElement(&pCtx->in, &value); id != 0 && id != WPS_AUTHENTICATOR; id = getDataElement(&pCtx->in, &value))
        {
            sme_trace_info((TR_SECURITY_LIB, "M8: Non-required optional (%04x)", id));
        }

        if(WPS_AUTHENTICATOR != id)
        {
            sme_trace_info((TR_SECURITY_LIB, "M8: Authenticator missing"));
            pCtx->result = FALSE;
            break;
        }
    }
    while (0);

    if (pCtx->result == FALSE)
    {
        return;
    }

    pCtx->rxAuthenticator = value.pStart;
    /* Do not include the 12 byte authenticator element in the current message */
    /* Compute Authenticator */
    pCtx->in.size -= WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;
    pCtx->message = (CsrUint8 *)CsrPmalloc(pCtx->out.size + pCtx->in.size);
    CsrMemCpy(pCtx->message, pCtx->out.pStart, pCtx->out.size);
    CsrMemCpy(pCtx->message + pCtx->out.size, pCtx->in.pStart, pCtx->in.size);

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->message, pCtx->out.size + pCtx->in.size,
                        digest);
    CsrPfree(pCtx->message);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", digest, WPS_AUTHENTICATOR_LENGTH));

    pCtx->in.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    if(CsrMemCmp(pCtx->rxAuthenticator, digest, WPS_AUTHENTICATOR_LENGTH))
    {
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Authenticator error", pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS computed authenticator", digest, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_info((TR_SECURITY_LIB, "WPS M8 Authenticator BAD"));
        pCtx->result = FALSE;
        return;
    }

    parseEncryptedSettingsM8(pCtx);
    if(pCtx->result == FALSE)
    {
        sme_trace_info((TR_SECURITY_LIB, "WPS Encrypted Settings parse failed"));
    }

    /* Everything present and correct if no break by this point */
}

void parseEncryptedSettingsM8(CsrWpsContext *pCtx)
{
    WpsBuffer value;
    CsrUint32 id;
    CsrInt32 credentialCount = 0;
    CsrUint8 digest[WPS_SHA256_DIGEST_LENGTH];

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings IV", pCtx->encryptedSettings.pStart, WPS_IV_LENGTH));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Encrypted Settings", &pCtx->encryptedSettings.pStart[WPS_IV_LENGTH], pCtx->encryptedSettings.size - WPS_IV_LENGTH));

    initBuffer(&pCtx->decryptedSettings, (CsrUint8 *)CsrPmalloc(1000), 1000);
    CsrCryptoAes128CbcDecrypt(pCtx->keyWrapKey, pCtx->encryptedSettings.pStart,
                              &pCtx->encryptedSettings.pStart[WPS_IV_LENGTH], pCtx->encryptedSettings.size - WPS_IV_LENGTH,
                              pCtx->decryptedSettings.pStart,
                              &pCtx->decryptedSettings.size,
                              PAD_MODE_PKCS7);
    sme_trace_info((TR_SECURITY_LIB, "Decrypted %d bytes from Encrypted Settings", pCtx->decryptedSettings.size));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS Decrypted Settings", pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size));

    if(WPS_CREDENTIAL != getDataElement(&pCtx->decryptedSettings, &value))
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: required Credential missing"));
        pCtx->result = FALSE;
        freeBuffer(&pCtx->decryptedSettings);
        return;
    }

    if(!parseCredential(pCtx, &value, credentialCount))
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Credential parse bad"));
    }
    credentialCount++;

    /* Check for any non-required optional data elements before final
       Authenticator data element */
    id = getDataElement(&pCtx->decryptedSettings, &value);

    while(WPS_CREDENTIAL == id)
    {
        if(!parseCredential(pCtx, &value, credentialCount))
        {
            sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Credential parse bad"));
        }
        credentialCount++;
        id = getDataElement(&pCtx->decryptedSettings, &value);
    }

    while(id != 0 && id != WPS_KEY_WRAP_AUTHENTICATOR)
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Non-required optional (%04x)", id));
        id = getDataElement(&pCtx->decryptedSettings, &value);
    }

    if(WPS_KEY_WRAP_AUTHENTICATOR != id)
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Key Wrap Authenticator missing"));
        pCtx->result = FALSE;
        freeBuffer(&pCtx->decryptedSettings);
        return;
    }

    pCtx->rxAuthenticator = value.pStart;

    /* Do not include the 12 byte key wrap authenticator element in the calculation */
    pCtx->decryptedSettings.size -= WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    CsrCryptoHmacSha256(pCtx->authKey, sizeof(pCtx->authKey),
                        pCtx->decryptedSettings.pStart, pCtx->decryptedSettings.size,
                        digest);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", digest, WPS_AUTHENTICATOR_LENGTH));

    pCtx->decryptedSettings.size += WPS_TL_LENGTH + WPS_AUTHENTICATOR_LENGTH;

    if(CsrMemCmp(digest, pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH))
    {
        sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Key Wrap Authenticator bad"));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS authenticator", pCtx->rxAuthenticator, WPS_AUTHENTICATOR_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "WPS computed authenticator", digest, WPS_AUTHENTICATOR_LENGTH));
        pCtx->result = FALSE;
        freeBuffer(&pCtx->decryptedSettings);
        return;
    }

    /* Everything present and correct if no break by this point */

    pCtx->securityContext->callbacks.wpsDone(pCtx->securityContext->externalContext,
                                             TRUE,
                                             pCtx->authenticationType,
                                             pCtx->encryptionType,
                                             pCtx->networkKey,
                                             pCtx->networkKeyLength,
                                             pCtx->networkKeyIndex,
                                             pCtx->ssid);

    freeBuffer(&pCtx->decryptedSettings);
}

CsrBool parseCredential( CsrWpsContext *pCtx, WpsBuffer *pCredential, CsrInt32 credentialCount )
{
    WpsBuffer value;
    CsrUint32 id;
    CsrBool result = FALSE;

    do
    {
       if(WPS_NETWORK_INDEX != getDataElement(pCredential, &value))
       {
           sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: required Network Index missing"));
           break;
       }
       if(WPS_SSID != getDataElement(pCredential, &value))
       {
           sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: required SSID missing"));
           break;
       }

       if(credentialCount != 0 && CsrMemCmp(value.pStart, pCtx->securityContext->setupData.ssid, value.size))
       {
           /* Terminate the parse early successfully if the SSID does not match */
           result = TRUE;
           break;
       }

       sme_trace_info((TR_SECURITY_LIB, "Match found for SSID %s", pCtx->securityContext->setupData.ssid));

       /* Remember the SSID for the final profile generation */
       CsrMemCpy(pCtx->ssid, value.pStart, value.size);

       if(WPS_AUTHENTICATION_TYPE != getDataElement(pCredential, &value))
       {
           sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: required Authentication Type missing"));
           break;
       }

       sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Authentication type", value.pStart, value.size));
       if(value.size == 2)
       {
           pCtx->authenticationType = value.pStart[0] << 8;
           pCtx->authenticationType |= value.pStart[1];
       }
       else
       {
           sme_trace_info((TR_SECURITY_LIB, "Authentication Type incorrect size"));
           pCtx->authenticationType = 0;
       }

       if(WPS_ENCRYPTION_TYPE != getDataElement(pCredential, &value))
       {
           sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: required Encryption Type missing"));
           break;
       }
       sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Encryption type", value.pStart, value.size));
       if(value.size == 2)
       {
           pCtx->encryptionType = value.pStart[0] << 8;
           pCtx->encryptionType |= value.pStart[1];
       }
       else
       {
           sme_trace_info((TR_SECURITY_LIB, "Encryption Type incorrect size"));
           pCtx->encryptionType = 0;
       }

       id = getDataElement(pCredential, &value);
       if(WPS_NETWORK_KEY_INDEX != id)
       {
           sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: optional Network Key Index missing"));
           /* Default to network key index 1 */
           pCtx->networkKeyIndex = 1;
       }
       else
       {
           if(value.size == 1 && value.pStart[0] >= 1 && value.pStart[0] <= 4)
           {
               pCtx->networkKeyIndex = value.pStart[0];
           }
           id = getDataElement(pCredential, &value);
       }

       if(WPS_NETWORK_KEY != id)
       {
           sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: required Network Key missing (ignored)"));
           pCtx->networkKeyLength = 0;
       }
       else
       {
           if ((CsrUint16) value.size > sizeof(pCtx->networkKey))
           {
               sme_trace_info((TR_SECURITY_LIB, "Network key too big"));
               break;
           }

           pCtx->networkKeyLength = (CsrUint16) value.size;
           CsrMemCpy(pCtx->networkKey, value.pStart, value.size);

           id = getDataElement(pCredential, &value);
       }

       if(WPS_MAC_ADDRESS != id)
       {
           sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: required MAC Address missing"));
           break;
       }

       /* Check for any optional data elements */
       for(id = getDataElement(pCredential, &value); id != 0; id = getDataElement(pCredential, &value))
       {
           sme_trace_info((TR_SECURITY_LIB, "Encrypted Settings Data: Optional (%04x)", id));
       }

       /* Everything present and correct if no break by this point */
       result = TRUE;
    }
    while(0);

    return result;
}
