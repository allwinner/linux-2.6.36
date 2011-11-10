/*
 * ---------------------------------------------------------------------------
 * FILE:     autojoin.c
 * 
 * PURPOSE:
 *      This file provides the "autojoin" functionality required for linux
 *      wireless extensions.
 *      It is effeectively a very, very simple SME.
 *      
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include "driver/unifi.h"
#include "unifi_priv.h"

#if 0
#undef func_enter
#undef func_exit
#define func_enter()    printk("=> %s\n", __FUNCTION__)
#define func_exit()     printk("<= %s\n", __FUNCTION__)
#endif

/* The additional time taken by the UniFi to do a scan per channel */
#define SCAN_STARTUP_TIME       300 /* in millisecs */


/*
 * ---------------------------------------------------------------------------
 *  unifi_do_scan
 *
 *      Convenience function to perform a scan to list all networks
 *      in the vicinity.
 * 
 *  Arguments:
 *	priv            Pointer to driver private struct.
 *      scantype        0 = active, 1 = passive
 *      bsstype         What sort of networks to scan for:
 *                        0 = Infra, 1 = IBSS, 2 = Any
 *      ssid            Pointer to a network name (ESSID) to scan for, or
 *                      NULL to scan for all networks.
 *      ssid_len        Length of ssid.
 *
 *  Returns:
 *      0 on success, error code on failure
 *
 *  Notes: 
 *      Parameters we could add:
 *        channel       The channel number to scan on or -1 meaning scan
 *                      all channels
 *      For a really quick scan we could scan a particular channel.
 *      Also:
 *        bssid           MAC address of network to scan for, or NULL
 *        ssid            Network name to scan for, or NULL
 *      but these won't reduce the scan time at all, so there's no point
 *      in using them.
 * ---------------------------------------------------------------------------
 */
int
unifi_do_scan(unifi_priv_t *priv, int scantype, CSR_BSS_TYPE bsstype,
              const char *ssid, int ssid_len)
{
    CSR_SIGNAL signal;
    CSR_MLME_SCAN_REQUEST *req = &signal.u.MlmeScanRequest;
    bulk_data_param_t data_ptrs;
    int r, rc;
    unsigned char *ie_buf = NULL;
    unsigned int ie_len = 0;
    unsigned long j;
    int timeout = 1000;
    int num_channels = 11;
    ul_client_t *pcli = priv->wext_client;

    /* clear the pre-existing scan results */
    unifi_clear_scan_table(priv);

    unifi_notice(priv, "Scanning for wireless networks\n");

    memset((void*)req, 0, sizeof(req));

    req->Ifindex         = priv->if_index; /* 2.4GHz or 5GHz operation */
    req->BssType         = bsstype;
    memset((void*)req->Da.x, 0xFF, 6);
    memset((void*)req->Bssid.x, 0xFF, 6);
    req->ScanType = scantype;    /* 0 = active, 1 = passive */
    if (scantype == UNIFI_SCAN_ACTIVE) {
        /* Use shorter times for an active scan */
        req->ProbeDelay      = 10;
        req->MinChannelTime  = 50;
        req->MaxChannelTime  = 150;
    } else {
        req->ProbeDelay      = 0;
        req->MinChannelTime  = 200;
        req->MaxChannelTime  = 250;
    }

    /* Figure out our timeout ms */
    timeout = num_channels * (req->MaxChannelTime +
            req->ProbeDelay +
            SCAN_STARTUP_TIME);
    timeout += timeout / 2;/* add 50% */
    
    /* Build Information Elements for a bulk data slot */

    /* If SSID is given, add it to the IE vector */
    if (ssid && ssid_len) {
        r = unifi_net_data_malloc(priv, &data_ptrs.d[1], ssid_len + 2);
        if (r != 0) {
            unifi_error(priv, "unifi_do_scan: failed to allocate ie_buf.\n");
            return -EIO;
        }
        ie_buf = (unsigned char*)data_ptrs.d[1].os_data_ptr;
        unifi_add_info_element(ie_buf, IE_SSID_ID, ssid, ssid_len);
        ie_len = ssid_len + 2;
    }

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_SCAN_REQUEST_ID;

    data_ptrs.d[0].os_data_ptr = data_ptrs.d[0].os_net_buf_ptr = NULL;
    data_ptrs.d[0].data_length = 0;
    /* Do not mess with data_ptrs.d[1].os_net_buf_ptr. */
    data_ptrs.d[1].os_data_ptr = ie_buf;
    data_ptrs.d[1].data_length = ie_len;

    j = jiffies;
    r = unifi_mlme_blocking_request(priv, pcli, &signal, &data_ptrs, timeout);
    if (r) {
        unifi_error(priv, "failed to send SCAN request, error %d\n", r);
        return r;
    }

    rc = pcli->reply_signal->u.MlmeScanConfirm.ResultCode;
    if (rc) {
        unifi_notice(priv, "SCAN request was rejected with with result 0x%X (%s)\n",
                     rc, lookup_result_code(rc));
        return -EIO;
    }

    unifi_trace(priv, UDBG1, "Scan time %ld jiffies\n", jiffies - j);
    
    return 0;
} /* unifi_do_scan() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_do_join
 *
 *      Join a BSS whose BSS description was previously obtained with
 *      a scan.
 *
 * Arguments:
 *	priv            Pointer to driver private struct.
 *	si              Pointer to scan info struct for BSS to join.
 *
 * Returns:
 *      0 on success, error code on failure
 * ---------------------------------------------------------------------------
 */
