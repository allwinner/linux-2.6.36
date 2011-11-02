/** @file pal_data_common.h
 *
 * PAL Data Common API
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
 *   Public Data Common API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/pal_data_common.h#4 $
 *
 ****************************************************************************/
#ifndef PAL_DATA_COMMON_H
#define PAL_DATA_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup ip_connection_mgr
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_internal.h"
#include "fsm/fsm_types.h"
#include "csr_cstl/csr_wifi_list.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"

/* PUBLIC MACROS ************************************************************/

/* should I move these macros to something like pal_config.h ??*/

/* number of concurrent physical links supported
 * Note: Make sure the macro PAL_DAM_MAX_PHYSICAL_LINKS in data_manager.c is updated as well
 * when you change this macro.
 */
#define PAL_MAX_PHYSICAL_LINKS (2)

/* Number of concurrent links supported. One is always for best effort. Alteast 2 links must be there
 * So that PAL supports one BE and one Guaranteed link
 */
#define PAL_MAX_LOGICAL_LINKS (2)
#define PAL_MAX_GUARANTEED_LOGICAL_LINKS (PAL_MAX_LOGICAL_LINKS-1) /* Number of concurrent guranteed links supported */
#define PAL_HCI_MAX_ALLOWED_COMMANDS (1) /* just one should do the job and avoid complications of handling multiple HCI commands pending */
#define PAL_CAPABILITIES (0) /*bit-0 -> Guranteed link not supported.*/
#define PAL_CAPABILITY_GURANTEED_LINK_SUPPORTED (1) /*bit-1 -> Guranteed link supported*/

/* as per section 6 of 802.11 PAL spec */
#define PAL_AMP_ASSOC_MAX_TOTAL_LENGTH (672)

/* expressed in Mbps. Value concluded by 802.11 PAL SIG during its investigations */
#define PAL_TOTAL_BANDWIDTH (24)

/* As per PAL spec clause 2.2
 * "An upper bound on the maximum data rate as seen by the application that the AMP
 * can guarantee for a logical channel. Any request made by an application above this
 * level would be rejected. It accounts for any bandwidth limitations of the HCI transport.
 * No sustained combination of transmit and receive operations shall exceed this value.
 * This can be used to help in AMP selection and admission control. Expressed in Mbps."
 * A reaslistic is assumed considering there can be best effort traffic. PAL has no means
 * to limit or police the best effort traffic. The policing is purely done on the basis
 * user priority. And arrived at this nominal figure 8 Mbps. Do you have a better reason
 * to increase the limit?
 */
#define PAL_MAX_GUARANTEED_BANDWIDTH (8)
#define PAL_MAX_PDU_SIZE (1492) /*as per clause 6 of PAL spec */
#define PAL_MAX_FLUSH_TIMEOUT_DEFAULT (84300) /* in microseconds .  */
#define PAL_MIN_LATENCY_DEFAULT (43) /* in microseconds .  */
#define PAL_MIN_SDU_INTER_ARRIVAL_TIME (100) /*FIXME: Not clear what minimum can be in microseconds??*/

/* Invalid physical link handle. This needs to be agreed with AMP Manager
 * as this is allocated by AMP Manager
 */
#define PAL_INVALID_PHYSICAL_LINK_HANDLE (0)
#define PAL_INVALID_LOGICAL_LINK_HANDLE (0xFFF)
#define PAL_HIP_FSM_HOST_TAG_ID PAL_INVALID_LOGICAL_LINK_HANDLE

/* Value determined on experimental basis
 * This value will help us to set a maximum flush timeout for logical links
 * Before a physical link is created, the dot11RTSThreshold is the default value (3000).
 * Hence the shortRetryLimit is used for retry attempts.
 * Once the link is created the RTS/CTS is enabled by default. henceforth LongRetryLimit
 * will be used for retry attempts. But if there is no SCO activity on the other side the RTS/CTS
 * needs to be disabled and retry attempts will start using shortRetryLimit again. So it is
 * important that both values are the same.
 */
#define PAL_MAX_DOT11_RETRY_LIMIT (100)
#define PAL_MIN_AVERAGE_RANDOM_BACKOFF_PER_RETRY (800) /* expressed in microseconds. Its random and guessed value. needs refining FIXME*/
#define PAL_MIN_DOT11_RETRY_LIMIT (7) /* only to determine a minimum flush timeout. again it must be greater than 1 */

