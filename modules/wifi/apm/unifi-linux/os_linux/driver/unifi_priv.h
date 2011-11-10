/*
 *****************************************************************************
 *
 * FILE : unifi_priv.h
 *
 * PURPOSE : Private header file for unifi driver.
 *
 *           UDI = UniFi Debug Interface
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 *****************************************************************************
 */
#ifndef __LINUX_UNIFI_PRIV_H__
#define __LINUX_UNIFI_PRIV_H__ 1

#include <linux/version.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/wireless.h>
#include <linux/cdev.h>
#include <linux/kthread.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
#include <linux/freezer.h>
#endif

#include <linux/fs.h>

#include "driver/unifi.h"
#include "driver/unifi_udi.h"

#include "unifiio.h"
#include "../lib_hip/card.h"

/*
 * The UniFi Linux Driver versions
 */
#include "os_version.h"


/* Define the unifi_priv_t before include the unifi_native.h */
struct unifi_priv;
typedef struct unifi_priv unifi_priv_t;


#ifdef CSR_SUPPORT_WEXT
#include "unifi_wext.h"
#endif

#include "unifi_clients.h"


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
#include <linux/workqueue.h>

#undef INIT_WORK
#define INIT_WORK(_work, _func)                                         \
    do {                                                                \
        INIT_LIST_HEAD(&(_work)->entry);                                \
        (_work)->pending = 0;                                           \
        PREPARE_WORK((_work), (_func), (_work));                        \
        init_timer(&(_work)->timer);                                    \
    } while(0)

#endif  /* Linux kernel < 2.6.20 */


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#define UF_NETIF_TX_WAKE_ALL_QUEUES(_netdev)    netif_tx_wake_all_queues(_netdev)
#define UF_NETIF_TX_START_ALL_QUEUES(_netdev)   netif_tx_start_all_queues(_netdev)
#define UF_NETIF_TX_STOP_ALL_QUEUES(_netdev)    netif_tx_stop_all_queues(_netdev)
#else
#define UF_NETIF_TX_WAKE_ALL_QUEUES(_netdev)    netif_wake_queue(_netdev)
#define UF_NETIF_TX_START_ALL_QUEUES(_netdev)   netif_start_queue(_netdev)
#define UF_NETIF_TX_STOP_ALL_QUEUES(_netdev)    netif_stop_queue(_netdev)
#endif  /* Linux kernel >= 2.6.27 */


#ifdef CSR_NATIVE_LINUX
#include "sme_native/unifi_native.h"
#else
#include "unifi_sme.h"
#endif

/* The device major number to use when registering the udi driver */
#define UNIFI_NAME      "unifi"
#define MAX_UNIFI_DEVS  2


/* Module parameter variables */
extern int buswidth;
extern int sdio_clock;
extern int use_5g;
extern int disable_hw_reset;
extern int disable_power_control;
extern int enable_wol;
extern int sme_debug;
extern int fw_init[MAX_UNIFI_DEVS];
extern int tl_80211d;
extern int sdio_byte_mode;
extern int sdio_block_size;
extern int run_bh_once;

struct dlpriv {
    const unsigned char *dl_data;
    int dl_len;
    void *fw_desc;
};


struct uf_thread {

    struct task_struct *thread_task;

    /* wait_queue for waking the unifi_thread kernel thread */
    wait_queue_head_t wakeup_q;
    unsigned int wakeup_flag;

    /*
     * Use it to block the I/O thread when
     * an error occurs or UniFi is reinitialised.
     */
    int block_thread;

    char name[16];

};

/*
 * Link list to hold the received packets for the period the port
 * remains closed.
 */
typedef struct rx_buffered_packets {
    /* List link structure */
    struct list_head q;
    /* Packet to indicate when the port reopens */
    struct sk_buff *skb;
    /* Bulkdata to free in case the port closes and need to discard the packet */
    bulk_data_desc_t bulkdata;
    /* The source address of the packet */
    unifi_MACAddress sa;
} rx_buffered_packets_t;


struct unifi_priv {

    card_t *card;
    CsrSdioFunction *sdio;

