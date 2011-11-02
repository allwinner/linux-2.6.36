/*
 * ---------------------------------------------------------------------------
 * FILE:     netdev.c
 *
 * PURPOSE:
 *      This file provides the upper edge interface to the linux netdevice
 *      and wireless extensions.
 *      It is part of the porting exercise.
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */


/*
 * Porting Notes:
 * This file implements the data plane of the UniFi linux driver.
 *
 * All the Tx packets are passed to the HIP core lib, using the
 * unifi_send_signal() API. For EAPOL packets use the MLME-EAPOL.req
 * signal, for all other use the MLME-UNITDATA.req. The unifi_send_signal()
 * expects the wire-formatted (packed) signal. For convenience, in the OS
 * layer we only use the native (unpacked) signal structures. The HIP core lib
 * provides the write_pack() helper function to convert to the packed signal.
 * The packet is stored in the bulk data of the signal. We do not need to
 * allocate new memory to store the packet, because unifi_net_data_malloc()
 * is implemented to return a skb, which is the format of packet in Linux.
 * The HIP core lib frees the bulk data buffers, so we do not need to do
 * this in the OS layer.
 *
 * All the Rx packets are MLME-UNITDATA.ind signals, passed by the HIP core lib
 * in unifi_receive_event(). We do not need to allocate an skb and copy the
 * received packet because the HIP core lib has stored in memory allocated by
 * unifi_net_data_malloc(). Also, we can perform the 802.11 to Ethernet
 * translation in-place because we allocate the extra memory allocated in
 * unifi_net_data_malloc().
 *
 * If possible, the porting exercise should appropriately implement
 * unifi_net_data_malloc() and unifi_net_data_free() to save copies between
 * network and driver buffers.
 *
 * Also, skb_80211_to_ether() and skb_ether_to_80211() demonstrate the
 * conversion between Ethernet and 802.11 frames. They are Linux specific
 * but can be used as an example for the conversion.
 */

#include <linux/types.h>
#include <linux/etherdevice.h>
#include <linux/vmalloc.h>
#include "driver/unifi.h"
#include "driver/conversions.h"
#include "unifi_priv.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#include <linux/ieee80211.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
#include <net/ieee80211.h>
#else
#include <net/iw_handler.h>
#endif
#include <net/pkt_sched.h>

#ifndef ETH_P_PAE
#define ETH_P_PAE 0x888e
#endif

#define ETH_P_WAI 0x88b4

#define ALLOW_Q_PAUSE

#define ieee2host16(n)  __le16_to_cpu(n)
#define ieee2host32(n)  __le32_to_cpu(n)
#define host2ieee16(n)  __cpu_to_le16(n)
#define host2ieee32(n)  __cpu_to_le32(n)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
#ifdef UNIFI_NET_NAME
#define UF_ALLOC_NETDEV(_dev, _size, _name, _setup, _num_of_queues)     \
    do {                                                                \
        static char name[8];                                           \
        sprintf(name, "%s%s", UNIFI_NET_NAME, _name);                   \
        _dev = alloc_netdev_mq(_size, name, _setup, _num_of_queues);    \
    } while (0);
#else
#define UF_ALLOC_NETDEV(_dev, _size, _name, _setup, _num_of_queues)     \
    do {                                                                \
        _dev = alloc_etherdev_mq(_size, _num_of_queues);                \
    } while (0);
#endif /* UNIFI_NET_NAME */
#else
#ifdef UNIFI_NET_NAME
#define UF_ALLOC_NETDEV(_dev, _size, _name, _setup, _num_of_queues)     \
    do {                                                                \
        static char name[8];                                           \
        sprintf(name, "%s%s", UNIFI_NET_NAME, _name);                   \
        _dev = alloc_netdev(_size, name, _setup);                       \
    } while (0);
#else
#define UF_ALLOC_NETDEV(_dev, _size, _name, _setup, _num_of_queues)     \
    do {                                                                \
        _dev = alloc_etherdev(_size);                                   \
    } while (0);
#endif /* UNIFI_NET_NAME */
#endif /* LINUX_VERSION_CODE */


/* Wext handler is suported only if CSR_SUPPORT_WEXT is defined */
#ifdef CSR_SUPPORT_WEXT
extern struct iw_handler_def unifi_iw_handler_def;
#endif /* CSR_SUPPORT_WEXT */