static int
unifi_do_join(unifi_priv_t *priv, scan_info_t *si)
{
    CSR_SIGNAL signal;
    CSR_MLME_JOIN_REQUEST *req = &signal.u.MlmeJoinRequest;
    bulk_data_param_t data_ptrs;
    struct wext_config *conf= &priv->wext_conf;
    int timeout;
    int r, rc;
    ul_client_t *pcli = priv->wext_client;
    unsigned char *scan_ie_buf = NULL;
    unsigned int scan_ie_len = 0;
    
    unifi_trace(priv, UDBG1, "Join: bssid=%02X:%02X:%02X:%02X:%02X:%02X, capability=0x%X, channel=%d\n",
          si->msi.Bssid.x[0], si->msi.Bssid.x[1], si->msi.Bssid.x[2],
          si->msi.Bssid.x[3], si->msi.Bssid.x[4], si->msi.Bssid.x[5],
          si->msi.CapabilityInformation, si->msi.Channel);

    /* 
     * Set up the JOIN request
     * req.JoinFailureTimeout is in BeaconPeriods. We make the assumption that an
     * 802.11 TU (1024us) is approximately 1ms, the units used for
     * conf->join_failure_timeout.
     */
    req->Ifindex = si->msi.Ifindex;
    memcpy((void*)req->Bssid.x, (void*)si->msi.Bssid.x, 6);
    req->BeaconPeriod = si->msi.BeaconPeriod;
    req->Timestamp = si->msi.Timestamp;
    req->LocalTime = si->msi.LocalTime;
    req->Channel = si->msi.Channel;
    req->CapabilityInformation = si->msi.CapabilityInformation;
    req->JoinFailureTimeout = conf->join_failure_timeout / si->msi.BeaconPeriod;
    req->ProbeDelay = 0;

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_JOIN_REQUEST_ID;

    r = unifi_net_data_malloc(priv, &data_ptrs.d[0], si->info_elem_length);
    if (r != 0) {
        unifi_error(priv, "unifi_do_join: failed to allocate scan_ie_buf.\n");
        return -EIO;
    }
    scan_ie_buf = (unsigned char*)data_ptrs.d[0].os_data_ptr;

    memcpy(scan_ie_buf, si->info_elems, si->info_elem_length);
    scan_ie_len = si->info_elem_length;

    /* Do not mess with data_ptrs.d[0].os_net_buf_ptr. */
    data_ptrs.d[0].os_data_ptr = scan_ie_buf;
    data_ptrs.d[0].data_length = scan_ie_len;
    data_ptrs.d[1].os_data_ptr = data_ptrs.d[1].os_net_buf_ptr = NULL;
    data_ptrs.d[1].data_length = 0;

    /* 
     * Calculate a suitable transport timeout.
     * Use the operation timeout plus 50%.
     */
    timeout = conf->join_failure_timeout + (conf->join_failure_timeout/2);
    
    r = unifi_mlme_blocking_request(priv, pcli, &signal, &data_ptrs, timeout);
    if (r) {
        unifi_error(priv, "failed to send JOIN request, error %d\n", r);
        return r;
    }
    
    rc = pcli->reply_signal->u.MlmeJoinConfirm.ResultCode;
    if (rc) {
        unifi_notice(priv, "JOIN requestwas rejected with result 0x%X (%s)\n",
                     rc, lookup_result_code(rc));
        return -EIO;
    }
    
    /* Copy BSSID and capability to wireless conf struct */
    memcpy((void*)priv->wext_conf.current_bssid, (void*)si->msi.Bssid.x, 6);
    priv->wext_conf.capability = si->msi.CapabilityInformation;
    priv->wext_conf.channel = si->msi.Channel;

    func_exit();
    return 0;
} /* unifi_do_join() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_do_start
 *
 *      Start an adhoc network using the parameters in priv->wext_conf.
 * 
 * Arguments:
 *	priv            Pointer to driver private struct.
 *
 * Returns:
 *      0 on success, error code on failure
 * ---------------------------------------------------------------------------
 */
static int
unifi_do_start(unifi_priv_t *priv, const char *ssid)
{
    CSR_SIGNAL signal;
    CSR_MLME_START_REQUEST *req = &signal.u.MlmeStartRequest;
    bulk_data_param_t data_ptrs;
    struct wext_config *conf= &priv->wext_conf;
    int timeout = 2000;
    int r, rc;
    int capability;
    int ssid_len;
    static const unsigned char supp_rates[] = {
        0x82, 0x84, 0x8B, 0x96, 12, 24, 48, 72, 18, 36, 96, 108
    };
    const int num_supp_rates = sizeof(supp_rates);
    ul_client_t *pcli = priv->wext_client;
    unsigned char *ie_buf = NULL;
    unsigned int ie_len = 0;

    /* Build the capability word */
    capability = SIG_CAP_SHORT_PREAMBLE;

    if (conf->mode == IW_MODE_INFRA) {
        capability |= SIG_CAP_ESS;
    } else {
        capability |= SIG_CAP_IBSS;
    }
    if (conf->privacy) {
        capability |= SIG_CAP_PRIVACY;
    }
    
    /* 
     * If the user has not selected a valid channel, we will start in 
     * channel 10 or channel 52 (802.11a)
     */
    if (conf->channel <= 0) {
        if (priv->if_index == CSR_INDEX_5G) {
            conf->channel = 52;
        } else {
            conf->channel = 10;
        }
    }

    unifi_trace(priv, UDBG1, "start: capability=0x%X, chan=%d, beacon=%d, ssid=\"%s\"\n",
          capability, conf->channel, conf->beacon_period, ssid);


    /*
     * Build the InformationElements
     */
    ssid_len = strlen(ssid);
    ie_len = ssid_len + num_supp_rates + ((capability & SIG_CAP_IBSS) ? 2 : 0);
    if (ie_len) {
        r = unifi_net_data_malloc(priv, &data_ptrs.d[0], ie_len);
        if (r != 0) {
            unifi_error(priv, "unifi_do_start: failed to allocate ie_buf.\n");
            return -EIO;
        }
        ie_buf = (unsigned char*)data_ptrs.d[0].os_data_ptr;

        /* If SSID is given, add it top the IE vector */
        if (ssid_len) {
            unifi_add_info_element(ie_buf, IE_SSID_ID, ssid, ssid_len);
            ie_len = ssid_len + 2;
        }

        if (num_supp_rates <= 8) {
            unifi_add_info_element(ie_buf + ie_len,
                                   IE_SUPPORTED_RATES_ID, supp_rates, num_supp_rates);
            ie_len += num_supp_rates + 2;
        } else {
            unifi_add_info_element(ie_buf + ie_len,
                                   IE_SUPPORTED_RATES_ID, supp_rates, 8);
            ie_len += 8 + 2;
        }

#if 0
        /* is this needed? */
        {
            unsigned char buf[4];
            buf[0] = conf->channel;
            INFO_ELEM_ADD(IE_DS_PARAM_SET_ID, buf, 1);
        }
#endif
    
#if 0
        /* For AP mode, which is not yet supported */
        /* If an ESS, add a TIM elememt with DTIM Count = 0 to set the DTIM Period. */
        if (capability & SIG_CAP_ESS)
        {
            unsigned char buf[8];
            buf[0] = 0;             /* DTIM Count */
            buf[1] = conf->dtim_period; /* DTIM Period */
            buf[2] = 0;             /* Bitmap Control */
            buf[3] = 0;             /* Partial Virtual Bitmap */
            INFO_ELEM_ADD(IE_TIM_ID, buf, 4);
        }
#endif
    
    
        /* 
        * In an IBSS, UniFi requires an ATIM Window paramter, which is specified
        * in the IBSS Parm Set information element
        * Use the value from the user if given, else use a default of 10% of
        * Beacon period.
        */
        if (capability & SIG_CAP_IBSS) {
            int atim;
            unsigned char buf[8];

            atim = conf->beacon_period / 10;

            buf[0] = atim & 0xFF;
            buf[1] = (atim >> 8) & 0xFF;
            unifi_add_info_element(ie_buf + ie_len, IE_IBSS_PARAM_SET_ID, buf, 2);
            ie_len += 2 + 2;
        }

        /* This comes here so that IE ids are in numeric sequence */
        if (num_supp_rates > 8) {
            unifi_add_info_element(ie_buf + ie_len, IE_EXTENDED_SUPPORTED_RATES_ID,
                                   supp_rates + 8, num_supp_rates - 8);
            ie_len += num_supp_rates - 8 + 2;
        }
    }

    /*
     * Build the request
     */
    memset((void*)req, 0, sizeof(req));

    req->Ifindex = priv->if_index;    /* 2.4GHz or 5GHz operation */

    req->BeaconPeriod = conf->beacon_period;
    req->Channel      = conf->channel;
    req->ProbeDelay   = 100;
    req->CapabilityInformation = capability;

    /* Reset the f/w 802.11 state */
    r = unifi_reset_state(priv, priv->netdev->dev_addr, 0);
    if (r) {
        unifi_notice(priv, "Failed to reset device\n");
        func_exit();
        return r;
    }
    
    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_START_REQUEST_ID;

    /* Do not mess with data_ptrs.d[0].os_net_buf_ptr. */
    data_ptrs.d[0].os_data_ptr = ie_buf;
    data_ptrs.d[0].data_length = ie_len;
    data_ptrs.d[1].os_data_ptr = data_ptrs.d[1].os_net_buf_ptr = NULL;
    data_ptrs.d[1].data_length = 0;

    r = unifi_mlme_blocking_request(priv, pcli, &signal, &data_ptrs, timeout);
    if (r) {
        unifi_error(priv, "failed to send START request, error %d\n", r);
        return r;
    }
    
    rc = pcli->reply_signal->u.MlmeStartConfirm.ResultCode;
    if (rc) {
        unifi_notice(priv, "START request was rejected with result 0x%X (%s)\n",
                     rc, lookup_result_code(rc));
        return -EIO;
    }

    /* Copy BSSID and SSID to wireless conf struct */
    memcpy((void*)priv->wext_conf.current_bssid,
           (void*)pcli->reply_signal->u.MlmeStartConfirm.Bssid.x, 6);
    strncpy(priv->wext_conf.current_ssid, ssid, UNIFI_MAX_SSID_LEN);

    func_exit();
    return 0;
} /* unifi_do_start() */




