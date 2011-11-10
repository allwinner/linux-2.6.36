/** @file ie_access_wmm.c
 *
 * Utilities to process WMM information elements
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
 *   Utilities to process WMM information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_wmm.c#4 $
 *
 ****************************************************************************/

#include "ie_access/ie_access.h"
#include "ie_access/ie_access_wmm.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "abstractions/osa.h"

#include "fsm/csr_wifi_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

/* MACROS *******************************************************************/

/** WMM QOS INFO BYTE */
#define WMM_QOS_INFO_FIELD_BYTE 0x08

/** WMM associate request ie total size including IE header */
#define WMM_ASSOC_REQ_IE_TOTAL_SIZE 0x09
/** WMM associate request ie total size including IE header */
#define WMM_ASSOC_REQ_IE_PAYLOAD_SIZE 0x07


/* GLOBAL VARIABLE DEFINITIONS **********************************************/
/** WMM OUI Definition */
const CsrUint8  wmmOui[4] = {0x00, 0x50, 0xF2, 0x02};

/**
 * @brief Client Report Group Status
 *
 * @par Description
 *   this is a fixed size message
 */
const CsrUint8  wmmAssociateReqDefault[WMM_ASSOC_REQ_IE_TOTAL_SIZE] =
            { IE_ID_VENDOR_SPECIFIC,            /* elementId */
              WMM_ASSOC_REQ_IE_PAYLOAD_SIZE,    /* length */
              0x00,                             /* OUI1 */
              0x50,                             /* OUI2 */
              0xF2,                             /* OUI3 */
              2,                                /* OUIType */
              0,                                /* OUISubType */
              1,                                /* version */
              0x00 };                           /* QoSInfoField */

/* PRIVATE TYPES DEFINITIONS ************************************************/
/**
 * @brief defines the WMM Subtypes
 *
 * @par Description
 *   defines the WMM Subtypes that are used in WMM informational elements.
 */
typedef enum ie_wmm_subtype
{
   IE_WMM_SUBTYPE__INFO            = (CsrUint8)0,
   IE_WMM_SUBTYPE__PARAMETER       = (CsrUint8)1
} ie_wmm_subtype;


/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in ie_access/ie_access_wmm.h
 */
/*---------------------------------------------------------------------------*/
CsrBool ie_wmm_check_ap_wmm_capability(CsrUint8 *pbuf, CsrUint16 length)
{
    return ie_get_wmm_ie(pbuf, length) != NULL;
}

/*
 * Description:
 * See description in ie_access/ie_access_wmm.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8* ie_wmm_check_for_wmm_parameter_ie(CsrUint8 *pbuf, CsrUint16 length)
{
    CsrUint8 subtype = IE_WMM_SUBTYPE__PARAMETER;
    CsrUint8 *result;

    sme_trace_entry((TR_IE_ACCESS, "ie_wmm_check_for_wmm_parameter_ie, %d", length));

    if(length == 0)
    {
        return NULL;
    }

    if(ie_success == ie_find_vendor_specific(wmmOui, &subtype, 1, pbuf, length, &result))
    {
        return result;
    }

    return NULL;
}


CsrUint8* ie_get_wmm_ie(
        const CsrUint8 *pbuf,
        const CsrUint16 length)
{
    CsrUint8 *result;
    CsrUint8 subtype = IE_WMM_SUBTYPE__INFO;

    if(ie_success == ie_find_vendor_specific(wmmOui, &subtype, 1, (CsrUint8*)pbuf, length, &result))
    {
        return result;
    }
    subtype = IE_WMM_SUBTYPE__PARAMETER;
    if(ie_success == ie_find_vendor_specific(wmmOui, &subtype, 1, (CsrUint8*)pbuf, length, &result))
    {
        return result;
    }

    return NULL;
}


/*
 * Description:
 * See description in ie_access/ie_access_wmm.h
 */
/*---------------------------------------------------------------------------*/
CsrBool ie_isWMMUAPSD(CsrUint8 *pbuf, CsrUint16 length)
{
    CsrUint8 *beaconIe = ie_get_wmm_ie(pbuf, length);
    if(beaconIe == NULL)
    {
        return FALSE;
    }

    return (beaconIe[WMM_QOS_INFO_FIELD_BYTE] & 0x80) != 0;
}

/*
 * Description:
 * See description in ie_access/ie_access_wmm.h
 */
