/** @file hip_interface.c
 *
 * HIP Interface
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
 *  This file contains the routines related forming and sending HIP messages.
 *
 ****************************************************************************
 *
 * @section REVISION
  *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/hip_interface.c#3 $
 *
 ****************************************************************************/

/** @{
 * @ingroup coex_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "csr_cstl/csr_wifi_list.h"
#include "link_manager_fsm/pal_data_common.h"
#include "link_manager_fsm/hip_interface.h"
#include "regulatory_domain/regulatory_domain.h"
#include "device_manager_fsm/device_manager_fsm.h"
#include "sys_sap/sys_sap_from_sme_interface.h"
#include "smeio/smeio_trace_types.h"


/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */

/**
 * Generic probe delay time (in usecs) - medium is held idle for at least this
 * duration prior to transmitting a probe request frame.
 * This value should be 0 unless a VERY long (5000+) value is used. (to long to be practical)
 *
 */
#define PAL_HIP_TMPFIXED_PROBE_DELAY_USEC       0      /* microseconds */

/**
 * Minimum time to spend on a full ACTIVE scan (potentially many channels).
 * If no beacon frames are received within the period the scan terminates
 */
#define PAL_HIP_TMPFIXED_FULLSCAN_ACTIVE_MIN_TU      35     /* TimeUnits */

/**
 * Maximum time to spend on a full ACTIVE scan (potentially many channels).
 * If beacon were detected within the Minimum time (above) then the MLME will
 * spend no longer than this value before terminating the scan
 */
#define PAL_HIP_TMPFIXED_FULLSCAN_ACTIVE_MAX_TU      60     /* TimeUnits */


/* probe delay for AMP device. Reference ?? */
#define PAL_HIP_DEFAULT_PROBE_DELAY (100)

/* Beacon period for AMP device. Reference ?? */
#define PAL_HIP_DEFAULT_BEACON_PERIOD (100) /* 2 seconds */

/* DTIM Period for AMP device. Reference ?? */
#define PAL_HIP_DEFAULT_DTIM_PERIOD (1)

#define PAL_HIP_AUTHENTICATION_FAILURE_TIMEOUT_MS   (200)

#define PAL_HIP_ASSOCIATE_FAILURE_TIMEOUT_MS         (200)

#define PAL_HIP_ASSOC_LISTEN_INTERVAL (10) /* beacon periods */

/* Capabilities constants */
#define PAL_HIP_SIG_CAP_SHORT_PREAMBLE  (0x0020)
#define PAL_HIP_SIG_CAP_ESS             (0x0001)
#define PAL_HIP_SIG_CAP_SHORT_SLOT_TIME        (0x400)
#define PAL_HIP_SIG_CAP_PRIVACY        (0x0010)
#define PAL_HIP_IE_TIM_DATA_SIZE (4)
#define PAL_HIP_IE_AMP_DATA_SIZE (10)
#define PAL_HIP_IE_TIM_SIZE (PAL_HIP_IE_HEADER_SIZE + PAL_HIP_IE_TIM_DATA_SIZE)
#define PAL_HIP_IE_AMP_SIZE (PAL_HIP_IE_HEADER_SIZE + PAL_HIP_IE_AMP_DATA_SIZE)
#define PAL_HIP_IE_SUPPORTED_RATES_SIZE(num) ((2*PAL_HIP_IE_HEADER_SIZE) + (num))

#define PAL_HIP_IE_HEADER_LEN (2)
#define PAL_HIP_IE_EDCA_PARAM_SET_DATA_SIZE (18) /* as per section 7.3.2.29 IEEE 2007 Spec */
#define PAL_HIP_IE_EDCA_PARAM_SET_SIZE (PAL_HIP_IE_HEADER_SIZE + PAL_HIP_IE_EDCA_PARAM_SET_DATA_SIZE)
#define PAL_HIP_IE_QOS_CAPABILITY_DATA_SIZE (1) /* as per section 7.3.2.35 IEEE 2007 Spec */
#define PAL_HIP_IE_QOS_CAPABILITY_SIZE (PAL_HIP_IE_HEADER_SIZE + PAL_HIP_IE_QOS_CAPABILITY_DATA_SIZE)

/* as per table 7-37 of IEEE 2007 spec */
#define PAL_HIP_AC_BK_AIFSN (0x07)
#define PAL_HIP_AC_BE_AIFSN (0x03)
#define PAL_HIP_AC_VI_AIFSN (0x02)
#define PAL_HIP_AC_VO_AIFSN (0x02)

#define PAL_HIP_AC_BE_ACI (0x00)
#define PAL_HIP_AC_BK_ACI (0x01)
#define PAL_HIP_AC_VI_ACI (0x02)
#define PAL_HIP_AC_VO_ACI (0x03)

