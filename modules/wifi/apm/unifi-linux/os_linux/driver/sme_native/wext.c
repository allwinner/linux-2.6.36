/*
 * ---------------------------------------------------------------------------
 * FILE:     wext.c
 *
 * PURPOSE:
 *      Handlers for ioctls from iwconfig.
 *      These provide the control plane operations.
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include <linux/types.h>
#include <linux/etherdevice.h>
#include <linux/if_arp.h>
#include <asm/uaccess.h>
#include "driver/unifi.h"
#include "driver/conversions.h"
#include "unifi_priv.h"

#define MAX_WPA_IE_LEN 64

/* The first channel in the 5GHz WLAN band */
#define FIRST_5GHZ_CHANNEL      32


/* MIB definitions */
#define dot11WEPDefaultKeyValue "1.2.840.10036.1.3.1.2"
#define dot11PrivacyInvoked     "1.2.840.10036.1.5.1.1"
#define dot11WEPDefaultKeyID    "1.2.840.10036.1.5.1.2"
#define dot11RSNAEnabled        "1.2.840.10036.1.5.1.7"

#define unifiRSSI               "1.3.6.1.4.1.22164.1.1.3.6.1"
#define unifiSNR                "1.3.6.1.4.1.22164.1.1.3.6.3"
#define unifiTxDataRate         "1.3.6.1.4.1.22164.1.1.3.6.5"
#define unifiFixTxDataRate      "1.3.6.1.4.1.22164.1.1.4.1.10"
#define unifiFixMaxTxDataRate   "1.3.6.1.4.1.22164.1.1.4.1.11"

#define dot11RTSThreshold       "1.2.840.10036.2.1.1.2"
#define dot11FragmentationThreshold "1.2.840.10036.2.1.1.5"
#define dot11RSNAConfigGroupCipher  "1.2.840.10036.1.9.1.4"
#define unifiMLMEFaultReportLevel   "1.3.6.1.4.1.22164.1.1.1.2"


#define CHECK_INITED(_priv)                             \
do {                                                    \
    if (_priv->init_progress != UNIFI_INIT_COMPLETED) { \
        return -ENODEV;                                 \
    }                                                   \
} while (0)


/*
 * ---------------------------------------------------------------------------
 *      Helper functions
 * ---------------------------------------------------------------------------
 */


/*
 * ---------------------------------------------------------------------------
 *  wext_freq_to_channel
 *  channel_to_mhz
 *
 *      These functions convert between channel number and frequency.
 *
 *  Arguments:
 *      ch      Channel number, as defined in 802.11 specs
 *      m, e    Mantissa and exponent as provided by wireless extension.
 *
 *  Returns:
 *      channel or frequency (in MHz) value
 * ---------------------------------------------------------------------------
 */
static int
wext_freq_to_channel(int m, int e)
{
    int mhz;

    mhz = m;
    while (e < 6) {
        mhz /= 10;
        e++;
    }
    while (e > 6) {
        mhz *= 10;
        e--;
    }

    if (mhz >= 5000) {
        return ((mhz - 5000) / 5);
    }

    if (mhz == 2482) {
        return 14;
    }

    if (mhz >= 2407) {
        return ((mhz - 2407) / 5);
    }

    return 0;
} /* wext_freq_to_channel() */

static int
channel_to_mhz(int ch, int dot11a)
{

    if (ch == 0) return 0;
    if (ch > 200) return 0;

    /* 5G */
    if (dot11a) {
        return (5000 + (5 * ch));
    }

    /* 2.4G */
    if (ch == 14) {
        return 2484;
    }

    if ((ch < 14) && (ch > 0)) {
        return (2407 + (5 * ch));
    }

    return 0;
}



/*
 * ---------------------------------------------------------------------------
 *  set_wep_key
 *
 *      Send the given WEP key to UniFi.
 *
 *  Arguments:
 *      priv            Pointer to driver private struct
 *      keynum          Which key to set, 1..4.
 *      keyval          Value of the key to set
 *      keylen          Number of bytes in the key.
 *
 *  Returns:
 *      0 on success, linux error code on error.
 * ---------------------------------------------------------------------------
 */
static int
set_wep_key(unifi_priv_t *priv, int keynum, u8 *keyval, int keylen)
{
    char oidstr[128];
    int err;

    snprintf(oidstr, 128, "%s.%d.%d",
             dot11WEPDefaultKeyValue, priv->if_index, keynum);

    err = unifi_set_mib_string(priv, priv->wext_client, oidstr, keyval, keylen);
    if (err < 0) {
        return -EIO;
    }
    if (err > 0) {
        return -EINVAL;
    }

    /* save key for GIWENCODE */
    priv->wext_conf.wep_keys[keynum-1].len = keylen;
    memcpy(priv->wext_conf.wep_keys[keynum-1].key, keyval, keylen);

    return 0;
} /* set_wep_key() */

/*
 * ---------------------------------------------------------------------------
 *  set_default_wep_key
 *
 *      Set the default WEP key to use.
 *
 *  Arguments:
 *      priv            Pointer to driver private struct
 *      keynum          Which key to set, 0..3.
 *
 *  Returns:
 *      0 on success, linux error code on error.
 * ---------------------------------------------------------------------------
 */
static int
set_default_wep_key(unifi_priv_t *priv, int keynum)
{
    char oidstr[128];
    int err;

    snprintf(oidstr, 128, "%s.%d",
             dot11WEPDefaultKeyID, priv->if_index);

    err = unifi_set_mib_int(priv, priv->wext_client, oidstr, keynum);
    if (err < 0) {
        return -EIO;
    }
    if (err > 0) {
        return -EINVAL;
    }

    return 0;
} /* set_default_wep_key() */

/*
 * ---------------------------------------------------------------------------
 *  set_privacy_invoked
 *
 *      Send a WEP-enable MIB to UniFi.
 *
 *  Arguments:
 *      priv            Pointer to driver private struct
 *      enable          Enable WEP if non-zero.
 *
 *  Returns:
 *      0 on success, linux error code on error.
 * ---------------------------------------------------------------------------
 */
static int
set_privacy_invoked(unifi_priv_t *priv, int privacy)
{
    char oidstr[128];
    int err;

    /* Build the OID string by appending the if_index */
    snprintf(oidstr, 128, "%s.%d", dot11PrivacyInvoked, priv->if_index);

    if (privacy) {
        err = unifi_set_mib_int(priv, priv->wext_client, oidstr, 1);
    } else {
        err = unifi_set_mib_int(priv, priv->wext_client, oidstr, 2); /* MIB false ! */
    }
    if (err < 0) {
        return -EIO;
    }
    if (err > 0) {
        return -EINVAL;
    }

    priv->wext_conf.privacy = privacy;

    return err;
} /* set_privacy_invoked() */

#if 0
static int
set_mlme_fault_report_level(unifi_priv_t *priv, int fault_level)
{
    int err;

    err = unifi_set_mib_int(priv, priv->wext_client, unifiMLMEFaultReportLevel, fault_level);
    if (err < 0) {
        return -EIO;
    }
    if (err > 0) {
        return -EINVAL;
    }

    return 0;
} /* set_mlme_fault_report_level */
#endif /* 0 */

/*
 * ---------------------------------------------------------------------------
 *  set_rsna_config_group_cipher
 *
 *      Send the supported group cipher suite MIB to UniFi.
 *
 *  Arguments:
 *      priv            Pointer to driver private struct
 *      wpa_mode        WPA or RSN mode.
 *      group_cipher    The desired group cipher suite
 *
 *  Returns:
 *      0 on success, linux error code on error.
 * ---------------------------------------------------------------------------
 */
#if WIRELESS_EXT > 17
static int
set_rsna_config_group_cipher(unifi_priv_t *priv, unsigned char wpa_mode, int group_cipher)
{
    const unsigned char WPA_OUI[3] = {0x00, 0x50, 0xf2};
    const unsigned char RSN_OUI[3] = {0x00, 0x0f, 0xac};
    char oidstr[128];
    unsigned char cipher[4];
    int err;

    /*
     * Translate the group cipher suite set the wpa_supplicant,
     * to 802.11i IE cipher suite values.
     */
    switch (group_cipher) {
        case 0x01:
            cipher[3] = (unsigned char)0x00;
            break;
        case 0x02:
            cipher[3] = (unsigned char)0x01;
            break;
        case 0x04:
            cipher[3] = (unsigned char)0x02;
            break;
        case 0x08:
            cipher[3] = (unsigned char)0x04;
            break;
        case 0x10:
            cipher[3] = (unsigned char)0x05;
            break;
        default:
            return 0;
    }

    /* Create the OUI+SC for setting to the MIB */
    switch (wpa_mode) {
        case 2:
            memcpy(cipher, WPA_OUI, 3);
            break;

        case 4:
            memcpy(cipher, RSN_OUI, 3);
            break;

        default:
            return 0;
    }

    /* Build the OID string by appending the if_index */
    snprintf(oidstr, 128, "%s.%d", dot11RSNAConfigGroupCipher, priv->if_index);

    err = unifi_set_mib_string(priv, priv->wext_client, oidstr, cipher, 4);
    if (err < 0) {
        return -EIO;
    }
    if (err > 0) {
        return -EINVAL;
    }

    return 0;
} /* set_rsna_config_group_cipher() */
#endif /* WIRELESS_EXT > 17 */


/*
 * ---------------------------------------------------------------------------
 *  set_rts_threshold
 *
 *      Send the RTS threshold MIB to UniFi.
 *
 *  Arguments:
 *      priv            Pointer to driver private struct
 *      rts_value       Value of the RTS threshold. 0...2347
 *
 *  Returns:
 *      0 on success, linux error code on error.
 * ---------------------------------------------------------------------------
 */
static int
set_rts_threshold(unifi_priv_t *priv, int rts_value)
{
    char oidstr[128];
    int err;

    /* Build the OID string by appending the if_index */
    snprintf(oidstr, 128, "%s.%d", dot11RTSThreshold, priv->if_index);

    err = unifi_set_mib_int(priv, priv->wext_client, oidstr, rts_value);
    if (err < 0) {
        return -EIO;
    }
    if (err > 0) {
        return -EINVAL;
    }

    /* Store the new threshold to the private data */
    priv->wext_conf.rts_thresh = rts_value;

    return 0;
} /* set_rts_threshold */


/*
 * ---------------------------------------------------------------------------
 *  set_fragmentation_threshold
 *
 *      Send the Fragmentation threshold MIB to UniFi.
 *
 *  Arguments:
 *      priv            Pointer to driver private struct
 *      frag_value      Value of the RTS threshold. 256...2346
 *
 *  Returns:
 *      0 on success, linux error code on error.
 * ---------------------------------------------------------------------------
 */
static int
set_fragmentation_threshold(unifi_priv_t *priv, int frag_value)
{
    char oidstr[128];
    int err;

    /* Build the OID string by appending the if_index */
    snprintf(oidstr, 128, "%s.%d", dot11FragmentationThreshold, priv->if_index);

    /* Fragmentation Threshold must be even */
    err = unifi_set_mib_int(priv, priv->wext_client, oidstr, (frag_value & ~0x1));
    if (err < 0) {
        return -EIO;
    }
    if (err > 0) {
        return -EINVAL;
    }

    /* Store the new threshold to the private data */
    priv->wext_conf.frag_thresh = (frag_value & ~0x1);

    return 0;
} /* set_fragmentation_threshold */


/*
 * ---------------------------------------------------------------------------
 *  set_rsna_enabled
 *
 *      Send a WEP-enable MIB to UniFi.
 *
 *  Arguments:
 *      priv            Pointer to driver private struct
 *      enable          Enable WEP if non-zero.
 *
 *  Returns:
 *      0 on success, linux error code on error.
 * ---------------------------------------------------------------------------
 */
#if WIRELESS_EXT > 17
static int
set_rsna_enabled(unifi_priv_t *priv, int enabled)
{
    char oidstr[128];
    int err;

    /* Build the OID string by appending the if_index */
    snprintf(oidstr, 128, "%s.%d", dot11RSNAEnabled, priv->if_index);

    if (enabled) {
        err = unifi_set_mib_int(priv, priv->wext_client, oidstr, 1);
    } else {
        err = unifi_set_mib_int(priv, priv->wext_client, oidstr, 2); /* MIB false ! */
    }

    if (err < 0) {
        return -EIO;
    }
    if (err > 0) {
        return -EINVAL;
    }

    return 0;
} /* set_rsna_enabled() */