static int uf_net_open(struct net_device *dev);
static int uf_net_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
static int uf_net_stop(struct net_device *dev);
static struct net_device_stats *uf_net_get_stats(struct net_device *dev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
static u16 uf_net_select_queue(struct net_device *dev, struct sk_buff *skb);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
static netdev_tx_t uf_net_xmit(struct sk_buff *skb, struct net_device *dev);
#else
static int uf_net_xmit(struct sk_buff *skb, struct net_device *dev);
#ifndef NETDEV_TX_OK
#define NETDEV_TX_OK        0
#endif
#ifndef NETDEV_TX_BUSY
#define NETDEV_TX_BUSY      1
#endif
#endif
static void uf_set_multicast_list(struct net_device *dev);

typedef int (*tx_signal_handler)(unifi_priv_t *priv, struct sk_buff *skb, const struct ethhdr *ehdr, CSR_PRIORITY priority);

#ifdef CONFIG_NET_SCHED
/*
 * Queueing Discipline Interface
 * Only used if kernel is configured with CONFIG_NET_SCHED
 */

/*
 * The driver uses the qdisc interface to buffer and control all
 * outgoing traffic. We create a root qdisc, register our qdisc operations
 * and later we create two subsiduary pfifo queues for the uncontrolled
 * and controlled ports.
 *
 * The network stack delivers all outgoing packets in our enqueue handler.
 * There, we classify the packet and decide whether to store it or drop it
 * (if the controlled port state is set to "discard").
 * If the packet is enqueued, the network stack call our dequeue handler.
 * There, we decide whether we can send the packet, delay it or drop it
 * (the controlled port configuration might have changed meanwhile).
 * If a packet is dequeued, then the network stack calls our hard_start_xmit
 * handler where finally we send the packet.
 *
 * If the hard_start_xmit handler fails to send the packet, we return
 * NETDEV_TX_BUSY and the network stack call our requeue handler where
 * we put the packet back in the same queue in came from.
 *
 */

struct uf_sched_data
{
    /* Traffic Classifier TBD */
    struct tcf_proto *filter_list;
    /* Our two queues */
    struct Qdisc *queues[UNIFI_TRAFFIC_Q_MAX];
};

struct uf_tx_packet_data {
    /* Queue the packet is stored in */
    unifi_TrafficQueue queue;
    /* QoS Priority determined when enqueing packet */
    CSR_PRIORITY priority;
    /* Debug */
    unsigned long host_tag;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
static int uf_qdiscop_enqueue(struct sk_buff *skb, struct Qdisc* qd);
static int uf_qdiscop_requeue(struct sk_buff *skb, struct Qdisc* qd);
static struct sk_buff *uf_qdiscop_dequeue(struct Qdisc* qd);
static void uf_qdiscop_reset(struct Qdisc* qd);
static void uf_qdiscop_destroy(struct Qdisc* qd);
static int uf_qdiscop_dump(struct Qdisc *qd, struct sk_buff *skb);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
static int uf_qdiscop_tune(struct Qdisc *qd, struct nlattr *opt);
static int uf_qdiscop_init(struct Qdisc *qd, struct nlattr *opt);
#else
static int uf_qdiscop_tune(struct Qdisc *qd, struct rtattr *opt);
static int uf_qdiscop_init(struct Qdisc *qd, struct rtattr *opt);
#endif
#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
/* queueing discipline operations */
static struct Qdisc_ops uf_qdisc_ops =
{
    .next = NULL,
    .cl_ops = NULL,
    .id = "UniFi Qdisc",
    .priv_size = sizeof(struct uf_sched_data),

    .enqueue = uf_qdiscop_enqueue,
    .dequeue = uf_qdiscop_dequeue,
    .requeue = uf_qdiscop_requeue,
    .drop = NULL, /* drop not needed since we are always the root qdisc */

    .init = uf_qdiscop_init,
    .reset = uf_qdiscop_reset,
    .destroy = uf_qdiscop_destroy,
    .change = uf_qdiscop_tune,

    .dump = uf_qdiscop_dump,
};
#endif /* LINUX_VERSION_CODE */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
#define UF_QDISC_CREATE_DFLT(_dev, _ops, _root)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#define UF_QDISC_CREATE_DFLT(_dev, _ops, _root)         \
    qdisc_create_dflt(dev, netdev_get_tx_queue(_dev, 0), _ops, _root)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
#define UF_QDISC_CREATE_DFLT(_dev, _ops, _root)         \
    qdisc_create_dflt(dev, _ops, _root)
#else
#define UF_QDISC_CREATE_DFLT(_dev, _ops, _root)         \
    qdisc_create_dflt(dev, _ops)
#endif /* LINUX_VERSION_CODE */

#endif /* CONFIG_NET_SCHED */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static const struct net_device_ops uf_netdev_ops =
{
    .ndo_open = uf_net_open,
    .ndo_stop = uf_net_stop,
    .ndo_start_xmit = uf_net_xmit,
    .ndo_do_ioctl = uf_net_ioctl,
    .ndo_get_stats = uf_net_get_stats, /* called by /proc/net/dev */
    .ndo_set_multicast_list = uf_set_multicast_list,
    .ndo_select_queue = uf_net_select_queue,
};
#endif

static u8 oui_rfc1042[P80211_OUI_LEN] = { 0x00, 0x00, 0x00 };
static u8 oui_8021h[P80211_OUI_LEN]   = { 0x00, 0x00, 0xf8 };


/* Callback for event logging to blocking clients */
static void netdev_mlme_event_handler(ul_client_t  *client,
                                      const u8 *sig_packed, int sig_len,
                                      const bulk_data_param_t *bulkdata,
                                      int dir);
/*
 * ---------------------------------------------------------------------------
 *  uf_alloc_netdevice
 *
 *      Allocate memory for the net_device and device private structs
 *      for this interface.
 *      Fill in the fields, but don't register the interface yet.
 *      We need to configure the UniFi first.
 *
 *  Arguments:
 *      sdio_dev        Pointer to SDIO context handle to use for all
 *                      SDIO ops.
 *      bus_id          A small number indicating the SDIO card position on the
 *                      bus. Typically this is the slot number, e.g. 0, 1 etc.
 *                      Valid values are 0 to MAX_UNIFI_DEVS-1.
 *
 *  Returns:
 *      Pointer to device private struct.
 *
 *  Notes:
 *      The net_device and device private structs are allocated together
 *      and should be freed by freeing the net_device pointer.
 * ---------------------------------------------------------------------------
 */
unifi_priv_t *
uf_alloc_netdevice(CsrSdioFunction *sdio_dev, int bus_id)
{
    struct net_device *dev;
    unifi_priv_t *priv;

    /*
     * Allocate netdevice struct, assign name template and
     * setup as an ethernet device.
     * The net_device and private structs are zeroed. Ether_setup() then
     * sets up ethernet handlers and values.
     * The RedHat 9 redhat-config-network tool doesn't recognise wlan* devices,
     * so use "eth*" (like other wireless extns drivers).
     */
    UF_ALLOC_NETDEV(dev, sizeof(unifi_priv_t), "%d", ether_setup, UNIFI_TRAFFIC_Q_MAX);
    if (dev == NULL) {
        return NULL;
    }


    /* Set up back pointer from priv to netdev */
    priv = netdev_priv(dev);
    priv->netdev = dev;

    /* Setup / override net_device fields */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    dev->netdev_ops = &uf_netdev_ops;
#else
    dev->open             = uf_net_open;
    dev->stop             = uf_net_stop;
    dev->hard_start_xmit  = uf_net_xmit;
    dev->do_ioctl         = uf_net_ioctl;

    /* called by /proc/net/dev */
    dev->get_stats = uf_net_get_stats;

    dev->set_multicast_list = uf_set_multicast_list;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    dev->select_queue       = uf_net_select_queue;
#endif
#endif

#ifdef CSR_SUPPORT_WEXT
    dev->wireless_handlers = &unifi_iw_handler_def;
#if IW_HANDLER_VERSION < 6
    dev->get_wireless_stats = unifi_get_wireless_stats;
#endif /* IW_HANDLER_VERSION */
#endif /* CSR_SUPPORT_WEXT */

    /* Use bus_id as instance number */
    priv->instance = bus_id;
    /* Store SDIO pointer to pass in the core */
    priv->sdio = sdio_dev;

    sdio_dev->driverData = (void*)priv;
    /* Consider UniFi to be uninitialised */
    priv->init_progress = UNIFI_INIT_NONE;

    priv->prev_queue = 0;

    /*
     * Initialise the clients structure array.
     * We do not need protection around ul_init_clients() because
     * the character device can not be used until uf_alloc_netdevice()
     * returns and Unifi_instances[bus_id]=priv is set, since unifi_open()
     * will return -ENODEV.
     */
    ul_init_clients(priv);

    /*
     * Register a new ul client to send the multicast list signals.
     * Note: priv->instance must be set before calling this.
     */
    priv->netdev_client = ul_register_client(priv,
                                             0,
                                             netdev_mlme_event_handler);
    if (priv->netdev_client == NULL) {
        unifi_error(priv,
                    "Failed to register a unifi client for background netdev processing\n");
        free_netdev(priv->netdev);
        return NULL;
    }
    unifi_trace(priv, UDBG2, "Netdev client (id:%d s:0x%X) is registered\n",
                priv->netdev_client->client_id, priv->netdev_client->sender_id);

    priv->sta_wmm_capabilities = 0;

    /*
     * Initialise the OS private struct.
     */
    /*
     * Instead of deciding in advance to use 11bg or 11a, we could do a more
     * clever scan on both radios.
     */
    if (use_5g) {
        priv->if_index = CSR_INDEX_5G;
        unifi_info(priv, "Using the 802.11a radio\n");
    } else {
        priv->if_index = CSR_INDEX_2G4;
    }

    /* Initialise bh thread structure */
    priv->bh_thread.thread_task = NULL;
    priv->bh_thread.block_thread = 1;
    init_waitqueue_head(&priv->bh_thread.wakeup_q);
    priv->bh_thread.wakeup_flag = 0;
    sprintf(priv->bh_thread.name, "uf_bh_thread");

    priv->connected = UnifiConnectedUnknown;  /* -1 unknown, 0 no, 1 yes */

#ifdef USE_DRIVER_LOCK
    init_MUTEX(&priv->lock);
#endif /* USE_DRIVER_LOCK */

    spin_lock_init(&priv->send_signal_lock);

    /* Create the Traffic Analysis workqueue */
    priv->unifi_workqueue = create_singlethread_workqueue("unifi_workq");
    if (priv->unifi_workqueue == NULL) {
        /* Deregister priv->netdev_client */
        ul_deregister_client(priv->netdev_client);
        free_netdev(priv->netdev);
        return NULL;
    }
    /* Create the Multicast Addresses list work structure */
    INIT_WORK(&priv->multicast_list_task, uf_multicast_list_wq);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#ifdef CONFIG_NET_SCHED
    /* Register the qdisc operations */
    register_qdisc(&uf_qdisc_ops);
#endif /* CONFIG_NET_SCHED */
#endif /* LINUX_VERSION_CODE */

    priv->ref_count = 1;


    priv->amp_client = NULL;
    priv->ptest_mode = 0;
    priv->wol_suspend = FALSE;

    INIT_LIST_HEAD(&priv->rx_uncontrolled_list);
    INIT_LIST_HEAD(&priv->rx_controlled_list);
    sema_init(&priv->rx_q_sem, 1);

    return priv;
} /* uf_alloc_netdevice() */



/*
 * ---------------------------------------------------------------------------
 *  uf_free_netdevice
 *
 *      Unregister the network device and free the memory allocated for it.
 *      NB This includes the memory for the priv struct.
 *
 *  Arguments:
 *      priv            Device private pointer.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
int
uf_free_netdevice(unifi_priv_t *priv)
{
    func_enter();

    if (!priv) {
        return -EINVAL;
    }

    /*
     * Free any buffers used for holding firmware
     */
    uf_release_firmware_files(priv);

#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
    if (priv->connection_config.mlmeAssociateReqInformationElements) {
        kfree(priv->connection_config.mlmeAssociateReqInformationElements);
    }
    priv->connection_config.mlmeAssociateReqInformationElements = NULL;
    priv->connection_config.mlmeAssociateReqInformationElementsLength = 0;

    if (priv->mib_data.length) {
        vfree(priv->mib_data.data);
    }
    priv->mib_data.data = NULL;
    priv->mib_data.length = 0;

#endif /* CSR_SUPPORT_SME && CSR_SUPPORT_WEXT*/

 
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#ifdef CONFIG_NET_SCHED
    /* Unregister the qdisc operations */
    unregister_qdisc(&uf_qdisc_ops);
#endif /* CONFIG_NET_SCHED */
#endif /* LINUX_VERSION_CODE */

    /* Cancel work items and destroy the workqueue */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
    cancel_work_sync(&priv->multicast_list_task);
#endif
    flush_workqueue(priv->unifi_workqueue);
    destroy_workqueue(priv->unifi_workqueue);

    /* Free the netdev struct and priv, which are all one lump. */
    free_netdev(priv->netdev);

    func_exit();
    return 0;
} /* uf_free_netdevice() */


/*
 * ---------------------------------------------------------------------------
 *  uf_net_open
 *
 *      Called when userland does "ifconfig wlan0 up".
 *
 *  Arguments:
 *      dev             Device pointer.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static int
uf_net_open(struct net_device *dev)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);

    func_enter();

    /* If we haven't finished UniFi initialisation, we can't start */
    if (priv->init_progress != UNIFI_INIT_COMPLETED) {
        unifi_trace(priv, UDBG1, "%s: unifi not ready, failing net_open\n", __FUNCTION__);
        return -EINVAL;
    }

#if (defined CSR_NATIVE_LINUX) && (defined UNIFI_SNIFF_ARPHRD)
    /*
     * To sniff, the user must do "iwconfig mode monitor", which sets
     * priv->wext_conf.mode to IW_MODE_MONITOR.
     * Then he/she must do "ifconfig ethn up", which calls this fn.
     * There is no point in starting the sniff with SNIFFJOIN until
     * this point.
     */
    if (priv->wext_conf.mode == IW_MODE_MONITOR) {
        int err;
        err = uf_start_sniff(priv);
        if (err) {
            return err;
        }
        netif_carrier_on(dev);
    }
#endif

    UF_NETIF_TX_START_ALL_QUEUES(dev);

    func_exit();
    return 0;
} /* uf_net_open() */


static int
uf_net_stop(struct net_device *dev)
{
#ifdef CSR_NATIVE_LINUX
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
#endif

    func_enter();

#ifdef CSR_NATIVE_LINUX
    /* Stop sniffing if in Monitor mode */
    if (priv->wext_conf.mode == IW_MODE_MONITOR) {
        if (priv->card) {
            int err;
            err = unifi_reset_state(priv, dev->dev_addr, 1);
            if (err) {
                return err;
            }
        }
    }
#endif

    UF_NETIF_TX_STOP_ALL_QUEUES(dev);

    func_exit();
    return 0;
} /* uf_net_stop() */


/* This is called after the WE handlers */
static int
uf_net_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    int rc;

    rc = -EOPNOTSUPP;

    return rc;
} /* uf_net_ioctl() */



static struct net_device_stats *
uf_net_get_stats(struct net_device *dev)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);

    return &priv->stats;
} /* uf_net_get_stats() */