/* These values are retrieved from the IOP logs used by other participants
 * FIXME: Find an IEEE spec reference
*/
#define PAL_HIP_AC_BE_ECW_MIN_MAX (0xa4)
#define PAL_HIP_AC_BK_ECW_MIN_MAX (0xa4)
#define PAL_HIP_AC_VI_ECW_MIN_MAX (0x43)
#define PAL_HIP_AC_VO_ECW_MIN_MAX (0x32)

/* Value of TXOP limit is in units of 32MicroSecs. Values as per table 7-37 IEEE 2007 spec
 * We are clause 19 PHY
 */
#define PAL_HIP_AC_BE_TXOP_LIMIT (0x00)
#define PAL_HIP_AC_BK_TXOP_LIMIT (0x00)
#define PAL_HIP_AC_VI_TXOP_LIMIT (0x5E) /* 0x5E * 32 = 3.008 ms) */
#define PAL_HIP_AC_VO_TXOP_LIMIT (0x2F) /* 0x2F * 32 = 1.504 ms) */
/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *    function that can be assigned to freeFucnction pointer in control path.
 *
 * @par Description
 *    
 *
 * @param[in]    ptr   : pointer to be freed
 *
 * @return
 *   void
 */
static void pal_hip_free_mem(void *ptr)
{
    sme_trace_entry((TR_PAL_DAM,"pal_hip_free_mem: free pointer-%p", ptr));
    CsrPfree(ptr);
}


/**
 * @brief
 *   form and return the capability field
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 *
 * @return void
 */
static CapabilityInformation get_capability(FsmContext *context)
{
    CapabilityInformation capability=0;
    capability = PAL_HIP_SIG_CAP_SHORT_PREAMBLE;
    capability |= PAL_HIP_SIG_CAP_ESS;

    if (pal_guranteed_link_supported(context))
    {
       sme_trace_info((TR_PAL_LM_HIP_FSM, "QoS Enabled"));
       capability |= PAL_HIP_SIG_CAP_QOS;
    }
    else
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM, "QoS Disabled"));
    }
    capability |= PAL_HIP_SIG_CAP_SHORT_SLOT_TIME;/* enable it as default */
    capability |= PAL_HIP_SIG_CAP_PRIVACY;
    return capability;
}

/**
 * @brief
 *   form an essid
 *
 * @par Description
 *    Function to form an essid following the criterion specified in
 *    PAL spec section 3.3.1. (The SSID information element for AMP devices shall be of the form 'AMP-xxxx-
 *    xx-xx-xx-xx' (with no null termination and no quotes) where the "x" characters
 *    are replaced by the lowercase hexadecimal characters of the MAC
 *    address of the local 802.11 device. This is referred to here as the AMP SSID.
 *     For example, if the MAC address of a device is 00:01:02:0A:0B:0C, then the
 *    AMP SSID would be 'AMP-00-01-02-0a-0b-0c'.)
 *
 * @param[in]    mac_address   : local mac address to form as part of the ESSID
 * @param[out]    essid   : essid populated using the MAC Address
 *
 * @return void
 */
static void pal_hip_form_essid(const CsrUint8 *mac_address, unifi_SSID *essid)
{
    essid->length = CsrSprintf((char *)essid->ssid,"AMP-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x",
                                mac_address[0],mac_address[1],mac_address[2],mac_address[3],
                                mac_address[4],mac_address[5]); /*lint !e734    mac_address */

    sme_trace_info((TR_PAL_LM_LINK_FSM,"form_essid:  essid formed is %s",essid->ssid));

}

/**
 * @brief
 *   adds an IE in the TLV format to an IE vector
 *
 * @par Description
 *
 * @param[in]    info   : Pointer to position in the IE vector to write to.
 * @param[in]    ieId   : IE type to add
 * @param[in]    ie_data   : IE data to add
 * @param[in]    ieLen   : length of data to add
 *
 * Notes:
 *      Assumes there is enough space in info for ieLen+2 bytes.
 *
 * @return void
 */
static CsrUint16 pal_hip_add_info_element(CsrUint8 *info,
                                            CsrUint8 ieId,
                                            const CsrUint8 *ie_data,
                                            CsrUint8 ieLen)
{
    ie_write_CsrUint8(info, ieId);
    ie_write_CsrUint8(info, ieLen);

    ie_CsrUint8array(ie_data,info, ieLen);

    return (CsrUint16)(ieLen+PAL_HIP_IE_HEADER_SIZE);
} /* unifi_add_info_element() */

