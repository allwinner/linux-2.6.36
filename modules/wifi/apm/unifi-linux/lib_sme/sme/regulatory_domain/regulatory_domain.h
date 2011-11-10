/** @file regulatory_domain.h
 *
 * Utilities for processing regulatory domain
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2006-2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Utilities for processing regulatory domain
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/regulatory_domain/regulatory_domain.h#2 $
 *
 ****************************************************************************/
#ifndef REGULATORY_DOMAIN_H
#define REGULATORY_DOMAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup regulatory_domain
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "sme_configuration/sme_configuration_fsm.h"

#include "ie_access/ie_access.h"

/* MACROS *******************************************************************/

#define IE_MIN_LEN_COUNTRY 6
#define COUNTRY_STRING_LENGTH 3
#define SUB_BAND_TRIPLET_LENGTH 3
#define HIGHEST_80211_b_g_CHANNEL_NUM  14
#define HIGHEST_80211_CHANNEL_NUM  200

/* Legal Channels for Each Regulatory Domain  (as per IEEE 802.11-2007) */
#define LEGAL_CHANS_FCC       0x07FF /* Legal Channels = { 0(#16 N/A), 0(#15 N/A), 0(#14), 0(#13), 0,1, 1, 1, 1, 1, 1, 1, 1, 1, 1(#2), 1(#1) */
#define LEGAL_CHANS_IC        0x07FF /* 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 */
#define LEGAL_CHANS_ETSI      0x1FFF /* 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 */
#define LEGAL_CHANS_SPAIN     0x0600 /* 0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 */
#define LEGAL_CHANS_FRANCE    0x1FFF /* 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 */
#define LEGAL_CHANS_JAPAN     0x2000 /* 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 */
#define LEGAL_CHANS_JAPAN_BIS 0x1FFF /* 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 */
#define LEGAL_CHANS_CHINA     0x1FFF /* 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 */
#define LEGAL_CHANS_CHINA_BIS 0x1FFF /* 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 */
#define LEGAL_CHANS_OTHER     0x1FFF /* 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 */

#define CHANNEL_LEGALITY_LSB_MASK  0x0001

#define PAD_SZ          1

#define ACTIVE_CHANNEL_VALIDITY_PERIOD                (30 * 60 * 1000)  /* 30 mins */
#define CHECK_CHANNEL_EXPIRY_TIMER_DURATION           (30 * 1000)       /* 30 seconds */
#define CHECK_CHANNEL_EXPIRY_TIMER_TOLERANCE          1000
#define TIME_BEFORE_SCAN_REFRESH                      (2 * 60 * 1000)  /* 2 mins */
#define GUARD_TIME_SCAN_REFRESH                       (90 * 1000)      /* 1 mins 30 secs */

#define POWER_VALIDITY_PERIOD                         (15 * 60 * 1000)  /* 15 mins */

/** Max Number of Sub-Bands Triplets (1st Chan, Num Chans, Power) in Country Info IE:
 *  802.11b_g only: worst case when each sub-band contains one channel and in Japan all channels (14) are used
 *  802.11a need to change 14 by 200
 */
#define MAX_NUM_SUB_BAND_TRIPLETS    14

#define CHANNEL_MIN_TX_POWER_dBm      8
#define CHANNEL_INVALID_POWER      (-127)

/* TYPES DEFINITIONS ********************************************************/

typedef enum RegulatoryDomOperation
{
    regDomain_Off=0,
    regDomain_On=1
} RegulatoryDomOperation;

typedef enum ProcessTriplet_RESULT
{
    SubBand_Triplet_Invalid=0,
    Regulatory_Triplet_Invalid=1,
    SubBand_Triplet_Valid=2,
    Regulatory_Triplet_Valid=3
} ProcessTriplet_RESULT;

typedef struct Triplet
{
    CsrUint8 first_channel;
    CsrUint8 num_channels;
    CsrInt8  legal_power_dBm;
} Triplet;

typedef struct COUNTRY_INFO_STRUCT
{
    CsrUint8 ieId;
    CsrUint8 ieLen;
    CsrUint8 countryString[COUNTRY_STRING_LENGTH];
    Triplet subBand[MAX_NUM_SUB_BAND_TRIPLETS];
} COUNTRY_INFO_STRUCT;

