/** @file amp_assoc_endec.c
 *
 * PAL AMP Assoc
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
 *  This file contains the routines related amp assoc encoding and decoding.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/pal/link_manager_fsm/amp_assoc_endec.c#1 $
 *
 ****************************************************************************/

/** @{
 * @ingroup link_manager
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "event_pack_unpack/event_pack_unpack.h"
#include "link_manager_fsm/pal_data_common.h"
#include "regulatory_domain/regulatory_domain.h"
#include "link_manager_fsm/amp_assoc_endec.h"
#include "scan_manager_fsm/scan_manager_fsm.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *   Simple accessor for this Process' Custom data
 *
 * @par Description
 *   see brief
 */

/* Macros defined to define the protocol for AMP Assoc
 * Values are as per PAL spec D07r15 section 2.10.1
 */
#define AMP_ASSOC_MAC_ADDRESS_TYPE (0x01)
#define AMP_ASSOC_PREFERRED_CHANNEL_LIST_TYPE (0x02)
#define AMP_ASSOC_CONNECTED_CHANNEL_TYPE (0x03)
#define AMP_ASSOC_PAL_CAPABILITY_FIELD_TYPE (0x04)
#define AMP_ASSOC_PAL_VERSION_TYPE (0x05)

#define AMP_ASSOC_MIN_NUM_CHANNELS (1)
#define AMP_ASSOC_BSSID_LEN (6)
#define AMP_ASSOC_ESSID_MAX_LEN (32)
#define AMP_ASSOC_MAC_ADDRESS_LEN (6)
#define AMP_ASSOC_CAPABILITIES_LEN (4)
#define AMP_ASSOC_PAL_VERSION_LEN (5)
#define AMP_ASSOC_CAP_ACTIVITY_REPORT_HANDLING_SUPPORTED (0x00000001)
#define AMP_ASSOC_CAP_SCHEDULING_INFO_HANDLING_SUPPORTED (0x00000002)
#define AMP_ASSOC_REGULATORY_EXTENSION_ID (201)

/* regulatory stuff for channel list */
#define AMP_ASSOC_COUNTRY_STRING_US "US"
#define AMP_ASSOC_COUNTRY_STRING_UK "GB"
#define AMP_ASSOC_COUNTRY_STRING_NON_COUNTRY_ENTITY "XXX"
#define AMP_ASSOC_CHANNEL_LIST_MIN_LENGTH (6) /* 3 byte Country and atleast one regulatory triplet of 3 bytes as per IEEE 2007 spec*/

#define AMP_ASSOC_SUPPORTED_REG_CLASS_FOR_US (12) /* Regualatory class for USA for 2.4GHz ISM band - PAL spec 3.2.3 */
#define AMP_ASSOC_SUPPORTED_REG_CLASS_FOR_UK () /* Regualatory class for USA for 2.4GHz ISM band - PAL spec 3.2.3 */

#define AMP_ASSOC_SUPPORT_CHANNEL_ELEMENT_ID (36) /* as per table 7-26 of 802.11 2007 spec*/
/*****************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief
 *   Decode PAL Capability field
 *
 * @par Description
 *
 * @param[in]    buffer   : double pointer where the parameter with encoded IE.
 * @param[in]    buflen   : length of the IE
 * @param[out]    capability   : decoded PAL capabilities
 *
 * @return
 *   void
 */