/**
 * @brief
 *   adds one entry of Access Class parameter record to EDCA Parameter set IE
 *
 * @par Description
 *
 * @param[in]    ie_buf   : Pointer to position in the IE vector to write to.
 * @param[in]    aifsn   :
 * @param[in]    aci   :
 * @param[in]    ecwminmax   :
 * @param[in]    txopLimit   :
 *
 * Notes:
 *      Assumes there is enough space in info for 4 bytes.
 *
 * @return void
 */
static int pal_hip_build_ac_param_record(CsrUint8 *ie_buf,
                                               CsrUint8 aifsn,
                                               CsrUint8 aci,
                                               CsrUint8 ecwminmax,
                                               CsrUint16 txopLimit)
{
    CsrUint8 aifsn_aci = (CsrUint8)(aifsn|(aci<<5));

    *ie_buf=0; /* Set it to zero so that acm and reserved field are zero */
    ie_write_CsrUint8(ie_buf, aifsn_aci); /* aci/aifsn  field */
    ie_write_CsrUint8(ie_buf, ecwminmax); /* set ECWmin/max to zero. Not sure what this should be ?? FIXME. Nothing specified in AMP spec */
    ie_le_write_CsrUint16(ie_buf, txopLimit); /* TXOP Limit */
    return 4;
}

/**
 * @brief
 *   adds EDCA Parameter set IE to an IE vector
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 * @param[out]    ie_buf   : Pointer to position in the IE vector to write to.
 *
 * Notes:
 *      Assumes there is enough space in info for PAL_HIP_IE_EDCA_PARAM_SET_SIZE bytes.
 *
 * @return void
 */
static int pal_hip_build_edca_parameter_set_element_ie(FsmContext *context,
                                                               CsrUint8 *ie_buf)
{
    int ieLen=0;
    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_build_edca_parameter_set_element_ie"));

    ie_write_CsrUint8(ie_buf, IE_ID_EDCA_PARAMS);
    ie_write_CsrUint8(ie_buf, PAL_HIP_IE_EDCA_PARAM_SET_DATA_SIZE);

    /* Encode as per section 7.3.2.29 of IEEE 2007 spec */
    ie_write_CsrUint8(ie_buf, 0x00); /* Qos Info */
    ie_write_CsrUint8(ie_buf, 0x00);  /* Reserved */

    /* AC_XX parameter records. */
    ieLen = pal_hip_build_ac_param_record(ie_buf,PAL_HIP_AC_BE_AIFSN, PAL_HIP_AC_BE_ACI, PAL_HIP_AC_BE_ECW_MIN_MAX, PAL_HIP_AC_BE_TXOP_LIMIT);
    ieLen += pal_hip_build_ac_param_record(ie_buf+ieLen,PAL_HIP_AC_BK_AIFSN, PAL_HIP_AC_BK_ACI, PAL_HIP_AC_BK_ECW_MIN_MAX, PAL_HIP_AC_BK_TXOP_LIMIT);
    ieLen += pal_hip_build_ac_param_record(ie_buf+ieLen,PAL_HIP_AC_VI_AIFSN, PAL_HIP_AC_VI_ACI, PAL_HIP_AC_VI_ECW_MIN_MAX,  PAL_HIP_AC_VI_TXOP_LIMIT);
    ieLen += pal_hip_build_ac_param_record(ie_buf+ieLen,PAL_HIP_AC_VO_AIFSN, PAL_HIP_AC_VO_ACI, PAL_HIP_AC_VO_ECW_MIN_MAX, PAL_HIP_AC_VO_TXOP_LIMIT);

    return (PAL_HIP_IE_EDCA_PARAM_SET_SIZE);
}

/**
 * @brief
 *   builds IE for MLME-START-REQ message
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 * @param[in]    capability   : capability of the device
 * @param[out]    dataRef   : pointer data reference to the encoded data returned
 *
 *
 * @return void
 */