/*
 * ---------------------------------------------------------------------------
 *  unifi_do_authenticate
 *
 *      Send a MLME-AUTHENTICATE.request to UniFi to begin an
 *      authentication exchange.
 *
 * Arguments:
 *	priv            Pointer to driver private struct.
 *
 * Returns:
 *      0 on success, error code on failure
 * ---------------------------------------------------------------------------
 */
static int
unifi_do_authenticate(unifi_priv_t *priv)
{
    CSR_SIGNAL signal;
    CSR_MLME_AUTHENTICATE_REQUEST *req = &signal.u.MlmeAuthenticateRequest;
    struct wext_config *conf= &priv->wext_conf;
    int timeout;
    int r, rc;
    ul_client_t *pcli = priv->wext_client;

    func_enter();

    unifi_trace(priv, UDBG1, "auth: type=%d, bssid=%02X:%02X:%02X:%02X:%02X:%02X\n",
          conf->auth_type,
          priv->wext_conf.current_bssid[0], priv->wext_conf.current_bssid[1],
          priv->wext_conf.current_bssid[2], priv->wext_conf.current_bssid[3],
          priv->wext_conf.current_bssid[4], priv->wext_conf.current_bssid[5]);

    memcpy((void*)req->PeerStaAddress.x, (void*)priv->wext_conf.current_bssid, 6);
    req->AuthenticationType = conf->auth_type;
    req->AuthenticationFailureTimeout = host2unifi_16(conf->auth_failure_timeout);

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_AUTHENTICATE_REQUEST_ID;

    /*
     * Calculate a suitable transport timeout.
     * Use the operation timeout plus 50%.
     */
    timeout = conf->auth_failure_timeout + conf->auth_failure_timeout/2;
    
    r = unifi_mlme_blocking_request(priv, pcli, &signal, NULL, timeout);
    if (r) {
        unifi_error(priv, "failed to send AUTHENTICATE request, error %d\n", r);
        return r;
    }
    
    rc = pcli->reply_signal->u.MlmeAuthenticateConfirm.ResultCode;
    if (rc) {
        unifi_notice(priv, "AUTHENTICATE request was rejected with result 0x%X (%s)\n",
                     rc, lookup_result_code(rc));
        return -EIO;
    }
    
    func_exit();
    return 0;
} /* unifi_do_authenticate() */



#define WMM_IE_QOS_INFO_OFFSET      8

/*
 * ---------------------------------------------------------------------------
 *  unifi_get_wmm_bss_capabilities
 *
 *      Searches for the WMM IE in the IEs buffer
 *
 * Arguments:
 *	ie_vector            Pointer to the buffer containing the IEs.
 *  ie_len               Size of the buffer
 *
 * Returns:
 *      1 on success, 0 on failure
 * ---------------------------------------------------------------------------
 */
unsigned int
unifi_get_wmm_bss_capabilities(unifi_priv_t *priv, unsigned char *ie_vector,
                               int ie_len, int *ap_capabilities)
{
    int tmp_ie_len = ie_len;
    const unsigned char *tmp_ie_ptr = ie_vector;
    const unsigned char *elem;
    unsigned int fiematch = 0;
    const unsigned char wmm_oui[4] = {0x00, 0x50, 0xF2, 0x02};

    *ap_capabilities = 0;
    /* Search for this IE in the buffer. */
    while (tmp_ie_len > 0)
    {
        /* Get the first matching element */
        elem = unifi_find_info_element(221, tmp_ie_ptr, tmp_ie_len);
        /* if the id does not exist we just want to get out of while loop */
        if (elem) {
            /* match the oui, etc */
            if ((memcmp(elem+2, wmm_oui, 4) == 0) &&
                ((elem[6] == 0x00) || (elem[6] == 0x01)))
            {
                fiematch = 1;
                *ap_capabilities |= QOS_CAPABILITY_WMM_ENABLED;
                if (elem[WMM_IE_QOS_INFO_OFFSET] & 0x80) {
                    unifi_trace(priv, UDBG3, "WMM: Peer has U-APSD flag anabled.\n");
                    *ap_capabilities |= QOS_CAPABILITY_WMM_UAPSD;
                }
                /* If it is a parameter IE we need to search for admission control */
                if ((elem[6] == 0x01) && (elem[1] >= 24)) {
                    int i;
                    for (i=10; i<23; i=i+4) {
                        if (elem[i] & 0x10) {
                            switch (elem[i] & 0x60) {
                              case 0x00:
                                *ap_capabilities |= QOS_CAPABILITY_ACM_BE_ENABLED;
                                break;
                              case 0x20:
                                *ap_capabilities |= QOS_CAPABILITY_ACM_BK_ENABLED;
                                break;
                              case 0x40:
                                *ap_capabilities |= QOS_CAPABILITY_ACM_VI_ENABLED;
                                break;
                              case 0x60:
                                *ap_capabilities |= QOS_CAPABILITY_ACM_VO_ENABLED;
                                break;
                              default:
                                unifi_error(priv, "Unrecognised WMM ACI parameter.\n");
                                break;
                            }
                        }
                    }
                }
                tmp_ie_len = 0;
            }

            /* if they match we just want to get out of while loop */
            if (!fiematch) {
                /* advance to the next IE in the buffer */
                tmp_ie_len -= (elem - tmp_ie_ptr) + elem[1] + 2;
                tmp_ie_ptr = elem + (elem[1] + 2);
            }
        } else {
            tmp_ie_len = 0;
        }
    }
    return fiematch;
} /* unifi_get_wmm_bss_capabilities() */


