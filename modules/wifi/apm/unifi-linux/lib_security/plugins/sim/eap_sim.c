/** @file eap_sim.c
 *
 * Implementation of the EAP-SIM method.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   This provides an implementation of the EAP-SIM authentication protocol.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/sim/eap_sim.c#1 $
 *
 ****************************************************************************/

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "csr_crypto.h"
#include "csr_security_private.h"
#include "csr_des/csr_des.h"
#include "csr_sha1/csr_sha1.h"
#include "csr_hmac/csr_hmac.h"
#include "csr_aes/csr_aes128_cbc.h"
#include "csr_bn/csr_bn.h"

#include "plugins/security_method.h"
#include "eap_sim.h"

/* PRIVATE MACROS *************************************************************/

#define EAP_SIM_PERMANENT_ID_LEN 15


/* Global Definitions *********************************************************/

CsrUint8 EAP_SIM_PERMANENT_ID[EAP_SIM_PERMANENT_ID_LEN] =  {2,3,2,0,1,0,0,0,0,0,0,0,0,0,0};
char EAP_SIM_PERMANENT_ID_ASCII[EAP_SIM_PERMANENT_ID_LEN] =  {'2','3','2','0','1','0','0','0','0','0','0','0','0','0','0'};

CsrUint8 hardCodedRand_0[GSM_RAND_LEN] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };
CsrUint8 hardCodedRand_1[GSM_RAND_LEN] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f };
CsrUint8 hardCodedRand_2[GSM_RAND_LEN] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f };

CsrUint8 hardCodedSRES_0[EAP_SIM_SRES_LEN] = { 0xd1, 0xd2, 0xd3, 0xd4};
CsrUint8 hardCodedSRES_1[EAP_SIM_SRES_LEN] = { 0xe1, 0xe2, 0xe3, 0xe4};
CsrUint8 hardCodedSRES_2[EAP_SIM_SRES_LEN] = { 0xf1, 0xf2, 0xf3, 0xf4};

CsrUint8 hardCodedKc_0[EAP_SIM_KC_LEN] = { 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7};
CsrUint8 hardCodedKc_1[EAP_SIM_KC_LEN] = { 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7};
CsrUint8 hardCodedKc_2[EAP_SIM_KC_LEN] = { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7};

/* TYPE DEFINITIONS ***********************************************************/

/* This is the control structure for the EAP-SIM state machine. */
typedef struct SimContext
{
    CsrWifiSecurityContext* securityContext;
    /* CsrSimState state; */

    /* All the generic EAP-SIM parameters go in here. */
    CsrSimData simData;

    /* All the current configurable EAP-SIM Option go in here */
    CsrSimConfig simConfig;

    eapKey key;

    DataBuffer dataref;

    CsrUint8 eapType;

} CsrSimContext;

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

static void eapSIM_init( CsrSimContext* simContext );

static void eapSIM_deinit( CsrSimContext* simContext );

static eEAP_SIM_RESULTS eapSIM_process( eapMethod* eap_method,
                                        CsrSimContext* simContext,
                                        eap_packet *reqPkt,
                                        eapol_packet *eapolRspPkt,
                                        CsrUint16 *eapRspPktLen );

static eEAP_SIM_RESULTS eapSIM_start( eapMethod* eap_method,
                                      CsrSimContext *simContext,
                                      eap_packet *reqData,
                                      CsrUint16 reqDataLen,
                                      eapol_packet *eapolRsp,
                                      CsrUint16 *eapRspLen );

static eEAP_SIM_RESULTS eapSIM_challengePre( eapMethod* eap_method,
                                             CsrSimContext *simContext,
                                             eap_packet *reqData,
                                             CsrUint16 reqDataLen,
                                             eapol_packet *eapolRsp,
                                             CsrUint16 *eapRspLen );

static eEAP_SIM_RESULTS eapSIM_challengePost( eapMethod* eap_method,
                                              CsrSimContext *simContext,
                                              eap_packet *reqData,
                                              CsrUint16 reqDataLen,
                                              eapol_packet *eapolRsp,
                                              CsrUint16 *eapRspLen );

static eEAP_SIM_RESULTS eapSIM_notification( eapMethod* eap_method,
                                             CsrSimContext *simContext,
                                             eap_packet *reqData,
                                             CsrUint16 reqDataLen,
                                             eapol_packet *eapolRsp,
                                             CsrUint16 *eapRspLen );

static eEAP_SIM_RESULTS eapSIM_reauthentication( eapMethod* eap_method,
                                                 CsrSimContext *simContext,
                                                 eap_packet *reqData,
                                                 CsrUint16 reqDataLen,
                                                 eapol_packet *eapolRsp,
                                                 CsrUint16 *eapRspLen );

static void eapSIM_clientError( CsrSimContext *simContext,
                                eEapSimErrorCodes errCode,
                                eapol_packet *eapolRsp,
                                CsrUint16 *eapErrRspLen );

static CsrUint16 increment_rand_history_index( CsrUint16 currentIndex );

static eEAP_SIM_RAND_RESULTS eapSIM_validateReceivedRANDs( CsrSimContext *simContext,
                                                           CsrUint8 numChallenges,
                                                           CsrUint8 *RandsPtr );

static eEAP_SIM_RESULTS eapSIM_calculateGSMkeys( CsrSimContext *simContext );

static eEAP_SIM_RESULTS eapSIM_getKcAndSRES( CsrUint8* inRand,
                                             CsrUint8* outKc,
                                             CsrUint8* outSRES );

static eEAP_SIM_RESULTS eapSIM_calculateMasterKey( CsrSimContext* simContext,
                                                   const CsrUint8 *identity,
                                                   const CsrUint16 identityLen );

static eEAP_SIM_RESULTS eapSIM_calculateFurtherKeys( CsrSimContext *simContext );

static eEAP_SIM_RESULTS eapSIM_calculateMasterKeyReauth( CsrSimContext* simContext,
                                                         const CsrUint8 *identity,
                                                         CsrUint16 identityLen );

static eEAP_SIM_RESULTS eapSIM_calculateFurtherKeysReauth( CsrSimContext *simContext );

static void pseudoRandFunctionFIPS186_2_changeNotice( const CsrUint8 *seedKey,
                                                      const CsrUint16 seedKeyLen,
                                                      CsrUint8 *x,
                                                      CsrUint16 xLen);

static eEAP_SIM_ATT_RESULTS eapSIM_findAndProcessAtMAC( CsrSimContext *simContext,
                                                        eap_packet *reqData,
                                                        CsrUint16 reqDataLen,
                                                        CsrUint8  msgSubType );

static eEAP_SIM_ATT_RESULTS eapSIM_findAndProcessAtIvAtEncrData( CsrSimContext *simContext,
                                                                 eap_packet *reqData,
                                                                 CsrUint16 reqDataLen,
                                                                 CsrUint8  msgSubType );

static eEAP_SIM_ATT_RESULTS eapSIM_findAndProcessAtResultInd( CsrSimContext *simContext,
                                                              eap_packet *reqData,
                                                              CsrUint16 reqDataLen,
                                                              CsrUint8  msgSubType );

static eEAP_SIM_RESULTS eapSIM_deriveMAC( const CsrUint8 *k_aut,
                                          const CsrUint8 *msg,
                                          CsrUint16 msgLen,
                                          CsrUint8 *mac,
                                          const CsrUint8 *moreData,
                                          CsrUint16 moreDataLen );

static eEAP_SIM_ATT_RESULTS eapSIM_checkReceivedMAC( const CsrUint8 *k_aut,
                                                     eap_packet *reqData,
                                                     CsrUint16 reqDataLen,
                                                     const CsrUint8 *mac,
                                                     const CsrUint8 *moreData,
                                                     CsrUint16 moreDataLen );

static eEAP_SIM_RESULTS eapSIM_InsertAtIvAtEncrData( CsrSimContext *simContext,
                                                     CsrUint8  *respPkt,
                                                     CsrUint16 *respPktLen,
                                                     CsrUint8  *current_att_Offset,
                                                     CsrUint8  msgSubType );

static void eapSIM_AesEncrypt( CsrSimData *myData,
                               const CsrUint8 *k_encr,
                               const CsrUint8 *inData,
                               const CsrUint16 inDataLen,
                               CsrUint8 *outData,
                               const CsrUint8 *iv );

static eEAP_SIM_RESULTS eapSIM_AesDecrypt( CsrSimData *myData,
                                           const CsrUint8 *k_encr,
                                           const CsrUint8 *encr_data,
                                           CsrUint16 encr_data_len,
                                           const CsrUint8 *iv,
                                           CsrUint8  msgSubType );

static eEAP_SIM_RESULTS eapSIM_storeNextPseudonym( CsrSimData *myData,
                                                   CsrUint8 *nextPseud,
                                                   CsrUint16 nextPseudLen );

static eEAP_SIM_RESULTS eapSIM_storeNextReauthId( CsrSimData *myData,
                                                  CsrUint8 *nextId,
                                                  CsrUint16 nextIdLen );

static CsrBool eapSIM_checkPadding( CsrUint8 *buffer,
                                    CsrUint8 BufferLen );

static CsrBool eapSIM_isNonSkippableAttribute( CsrUint8 attribId );

static CsrBool eapSIM_isPhaseBitZero( CsrUint16 notifxCode );

static CsrUint16 form16bitFrom2x8bit_se( CsrUint8 msbyte,
                                         CsrUint8 lsbyte );

void form2x8bitsFrom16bit( CsrUint16 num16,
                           CsrUint8 *lsbNum,
                           CsrUint8 *msbNum );

/* PRIVATE FUNCTION DEFINITIONS **********************************************/

static void eapSIM_init( CsrSimContext* simContext )

{
    CsrSimData *myData = &simContext->simData;
    CsrSimConfig *myConfig = &simContext->simConfig;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_init()" ));

    /* Optional Features */
    myConfig->protectedResultIndConfigured = ( EAP_SIM_STA_SUPPORTS_RESULTS_IND ? TRUE : FALSE);
    myConfig->anonymityOn                  = ( EAP_SIM_STA_SUPPORTS_ANONYMITY   ? TRUE : FALSE);
    myConfig->fastReauthOn                 = ( EAP_SIM_STA_SUPPORTS_FAST_REAUTH ? TRUE : FALSE);
    myConfig->randHistoryOn                = ( EAP_SIM_STA_SUPPORTS_RAND_HISTORY ? TRUE : FALSE);

    /* Initialise various variables */
    myData->numEapSimStartRounds = 0;
    myData->numNotificationRounds = 0;
    myData->numIdReqs = 0;
    myData->networkSentAtResultInd = FALSE;
    /*myData->randHistoryIndex = 0; */

    myData->lastReceivedPacket = EAP_SIM_NO_PACKET_RECEIVED;

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_init()"));
}

static void eapSIM_deinit( CsrSimContext* simContext )
{
    CsrSimData *myData = &(simContext->simData);

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_deinit()" ));

    myData->numEapSimStartRounds =0;
    CsrPfree( myData->versionList );
    CsrPfree( myData->pseudonym );
    CsrPfree( myData->reauthId );

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_deinit()" ));
}

/* ************************************************************************ */
/* ************************************************************************ */
/*                                                                          */
/*                  EAP-SIM MESSAGES PROCESSING FUNCTIONS                   */
/*                                                                          */
/* ************************************************************************ */
/* ************************************************************************ */

