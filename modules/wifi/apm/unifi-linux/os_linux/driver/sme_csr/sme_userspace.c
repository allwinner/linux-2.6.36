/*
 *****************************************************************************
 *
 * FILE : sme_userspace.c
 * 
 * PURPOSE : Support functions for userspace SME helper application.
 *
 *
 * Copyright (C) 2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 ***************************************************************************** 
 */

#include "unifi_priv.h"



int
uf_sme_init(unifi_priv_t *priv)
{
    int i;

    priv->smepriv = priv;

    init_waitqueue_head(&priv->sme_request_wq);

#ifdef CSR_SUPPORT_WEXT
    priv->ignore_bssid_join = FALSE;
    priv->mib_data.length = 0;

    priv->filter_tclas_ies = NULL;
    memset(&priv->packet_filters, 0, sizeof(uf_cfg_bcast_packet_filter_t));

    uf_sme_wext_set_defaults(priv);
#endif /* CSR_SUPPORT_WEXT*/

    priv->sta_ip_address = 0xFFFFFFFF;

    memset(&priv->controlled_data_port, 0, sizeof(unifi_port_config_t));
    priv->controlled_data_port.entries_in_use = 1;
    priv->controlled_data_port.overide_action = UF_DATA_PORT_OVERIDE;
    priv->controlled_data_port.port_cfg[0].port_action = unifi_8021xPortClosedDiscard;

    memset(&priv->controlled_data_port, 0, sizeof(unifi_port_config_t));
    priv->uncontrolled_data_port.entries_in_use = 1;
    priv->uncontrolled_data_port.overide_action = UF_DATA_PORT_OVERIDE;
    priv->uncontrolled_data_port.port_cfg[0].port_action = unifi_8021xPortClosedDiscard;

    priv->m4_monitor_state = m4_idle;
    priv->m4_bulk_data.net_buf_length = 0;
    priv->m4_bulk_data.data_length = 0;
    priv->m4_bulk_data.os_data_ptr = priv->m4_bulk_data.os_net_buf_ptr = NULL;

    priv->wifi_on_state = wifi_on_unspecified;

    sema_init(&priv->sme_sem, 1);
    memset(&priv->sme_reply, 0, sizeof(sme_reply_t));

    priv->ta_ind_work.in_use = 0;
    priv->ta_sample_ind_work.in_use = 0;

    for (i = 0; i < MAX_MA_UNIDATA_IND_FILTERS; i++) {
        priv->sme_unidata_ind_filters[i].in_use = 0;
    }

    /* Create a work queue item for Traffic Analysis indications to SME */
    INIT_WORK(&priv->ta_ind_work.task, uf_ta_ind_wq);
    INIT_WORK(&priv->ta_sample_ind_work.task, uf_ta_sample_ind_wq);
#ifdef CSR_SUPPORT_WEXT
    INIT_WORK(&priv->sme_config_task, uf_sme_config_wq);
#endif

    return 0;
} /* uf_sme_init() */


void
uf_sme_deinit(unifi_priv_t *priv)
{
    int i;

#ifdef CSR_SUPPORT_WEXT
    /* Free any TCLASs previously allocated */
    if (priv->packet_filters.tclas_ies_length) {
        priv->packet_filters.tclas_ies_length = 0;
        CsrPfree(priv->filter_tclas_ies);
        priv->filter_tclas_ies = NULL;
    }
#endif /* CSR_SUPPORT_WEXT*/

    for (i = 0; i < MAX_MA_UNIDATA_IND_FILTERS; i++) {
        priv->sme_unidata_ind_filters[i].in_use = 0;
    }

} /* uf_sme_deinit() */




int
sme_queue_message(unifi_priv_t *priv, u8 *buffer, int length)
{
    ul_client_t *pcli;
    udi_log_t *logptr;
    udi_msg_t *msgptr;
    u8 *p;

    func_enter();

    /* Just a sanity check */
    if ((buffer == NULL) || (length <= 0)) {
        return -EINVAL;
    }

    pcli = priv->sme_cli;
    if (pcli == NULL) {
        CsrPfree(buffer);
        return -EINVAL;
    }

    /* Allocate log structure plus actual signal. */
    logptr = (udi_log_t *)kmalloc(sizeof(udi_log_t) + length, GFP_KERNEL);
    if (logptr == NULL) {
        unifi_error(priv, "Failed to allocate %d bytes for an SME message\n",
                    sizeof(udi_log_t) + length);
        CsrPfree(buffer);
        return -ENOMEM;
    }

    /* Fill in udi_log struct */
    INIT_LIST_HEAD(&logptr->q);
    msgptr = &logptr->msg;
    msgptr->length = sizeof(udi_msg_t) + length;
    msgptr->signal_length = length;

    /* Copy signal and bulk data to the log */
    p = (u8 *)(msgptr + 1);
    memcpy(p, buffer, length);

    /* Add to tail of log queue */
    down(&pcli->udi_sem);
    list_add_tail(&logptr->q, &pcli->udi_log);
    up(&pcli->udi_sem);

    /* Wake any waiting user process */
    wake_up_interruptible(&pcli->udi_wq);

    /* It is our responsibility to free the buffer allocated in build_packed_*() */
    CsrPfree(buffer);

    func_exit();

    return 0;

} /* sme_queue_message() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_ta_indicate_protocol
 *
 *      Report that a packet of a particular type has been seen
 * 
 *  Arguments:
 *      drv_priv        The device context pointer passed to ta_init.
 *      protocol        The protocol type enum value.
 *      direction       Whether the packet was a tx or rx.
 *      src_addr        The source MAC address from the data packet.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      We defer the actual sending to a background workqueue,
 *      see uf_ta_ind_wq().
 * ---------------------------------------------------------------------------
 */
void
unifi_ta_indicate_protocol(void *ospriv, 
                           unifi_TrafficPacketType packet_type,
                           unifi_ProtocolDirection direction,
                           const unifi_MACAddress *src_addr)
{
    unifi_priv_t *priv = (unifi_priv_t*)ospriv;

    if (priv->ta_ind_work.in_use) {
        unifi_warning(priv,
                      "unifi_ta_indicate_protocol: workqueue item still in use, not sending\n");
        return;
    }

    priv->ta_ind_work.packet_type = packet_type;
    priv->ta_ind_work.direction = direction;
    priv->ta_ind_work.src_addr = *src_addr;

    queue_work(priv->unifi_workqueue, &priv->ta_ind_work.task);

} /* unifi_ta_indicate_protocol() */


/*
 * ---------------------------------------------------------------------------
 * unifi_ta_indicate_sampling
 *
 *      Send the TA sampling information to the SME.
 * 
 *  Arguments:
 *      drv_priv        The device context pointer passed to ta_init.
 *      stats   The TA sampling data to send.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
unifi_ta_indicate_sampling(void *ospriv, unifi_TrafficStats *stats)
{
    unifi_priv_t *priv = (unifi_priv_t*)ospriv;

    if (!priv) {
        return;
    }

    if (priv->ta_sample_ind_work.in_use) {
        unifi_warning(priv,
                      "unifi_ta_indicate_sampling: workqueue item still in use, not sending\n");
        return;
    }

    priv->ta_sample_ind_work.stats = *stats;

    queue_work(priv->unifi_workqueue, &priv->ta_sample_ind_work.task);

} /* unifi_ta_indicate_sampling() */