static CsrBool decode_amp_assoc_pal_capability_field(CsrUint8 **buffer, CsrUint16 buflen, PAL_CoexCapabilities *capability)
{
    CsrUint32 cap;
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"decode_amp_assoc_pal_capability_field "));

    if (AMP_ASSOC_CAPABILITIES_LEN == buflen)
    {
        cap = event_unpack_CsrUint32(buffer);
        capability->handleActivityReports = (CsrBool)(cap&AMP_ASSOC_CAP_ACTIVITY_REPORT_HANDLING_SUPPORTED);
        capability->handleSchedulingInfo  = (CsrBool)(cap&AMP_ASSOC_CAP_SCHEDULING_INFO_HANDLING_SUPPORTED);
    }
    else
    {
        sme_trace_warn((TR_PAL_LM_LINK_FSM,"decode_amp_assoc_pal_capability_field: wrong length"));
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief
 *   Decode  MAC address
 *
 * @par Description
 *
 * @param[in]    buffer   : double pointer where the parameter with encoded IE.
 * @param[in]    buflen   : length of the IE
 * @param[out]    addr   : decoded MAC Address
 *
 * @return
 *   void
 */
static CsrBool decode_amp_assoc_mac_address(CsrUint8 **buffer, CsrUint16 buflen, CsrUint8 *addr)
{
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"decode_amp_assoc_mac_address "));

    if (AMP_ASSOC_MAC_ADDRESS_LEN == buflen)
    {
        event_unpack_buffer(buffer, addr, AMP_ASSOC_MAC_ADDRESS_LEN);
    }
    else
    {
        sme_trace_warn((TR_PAL_LM_LINK_FSM,"decode_amp_assoc_mac_address: wrong length"));
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief
 *   Decode channel list (preferred or connected)
 *
 * @par Description
 *     decode channel list as per section 7.3.2.19 of IEEE 802.11-2007 referred in PAL spec D02r01 section 7.6
 *
 * @param[in]    context   : FSM Context
 * @param[in]    buffer   : double pointer where the parameter with encoded IE.
 * @param[in]    buflen   : length of the IE
 * @param[out]    channelList   : decoded channel list (preferred or connected)
 *
 * @return
 *   void
 */
static CsrBool decode_amp_assoc_channel_list(FsmContext *context,
                                                    CsrUint8 **buffer, 
                                                    CsrUint16 buflen, 
                                                    USED_CHANNELS_LIST * channelList)
{
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"decode_amp_assoc_channel_list len - %d",buflen));

    /* return false if the length parameters dont match OR assoc len
     * is not multiple of 3 (country string & regulatory/channel triplets)
     */
    if (buflen%3 && buflen>=AMP_ASSOC_CHANNEL_LIST_MIN_LENGTH) /* */
    {
        sme_trace_warn((TR_PAL_LM_LINK_FSM,"Invalid Assoc: Length (%d) inside assoc data is even",buflen));
        return FALSE;
    }
    reg_domain_amp_decode_country_ie(context, get_regulatory_data(context), channelList, *buffer, buflen);
    *buffer = *buffer+buflen;

    return TRUE;
}

/**
 * @brief
 *   encode AMP Assoc  channel list - preferred or connected
 *
 * @par Description
 *
 * @param[out]    buffer   : double pointer where the parameter will be encoded to.
 * @param[in]    countryInfoStruct   : channel list 
 * @param[in]    type   : type of channel list IE (preferred in this case).
 *
 * Notes:
 *      Assumes there is enough space in buffer to encode the IE (size is 3+Length for channel List entries where
 *      each entry is 3 bytes)).
 *
 * @return
 *   void
 */
static void encode_amp_assoc_channel_list(FsmContext *context,
                                                 CsrUint8 **buffer,
                                                 USED_CHANNELS_LIST *channelList,
                                                 CsrUint8 type)
{
    CsrUint8 *ptrToLen;
    CsrUint16 encodedLength;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"encode_amp_assoc_channel_list"));
    (void)event_pack_CsrUint8(buffer, type);
    /* leave length field for now */
    ptrToLen = *buffer;
    (*buffer) += 2; /* skip the buffer for length field. Length will be filled in the end */

    reg_domain_amp_encode_country_ie(context, get_regulatory_data(context), *buffer, &encodedLength, channelList);
    (*buffer) += encodedLength;
    (void)event_pack_CsrUint16(&ptrToLen, encodedLength); /* 4*5 entries*/
}