static eEAP_SIM_RESULTS eapSIM_start( eapMethod* eap_method,
                                      CsrSimContext *simContext,
                                      eap_packet *reqData,
                                      CsrUint16 reqDataLen,
                                      eapol_packet *eapolRsp,
                                      CsrUint16 *eapRspLen )
{
    CsrSimData *myData = &simContext->simData;
    CsrSimConfig *myConfig = &simContext->simConfig;

    /* Variables For Request */
    CsrUint8 * current_att_offset;
    CsrUint16 remaining_atts_len;
    CsrUint8 current_att_type, current_att_lenField, current_att_len;
    CsrUint8 numIdReqs=0;

    /* Variables For Response */
    CsrUint8 lsbNUM, msbNUM;
    CsrUint8 currentAttOffset=0;
    CsrUint8 *eapRspPacket = NULL;
    eID_TO_SEND whichID;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_start()" ));

    /* Skip SNAP HEADER and EAPOL HEADER (filled ouside) */
    eapRspPacket = (CsrUint8 *) eapolRsp;
    eapRspPacket += CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH;

    myData->numEapSimStartRounds +=1;
    if ( myData->numEapSimStartRounds > EAP_SIM_MAX_START_ROUNDS )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_start(): Max Number (%d) of EAP-Request/SIM/Start Rounds exceeded",
            EAP_SIM_MAX_START_ROUNDS ));

        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* Parse Attributes in EAP-Request/SIM/Start packet (assuming any order) */
    remaining_atts_len = reqDataLen - (CsrUint16) IE_EAP_SIM_PACKET__HEADER_LEN;
    current_att_offset = ((CsrUint8 *)reqData+IE_EAP_SIM__FIRST_ATTRIBUTE_OFFSET);
    while (remaining_atts_len > 0 )
    {
        current_att_type = *current_att_offset;
        current_att_lenField  = *(current_att_offset + 1);
        current_att_len = (CsrUint8) (4 * current_att_lenField);

        if ( current_att_type == EAP_SIM_AT_VERSION_LIST )
        {
            CsrUint16 ActualVersionListLen, thisVersion;
            CsrUint8 vIndex;
            CsrBool verFound=FALSE;

            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_start(): EAP-Request/SIM/Start (AT_VERSION_LIST) =",
                current_att_offset, current_att_len ));

            /* Number of Versions listed by network */
            ActualVersionListLen = form16bitFrom2x8bit_se( *( current_att_offset + 2),
            *( current_att_offset + 3) );

            for (vIndex=0 ; vIndex<ActualVersionListLen ; vIndex +=2)
            {
                thisVersion = form16bitFrom2x8bit_se( *(current_att_offset+4+vIndex),
                                                      *(current_att_offset+5+vIndex) );

                if ( thisVersion == (CsrUint16) EAP_SIM_VERSION )
                {
                    verFound=TRUE;
                    /* Store the selected Version */
                    myData->selectedVersion = thisVersion;
                    break;
                }

            }

            if ( ! verFound )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_start(): The STA does not support any of the versions list in AT_VERSION_LIST" ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNSUPPORTED_VERSION, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }

            sme_trace_debug(( TR_SECURITY,
                "eapSIM_start(): AT_VERSION_LIST found EAP-Request/SIM/Start which contains our current version" ));

            /* Store the received Version List */
            myData->versionListLen = (CsrUint8) ActualVersionListLen;
            myData->versionList = (CsrUint8 *) CsrPmalloc( sizeof(CsrUint8) * ActualVersionListLen);
            CsrMemCpy ( myData->versionList, current_att_offset+4, ActualVersionListLen);

            /* update num of bytes still needing to be parsed... */
            remaining_atts_len -= current_att_len;
        }
        else if ( current_att_type == EAP_SIM_AT_PERMANENT_ID_REQ )
        {
            numIdReqs++;
            sme_trace_debug(( TR_SECURITY, "eapSIM_start(): AT_PERMANENT_ID_REQ requestd by network" ));
            if ( current_att_len != IE_EAP_SIM__AT_PERMANENT_ID_REQ_LEN )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_start(): Invalid Length Field Value (%u) of AT_PERMANENT_ID_REQ (should be %u)",
                     current_att_lenField, IE_EAP_SIM__AT_PERMANENT_ID_REQ_LEN_FIELD ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }

            /* Note that network has requested the Permanenent ID */
            myData->idTypeReqByNetwork = PERMANENT_TYPE;

            remaining_atts_len -= IE_EAP_SIM__AT_PERMANENT_ID_REQ_LEN;

        }
        else if ( current_att_type == EAP_SIM_AT_FULLAUTH_ID_REQ )
        {
            numIdReqs++;
            sme_trace_debug(( TR_SECURITY, "eapSIM_start(): AT_FULLAUTH_ID_REQ requestd by network" ));
            if ( current_att_len != IE_EAP_SIM__AT_FULLAUTH_ID_REQ_LEN )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_start(): Invalid Length Field Value (%u) of AT_FULLAUTH_ID_REQ (should be %u)",
                    current_att_lenField,
                    IE_EAP_SIM__AT_PERMANENT_ID_REQ_LEN_FIELD ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }

            /* Note that network has requested a Full Athentication ID */
            myData->idTypeReqByNetwork =  FULLAUTH_TYPE;

            remaining_atts_len -= IE_EAP_SIM__AT_FULLAUTH_ID_REQ_LEN;
        }
        else if ( current_att_type == EAP_SIM_AT_ANY_ID_REQ )
        {
            numIdReqs++;
            sme_trace_debug(( TR_SECURITY, "eapSIM_start(): AT_ANY_ID_REQ requestd by network" ));
            if ( myData->numEapSimStartRounds >1 )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_start(): AT_ANY_ID_REQ can only be used in First EAP/SIM/Start Round we are in round (%u)",
                    myData->numEapSimStartRounds ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }

            if ( current_att_len != IE_EAP_SIM__AT_ANY_ID_REQ_LEN )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_start(): Invalid Length Field Value (%u) of AT_ANY_ID_REQ (should be %u)",
                    current_att_lenField,
                    IE_EAP_SIM__AT_ANY_ID_REQ_LEN_FIELD ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }

            /* Note that network has requested any ID */
            myData->idTypeReqByNetwork = ANY_TYPE;

            remaining_atts_len -= IE_EAP_SIM__AT_ANY_ID_REQ_LEN;
        }
        else if ( eapSIM_isNonSkippableAttribute (current_att_type) )
        {
            sme_trace_error(( TR_SECURITY,
                "eapSIM_start(): Unexpected Non-Skippable Attribute (%u) found in EAP-Request/SIM/Start",
                current_att_type ));

            eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
            return eapSIM_FAILURE;
        }
        else
        {
            /* No failure Just ignore */
            sme_trace_debug(( TR_SECURITY,
                "eapSIM_start(): Unexpected Skippable Attribute (%u) found in EAP-Request/SIM/Start: Ignored",
                current_att_type ));
        }

        current_att_offset += current_att_len;
    }

    if ( numIdReqs>1 )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_start(): Too many Identity Requests (%u) in one EAP-Request/SIM/Start",
            numIdReqs ));

        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    if ( numIdReqs==0 )
    {
        /* Note that network has requested No ID */
        myData->idTypeReqByNetwork = NO_TYPE;
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_start(): No ID was requestd by network(in EAP-Request/SIM/Start" ));
    }

    myData->lastReceivedPacket = EAP_SIM_START_REQ;

    /*******************************
    *
    * Generate the Response Message
    *
    ******************************/

    sme_trace_debug(( TR_SECURITY, "eapSIM_start(): GENERATING the EAP-Response/SIM/Start Message" ));

    /* But fist decide what type of Identity to send (depending on what network has asked for and what is available */
    switch ( myData->idTypeReqByNetwork )
    {
        case ANY_TYPE:
            if ( myData->reauthId && myConfig->fastReauthOn )
            {
                whichID = SEND_FAST_REAUTH_ID;
            }
            else if ( myData->pseudonym && myConfig->anonymityOn )
            {
                whichID = SEND_PSEUDONYM;
            }
            else
            {
                whichID = SEND_PERMANENT_ID;
            }
            break;

        case FULLAUTH_TYPE:
            if ( myData->pseudonym && myConfig->anonymityOn )
            {
                whichID = SEND_PSEUDONYM;
            }
            else
            {
                whichID = SEND_PERMANENT_ID;
            }
            break;

        case PERMANENT_TYPE:
            whichID = SEND_PERMANENT_ID;
            break;

        case NO_TYPE:
            whichID = SEND_NO_ID;
            break;

        default:
        {
            /* should not reach this */
            sme_trace_error(( TR_SECURITY,
            "eapSIM_start(): Unexpected error while processing EAP-Request/SIM/Start" ));

            eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
            return eapSIM_FAILURE;
        }
    }

    /* skip EAP-HEADER (code, id, Length(2 Bytes), Type); (filled outside ) */
    currentAttOffset = (CsrUint8) IE_EAP__HEADER_LEN;
    *eapRspLen = IE_EAP__HEADER_LEN;

    /* Subtype */
    *(eapRspPacket + currentAttOffset)  = (CsrUint8) EAP_SIM_SUBTYPE_START;
    /* Reserved */
    *(eapRspPacket + currentAttOffset + 1) = (CsrUint8) 0;
    *(eapRspPacket + currentAttOffset + 2) = (CsrUint8) 0;

    *eapRspLen += IE_EAP_SIM__HEADER_LEN;
    currentAttOffset += (CsrUint8) IE_EAP_SIM__HEADER_LEN;

    /* Generate Nonce (and Store it) */
    CsrWifiSecurityRandom( (CsrUint8*)myData->nonce_mt, (CsrUint32) EAP_SIM_NONCE_MT_LEN);
    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_start(): generated nonce_mt =", myData->nonce_mt, EAP_SIM_NONCE_MT_LEN ));

    /* And Now the Attributes */
    if (whichID != SEND_FAST_REAUTH_ID)
    {
        /* AT_SELECTED_VERSION =TLV */
        *(eapRspPacket+currentAttOffset) = (CsrUint8) EAP_SIM_AT_SELECTED_VERSION; /* T */
        *(eapRspPacket+currentAttOffset+1) = (CsrUint8) IE_EAP_SIM__AT_SELECTED_VERSION_LEN_FIELD;  /* L */
        *(eapRspPacket+currentAttOffset+2) = 0x00 ;  /* V */
        *(eapRspPacket+currentAttOffset+3) = 0x01 ;  /* V */

        *eapRspLen += IE_EAP_SIM__AT_SELECTED_VERSION_LEN;
        currentAttOffset += IE_EAP_SIM__AT_SELECTED_VERSION_LEN;

        sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_start(): EAP-Response/SIM/Start (AT_SELECTED_VERSION) =",
        eapRspPacket, *eapRspLen ));

        /* AT_NONCE_MT = TLV */
        *(eapRspPacket+currentAttOffset) = EAP_SIM_AT_NONCE_MT; /* T */
        *(eapRspPacket+currentAttOffset+1) = IE_EAP_SIM__AT_NONCE_MT_LEN_FIELD;  /* L */
        *(eapRspPacket+currentAttOffset+2) = 0x00;  /* V (reserved) */
        *(eapRspPacket+currentAttOffset+3) = 0x00;  /* V (reserved) */
        CsrMemCpy(eapRspPacket+currentAttOffset+4, (CsrUint8 *) &(myData->nonce_mt[0]), EAP_SIM_NONCE_MT_LEN); /* V */

        *eapRspLen += IE_EAP_SIM__AT_NONCE_MT_LEN;
    }
    else
    {
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_start(): AT_SELECTED_VERSION and AT_NONCE_MT have Not been included in EAP-Response/SIM/Start" ));
    }

    /* AT_IDENTITY = TLV */
    if (whichID > SEND_NO_ID)
    {
        CsrUint8 *id2Send;
        CsrUint8 id2SendLen;
        CsrUint8 paddingLen;
        CsrUint8 id2SendLenLSB, id2SendLenMSB ;
        CsrUint8 i;

        switch (whichID)
        {
            case SEND_FAST_REAUTH_ID:
                id2Send = myData->reauthId ;
                id2SendLen = (CsrUint8) myData->reauthIdLen;
                sme_trace_debug(( TR_SECURITY,
                    "eapSIM_start(): Including Fast ReAuth-Id in EAP-Response/SIM/Start" ));
                break;

            case SEND_PSEUDONYM:
                id2Send = myData->pseudonym;
                id2SendLen = (CsrUint8) myData->pseudonymLen;
                sme_trace_debug(( TR_SECURITY,
                    "eapSIM_start(): Including Pseudonym in EAP-Response/SIM/Start" ));
                break;

            case SEND_PERMANENT_ID:
                /* id2Send = EAP_SIM_PERMANENT_ID; */
                id2Send = (CsrUint8 *)EAP_SIM_PERMANENT_ID_ASCII;
                id2SendLen = EAP_SIM_PERMANENT_ID_LEN;
                sme_trace_debug(( TR_SECURITY,
                    "eapSIM_start(): Including Permanent-Id in EAP-Response/SIM/Start" ));
                break;

            default:
            {
                /* should not reach this */
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_start(): NO suitable Id has been found to include in EAP-Response/SIM/Start" ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }
        }

        sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_start(): ID to Send =", id2Send, id2SendLen ));

        currentAttOffset += IE_EAP_SIM__AT_NONCE_MT_LEN;
        *(eapRspPacket+currentAttOffset) = EAP_SIM_AT_IDENTITY; /* T */

        /* L */
        paddingLen = (4 - (id2SendLen %4 )) % 4;
        *(eapRspPacket+currentAttOffset+1) = (4+id2SendLen + paddingLen)/4;

        /* V */
        form2x8bitsFrom16bit( (CsrUint16) id2SendLen, &id2SendLenLSB, &id2SendLenMSB );
        *(eapRspPacket+currentAttOffset+2) = id2SendLenMSB;
        *(eapRspPacket+currentAttOffset+3) = id2SendLenLSB;

        CsrMemCpy ( eapRspPacket+currentAttOffset+4, id2Send, id2SendLen);
        for (i=1; i<=paddingLen ; i++)
        {
            *(eapRspPacket+currentAttOffset+4+id2SendLen+i) = 0x00;
        }

        *eapRspLen += 4+id2SendLen+paddingLen;
    }
    else
    {
        sme_trace_info(( TR_SECURITY,
        "eapSIM_start(): AT_IDENTITY has NOT been included in EAP-Response/SIM/Start" ));
    }

    /* all the eap Header fields have been set ouside except the length which needed calculating here
     * hence length needs to be set here */
    form2x8bitsFrom16bit( *eapRspLen, &lsbNUM, &msbNUM );
    sme_trace_debug((TR_SECURITY_LIB, "Resp_Len = %d", *eapRspLen));

    *(eapRspPacket+2) = msbNUM;
    *(eapRspPacket+3) = lsbNUM;

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_start(): EAP-Response/SIM/Start =",
        eapRspPacket, *eapRspLen ));

    /* Update methodeState and Decision */
    eap_method->methodState = MAY_CONT;
    eap_method->decision    = COND_SUCC;

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_start()" ));

    return eapSIM_SUCCESS;
}


static eEAP_SIM_RESULTS eapSIM_challengePre( eapMethod* eap_method,
                                             CsrSimContext *simContext,
                                             eap_packet *reqData,
                                             CsrUint16 reqDataLen,
                                             eapol_packet *eapolRsp,
                                             CsrUint16 *eapRspLen )
{
    CsrSimData *myData = &simContext->simData;

    CsrUint8 *current_att_offset;
    CsrUint16 remaining_atts_len;
    CsrUint8 current_att_type, current_att_lenField, current_att_len ;
    eEAP_SIM_RAND_RESULTS randRes=eapSIM_RAND_NOT_FOUND;
    CsrUint8 numReceivedRands=0;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_challengePre()" ));

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_challengePre(): EAP-Request/SIM/Challenge Packet = ",
        reqData, reqDataLen ));

    myData->numEapSimStartRounds =0;
    myData->numFastReAuthRounds = 0;

    /* *** AT_RAND *** */
    remaining_atts_len = reqDataLen - (CsrUint16) IE_EAP_SIM_PACKET__HEADER_LEN;
    current_att_offset = ((CsrUint8 *)reqData+IE_EAP_SIM__FIRST_ATTRIBUTE_OFFSET);
    while ( remaining_atts_len > 0 )
    {
        current_att_type = *current_att_offset;
        current_att_lenField  = *(current_att_offset + 1);
        current_att_len = (CsrUint8) (4 * current_att_lenField);

        if ( current_att_type == EAP_SIM_AT_RAND )
        {
            randRes = eapSIM_RAND_INVALID;

            numReceivedRands = (current_att_len - IE_EAP_SIM__ATT_GEN_HEADER_LEN)/GSM_RAND_LEN;

            sme_trace_debug(( TR_SECURITY,
                "eapSIM_challengePre(): %u RANDs have been received",
                numReceivedRands ));

            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_challengePre(): First RAND= ",
                current_att_offset+IE_EAP_SIM__ATT_GEN_HEADER_LEN,
                GSM_RAND_LEN ));

            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_challengePre(): Second RAND= ",
                current_att_offset+IE_EAP_SIM__ATT_GEN_HEADER_LEN+GSM_RAND_LEN,
                GSM_RAND_LEN ));

            if ( numReceivedRands == EAP_SIM_MAX_NUM_CHALLENGES )
            {
                sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                    "eapSIM_challengePre(): Third RAND= ",
                    current_att_offset+IE_EAP_SIM__ATT_GEN_HEADER_LEN + 2* GSM_RAND_LEN ,
                    GSM_RAND_LEN ));
            }

            /* Check if correct number of Challenges have been received and they are all different */
            randRes = eapSIM_validateReceivedRANDs( simContext,
                                                    numReceivedRands,
                                                    current_att_offset+IE_EAP_SIM__ATT_GEN_HEADER_LEN );
           break;
        }

        current_att_offset += current_att_len;
        remaining_atts_len -= current_att_len;
    }

    /* Failure If AT_RAND attribute NOT Found or NOT Valid */
    if ( randRes == eapSIM_RAND_NOT_FOUND )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_challengePre(): Mandatory Attribute AT_RAND not found in EAP-Request/SIM/Challenge" ));

        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }
    else if ( randRes == eapSIM_RAND_INVALID )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }
    else if ( randRes == eapSIM_RAND_INSUFFICIENT )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_INSUFFICIENT_NUM_OF_CHAL, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }
    else if ( randRes == eapSIM_RAND_NOT_FRESH )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_RAND_NOT_FRESH , eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /*
     * Otherwise Continue...
     */

    /* Store Num of Challenges */
    myData->numRANDs = numReceivedRands;

    /* Store first Challenge */
    CsrMemCpy( myData->gsmTriplet[0].RAND,
               current_att_offset+IE_EAP_SIM__ATT_GEN_HEADER_LEN,
               GSM_RAND_LEN);

    /* Store Second Challenge */
    CsrMemCpy( myData->gsmTriplet[1].RAND,
               current_att_offset+IE_EAP_SIM__ATT_GEN_HEADER_LEN + GSM_RAND_LEN,
               GSM_RAND_LEN);

    /* And if there a Third challenge store it */
    if (numReceivedRands == EAP_SIM_MAX_NUM_CHALLENGES)
    {
        CsrMemCpy( myData->gsmTriplet[2].RAND,
                   current_att_offset+IE_EAP_SIM__ATT_GEN_HEADER_LEN + 2*GSM_RAND_LEN,
                   GSM_RAND_LEN );
    }

/*************************************************************************************************
 *************************************************************************************************
 *************************************************************************************************
 *************************************************************************************************/

    /* Derive GSM keys */
    if ( eapSIM_calculateGSMkeys( simContext ) != eapSIM_SUCCESS )
    {
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_challengePre(): Unable to Calculate GSM Keys" ));

        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }


    if ( eapSIM_challengePost( eap_method, simContext, reqData, reqDataLen, eapolRsp, eapRspLen ) != eapSIM_SUCCESS )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }
    else
    {
        return eapSIM_SUCCESS;
    }
}