    /* Index into Unifi_instances[] for this device. */
    int instance;
    /* Reference count for this instance */
    int ref_count;

    /* Firmware images */
    struct dlpriv fw_loader;
    struct dlpriv fw_sta;
    struct dlpriv fw_conv;  /* used for conversion of production test image */

    /* Char device related structures */
    struct cdev unifi_cdev;
    struct cdev unifiudi_cdev;
    struct device *unifi_device;

    /* Which wireless interface to use (1 - 2.4GHz, 2 - 5GHz) */
    CSR_IFINTERFACE if_index;

    struct net_device *netdev;

    int prev_queue;

    /* Name of node under /proc */
    char proc_entry_name[64];

    /* Back pointer to associated net device. */
    struct net_device_stats stats;

    /*
     * Flag to reflect state of CONNECTED indication signal.
     * This indicates whether we are "joined" an Access Point (i.e. have
     * nominated an AP and are receiving beacons) but give no indication
     * of whether we are authenticated and/or associated.
     */
    enum {
        UnifiConnectedUnknown = -1,
        UnifiNotConnected = 0,
        UnifiConnected = 1,
    } connected;

    /*
     * Flags:
     *  drop_unencrypted
     *                - Not used?
     *  netdev_registered
     *                - whether the netdev has been registered.
     */
    unsigned int drop_unencrypted       : 1;
    unsigned int netdev_registered      : 1;

    /* Our list of unifi linux clients. */
    ul_client_t ul_clients[MAX_UDI_CLIENTS];

    /* Mutex to protect using the logging hook after UDI client is gone */
    struct semaphore udi_logging_mutex;
    /* Pointer to the ul_clients[] array */
    ul_client_t *logging_client;

    /* A ul_client_t* used to send the netdev related MIB requests. */
    ul_client_t *netdev_client;

    /* The SME ul_client_t pointer. */
    ul_client_t *sme_cli;

    /* The AMP ul_client_t pointer. */
    ul_client_t *amp_client;

    /*
     * Semaphore for locking the top-half to one user process.
     * This is necessary to prevent multiple processes calling driver
     * operations. This can happen because the network driver entry points
     * can be called from multiple processes.
     */
#ifdef USE_DRIVER_LOCK
    struct semaphore lock;
#endif /* USE_DRIVER_LOCK */

    /* Flag to say that an operation was aborted */
    int io_aborted;

    struct uf_thread bh_thread;

#define UNIFI_INIT_NONE         0x00
#define UNIFI_INIT_IN_PROGRESS  0x01
#define UNIFI_INIT_FW_DOWNLOADED 0x02
#define UNIFI_INIT_COMPLETED    0x04
    unsigned char init_progress;

    int sme_is_present;

    /* The WMM features that UniFi uses in the current BSS */
    unsigned int sta_wmm_capabilities;

    /* Debug only */
    char last_debug_string[256];
    unsigned short last_debug_word16[16];

#define UNIFI_MAX_MULTICAST_ADDRESSES 10
    /* The multicast addresses list that the thread needs to set. */
    u8 mc_list[UNIFI_MAX_MULTICAST_ADDRESSES*ETH_ALEN];
    /* The multicast addresses count that the thread needs to set. */
    int mc_list_count;

#ifdef CSR_NATIVE_LINUX
    /* wireless config */
    struct wext_config wext_conf;

    /* Mutex to protect the MLME blocking requests */
    struct semaphore mlme_blocking_mutex;

    /* The ul_client that provides the blocking API for WEXT calls */
    ul_client_t *wext_client;

    /* The ul_client that provides the blocking API for multicast workqueue */
    ul_client_t *multicast_client;

    /*
     * Multicast list is set using a thread since we can not sleep
     * in the netdev callback. The mc_list_count_stored keeps the
     * number of the addresses already stored in the MIB.
     */
    int mc_list_count_stored;
#endif /* CSR_NATIVE_LINUX */


#ifdef CSR_SUPPORT_SME
    wait_queue_head_t sme_request_wq;
    /* Semaphore to protect the SME blocking requests */
    struct semaphore sme_sem;
    /* Structure to hold the SME blocking requests data*/
    sme_reply_t sme_reply;