static void pal_hip_build_ie_for_mlme_start(FsmContext *context,
                                                CapabilityInformation capability,
                                                DataReference *dataRef)
{
/*    PldHdl hdl=0; */
    CsrUint16 ieLen = 0;
    unifi_SSID essid;
    static const unsigned char supp_rates[] = {
        0x82, 0x84, 0x8B, 0x96, 12, 24, 48, 72, 18, 36, 96, 108
    };
    const CsrUint8 num_supp_rates = sizeof(supp_rates);
    CsrUint8 *ie_buf;
    unifi_MACAddress *macAddr;

    macAddr = pal_get_local_mac_address(context);
    /*
     * Build the InformationElements
     */
    pal_hip_form_essid(macAddr->data,&essid);

    sme_trace_info((TR_PAL_LM_HIP_FSM, "start:cap=0x%X,chan=%d,beacon=%d\n",
                capability, pal_get_selected_channel_no(context), PAL_HIP_DEFAULT_BEACON_PERIOD));

    ieLen = essid.length +
             PAL_HIP_IE_HEADER_SIZE +
             PAL_HIP_IE_SUPPORTED_RATES_SIZE(num_supp_rates) +
             PAL_HIP_IE_TIM_SIZE; /* 4 for TIM*/

    if (pal_security_enabled(context))
    {
        ieLen += PAL_HIP_IE_RSN_SIZE;
    }
    if (pal_guranteed_link_supported(context))
    {
        ieLen += PAL_HIP_IE_EDCA_PARAM_SET_SIZE;
    }

    sme_trace_info((TR_PAL_LM_HIP_FSM, "start:ssid=\"%s\",len=%d, total ie len - %d\n",essid.ssid,essid.length,ieLen));

    if (ieLen)
    {
        ie_buf = CsrPmalloc(ieLen);

        /* If SSID is given, add it top the IE vector . This is mandatory*/
        if (essid.length) {
            ieLen = pal_hip_add_info_element(ie_buf, IE_ID_SSID, essid.ssid, essid.length);
        }

        ieLen += pal_hip_add_info_element(ie_buf + ieLen, IE_ID_SUPPORTED_RATES, supp_rates, 8);

        /* needed for AMP */
        /* For AP mode*/
        /* If an ESS, add a TIM elememt with DTIM Count = 0 to set the DTIM Period. */
        if (capability & PAL_HIP_SIG_CAP_ESS)
        {
            unsigned char buf[8];
            CsrUint8 *info = buf;
            ie_write_CsrUint8(info, 0x00); /* DTIM Count */
            ie_write_CsrUint8(info, PAL_HIP_DEFAULT_DTIM_PERIOD); /* DTIM Period */
            ie_write_CsrUint8(info, 0x00); /* Bitmap Control */
            ie_write_CsrUint8(info, 0x00); /* Partial Virtual Bitmap */
            ieLen += pal_hip_add_info_element(ie_buf + ieLen, IE_ID_TIM, buf, 4);
        }

        /* populate EDCA Param set element */
        if (pal_guranteed_link_supported(context))
        {
            ieLen += pal_hip_build_edca_parameter_set_element_ie(context, (CsrUint8 *)(ie_buf + ieLen)); /*lint !e734    ieLen */
        }

        if (pal_security_enabled(context))
        {
            /* populate RSN information element */
            ieLen += pal_hip_build_rsn_element_ie(context, ie_buf + ieLen);
        }
        /* This comes here so that IE ids are in numeric sequence */
        (void)pal_hip_add_info_element(ie_buf + ieLen, IE_ID_EXTENDED_SUPPORTED_RATES,
                               supp_rates + 8, num_supp_rates - 8);
        ieLen += (num_supp_rates - 8) + 2;

       *dataRef = regdom_create_ie_dr_src_buf(context, ie_buf, ieLen, FALSE);
        CsrPfree(ie_buf);
    }
}

/**
 * @brief
 *   builds IE for MLME-ASSOCIATE-RSP message
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 * @param[out]    dataRef   : pointer data reference to the encoded data returned
 *
 * @return void
 */
static void pal_hip_build_ie_for_associate_rsp(FsmContext *context,
                                               DataReference *dataRef)
{
    PldHdl hdl=0;
    CsrUint8 *ie_buf;

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_build_ie_for_associate_rsp"));

    dataRef->dataLength = PAL_HIP_IE_EDCA_PARAM_SET_SIZE;

    pld_create(getPldContext(context), dataRef->dataLength, (void **)&ie_buf, &hdl);
    dataRef->slotNumber = hdl;

    /* populate EDCA Param set element */
    (void)pal_hip_build_edca_parameter_set_element_ie(context, ie_buf);
}

/**
 * @brief
 *   builds IE for MLME-ASSOCIATE-REQ message
 *
 * @par Description
 *
 * @param[in]    context   : FSM Context
 * @param[out]    peerMacAddress   : remote MAC Address
 * @param[out]    dataRef   : pointer data reference to the encoded data returned
 *
 * @return void
 */