/*
 * ---------------------------------------------------------------------------
 *  set_rsn_key
 *
 *      Send a RSNA (i.e. WPA/WPA2) key to UniFi.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static int
set_rsn_key(unifi_priv_t *priv, unsigned char *addr,
            int algorithm,
            int keyid, unsigned char *keydata, int key_len,
            CSR_KEY_TYPE key_type, unsigned char *rx_seq)
{
    CSR_SIGNAL signal;
    CSR_MLME_SETKEYS_REQUEST *req = &signal.u.MlmeSetkeysRequest;
    bulk_data_param_t data_ptrs;
    unsigned char *CSS;
    int timeout = 1000;
    int r;
    ul_client_t *pcli = priv->wext_client;
    unsigned char *request_buf = NULL;
    unsigned int request_len = key_len;

    req->Length = key_len * 8;       /* in bits */
    req->KeyId = keyid;
    req->KeyType = key_type;
    memcpy(&req->Address, addr, ETH_ALEN);

    /* Receive Sequence Count (RSC) */
    if (rx_seq) {
        memcpy(&req->SequenceNumber, rx_seq, 8);
    } else {
        memset(&req->SequenceNumber, 0, 8);
    }

    /*
     * AuthenticatorSupplicantOrInitiatorPeer
     * As a station in a Managed network, we are always the Supplicant.
     * If we ever use RSN in an IBSS, we might be the Authenticator.
     */
    req->AuthenticatorSupplicantOrInitiatorPeer = 0;


    /* Set up Cipher Suite Selector */
    CSS = (unsigned char *)&req->CipherSuiteSelector;
    switch (priv->wext_conf.wpa_version) {
      case IW_AUTH_WPA_VERSION_WPA:
        CSS[0] = 0x00;
        CSS[1] = 0x50;
        CSS[2] = 0xF2;
        break;
      case IW_AUTH_WPA_VERSION_WPA2:
        CSS[0] = 0x00;
        CSS[1] = 0x0F;
        CSS[2] = 0xAC;
    }
    switch (algorithm) {
        /* IW_ENCODE_ALG_NONE, IW_ENCODE_ALG_WEP handled above */
      case IW_ENCODE_ALG_TKIP:
        CSS[3] = 0x02;
        break;
      case IW_ENCODE_ALG_CCMP:
        CSS[3] = 0x04;
        break;
      default:
        unifi_trace(priv, UDBG1, KERN_ERR "Invalid WE encrpytion algorithm: %d\n", algorithm);
        return -EINVAL;
    }

    unifi_trace(priv, UDBG1, "SETKEYS: key %d, len %d, CSS=%02x:%02x:%02x:%02x\n"
          "         addr=%02x:%02x:%02x:%02x:%02x:%02x, RSC=%lld\n"
          "         SN=%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
          keyid, key_len,
          CSS[0], CSS[1], CSS[2], CSS[3],
          req->Address.x[0], req->Address.x[1], req->Address.x[2],
          req->Address.x[3], req->Address.x[4], req->Address.x[5],
          req->SequenceNumber[0],req->SequenceNumber[1],req->SequenceNumber[2],
          req->SequenceNumber[3],req->SequenceNumber[4],req->SequenceNumber[5],
          req->SequenceNumber[6],req->SequenceNumber[7]);

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_SETKEYS_REQUEST_ID;

    if (request_len) {
        r = unifi_net_data_malloc(priv, &data_ptrs.d[0], request_len);
        if (r != 0) {
            unifi_error(priv, "set_rsn_key: failed to allocatd request_buf.\n");
            return -EIO;
        }
        request_buf = (unsigned char*)data_ptrs.d[0].os_data_ptr;

        memcpy(request_buf, keydata, request_len);
    }

    /* Do not mess with data_ptrs.d[0].os_net_buf_ptr. */
    data_ptrs.d[0].os_data_ptr = request_buf;
    data_ptrs.d[0].data_length = request_len;
    data_ptrs.d[1].os_data_ptr = data_ptrs.d[1].os_net_buf_ptr = NULL;
    data_ptrs.d[1].data_length = 0;

    /*
     * Queue request to UniFi and wait for confirm.
     */
    r = unifi_mlme_blocking_request(priv, pcli, &signal, &data_ptrs, timeout);
    if (r < 0) {
        unifi_error(priv, "failed to send SETKEYS request, error %d\n", r);
        return r;
    }
    
    r = pcli->reply_signal->u.MlmeSetkeysConfirm.ResultCode;
    if (r) {
        unifi_notice(priv, "SETKEYS request was rejected with result 0x%X (%s)\n",
                     r, lookup_result_code(r));
        return -EIO;
    }
    

    return 0;
} /* set_rsn_key() */

#endif /* WIRELESS_EXT > 17 */



/*
 * ---------------------------------------------------------------------------
 *  mlme_set_protection
 *
 *      Send a SETPROTECTION MLME command to UniFi.
 *
 *  Arguments:
 *      priv            Pointer to driver private struct
 *      prot            The protection to set:
 *                        CSR_PT_NONE
 *                        CSR_PT_RX
 *                        CSR_PT_TX
 *                        CSR_PT_RX_TX
 *      key_type        Key type, one of:
 *                        CSR_GROUP
 *                        CSR_PAIRWISE
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
int
mlme_set_protection(unifi_priv_t *priv, unsigned char *addr,
                    CSR_PROTECT_TYPE prot, CSR_KEY_TYPE key_type)
{
    CSR_SIGNAL signal;
    CSR_MLME_SETPROTECTION_REQUEST *req = &signal.u.MlmeSetprotectionRequest;
    int r;
    int timeout = 1000;
    ul_client_t *pcli = priv->wext_client;

    unifi_trace(priv, UDBG1, "SETPROTECTION: addr: %02x:%02x:%02x:%02x:%02x:%02x prot %d, type %d\n",
          addr[0], addr[1], addr[2],
          addr[3], addr[4], addr[5],
          prot, key_type);

    memcpy(&req->Address, addr, ETH_ALEN);
    req->ProtectType = prot;
    req->KeyType = key_type;

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_SETPROTECTION_REQUEST_ID;
    
    r = unifi_mlme_blocking_request(priv, pcli, &signal, NULL, timeout);
    if (r < 0) {
        unifi_error(priv, "failed to send SETPROTECTION request, error %d\n", r);
        return r;
    }

    r = pcli->reply_signal->u.MlmeSetprotectionConfirm.ResultCode;
    if (r) {
        unifi_notice(priv, "SETPROTECTION request was rejected with result 0x%X (%s)\n",
                     r, lookup_result_code(r));
        return -EIO;
    }
    
    return 0;
} /* mlme_set_protection() */




/*
 * ---------------------------------------------------------------------------
 *      WEXT methods
 * ---------------------------------------------------------------------------
 */

/*
 * ---------------------------------------------------------------------------
 *  unifi_giwname   - handler for SIOCGIWNAME
 *  unifi_siwfreq   - handler for SIOCSIWFREQ
 *  unifi_giwfreq   - handler for SIOCGIWFREQ
 *  unifi_siwmode   - handler for SIOCSIWMODE
 *  unifi_giwmode   - handler for SIOCGIWMODE
 *  unifi_giwrange  - handler for SIOCGIWRANGE
 *  unifi_siwap     - handler for SIOCSIWAP
 *  unifi_giwap     - handler for SIOCGIWAP
 *  unifi_siwscan   - handler for SIOCSIWSCAN
 *  unifi_giwscan   - handler for SIOCGIWSCAN
 *  unifi_siwessid  - handler for SIOCSIWESSID
 *  unifi_giwessid  - handler for SIOCGIWESSID
 *  unifi_siwencode - handler for SIOCSIWENCODE
 *  unifi_giwencode - handler for SIOCGIWENCODE
 *
 *      Handler functions for IW extensions.
 *      These are registered via the unifi_iw_handler_def struct below
 *      and called by the generic IW driver support code.
 *      See include/net/iw_handler.h.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */


static int
iwprivs80211ps(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int r = 0;
    int ps_mode = *extra;
    CsrInt32 csr_r;

    CHECK_INITED(priv);

    unifi_trace(priv, UDBG1, "iwprivs80211ps: power save mode = %d\n", ps_mode);

    /* Set power-saving mode if requested */
    if (priv->wext_conf.power_mode != ps_mode) {
        priv->wext_conf.power_mode = ps_mode;

        r = unifi_set_powermode(priv);
        if (r) {
            return -EIO;
        }

        /* Configure deep sleep signaling */
        if (priv->wext_conf.power_mode || (priv->connected == UnifiNotConnected)) {
            csr_r = unifi_configure_low_power_mode(priv->card,
                                                   UNIFI_LOW_POWER_ENABLED,
                                                   UNIFI_PERIODIC_WAKE_HOST_DISABLED);
        } else {
            csr_r = unifi_configure_low_power_mode(priv->card,
                                                   UNIFI_LOW_POWER_DISABLED,
                                                   UNIFI_PERIODIC_WAKE_HOST_DISABLED);
        }
        if (csr_r) {
            return convert_csr_error(csr_r);
        }
    }

    return 0;
}

static int
iwprivg80211ps(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);

    CHECK_INITED(priv);

    switch (priv->wext_conf.power_mode) {
      case CSR_PMM_ACTIVE_MODE:
        snprintf(extra, IWPRIV_POWER_SAVE_MAX_STRING,
                 "Power save mode: %d (Active)", priv->wext_conf.power_mode);
        break;
      case CSR_PMM_FAST_POWER_SAVE:
        snprintf(extra, IWPRIV_POWER_SAVE_MAX_STRING,
                 "Power save mode: %d (Fast)", priv->wext_conf.power_mode);
        break;
      case CSR_PMM_POWER_SAVE:
        snprintf(extra, IWPRIV_POWER_SAVE_MAX_STRING,
                 "Power save mode: %d (Full)", priv->wext_conf.power_mode);
        break;
      default:
        snprintf(extra, IWPRIV_POWER_SAVE_MAX_STRING,
                 "Power save mode: %d (Unknown)", priv->wext_conf.power_mode);
        break;
    }

    wrqu->data.length = strlen(extra) + 1;

    return 0;
}


static int
unifi_giwname(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    char *name = wrqu->name;

    if (priv->if_index == CSR_INDEX_5G) {
        strcpy(name, "IEEE 802.11-a");
    } else {
        strcpy(name, "IEEE 802.11-b/g");
    }
    return 0;
} /* unifi_giwname() */


static int
unifi_siwfreq(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_freq *freq = (struct iw_freq *)wrqu;
    int err = 0;

    func_enter();

    if ((freq->e == 0) && (freq->m <= 1000)) {
        priv->wext_conf.channel = freq->m;
    } else {
        priv->wext_conf.channel = wext_freq_to_channel(freq->m, freq->e);
    }

    func_exit();
    return err;
} /* unifi_siwfreq() */


static int
unifi_giwfreq(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_freq *freq = (struct iw_freq *)wrqu;
    int err = 0;

    func_enter();

    freq->m = priv->wext_conf.channel;
    freq->e = 0;

    func_exit();
    return err;
} /* unifi_giwfreq() */


static int
unifi_siwmode(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int err;
    struct wext_config *wext = &priv->wext_conf;

    func_enter();

    CHECK_INITED(priv);

    /*
     * Send an MLME-RESET when changing mode with SetDefaultMIB set to 1.
     * This will force the f/w to reset the MIBs to the values we have
     * set before the initial MLME-RESET.
     */
    err = unifi_reset_state(priv, dev->dev_addr, 1);

    /* All previously set 802.11 parameters are cleared. */
    wext->channel = -1;
    wext->capability = 0;
    wext->power_mode = 0;
    wext->wakeup_for_dtims = 1;
    wext->wep_key_id = 0;
    wext->auth_type = CSR_OPEN_SYSTEM;
    /* default privacy to "off". */
    wext->privacy = 0;
    wext->generic_ie_len = 0;
    wext->wpa_version = IW_AUTH_WPA_VERSION_DISABLED;
    wext->pairwise_cipher_used = IW_AUTH_CIPHER_NONE;
    wext->group_cipher_used = IW_AUTH_CIPHER_NONE;
    wext->frag_thresh = 2346;
    wext->rts_thresh = 2346;
    wext->disable_join_on_ssid_set = 0;
    memset(wext->desired_ssid, 0, UNIFI_MAX_SSID_LEN);
    memset(wext->current_ssid, 0, UNIFI_MAX_SSID_LEN);
    memset(wext->current_bssid, 0, 6);


    wext->mode = wrqu->mode;

    if (wext->mode == IW_MODE_MONITOR) {
#ifdef UNIFI_SNIFF_ARPHRD
        /* 
         * In monitor mode we will deliver Radiotap IEEE802.11
         * frames to the stack.
         */
        dev->type = UNIFI_SNIFF_ARPHRD;
#endif
    } else {
        /* 
         * If not in monitor mode, make sure frame MAC header type
         * is reset to Ethernet.
         */
        dev->type = ARPHRD_ETHER;
    }

    func_exit();
    if (err) {
        return -EIO;
    }
    return 0;
} /* unifi_siwmode() */