static eEAP_SIM_RESULTS eapSIM_challengePost( eapMethod* eap_method,
                                              CsrSimContext *simContext,
                                              eap_packet *reqData,
                                              CsrUint16 reqDataLen,
                                              eapol_packet *eapolRsp,
                                              CsrUint16 *eapRspLen )
{
    CsrSimData *myData = &simContext->simData;
    CsrSimConfig *myConfig = &simContext->simConfig;

    CsrUint8 *usedIdentity;
    CsrUint16 usedIdentityLen;
    CsrUint8 *current_att_offset;
    CsrUint16 remaining_atts_len;
    CsrUint8 current_att_type, current_att_lenField, current_att_len ;

    /* Variables For Response */
    CsrUint8 currentAttOffset = 0;
    CsrUint8 *responsePacket = NULL;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_challengePost()" ));

    /* skip SNAP HEADER and EAPOL HEADER (filled ouside) */
    responsePacket = (CsrUint8 *) eapolRsp;
    responsePacket += CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH;

    /* Select Appropriate ID to generate the Master Key */
    if ( myData->pseudonym )
    {
        usedIdentity = myData->pseudonym;
        usedIdentityLen = myData->pseudonymLen;

        sme_trace_info(( TR_SECURITY,
            "eapSIM_challengePost(): Using a Pseudonym to generate Master Key" ));
    }
    else
    {
        usedIdentity = (CsrUint8 *) EAP_SIM_PERMANENT_ID_ASCII;  /* EAP_SIM_PERMANENT_ID; */
        usedIdentityLen = EAP_SIM_PERMANENT_ID_LEN;

        sme_trace_info(( TR_SECURITY,
            "eapSIM_challengePost(): Using Permanent User Identity to generate Master Key" ));
    }

    /***********************************
     *
     * Derive The Remaining EAP-SIM Keys
     *
     **********************************/

    /* Master Key */
    if ( eapSIM_calculateMasterKey( simContext,
        (CsrUint8*) usedIdentity,                /* EAP_SIM_PERMANENT_ID_ASCII */
        usedIdentityLen ) != eapSIM_SUCCESS )   /* EAP_SIM_PERMANENT_ID_LEN   */
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_challengePost(): Cannot Derive Keys Master Key" ));

        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* Further keys: K_encr, k_aut Master Session Key and Extended Master Session Key */
    if ( eapSIM_calculateFurtherKeys( simContext ) != eapSIM_SUCCESS )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_challengePost(): Cannot Derive Keys (MSK, EMSK, K_encr and K_aut)" ));

        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* Deduce Pairwise Master Key (fisrt 32 bytes of EMSK */
    CsrWifiSecurityStorePmk(simContext->securityContext, myData->msk);

    /* ***  AT_MAC *** */
    if ( eapSIM_findAndProcessAtMAC( simContext, reqData, reqDataLen,
                                     EAP_SIM_SUBTYPE_CHALLENGE ) != eapSIM_AT_VALID )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* *** AT_ENCR_DATA and AT_IV *** */
    if ( eapSIM_findAndProcessAtIvAtEncrData( simContext, reqData, reqDataLen,
                                              EAP_SIM_SUBTYPE_CHALLENGE ) == eapSIM_AT_INVALID )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* *** AT_RESULT_IND BUT only if it is Enabled on our side *** */
    if ( myConfig->protectedResultIndConfigured )
    {
        if ( eapSIM_findAndProcessAtResultInd( simContext, reqData, reqDataLen,
                                               EAP_SIM_SUBTYPE_CHALLENGE ) == eapSIM_AT_INVALID )
        {
            eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
            return eapSIM_FAILURE;
        }
    }

    /* Finally Parse for Non-skippable Attributes that are NOT allowed in EAP-Request/SIM/Challenge */
    remaining_atts_len = reqDataLen - (CsrUint16) IE_EAP_SIM_PACKET__HEADER_LEN;
    current_att_offset = ((CsrUint8 *)reqData+IE_EAP_SIM__FIRST_ATTRIBUTE_OFFSET);
    while (remaining_atts_len > 0)
    {
        current_att_type = *current_att_offset;
        current_att_lenField  = *(current_att_offset + 1);
        current_att_len = (CsrUint8) (4 * current_att_lenField);
        if ( (current_att_type != EAP_SIM_AT_MAC)
                && (current_att_type != EAP_SIM_AT_RAND)
                && (current_att_type != EAP_SIM_AT_IV)
                && (current_att_type != EAP_SIM_AT_ENCR_DATA)
                && (current_att_type != EAP_SIM_AT_PADDING)
                && (current_att_type != EAP_SIM_AT_NEXT_PSEUDONYM)
                && (current_att_type != EAP_SIM_AT_NEXT_REAUTH_ID)
                && (current_att_type != EAP_SIM_AT_RESULT_IND))
        {
            if ( eapSIM_isNonSkippableAttribute (current_att_type) )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_challengePost(): Unexpected Non-Skippable Attribute (%u) found in EAP-Request/SIM/Challenge",
                    current_att_type ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }
            else
            {
                /* No failure Just ignore */
                sme_trace_debug(( TR_SECURITY,
                    "eapSIM_challengePost(): Unexpected Skippable Attribute (%u) found in EAP-Request/SIM/Challenge: Ignored",
                    current_att_type ));
            }
        }

        current_att_offset += current_att_len;
        remaining_atts_len -= current_att_len;
    }

    /* Update dynamic variables */
    myData->numIdReqs = 0;
    myData->numNotificationRounds = 0;

    /* As per rfc-4186 the value of counter must be initialized to one after Full Authentication */
    myData->counterValue = 1;

    myData->lastReceivedPacket = EAP_SIM_CHALLENGE_REQ;

    /*******************************
     *
     * Generate the Response Message
     *
     ******************************/
    currentAttOffset = IE_EAP__HEADER_LEN ; /* skip EAP-HEADER (code, id, Length(2 Bytes), Type) */
    *eapRspLen = IE_EAP__HEADER_LEN;         /* But count it in in the total length */

    /* Subtype */
    *(responsePacket + currentAttOffset)  = (CsrUint8) EAP_SIM_SUBTYPE_CHALLENGE;
    /* Reserved */
    *(responsePacket + currentAttOffset + 1) = 0x00;
    *(responsePacket + currentAttOffset + 2) = 0x00;

    *eapRspLen += IE_EAP_SIM__HEADER_LEN;
    currentAttOffset += IE_EAP_SIM__HEADER_LEN;

    /* And Now the Attributes */

    /* AT_RESULT_IND =TLV (optional) */
    if ( (myData->networkSentAtResultInd) && (myConfig->protectedResultIndConfigured) )
    {
        *(responsePacket+currentAttOffset) = EAP_SIM_AT_RESULT_IND; /* T */
        *(responsePacket+currentAttOffset+1) = IE_EAP_SIM__AT_RESULT_IND_LEN_FIELD;  /* L */

        *(responsePacket+currentAttOffset+2) = 0x00;  /* V */
        *(responsePacket+currentAttOffset+3) = 0x00;  /* V */

        *eapRspLen += IE_EAP_SIM__AT_RESULT_IND_LEN;
        currentAttOffset += IE_EAP_SIM__AT_RESULT_IND_LEN;
    }

    /* AT_MAC =TLV (Mandatory) */
    *(responsePacket+currentAttOffset) = EAP_SIM_AT_MAC; /* T */
    *(responsePacket+currentAttOffset+1) = IE_EAP_SIM__AT_MAC_LEN_FIELD;  /* L */

    /* V */
    {
        CsrUint8 computed_mac[EAP_SIM_MAC_LEN];
        CsrUint8 lsbNUM, msbNUM;
        CsrUint8 AllSRES[EAP_SIM_MAX_NUM_CHALLENGES * EAP_SIM_SRES_LEN];

        /* Reserved */
        *(responsePacket+currentAttOffset+2) = 0x00;
        *(responsePacket+currentAttOffset+3) = 0x00;

        /* set MAC value within packet to 0 */
        CsrMemSet(responsePacket+currentAttOffset+4, 0, EAP_SIM_MAC_LEN);

        /* Update Length of response (as need it next) */
        *eapRspLen += IE_EAP_SIM__AT_MAC_LEN;

        /* all the eap Header fields have been set ouside except the length which needed calculating here
         * hence length needs to be set here */
        form2x8bitsFrom16bit( *eapRspLen, &lsbNUM, &msbNUM );
        *(responsePacket+2) = msbNUM;
        *(responsePacket+3) = lsbNUM;

        /* Calculate MAC value over packet */
        CsrMemCpy( AllSRES, myData->gsmTriplet[0].SRES, EAP_SIM_SRES_LEN );
        CsrMemCpy( AllSRES + EAP_SIM_SRES_LEN , myData->gsmTriplet[1].SRES, EAP_SIM_SRES_LEN );
        if ( myData->numRANDs == EAP_SIM_MAX_NUM_CHALLENGES )
        {
            CsrMemCpy( AllSRES + 2*EAP_SIM_SRES_LEN , myData->gsmTriplet[2].SRES, EAP_SIM_SRES_LEN );
        }

        if (  eapSIM_deriveMAC( myData->k_aut, responsePacket, *eapRspLen,
                               (CsrUint8 *) computed_mac, AllSRES,
                               myData->numRANDs * EAP_SIM_SRES_LEN ) != eapSIM_SUCCESS )
        {
            sme_trace_error(( TR_SECURITY, "eapSIM_challengePost(): Cannot Derive MAC" ));

            eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
            return eapSIM_FAILURE;
        }

        /* Insert MAC value in the previoulsly zeroed (16) bytes */
        CsrMemCpy(responsePacket+currentAttOffset+4, computed_mac, EAP_SIM_MAC_LEN);
    }

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_challengePost(): EAP-Response/SIM/Challenge =",
        responsePacket, *eapRspLen ));

    /* Update methodeState and Decision as per rfc-4137, Section 4.2 */
    eap_method->methodState = DONE;
    eap_method->decision    = COND_SUCC;

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_challengePost()" ));

    return eapSIM_SUCCESS;
}


static eEAP_SIM_RESULTS eapSIM_reauthentication( eapMethod* eap_method,
                                                 CsrSimContext *simContext,
                                                 eap_packet *reqData,
                                                 CsrUint16 reqDataLen,
                                                 eapol_packet *eapolRsp,
                                                 CsrUint16 *eapRspLen )
{
    CsrSimData *myData = &simContext->simData;
    CsrSimConfig *myConfig = &simContext->simConfig;

    CsrUint8* usedIdentity;
    CsrUint16  usedIdentityLen;

    /* Variables For Response */
    CsrUint8  currentAttOffset=0;
    CsrUint8  *responsePacket = NULL;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_reauthentication()" ));

    /* skip SNAP HEADER and EAPOL HEADER (filled ouside) */
    responsePacket = (CsrUint8 *) eapolRsp;
    responsePacket += CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH;

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_reauthentication(): EAP-Request/SIM/Re-Authentication Packet = ",
        reqData, reqDataLen ));

    /* Network Should Not have sent this message if STA Configuration does not support Fast-Reauthentication */
    if ( ! myConfig->fastReauthOn )
    {
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_reauthentication(): STA does not support Fast-Reauthentication" ));

        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    myData->numEapSimStartRounds =0;
    (myData->numFastReAuthRounds)++;

    /* AT_MAC: Mandatory */
    if ( eapSIM_findAndProcessAtMAC( simContext, reqData, reqDataLen,
                                     EAP_SIM_SUBTYPE_REAUTHENTICATION ) != eapSIM_AT_VALID )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* AT_ENCR_DATA, AT_IV: Mandatory */
    /* AT_ENCR_DATA may contain
       - Mandatory: AT_NONCE_S and AT_COUNTER
       - optional: AT_NEXT_REAUTH_ID (and possibly AT_PADDING)
       */
    if ( eapSIM_findAndProcessAtIvAtEncrData( simContext, reqData, reqDataLen,
                                              EAP_SIM_SUBTYPE_REAUTHENTICATION ) != eapSIM_AT_VALID )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* AT_RESULT_IND: Optional (only if we enable it) */
    if ( myConfig->protectedResultIndConfigured )
    {
        if ( eapSIM_findAndProcessAtResultInd( simContext, reqData, reqDataLen,
                                               EAP_SIM_SUBTYPE_REAUTHENTICATION ) ==  eapSIM_AT_INVALID )
        {
            eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
            return eapSIM_FAILURE;
        }
    }

    myData->lastReceivedPacket = EAP_SIM_REAUTHENTICATION_REQ;

    /****************************************
     *
     * Derive Keys for FAST RE-AUTHENTICATION
     *
     ***************************************/

    /* Select Appropriate ID to generate the Master Key */
    if ( myData->reauthId )
    {
        usedIdentity = myData->reauthId;
        usedIdentityLen = myData->reauthIdLen;

        sme_trace_info(( TR_SECURITY,
            "eapSIM_reauthentication(): Using a Re-Authentication ID to generate Master Key" ));
    }
    else if ( myData->pseudonym )
    {
        usedIdentity = myData->pseudonym;
        usedIdentityLen = myData->pseudonymLen;

        sme_trace_info(( TR_SECURITY,
            "eapSIM_reauthentication(): Using a Pseudonym to generate Master Key" ));
    }
    else
    {
        usedIdentity = (CsrUint8 *) EAP_SIM_PERMANENT_ID_ASCII;  /* EAP_SIM_PERMANENT_ID; */
        usedIdentityLen = EAP_SIM_PERMANENT_ID_LEN;

        sme_trace_info(( TR_SECURITY,
            "eapSIM_reauthentication(): Using Permanent User Identity to generate Master Key" ));
    }

    /* Master Key Reauthentication */
    if ( eapSIM_calculateMasterKeyReauth( simContext,
             (CsrUint8*) usedIdentity,               /* EAP_SIM_PERMANENT_ID_ASCII  */
             usedIdentityLen ) != eapSIM_SUCCESS )  /* EAP_SIM_PERMANENT_ID_LEN */
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* MSK and EMSK Reauthentication */
    if ( eapSIM_calculateFurtherKeysReauth( simContext ) != eapSIM_SUCCESS )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* PMK = fisrt 32 bytes of EMSK */
    CsrWifiSecurityStorePmk(simContext->securityContext, myData->msk);

    /*******************************
     *
     * Generate the Response Message
     *
     ******************************/

    currentAttOffset = IE_EAP__HEADER_LEN ; /* skip EAP-HEADER (code, id, Length(2 Bytes), Type) */
    *eapRspLen = IE_EAP__HEADER_LEN;         /* But count it in in the total length */

    /* Subtype */
    *(responsePacket + currentAttOffset)  = (CsrUint8) EAP_SIM_SUBTYPE_REAUTHENTICATION;
    /* Reserved */
    *(responsePacket + currentAttOffset + 1) = 0x00;
    *(responsePacket + currentAttOffset + 2) = 0x00;

    *eapRspLen += IE_EAP_SIM__HEADER_LEN;
    currentAttOffset += IE_EAP_SIM__HEADER_LEN;

    /* And Now the Attributes */

    /* AT_RESULT_IND =TLV (optional) */
    if ( (myData->networkSentAtResultInd) && (myConfig->protectedResultIndConfigured) )
    {
        *(responsePacket+currentAttOffset) = EAP_SIM_AT_RESULT_IND; /* T */
        *(responsePacket+currentAttOffset+1) = IE_EAP_SIM__AT_RESULT_IND_LEN_FIELD;  /* L */

        *(responsePacket+currentAttOffset+2) = 0x00;  /* V */
        *(responsePacket+currentAttOffset+3) = 0x00;  /* V */

        *eapRspLen += IE_EAP_SIM__AT_RESULT_IND_LEN;
        currentAttOffset += IE_EAP_SIM__AT_RESULT_IND_LEN;
    }

    /* AT_IV and AT_ENCR_DATA
     * AT_ENCR_DATA must include AT_COUNTER
     * and optionally AT_COUNTER_TOO_SMALL and/or AT_PADDING
     */

    if ( eapSIM_InsertAtIvAtEncrData( simContext, responsePacket,
                                      eapRspLen, &currentAttOffset,
                                      EAP_SIM_SUBTYPE_REAUTHENTICATION ) != eapSIM_SUCCESS )
    {
        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }

    /* AT_MAC =TLV (Mandatory) */
    *(responsePacket+currentAttOffset) = EAP_SIM_AT_MAC; /* T */
    *(responsePacket+currentAttOffset+1) = IE_EAP_SIM__AT_MAC_LEN_FIELD;  /* L */

    /* V */
    {
        CsrUint8 computed_mac[EAP_SIM_MAC_LEN];
        CsrUint8 lsbNUM, msbNUM;
        CsrUint8 AllSRES[EAP_SIM_MAX_NUM_CHALLENGES * EAP_SIM_SRES_LEN];

        /* Reserved */
        *(responsePacket+currentAttOffset+2) = 0x00;
        *(responsePacket+currentAttOffset+3) = 0x00;

        /* set MAC value within packet to 0 */
        CsrMemSet(responsePacket+currentAttOffset+4, 0, EAP_SIM_MAC_LEN);

        /* Update Length of response (as need it next) */
        *eapRspLen += IE_EAP_SIM__AT_MAC_LEN;

        /* all the eap Header fields have been set ouside except the length which needed calculating here
         * hence length needs to be set here */
        form2x8bitsFrom16bit( *eapRspLen, &lsbNUM, &msbNUM );
        *(responsePacket+2) = msbNUM;
        *(responsePacket+3) = lsbNUM;

        /* Calculate MAC value over packet */
        CsrMemCpy( AllSRES, myData->gsmTriplet[0].SRES, EAP_SIM_SRES_LEN );
        CsrMemCpy( AllSRES + EAP_SIM_SRES_LEN , myData->gsmTriplet[1].SRES, EAP_SIM_SRES_LEN );
        if ( myData->numRANDs == EAP_SIM_MAX_NUM_CHALLENGES )
        {
            CsrMemCpy( AllSRES + 2*EAP_SIM_SRES_LEN , myData->gsmTriplet[2].SRES, EAP_SIM_SRES_LEN );
        }

        if (  eapSIM_deriveMAC( myData->k_aut, responsePacket, *eapRspLen,
                               (CsrUint8 *) computed_mac, AllSRES,
                               myData->numRANDs * EAP_SIM_SRES_LEN ) != eapSIM_SUCCESS )
        {
            sme_trace_error(( TR_SECURITY,
                "eapSIM_reauthentication(): Cannot Derive MAC" ));

            eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
            return eapSIM_FAILURE;
        }

        /* Insert MAC value in the previoulsly zeroed (16) bytes */
        CsrMemCpy(responsePacket+currentAttOffset+4, computed_mac, EAP_SIM_MAC_LEN);
    }

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_reauthentication(): EAP-Response/SIM/Reauthentication =",
        responsePacket, *eapRspLen ));

    /* Update methodeState and Decision as per rfc-4137, Section 4.2 */
    eap_method->methodState = DONE;
    eap_method->decision    = COND_SUCC;

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_reauthentication()" ));

    return eapSIM_SUCCESS;
}

