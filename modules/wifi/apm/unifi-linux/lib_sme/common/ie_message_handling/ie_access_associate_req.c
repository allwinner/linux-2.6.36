/** @file ie_access_associate_req.c
 *
 * Utilities to process WPS information elements
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
 *   Utilities to process WPS information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_message_handling/ie_access_associate_req.c#5 $
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_message_handling/ie_access_associate_req.c#5 $
 *
 ****************************************************************************/
#include "ie_message_handling/ie_access_associate_req.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "abstractions/osa.h"
#include "fsm/csr_wifi_fsm.h"
#include "ie_access/ie_access.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "ie_access/ie_access.h"

#include "qos_fsm/qos_fsm.h"

#include "sys_sap/sys_sap_from_sme_interface.h"

#include "smeio/smeio_trace_types.h"

#ifdef CCX_VARIANT
#include "ccx_mbssid/ccx_mbssid.h"
#endif

/* MACROS *******************************************************************/


/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in ie_access/ie_access_associate_req.h
 */
/*---------------------------------------------------------------------------*/
void ie_create_associated_req_ies(
                    FsmContext* context,
                    DataReference securityIeDataRef,
                    SmeConfigData* cfg)
{
    unifi_DataBlock assocReqInfoElements = {0, NULL};
    CsrUint16 totalLength;

    CsrUint8 bWPA2IePresent = 0;
    CsrUint8 bWPAIePresent = 0;
    CsrUint8 bWAPIIePresent = 0;
    CsrBool bWMMIePresent = FALSE;
    CsrBool bWPSIePresent = FALSE;

    sme_trace_entry((TR_IE_ACCESS, "ie_create_associated_req_ies"));

    /* free up the old ie if one exists */
    if (cfg->assocReqIeDataRef.dataLength != 0)
    {
        pld_release(getPldContext(context),cfg->assocReqIeDataRef.slotNumber);

        cfg->assocReqIeDataRef.dataLength = 0;
    }

    /* The Driver has the ability to inject its own IEs,
     * the SME must process this to determine what is included
     */
    if(cfg->connectionConfig.mlmeAssociateReqInformationElementsLength > 0)
    {
        assocReqInfoElements.length = cfg->connectionConfig.mlmeAssociateReqInformationElementsLength;
        assocReqInfoElements.data   = cfg->connectionConfig.mlmeAssociateReqInformationElements;

        bWPA2IePresent =  ie_rsn_check_ap_wpa2_capability(
                            assocReqInfoElements.data,
                            assocReqInfoElements.length);
        bWPAIePresent =  ie_rsn_check_ap_wpa_capability(
                            assocReqInfoElements.data,
                            assocReqInfoElements.length);
        bWAPIIePresent =  ie_rsn_check_ap_wapi_capability(
                            assocReqInfoElements.data,
                            assocReqInfoElements.length);

        bWMMIePresent =  ie_wmm_check_ap_wmm_capability(
                            assocReqInfoElements.data,
                            assocReqInfoElements.length);

        bWPSIePresent =  ie_wps_check_ap_wps_capability(
                            assocReqInfoElements.data,
                            assocReqInfoElements.length);

        sme_trace_debug((TR_IE_ACCESS, "bWPA2IePresent %d, bWPAIePresent %d, bWMMIePresent %d, bWPSIePresent %d", bWPA2IePresent, bWPAIePresent, bWMMIePresent, bWPSIePresent));
    }

    /* ------------------------------------------------------------------------
     * Calculate the length
     */
    totalLength = 0;

    /* add any overridding IEs */
    totalLength += assocReqInfoElements.length;

    /* Do not use wpa/wpa2/wapi IE when WPS is in use */
    if (!bWPSIePresent)
    {
        totalLength -= bWPAIePresent;
        totalLength -= bWPA2IePresent;
        totalLength -= bWAPIIePresent;
        totalLength += securityIeDataRef.dataLength;

    }

#ifdef CCX_VARIANT
    if (cfg->joinIECapabilities & CCX_Capable)
    {
        /* check to see if the H85 is required */
        if (ccx_ie_check_if_h85_required(context))
        {
            totalLength += ccx_ie_get_h85_length();
        }
    }

    if (cfg->ccxConfig.ccxRadioMgtEnabled == TRUE)
    {
        totalLength += IE_CCX_RADIO_MANAGEMENT__TOTAL_SIZE;
    }
#endif

/*
    if( (cfg->joinIECapabilities & CCX_Capable)
      &&(cfg->joinIECapabilities & WMM_Capable))
     */
    if (cfg->joinIECapabilities & CCX_Capable)
    {
        totalLength += qos_tspec_associate_tspec_length(qos_get_tspecdatablk_context(context));
    }

    /* increment length for wmm capabilities if required */
    if (!bWMMIePresent)
    {
        if (cfg->joinIECapabilities & WMM_Capable)
        {
            sme_trace_debug((TR_IE_ACCESS, "ie_create_associated_req_ies"));
        }
    }
    if (cfg->smeConfig.wmmModeMask != unifi_WmmDisabled &&
        !bWMMIePresent &&
        (cfg->joinIECapabilities & WMM_Capable))
    {
        totalLength  += ie_wmm_get_associate_req_ie_length();
    }

    /* ------------------------------------------------------------------------
     * Create the IE buffer
     */
    if(totalLength != 0)
    {
        CsrUint8 *pCurrent;
        CsrUint8* ieBuf;
        PldHdl hdl;

        pld_create(getPldContext(context), totalLength, (void **)&ieBuf, &hdl );

        CsrMemSet(ieBuf, 0x00, totalLength);
        /* initialise the transition pointer */
        pCurrent = ieBuf;

        /* add any overridding IEs except WPA or WPA2 IE's */
        if (assocReqInfoElements.length != 0)
        {
            CsrUint8* curr = assocReqInfoElements.data;
            CsrUint8* end  = assocReqInfoElements.data + assocReqInfoElements.length;

            while (curr < end)
            {
                if (!ie_valid(curr, end))
                {
                    sme_trace_error((TR_IE_ACCESS, "ie_create_associated_req_ies() :: ie_invalid :: id = %d, length = %d", ie_id(curr), ie_len(curr)));
                    verify(TR_IE_ACCESS, 0);    /*lint !e774*/
                }

                if (ie_id(curr) == IE_ID_RSN)
                {
                    sme_trace_debug((TR_IE_ACCESS, "ie_create_associated_req_ies() :: skip wpa2 rsn ie"));
                }
                else if (ie_id(curr) == IE_ID_VENDOR_SPECIFIC && CsrMemCmp(&curr[2], ieWpaOui, 4) == 0)
                {
                    sme_trace_debug((TR_IE_ACCESS, "ie_create_associated_req_ies() :: skip wpa ie"));
                }
                else
                {
                    CsrMemCpy(pCurrent, curr, ie_len(curr) + 2);
                    pCurrent += ie_len(curr) + 2;
                }

                curr = ie_next(curr);
            }
        }

        /* Do not use wpa/wpa2/wapi IE when WPS is in use */
        if (!bWPSIePresent)
        {
            if (securityIeDataRef.dataLength != 0)
            {
                /* access the payload */
                CsrUint16 len;
                CsrUint8 *secBuf;

                pld_access(getPldContext(context),securityIeDataRef.slotNumber,
                           (void**)&secBuf,
                           &len);

                ie_printIEInfo(secBuf, len);

                CsrMemCpy(pCurrent, secBuf, securityIeDataRef.dataLength);

                sme_trace_hex((TR_IE_ACCESS, TR_LVL_DEBUG, "secBuf added", secBuf, len));

                /* advance the current pointer */
                pCurrent += securityIeDataRef.dataLength;
            }
        }

#ifdef CCX_VARIANT
        /* CCX IEs */
        if(cfg->joinIECapabilities & CCX_Capable)
        {
            if (ccx_ie_check_if_h85_required(context))
            {
                pCurrent = ccx_ie_set_h85(context, pCurrent);

                send_ccx_set_keep_alive_req(context, getSmeContext(context)->ccxInstance );
            }

/*            if( (cfg->joinIECapabilities & CCX_Capable)
              &&(cfg->joinIECapabilities & WMM_Capable))
*/
            sme_trace_hex((TR_QOS, TR_LVL_DEBUG, "before", ieBuf, totalLength));
            {
                pCurrent = qos_tspec_get_associate_tspec_ies(qos_get_tspecdatablk_context(context), pCurrent);
                qos_tspec_update_associate_tspec_status(qos_get_tspecdatablk_context(context), TSPEC_PENDING);
            }
            sme_trace_hex((TR_QOS, TR_LVL_DEBUG, "after", ieBuf, totalLength));
        }

        if (cfg->ccxConfig.ccxRadioMgtEnabled == TRUE)
        {
            pCurrent = ccx_mbd_generate_radio_mgt_ie( context, pCurrent );
        }
#endif

        /* Add WMM Capabilities */
        if (cfg->smeConfig.wmmModeMask != unifi_WmmDisabled &&
            !bWMMIePresent &&
            (cfg->joinIECapabilities & WMM_Capable))
        {
            CsrUint8 *apWmmIe;
            sme_trace_debug((TR_IE_ACCESS, "WMM_Capable"));

            apWmmIe = ie_get_wmm_ie(cfg->connectionInfo.assocScanInfoElements, cfg->connectionInfo.assocScanInfoElementsLength);

            pCurrent = ie_wmm_generate_associate_req_ie(
                    pCurrent,
                    apWmmIe,
                    cfg->connectionConfig.wmmQosInfo,
                    (CsrBool)((cfg->smeConfig.wmmModeMask & unifi_WmmPSEnabled) != 0),
                    cfg->coexInfo.hasBtDevice);

            call_unifi_sys_qos_control_req(context, unifi_QoSWMMOn);
        }
        else
        {
            call_unifi_sys_qos_control_req(context, unifi_QoSOff);
        }

        /* update config manager ready to supply a copy of the transmitted IE */
        cfg->connectionInfo.assocReqInfoElementsLength = totalLength;
        cfg->connectionInfo.assocReqInfoElements = ieBuf;
        /* update the local buffer for retry if needed */
        cfg->assocReqIeDataRef.slotNumber = hdl;
        cfg->assocReqIeDataRef.dataLength = totalLength;

        sme_trace_hex((TR_QOS, TR_LVL_DEBUG, "Associate Req", ieBuf, totalLength));
    }
    else
    {
        /* reset all pointers */
        cfg->connectionInfo.assocReqInfoElementsLength = 0;
        cfg->connectionInfo.assocReqInfoElements = NULL;
        cfg->assocReqIeDataRef.slotNumber = 0;
        cfg->assocReqIeDataRef.dataLength = 0;
        call_unifi_sys_qos_control_req(context, unifi_QoSOff);
    }
}