static int
unifi_giwmode(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int err = 0;

    func_enter();

    CHECK_INITED(priv);

    wrqu->mode = priv->wext_conf.mode;

    unifi_trace(priv, UDBG1, "reporting mode = %d\n", priv->wext_conf.mode);
    func_exit();
    return err;
} /* unifi_giwmode() */



static int
unifi_giwrange(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    struct iw_point *dwrq = &wrqu->data;
    struct iw_range *range = (struct iw_range *) extra;
    int i;

    dwrq->length = sizeof(struct iw_range);
    memset(range, 0, sizeof(*range));
    range->min_nwid = 0x0000;
    range->max_nwid = 0x0000;

    /*
     * Don't report the frequency/channel table, then the channel
     * number returned elsewhere will be printed as a channel number.
     */

    /* Ranges of values reported in quality structs */
    range->max_qual.qual  = 40;         /* Max expected qual value */
    range->max_qual.level = -120;       /* Noise floor in dBm */
    range->max_qual.noise = -120;       /* Noise floor in dBm */


    /* space for IW_MAX_BITRATES (8 up to WE15, 32 later) */
    i = 0;
#if WIRELESS_EXT > 15
    range->bitrate[i++] =   2 * 500000;
    range->bitrate[i++] =   4 * 500000;
    range->bitrate[i++] =  11 * 500000;
    range->bitrate[i++] =  22 * 500000;
    range->bitrate[i++] =  12 * 500000;
    range->bitrate[i++] =  18 * 500000;
    range->bitrate[i++] =  24 * 500000;
    range->bitrate[i++] =  36 * 500000;
    range->bitrate[i++] =  48 * 500000;
    range->bitrate[i++] =  72 * 500000;
    range->bitrate[i++] =  96 * 500000;
    range->bitrate[i++] = 108 * 500000;
#else
    range->bitrate[i++] =   2 * 500000;
    range->bitrate[i++] =   4 * 500000;
    range->bitrate[i++] =  11 * 500000;
    range->bitrate[i++] =  22 * 500000;
    range->bitrate[i++] =  24 * 500000;
    range->bitrate[i++] =  48 * 500000;
    range->bitrate[i++] =  96 * 500000;
    range->bitrate[i++] = 108 * 500000;
#endif /* WIRELESS_EXT < 16 */
    range->num_bitrates = i;

    range->max_encoding_tokens = NUM_WEPKEYS;
    range->num_encoding_sizes = 2;
    range->encoding_size[0] = 5;
    range->encoding_size[1] = 13;

    range->we_version_source = 20;
    range->we_version_compiled = WIRELESS_EXT;

    /* Number of channels available in h/w */
    range->num_channels = 14;
    /* Number of entries in freq[] array */
    range->num_frequency = 14;
    for (i = 0; i < range->num_frequency; i++) {
        int chan = i + 1;
        range->freq[i].i = chan;
        range->freq[i].m = channel_to_mhz(chan, 0);
        range->freq[i].e = 6;
    }

#if WIRELESS_EXT > 16
    /* Event capability (kernel + driver) */
    range->event_capa[0] = (IW_EVENT_CAPA_K_0 |
                            IW_EVENT_CAPA_MASK(SIOCGIWTHRSPY) |
                            IW_EVENT_CAPA_MASK(SIOCGIWAP) |
                            IW_EVENT_CAPA_MASK(SIOCGIWSCAN));
    range->event_capa[1] = IW_EVENT_CAPA_K_1;
    range->event_capa[4] = (IW_EVENT_CAPA_MASK(IWEVTXDROP) |
                            IW_EVENT_CAPA_MASK(IWEVCUSTOM) |
                            IW_EVENT_CAPA_MASK(IWEVREGISTERED) |
                            IW_EVENT_CAPA_MASK(IWEVEXPIRED));
#endif /* WIRELESS_EXT > 16 */

#if WIRELESS_EXT > 17
    range->enc_capa = IW_ENC_CAPA_WPA | IW_ENC_CAPA_WPA2 |
                      IW_ENC_CAPA_CIPHER_TKIP | IW_ENC_CAPA_CIPHER_CCMP;
#endif /* WIRELESS_EXT > 17 */


    return 0;
} /* unifi_giwrange() */


static int
unifi_siwap(struct net_device *dev, struct iw_request_info *info,
            union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int err = 0;
    unsigned char ap_bssid[ETH_ALEN];

    func_enter();

    CHECK_INITED(priv);

    if (wrqu->ap_addr.sa_family != ARPHRD_ETHER) {
        return -EINVAL;
    }

    LOCK_DRIVER(priv);

    memcpy(ap_bssid, wrqu->ap_addr.sa_data, ETH_ALEN);

    unifi_trace(priv, UDBG1, "siwap: %02x:%02x:%02x:%02x:%02x:%02x\n",
          ap_bssid[0],
          ap_bssid[1],
          ap_bssid[2],
          ap_bssid[3],
          ap_bssid[4],
          ap_bssid[5]);

    if (((ap_bssid[0] | ap_bssid[1] | ap_bssid[2] | ap_bssid[3] | ap_bssid[4] | ap_bssid[5]) != 0) &&
        ((ap_bssid[0] & ap_bssid[1] & ap_bssid[2] & ap_bssid[3] & ap_bssid[4] & ap_bssid[5]) != 0xFF))
    {
        err = unifi_join_bss(priv, ap_bssid);
    }

    UNLOCK_DRIVER(priv);

    func_exit();

    return err;
} /* unifi_siwap() */


static int
unifi_giwap(struct net_device *dev, struct iw_request_info *info,
            union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);

    CHECK_INITED(priv);

    wrqu->ap_addr.sa_family = ARPHRD_ETHER;
    memcpy(wrqu->ap_addr.sa_data, priv->wext_conf.current_bssid, ETH_ALEN);

    /* We don't do anything with this yet */
    return 0;
} /* unifi_giwap() */


static int
unifi_siwscan(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int scantype;
    int r;
    u8 *ssid = NULL;
    int ssid_len = 0;
#if WIRELESS_EXT > 17
    struct iw_point *data = &wrqu->data;
    struct iw_scan_req *req = (struct iw_scan_req *) extra;
#endif

    func_enter();

    CHECK_INITED(priv);

#if WIRELESS_EXT > 15
    if (priv->wext_conf.mode == IW_MODE_MONITOR) {
        scantype = UNIFI_SCAN_PASSIVE;
    } else
#endif /* WIRELESS_EXT */
    {
        scantype = UNIFI_SCAN_ACTIVE;
    }

#if WIRELESS_EXT > 17
    if (req && (data->flags & IW_SCAN_THIS_ESSID)) {
        ssid = req->essid;
        ssid_len = req->essid_len;
        unifi_trace(priv, UDBG1, "SIWSCAN: Scanning for %.*s\n", ssid_len, ssid);
    } else
#endif
    {
        unifi_trace(priv, UDBG1, "SIWSCAN: Scanning for all APs\n");
    }

    LOCK_DRIVER(priv);

    r = unifi_do_scan(priv, scantype, CSR_ANY_BSS, ssid, ssid_len);

    if (r == 0) {
        wext_send_scan_results_event(priv);
    }

    UNLOCK_DRIVER(priv);

    func_exit();

    return r;

} /* unifi_siwscan() */




/*
 * Translate scan data returned from the card to a card independent
 * format that the Wireless Tools will understand
 */
static inline int
unifi_translate_scan(struct net_device *dev,
                     struct iw_request_info *info,
                     char *current_ev, char *end_buf,
                     scan_info_t *si,
                     int scan_index)
{
    struct iw_event iwe;                /* Temporary buffer */
    unsigned char *info_elems;
    CSR_MLME_SCAN_INDICATION *msi;
    int info_elem_len;
    const unsigned char *elem;
    u16 capabilities;
    int signal, noise, snr;
    char *start_buf = current_ev;
    char *current_val;  /* For rates */
    int i, r;

    msi = &(si->msi);
    info_elems    = si->info_elems;
    info_elem_len = si->info_elem_length;

    /* get capinfo bits */
    capabilities = (s16)unifi2host_16(msi->CapabilityInformation);

    /* First entry *MUST* be the AP MAC address */
    memset(&iwe, 0, sizeof(iwe));
    iwe.cmd = SIOCGIWAP;
    iwe.u.ap_addr.sa_family = ARPHRD_ETHER;
    memcpy(iwe.u.ap_addr.sa_data, msi->Bssid.x, ETH_ALEN);
    iwe.len = IW_EV_ADDR_LEN;
    r = uf_iwe_stream_add_event(info, start_buf, end_buf, &iwe, IW_EV_ADDR_LEN);
    if (r < 0) {
        return r;
    }
    start_buf += r;

    /* Other entries will be displayed in the order we give them */

    /* Add the ESSID */
    /* find SSID in Info Elems */
    elem = unifi_find_info_element(IE_SSID_ID, info_elems, info_elem_len);
    if (elem) {
        int e_len = elem[1];
        const unsigned char *e_ptr = elem + 2;
        unsigned char buf[33];

        memset(&iwe, 0, sizeof(iwe));
        iwe.cmd = SIOCGIWESSID;
        iwe.u.essid.length = e_len;
        if (iwe.u.essid.length > 32) {
            iwe.u.essid.length = 32;
        }
        iwe.u.essid.flags = scan_index;
        memcpy(buf, e_ptr, iwe.u.essid.length);
        buf[iwe.u.essid.length] = '\0';
        r = uf_iwe_stream_add_point(info, start_buf, end_buf, &iwe, buf);
        if (r < 0) {
            return r;
        }
        start_buf += r;
    }

    /* Add mode */
    memset(&iwe, 0, sizeof(iwe));
    iwe.cmd = SIOCGIWMODE;
    if (msi->BssType == CSR_INFRASTRUCTURE) {
        iwe.u.mode = IW_MODE_INFRA;
    } else {
        iwe.u.mode = IW_MODE_ADHOC;
    }
    iwe.len = IW_EV_UINT_LEN;
    r = uf_iwe_stream_add_event(info, start_buf, end_buf, &iwe, IW_EV_UINT_LEN);
    if (r < 0) {
        return r;
    }
    start_buf += r;

    /* Add frequency. iwlist will convert to channel using table given in giwrange */
    memset(&iwe, 0, sizeof(iwe));
    iwe.cmd = SIOCGIWFREQ;
    iwe.u.freq.m = unifi2host_16(msi->ChannelFrequency);
    iwe.u.freq.e = 6;
    r = uf_iwe_stream_add_event(info, start_buf, end_buf, &iwe, IW_EV_FREQ_LEN);
    if (r < 0) {
        return r;
    }
    start_buf += r;


    /* Add quality statistics */
    iwe.cmd = IWEVQUAL;
    /*
     * level and noise below are mapped into an unsigned 8 bit number,
     * ranging from [-192; 63]. The way this is achieved is simply to
     * add 0x100 onto the number if it is negative,
     * once clipped to the correct range.
     */
    signal = (s16)unifi2host_16(msi->Rssi); /* This value is in dBm */
    /* Clip range of snr */
    snr    = (((s16)unifi2host_16(msi->Snr)) > 0) ? (s16)unifi2host_16(msi->Snr) : 0; /* In dB relative, from 0 - 255 */
    snr    = (snr < 255) ? snr : 255;
    noise  = signal - snr;

    /* Clip range of signal */
    signal = (signal < 63) ? signal : 63;
    signal = (signal > -192) ? signal : -192;

    /* Clip range of noise */
    noise = (noise < 63) ? noise : 63;
    noise = (noise > -192) ? noise : -192;

    /* Make u8 */
    signal = ( signal < 0 ) ? signal + 0x100 : signal;
    noise = ( noise < 0 ) ? noise + 0x100 : noise;

    iwe.u.qual.level = (u8)signal; /* -192 : 63 */
    iwe.u.qual.noise = (u8)noise;  /* -192 : 63 */
    iwe.u.qual.qual = snr;         /* 0 : 255 */
    iwe.u.qual.updated = 0;
#if WIRELESS_EXT > 16
    iwe.u.qual.updated |= IW_QUAL_LEVEL_UPDATED | IW_QUAL_NOISE_UPDATED |
                          IW_QUAL_QUAL_UPDATED;
#if WIRELESS_EXT > 18
    iwe.u.qual.updated |= IW_QUAL_DBM;
#endif
#endif
    r = uf_iwe_stream_add_event(info, start_buf, end_buf, &iwe, IW_EV_QUAL_LEN);
    if (r < 0) {
        return r;
    }
    start_buf += r;

    /* Add encryption capability */
    iwe.cmd = SIOCGIWENCODE;
    if (capabilities & SIG_CAP_PRIVACY) {
        iwe.u.data.flags = IW_ENCODE_ENABLED | IW_ENCODE_NOKEY;
    } else {
        iwe.u.data.flags = IW_ENCODE_DISABLED;
    }
    iwe.u.data.length = 0;
    iwe.len = IW_EV_POINT_LEN + iwe.u.data.length;
    r = uf_iwe_stream_add_point(info, start_buf, end_buf, &iwe, "");
    if (r < 0) {
        return r;
    }
    start_buf += r;


    /*
     * Rate : stuffing multiple values in a single event require a bit
     * more of magic - Jean II
     */
    current_val = start_buf + IW_EV_LCP_LEN;

    iwe.cmd = SIOCGIWRATE;
    /* Those two flags are ignored... */
    iwe.u.bitrate.fixed = iwe.u.bitrate.disabled = 0;

    elem = unifi_find_info_element(IE_SUPPORTED_RATES_ID,
                                   info_elems, info_elem_len);
    if (elem) {
        int e_len = elem[1];
        const unsigned char *e_ptr = elem + 2;

        /*
         * Count how many rates we have.
         * Zero marks the end of the list, if the list is not truncated.
         */
        /* Max 8 values */
        for (i = 0; i < e_len; i++) {
            if (e_ptr[i] == 0) {
                break;
            }
            /* Bit rate given in 500 kb/s units (+ 0x80) */
            iwe.u.bitrate.value = ((e_ptr[i] & 0x7f) * 500000);
            /* Add new value to event */
            r = uf_iwe_stream_add_value(info, start_buf, current_val, end_buf, &iwe, IW_EV_PARAM_LEN);
            if (r < 0) {
                return r;
            }
            current_val += r;
        }
    }
    elem = unifi_find_info_element(IE_EXTENDED_SUPPORTED_RATES_ID,
                                   info_elems, info_elem_len);
    if (elem) {
        int e_len = elem[1];
        const unsigned char *e_ptr = elem + 2;

        /*
         * Count how many rates we have.
         * Zero marks the end of the list, if the list is not truncated.
         */
        /* Max 8 values */
        for (i = 0; i < e_len; i++) {
            if (e_ptr[i] == 0) {
                break;
            }
            /* Bit rate given in 500 kb/s units (+ 0x80) */
            iwe.u.bitrate.value = ((e_ptr[i] & 0x7f) * 500000);
            /* Add new value to event */
            r = uf_iwe_stream_add_value(info, start_buf, current_val, end_buf, &iwe, IW_EV_PARAM_LEN);
            if (r < 0) {
                return r;
            }
            current_val += r;
        }
    }
    /* Check if we added any rates event */
    if ((current_val - start_buf) > IW_EV_LCP_LEN) {
        start_buf = current_val;
    }


#if WIRELESS_EXT > 17
    memset(&iwe, 0, sizeof(iwe));
    iwe.cmd = IWEVGENIE;
    iwe.u.data.length = info_elem_len;

    r = uf_iwe_stream_add_point(info, start_buf, end_buf, &iwe, info_elems);
    if (r < 0) {
        return r;
    }

    start_buf += r;
#endif /* WE > 17 */

    return (start_buf - current_ev);
} /* unifi_translate_scan() */