static CSR_PRIORITY 
get_packet_priority(unifi_priv_t *priv, struct sk_buff *skb, const struct ethhdr *ehdr)
{
    CSR_PRIORITY priority;
    const int proto = ntohs(ehdr->h_proto);

    func_enter();

    if (((priv->sta_wmm_capabilities & QOS_CAPABILITY_WMM_ENABLED) == 0) ||
        ((ehdr->h_dest[0] & 0x01) == 0x01)) {
        priority = CSR_CONTENTION;
    }
    else {

        priority = skb->priority >> 5;

        if (priority == 0) {

            unifi_trace(priv, UDBG2, "proto = %x\n", proto);

            switch (proto) {
              case 0x0800:        /* IPv4 */
              case 0x814C:        /* SNMP */
              case 0x880C:        /* GSMP */
                  priority = (CSR_PRIORITY) (skb->data[1 + ETH_HLEN] >> 5);
                break;

              case 0x8100:        /* VLAN */
                  priority = (CSR_PRIORITY) (skb->data[0 + ETH_HLEN] >> 5);
                break;

              case 0x86DD:        /* IPv6 */
                priority = (CSR_PRIORITY) ((skb->data[0 + ETH_HLEN] & 0x0E) >> 1);
                break;

              default:
                priority = 0;
                break;
            }
        }
    }

    unifi_trace(priv, UDBG5, "priority = %x\n", priority);

    func_exit();
    return priority;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
/*
 * ---------------------------------------------------------------------------
 *  uf_net_select_queue
 *
 *      Called by the kernel to select which queue to put the packet in
 *
 *  Arguments:
 *      dev             Device pointer
 *      skb             Packet
 *
 *  Returns:
 *      Queue index
 * ---------------------------------------------------------------------------
 */
static u16
uf_net_select_queue(struct net_device *dev, struct sk_buff *skb)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct ethhdr ehdr;
    unifi_TrafficQueue queue;
    int proto;
    CSR_PRIORITY priority;

    func_enter();

    memcpy(&ehdr, skb->data, ETH_HLEN);
    proto = ntohs(ehdr.h_proto);

    /* 802.1x - apply controlled/uncontrolled port rules */
    if ((proto != ETH_P_PAE)
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
            && (proto != ETH_P_WAI)
#endif
            ) {
        /* queues 0 - 3 */
        priority = get_packet_priority(priv, skb, &ehdr);
        queue = unifi_frame_priority_to_queue(priority);
    } else {
        /* queue 4 */
        queue = UNIFI_TRAFFIC_Q_EAPOL;
    }

    unifi_trace(priv, UDBG5, "uf_net_select_queue: queue %d skb %x\n", queue, skb);

    func_exit();
    return (u16)queue;
} /* uf_net_select_queue() */
#endif

int
skb_ether_to_80211(struct net_device *dev, struct sk_buff *skb, int proto)
{
    llc_snap_hdr_t *snap;

    /* step 1: classify ether frame, DIX or 802.3? */

    if (proto < 0x600) {
        /* codes <= 1500 reserved for 802.3 lengths */
        /* it's 802.3, pass ether payload unchanged,  */
        unifi_trace(netdev_priv(dev), UDBG3, "802.3 len: %d\n", skb->len);

        /*   leave off any PAD octets.  */
        skb_trim(skb, proto);
    } else if (proto == ETH_P_8021Q) {
        /* Store the VLAN SNAP (should be 87-65). */
        u16 vlan_snap = *(u16*)skb->data;
        /* Add AA-AA-03-00-00-00 */
        snap = (llc_snap_hdr_t *)skb_push(skb, 4);
        snap->dsap = snap->ssap = 0xAA;
        snap->ctrl = 0x03;
        memcpy(snap->oui, oui_rfc1042, P80211_OUI_LEN);

        /* Add AA-AA-03-00-00-00 */
        snap = (llc_snap_hdr_t *)skb_push(skb, 10);
        snap->dsap = snap->ssap = 0xAA;
        snap->ctrl = 0x03;
        memcpy(snap->oui, oui_rfc1042, P80211_OUI_LEN);

        /* Add the VLAN specific information */
        snap->protocol = htons(proto);
        *(u16*)(snap + 1) = vlan_snap;

    } else {
        /* it's DIXII, time for some conversion */
        unifi_trace(netdev_priv(dev), UDBG3, "DIXII len: %d\n", skb->len);

        /* tack on SNAP */
        snap = (llc_snap_hdr_t *)skb_push(skb, sizeof(llc_snap_hdr_t));
        snap->dsap = snap->ssap = 0xAA;
        snap->ctrl = 0x03;
        /* Use the appropriate OUI. */
        if ((proto == ETH_P_AARP) || (proto == ETH_P_IPX)) {
            memcpy(snap->oui, oui_8021h, P80211_OUI_LEN);
        } else {
            memcpy(snap->oui, oui_rfc1042, P80211_OUI_LEN);
        }
        snap->protocol = htons(proto);
    }

    return 0;
} /* skb_ether_to_80211() */

#ifdef CSR_SUPPORT_SME
static int
_identify_sme_unidata_ind(unifi_priv_t *priv,
                          const CsrInt8 *oui, CsrUint16 protocol,
                          const u8 *packed_signal, int signal_len,
                          const CSR_SIGNAL *signal,
                          bulk_data_param_t *bulkdata)
{
    const CSR_MA_UNITDATA_INDICATION *ind = &signal->u.MaUnitdataIndication;
    int r;
    CsrUint8 i;

    unifi_trace(priv, UDBG5,
                "_identify_sme_unidata_ind -->\n");
    for (i = 0; i < MAX_MA_UNIDATA_IND_FILTERS; i++) {
        if (priv->sme_unidata_ind_filters[i].in_use) {
            if (!memcmp(oui, priv->sme_unidata_ind_filters[i].oui, 3) &&
                (protocol == priv->sme_unidata_ind_filters[i].protocol)) {

                /* Send to client */
                if (priv->sme_cli) {
                    /*
                     * Pass the packet to the SME, using unifi_sys_ma_unitdata_ind().
                     * The frame needs to be converted according to the encapsulation.
                     */
                    unifi_trace(priv, UDBG1,
                                "_identify_sme_unidata_ind: handle=%d, encap=%d, proto=%d\n",
                                i, priv->sme_unidata_ind_filters[i].encapsulation,
                                priv->sme_unidata_ind_filters[i].protocol);
                    if (priv->sme_unidata_ind_filters[i].encapsulation == unifi_Ethernet) {
                        struct sk_buff *skb;
                        /* The translation is performed on skb... */
                        skb = (struct sk_buff*)bulkdata->d[0].os_net_buf_ptr;
                        skb->len = bulkdata->d[0].data_length;

                        unifi_trace(priv, UDBG1,
                                    "_identify_sme_unidata_ind: skb_80211_to_ether -->\n");
                        r = skb_80211_to_ether(priv, skb, ind->Da.x, ind->Sa.x,
                                               packed_signal, signal_len,
                                               signal, bulkdata);
                        unifi_trace(priv, UDBG1,
                                    "_identify_sme_unidata_ind: skb_80211_to_ether <--\n");
                        if (r) {
                            return -EINVAL;
                        }

                        /* ... but we indicate buffer and length */
                        bulkdata->d[0].os_data_ptr = skb->data;
                        bulkdata->d[0].data_length = skb->len;
                    } else {
                        /* Add the MAC addresses before the SNAP */
                        bulkdata->d[0].os_data_ptr -= 2*ETH_ALEN;
                        bulkdata->d[0].data_length += 2*ETH_ALEN;
                        memcpy((void*)bulkdata->d[0].os_data_ptr, ind->Da.x, ETH_ALEN);
                        memcpy((void*)bulkdata->d[0].os_data_ptr + ETH_ALEN, ind->Sa.x, ETH_ALEN);
                    }

                    unifi_trace(priv, UDBG1,
                                "_identify_sme_unidata_ind: unifi_sys_ma_unitdata_ind -->\n");
                    unifi_sys_ma_unitdata_ind(priv->smepriv,
                                              priv->sme_unidata_ind_filters[i].appHandle,
                                              i,
                                              bulkdata->d[0].data_length,
                                              bulkdata->d[0].os_data_ptr,
                                              0, ind->ReceptionStatus,
                                              ind->Priority,
                                              ind->ServiceClass);
                    unifi_trace(priv, UDBG1,
                                "_identify_sme_unidata_ind: unifi_sys_ma_unitdata_ind <--\n");
                }

                return 0;
            }
        }
    }

    return -1;
}
#endif /* CSR_SUPPORT_SME */

/*
 * ---------------------------------------------------------------------------
 *  skb_80211_to_ether
 *
 *      Make sure the received frame is in Ethernet (802.3) form.
 *      De-encapsulates SNAP if necessary, adds a ethernet header.
 *
 *  Arguments:
 *      payload         Pointer to packet data received from UniFi.
 *      payload_length  Number of bytes of data received from UniFi.
 *      daddr           Destination MAC address.
 *      saddr           Source MAC address.
 *
 *  Returns:
 *      0 on success, -1 if the packet is bad and should be dropped,
 *      1 if the packet was forwarded to the SME or AMP client.
 * ---------------------------------------------------------------------------
 */
