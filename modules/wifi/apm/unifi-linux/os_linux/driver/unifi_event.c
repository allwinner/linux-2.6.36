/*
 * ***************************************************************************
 *  FILE:     unifi_event.c
 *
 *  PURPOSE:
 *      Process the signals received by UniFi.
 *      It is part of the porting exercise.
 *
 * Copyright (C) 2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */


/*
 * Porting notes:
 * The implementation of unifi_receive_event() in Linux is fairly complicated.
 * The linux driver support multiple userspace applications and several
 * build configurations, so the received signals are processed by different
 * processes and multiple times.
 * In a simple implementation, this function needs to deliver:
 * - The MLME-UNITDATA.ind signals to the Rx data plane and to the Traffic
 *   Analysis using unifi_ta_sample().
 * - The MLME-UNITDATA-STATUS.ind signals to the Tx data plane.
 * - All the other signals to the SME using unifi_sys_hip_ind().
 */

#include "driver/unifi.h"
#include "driver/conversions.h"
#include "unifi_priv.h"


/*
 * ---------------------------------------------------------------------------
 *  send_to_client
 *
 *      Helper for unifi_receive_event.
 *
 *      This function forwards a signal to one client.
 *
 *  Arguments:
 *      priv        Pointer to driver's private data.
 *      client      Pointer to the client structure.
 *      receiver_id The reciever id of the signal.
 *      sigdata     Pointer to the packed signal buffer.
 *      siglen      Length of the packed signal.
 *      bulkdata    Pointer to the signal's bulk data.
 *
 *  Returns:
 *      None.
 *
 * ---------------------------------------------------------------------------
 */
static void send_to_client(unifi_priv_t *priv, ul_client_t *client,
                           int receiver_id,
                           unsigned char *sigdata, int siglen,
                           const bulk_data_param_t *bulkdata)
{
    if (client && client->event_hook) {
        unifi_trace(priv, UDBG3,
                    "Receive: client %d, (s:0x%X, r:0x%X) - Signal %s \n",
                    client->client_id, client->sender_id, receiver_id,
                    lookup_signal_name(COAL_GET_UINT16_FROM_LITTLE_ENDIAN(sigdata)));

        client->event_hook(client, sigdata, siglen, bulkdata, UDI_TO_HOST);
    }
}


/*
 * ---------------------------------------------------------------------------
 *  unifi_receive_event
 *
 *      Dispatcher for received signals.
 *
 *      This function receives the 'to host' signals and forwards
 *      them to the unifi linux clients.
 *
 *  Arguments:
 *      ospriv      Pointer to driver's private data.
 *      sigdata     Pointer to the packed signal buffer.
 *      siglen      Length of the packed signal.
 *      bulkdata    Pointer to the signal's bulk data.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *  The signals are received in the format described in the host interface
 *  specification, i.e wire formatted. Certain clients use the same format
 *  to interpret them and other clients use the host formatted structures.
 *  Each client has to call read_unpack_signal() to transform the wire
 *  formatted signal into the host formatted signal, if necessary.
 *  The code is in the core, since the signals are defined therefore
 *  binded to the host interface specification.
 * ---------------------------------------------------------------------------
 */
void
unifi_receive_event(void *ospriv,
                    CsrUint8 *sigdata, CsrUint32 siglen,
                    const bulk_data_param_t *bulkdata)
{
    unifi_priv_t *priv = (unifi_priv_t*)ospriv;
    int i, receiver_id;
    int client_id;
    CsrInt16 signal_id;

    func_enter();

    unifi_trace(priv, UDBG5, "unifi_receive_event: "
                "%04x %04x %04x %04x %04x %04x %04x %04x (%d)\n",
                COAL_GET_UINT16_FROM_LITTLE_ENDIAN((sigdata) + sizeof(CsrInt16)*0) & 0xFFFF,
                COAL_GET_UINT16_FROM_LITTLE_ENDIAN((sigdata) + sizeof(CsrInt16)*1) & 0xFFFF,
                COAL_GET_UINT16_FROM_LITTLE_ENDIAN((sigdata) + sizeof(CsrInt16)*2) & 0xFFFF,
                COAL_GET_UINT16_FROM_LITTLE_ENDIAN((sigdata) + sizeof(CsrInt16)*3) & 0xFFFF,
                COAL_GET_UINT16_FROM_LITTLE_ENDIAN((sigdata) + sizeof(CsrInt16)*4) & 0xFFFF,
                COAL_GET_UINT16_FROM_LITTLE_ENDIAN((sigdata) + sizeof(CsrInt16)*5) & 0xFFFF,
                COAL_GET_UINT16_FROM_LITTLE_ENDIAN((sigdata) + sizeof(CsrInt16)*6) & 0xFFFF,
                COAL_GET_UINT16_FROM_LITTLE_ENDIAN((sigdata) + sizeof(CsrInt16)*7) & 0xFFFF, siglen);

    receiver_id = COAL_GET_UINT16_FROM_LITTLE_ENDIAN((sigdata) + sizeof(CsrInt16)) & 0xFFF0;
    client_id = (receiver_id & 0x0F00) >> UDI_SENDER_ID_SHIFT;
    signal_id = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(sigdata);

    /* Signals with ReceiverId==0 are also reported to SME / WEXT */
    if (receiver_id == 0) {

        /*
         * We must not pass MA-UNITDATA.INDICATIONs to the SME because
         * we can not filter them in the handler. The reason is that we
         * need to pass some AMP related data, but the filtering needs
         * be done in the 802.11->802.3 translation to avoid extra process
         * in the data path.
         * Also, we filter out the MA-UNITDATA.CONFIRMs which are not
         * directed to the SME.
         */
        if ((signal_id != CSR_MA_UNITDATA_INDICATION_ID) &&
            (signal_id != CSR_MA_UNITDATA_CONFIRM_ID)) {
            send_to_client(priv, priv->sme_cli,
                           receiver_id,
                           sigdata, siglen, bulkdata);
        }

#ifdef CSR_NATIVE_LINUX
        send_to_client(priv, priv->wext_client,
                       receiver_id,
                       sigdata, siglen, bulkdata);
#endif
    }

#ifdef CSR_SUPPORT_SME
    if (signal_id == CSR_MLME_EAPOL_CONFIRM_ID)
    {
        if (priv->m4_monitor_state == m4_wait_eapol_confirm) {
            unifi_trace(priv, UDBG1, "unifi_receive_event: Sending M4 Transmitted IND\n");
            unifi_sys_m4_transmitted_ind(priv->smepriv);
            priv->m4_monitor_state = m4_idle;
        }
    }
#endif

    if ((client_id < MAX_UDI_CLIENTS) &&
        (&priv->ul_clients[client_id] != priv->logging_client)) {
        send_to_client(priv, &priv->ul_clients[client_id],
                       receiver_id,
                       sigdata, siglen, bulkdata);
    }

    /*
     * Free bulk data buffers here unless it is a CSR_MA_UNITDATA_INDICATION
     */
    switch (signal_id)
    {
    case CSR_MA_UNITDATA_INDICATION_ID:
#ifdef UNIFI_SNIFF_ARPHRD
    case CSR_MA_SNIFFDATA_INDICATION_ID:
#endif
        break;

    default:
        for (i = 0; i < UNIFI_MAX_DATA_REFERENCES; i++) {
            if (bulkdata->d[i].data_length != 0) {
                unifi_net_data_free(priv, (void *)&bulkdata->d[i]);
            }
        }
    }

    func_exit();
} /* unifi_receive_event() */