/*
 * ---------------------------------------------------------------------------
 *  set_wmm_traffic_streams
 *
 *      Sends TSPEC WMM request for any AC that has Admission Control enabled.
 *
 * Arguments:
 *    priv            Pointer to driver private struct.
 *
 * Returns:
 *
 * ---------------------------------------------------------------------------
 */
static int
set_wmm_traffic_streams(unifi_priv_t *priv)
{
    CSR_SIGNAL signal;
    CSR_MLME_ADDTS_REQUEST *req = &signal.u.MlmeAddtsRequest;
    bulk_data_param_t data_ptrs;
    ul_client_t *pcli = priv->wext_client;
    int i, r = 0, rc;
    static const int timeout = 5000;
    static unsigned char psb;
    unsigned char *tspec_ie = NULL;

    const static unsigned char _tspec_ie[] = {
        0xDD, 61,
        0x00, 0x50, 0xF2, 0x02, 0x02, 0x01,
        0xE0, 0x00, 0x00,               /* TS Info Field (Bi-directional)*/
        0xD0, 0x80,                     /* Nominal MSDU Size */
        0x00, 0x00,                     /* Maximum MSDU Size */
        0x20, 0x4e, 0x00, 0x00,         /* Minimum Service Interval */
        0x20, 0x4e, 0x00, 0x00,         /* Maximum Service Interval */
        0x00, 0x00, 0x00, 0x00,         /* Inactivity Interval */
        0x00, 0x00, 0x00, 0x00,         /* Suspension Interval */
        0x00, 0x00, 0x00, 0x00,         /* Service Start Time (if APSD=0 then this is 0)*/
        0x00, 0x00, 0x00, 0x00,         /* Minimum Data Rate */
        0x00, 0x45, 0x01, 0x00,         /* Mean Data Rate */
        0x00, 0x00, 0x00, 0x00,         /* Peak Data Rate */
        0x00, 0x00, 0x00, 0x00,         /* Burst Size (No bursts) */
        0x00, 0x00, 0x00, 0x00,         /* Delay Bound */
        0x80, 0x8D, 0x5B, 0x00,         /* Minimum PHY Rate */
        0x00, 0x24,                     /* Surplus Bandwidth Allowance */
        0x00, 0x00                      /* Medium Time */
    };

    /* Allow a reasonable period between the failure timeout and our request's timeout. */
    req->AddtsFailureTimeout = timeout/2;
    for (i = 0; i < 4; i++) {
        if (priv->wext_conf.bss_wmm_capabilities & (QOS_CAPABILITY_ACM_BE_ENABLED << i)) {

            r = unifi_net_data_malloc(priv, &data_ptrs.d[0], 63);
            if (r != 0) {
                unifi_error(priv, "set_wmm_traffic_streams: failed to allocate tspec_ie.\n");
                return -EIO;
            }
            tspec_ie = (unsigned char*)data_ptrs.d[0].os_data_ptr;

            memcpy(tspec_ie, _tspec_ie, 63);

            if (priv->wext_conf.wmm_bss_uapsd_mask & (0x01 << i)) {
                psb = 0x04;
            } else {
                psb = 0;
            }

            req->DialogToken = 1 << i;
            switch (i) {
              case 1:                       /* ac_bk */
                tspec_ie[8] = 0xE0 | 0x02;
                tspec_ie[9] = psb | 0x08;
                tspec_ie[11] = 0x96;
                tspec_ie[39] = 0x00;
                tspec_ie[40] = 0x08;
                tspec_ie[41] = 0x00;
                break;
              case 3:                       /* ac_vo */
                tspec_ie[8] = 0xE0 | (i << 2);
                tspec_ie[9] = psb | (i << 4);
                tspec_ie[11] = 0xD0;
                tspec_ie[39] = 0x00;
                tspec_ie[40] = 0x45;
                tspec_ie[41] = 0x01;
                break;
              case 0:                       /* ac_be */
              case 2:                       /* ac_vi */
                tspec_ie[8] = 0xE0 | (i << 2);
                tspec_ie[9] = psb | (i << 4);
                tspec_ie[11] = 0x96;
                tspec_ie[39] = 0x00;
                tspec_ie[40] = 0x08;
                tspec_ie[41] = 0x00;
                break;
            }

            signal.SignalPrimitiveHeader.SignalId = CSR_MLME_ADDTS_REQUEST_ID;

            /* Do not mess with data_ptrs.d[0].os_net_buf_ptr. */
            data_ptrs.d[0].os_data_ptr = tspec_ie;
            data_ptrs.d[0].data_length = 63;
            data_ptrs.d[1].os_data_ptr = data_ptrs.d[1].os_net_buf_ptr = NULL;
            data_ptrs.d[1].data_length = 0;

            /* Send an MLME-ADDTS.req */
            r = unifi_mlme_blocking_request(priv, pcli, &signal, &data_ptrs, timeout);
            if (r < 0) {
                unifi_error(priv, "failed to send ADDTS request, error %d\n", r);
                return -EIO;
            }

            rc = pcli->reply_signal->u.MlmeAddtsConfirm.ResultCode;
            if (rc) {
                unifi_notice(priv, "ADDTS request was rejected with result 0x%X (%s)\n",
                             rc, lookup_result_code(rc));
            }

            /* If the reply is successful, enable the TS for this AC */
            if (rc == CSR_RC_SUCCESS) {
                priv->sta_wmm_capabilities |= (QOS_CAPABILITY_TS_BE_ENABLED << i);
            }
        }
    }

    return 0;
} /* set_wmm_traffic_streams() */

#define WMM_IE_SIZE     9


/*
 * ---------------------------------------------------------------------------
 *  unifi_do_associate
 *
 *      Associate with an ESS.
 *
 * Arguments:
 *	priv            Pointer to driver private struct.
 *  si              Pointer to the scan information of the selected BSSID
 *
 * Returns:
 *      0 on success, error code on failure
 * ---------------------------------------------------------------------------
 */
