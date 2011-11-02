/*
 * ---------------------------------------------------------------------------
 * FILE:     data_tx.c
 * 
 * PURPOSE:
 *      This file provides functions to send data requests to the UniFi.
 *
 * Copyright (C) 2007-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include "driver/unifi.h"
#include "unifi_priv.h"

int
uf_verify_m4(unifi_priv_t *priv, const unsigned char *packet, unsigned int length)
{
    const unsigned char *p = packet;
    CsrUint16 keyinfo;


    if (length < (4 + 5 + 8 + 32 + 16 + 8 + 8 + 16 + 1 + 8)) {
        return 1;
    }

    p += 8;
    keyinfo = p[5] << 8 | p[6]; /* big-endian */ 
    if (
          (p[0] == 1 || p[0] == 2) /* protocol version 802.1X-2001 (WPA) or -2004 (WPA2) */ && 
          p[1] == 3 /* EAPOL-Key */ && 
          /* don't bother checking p[2] p[3] (hh ll, packet body length) */ 
          (p[4] == 254 || p[4] == 2) /* descriptor type P802.1i-D3.0 (WPA) or 802.11i-2004 (WPA2) */ &&
          ((keyinfo & 0x0007) == 1 || (keyinfo & 0x0007) == 2) /* key descriptor version */ &&
         (keyinfo & ~0x0207U) == 0x0108 && /* key info for 4/4 or 4/2 -- ignore key desc version and sec bit (since varies in WPA 4/4) */
          (p[4 + 5 + 8 + 32 + 16 + 8 + 8 + 16 + 0] == 0 && /* key data length (2 octets) 0 for 4/4 only */
           p[4 + 5 + 8 + 32 + 16 + 8 + 8 + 16 + 1] == 0)
        ) {
        unifi_trace(priv, UDBG1, "uf_verify_m4: M4 detected \n");
        return 0;
    }
    else
    {
        return 1;
    }
}

/*
 * ---------------------------------------------------------------------------
 *
 *      Data transport signals.
 *
 * ---------------------------------------------------------------------------
 */

/*
 * ---------------------------------------------------------------------------
 * uf_mlme_eapol
 *
 *      Send a EAP data packet in a MLME-EAPOL signal to UniFi.
 *
 * Arguments:
 *      priv            Pointer to device private context struct
 *      pcli            Pointer to context of calling process
 *      sig             Pointer to a signal containing a MLME-EAPOL.req
 *      bulkdata        Pointer to a bulk data structure, describing
 *                      the data to be sent.
 *
 * Returns:
 *      0 on success
 *      -1 if an error occurred
 * ---------------------------------------------------------------------------
 */
int
uf_mlme_eapol(unifi_priv_t *priv, ul_client_t *pcli,
              CSR_SIGNAL *sig, const bulk_data_param_t *bulkdata)
{
    int r;

    sig->SignalPrimitiveHeader.ReceiverProcessId = 0;
    sig->SignalPrimitiveHeader.SenderProcessId = pcli->sender_id;

#ifdef CSR_SUPPORT_SME
    if ((priv->m4_monitor_state == m4_trap) && 
        (0 == uf_verify_m4(priv, bulkdata->d[0].os_data_ptr, bulkdata->d[0].data_length))) {

        /* Store the EAPOL M4 packet for later */
        priv->m4_signal = *sig;
        priv->m4_bulk_data.net_buf_length = bulkdata->d[0].net_buf_length;
        priv->m4_bulk_data.data_length = bulkdata->d[0].data_length;
        priv->m4_bulk_data.os_data_ptr = bulkdata->d[0].os_data_ptr;
        priv->m4_bulk_data.os_net_buf_ptr = bulkdata->d[0].os_net_buf_ptr;
        priv->m4_monitor_state = m4_trapped;

        return 0;
    }
#endif

    /* Send the signal to UniFi */
    r = ul_send_signal_unpacked(priv, sig, bulkdata);
    if (r) {
        unifi_error(priv, "Error queueing CSR_MLME_EAPOL_REQUEST signal\n");
        return -1;
    }

    /* The final unifi_sys_ma_unitdata_cfm() will called when the actual MA-UNITDATA.cfm is received from the chip */

    /*
     * We do not advance the sequence number of the last sent signal
     * because we will not wait for a response from UniFi.
     */

    return 0;
} /* uf_mlme_eapol() */


/* 
 * ---------------------------------------------------------------------------
 * uf_ma_unitdata
 * 
 *      Send a UNITDATA signal to UniFi.
 *
 * Arguments:
 *      priv            Pointer to device private context struct
 *      pcli            Pointer to context of calling process
 *      sig             Pointer to a signal containing a MA_UNITDATA 
 *                      or DS_UNITDATA request structure to send.
 *      bulkdata        Pointer a to bulk data structure, describing
 *                      the data to be sent.
 *
 * Returns:
 *      0 on success
 *      -1 if an error occurred
 * ---------------------------------------------------------------------------
 */
int
uf_ma_unitdata(unifi_priv_t *priv, ul_client_t *pcli,
               CSR_SIGNAL *sig, const bulk_data_param_t *bulkdata)
{
    int r;

    sig->SignalPrimitiveHeader.ReceiverProcessId = 0;
    sig->SignalPrimitiveHeader.SenderProcessId = pcli->sender_id;

    /* Send the signal to UniFi */
    r = ul_send_signal_unpacked(priv, sig, bulkdata);
    if (r) {
        unifi_error(priv, "Error queueing CSR_MA_UNITDATA_REQUEST signal\n");
        return -1;
    }

    /*
     * We do not advance the sequence number of the last sent signal
     * because we will not wait for a response from UniFi.
     */

    return 0;
} /* uf_ma_unitdata() */