static int
unifi_giwscan(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_point *dwrq = &wrqu->data;

    char *current_ev = extra;
    int index, r;

    CHECK_INITED(priv);

    LOCK_DRIVER(priv);

    /* Read and parse all entries. Loop until there are no more. */
    index = 0;
    while (1) {
        /* Retrieve a scan report and translate it to WE format */
        scan_info_t *si;

        si = unifi_get_scan_report(priv, index);
        if (!si) {
            break;
        }
#if 0
        printk("type %d, mac %02x:%02x:%02x:%02x:%02x:%02x, "
               "beacon %d, ts %lld, lt %lld,\n"
               "chan %d, freq %d, caps 0x%X, rssi %d, snr %d\n",
               si->msi.BssType,
               si->msi.Bssid.x[0], si->msi.Bssid.x[1], si->msi.Bssid.x[2],
               si->msi.Bssid.x[3], si->msi.Bssid.x[4], si->msi.Bssid.x[5],
               si->msi.BeaconPeriod,
               si->msi.Timestamp,
               si->msi.LocalTime,
               si->msi.Channel,
               si->msi.ChannelFrequency,
               si->msi.CapabilityInformation,
               si->msi.Rssi,
               si->msi.Snr);
#endif

        r = unifi_translate_scan(dev, info, current_ev, extra + dwrq->length,
                                 si, index+1);
        if (r < 0) {
            UNLOCK_DRIVER(priv);
            return r;
        }

        current_ev += r;
        index++;
    }

    UNLOCK_DRIVER(priv);

    /* Length of data */
    dwrq->length = (current_ev - extra);
    dwrq->flags = 0;    /* todo */

    return 0;
} /* unifi_giwscan() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_siwessid
 *
 *      Request to join a network or start and AdHoc.
 *
 *  Arguments:
 *      dev             Pointer to network device struct.
 *      info            Pointer to broken-out ioctl request.
 *      data            Pointer to argument data.
 *      essid           Pointer to string giving name of network to join
 *                      or start
 *
 *  Returns:
 *      0 on success and everything complete
 *      -EINPROGRESS to have the higher level call the commit method.
 * ---------------------------------------------------------------------------
 */
static int
unifi_siwessid(struct net_device *dev, struct iw_request_info *info,
               struct iw_point *data, char *essid)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int len;
    int err = 0;

    char *ssid;

    func_enter();

    CHECK_INITED(priv);

    if (priv->wext_conf.mode == IW_MODE_MONITOR) {
        return -EINVAL;
    }

    /* Zero out string buffer */
    memset(priv->wext_conf.desired_ssid, 0, UNIFI_MAX_SSID_LEN);
    ssid = NULL;

    /*
     * Bit 0 of flags indicates whether to join a particular
     * AP (flags=1) or any AP (flags=0).
     */
    if (data->flags & 1) {
        /* Limit length  */
        len = data->length;
        if (len > UNIFI_MAX_SSID_LEN) {
            len = UNIFI_MAX_SSID_LEN;
        }

        memcpy(priv->wext_conf.desired_ssid, essid, len);
        ssid = priv->wext_conf.desired_ssid;
        unifi_trace(priv, UDBG1, "siwessid: %s, len %d, flags=%#x\n",
              priv->wext_conf.desired_ssid, len, data->flags);
    }

    /*
     * If this flag is set we need to wait for a bssid set before
     * attempting to join the desired network.
     */
    if (priv->wext_conf.disable_join_on_ssid_set == 0) {
        /*
         * ssid is NULL for wildcard, but could be empty string
         * to look for a genuinely empty SSID.
         */
        err = unifi_autojoin(priv, ssid);
    } else {
        /*
         * If the flag was set, we clear it and wait a new set, of either
         * ssid or bssid. Normaly this flag was set by the wpa_supplicant
         * and the next step will be a call to SIOCSIWAP handler.
         * See more info in SIOCSIWGENIE handler.
         */
        priv->wext_conf.disable_join_on_ssid_set = 0;
    }

    func_exit();
    return err;
} /* unifi_siwessid() */



static int
unifi_giwessid(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *essid)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_point *data = &wrqu->essid;

    func_enter();

    CHECK_INITED(priv);

    if (priv->wext_conf.flag_associated) {
        data->length = strlen(priv->wext_conf.current_ssid);
        strncpy(essid, priv->wext_conf.current_ssid, data->length);
        data->flags = 1;            /* active */
    }

    unifi_trace(priv, UDBG2, "unifi_giwessid: %.*s\n", data->length, essid);

    func_exit();
    return 0;
} /* unifi_giwessid() */



static int
unifi_siwrate(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_param *args = &wrqu->bitrate;
    int rate, flag;
    int r;

    CHECK_INITED(priv);

    /*
     * If args->fixed == 0, value is max rate or -1 for best
     * If args->fixed == 1, value is rate to set or -1 for best
     * args->disabled and args->flags are not used in SIOCSIWRATE
     */

    /* Default to auto rate algorithm */
    rate = 0;   /* in 500Kbit/s, 0 means auto */
    flag = 1;   /* 1 means rate is a maximum, 2 means rate is a set value */

    if (args->fixed == 1) {
        flag = 2;
    }
    if (args->value != -1) {
        rate = args->value / 500000;
    }

    r = unifi_set_mib_int(priv, priv->wext_client, unifiFixTxDataRate, rate);
    if (r == 0) {
        r = unifi_set_mib_int(priv, priv->wext_client, unifiFixMaxTxDataRate, flag);
    }
    if (r < 0) {
        return -EIO;
    }
    if (r > 0) {
        return -EINVAL;
    }

    return 0;
} /* unifi_siwrate() */


static int
unifi_giwrate(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_param *args = &wrqu->bitrate;
    int r;
    int bitrate, flag;

    CHECK_INITED(priv);

    flag = 0;
    bitrate = 0;

    r = unifi_get_mib_int(priv, priv->wext_client, unifiTxDataRate, &bitrate);
    if (r == 0) {
        r = unifi_get_mib_int(priv, priv->wext_client, unifiFixMaxTxDataRate, &flag);
        if (r == 1) {
            /* MIB not present, not fatal */
            r = 0;
        }
    }

    if (r < 0) {
        return -EIO;
    }
    if (r > 0) {
        return -EINVAL;
    }

    args->value = bitrate * 500000;
    args->fixed = (flag == 2) ? 1 : 0;

    return 0;
} /* unifi_giwrate() */

static int
unifi_siwrts(struct net_device *dev, struct iw_request_info *info,
             union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int val = wrqu->rts.value;
    int ret = 0;

    CHECK_INITED(priv);

    unifi_trace(priv, UDBG2, "Set RTS from %d to %d (dis: %d)\n",
          priv->wext_conf.rts_thresh, val, wrqu->rts.disabled);

    if (wrqu->rts.disabled) {
        val = 2347;
    }

    if ( (val < 0) || (val > 2347) )
    {
        return -EINVAL;
    }

    ret = set_rts_threshold(priv, val);

    return ret;
}

static int
unifi_giwrts(struct net_device *dev, struct iw_request_info *info,
             union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int r;
    int rts_thresh;
    char oidstr[128];

    CHECK_INITED(priv);

    /* Build the OID string by appending the if_index */
    snprintf(oidstr, 128, "%s.%d", dot11RTSThreshold, priv->if_index);

    r = unifi_get_mib_int(priv, priv->wext_client, oidstr, &rts_thresh);
    if (r < 0) {
        return -EIO;
    }
    if (r > 0) {
        return -EINVAL;
    }

    if (rts_thresh > 2347) {
        rts_thresh = 2347;
    }

    wrqu->rts.value = rts_thresh;
    wrqu->rts.disabled = (rts_thresh == 2347);
    wrqu->rts.fixed = 1;

    return 0;
}

static int
unifi_siwfrag(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int val = wrqu->frag.value;
    int r = 0;

    CHECK_INITED(priv);

    unifi_trace(priv, UDBG2, "Set Frag from %d to %d (dis: %d)\n",
          priv->wext_conf.frag_thresh, val, wrqu->frag.disabled);

    if (wrqu->frag.disabled)
        val = 2346;

    if ( (val < 256) || (val > 2347) )
        return -EINVAL;

    /* Fragmentation Threashold must be even */
    r = set_fragmentation_threshold(priv, (val & ~0x1));

    return 0;
}

static int
unifi_giwfrag(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int r;
    int frag_thresh;
    char oidstr[128];

    CHECK_INITED(priv);

    /* Build the OID string by appending the if_index */
    snprintf(oidstr, 128, "%s.%d", dot11FragmentationThreshold, priv->if_index);
    r = unifi_get_mib_int(priv, priv->wext_client, oidstr, &frag_thresh);
    if (r < 0) {
        return -EIO;
    }
    if (r > 0) {
        return -EINVAL;
    }

    /* Sanity checks */
    if (frag_thresh > 2346) {
        frag_thresh = 2346;
    } else {
        if (frag_thresh < 256) {
            frag_thresh = 256;
        }
    }

    /* Build the return structure */
    wrqu->frag.value = frag_thresh;
    wrqu->frag.disabled = (frag_thresh == 2346);
    wrqu->frag.fixed = 1;

    return 0;
}

#if 0
static int
unifi_siwtxpow(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    return 0;
}


static int
unifi_giwtxpow(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    return 0;
}


static int
unifi_siwretry(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    return 0;
}