static int
unifi_do_associate(unifi_priv_t *priv, scan_info_t *si)
{
    CSR_SIGNAL signal;
    CSR_MLME_ASSOCIATE_REQUEST *req = &signal.u.MlmeAssociateRequest;
    bulk_data_param_t data_ptrs;
    struct wext_config *conf= &priv->wext_conf;
    ul_client_t *pcli = priv->wext_client;
    int timeout;
    int r, rc;
    unsigned char ie_confirm_vector[IE_VECTOR_MAXLEN];
    int ie_confirm_len;
    unsigned char wmm_ie[] = {
        0xDD, 0x07, 0x00, 0x50, 0xF2, 0x02, 0x00, 0x01, 0x00
    };
    unsigned char *ie_request_buf = NULL;
    unsigned int ie_request_len = 0;
    unsigned char append_wmm_ie = 0;

    func_enter();

    unifi_trace(priv, UDBG1, "assoc: bssid=%02X:%02X:%02X:%02X:%02X:%02X, listen=%d, timeout=%d\n",
          priv->wext_conf.current_bssid[0], priv->wext_conf.current_bssid[1],
          priv->wext_conf.current_bssid[2], priv->wext_conf.current_bssid[3],
          priv->wext_conf.current_bssid[4], priv->wext_conf.current_bssid[5],
          conf->assoc_listen_interval,
          conf->assoc_failure_timeout);

    /* 
     * If the AP supports QOS (WMM) we have to add the WMM IE to our
     * association request. We temporary use the buffer comming from the
     * wireless extensions. Note that we just append the WMM IE, without
     * modifying the length we know for the data in the buffer.
     */
    if (priv->wext_conf.bss_wmm_capabilities & QOS_CAPABILITY_WMM_ENABLED) {
        if ((WMM_IE_SIZE + conf->generic_ie_len) <= IE_VECTOR_MAXLEN) {
            unifi_trace(priv, UDBG3, "unifi_do_associate: adding wmm ie in probe request.\n");
            if (priv->wext_conf.bss_wmm_capabilities & QOS_CAPABILITY_WMM_UAPSD) {
                unifi_trace(priv, UDBG3, "unifi_do_associate: adding U-APSD mask (0x%x).\n",
                      priv->wext_conf.wmm_bss_uapsd_mask);
                wmm_ie[WMM_IE_QOS_INFO_OFFSET] |= priv->wext_conf.wmm_bss_uapsd_mask;
            }
            /* Append the WMM ie at the end of the buffer */
            append_wmm_ie = 1;
            /* update the length we will tell the device */
            ie_request_len = conf->generic_ie_len + WMM_IE_SIZE;
        }
        else {
            unifi_notice(priv, "unifi_do_associate: Not enough room in generic_ie for WMM.\n");
            /* update the length we will tell the device */
            ie_request_len = conf->generic_ie_len;
        }
    }
    else {
        /* update the length we will tell the device */
        ie_request_len = conf->generic_ie_len;
    }

    if (ie_request_len) {
        r = unifi_net_data_malloc(priv, &data_ptrs.d[0], ie_request_len);
        if (r != 0) {
            unifi_error(priv, "unifi_do_associate: failed to allocate ie_request_buf.\n");
            return -EIO;
        }
        ie_request_buf = (unsigned char*)data_ptrs.d[0].os_data_ptr;

        if (conf->generic_ie_len) {
            memcpy(ie_request_buf, conf->generic_ie, conf->generic_ie_len);
        }
        if (append_wmm_ie) {
            memcpy(ie_request_buf + conf->generic_ie_len, wmm_ie, WMM_IE_SIZE);
        }
    }

    memcpy((void*)req->PeerStaAddress.x, (void*)priv->wext_conf.current_bssid, 6);

    req->AssociateFailureTimeout = conf->assoc_failure_timeout;
    req->CapabilityInformation   = (s16)host2unifi_16(conf->capability);
    /* Convert msecs to beacon periods */
    if (si->msi.BeaconPeriod > 0) {
        req->ListenInterval = conf->assoc_listen_interval / si->msi.BeaconPeriod;
    } else {
        req->ListenInterval = 1;
    }

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_ASSOCIATE_REQUEST_ID;

    /* Do not mess with data_ptrs.d[0].os_net_buf_ptr. */
    data_ptrs.d[0].os_data_ptr = ie_request_buf;
    data_ptrs.d[0].data_length = ie_request_len;
    data_ptrs.d[1].os_data_ptr = data_ptrs.d[1].os_net_buf_ptr = NULL;
    data_ptrs.d[1].data_length = 0;

    /*
     * Calculate a suitable transport timeout.
     * Use the operation timeout plus 50%.
     */
    timeout = conf->assoc_failure_timeout + conf->assoc_failure_timeout/2;

    r = unifi_mlme_blocking_request(priv, pcli, &signal, &data_ptrs, timeout);
    if (r < 0) {
        unifi_error(priv, "failed to send ASSOCIATE request, error %d\n", r);
        return r;
    }

    rc = pcli->reply_signal->u.MlmeAssociateConfirm.ResultCode;
    if (rc) {
        unifi_notice(priv, "ASSOCIATE request was rejected with result 0x%X (%s)\n",
                     rc, lookup_result_code(rc));
        return -EIO;
    }

    priv->wext_conf.flag_associated = 1;

    /* 
     * Need to send IW events for assoc request IE and assoc response IE
     * and then a SIOCGIWAP event.
     */
    ie_confirm_len = pcli->reply_signal->u.MlmeAssociateConfirm.InformationElements.DataLength;
    if ((ie_confirm_len > 0) && (ie_confirm_len < IE_VECTOR_MAXLEN)) {
        memcpy(ie_confirm_vector, pcli->reply_bulkdata[0]->ptr, ie_confirm_len);
        wext_send_assoc_event(priv, priv->wext_conf.current_bssid,
                              conf->generic_ie, conf->generic_ie_len,
                              ie_confirm_vector, ie_confirm_len,
                              NULL, 0);

        /* Need to get information about QOS from the assoc response */
        r = unifi_get_wmm_bss_capabilities(priv, ie_confirm_vector,
                                           ie_confirm_len,
                                           &priv->wext_conf.bss_wmm_capabilities);
        if (r == 1) {
            /* Set any Traffic Streams if nessesary. */
            priv->sta_wmm_capabilities = priv->wext_conf.bss_wmm_capabilities;
            set_wmm_traffic_streams(priv);
        } else {
            priv->sta_wmm_capabilities = 0;
        }
    }

    func_exit();
    return 0;
} /* unifi_do_associate() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_set_powermode
 *
 *      Send a MLME-POWERMGT request using the values in the wext_conf
 *      struct.
 *
 * Arguments:
 *	priv            Pointer to driver private struct.
 *
 * Returns:
 *      0 on success, error code on failure
 * ---------------------------------------------------------------------------
 */
