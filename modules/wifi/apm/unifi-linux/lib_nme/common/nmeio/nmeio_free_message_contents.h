/*******************************************************************************

                (c) Cambridge Silicon Radio Limited 2009

                All rights reserved and confidential information of CSR

                WARNING: This is an auto-generated file! Do NOT edit!

REVISION:       $Revision: #1 $

*******************************************************************************/
#ifndef NMEIO_FREE_MESSAGE_CONTENTS_H
#define NMEIO_FREE_MESSAGE_CONTENTS_H

#include "nme_top_level_fsm/nme_top_level_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void free_unifi_eapcredentials_contents(const unifi_EapCredentials* pType);
extern void free_unifi_fastcredentials_contents(const unifi_FastCredentials* pType);
extern void free_unifi_leapcredentials_contents(const unifi_LeapCredentials* pType);
extern void free_unifi_passphrase_contents(const unifi_Passphrase* pType);
extern void free_unifi_tlscertificate_contents(const unifi_TlsCertificate* pType);
extern void free_unifi_usernameandpassword_contents(const unifi_UsernameAndPassword* pType);
extern void free_unifi_wapicredentials_contents(const unifi_WapiCredentials* pType);
extern void free_unifi_wpspin_contents(const unifi_WpsPin* pType);
extern void free_unifi_credentials_contents(const unifi_Credentials* pType);
extern void free_unifi_profile_contents(const unifi_Profile* pType);
extern void free_unifi_nme_certificate_validate_ind_contents(const UnifiNmeCertificateValidateInd_Evt* evt);
extern void free_unifi_nme_profile_connect_cfm_contents(const UnifiNmeProfileConnectCfm_Evt* evt);
extern void free_unifi_nme_profile_disconnect_ind_contents(const UnifiNmeProfileDisconnectInd_Evt* evt);
extern void free_unifi_nme_profile_order_set_req_contents(const UnifiNmeProfileOrderSetReq_Evt* evt);
extern void free_unifi_nme_profile_set_req_contents(const UnifiNmeProfileSetReq_Evt* evt);
extern void free_unifi_nme_profile_update_ind_contents(const UnifiNmeProfileUpdateInd_Evt* evt);
extern void free_unifi_nme_wps_cfm_contents(const UnifiNmeWpsCfm_Evt* evt);
extern void nmeio_free_message_contents(const FsmEvent* evt);


#ifdef __cplusplus
}
#endif

#endif