static int
unifi_giwretry(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    return 0;
}
#endif




/*
 * ---------------------------------------------------------------------------
 *  unifi_siwencode
 *
 *      Set the privacy mode, i.e. encryption and authentication.
 *      This is only WEP and primarily for "iwconfig eth2 encode ...".
 *      WPA is handled differently, see unifi_siwencodeext.
 *
 *      The interpretation of the arguments is somewhat convoluted:
 *       - setting a key value implies that key should be used and privacy
 *         turned on.
 *       - explicit settings override this.
 *
 *  Arguments:
 *      dev             Pointer to network device struct.
 *      info            Pointer to broken-out ioctl request.
 *      wrqu            Pointer to argument data.
 *      extra           Pointer to any additional bulk data.
 *
 *  Returns:
 *      0 on success and everything complete
 *      -EINPROGRESS to have the higher level call the commit method.
 * ---------------------------------------------------------------------------
 */
static int
_unifi_siwencode(struct net_device *dev, struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_point *erq = &wrqu->encoding;
    int index;
    int privacy = -1;
    int rc = 0;
    int force_set_key_index = 0;

    func_enter();

    /*
     * Key index is encoded in the flags.
     * 0 - use current default,
     * 1-4 - if a key value is given set that key
     *       if not use that key
     */
    index = (erq->flags & IW_ENCODE_INDEX);  /* key number, 1-4 */
    if ((index < 0) || (index > 4)) {
        return -EINVAL;
    }

    /*
     * Basic checking: do we have a key to set ?
     * The IW_ENCODE_NOKEY flag is set when no key is present (only change flags),
     * but older versions rely on sending a key id 1-4.
     */
    if (erq->length > 0) {
        wep_key_t key;

        /* Check the size of the key */
        if ((erq->length > LARGE_KEY_SIZE) || (erq->length < SMALL_KEY_SIZE)) {
            return -EINVAL;
        }

        /* Check the index (none (i.e. 0) means use current) */
        if ((index < 1) || (index > 4)) {
            /* If we do not have a previous key, use 1 as default */
            if (!priv->wext_conf.wep_key_id) {
                priv->wext_conf.wep_key_id = 1;
                force_set_key_index = 1;
            }
            index = priv->wext_conf.wep_key_id;
        }

        /* If we didn't have a key and a valid index is set, we want to remember it*/
        if (!priv->wext_conf.wep_key_id) {
            priv->wext_conf.wep_key_id = index;
            force_set_key_index = 1;
        }

        privacy = 1;

        /* Set the length */
        if (erq->length > SMALL_KEY_SIZE) {
            key.len = LARGE_KEY_SIZE;
        } else {
            key.len = SMALL_KEY_SIZE;
        }
        /* Check if the key is not marked as invalid */
        if ((erq->flags & IW_ENCODE_NOKEY) == 0) {

            /* Cleanup */
            memset(key.key, 0, UNIFI_MAX_KEY_SIZE);

            /* Copy the key in the driver */
            memcpy(key.key, extra, erq->length);

            /* Send the key to the card */
            unifi_trace(priv, UDBG1, "set WEP key %d\n", index);
            set_wep_key(priv, index, key.key, key.len);

            if (force_set_key_index == 1) {
                unifi_trace(priv, UDBG1, "set WEP key index %d\n", index - 1);
                rc = set_default_wep_key(priv, index - 1);
            }
        }
    } else {
        /*
         * No additional key data, so it must be a request to change the
         * active key.
         */
        if (index != 0) {
            unifi_trace(priv, UDBG1, "use WEP key %d\n", index - 1);

            /* Tell UniFi which key to use */
            rc = set_default_wep_key(priv, index - 1);

            /* Remember current key id */
            priv->wext_conf.wep_key_id = index;

            /* Turn on encryption */
            privacy = 1;
        }
    }


    /* Read the flags */
    if (erq->flags & IW_ENCODE_DISABLED) {
        /* disable encryption */
        unifi_trace(priv, UDBG1, "disable WEP encryption\n");
        privacy = 0;
        priv->wext_conf.auth_type = CSR_OPEN_SYSTEM;
        priv->wext_conf.wep_key_id = 0;
    }

    if (erq->flags & IW_ENCODE_RESTRICTED) {
        /* Use shared key auth */
        unifi_trace(priv, UDBG1, "use WEP shared-key auth\n");
        priv->wext_conf.auth_type = CSR_SHARED_KEY;
    }
    if (erq->flags & IW_ENCODE_OPEN) {
        /* Only Wep */
        unifi_trace(priv, UDBG1, "use WEP open-system auth\n");
        priv->wext_conf.auth_type = CSR_OPEN_SYSTEM;
    }


    /* Commit the changes to flags if needed */
    if (privacy != -1) {
        unifi_trace(priv, UDBG1, "setting WEP encryption = %d\n", privacy);
        set_privacy_invoked(priv, privacy);
        priv->wext_conf.block_controlled_port = unifi_8021xPortOpen;

        /* Clear the WPA state. */
#if WIRELESS_EXT > 17
        priv->wext_conf.wpa_version = IW_AUTH_WPA_VERSION_DISABLED;
#endif /* WIRELESS_EXT > 17 */
        priv->wext_conf.generic_ie_len = 0;
    }

    func_exit_r(rc);
    return rc;
} /* _unifi_siwencode() */


static int
unifi_siwencode(struct net_device *dev, struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int err;

    CHECK_INITED(priv);

    LOCK_DRIVER(priv);
    err = _unifi_siwencode(dev, info, wrqu, extra);
    /* force a rejoin if associated */
    if (!err && priv->wext_conf.flag_associated) {
        unifi_join_bss(priv, priv->wext_conf.current_bssid);
    }
    UNLOCK_DRIVER(priv);

    return err;
} /* unifi_siwencode() */

static int
unifi_giwencode(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_point *erq = &wrqu->encoding;

    CHECK_INITED(priv);

    if (priv->wext_conf.wep_key_id == 0)
        erq->flags = IW_ENCODE_DISABLED;
    else {
        int index = priv->wext_conf.wep_key_id - 1;
        wep_key_t *key = priv->wext_conf.wep_keys + index;

        if (priv->wext_conf.auth_type == CSR_SHARED_KEY)
            erq->flags = IW_ENCODE_RESTRICTED;
        else
            erq->flags = IW_ENCODE_OPEN;

        erq->flags |= (index & IW_ENCODE_INDEX);
        erq->length = key->len;
        memcpy(extra, key->key, key->len);
    }

    return 0;
} /* unifi_giwencode() */


static int
unifi_siwpower(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    struct iw_param *args = &wrqu->power;
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int pm_mode, listen_interval, wake_for_dtim;
    int rc = 0;
    CsrInt32 csr_r;

    CHECK_INITED(priv);

    pm_mode         = priv->wext_conf.power_mode;
    listen_interval = priv->wext_conf.assoc_listen_interval;
    wake_for_dtim   = priv->wext_conf.wakeup_for_dtims;

    /*
     * If "off" is given, set mode to Active.
     * Otherwise set mode to PowerSave.
     * Optionally set the listen interval, but this is only
     * used if called before associating with an AP.
     */
    if (args->disabled) {
        pm_mode = CSR_PMM_ACTIVE_MODE;      /* Active */
    }
    else
    {
        pm_mode = CSR_PMM_POWER_SAVE;       /* PowerSave */

        switch (args->flags & IW_POWER_TYPE) {
          case 0:
            /* not specified */
            break;
          case IW_POWER_PERIOD:
            listen_interval = args->value / 1000;
            break;
          default:
            return -EINVAL;
        }

        switch (args->flags & IW_POWER_MODE) {
          case 0:
            /* not specified */
            break;
          case IW_POWER_UNICAST_R:
            /* not interested in broadcast packets */
            wake_for_dtim = 0;
            break;
          case IW_POWER_ALL_R:
            /* yes, we are interested in broadcast packets */
            wake_for_dtim = 1;
            break;
          default:
            return -EINVAL;
        }
    }

    /* Set power-saving mode if requested */
    if ((priv->wext_conf.power_mode != pm_mode) || (priv->wext_conf.wakeup_for_dtims != wake_for_dtim)) {
        priv->wext_conf.power_mode            = pm_mode;
        priv->wext_conf.wakeup_for_dtims      = wake_for_dtim;

        LOCK_DRIVER(priv);
        rc = unifi_set_powermode(priv);
        UNLOCK_DRIVER(priv);
        if (rc) {
            return -EIO;
        }

        /* Configure deep sleep signaling */
        if (priv->wext_conf.power_mode || (priv->connected == UnifiNotConnected)) {
            csr_r = unifi_configure_low_power_mode(priv->card,
                                                   UNIFI_LOW_POWER_ENABLED,
                                                   UNIFI_PERIODIC_WAKE_HOST_DISABLED);
        } else {
            csr_r = unifi_configure_low_power_mode(priv->card,
                                                   UNIFI_LOW_POWER_DISABLED,
                                                   UNIFI_PERIODIC_WAKE_HOST_DISABLED);
        }
        if (csr_r) {
            return convert_csr_error(csr_r);
        }

    }

    /* If the listen interval has changed .. */
    if (priv->wext_conf.assoc_listen_interval != listen_interval) {
        /* .. store the new value. */
        priv->wext_conf.assoc_listen_interval = listen_interval;

#if WIRELESS_EXT > 17
        /*
         * If the wpa_supplicant drives us, we can not apply the new
         * value right now. We will use it the next time that
         * we will send the association request.
         */
        if ((priv->wext_conf.wpa_version == IW_AUTH_WPA_VERSION_WPA) ||
            (priv->wext_conf.wpa_version == IW_AUTH_WPA_VERSION_WPA2)) {
            return rc;
        }
#endif /* WIRELESS_EXT > 17 */

        /* If we are associated to an AP, reassociate to apply the new value. */
        if ((priv->connected == UnifiConnected) &&
            ((priv->wext_conf.capability & SIG_CAP_IBSS) == 0)) {
            rc = unifi_join_bss(priv, priv->wext_conf.current_bssid);
        }
    }

    return rc;
} /* unifi_siwpower() */

static int
unifi_giwpower(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    struct iw_param *args = &wrqu->power;
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);

    CHECK_INITED(priv);

    args->disabled = (priv->wext_conf.power_mode == 0);
    if (args->disabled) {
        args->flags = 0;
        return 0;
    }

    args->flags = 0;
    if (priv->wext_conf.wakeup_for_dtims) {
        args->flags |= IW_POWER_ALL_R;
    } else {
        args->flags |= IW_POWER_UNICAST_R;
    }

    args->flags |= IW_POWER_PERIOD;
    args->value = priv->wext_conf.assoc_listen_interval * 1000;

    return 0;
} /* unifi_giwpower() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_siwcommit - handler for SIOCSIWCOMMIT
 *
 *      Apply all the parameters that have been set.
 *      In practice this means:
 *       - do a scan
 *       - join a network or start an AdHoc
 *       - authenticate and associate.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static int
unifi_siwcommit(struct net_device *dev, struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
    return 0;
} /* unifi_siwcommit() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_siwmlme
 *
 *      Handler for SIOCSIWMLME.
 *      Requests a MLME operation; uses struct iw_mlme
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
#if WIRELESS_EXT > 17
static int
_unifi_siwmlme(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    CSR_SIGNAL signal;
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_mlme *mlme = (struct iw_mlme *)extra;
    int timeout = 5000;
    int r, result=0;

    switch (mlme->cmd) {
      case IW_MLME_DEAUTH:
        {
            CSR_MLME_DEAUTHENTICATE_REQUEST *req = &signal.u.MlmeDeauthenticateRequest;

            memcpy(&req->PeerStaAddress, mlme->addr.sa_data, ETH_ALEN);
            req->ReasonCode = cpu_to_le16(mlme->reason_code);

            signal.SignalPrimitiveHeader.SignalId = CSR_MLME_DEAUTHENTICATE_REQUEST_ID;

            r = unifi_mlme_blocking_request(priv, priv->wext_client, &signal, NULL, timeout);
            if (r < 0) {
                unifi_error(priv, "failed to send DEAUTHENTICATE request, error %d\n", r);
                return r;
            }
    
            r = priv->wext_client->reply_signal->u.MlmeDeauthenticateConfirm.ResultCode;
            if (r) {
                unifi_notice(priv, "DEAUTHENTICATE request was rejected with result 0x%X (%s)\n",
                             r, lookup_result_code(r));
            }

        }
        break;

      case IW_MLME_DISASSOC:
        {
            CSR_MLME_DISASSOCIATE_REQUEST *req = &signal.u.MlmeDisassociateRequest;

            memcpy(&req->PeerStaAddress, mlme->addr.sa_data, ETH_ALEN);
            req->ReasonCode = cpu_to_le16(mlme->reason_code);

            signal.SignalPrimitiveHeader.SignalId = CSR_MLME_DISASSOCIATE_REQUEST_ID;

            r = unifi_mlme_blocking_request(priv, priv->wext_client, &signal, NULL, timeout);
            if (r < 0) {
                unifi_error(priv, "failed to send DISASSOCIATE request, error %d\n", r);
                return r;
            }
    
            r = priv->wext_client->reply_signal->u.MlmeDisassociateConfirm.ResultCode;
            if (r) {
                unifi_notice(priv, "DISASSOCIATE request was rejected with result 0x%X (%s)\n",
                             r, lookup_result_code(r));
            }

        }
        break;

      default:
        return -EOPNOTSUPP;
    }

    return result;
} /* _unifi_siwmlme() */