static eEAP_SIM_RESULTS eapSIM_notification( eapMethod* eap_method,
                                             CsrSimContext *simContext,
                                             eap_packet *reqData,
                                             CsrUint16 reqDataLen,
                                             eapol_packet *eapolRsp,
                                             CsrUint16 *eapRspLen )
{
    CsrSimData *myData = &simContext->simData;
    CsrSimConfig *myConfig = &simContext->simConfig;

    /* Variables For Request */
    CsrUint8 *k_aut = NULL;
    CsrUint8 * current_att_offset;
    CsrUint16 remaining_atts_len;
    CsrUint8 current_att_type, current_att_lenField, current_att_len;
    CsrUint16 notificationCode=0;
    eEAP_SIM_ATT_RESULTS atNotifRes    = eapSIM_AT_NOT_FOUND;
    eEAP_SIM_ATT_RESULTS atMacRes      = eapSIM_AT_NOT_FOUND;
    eEAP_SIM_ATT_RESULTS atEncrDataRes = eapSIM_AT_NOT_FOUND;

    /* Variables For Response */
    CsrUint8 lsbNUM, msbNUM;
    CsrUint8 currentAttOffset=0;
    CsrUint8 *responsePacket = NULL;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_notification()"));

    /* skip SNAP HEADER and EAPOL HEADER (filled ouside) */
    responsePacket = (CsrUint8 *) eapolRsp;
    responsePacket += CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH;

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_notification(): EAP-Request/SIM/Notification Packet = ",
        reqData, reqDataLen ));

    if (myData->numNotificationRounds > 0)
    {
       sme_trace_debug(( TR_SECURITY,
            "eapSIM_notification(): Only one notification round is allowed" ));

       eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
       return eapSIM_FAILURE;
    }

    myData->numNotificationRounds++;

    /* AT_NOTIFICATION */
    remaining_atts_len = reqDataLen - (CsrUint16) IE_EAP_SIM_PACKET__HEADER_LEN;
    current_att_offset = ((CsrUint8 *)reqData+IE_EAP_SIM__FIRST_ATTRIBUTE_OFFSET);
    while ( remaining_atts_len > 0 )
    {
        current_att_type = *current_att_offset;
        current_att_lenField  = *(current_att_offset + 1);
        current_att_len = (CsrUint8) (4 * current_att_lenField);

        if ( current_att_type == EAP_SIM_AT_NOTIFICATION )
        {

            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_notification(): AT_NOTIFICATION = ",
                current_att_offset, current_att_len ));

            atNotifRes = eapSIM_AT_INVALID;

            if (current_att_len != IE_EAP_SIM__AT_NOTIFICATION_LEN)
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_notification(): Invalid Length Field Value (%u) of AT_NOTIFICATION (should be %u)",
                    current_att_lenField, IE_EAP_SIM__AT_NOTIFICATION_LEN_FIELD ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }
            else
            {
                notificationCode = form16bitFrom2x8bit_se( *( current_att_offset + 2),
                                                           *( current_att_offset + 3) );

                if ( notificationCode < eapSIM_SUCCESS )
                {
                    sme_trace_debug(( TR_SECURITY,
                        "eapSIM_notification(): Notification Code (%lu) Implying Failure",
                        notificationCode ));

                }
                else /* if ( notificationCode >= eapSIM_SUCCESS ) */
                {
                    atNotifRes = eapSIM_AT_VALID;
                    sme_trace_debug(( TR_SECURITY,
                        "eapSIM_notification(): Notification Code (%lu) Implying Success",
                        notificationCode ));
                }
            }
        }

        current_att_offset += current_att_len;
        remaining_atts_len -= current_att_len;
    }

    /* Failure if AT_NOTIFICATION attribute NOT Found or NOT Valid */
    if (atNotifRes == eapSIM_AT_NOT_FOUND)
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_notification(): Mandatory AT_NOTIFICATION NOT Found in EAP-Request/SIM/Notification" ));

        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
        return eapSIM_FAILURE;
    }
    else if (atNotifRes == eapSIM_AT_INVALID)
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_notification(): AT_NOTIFICATION found in EAP-Request/SIM/Notification is Invalid" ));

        eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );

        return eapSIM_FAILURE;
    }

    /* AT_MAC: Conditional on P-bit=0 */
    atMacRes = eapSIM_findAndProcessAtMAC( simContext, reqData, reqDataLen,
                                           EAP_SIM_SUBTYPE_NOTIFICATION );

    /* AT_ENCR_DATA, AT_IV: Conditional on FastReauth and P-bit=0 */
    if ( eapSIM_isPhaseBitZero(notificationCode) )
    {
        if ( atMacRes != eapSIM_AT_VALID  )
        {
            sme_trace_error(( TR_SECURITY,
                "eapSIM_notification(): P-bit=0 but AT_MAC NOT Found/Invalid in EAP-Request/SIM/Notification" ));

            eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
            return eapSIM_FAILURE;
        }

        if ( myConfig->fastReauthOn )
        {
            /* AT_ENCR_DATA must contain AT_COUNTER and possibly AT_PADDING */
            atEncrDataRes = eapSIM_findAndProcessAtIvAtEncrData( simContext, reqData, reqDataLen,
                                                                 EAP_SIM_SUBTYPE_NOTIFICATION );

            if ( atEncrDataRes !=  eapSIM_AT_VALID )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_notification(): P-bit=0 but AT_ENCR_DATA/AT_IV NOT Found/Invalid in EAP-Request/SIM/Notification" ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }
        }
    }
    else /* P-Bit == 1 */
    {
        if ( atMacRes != eapSIM_AT_NOT_FOUND )
        {
            sme_trace_error(( TR_SECURITY,
                "eapSIM_notification(): P-bit=1 but Found AT_MAC in EAP-Request/SIM/Notification" ));

            eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
            return eapSIM_FAILURE;
        }
    }

    /* Finally Parse for Non-skippable Attributes that are NOT allowed in EAP-Request/SIM/Notification */
    remaining_atts_len = reqDataLen - (CsrUint16) IE_EAP_SIM_PACKET__HEADER_LEN;
    current_att_offset = ((CsrUint8 *)reqData+IE_EAP_SIM__FIRST_ATTRIBUTE_OFFSET);
    while (remaining_atts_len > 0)
    {
        current_att_type = *current_att_offset;
        current_att_lenField  = *(current_att_offset + 1);
        current_att_len = (CsrUint8) (4 * current_att_lenField);
        if ( (current_att_type != EAP_SIM_AT_NOTIFICATION)
             && (current_att_type != EAP_SIM_AT_MAC)
             && (current_att_type != EAP_SIM_AT_ENCR_DATA)
             && (current_att_type != EAP_SIM_AT_IV) )
        {
            if ( eapSIM_isNonSkippableAttribute (current_att_type) )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_notification(): Unexpected Non-Skippable Attribute (%u) found in EAP-Request/SIM/Notification",
                    current_att_type ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }
            else
            {
                /* No failure Just ignore */
                sme_trace_debug(( TR_SECURITY,
                    "eapSIM_notification(): Unexpected Skippable Attribute (%u) found in EAP-Request/SIM/Notification: Ignored",
                    current_att_type ));
            }
        }

        current_att_offset += current_att_len;
        remaining_atts_len -= current_att_len;
    }

    myData->numEapSimStartRounds = 0;

    myData->lastReceivedPacket = EAP_SIM_NOTIFICATION_REQ;

    /*******************************
     *
     * Generate the Response Message
     *
     ******************************/
    currentAttOffset = IE_EAP__HEADER_LEN ; /* skip EAP-HEADER (code, id, Length(2 Bytes), Type) */
    *eapRspLen = IE_EAP__HEADER_LEN;         /* But count it in in the total length */

    /* Subtype */
    *(responsePacket + currentAttOffset)  = (CsrUint8) EAP_SIM_SUBTYPE_NOTIFICATION;
    /* Reserved */
    *(responsePacket + currentAttOffset + 1) = 0x00;
    *(responsePacket + currentAttOffset + 2) = 0x00;

    *eapRspLen += IE_EAP_SIM__HEADER_LEN;
    currentAttOffset += IE_EAP_SIM__HEADER_LEN;

    /* And Now the Attributes (ONLY if P-Bit in Request was zero) */
    if ( eapSIM_isPhaseBitZero(notificationCode) )
    {
        /* Only if Fast Re-Auth */
        if ( myConfig->fastReauthOn )
        {
            /* AT_IV and AT_ENCR_DATA (AT_COUNTER + AT_PADDING) */
            if ( eapSIM_InsertAtIvAtEncrData( simContext, responsePacket,
                                              eapRspLen, &currentAttOffset,
                                              EAP_SIM_SUBTYPE_NOTIFICATION ) != eapSIM_SUCCESS )
            {
                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }
        }

        /* AT_MAC =TLV */
        *(responsePacket+currentAttOffset) = EAP_SIM_AT_MAC; /* T */
        *(responsePacket+currentAttOffset+1) = IE_EAP_SIM__AT_MAC_LEN_FIELD;  /* L */

        /* V */
        {
            CsrUint8 computed_mac[EAP_SIM_MAC_LEN];

            /* Reserved */
            *(responsePacket+currentAttOffset+2) = 0x00;
            *(responsePacket+currentAttOffset+3) = 0x00;

            /* set MAC value within packet to 0 */
            CsrMemSet(responsePacket+currentAttOffset+4, 0, EAP_SIM_MAC_LEN);

            /* Update Length of response (as need it next) */
            *eapRspLen += IE_EAP_SIM__AT_MAC_LEN;

            /* Calculate MAC value over packet */
            if ( eapSIM_deriveMAC( k_aut, responsePacket, *eapRspLen, (CsrUint8 *) computed_mac,
                     (CsrUint8 *) "" , 0) != eapSIM_SUCCESS )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_notification(): Cannot Derive MAC" ));

                eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRsp, eapRspLen );
                return eapSIM_FAILURE;
            }

            /* Insert MAC value in the previoulsly zeroed (16) bytes */
            CsrMemCpy(responsePacket+currentAttOffset+4, computed_mac, EAP_SIM_MAC_LEN);
        }
    }

    /* all the eap Header fields have been set ouside except the length which needed calculating here
     * hence length needs to be set here */
    form2x8bitsFrom16bit( *eapRspLen, &lsbNUM, &msbNUM );
    sme_trace_debug((TR_SECURITY_LIB, "Resp_Len = %d", *eapRspLen));

    *(responsePacket+2) = msbNUM;
    *(responsePacket+3) = lsbNUM;

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_notification(): EAP-Response/SIM/Notification =",
        responsePacket, *eapRspLen ));

    /* Update methodeState and Decision */
    eap_method->methodState = DONE;
    eap_method->decision    = COND_SUCC;

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_notification()"));

    return eapSIM_SUCCESS;
}

static void eapSIM_clientError( CsrSimContext *simContext,
                                eEapSimErrorCodes errCode,
                                eapol_packet *eapolRsp,
                                CsrUint16 *eapErrRspLen )
{
    CsrSimData *myData = &simContext->simData;

    CsrUint8 lsbNUM, msbNUM;
    CsrUint8 currentAttOffset=0;
    CsrUint8 *responsePacket = NULL;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_clientError()"));

    /* skip SNAP HEADER and EAPOL HEADER (filled ouside) */
    responsePacket = (CsrUint8 *) eapolRsp;
    responsePacket += CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH;

    myData->lastReceivedPacket = EAP_SIM_INVALID_REQ;

    /* eap_sim_state(data, FAILURE); */
    myData->numIdReqs = 0;
    myData->numNotificationRounds = 0;
    myData->numEapSimStartRounds = 0;

    /*******************************
     *
     * Generate the Response Message
     *
     ******************************/
    currentAttOffset = IE_EAP__HEADER_LEN ; /* skip EAP-HEADER (code, id, Length(2 Bytes), Type) */
    *eapErrRspLen = IE_EAP__HEADER_LEN;         /* But count it in in the total length */

    /* Subtype */
    *(responsePacket + currentAttOffset)  = (CsrUint8) EAP_SIM_SUBTYPE_CLIENT_ERROR;
    /* Reserved */
    *(responsePacket + currentAttOffset + 1) = 0x00;
    *(responsePacket + currentAttOffset + 2) = 0x00;

    *eapErrRspLen += IE_EAP_SIM__HEADER_LEN;
    currentAttOffset += IE_EAP_SIM__HEADER_LEN;

    /* AT_CLIENT_ERROR_CODE =TLV */
    *(responsePacket+currentAttOffset) = EAP_SIM_AT_CLIENT_ERROR_CODE; /* T */
    *(responsePacket+currentAttOffset+1) = IE_EAP_SIM__AT_CLIENT_ERROR_CODE_LEN_FIELD;  /* L */
    *(responsePacket+currentAttOffset+2) = 0x00;  /* V */
    *(responsePacket+currentAttOffset+3) = (CsrUint8) errCode;  /* V */

    *eapErrRspLen += IE_EAP_SIM__AT_CLIENT_ERROR_CODE_LEN;

    /* all the eap Header fields have been set ouside except the length which needed calculating here
     * hence length needs to be set here */
    form2x8bitsFrom16bit( *eapErrRspLen, &lsbNUM, &msbNUM );
    *(responsePacket+2) = msbNUM;
    *(responsePacket+3) = lsbNUM;

    sme_trace_debug((TR_SECURITY_LIB, "Resp_Len = %d", *eapErrRspLen));

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_clientError(): Sending EAP-Response-SIM-Client-Error Packet:",
        responsePacket, *eapErrRspLen ));

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_clientError()"));
}

static eEAP_SIM_RESULTS eapSIM_process( eapMethod* eap_method,
                                        CsrSimContext* simContext,
                                        eap_packet *reqPkt,
                                        eapol_packet *eapolRspPkt,
                                        CsrUint16 *eapRspPktLen )
{
    CsrSimData *myData = &simContext->simData;
    CsrSimConfig *myConfig = &simContext->simConfig;

    CsrUint8 subtype, *reqPktBytes;
    CsrUint16 reqPktLen, lenHi, lenLo;
    eEAP_SIM_RESULTS processRes = eapSIM_FAILURE;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_process()"));

    /* Read Length of Incoming Message */
    lenHi = (CsrUint16) reqPkt->eap_length_hi;
    lenLo = (CsrUint16) reqPkt->eap_length_low;
    reqPktLen = ((CsrUint16) (lenHi << 8)) | (lenLo) ;

    /* Read Subtype of Incoming Message */
    reqPktBytes = (CsrUint8 *) reqPkt;
    subtype = *(reqPktBytes + IE_EAP__HEADER_LEN) ;
    sme_trace_debug(( TR_SECURITY,
        "eapSIM_process(): Request Packet SubType= %d", subtype ));

    /* Process Message According to its Subtype */
    switch ( subtype )
    {
        case EAP_SIM_SUBTYPE_START:
            processRes = eapSIM_start( eap_method, simContext, reqPkt, reqPktLen, eapolRspPkt, eapRspPktLen );
            break;

        case EAP_SIM_SUBTYPE_CHALLENGE:
            processRes = eapSIM_challengePre( eap_method, simContext, reqPkt, reqPktLen, eapolRspPkt, eapRspPktLen );
            break;

        case EAP_SIM_SUBTYPE_NOTIFICATION:
            processRes = eapSIM_notification( eap_method, simContext, reqPkt, reqPktLen, eapolRspPkt, eapRspPktLen );
            break;

        case EAP_SIM_SUBTYPE_REAUTHENTICATION:
            processRes = eapSIM_reauthentication( eap_method, simContext, reqPkt, reqPktLen, eapolRspPkt, eapRspPktLen );
            break;

        default: /* Anything else must not be allowed */
        {
            sme_trace_error(( TR_SECURITY,
                "eapSIM_process(): Unknown EAP-SIM Packet Subtype = %d",
                subtype ));

            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_process(): Received Packet = ", reqPkt, reqPktLen ));

            processRes = eapSIM_FAILURE;
            eapSIM_clientError ( simContext, eapSIM_ERROR_UNABLE_TO_PROCESS_PACKET, eapolRspPkt, eapRspPktLen );
            break;
        }
    }

    /* Update methodeState and Decision as per rfc-4137, Section 4.2 */
    if ( processRes == eapSIM_FAILURE )
    {
        eap_method->methodState = CONT;
        eap_method->decision = FAIL;
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_process(): processRes is Failure (at the end of eapSIM_process())",
            subtype ));
    }

    if ( ((eap_method->methodState == CONT) || (eap_method->methodState == MAY_CONT))
          && (myData->networkSentAtResultInd &&  myConfig->protectedResultIndConfigured) )
    {
        eap_method->allowNotifications = TRUE;
    }
    else
    {
        eap_method->allowNotifications = FALSE;
    }

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_process()"));

    return processRes;
}