/**
 * @brief
 *   encode AMP Assoc Local MAC Address
 *
 * @par Description
 *
 * @param[out]    buffer   : double pointer where the parameter will be encoded to.
 * @param[in]    addr   : local mac address
 *
 * Notes:
 *      Assumes there is enough space in buffer to encode the IE (size is 9 bytes)
 *
 * @return
 *   void
 */
static void encode_amp_assoc_mac_address(CsrUint8 **buffer,CsrUint8 *addr)
{
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"encode_amp_assoc_mac_address"));
    (void)event_pack_CsrUint8(buffer, AMP_ASSOC_MAC_ADDRESS_TYPE);
    (void)event_pack_CsrUint16(buffer, AMP_ASSOC_MAC_ADDRESS_LEN);
    (void)event_pack_buffer(buffer, addr,AMP_ASSOC_MAC_ADDRESS_LEN);
}

/**
 * @brief
 *   encode AMP Assoc Capabilities
 *
 * @par Description
 *
 * @param[out]    buffer   : double pointer where the parameter will be encoded to.
 * @param[in]    capabilities   : PAL Capabilities
 *
 * Notes:
 *      Assumes there is enough space in buffer to encode the IE (size is 7 bytes)
 *
 * @return
 *   void
 */
static void encode_amp_assoc_capabilities(CsrUint8 **buffer, PAL_CoexCapabilities *capabilities)
{
    CsrUint32 cap;
    cap =  (TRUE==capabilities->handleActivityReports)?AMP_ASSOC_CAP_ACTIVITY_REPORT_HANDLING_SUPPORTED:0;
    cap |=  (TRUE==capabilities->handleSchedulingInfo)?AMP_ASSOC_CAP_SCHEDULING_INFO_HANDLING_SUPPORTED:0;
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"encode_amp_assoc_capabilities"));
    (void)event_pack_CsrUint8(buffer, AMP_ASSOC_PAL_CAPABILITY_FIELD_TYPE);
    (void)event_pack_CsrUint16(buffer, AMP_ASSOC_CAPABILITIES_LEN);
    (void)event_pack_CsrUint32(buffer, cap);
}

/**
 * @brief
 *   encode AMP Assoc PAL Version
 *
 * @par Description
 *
 * @param[out]    buffer   : double pointer where the parameter will be encoded to.
 *
 * Notes:
 *      Assumes there is enough space in buffer to encode the IE (size is 8 bytes)
 *
 * @return
 *   void
 */
static void encode_amp_assoc_pal_version(CsrUint8 **buffer)
{
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"encode_amp_assoc_pal_version"));
    (void)event_pack_CsrUint8(buffer, AMP_ASSOC_PAL_VERSION_TYPE);
    (void)event_pack_CsrUint16(buffer, AMP_ASSOC_PAL_VERSION_LEN);
    (void)event_pack_CsrUint8(buffer, PAL_VERSION); 
    (void)event_pack_CsrUint16(buffer, PAL_BT_SIG_COMPANY_IDENTIFIER); 
    (void)event_pack_CsrUint16(buffer, PAL_SUB_VERSION); /* PAL Sub-Version */
}

/* PUBLIC FUNCTIONS *********************************************************/