static int
unifi_siwmlme(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int err;

    CHECK_INITED(priv);

    LOCK_DRIVER(priv);
    err = _unifi_siwmlme(dev, info, wrqu, extra);
    UNLOCK_DRIVER(priv);

    return err;
} /* unifi_siwmlme() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_siwgenie
 *  unifi_giwgenie
 *
 *      WPA : Generic IEEE 802.11 information element (e.g., for WPA/RSN/WMM).
 *      Handlers for SIOCSIWGENIE, SIOCGIWGENIE - set/get generic IE
 *
 *      The host program (e.g. wpa_supplicant) uses this call to set the
 *      additional IEs to accompany the next (Associate?) request.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 *  Notes:
 *      From wireless.h:
 *        This ioctl uses struct iw_point and data buffer that includes IE id
 *        and len fields. More than one IE may be included in the
 *        request. Setting the generic IE to empty buffer (len=0) removes the
 *        generic IE from the driver.
 * ---------------------------------------------------------------------------
 */
static int
unifi_siwgenie(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int len;

    func_enter();

    CHECK_INITED(priv);

    len = wrqu->data.length;
    if (len > IE_VECTOR_MAXLEN) {
        len = IE_VECTOR_MAXLEN;
    }

    priv->wext_conf.generic_ie_len = len;
    memcpy(priv->wext_conf.generic_ie, extra, len);

    func_exit();
    return 0;
} /* unifi_siwgenie() */

static int
unifi_giwgenie(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int len = priv->wext_conf.generic_ie_len;

    func_enter();

    CHECK_INITED(priv);

    if (len == 0) {
        wrqu->data.length = 0;
        return 0;
    }

    if (wrqu->data.length < len) {
        return -E2BIG;
    }

    wrqu->data.length = len;
    memcpy(extra, priv->wext_conf.generic_ie, len);

    func_exit();

    return 0;
} /* unifi_giwgenie() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_siwauth
 *  unifi_giwauth
 *
 *      Handlers for SIOCSIWAUTH, SIOCGIWAUTH
 *      Set/get various authentication parameters.
 *
 *  Arguments:
 *
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static int
_unifi_siwauth(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int ret = 0;

    func_enter();

    switch (wrqu->param.flags & IW_AUTH_INDEX) {

      case IW_AUTH_WPA_ENABLED:
        unifi_trace(priv, UDBG1, "IW_AUTH_WPA_ENABLED: %d\n", wrqu->param.value);

        if (wrqu->param.value == 0) {
            ret = set_rsna_enabled(priv, 0);
            if (ret != 0) {
                unifi_trace(priv, UDBG2, "set_rsna_enabled returned %d\n", ret);
                ret = -EIO;
            }

            /* Clear the WPA state information. */
            priv->wext_conf.generic_ie_len = 0;
            priv->wext_conf.block_controlled_port = unifi_8021xPortOpen;
        }
        break;

      case IW_AUTH_PRIVACY_INVOKED:
        unifi_trace(priv, UDBG1, "IW_AUTH_PRIVACY_INVOKED: %d\n", wrqu->param.value);
        set_privacy_invoked(priv, wrqu->param.value);
        break;

      case IW_AUTH_80211_AUTH_ALG:
        switch (wrqu->param.value) {
          case IW_AUTH_ALG_OPEN_SYSTEM :
            unifi_trace(priv, UDBG1, "IW_AUTH_80211_AUTH_ALG: %d (IW_AUTH_ALG_OPEN_SYSTEM)\n", wrqu->param.value);
            priv->wext_conf.auth_type = CSR_OPEN_SYSTEM;
            break;
          case IW_AUTH_ALG_SHARED_KEY :
            unifi_trace(priv, UDBG1, "IW_AUTH_80211_AUTH_ALG: %d (IW_AUTH_ALG_SHARED_KEY)\n", wrqu->param.value);
            priv->wext_conf.auth_type = CSR_SHARED_KEY;
            break;
          case IW_AUTH_ALG_LEAP	:
            /* Initial exchanges using open-system to set EAP */
            unifi_trace(priv, UDBG1, "IW_AUTH_80211_AUTH_ALG: %d (IW_AUTH_ALG_LEAP)\n", wrqu->param.value);
            priv->wext_conf.auth_type = CSR_OPEN_SYSTEM;
            break;
          default:
            unifi_trace(priv, UDBG1, "IW_AUTH_80211_AUTH_ALG: invalid value %d\n",
                  wrqu->param.value);
            return -EINVAL;
        }
        break;

      case IW_AUTH_WPA_VERSION:
        unifi_trace(priv, UDBG1, "IW_AUTH_WPA_VERSION: %d\n", wrqu->param.value);
        /*
           IW_AUTH_WPA_VERSION_DISABLED 0x00000001
           IW_AUTH_WPA_VERSION_WPA      0x00000002
           IW_AUTH_WPA_VERSION_WPA2     0x00000004
        */
        priv->wext_conf.wpa_version = wrqu->param.value;
        /* wpa_supplicant sets this ioctl prior ssid and essid set.
         * We do not want to join to the selected ssid, until the
         * essid is set. So we temprorarly disable the join request
         * when the ssid is set.
         * For more info see also SIOCSIWESSID handler.
         */
        priv->wext_conf.disable_join_on_ssid_set = 1;

        if (wrqu->param.value & IW_AUTH_WPA_VERSION_DISABLED) {
            ret = set_rsna_enabled(priv, 0);
            if (ret != 0) {
                unifi_trace(priv, UDBG2, "set_rsna_enabled returned %d\n", ret);
                ret = -EIO;
            }

            /* Clear the WPA state information. */
            priv->wext_conf.generic_ie_len = 0;
            priv->wext_conf.block_controlled_port = unifi_8021xPortOpen;
        } else {
            ret = set_rsna_enabled(priv, 1);
            if (ret != 0) {
                unifi_trace(priv, UDBG2, "set_rsna_enabled returned %d\n", ret);
                ret = -EIO;
            }
            priv->wext_conf.block_controlled_port = unifi_8021xPortClosedDiscard;
        }
        break;

      case IW_AUTH_CIPHER_PAIRWISE:
        unifi_trace(priv, UDBG1, "IW_AUTH_CIPHER_PAIRWISE: %d\n", wrqu->param.value);
        /*
         * one of:
         IW_AUTH_CIPHER_NONE	0x00000001
         IW_AUTH_CIPHER_WEP40	0x00000002
         IW_AUTH_CIPHER_TKIP	0x00000004
         IW_AUTH_CIPHER_CCMP	0x00000008
         IW_AUTH_CIPHER_WEP104	0x00000010
        */
        priv->wext_conf.pairwise_cipher_used = wrqu->param.value;

        break;

      case IW_AUTH_CIPHER_GROUP:
        unifi_trace(priv, UDBG1, "IW_AUTH_CIPHER_GROUP: %d\n", wrqu->param.value);
        priv->wext_conf.group_cipher_used = wrqu->param.value;
        /*
         * Use the WPA version and the group cipher suite to set the permitted
         * group key in the MIB. f/w uses this value to validate WPA and RSN IEs
         * in the probe responses from the desired BSS(ID)
         */
        ret = set_rsna_config_group_cipher(priv, priv->wext_conf.wpa_version,
                                           priv->wext_conf.group_cipher_used);
        if (ret != 0) {
            unifi_trace(priv, UDBG1, "set_rsna_config_group_cipher failed (ret=%d)\n", ret);
            ret = -EIO;
        }

        break;

      case IW_AUTH_KEY_MGMT:
        unifi_trace(priv, UDBG1, "IW_AUTH_KEY_MGMT: %d\n", wrqu->param.value);
        /*
           IW_AUTH_KEY_MGMT_802_1X 1
           IW_AUTH_KEY_MGMT_PSK    2
        */
        break;
      case IW_AUTH_TKIP_COUNTERMEASURES:
        /*
         * Set to true at the start of the 60 second backup-off period
         * following 2 MichaelMIC failures within 60s.
         */
        unifi_trace(priv, UDBG1, "IW_AUTH_TKIP_COUNTERMEASURES: %d\n", wrqu->param.value);
        break;

      case IW_AUTH_DROP_UNENCRYPTED:
        /*
         * Set to true on init.
         * Set to false just before associate if encryption will not be
         * required.
         *
         * Note this is not the same as the 802.1X controlled port
         */
        unifi_trace(priv, UDBG1, "IW_AUTH_DROP_UNENCRYPTED: %d\n", wrqu->param.value);
        priv->drop_unencrypted = (wrqu->param.value) ? 1 : 0;
        break;

      case IW_AUTH_RX_UNENCRYPTED_EAPOL:
	/*
         * This is set by wpa_supplicant to allow unencrypted EAPOL messages
         * even if pairwise keys are set when not using WPA. IEEE 802.1X
         * specifies that these frames are not encrypted, but WPA encrypts
         * them when pairwise keys are in use.
         * I think the UniFi f/w handles this decision for us.
         */
        unifi_trace(priv, UDBG1, "IW_AUTH_RX_UNENCRYPTED_EAPOL: %d\n", wrqu->param.value);
        break;

      case IW_AUTH_ROAMING_CONTROL:
        unifi_trace(priv, UDBG1, "IW_AUTH_ROAMING_CONTROL: %d\n", wrqu->param.value);
        break;

      default:
        unifi_trace(priv, UDBG1, "Unsupported auth param %d to 0x%X\n",
              wrqu->param.flags & IW_AUTH_INDEX,
              wrqu->param.value);
        return -EOPNOTSUPP;
    }

    func_exit();

    return ret;
} /* _unifi_siwauth() */

static int
unifi_siwauth(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int err;

    CHECK_INITED(priv);

    LOCK_DRIVER(priv);
    err = _unifi_siwauth(dev, info, wrqu, extra);
    UNLOCK_DRIVER(priv);

    return err;
} /* unifi_siwauth() */


static int
unifi_giwauth(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    return -EOPNOTSUPP;
} /* unifi_giwauth() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_siwencodeext
 *  unifi_giwencodeext
 *
 *      Handlers for SIOCSIWENCODEEXT, SIOCGIWENCODEEXT - set/get
 *      encoding token & mode
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      For WPA/WPA2 we don't take note of the IW_ENCODE_EXT_SET_TX_KEY flag.
 *      This flag means "use this key to encode transmissions"; we just
 *      assume only one key will be set and that is the one to use.
 * ---------------------------------------------------------------------------
 */
static int
_unifi_siwencodeext(struct net_device *dev, struct iw_request_info *info,
                   union iwreq_data *wrqu, char *extra)
{
    CSR_SIGNAL signal;
    CSR_MLME_DELETEKEYS_REQUEST *req = &signal.u.MlmeDeletekeysRequest;
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_encode_ext *ext = (struct iw_encode_ext *)extra;
    int timeout = 1000;
    int r, rc;
    CSR_KEY_TYPE key_type;
    unsigned char *keydata;
    unsigned char tkip_key[32];
    int keyid;
    unsigned char *a = (unsigned char *)ext->addr.sa_data;

    func_enter();

    unifi_trace(priv, UDBG3, "siwencodeext: flags=0x%X, alg=%d, ext_flags=0x%X, key_len=%d,\n",
           wrqu->encoding.flags, ext->alg, ext->ext_flags, ext->key_len);
    unifi_trace(priv, UDBG3, "              addr=%02X:%02X:%02X:%02X:%02X:%02X\n",
          a[0], a[1], a[2], a[3], a[4], a[5]);

    if ((ext->key_len == 0) && (ext->ext_flags & IW_ENCODE_EXT_SET_TX_KEY)) {
        /* This means use a different key (given by key_idx) for Tx. */
        /* NYI */
        unifi_trace(priv, UDBG1, KERN_ERR "unifi_siwencodeext: NYI should change tx key id here!!\n");
        return -ENOTSUPP;
    }