/* ************************************************************************ */
/* ************************************************************************ */
/*                                                                          */
/*                  ATTRIBUTES PROCESSING FUNCTIONS                          */
/*                                                                          */
/* ************************************************************************ */
/* ************************************************************************ */

static eEAP_SIM_ATT_RESULTS eapSIM_findAndProcessAtMAC( CsrSimContext *simContext,
                                                        eap_packet *reqData,
                                                        CsrUint16 reqDataLen,
                                                        CsrUint8  msgSubType )
{
    CsrSimData *myData = &simContext->simData;

    CsrUint8 * current_offset;
    CsrUint16 remaining_len;
    CsrUint8 current_att_type, current_att_lenField, current_att_len ;
    eEAP_SIM_ATT_RESULTS atMacRes=eapSIM_AT_NOT_FOUND;
    CsrUint8 numOccurences=0;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_findAndProcessAtMAC()" ));

    if ( (msgSubType != EAP_SIM_SUBTYPE_CHALLENGE)
          && (msgSubType != EAP_SIM_SUBTYPE_REAUTHENTICATION)
          && (msgSubType != EAP_SIM_SUBTYPE_NOTIFICATION) )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_findAndProcessAtMAC(): Function called from the wrong place (cannot process sub-type %d)",
            msgSubType ));

        return eapSIM_AT_NOT_FOUND;
    }

    remaining_len = reqDataLen - (CsrUint16) IE_EAP_SIM_PACKET__HEADER_LEN;
    current_offset = ((CsrUint8 *)reqData+IE_EAP_SIM__FIRST_ATTRIBUTE_OFFSET);
    while ( remaining_len > 0 )
    {
        current_att_type = *current_offset;
        current_att_lenField  = *(current_offset + 1);
        current_att_len = (CsrUint8) (4 * current_att_lenField);

        if ( current_att_type == EAP_SIM_AT_MAC )
        {
            numOccurences++;
            if (numOccurences > 1)
            {
                atMacRes = eapSIM_AT_MULTIPLE;
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_findAndProcessAtMAC(): AT_MAC Attribute found more than once in Request Message)" ));

                return eapSIM_AT_MULTIPLE;
            }

            atMacRes = eapSIM_AT_INVALID;

            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_findAndProcessAtMAC(): AT_MAC Value= ",
                current_offset, current_att_len ));

            if ( current_att_len != IE_EAP_SIM__AT_MAC_LEN )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_findAndProcessAtMAC(): Invalid Length Field Value (%u) of AT_MAC (should be %u)",
                    current_att_lenField, IE_EAP_SIM__AT_MAC_LEN_FIELD ));

                return eapSIM_AT_INVALID;
            }

            /* Check if Received MAC agrees with Computed MAC */
            if ( msgSubType == EAP_SIM_SUBTYPE_CHALLENGE )
            {
                atMacRes = eapSIM_checkReceivedMAC( myData->k_aut, reqData, reqDataLen,
                                                    current_offset, myData->nonce_mt, EAP_SIM_NONCE_MT_LEN );
            }
            else /* if ( msgSubType == EAP_SIM_SUBTYPE_REAUTHENTICATION ) || ( msgSubType == EAP_SIM_SUBTYPE_NOTIFICATION ) */
            {
                atMacRes = eapSIM_checkReceivedMAC(myData->k_aut, reqData, reqDataLen, current_offset, NULL, 0);
            }

            if ( atMacRes != eapSIM_AT_VALID )
            {
                return eapSIM_AT_INVALID;
            }
        }

        current_offset += current_att_len;
        remaining_len -= current_att_len;
    }

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_findAndProcessAtMAC()" ));

    return eapSIM_AT_VALID;
}


static eEAP_SIM_ATT_RESULTS eapSIM_findAndProcessAtIvAtEncrData( CsrSimContext *simContext,
                                                                 eap_packet *reqData,
                                                                 CsrUint16 reqDataLen,
                                                                 CsrUint8  msgSubType )
{
    CsrSimData *myData = &simContext->simData;

    CsrUint8 *current_offset;
    CsrUint16 remaining_len;
    CsrUint8 current_att_type, current_att_lenField, current_att_len ;
    CsrUint8 numOccurencesAT_ENCR=0, numOccurencesAT_IV=0;
    eEAP_SIM_ATT_RESULTS atIvRes= eapSIM_AT_NOT_FOUND;
    eEAP_SIM_ATT_RESULTS atEncrDataRes= eapSIM_AT_NOT_FOUND;
    CsrUint8 *pENCR_DATA=NULL, *pIV=NULL;
    CsrUint8 encr_data_len=0;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_findAndProcessAtIvAtEncrData()" ));

    if ( (msgSubType != EAP_SIM_SUBTYPE_CHALLENGE)
          && (msgSubType != EAP_SIM_SUBTYPE_REAUTHENTICATION)
          && (msgSubType != EAP_SIM_SUBTYPE_NOTIFICATION) )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_findAndProcessAtIvAtEncrData(): Function called from the wrong place (cannot process sub-type %d)",
            msgSubType ));

        return eapSIM_AT_NOT_FOUND;
    }

    remaining_len = reqDataLen - (CsrUint16) IE_EAP_SIM_PACKET__HEADER_LEN;
    current_offset = ((CsrUint8 *)reqData+IE_EAP_SIM__FIRST_ATTRIBUTE_OFFSET);
    while ( remaining_len > 0 )
    {
        current_att_type = *current_offset;
        current_att_lenField  = *(current_offset + 1);
        current_att_len = (CsrUint8) (4 * current_att_lenField);

        if (current_att_type == EAP_SIM_AT_ENCR_DATA)
        {
            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_findAndProcessAtIvAtEncrData(): AT_ENCR_DAT = ",
                current_offset, current_att_len ));

            numOccurencesAT_ENCR++;
            if ( numOccurencesAT_ENCR > 1)
            {
                atEncrDataRes = eapSIM_AT_MULTIPLE;
                sme_trace_error(( TR_SECURITY,
                   "eapSIM_findAndProcessAtIvAtEncrData(): AT_ENCR_DATA Attribute found more than once in Request packet)" ));

                return eapSIM_AT_MULTIPLE;
            }

            atEncrDataRes = eapSIM_AT_INVALID; /* Found but Invalid until proven Valid */

            if (current_att_len % 16)
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_findAndProcessAtIvAtEncrData(): Invalid Length (%u) of AT_ENCR_DATA (Total Length should be multiple of 16 bytes)",
                    current_att_len ));

                return eapSIM_AT_INVALID;
            }

            pENCR_DATA = current_offset;
            encr_data_len = current_att_len;
        }
        else if ( current_att_type == EAP_SIM_AT_IV )
        {
            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_findAndProcessAtIvAtEncrData(): AT_ENCR_DAT = ",
                current_offset, current_att_len ));

            numOccurencesAT_IV++;
            if ( numOccurencesAT_IV > 1 )
            {
                atEncrDataRes = eapSIM_AT_MULTIPLE;
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_findAndProcessAtIvAtEncrData(): AT_ENCR_DATA Attribute found more than once in Request packet)" ));

                return eapSIM_AT_MULTIPLE;
            }

            atIvRes = eapSIM_AT_INVALID;  /* Found but Invalid until proven Valid */

            if ( current_att_len != IE_EAP_SIM__AT_IV_LEN )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_findAndProcessAtIvAtEncrData(): Invalid Length Field Value (%u) of AT_IV (should be %u)",
                    current_att_lenField, IE_EAP_SIM__AT_IV_LEN_FIELD ));

                return eapSIM_AT_INVALID;
            }

            pIV = current_offset;
        }

        current_offset += current_att_len;
        remaining_len -= current_att_len;
    }

    /* Either
     * - Found both AT_IV and AT_ENCR_DATA then process them
     * - Found one but not the other one then Error
     * - both Not found: if ReAuth then error but if FullAuth no problem since optional
     */
    if ( (atIvRes == eapSIM_AT_NOT_FOUND) && (atEncrDataRes != eapSIM_AT_NOT_FOUND) )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_findAndProcessAtIvAtEncrData(): AT_IV found but AT_ENCR_DATA not found in Request packet" ));

        return eapSIM_AT_INVALID;
    }
    else if ( (atIvRes != eapSIM_AT_NOT_FOUND) && (atEncrDataRes == eapSIM_AT_NOT_FOUND) )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_findAndProcessAtIvAtEncrData(): AT_IV Not found but AT_ENCR_DATA found in Request packet" ));

        return eapSIM_AT_INVALID;
    }
    else if ( (atIvRes != eapSIM_AT_NOT_FOUND) && (atEncrDataRes != eapSIM_AT_NOT_FOUND) )
    {
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_findAndProcessAtIvAtEncrData(): found AT_IV and AT_ENCR_DATA in Request packet" ));

        /*
         * *** Process AT_ENCR_DATA (using AT_IV)
         */

        /* Previous Re-Authentication Id must not be used again */
        CsrPfree(myData->reauthId);
        myData->reauthId = NULL; /* Not Necessary */
        myData->reauthIdLen = 0;

        if ( eapSIM_AesDecrypt( myData, myData->k_encr, pENCR_DATA, encr_data_len,
                                pIV, msgSubType ) ==  eapSIM_FAILURE )
        {
            return eapSIM_AT_INVALID;
        }
    }
    else if ((atIvRes == eapSIM_AT_NOT_FOUND) && (atEncrDataRes == eapSIM_AT_NOT_FOUND))
    {
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_findAndProcessAtIvAtEncrData(): both AT_IV and AT_ENCR_DATA Not found in Request packet" ));

        return eapSIM_AT_NOT_FOUND;
    }

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_findAndProcessAtIvAtEncrData()" ));

    return eapSIM_AT_VALID;
}


static eEAP_SIM_ATT_RESULTS eapSIM_findAndProcessAtResultInd( CsrSimContext *simContext,
                                                              eap_packet *reqData,
                                                              CsrUint16 reqDataLen,
                                                              CsrUint8  msgSubType )
{
    CsrSimData *myData = &simContext->simData;

    eEAP_SIM_ATT_RESULTS atResIndRes= eapSIM_AT_NOT_FOUND;
    CsrUint8 * current_offset;
    CsrUint16 remaining_len;
    CsrUint8 current_att_type, current_att_lenField, current_att_len ;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_findAndProcessAtResultInd()" ));

    if ( (msgSubType != EAP_SIM_SUBTYPE_CHALLENGE) && (msgSubType != EAP_SIM_SUBTYPE_REAUTHENTICATION) )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_findAndProcessAtResultInd(): Function called from the wrong place (cannot process sub-type %d)",
            msgSubType ));
        return eapSIM_AT_NOT_FOUND;
    }

    remaining_len = reqDataLen - (CsrUint16) IE_EAP_SIM_PACKET__HEADER_LEN;
    current_offset = ((CsrUint8 *)reqData+IE_EAP_SIM__FIRST_ATTRIBUTE_OFFSET);
    while (remaining_len > 0)
    {
        current_att_type = *current_offset;
        current_att_lenField  = *(current_offset + 1);
        current_att_len = (CsrUint8) (4 * current_att_lenField);

        if ( current_att_type == EAP_SIM_AT_RESULT_IND )
        {
            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_findAndProcessAtResultInd(): AT_RESULT_IND = ",
                current_offset, current_att_len ));

            atResIndRes= eapSIM_AT_INVALID;

            if (current_att_len != IE_EAP_SIM__AT_RESULT_IND_LEN)
            {
               /* since skippable Attribute we just ignore (no failure) */
               sme_trace_warn(( TR_SECURITY,
                   "eapSIM_findAndProcessAtResultInd(): Invalid Length Field Value (%u) of AT_RESULT_IND (should be %u)",
                   current_att_lenField, IE_EAP_SIM__AT_RESULT_IND_LEN_FIELD ));
            }
            else
            {
                atResIndRes= eapSIM_AT_VALID;
                myData->networkSentAtResultInd = TRUE;
            }
            break;
        }

        current_offset += current_att_len;
        remaining_len -= current_att_len;
    }

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_findAndProcessAtResultInd()" ));

    return atResIndRes;
}

static eEAP_SIM_RESULTS eapSIM_InsertAtIvAtEncrData( CsrSimContext *simContext,
                                                     CsrUint8  *respPkt,
                                                     CsrUint16 *respPktLen,
                                                     CsrUint8  *current_att_Offset,
                                                     CsrUint8  msgSubType )
{
    CsrSimData *myData = &simContext->simData;

    CsrUint8  iv[EAP_SIM_IV_LEN];
    CsrUint8  lsbNUM, msbNUM;
    CsrUint8  *originalEncrData;
    CsrUint8  originalEncrDataLen;
    CsrUint8  at_encr_data_total_len, padLen;
    CsrUint8  encrDataOffset=0;
    CsrUint8  currentOffset;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_InsertAtIvAtEncrData()" ));

    currentOffset = *current_att_Offset;

    /*** AT_IV ****/
    *(respPkt+currentOffset) = EAP_SIM_AT_IV; /* T */
    *(respPkt+currentOffset+1) = IE_EAP_SIM__AT_IV_LEN_FIELD;  /* L */

    *(respPkt+currentOffset+2) = 0x00;  /* V */
    *(respPkt+currentOffset+3) = 0x00;  /* V */
    /* V */
    CsrWifiSecurityRandom( (CsrUint8*) iv, (CsrUint32) EAP_SIM_IV_LEN);
    CsrMemCpy( respPkt+currentOffset+4, iv , EAP_SIM_IV_LEN);

    *respPktLen += IE_EAP_SIM__AT_IV_LEN;
    *current_att_Offset += IE_EAP_SIM__AT_IV_LEN;

    /*** AT_ENCR_DATA ***/
    originalEncrDataLen = IE_EAP_SIM__AT_COUNTER_LEN;
    if ( (msgSubType== EAP_SIM_SUBTYPE_REAUTHENTICATION) &&  (myData->counterValueMismatch) )
    {
        originalEncrDataLen += IE_EAP_SIM__AT_COUNTER_TOO_SMALL_LEN;
    }

    /* Do we need Padding? */
    padLen = ( 16 - (originalEncrDataLen % 16)) % 16 ;
    originalEncrDataLen += padLen + EAP_SIM__AT_PAD_HDR_LEN;

    originalEncrData = (CsrUint8 *) CsrPmalloc ( originalEncrDataLen );
    if ( originalEncrData == NULL)
    {
        sme_trace_debug(( TR_SECURITY, "eapSIM_InsertAtIvAtEncrData(): Memory Allocation Failure" ));
        return eapSIM_FAILURE;
    }

    /* Start filling AT_ENCR_DATA (header first) */
    at_encr_data_total_len = (originalEncrDataLen+ EAP_SIM__AT_ENCR_DATA_HDR_LEN+2); /* 2 bytes Not in the header of AT_ENCR_DATA */
    *(respPkt+currentOffset) = EAP_SIM_AT_ENCR_DATA; /* T */
    *(respPkt+currentOffset+1) = at_encr_data_total_len/4 ; /* L */
    *(respPkt+currentOffset+2) = 0x00;  /* V */
    *(respPkt+currentOffset+3) = 0x00;  /* V */

    /* *respPktLen += EAP_SIM__AT_ENCR_DATA_HDR_LEN+2; */  /* T(1byte)+L(1byte)+ reserved(2 bytes) fields of AT_ENCR_DATA */
    currentOffset += EAP_SIM__AT_ENCR_DATA_HDR_LEN+2;

    /* AT_COUNTER */
    *originalEncrData = EAP_SIM_AT_COUNTER;
    *(originalEncrData+1) = IE_EAP_SIM__AT_COUNTER_LEN_FIELD;
    form2x8bitsFrom16bit( myData->counterValue, &lsbNUM, &msbNUM );
    *(originalEncrData+2) = msbNUM;
    *(originalEncrData+3) = lsbNUM;

    encrDataOffset = IE_EAP_SIM__AT_COUNTER_LEN;

    /* AT_COUNTER_TOO_SMALL */
    if ( (msgSubType== EAP_SIM_SUBTYPE_REAUTHENTICATION) &&  (myData->counterValueMismatch) )
    {
        *(originalEncrData+encrDataOffset) = EAP_SIM_AT_COUNTER_TOO_SMALL;
        *(originalEncrData+encrDataOffset+1) = IE_EAP_SIM__AT_COUNTER_TOO_SMALL_LEN_FIELD;
        *(originalEncrData+encrDataOffset+2) = 0x00;
        *(originalEncrData+encrDataOffset+3) = 0x00;

        encrDataOffset += IE_EAP_SIM__AT_COUNTER_TOO_SMALL_LEN;
    }

    /* AT_PADDING */
    if ( padLen > 0 )
    {
        *(originalEncrData+encrDataOffset) = EAP_SIM_AT_PADDING;
        *(originalEncrData+encrDataOffset+1) = ( EAP_SIM__AT_PAD_HDR_LEN + padLen )/4;
        CsrMemSet( originalEncrData+encrDataOffset+2, 0, padLen);
    }

    /* Encrypt the Data */
    eapSIM_AesEncrypt( myData, myData->k_encr, originalEncrData,
                       originalEncrDataLen, &currentOffset, iv );

    CsrPfree( originalEncrData );

    *respPktLen += at_encr_data_total_len;
    *current_att_Offset += at_encr_data_total_len;

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_InsertAtIvAtEncrData()" ));

    return eapSIM_SUCCESS;
}