int
skb_80211_to_ether(unifi_priv_t *priv, struct sk_buff *skb,
                   const unsigned char *daddr, const unsigned char *saddr,
                   const u8 *packed_signal, int signal_len,
                   const CSR_SIGNAL *signal,
                   bulk_data_param_t *bulkdata)
{
    unsigned char *payload;
    int payload_length;
    struct ethhdr *eth;
    llc_snap_hdr_t *snap;
#define UF_VLAN_LLC_HEADER_SIZE     18
    static const u8 vlan_inner_snap[] = { 0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00 };

    payload = skb->data;
    payload_length = skb->len;

    snap = (llc_snap_hdr_t *)payload;
    eth  = (struct ethhdr *)payload;

    /*
     * Test for the various encodings
     */
    if ((payload_length >= sizeof(llc_snap_hdr_t)) &&
        (snap->dsap == 0xAA) &&
        (snap->ssap == 0xAA) &&
        (snap->ctrl == 0x03) &&
        (snap->oui[0] == 0) &&
        (snap->oui[1] == 0) &&
        ((snap->oui[2] == 0) || (snap->oui[2] == 0xF8)))
    {
        /* AppleTalk AARP (2) or IPX SNAP */
        if ((snap->oui[2] == 0) &&
            ((ntohs(snap->protocol) == ETH_P_AARP) || (ntohs(snap->protocol) == ETH_P_IPX)))
        {
            u16 len;

            unifi_trace(priv, UDBG3, "%s len: %d\n",
                  (ntohs(snap->protocol) == ETH_P_AARP) ? "ETH_P_AARP" : "ETH_P_IPX",
                  payload_length);
            /* Add 802.3 header and leave full payload */
            len = htons(skb->len);
            memcpy(skb_push(skb, 2), &len, 2);
            memcpy(skb_push(skb, ETH_ALEN), saddr, ETH_ALEN);
            memcpy(skb_push(skb, ETH_ALEN), daddr, ETH_ALEN);

            return 0;
        }
        /* VLAN-tagged IP */
        if ((snap->oui[2] == 0) && (ntohs(snap->protocol) == ETH_P_8021Q))
        {
            /*
             * The translation doesn't change the packet length, so is done in-place.
             *
             * Example header (from Std 802.11-2007 Annex M):
             * AA-AA-03-00-00-00-81-00-87-65-AA-AA-03-00-00-00-08-06
             * -------SNAP-------p1-p1-ll-ll-------SNAP--------p2-p2
             * dd-dd-dd-dd-dd-dd-aa-aa-aa-aa-aa-aa-p1-p1-ll-ll-p2-p2
             * dd-dd-dd-dd-dd-dd-aa-aa-aa-aa-aa-aa-81-00-87-65-08-06
             */
            u16 vlan_snap;

            if (payload_length < UF_VLAN_LLC_HEADER_SIZE) {
                unifi_warning(priv, "VLAN SNAP header too short: %d bytes\n", payload_length);
                return -1;
            }

            if (memcmp(payload + 10, vlan_inner_snap, 6)) {
                unifi_warning(priv, "VLAN malformatted SNAP header.\n");
                return -1;
            }

            unifi_trace(priv, UDBG3, "VLAN SNAP: %02x-%02x\n", payload[8], payload[9]);
            unifi_trace(priv, UDBG3, "VLAN len: %d\n", payload_length);

            /* Create the 802.3 header */

            vlan_snap = *((u16*)(payload + 8));

            /* Create LLC header without byte-swapping */
            eth->h_proto = snap->protocol;

            memcpy(eth->h_dest, daddr, ETH_ALEN);
            memcpy(eth->h_source, saddr, ETH_ALEN);
            *(u16*)(eth + 1) = vlan_snap;
            return 0;
        }

#ifdef CSR_SUPPORT_SME
        /*
         * The SME ethernet filters are currently applied only to EAP packets.
         */
        if (snap->oui[2] == 0)
        {
            _identify_sme_unidata_ind(priv,
                                      snap->oui, ntohs(snap->protocol),
                                      packed_signal, signal_len,
                                      signal,
                                      bulkdata);
        }
#endif /* CSR_SUPPORT_SME */

        /* it's a SNAP + RFC1042 frame */
        unifi_trace(priv, UDBG3, "SNAP+RFC1042 len: %d\n", payload_length);

        /* chop SNAP+llc header from skb. */
        skb_pull(skb, sizeof(llc_snap_hdr_t));

        /* create 802.3 header at beginning of skb. */
        eth = (struct ethhdr *)skb_push(skb, ETH_HLEN);
        memcpy(eth->h_dest, daddr, ETH_ALEN);
        memcpy(eth->h_source, saddr, ETH_ALEN);
        /* Copy protocol field without byte-swapping */
        eth->h_proto = snap->protocol;
    } else {
        u16 len;

#ifdef CSR_SUPPORT_SME
        int r;

        r = _identify_sme_unidata_ind(priv,
                                      snap->oui, ntohs(snap->protocol),
                                      packed_signal, signal_len,
                                      signal,
                                      bulkdata);
        if (r == 0) {
            unifi_net_data_free(priv, &bulkdata->d[0]);
            return 1;
        }
#endif /* CSR_SUPPORT_SME */

        /* Add 802.3 header and leave full payload */
        len = htons(skb->len);
        memcpy(skb_push(skb, 2), &len, 2);
        memcpy(skb_push(skb, ETH_ALEN), saddr, ETH_ALEN);
        memcpy(skb_push(skb, ETH_ALEN), daddr, ETH_ALEN);
    }

    return 0;
} /* skb_80211_to_ether() */


static unifi_PortAction verify_port(unifi_priv_t *priv,
                                    unsigned char *address,
                                    int queue)
{
#ifdef CSR_NATIVE_LINUX
    if (queue == UF_CONTROLLED_PORT_Q) {
        return priv->wext_conf.block_controlled_port;
    } else {
        return unifi_8021xPortOpen;
    }
#else
    return uf_sme_port_state(priv, address, queue);
#endif
}


/*
 * ---------------------------------------------------------------------------
 *  send_ma_unidata_request
 *  send_mlme_eapol_request
 *
 *      These functions send a data packet to UniFi for transmission.
 *      EAP protocol packets are sent using send_mlme_eapol_request(),
 *      all other packets are sent using send_ma_unidata_request().
 *
 *  Arguments:
 *      priv            Pointer to device private context struct
 *      skb             Socket buffer containing data packet to transmit
 *      ehdr            Pointer to Ethernet header within skb.
 *
 *  Returns:
 *      Zero on success or error code.
 * ---------------------------------------------------------------------------
 */
static int
send_ma_unidata_request(unifi_priv_t *priv, struct sk_buff *skb, const struct ethhdr *ehdr, CSR_PRIORITY priority)
{
    CSR_SIGNAL signal;
    CSR_MA_UNITDATA_REQUEST *req = &signal.u.MaUnitdataRequest;
    bulk_data_param_t bulkdata;
    int r;
    const int proto = ntohs(ehdr->h_proto);
    static unsigned long tag = 1;

    req->Priority = priority;

    if (priority == CSR_CONTENTION) {
        req->ServiceClass = CSR_REORDERABLE_MULTICAST;
    }
    else {
        req->ServiceClass = CSR_QOS_ACK;
    }
    
    /* Remove the ethernet header and add a SNAP header if necessary */
    if (skb_ether_to_80211(priv->netdev, skb, proto) != 0) {
        /* convert failed */
        unifi_error(priv, "ether_to_80211 failed.\n");
        return -1;
    }
    unifi_trace(priv, UDBG3, "Tx Frame with Priority: %d\n", req->Priority);

#if 0
    printk("after SNAP hdr:\n");
    dump(skb->data, (skb->len > 32) ? 32 : skb->len);
#endif

    memcpy(req->Da.x, ehdr->h_dest, ETH_ALEN);
    memcpy(req->Sa.x, ehdr->h_source, ETH_ALEN);

#if 0
    printk("MA-UNITDATA.req: %d bytes\n", skb->len);
    printk(" Da %02X:%02X:%02X:%02X:%02X:%02X, Sa %02X:%02X:%02X:%02X:%02X:%02X\n",
           req.Da.x[0], req.Da.x[1], req.Da.x[2],
           req.Da.x[3], req.Da.x[4], req.Da.x[5],
           req.Sa.x[0], req.Sa.x[1], req.Sa.x[2],
           req.Sa.x[3], req.Sa.x[4], req.Sa.x[5]);
#endif

    req->RoutingInformation = CSR_NULL_RT; /* always set to zero */

    req->HostTag = tag++;

    signal.SignalPrimitiveHeader.SignalId = CSR_MA_UNITDATA_REQUEST_ID;
    bulkdata.d[0].os_data_ptr = skb->data;
    bulkdata.d[0].os_net_buf_ptr = (unsigned char*)skb;
    bulkdata.d[0].data_length = skb->len;
    bulkdata.d[1].os_data_ptr = NULL;
    bulkdata.d[1].os_net_buf_ptr = NULL;
    bulkdata.d[1].data_length = 0;

#ifdef CSR_SUPPORT_SME
    /* Notify the TA module for the Tx frame */
    unifi_ta_sample(priv->card, unifi_TrafficTx,
                    &bulkdata.d[0], ehdr->h_source,
                    priv->netdev->dev_addr,
                    jiffies_to_msecs(jiffies),
                    0);     /* rate is unknown on tx */
#endif /* CSR_SUPPORT_SME */

    /* Send UniFi msg */
    r = uf_ma_unitdata(priv, priv->netdev_client, &signal, &bulkdata);
    unifi_trace(priv, UDBG3, "UNITDATA result code = %d\n", r);

    return r;
} /* send_ma_unidata_request() */