typedef enum ChannelScanMode
{
    channelScanMode_passive=0,
    channelScanMode_active=1,
    channelScanMode_none=2
} ChannelScanMode;

typedef enum POWER_REGIME
{
    EIRP_PWR_REGIME,
    TPO_PWR_REGIME,
    UNKNOWN_PWR_REGIME
} POWER_REGIME;

typedef struct Channel_Info
{
    CsrUint8            chan_number;
    ChannelScanMode  chan_scan_mode;
    POWER_REGIME     chan_pwr_regime;
    CsrInt8             chan_tx_power;

    /* Data To control Timers */
    CsrUint32  expiryTime;
    CsrBool expiryFlag;

    /* Power Timer */
    CsrUint32 expiryTimePower;
    CsrBool hasPwrExpired;
} Channel_Info;

typedef struct USED_CHANNELS_LIST
{
    CsrBool       listChangedFlag;
    Channel_Info  Dott11_b_g_ChanList[HIGHEST_80211_b_g_CHANNEL_NUM];
} USED_CHANNELS_LIST;

typedef struct RegulatoryDomainInfoStruct
{
    unifi_RegulatoryDomain   RegDomain; /* Enum Value */
#ifdef SME_TRACE_ENABLE
    const char* const    RegDomainStr; /* String value (useful for debug/GUI etc) */
#endif
    POWER_REGIME         powerRegime;
    CsrUint16               chan_legality;
    CsrInt8                 chan_power[HIGHEST_80211_b_g_CHANNEL_NUM];
} RegulatoryDomainInfoStruct;

typedef enum InOutOperation
{
    Regulatory_Domain_Invalid,
    Regulatory_Domain_NonRegulatory,
    Regulatory_Domain_Indoor,
    Regulatory_Domain_Outdoor,
    Regulatory_Domain_Indoor_Outdoor
} InOutOperation;

typedef enum UpdateLocationResult
{
    updateLocation_Error=-1,
    updateLocation_NoUpdate=0,
    updateLocation_CountryOnly=1,
    updateLocation_UpdateAll=2,
    updateLocation_Ignore=3
} UpdateLocationResult;

typedef enum LocationValidity
{
    Location_Invalid=0,
    Location_Valid=1
} LocationValidity;

typedef struct GeographicalLocation
{
    LocationValidity locStatus;
    char countryString[3];
    unifi_RegulatoryDomain regDomain;
    POWER_REGIME  locationPwrRegime;

    /* Regulatory Class (802.11h/j) */
    CsrUint8 regulatory_ext_id;
    CsrUint8 regulatory_class;
    CsrInt8  coverage_class;
} GeographicalLocation;

typedef struct CountryItem
{
    const char* const countryCode;
    unifi_RegulatoryDomain regDomain;
} CountryItem;

typedef struct RegDomData
{
    RegulatoryDomOperation regulatoryDomainOperation;
    FsmTimerId chkChanXpryTimerId;

    GeographicalLocation CurrentLocation;
    GeographicalLocation DefaultLocation;

    CsrInt8 AntennaGain;

    USED_CHANNELS_LIST  ChannelsInUseList;

    /* Storage for incoming Country IE when Joining
     * Assume maximum number of Triplets (14; when each channel has a different power level)
     * CsrInt8 necessary since power is signed
     * CsrInt8 will NOT work for 802.11a as channel num greater than 127
     */
    CsrInt8 countryIeFromConnection[2+COUNTRY_STRING_LENGTH+MAX_NUM_SUB_BAND_TRIPLETS*SUB_BAND_TRIPLET_LENGTH];

} RegDomData;

/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/


/**
 * @brief  Initialises Regulatory Domain Data
 *
 * @par Description
 * called from Scan Manager at initialisation before initial scan but after MIB
 * values have been read. This function initialises all the relevant regulatory
 * domain data (depending on Trust Level) and current Regulatory Domain held in
 * Firmware. If Trust Level is not compatible with the Read MIB value
 * dot11MultiDomainCapabilityEnabled then the return code indicates (to scan manager)
 * to set dot11MultiDomainCapabilityEnabled either to TRUE or FALSE so as to agree with
 * configured Trust Level.
 *
 * @param[in]    context  :   FSM context
 * @param[in]    regdata  :   RegDomData (where Regulatory Domain data is stored)
 *
 * @return
 *    None
 */