int
unifi_set_powermode(unifi_priv_t *priv)
{
    CSR_SIGNAL signal;
    CSR_MLME_POWERMGT_REQUEST *req = &signal.u.MlmePowermgtRequest;
    struct wext_config *conf= &priv->wext_conf;
    int timeout = 1000;
    int r, rc;
    ul_client_t *pcli = priv->wext_client;

    func_enter();

    unifi_trace(priv, UDBG1, "powermgt: mode=%d\n", conf->power_mode);
    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_POWERMGT_REQUEST_ID;

    req->PowerManagementMode = conf->power_mode;
    req->WakeUp = 0;
    req->ReceiveDtims = conf->wakeup_for_dtims;

    r = unifi_mlme_blocking_request(priv, pcli, &signal, NULL, timeout);
    if (r < 0) {
        unifi_error(priv, "failed to send POWERMGT request, error %d\n", r);
        return r;
    }

    rc = pcli->reply_signal->u.MlmePowermgtConfirm.ResultCode;
    if (rc) {
        unifi_notice(priv, "POWERMGT request was rejected with result 0x%X (%s)\n",
                     rc, lookup_result_code(rc));
        return -EIO;
    }
    
    func_exit();
    return 0;
} /* unifi_set_powermode() */



/*
 * ---------------------------------------------------------------------------
 *  _join_ap
 *
 *      Establish ourself as a member of a given Access Point (AP).
 *      This means doing JOIN, AUTHENTICATE and ASSOCIATE actions.
 * 
 *  Arguments:
 *	priv            Pointer to driver private struct.
 *	si              Pointer to scan_info_t entry for the AP tp join.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static int
_join_ap(unifi_priv_t *priv, scan_info_t *si)
{
    int r;

    func_enter();

    /* Reset the f/w 802.11 state */
    r = unifi_reset_state(priv, priv->netdev->dev_addr, 0);
    if (r) {
        unifi_notice(priv, "Failed to reset device\n");
        func_exit();
        return r;
    }
    
    r = unifi_do_join(priv, si);
    if (r) {
        unifi_notice(priv, "Failed to join wireless network\n");
        func_exit();
        return r;
    }

    /* Set power-saving mode if requested, the previous mlme-reset clears it. */
    if ((priv->wext_conf.capability & SIG_CAP_IBSS) == 0) {
        if (priv->wext_conf.power_mode) {
            r = unifi_set_powermode(priv);
            if (r) {
                unifi_notice(priv, "Failed to set powersave mode\n");
                func_exit();
                return r;
            }
        }
    }

    /* 
     * F/W needs to have a dummy protection set before it can transmit
     * the WPA opening sequences.
     */
#if WIRELESS_EXT > 17
    if ((priv->wext_conf.wpa_version == IW_AUTH_WPA_VERSION_WPA) ||
        (priv->wext_conf.wpa_version == IW_AUTH_WPA_VERSION_WPA2))
    {
        r = mlme_set_protection(priv, priv->wext_conf.current_bssid,
                                CSR_PT_NONE, CSR_PAIRWISE);
        if (r) {
            unifi_notice(priv, "Failed to set dummy WPA protection (pairwise key)\n");
            func_exit();
            return r;
        }

        r = mlme_set_protection(priv, priv->wext_conf.current_bssid,
                                CSR_PT_NONE, CSR_GROUP);
        if (r) {
            unifi_notice(priv, "Failed to set dummy WPA protection (group key)\n");
            func_exit();
            return r;
        }
    }
#endif

    /* Procedure for joining an IBSS stops here */
    if ((priv->wext_conf.capability & SIG_CAP_IBSS) != 0) {
        func_exit();
        return 0;
    }

    /* --------------------------------------------------------------------- */
    /* This is for Infrastructure networks */
    
    /* send AUTHENTICATE */
    r = unifi_do_authenticate(priv);
    if (r) {
        unifi_notice(priv, "Failed to authenticate with wireless network\n");
        func_exit();
        return r;
    }
    
    /* send ASSOCIATE */
    r = unifi_do_associate(priv, si);
    if (r) {
        unifi_notice(priv, "Failed to associate with wireless network\n");
        func_exit();
        return r;
    }

    func_exit();
    return 0;
} /* _join_ap() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_join_bss
 *  unifi_join_ap
 *
 *      These two functions do the same task, but use alternate ways
 *      to specify the AP.
 * 
 *  Arguments:
 *	priv            Pointer to driver private struct.
 *	si              Pointer to scan_info_t entry for the AP tp join.
 *      bss             MAC address of AP to join. This must exist in the
 *                      current set of scan results.
 *
 * Returns:
 *      0 on success, error code on failure
 * ---------------------------------------------------------------------------
 */
int
unifi_join_ap(unifi_priv_t *priv, scan_info_t *si)
{
    int r;
    char ssid[UNIFI_MAX_SSID_LEN];
    char *elem;

    /* Find the SSID field */
    memset(ssid, 0, UNIFI_MAX_SSID_LEN);
    elem = (char *)unifi_find_info_element(IE_SSID_ID,
                                           si->info_elems,
                                           si->info_elem_length);
    if (elem) {
        int elen;
        elen = elem[1];
        if (elen > UNIFI_MAX_SSID_LEN) {
            elen = UNIFI_MAX_SSID_LEN;
        }
        strncpy(ssid, elem + 2, elen);
    } else {
        strncpy(ssid, "<unknown>", UNIFI_MAX_SSID_LEN);
    }
    
    /* 
     * We have to search for WMM ie at this point. We are going to use
     * this information later, when we set the association request.
     * There is no way to know if our AP supports QOS other than parsing
     * the IEs we have got from the beacons and probe responses.
     */
    r = unifi_get_wmm_bss_capabilities(priv, si->info_elems,
                                       si->info_elem_length,
                                       &priv->wext_conf.bss_wmm_capabilities);
    if (r == 1) {
        /* Don't expect an association response in Ad-Hoc. */
        if (si->msi.BssType == CSR_INDEPENDENT) {
            unifi_trace(priv, UDBG1, "unifi_join_ap: IBSS supports WMM\n");
            priv->sta_wmm_capabilities = priv->wext_conf.bss_wmm_capabilities;
        }
    } else {
        priv->sta_wmm_capabilities = 0;
    }
    
    unifi_trace(priv, UDBG1, "joining ssid=\"%s\"\n", ssid);
    r = _join_ap(priv, si);

    if (r) {
        unifi_notice(priv, "Failed to join \"%s\"\n", ssid);
    } else {
        /* Success. Remember the SSID and report it in the log. */
        strncpy(priv->wext_conf.current_ssid, ssid, UNIFI_MAX_SSID_LEN);

        if ((priv->wext_conf.capability & SIG_CAP_IBSS) != 0) {
            unifi_notice(priv, "Joined ad-hoc network \"%s\"\n", priv->wext_conf.current_ssid);
        } else {
            unifi_notice(priv, "Associated with \"%s\"\n", priv->wext_conf.current_ssid);
        }
    }

    return r;
} /* unifi_join_ap() */