/* get current instance id */
#define PAL_GET_CURRENT_INSTANCE_ID(context) (context->currentInstance->instanceId)

#define PAL_ENABLE_TIMER(timer) ((timer).enabled=TRUE)
#define PAL_DISABLE_TIMER(timer) ((timer).enabled=FALSE)
#define LINK_ACCEPT_TIMEOUT_VALUE_DEFAULT_IN_MILLIS (5000)

#define PAL_LINK_QUALITY_INDICATION_UNAVAILABLE 0x00
#define PAL_RSSI_INDICATION_UNAVAILABLE         0x80
#define PAL_MAX_CHANNEL_NUMBER                  0x000b
#define PAL_INVALID_CHANNEL_NUMBER (0xFF) /* range is 0-200 as per 802.11 2007 spec section 17.3.8.3.3*/

/* company id for CSR as in https://www.bluetooth.org/Technical/AssignedNumbers/identifiers.htm */
#define PAL_BT_SIG_COMPANY_IDENTIFIER 0x000a

/* PAL version according to section 2.1 of PAL Spec */
#define PAL_VERSION                   0x01

/* sub version that is vendor specific
  * This is current PAL release version 
  * Refer: http://wiki/Wifi_AMP_Release_Procedure#AMP_Releases
  */
#define PAL_SUB_VERSION               0x0004 

/* Not assigned yet but likey to be 5. 
  * Refer https://www.bluetooth.org/Technical/AssignedNumbers/hci.htm
  */
#define PAL_HCI_VERSION               0x05

/* Revision of the Current HCI in the Bluetooth device. 
  * Not sure what this should but set to our internal major version
  */
#define PAL_HCI_REVISION               AMP_HCI_SAP_API_VERSION_MAJOR

#define VALID_PID(pid) (pid != FSM_TERMINATE && context->instanceArray[pid].state != FSM_TERMINATE)


/* PUBLIC TYPES DEFINITIONS *************************************************/

typedef struct PalChannellist
{
    CsrUint8                          numChannels;
    CsrUint8                          number[15];
}PalChannellist;

typedef enum PalRole
{
    PalRole_PalRoleInitiator                 = 0x0000,
    PalRole_PalRoleResponder                 = 0x0001
} PalRole;

/* AMP Protocol Defines - section 5.1 802.11 AMP DIPD D07r15*/
typedef enum PAL_DataProtocolId
{
    PAL_DATA_PROTO_L2CAP_ID = 0x0001,
    PAL_DATA_PROTO_ACTIVITY_REPORT_ID = 0x0002,
    PAL_DATA_PROTO_SECURITY_FRAMES_ID = 0x0003,
    PAL_DATA_PROTO_LINK_SUPERVISION_REQUEST_ID = 0x0004,
    PAL_DATA_PROTO_LINK_SUPERVISION_RESPONSE_ID = 0x0005
}PAL_DataProtocolId;

typedef struct PAL_CoexCapabilities
{
    CsrBool handleActivityReports;
    CsrBool handleSchedulingInfo;
}PAL_CoexCapabilities;

/* Control structure storing MIB Data */

typedef struct PAL_MibData
{
    CsrUint16 dot11HCCWmin;

    CsrUint16 dot11RTSThreshold;
    CsrUint16 dot11LongRetryLimit;
    CsrUint16 dot11ShortRetryLimit;
    CsrUint16 dot11ShortSlotTimeOptionImplemented;
    CsrUint16 dot11ShortSlotTimeOptionEnabled;

    /* link quality MIB Attribs */
    unifi_LinkQuality qual;
#ifdef REMOVE_CODE
    CsrUint16 dot11MultiDomainCapabilityImplemented;
    CsrUint16 dot11MultiDomainCapabilityEnabled;
    CsrUint16 dot11CurrentRegDomain;
#endif
    char dot11CountryString[3]; /* 3 bytes for country string as per IEEE 2007 spec */

    unifi_80211dTrustLevel         trustLevel;
    CsrUint8 cipherSuite[4];
}PAL_MibData;

/**
 * @brief
 *   PAL Timer structure.
 *
 * @par Description
 *
 */
typedef struct PAL_Timer
{
    FsmTimerId     id;
    CsrUint32         value;
    CsrBool enabled; /* Timer is enabled if the value is TRUE. Defaults to TRUE*/
}PAL_Timer;

/* Control structure containing attributes common for all physical links
 * Access to these parameters needs to be thread-safe
 */