static void pal_hip_build_ie_for_associate_req(FsmContext *context,
                                               const unifi_MACAddress *peerMacAddress,
                                               DataReference *dataRef)
{
    PldHdl hdl=0;
    CsrUint8 *ie_buf;
    CsrUint8 qosInfo=0; /* FIXME: need to fill in the qos info appropriately. Refer section 7.3.1.17 of IEEE 2007 */
    unifi_SSID essid;
    CsrUint16 ieLen=0;
    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_build_ie_for_associate_rsp"));

    dataRef->dataLength=0;
    dataRef->slotNumber=0;

    pal_hip_form_essid(peerMacAddress->data,&essid);

    dataRef->dataLength = essid.length + PAL_HIP_IE_HEADER_SIZE;
    if (pal_guranteed_link_supported(context))
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM, "pal_hip_build_ie_for_associate_rsp: QOS Info added as QOS links are supported"));

        if (pal_security_enabled(context))
        {
            dataRef->dataLength += PAL_HIP_IE_QOS_CAPABILITY_SIZE+PAL_HIP_IE_RSN_SIZE;
        }
        else
        {
            dataRef->dataLength += PAL_HIP_IE_QOS_CAPABILITY_SIZE;
        }
    }
    else if (pal_security_enabled(context))
    {
        sme_trace_info((TR_PAL_LM_HIP_FSM, "pal_hip_build_ie_for_associate_rsp: QOS Info NOT added as QOS links are NOT supported"));
        dataRef->dataLength += PAL_HIP_IE_RSN_SIZE;
    }

    if (dataRef->dataLength)
    {
        pld_create(getPldContext(context), dataRef->dataLength, (void **)&ie_buf, &hdl);
        dataRef->slotNumber = hdl;

        ieLen=pal_hip_add_info_element(ie_buf, IE_ID_SSID, essid.ssid, essid.length);
        ie_buf += ieLen;

        if (pal_guranteed_link_supported(context))
        {
            ie_write_CsrUint8(ie_buf, IE_ID_QOS_CAPABILITY);
            ie_write_CsrUint8(ie_buf, PAL_HIP_IE_QOS_CAPABILITY_DATA_SIZE);

            /* populate EDCA Param set element */
            ie_write_CsrUint8(ie_buf, qosInfo); /* Qos Info */
        }

        if (pal_security_enabled(context))
        {
            /* fill in RSN IE now */
            (void)pal_hip_build_rsn_element_ie(context, ie_buf);
        }
    }
}

/**
 * @brief
 *   Function to find a specific IE from an IE vector
 *
 * @par Description
 *
 * @param[in]    id : IE ID to find
 * @param[in]    elements : pointer to the IE vector
 * @param[in]    length : length of IE vector
 * @param[out]    result : result pointer pointing to macting IE or NULL if no match found
 *
 * @return ie_success if IE found else ie_not_found
 */
static ie_result pal_hip_ie_find(ie_elementid id,
                                  CsrUint8* elements,
                                  CsrUint32 length,
                                  CsrUint8** result)
{
    CsrUint8* curr = elements;
    CsrUint8* end = elements + length;

    require(TR_PAL_LM_HIP_FSM, elements != NULL);

    *result = NULL;

    while (curr < end)
    {
        if (!ie_valid(curr, end))
        {
            sme_trace_error((TR_PAL_LM_HIP_FSM, "pal_hip_ie_find() :: ie_invalid :: id = %d, length = %d",
                                                ie_id(elements), ie_len(elements)));
            return ie_invalid;
        }
        if (ie_id(curr) == id)
        {
            *result = curr;
            return ie_success;
        }
        curr = ie_next(curr);
    }

    return ie_not_found;
}


/* PUBLIC FUNCTIONS *********************************************************/
CsrUint16 pal_hip_build_rsn_element_ie(FsmContext *context, CsrUint8 *ie_buf)
{
    /* Encode as per section 7.3.2.25 of IEEE 2007 spec */
    CsrUint8 rsnie[] = {0x01, 0x00, /* Version 1 */
                     0x00, 0x0F, 0xAC, 0x04, /*  CCMP as group cipher suite */
                     0x01, 0x00, /* pairwise cipher suite count */
                     0x00, 0x0F, 0xAC, 0x04, /* CCMP as pairwise cipher suite */
                     0x01, 0x00, /* authentication count */
                     0x00, 0x0F, 0xAC, 0x02, /* PSK */
                     0x08, 0x00 /* pre-auth supported (FIXME: Not clear if its required) and Pairwise bit NOT set (as per PAL spec clause 3.4.2)*/
                    };

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_build_rsn_element_ie"));

    return pal_hip_add_info_element(ie_buf, IE_ID_RSN, rsnie, PAL_HIP_IE_RSN_DATA_SIZE);
}

void pal_hip_send_mlme_start_req(FsmContext *context, Interface iftype, ChannelNumber channelNo)
{
    DataReference dataRef1={0,0};
    CapabilityInformation capability = get_capability(context);

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_send_mlme_start_req: on channel %d",channelNo));
    pal_hip_build_ie_for_mlme_start(context, capability, &dataRef1);

    send_mlme_start_req(context, dataRef1,
                       iftype,
                       PAL_HIP_DEFAULT_BEACON_PERIOD,
                       channelNo,
                       PAL_HIP_DEFAULT_PROBE_DELAY,
                       capability,
                       TRUE);
}