int
unifi_join_bss(unifi_priv_t *priv, unsigned char *macaddr)
{
    scan_info_t *psi;
    int i;
    char *elem;
    int desired_ssid_len = strlen(priv->wext_conf.desired_ssid);

    func_enter();

    /* Look for bss in scan list */
    psi = NULL;
    
    for (i = 0; 1; i++) {
        psi = unifi_get_scan_report(priv, i);
        if (psi == NULL) {
            unifi_trace(priv, UDBG2, "Exhausted Scan Results.\n");
            break;
        }
        /* Check if the requested BSSID matches the scan result. */
        if (memcmp(macaddr, psi->msi.Bssid.x, ETH_ALEN) == 0) {
            /* The requested BSSID must belong to the last set SSID. */
            if (desired_ssid_len > 0) {
                /* Get the SSID from the san result.*/
                elem = (char *)unifi_find_info_element(IE_SSID_ID,
                                                       psi->info_elems,
                                                       psi->info_elem_length);
                if (elem != NULL) {
                    int elen;
                    elen = elem[1];
                    if (elen > UNIFI_MAX_SSID_LEN) {
                        elen = UNIFI_MAX_SSID_LEN;
                    }
                    /* The lengths must be equal .. */
                    if (elen == desired_ssid_len) {
                        /* .. and the SSIDs. */
                        if (memcmp(priv->wext_conf.desired_ssid, elem + 2, elen) == 0) {
                            break;
                        }
                    }
                } else {
                    unifi_notice(priv, "Scan Result with no SSID IE.\n");
                }
            }
            else {
                /* found it */
                unifi_trace(priv, UDBG2, "Desired SSID empty, BSSID match..\n");
                break;
            }
        }
    }

    if (psi == NULL) {
        unifi_error(priv, "Requested AP BSS not found in scan list\n");
        return -EINVAL;
    }

    func_exit();

    return unifi_join_ap(priv, psi);
} /* unifi_join_bss() */



/* 
 * ---------------------------------------------------------------------------
 * unifi_autojoin
 *
 *      This function implements a very basic Station Management Entity (SME).
 *
 *      Perform all the operations needed to join a WiFi network.
 *      This involves:
 *       - a scan to detect networks (adhoc and AP) available
 *       - MLME-JOIN to join the network or MLME-START to start an adhoc
 *         network
 *       - authenticate and associate
 *
 * Arguments:
 *	priv            Pointer to driver private struct.
 *
 * Returns:
 *      0 on success, error code on failure
 *
 * Notes:
 *      Results of the scan are stored by the driver core in the card
 *      struct. We use unifi_get_scan_report() to retrieve the list.
 * ---------------------------------------------------------------------------
 */
int
unifi_autojoin(unifi_priv_t *priv, const char *ssid)
{
    struct wext_config *conf;
    scan_info_t *si;
    int i, r;
    int ssid_len = 0;
    int best_snr;
    CSR_BSS_TYPE bsstype;

    conf = &priv->wext_conf;

    /*
     * Scan for networks.
     */
    switch (conf->mode) {
      case IW_MODE_ADHOC:       bsstype = CSR_INDEPENDENT;      break;
      case IW_MODE_INFRA:       bsstype = CSR_INFRASTRUCTURE;   break;
      default:                  bsstype = CSR_ANY_BSS;          break;
    }

    LOCK_DRIVER(priv);
    unifi_trace(priv, UDBG1, "scanning for bsstype %d\n", bsstype);
    if (ssid) {
        ssid_len = strlen(ssid);
        r = unifi_do_scan(priv, UNIFI_SCAN_ACTIVE, bsstype, ssid, ssid_len);
    } else {
        /* Wildcard scan */
        r = unifi_do_scan(priv, UNIFI_SCAN_ACTIVE, bsstype, NULL, 0);
    }
    UNLOCK_DRIVER(priv);
    if (r) {
        unifi_error(priv, "Scan failed\n");
        func_exit();
        return r;
    }

    /* Drop any existing association */
    if (priv->wext_conf.flag_associated) {
        unifi_leave(priv);
    }


    /* If an SSID is given, search scan list for a matching SSID. */
    /* Otherwise take first found */
    si = NULL;
    if (ssid == NULL)
    {
        /* No SSID given, just take the first in the list */
        /* Could look for the strongest here */
        unifi_trace(priv, UDBG1, "selecting first AP in scan list\n");
        si = unifi_get_scan_report(priv, 0);
    }
    else
    {
        unifi_trace(priv, UDBG1, "checking scan list for SSID \"%s\"\n", ssid);
        best_snr = -1;  /* force first result to win */
        for (i = 0; 1; i++) {
            scan_info_t *psi;
            char *elem;
            int e_len;
            char *e_ptr;

            psi = unifi_get_scan_report(priv, i);
            if (!psi) {
                break;
            }

            elem = (char *)unifi_find_info_element(IE_SSID_ID,
                                                   psi->info_elems,
                                                   psi->info_elem_length);
            if (!elem) {
                /* No SSID field in this scan result */
                continue;
            }

            e_len = elem[1];
            e_ptr = elem + 2;
#if 0
            /* debugging */
            if (e_len) {
                printk(" comparing entry %d: \"%s\"\n", i, e_ptr);
            } else {
                printk(" entry %d has zero length SSID\n", i);
            }
#endif
            if ((e_len == ssid_len) && (strncmp(e_ptr, ssid, e_len) == 0))
            {
                int snr;
                /* Found a match, is this the strongest signal? */
                /* SNR is an unsigned 16-bit value. */
                snr = (s16)unifi2host_16(psi->msi.Snr);
                if (snr > best_snr) {
                    si = psi;
                }
            }
        }
    }
    /* Didn't find requested network */
    if (si == NULL)
    {
        unifi_trace(priv, UDBG1, "Network not found in scan list\n");
        /* 
         * If an SSID was given and mode is ADHOC then start an IBSS.
         */
        if (ssid_len && (conf->mode == IW_MODE_ADHOC))
        {
            r = unifi_do_start(priv, ssid);
            if (r) {
                func_exit();
                return r;
            }
            unifi_notice(priv, "Started adhoc network \"%s\"\n", ssid);

            func_exit();
            return 0;
        }

        /* Couldn't find a network to join, report failure. */
        if (ssid_len == 0) {
            unifi_notice(priv, "Didn't find a network to join\n");
        } else {
            unifi_notice(priv, "Network not found\n");
        }

        func_exit();
        return -ENETUNREACH;
    }

    /* send JOIN with info for desired network */
    r = unifi_join_ap(priv, si);
    if (r) {
        /* Clean up if JOIN fails */
        memset(priv->wext_conf.current_ssid, 0, UNIFI_MAX_SSID_LEN);
        memset((void*)priv->wext_conf.current_bssid, 0, ETH_ALEN);
        priv->wext_conf.capability = 0;
        priv->wext_conf.flag_associated = 0;
    }

    func_exit();
    return 0;
} /* unifi_autojoin() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_leave
 *
 *      Disassociate from a BSS.
 * 
 *  Arguments:
 *	priv            Pointer to driver context.
 *
 *  Returns:
 *      0 on success or error code.
 * ---------------------------------------------------------------------------
 */