    /* (un)controlled port configuration */
    unifi_port_config_t controlled_data_port;
    unifi_port_config_t uncontrolled_data_port;

    /* Structure to hold a traffic protocol indication */
    struct ta_ind {
        struct work_struct task;
        unifi_TrafficPacketType packet_type;
        unifi_ProtocolDirection direction;
        unifi_MACAddress src_addr;
        int in_use;
    } ta_ind_work;

    struct ta_sample_ind {
        struct work_struct task;
        unifi_TrafficStats stats;
        int in_use;
    } ta_sample_ind_work;

    __be32 sta_ip_address;
    unifi_SmeVersions    sme_versions;

    /*
     * Flag to reflect state of unifi_sys_wifi_on_*() progress.
     * This indicates whether we are in an "wifi on" state when we are
     * allowed to indication errors with unifi_mgt_wifi_off_ind()
     */
    enum {
        wifi_on_unspecified = -1,
        wifi_on_in_progress = 0,
        wifi_on_done = 1,
    } wifi_on_state;

    /*
     * The SME installs filters to ask for specific MA-UNITDATA.req
     * to be passed to different SME components.
     */
#define MAX_MA_UNIDATA_IND_FILTERS      8
    sme_ma_unidata_ind_filter_t sme_unidata_ind_filters[MAX_MA_UNIDATA_IND_FILTERS];

    enum {
        m4_idle,        /* Dont do anything */
        m4_trap,        /* Look out for M4 and trap it, after new connection */
        m4_trapped,     /* M4 found and stored */
        m4_wait_eapol_confirm /* Once M4 is sent, m4_transmitted signal needs to be sent */
    } m4_monitor_state;

    CSR_SIGNAL m4_signal;
    bulk_data_desc_t m4_bulk_data;

#ifdef CSR_SUPPORT_WEXT
    /* The structure that holds all the connection configuration. */
    unifi_ConnectionConfig connection_config;
    int ignore_bssid_join;
    struct iw_statistics wext_wireless_stats;

    /* The MIB and MAC address files contents, read from userspace */
    unifi_DataBlock mib_data;
    unifi_MACAddress sta_mac_address;

    int wep_tx_key_index;
    wep_key_t wep_keys[NUM_WEPKEYS];

    /* UNIFI_CFG related parameters */
    uf_cfg_bcast_packet_filter_t packet_filters;
    unsigned char *filter_tclas_ies;

    struct work_struct sme_config_task;

#endif /* CSR_SUPPORT_WEXT */

#endif /* CSR_SUPPORT_SME */


#ifdef CSR_SME_EMB
    FsmContext* smepriv;

    struct uf_thread sme_thread;

#endif /* CSR_SME_EMB */


#ifdef CSR_SME_USERSPACE
    void *smepriv;

#endif /* CSR_SME_USERSPACE */

    card_info_t card_info;

    /* Mutex to protect unifi_send_signal() */
    spinlock_t send_signal_lock;


    /*
     * The workqueue to offload the TA run
     * and the multicast addresses list set
     */
    struct workqueue_struct *unifi_workqueue;
    struct work_struct multicast_list_task;

    unsigned char *mib_cfm_buffer;
    unsigned int mib_cfm_buffer_length;

    int ptest_mode;     /* Set when in production test mode */
    CsrBool wol_suspend; /* Set when suspending with UniFi powered */

#define UF_UNCONTROLLED_PORT_Q      0
#define UF_CONTROLLED_PORT_Q        1
    /* A list to hold the buffered uncontrolled port packets */
    struct list_head rx_uncontrolled_list;
    /* A list to hold the buffered controlled port packets */
    struct list_head rx_controlled_list;
    /* Semaphore to protect the rx queues list */
    struct semaphore rx_q_sem;

#ifndef ALLOW_Q_PAUSE
    /* Bit mask to indicate if a particular Tx queue is paused, this may not be
     * required in a multiqueue implementation since we can directly stop kernel
     * queues */
    CsrUint32 tx_q_paused_mask;
#endif
};