static eEAP_SIM_RESULTS eapSIM_deriveMAC( const CsrUint8 *k_aut,
                                          const CsrUint8 *msg,
                                          CsrUint16 msgLen,
                                          CsrUint8 *mac,
                                          const CsrUint8 *moreData,
                                          CsrUint16 moreDataLen )
{
    CsrUint8 hmac[CSR_SHA1_DIGEST_LENGTH];
    CsrUint16 allDataLen;
    CsrUint8 *allData;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_deriveMAC()" ));

    allDataLen = msgLen + moreDataLen ;
    allData = CsrPmalloc(allDataLen);
    if (allData == NULL)
    {
        sme_trace_debug(( TR_SECURITY, "eapSIM_deriveMAC(): Memory Allocation Failure" ));

        return eapSIM_FAILURE;
    }

    CsrMemCpy(allData, msg, msgLen);
    CsrMemCpy(allData + msgLen, moreData, moreDataLen);

    CsrMemSet(mac, 0, EAP_SIM_MAC_LEN);

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_deriveMAC(): K_aut = ", k_aut, EAP_SIM_K_AUT_KEY_LEN ));

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_deriveMAC(): msg = ", msg, msgLen ));

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_deriveMAC(): moreData = ", moreData, moreDataLen ));

    /* Derive HMAC using SHA1 algorithm */
    CsrHmacSha1( k_aut, (CsrUint32)EAP_SIM_K_AUT_KEY_LEN, allData, (CsrUint32) allDataLen, hmac);

    CsrMemCpy(mac, hmac, EAP_SIM_MAC_LEN);

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_deriveMAC(): Calculated MAC = ", mac, EAP_SIM_MAC_LEN ));

    CsrPfree(allData);

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_deriveMAC()" ));

    return eapSIM_SUCCESS;
}

static eEAP_SIM_ATT_RESULTS eapSIM_checkReceivedMAC( const CsrUint8 *k_aut,
                                                     eap_packet *reqData,
                                                     CsrUint16 reqDataLen,
                                                     const CsrUint8 *mac,
                                                     const CsrUint8 *moreData,
                                                     CsrUint16 moreDataLen )
{
    CsrUint8 hmac[CSR_SHA1_DIGEST_LENGTH];
    CsrUint16 allDataLen;
    CsrUint8 *allData;
    eEAP_SIM_ATT_RESULTS macCmpareRes=eapSIM_AT_INVALID;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_checkReceivedMAC()" ));

    allDataLen = reqDataLen + moreDataLen;
    allData = CsrPmalloc(allDataLen);
    if (allData == NULL)
    {
        sme_trace_debug(( TR_SECURITY, "eapSIM_checkReceivedMAC(): Memory Allocation Failure" ));
        return eapSIM_AT_INVALID;
    }

    CsrMemCpy(allData, reqData, reqDataLen);
    CsrMemSet(allData + (mac - (CsrUint8*)reqData) +4, 0, EAP_SIM_MAC_LEN);
    if ( (moreData != NULL) && (moreDataLen>0) )
    {
        CsrMemCpy(allData+reqDataLen, moreData, moreDataLen);
    }

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_checkReceivedMAC(): K_aut = ", k_aut, EAP_SIM_K_AUT_KEY_LEN ));

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_checkReceivedMAC(): reqData = ", reqData, reqDataLen ));

    if ( (moreData != NULL) && (moreDataLen>0) )
    {
        sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
            "eapSIM_checkReceivedMAC(): moreData = ", moreData, moreDataLen ));
    }
    else
    {
        sme_trace_info(( TR_SECURITY,
            "eapSIM_checkReceivedMAC(): No Message Specific Data within MAC" ));
    }

    /* Derive HMAC using SHA1 algorithm */
    CsrHmacSha1( k_aut, (CsrUint32)EAP_SIM_K_AUT_KEY_LEN, allData, (CsrUint32) allDataLen, hmac);

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_checkReceivedMAC(): Computed MAC = ", hmac, EAP_SIM_MAC_LEN ));

    /* Compare received and expected mac values */
    if (CsrMemCmp(hmac, mac+4, EAP_SIM_MAC_LEN) == 0)
    {
        macCmpareRes=eapSIM_AT_VALID;
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_checkReceivedMAC(): Successful - Computed MAC matches Received MAC" ));
    }
    else
    {
        macCmpareRes=eapSIM_AT_INVALID;
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_checkReceivedMAC(): Failure - Computed MAC does NOT match Received MAC" ));
    }

    CsrPfree(allData);

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_checkReceivedMAC()" ));

    return macCmpareRes;
}


static void eapSIM_AesEncrypt( CsrSimData *myData,
                               const CsrUint8 *k_encr,
                               const CsrUint8 *inData,
                               const CsrUint16 inDataLen,
                               CsrUint8 *outData,
                               const CsrUint8 *iv )
{
    CsrUint32 outDataLen;
    CsrUint8 *keyWrap= (CsrUint8*)&(k_encr[0]);

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_AesEncrypt()" ));

    CsrCryptoAes128CbcEncrypt( keyWrap,
                               (CsrUint8 *) iv,
                               (CsrUint8 *) inData,
                               (CsrUint32) inDataLen,
                               outData,
                               &outDataLen,
                               PAD_MODE_NONE );

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_AesEncrypt()" ));
}


static eEAP_SIM_RESULTS eapSIM_AesDecrypt( CsrSimData *myData,
                                           const CsrUint8 *k_encr,
                                           const CsrUint8 *encr_data,
                                           CsrUint16 encr_data_len,
                                           const CsrUint8 *iv,
                                           CsrUint8  msgSubType )
{
    CsrUint8 *decrypted;
    CsrUint32 decryptedLen=0;
    CsrUint8 *keyWrap= (CsrUint8*)&(k_encr[0]);
    CsrUint8 *current_att_offset;
    CsrUint16 remaining_atts_len;
    CsrUint8 current_att_type, current_att_lenField, current_att_len ;
    eEAP_SIM_ATT_RESULTS atNxtPseudoRes=eapSIM_AT_NOT_FOUND;
    eEAP_SIM_ATT_RESULTS atNxtFReauthRes=eapSIM_AT_NOT_FOUND;
    eEAP_SIM_ATT_RESULTS atPadRes=eapSIM_AT_NOT_FOUND;
    eEAP_SIM_ATT_RESULTS atnoncesRes=eapSIM_AT_NOT_FOUND;
    eEAP_SIM_ATT_RESULTS atcounterRes=eapSIM_AT_NOT_FOUND;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_AesDecrypt()" ));

    require ( TR_SECURITY, iv != NULL );
    require ( TR_SECURITY, encr_data != NULL );

    decrypted = CsrPmalloc(encr_data_len);
    if (decrypted == NULL)
    {
        sme_trace_debug(( TR_SECURITY, "eapSIM_AesDecrypt(): Memory Allocation Failure" ));
        return eapSIM_FAILURE;
    }

    /* Decrypt the encrypted data */
    CsrCryptoAes128CbcDecrypt( keyWrap,
                               (CsrUint8*) iv,
                               (CsrUint8*) encr_data,
                               (CsrUint32) encr_data_len,
                               decrypted,
                               &decryptedLen,
                               PAD_MODE_NONE );
    if (decryptedLen==0)
    {
        CsrPfree(decrypted);
        return eapSIM_FAILURE;
    }

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_AesDecrypt(): Decrypted AT_ENCR_DATA",
        decrypted, decryptedLen ));

    /*
     * Extract Attributes that may be there:
     * FullAuth: AT_NEXT_PSEUDONYM, EAP_SIM_AT_NEXT_REAUTH_ID and EAP_SIM_AT_PADDING
     * FastReAut: EAP_SIM_AT_NEXT_REAUTH_ID, AT_COUNTER, AT_NONCE_S and EAP_SIM_AT_PADDING
     * NotificationReq: AT_COUNTER and EAP_SIM_AT_PADDING
     *
     */

    remaining_atts_len = (CsrUint16) decryptedLen;
    current_att_offset = decrypted;
    while ( remaining_atts_len > 0 )
    {
        CsrUint16 actualNextPseudonymLen;
        CsrUint16 actualNextFastReauthIdLen;

        current_att_type = *current_att_offset;
        current_att_lenField  = *(current_att_offset + 1);
        current_att_len = (CsrUint8) (4 * current_att_lenField);

        if ( current_att_type == EAP_SIM_AT_NEXT_PSEUDONYM )
        {
            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_AesDecrypt(): Found AT_NEXT_PSEUDONYM (in AT_ENCRYPT_DATA) = ",
                current_att_offset, current_att_len ));

            atNxtPseudoRes = eapSIM_AT_INVALID; /* Found but Invalid until proven Valid */

            if ( msgSubType == EAP_SIM_SUBTYPE_CHALLENGE )
            {
                actualNextPseudonymLen = form16bitFrom2x8bit_se( *( current_att_offset + 2),
                                                                *( current_att_offset + 3) );

                if (current_att_len < actualNextPseudonymLen)
                {
                    sme_trace_error(( TR_SECURITY,
                        "eapSIM_AesDecrypt(): Invalid Length (%u) of AT_NEXT_PSEUDONYM)",
                        current_att_len ));

                    CsrPfree(decrypted);
                    return eapSIM_FAILURE;
                }

                atNxtPseudoRes = eapSIM_AT_VALID;

                (void) eapSIM_storeNextPseudonym( myData,
                                                  current_att_offset + 4,
                                                  actualNextPseudonymLen );
            }
            else if ( (msgSubType == EAP_SIM_SUBTYPE_REAUTHENTICATION)
                   || (msgSubType == EAP_SIM_SUBTYPE_NOTIFICATION) )
            {
                /* Not an error since skippable */
                sme_trace_debug(( TR_SECURITY,
                "eapSIM_AesDecrypt(): Unexpected AT_NEXT_PSEUDONYM in EAP-Request/SIM/Re-Authentication Packet)" ));
            }
        }
        else if ( current_att_type == EAP_SIM_AT_NEXT_REAUTH_ID )
        {
            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_AesDecrypt(): Found AT_NEXT_REAUTH_ID (in AT_ENCRYPT_DATA) = ",
                current_att_offset, current_att_len ));

            atNxtFReauthRes = eapSIM_AT_INVALID;  /* Found but Invalid until proven Valid */

            if ( (msgSubType == EAP_SIM_SUBTYPE_CHALLENGE)
              || (msgSubType == EAP_SIM_SUBTYPE_REAUTHENTICATION) )
            {
                actualNextFastReauthIdLen = form16bitFrom2x8bit_se( *( current_att_offset + 2),
                                                                    *( current_att_offset + 3) );

                if ( current_att_len < actualNextFastReauthIdLen )
                {
                    sme_trace_error(( TR_SECURITY,
                        "eapSIM_AesDecrypt(): Invalid Length (%u) of AT_NEXT_REAUTH_ID",
                        current_att_len ));

                    CsrPfree(decrypted);
                    return eapSIM_FAILURE;
                }

                atNxtFReauthRes = eapSIM_AT_VALID;

                (void) eapSIM_storeNextReauthId( myData,
                                                  current_att_offset + 4,
                                                  actualNextFastReauthIdLen );
            }
            else if ( msgSubType == EAP_SIM_SUBTYPE_NOTIFICATION)
            {
                /* Not an error since skippable */
                sme_trace_debug(( TR_SECURITY,
                    "eapSIM_AesDecrypt(): Unexpected AT_NEXT_REAUTH_ID in EAP-Request/SIM/Notification" ));
            }

        }
        else if ( current_att_type == EAP_SIM_AT_PADDING )
        {
            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_AesDecrypt(): Found AT_PADDING (in AT_ENCRYPT_DATA) = ",
                current_att_offset, current_att_len ));

            atPadRes = eapSIM_AT_INVALID;

            if ( eapSIM_checkPadding( current_att_offset, current_att_len ) == FALSE )
            {
                CsrPfree(decrypted);
                return eapSIM_FAILURE;
            }

            atPadRes = eapSIM_AT_VALID;
        }
        else if ( current_att_type == EAP_SIM_AT_NONCE_S )
        {
            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_AesDecrypt(): Found AT_NONCE_S (in AT_ENCRYPT_DATA) = ",
                current_att_offset, current_att_len ));

            atnoncesRes = eapSIM_AT_INVALID;

            if ( msgSubType == EAP_SIM_SUBTYPE_CHALLENGE )
            {
                sme_trace_error(( TR_SECURITY,
                "eapSIM_AesDecrypt(): Unexpected AT_NONCE_S in EAP-Request/SIM/Challenge Packet)" ));

                CsrPfree(decrypted);
                return eapSIM_FAILURE;
            }
            else if ( msgSubType == EAP_SIM_SUBTYPE_REAUTHENTICATION )
            {
                if ( current_att_len != IE_EAP_SIM__AT_NONCE_MS_LEN )
                {
                    sme_trace_error(( TR_SECURITY,
                        "eapSIM_AesDecrypt(): Invalid Length (%u) of AT_NONCE_S",
                        current_att_len ));

                    CsrPfree(decrypted);
                    return eapSIM_FAILURE;
                }

                atnoncesRes = eapSIM_AT_VALID;
                CsrMemCpy( myData->nonce_s, current_att_offset + 4, EAP_SIM_NONCE_S_LEN );
            }

        }
        else if ( current_att_type == EAP_SIM_AT_COUNTER )
        {
            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_AesDecrypt(): Found AT_COUNTER  (in AT_ENCRYPT_DATA) = ",
                current_att_offset, current_att_len ));

            atcounterRes = eapSIM_AT_INVALID;

            if ( msgSubType == EAP_SIM_SUBTYPE_CHALLENGE )
            {
                sme_trace_error(( TR_SECURITY,
                    "eapSIM_AesDecrypt(): Unexpected AT_COUNTER in EAP-Request/SIM/Challenge Packet)" ));

                CsrPfree(decrypted);
                return eapSIM_FAILURE;
            }
            else if ( msgSubType == EAP_SIM_SUBTYPE_REAUTHENTICATION )
            {
                CsrUint16 rxCounterValue;

                if ( current_att_len != IE_EAP_SIM__AT_COUNTER_LEN )
                {
                    sme_trace_error(( TR_SECURITY,
                        "eapSIM_AesDecrypt(): Invalid Length (%u) of AT_COUNTER",
                        current_att_len ));

                   CsrPfree(decrypted);
                   return eapSIM_FAILURE;
                }

                atcounterRes = eapSIM_AT_VALID;
                rxCounterValue = form16bitFrom2x8bit_se( *( current_att_offset + 2),
                                                         *( current_att_offset + 3) );
                if (myData->numFastReAuthRounds == 1)
                {
                    if ( rxCounterValue < 1 )
                    {
                        myData->counterValueMismatch = TRUE;
                    }
                }
                else
                {
                    if ( myData->counterValue > rxCounterValue )
                    {
                        myData->counterValueMismatch = TRUE;
                    }
                }

                myData->counterValue = rxCounterValue;
            }
        }

        current_att_offset += current_att_len;
        remaining_atts_len -= current_att_len;
    }

    CsrPfree(decrypted);

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_AesDecrypt()" ));

    if ( (atNxtPseudoRes == eapSIM_AT_VALID)
      && (atNxtFReauthRes == eapSIM_AT_VALID)
      && (atPadRes == eapSIM_AT_VALID)
      && (atcounterRes == eapSIM_AT_VALID)
      && (atnoncesRes == eapSIM_AT_VALID) )
    {
        return eapSIM_SUCCESS;
    }
    else
    {
        return eapSIM_FAILURE;
    }
}

/* ************************************************************************ */
/* ************************************************************************ */
/*                                                                          */
/*                  KEYS CALCULATION FUNCTIONS                              */
/*                                                                          */
/* ************************************************************************ */
/* ************************************************************************ */

static CsrUint16 increment_rand_history_index(CsrUint16 currentIndex)
{
    currentIndex++;
    return  (currentIndex % MAX_NUM_OF_RANDS_IN_HISTORY);
}

