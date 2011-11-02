/** @file hip_interface.h
 *
 * HIP Interfaces API
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
 *   HIP Interface API
 *
 ****************************************************************************
 *
 * @section REVISION
  *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/hip_interface.h#2 $
 *
 ****************************************************************************/
#ifndef HIP_INTERFACE_H
#define HIP_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup link_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_internal.h"
#include "fsm/fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "hostio/hip_fsm_types.h"
#include "ie_access/ie_access.h"

/* PUBLIC TYPES DEFINITIONS ************************************************************/

typedef struct pal_hip_llc_snap_hdr_t
{
    CsrUint8    dsap;   /* always 0xAA */
    CsrUint8    ssap;   /* always 0xAA */
    CsrUint8    ctrl;   /* always 0x03 */
    CsrUint8    oui[3];    /* organizational universal id */
    CsrUint16 protocol;
}pal_hip_llc_snap_hdr_t;

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* AppHandle assigned such that the unitdataCfm can be routed properly to right instance in the right context. 
 * It takes only 1 byte. Most significant 2 bits is the appHandle and least 6 bits is the instance id
 */
#define PAL_SYS_APP_HANDLE(context) (void *)((UNIFI_SME_APPHANDLE << 6) | (context->currentInstance->instanceId))


/* CWMin is not available to access from the unifi firmware. Hence the default
 * as per the IEEE 2007 table 15-2 of section 15.3.3  OR or as per
 * table 19-7 section 19.8.4 - Its not clear which one of these is the right PHY for Unifi.
 */
#define PAL_HIP_DSSS_DEFAULT_CW_MIN (31) /* This value could 15 according to table 19-7. Not clear which one to pick?? - FIXME */
#define PAL_HIP_DSSS_SIFS_TIME (10) /* In microseconds*/
#define PAL_HIP_DSSS_SHORT_SLOT_TIME_PERIOD (9) /* In micro seconds */


#define PAL_HIP_SIG_CAP_QOS                    0x200

#define PAL_HIP_IE_HEADER_SIZE (2) /* 1 byte id + 1 byte len field */


/* RSN IE length */
#define PAL_HIP_IE_RSN_DATA_SIZE (20) /* as per section 7.3.2.25 IEEE 2007 Spec */
#define PAL_HIP_IE_RSN_SIZE (PAL_HIP_IE_HEADER_SIZE + PAL_HIP_IE_RSN_DATA_SIZE)

#define PAL_HIP_SNAP_HEADER_SIZE (8)

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the ip connection manager state machine data
 */

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/
/** @}
 */

/**
 * @brief
 *   Function populates and sends MLME-START-REQ
 *
 * @par Description
 *   All relevant IEs are populated by this function before sending the HIP message.
 *
 * @param[in]    context : FSM Context
 * @param[in]    iftype : interface type
 * @param[in]    channelNo : channel on which the network needs to be started
 *
 * @return void
 */
extern void pal_hip_send_mlme_start_req(FsmContext *context,
                                              Interface iftype,
                                              ChannelNumber channelNo);


/**
 * @brief
 *   Function populates and sends MLME-AUTHENTICATE-REQ
 *
 * @par Description
 *   All relevant IEs are populated by this function before sending the HIP message.
 *
 * @param[in]    context : FSM Context
 * @param[in]    peerStaAddress : remote MAC address
 *
 * @return void
 */
extern void pal_hip_send_mlme_authenticate_req(FsmContext *context, const unifi_MACAddress *peerStaAddress);

/**
 * @brief
 *   Function populates and sends MLME-ASSOCIATE-REQ
 *
 * @par Description
 *   All relevant IEs are populated by this function before sending the HIP message.
 *
 * @param[in]    context : FSM Context
 * @param[in]    peerStaAddress : remote MAC address
 *
 * @return void
 */
extern void pal_hip_send_mlme_associate_req(FsmContext *context, const unifi_MACAddress *peerStaAddress);

/**
 * @brief
 *   Function populates and sends MLME-DEAUTHENTICATE-REQ
 *
 * @par Description
 *   All relevant IEs are populated by this function before sending the HIP message.
 *
 * @param[in]    context : FSM Context
 * @param[in]    peerStaAddress : remote MAC address
 *
 * @return void
 */
extern void pal_hip_send_mlme_deauthenticate_req(FsmContext *context, const unifi_MACAddress *peerStaAddress);

/**
 * @brief
 *   Function populates and sends MLME-ASSOCIATE-RSP
 *
 * @par Description
 *   All relevant IEs are populated by this function before sending the HIP message.
 *
 * @param[in]    context : FSM Context
 * @param[in]    resultCode : result of association
 * @param[in]    peerStaAddress : remote MAC address
 * @param[in]    associationId : association ID as received in association request
 *
 * @return void
 */
extern void pal_hip_send_mlme_associate_rsp(FsmContext *context,
                                                   ResultCode resultCode,
                                                   const unifi_MACAddress *peerStaAddress,
                                                   AssociationId associationId);