static int
send_mlme_eapol_request(unifi_priv_t *priv, struct sk_buff *skb, const struct ethhdr *ehdr, CSR_PRIORITY priority)
{
    CSR_SIGNAL signal;
    CSR_MLME_EAPOL_REQUEST *req = &signal.u.MlmeEapolRequest;
    bulk_data_param_t bulkdata;
    int r;
    int proto = ntohs(ehdr->h_proto);

    /* Remove the ethernet header and add a SNAP header if necessary */
    if (skb_ether_to_80211(priv->netdev, skb, proto) != 0) {
        /* convert failed */
        unifi_error(priv, "ether_to_80211 failed.\n");
        return -1;
    }

#if 0
    printk("after SNAP hdr:\n");
    dump(skb->data, (skb->len > 32) ? 32 : skb->len);
#endif

    memcpy(req->Da.x, &ehdr->h_dest, ETH_ALEN);
    memcpy(req->Sa.x, &ehdr->h_source, ETH_ALEN);

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_EAPOL_REQUEST_ID;
    bulkdata.d[0].os_data_ptr = skb->data;
    bulkdata.d[0].os_net_buf_ptr = (unsigned char*)skb;
    bulkdata.d[0].data_length = skb->len;
    bulkdata.d[1].os_data_ptr = NULL;
    bulkdata.d[1].os_net_buf_ptr = NULL;
    bulkdata.d[1].data_length = 0;

#ifdef CSR_SUPPORT_SME
    /* Notify the TA module for the Tx frame */
    unifi_ta_sample(priv->card, unifi_TrafficTx,
                    &bulkdata.d[0], ehdr->h_source,
                    priv->netdev->dev_addr,
                    jiffies_to_msecs(jiffies),
                    0);     /* rate is unknown on tx */
#endif /* CSR_SUPPORT_SME */

    /* Send to UniFi. This does not block for the reply. */
    r = uf_mlme_eapol(priv, priv->netdev_client, &signal, &bulkdata);
    unifi_trace(priv, UDBG3, "EAPOL result code = %d\n", r);

    return r;
} /* send_mlme_eapol_request() */


/*
 * ---------------------------------------------------------------------------
 *  uf_net_xmit
 *
 *      This function is called by the higher level stack to transmit an
 *      ethernet packet.
 *
 *  Arguments:
 *      skb     Ethernet packet to send.
 *      dev     Pointer to the linux net device.
 *
 *  Returns:
 *      0   on success (packet was consumed, not necessarily transmitted)
 *      1   if packet was requeued
 *     -1   on error
 *
 *
 *  Notes:
 *      The controlled port is handled in the qdisc dequeue handler.
 * ---------------------------------------------------------------------------
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
static netdev_tx_t
#else
static int
#endif
uf_net_xmit(struct sk_buff *skb, struct net_device *dev)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct ethhdr ehdr;
    int proto, port;
    int result;
    static tx_signal_handler tx_handler;
    CSR_PRIORITY priority;
#if !defined (CONFIG_NET_SCHED) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28))
    unifi_PortAction port_action;
#endif /* CONFIG_NET_SCHED */

    func_enter();

    unifi_trace(priv, UDBG5, "unifi_net_xmit: skb = %x\n", skb);
#if 0
    printk("xmit %d bytes\n", skb->len);
    dump(skb->data, skb->len);
#endif

    memcpy(&ehdr, skb->data, ETH_HLEN);
    proto = ntohs(ehdr.h_proto);
    priority = get_packet_priority(priv, skb, &ehdr);

    /* 802.1x - apply controlled/uncontrolled port rules */
    if ((proto != ETH_P_PAE)
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
            && (proto != ETH_P_WAI)
#endif
            ) {
        tx_handler = send_ma_unidata_request;
        port = UF_CONTROLLED_PORT_Q;
    } else {
        /* queue 4 */
        tx_handler = send_mlme_eapol_request;
        port = UF_UNCONTROLLED_PORT_Q;
    }

    /* Remove the ethernet header */
    skb_pull(skb, ETH_HLEN);

#if defined (CONFIG_NET_SCHED) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28))
    result = tx_handler(priv, skb, &ehdr, priority);
#else
    /* Uncontrolled port rules apply */
    port_action = verify_port(priv, ehdr.h_dest, port);

    if (port_action == unifi_8021xPortOpen) {
        result = tx_handler(priv, skb, &ehdr, priority);
    } else {

        /* Discard or block the packet if necessary */
        if (port_action == unifi_8021xPortClosedBlock) {
            /* We can not send the packet now, put it back to the queue */
            unifi_trace(priv, UDBG5,
                        "uf_net_xmit: %s controlled port blocked\n",
                        port ? "" : "un");
            result = NETDEV_TX_BUSY;
        } else {
            unifi_trace(priv, UDBG5,
                        "uf_net_xmit: %s controlled port closed\n",
                        port ? "" : "un");
            priv->stats.tx_dropped++;
            kfree_skb(skb);

            func_exit();
            return NETDEV_TX_OK;
        }
    }
#endif /* CONFIG_NET_SCHED */

    if (result == NETDEV_TX_OK) {

        dev->trans_start = jiffies;

        /*
         * Should really count tx stats in the UNITDATA.status signal but
         * that doesn't have the length.
         */
        priv->stats.tx_packets++;
        /* count only the packet payload */
        priv->stats.tx_bytes += skb->len;

    } else if (result < 0) {

        /* Failed to send: fh queue was full, and the skb was discarded.
         * Return OK to indicate that the buffer was consumed, to stop the
         * kernel re-transmitting the freed buffer.
         */
        unifi_trace(priv, UDBG5, "unifi_net_xmit: packet dropped, queue full\n");
        priv->stats.tx_dropped++;
        result = NETDEV_TX_OK;
    }

    /* The skb will have been freed by send_XXX_request() */

    func_exit();
    return result;
} /* uf_net_xmit() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_pause_xmit
 *  unifi_restart_xmit
 *
 *      These functions are called from the UniFi core to control the flow
 *      of packets from the upper layers.
 *      unifi_pause_xmit() is called when the internal queue is full and
 *      should take action to stop unifi_ma_unitdata() being called.
 *      When the queue has drained, unifi_restart_xmit() will be called to
 *      re-enable the flow of packets for transmission.
 *
 *  Arguments:
 *      ospriv          OS private context pointer.
 *
 *  Returns:
 *      unifi_pause_xmit() is called from interrupt context.
 * ---------------------------------------------------------------------------
 */
void
unifi_pause_xmit(void *ospriv, unifi_TrafficQueue queue)
{
    unifi_priv_t *priv = ospriv;

    func_enter();
    unifi_trace(priv, UDBG2, "Stopping queue %d\n", queue);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    netif_stop_subqueue(priv->netdev, (u16)queue);
#else
#ifdef ALLOW_Q_PAUSE
    unifi_trace(priv, UDBG2, "Stopping netif\n");
    if (netif_running(priv->netdev)) {
        UF_NETIF_TX_STOP_ALL_QUEUES(priv->netdev);
    }
#else
    if (net_is_tx_q_paused(priv, queue)) {
        unifi_trace(priv, UDBG2, "Queue already stopped\n");
        return;
    }
    net_tx_q_pause(priv, queue);
#endif
#endif

    func_exit();

} /* unifi_pause_xmit() */


void
unifi_restart_xmit(void *ospriv, unifi_TrafficQueue queue)
{
    unifi_priv_t *priv = ospriv;

    func_enter();
    unifi_trace(priv, UDBG2, "Waking queue %d\n", queue);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    netif_wake_subqueue(priv->netdev, (u16)queue);
#else
#ifdef ALLOW_Q_PAUSE
    /* Need to supply queue number depending on Kernel support */
    if (netif_running(priv->netdev)) {
        UF_NETIF_TX_WAKE_ALL_QUEUES(priv->netdev);
    }
#else
    if (!(net_is_tx_q_paused(priv, queue))) {
        unifi_trace(priv, UDBG2, "Queue already running\n");
        return;
    }
    net_tx_q_unpause(priv, queue);
#endif
#endif

    func_exit();
} /* unifi_restart_xmit() */


static void
process_rx_pending_queue(unifi_priv_t *priv, int queue,
                         unifi_MACAddress source_address,
                         int indicate)
{
    rx_buffered_packets_t *rx_q_item;
    struct list_head *rx_list;
    struct list_head *n;
    struct list_head *l_h;
    struct net_device *dev = priv->netdev;
    static const unifi_MACAddress broadcast_address = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

    if (queue == UF_CONTROLLED_PORT_Q) {
        rx_list = &priv->rx_controlled_list;
    } else {
        rx_list = &priv->rx_uncontrolled_list;
    }

    down(&priv->rx_q_sem);
    list_for_each_safe(l_h, n, rx_list) {
        rx_q_item = list_entry(l_h, rx_buffered_packets_t, q);

        /* Validate against the source address */
        if (memcmp(broadcast_address.data, source_address.data, ETH_ALEN) &&
            memcmp(rx_q_item->sa.data, source_address.data, ETH_ALEN)) {

            unifi_trace(priv, UDBG2,
                        "proccess_rx: Skipping sa=%02X%02X%02X%02X%02X%02X skb=%p, bulkdata=%p\n",
                        rx_q_item->sa.data[0], rx_q_item->sa.data[1],
                        rx_q_item->sa.data[2], rx_q_item->sa.data[3],
                        rx_q_item->sa.data[4], rx_q_item->sa.data[5],
                        rx_q_item->skb, &rx_q_item->bulkdata);
            continue;
        }

        list_del(l_h);


        unifi_trace(priv, UDBG2,
                    "proccess_rx: Was Blocked skb=%p, bulkdata=%p\n",
                    rx_q_item->skb, &rx_q_item->bulkdata);

        if (indicate) {
            /* Pass SKB up the stack */
            netif_rx(rx_q_item->skb);
            if (dev != NULL) {
                dev->last_rx = jiffies;
            }

            /* Bump the rx stats */
            priv->stats.rx_packets++;
            priv->stats.rx_bytes += rx_q_item->bulkdata.data_length;
        } else {
            priv->stats.rx_dropped++;
            unifi_net_data_free(priv, &rx_q_item->bulkdata);
        }

        /* It is our resposibility to free the Rx structure object. */
        kfree(rx_q_item);
    }
    up(&priv->rx_q_sem);
}

/*
 * ---------------------------------------------------------------------------
 *  uf_resume_data_plane
 *
 *      Is called when the (un)controlled port is set to open,
 *      to notify the network stack to schedule for transmission
 *      any packets queued in the qdisk while port was closed and
 *      indicated to the stack any packets buffered in the Rx queues.
 *
 *  Arguments:
 *      priv        Pointer to device private struct
 *
 *  Returns:
 * ---------------------------------------------------------------------------
 */