int
unifi_leave(unifi_priv_t *priv)
{
    CSR_SIGNAL signal;
    CSR_MLME_DISASSOCIATE_REQUEST *req = &signal.u.MlmeDisassociateRequest;
    bulk_data_param_t data_ptrs;
    struct wext_config *conf= &priv->wext_conf;
    int timeout = 5000;
    int r;
    ul_client_t *pcli = priv->wext_client;
    unsigned char *ie_request_buf = NULL;
    unsigned int ie_request_len = 0;
    CsrInt32 csr_r;

    func_enter();

    unifi_notice(priv, "Leaving wireless network \"%s\"\n", priv->wext_conf.current_ssid);

    /* Configure the deep sleep signaling */
    csr_r = unifi_configure_low_power_mode(priv->card,
                                           UNIFI_LOW_POWER_ENABLED,
                                           UNIFI_PERIODIC_WAKE_HOST_DISABLED);
    if (csr_r) {
        unifi_warning(priv,
                      "unifi_leave: unifi_configure_low_power_mode() returned an error\n");
    }

    memcpy((void*)req->PeerStaAddress.x,
           (void*)priv->wext_conf.current_bssid, ETH_ALEN);
    req->ReasonCode = 1;         /* UnspecifiedReason */

#if 0
    printk("disass: addr %02X:%02X:%02X:%02X:%02X:%02X\n",
           req.PeerStaAddress.x[0], req.PeerStaAddress.x[1], req.PeerStaAddress.x[2], 
           req.PeerStaAddress.x[3], req.PeerStaAddress.x[4], req.PeerStaAddress.x[5]);
#endif

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_DISASSOCIATE_REQUEST_ID;

    if (conf->generic_ie_len) {
        r = unifi_net_data_malloc(priv, &data_ptrs.d[0], conf->generic_ie_len);
        if (r != 0) {
            unifi_error(priv, "unifi_leave: failed to allocate ie_request_buf.\n");
            return -EIO;
        }
        ie_request_buf = (unsigned char*)data_ptrs.d[0].os_data_ptr;

        memcpy(ie_request_buf, conf->generic_ie, conf->generic_ie_len);
        ie_request_len = conf->generic_ie_len;
    }

    /* Do not mess with data_ptrs.d[0].os_net_buf_ptr. */
    data_ptrs.d[0].os_data_ptr = ie_request_buf;
    data_ptrs.d[0].data_length = ie_request_len;
    data_ptrs.d[1].os_data_ptr = data_ptrs.d[1].os_net_buf_ptr = NULL;
    data_ptrs.d[1].data_length = 0;

    r = unifi_mlme_blocking_request(priv, pcli, &signal, &data_ptrs, timeout);
    if (r < 0) {
        unifi_error(priv, "failed to send DISASSOCIATE request, error %d\n", r);
        return r;
    }

    r = pcli->reply_signal->u.MlmeDisassociateConfirm.ResultCode;
    if (r) {
        unifi_notice(priv, "DISASSOCIATE request was rejected with result 0x%X (%s)\n",
                     r, lookup_result_code(r));
        /* Not fatal, we still want to clean up as if it succeeded. */
    }
    
    /*
     * Send wireless-extension event up to userland to announce
     * connection lost to a BSS.
     */
    wext_send_disassoc_event(priv);

    /*
     * Invalidate the generic_ie information.
     * When the wpa_supplicant re-configures us, it will set the new IEs.
     */
    conf->generic_ie_len = 0;

    memset(priv->wext_conf.current_ssid, 0, UNIFI_MAX_SSID_LEN);
    memset((void*)priv->wext_conf.current_bssid, 0, ETH_ALEN);
    priv->wext_conf.capability = 0;
    priv->wext_conf.flag_associated = 0;

    func_exit();

    return 0;
} /* unifi_leave() */



/*
 * -------------------------------------------------------------------------
 *  unifi_reset_state
 *
 *      Ensure that a MAC address has been set.
 *      Send the MLME-RESET signal.
 *      This must be called at least once before starting to do any
 *      network activities (e.g. scan, join etc).
 *
 * Arguments:
 *      priv            Pointer to device private context struct
 *      macaddr         Pointer to chip MAC address.
 *                      If this is FF:FF:FF:FF:FF:FF it will be replaced
 *                      with the MAC address from the chip.
 *      set_default_mib 1 if the f/w must reset the MIB to the default values
 *                      0 otherwise
 *
 * Returns:
 *      0 on success, an error code otherwise.
 * -------------------------------------------------------------------------
 */
int
unifi_reset_state(unifi_priv_t *priv, unsigned char *macaddr,
                  unsigned char set_default_mib)
{
    ul_client_t *pcli = priv->wext_client;
    int r;

    func_enter();

    r = unifi_mlme_reset(priv, pcli, macaddr, set_default_mib);
    if (r) {
        unifi_notice(priv, "MLME-RESET request was rejected with result 0x%X (%s)\n",
                     r, lookup_result_code(r));
    }

    /* The reset clears any 802.11 association. */
    priv->wext_conf.flag_associated = 0;
    /*
     * The counter for the multicast addresses list needs
     * to be clear after reset, since the f/w clears the list.
     */
    priv->mc_list_count_stored = 0;

    func_exit();
    return r;
} /* unifi_reset_state() */




/*
 * ---------------------------------------------------------------------------
 *
 *      Utility functions.
 *
 * ---------------------------------------------------------------------------
 */

/*
 * ---------------------------------------------------------------------------
 * unifi_find_info_element
 *
 *      Search an Information Element vector for a particular id number.
 *
 * Arguments:
 *      id      IE number to search for.
 *      info    Pointer to the IE vector to search.
 *      len     Length of the IE vector.
 *
 * Returns:
 *      Pointer to info element or NULL if not found.
 *  
 * Notes:
 * ---------------------------------------------------------------------------
 */
const unsigned char *
unifi_find_info_element(int id, const unsigned char *info, int len)
{
    const unsigned char *ie = info;

    while (len > 1)
    {
        int e_id, e_len;
        e_id = ie[0];
        e_len = ie[1];

        /* Return if we find a match */
        if (e_id == id)
        {
            return ie;
        }

        len -= (e_len + 2);
        ie  += (e_len + 2);
    }

    return NULL;
} /* unifi_find_info_element() */



/*
 * ---------------------------------------------------------------------------
 * unifi_add_info_element
 *
 *      Add an Information Element to an IE vector.
 *
 * Arguments:
 *      info            Pointer to position in the IE vector to write to.
 *      ie_id           IE number to add.
 *      ie_data         IE number to add.
 *      ie_len          Length of the data to add.
 *
 * Returns:
 *      Number of bytes written to info
 *  
 * Notes:
 *      Assumes there is enough space in info for ie_len+2 bytes.
 * ---------------------------------------------------------------------------
 */
int
unifi_add_info_element(unsigned char *info,
                       int ie_id,
                       const unsigned char *ie_data, int ie_len)
{
    info[0] = ie_id & 0xff;
    info[1] = ie_len & 0xff;

    memcpy(info + 2, ie_data, ie_len & 0xff);

    return (ie_len & 0xff) + 2;
} /* unifi_add_info_element() */