/**
 * @brief
 *   Function populates and sends MLME-AUTHENTICATE-RSP
 *
 * @par Description
 *   All relevant IEs are populated by this function before sending the HIP message.
 *
 * @param[in]    context : FSM Context
 * @param[in]    resultCode : result of authentication
 * @param[in]    peerStaAddress : remote MAC address
 *
 * @return void
 */
extern void pal_hip_send_mlme_authenticate_rsp(FsmContext *context,
                                                      ResultCode resultCode,
                                                      const unifi_MACAddress *peerStaAddress);

/**
 * @brief
 *   Function populates and sends MA-UNITDATA--REQ
 *
 * @par Description
 *   All relevant IEs are populated by this function before sending the HIP message.
 *
 * @param[in]    context : FSM Context
 * @param[in]    peerStaAddress : remote MAC address
 * @param[in]    qosSupported : boolean to determine the access class of data request
 * @param[in]    data : acutal data to send
 * @param[in]    dataLength : length of data
 * @param[in]    protocolId : the SNAP protocol ID for the data (Security, Activity Report etc).
 * @param[in]    hostTag : tag to map the tx confirmation for this data request
 *
 * @return void
 */
extern void pal_hip_send_ma_unitdata_req(FsmContext *context,
                                               const unifi_MACAddress *peerStaAddress,
                                               CsrBool qosSupported,
                                               const CsrUint8 *data,
                                               CsrUint16 dataLength,
                                               CsrUint16 protocolId,
                                               CsrUint32 hostTag);

/**
 * @brief
 *   Function populates and sends MLME-SETKEYS-REQ
 *
 * @par Description
 *   All relevant IEs are populated by this function before sending the HIP message.
 *
 * @param[in]    context : FSM Context
 * @param[in]    resultCode : result of authentication
 * @param[in]    peerStaAddress : remote MAC address
 *
 * @return void
 */
extern void pal_hip_send_mlme_setkeys_req(FsmContext *context,
                                                 KeyType keyType,
                                                 const CsrUint8 *key,
                                                 CsrUint32 keyLength,
                                                 CsrUint16 keyId,
                                                 CsrUint16 *recvSeqCount,
                                                 unifi_MACAddress *remoteMacAddress,
                                                 CsrBool authenticator_or_supplicant);

/**
 * @brief
 *   Function access pld and finds a specific IE from an IE vector
 *
 * @par Description
 *
 * @param[in]    context : FSM Context
 * @param[out]    ieDataRef : pointer to data reference
 * @param[in]    length : IE ID to find
 *
 * @return pointer to the IE location if IE is found else return NULL
 */
extern CsrUint8 * pal_hip_ie_parameter_check(FsmContext *context,
                                               const DataReference *ieDataRef,
                                               ie_elementid id);

/**
 * @brief
 *   Function calculates and returns CWMin for a particular User Priority
 *
 * @par Description
 *  Calculate CWMin for the corresponding Accesss class according to the table 7-37 in IEEE 2007 spec
 *
 * @param[in]    dot11HCCWmin : CWMin default in MIB
 * @param[in]    up : User Priority for which the CWMin needs to be calculated
 *
 * @return 0 if failed or the value.
 */
extern CsrUint16 pal_hip_get_cwmin_for_user_priority(CsrUint16 dot11HCCWmin, Priority up);

/**
 * @brief
 *   Function calculates minimum latency
 *
 * @par Description
 *
 * @param[in]    dot11HCCWmin : CWMin default in MIB
 *
 * @return min latency in microseconds
 */
extern CsrUint32 pal_hip_get_min_latency(CsrUint16 dot11HCCWmin);

/**
 * @brief
 *   Function calculates and returns the maximum flush timeout
 *
 * @par Description
 * The value caculated will be the maximum value that PAL will use for its flush timers.
 *
 * @param[in]    dot11HCCWmin : CWMin default in MIB
 *
 * @return max flush timeout in microseconds
 */
extern CsrUint32 pal_hip_get_max_flush_timeout(CsrUint16 dot11HCCWmin);

/**
 * @brief
 *   Function calculates and returns the minimum flush timeout
 *
 * @par Description
 * The value caculated will be the minimum value that PAL will use for its flush timers.
 *
 * @param[in]    dot11HCCWmin : CWMin default in MIB
 *
 * @return max flush timeout in microseconds
 */
extern CsrUint32 pal_hip_get_min_flush_timeout(CsrUint16 dot11HCCWmin);

/**
 * @brief
 *   adds RSN IE to an IE vector
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 * @param[out]    ie_buf   : Pointer to position in the IE vector to write to.
 *
 * Notes:
 *      Assumes there is enough space in info for PAL_HIP_IE_RSN_DATA_SIZE bytes.
 *
 * @return void
 */
extern CsrUint16 pal_hip_build_rsn_element_ie(FsmContext *context,CsrUint8 *ie_buf);

extern void pal_hip_encode_snap(CsrUint8 *buf, pal_hip_llc_snap_hdr_t *snap);
extern void pal_hip_decode_snap(CsrUint8 *buf, pal_hip_llc_snap_hdr_t *snap);

#ifdef __cplusplus
}
#endif

#endif /* HIP_INTERFACE_H */