void
uf_resume_data_plane(unifi_priv_t *priv, int queue,
                     unifi_MACAddress peer_address)
{
    unifi_trace(priv, UDBG2, "Resuming netif\n");

#ifdef CONFIG_NET_SCHED
    if (netif_running(priv->netdev)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
        netif_tx_schedule_all(priv->netdev);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
        netif_schedule_queue(netdev_get_tx_queue(priv->netdev, 0));
#else
        netif_schedule(priv->netdev);
#endif /* LINUX_VERSION_CODE */
    }
#endif

    process_rx_pending_queue(priv, queue, peer_address, 1);

} /* uf_resume_data_plane() */


void uf_free_pending_rx_packets(unifi_priv_t *priv,
                                int queue,
                                unifi_MACAddress peer_address)
{
    process_rx_pending_queue(priv, queue, peer_address, 0);

} /* uf_free_pending_rx_packets() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_rx
 *
 *      Reformat a UniFi data received packet into a p80211 packet and
 *      pass it up the protocol stack.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
unifi_rx(unifi_priv_t *priv, const u8 *packed_signal, int signal_len,
         CSR_SIGNAL *signal, bulk_data_param_t *bulkdata)
{
    struct net_device *dev = priv->netdev;
    const CSR_MA_UNITDATA_INDICATION *ind = &signal->u.MaUnitdataIndication;
    struct sk_buff *skb;
    unifi_PortAction port_action;
    int r;
    int proto;
    int queue;

    func_enter();

    if (bulkdata->d[0].data_length == 0) {
        unifi_warning(priv, "rx: MA-UNITDATA indication with zero bulk data\n");
        func_exit();
        return;
    }

#ifdef CSR_SUPPORT_SME
    /* Notify the TA module for the Rx frame */
    unifi_ta_sample(priv->card, unifi_TrafficRx,
                    &bulkdata->d[0],
                    ind->Sa.x,
                    priv->netdev->dev_addr,
                    jiffies_to_msecs(jiffies),
                    ind->Rate);
#endif /* CSR_SUPPORT_SME */

    skb = (struct sk_buff*)bulkdata->d[0].os_net_buf_ptr;
    skb->len = bulkdata->d[0].data_length;

    /*
     * De-encapsulate any SNAP header and
     * prepend an ethernet header so that the skb manipulation and ARP
     * stuff works.
     */
    r = skb_80211_to_ether(priv, skb, ind->Da.x, ind->Sa.x,
                           packed_signal, signal_len,
                           signal,
                           bulkdata);
    if (r)
    {
        if (r == -1) {
            /* Drop the packet and return */
            priv->stats.rx_errors++;
            priv->stats.rx_frame_errors++;
            unifi_net_data_free(priv, &bulkdata->d[0]);
            unifi_notice(priv, "unifi_rx: Discard unknown frame.\n");
        }
        func_exit();
        return;
    }

    /* Now we look like a regular ethernet frame */

    /* Fill in SKB meta data */
    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    proto = ntohs(skb->protocol);

    /* Test for an overlength frame */
    if (skb->len > (dev->mtu + ETH_HLEN)) {
        /* A bogus length ethfrm has been encap'd. */
        /* Is someone trying an oflow attack? */
        unifi_error(priv, "%s: oversize frame (%d > %d)\n",
                    dev->name,
                    skb->len, dev->mtu + ETH_HLEN);

        /* Drop the packet and return */
        priv->stats.rx_errors++;
        priv->stats.rx_length_errors++;
        unifi_net_data_free(priv, &bulkdata->d[0]);
        func_exit();
        return;
    }

    /* Apply 802.1x (un)controlled port */
    if ((proto != ETH_P_PAE)
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
         && (proto != ETH_P_WAI)
#endif
       ) {
        queue = UF_CONTROLLED_PORT_Q;
    } else {
        queue = UF_UNCONTROLLED_PORT_Q;
    }

    port_action = verify_port(priv, (unsigned char*)ind->Sa.x, queue);

    if (port_action == unifi_8021xPortClosedDiscard) {
        /* Drop the packet and return */
        priv->stats.rx_dropped++;
        unifi_net_data_free(priv, &bulkdata->d[0]);
        unifi_notice(priv,
                     "unifi_rx: Dropping packet, proto=0x%04x, %s port\n",
                     proto, queue ? "controlled" : "uncontrolled");
        func_exit();
        return;

    } else if (port_action == unifi_8021xPortClosedBlock) {
        /* Buffer the packet into the Rx queues */
        rx_buffered_packets_t *rx_q_item;
        struct list_head *rx_list;

        rx_q_item = (rx_buffered_packets_t *)kmalloc(sizeof(rx_buffered_packets_t),
                                                     GFP_KERNEL);
        if (rx_q_item == NULL) {
            unifi_error(priv,
                        "Failed to allocate %d bytes for rx packet record\n",
                        sizeof(rx_buffered_packets_t));
            priv->stats.rx_dropped++;
            unifi_net_data_free(priv, &bulkdata->d[0]);
            func_exit();
            return;
        }

        INIT_LIST_HEAD(&rx_q_item->q);
        rx_q_item->bulkdata = bulkdata->d[0];
        rx_q_item->skb = skb;
        memcpy(rx_q_item->sa.data, ind->Sa.x, ETH_ALEN);
        unifi_trace(priv, UDBG2, "unifi_rx: Blocked skb=%p, bulkdata=%p\n",
                    rx_q_item->skb, &rx_q_item->bulkdata);

        if (queue == UF_CONTROLLED_PORT_Q) {
            rx_list = &priv->rx_controlled_list;
        } else {
            rx_list = &priv->rx_uncontrolled_list;
        }

        /* Add to tail of packets queue */
        down(&priv->rx_q_sem);
        list_add_tail(&rx_q_item->q, rx_list);
        up(&priv->rx_q_sem);

        func_exit();
        return;

    }



    /* Pass SKB up the stack */
    netif_rx(skb);
    dev->last_rx = jiffies;

    /* Bump the rx stats */
    priv->stats.rx_packets++;
    priv->stats.rx_bytes += bulkdata->d[0].data_length;

    func_exit();

} /* unifi_rx() */



/*
 * ---------------------------------------------------------------------------
 *  uf_set_multicast_list
 *
 *      This function is called by the higher level stack to set
 *      a list of multicast rx addresses.
 *
 *  Arguments:
 *      dev             Network Device pointer.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */

static void
uf_set_multicast_list(struct net_device *dev)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int i;
    u8 *mc_list = priv->mc_list;
//zhj_add
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)		
    struct dev_mc_list *p;      /* Pointer to the addresses structure. */
#else
    struct netdev_hw_addr *ha;;    //ices add
    netdev_for_each_mc_addr(ha, dev);//ices add
#endif	

    if (priv->init_progress != UNIFI_INIT_COMPLETED) {
        return;
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
    unifi_trace(priv, UDBG3,
                "uf_set_multicast_list (count=%d)\n", dev->mc_count);

    /* Not enough space? */
    if (dev->mc_count > UNIFI_MAX_MULTICAST_ADDRESSES) {
        return;
	}

    /* Store the list to be processed by the work item. */
    priv->mc_list_count = dev->mc_count;	
    p = dev->mc_list;
    for (i = 0; i < dev->mc_count; i++) {
        memcpy(mc_list, p->dmi_addr, ETH_ALEN);
        p = p->next;
        mc_list += ETH_ALEN;
    }
#else
    unifi_trace(priv, UDBG3,
                "uf_set_multicast_list (count=%d)\n", dev->mc.count);

    /* Not enough space? */
    if (dev->mc.count > UNIFI_MAX_MULTICAST_ADDRESSES) {
        return;
	}

    /* Store the list to be processed by the work item. */
    priv->mc_list_count = dev->mc.count;	
    ha->list = dev->mc.list;//ices mod dev->mc.list to dev->mc_list
    for (i = 0; i < dev->mc.count; i++) {
        memcpy(mc_list, ha->addr, ETH_ALEN);
        ha = ha->list.next;
        mc_list += ETH_ALEN;
    }
#endif

    /* Send a message to the workqueue */
    queue_work(priv->unifi_workqueue, &priv->multicast_list_task);

} /* uf_set_multicast_list() */