extern void regdom_init(FsmContext* context,
                        RegDomData *regdata);

/**
 * @brief  Creates a data reference containing a country Infor IE
 *
 * @par Description
 * These functions return a data reference that, if possible, will contain a
 * country Info IE. There are two variants - one that accepts a data reference
 * as input, another that accepts a buffer plus its length.
 *
 * If the input does not contain a country IE, these functions will attempt to
 * artificially generate one (based on the contents of the active list)
 *
 * @param[in]    context  :   FSM context
 * @param[in]    regdata  :   RegDomData (where Regulatory Domain data is stored)
 *
 * THEN:
 *   @param[in]  inIeDR   :   Input DataRef containing a starting set of IEs
 * OR:
 *   @param[in]  inBuf    :   Input buffer containing a starting set of IEs
 *   @param[in]  inBufLen :   The length of the input buffer.
 *
 * FOLLOWED BY:
 *   @param[in]  forBssJo :   The data reference is for joining a BSS
 *
 * @return
 *    DataReference containing input IEs plus (where poss.) a country IE.
 */
DataReference regdom_create_ie_dr(FsmContext* context, DataReference inputIeDR, CsrBool forBssJoin);
DataReference regdom_create_ie_dr_src_buf(FsmContext* context, CsrUint8 *inBuf, CsrUint16 inBufLen, CsrBool forBssJoin);

/**
 * @brief extracts the Country Information IE from the Scan Indication
 *
 * @par Description:
 * the passed mlme scan indication points to some Information Elements
 * one of which may (or may not) be a Country Information IE.
 * Therefore this function check if it is and if it is correct (to a certain
 * extent). The correctness criteria are lenient, ie: as long as country code is correct
 * we can deduce Regulatory domain and channels to use in Active scan.
 *
 * @ingroup scanmanager
 *
 *
 * @param[in]    context          : FSM context
 * @param[in]    regdata          : RegDomData (where Regulatory Domain data is stored)
 * @param[in]    receive_bss_type : BSSType (to distinguish between IBSS and Infrastructure)
 * @param[in]    receive_chan     : ChannelNumber (channel which received beacon)
 * @param[in]    receive_freq     : Megahertz (Frequency corresponding to channel)
 * @param[in]    ieRef            : IE to be processed
 *
 * @return
 *      none
 */
/*---------------------------------------------------------------------------*/
extern void  regdom_process_beacon(FsmContext* context,
                                   RegDomData *regdata,
                                   BssType receive_bss_type,
                                   ChannelNumber receive_chan,
                                   Megahertz receive_freq,
                                   const DataReference ieRef);

/**
 * @brief  Checks if there is an active channel about to expire
 *
 * @par Description:
 * Checks if there is an active channel about to expire
 *
 * @ingroup scanmanager
 *
 *
 * @param[in]    context          : FSM context
 * @param[in]    regdata          : RegDomData (where Regulatory Domain data is stored)
 *
 * @return
  *      CsrBool       : TRUE if about to expire otherwise FALSE
 */
/*---------------------------------------------------------------------------*/
CsrBool regdom_check_for_scan_refresh(FsmContext* context,
                                      RegDomData* regdata);

/**
 * @brief  Checks if channels timer has expired and if so changes channel to passive
 *
 * @par Description:
 * Checks for each active channel (with expiryTimer Enabled) if associated timer has expired
 * and if so it changes channel scan mode to passive.
 *
 * @ingroup scanmanager
 *
 *
 * @param[in]    context          : FSM context
 * @param[in]    regdata          : RegDomData (where Regulatory Domain data is stored)
 *
 * @return
  *      CsrBool.
 */
/*---------------------------------------------------------------------------*/
CsrBool regdom_process_expired_channels(FsmContext* context,
                                        RegDomData * regdata);