void pal_hip_send_mlme_authenticate_req(FsmContext *context, const unifi_MACAddress *peerStaAddress)
{
    DataReference NullDataRef = {0, 0};

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_send_mlme_authenticate_req"));
    send_mlme_authenticate_req(context,
                               NullDataRef,
                               *peerStaAddress,
                               AuthenticationType_OpenSystem,
                               PAL_HIP_AUTHENTICATION_FAILURE_TIMEOUT_MS);
}

void pal_hip_send_mlme_deauthenticate_req(FsmContext *context, const unifi_MACAddress *peerStaAddress)
{
    DataReference NullDataRef = {0, 0};

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_send_mlme_deauthenticate_req"));
    send_mlme_deauthenticate_req(context, NullDataRef,
                                *peerStaAddress,
                                ReasonCode_UnspecifiedReason );
}

void pal_hip_send_mlme_associate_req(FsmContext *context, const unifi_MACAddress *peerStaAddress)
{
    CapabilityInformation capability = get_capability(context);
    DataReference dataRef;

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_send_mlme_associate_req"));
    pal_hip_build_ie_for_associate_req(context, peerStaAddress, &dataRef);

    send_mlme_associate_req(context, dataRef,
                            *peerStaAddress,
                            PAL_HIP_ASSOCIATE_FAILURE_TIMEOUT_MS,
                            capability,
                            PAL_HIP_ASSOC_LISTEN_INTERVAL);
}

void pal_hip_send_mlme_authenticate_rsp(FsmContext *context,
                                        ResultCode resultCode,
                                        const unifi_MACAddress *peerStaAddress)
{
    DataReference NullDataRef = {0, 0};

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_send_mlme_authenticate_rsp"));
    send_mlme_authenticate_rsp(context,
                               NullDataRef,
                               *peerStaAddress,
                               resultCode);
}

void pal_hip_send_mlme_associate_rsp(FsmContext *context,
                                            ResultCode resultCode,
                                            const unifi_MACAddress *peerStaAddress,
                                            AssociationId associationId)
{
    CapabilityInformation capability = get_capability(context);
    DataReference dataRef={0, 0};

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_send_mlme_associate_rsp"));
    if (pal_guranteed_link_supported(context))
    {
        pal_hip_build_ie_for_associate_rsp(context,&dataRef);
    }

    send_mlme_associate_rsp(context,
                            dataRef,
                            *peerStaAddress,
                            resultCode,
                            capability,
                            associationId,
                            0,0); /* FIXME: what are the real values for these parameters. */
}

void pal_hip_send_ma_unitdata_req(FsmContext *context,
                                         const unifi_MACAddress *peerStaAddress,
                                         CsrBool qosSupported,
                                         const CsrUint8 *data,
                                         CsrUint16 dataLength,
                                         CsrUint16 protocolId,
                                         CsrUint32 hostTag)
{
    unifi_MACAddress *localMacAddress = pal_get_local_mac_address(context);
    pal_hip_llc_snap_hdr_t snap;
    CsrUint8 bt_oui[] = {0x00, 0x19, 0x58};
    CsrUint8 *buf; /*lint !e429*/
    Priority userPriority = qosSupported?Priority_QoSUP4:Priority_Contention;
    ServiceClass serviceClass = qosSupported?ServiceClass_QoSAck:ServiceClass_ReorderableMulticast;
    CsrUint16 frameLength = dataLength+PAL_HIP_SNAP_HEADER_SIZE+(2*6);

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_send_ma_unitdata_req: data length-%d,protId-%d,hostTag-0x%x",dataLength,protocolId,hostTag));

    buf = CsrPmalloc(frameLength);

    CsrMemCpy(buf, peerStaAddress->data, 6);
    CsrMemCpy(buf+6, localMacAddress->data, 6);

    snap.dsap = snap.ssap = 0xAA;
    snap.ctrl = 0x03;
    CsrMemCpy(snap.oui, bt_oui, 3);
    snap.protocol = protocolId;
    pal_hip_encode_snap(buf+12,&snap);

    if (data)
    {
        CsrMemCpy(buf+12+PAL_HIP_SNAP_HEADER_SIZE,data,dataLength);
    }
/* MlmeEapolReq cannot be used for AMP as it uses 3 address frames instead of 4 address frames. 
  * 4 Address frame is mandatory for AMP
  */
#ifdef PAL_USE_EAPOL_REQ
    /* Use EapolReq for security frames to bypass port filtering. */
    if (PAL_DATA_PROTO_SECURITY_FRAMES_ID == protocolId)
    {
        call_unifi_sys_eapol_req(context,
                                 PAL_SYS_APP_HANDLE(context),
                                 pal_dm_get_subscription_handle(context,protocolId),
                                 frameLength,
                                 buf,
                                 (unifi_FrameFreeFunction)&pal_hip_free_mem); /*lint !e546*/
    }
    else
#endif
    {
        call_unifi_sys_ma_unitdata_req(context,
                                       PAL_SYS_APP_HANDLE(context),
                                       pal_dm_get_subscription_handle(context,protocolId),
                                       frameLength,
                                       buf,
                                       (unifi_FrameFreeFunction)&pal_hip_free_mem, 
                                       userPriority,
                                       serviceClass,
                                       hostTag);/*lint !e546*/
    }
} /*lint !e429*/