    keydata = (unsigned char *)(ext + 1);
    keyid = (wrqu->encoding.flags & IW_ENCODE_INDEX) - 1;

    /*
     * Check for request to delete keys for an address.
     */
    /* Pick out request for no privacy. */
    if (ext->alg == IW_ENCODE_ALG_NONE) {

        unifi_trace(priv, UDBG1, "Deleting %s key %d\n",
              (ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY) ? "GROUP" : "PAIRWISE",
              keyid+1);

        req->KeyId = keyid;
        if (ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY) {
            req->KeyType = CSR_GROUP;
        } else {
            req->KeyType = CSR_PAIRWISE; /* Pairwise Key */
        }
        memcpy(&req->Address, ext->addr.sa_data, ETH_ALEN);

        signal.SignalPrimitiveHeader.SignalId = CSR_MLME_DELETEKEYS_REQUEST_ID;

        /* Queue request to UniFi and wait for confirm. */
        r = unifi_mlme_blocking_request(priv, priv->wext_client, &signal, NULL, timeout);
        if (r) {
            unifi_error(priv, "failed to send DELETEKEYS request, error %d\n", r);
            return r;
        }

        rc = priv->wext_client->reply_signal->u.MlmeDeletekeysConfirm.ResultCode;
        if (rc) {
            unifi_notice(priv, "DELETEKEYS request was rejected with result 0x%X (%s)\n",
                         rc, lookup_result_code(rc));
            return -EIO;
        }

        priv->wext_conf.generic_ie_len = 0;

        return 0;
    }


    /*
     * Request is to set a key, not delete
     */

    /* Pick out WEP and use set_wep_key(). */
    if (ext->alg == IW_ENCODE_ALG_WEP) {
        /* WEP-40, WEP-104 */

        /* Check for valid key length */
        if (!((ext->key_len == 5) || (ext->key_len == 13))) {
            unifi_trace(priv, UDBG1, KERN_ERR "Invalid length for WEP key: %d\n", ext->key_len);
            return -EINVAL;
        }

        unifi_trace(priv, UDBG1, "Setting WEP key %d\n", keyid+1);
        r = set_wep_key(priv, keyid+1, keydata, ext->key_len);
        if (r) {
            return r;
        }

        /*  IW_ENCODE_EXT_SET_TX_KEY means use this key */
        if (ext->ext_flags & IW_ENCODE_EXT_SET_TX_KEY) {
            unifi_trace(priv, UDBG1, "Setting WEP key %d as default\n", keyid+1);
            r = set_default_wep_key(priv, keyid);
            if (r) {
                return r;
            }
            /* Remember current key id */
            priv->wext_conf.wep_key_id = keyid;
        }

        return 0;
    }

    /*
     *
     * If we reach here, we are dealing with a WPA/WPA2 key
     *
     */
    /*
     * TKIP keys from wpa_supplicant need swapping.
     * What about other supplicants (when they come along)?
     */
    if ((ext->alg == IW_ENCODE_ALG_TKIP) && (ext->key_len == 32)) {
        memcpy(tkip_key, keydata, 16);
        memcpy(tkip_key + 16, keydata + 24, 8);
        memcpy(tkip_key + 24, keydata + 16, 8);
        keydata = tkip_key;
    }

    key_type = (ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY) ?
        CSR_GROUP : /* Group Key */
        CSR_PAIRWISE; /* Pairwise Key */

    r = set_rsn_key(priv, ext->addr.sa_data, ext->alg,
                    keyid, keydata, ext->key_len, key_type,
                    ((ext->ext_flags & IW_ENCODE_EXT_RX_SEQ_VALID) ?
                     ext->rx_seq : NULL));
    if (r) {
        return r;
    }


    /*
     * Set protection
     */
    r = mlme_set_protection(priv, ext->addr.sa_data, CSR_PT_RX_TX, key_type);
    if (r) {
        return r;
    }

    /*
     * If this was the GROUP key, we have completed the key exchange and
     * can open the 802.1X controlled port.
     */
    if (key_type == CSR_GROUP) {
        priv->wext_conf.block_controlled_port = unifi_8021xPortOpen;
    }


    func_exit();
    return 0;
} /* _unifi_siwencodeext() */

static int
unifi_siwencodeext(struct net_device *dev, struct iw_request_info *info,
                   union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int err;

    CHECK_INITED(priv);

    LOCK_DRIVER(priv);
    err = _unifi_siwencodeext(dev, info, wrqu, extra);
    UNLOCK_DRIVER(priv);

    return err;
} /* unifi_siwencodeext() */


static int
unifi_giwencodeext(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    return -EOPNOTSUPP;
} /* unifi_giwencodeext() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_siwpmksa
 *
 *      SIOCSIWPMKSA - PMKSA cache operation
 *      The caller passes a pmksa structure:
 *        - cmd         one of ADD, REMOVE, FLUSH
 *        - bssid       MAC address
 *        - pmkid       ID string (16 bytes)
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      This is not needed since we provide a siwgenie method.
 * ---------------------------------------------------------------------------
 */
static int
unifi_siwpmksa(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    struct iw_pmksa *pmksa = (struct iw_pmksa *)extra;

    unifi_trace(netdev_priv(dev),
                UDBG1, "SIWPMKSA: cmd %d, %02x:%02x:%02x:%02x:%02x:%02x\n",
                pmksa->cmd,
                pmksa->bssid.sa_data[0], pmksa->bssid.sa_data[1],
                pmksa->bssid.sa_data[2], pmksa->bssid.sa_data[3],
                pmksa->bssid.sa_data[4], pmksa->bssid.sa_data[5]);

    return -EOPNOTSUPP;
} /* unifi_siwpmksa() */

#endif /* WIRELESS_EXT > 17 */



/*
 * ---------------------------------------------------------------------------
 *  unifi_get_wireless_stats
 *
 *      get_wireless_stats method for Linux wireless extensions.
 *
 *  Arguments:
 *      dev             Pointer to associated netdevice.
 *
 *  Returns:
 *      Pointer to iw_statistics struct.
 * ---------------------------------------------------------------------------
 */
struct iw_statistics *
unifi_get_wireless_stats(struct net_device *dev)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);

    if (priv->init_progress != UNIFI_INIT_COMPLETED) {
        return NULL;
    }

    return &priv->wext_conf.wireless_stats;
} /* unifi_get_wireless_stats() */


/*
 * Structures to export the Wireless Handlers
 */

static const struct iw_priv_args unifi_private_args[] = {
/*{ cmd,         set_args,                            get_args, name } */
    { SIOCIWS80211POWERSAVEPRIV, IW_PRIV_TYPE_BYTE | IW_PRIV_SIZE_FIXED | 1,
    IW_PRIV_TYPE_NONE, "iwprivs80211ps" },
    { SIOCIWG80211POWERSAVEPRIV, IW_PRIV_TYPE_NONE,
    IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | IWPRIV_POWER_SAVE_MAX_STRING, "iwprivg80211ps" },
};

static const iw_handler unifi_handler[] =
{
    (iw_handler) unifi_siwcommit,           /* SIOCSIWCOMMIT */
    (iw_handler) unifi_giwname,             /* SIOCGIWNAME */
    (iw_handler) NULL,                      /* SIOCSIWNWID */
    (iw_handler) NULL,                      /* SIOCGIWNWID */
    (iw_handler) unifi_siwfreq,             /* SIOCSIWFREQ */
    (iw_handler) unifi_giwfreq,             /* SIOCGIWFREQ */
    (iw_handler) unifi_siwmode,             /* SIOCSIWMODE */
    (iw_handler) unifi_giwmode,             /* SIOCGIWMODE */
    (iw_handler) NULL,                      /* SIOCSIWSENS */
    (iw_handler) NULL,                      /* SIOCGIWSENS */
    (iw_handler) NULL,                      /* SIOCSIWRANGE */
    (iw_handler) unifi_giwrange,            /* SIOCGIWRANGE */
    (iw_handler) NULL,                      /* SIOCSIWPRIV */
    (iw_handler) NULL,                      /* SIOCGIWPRIV */
    (iw_handler) NULL,                      /* SIOCSIWSTATS */
    (iw_handler) NULL,                      /* SIOCGIWSTATS */
#if 0
    iw_handler_set_spy,                     /* SIOCSIWSPY */
    iw_handler_get_spy,                     /* SIOCGIWSPY */
    iw_handler_set_thrspy,                  /* SIOCSIWTHRSPY */
    iw_handler_get_thrspy,                  /* SIOCGIWTHRSPY */
#else
    (iw_handler) NULL,                      /* SIOCSIWSPY */
    (iw_handler) NULL,                      /* SIOCGIWSPY */
    (iw_handler) NULL,                      /* SIOCSIWTHRSPY */
    (iw_handler) NULL,                      /* SIOCGIWTHRSPY */
#endif
    (iw_handler) unifi_siwap,               /* SIOCSIWAP */
    (iw_handler) unifi_giwap,               /* SIOCGIWAP */
#if WIRELESS_EXT > 17
    /* WPA : IEEE 802.11 MLME requests */
    unifi_siwmlme,              /* SIOCSIWMLME, request MLME operation */
#else
    (iw_handler) NULL,                      /* -- hole -- */
#endif
    (iw_handler) NULL,                      /* SIOCGIWAPLIST */
    (iw_handler) unifi_siwscan,             /* SIOCSIWSCAN */
    (iw_handler) unifi_giwscan,             /* SIOCGIWSCAN */
    (iw_handler) unifi_siwessid,            /* SIOCSIWESSID */
    (iw_handler) unifi_giwessid,            /* SIOCGIWESSID */
    (iw_handler) NULL,                      /* SIOCSIWNICKN */
    (iw_handler) NULL,                      /* SIOCGIWNICKN */
    (iw_handler) NULL,                      /* -- hole -- */
    (iw_handler) NULL,                      /* -- hole -- */
    unifi_siwrate,                          /* SIOCSIWRATE */
    unifi_giwrate,                          /* SIOCGIWRATE */
    unifi_siwrts,                           /* SIOCSIWRTS */
    unifi_giwrts,                           /* SIOCGIWRTS */
    unifi_siwfrag,                          /* SIOCSIWFRAG */
    unifi_giwfrag,                          /* SIOCGIWFRAG */
    (iw_handler) NULL,                      /* SIOCSIWTXPOW */
    (iw_handler) NULL,                      /* SIOCGIWTXPOW */
    (iw_handler) NULL,                      /* SIOCSIWRETRY */
    (iw_handler) NULL,                      /* SIOCGIWRETRY */
    unifi_siwencode,                        /* SIOCSIWENCODE */
    unifi_giwencode,                        /* SIOCGIWENCODE */
    unifi_siwpower,                         /* SIOCSIWPOWER */
    unifi_giwpower,                         /* SIOCGIWPOWER */
#if WIRELESS_EXT > 17
    (iw_handler) NULL,                      /* -- hole -- */
    (iw_handler) NULL,                      /* -- hole -- */

    /* WPA : Generic IEEE 802.11 informatiom element (e.g., for WPA/RSN/WMM). */
    unifi_siwgenie,             /* SIOCSIWGENIE */      /* set generic IE */
    unifi_giwgenie,             /* SIOCGIWGENIE */      /* get generic IE */

    /* WPA : Authentication mode parameters */
    unifi_siwauth,              /* SIOCSIWAUTH */       /* set authentication mode params */
    unifi_giwauth,              /* SIOCGIWAUTH */       /* get authentication mode params */

    /* WPA : Extended version of encoding configuration */
    unifi_siwencodeext,         /* SIOCSIWENCODEEXT */  /* set encoding token & mode */
    unifi_giwencodeext,         /* SIOCGIWENCODEEXT */  /* get encoding token & mode */

    /* WPA2 : PMKSA cache management */
    unifi_siwpmksa,             /* SIOCSIWPMKSA */      /* PMKSA cache operation */
    (iw_handler) NULL,          /* -- hole -- */
#endif /* WIRELESS_EXT > 17 */
};


static const iw_handler unifi_private_handler[] =
{
    iwprivs80211ps,
    iwprivg80211ps,
};

struct iw_handler_def unifi_iw_handler_def =
{
    .num_standard       = sizeof(unifi_handler) / sizeof(iw_handler),
    .num_private        = sizeof(unifi_private_handler) / sizeof(iw_handler),
    .num_private_args   = sizeof(unifi_private_args) / sizeof(struct iw_priv_args),
    .standard           = (iw_handler *) unifi_handler,
    .private            = (iw_handler *) unifi_private_handler,
    .private_args       = (struct iw_priv_args *) unifi_private_args,
#if IW_HANDLER_VERSION >= 6
    .get_wireless_stats = unifi_get_wireless_stats,
#endif
#if 0
    .spy_offset         = ((void *) (&((struct unifi_info *) NULL)->spy_data) -
                           (void *) NULL),
#endif
};



