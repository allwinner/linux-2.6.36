/*******************************************************************************

                (c) Cambridge Silicon Radio Limited 2009

                All rights reserved and confidential information of CSR

                WARNING: This is an auto-generated file! Do NOT edit!

REVISION:       $Revision: #1 $

*******************************************************************************/

#include "nmeio/nmeio_free_message_contents.h"


void free_unifi_eapcredentials_contents(const unifi_EapCredentials* pType)
{
    CsrPfree(pType->authServerUserIdentity);
    CsrPfree(pType->username);
    CsrPfree(pType->password);
    CsrPfree(pType->session);
}

void free_unifi_fastcredentials_contents(const unifi_FastCredentials* pType)
{
    free_unifi_eapcredentials_contents((const unifi_EapCredentials*)&(pType->eapCredentials));
    CsrPfree(pType->pac);
}

void free_unifi_leapcredentials_contents(const unifi_LeapCredentials* pType)
{
    CsrPfree(pType->username);
    CsrPfree(pType->password);
}

void free_unifi_passphrase_contents(const unifi_Passphrase* pType)
{
    CsrPfree(pType->passphrase);
}

void free_unifi_tlscertificate_contents(const unifi_TlsCertificate* pType)
{
    CsrPfree(pType->username);
    CsrPfree(pType->certificate);
    CsrPfree(pType->privateKey);
    CsrPfree(pType->session);
}

void free_unifi_usernameandpassword_contents(const unifi_UsernameAndPassword* pType)
{
    CsrPfree(pType->username);
    CsrPfree(pType->password);
}

void free_unifi_wapicredentials_contents(const unifi_WapiCredentials* pType)
{
    CsrPfree(pType->certificate);
    CsrPfree(pType->privateKey);
    CsrPfree(pType->caCertificate);
}

void free_unifi_wpspin_contents(const unifi_WpsPin* pType)
{
    CsrPfree(pType->pin);
}

void free_unifi_credentials_contents(const unifi_Credentials* pType)
{
    switch(pType->credentialType)
    {
    case unifi_CredentialTypeWapiPassphrase:
        free_unifi_passphrase_contents((const unifi_Passphrase*) &(pType->credential.wapiPassphrase));
        break;
    case unifi_CredentialType8021xLeap:
        free_unifi_leapcredentials_contents((const unifi_LeapCredentials*) &(pType->credential.leap));
        break;
    case unifi_CredentialTypeWpaPassphrase:
        free_unifi_passphrase_contents((const unifi_Passphrase*) &(pType->credential.wpaPassphrase));
        break;
    case unifi_CredentialType8021xPeapGtc:
        free_unifi_eapcredentials_contents((const unifi_EapCredentials*) &(pType->credential.peapGtc));
        break;
    case unifi_CredentialType8021xTls:
        free_unifi_tlscertificate_contents((const unifi_TlsCertificate*) &(pType->credential.tls));
        break;
    case unifi_CredentialType8021xFast:
        free_unifi_fastcredentials_contents((const unifi_FastCredentials*) &(pType->credential.fast));
        break;
    case unifi_CredentialTypeWpa2Passphrase:
        free_unifi_passphrase_contents((const unifi_Passphrase*) &(pType->credential.wpa2Passphrase));
        break;
    case unifi_CredentialType8021xTtls:
        free_unifi_eapcredentials_contents((const unifi_EapCredentials*) &(pType->credential.ttls));
        break;
    case unifi_CredentialTypeWapi:
        free_unifi_wapicredentials_contents((const unifi_WapiCredentials*) &(pType->credential.wapi));
        break;
    case unifi_CredentialType8021xPeapMschapv2:
        free_unifi_eapcredentials_contents((const unifi_EapCredentials*) &(pType->credential.peapMsChapV2));
        break;
    default:
        break;
    };
}

void free_unifi_profile_contents(const unifi_Profile* pType)
{
    free_unifi_credentials_contents((const unifi_Credentials*)&(pType->credentials));
}

void free_unifi_nme_certificate_validate_ind_contents(const UnifiNmeCertificateValidateInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
    CsrPfree(pType->certificate);
}

void free_unifi_nme_profile_connect_cfm_contents(const UnifiNmeProfileConnectCfm_Evt* pType)
{
    CsrPfree(pType->connectAttempts);
}

void free_unifi_nme_profile_disconnect_ind_contents(const UnifiNmeProfileDisconnectInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
    CsrPfree(pType->connectAttempts);
}

void free_unifi_nme_profile_order_set_req_contents(const UnifiNmeProfileOrderSetReq_Evt* pType)
{
    CsrPfree(pType->profileIdentitys);
}

void free_unifi_nme_profile_set_req_contents(const UnifiNmeProfileSetReq_Evt* pType)
{
    free_unifi_profile_contents((const unifi_Profile*)&(pType->profile));
}

void free_unifi_nme_profile_update_ind_contents(const UnifiNmeProfileUpdateInd_Evt* pType)
{
    CsrPfree(pType->appHandles);
    free_unifi_profile_contents((const unifi_Profile*)&(pType->profile));
}

void free_unifi_nme_wps_cfm_contents(const UnifiNmeWpsCfm_Evt* pType)
{
    free_unifi_profile_contents((const unifi_Profile*)&(pType->profile));
}

void nmeio_free_message_contents(const FsmEvent* evt)
{
    switch(evt->id)
    {
    case UNIFI_NME_CERTIFICATE_VALIDATE_IND_ID:
        free_unifi_nme_certificate_validate_ind_contents((UnifiNmeCertificateValidateInd_Evt*)evt);
        break;
    case UNIFI_NME_PROFILE_CONNECT_CFM_ID:
        free_unifi_nme_profile_connect_cfm_contents((UnifiNmeProfileConnectCfm_Evt*)evt);
        break;
    case UNIFI_NME_PROFILE_DISCONNECT_IND_ID:
        free_unifi_nme_profile_disconnect_ind_contents((UnifiNmeProfileDisconnectInd_Evt*)evt);
        break;
    case UNIFI_NME_PROFILE_ORDER_SET_REQ_ID:
        free_unifi_nme_profile_order_set_req_contents((UnifiNmeProfileOrderSetReq_Evt*)evt);
        break;
    case UNIFI_NME_PROFILE_SET_REQ_ID:
        free_unifi_nme_profile_set_req_contents((UnifiNmeProfileSetReq_Evt*)evt);
        break;
    case UNIFI_NME_PROFILE_UPDATE_IND_ID:
        free_unifi_nme_profile_update_ind_contents((UnifiNmeProfileUpdateInd_Evt*)evt);
        break;
    case UNIFI_NME_WPS_CFM_ID:
        free_unifi_nme_wps_cfm_contents((UnifiNmeWpsCfm_Evt*)evt);
        break;
    default:
        break;
    }
}