typedef struct physicalLinkCommonAttrib
{
    PAL_Timer     connectionAcceptTimer;
    PAL_Timer     logicalLinkAcceptTimer;
    unifi_MACAddress localMacAddress;
    CsrUint8 selectedChannel;
    PAL_MibData mibData;
    CsrBool guranteedLinkSupported;
    CsrBool securityEnabled;
    CsrUint16 numTotalDataBlocks; /* total number of data blocks in data manager. read during start up*/
}physicalLinkCommonAttrib;

/* control structure containing attributes for physical link shared by various modules
 * Access to these parameters needs to thread-safe
 */
typedef struct physicalLinkSharedAttrib
{
    CsrBool used;
    unifi_MACAddress remoteMacAddress;
    CsrUint8 physicalLinkHandle;
    CsrUint16 linkFsmPid;
    CsrUint16 hipFsmPid;
    CsrBool qosSupported;
}physicalLinkSharedAttrib;

/* PUBLIC CONSTANT DECLARATIONS *********************************************/

/* PUBLIC VARIABLE DECLARATIONS *********************************************/
/**
 * @brief const fsm definition
 *
 * @par Description
 *   Defines the ip connection manager state machine data
 */

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

/**
 * @brief
 *   create Link Manager FSM instance with the supplied physical link handle
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    handle   : physical link handle for the new instance
 *
 * @return
 *   returns instance ID if successfully create else return FSM_TERMINATE
 */
extern CsrUint16 pal_lm_create_instance(FsmContext *context, CsrUint8 handle);

/**
 * @brief
 *   create LM HIP FSM instance with the supplied physical link handle
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    handle   : physical link handle for the new instance
 *
 * @return
 *   returns instance ID if successfully create else return FSM_TERMINATE
 */
extern CsrUint16 pal_lm_create_hip_instance(FsmContext *context, CsrUint8 handle);

/**
 * @brief
 *   Delete LM FSM instances (both Link and HIP FSM) for the matching Link FSM PID.
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    pid   : Link FSM process ID
 *
 * @return
 *   void
 */
extern void pal_delete_lm_instance(FsmContext *context, CsrUint16 pid);

/**
 * @brief
 *   get status of LM instance
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *   returns TRUE if atleast one instance of LM exists else FALSE.
 */
extern CsrBool pal_lm_instance_exits(FsmContext *context);

/**
 * @brief
 *    get the Link FSM PID from the physical link handle supplied
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    handle   : physical link handle
 *
 * @return
 *   returns instance ID if successfully create else return FSM_TERMINATE
 */
extern CsrUint16 pal_get_link_fsm_pid_from_handle(FsmContext *context, CsrUint8 handle);

/**
 * @brief
 *    get the HIP FSM PID from the physical link handle supplied
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    handle   : physical link handle
 *
 * @return
 *   returns instance ID if successfully create else return FSM_TERMINATE
 */
extern CsrUint16 pal_get_hip_fsm_pid_from_handle(FsmContext *context, CsrUint8 handle);

/**
 * @brief
 *    get the HIP FSM PID from the remote MAC Address supplied
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    remoteMacAddress   : remote MAC Address
 *
 * @return
 *   returns instance ID if successfully create else return FSM_TERMINATE
 */
extern CsrUint16 pal_get_hip_fsm_pid_from_remote_mac_address(FsmContext *context, const unifi_MACAddress *remoteMacAddress);

/**
 * @brief
 *    get the selected channel number if any
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *   returns channel number if there is atleast one link started else returns PAL_INVALID_CHANNEL_NUMBER
 */
extern CsrUint8 pal_get_selected_channel_no(FsmContext *context);

/**
 * @brief
 *    Set the selected channel number if any
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    channelNo   :  channel number to set
 *
 * @return
 *   void
 */
extern void pal_set_selected_channel_no(FsmContext *context, CsrUint8 channelNo);

/**
 * @brief
 *    Get the local MAC Address
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *   MAC Address
 */
extern unifi_MACAddress *pal_get_local_mac_address(FsmContext *context);

/**
 * @brief
 *    Set the local MAC Address
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    macAddress   : local MAC Address
 *
 * @return
 *   void
 */
extern void pal_set_local_mac_address(FsmContext *context, const unifi_MACAddress *macAddress);

/**
 * @brief
 *    Get the remote MAC Address for the physical link handle
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    phyLinkHandle   : physical link handle
 *
 * @return
 *    remote MAC Address if successfull else return NULL
 */