void pal_hip_send_mlme_setkeys_req(FsmContext *context,
                                          KeyType keyType,
                                          const CsrUint8 *key,
                                          CsrUint32 keyLength,
                                          CsrUint16 keyId,
                                          CsrUint16 *recvSeqCount,
                                          unifi_MACAddress *remoteMacAddress,
                                          CsrBool authenticator_or_supplicant)
{
    DataReference dataRef;
    CsrUint8 *buf;
    CipherSuiteSelector cipherSuiteSelector;
    PAL_MibData *mibData = pal_get_common_mib_data(context);

    pld_create(getPldContext(context),
               (CsrUint16)keyLength,
               (void **)&buf, (PldHdl *)&dataRef.slotNumber);

    CsrMemCpy(buf,key,keyLength);
    dataRef.dataLength = (CsrUint16)keyLength;

    /* Cipher suite selector is same as in RSN IE (clause 10.3.17.1.2 IEEE 2007)*/
    cipherSuiteSelector = mibData->cipherSuite[0] | mibData->cipherSuite[1]<<8 | mibData->cipherSuite[2]<<16 | mibData->cipherSuite[3]<<24;

    send_mlme_setkeys_req(context,
                          dataRef,
                          (keyLength*8),  /* length is in bits */
                          keyId,/*FIXME: p3_Key_Id . what is supposed to have. */
                          keyType,
                          *remoteMacAddress,
                          recvSeqCount,
                          authenticator_or_supplicant,/* (clause 10.3.17.1.2 IEEE 2007)
                                                       * "Whether the key is configured by the
                                                       * Authenticator or Supplicant; true indicates
                                                       * Authenticator or Initiator."
                                                       */
                          cipherSuiteSelector);
}

CsrUint8 * pal_hip_ie_parameter_check(FsmContext *context,
                                        const DataReference *ieDataRef,
                                        ie_elementid id)
{
    unifi_DataBlock ieBuf;
    CsrUint8 *result=NULL;

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_ie_parameter_check: IEId-%d,len-%d,slot-%d",id,ieDataRef->dataLength,ieDataRef->slotNumber));
    if (ieDataRef->dataLength)
    {
        pld_access(getPldContext(context), (PldHdl)ieDataRef->slotNumber,(void **) &ieBuf.data, &ieBuf.length);
        (void)pal_hip_ie_find(id, ieBuf.data, ieBuf.length, &result);
    }
    return result;
}

CsrUint16 pal_hip_get_cwmin_for_user_priority(CsrUint16 dot11HCCWmin, Priority up)
{
    CsrUint16 CWMin=0;

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_get_cwmin_for_user_priority: dot11HCCWmin-%d,userprio-%d",dot11HCCWmin,up));
    /* Calculate CWMin for the correspondin Accesss class according to the table 7-37 in IEEE 2007 spec */
    if (Priority_QoSUP7 == up)
    {
        CWMin = ((dot11HCCWmin+1)/4) - 1;
    }
    else if (Priority_QoSUP4 == up)
    {
        CWMin = ((dot11HCCWmin+1)/2) - 1;
    }
    else if (Priority_QoSUP0 == up)
    {
        CWMin = dot11HCCWmin;
    }
    return CWMin;
}