typedef struct {
    CsrUint16 queue_length[4];
    CsrUint8 os_queue_paused; 
} unifi_OsQosInfo;

#ifndef ALLOW_Q_PAUSE
#define net_is_tx_q_paused(priv, q)   ((priv->tx_q_paused_mask & (1 << q)) >> q)
#define net_tx_q_unpause(priv, q)   (priv->tx_q_paused_mask &= ~(1 << q))
#define net_tx_q_pause(priv, q)   (priv->tx_q_paused_mask |= (1 << q))
#endif


#define convert_csr_error(csr_r) (csr_r)

#ifdef USE_DRIVER_LOCK
#define LOCK_DRIVER(_p)         down_interruptible(&(_p)->lock)
#define UNLOCK_DRIVER(_p)       up(&(_p)->lock)
#else
#define LOCK_DRIVER(_p)         (void)(_p); /* as nothing */
#define UNLOCK_DRIVER(_p)       (void)(_p); /* as nothing */
#endif /* USE_DRIVER_LOCK */


/*
 * SDIO related functions and callbacks
 */
int  uf_sdio_load(void);
void uf_sdio_unload(void);
unifi_priv_t *uf_find_instance(int inst);
int uf_find_priv(unifi_priv_t *priv);
unifi_priv_t *uf_get_instance(int inst);
void uf_put_instance(int inst);
int csr_sdio_linux_install_irq(CsrSdioFunction *sdio);
int csr_sdio_linux_remove_irq(CsrSdioFunction *sdio);

void uf_add_os_device(int bus_id, struct device *os_device);
void uf_remove_os_device(int bus_id);

/*
 * Functions to allocate and free an ethernet device.
 */
unifi_priv_t *uf_alloc_netdevice(CsrSdioFunction *sdio_dev, int bus_id);
int uf_free_netdevice(unifi_priv_t *priv);


/*
 * Firmware download related functions.
 */
int uf_run_unifihelper(unifi_priv_t *priv);
int uf_request_firmware_files(unifi_priv_t *priv, int is_fw);
int uf_release_firmware_files(unifi_priv_t *priv);
int uf_release_firmware(unifi_priv_t *priv, struct dlpriv *to_free);

/*
 * Functions to create and delete the device nodes.
 */
int uf_create_device_nodes(unifi_priv_t *priv, int bus_id);
void uf_destroy_device_nodes(unifi_priv_t *priv);

/*
 * Upper Edge Initialisation functions
 */
int uf_init_bh(unifi_priv_t *priv);
int uf_init_hw(unifi_priv_t *priv);

/* Thread related helper functions */
int uf_start_thread(unifi_priv_t *priv, struct uf_thread *thread, int (*func)(void *));
void uf_stop_thread(unifi_priv_t *priv, struct uf_thread *thread);
void uf_wait_for_thread_to_stop(unifi_priv_t *priv, struct uf_thread *thread);


/*
 * Unifi Linux functions
 */
void ul_init_clients(unifi_priv_t *priv);

/* Configuration flags */
#define CLI_USING_WIRE_FORMAT   0x0002
#define CLI_SME_USERSPACE       0x0020
ul_client_t *ul_register_client(unifi_priv_t *priv,
                                unsigned int configuration,
                                udi_event_t udi_event_clbk);
int ul_deregister_client(ul_client_t *pcli);

int ul_send_signal_unpacked(unifi_priv_t *priv,
                            const CSR_SIGNAL *sigptr,
                            const bulk_data_param_t *bulkdata);
int ul_send_signal_raw(unifi_priv_t *priv,
                       const unsigned char *sigptr, int siglen,
                       const bulk_data_param_t *bulkdata);

void ul_log_config_ind(unifi_priv_t *priv, u8 *conf_param, int len);


/*
 * Data plane operations
 */
/*
 *      data_tx.c
 */
int uf_verify_m4(unifi_priv_t *priv, const unsigned char *packet, 
                 unsigned int length);