static eEAP_SIM_RAND_RESULTS eapSIM_validateReceivedRANDs( CsrSimContext *simContext,
                                                           CsrUint8 numChallenges,
                                                           CsrUint8 *RandsPtr )
{
    CsrSimData *myData = &simContext->simData;
    CsrSimConfig *myConfig = &simContext->simConfig;
    CsrUint8 *rand1=NULL, *rand2=NULL, *rand3=NULL;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_validateReceivedRANDs()" ));

    /* Check if we have received a valid number of RANDs */
    if ( numChallenges > EAP_SIM_MAX_NUM_CHALLENGES )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_validateReceivedRANDs(): Number of Received RANDs (%u) is HIGHER than maximum allowed (%u)",
            numChallenges, EAP_SIM_MAX_NUM_CHALLENGES ));

        return eapSIM_RAND_INVALID;
    }
    else if ( numChallenges < EAP_SIM_MIN_NUM_CHALLENGES )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_validateReceivedRANDs(): Number of Received RANDs (%u) is LOWER than minimum allowed (%u)",
            numChallenges, EAP_SIM_MIN_NUM_CHALLENGES ));

        return eapSIM_RAND_INSUFFICIENT;
    }


    /* Check if the received RANDs are all different */
    rand1 = RandsPtr;
    rand2 = RandsPtr + GSM_RAND_LEN;
    sme_trace_debug(( TR_SECURITY, "eapSIM_validateReceivedRANDs(): Checking if RANDs are Different" ));
    if (numChallenges == EAP_SIM_MIN_NUM_CHALLENGES)

    {
        if ( (CsrMemCmp( rand1 , rand2, GSM_RAND_LEN)== 0) )
        {
            sme_trace_error(( TR_SECURITY,
                "eapSIM_validateReceivedRANDs(): The %u RAND values must be different",
                EAP_SIM_MIN_NUM_CHALLENGES ));

            return eapSIM_RAND_INVALID;
        }
    }
    else if (numChallenges == EAP_SIM_MAX_NUM_CHALLENGES)
    {
        rand3 = RandsPtr + 2*GSM_RAND_LEN;

        if ( (CsrMemCmp( rand1, rand2, GSM_RAND_LEN)== 0)
          || (CsrMemCmp( rand1, rand3, GSM_RAND_LEN)== 0)
          || (CsrMemCmp( rand2, rand3, GSM_RAND_LEN)== 0) )
        {
            sme_trace_error(( TR_SECURITY,
                "eapSIM_validateReceivedRANDs(): The %u RAND values must be different",
                EAP_SIM_MAX_NUM_CHALLENGES ));

            return eapSIM_RAND_INVALID;
        }
    }

    /* Check if RANDs have not been used before */
    /* if not store them for future checking (erasing oldest entry if table is full */
    if ( myConfig->randHistoryOn )
    {
        CsrUint8 i;
        CsrUint8 currentStoredRand[GSM_RAND_LEN];
        CsrUint16 newIndex;

        sme_trace_debug(( TR_SECURITY, "eapSIM_validateReceivedRANDs(): Checking if RANDs are Fresh" ));
        newIndex = myData->randHistoryIndex;
        for ( i=0 ; i< newIndex ; i++ )
        {
            CsrMemCpy(currentStoredRand, myData->randHistory[i], GSM_RAND_LEN );
            if ( (CsrMemCmp( currentStoredRand, rand1, GSM_RAND_LEN)== 0)
              || (CsrMemCmp( currentStoredRand, rand2, GSM_RAND_LEN)== 0)
              || ((numChallenges == EAP_SIM_MAX_NUM_CHALLENGES) && (CsrMemCmp( currentStoredRand, rand3, GSM_RAND_LEN)== 0)))
            {
                sme_trace_error(( TR_SECURITY, "eapSIM_validateReceivedRANDs(): Received RANDs are Not Fresh" ));

                return eapSIM_RAND_NOT_FRESH;
            }
        }

        /* Sotre Rand values in history */
        sme_trace_debug(( TR_SECURITY, "eapSIM_validateReceivedRANDs(): Storing RANDs in history" ));
        CsrMemCpy( myData->randHistory[newIndex], rand1, GSM_RAND_LEN );

        newIndex = increment_rand_history_index(newIndex);
        CsrMemCpy( myData->randHistory[newIndex], rand2, GSM_RAND_LEN );

        if (numChallenges == EAP_SIM_MAX_NUM_CHALLENGES)
        {
            newIndex = increment_rand_history_index(newIndex);
            CsrMemCpy( myData->randHistory[newIndex], rand3, GSM_RAND_LEN );
        }

        /* keep track of next free position */
        myData->randHistoryIndex = increment_rand_history_index(newIndex);

    }

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_validateReceivedRANDs()" ));

    return eapSIM_RAND_VALID;
}


static eEAP_SIM_RESULTS eapSIM_getKcAndSRES( CsrUint8* inRand,
                                             CsrUint8* outKc,
                                             CsrUint8* outSRES )
{
    eEAP_SIM_RESULTS outcome = eapSIM_SUCCESS ;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_getKcAndSRES()" ));

    if ( ! CsrMemCmp( inRand, hardCodedRand_0, GSM_RAND_LEN ) )
    {
        CsrMemCpy( outKc, hardCodedKc_0, EAP_SIM_KC_LEN);
        CsrMemCpy( outSRES, hardCodedSRES_0, EAP_SIM_SRES_LEN);
    }
    else if ( ! CsrMemCmp( inRand, hardCodedRand_1, GSM_RAND_LEN ) )
    {
        CsrMemCpy( outKc, hardCodedKc_1, EAP_SIM_KC_LEN);
        CsrMemCpy( outSRES, hardCodedSRES_1, EAP_SIM_SRES_LEN);
    }
    else if ( ! CsrMemCmp( inRand, hardCodedRand_2, GSM_RAND_LEN ) )
    {
        CsrMemCpy( outKc, hardCodedKc_2, EAP_SIM_KC_LEN);
        CsrMemCpy( outSRES, hardCodedSRES_2, EAP_SIM_SRES_LEN);
    }
    else
    {
        outcome = eapSIM_FAILURE ;
        sme_trace_error(( TR_SECURITY,
            "eapSIM_getKcAndSRES(): Cannot match RAND value" ));
    }

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_getKcAndSRES()" ));

    return outcome;
}

static eEAP_SIM_RESULTS eapSIM_calculateGSMkeys( CsrSimContext *simContext )
{
    CsrSimData *myData = &simContext->simData;
    CsrUint8 k;

    for (k=0 ; k< myData->numRANDs ; k++)
    {
        sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
            "eapSIM_calculateGSMkeys(): Input RAND:",
            myData->gsmTriplet[k].RAND, GSM_RAND_LEN ));

        if ( eapSIM_getKcAndSRES( (CsrUint8 *) myData->gsmTriplet[k].RAND,
                                  (CsrUint8 *) myData->gsmTriplet[k].Kc,
                                  (CsrUint8 *) myData->gsmTriplet[k].SRES ) == eapSIM_SUCCESS )
        {
            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_calculateGSMkeys(): Calculated SRES:",
                myData->gsmTriplet[k].SRES, EAP_SIM_SRES_LEN ));

            sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
                "eapSIM_calculateGSMkeys(): Calculated Kc:",
                myData->gsmTriplet[k].Kc, EAP_SIM_KC_LEN ));
        }
        else
        {
            return eapSIM_FAILURE;
        }
    }

    return eapSIM_SUCCESS;
}


static eEAP_SIM_RESULTS eapSIM_calculateMasterKey( CsrSimContext* simContext,
                                                   const CsrUint8 *identity,
                                                   const CsrUint16 identityLen )

{
    CsrSimData *myData = &simContext->simData;

    CsrUint8 mySelectedVersion[EAP_SIM_SEL_VERSION_LEN];
    CsrUint16 concatOffset;
    CsrUint16 allDataLen;
    CsrUint8 *allData;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_calculateMasterKey()" ));

    /* As per rfc-4186: The Master Key for Full Authentication is given by:
     *
     *  SHA1 ( Identity | n*Kc | NONCE_MT | Version List | Selected Version )
     *
     *  So concatenate all input data (in the right order and then
     *  feed the resulting vector into the SHA-1 function.
     */

    /* Obtain Total Length */
    allDataLen = identityLen + myData->numRANDs * EAP_SIM_KC_LEN + EAP_SIM_NONCE_MT_LEN + myData->versionListLen + 2;

    /* Allocate Memory for buffer holding the concatenated data */
    allData = (CsrUint8 *) CsrPmalloc(allDataLen);
    if (allData == NULL)
    {
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_calculateMasterKey(): Memory Allocation Failure" ));
        return eapSIM_FAILURE;
    }

    /* Selected Version in Network Byte order before we feed it into the SHA-1 */
    form2x8bitsFrom16bit( myData->selectedVersion, &mySelectedVersion[1], &mySelectedVersion[0] );

    sme_trace_debug(( TR_SECURITY, "eapSIM_calculateMasterKey(): Idendity Length = %d", identityLen));
    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_calculateMasterKey(): Idendity = ", identity, identityLen));

    sme_trace_debug(( TR_SECURITY, "eapSIM_calculateMasterKey(): n*Kc Length = %d", myData->numRANDs * EAP_SIM_KC_LEN ));
    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_calculateMasterKey(): Kc[0] = ", myData->gsmTriplet[0].Kc, EAP_SIM_KC_LEN ));
    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_calculateMasterKey(): Kc[1] = ", myData->gsmTriplet[1].Kc, EAP_SIM_KC_LEN ));
    if ( myData->numRANDs == EAP_SIM_MAX_NUM_CHALLENGES )
    {
        sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_calculateMasterKey(): Kc[2] = ", myData->gsmTriplet[2].Kc, EAP_SIM_KC_LEN ));
    }

    sme_trace_debug(( TR_SECURITY, "eapSIM_calculateMasterKey(): nonce_mt Length = %d", EAP_SIM_NONCE_MT_LEN ));
    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_calculateMasterKey(): nonce_mt  = ", myData->nonce_mt, EAP_SIM_NONCE_MT_LEN ));

    sme_trace_debug(( TR_SECURITY, "eapSIM_calculateMasterKey(): Ver_List Length = %d", myData->versionListLen ));
    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "eapSIM_calculateMasterKey(): Ver_List = ", myData->versionList, myData->versionListLen ));

    sme_trace_debug(( TR_SECURITY, "eapSIM_calculateMasterKey(): Selected Version Length = %d", EAP_SIM_SEL_VERSION_LEN ));
    sme_trace_debug(( TR_SECURITY, "eapSIM_calculateMasterKey(): Selected Version = %d", myData->selectedVersion ));

    /* Concatenate items in the right order */
    CsrMemCpy(allData, identity, identityLen);                                              concatOffset  = identityLen;
    CsrMemCpy(allData + concatOffset, myData->gsmTriplet[0].Kc, EAP_SIM_KC_LEN);            concatOffset += EAP_SIM_KC_LEN;
    CsrMemCpy(allData + concatOffset, myData->gsmTriplet[1].Kc, EAP_SIM_KC_LEN);            concatOffset += EAP_SIM_KC_LEN;
    if ( myData->numRANDs == EAP_SIM_MAX_NUM_CHALLENGES )
    {
        CsrMemCpy(allData + concatOffset, myData->gsmTriplet[2].Kc, EAP_SIM_KC_LEN);            concatOffset += EAP_SIM_KC_LEN;
    }
    CsrMemCpy(allData + concatOffset, myData->nonce_mt, EAP_SIM_NONCE_MT_LEN);              concatOffset += EAP_SIM_NONCE_MT_LEN;
    CsrMemCpy(allData + concatOffset, myData->versionList, myData->versionListLen);         concatOffset += myData->versionListLen;
    CsrMemCpy(allData + concatOffset, mySelectedVersion, EAP_SIM_SEL_VERSION_LEN );

    /*****************************************
     *
     *    Calculate Master Key using SHA1
     *
     *****************************************/
    CsrCryptoSha1( allData,
                   allDataLen,
                   myData->mk );

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_calculateMasterKey(): Master Key = ", myData->mk, EAP_SIM_MASTER_KEY_LEN ));

    CsrPfree( allData );

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_calculateMasterKey()" ));

    return eapSIM_SUCCESS;
}

static eEAP_SIM_RESULTS eapSIM_calculateMasterKeyReauth( CsrSimContext* simContext,
                                                         const CsrUint8 *identity,
                                                         CsrUint16 identityLen )
{
    CsrSimData *myData = &simContext->simData;

    CsrUint8 myCounter[2];
    CsrUint16 concatOffset;
    CsrUint16 allDataLen;
    CsrUint8 *allData;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_calculateMasterKeyReauth()" ));

    /* As per rfc-4186: The Master Key for Re-Authentication is given by:
     *
     *  XKEY' = SHA1 ( Identity | counter | NONCE_S | MK )
     *
     *  So concatenate all input data (in the right order and then
     *  feed the resulting vector into the SHA-1 function.
     */
    allDataLen = identityLen + sizeof(CsrUint16) + EAP_SIM_NONCE_S_LEN + EAP_SIM_MASTER_KEY_LEN;

    /* Allocate Memory for buffer holding the concatenated data */
    allData = (CsrUint8 *) CsrPmalloc(allDataLen);
    if (allData == NULL)
    {
        sme_trace_debug(( TR_SECURITY,
            "eapSIM_calculateMasterKeyReauth(): Memory Allocation Failure" ));

        return eapSIM_FAILURE;
    }
    /* Counter Value in Network Byte order before we feed it into the SHA-1 */
    form2x8bitsFrom16bit( myData->counterValue, &myCounter[1], &myCounter[0] );

    /* Concatenate items in the right order */
    CsrMemCpy(allData, identity, identityLen);                                 concatOffset  = identityLen;
    CsrMemCpy(allData + concatOffset, myCounter, 2 );                          concatOffset += 2;
    CsrMemCpy(allData + concatOffset, myData->nonce_s, EAP_SIM_NONCE_S_LEN );  concatOffset += EAP_SIM_NONCE_S_LEN;
    CsrMemCpy(allData + concatOffset, myData->mk, EAP_SIM_MASTER_KEY_LEN);

    /***************************************************************
     *
     *    Calculate Master Key for Fast Re-Authentication using SHA1
     *
     ***************************************************************/
    CsrCryptoSha1( allData, allDataLen, myData->mkReauth );


    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_calculateMasterKeyReauth(): XKEYprime = ", myData->mkReauth, EAP_SIM_MASTER_KEY_LEN ));

    CsrPfree( allData );

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_calculateMasterKeyReauth()" ));

    return eapSIM_SUCCESS;
}

static eEAP_SIM_RESULTS eapSIM_calculateFurtherKeys( CsrSimContext *simContext )
{
    CsrUint8 *prfResultBuffer;
    CsrSimData *myData = &simContext->simData;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_calculateFurtherKeys()" ));

    /* Create Buffer (and Zero its contents) */
    prfResultBuffer = (CsrUint8 *) CsrPmalloc( EAP_SIM_PRF_RESULT_BUF_LEN );
    if (prfResultBuffer == NULL)
    {
        sme_trace_debug(( TR_SECURITY, "eapSIM_calculateFurtherKeys(): Memory Allocation Failure" ));
        return eapSIM_FAILURE;
    }
    CsrMemSet( prfResultBuffer, 0, EAP_SIM_PRF_RESULT_BUF_LEN );

    /**********************************************************************************************
     *
     * Calculate k_encr, k_aut, MSK and EMSK Keys using prf defined by FIPS186-2 with Change Notice
     *
     *********************************************************************************************/
    pseudoRandFunctionFIPS186_2_changeNotice( myData->mk,
                                              EAP_SIM_MASTER_KEY_LEN,
                                              prfResultBuffer,
                                              (CsrUint16)EAP_SIM_PRF_RESULT_BUF_LEN );

    /************************************************************************
     *
     * Copy portions of result buffer into each correspondig key respectively
     *
     ************************************************************************/

     /* Firstly: K_enr Key */
    CsrMemCpy( myData->k_encr,
               prfResultBuffer+EAP_SIM_PRF_RESULT_BUF_K_ENCR_OFFSET,
               EAP_SIM_K_ENCR_KEY_LEN );

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_calculateFurtherKeys(): K_encr Key =",
        myData->k_encr, EAP_SIM_K_ENCR_KEY_LEN ));

     /* Secondly: K_aut Key */
    CsrMemCpy( myData->k_aut,
               prfResultBuffer+EAP_SIM_PRF_RESULT_BUF_K_AUT_OFFSET,
               EAP_SIM_K_AUT_KEY_LEN );

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_calculateFurtherKeys(): K_aut Key =",
        myData->k_aut, EAP_SIM_K_AUT_KEY_LEN ));

     /* Thirdly: Master Session Key */
    CsrMemCpy( myData->msk,
               prfResultBuffer+EAP_SIM_PRF_RESULT_BUF_MSK_OFFSET,
               EAP_SIM_MASTER_SESSION_KEY_LEN );

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_calculateFurtherKeys(): Master Session Key (MSK) =",
        myData->msk, EAP_SIM_MASTER_SESSION_KEY_LEN ));

     /* Fourthly: Extended Master Session Key */
    CsrMemCpy( myData->emsk,
               prfResultBuffer+EAP_SIM_PRF_RESULT_BUF_EMSK_OFFSET,
               EAP_SIM_EXTENDED_MASTER_SESSION_KEY_LEN );

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_calculateFurtherKeys(): Extended Master Session Key (EMSK) =",
        myData->emsk, EAP_SIM_EXTENDED_MASTER_SESSION_KEY_LEN ));

    /* Free allocated memory */
    CsrPfree(prfResultBuffer);

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_calculateFurtherKeys()" ));

    return eapSIM_SUCCESS;
}


