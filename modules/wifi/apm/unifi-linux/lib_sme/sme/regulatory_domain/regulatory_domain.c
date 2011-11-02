/** @file regulatory_domain.c
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
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/regulatory_domain/regulatory_domain.c#3 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_trace/sme_pbc.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "scan_manager_fsm/scan_manager_fsm.h"
#include "network_selector_fsm/network_selector_fsm.h"
#include "sme_configuration/sme_configuration_fsm.h"

#include "payload_manager/payload_manager.h"

#include "ie_access/ie_access.h"

#include "regulatory_domain/regulatory_domain.h"

/* MACROS *******************************************************************/
#define REGULATORY_CHANNEL_LEGAL(dom, ch)     \
            (((dom) >> ((ch)-1)) & CHANNEL_LEGALITY_LSB_MASK)

/* TYPES DEFINITIONS ********************************************************/


/* GLOBAL VARIABLE DECLARATIONS *********************************************/

FsmTimerId RegDomainTimerId;

/*
 * List defining order in which channels are passed to Firmware for Scanning
 * Based on the fact that 1, 6 and 11 are most distant (lesser interferences) so
 * that most widely used hence gven higher prioriry.
 * But stritly speaking only valid under FCC, as under ETSI most distant will be
 * 1,7 and 13 and under Japan 1, 7, 13 or 14...
 * Hence ordering is Regulatory Domain dependent But to simplify implementation
 * use one list.
 */
const CsrUint8 channelScanOrderList[ HIGHEST_80211_b_g_CHANNEL_NUM ] = {  1, 6, 11, 4, 9, 13,
                                                                       3, 8, 12, 2, 7, 10,
                                                                       5, 14
                                                                    };
/*
 * Table listing all countries we can handle at the moment
 */
static const CountryItem CountryList [] = {
    /*
     * Currently supporting all countries handled in Firmware (and UK)
     * any country that is not in table will be assumed ETSI...
     */
    {"CA",  unifi_RegulatoryDomainIc          /* CANADA */                  },
    {"CN",  unifi_RegulatoryDomainChina       /* CHINA  */                  },
    {"ES",  unifi_RegulatoryDomainSpain       /* SPAIN  */                  },
    {"FR",  unifi_RegulatoryDomainFrance      /* FRANCE */                  },
    {"GB",  unifi_RegulatoryDomainEtsi        /* UNITED KINGDOM */          },
    {"GF",  unifi_RegulatoryDomainFrance      /* FRENCH GUIANA */           },
    {"JP",  unifi_RegulatoryDomainJapan       /* JAPAN */                   },
    {"HK",  unifi_RegulatoryDomainChina       /* HONK-KONG */               },
    {"MX",  unifi_RegulatoryDomainFcc         /* MEXICO */                  },
    {"PF",  unifi_RegulatoryDomainFrance      /* FRENCH POLYNESIA */        },
    {"PR",  unifi_RegulatoryDomainFcc         /* PUERTO RICO */             },
    {"TF",  unifi_RegulatoryDomainFrance      /* FRENCH Sth Territories */  },
    {"TW",  unifi_RegulatoryDomainFcc         /* TAIWAN */                  },
    {"UM",  unifi_RegulatoryDomainFcc         /* US Minor Outlining Islands */   },
    {"US",  unifi_RegulatoryDomainFcc         /* UNITED STATES */                },
    {"NL",  unifi_RegulatoryDomainEtsi        /* NETHERLANDS */                  },
    {"BR",  unifi_RegulatoryDomainFcc         /* BRAZIL */                       },
    {"AT",  unifi_RegulatoryDomainEtsi        /* AUSTRIA */                      },
    {"BE",  unifi_RegulatoryDomainEtsi        /* BELGIUM */                      },
    {"BG",  unifi_RegulatoryDomainEtsi        /* BULGARIA */                     },
    {"KH",  unifi_RegulatoryDomainOther       /* CAMBODIA */                     },
    {"MA",  unifi_RegulatoryDomainOther       /* MOROCCO */                      },
    {"MG",  unifi_RegulatoryDomainOther       /* MADAGASCAR */                   },
    {"NA",  unifi_RegulatoryDomainOther       /* NAMIBIA */                      },
    {"NG",  unifi_RegulatoryDomainOther       /* NIGERIA */                      },
    {"QA",  unifi_RegulatoryDomainOther       /* QATAR */                        },
    {"SG",  unifi_RegulatoryDomainOther       /* SINGAPORE */                    }
};

static const CsrUint16 NUM_LISTED_COUNTRIES= sizeof(CountryList) / sizeof(CountryItem);


/* Stored Reference data For Regulatory Domain FCC (Noth America) */
static const RegulatoryDomainInfoStruct RegulatoryDomData_FCC =
{
    unifi_RegulatoryDomainFcc,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainFCC",
#endif
    TPO_PWR_REGIME,
    LEGAL_CHANS_FCC,
    {
        /* Max(TPO) : 1w, 30dBm */
        /* TODO:  Need to Use EIRP values which we do not have, so use Canada's (36) which has same TPO ( BUT need to Check) */
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER
    }
};

/* Stored Reference data For Regulatory Domain CANADA */
static const RegulatoryDomainInfoStruct RegulatoryDomData_IC =
{
    unifi_RegulatoryDomainIc,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainIC",
#endif
    TPO_PWR_REGIME,
    LEGAL_CHANS_IC,
    {
        /* Max(EIRP): 4w, 36dBm */
        /* Max(TPO) : 1w, 30dBm */
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER
    }
};

/* Stored Reference data For Regulatory Domain ETSI (EUROPE) */
static const RegulatoryDomainInfoStruct RegulatoryDomData_ETSI =
{
    unifi_RegulatoryDomainEtsi,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainETSI",
#endif
    EIRP_PWR_REGIME,
    LEGAL_CHANS_ETSI,
    {
        /* Max(EIRP): 100mW, 20dBm */
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        CHANNEL_INVALID_POWER
    }
};

/* Stored Reference data For Regulatory Domain SPAIN */
static const RegulatoryDomainInfoStruct RegulatoryDomData_SPAIN =
{
    unifi_RegulatoryDomainSpain,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainSPAIN",
#endif
    EIRP_PWR_REGIME,
    LEGAL_CHANS_SPAIN,
    {
        /* TODO:  Now channels 1-13 are legal but need to agree with firmware */
        /* Max(EIRP): 100mW, 20dBm */
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        20,
        20,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER
    }
};

/* Stored Reference data For Regulatory Domain FRANCE */
static const RegulatoryDomainInfoStruct RegulatoryDomData_FRANCE =
{
    unifi_RegulatoryDomainFrance,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainFRANCE",
#endif
    EIRP_PWR_REGIME,
    LEGAL_CHANS_FRANCE,
    {
        /* TODO:  Now channels 1-13 are legal but need to agree with firmware */
        /* Max(EIRP): 100mW, 20dBm */
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        10,
        10,
        10,
        10,
        CHANNEL_INVALID_POWER
    }
};

/* Stored Reference data For Regulatory Domain JAPAN */
static const RegulatoryDomainInfoStruct RegulatoryDomData_JAPAN =
{
    unifi_RegulatoryDomainJapan,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainJAPAN",
#endif
    EIRP_PWR_REGIME,
    LEGAL_CHANS_JAPAN,
    {
        /* Max(EIRP): 200mW, 23dBm */
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        CHANNEL_INVALID_POWER,
        23
    }
};

/* Stored Reference data For Regulatory Domain JapanBis  */
static const RegulatoryDomainInfoStruct RegulatoryDomData_JapanBis  =
{
    unifi_RegulatoryDomainJapanBis,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainJapanBis",
#endif
    EIRP_PWR_REGIME,
    LEGAL_CHANS_JAPAN_BIS,
    {
        /* Max(EIRP): 200mW, 23dBm */
        23,
        23,
        23,
        23,
        23,
        23,
        23,
        23,
        23,
        23,
        23,
        23,
        23,
        CHANNEL_INVALID_POWER
    }
};

/* Stored Reference data For Regulatory Domain CHINA */
static const RegulatoryDomainInfoStruct RegulatoryDomData_CHINA =
{
    unifi_RegulatoryDomainChina,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainCHINA",
#endif
    EIRP_PWR_REGIME,
    LEGAL_CHANS_CHINA,
    {
        /* Max(EIRP): 2w, 33dBm */
        33,
        33,
        33,
        33,
        33,
        33,
        33,
        33,
        33,
        33,
        33,
        33,
        33,
        CHANNEL_INVALID_POWER
    }
};

#if 0 /* NB: CHINA_BIS not currently supported */
/* Stored Reference data For Regulatory Domain CHINA_BIS */
static const RegulatoryDomainInfoStruct RegulatoryDomData_CHINA_BIS =
{
    unifi_RegulatoryDomainChinaBis,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainCHINA_BIS",
    "China",
#endif
    "CN ",
    EIRP_PWR_REGIME,
    LEGAL_CHANS_CHINA_BIS,
    { /* Max(EIRP): 2w, 33dBm */
      33, 33, 33, 33, 33,
      33, 33, 33, 33, 33,
      33, 33, 33, CHANNEL_INVALID_POWER }
};
#endif

/* Stored Reference data for any Regulatory Domain that is not defined above */
static const RegulatoryDomainInfoStruct RegulatoryDomData_OTHER =
{
    unifi_RegulatoryDomainOther,
#ifdef SME_TRACE_ENABLE
    "unifi_RegulatoryDomainOther",
#endif
    EIRP_PWR_REGIME,
    LEGAL_CHANS_OTHER,
    {
        /* Many Countries with Different Power Requirements */
        /* So use same value as for ETSI above (20dBm EIRP) */
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        CHANNEL_INVALID_POWER
    }
};


/* Private FUNCTION Declarations ***********************************************/

static void init_default_country_using_mib_regulatory_domain( FsmContext* context,
                                                              SmeConfigData* cfg,
                                                              RegDomData *regdata );

static void invalidate_current_location( RegDomData *regdata );

static void create_channels_lists_for_initial_scan(FsmContext* context,
                                                   SmeConfigData* cfg,
                                                   RegDomData *regdata );

static ie_result ie_getCountryInfoIE( RegDomData *regdata,
                                      const CsrUint8* element,
                                      COUNTRY_INFO_STRUCT **result,
                                      CsrUint8 *numTriplets );

static CsrUint8 * create_country_ie_from_active_list( FsmContext * context,
                                                   CsrUint8 ** ieBuf,
                                                   CsrUint16 * ieLength );

static RegulatoryDomainInfoStruct* get_ptr_to_regulatory_domain_info( unifi_RegulatoryDomain reg_dom );

static CsrUint16 is_country_code_valid( CsrUint8* pCountryStr );

static InOutOperation indoor_outdoor_specified( RegDomData *regdata,
                                                CsrUint8* pCountryStr );

static UpdateLocationResult update_current_location( FsmContext* context,
                                                     RegDomData *regdata,
                                                     ChannelNumber rxChan,
                                                     CsrUint16 idx );

static UpdateLocationResult update_current_location_etsi( RegDomData *regdata,
                                                          CsrUint8* pCountryStr );


static ProcessTriplet_RESULT process_sub_band_triplet(FsmContext* context,
                                                      RegDomData *regdata,
                                                      Triplet *sb_triplet,
                                                      CsrUint8 tripletnumber );

static ProcessTriplet_RESULT process_regulatory_triplet( RegDomData *regdata,
                                                         Triplet *rg_triplet );

static ProcessTriplet_RESULT process_first_triplet(FsmContext* context,
                                                   RegDomData *regdata,
                                                   Triplet *firstTriplet );

static void process_80211d_ie( FsmContext *context,
                               RegDomData *regdata,
                               CsrUint8 *element,
                               BssType receive_bss_type,
                               ChannelNumber rxChan );

sme_trace_debug_code(
static const char * scan_mode_to_string(ChannelScanMode mode);
)

static void config_channel_for_reg_domain(FsmContext* context,
                                          CsrUint8 chnlNum,
                                          SmeConfigData* cfg,
                                          RegDomData *regdata,
                                          unifi_RegulatoryDomain reg_dom,
                                          CsrBool regDomIsMibDefault );

static void refresh_timer_on_active_legal_chans(FsmContext* context, RegDomData *regdata );

static void set_default_pwr_regime_on_legal_chans( RegDomData *regdata );

static CsrInt8 get_channel_power_from_stored_cntry_ie( RegDomData* regdata,
                                                    CsrUint8 chnl );

static CsrBool is_active_list_empty( RegDomData *regdata );

static CsrBool are_all_active_chans_legal_under_specifiedRegDom( RegDomData *regdata,
                                                                 unifi_RegulatoryDomain regdom );

static CsrInt8  convert_to_tpo( RegDomData *regdata,
                             CsrInt8 eirp_pwr );
static CsrInt8  convert_to_eirp( RegDomData *regdata,
                              CsrInt8 tpo_pwr );

static CsrUint8 map_channel_to_position(CsrUint8 channel);
static CsrUint8 map_position_to_channel(CsrUint8 pos);

static CsrUint8 to_upper_case(CsrUint8 ch);

static void print_channels_status(FsmContext* context, RegDomData *regdata );

 /* PRIVATE FUNCTION DEFINITIONS ***********************************************/

/**
 * @brief  Initialises the default country based on MIB default Regulatory Domain
 *
 * @par Description
 * Initialises the default country based on MIB default Regulatory Domain
 * This function is called by initialise_regulatory_domain_data().
 *
 * @param[in]    context  :   FSM context
 * @param[in]    cfg      :   SmeConfigData (where SME configuration Data is held)
 * @param[in]    regdata  :   RegDomData (where Regulatory Domain data is stored)
 *
 * @return
 *       None.
 */
static void init_default_country_using_mib_regulatory_domain(FsmContext* context,
                                                             SmeConfigData* cfg,
                                                             RegDomData *regdata)
{
    RegulatoryDomainInfoStruct *pDefRegDom;

    sme_trace_entry((TR_REGDOM, ">> init_default_country_using_mib_regulatory_domain()"));

    regdata->DefaultLocation.locStatus = Location_Valid;

    /* Use Int value returned by MIB  and get pointer to Default Regulatory Domain Structure */
    pDefRegDom = get_ptr_to_regulatory_domain_info(cfg->dot11CurrentRegDomain);

    if (pDefRegDom == NULL)
    {
        /* F/W did not return a Valid Regulatory Domain, So set to FCC (instead of Failure) */
        pDefRegDom = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_FCC;
        cfg->dot11CurrentRegDomain = unifi_RegulatoryDomainFcc;

        sme_trace_error((TR_REGDOM,
          "init_default_country_using_mib_regulatory_domain():: Firmware did not return a valid Default Regulatory Domain So using FCC"));
    }

    regdata->DefaultLocation.regDomain =  pDefRegDom->RegDomain;

    sme_trace_info((TR_REGDOM,
                     "init_default_country_using_mib_regulatory_domain():: MIB Default Regulatory Domain= %s (%d)",
                     pDefRegDom->RegDomainStr, pDefRegDom->RegDomain));

    /* Country String :
     * For Japan, Canada, France and Spain we can determine correct country code since it is unique
     * within regulatory domain,
     * For the rest use XXX (Unknown)
     */
    if ( (pDefRegDom->RegDomain == unifi_RegulatoryDomainJapan) || (pDefRegDom->RegDomain == unifi_RegulatoryDomainJapanBis) )
    {
        CsrMemCpy (regdata->DefaultLocation.countryString , "JP ", 3);
    }
    else if ( pDefRegDom->RegDomain == unifi_RegulatoryDomainIc )
    {
        CsrMemCpy (regdata->DefaultLocation.countryString , "CA ", 3);
    }
    else if ( pDefRegDom->RegDomain == unifi_RegulatoryDomainFrance )
    {
        CsrMemCpy (regdata->DefaultLocation.countryString , "FR ", 3);
    }
    else if ( pDefRegDom->RegDomain == unifi_RegulatoryDomainSpain )
    {
        CsrMemCpy (regdata->DefaultLocation.countryString , "ES ", 3);
    }
    else
    {
        CsrMemCpy (regdata->DefaultLocation.countryString , "XXX", 3);
    }

    sme_trace_debug((TR_REGDOM,
                     "init_default_country_using_mib_regulatory_domain(): Default Country Code is set to: %c%c",
                     regdata->DefaultLocation.countryString[0],
                     regdata->DefaultLocation.countryString[1] ));

    /*
     * Power Regime (for Default):
     * Use TPO if Regulatory Domain uses TPO and Country Code is not XX
     * otherwise use EIRP
     */
    regdata->DefaultLocation.locationPwrRegime = EIRP_PWR_REGIME;
    if ( (pDefRegDom->powerRegime== TPO_PWR_REGIME ) && (CsrStrNCmp(regdata->DefaultLocation.countryString, "XX", 2) != 0) )
    {
        regdata->DefaultLocation.locationPwrRegime = TPO_PWR_REGIME;
    }

    sme_trace_debug(( TR_REGDOM, "init_default_country_using_mib_regulatory_domain(): Default Country Power Regime is set to: %s",
                      ((regdata->DefaultLocation.locationPwrRegime== EIRP_PWR_REGIME) ? "EIRP" : ((regdata->DefaultLocation.locationPwrRegime== TPO_PWR_REGIME) ? "TPO" : "UNKNOWN")) ));

    sme_trace_entry((TR_REGDOM, "<< init_default_country_using_mib_regulatory_domain()"));
}


/**
 * @brief Prints the current channel status
 *
 * @par Description
 * Prints the current channel status for all channels 1..14 with their:
 * - scan mode
 * - power value
 * - power regime
 * - legality within current regulatory domain
 * - Time to expiry
 * - Timer enabled flag
 * - Channel power expiry time
 * - Channel power expired flag
 *
 * Useful for debbuging.
 *
 * @param[in] regdata : RegDomData (where Regulatory Domain data is stored)
 *
 * @return
 *        None.
 */