CsrUint32 pal_hip_get_min_latency(CsrUint16 dot11HCCWmin)
{
    /* From PAL Spec D07r15
     * "The Min_Latency parameter value is the practical lower bound on the service
     * latency that can be provided by the 802.11 AMP.  The lower bound of the service
     * latency is the time the MAC takes to start transmitting an initial frame with no
     * contention window back off.  This is equal to DIFS + CWmin as given in IEEE 2007 clause 9.2.10."
     * The value of Minimum Latency is expressed in Micro seconds.
     */
    CsrUint32 difs = PAL_HIP_DSSS_SIFS_TIME + 2*PAL_HIP_DSSS_SHORT_SLOT_TIME_PERIOD; /* From clause 9.2.10 of IEEE 2007*/

    sme_trace_entry((TR_PAL_LM_HIP_FSM, "pal_hip_get_min_latency: dot11HCCWmin-%d, difs-%d",dot11HCCWmin,difs));
    return difs+pal_hip_get_cwmin_for_user_priority(dot11HCCWmin, Priority_QoSUP4);

}

CsrUint32 pal_hip_get_min_flush_timeout(CsrUint16 dot11HCCWmin)
{
    CsrUint32 minFlushTimeout;
    /* From PAL Spec D07r15
     * "The Min_Latency parameter value is the practical lower bound on the service
     * latency that can be provided by the 802.11 AMP.  The lower bound of the service
     * latency is the time the MAC takes to start transmitting an initial frame with no
     * contention window back off.  This is equal to DIFS + CWmin as given in IEEE 2007 clause 9.2.10."
     * The value of Minimum Latency is expressed in Micro seconds.
     */
    CsrUint32 difs = PAL_HIP_DSSS_SIFS_TIME + 2*PAL_HIP_DSSS_SHORT_SLOT_TIME_PERIOD; /* From clause 9.2.10 of IEEE 2007*/

    /* min flushtimeout (in microseconds) = number of min retries * min time for one retry */
    minFlushTimeout = (PAL_MIN_AVERAGE_RANDOM_BACKOFF_PER_RETRY+difs+pal_hip_get_cwmin_for_user_priority(dot11HCCWmin, Priority_QoSUP4))*PAL_MIN_DOT11_RETRY_LIMIT;

    sme_trace_info((TR_PAL_LM_HIP_FSM, "pal_hip_get_min_flush_timeout: dot11HCCWmin-%d, difs-%d,minflushtimeout-%d",dot11HCCWmin,difs,minFlushTimeout));
    return minFlushTimeout;
}

CsrUint32 pal_hip_get_max_flush_timeout(CsrUint16 dot11HCCWmin)
{
    CsrUint32 maxFlushTimeout;
    /* From PAL Spec D07r15
     * "The Min_Latency parameter value is the practical lower bound on the service
     * latency that can be provided by the 802.11 AMP.  The lower bound of the service
     * latency is the time the MAC takes to start transmitting an initial frame with no
     * contention window back off.  This is equal to DIFS + CWmin as given in IEEE 2007 clause 9.2.10."
     * The value of Minimum Latency is expressed in Micro seconds.
     */
    CsrUint32 difs = PAL_HIP_DSSS_SIFS_TIME + 2*PAL_HIP_DSSS_SHORT_SLOT_TIME_PERIOD; /* From clause 9.2.10 of IEEE 2007*/

    /* max flushtimeout (in microseconds) = number of retries * min time for one retry */
    maxFlushTimeout = (PAL_MIN_AVERAGE_RANDOM_BACKOFF_PER_RETRY+difs+pal_hip_get_cwmin_for_user_priority(dot11HCCWmin, Priority_QoSUP4)) * PAL_MAX_DOT11_RETRY_LIMIT;

    sme_trace_info((TR_PAL_LM_HIP_FSM, "pal_hip_get_max_flush_timeout: dot11HCCWmin-%d, difs-%d,maxflushtimeout-%d",dot11HCCWmin,difs,maxFlushTimeout));
    return maxFlushTimeout;
}

void pal_hip_encode_snap(CsrUint8 *buf, pal_hip_llc_snap_hdr_t *snap)
{
    (void)event_pack_CsrUint8(&buf,snap->dsap);
    (void)event_pack_CsrUint8(&buf,snap->ssap);
    (void)event_pack_CsrUint8(&buf,snap->ctrl);
    (void)event_pack_buffer(&buf, snap->oui, 3);

    /* SNAP Protocol is encoded in big endian (network byte order) - reference ?? */
    buf[0] =  (snap->protocol >>  8) & 0xFF;
    buf[1] = snap->protocol        & 0xFF;
}

void pal_hip_decode_snap(CsrUint8 *buf, pal_hip_llc_snap_hdr_t *snap)
{
    snap->dsap = event_unpack_CsrUint8(&buf);
    snap->ssap = event_unpack_CsrUint8(&buf);
    snap->ctrl = event_unpack_CsrUint8(&buf);
    event_unpack_buffer(&buf, snap->oui, 3);

    /* SNAP Protocol is decoded in big endian (network byte order) - reference ?? */
    snap->protocol = (buf[0] << 8) | buf[1];
}

/** @}
 */