/*---------------------------------------------------------------------------*/
CsrBool ie_wmm_sme_uapsd_enable(
        CsrUint8 QoSInfoField)
{
    /* any set ACs will trigger UASPD */
    return (QoSInfoField & (unifi_WmmAcVo |
                            unifi_WmmAcVi |
                            unifi_WmmAcBk |
                            unifi_WmmAcBe ) ) != 0;
}

/*
 * Description:
 * See description in ie_access/ie_access_wmm.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8 ie_wmm_get_associate_req_ie_length(
                    void)
{
    return  WMM_ASSOC_REQ_IE_TOTAL_SIZE;
}

/*
 * Description:
 * See description in ie_access/ie_access_wmm.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8* ie_wmm_generate_associate_req_ie(
        CsrUint8 *pbuf,
        CsrUint8 *apWmmIe,
        CsrUint8 userQoSInfoField,
        CsrBool wmmPSAllowed,
        CsrBool btEnabled)
{
    CsrUint8 QoSInfoField = 0x00;

    /* start with a default WMM IE */
    CsrMemCpy(pbuf, wmmAssociateReqDefault, WMM_ASSOC_REQ_IE_TOTAL_SIZE);

    /* check if the AP is UASPD enabled */
    if ((apWmmIe[WMM_QOS_INFO_FIELD_BYTE] & 0x80) && wmmPSAllowed)
    {
        /* OxFF indicate that the SME has control over the QoS Info field
         * use the APs settings.*/
        if (userQoSInfoField == 0xFF)
        {
            /* the ap is uaspd enabled, enable all bits */
            QoSInfoField |= 0x0F;

            if (btEnabled)
            {
                /* Force the MaxSP value to be 2 */
                QoSInfoField = 0x20 | (QoSInfoField & 0x0F);
            }
        }
        else
        {
            QoSInfoField = userQoSInfoField;
        }

        sme_trace_info((TR_IE_ACCESS, "QoSInfoField == 0x%.2X", QoSInfoField));
    }
    else
    {
        sme_trace_info((TR_IE_ACCESS, "U-APSD disabled"));
        QoSInfoField = 0x00;
    }

    pbuf[WMM_QOS_INFO_FIELD_BYTE] = QoSInfoField;

    return (pbuf+(WMM_ASSOC_REQ_IE_TOTAL_SIZE));
}

/*
 * Description:
 * See description in ie_access/ie_access_wmm.h
 */
/*---------------------------------------------------------------------------*/
qos_ac_mask build_acm_mask(
        CsrUint8 *pbuf,
        CsrUint16 length)
{
    qos_ac_mask acMask =  AC_NONE_MASK;
    ie_wmm_parameter *pWmmParameter;
    ie_result result;

    sme_trace_entry((TR_QOS, "qos_ac_mask()"));
    sme_trace_hex((TR_QOS, TR_LVL_DEBUG, "ie_getWMM", pbuf, 24));

    result = ie_get_wmm_parameter(pbuf, length, pWmmParameter);

    if (result == ie_success)
    {
        sme_trace_hex((TR_QOS, TR_LVL_DEBUG, "IE_WMM_PARAMETER__AC_PARAS_GETPTR", pbuf, 16));

        /* Best Effort */
        if(IE_ACI_AIFSN__ACM_GET(pWmmParameter->acBestEffortAciAifsn))
        {
            sme_trace_entry((TR_QOS, "AC_BEST_EFFORT_MASK()"));
            acMask |= AC_BEST_EFFORT_MASK;
        }

        /* Background */
        if(IE_ACI_AIFSN__ACM_GET(pWmmParameter->acBackgroundAciAifsn))
        {
            sme_trace_debug((TR_QOS, "AC_BACKGROUND_MASK()"));
            acMask |= AC_BACKGROUND_MASK;
        }

        /* Video */
        if(IE_ACI_AIFSN__ACM_GET(pWmmParameter->acVideoAciAifsn))
        {
            sme_trace_debug((TR_QOS, "AC_VIDEO_MASK()"));
            acMask |= AC_VIDEO_MASK;
        }

        /* Voice */
        if(IE_ACI_AIFSN__ACM_GET(pWmmParameter->acVoiceAciAifsn))
        {
            sme_trace_debug((TR_QOS, "AC_VOICE_MASK()"));
            acMask |= AC_VOICE_MASK;
        }
    }

    return acMask;
}