static void print_channels_status(FsmContext* context, RegDomData *regdata)
{
    sme_trace_info_code
    (
        CsrUint8  i;
        CsrUint8 chnlNum ;
        CsrBool legality;
        unifi_RegulatoryDomain reg_dom;
        RegulatoryDomainInfoStruct *currentRegDom;
        CsrUint32 currentXpryTm;
        CsrUint32 currentPowerXpryTm;
        POWER_REGIME pwrRegime;
        CsrUint32 currentTime = fsm_get_time_of_day_ms(context);

        sme_trace_entry(( TR_REGDOM, ">> print_channels_status()"));

        /* Get Enum value for Current Regulatory Domain */
        if ( regdata->CurrentLocation.locStatus == Location_Valid )
        {
            reg_dom = regdata->CurrentLocation.regDomain;
        }
        else
        {
            reg_dom = regdata->DefaultLocation.regDomain;
        }

        /* Use Enum value to access structure containig info about this regulatory domain */
        currentRegDom = get_ptr_to_regulatory_domain_info(reg_dom);

        sme_trace_info((TR_REGDOM, ""));
        sme_trace_info((TR_REGDOM, "Current 802.11b/g Channels Status:"));
        sme_trace_info((TR_REGDOM, ""));

        /* Print the passive channels */
        sme_trace_info((TR_REGDOM, "--------------------------------------------------------------------------------------------------------------------"));
        sme_trace_info((TR_REGDOM, "                                      PASSIVE CHANNELS                                                              "));
        sme_trace_info((TR_REGDOM, ""));
        sme_trace_info((TR_REGDOM, "Channel   Legality   Expires In(ms)   Timer Enabled   Power(dBm)   Regime   hasPowerExpired   Power Expires in(ms)   "));
        sme_trace_info((TR_REGDOM, "--------------------------------------------------------------------------------------------------------------------"));
        for (i=0 ; i< HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
        {
            Channel_Info * chnlInfo;
            chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[i];

            chnlNum = map_position_to_channel(i);
            legality = (REGULATORY_CHANNEL_LEGAL(currentRegDom->chan_legality, chnlNum));

            currentXpryTm = chnlInfo->expiryTime;
            pwrRegime = chnlInfo->chan_pwr_regime;
            currentPowerXpryTm = chnlInfo->expiryTimePower;
            if (chnlInfo->chan_scan_mode == channelScanMode_passive)
            {
                sme_trace_info((TR_REGDOM, "%02d        %s    %10lu       %s            %4d        %s  %s             %10lu",
                                chnlInfo->chan_number,
                                (legality == 1 ? "Legal  " : "Illegal"),
                                (chnlInfo->expiryFlag ?  (currentXpryTm - currentTime) : 0),
                                (chnlInfo->expiryFlag ? "TRUE " : "FALSE"),
                                chnlInfo->chan_tx_power,
                ( (pwrRegime== EIRP_PWR_REGIME) ? "EIRP   " : ((pwrRegime== TPO_PWR_REGIME) ? "TPO    " : "UNKNOWN") ),
                                (chnlInfo->hasPwrExpired ? "TRUE " : "FALSE"),
                ( (currentPowerXpryTm == 0xffffffff) ?  0 : (currentPowerXpryTm - currentTime) ) ));
            }
        }

        sme_trace_info((TR_REGDOM, ""));
        /* Print the active channels */
        sme_trace_info((TR_REGDOM, "--------------------------------------------------------------------------------------------------------------------"));
        sme_trace_info((TR_REGDOM, "                                      ACTIVE CHANNELS                                                               "));
        sme_trace_info((TR_REGDOM, ""));
        sme_trace_info((TR_REGDOM, "Channel   Legality   Expires In(ms)   Timer Enabled   Power(dBm)   Regime   hasPowerExpired   Power Expires in(ms)   "));
        sme_trace_info((TR_REGDOM, "--------------------------------------------------------------------------------------------------------------------"));
        for (i=0 ; i< HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
        {
            Channel_Info * chnlInfo;
            chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[i];

            chnlNum = map_position_to_channel(i);
            legality = (REGULATORY_CHANNEL_LEGAL(currentRegDom->chan_legality, chnlNum));

            currentXpryTm = chnlInfo->expiryTime;
            pwrRegime = chnlInfo->chan_pwr_regime;
            currentPowerXpryTm = chnlInfo->expiryTimePower;
            if (chnlInfo->chan_scan_mode == channelScanMode_active)
            {
                sme_trace_info((TR_REGDOM, "%02d        %s    %10lu       %s            %4d        %s  %s             %10lu",
                                chnlInfo->chan_number,
                                (legality == 1 ? "Legal  " : "Illegal"),
                                (chnlInfo->expiryFlag ?  (currentXpryTm - currentTime) : 0),
                                (chnlInfo->expiryFlag ? "TRUE " : "FALSE"),
                                chnlInfo->chan_tx_power,
                ( (pwrRegime== EIRP_PWR_REGIME) ? "EIRP   " : ((pwrRegime== TPO_PWR_REGIME) ? "TPO    " : "UNKNOWN") ),
                                (chnlInfo->hasPwrExpired ? "TRUE " : "FALSE"),
                ( (currentPowerXpryTm == 0xffffffff) ?  0 : (currentPowerXpryTm - currentTime) ) ));
            }
        }
        sme_trace_entry(( TR_REGDOM, "<< print_channels_status()"));
    )
}

/**
 * @brief Process an 802.11d information element from a beacon
 *
 * @par Description
 * Examines the beacon information elements looking for a country IE. If found
 * this IE is then processed to determine what channels the network claims are
 * legal, and the maximum transmit power levels that can be used.
 *
 * @param[in] regdata : RegDomData (where Regulatory Domain data is stored)
 * @param[in] element : Pointer to the info elements in the beacon
 * @param[in] r_bss_t : The type of network the beacon was received from
 * @param[in] rxChan  : The channel number the 802.11d beacon was received
 *
 * @return
 *        None.
 */
static void process_80211d_ie( FsmContext *context,
                               RegDomData *regdata,
                               CsrUint8 *element,
                               BssType receive_bss_type,
                               ChannelNumber rxChan)
{
    COUNTRY_INFO_STRUCT *pCountryInfoStruct  = NULL;
    CsrUint8                numReceivedTriplets = 0;
    SmeConfigData       *cfg                 = get_sme_config(context);

    sme_trace_entry((TR_REGDOM, ">> process_80211d_ie()"));

    if (ie_getCountryInfoIE(regdata, element, &pCountryInfoStruct, &numReceivedTriplets) == ie_success)
    {
        CsrUint16 cntry_index;
        UpdateLocationResult updateMylocationResult;

        sme_trace_info((TR_REGDOM,
                        "process_80211d_ie(): 802.11d IE found in beacon from channel %d",
                        rxChan));

        /* Check the Country Code (2 first characters of Country string) */
        cntry_index = is_country_code_valid(pCountryInfoStruct->countryString);

        /* we do not fail on indoor/outdoor we use default (both indoor and outdoor) */
        (void)indoor_outdoor_specified(regdata, pCountryInfoStruct->countryString);

        /* Update Country Info (and subsequent processing of triplets)
         * but only if update result is true (ie: Country for first time OR New Country)
         *
         * Any country not found in Table is assumed under ETSI (to agree with Firmware)
         * Hence a different (simpler) function is called.
         */
        if (cntry_index < NUM_LISTED_COUNTRIES)
        {
            updateMylocationResult = update_current_location(context, regdata, rxChan, cntry_index);
        }
        else if (cntry_index == 0xfffd)
        {
            updateMylocationResult = updateLocation_Ignore;
        }
        else
        { /* cntry_index == 0xffff  or cntry_index == 0xfffe */
            updateMylocationResult = update_current_location_etsi(regdata, pCountryInfoStruct->countryString );
        }

        /* Use Information in Triplets (if any) to overwrite Information derived from Stored List */
        if ( (updateMylocationResult != updateLocation_Error)
          && (updateMylocationResult != updateLocation_Ignore) )
        {
            ProcessTriplet_RESULT whatTriplet;
            CsrUint8                 ith_triplet;
            sme_trace_debug_code(CsrUint8 numCorrectTriplets = 0;)

            /* Process first triplet */
            whatTriplet = process_first_triplet (context, regdata, &(pCountryInfoStruct->subBand[0]) );
            if (whatTriplet == SubBand_Triplet_Valid)
            {
                sme_trace_debug_code(numCorrectTriplets++;)
            }
            else if (whatTriplet == SubBand_Triplet_Invalid)
            {
                /* we do not fail on an invalid sub-band, simply ignore it */
            }
            else if ((whatTriplet == Regulatory_Triplet_Invalid) || (whatTriplet == Regulatory_Triplet_Valid))
            {
                /*  We do not support Regulatory Triplet (802.11h?) at the moment */
                sme_trace_warn(( TR_REGDOM,
                                 "process_80211d_ie(): Regulatory Triplet ignored (Reg Triplets Are Not Currently Supported)" ));
            }

            /* Process subsequent Triplets if any */
            for (ith_triplet = 1 ; ith_triplet < numReceivedTriplets; ith_triplet++)
            {
                /* Do not fail on an invalid sub-band, Just ignore it */
                whatTriplet = process_sub_band_triplet(context, regdata, &(pCountryInfoStruct->subBand[ith_triplet]), (CsrUint8)(ith_triplet+1));
                sme_trace_debug_code(
                if (whatTriplet == SubBand_Triplet_Valid)
                {
                    numCorrectTriplets++;
                }
                )
            }

            sme_trace_debug(( TR_REGDOM,
                              "process_80211d_ie(): Num Valid Sub-Band Triplets Extracted= %d",
                              numCorrectTriplets ));
        }

        /* Update channels Lists based on Stored List for Regulatory Domain of Received Country */
        /*  But only if Regulatory Domain has changed
         *  Otherwise update timer information
         */
        if (updateMylocationResult == updateLocation_UpdateAll)
        {
            config_channel_for_reg_domain(context, 0, cfg, regdata, regdata->CurrentLocation.regDomain, FALSE);
        }
        else if ((updateMylocationResult == updateLocation_NoUpdate)   ||
                 (updateMylocationResult == updateLocation_CountryOnly))
        {
            if ( regdata->ChannelsInUseList.listChangedFlag == TRUE)
            {
                /* if power changes (esp TPO->EIRP or vice-versa)*/
                config_channel_for_reg_domain(context, 0, cfg, regdata, regdata->CurrentLocation.regDomain, FALSE);
            }
            else
            {
                /* otherwise just refresh timers on active channels */
                refresh_timer_on_active_legal_chans(context, regdata);
            }
        }

        print_channels_status(context, regdata);
    }

    if (pCountryInfoStruct)
    {
        CsrPfree(pCountryInfoStruct);
    }

    sme_trace_entry((TR_REGDOM, "<< process_80211d_ie()"));
}

/**
 * @brief points to relevant Regulatory Domain Structure
 *
 * @par Description
 * Each regulatory domain has an info structure containing important information
 * (esp legal channels to use).
 * Given the Regulatory Domain (enum) this function returns a pointer to corresponding
 * Regulatory Domain Info Structure or NULL  (Error) if enum value is invalid.
 *
 * @param[in]  reg_dom : unifi_RegulatoryDomain (enum identifying regulatory domain)
 *
 * @return
 *        RegulatoryDomainInfoStruct: pointer to Regulatory Domain structure
 */
static RegulatoryDomainInfoStruct* get_ptr_to_regulatory_domain_info(unifi_RegulatoryDomain reg_dom)
{
    RegulatoryDomainInfoStruct* regDomPtr;

    sme_trace_debug((TR_REGDOM, "get_ptr_to_regulatory_domain_info: reg-domain code = 0x%02x (%d)", reg_dom, reg_dom));
    switch (reg_dom)
    {
        case unifi_RegulatoryDomainEtsi:
            regDomPtr = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_ETSI;
            break;

        case unifi_RegulatoryDomainFcc:
            regDomPtr = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_FCC;
            break;

        case unifi_RegulatoryDomainIc:
            regDomPtr = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_IC;
            break;

        case unifi_RegulatoryDomainSpain:
            regDomPtr = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_SPAIN;
            break;

        case unifi_RegulatoryDomainFrance:
            regDomPtr = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_FRANCE;
            break;

        case unifi_RegulatoryDomainJapan:
            regDomPtr = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_JAPAN;
            break;

        case unifi_RegulatoryDomainJapanBis:
            regDomPtr = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_JapanBis;
            break;

        case unifi_RegulatoryDomainChina:
        case unifi_RegulatoryDomainChinaBis:
            regDomPtr = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_CHINA;
            break;

        case unifi_RegulatoryDomainOther:
            regDomPtr = (struct RegulatoryDomainInfoStruct *) &RegulatoryDomData_OTHER;
            break;

        case unifi_RegulatoryDomainNone:
        default:
            regDomPtr = NULL;
            sme_trace_warn((TR_REGDOM,
                               "get_ptr_to_regulatory_domain_info():: Invalid Regulatory Domain Value: %d",
                               reg_dom));
    }
    return regDomPtr;
}

/**
 * @brief Checks if channel is forced for passive scanning.
 *
 * @par Description
 *      Checks if channel is forced for passive scanning.
 *
 * @param[in] cfg     : SmeConfigData (for Trust Level)
 * @param[in] channel  : The channel to check
 *
 * @return
 *        CsrBool.
 */
static CsrBool isChannelScanForcedPassive(SmeConfigData* cfg, CsrUint8 channel)
{
    CsrUint16   i;
    for (i = 0; i < cfg->scanConfig.passiveChannelListCount; i++)
    {
        if (cfg->scanConfig.passiveChannelList[i] == channel)
            return TRUE;
    }
    return FALSE;
}

/**
 * @brief Create channels lists for initial scan
 *
 * @par Description
 * Create channels lists for initial scan. Legal Channels are based
 * on default regulatory domain. Scan mode (active, passive) based
 * on Trust Level.
 *
 * @param[in] cfg     : SmeConfigData (for Trust Level)
 * @param[in] regdata : RegDomData (where Regulatory Domain data is stored)
 *
 * @return
 *       None.
 */
static void create_channels_lists_for_initial_scan(FsmContext* context, SmeConfigData* cfg, RegDomData *regdata)
{
    unifi_RegulatoryDomain reg_dom;
    RegulatoryDomainInfoStruct *currentRegDom;
    ChannelScanMode requiredScanMode;
    CsrUint8 i, chnlNum;

    sme_trace_entry((TR_REGDOM, ">> create_channels_lists_for_initial_scan()"));

    /* Get Enum value for Default Regulatory Domain */
    reg_dom = regdata->DefaultLocation.regDomain;

    /* Use Enum value to access structure containig info about this regulatory domain */
    currentRegDom = get_ptr_to_regulatory_domain_info(reg_dom);

    sme_trace_entry((TR_REGDOM,
                     ">> create_channels_lists_for_initial_scan():: Using Default Regulatory Domain (%s)",
                     currentRegDom->RegDomainStr));

    /*
     * Note: The requiredScanMode and the requiredXpryFlag are set here for the legal channels but
     *        will be refined below for the illegal channels
     */
    if (cfg->smeConfig.trustLevel >= unifi_TrustMIB)
    {
        requiredScanMode = channelScanMode_active;
    }
    else
    {
        requiredScanMode = channelScanMode_passive;
    }

    /* Extract Channel info from pointed-to regulatory Domain structure */
    for (i=0 ; i < HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
    {
        CsrInt8 chanPwrUnderRegDomain;
        Channel_Info * chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[i];

        chnlNum = map_position_to_channel(i);
        chnlInfo->chan_number = chnlNum;

        /*
         * Check to see if the MIB's configured reg-dom claims the current
         * channel is legal
         */
        if (REGULATORY_CHANNEL_LEGAL(currentRegDom->chan_legality, chnlNum))
        {
            chnlInfo->chan_scan_mode = requiredScanMode;

            /*
            * If channel can be actively scanned, check the configuration to see
            * if the user has requested it be initially passively scanned
            */
            if ((requiredScanMode == channelScanMode_active) &&
                 isChannelScanForcedPassive(cfg, chnlNum))
            {
                chnlInfo->chan_scan_mode = channelScanMode_passive;
                sme_trace_debug((TR_REGDOM, "force initial passive scan on channel %d (MIB override)", chnlNum));
            }
            else
            {
                sme_trace_debug((TR_REGDOM, "channel %d not in MIB override (force passive) list", chnlNum));
            }
        }
        else    /* Current MIB config claims current channel NOT legal */
        {
            if (cfg->smeConfig.trustLevel == unifi_TrustDisabled)
            {
                chnlInfo->chan_scan_mode = channelScanMode_none;
            }
            else
            {
                chnlInfo->chan_scan_mode = channelScanMode_passive;
            }
        }

        /* Set Power Level and Regime for Channel */
        chanPwrUnderRegDomain = currentRegDom->chan_power[chnlNum-1];
        if ( (regdata->DefaultLocation.locationPwrRegime==TPO_PWR_REGIME) && chanPwrUnderRegDomain!= CHANNEL_INVALID_POWER  )
        {
            chnlInfo->chan_tx_power = convert_to_tpo( regdata, chanPwrUnderRegDomain );
            chnlInfo->chan_pwr_regime = TPO_PWR_REGIME;
        }
        else if ( (regdata->DefaultLocation.locationPwrRegime==EIRP_PWR_REGIME ) && chanPwrUnderRegDomain!= CHANNEL_INVALID_POWER  )
        {
            chnlInfo->chan_tx_power = chanPwrUnderRegDomain;
            chnlInfo->chan_pwr_regime = EIRP_PWR_REGIME;
        }
        else
        {
            chnlInfo->chan_tx_power = CHANNEL_INVALID_POWER;
            chnlInfo->chan_pwr_regime = UNKNOWN_PWR_REGIME;
        }

        sme_trace_debug(( TR_REGDOM, "Power level for channel %d is %d dBm (%s)",
                          chnlNum,
                          chnlInfo->chan_tx_power,
                          ((chnlInfo->chan_pwr_regime== EIRP_PWR_REGIME) ? "EIRP" : ((chnlInfo->chan_pwr_regime== TPO_PWR_REGIME) ? "TPO" : "UNKNOWN")) ));

        /* Initialise Expiry time (no timer yet) */
            /* Channel Timer */
            chnlInfo->expiryFlag = FALSE;
            chnlInfo->expiryTime = 0xffffffff;
            /* Power Timer */
            chnlInfo->hasPwrExpired= FALSE;
            chnlInfo->expiryTimePower= 0xffffffff;
    }
    regdata->ChannelsInUseList.listChangedFlag = TRUE;
    print_channels_status(context, regdata);

    sme_trace_entry((TR_REGDOM, "<< create_channels_lists_for_initial_scan()"));
}

/**
 * @brief Checks validity of Country Code received and deduces Regulatory Domain.
 *
 * @par Description
 * Checks validity of Country Code received and deduces Regulatory Domain
 * a Lookup table containing countries (not all at the moment) with
 * the corresponding Regulatory Domain. The receive channel is useful for Japan
 * which has 2 different Regulatory Domains.
 *
 * @param[in] pCountryStr : pointer to (3-letter) Country String
 *
 * @return
 *        CsrUint16:
 *          0..maxNumEntries-1 : Index of Valid Country Code stored in table
 *          0xfffe             : Country code received is: XX
 *          0xffff             : Invalid Country Code (or Valid but not in table)
 *
 */
static CsrUint16 is_country_code_valid(CsrUint8* pCountryStr)
{
    char ctry[3];
    CsrUint16 i;

    sme_trace_entry((TR_REGDOM, ">> is_country_code_valid()"));

    ctry[0] = *(char *)pCountryStr;
    ctry[1] = *((char *)pCountryStr+1);
    ctry[2] =  '\0';

    /* Treat Non country Entity separately */
    if ( (!CsrStrNCmp("XX", ctry, 2)) || (!CsrStrNCmp("xx", ctry, 2)) )
    {
        sme_trace_debug((TR_REGDOM, "<< is_country_code_valid(): Country Code (XX) received"));
        return 0xfffe;
    }

    /* Treat 00 (for Adjunct) also separately */
    if ( !CsrStrNCmp("00", ctry, 2) )
    {
        sme_trace_debug((TR_REGDOM, "<< is_country_code_valid(): Country Code (00) found: Ignored"));
        return 0xfffd;
    }

    /* Look in stored CountryList table if country code exist */
    for (i=0 ; i < NUM_LISTED_COUNTRIES ; i++)
    {
        if (!CsrStrNCmp(CountryList[i].countryCode, ctry, 2))
        {
            /* found it */
            sme_trace_debug((TR_REGDOM, "<< is_country_code_valid(): Found Country Code (%s) in stored List at index=%d, (domain=%d)",
                             ctry, i, CountryList[i].regDomain));
            return (i);
        }
    }

    /* Did Not Find it (either our list does not contain it or Invalid Country Code), Hence:
     * cannot find Country specified (Regulatory Domain will be assumed ETSI)
     */
    sme_trace_entry((TR_REGDOM,
                      "<< is_country_code_valid(): Could not find Country Code (%s) in Stored List", ctry ));
    return 0xffff;
}


/**
 * @brief Checks if indoor/outdoor/both/other operation is specified in Country String
 *
 * @par Description
 * Checks if indoor/outdoor/both/other operation is specified in Country String
 * if letter does not exist (or invalid) then assume both indoor and outdoor,
 * we do not reject IE (we are lenient).
 *
 * @param[in] regdata    : RegDomData (where Regulatory Domain data is stored)
 * @param[in] pCountryStr: pointer to (3-letter) Country String
 *
 * @return
 *    InOutOperation :
 *      - Regulatory_Domain_Indoor        : AP specified Indoor ('I')
 *      - Regulatory_Domain_Outdoor       : AP specified Outdoor ('O')
 *      - Regulatory_Domain_Indoor_Outdoor: AP specified both Indoor and Outdoor (' ')
 *                                          Or all other cases ('X', invalid)
 */
static InOutOperation indoor_outdoor_specified(RegDomData *regdata, CsrUint8* pCountryStr)
{
    InOutOperation in_out_result;
/*
    sme_trace_entry((TR_REGDOM, ">> indoor_outdoor_specified():: Int Value is  %d; Char Value is  '%c'",
                      *(pCountryStr+2), *(pCountryStr+2)));
*/

    switch ((unsigned char) *(pCountryStr+2))
    {
        case 'I':
            in_out_result = Regulatory_Domain_Indoor;
            sme_trace_info((TR_REGDOM,
                            "indoor_outdoor_specified():: Indoor Operation Specified"));
            break;

        case 'O':
            in_out_result = Regulatory_Domain_Outdoor;
            sme_trace_info((TR_REGDOM,
                            "indoor_outdoor_specified():: Outdoor Operation Specified"));
            break;

        case ' ':
            in_out_result = Regulatory_Domain_Indoor_Outdoor;
            sme_trace_info((TR_REGDOM,
                            "indoor_outdoor_specified():: Both Indoor and outdoor Operation Specified"));
            break;

        case 'X':
            in_out_result = Regulatory_Domain_Indoor_Outdoor; /* Regulatory_Domain_NonRegulatory */
            sme_trace_info((TR_REGDOM,
                            "indoor_outdoor_specified():: Non-Country Operation Specified"));
            break;

        default:
            in_out_result = Regulatory_Domain_Indoor_Outdoor; /* Regulatory_Domain_Invalid */
            sme_trace_info((TR_REGDOM,
                            "indoor_outdoor_specified():: Invalid Indoor/Outdoor specifier; Both Indoor/outdoor operation assumed"));
    }

    sme_trace_entry((TR_REGDOM, "<< indoor_outdoor_specified()"));

    return in_out_result;
}


/**
 * @brief Checks the First Triplet in a Country Information IE
 *
 * @par Description
 * Checks the First Triplet in a Country Information IE and decides if regulatory
 * triplet calls  process_regulatory_triplet() or if  Sub-Band triplet calls
 * process_sub_band_triplet().
 *
 * @param[in] regdata       : RegDomData (where Regulatory Domain data is stored)
 * @param[in] first_triplet : Triplet
 *
 * @return
 *     ProcessTriplet_RESULT: value returned by either process_regulatory_triplet()
 *                            or by process_sub_band_triplet().
 *      - Regulatory_Triplet_Invalid : Invalid Regulatory Triplet
 *      - Regulatory_Triplet_Valid   : Valid Regulatory Triplet
 *      - SubBand_Triplet_Invalid    : Invalid SubBand Triplet
 *      - SubBand_Triplet_Valid      : Valid SubBand Triplet
 */
static ProcessTriplet_RESULT process_first_triplet(FsmContext* context, RegDomData *regdata, Triplet *firstTriplet )
{
    ProcessTriplet_RESULT result;

    sme_trace_entry((TR_REGDOM, ">> process_first_triplet()"));

    if (firstTriplet->first_channel > HIGHEST_80211_CHANNEL_NUM)
    {
        /* Regulatory triplet */
        result = process_regulatory_triplet (regdata, firstTriplet);
    }
    else
    {
        /* Sub-Band triplet */
        result = process_sub_band_triplet (context, regdata, firstTriplet, 1);
    }

    sme_trace_entry((TR_REGDOM, "<< process_first_triplet()"));

    return result;
}


/**
 * @brief Processes a Regulatory Triplet
 *
 * @par Description
 * Processes a Regulatory Triplet
 *
 * @param[in] regdata    : RegDomData (where Regulatory Domain data is stored)
 * @param[in] rg_triplet : Triplet
 *
 * @return
 *     ProcessTriplet_RESULT:
 *      - Regulatory_Triplet_Invalid : Invalid Regulatory Triplet
 *      - Regulatory_Triplet_Valid   : Valid Regulatory Triplet
 */
static ProcessTriplet_RESULT process_regulatory_triplet(RegDomData *regdata, Triplet *rg_triplet)
{
    ProcessTriplet_RESULT outcome;

    sme_trace_entry((TR_REGDOM, ">> process_regulatory_triplet()"));

    /* @@@@@ TODO: Need to do some checking on the values
     * and set outcome to either:
     *
     *  - Regulatory_Triplet_Invalid
     *  OR
     * - Regulatory_Triplet_Valid
     *
     */
    regdata->CurrentLocation.regulatory_ext_id = rg_triplet->first_channel;
    regdata->CurrentLocation.regulatory_class = rg_triplet->num_channels;
    regdata->CurrentLocation.coverage_class = rg_triplet->legal_power_dBm;

    outcome = Regulatory_Triplet_Valid;

    sme_trace_entry((TR_REGDOM, "<< process_regulatory_triplet()"));

    return outcome;
}


/**
 * @brief Processes a Sub-Band Triplet
 *
 * @par Description
 * Processes a Sub-Band Triplet
 *
 * @param[in] regdata       : RegDomData (where Regulatory Domain data is stored)
 * @param[in] sb_triplet    : Triplet
 * @param[in] tripletnumber : CsrUint8
 *
 * @return
 *     ProcessTriplet_RESULT:
 *      - SubBand_Triplet_Invalid : Invalid SubBand Triplet
 *      - SubBand_Triplet_Valid   : Valid SubBand Triplet
 */
static ProcessTriplet_RESULT process_sub_band_triplet(FsmContext* context,
                                                      RegDomData *regdata,
                                                      Triplet *sb_triplet,
                                                      CsrUint8 tripletnumber)
{
    CsrUint8 i, ordered_position;
    CsrUint32 currentTime;
    ProcessTriplet_RESULT outcome;
    CsrUint32 now = fsm_get_time_of_day_ms(context);
    sme_trace_debug_code (char orderStr[25];)


    sme_trace_entry((TR_REGDOM, ">> process_sub_band_triplet()"));

    /* Just for printing */
    sme_trace_debug_code
    (
        if (tripletnumber == 1)
        {
            CsrStrCpy(orderStr, "1-st");
        }
        else if (tripletnumber == 2)
        {
            CsrStrCpy(orderStr, "2-nd");
        }
        else if (tripletnumber == 3)
        {
            CsrStrCpy(orderStr, "3-rd");
        }
        else
        {
            CsrSprintf(orderStr, "%d-th", tripletnumber);
        }
    )

    if ((sb_triplet->first_channel <= HIGHEST_80211_b_g_CHANNEL_NUM) &&
        ((sb_triplet->first_channel + sb_triplet->num_channels) <= (HIGHEST_80211_b_g_CHANNEL_NUM+1)))
    {
        POWER_REGIME subbandPwrRegime;

        /* Power Regime implied by IE */
        if ((regdata->CurrentLocation.regDomain == unifi_RegulatoryDomainFcc ) || (regdata->CurrentLocation.regDomain == unifi_RegulatoryDomainIc))
        {
            subbandPwrRegime = TPO_PWR_REGIME;
        }
        else
        {
            subbandPwrRegime = EIRP_PWR_REGIME;
        }

        sme_trace_debug((TR_REGDOM, "process_sub_band_triplet(): %s Triplet is Valid", orderStr));
        sme_trace_debug((TR_REGDOM, "process_sub_band_triplet():   >> Start Channel: %d", sb_triplet->first_channel));
        sme_trace_debug((TR_REGDOM, "process_sub_band_triplet():   >> Num Channels: %d", (int) sb_triplet->num_channels));
        sme_trace_debug((TR_REGDOM, "process_sub_band_triplet():   >> legal power: %d dBm (%s)", (int) sb_triplet->legal_power_dBm,
                                                                                                  ((subbandPwrRegime == EIRP_PWR_REGIME) ? "EIRP" : "TPO") ));
        /* Get Current Time */
        currentTime = now;

        for (i=sb_triplet->first_channel; i < sb_triplet->first_channel + sb_triplet->num_channels ; i++)
        {
            Channel_Info * chnlInfo;

            ordered_position = map_channel_to_position(i);
            chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[ordered_position];

            if (ordered_position == 0xff)
            {
                sme_trace_warn((TR_REGDOM, "process_sub_band_triplet: Failed to map channel %d to internal list - bailing", i));
                return SubBand_Triplet_Invalid;
            }

            /* For already active channels Replace power value but only
             * if 'power timer has expired' or 'value from Country info IE is lower'
             * Paying careful attention to Power Regime compatibility
             * Note: if power is the same then just refresh time for power to expire
             */
            if (chnlInfo->chan_scan_mode == channelScanMode_active)
            {
                if (chnlInfo->chan_pwr_regime == subbandPwrRegime)
                {
                    if ( (chnlInfo->hasPwrExpired==TRUE) || (chnlInfo->chan_tx_power > sb_triplet->legal_power_dBm) )
                    {
                        /* No need to update power regime with same value */
                        /* Update power value */
                        chnlInfo->chan_tx_power = sb_triplet->legal_power_dBm;
                        sme_trace_debug(( TR_REGDOM, "process_sub_band_triplet(): Updating Power for already Active Channel#%d to %d dBm (%s)",
                                          i,
                                          (int) sb_triplet->legal_power_dBm,
                                          ((chnlInfo->chan_pwr_regime==EIRP_PWR_REGIME) ? "EIRP" : "TPO") ));

                        /* Update Expiry Time of Power Value */
                        chnlInfo->hasPwrExpired= FALSE;
                        chnlInfo->expiryTimePower= currentTime + POWER_VALIDITY_PERIOD;

                        /* Channel list has Changed */
                        regdata->ChannelsInUseList.listChangedFlag = TRUE;
                    }
                    else if ( chnlInfo->chan_tx_power == sb_triplet->legal_power_dBm )
                    {
                        /* Refresh Expiry Time of Power Value */
                        chnlInfo->expiryTimePower= currentTime + POWER_VALIDITY_PERIOD;
                        sme_trace_debug(( TR_REGDOM, "process_sub_band_triplet(): Refreshing power expiry time for Channel#%d", i ));
                    }
                }
                else if (subbandPwrRegime == EIRP_PWR_REGIME)  /* and channel power is TPO */
                {
                    CsrInt8 sb_triplet_power_tpo;

                    sb_triplet_power_tpo = convert_to_tpo( regdata, sb_triplet->legal_power_dBm );

                    if ( (chnlInfo->hasPwrExpired==TRUE) || (chnlInfo->chan_tx_power > sb_triplet_power_tpo) )
                    {
                        sme_trace_debug_code(
                                POWER_REGIME currentChanPwrRegime;
                                currentChanPwrRegime = chnlInfo->chan_pwr_regime;
                        )

                        chnlInfo->chan_pwr_regime = subbandPwrRegime;
                        chnlInfo->chan_tx_power = sb_triplet->legal_power_dBm;
                        sme_trace_debug(( TR_REGDOM, "process_sub_band_triplet(): Updating Power for already Active Channel#%d to %d dBm (%s->%s)",
                                          i,
                                          (int) sb_triplet->legal_power_dBm,
                                          ((currentChanPwrRegime==EIRP_PWR_REGIME) ? "EIRP" : "TPO"),
                                          ((subbandPwrRegime==EIRP_PWR_REGIME) ? "EIRP" : "TPO") ));

                        /* Update Expiry Time of Power Value */
                        chnlInfo->hasPwrExpired= FALSE;
                        chnlInfo->expiryTimePower= currentTime + POWER_VALIDITY_PERIOD;

                        /* Channel list has Changed */
                        regdata->ChannelsInUseList.listChangedFlag = TRUE;
                    }
                    else if ( chnlInfo->chan_tx_power == sb_triplet_power_tpo )
                    {
                        /* Refresh Expiry Time of Power Value */
                        chnlInfo->expiryTimePower= currentTime + POWER_VALIDITY_PERIOD;
                        sme_trace_debug(( TR_REGDOM, "process_sub_band_triplet(): Refreshing power expiry time for Channel#%d", i ));
                    }
                }
                else /* subbandPwrRegime is TPO and channel power is EIRP */
                {
                    CsrInt8 sb_triplet_power_eirp;

                    sb_triplet_power_eirp = convert_to_eirp( regdata, sb_triplet->legal_power_dBm );

                    if ( (chnlInfo->hasPwrExpired==TRUE) || (chnlInfo->chan_tx_power > sb_triplet_power_eirp) )
                    {
                        sme_trace_debug_code(
                                POWER_REGIME currentChanPwrRegime;
                            currentChanPwrRegime = chnlInfo->chan_pwr_regime;
                        )

                        chnlInfo->chan_pwr_regime = subbandPwrRegime;
                        chnlInfo->chan_tx_power = sb_triplet->legal_power_dBm;
                        sme_trace_debug(( TR_REGDOM, "process_sub_band_triplet(): Updating Power for already Active Channel#%d to %d dBm (%s->%s)",
                                          i,
                                          (int) sb_triplet->legal_power_dBm,
                                          ((currentChanPwrRegime==EIRP_PWR_REGIME) ? "EIRP" : "TPO"),
                                          ((subbandPwrRegime==EIRP_PWR_REGIME) ? "EIRP" : "TPO") ));

                        /* Update Expiry Time of Power Value */
                        chnlInfo->hasPwrExpired= FALSE;
                        chnlInfo->expiryTimePower= currentTime + POWER_VALIDITY_PERIOD;

                        /* Channel list has Changed */
                        regdata->ChannelsInUseList.listChangedFlag = TRUE;
                    }
                    else if ( chnlInfo->chan_tx_power == sb_triplet_power_eirp )
                    {
                        /* Refresh Expiry Time of Power Value */
                        chnlInfo->expiryTimePower= currentTime + POWER_VALIDITY_PERIOD;
                        sme_trace_debug(( TR_REGDOM, "process_sub_band_triplet(): Refreshing power expiry time for Channel#%d", i ));
                    }
                }
            }

            /* Process Passive Channels that need to become active */
            if (chnlInfo->chan_scan_mode != channelScanMode_active)
            {
                chnlInfo->chan_scan_mode = channelScanMode_active;
                chnlInfo->expiryFlag     = TRUE;

                /* Set Power of channel to value received from Country IE and update with corresponding Power Regime */
                chnlInfo->chan_pwr_regime = subbandPwrRegime;
                chnlInfo->chan_tx_power = sb_triplet->legal_power_dBm;

                sme_trace_debug(( TR_REGDOM, "process_sub_band_triplet(): Turning Channel#%d to Active and Setting its Power to %d dBm (%s)",
                                  i,
                                  (int) sb_triplet->legal_power_dBm,
                                  ((subbandPwrRegime==EIRP_PWR_REGIME) ? "EIRP" : "TPO") ));

                /* Update Power Timer Information */
                chnlInfo->hasPwrExpired= FALSE;
                chnlInfo->expiryTimePower= currentTime + POWER_VALIDITY_PERIOD;

                /* Channel list has Changed */
                regdata->ChannelsInUseList.listChangedFlag = TRUE;
            }

            /* if channel is marked for timer expiry then update the timer value */
            if (chnlInfo->expiryFlag)
            {
                chnlInfo->expiryTime = currentTime + ACTIVE_CHANNEL_VALIDITY_PERIOD;
            }
        }

        outcome = SubBand_Triplet_Valid;
    }
    else
    {
        /* Invalid Channel Sub-Band specified:
         * Normally reject all the IE but since we are lenient:
         * ignore this triplet and move to next one
         */
        sme_trace_debug((TR_REGDOM,
                          "process_sub_band_triplet(): %s Triplet is Invalid",
                          orderStr));

        outcome = SubBand_Triplet_Invalid;
    }

    sme_trace_entry((TR_REGDOM, "<< process_sub_band_triplet()"));

    return outcome;
}

/**
 * @brief Invalidate the Current location
 *
 * @par Description
 * Invalidate the Current location (country and all corresponding information).
 * used after the current country has expired (long time since we received a Country
 * info IE), Or at beginning when we have no country.
 *
 * @param[in] regdata : RegDomData (where Regulatory Domain data is stored)
 *
 * @return
 *        None.
 */
static void invalidate_current_location(RegDomData *regdata)
{
    sme_trace_entry((TR_REGDOM, ">> invalidate_current_location()"));

    /* Validity Flag */
    regdata->CurrentLocation.locStatus = Location_Invalid;

    /* Country String */
    CsrMemCpy (regdata->CurrentLocation.countryString, "XXX", 3);

    /* Power Regime */
    regdata->CurrentLocation.locationPwrRegime = UNKNOWN_PWR_REGIME;

    /* Regulatory Domain */
    regdata->CurrentLocation.regDomain = unifi_RegulatoryDomainNone;

    sme_trace_entry((TR_REGDOM, "<< invalidate_current_location()"));
}

/**
 * @brief Update the Current location
 *
 * @par Description
 * Update the Current location (country and all corresponding information).
 * used after a new Country has been signalled in a Beacon.
 *
 * @param[in] regdata : RegDomData (where Regulatory Domain data is stored)
 * @param[in] rxChan  : Channel receiving Beacon
 * @param[in] regdata : Index (in Table) where country code matched received one
 *
 * @return
 *    UpdateLocationResult:
 *          - updateLocation_NoUpdate    :  Same country and same regulatory Domain
 *          - updateLocation_CountryOnly :  Different Country But same Reg Domain
 *          - updateLocation_UpdateAll   :  Different Reg Domains
 *          - updateLocation_Error       :  Just to catch exceptional cases
 *
 */
static UpdateLocationResult update_current_location( FsmContext* context,
                                                     RegDomData *regdata,
                                                     ChannelNumber rxChan,
                                                     CsrUint16 idx )
{
    UpdateLocationResult updateFlag = updateLocation_NoUpdate;

    sme_trace_entry((TR_REGDOM, ">> update_current_location()"));

    /* Print Location before the update */
    if (regdata->CurrentLocation.locStatus == Location_Valid)
    {
        sme_trace_debug((TR_REGDOM, "update_current_location(): Current locn: %c%c",
                                     regdata->CurrentLocation.countryString[0],
                                     regdata->CurrentLocation.countryString[1] ));
    }
    else
    {
        sme_trace_debug((TR_REGDOM, "update_current_location(): Default locn: %c%c",
                                     regdata->DefaultLocation.countryString[0],
                                     regdata->DefaultLocation.countryString[1] ));
    }

    /*
     *** Decide whether to Update or Not
     */
    if (regdata->CurrentLocation.locStatus == Location_Invalid)
    {
        /* Did not have a Country before */
        updateFlag = updateLocation_UpdateAll;
        sme_trace_info((TR_REGDOM,
                        "update_current_location(): Got a Country (%s) for 1st time (from Default).",
                        CountryList[idx].countryCode));
    }
    else if ((regdata->CurrentLocation.locStatus == Location_Valid) &&
              (CsrStrNCmp(regdata->CurrentLocation.countryString, CountryList[idx].countryCode, 2) == 0))
    {
        /* Same Country so do not update UNLESS Japan which has 2 different Regulatory Domains */
            sme_trace_debug((TR_REGDOM,
                           "update_current_location(): Same Country: %s", CountryList[idx].countryCode));

        if (CsrStrNCmp(regdata->CurrentLocation.countryString, "JP", 2) == 0)
        {
            /* Same Country: Japan */
            /* Update only if regulatory domain has changed */
            if ((regdata->CurrentLocation.regDomain==unifi_RegulatoryDomainJapan) &&  (rxChan!=14) && (rxChan!=0))
            {
                /* Japan to Japan_bis */
                updateFlag = updateLocation_UpdateAll;
                sme_trace_debug((TR_REGDOM,
                              "update_current_location(): Regulatory Domain change from: %s (%d), to: %s (%d)",
                              "unifi_RegulatoryDomainJAPAN", unifi_RegulatoryDomainJapan,
                              "unifi_RegulatoryDomainJapanBis", unifi_RegulatoryDomainJapanBis));
            }
            else if ((regdata->CurrentLocation.regDomain==unifi_RegulatoryDomainJapanBis) && (rxChan==14))
            {
                /* Japan_bis to Japan */
                updateFlag = updateLocation_UpdateAll;
                sme_trace_debug((TR_REGDOM,
                              "update_current_location(): Regulatory Domain change from: %s (%d), to: %s (%d)",
                              "unifi_RegulatoryDomainJapanBis", unifi_RegulatoryDomainJapanBis,
                              "unifi_RegulatoryDomainJAPAN", unifi_RegulatoryDomainJapan));
            }
            else if ( rxChan==0 ) /* Handles Adjunct Technology where we do not have a receive channel */
            {
                /* we stay in the current regulatory domain (JAPAN or JapanBis) we do not change */
                updateFlag = updateLocation_NoUpdate;
            }
            else
            {
                /* Same Japanese regulatory Domain */
                updateFlag = updateLocation_NoUpdate;
            }
        }
        else
        {
            /* Same Country but Not Japan */
            updateFlag = updateLocation_NoUpdate;
        }

        /* NOTE: also China has 2 different Reg domains But no point distinguishing
        *       between them here until sorted in F/W First
        */
    }
    else if ((regdata->CurrentLocation.locStatus == Location_Valid) &&
              (CsrStrNCmp(regdata->CurrentLocation.countryString, CountryList[idx].countryCode, 2) !=0))
    {
        /* Different Country ... */

        if (regdata->CurrentLocation.regDomain == CountryList[idx].regDomain)
        {
            /* Different Country and Same Regulatory Domain */
            updateFlag = updateLocation_CountryOnly; /* need only to update country code
                                      other info (esp channels) can remain as they are (Avoid thus
                                      stopping current scans and initiating new ones ... */
            sme_trace_info((TR_REGDOM,
                           "update_current_location(): New Country: %s, Same Regulatory Domain: %d",
                           CountryList[idx].countryCode,
                           CountryList[idx].regDomain));
        }
        else
        {
            /* Different Country and Different Regulatory Domain */
            updateFlag = updateLocation_UpdateAll;
            sme_trace_info((TR_REGDOM,
                           "update_current_location(): New Country: %s, Different Regulatory Domain: %d",
                           CountryList[idx].countryCode,
                           CountryList[idx].regDomain));
        }

    }
    else
    {
            /* Should never get here (so just to catch errors) */
            updateFlag = updateLocation_Error;
            sme_trace_error((TR_REGDOM,
                           "update_current_location(): Should never get here !"));
    }

    /*
     *** Update based on decision above
     */

    /* Regulatory Domain */
    if (updateFlag == updateLocation_UpdateAll)
    {
        /* Set the Regulatory Domain */
        if (!CsrStrNCmp(CountryList[idx].countryCode, "JP", 2))
        {
            if ( rxChan == 14 )
            {
                regdata->CurrentLocation.regDomain = unifi_RegulatoryDomainJapan;
            }
            else if ( rxChan==0 ) /* Handles Adjunct Technology where we do not have a receive channel */
            {
                 if ( is_active_list_empty(regdata) )
                 {
                     /* Do Nothing: but this caused a crash as we need to set the regulatory domain:JAPAN or JapanBis */
                     regdata->CurrentLocation.regDomain = unifi_RegulatoryDomainJapan;
                 }
                 else if ( regdom_is_channel_actively_usable( context, 14) )
                 {
                     regdata->CurrentLocation.regDomain = unifi_RegulatoryDomainJapan;
                 }
                 else  /* Active list is Not empty and Channel 14 is not in it */
                 {
                     regdata->CurrentLocation.regDomain = unifi_RegulatoryDomainJapanBis;
                 }
            }
            else /* rxChan: 1 to 13 */
            {
                regdata->CurrentLocation.regDomain = unifi_RegulatoryDomainJapanBis;
            }
        }
        else
        {
            regdata->CurrentLocation.regDomain = CountryList[idx].regDomain;
        }
    }

    /* Country String */
    if ((updateFlag == updateLocation_UpdateAll) || (updateFlag == updateLocation_CountryOnly))
    {
        regdata->CurrentLocation.locStatus = Location_Valid;

        /* Country String */
        if ( is_active_list_empty(regdata) || are_all_active_chans_legal_under_specifiedRegDom(regdata, regdata->CurrentLocation.regDomain ) )
        {
            regdata->CurrentLocation.countryString[0] = CountryList[idx].countryCode[0];
            regdata->CurrentLocation.countryString[1] = CountryList[idx].countryCode[1];
            regdata->CurrentLocation.countryString[2] = ' '; /*  Always in/outdoor */
        }
        else if (CsrStrNCmp(regdata->CurrentLocation.countryString, CountryList[idx].countryCode, 2) == 0)
        {
            /* for Japan to Japan_bis and Japan_bis to Japan transitions
             * Country code should not change even though regulatory domain has changed,
             * (and certainly no XXX).
             */
            /* Transitions where current country code From AP (not default) is same as country code received from another AP but different regulatory domains:
             * eg:
             * - current=Japan to new=Japan_bis,
             *   or
             * - current=Japan_bis to new=Japan
             */
            ;
        }
        else if (CsrStrNCmp(regdata->DefaultLocation.countryString, CountryList[idx].countryCode, 2) == 0)
        {
            /* Transitions where country code in default is same as country code received from AP but different regulatory domains:
             * eg:
             * - default=Japan to Japan_bis,
             *   or
             * - default=Japan_bis to Japan
             *
             *   Note: only for TrustLevel_MIB (where active list is not empty at default).
             */
            regdata->CurrentLocation.countryString[0] = CountryList[idx].countryCode[0];
            regdata->CurrentLocation.countryString[1] = CountryList[idx].countryCode[1];
            regdata->CurrentLocation.countryString[2] = ' '; /*  Always in/outdoor */
        }
        else
        {
            regdata->CurrentLocation.countryString[0] =  'X';
            regdata->CurrentLocation.countryString[1] =  'X';
            regdata->CurrentLocation.countryString[2] =  'X';
        }
    }


    /* Power Regime */
    if (updateFlag == updateLocation_UpdateAll)
    {
        RegulatoryDomainInfoStruct *currentRegDom;

        /* Use Enum value to access structure containing info about this regulatory domain */
        currentRegDom = get_ptr_to_regulatory_domain_info(regdata->CurrentLocation.regDomain);

        /*
         * Power Regime (for Updated location):
         * Use TPO if Regulatory Domain uses TPO and Country Code is not XX
         * otherwise use EIRP
         */
        regdata->CurrentLocation.locationPwrRegime = EIRP_PWR_REGIME;
        if ( (currentRegDom->powerRegime==TPO_PWR_REGIME) && (CsrStrNCmp(regdata->CurrentLocation.countryString, "XX", 2) != 0) )
        {
            regdata->CurrentLocation.locationPwrRegime = TPO_PWR_REGIME;
        }

        sme_trace_debug(( TR_REGDOM, "update_current_location(): Current Country Power Regime is set to: %s",
                      ((regdata->CurrentLocation.locationPwrRegime== EIRP_PWR_REGIME) ? "EIRP" : ((regdata->CurrentLocation.locationPwrRegime== TPO_PWR_REGIME) ? "TPO" : "UNKNOWN")) ));

        sme_trace_info_code
        (

            sme_trace_info((TR_REGDOM,
                           "update_current_location(): Enabling for Country '%c%c', regulatory domain 0x%02x (%s)",
                           regdata->CurrentLocation.countryString[0],
                           regdata->CurrentLocation.countryString[1],
                           currentRegDom->RegDomain, currentRegDom->RegDomainStr));
        )
    }

    sme_trace_entry((TR_REGDOM, "<< update_current_location()"));

    return updateFlag;
}

/*
 * @brief Update the Current location for ETSI
 *
 * @par Description
 * Update the Current location (country and all corresponding information).
 * used after a new Country has been signalled in a Beacon But not found in
 * our stored list. In this case assume ETSI (to agree with Firmware) and
 * Country String is 'XXX'
 *
 * Note: this function is a simpler version of update_current_location_etsi()
 *
 * @param[in] regdata : RegDomData (where Regulatory Domain data is stored)
 *
 * @return
 *    UpdateLocationResult:
 *          - updateLocation_NoUpdate    :  Same country and same regulatory Domain
 *          - updateLocation_CountryOnly :  Different Country But same Reg Domain
 *          - updateLocation_UpdateAll   :  Different Reg Domains
 *          - updateLocation_Error       :  Just to catch exceptional cases
 *
 */
static UpdateLocationResult update_current_location_etsi(RegDomData *regdata, CsrUint8* pCountryStr )
{
    UpdateLocationResult updateFlag = updateLocation_NoUpdate;

    sme_trace_entry((TR_REGDOM, ">> update_current_location_etsi()"));

    /*
     *** Decide whether to Update or Not
     */
    if ( regdata->CurrentLocation.locStatus == Location_Invalid ) /* Did not have a Country before (using default) */
    {
        sme_trace_debug((TR_REGDOM, "update_current_location_etsi(): Default Country Code: %c%c",
                                     regdata->DefaultLocation.countryString[0],
                                     regdata->DefaultLocation.countryString[1] ));

        updateFlag = updateLocation_UpdateAll;

        sme_trace_info(( TR_REGDOM,
                         "update_current_location_etsi(): Got a Country Code (%c%c) assumed under ETSI, for 1st time since default.",
                         *pCountryStr, *(pCountryStr+1) ));
    }
    else if (regdata->CurrentLocation.locStatus == Location_Valid ) /* had a country before (which could have been XX) */
    {
        sme_trace_debug(( TR_REGDOM, "update_current_location_etsi(): Current Country Code: %c%c",
                                     regdata->CurrentLocation.countryString[0],
                                     regdata->CurrentLocation.countryString[1] ));

        if ( CsrStrNCmp(regdata->CurrentLocation.countryString, "XX", 2) == 0 ) /* Country Code Was XX */
        {
            if (regdata->CurrentLocation.regDomain == unifi_RegulatoryDomainEtsi)
            {
                /* Country was under ETSI */
                if ( (*pCountryStr=='X') && (*(pCountryStr+1) == 'X') )
                {
                    updateFlag = updateLocation_NoUpdate;
                    sme_trace_info(( TR_REGDOM,
                                     "update_current_location_etsi(): Same Country Code (XX); Same Regulatory Domain (ETSI)" ));
                }
                else
                {
                    updateFlag = updateLocation_CountryOnly;
                    sme_trace_info(( TR_REGDOM,
                                     "update_current_location_etsi(): Different Country Code (%c%c); Same Regulatory Domain (ETSI)",
                                     *pCountryStr, *(pCountryStr+1) ));
                }
            }
            else
            {
                /* Country was NOT under ETSI */
                updateFlag = updateLocation_UpdateAll;
                sme_trace_info(( TR_REGDOM,
                                 "update_current_location_etsi(): Same Country Code (%s); Different Regulatory Domain (ETSI)", "XX" ));
            }
        }
        else /* if ( CsrStrNCmp(regdata->CurrentLocation.countryString, "XX", 2) != 0 ): Country Code Was NOT XX */
        {
            if (regdata->CurrentLocation.regDomain == unifi_RegulatoryDomainEtsi)
            {
                /* Country was under ETSI */
                updateFlag = updateLocation_CountryOnly; /* need only to update country code to XX */
                sme_trace_info(( TR_REGDOM,
                                 "update_current_location_etsi(): New Country Code (%c%c); Same Regulatory Domain (ETSI)",
                                 *pCountryStr, *(pCountryStr+1) ));
            }
            else
            {
                /* Country was NOT under ETSI */
                updateFlag = updateLocation_UpdateAll;
                sme_trace_info(( TR_REGDOM,
                                 "update_current_location_etsi(): New Country Code (%c%c); Different Regulatory Domain (ETSI)",
                                 *pCountryStr, *(pCountryStr+1) ));
            }
        }
    }
    else /* Should never get here (so just to catch errors) */
    {
        updateFlag = updateLocation_Error;
        sme_trace_error(( TR_REGDOM,
                          "update_current_location_etsi(): error in location validity (%d)", regdata->CurrentLocation.locStatus ));
    }

    /*
     *** Update based on decision above
     */

    /* Regulatory Domain */
    if (updateFlag == updateLocation_UpdateAll)
    {
        regdata->CurrentLocation.regDomain = unifi_RegulatoryDomainEtsi;
    }

    /* Country String */
    if ((updateFlag == updateLocation_UpdateAll) || (updateFlag == updateLocation_CountryOnly))
    {
        regdata->CurrentLocation.locStatus = Location_Valid;

        if ( is_active_list_empty(regdata) || are_all_active_chans_legal_under_specifiedRegDom(regdata, unifi_RegulatoryDomainEtsi) )
        {
            regdata->CurrentLocation.countryString[0] =  *(char *)pCountryStr;
            regdata->CurrentLocation.countryString[1] =  *((char *)pCountryStr+1);
            regdata->CurrentLocation.countryString[2] =  ' '; /* Assume in/out door as Adjunct Technology does not provide this */
        }
        else
        {
            regdata->CurrentLocation.countryString[0] =  'X';
            regdata->CurrentLocation.countryString[1] =  'X';
            regdata->CurrentLocation.countryString[2] =  'X';
        }
    }

    /* Power Regime */
    if (updateFlag == updateLocation_UpdateAll)
    {
        regdata->CurrentLocation.locationPwrRegime = EIRP_PWR_REGIME; /* since ETSI */
    }

    /* Finally ... */
    if ((updateFlag == updateLocation_UpdateAll) || (updateFlag == updateLocation_CountryOnly))
    {
        sme_trace_info((TR_REGDOM,
                        "update_current_location_etsi(): Enabling for Country Code '%c%c', Regulatory Domain ETSI",
                        regdata->CurrentLocation.countryString[0], regdata->CurrentLocation.countryString[1] ));
    }

    sme_trace_entry((TR_REGDOM, "<< update_current_location_etsi()"));

    return updateFlag;
}


/**
 * @brief  Checks if all active channels are legal under specified (arg 2) regulatory Domain
 *
 * @par Description
 *
 * @param[in]
 *      regdata       : RegDomData (where Regulatory Domain data is stored)
 *      regdom        : value of Regulatory Domain
 *
 * @return
 *      outcome       : CsrBool
 */
static CsrBool are_all_active_chans_legal_under_specifiedRegDom( RegDomData *regdata, unifi_RegulatoryDomain regdom  )
{
    RegulatoryDomainInfoStruct *currentRegDom;
    CsrUint8 i, chnlNum;

    sme_trace_entry((TR_REGDOM, ">> are_all_active_chans_legal_under_specifiedRegDom()"));

    /* Access structure containig info about this regulatory domain */
    currentRegDom = get_ptr_to_regulatory_domain_info(regdom);

    for (i=0 ; i< HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
    {
        if (regdata->ChannelsInUseList.Dott11_b_g_ChanList[i].chan_scan_mode == channelScanMode_active)
        {
            chnlNum = map_position_to_channel(i);
            if ( ! REGULATORY_CHANNEL_LEGAL(currentRegDom->chan_legality, chnlNum) )
            {
                /* Found one that isn't legal (don't need to know more) */
                sme_trace_debug((TR_REGDOM, "Found illegal channel %d", chnlNum));
                sme_trace_entry((TR_REGDOM, "<< are_all_active_chans_legal_under_specifiedRegDom(FALSE)"));
                return FALSE;
            }
        }
    }

    /* have not found any illegal channel so all actiuve channel are legal */
    sme_trace_entry((TR_REGDOM, "<< are_all_active_chans_legal_under_specifiedRegDom(TRUE)"));
    return TRUE;
}


/**
 * @brief  Checks if active channel is empty
 *
 * @par Description
 *
 * @param[in]
 *      regdata       : RegDomData (where Regulatory Domain data is stored)
 *
 * @return
 *      outcome       : CsrBool (TRUE=empty; FALSE= Not Empty)
 */
static CsrBool is_active_list_empty( RegDomData *regdata )
{
    CsrUint8 i;

    sme_trace_entry((TR_REGDOM, ">> is_active_list_empty()"));

    for (i=0 ; i < HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
    {
        if (regdata->ChannelsInUseList.Dott11_b_g_ChanList[i].chan_scan_mode == channelScanMode_active)
        {
            /* Found one so list is not empty (don't need to know more) */
            sme_trace_entry((TR_REGDOM, "<< is_active_list_empty(FALSE)"));
            return FALSE;
        }
    }

    /* Could not Find one so list is empty */
    sme_trace_entry((TR_REGDOM, "<< is_active_list_empty(TRUE)"));
    return TRUE;
}


/**
 * @brief Obtains Country Info IE and  assesses its validity
 *
 * @par Description
 * Obtains Country Info IE and  assesses its validity (ID, minimum Length)
 * also:
 * - Stores relevant information in a easily accessible format
 * - Calculates the number of triplets included.
 *
 * Note: We are Lenient we do not reject IE
 *
 * @param[in] regdata       : RegDomData (where Regulatory Domain data is stored)
 * @param[in] element       : CsrUint8 (where IE is initially stored)
 * @param[out] result       : COUNTRY_INFO_STRUCT (stored information)
 * @param[out] numTriplets  : CsrUint8 (calculated number of Triplet received)
 *
 * @return
 *        ie_result :
 *          - ie_success   :  Correct IE has been received
 *          - ie_not_found :  IE has not been found (IE-Id does not match)
 *          - ie_invalid   :  this code is not returned by this function at the moment
 *          - ie_error     :  IE has illegal length or num of triplets inconsitent with length
 */
static ie_result ie_getCountryInfoIE(RegDomData *regdata,
                                     const CsrUint8* element,
                                     COUNTRY_INFO_STRUCT **result,
                                     CsrUint8 *numTriplets)
{
    CsrUint8 ie_length, i;
    CsrUint8 num_subband_triplets;
    ie_result ieCode;

    sme_trace_entry((TR_REGDOM, ">> ie_getCountryInfoIE()"));

    require(TR_REGDOM, element != NULL);

    *result = (COUNTRY_INFO_STRUCT *)NULL;
    if (ie_id(element) == IE_ID_COUNTRY)
    {
        ie_length = ie_len(element);
        if (ie_length < IE_MIN_LEN_COUNTRY)
        {
            sme_trace_debug((TR_REGDOM,
                            "ie_getCountryInfoIE(): Length of Country Information IE is: %d, Below Minimum length",
                            ie_length));
            *numTriplets = 0;
            ieCode = ie_error;
        }
        else
        {
            num_subband_triplets = (ie_length - COUNTRY_STRING_LENGTH) / 3;
            if (num_subband_triplets < 1)
            {
                sme_trace_debug((TR_REGDOM,
                               "ie_getCountryInfoIE(): Num of Triplets in Country Information IE is: %d, Incorrect",
                               num_subband_triplets));
                *numTriplets = 0;
                ieCode = ie_error;
            }
            else
            {
                *numTriplets = num_subband_triplets;
                sme_trace_info((TR_REGDOM, "ie_getCountryInfoIE(): Num Sub-Band Triplets Received: %d",
                              num_subband_triplets));

                *result = (COUNTRY_INFO_STRUCT *)CsrPmalloc(sizeof(COUNTRY_INFO_STRUCT));

                /* IE Id and Length */
                (*result)->ieId = IE_ID_COUNTRY;
                (*result)->ieLen = ie_length;

                /* Extract received  Country String */
                CsrMemCpy((*result)->countryString, element+2, COUNTRY_STRING_LENGTH);

                /* Convert Country String to Uppercase */
                (*result)->countryString[0] = to_upper_case((*result)->countryString[0]);
                (*result)->countryString[1] = to_upper_case((*result)->countryString[1]);
                (*result)->countryString[2] = to_upper_case((*result)->countryString[2]);

                /* Extract received triplets */
                for (i=0 ; i<num_subband_triplets ; i++)
                {
                    (*result)->subBand[i].first_channel = *(element + 2 + COUNTRY_STRING_LENGTH + i* SUB_BAND_TRIPLET_LENGTH);
                    (*result)->subBand[i].num_channels = *(element + 2 + COUNTRY_STRING_LENGTH + i* SUB_BAND_TRIPLET_LENGTH +1);
                    (*result)->subBand[i].legal_power_dBm = (CsrInt8)*(element + 2 + COUNTRY_STRING_LENGTH + i* SUB_BAND_TRIPLET_LENGTH +2);
                }

                /* Zero remaining triplets (nothing received for them) */
                for (i=num_subband_triplets ; i< MAX_NUM_SUB_BAND_TRIPLETS ; i++)
                {
                    (*result)->subBand[i].first_channel = 0;
                    (*result)->subBand[i].num_channels = 0;
                    (*result)->subBand[i].legal_power_dBm = CHANNEL_INVALID_POWER;
                }
                ieCode = ie_success;
            }
        }
    }
    else
    {
        *numTriplets = 0;
        ieCode = ie_not_found;
    }

    sme_trace_entry((TR_REGDOM, "<< ie_getCountryInfoIE()"));

    return ieCode;
}

/**
 * @brief Refreshes the expiry timers on active legal channels
 *
 * @par Description
 * This function refreshes the expiry time for all channels that are marked
 * as active and are legal under the current regulatory domain.
 *
 * Note: Illegal channel will not be refreshed until they expire.
 *
 * @param[in] regdata : RegDomData (where Regulatory Domain data is stored)
 *
 * @return
 *       None.
 */
static void refresh_timer_on_active_legal_chans(FsmContext* context, RegDomData *regdata)
{
    RegulatoryDomainInfoStruct *RegDomStruct;
    CsrUint8 i, chnlNum;
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    /* Access structure containig info about this regulatory domain */
    RegDomStruct = get_ptr_to_regulatory_domain_info( regdata->CurrentLocation.regDomain );

    sme_trace_entry((TR_REGDOM, ">> refresh_timer_on_active_legal_chans()"));

    sme_trace_debug((TR_REGDOM, "refresh_timer_on_active_legal_chans(): Current Time (ms) = %lu", now));

    for (i=0 ; i < HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
    {
        Channel_Info * chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[i];

        chnlNum = map_position_to_channel(i);
        if ( REGULATORY_CHANNEL_LEGAL(RegDomStruct->chan_legality, chnlNum)  &&
             (chnlInfo->expiryFlag) &&
             (chnlInfo->chan_scan_mode==channelScanMode_active) )
        {
            chnlInfo->expiryTime = now + ACTIVE_CHANNEL_VALIDITY_PERIOD;
        }
    }

    sme_trace_entry((TR_REGDOM, "<< refresh_timer_on_active_legal_chans()"));
}

/**
 * @brief set the Default power regime on all active channels
 *
 * @par Description
 * Set the power regime (EIRP/TPO) of Default regulatory domain on all active channels
 *
 * @param[in] regdata        : RegDomData (where Regulatory Domain data is stored)
 *
 * @return
 *       None.
 */
static void set_default_pwr_regime_on_legal_chans(RegDomData *regdata )
{
    CsrUint8 i, chnlNum;
    POWER_REGIME default_pwr_regime;
    RegulatoryDomainInfoStruct * pDefRegDomstruct;

    sme_trace_entry((TR_REGDOM, ">> set_default_pwr_regime_on_legal_chans()"));

    default_pwr_regime = regdata->DefaultLocation.locationPwrRegime;

    pDefRegDomstruct = get_ptr_to_regulatory_domain_info(regdata->DefaultLocation.regDomain);

    for (i=0 ; i < HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
    {
        Channel_Info * chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[i];

        chnlNum = map_position_to_channel(i);

        if ( REGULATORY_CHANNEL_LEGAL(pDefRegDomstruct->chan_legality, chnlNum) )
        {
            if ( chnlInfo->chan_pwr_regime != default_pwr_regime )
            {
                if ( default_pwr_regime == TPO_PWR_REGIME )
                {
                    chnlInfo->chan_tx_power = convert_to_tpo( regdata, chnlInfo->chan_tx_power );
                }
                else
                {
                    chnlInfo->chan_tx_power = convert_to_eirp( regdata, chnlInfo->chan_tx_power );
                }
                chnlInfo->chan_pwr_regime = default_pwr_regime;
            }

            /* Extra Channel in Default Regulatory Domain but not in the one before the expiry
             * will have a power of -127 so need to get the correct value
             */
            if ( chnlInfo->chan_tx_power == CHANNEL_INVALID_POWER )
            {
                /* All power values are stored currently (under each regulatory domain) as EIRP thus only convert if require TPO */
                chnlInfo->chan_tx_power = pDefRegDomstruct->chan_power[chnlNum-1];
                if ( default_pwr_regime == TPO_PWR_REGIME )
                {
                    chnlInfo->chan_tx_power = convert_to_tpo( regdata, chnlInfo->chan_tx_power );
                }
            }
        }
        else
        {
            chnlInfo->chan_pwr_regime = UNKNOWN_PWR_REGIME;
            chnlInfo->chan_tx_power = CHANNEL_INVALID_POWER;
        }
    }

    sme_trace_entry((TR_REGDOM, "<< set_default_pwr_regime_on_legal_chans()"));
}

/**
 * @brief Configures channels lists based on some regulatory domain
 *
 * @par Description
 * This function configures the specified channels (arg1) based upon the
 * specified regulatory domain (arg3). It must be called whenever a country
 * IE is received in a beacon that differs from the 'current' regulatory
 * domain. When called in this mode, channels currently unavailable to
 * active scanning will be made actively scannable. Channels that are
 * currently actively scannable will remain actively scannable (to prevent
 * 'thrashing').
 *
 * Whenever information that made a given channel actively scannable expires
 * (ages), this function must be called and presented with a default regulatory
 * domain; the regDomIsMibDefault argument must be set to true. This may force
 * the specifie channel into a non-actively scannable mode.
 *
 * @param[in] channel : specific channel to be configured or 0 to configure all channels
 * @param[in] cfg     : SmeConfigData (for Trust Level)
 * @param[in] regdata : RegDomData (where Regulatory Domain data is stored)
 * @param[in] reg_dom : unifi_RegulatoryDomain (specific regulatory domain we are in)
 * @param[in] regDomIsMibDefault: CsrBool (True if Current Regulatory Domain is the MIB default)
 *
 * @return
 *       None.
 */
static void config_channel_for_reg_domain(FsmContext* context,
                                          CsrUint8 channel,
                                          SmeConfigData* cfg,
                                          RegDomData *regdata,
                                          unifi_RegulatoryDomain reg_dom,
                                          CsrBool regDomIsMibDefault)
{
    RegulatoryDomainInfoStruct *currentRegDom;
    CsrUint8 i, chnlNum;
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    sme_trace_entry((TR_REGDOM, ">> config_channel_for_reg_domain()"));

    /* Use Enum value to access structure containig info about this regulatory domain */
    currentRegDom = get_ptr_to_regulatory_domain_info(reg_dom);
    sme_trace_debug((TR_REGDOM, "config_channel_for_reg_domain(): Current domain is %s %s",
                     currentRegDom->RegDomainStr, regDomIsMibDefault?"[FALLBACK TO DEFAULT]":""));

    /* Extract Regulatory Domain Info from (pointed-to) structure */
    for (i=0 ; i < HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
    {
        sme_trace_debug_code(POWER_REGIME previousChanPwrRegime;)
        Channel_Info * chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[i];

        chnlNum = map_position_to_channel(i);

        /*
         * If we've been asked to configure just a single channel - make sure
         * we process only the channel specified, skip all others.
         */
        if (channel && (channel != chnlNum))
        {
            continue;
        }

        /*
         * If the current domain claims the current channel is NOT legal, we may
         * have to prevent active scanning on it...
         */
        if (!REGULATORY_CHANNEL_LEGAL(currentRegDom->chan_legality, chnlNum)
            || (regDomIsMibDefault && cfg->smeConfig.trustLevel < unifi_TrustMIB))
        {
            if (!REGULATORY_CHANNEL_LEGAL(currentRegDom->chan_legality, chnlNum))
            {
                sme_trace_debug((TR_REGDOM, "config_channel_for_reg_domain(): channel %d is marked as ILLEGAL", chnlNum));

                /* If Channel is illegal and passive cannot have power Value and Regime */
                if (chnlInfo->chan_scan_mode == channelScanMode_passive)
                {
                    chnlInfo->chan_pwr_regime = UNKNOWN_PWR_REGIME;
                    chnlInfo->chan_tx_power = CHANNEL_INVALID_POWER ;
                }

                /*
                 * If we're not installing a default regulatory domain and are
                 * reconfiguring all channels then this is due to us receiving
                 * a new country IE - we ignore channels marked as illegal in
                 * the new domain to prevent thrashing. Instead, aging of the
                 * information makes channels 'legal' - this takes care of things.
                 *
                 * Except that now we need to be careful if TPO or EIRP should be used
                 * and convert power value if necessary...
                 */
                if (channel == 0 && !regDomIsMibDefault)
                {
                    sme_trace_debug((TR_REGDOM,
                    "config_channel_for_reg_domain(): configuring for new reg-dom, DO NOT disable enabled channels"));

                    /* SET power regime of Channel and CORRECT power value if necessary */
                    sme_trace_debug_code(previousChanPwrRegime = chnlInfo->chan_pwr_regime;)
                    if ( (regdata->CurrentLocation.locationPwrRegime==TPO_PWR_REGIME) && (chnlInfo->chan_tx_power != CHANNEL_INVALID_POWER) )
                    {
                        if (chnlInfo->chan_pwr_regime== EIRP_PWR_REGIME)
                        {
                            chnlInfo->chan_tx_power = convert_to_tpo( regdata, chnlInfo->chan_tx_power );
                        }
                        chnlInfo->chan_pwr_regime = TPO_PWR_REGIME;
                    }
                    else if ( (regdata->CurrentLocation.locationPwrRegime==EIRP_PWR_REGIME ) && ( chnlInfo->chan_tx_power != CHANNEL_INVALID_POWER) )
                    {
                        if (chnlInfo->chan_pwr_regime== TPO_PWR_REGIME)
                        {
                            chnlInfo->chan_tx_power = convert_to_eirp( regdata, chnlInfo->chan_tx_power );
                        }
                        chnlInfo->chan_pwr_regime = EIRP_PWR_REGIME;
                    }
                    else
                    {
                        chnlInfo->chan_pwr_regime = UNKNOWN_PWR_REGIME;
                    }

                    sme_trace_debug(( TR_REGDOM, "Power Level for Channel#%d is %d dBm (%s->%s)",
                                      chnlNum,
                                      chnlInfo->chan_tx_power,
                                      ((previousChanPwrRegime==EIRP_PWR_REGIME) ? "EIRP" : ((previousChanPwrRegime==TPO_PWR_REGIME) ? "TPO" : "UNKNOWN")),
                                      ((chnlInfo->chan_pwr_regime==EIRP_PWR_REGIME) ? "EIRP" : ((chnlInfo->chan_pwr_regime==TPO_PWR_REGIME) ? "TPO" : "UNKNOWN")) ));

                    continue;
                }
            }
            else
            {
                sme_trace_debug((TR_REGDOM, "config_channel_for_reg_domain(): channel %d is marked as LEGAL by MIB, but cannot trust", chnlNum));
            }
            sme_trace_debug((TR_REGDOM, "config_channel_for_reg_domain: channel %d is currently in mode %s",
                             chnlNum, scan_mode_to_string(chnlInfo->chan_scan_mode)));

            /*
             * If the channel is marked for active scan then fall back to the
             * original default
             */
            if (chnlInfo->chan_scan_mode == channelScanMode_active)
            {
                RegulatoryDomainInfoStruct * defaultRegDom;

                defaultRegDom = get_ptr_to_regulatory_domain_info(regdata->DefaultLocation.regDomain);

                if (REGULATORY_CHANNEL_LEGAL(defaultRegDom->chan_legality, chnlNum))
                {
                    sme_trace_debug((TR_REGDOM,
                                     "config_channel_for_reg_domain(): channel was legal in default domain - go passive"));
                    chnlInfo->chan_scan_mode = channelScanMode_passive;
                    chnlInfo->chan_tx_power = defaultRegDom->chan_power[chnlNum-1];
                }
                else
                {
                    sme_trace_debug((TR_REGDOM,
                                     "config_channel_for_reg_domain(): channel was illegal in default domain - can go passive"));
                    chnlInfo->chan_scan_mode = channelScanMode_passive;
                    chnlInfo->chan_tx_power = currentRegDom->chan_power[chnlNum-1];
                }

                chnlInfo->chan_pwr_regime = EIRP_PWR_REGIME; /* since all RegDoms store currently power values as EIRP
                                                              * This will be picked up below and corrected,
                                                              * (otherwise it will be missed hence error)
                                                              */
                regdata->ChannelsInUseList.listChangedFlag = TRUE;
                chnlInfo->expiryFlag                       = FALSE; /* MIB Data never expires */
            }
        }
        else        /* Channel is deemed legal */
        {
            sme_trace_debug((TR_REGDOM, "config_channel_for_reg_domain(): channel %d is marked as legal", chnlNum));

            /*
             * According to the current domain, this channel can be actively
             * scanned; however, we need to check to make sure
             */
            if (regDomIsMibDefault && isChannelScanForcedPassive(cfg, chnlNum))
            {
                sme_trace_debug((TR_REGDOM, "force initial passive scan on channel %d (MIB override)", chnlNum));

                chnlInfo->chan_scan_mode    = channelScanMode_passive;
                chnlInfo->expiryFlag        = TRUE;
                chnlInfo->chan_tx_power = currentRegDom->chan_power[chnlNum-1];
                chnlInfo->chan_pwr_regime = EIRP_PWR_REGIME; /* since all RegDoms store currently power values as EIRP
                                                              * This will be picked up below and corrected,
                                                              * (otherwise it will be missed hence error)
                                                              */
            }
            else if (chnlInfo->chan_scan_mode == channelScanMode_passive)
            {
                /*
                 * If in strict mode we will not allow active scanning on a
                 * channel just because it maps to a domain where the channel
                 * is deemed legal. Instead, we will mark it for passive
                 * scanning. In strict mode, the only channels that will
                 * be enabled for active scanning will be those SPECIFICALLY
                 * listed in the received country IEs
                 */
                /* if ( (cfg->smeConfig.trustLevel != unifi_TrustStrict) && (cfg->smeConfig.trustLevel != unifi_TrustAdjunct) )  */
                if (cfg->smeConfig.trustLevel != unifi_TrustStrict)
                {
                    sme_trace_debug((TR_REGDOM, "Channel %d marked for active scanning trstLvl=%d", chnlNum, cfg->smeConfig.trustLevel));
                    chnlInfo->chan_scan_mode                   = channelScanMode_active;
                    chnlInfo->chan_tx_power                = currentRegDom->chan_power[chnlNum-1];
                    chnlInfo->chan_pwr_regime = EIRP_PWR_REGIME; /* since all RegDoms store currently power values as EIRP
                                                                  * This will be picked up below and corrected,
                                                                  * (otherwise it will be missed hence error)
                                                                  */
                    regdata->ChannelsInUseList.listChangedFlag = TRUE;
                }

                if (regDomIsMibDefault)
                {
                    chnlInfo->expiryFlag                       = FALSE;
                }
                else
                {
                    chnlInfo->expiryFlag                       = TRUE;
                }
            }
            else if (chnlInfo->chan_scan_mode == channelScanMode_active)
            {
                /*
                sme_trace_debug((TR_REGDOM, "Current POWER REGIME for Channel %d is : %s",
                                 chnlNum,
                               ((chnlInfo->chan_pwr_regime==EIRP_PWR_REGIME) ? "EIRP" : ((chnlInfo->chan_pwr_regime==TPO_PWR_REGIME) ? "TPO" : "UNKNOWN")) ));
                */
            }

            /* Set Expiry Time */
            if ((chnlInfo->expiryFlag) && (chnlInfo->chan_scan_mode== channelScanMode_active))
            {
                sme_trace_debug((TR_REGDOM, "Channel %d marked for expiry enabled", chnlNum));
                chnlInfo->expiryTime = now + ACTIVE_CHANNEL_VALIDITY_PERIOD;
            }

        }

        /* SET power regime of Channel and CORRECT power value if necessary */
        sme_trace_debug_code(previousChanPwrRegime = chnlInfo->chan_pwr_regime;)
        if ( (regdata->CurrentLocation.locationPwrRegime==TPO_PWR_REGIME) && (chnlInfo->chan_tx_power != CHANNEL_INVALID_POWER) )
        {
            if (chnlInfo->chan_pwr_regime== EIRP_PWR_REGIME)
            {
                chnlInfo->chan_tx_power = convert_to_tpo( regdata, chnlInfo->chan_tx_power );
            }
            chnlInfo->chan_pwr_regime = TPO_PWR_REGIME;
        }
        else if ( (regdata->CurrentLocation.locationPwrRegime==EIRP_PWR_REGIME ) && ( chnlInfo->chan_tx_power != CHANNEL_INVALID_POWER) )
        {
            if (chnlInfo->chan_pwr_regime== TPO_PWR_REGIME)
            {
                chnlInfo->chan_tx_power = convert_to_eirp( regdata, chnlInfo->chan_tx_power );
            }
            chnlInfo->chan_pwr_regime = EIRP_PWR_REGIME;
        }
        else
        {
            chnlInfo->chan_pwr_regime = UNKNOWN_PWR_REGIME;
        }

        sme_trace_debug(( TR_REGDOM, "Power Level for Channel#%d is %d dBm (%s->%s)",
                          chnlNum,
                          chnlInfo->chan_tx_power,
                          ((previousChanPwrRegime==EIRP_PWR_REGIME) ? "EIRP" : ((previousChanPwrRegime==TPO_PWR_REGIME) ? "TPO" : "UNKNOWN")),
                          ((chnlInfo->chan_pwr_regime==EIRP_PWR_REGIME) ? "EIRP" : ((chnlInfo->chan_pwr_regime==TPO_PWR_REGIME) ? "TPO" : "UNKNOWN")) ));

    }

    sme_trace_entry((TR_REGDOM, "<< config_channel_for_reg_domain()"));
}

/**
 * @brief Creates a country IE from the contents of the active list.
 *
 * @par Description
 * This function creates a country IE that is based on the current contents
 * of the active list (i.e. channels that may be legally transmitted on).
 *
 * The function allocates the memory for the IE so the calling context must
 * release this memory via CsrPfree when it has finished using it.
 *
 * @param[out] ieBuf   : double pointer to the buffer - alloc'd data returned via this
 * @param[out] ieBufL  : pointer to integer - length of buf returned via this
 *
 * @return
 *       Pointer to allocated memory (i.e. value of *ieBuf)
 */
static CsrUint8 * create_country_ie_from_active_list(FsmContext * context, CsrUint8 ** ieBuf, CsrUint16 * ieLength)
{
    CsrUint8          i, connChan;
    CsrInt8           initialPowerOfChanConnect=CHANNEL_INVALID_POWER;
    ns_ConnectionStatus   connectionStatus = ns_get_connection_status(context);
    RegDomData   * regulatoryData = get_regulatory_data(context);
    CsrBool        lastChanActive = FALSE;
    CsrInt8           lastPower      = 0, connChanPower=CHANNEL_INVALID_POWER;
    CsrUint8          startChan      = 0;
    CsrUint8        * ieAssyPtr;
    SmeConfigData* cfg            = get_sme_config(context);
    Channel_Info * actChnls;

    sme_trace_entry((TR_REGDOM,  ">> create_country_ie_from_active_list()"));

    /*
     * NB: allocated memory for the country IE as follows:
     *            2 bytes for the IE type and length
     *            3 bytes for the country string
     *       2 * 14 bytes for the triplets (maximum of 14)
     *            1 byte for padding (total IE length must be an even # of bytes
     */
    *ieLength  = 0;
    *ieBuf     = NULL;

    if (cfg->smeConfig.trustLevel == unifi_TrustDisabled)
    {
        sme_trace_entry((TR_REGDOM,  "<< create_country_ie_from_active_list()"));
        return NULL;
    }

    /* if Connected (to an AP) we need to use power level (on the connection channel) seen on the Country IE from this AP */
    if (connectionStatus == ns_ConnectionStatus_Connected)
    {
        CsrUint8 chnindex;

        connChan = cfg->connectionInfo.channelNumber ;
        connChanPower = get_channel_power_from_stored_cntry_ie( regulatoryData, connChan);
        sme_trace_debug((TR_REGDOM,  "create_country_ie_from_active_list(): Connection Channel= %d, Power(from CtryIE)= %d",
                         connChan, connChanPower ));

        /* Save existing power value and replace by value from Country IE */
        if ( connChanPower != CHANNEL_INVALID_POWER )
        {
            chnindex=map_channel_to_position(connChan);
            initialPowerOfChanConnect = regulatoryData->ChannelsInUseList.Dott11_b_g_ChanList[chnindex].chan_tx_power;
            regulatoryData->ChannelsInUseList.Dott11_b_g_ChanList[chnindex].chan_tx_power = connChanPower;
        }
    }

    ieAssyPtr  = CsrPmalloc(5 + (HIGHEST_80211_b_g_CHANNEL_NUM * 3));
    *ieBuf     = ieAssyPtr;
    ieAssyPtr += 5;     /* skip over the country string portion of the IE */
    actChnls   = regulatoryData->ChannelsInUseList.Dott11_b_g_ChanList;

    for (i = 1 ; i <= HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
    {
        Channel_Info * chnlInfo;
        CsrUint8          chnlIdx;

        chnlIdx   = map_channel_to_position(i);
        chnlInfo = &actChnls[chnlIdx];

        sme_trace_debug((TR_REGDOM,
                         "create_country_ie_from_active_list: examining channel %d: %slegal; power %d dBm",
                         i, (chnlInfo->chan_scan_mode == channelScanMode_active)?"":"NOT ",
                         chnlInfo->chan_tx_power));
        if (chnlInfo->chan_scan_mode == channelScanMode_active)
        {
            if (lastChanActive == FALSE || lastPower != chnlInfo->chan_tx_power)
            {
                if (lastChanActive)
                {
                    sme_trace_debug
                    ((TR_REGDOM,
                      "create_country_ie_from_active_list: Power level change detected between channels %d and %d (%d -> %d)",
                      i - 1, i, lastPower, chnlInfo->chan_tx_power
                    ));

                    /* end last triplet, start another */
                    *ieAssyPtr++ = startChan;
                    *ieAssyPtr++ = i - startChan;
                    *ieAssyPtr++ = (CsrUint8)lastPower;

                    startChan = i;
                    lastPower = chnlInfo->chan_tx_power;
                }
                else
                {
                    sme_trace_debug
                    ((TR_REGDOM,
                      "create_country_ie_from_active_list: Legality change: %d not legal, %d legal",
                      i - 1, i
                    ));

                    /* start of new triplet */
                    lastChanActive = TRUE;
                    lastPower      = chnlInfo->chan_tx_power;
                    startChan      = i;
                }
            }
            else
            {
                continue;
            }
        }
        else if (lastChanActive == TRUE)
        {
            sme_trace_debug
            ((TR_REGDOM,
              "create_country_ie_from_active_list: Legality change: %d legal, %d not legal",
              i - 1, i
            ));

            /* end of triplet - create it now*/
            *ieAssyPtr++  = startChan;
            *ieAssyPtr++  = i - startChan;
            *ieAssyPtr++  = (CsrUint8)lastPower;

            lastChanActive = FALSE;
        }
    }

    /* end of chan list reached and still 'active' - create triplet */
    if (lastChanActive)
    {
        *ieAssyPtr++  = startChan;
        *ieAssyPtr++  = i - startChan;
        *ieAssyPtr++  = (CsrUint8)lastPower;
    }
    *ieLength = (CsrUint16)(ieAssyPtr - *ieBuf);

    /* Restore Initial power value */
    if ( connectionStatus == ns_ConnectionStatus_Connected )
    {
        if ( connChanPower != CHANNEL_INVALID_POWER )
        {
            CsrUint8 chnindex;
            connChan = cfg->connectionInfo.channelNumber ;
            chnindex=map_channel_to_position(connChan);
            regulatoryData->ChannelsInUseList.Dott11_b_g_ChanList[chnindex].chan_tx_power = initialPowerOfChanConnect;
        }
    }

    if (*ieLength == 5)
    {
        *ieLength = 0;
        CsrPfree(*ieBuf);

        sme_trace_debug((TR_REGDOM,
                         "create_country_ie_from_active_list: unable to create country IE - presumably active list empty"));
        sme_trace_entry((TR_REGDOM,  "<< create_country_ie_from_active_list()"));
        return NULL;
    }

    if (*ieLength % 2)
    {
        sme_trace_debug((TR_REGDOM, "Adding a padding byte - length goes from %d to %d", *ieLength, *ieLength + 1));
        (*ieBuf)[*ieLength] = 0;
        (*ieLength) += 1;
    }

    /* Insert header for IE */
    (*ieBuf)[0] = IE_ID_COUNTRY;
    (*ieBuf)[1] = (CsrUint8)(*ieLength - 2);

    /* Country String */
    if ( regulatoryData->CurrentLocation.locStatus == Location_Valid )
    {
        /*  We have a country: Use currently stored country string (which may or may not be XXX) */
        (*ieBuf)[2] = (CsrUint8)regulatoryData->CurrentLocation.countryString[0] ;
        (*ieBuf)[3] = (CsrUint8)regulatoryData->CurrentLocation.countryString[1] ;
        (*ieBuf)[4] = (CsrUint8)regulatoryData->CurrentLocation.countryString[2] ;
    }
    else
    {
        /* We do not have a country: Use XXX Unless MIB in which case use default country string */
        if ( cfg->smeConfig.trustLevel == unifi_TrustMIB )
        {
            (*ieBuf)[2] = (CsrUint8)regulatoryData->DefaultLocation.countryString[0] ;
            (*ieBuf)[3] = (CsrUint8)regulatoryData->DefaultLocation.countryString[1] ;
            (*ieBuf)[4] = (CsrUint8)regulatoryData->DefaultLocation.countryString[2] ;
        }
        else
        {
            (*ieBuf)[2] = (*ieBuf)[3] = (*ieBuf)[4] = 'X';
        }
    }

    sme_trace_hex((TR_REGDOM, TR_LVL_INFO, "create_country_ie_from_active_list: generated IE", *ieBuf, *ieLength));
    sme_trace_entry((TR_REGDOM,  "<< create_country_ie_from_active_list()"));

    return *ieBuf;
}

/**
 * @brief Converts a channel number to an index into the channel list
 *
 * @par Description
 * Given an 802.11b/g channel (1-14), this function returns its order/index in
 * the regulatory domain channel list (this is ordered by 'priority' rather than
 * by any numerical ascending/descending order).
 *
 * @param[in]  channel : The channel number
 *
 * @return
 *       Index of the channel in the in-use channel list
 */
static CsrUint8 map_channel_to_position(CsrUint8 channel)
{
    CsrUint8 pos;

    for (pos=0 ; pos<HIGHEST_80211_b_g_CHANNEL_NUM ; pos++)
    {
        if (channel == channelScanOrderList[pos])
        {
            return pos;
        }
    }

    /* only reaches here if problems */
    sme_trace_error((TR_REGDOM,
                     "map_channel_to_position(): couldn't find position for given channel %d",
                     channel));

    verify(TR_REGDOM, pos < HIGHEST_80211_b_g_CHANNEL_NUM);
    return 0x0;
}

/**
 * @brief Converts an index in the channel list to a channel number
 *
 * @par Description
 * Given a position in the regulatory in-use channel list, this function returns
 * the corresponding 802.11b/g channel number (1-14)
 *
 * @param[in]  pos     : The index of interest
 *
 * @return
 *       The channel number associated with the supplied list index
 */
static CsrUint8 map_position_to_channel(CsrUint8 pos)
{
    if (pos < HIGHEST_80211_b_g_CHANNEL_NUM)
    {
        return channelScanOrderList[pos];
    }

    sme_trace_error((TR_REGDOM,
                     "map_position_to_channel(): out-of-bounds position specified %d", pos));
    verify(TR_REGDOM, pos < HIGHEST_80211_b_g_CHANNEL_NUM);
    return 1;
}

/**
 * @brief Converts a character to its upper case equivalent
 *
 * @par Description
 * See brief
 *
 * @param[in]  ch      : The character to convert to upper case
 *
 * @return
 *       The upper case equivalent of the input
 */
static CsrUint8 to_upper_case(CsrUint8 ch)
{
    if ((CsrUint16) (ch - 'a') < 26u)
        ch += (CsrUint8)('A' - 'a');
    return ch;
}

/**
 * @brief Converts from EIRP to  TPO
 *
 * @par Description
 * See brief
 *
 * @param[in]  regdata  : RegDomData (where Regulatory Domain data is stored)
 * @param[in]  eirp_pwr : The input eirp to convert to tpo
 *
 * @return
 *       The tpo value
 */
static CsrInt8  convert_to_tpo( RegDomData *regdata,
                             CsrInt8 eirp_pwr )
{
    if ( eirp_pwr == CHANNEL_INVALID_POWER )
        return CHANNEL_INVALID_POWER;
    else return ( eirp_pwr - regdata->AntennaGain );
}

/**
 * @brief Converts from TPO to EIRP
 *
 * @par Description
 * See brief
 *
 * @param[in]  regdata  : RegDomData (where Regulatory Domain data is stored)
 * @param[in]  eirp_pwr : The input tpo to convert to eirp
 *
 * @return
 *       The eirp value
 */
static CsrInt8  convert_to_eirp( RegDomData *regdata,
                              CsrInt8 tpo_pwr )
{
    if ( tpo_pwr == CHANNEL_INVALID_POWER )
        return CHANNEL_INVALID_POWER;
    else return ( tpo_pwr + regdata->AntennaGain );
}

/**
 * @brief Converts a scanning mode to a descriptive scan
 *
 * @par Description
 * See brief
 *
 * @param[in]  mode    : The scanning mode
 *
 * @return
 *       Descriptive string ("active", or "passive")
 */
sme_trace_debug_code
(
static const char * scan_mode_to_string(ChannelScanMode mode)
{
    switch (mode)
    {
        case channelScanMode_active:
            return "active";

        case channelScanMode_passive:
            return "passive";

        default:
            break;
    }

    return "ILLEGAL MODE!";
}
)

/**
 * @brief Returns Power of specified Channel from stored Country IE
 *
 * @par Description
 * See brief
 *
 * @param[in] regdata : RegDomData (where Regulatory Domain data is stored)
 * @param[in] chnl    : CsrUint8 (specified channel)
 *
 * @return
 *       required power for channel: CsrInt8
 */
static CsrInt8 get_channel_power_from_stored_cntry_ie(RegDomData* regdata, CsrUint8 chnl )
{
    CsrUint8 pastCtryStngOfsset, nTplet, i;

    sme_trace_entry((TR_REGDOM, ">> get_channel_power_from_stored_cntry_ie()"));

    /* Check if we have a valid Country IE */
    if ( (regdata->countryIeFromConnection[0] != IE_ID_COUNTRY) || (regdata->countryIeFromConnection[1]< IE_MIN_LEN_COUNTRY) )
    {
        sme_trace_debug((TR_REGDOM, "No Country IE has been stored for this connection" ));
        sme_trace_debug((TR_REGDOM, "The value of Country IE ID is: %d", regdata->countryIeFromConnection[0] ));
        sme_trace_debug((TR_REGDOM, "The value of Country IE Length: %d", regdata->countryIeFromConnection[1] ));

        sme_trace_entry((TR_REGDOM, "<< get_channel_power_from_stored_cntry_ie()"));
        return CHANNEL_INVALID_POWER;
    }

    /* How many Triplets are there */
    nTplet = ( ((CsrUint8) regdata->countryIeFromConnection[1]) - COUNTRY_STRING_LENGTH) / SUB_BAND_TRIPLET_LENGTH;
    if ( (nTplet<1) || (nTplet>MAX_NUM_SUB_BAND_TRIPLETS) )
    {
        sme_trace_debug((TR_REGDOM, "Invalid Number of Triplets: %d", nTplet ));
        sme_trace_entry((TR_REGDOM, "<< get_channel_power_from_stored_cntry_ie()"));
        return CHANNEL_INVALID_POWER;
    }

    pastCtryStngOfsset = 2 + COUNTRY_STRING_LENGTH; /* 2 bytes in front for IeId and IeLength */

    /* Find triplet which specifies power for channel */
    for (i=0 ; i<nTplet ; i++)
    {
        CsrUint8 startChan=0;
        CsrUint8 numChans=0;

        startChan = (CsrUint8) regdata->countryIeFromConnection[pastCtryStngOfsset + i*SUB_BAND_TRIPLET_LENGTH];
        numChans =  (CsrUint8) regdata->countryIeFromConnection[pastCtryStngOfsset + i*SUB_BAND_TRIPLET_LENGTH +1];

        /* Check if Triplet is correct first */
        if ( ((startChan>0) && (startChan<=HIGHEST_80211_b_g_CHANNEL_NUM))
          && ((numChans>0) && (numChans<=HIGHEST_80211_b_g_CHANNEL_NUM)) )
        {
            if ( (startChan<=chnl) &&  (chnl<startChan+numChans) )
            {
               /* found it (return corresponding power) */
               sme_trace_entry((TR_REGDOM, "<< get_channel_power_from_stored_cntry_ie()"));
               return (regdata->countryIeFromConnection[pastCtryStngOfsset + i*SUB_BAND_TRIPLET_LENGTH +2]);
            }
        }
    }

    /* could find it */
    sme_trace_entry((TR_REGDOM, "<< get_channel_power_from_stored_cntry_ie()"));
    return CHANNEL_INVALID_POWER;
}

/* PUBLIC FUNCTION DEFINITIONS ***********************************************/

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 */
/*---------------------------------------------------------------------------*/
void regdom_init(FsmContext* context, RegDomData *regdata)
{
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_REGDOM, ">> regdom_init()"));

    /* At this stage we do not know where we are so Invalidate current Country */
    invalidate_current_location(regdata);

    /* Initialise Default Location (either Firmware value or failing that we use FCC */
    init_default_country_using_mib_regulatory_domain(context, cfg, regdata);

    sme_trace_info((TR_REGDOM,
                    "regdom_init(): Configured Trust Level = %d",
                    cfg->smeConfig.trustLevel));

    /* RegDomain is Not Implemented, OR is Implemented but disabled */
    /* We only allow TrustLevelDisabled hence change if any other value */
    if (cfg->regdomReadFromMib)
    {
        sme_trace_debug((TR_REGDOM,
                         "regdom_init(): MIB Value For dot11MultiDomainCapabilityImplemented = %d",
                         cfg->regDomInfo.dot11MultiDomainCapabilityImplemented));

        sme_trace_debug((TR_REGDOM,
                         "regdom_init(): MIB Value For dot11MultiDomainCapabilityEnabled = %d",
                         cfg->regDomInfo.dot11MultiDomainCapabilityEnabled));

        /*
           **** Cannot read dot11CountryString from MIBs at the moment !
           sme_trace_debug((TR_REGDOM,
                            "regdom_init(): MIB Value For dot11CountryString = '%s'",
                            cfg->dot11CountryString));
        */

        sme_trace_debug(( TR_REGDOM,
                          "regdom_init(): MIB Value For unifiTxPowerAdjustment (TPO) = %d",
                          cfg->unifiTxPowerAdjustment_tpo ));

        sme_trace_debug(( TR_REGDOM,
                          "regdom_init(): MIB Value For unifiTxPowerAdjustment (EIRP) = %d",
                          cfg->unifiTxPowerAdjustment_eirp ));

        /* Need to use difference between the two value read from MIB */
        regdata->AntennaGain = cfg->unifiTxPowerAdjustment_tpo - cfg->unifiTxPowerAdjustment_eirp;
        sme_trace_debug(( TR_REGDOM,
                          "regdom_init(): Hence Antenna Gain = %d",
                          regdata->AntennaGain ));

        if ((cfg->regDomInfo.dot11MultiDomainCapabilityImplemented == FALSE) ||
            (cfg->regDomInfo.dot11MultiDomainCapabilityEnabled == FALSE))
        {
            sme_trace_info((TR_REGDOM,
                            "regdom_init(): 802.11d not Implemented or Not Enabled in this Firmware version."));

            if (cfg->smeConfig.trustLevel != unifi_TrustDisabled)
            {
                sme_trace_warn((TR_REGDOM,
                               "regdom_init(): Configured Trust Level is Invalid or Not Compatible with this Firmware version, So Ignored"));

                cfg->smeConfig.trustLevel = unifi_TrustDisabled;
                sme_trace_warn((TR_REGDOM,
                               "regdom_init(): Trust Level has been reset to unifi_TrustDisabled."));
            }

            regdata->regulatoryDomainOperation = regDomain_Off;
            sme_trace_info((TR_REGDOM,
                            "regdom_init(): 802.11d Operation is disabled."));
        }
        else /* RegDomain is Implemented and Enabled */
        {
            if ((cfg->smeConfig.trustLevel < unifi_TrustStrict) ||
                (cfg->smeConfig.trustLevel > unifi_TrustMIB))
            {
                sme_trace_warn((TR_REGDOM,
                               "regdom_init(): Configured Trust Level is Invalid or Not Compatible with this Firmware version, So Ignored"));

                cfg->smeConfig.trustLevel = unifi_TrustMIB;
                sme_trace_warn((TR_REGDOM,
                               "regdom_init(): Trust Level has been reset to unifi_TrustMIB."));
            }
            regdata->regulatoryDomainOperation = regDomain_On;
            sme_trace_info((TR_REGDOM,
                            "regdom_init(): 802.11d Operation Enabled"));
        }
    }

    /* Initialise Country IE to connect to */
    CsrMemSet( regdata->countryIeFromConnection, (CsrInt8) 0, 5+14*3 );

    /* List of channels  (based on default regulatory domain and configured trust level */
    create_channels_lists_for_initial_scan(context, cfg, regdata);

    /* For Trust Level Adjunct, BSS, IBSS or MIB Check if adjunct tech has already given us some valid country code
     *  and if so then configure Reg Dom accordingly
     */
    if (cfg->regdomReadFromMib)
    {
        regdom_process_location_signal( context,
                                        regdata,
                                        cfg->smeConfig.countryCode );
    }

    sme_trace_entry((TR_REGDOM, "<< regdom_init()"));
}


/**************************************************************************************************/

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 */
/*---------------------------------------------------------------------------*/
DataReference regdom_create_ie_dr(FsmContext* context, DataReference inputIeDR, CsrBool forBssJoin)
{
    RegDomData   * regData   = get_regulatory_data(context);
    SmeConfigData* cfg       = get_sme_config(context);
    CsrUint8        * inBuf     = NULL;        /* accessor to the input IEs  */
    CsrUint16         inBufLen  = 0;           /* length of the input IEs    */
    CsrUint8        * cIePtr    = NULL;        /* pointer to country info IE */
    DataReference  returnRef = {0xDEAD, 0}; /* The return value           */

    sme_trace_entry((TR_REGDOM, ">> regdom_create_ie_dr(): forBssJoin %s",
                     forBssJoin?"YES":"NO"));

    /*
     * If regulatory mode is disabled, just return with a copy of the input
     */
    if (cfg->smeConfig.trustLevel == unifi_TrustDisabled)
    {
        if (inputIeDR.dataLength)
        {
            CsrUint8 * dbgBuf;
            CsrUint16  dbgBufLen;

            /* Do not increment the payload, create a new one to disconnect the
             * scanresult payload from the start IE.  the scanresult IE can change
             * underneath causing loss of payloads.
             */
            pld_access(getPldContext(context), (PldHdl)inputIeDR.slotNumber, (void **)&dbgBuf, &dbgBufLen);
            sme_trace_hex((TR_REGDOM, TR_LVL_DEBUG, "regdom_create_ie_dr: return IEs",
                                   dbgBuf, dbgBufLen));

            returnRef = regdom_create_ie_dr_src_buf(context, dbgBuf, dbgBufLen, forBssJoin);
        }
        else
        {
            sme_trace_debug((TR_REGDOM,
                         "regdom_create_ie_dr: regulatory mode disabled and no input IEs provided - return empty DR"));
        }
        return returnRef;
    }

    /*
     * If passed some input IEs, have a look to see if they contain a country
     * IE. If so, our job is done.
     */
    if (inputIeDR.dataLength)
    {
        pld_access(getPldContext(context), (PldHdl)inputIeDR.slotNumber,
                   (void **)&inBuf, &inBufLen);

        sme_trace_hex((TR_REGDOM, TR_LVL_DEBUG, "regdom_create_ie_dr: input IEs",
                       inBuf, inBufLen));

        if (regData->regulatoryDomainOperation == regDomain_Off ||
            ie_find(IE_ID_COUNTRY, inBuf, inBufLen, &cIePtr) == ie_success)
        {
            /* job done - the input info elements are sufficient */
            pld_add_ref(getPldContext(context), (PldHdl)inputIeDR.slotNumber);
            returnRef.slotNumber = inputIeDR.slotNumber;
            returnRef.dataLength = inputIeDR.dataLength;

            sme_trace_debug((TR_REGDOM,
                             "regdom_create_ie_dr: Returning with orig IE, Country IE present \"%s\", reg-support \"%s\"",
                             cIePtr ? "YES" : "NO",
                             regData->regulatoryDomainOperation == regDomain_On ? "ENABLED" : "DISABLED"));



            if (forBssJoin)
            {
                /* store received Country IE (of AP we want to join) */
                CsrMemCpy (  regData->countryIeFromConnection, cIePtr, *(cIePtr+1)+2 );
                sme_trace_debug((TR_REGDOM, "regdom_create_ie_dr(): Saving Received country IE"));
            }

            sme_trace_entry((TR_REGDOM, "<< regdom_create_ie_dr()"));
            return returnRef;
        }
    }

    if (forBssJoin)
    {
        /* reset stored connection country IE - the network we're joining didn't provide one */
        CsrMemSet( regData->countryIeFromConnection, (CsrInt8) 0, 5+14*3 );
        sme_trace_debug((TR_REGDOM, "regdom_create_ie_dr(): Reset connection-country-IE, none provided by AP"));
     }

    returnRef = regdom_create_ie_dr_src_buf(context, inBuf, inBufLen, forBssJoin);
    sme_trace_entry((TR_REGDOM, "<< regdom_create_ie_dr()"));
    return returnRef;
}

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 */
/*---------------------------------------------------------------------------*/
DataReference regdom_create_ie_dr_src_buf(FsmContext* context, CsrUint8 *inBuf, CsrUint16 inBufLen, CsrBool forBssJoin)
{
    RegDomData   * regData   = get_regulatory_data(context);
    SmeConfigData* cfg       = get_sme_config(context);
    CsrUint8        * cIePtr    = NULL;        /* pointer to country info IE */
    CsrUint16         cIeLen    = 0;           /* Length of the country IE   */
    DataReference  returnRef = {0xDEAD, 0}; /* The return value           */

    sme_trace_entry((TR_REGDOM, ">> regdom_create_ie_dr_src_buf(): forBssJoin %s",
                    forBssJoin?"YES":"NO"));

    if (cfg->smeConfig.trustLevel == unifi_TrustDisabled)
    {
        if (inBuf)
        {
            sme_trace_debug((TR_REGDOM,
                             "regdom_create_ie_dr_src_buf: Regulatory mode disabled - returning with input data"));

            pld_store(getPldContext(context), inBuf, inBufLen, (PldHdl *)&returnRef.slotNumber);
            returnRef.dataLength = inBufLen;
        }
        else
        {
            sme_trace_debug((TR_REGDOM,
                             "regdom_create_ie_dr_src_buf: Regulatory mode disabled, no input data - return empty DR"));
        }
        return returnRef;
    }

    /*
     * If passed some input IEs, have a look to see if they contain a country
     * IE. If so, our job is done.
     */
    if (inBufLen)
    {
        sme_trace_hex((TR_REGDOM, TR_LVL_DEBUG, "regdom_create_ie_dr_src_buf: input IEs",
                       inBuf, inBufLen));

        if (regData->regulatoryDomainOperation == regDomain_Off ||
            ie_find(IE_ID_COUNTRY, inBuf, inBufLen, &cIePtr) == ie_success)
        {
            /* job done - the input info elements are sufficient */
            pld_store(getPldContext(context), inBuf, inBufLen, (PldHdl *)&returnRef.slotNumber);
            returnRef.dataLength = inBufLen;

            sme_trace_debug((TR_REGDOM,
                             "regdom_create_ie_dr_src_buf: orig IE, Country IE present \"%s\", reg-support \"%s\"",
                             cIePtr ? "YES" : "NO",
                             regData->regulatoryDomainOperation == regDomain_On ? "ENABLED" : "DISABLED"));

            if (forBssJoin)
            {
                /* store received Country IE (of AP we want to join) */
                CsrMemCpy (  regData->countryIeFromConnection, cIePtr, *(cIePtr+1)+2 );
                sme_trace_debug((TR_REGDOM, "regdom_create_ie_dr_src_buf(): Saving Received country IE"));
            }

            sme_trace_entry((TR_REGDOM, "<< regdom_create_ie_dr_src_buf()"));
            return returnRef;
        }
    }

    /*
     * Either no input IEs, or input IEs do not contain a country info IE
     * We need to create one ourselves from the active channel list.
     */
    if (forBssJoin)
    {
        /* reset stored connection country IE - the network we're joining didn't provide one */
        CsrMemSet( regData->countryIeFromConnection, (CsrInt8) 0, 5+14*3 );
        sme_trace_debug((TR_REGDOM, "regdom_create_ie_dr_src_buf(): Reset connection-country-IE, none provided by AP"));
     }
    sme_trace_debug((TR_REGDOM, "regdom_create_ie_dr_src_buf: Need to create artificial country IE"));

    /*
     * BUT, can't generate an IE if we're operating in strict mode and
     * generating for a join
     */
    if (cfg->smeConfig.trustLevel == unifi_TrustStrict && forBssJoin)
    {
        sme_trace_info(( TR_REGDOM,
                         "regdom_create_ie_dr_src_buf: Cannot generate country IE when Mode is STRICT or ADJUNCT" ));
        return returnRef;
    }

    /*
     * We're clear to artificially generate our own IE - let's do it
     */
    if (create_country_ie_from_active_list(context, &cIePtr, &cIeLen))
    {
        /*
         * If we have an input set of IEs then we must merge in the
         * country info we've just created
         */
        if (inBuf)
        {
            CsrUint8   * newBuf;
            CsrUint16    newLen;

            sme_trace_debug((TR_REGDOM, "regdom_create_ie_dr_src_buf: Merging artificial IE with input IEs"));
            newLen = inBufLen + cIeLen;

            /*
             * Create new buffer with the input info elements merged with a
             * country IE of our own construction. Create it in the payload
             * subsystem, sort out the return data reference.
             */
            pld_create(getPldContext(context), newLen, (void **)&newBuf, (PldHdl *)&returnRef.slotNumber);
            returnRef.dataLength = newLen;
            CsrMemCpy(newBuf, inBuf, inBufLen);
            CsrMemCpy(newBuf + inBufLen, cIePtr, cIeLen);

            sme_trace_hex((TR_REGDOM, TR_LVL_DEBUG, "regdom_create_ie_dr_src_buf: Returned data reference content",
                           newBuf, newLen));
        }
        else
        {
            /*
             * Just need to store this info in the payload subsystem and sort
             * out the return data reference
             */
            sme_trace_debug((TR_REGDOM,
                             "regdom_create_ie_dr_src_buf: Registering standalone artificial IE with payload subsys"));

            /*
             * NB: pld_store makes a copy of the buffer so we need to release
             * what we pass in
             */
            pld_store(getPldContext(context), cIePtr, cIeLen, (PldHdl *)&returnRef.slotNumber);
            returnRef.dataLength = cIeLen;

            sme_trace_hex((TR_REGDOM, TR_LVL_DEBUG, "regdom_create_ie_dr_src_buf: Returned data reference content",
                           cIePtr, cIeLen));
        }
        CsrPfree(cIePtr);
    }

    sme_trace_entry((TR_REGDOM, "<< regdom_create_ie_dr_src_buf()"));
    return returnRef;
}

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 */
/*---------------------------------------------------------------------------*/
void regdom_process_beacon(FsmContext* context,
                           RegDomData *regdata,
                           BssType receive_bss_type,
                           ChannelNumber receive_chan,
                           Megahertz receive_freq,
                           const DataReference ieRef)
{
    SmeConfigData* cfg = get_sme_config(context);

    sme_trace_entry((TR_REGDOM, ">> process_beacon()"));

    /* when unifi_TrustDisabled (802.11d Operation Disabled) the received Country IE
     * is ignored: So This function does nothing.
     */
    if (cfg->smeConfig.trustLevel == unifi_TrustDisabled)
    {
        sme_trace_debug((TR_REGDOM, "<< process_beacon(): 802.11d Operation Disabled"));
        return;
    }

    sme_trace_debug((TR_REGDOM,
                     "process_beacon(): Beacon Received on Channel: %d; (Freq: %d)",
                     receive_chan, receive_freq));

    /*
     * Firstly, check to see if the beacon possess a country IE
     */
    if (ieRef.dataLength > 0)
    {
        CsrUint16 length;
        void  * buf     = NULL;
        CsrUint8 * element = NULL;

        pld_access(getPldContext(context), (PldHdl)ieRef.slotNumber, &buf, &length);
        if (ie_find(IE_ID_COUNTRY, buf, length, &element) == ie_success)
        {
            process_80211d_ie(context, regdata, element, receive_bss_type, receive_chan);
            return;
        }
    }

    /*
     * If the beacon is from a trusted source, then we can mark it as Active
     */
    if (((cfg->smeConfig.trustLevel >= unifi_TrustBSS) && (receive_bss_type == BssType_Infrastructure))
          || (cfg->smeConfig.trustLevel >= unifi_TrustIBSS))
    {
        Channel_Info * chnlInfo;
        CsrUint8          ordered_position;

        ordered_position = map_channel_to_position((CsrUint8)receive_chan);
        if (ordered_position == 0xff)
        {
            sme_trace_warn((TR_REGDOM, "Failed to map channel %d to internal list - ignoring beacon", receive_chan));
            return;
        }

        chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[ordered_position];

        sme_trace_debug((TR_REGDOM, "process_beacon(): trustLvl:%d receive chan = %d, maps to posn %d which is %s",
                         cfg->smeConfig.trustLevel, receive_chan, ordered_position,
                         chnlInfo->chan_scan_mode == channelScanMode_active?"ACTIVE":"Passive"));

        /* Only add channel to active list if not already there */
        if (chnlInfo->chan_scan_mode != channelScanMode_active)
        {
            RegulatoryDomainInfoStruct *pRegDomStctr;
            POWER_REGIME   locationPwrRegime;
            CsrUint8 i;

            /* Turn Channel to Active */
            chnlInfo->chan_scan_mode = channelScanMode_active;

            /* We do not have a power value so:
             * use power derived from current regulatory domain if we have one
             * Otherwise use power derived from Default regulatory domain
             */
            if ( regdata->CurrentLocation.locStatus == Location_Valid )
            {
                pRegDomStctr = get_ptr_to_regulatory_domain_info( regdata->CurrentLocation.regDomain );
                locationPwrRegime = regdata->CurrentLocation.locationPwrRegime;
            }
            else
            {
                pRegDomStctr = get_ptr_to_regulatory_domain_info( regdata->DefaultLocation.regDomain );
                locationPwrRegime = regdata->DefaultLocation.locationPwrRegime;
            }

            /* Get the stored power level for this channel under the current Regulatory Domain */
            chnlInfo->chan_tx_power =  pRegDomStctr->chan_power[receive_chan-1];
            /* if added channel is illegal in the regulatory domain then power will be -127
             * which is a problem; in such a case use power of another legal channel
             */
            i=0;
            while ( (chnlInfo->chan_tx_power==CHANNEL_INVALID_POWER) && (i<HIGHEST_80211_b_g_CHANNEL_NUM)  )
            {
                chnlInfo->chan_tx_power =  pRegDomStctr->chan_power[i++];
            }

            /* Currently all Reg doms store power as EIRP, so label accordingly as EIRP */
            chnlInfo->chan_pwr_regime = EIRP_PWR_REGIME;

            /* May Need to convert to TPO */
            if ( locationPwrRegime == TPO_PWR_REGIME )
            {
                 chnlInfo->chan_tx_power = convert_to_tpo( regdata, chnlInfo->chan_tx_power );
                 chnlInfo->chan_pwr_regime = TPO_PWR_REGIME;
            }
            else if ( locationPwrRegime == UNKNOWN_PWR_REGIME )
            {
                /* IF Sth went wrong cancel changes and return */
                 chnlInfo->chan_tx_power = CHANNEL_INVALID_POWER;
                 chnlInfo->chan_pwr_regime = UNKNOWN_PWR_REGIME;
                 chnlInfo->chan_scan_mode = channelScanMode_passive;
                 sme_trace_warn(( TR_REGDOM,
                                  "process_beacon(): Could NOT Add (Receive) Channel %d to Active List. Problem getting Location Power Regime",
                                  receive_chan ));
                 return;
            }

            /* Indicate that channels list has changed */
            regdata->ChannelsInUseList.listChangedFlag = TRUE;

            sme_trace_debug(( TR_REGDOM,
                              "process_beacon(): Added (Receive) Channel %d to Active List, with power set to %d dBm (%s)",
                              receive_chan,
                              chnlInfo->chan_tx_power,
                              ((chnlInfo->chan_pwr_regime== EIRP_PWR_REGIME) ? "EIRP" : "TPO" ) ));

            chnlInfo->expiryFlag = TRUE;
        }

        /* Set/Update Expiry Time for channel */
        if ( chnlInfo->expiryFlag )
        {
            CsrUint32 now = fsm_get_time_of_day_ms(context);
            chnlInfo->expiryTime = now + ACTIVE_CHANNEL_VALIDITY_PERIOD;

            sme_trace_debug(( TR_REGDOM,
                              "process_beacon(): Updated Expiry Time for (Receive) Channel %d to %lu",
                              receive_chan, chnlInfo->expiryTime));

            print_channels_status(context, regdata);
        }
    }
    sme_trace_entry((TR_REGDOM, "<< process_beacon()"));
}

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 */
/*---------------------------------------------------------------------------*/
CsrBool regdom_check_for_scan_refresh(FsmContext* context, RegDomData* regdata)
{
    CsrUint8          i;
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    sme_trace_entry((TR_REGDOM, ">> regdom_check_for_scan_refresh()"));

    sme_trace_debug((TR_REGDOM,
                      "regdom_check_for_scan_refresh(): nowTime = %lu",
                      now));

    for (i=0 ; i < HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
    {
        Channel_Info* chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[i];

        if ( chnlInfo->expiryFlag &&
             (chnlInfo->chan_scan_mode == channelScanMode_active) &&
             (CsrTimeSub(chnlInfo->expiryTime, now) < TIME_BEFORE_SCAN_REFRESH) &&
             (CsrTimeSub(chnlInfo->expiryTime, now) >= GUARD_TIME_SCAN_REFRESH ) )
        {
                sme_trace_entry((TR_REGDOM, "<< regdom_check_for_scan_refresh(TRUE)"));
                return TRUE;
        }
    }

    sme_trace_entry((TR_REGDOM, "<< regdom_check_for_scan_refresh(FALSE)"));
    return FALSE;
}

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 */
/*---------------------------------------------------------------------------*/
CsrBool regdom_process_expired_channels(FsmContext* context, RegDomData* regdata)
{
    CsrUint8          i;
    CsrBool        outcome_channelExpired  = FALSE;
    CsrBool        outcome_powerExpired    = FALSE;
    SmeConfigData* cfg                     = get_sme_config(context);
    CsrUint32 now = fsm_get_time_of_day_ms(context);

    sme_trace_entry((TR_REGDOM, ">> process_expired_channels()"));

    sme_trace_debug((TR_REGDOM,
                      "process_expired_channels(): nowTime = %lu",
                      now));

    for (i=0 ; i < HIGHEST_80211_b_g_CHANNEL_NUM ; i++)
    {
        Channel_Info* chnlInfo = &regdata->ChannelsInUseList.Dott11_b_g_ChanList[i];

        /*
         * Only bother checking currently Active channels
         */
        if (chnlInfo->chan_scan_mode == channelScanMode_active)
        {
            CsrUint8 ch = map_position_to_channel(i);
            /*
             * If channel has expired then we can only passively scan this channel
             * If channel has not expired then we need to check if power on this channel has expired
             */
            if ( chnlInfo->expiryFlag && CsrTimeLt(chnlInfo->expiryTime, now) ) /* In order to expire a channel must also expiry-enabled */
            {
                sme_trace_info((TR_REGDOM, "Channel %d has expired - Reconfig for Reg Domain 0x%02x",
                                ch, regdata->DefaultLocation.regDomain ));

                /* Configure channel for passive scan */
                config_channel_for_reg_domain(context, ch, cfg, regdata, regdata->DefaultLocation.regDomain, TRUE);

                /* Update expiry variables */
                chnlInfo->expiryTime = 0xFFFFFFFF;
                chnlInfo->hasPwrExpired = FALSE; /* since Channel has expired and going to passive */
                chnlInfo->expiryTimePower = 0xFFFFFFFF;

                /* Flag that a channel has expired */
                outcome_channelExpired = TRUE;
            }
            else if ( (chnlInfo->expiryTimePower != 0xFFFFFFFF)  && CsrTimeLt(chnlInfo->expiryTimePower, now))
            {
                sme_trace_info((TR_REGDOM, "Power on Channel %d has expired", ch ));

                /* Update power expiry variables */
                chnlInfo->hasPwrExpired = TRUE;
                chnlInfo->expiryTimePower = 0xFFFFFFFF;
                /* chnlInfo->chan_tx_power remains until a future Access point gives us a value */

                /* Flag that power of this channel has expired */
                outcome_powerExpired = TRUE;
            }
        }
    }

    /*
     *  - If some channels have expired, then 3 Possibilities:
     *    1) Active list is Empty -> Move Back to default Regulatory Domain (invalidate the current location)
     *    2) All Channels in Active list are legal under default Reg Dom -> Move Back to default Regulatory Domain (invalidate the current location)
     *    3) At least one Channel in Active list is Not legal under default Reg Dom  -> do nothing (at some point we will end up in 1) or 2) above...
     *
     *  - If only power have expired just print status
     *
     *  - If no channel has expired then no point doing anything
     */
    if ( outcome_channelExpired )
    {
        if ( is_active_list_empty(regdata) )
        {
            sme_trace_debug((TR_REGDOM, "process_expired_channels(): All Active Channels have expired"));

            /* No active channels means current location has expired */
            invalidate_current_location(regdata);

            /* Set power regime for each active channel (based on default regime) */
            set_default_pwr_regime_on_legal_chans( regdata );
        }
        else
        {
            if ( are_all_active_chans_legal_under_specifiedRegDom(regdata, regdata->DefaultLocation.regDomain) )
            {
                sme_trace_debug((TR_REGDOM, "process_expired_channels(): All remaining Active channels are legal under default Regulatory Domain"));

                /* The current location has also expired if all remaining active channels are legal under Default regulatory Domain */
                invalidate_current_location(regdata);

                /* Set power regime for each active channel (based on default regime) */
                set_default_pwr_regime_on_legal_chans( regdata );
            }
            else
            {
                sme_trace_debug((TR_REGDOM, "process_expired_channels(): Some remaining Active channels are illegal under default Regulatory Domain"));
            }
        }

        /* Print updated channels status (since changed) */
        print_channels_status(context, regdata);
    }
    else if ( outcome_powerExpired )
    {
        sme_trace_entry((TR_REGDOM, "process_expired_channels(): No channel has Expired"));

        /* Print updated channels status to show power has expired */
        print_channels_status(context, regdata);
    }
    else
    {
        /* no need to print channels status since nothing has changed */
        sme_trace_entry((TR_REGDOM, "process_expired_channels(): No channel has Expired and No Power has expired"));
    }

    sme_trace_entry((TR_REGDOM, "<< process_expired_channels()"));

    return outcome_channelExpired ;
}

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 */
/*---------------------------------------------------------------------------*/
CsrBool regdom_is_channel_actively_usable(FsmContext* context, CsrUint8 channelNum)
{
    SmeConfigData* cfg       = get_sme_config(context);

    if (cfg->smeConfig.trustLevel == unifi_TrustDisabled)
    {
        sme_trace_debug((TR_REGDOM, "regdom_is_channel_actively_usable: may transmit on channel %d", channelNum));
        return TRUE;
    }
    else
    {
        RegDomData    * regData   = get_regulatory_data(context);
        CsrUint8           idx;
        ChannelScanMode sm;

        idx   = map_channel_to_position(channelNum);
        sm    = regData->ChannelsInUseList.Dott11_b_g_ChanList[idx].chan_scan_mode;

        return (sm == channelScanMode_active)?TRUE:FALSE;
    }
}

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8 regdom_get_first_active_channel(FsmContext* context)
{
    RegDomData    * regData   = get_regulatory_data(context);
    SmeConfigData * cfg       = get_sme_config(context);
    CsrUint8           i;

    if (cfg->smeConfig.trustLevel == unifi_TrustDisabled)
    {
        sme_trace_error((TR_REGDOM, "regdom_get_first_active_channel: illegally called (regulatory support disabled)"));
        return 0;
    }

    for (i = 0; i < HIGHEST_80211_b_g_CHANNEL_NUM; i++)
    {
        if (regData->ChannelsInUseList.Dott11_b_g_ChanList[i].chan_scan_mode==channelScanMode_active)
        {
            CsrUint8   chNum = map_position_to_channel(i);

            sme_trace_info((TR_REGDOM, "regdom_get_first_active_channel: First legal channel is %d", chNum));
            return chNum;
        }
    }
    return 0;
}

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 */
/*---------------------------------------------------------------------------*/
void regdom_process_location_signal(FsmContext* context, RegDomData* regdata, CsrUint8 *ctryCode)
{
    CsrUint16 cntry_index;
    UpdateLocationResult  updateMylocationResult;
    SmeConfigData         *cfg = get_sme_config(context);

    sme_trace_entry((TR_REGDOM, ">> regdom_process_location_signal()"));

    if ( (cfg->smeConfig.trustLevel < unifi_TrustAdjunct) || (cfg->smeConfig.trustLevel > unifi_TrustMIB) )
    {
         sme_trace_debug((TR_REGDOM, "location notification ignored for current trust level: %d", cfg->smeConfig.trustLevel ));
         sme_trace_entry((TR_REGDOM, "<< regdom_process_location_signal()"));
         return;
    }

    /* Check the Country Code (2 first characters of Country string) */
    cntry_index = is_country_code_valid( ctryCode );

    /* Update Country Info
     * Any country not found in Table (except "00" is assumed under ETSI (to agree with Firmware)
     * Hence a different (simpler) function is called. For "00" Adjunct Tecnology could not provide
     * a country hence ignore.
     */
    if (cntry_index < NUM_LISTED_COUNTRIES)
    {
        CsrUint8 rxChan=0; /* we do not have a receive channel with Adjunct Technology so use 0 */
        updateMylocationResult = update_current_location( context, regdata, rxChan, cntry_index);
    }
    else if (cntry_index == 0xfffd)
    {
        updateMylocationResult = updateLocation_Ignore;
    }
    else /* cntry_index == 0xfffe or == 0xffff */
    {
        updateMylocationResult = update_current_location_etsi(regdata, ctryCode );
    }

    /* Update channels Lists based on Stored List for Regulatory Domain of Received Country */
    if ( updateMylocationResult == updateLocation_UpdateAll )
    {
        /*  But only if Regulatory Domain has changed */
        config_channel_for_reg_domain(context, 0, cfg, regdata, regdata->CurrentLocation.regDomain, FALSE);
    }
    else if ((updateMylocationResult == updateLocation_NoUpdate) || (updateMylocationResult == updateLocation_CountryOnly))
    {
        /* otherwise just refresh timers on active channels */
        refresh_timer_on_active_legal_chans(context, regdata);
    }

    if ( (updateMylocationResult == updateLocation_UpdateAll ) ||
         (updateMylocationResult == updateLocation_NoUpdate) ||
         (updateMylocationResult == updateLocation_CountryOnly) )
    {
        print_channels_status(context, regdata);
    }

    sme_trace_entry((TR_REGDOM, "<< regdom_process_location_signal()"));
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

#ifdef CSR_AMP_ENABLE

void reg_domain_amp_encode_country_ie(FsmContext* context, RegDomData *regdata, CsrUint8 *encodedIE, CsrUint16 *encodedLength, USED_CHANNELS_LIST *channelList)
{
    CsrUint16 ieLen=0;

    sme_trace_entry((TR_REGDOM, ">> reg_domain_amp_encode_country_ie(): "));

/* Ideally you would want to call reg domain API to get the IE encoded. But the encoded IE from reg domain does not contain
 * regulatory triplet that is mandatory for AMP  So do some thing hacky for now.
 */
/*     returnRef = regdom_create_ie_dr_src_buf(context, NULL, 0, FALSE); */

    /* For the timebeing create an IE with XXX as country and regulatory triplet with regulatory class as 254.
     * Do this if regulatory is disabled (check for returnBuf length)
     */
    encodedIE[ieLen++] = 'X';
    encodedIE[ieLen++] = 'X';
    encodedIE[ieLen++] = 'X';
    encodedIE[ieLen++] = 201; /*regulatory triplet id */
    encodedIE[ieLen++] = 254; /* regulatory class for non-country - according to BT3.0+HS PAL spec. section 3.2.3*/
    encodedIE[ieLen++] = 0; /* Coverage class not used by AMP */

    if (channelList->listChangedFlag)
    {
        CsrUint8 i;
        CsrUint8 numChannels=0;
        CsrUint8 startChannel=0;

        sme_trace_info((TR_REGDOM, ">> reg_domain_amp_encode_country_ie(): There are channels in the used list. so add sub-band triplets"));

        for (i=0; i<HIGHEST_80211_b_g_CHANNEL_NUM&& channelScanMode_active==channelList->Dott11_b_g_ChanList[i].chan_scan_mode; i++)
        {
            sme_trace_info((TR_REGDOM, ">> reg_domain_amp_encode_country_ie(): loop-%d: numChannels-%d, startChannel-%d",i,numChannels,startChannel));
            if (0 == numChannels)
            {
                sme_trace_info((TR_REGDOM, ">> reg_domain_amp_encode_country_ie(): first subband triplet started"));
                startChannel = channelList->Dott11_b_g_ChanList[i].chan_number;
            }
            else if (channelList->Dott11_b_g_ChanList[i].chan_number != startChannel+numChannels)
            {
                encodedIE[ieLen++] = startChannel;
                encodedIE[ieLen++] = numChannels;
                encodedIE[ieLen++] = 20; /* as per PAL spec for a default value section 3.2.3 */
                sme_trace_info((TR_REGDOM, ">> reg_domain_amp_encode_country_ie(): added sub-band triplet- first channel-%d,numChannels-%d",startChannel,numChannels));

                /* start the next triplet */
                startChannel = channelList->Dott11_b_g_ChanList[i].chan_number;
                numChannels = 0;
            }
            numChannels++;

            /* If this is the last channel in the list then add it as another triplet */
            if (HIGHEST_80211_b_g_CHANNEL_NUM == i+1 ||
                channelList->Dott11_b_g_ChanList[i+1].chan_scan_mode != channelScanMode_active)
            {
                encodedIE[ieLen++] = startChannel;
                encodedIE[ieLen++] = numChannels;
                encodedIE[ieLen++] = 20; /* as per PAL spec for a default value section 3.2.3 */
                sme_trace_info((TR_REGDOM, ">> reg_domain_amp_encode_country_ie(): added final sub-band triplet- first channel-%d,numChannels-%d",startChannel, numChannels));
            }
        }

    }

    *encodedLength = ieLen;

}


  /*
     * @brief Validate regulatory class and obtain the legal channel list for the class.
     *
     * @par Description
     * Regulatory class is not supported in our regulatory domain module as the ratified 802.11 standard does not specify regulatory class
     * 2.4Ghz. So special handling is implemented for AMP. This function will need revisiting when SME moves to latest specs (802.11k) to support
     * regulatory classes.
     *
     * Note: this function is a simpler version of update_current_location_etsi()
     *
     * @param[in] regulatory_class : regulatory class
     * @param[in] regDom : regulatory domain structure that contains all information about the regulatory domain
     * @param[out] legalChannels : list of legal channel populated by this function for the requested regulatory class.
     *
     * @return
     *    status:
     *          - TRUE    :  regulatory class is supported and legal channels populated
     *          - FALSE :  regulatory class not valid or not supported
     *
     */
static CsrBool regulatory_class_supported(CsrUint8 regulatoryClass, RegulatoryDomainInfoStruct *regDom, CsrUint16 *legalChannels)
{
    CsrBool status = FALSE;

    sme_trace_entry((TR_REGDOM, ">> regulatory_class_supported(): check class - %d for the domain - %d",regulatoryClass, regDom->RegDomain));

    /* Refer Table J.1 in Annex J of IEEE Draft P802.11-REVmb/D1.01, July 2009 for the regulatory class table.
    * Only US, Europe and Japan is specified in the spec. Others are not. So we only support what is mentioned.
    */
    switch (regDom->RegDomain)
    {
        case unifi_RegulatoryDomainFcc:
            /* as per PAL Spec section 3.2.3,
             * In context of Country String 254. signifies 2.4 GHz ISM band channels 1 - 11
             */
            if (254 == regulatoryClass || 12 == regulatoryClass)
            {
                *legalChannels = regDom->chan_legality;
                status=TRUE;
            }
            break;

        case unifi_RegulatoryDomainEtsi:
        case unifi_RegulatoryDomainSpain:
        case unifi_RegulatoryDomainFrance:
            if (4 == regulatoryClass)
            {
                *legalChannels = regDom->chan_legality;
                status=TRUE;
            }
            break;

        case unifi_RegulatoryDomainJapan:
            if (30 == regulatoryClass)
            {
                *legalChannels = regDom->chan_legality;
                status=TRUE;
            }
            break;

        default:
            sme_trace_warn((TR_REGDOM, ">> regulatory_class_supported(): unknown reg domain "));
            break;
    }

    return status;
}

/*
 * Description:
 * See description in regulatory_domain/regulatory_domain.h
 * This function should eventually merge with regulatory domain module and use APIs provided by the module to decode the IE.
 * Until then, use some AMP specific stuff.
 */
void reg_domain_amp_decode_country_ie(FsmContext* context, RegDomData *regdata, USED_CHANNELS_LIST  *preferredChannelList, CsrUint8 *element, CsrUint16 length)
{
    COUNTRY_INFO_STRUCT *pCountryInfoStruct  = NULL;
    CsrUint8                numReceivedTriplets = 0;
    CsrUint8 *ie = CsrPmalloc(length+2);

    sme_trace_entry((TR_REGDOM, ">> reg_domain_amp_decode_country_ie()"));

    /* put a country Id for the sake of the function ie_getCountryInfoIE()*/
    ie[0] = IE_ID_COUNTRY;
    ie[1] = (CsrUint8)length;
    CsrMemCpy(ie+2,element,length);

    CsrMemSet(preferredChannelList,0,sizeof(USED_CHANNELS_LIST));
    if (ie_getCountryInfoIE(regdata, ie, &pCountryInfoStruct, &numReceivedTriplets) == ie_success)
    {
        CsrUint16 channelIndex=0;
        unifi_RegulatoryDomain regDomain;
        CsrUint8 triplets;
        RegulatoryDomainInfoStruct *currentRegDom;
        CsrUint16 cntry_index = is_country_code_valid(pCountryInfoStruct->countryString);

        if (cntry_index < NUM_LISTED_COUNTRIES)
        {
            regDomain = CountryList[cntry_index].regDomain;
        }
        else if (cntry_index == 0xfffd)
        {
            regDomain = unifi_RegulatoryDomainFcc;
        }
        else if (cntry_index == 0xfffe) /* country code is 'XXX' then assume FCC domain so that it sticks to the PAL spec section 3.2.3 for legal channel (1 through 11)*/
        {
            regDomain = unifi_RegulatoryDomainFcc;
        }
        else
        { /* cntry_index == 0xffff*/
            regDomain = unifi_RegulatoryDomainEtsi;
        }
        sme_trace_info((TR_REGDOM, "numTriplets-%d, first channel-%d",numReceivedTriplets, pCountryInfoStruct->subBand[0].first_channel));

        /* Access structure containig info about this regulatory domain */
        currentRegDom = get_ptr_to_regulatory_domain_info( regDomain );
        verify(TR_REGDOM, currentRegDom != NULL);

        /* Loop through the triplets until we get a first set of channel. Skip the triplets if outr channelList is full */
        for (triplets=0; triplets < numReceivedTriplets && channelIndex < HIGHEST_80211_b_g_CHANNEL_NUM; )
        {
            /* Regulatory triplet is mandatory for AMP*/
            if (pCountryInfoStruct->subBand[triplets].first_channel == HIGHEST_80211_CHANNEL_NUM+1) /* Reg extension id for AMP */
            {
                CsrUint8 regulatoryClass = pCountryInfoStruct->subBand[triplets].num_channels;
                CsrUint8 i,j;
                CsrUint16 subbandChannelIndex=channelIndex;
                CsrBool subBandTripletPresent=FALSE;
                CsrUint16 legalChannels;

                triplets++;
                if (!regulatory_class_supported(regulatoryClass, currentRegDom, &legalChannels))
                {
                    sme_trace_info((TR_REGDOM, "Unsupported regulatory class. Skip to get next triplet"));
                    continue;
                }
                sme_trace_info((TR_REGDOM, "Valid regulatory class"));

                for (j=1;
                     j <= HIGHEST_80211_b_g_CHANNEL_NUM && channelIndex < HIGHEST_80211_b_g_CHANNEL_NUM;
                     j++)
                {
                    if (REGULATORY_CHANNEL_LEGAL(legalChannels,j))
                    {
                        sme_trace_info((TR_REGDOM, "Reg Triplet-channel added-%d at position-%d",j,channelIndex));
                        preferredChannelList->Dott11_b_g_ChanList[channelIndex].chan_number = j;
                        preferredChannelList->Dott11_b_g_ChanList[channelIndex].chan_scan_mode= channelScanMode_active;
                        preferredChannelList->Dott11_b_g_ChanList[channelIndex].chan_tx_power = 20; /* put a default value. not sure if its correct. FIXME  */
                        channelIndex++;
                        if (!preferredChannelList->listChangedFlag)
                        {
                            preferredChannelList->listChangedFlag = TRUE;
                        }
                    }
                }
                /* now look for any subband triplets for this regulatory triplet. According to PAL Spec section 3.2.3
                * "If sub-band triplets are given then they are to be assumed to modify the immediately prior regulatory triplet."
                * So overwrite whatever this regulatory triplet would have written
                */
                if (pCountryInfoStruct->subBand[triplets].first_channel <= HIGHEST_80211_CHANNEL_NUM)
                {
                    for (i=triplets;
                         i<numReceivedTriplets && subbandChannelIndex < HIGHEST_80211_b_g_CHANNEL_NUM;
                         i++)
                    {
                        if (pCountryInfoStruct->subBand[i].first_channel <= HIGHEST_80211_CHANNEL_NUM)
                        {
                            sme_trace_info((TR_REGDOM, "Sub-Band triplet found - %d, first channel-%d,numChannels-%d",
                                i,pCountryInfoStruct->subBand[i].first_channel,
                                pCountryInfoStruct->subBand[i].num_channels));

                            subBandTripletPresent = TRUE;
                            for (j=pCountryInfoStruct->subBand[i].first_channel;
                                 j < pCountryInfoStruct->subBand[i].first_channel + pCountryInfoStruct->subBand[i].num_channels &&
                                 subbandChannelIndex < HIGHEST_80211_b_g_CHANNEL_NUM;
                                 j++)
                            {
                                /* accept the channel only if it is valid in this regulatory domain. */
                                if (REGULATORY_CHANNEL_LEGAL(legalChannels,i))
                                {
                                    sme_trace_info((TR_REGDOM, "Sub-Band triplet-channel added-%d at position-%d",j,subbandChannelIndex));
                                    preferredChannelList->Dott11_b_g_ChanList[subbandChannelIndex].chan_number = (CsrUint8)j;
                                    preferredChannelList->Dott11_b_g_ChanList[subbandChannelIndex].chan_scan_mode= channelScanMode_active;
                                    preferredChannelList->Dott11_b_g_ChanList[subbandChannelIndex].chan_tx_power = pCountryInfoStruct->subBand[i].legal_power_dBm;
                                    subbandChannelIndex++;
                                    if (!preferredChannelList->listChangedFlag)
                                    {
                                        preferredChannelList->listChangedFlag = TRUE;
                                    }
                                }
                            }
                        }
                        else
                        {
                            sme_trace_info((TR_REGDOM, "Skip the loop as the new triplet is not a sub-band triplet"));
                            break;
                        }
                    }
                    triplets = i;
                }

                if (subBandTripletPresent && channelIndex != subbandChannelIndex)
                {
                    sme_trace_info((TR_REGDOM, "Reg Triplet- subband triplet replaced reg tripet channels. - cuurentIndex-%d,regTripletIndex-%d",subbandChannelIndex,channelIndex));
                    if (channelIndex > subbandChannelIndex)
                    {
                        sme_trace_info((TR_REGDOM, "Reg Triplet-reset extra channels from reg triplet - cuurentIndex-%d,regTripletIndex-%d",subbandChannelIndex,channelIndex));
                        CsrMemSet(&preferredChannelList->Dott11_b_g_ChanList[subbandChannelIndex],0,sizeof(Channel_Info)*(channelIndex - subbandChannelIndex));
                    }
                    channelIndex = subbandChannelIndex;
                }
            }
            else
            {
                triplets++;
                sme_trace_info((TR_REGDOM, "sub-band triplet not supported.skipping to find next regulatory triplet"));
            }
        }
    }
    sme_trace_entry((TR_REGDOM, "<< process_amp_assoc_ie()"));

    if (pCountryInfoStruct)
    {
        CsrPfree(pCountryInfoStruct);
    }
    CsrPfree(ie);

#ifdef SME_TRACE_ENABLE
    {
        CsrInt32 j;
        sme_trace_info((TR_REGDOM, "reg_domain_amp_decode_country_ie: Channel is %s \n", preferredChannelList->listChangedFlag?"UPDATED":"NOT UPDATED"));
        for (j=0; j<HIGHEST_80211_b_g_CHANNEL_NUM; j++)
        {
             sme_trace_info((TR_REGDOM, "Channel -%d , power-%d",preferredChannelList->Dott11_b_g_ChanList[j].chan_number,preferredChannelList->Dott11_b_g_ChanList[j].chan_tx_power));
        }
    }
#endif

}
#endif
