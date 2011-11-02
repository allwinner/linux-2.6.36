/*
 * ***************************************************************************
 *  FILE:     unifi_sme.h
 *
 *  PURPOSE:    SME related definitions.
 *
 *  Copyright (C) 2007-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */
#ifndef __LINUX_UNIFI_SME_H__
#define __LINUX_UNIFI_SME_H__ 1

#include <linux/kernel.h>

#ifdef CSR_SME_EMB
#include "sme_csr_emb/sme_emb.h"
#endif

#ifdef CSR_SME_USERSPACE
#include "sme_csr/sme_userspace.h"
#endif


typedef int unifi_data_port_action;

typedef struct unifi_port_cfg
{
    unifi_PortAction port_action;
    unifi_MACAddress mac_address;
} unifi_port_cfg_t;

#define UNIFI_MAX_CONNECTIONS           8
#define UF_DATA_PORT_NOT_OVERIDE        0
#define UF_DATA_PORT_OVERIDE            1

typedef struct unifi_port_config
{
    int entries_in_use;
    int overide_action;
    unifi_port_cfg_t port_cfg[UNIFI_MAX_CONNECTIONS];
} unifi_port_config_t;


enum sme_request_status {
    SME_REQUEST_EMPTY,
    SME_REQUEST_PENDING,
    SME_REQUEST_RECEIVED,
    SME_REQUEST_TIMEDOUT,
};

/* Structure to hold a UDI logged signal */
typedef struct {

    /* The current status of the request */
    enum sme_request_status request_status;

    /* The status the SME has passed to us */
    unifi_Status reply_status;

    /* SME's reply to a unifi_AppValue request */
    unifi_AppValue reply_app_value;

    /* SME's reply to a scan request */
    CsrUint16 reply_scan_results_count;
    unifi_ScanResult* reply_scan_results;

} sme_reply_t;


typedef struct {
    void* appHandle;
    unifi_Encapsulation encapsulation;
    CsrUint16 protocol;
    CsrUint8 oui[3];
    CsrUint8 in_use;
} sme_ma_unidata_ind_filter_t;


unifi_PortAction uf_sme_port_state(unifi_priv_t *priv,
                                   unsigned char *address,
                                   int queue);


/* Callback for event logging to SME clients */
void sme_log_event(ul_client_t *client, const u8 *signal, int signal_len,
                   const bulk_data_param_t *bulkdata, int dir);

/* The workqueue task to the set the multicast addresses list */
void uf_multicast_list_wq(struct work_struct *work);

/* The workqueue task to execute the TA module */
void uf_ta_wq(struct work_struct *work);


/*
 * SME blocking helper functions
 */
void sme_complete_request(unifi_priv_t *priv, unifi_Status reply_status);


/*
 * Blocking functions using the SME SYS API.
 */
int sme_sys_suspend(unifi_priv_t *priv);
int sme_sys_resume(unifi_priv_t *priv);


/*
 * Traffic Analysis workqueue jobs
 */
void uf_ta_ind_wq(struct work_struct *work);
void uf_ta_sample_ind_wq(struct work_struct *work);

/*
 * SME config workqueue job
 */
void uf_sme_config_wq(struct work_struct *work);


#ifdef CSR_SUPPORT_WEXT
/*
 * Blocking functions using the SME MGT API.
 */
int sme_mgt_wifi_on(unifi_priv_t *priv);
int sme_mgt_wifi_off(unifi_priv_t *priv);
int sme_mgt_set_value_async(unifi_priv_t *priv, unifi_AppValue *app_value);
int sme_mgt_get_value_async(unifi_priv_t *priv, unifi_AppValue *app_value);
int sme_mgt_get_value(unifi_priv_t *priv, unifi_AppValue *app_value);
int sme_mgt_set_value(unifi_priv_t *priv, unifi_AppValue *app_value);
int sme_mgt_scan_full(unifi_priv_t *priv, unifi_SSID *specific_ssid,
                      int num_channels, unsigned char *channel_list);
int sme_mgt_scan_results_get_async(unifi_priv_t *priv,
                                   struct iw_request_info *info,
                                   char *scan_results,
                                   long scan_results_len);
int sme_mgt_disconnect(unifi_priv_t *priv);
int sme_mgt_connect(unifi_priv_t *priv);
int sme_mgt_get_versions(unifi_priv_t *priv, unifi_Versions *versions);
int sme_mgt_key(unifi_priv_t *priv, unifi_Key *sme_key,
                enum unifi_ListAction action);
int sme_mgt_pmkid(unifi_priv_t *priv, unifi_ListAction action,
                  unifi_PmkidList *pmkid_list);
int sme_mgt_packet_filter_set(unifi_priv_t *priv);
int sme_mgt_mib_get(unifi_priv_t *priv,
                    unsigned char *varbind, int *length);
int sme_mgt_mib_set(unifi_priv_t *priv,
                    unsigned char *varbind, int length);
int sme_mgt_tspec(unifi_priv_t *priv, unifi_ListAction action,
                  CsrUint32 tid, unifi_DataBlock *tspec, unifi_DataBlock *tclas);

int unifi_translate_scan(struct net_device *dev,
                         struct iw_request_info *info,
                         char *current_ev, char *end_buf,
                         unifi_ScanResult *scan_data,
                         int scan_index);

int unifi_cfg_power(unifi_priv_t *priv, unsigned char *arg);
int unifi_cfg_power_save(unifi_priv_t *priv, unsigned char *arg);
int unifi_cfg_power_supply(unifi_priv_t *priv, unsigned char *arg);
int unifi_cfg_packet_filters(unifi_priv_t *priv, unsigned char *arg);
int unifi_cfg_wmm_qos_info(unifi_priv_t *priv, unsigned char *arg);
int unifi_cfg_wmm_addts(unifi_priv_t *priv, unsigned char *arg);
int unifi_cfg_wmm_delts(unifi_priv_t *priv, unsigned char *arg);
int unifi_cfg_get_info(unifi_priv_t *priv, unsigned char *arg);
int unifi_cfg_strict_draft_n(unifi_priv_t *priv, unsigned char *arg);
int unifi_cfg_enable_okc(unifi_priv_t *priv, unsigned char *arg);

#endif /* CSR_SUPPORT_WEXT */

int convert_sme_error(enum unifi_Status error);


#endif /* __LINUX_UNIFI_SME_H__ */