CsrBool pal_decode_amp_assoc(FsmContext *context, const CsrUint8 *data,CsrUint16 len, AMP_Assoc *assoc)
{
    CsrBool status=TRUE;
    CsrUint8 *buffer=(CsrUint8 *)data;
    CsrUint8 type;
    CsrUint16 ieLength;
    /* checks for mandatory fields */
    CsrBool macAddressPresent=FALSE;
    CsrBool preferredChannelListPresent=FALSE;

    sme_trace_entry((TR_PAL_LM_LINK_FSM,"decode_amp_assoc"));

    /* Very least a MAC address is expected */
    if (len<AMP_ASSOC_MAC_ADDRESS_LEN || !data || !assoc)
    {
        sme_trace_warn((TR_PAL_LM_LINK_FSM,"decode_amp_assoc: rejected len-%d",len));
        return FALSE;
    }

    do
    {
        type = event_unpack_CsrUint8(&buffer);
        ieLength = event_unpack_CsrUint16(&buffer);
        sme_trace_info((TR_PAL_LM_LINK_FSM,"decode_amp_assoc: ieLen-%d,type -%d",ieLength,type));
        switch (type)
        {
            case AMP_ASSOC_PREFERRED_CHANNEL_LIST_TYPE:
                 {
                    USED_CHANNELS_LIST preferredChannelList;

                    status = decode_amp_assoc_channel_list(context,&buffer, ieLength, &preferredChannelList);

                    /* FIXME: for the time being copy the channel list onto a seperate list with only channel number.
                     * eventually PAL should handle other info like tx power as well
                     */
                    assoc->channelList.numChannels=0;
                    if (preferredChannelList.listChangedFlag)
                    {
                        int i=0;
                        while (i<HIGHEST_80211_b_g_CHANNEL_NUM &&
                               channelScanMode_passive != preferredChannelList.Dott11_b_g_ChanList[i].chan_scan_mode)
                        {
                            sme_trace_info((TR_PAL_LM_LINK_FSM,"decode_amp_assoc: preferred list-scan mode --%d",preferredChannelList.Dott11_b_g_ChanList[i].chan_scan_mode));
                            assoc->channelList.number[i] = preferredChannelList.Dott11_b_g_ChanList[i].chan_number;
                            assoc->channelList.numChannels++;
                            i++;
                        }
                        sme_trace_info((TR_PAL_LM_LINK_FSM,"decode_amp_assoc: num preferred channel--%d",assoc->channelList.numChannels));
                    }
                }
                preferredChannelListPresent = TRUE;
                break;

            case AMP_ASSOC_MAC_ADDRESS_TYPE:
                status = decode_amp_assoc_mac_address(&buffer, ieLength, assoc->macAddress.data);
                macAddressPresent = TRUE;
                break;

            case AMP_ASSOC_PAL_CAPABILITY_FIELD_TYPE:
                status = decode_amp_assoc_pal_capability_field(&buffer,
                                                               ieLength,
                                                               &assoc->capability);
                break;

            case AMP_ASSOC_CONNECTED_CHANNEL_TYPE:
                 {
                    USED_CHANNELS_LIST connectedChannelList;

                    status = decode_amp_assoc_channel_list(context,&buffer, ieLength, &connectedChannelList);

                    /* FIXME: for the time being copy the channel list onto a seperate list with only channel number.
                     * eventually PAL should handle other info like tx power as well
                     */
                    assoc->connectedChannelList.numChannels=0;
                    if (connectedChannelList.listChangedFlag)
                    {
                        int i=0;
                        while (i<HIGHEST_80211_b_g_CHANNEL_NUM &&
                               channelScanMode_passive != connectedChannelList.Dott11_b_g_ChanList[i].chan_scan_mode)
                        {
                            sme_trace_info((TR_PAL_LM_LINK_FSM,"decode_amp_assoc:connected list- scan mode --%d, count=%d",connectedChannelList.Dott11_b_g_ChanList[i].chan_scan_mode,i));
                            assoc->connectedChannelList.number[i] = connectedChannelList.Dott11_b_g_ChanList[i].chan_number;
                            assoc->connectedChannelList.numChannels++;
                            i++;
                        }
                        sme_trace_info((TR_PAL_LM_LINK_FSM,"decode_amp_assoc: num connected channels--%d",assoc->connectedChannelList.numChannels));
                    }
                }
                break;

            case AMP_ASSOC_PAL_VERSION_TYPE:
                sme_trace_warn((TR_PAL_LM_LINK_FSM,"decode_amp_assoc: pal version. not handled. discarding the ie", type, ieLength));
                buffer += ieLength;
                status = TRUE;
                break;

            default:
                sme_trace_warn((TR_PAL_LM_LINK_FSM,"Unrecognised Assoc IE Type 0x%x of length - %d.Ignore this IE", type, ieLength));
                buffer += ieLength;
                status = TRUE;
                break;
        }
    }while ( (buffer-data) < len && status==TRUE);

    /* As per spec D09r08 both MAC Address and Preferrred Channel List are mandatory . see clause 2.10.1 */
    return (TRUE==macAddressPresent && TRUE==preferredChannelListPresent)?status:FALSE;
}

