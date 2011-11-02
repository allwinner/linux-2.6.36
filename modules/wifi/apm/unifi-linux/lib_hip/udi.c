/*
 * ---------------------------------------------------------------------------
 *  FILE:     card_udi.c
 * 
 *  PURPOSE:
 *      Maintain a list of callbacks to log UniFi exchanges to one or more
 *      debug/monitoring client applications.
 *
 * NOTES:
 *      Just call the UDI driver log fn directly for now.
 *      When done properly, each open() on the UDI device will install
 *      a log function. We will call all log fns whenever a signal is written
 *      to or read form the UniFi.
 * 
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include "driver/unifi.h"
#include "card.h"


/*
 * ---------------------------------------------------------------------------
 *  unifi_print_status
 *
 *      Print status info to given character buffer.
 * 
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_print_status(card_t *card, CsrInt8 *str)
{
    CsrInt8 *p = str;
    sdio_config_data_t *cfg;
    CsrUint16 i, n;
    
    i = n = 0;
    p += CsrSprintf(p, "Chip ID %u\n", (CsrUint16)card->chip_id);
    p += CsrSprintf(p, "Chip Version %04X\n", card->chip_version);
    p += CsrSprintf(p, "HIP v%u.%u\n",
                 (card->config_data.version >> 8) & 0xFF,
                 card->config_data.version & 0xFF);
    p += CsrSprintf(p, "Build %lu: %s\n", card->build_id, card->build_id_string);


    cfg = &card->config_data;

    p += CsrSprintf(p, "sdio ctrl offset          %u\n", cfg->sdio_ctrl_offset);
    p += CsrSprintf(p, "fromhost sigbuf handle    %u\n", cfg->fromhost_sigbuf_handle);
    p += CsrSprintf(p, "tohost_sigbuf_handle      %u\n", cfg->tohost_sigbuf_handle);
    p += CsrSprintf(p, "num_fromhost_sig_frags    %u\n", cfg->num_fromhost_sig_frags);
    p += CsrSprintf(p, "num_tohost_sig_frags      %u\n", cfg->num_tohost_sig_frags);
    p += CsrSprintf(p, "num_fromhost_data_slots   %u\n", cfg->num_fromhost_data_slots);
    p += CsrSprintf(p, "num_tohost_data_slots     %u\n", cfg->num_tohost_data_slots);
    p += CsrSprintf(p, "data_slot_size            %u\n", cfg->data_slot_size);

    /* Added by protocol version 0x0001 */
    p += CsrSprintf(p, "overlay_size              %u\n", (CsrUint16)cfg->overlay_size);

    /* Added by protocol version 0x0300 */
    p += CsrSprintf(p, "data_slot_round           %u\n", cfg->data_slot_round);
    p += CsrSprintf(p, "sig_frag_size             %u\n", cfg->sig_frag_size);

    /* Added by protocol version 0x0300 */
    p += CsrSprintf(p, "tohost_sig_pad            %u\n", cfg->tohost_signal_padding);
    
    p += CsrSprintf(p, "\nInternal state:\n");

    p += CsrSprintf(p, "fhsr: %u\n", (CsrUint16)card->from_host_signals_r);
    p += CsrSprintf(p, "fhsw: %u\n", (CsrUint16)card->from_host_signals_w);
    p += CsrSprintf(p, "thsr: %u\n", (CsrUint16)card->to_host_signals_r);
    p += CsrSprintf(p, "thsw: %u\n", (CsrUint16)card->to_host_signals_w);
    p += CsrSprintf(p, "fh buffer contains: %u signals, %u bytes\n",
                 card->fh_buffer.count,
                 card->fh_buffer.ptr - card->fh_buffer.buf);
    p += CsrSprintf(p, "paused: ");
    for (i = 0; i < sizeof(card->tx_q_paused_flag)/sizeof(card->tx_q_paused_flag[0]); i++)
    {
        p += CsrSprintf(p, card->tx_q_paused_flag[i]? "1": "0");
    }
    p += CsrSprintf(p, "\n");
    p += CsrSprintf(p, "fh command q: %u waiting, %u free of %u:\n",
                 q_slots_used(&card->fh_command_queue),
                 q_slots_free(&card->fh_command_queue),
                 UNIFI_SOFT_COMMAND_Q_LENGTH);
    for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
        p += CsrSprintf(p, "fh traffic q[%u]: %u waiting, %u free of %u:\n", i,
                     q_slots_used(&card->fh_traffic_queue[i]),
                     q_slots_free(&card->fh_traffic_queue[i]),
                     UNIFI_SOFT_TRAFFIC_Q_LENGTH);
        p += CsrSprintf(p, "fh data slots free q[%u]: %u\n",
                     i, CardGetFreeFromHostDataSlots(card, i));
    }


    p += CsrSprintf(p, "From host data slots:");
    n = card->config_data.num_fromhost_data_slots;
    for (i = 0; i < n; i++) {
        p += CsrSprintf(p, " %u", (CsrUint16)card->from_host_data[i].bd.data_length);
    }
    p += CsrSprintf(p, "\n");
    
    p += CsrSprintf(p, "To host data slots:");
    n = card->config_data.num_tohost_data_slots;
    for (i = 0; i < n; i++) {
        p += CsrSprintf(p, " %u", (CsrUint16)card->to_host_data[i].data_length);
    }
    
    p += CsrSprintf(p, "\n");


    p += CsrSprintf(p, "\nStats:\n");
    p += CsrSprintf(p, "Total SDIO bytes: R=%lu W=%lu\n",
                 card->sdio_bytes_read,
                 card->sdio_bytes_written);

    p += CsrSprintf(p, "Interrupts generated: %lu\n", card->unifi_interrupt_seq);

    return (p - str);
} /* unifi_print_status() */