/* 
 * ---------------------------------------------------------------------------
 * uf_init_wext_interface
 *
 *      Registers the wext client and initialises wext related parameters.
 *
 * Arguments:
 *      priv            Pointer to device private context struct
 *
 * Returns:
 *      0 on success, -1 on error.
 * ---------------------------------------------------------------------------
 */
int uf_init_wext_interface(unifi_priv_t *priv)
{
    /* register wext as a client */
    priv->wext_client = ul_register_client(priv, 0, sme_native_mlme_event_handler);
    if (priv->wext_client == NULL) {
        printk(KERN_ERR "Failed to register WEXT as a unifi client\n");
        return -1;
    }

    /* Set up fallback config */
    priv->wext_conf.channel = -1;
    priv->wext_conf.beacon_period = 100;        /* to use in starting AdHoc network */
    priv->wext_conf.join_failure_timeout  = 2000;  /* in TU (1024us) */
    priv->wext_conf.auth_failure_timeout  = 2000;  /* in TU (1024us) */
    priv->wext_conf.assoc_failure_timeout = 2000;  /* in TU (1024us) */
    memset(priv->wext_conf.current_bssid, 0xFF, ETH_ALEN);
    priv->wext_conf.disable_join_on_ssid_set = 0;
    priv->wext_conf.bss_wmm_capabilities = 0;
    priv->wext_conf.wmm_bss_uapsd_mask = 0x0F;    /* U-APSD */

    /* power save parameters, modify with "iwconfig power ..." */
    priv->wext_conf.power_mode = 0;             /* "on" or "off" */
    priv->wext_conf.assoc_listen_interval = 10; /* "period n" */
    priv->wext_conf.wakeup_for_dtims = 1;       /* "unicast" or "all" */

    priv->wext_conf.wireless_stats.qual.qual = 0;
    priv->wext_conf.wireless_stats.qual.level = 0;
    priv->wext_conf.wireless_stats.qual.noise = 0;
    priv->wext_conf.wireless_stats.qual.updated = 0x70;

    return 0;
} /* uf_init_wext_interface() */



/* 
 * ---------------------------------------------------------------------------
 * uf_deinit_wext_interface
 *
 *      Deregisters the wext client.
 *
 * Arguments:
 *      priv            Pointer to device private context struct
 *
 * Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void uf_deinit_wext_interface(unifi_priv_t *priv)
{
    /* Unregister WEXT as a client. */
    if (priv->wext_client) {
        ul_deregister_client(priv->wext_client);
        priv->wext_client = NULL;
    }
} /* uf_deinit_wext_interface() */



/* 
 * ---------------------------------------------------------------------------
 * _clear_association_status
 *
 *      Reinitialise 802.11 association related wext parameters.
 *
 * Arguments:
 *      priv            Pointer to device private context struct
 *
 * Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void 
_clear_association_status(unifi_priv_t *priv)
{
    memset(priv->wext_conf.current_ssid, 0, UNIFI_MAX_SSID_LEN);
    memset((void*)priv->wext_conf.current_bssid, 0, ETH_ALEN);
    priv->wext_conf.capability = 0;
    priv->wext_conf.flag_associated = 0;
    if (priv->wext_conf.mode == IW_MODE_INFRA) {
        priv->wext_conf.channel = -1;
    }
} /* _clear_association_status() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_connected_ind
 *
 *      Called when a CONNECTED indication is received from UniFi.
 * 
 *  Arguments:
 *      ospriv          OS private context pointer.
 *      ind             Pointer to a MLME_CONNECTED_INDICATION struct.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      The connect indication from UniFi is used to configure the 
 *      network device status of the driver. 
 * ---------------------------------------------------------------------------
 */
static void
unifi_connected_ind(unifi_priv_t *priv, const CSR_MLME_CONNECTED_INDICATION *ind)
{
    struct net_device *dev = priv->netdev;
    CsrInt32 csr_r;

    func_enter();

    unifi_trace(priv, UDBG2, "ConnectionStatus = %d\n", ind->ConnectionStatus);

    if (priv->netdev_registered) {
        /* Report using device name when we have registered the net device. */
        unifi_notice(priv, "%s: link is %s\n",
                     dev->name,
                     (ind->ConnectionStatus) ? "up" : "down");
    } else {
        unifi_notice(priv, "unifi%d: link is %s\n",
                     priv->instance,
                     (ind->ConnectionStatus) ? "up" : "down");
    }

    if (ind->ConnectionStatus) {
        netif_carrier_on(dev);
        UF_NETIF_TX_WAKE_ALL_QUEUES(priv->netdev);
        priv->connected = UnifiConnected;

        /*
         * Send wireless-extension event up to userland to announce
         * connection to an IBSS.
         */
        if ((priv->wext_conf.capability & SIG_CAP_IBSS) == SIG_CAP_IBSS) {
            wext_send_assoc_event(priv, priv->wext_conf.current_bssid,
                                  NULL, 0, NULL, 0, NULL, 0);
            priv->wext_conf.flag_associated = 1;
        }

        /*
         * If the power save mode is active, we should disable
         * the deep sleep signaling, to avoid the overhead.
         */
        if (priv->wext_conf.power_mode == 0) {
            csr_r = unifi_configure_low_power_mode(priv->card,
                                                   UNIFI_LOW_POWER_DISABLED,
                                                   UNIFI_PERIODIC_WAKE_HOST_DISABLED);
        }

    } else {
        netif_carrier_off(dev);
        priv->connected = UnifiNotConnected;

        /*
         * Send wireless-extension event up to userland to announce
         * connection lost to a BSS.
         */
        wext_send_disassoc_event(priv);
        priv->wext_conf.flag_associated = 0;

        /*
         * Enable the deep sleep signaling.
         */
        csr_r = unifi_configure_low_power_mode(priv->card,
                                               UNIFI_LOW_POWER_ENABLED,
                                               UNIFI_PERIODIC_WAKE_HOST_DISABLED);

    }

    func_exit();
} /* unifi_connected_ind() */


/*
 * ---------------------------------------------------------------------------
 *  sme_native_mlme_event_handler
 *
 *      Callback function to be used as the udi_event_callback when registering
 *      as a client.
 *      This function implements a blocking request-reply interface for WEXT.
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
void
sme_native_mlme_event_handler(ul_client_t *pcli,
                              const u8 *sig_packed, int sig_len,
                              const bulk_data_param_t *bulkdata,
                              int dir)
{
    CSR_SIGNAL signal;
    int signal_len;
    unifi_priv_t *priv = uf_find_instance(pcli->instance);
    int id, r;

    func_enter();

    /* Just a sanity check */
    if ((sig_packed == NULL) || (sig_len <= 0)) {
        return;
    }

    /* Get the unpacked signal */
    r = read_unpack_signal(sig_packed, &signal);
    if (r == 0) {
        signal_len = SigGetSize(&signal);
    } else {
        unifi_error(priv, "Received unknown or corrupted signal.\n");
        return;
    }

    id = signal.SignalPrimitiveHeader.SignalId;
    unifi_trace(priv, UDBG4, "wext - Process signal 0x%X %s\n", id, lookup_signal_name(id));

    /*
     * Take the appropriate action for the signal.
     */
    switch (id) {
        /*
         * Confirm replies from UniFi.
         * These all have zero or one CSR_DATAREF member.
         */
      case CSR_MLME_SET_CONFIRM_ID:
      case CSR_MLME_GET_CONFIRM_ID:
      case CSR_MLME_GET_NEXT_CONFIRM_ID:
      case CSR_MLME_SCAN_CONFIRM_ID:
      case CSR_MLME_JOIN_CONFIRM_ID:
      case CSR_MLME_POWERMGT_CONFIRM_ID:
      case CSR_MLME_DEAUTHENTICATE_CONFIRM_ID:
      case CSR_MLME_DISASSOCIATE_CONFIRM_ID:
      case CSR_MLME_RESET_CONFIRM_ID:
      case CSR_MLME_START_CONFIRM_ID:
      case CSR_MLME_SNIFFJOIN_CONFIRM_ID:
      case CSR_MLME_AUTHENTICATE_CONFIRM_ID:
      case CSR_MLME_ASSOCIATE_CONFIRM_ID:
      case CSR_MLME_REASSOCIATE_CONFIRM_ID:
      case CSR_MLME_SETKEYS_CONFIRM_ID:
      case CSR_MLME_DELETEKEYS_CONFIRM_ID:
      case CSR_MLME_SETPROTECTION_CONFIRM_ID:
      case CSR_MLME_ADDTS_CONFIRM_ID:
          unifi_mlme_copy_reply_and_wakeup_client(pcli, &signal, signal_len, bulkdata);
        break;

      case CSR_MA_SNIFFDATA_INDICATION_ID:
#ifdef UNIFI_SNIFF_ARPHRD
        ma_sniffdata_ind(priv,
                         &signal.u.MaSniffdataIndication,
                         bulkdata);
#endif
        break;

      case CSR_MLME_CONNECTED_INDICATION_ID:
        unifi_connected_ind(priv,
                            &signal.u.MlmeConnectedIndication);
        break;

      case CSR_MLME_MICHAELMICFAILURE_INDICATION_ID:
        unifi_warning(priv, "MichaelMIC Failure: %d, %02x:%02x:%02x:%02x:%02x:%02x type %d, id %d, tsc %llu\n",
                      signal.u.MlmeMichaelmicfailureIndication.Count,
                      signal.u.MlmeMichaelmicfailureIndication.Address.x[0],
                      signal.u.MlmeMichaelmicfailureIndication.Address.x[1],
                      signal.u.MlmeMichaelmicfailureIndication.Address.x[2],
                      signal.u.MlmeMichaelmicfailureIndication.Address.x[3],
                      signal.u.MlmeMichaelmicfailureIndication.Address.x[4],
                      signal.u.MlmeMichaelmicfailureIndication.Address.x[5],
                      signal.u.MlmeMichaelmicfailureIndication.KeyType,
                      signal.u.MlmeMichaelmicfailureIndication.KeyId,
                      signal.u.MlmeMichaelmicfailureIndication.Tsc);

        wext_send_michaelmicfailure_event(priv,
                                          &signal.u.MlmeMichaelmicfailureIndication);
        break;

      case CSR_MLME_DEAUTHENTICATE_INDICATION_ID:
        unifi_info(priv, "deauthenticated by AP %02X:%02X:%02X:%02X:%02X:%02X with reason %d (%s)\n",
                   signal.u.MlmeDeauthenticateIndication.PeerStaAddress.x[0],
                   signal.u.MlmeDeauthenticateIndication.PeerStaAddress.x[1],
                   signal.u.MlmeDeauthenticateIndication.PeerStaAddress.x[2],
                   signal.u.MlmeDeauthenticateIndication.PeerStaAddress.x[3],
                   signal.u.MlmeDeauthenticateIndication.PeerStaAddress.x[4],
                   signal.u.MlmeDeauthenticateIndication.PeerStaAddress.x[5],
                   signal.u.MlmeDeauthenticateIndication.ReasonCode,
                   lookup_reason_code(signal.u.MlmeDeauthenticateIndication.ReasonCode));

        _clear_association_status(priv);
        wext_send_disassoc_event(priv);
        break;

      case CSR_MLME_DISASSOCIATE_INDICATION_ID:
        unifi_info(priv, "disassociated by AP %02X:%02X:%02X:%02X:%02X:%02X with reason %d (%s)\n",
                   signal.u.MlmeDisassociateIndication.PeerStaAddress.x[0],
                   signal.u.MlmeDisassociateIndication.PeerStaAddress.x[1],
                   signal.u.MlmeDisassociateIndication.PeerStaAddress.x[2],
                   signal.u.MlmeDisassociateIndication.PeerStaAddress.x[3],
                   signal.u.MlmeDisassociateIndication.PeerStaAddress.x[4],
                   signal.u.MlmeDisassociateIndication.PeerStaAddress.x[5],
                   signal.u.MlmeDisassociateIndication.ReasonCode,
                   lookup_reason_code(signal.u.MlmeDisassociateIndication.ReasonCode));

        _clear_association_status(priv);
        wext_send_disassoc_event(priv);
        break;


      case CSR_MLME_SCAN_INDICATION_ID:
        unifi_scan_indication_handler(priv,
                                      &signal.u.MlmeScanIndication,
                                      bulkdata->d[0].os_data_ptr,
                                      bulkdata->d[0].data_length);
        break;

      default:
        break;
    }

    func_exit();
} /* sme_native_mlme_event_handler() */