#if 0
static CsrInt32
unifi_get_os_qos_info(unifi_priv_t *priv, unifi_OsQosInfo *os_info)
{
    CsrUint8 i;
    struct Qdisc* qd;
    struct uf_sched_data *q;

    memset(os_info, 0, sizeof(unifi_OsQosInfo));
    os_info->os_queue_paused = 1;
    if (netif_running(priv->netdev)) {
        os_info->os_queue_paused = 0;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
    qd = priv->netdev->qdisc_sleeping;
#endif
    if (qd == NULL) {
        unifi_error(priv, "unifi_get_os_qos_info: Qdisc not initialized\n");
        return -1;
    }

    q = qdisc_priv(qd);
    if (q == NULL) {
        unifi_error(priv, "unifi_get_os_qos_info: Queue not initialized\n");
        return -1;
    }

    for (i = 0; i < UNIFI_TRAFFIC_Q_MAX; i++) {
         if (i == UNIFI_TRAFFIC_Q_EAPOL || q->queues[i] == NULL) {
             continue;
         }
         os_info->queue_length[i] = q->queues[i]->q.qlen;
    }

    return 1;
}

void
uf_send_queue_info(unifi_priv_t *priv)
{
    CSR_SIGNAL queue_info;
    CsrUint8 sigbuf[UNIFI_PACKED_SIGBUF_SIZE];
    CsrUint16 packed_siglen, i;
    unifi_HipQosInfo hip_info;
    unifi_OsQosInfo os_info;
    bulk_data_param_t data_ptrs;
    CsrInt32 csr_r;
    CsrUint8 *info;

    func_enter();

    if (priv == NULL) {
        return;
    }

    queue_info.SignalPrimitiveHeader.SignalId = CSR_DEBUG_GENERIC_INDICATION_ID;
    queue_info.u.DebugGenericIndication.Dummydataref2.DataLength = 0;
    queue_info.u.DebugGenericIndication.Dummydataref2.SlotNumber = 0;
    queue_info.u.DebugGenericIndication.DebugVariable.DataLength = 0;
    queue_info.u.DebugGenericIndication.Dummydataref2.SlotNumber = 0;

    unifi_trace(priv, UDBG1, "Getting queue info for HIP\n");
    /* Get queue information from the HIP */
    unifi_get_hip_qos_info(priv->card, &hip_info);

    unifi_trace(priv, UDBG1, "Free sig Queue 0 (BK) slots %d\n", hip_info.free_fh_sig_queue_slots[0]);
    unifi_trace(priv, UDBG1, "Free sig Queue 1 (BE) slots %d\n", hip_info.free_fh_sig_queue_slots[1]);
    unifi_trace(priv, UDBG1, "Free sig Queue 2 (VI) slots %d\n", hip_info.free_fh_sig_queue_slots[2]);
    unifi_trace(priv, UDBG1, "Free sig Queue 3 (VO) slots %d\n", hip_info.free_fh_sig_queue_slots[3]);
    unifi_trace(priv, UDBG1, "Free Bulk Data slots %d\n", hip_info.free_fh_bulkdata_slots);
    unifi_trace(priv, UDBG1, "Free FW slots %d\n", hip_info.free_fh_fw_slots);
    unifi_trace(priv, UDBG1, "(Note: Value of 0xfa probably indicates an error in reading)\n");

    unifi_trace(priv, UDBG1, "Getting queue info for OS layer\n");
    csr_r = unifi_get_os_qos_info(priv, &os_info);
    if (csr_r < 0) {
        unifi_error(priv, "Unable to get OS layer information\n");
        return;
    }
 
    unifi_trace(priv, UDBG1, "Queue 0 (BK) length %d\n", os_info.queue_length[0]);
    unifi_trace(priv, UDBG1, "Queue 1 (BE) length %d\n", os_info.queue_length[1]);
    unifi_trace(priv, UDBG1, "Queue 2 (VI) length %d\n", os_info.queue_length[2]);
    unifi_trace(priv, UDBG1, "Queue 3 (VO) length %d\n", os_info.queue_length[3]);
    unifi_trace(priv, UDBG1, "Queue Paused %d\n", os_info.os_queue_paused);

    /* Fill in the debug indication */
    info = (CsrUint8*) &queue_info.u.DebugGenericIndication.DebugWords[0];

    /* Host ID is 0xdbd, this is version 1 */
    *info++ = 0xd1;
    *info++ = 0xdb;

    *info++ = os_info.queue_length[0] & 0xff;
    *info++ = (os_info.queue_length[0] & 0xff00) >> 8;
    *info++ = os_info.queue_length[1] & 0xff;
    *info++ = (os_info.queue_length[1] & 0xff00) >> 8;
    *info++ = os_info.queue_length[2] & 0xff;
    *info++ = (os_info.queue_length[2] & 0xff00) >> 8;
    *info++ = os_info.queue_length[3] & 0xff;
    *info++ = (os_info.queue_length[3] & 0xff00) >> 8;

    *info++ = hip_info.free_fh_sig_queue_slots[0];
    *info++ = hip_info.free_fh_sig_queue_slots[1];
    *info++ = hip_info.free_fh_sig_queue_slots[2];
    *info++ = hip_info.free_fh_sig_queue_slots[3];

    /* Only 6 bits for bulk data slots */
    if (hip_info.free_fh_bulkdata_slots > 0x3f) {
        hip_info.free_fh_bulkdata_slots = 0x3f;
    }
  
    /* Only 6 bits for FW slots */
    if (hip_info.free_fh_fw_slots > 0x3f) {
        hip_info.free_fh_fw_slots = 0x3f;
    }

    if (os_info.os_queue_paused) {
        /* Per queue */
        os_info.os_queue_paused = 0xf;
    }

    queue_info.u.DebugGenericIndication.DebugWords[7] =
                   (os_info.os_queue_paused << 12) | 
                   (hip_info.free_fh_fw_slots << 6) | 
                    hip_info.free_fh_bulkdata_slots;

    csr_r = write_pack(&queue_info, sigbuf, &packed_siglen);
    if (csr_r) {
        unifi_error(priv, "Malformed signal in send_queue_info, error %d\n",
                    csr_r);
        return;
    }

    for (i = 0; i < UNIFI_MAX_DATA_REFERENCES; i++) {
         data_ptrs.d[i].data_length = 0;
    }
        
    logging_handler(priv, sigbuf, packed_siglen, &data_ptrs, UDI_LOG_TO_HOST);

    func_exit();
}
#endif

/*
 * ---------------------------------------------------------------------------
 *  netdev_mlme_event_handler
 *
 *      Callback function to be used as the udi_event_callback when registering
 *      as a netdev client.
 *      To use it, a client specifies this function as the udi_event_callback
 *      to ul_register_client(). The signal dispatcher in
 *      unifi_receive_event() will call this function to deliver a signal.
 *
 *  Arguments:
 *      pcli            Pointer to the client instance.
 *      signal          Pointer to the received signal.
 *      signal_len      Size of the signal structure in bytes.
 *      bulkdata        Pointer to structure containing any associated bulk data.
 *      dir             Direction of the signal. Zero means from host,
 *                      non-zero means to host.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
netdev_mlme_event_handler(ul_client_t *pcli,
                          const u8 *sig_packed, int sig_len,
                          const bulk_data_param_t *bulkdata_o,
                          int dir)
{
    CSR_SIGNAL signal;
    unifi_priv_t *priv = uf_find_instance(pcli->instance);
    int id, r;
    bulk_data_param_t bulkdata;

    func_enter();

    /* Just a sanity check */
    if (sig_packed == NULL) {
        return;
    }

    /*
     * This copy is to silence a compiler warning about discarding the
     * const qualifier.
     */
    bulkdata = *bulkdata_o;

    /* Get the unpacked signal */
    r = read_unpack_signal(sig_packed, &signal);
    if (r) {
        unifi_error(priv, "Received unknown or corrupted signal.\n");
        return;
    }

    id = signal.SignalPrimitiveHeader.SignalId;
    unifi_trace(priv, UDBG3, "Netdev - Process signal 0x%X %s\n", id, lookup_signal_name(id));

    /*
     * Take the appropriate action for the signal.
     */
    switch (id) {
      case CSR_MA_UNITDATA_INDICATION_ID:
        unifi_rx(priv, sig_packed, sig_len, &signal, &bulkdata);
        break;

      case CSR_MA_UNITDATA_CONFIRM_ID:
        uf_ma_unitdata_status_ind(priv, &signal.u.MaUnitdataConfirm);
        break;

      case CSR_DEBUG_STRING_INDICATION_ID:
        debug_string_indication(priv, bulkdata.d[0].os_data_ptr, bulkdata.d[0].data_length);
        break;

      case CSR_DEBUG_WORD16_INDICATION_ID:
        debug_word16_indication(priv, &signal);
#if 0
        /* Send host queue information */
        if (priv->last_debug_word16[0] == 0xdbf1) {
            uf_send_queue_info(priv);
        }
#endif
        break;

      case CSR_DEBUG_GENERIC_CONFIRM_ID:
      case CSR_DEBUG_GENERIC_INDICATION_ID:
        debug_generic_indication(priv, &signal);
        break;

      default:
        break;
    }

    func_exit();
} /* netdev_mlme_event_handler() */


/*
 * ---------------------------------------------------------------------------
 *  uf_net_get_name
 *
 *      Retrieve the name (e.g. eth1) associated with this network device
 *
 *  Arguments:
 *      dev             Pointer to the network device.
 *      name            Buffer to write name
 *      len             Size of buffer in bytes
 *
 *  Returns:
 *      None
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
void uf_net_get_name(struct net_device *dev, char *name, int len)
{
    *name = '\0';
    if (dev) {
        strlcpy(name, dev->name, (len > IFNAMSIZ) ? IFNAMSIZ : len);
    }

} /* uf_net_get_name */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#ifdef CONFIG_NET_SCHED

/*
 * ---------------------------------------------------------------------------
 *  uf_install_qdisc
 *
 *      Creates a root qdisc, registers our qdisc handlers and
 *      overrides the device's qdisc_sleeping to prevent the system
 *      from creating a new one for our network device.
 *
 *  Arguments:
 *      dev             Pointer to the network device.
 *
 *  Returns:
 *      0 on success, Linux error code otherwise.
 *
 *  Notes:
 *      This function holds the qdisk lock so it needs to be called
 *      after registering the network device in uf_register_netdev().
 *      Also, the qdisc_create_dflt() API has changed in 2.6.20 to
 *      include the parentid.
 * ---------------------------------------------------------------------------
 */