extern unifi_MACAddress *pal_get_remote_mac_address(FsmContext *context, CsrUint8 phyLinkHandle);

/**
 * @brief
 *    Set the remote MAC Address for the physical link handle
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    phyLinkHandle   : physical link handle
 * @param[in]    macAddress   : remote MAC Address
 *
 * @return
 *    void
 */
extern void pal_set_remote_mac_address(FsmContext *context, CsrUint8 phyLinkHandle, const unifi_MACAddress *macAddress);

/**
 * @brief
 *    Get the Mib data
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    Pointer to the mib data structure
 */
extern PAL_MibData *pal_get_common_mib_data(FsmContext *context);

/**
 * @brief
 *    Set the Mib data
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    mibData   :  pointer to the Mib data structure to be saved
 *
 * @return
 *    void
 */
extern void pal_set_common_mib_data(FsmContext *context, PAL_MibData *mibData);

/**
 * @brief
 *    Get the connection accept timer details used for the physical link creation
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    pointer to the timer structure
 */
extern PAL_Timer *pal_get_connection_accept_timer(FsmContext *context);

/**
 * @brief
 *    Get the logical link accept timer details used for the logical link creation
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 *
 * @return
 *    pointer to the timer structure
 */
extern PAL_Timer *pal_get_logical_link_accept_timer(FsmContext *context);

/**
 * @brief
 *    Set the connection accept timer details used for the physical link creation
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    value   :  Timeout value
 *
 * @return
 *    void
 */
extern void pal_set_connection_accept_timer(FsmContext *context, CsrUint32 value);

/**
 * @brief
 *    Set the logical link accept timer details used for the logical link creation
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    value   :  Timeout value
 *
 * @return
 *    void
 */
extern void pal_set_logical_link_accept_timer(FsmContext *context, CsrUint32 value);

/**
 * @brief
 *    Get physical link handle from the remote MAC Address
 *
 * @par Description
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    remoteMacAddr   : remote MAC Address
 * @param[in]    phyLinkHandle   :  physical link handle
 *
 * @return
 *    unifi_Success if successfull else returns unifi_Error
 */
extern unifi_Status pal_get_phy_link_handle_from_remote_mac_address(FsmContext *context,
                                                                                const unifi_MACAddress *remoteMacAddr,
                                                                                CsrUint8 *phyLinkHandle);

/**
 * @brief
 *    Get the status of support of guaranteed links for a particular physical link.
 *
 * @par Description
 *     The status will depend on the Guranteed link support on both local and peer devices.
 *  The flag will be TRUE only if both peers support guaranteed links.
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    phyLinkHandle   :  physical link handle
 *
 * @return
 *    TRUE if both devices support guranteed links else returns FALSE
 */
extern CsrBool pal_get_link_qos_support(FsmContext *context, CsrUint8 phyLinkHandle);

/**
 * @brief
 *    Set the status of support of guaranteed links for a particular physical link.
 *
 * @par Description
 *     Link FSM will set this flag to appropriate value when it knows about the guranteed link support of the peer.
 *
 * @param[in]    context   :  FSM Context
 * @param[in]    phyLinkHandle   :  physical link handle
 * @param[in]    qosSupported   :  boolean denoting support for Guaranteed links
 *
 * @return
 *    void
 */
extern void pal_set_link_qos_support(FsmContext *context, CsrUint8 phyLinkHandle, CsrBool qosSupported);

/**
 * @brief
 *    Get the status of support of guaranteed links locally
 *
 * @par Description
 *    This field indicates the support for guaranteed links. This is applicable to all physical links.
 * A physical link will support a guaranteed link only if both peers supports guaranteed links. So this
 * field is only one of the inputs to determine the guaranteed link support for a physical link.
 *
 * @param[in]    context   :  FSM Context
 *
 * Note:
 *   This field can be configured during runtime or compile time.
 *
 * @return
 *    TRUE if guranteed links supported locally else FALSE.
 */
extern CsrBool pal_guranteed_link_supported(FsmContext *context);

/**
 * @brief
 *    Get the status of security support
 *
 * @par Description
 *    Security handshake is triggered only if this flag is enabled. This is a mandatory feature hence it
 * is enabled by default.
 *
 * @param[in]    context   :  FSM Context
 *
 * Note:
 *   This field can be configured during runtime or compile time.
 *
 * @return
 *    TRUE if seucrity handshake supported locally else FALSE.
 */
extern CsrBool pal_security_enabled(FsmContext *context);

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PAL_DATA_COMMON_H */