void pal_encode_amp_assoc(FsmContext *context,
                          PalChannellist *connectedChannelList,
                          PalChannellist *preferredChannelList,
                          unifi_MACAddress *macAddress,
                          PAL_CoexCapabilities *capabilities,
                          CsrUint8 **data,
                          CsrUint16 *len)
{
    CsrUint8 *buffer;
    CsrUint8 i;
    USED_CHANNELS_LIST channelList;

    *data = CsrPmalloc(PAL_AMP_ASSOC_MAX_TOTAL_LENGTH); /* alloc memory big enough to hold the biggest assoc*/
    buffer = *data;
    sme_trace_entry((TR_PAL_LM_LINK_FSM,"encode_amp_assoc"));
    encode_amp_assoc_mac_address(&buffer,macAddress->data);

    CsrMemSet(&channelList, 0, sizeof(USED_CHANNELS_LIST));

    /* consider the preferred only if PAL has started the network (which means numChannels==1).
    * Otherwise regulatory domain can get the appropriate valid channel list
    */
    if (1 == preferredChannelList->numChannels)
    {
        channelList.listChangedFlag = TRUE;
        channelList.Dott11_b_g_ChanList[0].chan_scan_mode = channelScanMode_active;
        channelList.Dott11_b_g_ChanList[0].chan_number = preferredChannelList->number[0];
    }
    encode_amp_assoc_channel_list(context, &buffer, &channelList, AMP_ASSOC_PREFERRED_CHANNEL_LIST_TYPE);
    verify(TR_PAL_LM_LINK_FSM, (CsrUint16)(buffer-(*data))<=PAL_AMP_ASSOC_MAX_TOTAL_LENGTH);

    if (connectedChannelList && connectedChannelList->numChannels)
    {
        CsrMemSet(&channelList, 0, sizeof(USED_CHANNELS_LIST));
        channelList.listChangedFlag = TRUE;
        for (i=0; i<connectedChannelList->numChannels && i<HIGHEST_80211_b_g_CHANNEL_NUM; i++)
        {
            channelList.Dott11_b_g_ChanList[i].chan_scan_mode = channelScanMode_active;
            channelList.Dott11_b_g_ChanList[i].chan_number = connectedChannelList->number[i];
        }
        encode_amp_assoc_channel_list(context, &buffer,&channelList, AMP_ASSOC_CONNECTED_CHANNEL_TYPE);
        verify(TR_PAL_LM_LINK_FSM, (CsrUint16)(buffer-(*data))<=PAL_AMP_ASSOC_MAX_TOTAL_LENGTH);
    }
    encode_amp_assoc_capabilities(&buffer,capabilities);
    encode_amp_assoc_pal_version(&buffer);

    *len = (CsrUint16)(buffer-(*data));
    verify(TR_PAL_LM_LINK_FSM,*len <= PAL_AMP_ASSOC_MAX_TOTAL_LENGTH);
    sme_trace_info((TR_PAL_LM_LINK_FSM,"encode_amp_assoc:encoded %d bytes",*len));
}

void pal_reset_amp_assoc_info(FsmContext *context, assocDataInfo *assocInfo)
{
    if (assocInfo->data)
    {
        assocInfo->currentLen=assocInfo->totalLen=0;
        CsrPfree(assocInfo->data);
        assocInfo->data=NULL;
    }
}


/** @}
 */