static eEAP_SIM_RESULTS eapSIM_calculateFurtherKeysReauth( CsrSimContext *simContext )
{
    CsrUint8 *prfResultBuffer;
    CsrSimData *myData = &simContext->simData;

    sme_trace_entry(( TR_SECURITY, ">> eapSIM_calculateFurtherKeysReauth()" ));

    /* Create Buffer (and Zero its contents) */
    prfResultBuffer = (CsrUint8 *) CsrPmalloc( EAP_SIM_PRF_RESULT_BUF_LEN );
    if (prfResultBuffer == NULL)
    {
        sme_trace_debug(( TR_SECURITY, "eapSIM_calculateFurtherKeysReauth(): Memory Allocation Failure" ));
        return eapSIM_FAILURE;
    }
    CsrMemSet( prfResultBuffer, 0, EAP_SIM_PRF_RESULT_BUF_LEN );

    /**********************************************************************************************
     *
     * Calculate k_encr, k_aut, MSK and EMSK Keys using prf defined by FIPS186-2 with Change Notice
     *
     *********************************************************************************************/
    pseudoRandFunctionFIPS186_2_changeNotice( myData->mkReauth,
                                              EAP_SIM_MASTER_KEY_LEN,
                                              prfResultBuffer,
                                             (CsrUint16) EAP_SIM_PRF_RESULT_BUF_LEN );

    /************************************************************************
     *
     * Copy portions of result buffer into each correspondig key respectively
     *
     ************************************************************************/

     /* Firstly: Master Session Key */
    CsrMemCpy( myData->msk,
               prfResultBuffer+EAP_SIM_PRF_RESULT_BUF_MSK_REAUTH_OFFSET,
               EAP_SIM_MASTER_SESSION_KEY_LEN );

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_calculateFurtherKeysReauth(): Reauth Master Session Key (MSK) =",
        myData->msk, EAP_SIM_MASTER_SESSION_KEY_LEN ));

     /* Secondly: Extended Master Session Key */
    CsrMemCpy( myData->emsk,
               prfResultBuffer+EAP_SIM_PRF_RESULT_BUF_EMSK_REAUTH_OFFSET,
               EAP_SIM_EXTENDED_MASTER_SESSION_KEY_LEN );

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_calculateFurtherKeysReauth(): Reauth Extended Master Session Key (EMSK) =",
        myData->emsk, EAP_SIM_EXTENDED_MASTER_SESSION_KEY_LEN ));

    /* Free allocated memory */
    CsrPfree(prfResultBuffer);

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_calculateFurtherKeysReauth()" ));

    return eapSIM_SUCCESS;
}


static void pseudoRandFunctionFIPS186_2_changeNotice( const CsrUint8 *seedKey,
                                                      const CsrUint16 seedKeyLen,
                                                      CsrUint8 *x_in,
                                                      CsrUint16 xLen)
{
    CsrUint8  XKEY[EAP_SIM_MASTER_KEY_LEN];
    CsrUint8  XVAL[EAP_SIM_MASTER_KEY_LEN];
    CsrUint8  w[2*EAP_SIM_MASTER_KEY_LEN];
    CsrUint8  m, j, i;

    CsrUint8 *x = x_in;

    CsrBignum *oneVal_Bn, *mod_2Pow160_bn;

    sme_trace_debug(( TR_SECURITY, ">> pseudoRandFunctionFIPS186_2()" ));

    require ( TR_SECURITY, x != NULL );

    /* Create 2^16 as big number */
    mod_2Pow160_bn = CsrCryptoBnHexToBn( EAP_SIM_PRF_CONST_2POW160, NULL );

    /* Create 1 as Big number */
    oneVal_Bn = CsrCryptoBnOneNew();

    /* seedKey is Master Key so must have same length */
    if ( seedKeyLen != EAP_SIM_MASTER_KEY_LEN)
    {
        sme_trace_warn(( TR_SECURITY,
            "pseudoRandFunctionFIPS186_2(): Seed Key Length (%d) is different than of Master Key Length (%d)",
            seedKeyLen, EAP_SIM_MASTER_KEY_LEN ));
    }

    /* Set the Number of iteration so as to Make the output size match the size of all the keys */
    /* ie: (160 bytes / 40 bytes) (or 1280 bits / 160 bits) gives Four iterations */
    /* hence: 4x40 bytes = Sizeof(Kaut+Kencr+MSK+EMSK) = 160 bytes */
    m = (CsrUint8) (xLen/EAP_SIM_PRF_OUTPUT_xj_SIZE);
    sme_trace_debug(( TR_SECURITY, "pseudoRandFunctionFIPS186_2(): Num of Iterations is %d", m ));

    CsrMemCpy(XKEY, seedKey, seedKeyLen);

    /* Zero w values */
    CsrMemSet(w, 0, 2*EAP_SIM_MASTER_KEY_LEN);

    for (j=0 ; j<m ; j++)
    {
        for (i=0 ; i<=1 ; i++)
        {
            CsrBignum *XKEY_Bn, *w_Bn, *sum_bn, *sum2_bn;
            CsrUint8  sum2[EAP_SIM_MASTER_KEY_LEN];
            CsrUint32 NumBytes;

            /* XVAL = (XKEY + XSEED_j) mod 2^b */
                CsrMemCpy( XVAL, XKEY, EAP_SIM_MASTER_KEY_LEN );

            /* w_i = G(t, XVAL) */
                CsrCryptoSha1ZeroPadding( XVAL, EAP_SIM_MASTER_KEY_LEN, w+i*EAP_SIM_MASTER_KEY_LEN );

            /* XKEY = (1 + XKEY + w_i) mod 2^b */
                XKEY_Bn = CsrCryptoBnBinToBn(XKEY, (CsrUint32) EAP_SIM_MASTER_KEY_LEN , NULL );
                w_Bn = CsrCryptoBnBinToBn(w+EAP_SIM_MASTER_KEY_LEN*i, (CsrUint32) EAP_SIM_MASTER_KEY_LEN , NULL );

                sum_bn = CsrCryptoBnNew();
                CsrCryptoBnUnsignedAddMod( sum_bn, XKEY_Bn, w_Bn, mod_2Pow160_bn ); /* sum_Bn = XKEY_Bn + W_Bn mod 2^160 */

                sum2_bn = CsrCryptoBnNew();
                CsrCryptoBnUnsignedAddMod( sum2_bn, sum_bn, oneVal_Bn, mod_2Pow160_bn ); /* sum2_Bn = sum_Bn +1 mod 2^160 */

                NumBytes = CsrCryptoBnNumOctets(sum2_bn);
                if ( NumBytes> EAP_SIM_MASTER_KEY_LEN )
                {
                    sme_trace_error(( TR_SECURITY,
                    "pseudoRandFunctionFIPS186_2(): Unacceptable Num of Bytes in sum2  (%d)", NumBytes ));
                }


                (void) CsrCryptoBnBnToBin( sum2_bn, sum2); /* Convert result to non-big num representation */
                CsrMemCpy(XKEY,sum2, EAP_SIM_MASTER_KEY_LEN );

            /* Free Memory */
            CsrCryptoBnFree(XKEY_Bn);
            CsrCryptoBnFree(w_Bn);
            CsrCryptoBnFree(sum_bn);
            CsrCryptoBnFree(sum2_bn);
        }

        /* x_j = w_0 | w_1 */
        CsrMemCpy ( x+2*EAP_SIM_MASTER_KEY_LEN*j, w, 2*EAP_SIM_MASTER_KEY_LEN);

        /* sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG, "x = ", x, 160 )); */
    }

    /* Free Memory */
    CsrCryptoBnFree(oneVal_Bn);
    CsrCryptoBnFree(mod_2Pow160_bn);

    sme_trace_debug(( TR_SECURITY, "<< pseudoRandFunctionFIPS186_2()" ));
}


/* ************************************************************************ */
/* ************************************************************************ */
/*                                                                          */
/*                  VARIOUS UTILITIES FUNCTIONS                             */
/*                                                                          */
/* ************************************************************************ */
/* ************************************************************************ */

static CsrBool eapSIM_isNonSkippableAttribute (CsrUint8 attribId)
{
    return (attribId <= EAP_SIM_ATT_SKIP_THRESHOLD ? TRUE : FALSE);
}

static CsrBool eapSIM_isPhaseBitZero(CsrUint16 notifxCode)
{
    return ( (notifxCode & EAP_SIM_PBIT_MASK)==0 ? TRUE : FALSE );
}

static CsrBool eapSIM_checkPadding( CsrUint8 *buffer,
                                    CsrUint8 BufferLen )
{
    CsrUint8 i;

    for (i=0 ; i<BufferLen ; i++)
    {
        if ( *(buffer+i) )
        {
            sme_trace_error(( TR_SECURITY,
                "eapSIM_checkPadding(): Found Non-zero byte within AT_PADDING" ));
            return FALSE;
        }
    }

    return TRUE;
}

static eEAP_SIM_RESULTS eapSIM_storeNextReauthId( CsrSimData *myData,
                                                  CsrUint8 *nextId,
                                                  CsrUint16 nextIdLen )
{
    sme_trace_entry(( TR_SECURITY, ">> eapSIM_storeNextReauthId()"));

    /* Get Rid of Old FastReauth-Id */
    CsrPfree( myData->reauthId );

    /* Allocate Memory for New FastReauth-Id */
    myData->reauthId = CsrPmalloc( nextIdLen );
    if ( myData->reauthId == NULL )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_storeNextReauthId(): No memory for storing next FastReAuth-Id" ));

        return eapSIM_FAILURE;
    }

    /* Store New FastReauth-Id */
    CsrMemCpy( myData->reauthId, nextId, nextIdLen );
    myData->reauthIdLen = nextIdLen;

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_storeNextReauthId():  Sucessfully stored the Next FastReauth-Id",
        myData->reauthId, myData->reauthIdLen ));

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_storeNextReauthId()"));

    return eapSIM_SUCCESS;
}


static eEAP_SIM_RESULTS eapSIM_storeNextPseudonym( CsrSimData *myData,
                                                   CsrUint8 *nextPseud,
                                                   CsrUint16 nextPseudLen )
{
    sme_trace_entry(( TR_SECURITY, ">> eapSIM_storeNextPseudonym()"));

    /* Get Rid of Old Pseudonym */
    CsrPfree( myData->pseudonym );

    /* Allocate Memory for New Pseudonym */
    myData->pseudonym = CsrPmalloc( nextPseudLen);
    if ( myData->pseudonym == NULL )
    {
        sme_trace_error(( TR_SECURITY,
            "eapSIM_storeNextPseudonym(): No memory for storing next Pseudonym" ));

            return eapSIM_FAILURE;
    }

    /* Store New Pseudonym */
    CsrMemCpy( myData->pseudonym, nextPseud, nextPseudLen );
    myData->pseudonymLen = nextPseudLen;

    sme_trace_hex(( TR_SECURITY, TR_LVL_DEBUG,
        "eapSIM_storeNextPseudonym():  Sucessfully stored the Next Pseudonym",
        myData->pseudonym, myData->pseudonymLen ));

    sme_trace_entry(( TR_SECURITY, "<< eapSIM_storeNextPseudonym()"));

    return eapSIM_SUCCESS;
}

/* concat 2 consecutive bytes (in a message) into a 16-byte number (small endian version) */
CsrUint16 form16bitFrom2x8bit_se( CsrUint8 msbyte, CsrUint8 lsbyte )
{
    CsrUint16 res16bitnum;

    res16bitnum= ((CsrUint16) (((CsrUint16)msbyte)<< 8)) | ((CsrUint16)lsbyte) ;

    return res16bitnum;
}

void form2x8bitsFrom16bit( CsrUint16 num16, CsrUint8 *lsbNum, CsrUint8 *msbNum )
{
    *msbNum = ((CsrUint8) ((num16>> 8) & 0x00FF));
    *lsbNum = ((CsrUint8) (num16 & 0x00FF));
}

/* ************************************************************************ */
/* ************************************************************************ */
/*                                                                          */
/*                  CSR SECURITY FRAMEWORK FUNCTIONS                        */
/*                                                                          */
/* ************************************************************************ */
/* ************************************************************************ */

static void SimDeinit(eapMethod* eap_method)
{
    CsrSimContext* simContext = (CsrSimContext*)eap_method->methodContext;

    sme_trace_entry(( TR_SECURITY_LIB, ">> SimDeinit()"));

    eapSIM_deinit( simContext );

    CsrPfree(eap_method->methodContext);
    eap_method->methodContext = NULL;

    sme_trace_entry(( TR_SECURITY_LIB, "<< SimDeinit()"));
}

static CsrBool SimCheck(eapMethod* eap_method, eap_packet *eapReqData, CsrUint16 size)
{
    sme_trace_entry(( TR_SECURITY_LIB, ">> SimCheck()"));

    /* All Checking is done by the Process Method */

    sme_trace_entry(( TR_SECURITY_LIB, "<< SimCheck()"));

    return TRUE;
}

static void SimProcess(eapMethod* eap_method, eap_packet *eapReqData, CsrUint8 reqId)
{
    CsrUint16 eapRspLength;
    CsrSimContext* simContext = (CsrSimContext*)eap_method->methodContext;
    eapol_packet *eapol = (eapol_packet *)simContext->securityContext->buffer;

    sme_trace_entry((TR_SECURITY_LIB, ">> SimProcess"));

    /* Fill out the normal EAP header (except length which need to be calculated) */
    eapol->version = 1;
    eapol->packet_type = EAPOL_PACKET_TYPE_EAP;
    eapol->u.eap.code = EAP_CODE_RESP;
    eapol->u.eap.id = reqId;
    /* EAP-SIM type */
    eapol->u.eap.u.resp.type = simContext->eapType;

    /* The rest of the message filled here */
    /*  Also length calculated here as depends on message contents */
    (void) eapSIM_process( eap_method, simContext, eapReqData, eapol, &eapRspLength );

    simContext->dataref.dataLength = CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH + eapRspLength;
    simContext->dataref.buf = (CsrUint8 *)eapol;

    EAPOL_LENGTH_ASSIGN(eapol, simContext->dataref.dataLength - (CSR_WIFI_SECURITY_SNAP_LENGTH + EAPOL_HEADER_LENGTH));

    sme_trace_entry((TR_SECURITY_LIB, "<< SimProcess"));
}

static DataBuffer SimBuildResp(eapMethod* eap_method, CsrUint8 reqId)
{
    CsrSimContext* simContext = (CsrSimContext*)eap_method->methodContext;

    sme_trace_entry((TR_SECURITY_LIB, ">> SimBuildResp"));

    return simContext->dataref;
}

static eapKey *SimGetKey(eapMethod* eap_method)
{
    CsrSimContext* simContext = (CsrSimContext*)eap_method->methodContext;

    return &simContext->key;
}

/* This is the method interface for use by the EAP state machine. */
static const eapMethod sim_method =
{
    NULL,  /* Pointer to local Context */
    NONE,  /* state */
    FAIL,  /* decision */
    FALSE, /* allow notifications */
    FALSE, /* isKeyAvaiable */
    FALSE, /* isEapolKeyHint */
    /* Next: Pointers to functions each of which will call
     * one or several of my defined functions ...
     */
    SimDeinit,
    SimCheck,
    SimProcess,
    SimBuildResp,
    SimGetKey
};

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in eap_sim.h
 */
/*---------------------------------------------------------------------------*/
void SimEapMethodInit(CsrWifiSecurityContext* context, eapMethod* method)
{
    CsrSimContext* simContext = (CsrSimContext*) CsrPmalloc(sizeof(CsrSimContext));

    sme_trace_info((TR_SECURITY_LIB, "SimEapMethodInit"));
    CsrMemCpy(method, &sim_method, sizeof(sim_method));
    method->methodContext = simContext;

    simContext->securityContext = context;
    simContext->eapType = EAP_TYPE_SIM;

    eapSIM_init( simContext );
}