/**
 * @brief  Returns TRUE if the specified channel can be transmitted on
 *
 * @par Description:
 * If the regulatory subsystem is disabled, the function always returns TRUE; otherwise, it
 * checks to see if the channel is currently in the active scan list.
 *
 * @ingroup scanmanager
 *
 * @param[in]    context          : FSM context
 * @param[in]    channelNum       : The channel number to test
 *
 * @return
  *      CsrBool.
 */
/*---------------------------------------------------------------------------*/
CsrBool regdom_is_channel_actively_usable(FsmContext* context,
                                          CsrUint8 channelNum);

/**
 * @brief  Returns the first channel that transmissions are legal on, zero if none.
 *
 * @par Description:
 * Returns the first channel that transmissions are legal on, zero if none.
 *
 * @ingroup scanmanager
 *
 * @param[in]    context          : FSM context
 *
 * @return
  *      CsrUint8                    : first active channel (1-14) or 0 if no active channel
 */
/*---------------------------------------------------------------------------*/
CsrUint8 regdom_get_first_active_channel(FsmContext* context);

void regdom_process_location_signal(FsmContext* context, RegDomData* regdata, CsrUint8* ctryCode);

#ifdef CSR_AMP_ENABLE
/**
 * @brief  return channel list after decoding the country ie.
 *
 * @par Description:
 * function for decoding the country ie in amp assoc ie. It returns the a structure with
 * the triplet expanded to individual entries of channels. The IE excludes id, length and padding. 
 * So the length will be a multiple of 3 which it will be just triplets (either regulatory triplet or sub-band triplets 
 * or a combination).
 *
 * @ingroup scanmanager
 *
 * @param[in]    context          : FSM context
 * @param[in]    regdata          : reg data
 * @param[out]    preferredChannelList          : channel list populated by the function. unused entries in the array is set to all zeros. listchangedflag is set to TRUE if atleast one channel is present.
 * @param[in]    element          :  country ie as received in amp assoc. According to BT3.0+HS spec section 2.14.4/2.14.5 
 *                                              "The format of the list is identical to the 802.11 Country information element, excluding the 802.11 information,
 *                                               element identifier, length, and pad fields".
 * @param[in]    length          : amp assoc country ie length
 *
 * @return
  *
 */
void reg_domain_amp_decode_country_ie(FsmContext* context, RegDomData *regdata, USED_CHANNELS_LIST  *preferredChannelList, CsrUint8 *element, CsrUint16 length);

/**
 * @brief  return channel list after decoding the country ie.
 *
 * @par Description:
 * function for encoding the country ie to be used in amp assoc ie. It returns the encoded IE in the
 * buffer provided by the caller. If channelList is valid (mean atleast one channel present) then these needs to be
 * included as sub-band triplet to the corresponding regulatory triplet. AMP PAL standard requires atleast one regulatory 
 * triplet to be present.  If the channelList includes only one channel, then there should be only one regulatory triplet corresponding
 * to that channel. This is to handle the case where the AMP has started a network and beaconing on that channel.
 * The encoded IE excludes id, length and padding. 
 * So the length will be a multiple of 3 which it will be just triplets (either regulatory triplet or sub-band triplets 
 * or a combination).
 *
 * @ingroup scanmanager
 *
 * @param[in]    context          : FSM context
 * @param[in]    regdata          : reg data
 * @param[out]    encodedIE          :  encoded country ie. According to BT3.0+HS spec section 2.14.4/2.14.5 
 *                                              "The format of the list is identical to the 802.11 Country information element, excluding the 802.11 information,
 *                                               element identifier, length, and pad fields". memory is allocated by caller to accomodate a  maximum size. function does not check memory overruns.
 * @param[out]    encodedLength          : length of the encoded IE.
 * @param[in]    channelList          : channel list optionally filled with channels to override default channel in the regulatory class. Used
 *                                                  primarly to add the beaconing channel when PAL is the initiator. Caller should set the listchangedflag to TRUE if channel is added.
 *
 * @return
  *
 */
void reg_domain_amp_encode_country_ie(FsmContext* context, RegDomData *regdata, CsrUint8 *encodedIE, CsrUint16 *encodedLength, USED_CHANNELS_LIST *channelList);

#endif
/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* REGULATORY_DOMAIN_H */