int uf_install_qdisc(struct net_device *dev)
{
    struct Qdisc *qdisc;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    struct netdev_queue *queue0;
#endif /* LINUX_VERSION_CODE */


    func_enter();

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
    /*
     * check that there is no qdisc currently attached to device
     * this ensures that we will be the root qdisc. (I can't find a better
     * way to test this explicitly)
     */
    if (dev->qdisc_sleeping != &noop_qdisc) {
        func_exit_r(-EFAULT);
        return -EINVAL;
    }
#endif /* LINUX_VERSION_CODE */

    qdisc = UF_QDISC_CREATE_DFLT(dev, &uf_qdisc_ops, TC_H_ROOT);
    if (!qdisc) {
        unifi_error(NULL, "%s: qdisc installation failed\n", dev->name);
        func_exit_r(-EFAULT);
        return -EFAULT;
    }
    unifi_trace(NULL, UDBG5, "%s: parent qdisc=0x%p\n",
                dev->name, qdisc);

    /*
     * The following code is copied from the mac80211 code in the
     * linux kernel. The usage of the handle is still not very clear.
     */
    qdisc->handle = 0x80020000;
    qdisc->flags = 0x0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    queue0 = netdev_get_tx_queue(dev, 0);
    if (queue0 == NULL) {
        unifi_error(NULL, "%s: netdev_get_tx_queue returned no queue\n",
                    dev->name);
        func_exit_r(-EFAULT);
        return -EFAULT;
    }
    queue0->qdisc = qdisc;
    queue0->qdisc_sleeping = qdisc;
#else
    qdisc_lock_tree(dev);
    list_add_tail(&qdisc->list, &dev->qdisc_list);
    dev->qdisc_sleeping = qdisc;
    qdisc_unlock_tree(dev);
#endif /* LINUX_VERSION_CODE */

    func_exit_r(0);
    return 0;

} /* uf_install_qdisc() */



static int uf_qdiscop_enqueue(struct sk_buff *skb, struct Qdisc* qd)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(qd->dev_queue->dev);
#else
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(qd->dev);
#endif /* LINUX_VERSION_CODE */
    struct uf_sched_data *q = qdisc_priv(qd);
    struct uf_tx_packet_data *pkt_data = (struct uf_tx_packet_data *) skb->cb;
    struct ethhdr ehdr;
    struct Qdisc *qdisc;
    int r, proto;

    func_enter();

    memcpy(&ehdr, skb->data, ETH_HLEN);
    proto = ntohs(ehdr.h_proto);

    /* 802.1x - apply controlled/uncontrolled port rules */
    if ((proto != ETH_P_PAE)
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
            && (proto != ETH_P_WAI)
#endif
            ) {
        /* queues 0 - 3 */
        pkt_data->priority = get_packet_priority(priv, skb, &ehdr);
        pkt_data->queue = unifi_frame_priority_to_queue(pkt_data->priority);
    } else {
        pkt_data->queue = UNIFI_TRAFFIC_Q_EAPOL;
    }


    qdisc = q->queues[pkt_data->queue];
    r = qdisc->enqueue(skb, qdisc);
    if (r == NET_XMIT_SUCCESS) {
        qd->q.qlen++;
        qd->bstats.bytes += skb->len;
        qd->bstats.packets++;
        func_exit_r(NET_XMIT_SUCCESS);
        return NET_XMIT_SUCCESS;
    }

    unifi_error(priv, "uf_qdiscop_enqueue: dropped\n");
    qd->qstats.drops++;

    func_exit_r(r);
    return r;

} /* uf_qdiscop_enqueue() */


static int uf_qdiscop_requeue(struct sk_buff *skb, struct Qdisc* qd)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(qd->dev_queue->dev);
#else
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(qd->dev);
#endif /* LINUX_VERSION_CODE */
    struct uf_sched_data *q = qdisc_priv(qd);
    struct uf_tx_packet_data *pkt_data = (struct uf_tx_packet_data *) skb->cb;
    struct Qdisc *qdisc;
    int r;

    func_enter();

    unifi_trace(priv, UDBG5, "uf_qdiscop_requeue: (q=%d), tag=%u\n",
                pkt_data->queue, pkt_data->host_tag);

    /* we recorded which queue to use earlier! */
    qdisc = q->queues[pkt_data->queue];

    if ((r = qdisc->ops->requeue(skb, qdisc)) == 0) {
        qd->q.qlen++;
        func_exit_r(0);
        return 0;
    }

    unifi_error(priv, "uf_qdiscop_requeue: dropped\n");
    qd->qstats.drops++;

    func_exit_r(r);
    return r;
} /* uf_qdiscop_requeue() */

static struct sk_buff *uf_qdiscop_dequeue(struct Qdisc* qd)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(qd->dev_queue->dev);
#else
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(qd->dev);
#endif /* LINUX_VERSION_CODE */
    struct uf_sched_data *q = qdisc_priv(qd);
    struct sk_buff *skb;
    struct Qdisc *qdisc;
    int queue, i;
    struct ethhdr ehdr;
    struct uf_tx_packet_data *pkt_data;
    unifi_PortAction port_action;

    func_enter();

    /* check all the queues */
    for (i = UNIFI_TRAFFIC_Q_MAX - 1; i >= 0; i--) {

        if (i != UNIFI_TRAFFIC_Q_EAPOL) {
            queue = priv->prev_queue;
            if (++priv->prev_queue >= UNIFI_TRAFFIC_Q_EAPOL) {
                priv->prev_queue = 0;
            }
        } else {
            queue = i;
        }

#ifndef ALLOW_Q_PAUSE
        /* If queue is paused, do not dequeue */
        if (net_is_tx_q_paused(priv, queue)) {
            unifi_trace(priv, UDBG5,
                        "uf_qdiscop_dequeue: tx queue paused (q=%d)\n", queue);
            continue;
        }
#endif

        qdisc = q->queues[queue];
        skb = qdisc->dequeue(qdisc);
        if (skb) {
            /* A packet has been dequeued, decrease the queued packets count */
            qd->q.qlen--;

            pkt_data = (struct uf_tx_packet_data *) skb->cb;

            /* Check the (un)controlled port status */
            memcpy(&ehdr, skb->data, ETH_HLEN);
            /* TODO: Check why this port verification is needed twice */
            if (queue == UNIFI_TRAFFIC_Q_EAPOL) {
                port_action = verify_port(priv, ehdr.h_dest, UF_UNCONTROLLED_PORT_Q);
            }
            else {
                port_action = verify_port(priv, ehdr.h_dest, UF_CONTROLLED_PORT_Q);
            }


            /* Dequeue packet if port is open */
            if (port_action == unifi_8021xPortOpen) {
                unifi_trace(priv, UDBG5,
                            "uf_qdiscop_dequeue: new (q=%d), tag=%u\n",
                            queue, pkt_data->host_tag);

                func_exit();
                return skb;
            }

            /* Discard or block the packet if necessary */
            if (port_action == unifi_8021xPortClosedDiscard) {
                unifi_trace(priv, UDBG5,
                            "uf_qdiscop_dequeue: drop (q=%d), tag=%u\n",
                            queue, pkt_data->host_tag);
                kfree_skb(skb);
                break;
            }

            /* We can not send the packet now, put it back to the queue */
            if (qdisc->ops->requeue(skb, qdisc) != 0) {
                unifi_error(priv,
                            "uf_qdiscop_dequeue: requeue (q=%d) failed, tag=%u, drop it\n",
                            queue, pkt_data->host_tag);

                /* Requeue failed, drop the packet */
                kfree_skb(skb);
                break;
            }
            /* We requeued the packet, increase the queued packets count */
            qd->q.qlen++;

            unifi_trace(priv, UDBG5,
                        "uf_qdiscop_dequeue: skip (q=%d), tag=%u\n",
                        queue, pkt_data->host_tag);
        }
    }

    func_exit();
    return NULL;
} /* uf_qdiscop_dequeue() */


static void uf_qdiscop_reset(struct Qdisc* qd)
{
    struct uf_sched_data *q = qdisc_priv(qd);
    int queue;
    func_enter();

    for (queue = 0; queue < UNIFI_TRAFFIC_Q_MAX; queue++) {
        qdisc_reset(q->queues[queue]);
    }
    qd->q.qlen = 0;

    func_exit();
} /* uf_qdiscop_reset() */


static void uf_qdiscop_destroy(struct Qdisc* qd)
{
    struct uf_sched_data *q = qdisc_priv(qd);
    int queue;

    func_enter();

    for (queue=0; queue < UNIFI_TRAFFIC_Q_MAX; queue++) {
        qdisc_destroy(q->queues[queue]);
        q->queues[queue] = &noop_qdisc;
    }

    func_exit();
} /* uf_qdiscop_destroy() */


/* called whenever parameters are updated on existing qdisc */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
static int uf_qdiscop_tune(struct Qdisc *qd, struct nlattr *opt)
#else
static int uf_qdiscop_tune(struct Qdisc *qd, struct rtattr *opt)
#endif
{
    func_enter();
    func_exit();
    return 0;
} /* uf_qdiscop_tune() */


/* called during initial creation of qdisc on device */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
static int uf_qdiscop_init(struct Qdisc *qd, struct nlattr *opt)
#else
static int uf_qdiscop_init(struct Qdisc *qd, struct rtattr *opt)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    struct net_device *dev = qd->dev_queue->dev;
#else
    struct net_device *dev = qd->dev;
#endif /* LINUX_VERSION_CODE */
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct uf_sched_data *q = qdisc_priv(qd);
    int err = 0, i;

    func_enter();

    /* make sure we do not mess with the ingress qdisc */
    if (qd->flags & TCQ_F_INGRESS) {
        func_exit();
        return -EINVAL;
    }

    /* if options were passed in, set them */
    if (opt) {
        err = uf_qdiscop_tune(qd, opt);
    }

    /* create child queues */
    for (i = 0; i < UNIFI_TRAFFIC_Q_MAX; i++) {
        q->queues[i] = UF_QDISC_CREATE_DFLT(dev, &pfifo_qdisc_ops,
                                            qd->handle);
        if (!q->queues[i]) {
            q->queues[i] = &noop_qdisc;
            unifi_error(priv, "%s child qdisc %i creation failed\n");
        }

        unifi_trace(priv, UDBG5, "%s: child qdisc=0x%p\n",
                    dev->name, q->queues[i]);
    }

    func_exit_r(err);
    return err;
} /* uf_qdiscop_init() */


static int uf_qdiscop_dump(struct Qdisc *qd, struct sk_buff *skb)
{
    func_enter();
    func_exit_r(skb->len);
    return skb->len;
} /* uf_qdiscop_dump() */

#endif /* CONFIG_NET_SCHED */
#endif /* LINUX_VERSION_CODE */