int uf_ma_unitdata(unifi_priv_t *priv, ul_client_t *pcli,
                   CSR_SIGNAL *sig, const bulk_data_param_t *bulkdata);

int uf_mlme_eapol(unifi_priv_t *priv, ul_client_t *pcli,
                  CSR_SIGNAL *sig, const bulk_data_param_t *bulkdata);
/*
 *      indications.c
 */
void uf_ma_unitdata_status_ind(unifi_priv_t *priv,
                                  const CSR_MA_UNITDATA_CONFIRM *ind);


/*
 *      netdev.c
 */

#ifndef P80211_OUI_LEN
#define P80211_OUI_LEN  3
#endif
typedef struct {
    u8    dsap;   /* always 0xAA */
    u8    ssap;   /* always 0xAA */
    u8    ctrl;   /* always 0x03 */
    u8    oui[P80211_OUI_LEN];    /* organizational universal id */
    u16 protocol;
} __attribute__ ((packed)) llc_snap_hdr_t;
int skb_ether_to_80211(struct net_device *dev, struct sk_buff *skb, int proto);
int skb_80211_to_ether(unifi_priv_t *priv, struct sk_buff *skb,
                       const unsigned char *daddr, const unsigned char *saddr,
                       const u8 *packed_signal, int signal_len,
                       const CSR_SIGNAL *signal, bulk_data_param_t *bulkdata);

const char *result_code_str(int result);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
int uf_install_qdisc(struct net_device *dev);
#endif

void uf_resume_data_plane(unifi_priv_t *priv, int queue,
                          unifi_MACAddress peer_address);
void uf_free_pending_rx_packets(unifi_priv_t *priv, int queue,
                                unifi_MACAddress peer_address);

int uf_register_netdev(unifi_priv_t *priv);

void uf_net_get_name(struct net_device *dev, char *name, int len);

void uf_send_queue_info(unifi_priv_t *priv);

/*
 *      inet.c
 */
void uf_register_inet_notifier(void);
void uf_unregister_inet_notifier(void);


/*
 * Suspend / Resume handlers
 */
void unifi_resume(void *ospriv);
void unifi_suspend(void *ospriv);


#define QOS_CAPABILITY_WMM_ENABLED      0x0001
#define QOS_CAPABILITY_WMM_UAPSD        0x0002
#define QOS_CAPABILITY_ACM_BE_ENABLED   0x0010
#define QOS_CAPABILITY_ACM_BK_ENABLED   0x0020
#define QOS_CAPABILITY_ACM_VI_ENABLED   0x0040
#define QOS_CAPABILITY_ACM_VO_ENABLED   0x0080
#define QOS_CAPABILITY_TS_BE_ENABLED    0x0100
#define QOS_CAPABILITY_TS_BK_ENABLED    0x0200
#define QOS_CAPABILITY_TS_VI_ENABLED    0x0400
#define QOS_CAPABILITY_TS_VO_ENABLED    0x0800

/*
 * unifi_dbg.c
 */
void debug_string_indication(unifi_priv_t *priv,
                             const unsigned char *extra,
                             unsigned int extralen);
void debug_word16_indication(unifi_priv_t *priv, const CSR_SIGNAL *sigptr);
void debug_generic_indication(unifi_priv_t *priv, const CSR_SIGNAL *sigptr);


/*
 * putest.c
 */
int unifi_putest_start(unifi_priv_t *priv, unsigned char *arg);
int unifi_putest_stop(unifi_priv_t *priv, unsigned char *arg);
int unifi_putest_set_sdio_clock(unifi_priv_t *priv, unsigned char *arg);
int unifi_putest_cmd52_read(unifi_priv_t *priv, unsigned char *arg);
int unifi_putest_cmd52_write(unifi_priv_t *priv, unsigned char *arg);
int unifi_putest_dl_fw(unifi_priv_t *priv, unsigned char *arg);
int unifi_putest_dl_fw_buff(unifi_priv_t *priv, unsigned char *arg);


#endif /* __LINUX_UNIFI_PRIV_H__ */
