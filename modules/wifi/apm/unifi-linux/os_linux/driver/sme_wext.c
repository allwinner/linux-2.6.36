/*
 * ---------------------------------------------------------------------------
 * FILE:     sme_wext.c
 *
 * PURPOSE:
 *      Handlers for ioctls from iwconfig.
 *      These provide the control plane operations.
 *
 * Copyright (C) 2007-2009 by Cambridge Silicon Radio Ltd.
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
#include "unifi_priv.h"


#ifdef CSR_SME_EMB
#include "fsm/fsm_debug.h"
#endif

#define CHECK_INITED(_priv)                             \
do {                                                    \
    if (_priv->init_progress != UNIFI_INIT_COMPLETED) { \
        unifi_trace(_priv, UDBG2, "%s unifi not ready, failing wext call\n", __FUNCTION__); \
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
 *  uf_sme_wext_set_defaults
 *
 *      Set up power-on defaults for driver config.
 *
 *      Note: The SME Management API *cannot* be used in this function.
 *
 *  Arguments:
 *      priv            Pointer to device private context struct
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
uf_sme_wext_set_defaults(unifi_priv_t *priv)
{
    memset(&priv->connection_config, 0, sizeof(unifi_ConnectionConfig));

    priv->connection_config.bssType = unifi_Infrastructure;
    priv->connection_config.authModeMask = unifi_80211AuthOpen;
    priv->connection_config.encryptionModeMask = unifi_EncryptionCipherNone;
    priv->connection_config.privacyMode = unifi_80211PrivacyDisabled;
    priv->connection_config.wmmQosInfo = 0xFF;
    priv->connection_config.ifIndex = unifi_GHZ_Both;
    priv->connection_config.adhocJoinOnly = FALSE;
    priv->connection_config.adhocChannel = 6;

    priv->wep_tx_key_index = 0;

    priv->wext_wireless_stats.qual.qual = 0;
    priv->wext_wireless_stats.qual.level = 0;
    priv->wext_wireless_stats.qual.noise = 0;
    priv->wext_wireless_stats.qual.updated = 0x70;

} /* uf_sme_wext_set_defaults() */


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
iwprivsdefs(struct net_device *dev, struct iw_request_info *info,
            union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int r;
    unifi_AppValue sme_app_value;

    unifi_trace(priv, UDBG1, "iwprivs80211defaults: reload defaults\n");

    uf_sme_wext_set_defaults(priv);

    /* Get, modify and set the MIB data */
    sme_app_value.id = unifi_MibConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "iwprivs80211defaults: Get unifi_MibConfigValue failed.\n");
        return r;
    }
    sme_app_value.unifi_Value_union.mibConfig.dot11RtsThreshold = 2347;
    sme_app_value.unifi_Value_union.mibConfig.dot11FragmentationThreshold = 2346;
    r = sme_mgt_set_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "iwprivs80211defaults: Set unifi_MibConfigValue failed.\n");
        return r;
    }

    sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveLow;
    sme_app_value.unifi_Value_union.powerConfig.listenIntervalBeacons = 100;
    sme_app_value.unifi_Value_union.powerConfig.rxDtims = 1;
    sme_app_value.id = unifi_PowerConfigValue;

    r = sme_mgt_set_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "iwprivs80211defaults: Set unifi_PowerConfigValue failed.\n");
        return r;
    }

    return 0;
} /* iwprivsdefs() */

static int
iwprivs80211ps(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int r = 0;
    int ps_mode = (int)(*extra);
    unifi_AppValue sme_app_value;

    unifi_trace(priv, UDBG1, "iwprivs80211ps: power save mode = %d\n", ps_mode);

    sme_app_value.id = unifi_PowerConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "iwprivs80211ps: Get unifi_PowerConfigValue failed.\n");
        return r;
    }

    switch (ps_mode) {
      case CSR_PMM_ACTIVE_MODE:
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveLow;
        break;
      case CSR_PMM_POWER_SAVE:
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveHigh;
        break;
      case CSR_PMM_FAST_POWER_SAVE:
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveMed;
        break;
      default:
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveAuto;
        break;
    }

    sme_app_value.id = unifi_PowerConfigValue;
    r = sme_mgt_set_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "iwprivs80211ps: Set unifi_PowerConfigValue failed.\n");
    }

    return r;
}

static int
iwprivg80211ps(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    unifi_AppValue sme_app_value;
    int r;

    sme_app_value.id = unifi_PowerConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "iwprivg80211ps: Get 802.11 power mode failed.\n");
        return r;
    }

    switch (sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel) {
      case unifi_PowerSaveLow:
        snprintf(extra, IWPRIV_POWER_SAVE_MAX_STRING,
                 "Power save mode: %d (Active)",
                 sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel);
        break;
      case unifi_PowerSaveMed:
        snprintf(extra, IWPRIV_POWER_SAVE_MAX_STRING,
                 "Power save mode: %d (Fast)",
                 sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel);
        break;
      case unifi_PowerSaveHigh:
        snprintf(extra, IWPRIV_POWER_SAVE_MAX_STRING,
                 "Power save mode: %d (Full)",
                 sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel);
        break;
      case unifi_PowerSaveAuto:
        snprintf(extra, IWPRIV_POWER_SAVE_MAX_STRING,
                 "Power save mode: %d (Auto)",
                 sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel);
        break;
      default:
        snprintf(extra, IWPRIV_POWER_SAVE_MAX_STRING,
                 "Power save mode: %d (Unknown)",
                 sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel);
        break;
    }

    wrqu->data.length = strlen(extra) + 1;

    return 0;
}

static int
iwprivssmedebug(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    if (priv->smepriv != NULL && extra != NULL) {
#ifdef CSR_SME_EMB
        unifi_dbg_cmd_req(priv->smepriv, extra);
#endif
#ifdef CSR_SME_USERSPACE
        unifi_dbg_cmd_req(priv->smepriv, extra);
#endif
    }
    return 0;
}

#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
static int
iwprivsconfwapi(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    CsrUint8 enable;
    func_enter();

    unifi_trace(priv, UDBG1, "iwprivsconfwapi\n" );

    enable = *(CsrUint8*)(extra);

    if (enable) {
        priv->connection_config.authModeMask = unifi_80211AuthOpen;
        priv->connection_config.authModeMask |= (unifi_WAPIAuthWAIPSK | unifi_WAPIAuthWAI);
        priv->connection_config.encryptionModeMask |=
                        unifi_EncryptionCipherPairwiseSms4 | unifi_EncryptionCipherGroupSms4;
    } else {
        priv->connection_config.authModeMask &= ~(unifi_WAPIAuthWAIPSK | unifi_WAPIAuthWAI);
        priv->connection_config.encryptionModeMask &=
                            ~(unifi_EncryptionCipherPairwiseSms4 | unifi_EncryptionCipherGroupSms4);
    }

    func_exit();
    return 0;
}

static int
iwprivswpikey(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int r = 0, i;
    unifi_Key key;
    unifiio_wapi_key_t inKey;
    func_enter();

    unifi_trace(priv, UDBG1, "iwprivswpikey\n" );

    inKey = *(unifiio_wapi_key_t*)(extra);

    if (inKey.unicastKey) {
        key.keyType   = unifi_PairwiseKey;
    } else {
        key.keyType   = unifi_GroupKey;
    }

    key.keyIndex  = inKey.keyIndex;
    /* memcpy(key.keyRsc, inKey.keyRsc, 16); */
    for (i = 0; i < 16; i+= 2)
    {
        key.keyRsc[i/2] = inKey.keyRsc[i+1] << 8 | inKey.keyRsc[i];
    }
    memcpy(key.address.data, inKey.address, 6);
    key.keyLength = 32;
    memcpy(key.key, inKey.key, 32);
    key.authenticator = 0;
    key.wepTxKey = 0;

    unifi_trace(priv, UDBG1, "keyType = %d, keyIndex = %d, wepTxKey = %d, keyRsc = %x:%x, auth = %d, address = %x:%x, "
                "keylength = %d, key = %x:%x\n", key.keyType, key.keyIndex, key.wepTxKey,
                key.keyRsc[0], key.keyRsc[7], key.authenticator,
                key.address.data[0], key.address.data[5], key.keyLength, key.key[0],
                key.key[15]);

    r = sme_mgt_key(priv, &key, unifi_ListActionAdd);
    if (r) {
        unifi_error(priv, "SETKEYS request was rejected with result %d\n", r);
        return convert_sme_error(r);
    }

    func_exit();
    return r;
}
#endif


static int
unifi_giwname(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    char *name = wrqu->name;
    unifi_trace(priv, UDBG2, "unifi_giwname\n");

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

    func_enter();
    unifi_trace(priv, UDBG2, "unifi_siwfreq\n");

    /*
     * Channel is stored in the connection configuration,
     * and set later when ask for a connection.
     */
    if ((freq->e == 0) && (freq->m <= 1000)) {
        priv->connection_config.adhocChannel = freq->m;
    } else {
        priv->connection_config.adhocChannel = wext_freq_to_channel(freq->m, freq->e);
    }

    func_exit();
    return 0;
} /* unifi_siwfreq() */


static int
unifi_giwfreq(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_freq *freq = (struct iw_freq *)wrqu;
    int err = 0;
    unifi_AppValue sme_app_value;

    func_enter();
    unifi_trace(priv, UDBG2, "unifi_giwfreq\n");
    CHECK_INITED(priv);

    sme_app_value.id = unifi_ConnectionInfoValue;
    err = sme_mgt_get_value(priv, &sme_app_value);

    freq->m = channel_to_mhz(sme_app_value.unifi_Value_union.connectionInfo.channelNumber,
                             (sme_app_value.unifi_Value_union.connectionInfo.networkType80211 == unifi_GHZ_5_0));
    freq->e = 6;

    func_exit();
    return convert_sme_error(err);
} /* unifi_giwfreq() */


static int
unifi_siwmode(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);

    func_enter();
    unifi_trace(priv, UDBG2, "unifi_siwmode\n");

    switch(wrqu->mode) {
      case IW_MODE_ADHOC:
        priv->connection_config.bssType = unifi_Adhoc;
        break;
      case IW_MODE_INFRA:
        priv->connection_config.bssType = unifi_Infrastructure;
        break;
      case IW_MODE_AUTO:
        priv->connection_config.bssType = unifi_AnyBss;
        break;
      default:
        unifi_notice(priv, "Unknown IW MODE value.\n");
    }

    /* Clear the SSID and BSSID configuration */
    priv->connection_config.ssid.length = 0;
    memset(priv->connection_config.bssid.data, 0xFF, ETH_ALEN);

    func_exit();
    return 0;
} /* unifi_siwmode() */



static int
unifi_giwmode(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int r = 0;
    unifi_AppValue sme_app_value;

    func_enter();
    unifi_trace(priv, UDBG2, "unifi_giwmode\n");
    CHECK_INITED(priv);

    sme_app_value.id = unifi_ConnectionConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r == 0) {
        switch(sme_app_value.unifi_Value_union.connectionConfig.bssType) {
          case unifi_Adhoc:
            wrqu->mode = IW_MODE_ADHOC;
            break;
          case unifi_Infrastructure:
            wrqu->mode = IW_MODE_INFRA;
            break;
          default:
            wrqu->mode = IW_MODE_AUTO;
            unifi_notice(priv, "Unknown IW MODE value.\n");
        }
    }

    unifi_trace(priv, UDBG4, "unifi_giwmode: mode = %d\n", wrqu->mode);
    func_exit();
    return r;
} /* unifi_giwmode() */



static int
unifi_giwrange(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    struct iw_point *dwrq = &wrqu->data;
    struct iw_range *range = (struct iw_range *) extra;
    int i;

    unifi_trace(NULL, UDBG2, "unifi_giwrange\n");

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
    for (i = 0; (i < range->num_frequency) && (i < IW_MAX_FREQUENCIES); i++) {
        int chan = i + 1;
        range->freq[i].i = chan;
        range->freq[i].m = channel_to_mhz(chan, 0);
        range->freq[i].e = 6;
    }
    if ((i+3) < IW_MAX_FREQUENCIES) {
        range->freq[i].i = 36;
        range->freq[i].m = channel_to_mhz(36, 1);
        range->freq[i].e = 6;
        range->freq[i+1].i = 40;
        range->freq[i+1].m = channel_to_mhz(40, 1);
        range->freq[i+1].e = 6;
        range->freq[i+2].i = 44;
        range->freq[i+2].m = channel_to_mhz(44, 1);
        range->freq[i+2].e = 6;
        range->freq[i+3].i = 48;
        range->freq[i+3].m = channel_to_mhz(48, 1);
        range->freq[i+3].e = 6;
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
#if 1   //ices add
static int
unifi_giwrate(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra);

static int
unifi_siwpriv(struct net_device *dev, struct iw_request_info *info,
               struct iw_point *dwrq, char *ext)
{
	// TTPan
	unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
	int ret = 0;
	char * extra;

	func_enter();

	CHECK_INITED(priv);

	if (!(extra = kmalloc(dwrq->length, GFP_KERNEL)))
	    return -ENOMEM;

	if (copy_from_user(extra, dwrq->pointer, dwrq->length)) {
	    kfree(extra);
	    return -EFAULT;
	}
	//unifi_info(NULL, "%s: SIOCSIWPRIV request %s, info->cmd:%x, info->flags:%d, dwrq->length:%d\n",
	//	dev->name, extra, info->cmd, info->flags, dwrq->length);

	if (dwrq->length && extra) {

		if (strnicmp(extra, "RSSI", strlen("RSSI")) == 0) {
			char *p = extra;

			//ret = wl_iw_get_rssi(dev, info, (union iwreq_data *)dwrq, extra);
			p += snprintf(p, 80, "rssi %d ", priv->wext_wireless_stats.qual.level - 256); // MAX_WX_STRING=80
			((union iwreq_data *) dwrq)->data.length = p - extra + 1;
			//unifi_info(NULL, "qual: %d\n", priv->wext_wireless_stats.qual.qual);
			//unifi_info(NULL, "level: %d\n", priv->wext_wireless_stats.qual.level);
			//unifi_info(NULL, "noise: %d\n", priv->wext_wireless_stats.qual.noise);
			//unifi_info(NULL, "RSSI: %d\n", priv->wext_wireless_stats.qual.level - 256);
			unifi_trace(priv, UDBG1, "RSSI: %d\n", priv->wext_wireless_stats.qual.level - 256);//ices add

		} else if (strnicmp(extra, "LINKSPEED", strlen("LINKSPEED")) == 0) {
#if 0
			char *p = extra;
			struct iw_param *args = &((union iwreq_data *)dwrq)->bitrate;

			unifi_giwrate(dev, info, (union iwreq_data *)dwrq, extra);
			p += snprintf(p, 80, "LinkSpeed %d", (args->value)/1000000); // MAX_WX_STRING=80 // TODO-Fixed Value Now
			((union iwreq_data *) dwrq)->data.length = p - extra + 1;
			//unifi_info(NULL, "LINKSPEED: %d Mbps\n", (args->value)/1000000);
#endif
            char *p = extra;
            int r = 0;
            unifi_AppValue sme_app_value;
            sme_app_value.id = unifi_ConnectionStatsValue;
            r = sme_mgt_get_value(priv, &sme_app_value);
            r = sme_app_value.unifi_Value_union.connectionStats.unifiTxDataRate * 500000 / 1000000;
            p += snprintf(p, 80, "LinkSpeed %d", r); // MAX_WX_STRING=80 // Fixed Value = 54 Mbps
            ((union iwreq_data *) dwrq)->data.length = p - extra + 1;
            unifi_trace(priv, UDBG1, "LINKSPEED: %d Mbps\n",
                (sme_app_value.unifi_Value_union.connectionStats.unifiTxDataRate * 500000 / 1000000));//ices add
		} else if (strnicmp(extra, "MACADDR", strlen("MACADDR")) == 0) {
			char *p = extra;
			//unifi_info(NULL, "%s\n", "MACADDR");
			//unifi_info(NULL, "MAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
			//	priv->netdev->dev_addr[0], priv->netdev->dev_addr[1], priv->netdev->dev_addr[2],
			//	priv->netdev->dev_addr[3], priv->netdev->dev_addr[4], priv->netdev->dev_addr[5]);
			unifi_trace(priv, UDBG1, "MAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
				priv->netdev->dev_addr[0], priv->netdev->dev_addr[1], priv->netdev->dev_addr[2],
				priv->netdev->dev_addr[3], priv->netdev->dev_addr[4], priv->netdev->dev_addr[5]);//ices add

			p += snprintf(p, 80, "Macaddr = %02X:%02X:%02X:%02X:%02X:%02X\n", // MAX_WX_STRING=80
				priv->netdev->dev_addr[0], priv->netdev->dev_addr[1], priv->netdev->dev_addr[2],
				priv->netdev->dev_addr[3], priv->netdev->dev_addr[4], priv->netdev->dev_addr[5]);
			((union iwreq_data *) dwrq)->data.length = p - extra + 1;

		} else {
		    unifi_trace(priv, UDBG1, "Unknown PRIVATE command: %s: ignored\n", extra);  //ices add
			//unifi_info(NULL, "Unknown PRIVATE command: %s: ignored\n", extra);
			snprintf(extra, 80 , "OK"); // MAX_WX_STRING=80
			dwrq->length = strlen("OK") + 1;

		}
	}

	func_exit();

	if (extra) {
		if (copy_to_user(dwrq->pointer, extra, dwrq->length)) {
			kfree(extra);
			return -EFAULT;
		}

		kfree(extra);
	}

	return ret;
}
#endif

static int
unifi_siwap(struct net_device *dev, struct iw_request_info *info,
            union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int err = 0;
    const unsigned char zero_bssid[ETH_ALEN] = {0x00, 0x00, 0x00,
                                                0x00, 0x00, 0x00};

    func_enter();

    CHECK_INITED(priv);

    if (wrqu->ap_addr.sa_family != ARPHRD_ETHER) {
        return -EINVAL;
    }

    unifi_trace(priv, UDBG1, "unifi_siwap: asked for %02X:%02X:%02X:%02X:%02X:%02X\n",
                (u8)wrqu->ap_addr.sa_data[0],
                (u8)wrqu->ap_addr.sa_data[1],
                (u8)wrqu->ap_addr.sa_data[2],
                (u8)wrqu->ap_addr.sa_data[3],
                (u8)wrqu->ap_addr.sa_data[4],
                (u8)wrqu->ap_addr.sa_data[5]);

    if (!memcmp(wrqu->ap_addr.sa_data, zero_bssid, ETH_ALEN)) {
        priv->ignore_bssid_join = FALSE;
        err = sme_mgt_disconnect(priv);
        if (err) {
            unifi_trace(priv, UDBG4, "unifi_siwap: Disconnect failed, status %d\n", err);
        }
        return 0;
    }

    if (priv->ignore_bssid_join) {
        unifi_trace(priv, UDBG4, "unifi_siwap: ignoring second join\n");
        priv->ignore_bssid_join = FALSE;
    } else {
        memcpy(priv->connection_config.bssid.data, wrqu->ap_addr.sa_data, ETH_ALEN);
        unifi_trace(priv, UDBG1, "unifi_siwap: Joining %X:%X:%X:%X:%X:%X\n",
                    priv->connection_config.bssid.data[0],
                    priv->connection_config.bssid.data[1],
                    priv->connection_config.bssid.data[2],
                    priv->connection_config.bssid.data[3],
                    priv->connection_config.bssid.data[4],
                    priv->connection_config.bssid.data[5]);
        err = sme_mgt_connect(priv);
        if (err) {
            unifi_error(priv, "unifi_siwap: Join failed, status %d\n", err);
            func_exit();
            return convert_sme_error(err);
        }
    }
    func_exit();

    return 0;
} /* unifi_siwap() */


static int
unifi_giwap(struct net_device *dev, struct iw_request_info *info,
            union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    unifi_AppValue sme_app_value;
    int r = 0;
    CsrUint8 *bssid;

    func_enter();

    CHECK_INITED(priv);
    unifi_trace(priv, UDBG2, "unifi_giwap\n");

    sme_app_value.id = unifi_ConnectionInfoValue;
    r = sme_mgt_get_value(priv, &sme_app_value);

    if (r == 0) {
        bssid = sme_app_value.unifi_Value_union.connectionInfo.bssid.data;
        wrqu->ap_addr.sa_family = ARPHRD_ETHER;
        unifi_trace(priv, UDBG4,
                    "unifi_giwap: BSSID = %02X:%02X:%02X:%02X:%02X:%02X\n",
                    bssid[0], bssid[1], bssid[2],
                    bssid[3], bssid[4], bssid[5]);

        memcpy(wrqu->ap_addr.sa_data, bssid, ETH_ALEN);
    } else {
        memset(wrqu->ap_addr.sa_data, 0, ETH_ALEN);
    }

    func_exit();
    return 0;
} /* unifi_giwap() */


static int
unifi_siwscan(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int scantype;
    int r;
    unifi_SSID scan_ssid;
    unsigned char *channel_list = NULL;
    int chans_good = 0;
#if WIRELESS_EXT > 17
    struct iw_point *data = &wrqu->data;
    struct iw_scan_req *req = (struct iw_scan_req *) extra;
#endif

    func_enter();

    CHECK_INITED(priv);

    scantype = UNIFI_SCAN_ACTIVE;

#if WIRELESS_EXT > 17
    /* Providing a valid channel list will force an active scan */
    if (req) {
        if ((req->num_channels > 0) && (req->num_channels < IW_MAX_FREQUENCIES)) {
            channel_list = kmalloc(req->num_channels, GFP_KERNEL);
            if (channel_list) {
                int i;
                for (i = 0; i < req->num_channels; i++) {
                    /* Convert frequency to channel number */
                    int ch = wext_freq_to_channel(req->channel_list[i].m,
                                                  req->channel_list[i].e);
                    if (ch) {
                        channel_list[chans_good++] = ch;
                    }
                }
                unifi_trace(priv, UDBG1,
                            "SIWSCAN: Scanning %d channels\n", chans_good);
            } else {
                /* Fall back to scanning all */
                unifi_error(priv, "SIWSCAN: Can't alloc channel_list (%d)\n",
                            req->num_channels);
            }
        }
    }

    if (req && (data->flags & IW_SCAN_THIS_ESSID)) {
        memcpy(scan_ssid.ssid, req->essid, req->essid_len);
        scan_ssid.length = req->essid_len;
        unifi_trace(priv, UDBG1,
                    "SIWSCAN: Scanning for %.*s\n",
                    scan_ssid.length, scan_ssid.ssid);
    } else
#endif
    {
        unifi_trace(priv, UDBG1, "SIWSCAN: Scanning for all APs\n");
        scan_ssid.length = 0;
    }

    r = sme_mgt_scan_full(priv, &scan_ssid, chans_good, channel_list);
    if (r) {
        unifi_error(priv, "SIWSCAN: Scan returned error %d\n", r);
    } else {
        unifi_trace(priv, UDBG1, "SIWSCAN: Scan done\n");
        wext_send_scan_results_event(priv);
    }

    if (channel_list) {
        kfree(channel_list);
    }

    func_exit();
    return r;

} /* unifi_siwscan() */


static const unsigned char *
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
 * Translate scan data returned from the card to a card independent
 * format that the Wireless Tools will understand - Jean II
 */
int
unifi_translate_scan(struct net_device *dev,
                     struct iw_request_info *info,
                     char *current_ev, char *end_buf,
                     unifi_ScanResult *scan_data,
                     int scan_index)
{
    struct iw_event iwe;                /* Temporary buffer */
    unsigned char *info_elems;
    int info_elem_len;
    const unsigned char *elem;
    u16 capabilities;
    int signal, noise, snr;
    char *start_buf = current_ev;
    char *current_val;  /* For rates */
    int i, r;

    info_elems    = scan_data->informationElements;
    info_elem_len = scan_data->informationElementsLength;

    if (!scan_data->informationElementsLength || !scan_data->informationElements) {
        unifi_error(NULL, "*** NULL SCAN IEs ***\n");
        return -EIO;
    }

    /* get capinfo bits */
    capabilities = scan_data->capabilityInformation;

    unifi_trace(NULL, UDBG5, "Capabilities: 0x%x\n", capabilities);

    /* First entry *MUST* be the AP MAC address */
    memset(&iwe, 0, sizeof(iwe));
    iwe.cmd = SIOCGIWAP;
    iwe.u.ap_addr.sa_family = ARPHRD_ETHER;
    memcpy(iwe.u.ap_addr.sa_data, scan_data->bssid.data, ETH_ALEN);
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
    if (scan_data->bssType == unifi_Infrastructure) {
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
    iwe.u.freq.m = scan_data->channelFrequency;
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
    signal = scan_data->rssi; /* This value is in dBm */
    /* Clip range of snr */
    snr    = (scan_data->snr > 0) ? scan_data->snr : 0; /* In dB relative, from 0 - 255 */
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
            current_val +=r;

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
            current_val +=r;
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
    int r;

    CHECK_INITED(priv);

    unifi_trace(priv, UDBG1,
                "unifi_giwscan: buffer (%d bytes) \n",
                dwrq->length);
    r = sme_mgt_scan_results_get_async(priv, info, extra, dwrq->length);
    if (r < 0) {
        unifi_trace(priv, UDBG1,
                   "unifi_giwscan: buffer (%d bytes) not big enough.\n",
                   dwrq->length);
        return r;
    }

    dwrq->length = r;
    dwrq->flags = 0;

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

    func_enter();
    CHECK_INITED(priv);

    len = 0;
    if (data->flags & 1) {
        /* Limit length  */
        len = data->length;
        if (len > UNIFI_MAX_SSID_LEN) {
            len = UNIFI_MAX_SSID_LEN;
        }
    }

    unifi_trace(priv, UDBG1, "unifi_siwessid: asked for %*s\n", len, essid);
    unifi_trace(priv, UDBG2, " with authModeMask = %d", priv->connection_config.authModeMask);

    memset(priv->connection_config.bssid.data, 0xFF, ETH_ALEN);
    if (len) {
        if (essid[len - 1] == 0) {
            len --;
        }

        memcpy(priv->connection_config.ssid.ssid, essid, len);
        priv->connection_config.ssid.length = len;

    } else {
        priv->connection_config.ssid.length = 0;
    }

    err = sme_mgt_connect(priv);
    if (err) {
        unifi_error(priv, "unifi_siwessid: Join failed, status %d\n", err);
        func_exit();
        return convert_sme_error(err);
    }

    func_exit();
    return 0;
} /* unifi_siwessid() */


static int
unifi_giwessid(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *essid)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_point *data = &wrqu->essid;
    unifi_AppValue sme_app_value;
    int r = 0;

    func_enter();
    unifi_trace(priv, UDBG2, "unifi_giwessid\n");
    CHECK_INITED(priv);

    sme_app_value.id = unifi_ConnectionInfoValue;
    r = sme_mgt_get_value(priv, &sme_app_value);

    if (r == 0) {
        data->length = sme_app_value.unifi_Value_union.connectionInfo.ssid.length;
        strncpy(essid,
                sme_app_value.unifi_Value_union.connectionInfo.ssid.ssid,
                data->length);
        data->flags = 1;            /* active */

        unifi_trace(priv, UDBG2, "unifi_giwessid: %.*s\n",
                    data->length, essid);
    }

    func_exit();

    return 0;
} /* unifi_giwessid() */


static int
unifi_siwrate(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_param *args = &wrqu->bitrate;
    unifi_AppValue sme_app_value;
    int r;

    func_enter();

    CHECK_INITED(priv);
    unifi_trace(priv, UDBG2, "unifi_siwrate\n");

    /*
    * If args->fixed == 0, value is max rate or -1 for best
    * If args->fixed == 1, value is rate to set or -1 for best
    * args->disabled and args->flags are not used in SIOCSIWRATE
    */

    /* Get, modify and set the MIB data */
    sme_app_value.id = unifi_MibConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_siwrate: Get unifi_MibConfigValue failed.\n");
        return r;
    }

    /* Default to auto rate algorithm */
    /* in 500Kbit/s, 0 means auto */
    sme_app_value.unifi_Value_union.mibConfig.unifiFixTxDataRate = 0;

    if (args->value != -1) {
        sme_app_value.unifi_Value_union.mibConfig.unifiFixTxDataRate = args->value / 500000;
    }

    /* 1 means rate is a maximum, 2 means rate is a set value */
    if (args->fixed == 1) {
        sme_app_value.unifi_Value_union.mibConfig.unifiFixMaxTxDataRate = 0;
    } else {
        sme_app_value.unifi_Value_union.mibConfig.unifiFixMaxTxDataRate = 1;
    }

    sme_app_value.id = unifi_MibConfigValue;
    r = sme_mgt_set_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_siwrate: Set unifi_MibConfigValue failed.\n");
        return r;
    }

    func_exit();

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
    unifi_AppValue sme_app_value;

    func_enter();
    unifi_trace(priv, UDBG2, "unifi_giwrate\n");
    CHECK_INITED(priv);

    flag = 0;
    bitrate = 0;

    sme_app_value.id = unifi_MibConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_giwrate: Get unifi_MibConfigValue failed.\n");
        return r;
    }

    bitrate = sme_app_value.unifi_Value_union.mibConfig.unifiFixTxDataRate;
    flag = sme_app_value.unifi_Value_union.mibConfig.unifiFixMaxTxDataRate;

    /* Used the value returned by the SME if MIB returns 0 */
    if (bitrate == 0) {
        sme_app_value.id = unifi_ConnectionStatsValue;
        r = sme_mgt_get_value(priv, &sme_app_value);
        /* Ignore errors, we may be disconnected */
        if (r == 0) {
            bitrate = sme_app_value.unifi_Value_union.connectionStats.unifiTxDataRate;
        }
    }

    args->value = bitrate * 500000;
    args->fixed = !flag;

    func_exit();

    return 0;
} /* unifi_giwrate() */


static int
unifi_siwrts(struct net_device *dev, struct iw_request_info *info,
             union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int val = wrqu->rts.value;
    int r = 0;
    unifi_AppValue sme_app_value;

    unifi_trace(priv, UDBG2, "unifi_siwrts\n");
    CHECK_INITED(priv);

    if (wrqu->rts.disabled) {
        val = 2347;
    }

    if ( (val < 0) || (val > 2347) )
    {
        return -EINVAL;
    }

    /* Get, modify and set the MIB data */
    sme_app_value.id = unifi_MibConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_siwrts: Get unifi_MibConfigValue failed.\n");
        return r;
    }
    sme_app_value.unifi_Value_union.mibConfig.dot11RtsThreshold = val;
    r = sme_mgt_set_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_siwrts: Set unifi_MibConfigValue failed.\n");
        return r;
    }

    return 0;
}


static int
unifi_giwrts(struct net_device *dev, struct iw_request_info *info,
             union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int r;
    int rts_thresh;
    unifi_AppValue sme_app_value;

    unifi_trace(priv, UDBG2, "unifi_giwrts\n");
    CHECK_INITED(priv);

    sme_app_value.id = unifi_MibConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_giwrts: Get unifi_MibConfigValue failed.\n");
        return r;
    }

    rts_thresh = sme_app_value.unifi_Value_union.mibConfig.dot11RtsThreshold;
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
    unifi_AppValue sme_app_value;

    unifi_trace(priv, UDBG2, "unifi_siwfrag\n");
    CHECK_INITED(priv);

    if (wrqu->frag.disabled)
        val = 2346;

    if ( (val < 256) || (val > 2347) )
        return -EINVAL;

    /* Get, modify and set the MIB data */
    sme_app_value.id = unifi_MibConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_siwfrag: Get unifi_MibConfigValue failed.\n");
        return r;
    }
    /* Fragmentation Threashold must be even */
    sme_app_value.unifi_Value_union.mibConfig.dot11FragmentationThreshold = (val & ~0x1);
    r = sme_mgt_set_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_siwfrag: Set unifi_MibConfigValue failed.\n");
        return r;
    }

    return 0;
}


static int
unifi_giwfrag(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int r;
    int frag_thresh;
    unifi_AppValue sme_app_value;

    unifi_trace(priv, UDBG2, "unifi_giwfrag\n");
    CHECK_INITED(priv);

    sme_app_value.id = unifi_MibConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_giwfrag: Get unifi_MibConfigValue failed.\n");
        return r;
    }

    frag_thresh = sme_app_value.unifi_Value_union.mibConfig.dot11FragmentationThreshold;

    /* Build the return structure */
    wrqu->frag.value = frag_thresh;
    wrqu->frag.disabled = (frag_thresh >= 2346);
    wrqu->frag.fixed = 1;

    return 0;
}


static int
unifi_siwencode(struct net_device *dev, struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_point *erq = &wrqu->encoding;
    int index;
    int rc = 0;
    int privacy = -1;
    unifi_Key sme_key;

    func_enter();
    unifi_trace(priv, UDBG2, "unifi_siwencode\n");

    CHECK_INITED(priv);

    /*
     * Key index is encoded in the flags.
     * 0 - use current default,
     * 1-4 - if a key value is given set that key
     *       if not use that key
     */
    index = (erq->flags & IW_ENCODE_INDEX);  /* key number, 1-4 */
    if ((index < 0) || (index > 4)) {
        unifi_error(priv, "unifi_siwencode: Request to set an invalid key (index:%d)", index);
        return -EINVAL;
    }

    /*
     * Basic checking: do we have a key to set ?
     * The IW_ENCODE_NOKEY flag is set when no key is present (only change flags),
     * but older versions rely on sending a key id 1-4.
     */
    if (erq->length > 0) {

        /* Check the size of the key */
        if ((erq->length > LARGE_KEY_SIZE) || (erq->length < SMALL_KEY_SIZE)) {
            unifi_error(priv, "unifi_siwencode: Request to set an invalid key (length:%d)",
                        erq->length);
            return -EINVAL;
        }

        /* Check the index (none (i.e. 0) means use current) */
        if ((index < 1) || (index > 4)) {
            /* If we do not have a previous key, use 1 as default */
            if (!priv->wep_tx_key_index) {
                priv->wep_tx_key_index = 1;
            }
            index = priv->wep_tx_key_index;
        }

        /* If we didn't have a key and a valid index is set, we want to remember it*/
        if (!priv->wep_tx_key_index) {
            priv->wep_tx_key_index = index;
        }

        unifi_trace(priv, UDBG1, "Tx key Index is %d\n", priv->wep_tx_key_index);

        privacy = 1;

        /* Check if the key is not marked as invalid */
        if ((erq->flags & IW_ENCODE_NOKEY) == 0) {

            unifi_trace(priv, UDBG1, "New %s key (len=%d, index=%d)\n",
                        (priv->wep_tx_key_index == index) ? "tx" : "",
                        erq->length, index);

            sme_key.wepTxKey = (priv->wep_tx_key_index == index);
            if (priv->wep_tx_key_index == index) {
                sme_key.keyType = unifi_PairwiseKey;
            } else {
                sme_key.keyType = unifi_GroupKey;
            }
            /* Key index is zero based in SME but 1 based in wext */
            sme_key.keyIndex = (index - 1);
            sme_key.keyLength = erq->length;
            sme_key.authenticator = 0;
            memset(sme_key.address.data, 0xFF, ETH_ALEN);
            memcpy(sme_key.key, extra, erq->length);

            rc = sme_mgt_key(priv, &sme_key, unifi_ListActionAdd);
            if (rc) {
                unifi_error(priv, "unifi_siwencode: Set key failed (%d)", rc);
                return convert_sme_error(rc);
            }

            /* Store the key to be reported by the SIOCGIWENCODE handler */
            priv->wep_keys[index - 1].len = erq->length;
            memcpy(priv->wep_keys[index - 1].key, extra, erq->length);
        }
    } else {
        /*
         * No additional key data, so it must be a request to change the
         * active key.
         */
        if (index != 0) {
            unifi_trace(priv, UDBG1, "Tx key Index is %d\n", index - 1);

            /* Store the index to be reported by the SIOCGIWENCODE handler */
            priv->wep_tx_key_index = index;

            sme_key.wepTxKey = 1;
            sme_key.keyType = unifi_PairwiseKey;

            /* Key index is zero based in SME but 1 based in wext */
            sme_key.keyIndex = (index - 1);
            sme_key.keyLength = 0;
            sme_key.authenticator = 0;

            rc = sme_mgt_key(priv, &sme_key, unifi_ListActionAdd);
            if (rc) {
                unifi_error(priv, "unifi_siwencode: Set key failed (%d)", rc);
                return convert_sme_error(rc);
            }

            /* Turn on encryption */
            privacy = 1;
        }
    }

    /* Read the flags */
    if (erq->flags & IW_ENCODE_DISABLED) {
        /* disable encryption */
        unifi_trace(priv, UDBG1, "disable WEP encryption\n");
        privacy = 0;

        priv->wep_tx_key_index = 0;

        unifi_trace(priv, UDBG1, "IW_ENCODE_DISABLED: unifi_80211AuthOpen\n");
        priv->connection_config.authModeMask = unifi_80211AuthOpen;
    }

    if (erq->flags & IW_ENCODE_RESTRICTED) {
        /* Use shared key auth */
        unifi_trace(priv, UDBG1, "IW_ENCODE_RESTRICTED: unifi_80211AuthShared\n");
        priv->connection_config.authModeMask = unifi_80211AuthShared;

        /* Turn on encryption */
        privacy = 1;
    }
    if (erq->flags & IW_ENCODE_OPEN) {
        unifi_trace(priv, UDBG1, "IW_ENCODE_OPEN: unifi_80211AuthOpen\n");
        priv->connection_config.authModeMask = unifi_80211AuthOpen;
    }

    /* Commit the changes to flags if needed */
    if (privacy != -1) {
        priv->connection_config.privacyMode = privacy ? unifi_80211PrivacyEnabled : unifi_80211PrivacyDisabled;
        priv->connection_config.encryptionModeMask = privacy ? (unifi_EncryptionCipherPairwiseWep40 |
                                                            unifi_EncryptionCipherPairwiseWep104 |
                                                            unifi_EncryptionCipherGroupWep40 |
                                                            unifi_EncryptionCipherGroupWep104) :
                                                           unifi_EncryptionCipherNone;
    }

    func_exit_r(rc);
    return convert_sme_error(rc);

} /* unifi_siwencode() */



static int
unifi_giwencode(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_point *erq = &wrqu->encoding;

    unifi_trace(priv, UDBG2, "unifi_giwencode\n");

    CHECK_INITED(priv);

    if (priv->connection_config.authModeMask == unifi_80211AuthShared) {
        erq->flags = IW_ENCODE_RESTRICTED;
    }
    else {
        if (priv->connection_config.privacyMode == unifi_80211PrivacyDisabled) {
            erq->flags = IW_ENCODE_DISABLED;
        } else {
            erq->flags = IW_ENCODE_OPEN;
        }
    }

    erq->length = 0;

    if (erq->flags != IW_ENCODE_DISABLED) {
        int index = priv->wep_tx_key_index;

        if ((index > 0) && (index <= NUM_WEPKEYS)) {
            erq->flags |= (index & IW_ENCODE_INDEX);
            erq->length = priv->wep_keys[index - 1].len;
            memcpy(extra, priv->wep_keys[index - 1].key, erq->length);
        } else {
            unifi_notice(priv, "unifi_giwencode: Surprise, do not have a valid key index (%d)\n",
                         index);
        }
    }

    return 0;
} /* unifi_giwencode() */


static int
unifi_siwpower(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    struct iw_param *args = &wrqu->power;
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int listen_interval, wake_for_dtim;
    int r = 0;
    unifi_AppValue sme_app_value;

    unifi_trace(priv, UDBG2, "unifi_siwpower\n");

    CHECK_INITED(priv);

    sme_app_value.id = unifi_PowerConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_siwpower: Get unifi_PowerConfigValue failed.\n");
        return r;
    }

    listen_interval = -1;
    wake_for_dtim = -1;
    if (args->disabled) {
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveLow;
    }
    else
    {
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveHigh;

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

    if (listen_interval > 0) {
        sme_app_value.unifi_Value_union.powerConfig.listenIntervalBeacons = listen_interval;
        unifi_trace(priv, UDBG4, "unifi_siwpower: new Listen Interval = %d.\n",
                    sme_app_value.unifi_Value_union.powerConfig.listenIntervalBeacons);
    }

    if (wake_for_dtim >= 0) {
        sme_app_value.unifi_Value_union.powerConfig.rxDtims = wake_for_dtim;
    }

    sme_app_value.id = unifi_PowerConfigValue;
    r = sme_mgt_set_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_siwpower: Set unifi_PowerConfigValue failed.\n");
        return r;
    }

    return 0;
} /* unifi_siwpower() */


static int
unifi_giwpower(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    struct iw_param *args = &wrqu->power;
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    unifi_AppValue sme_app_value;
    int r;

    unifi_trace(priv, UDBG2, "unifi_giwpower\n");

    CHECK_INITED(priv);

    args->flags = 0;

    sme_app_value.id = unifi_PowerConfigValue;
    r = sme_mgt_get_value(priv, &sme_app_value);
    if (r) {
        unifi_error(priv, "unifi_giwpower: Get unifi_PowerConfigValue failed.\n");
        return r;
    }

    unifi_trace(priv, UDBG4, "unifi_giwpower: mode=%d\n",
                sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel);

    args->disabled = (sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel == unifi_PowerSaveLow);
    if (args->disabled) {
        args->flags = 0;
        return 0;
    }

    args->value = sme_app_value.unifi_Value_union.powerConfig.listenIntervalBeacons * 1000;
    args->flags |= IW_POWER_PERIOD;

    if (sme_app_value.unifi_Value_union.powerConfig.rxDtims) {
        args->flags |= IW_POWER_ALL_R;
    } else {
        args->flags |= IW_POWER_UNICAST_R;
    }

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



static int
unifi_siwmlme(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_mlme *mlme = (struct iw_mlme *)extra;
    func_enter();

    unifi_trace(priv, UDBG2, "unifi_siwmlme\n");
    CHECK_INITED(priv);

    switch (mlme->cmd) {
      case IW_MLME_DEAUTH:
      case IW_MLME_DISASSOC:
        sme_mgt_disconnect(priv);
        break;
      default:
        func_exit_r(-EOPNOTSUPP);
        return -EOPNOTSUPP;
    }

    func_exit();
    return 0;
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
    unifi_trace(priv, UDBG2, "unifi_siwgenie\n");

    if ( priv->connection_config.mlmeAssociateReqInformationElements) {
        kfree( priv->connection_config.mlmeAssociateReqInformationElements);
    }
    priv->connection_config.mlmeAssociateReqInformationElementsLength = 0;
    priv->connection_config.mlmeAssociateReqInformationElements = NULL;

    len = wrqu->data.length;
    if (len == 0) {
        func_exit();
        return 0;
    }

    priv->connection_config.mlmeAssociateReqInformationElements = kmalloc(len, GFP_KERNEL);
    if (priv->connection_config.mlmeAssociateReqInformationElements == NULL) {
        func_exit();
        return -ENOMEM;
    }

    priv->connection_config.mlmeAssociateReqInformationElementsLength = len;
    memcpy( priv->connection_config.mlmeAssociateReqInformationElements, extra, len);

    func_exit();
    return 0;
} /* unifi_siwgenie() */


static int
unifi_giwgenie(struct net_device *dev, struct iw_request_info *info,
               union iwreq_data *wrqu, char *extra)
{
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    int len;

    func_enter();
    unifi_trace(priv, UDBG2, "unifi_giwgenie\n");

    len = priv->connection_config.mlmeAssociateReqInformationElementsLength;

    if (len == 0) {
        wrqu->data.length = 0;
        return 0;
    }

    if (wrqu->data.length < len) {
        return -E2BIG;
    }

    wrqu->data.length = len;
    memcpy(extra, priv->connection_config.mlmeAssociateReqInformationElements, len);

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
    CsrUint16 new_auth;

    func_enter();
    unifi_trace(priv, UDBG2, "unifi_siwauth\n");

    /*
     * This ioctl is safe to call even when UniFi is powered off.
     * wpa_supplicant calls it to test whether we support WPA.
     */

    switch (wrqu->param.flags & IW_AUTH_INDEX) {

      case IW_AUTH_WPA_ENABLED:
        unifi_trace(priv, UDBG1, "IW_AUTH_WPA_ENABLED: %d\n", wrqu->param.value);

        if (wrqu->param.value == 0) {
            unifi_trace(priv, UDBG5, "IW_AUTH_WPA_ENABLED: unifi_80211AuthOpen\n");
            priv->connection_config.authModeMask = unifi_80211AuthOpen;
        }
        break;

      case IW_AUTH_PRIVACY_INVOKED:
        unifi_trace(priv, UDBG1, "IW_AUTH_PRIVACY_INVOKED: %d\n", wrqu->param.value);

        priv->connection_config.privacyMode = wrqu->param.value ? unifi_80211PrivacyEnabled : unifi_80211PrivacyDisabled;
        if (wrqu->param.value == unifi_80211PrivacyDisabled)
        {
            priv->connection_config.encryptionModeMask = unifi_EncryptionCipherNone;
        }
        break;

      case IW_AUTH_80211_AUTH_ALG:
        /*
           IW_AUTH_ALG_OPEN_SYSTEM      0x00000001
           IW_AUTH_ALG_SHARED_KEY       0x00000002
           IW_AUTH_ALG_LEAP             0x00000004
        */
        new_auth = 0;
        if (wrqu->param.value & IW_AUTH_ALG_OPEN_SYSTEM) {
            unifi_trace(priv, UDBG1, "IW_AUTH_80211_AUTH_ALG: %d (IW_AUTH_ALG_OPEN_SYSTEM)\n", wrqu->param.value);
            new_auth |= unifi_80211AuthOpen;
        }
        if (wrqu->param.value & IW_AUTH_ALG_SHARED_KEY) {
            unifi_trace(priv, UDBG1, "IW_AUTH_80211_AUTH_ALG: %d (IW_AUTH_ALG_SHARED_KEY)\n", wrqu->param.value);
            new_auth |= unifi_80211AuthShared;
        }
        if (wrqu->param.value & IW_AUTH_ALG_LEAP) {
            /* Initial exchanges using open-system to set EAP */
            unifi_trace(priv, UDBG1, "IW_AUTH_80211_AUTH_ALG: %d (IW_AUTH_ALG_LEAP)\n", wrqu->param.value);
            new_auth |= unifi_8021xAuthOther1x;
        }
        if (new_auth == 0) {
            unifi_trace(priv, UDBG1, "IW_AUTH_80211_AUTH_ALG: invalid value %d\n",
                  wrqu->param.value);
            return -EINVAL;
        } else {
            priv->connection_config.authModeMask = new_auth;
        }
        break;

      case IW_AUTH_WPA_VERSION:
        unifi_trace(priv, UDBG1, "IW_AUTH_WPA_VERSION: %d\n", wrqu->param.value);
        priv->ignore_bssid_join = TRUE;
        /*
           IW_AUTH_WPA_VERSION_DISABLED 0x00000001
           IW_AUTH_WPA_VERSION_WPA      0x00000002
           IW_AUTH_WPA_VERSION_WPA2     0x00000004
        */

        if (!(wrqu->param.value & IW_AUTH_WPA_VERSION_DISABLED)) {

            priv->connection_config.authModeMask = unifi_80211AuthOpen;

            if (wrqu->param.value & IW_AUTH_WPA_VERSION_WPA) {
                unifi_trace(priv, UDBG4, "IW_AUTH_WPA_VERSION: WPA, WPA-PSK\n");
                priv->connection_config.authModeMask |= (unifi_8021xAuthWPA | unifi_8021xAuthWPAPSK);
            }
            if (wrqu->param.value & IW_AUTH_WPA_VERSION_WPA2) {
                unifi_trace(priv, UDBG4, "IW_AUTH_WPA_VERSION: WPA2, WPA2-PSK\n");
                priv->connection_config.authModeMask |= (unifi_8021xAuthWPA2 | unifi_8021xAuthWPA2PSK);
            }
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

        priv->connection_config.encryptionModeMask = unifi_EncryptionCipherNone;

        if (wrqu->param.value & IW_AUTH_CIPHER_WEP40) {
            priv->connection_config.encryptionModeMask |=
                    unifi_EncryptionCipherPairwiseWep40 | unifi_EncryptionCipherGroupWep40;
        }
        if (wrqu->param.value & IW_AUTH_CIPHER_WEP104) {
            priv->connection_config.encryptionModeMask |=
                    unifi_EncryptionCipherPairwiseWep104 | unifi_EncryptionCipherGroupWep104;
        }
        if (wrqu->param.value & IW_AUTH_CIPHER_TKIP) {
            priv->connection_config.encryptionModeMask |=
                    unifi_EncryptionCipherPairwiseTkip | unifi_EncryptionCipherGroupTkip;
        }
        if (wrqu->param.value & IW_AUTH_CIPHER_CCMP) {
            priv->connection_config.encryptionModeMask |=
                    unifi_EncryptionCipherPairwiseCcmp | unifi_EncryptionCipherGroupCcmp;
        }

        break;

      case IW_AUTH_CIPHER_GROUP:
        unifi_trace(priv, UDBG1, "IW_AUTH_CIPHER_GROUP: %d\n", wrqu->param.value);
        /*
         * Use the WPA version and the group cipher suite to set the permitted
         * group key in the MIB. f/w uses this value to validate WPA and RSN IEs
         * in the probe responses from the desired BSS(ID)
         */

        priv->connection_config.encryptionModeMask &= ~(unifi_EncryptionCipherGroupWep40 |
                                                              unifi_EncryptionCipherGroupWep104 |
                                                              unifi_EncryptionCipherGroupTkip |
                                                              unifi_EncryptionCipherGroupCcmp);
        if (wrqu->param.value & IW_AUTH_CIPHER_WEP40) {
            priv->connection_config.encryptionModeMask |= unifi_EncryptionCipherGroupWep40;
        }
        if (wrqu->param.value & IW_AUTH_CIPHER_WEP104) {
            priv->connection_config.encryptionModeMask |= unifi_EncryptionCipherGroupWep104;
        }
        if (wrqu->param.value & IW_AUTH_CIPHER_TKIP) {
            priv->connection_config.encryptionModeMask |= unifi_EncryptionCipherGroupTkip;
        }
        if (wrqu->param.value & IW_AUTH_CIPHER_CCMP) {
            priv->connection_config.encryptionModeMask |= unifi_EncryptionCipherGroupCcmp;
        }

        break;

      case IW_AUTH_KEY_MGMT:
        unifi_trace(priv, UDBG1, "IW_AUTH_KEY_MGMT: %d\n", wrqu->param.value);
        /*
           IW_AUTH_KEY_MGMT_802_1X 1
           IW_AUTH_KEY_MGMT_PSK    2
        */
        if (priv->connection_config.authModeMask & (unifi_8021xAuthWPA | unifi_8021xAuthWPAPSK)) {
            /* Check for explicitly set mode. */
            if (wrqu->param.value == IW_AUTH_KEY_MGMT_802_1X) {
                priv->connection_config.authModeMask &= ~unifi_8021xAuthWPAPSK;
            }
            if (wrqu->param.value == IW_AUTH_KEY_MGMT_PSK) {
                priv->connection_config.authModeMask &= ~unifi_8021xAuthWPA;
            }
            unifi_trace(priv, UDBG5, "IW_AUTH_KEY_MGMT: WPA: %d\n",
                        priv->connection_config.authModeMask);
        }
        if (priv->connection_config.authModeMask & (unifi_8021xAuthWPA2 | unifi_8021xAuthWPA2PSK)) {
            /* Check for explicitly set mode. */
            if (wrqu->param.value == IW_AUTH_KEY_MGMT_802_1X) {
                priv->connection_config.authModeMask &= ~unifi_8021xAuthWPA2PSK;
            }
            if (wrqu->param.value == IW_AUTH_KEY_MGMT_PSK) {
                priv->connection_config.authModeMask &= ~unifi_8021xAuthWPA2;
            }
            unifi_trace(priv, UDBG5, "IW_AUTH_KEY_MGMT: WPA2: %d\n",
                        priv->connection_config.authModeMask);
        }

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
        /* TODO: Anything to do here? */
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

    unifi_trace(priv, UDBG2, "authModeMask = %d", priv->connection_config.authModeMask);
    func_exit();

    return 0;
} /* _unifi_siwauth() */


static int
unifi_siwauth(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    int err = 0;

    err = _unifi_siwauth(dev, info, wrqu, extra);

    return err;
} /* unifi_siwauth() */


static int
unifi_giwauth(struct net_device *dev, struct iw_request_info *info,
              union iwreq_data *wrqu, char *extra)
{
    unifi_trace(NULL, UDBG2, "unifi_giwauth\n");
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
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_encode_ext *ext = (struct iw_encode_ext *)extra;
    int r = 0;
    unsigned char *keydata;
    unsigned char tkip_key[32];
    int keyid;
    unsigned char *a = (unsigned char *)ext->addr.sa_data;
    unifi_Key sme_key;
    unifi_KeyType key_type;

    func_enter();

    CHECK_INITED(priv);

    unifi_trace(priv, UDBG1, "siwencodeext: flags=0x%X, alg=%d, ext_flags=0x%X, len=%d, index=%d,\n",
                wrqu->encoding.flags, ext->alg, ext->ext_flags,
                ext->key_len, (wrqu->encoding.flags & IW_ENCODE_INDEX));
    unifi_trace(priv, UDBG3, "              addr=%02X:%02X:%02X:%02X:%02X:%02X\n",
                a[0], a[1], a[2], a[3], a[4], a[5]);

    memset(&sme_key, 0, sizeof(sme_key));

    if ((ext->key_len == 0) && (ext->ext_flags & IW_ENCODE_EXT_SET_TX_KEY)) {
        /* This means use a different key (given by key_idx) for Tx. */
        /* NYI */
        unifi_trace(priv, UDBG1, KERN_ERR "unifi_siwencodeext: NYI should change tx key id here!!\n");
        return -ENOTSUPP;
    }

    keydata = (unsigned char *)(ext + 1);
    keyid = (wrqu->encoding.flags & IW_ENCODE_INDEX);

    /*
     * Check for request to delete keys for an address.
     */
    /* Pick out request for no privacy. */
    if (ext->alg == IW_ENCODE_ALG_NONE) {

        unifi_trace(priv, UDBG1, "Deleting %s key %d\n",
              (ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY) ? "GROUP" : "PAIRWISE",
              keyid);

        if (ext->ext_flags & IW_ENCODE_EXT_GROUP_KEY) {
            sme_key.keyType = unifi_GroupKey;
        } else {
            sme_key.keyType = unifi_PairwiseKey;
        }
        sme_key.keyIndex = (keyid - 1);
        sme_key.keyLength = 0;
        sme_key.authenticator = 0;
        memcpy(sme_key.address.data, a, ETH_ALEN);

        r = sme_mgt_key(priv, &sme_key, unifi_ListActionRemove);
        if (r) {
            unifi_error(priv, "Delete key request was rejected with result %d", r);
            return convert_sme_error(r);
        }

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

        unifi_trace(priv, UDBG1, "Setting WEP key %d tx:%d\n",
                    keyid, ext->ext_flags & IW_ENCODE_EXT_SET_TX_KEY);

        if (ext->ext_flags & IW_ENCODE_EXT_SET_TX_KEY) {
            sme_key.wepTxKey = TRUE;
            sme_key.keyType = unifi_PairwiseKey;
        } else {
            sme_key.wepTxKey = FALSE;
            sme_key.keyType = unifi_GroupKey;
        }
        sme_key.keyIndex = (keyid - 1);
        sme_key.keyLength = ext->key_len;
        sme_key.authenticator = 0;
        memset(sme_key.address.data, 0xFF, ETH_ALEN);
        memcpy(sme_key.key, keydata, ext->key_len);

        r = sme_mgt_key(priv, &sme_key, unifi_ListActionAdd);
        if (r) {
            unifi_error(priv, "siwencodeext: Set key failed (%d)", r);
            return convert_sme_error(r);
        }

        return 0;
    }

    /*
     *
     * If we reach here, we are dealing with a WPA/WPA2 key
     *
     */
    if (ext->key_len > 32) {
        return -EINVAL;
    }

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
            unifi_GroupKey : /* Group Key */
            unifi_PairwiseKey; /* Pairwise Key */

    sme_key.keyType = key_type;
    sme_key.keyIndex = (keyid - 1);
    sme_key.keyLength = ext->key_len;
    sme_key.authenticator = 0;
    memcpy(sme_key.address.data, ext->addr.sa_data, ETH_ALEN);
    if (ext->ext_flags & IW_ENCODE_EXT_RX_SEQ_VALID) {

         unifi_trace(priv, UDBG5, "RSC first 6 bytes = %02X:%02X:%02X:%02X:%02X:%02X\n",
                    ext->rx_seq[0], ext->rx_seq[1], ext->rx_seq[2], ext->rx_seq[3], ext->rx_seq[4], ext->rx_seq[5]);

        /* memcpy((u8*)(&sme_key.keyRsc), ext->rx_seq, 8); */
        sme_key.keyRsc[0] = ext->rx_seq[1] << 8 | ext->rx_seq[0];
        sme_key.keyRsc[1] = ext->rx_seq[3] << 8 | ext->rx_seq[2];
        sme_key.keyRsc[2] = ext->rx_seq[5] << 8 | ext->rx_seq[4];
        sme_key.keyRsc[3] = ext->rx_seq[7] << 8 | ext->rx_seq[6];
    }

    memcpy(sme_key.key, keydata, ext->key_len);

    r = sme_mgt_key(priv, &sme_key, unifi_ListActionAdd);
    if (r) {
        unifi_error(priv, "SETKEYS request was rejected with result %d\n", r);
        return convert_sme_error(r);
    }

    func_exit();
    return r;
} /* _unifi_siwencodeext() */


static int
unifi_siwencodeext(struct net_device *dev, struct iw_request_info *info,
                   union iwreq_data *wrqu, char *extra)
{
    int err = 0;

    err = _unifi_siwencodeext(dev, info, wrqu, extra);

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
    unifi_priv_t *priv = (unifi_priv_t *)netdev_priv(dev);
    struct iw_pmksa *pmksa = (struct iw_pmksa *)extra;
    unifi_Status r = 0;
    unifi_PmkidList pmkid_list;
    unifi_Pmkid pmkid;
    unifi_ListAction action;

    CHECK_INITED(priv);

    unifi_trace(priv, UDBG1, "SIWPMKSA: cmd %d, %02x:%02x:%02x:%02x:%02x:%02x\n",
                pmksa->cmd,
                pmksa->bssid.sa_data[0],
                pmksa->bssid.sa_data[1],
                pmksa->bssid.sa_data[2],
                pmksa->bssid.sa_data[3],
                pmksa->bssid.sa_data[4],
                pmksa->bssid.sa_data[5]);

    pmkid_list.pmkids = NULL;
    switch (pmksa->cmd) {
      case IW_PMKSA_ADD:
        pmkid_list.pmkids = &pmkid;
        action = unifi_ListActionAdd;
        pmkid_list.numElements = 1;
        memcpy(pmkid.bssid.data, pmksa->bssid.sa_data, ETH_ALEN);
        memcpy(pmkid.pmkid, pmksa->pmkid, UNIFI_PMKID_KEY_SIZE);
        break;
      case IW_PMKSA_REMOVE:
        pmkid_list.pmkids = &pmkid;
        action = unifi_ListActionRemove;
        pmkid_list.numElements = 1;
        memcpy(pmkid.bssid.data, pmksa->bssid.sa_data, ETH_ALEN);
        memcpy(pmkid.pmkid, pmksa->pmkid, UNIFI_PMKID_KEY_SIZE);
        break;
      case IW_PMKSA_FLUSH:
        /* Replace current PMKID's with an empty list */
        pmkid_list.numElements = 0;
        action = unifi_ListActionFlush;
        break;
      default:
        unifi_notice(priv, "SIWPMKSA: Unknown command (0x%x)\n", pmksa->cmd);
        return -EINVAL;
    }

    /* Set the Value the pmkid's will have 1 added OR 1 removed OR be cleared at this point */
    r = sme_mgt_pmkid(priv, action, &pmkid_list);
    if (r) {
        unifi_error(priv, "SIWPMKSA: Set PMKID's Failed.\n");
    }

    return r;

} /* unifi_siwpmksa() */


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

    return &priv->wext_wireless_stats;
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
    { SIOCIWS80211RELOADDEFAULTSPRIV, IW_PRIV_TYPE_NONE,
    IW_PRIV_TYPE_NONE, "iwprivsdefs" },
    { SIOCIWSSMEDEBUGPRIV, IW_PRIV_TYPE_CHAR | IWPRIV_SME_DEBUG_MAX_STRING, IW_PRIV_TYPE_NONE, "iwprivssmedebug" },
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    { SIOCIWSCONFWAPIPRIV, IW_PRIV_TYPE_BYTE | IW_PRIV_SIZE_FIXED | 1,
    IW_PRIV_TYPE_NONE, "iwprivsconfwapi" },
    { SIOCIWSWAPIKEYPRIV, IW_PRIV_TYPE_BYTE | IW_PRIV_SIZE_FIXED | sizeof(unifiio_wapi_key_t),
    IW_PRIV_TYPE_NONE, "iwprivswpikey" },
#endif
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
//    (iw_handler) NULL,                      /* SIOCSIWPRIV */
 (iw_handler) unifi_siwpriv,             /* SIOCSIWPRIV */ /* Ices add */
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
    iwprivs80211ps,                 /* SIOCIWFIRSTPRIV */
    iwprivg80211ps,                 /* SIOCIWFIRSTPRIV + 1 */
    iwprivsdefs,                    /* SIOCIWFIRSTPRIV + 2 */
    (iw_handler) NULL,
#ifdef CSR_WIFI_SECURITY_WAPI_ENABLE
    iwprivsconfwapi,                /* SIOCIWFIRSTPRIV + 4 */
    (iw_handler) NULL,              /* SIOCIWFIRSTPRIV + 5 */
    iwprivswpikey,                  /* SIOCIWFIRSTPRIV + 6 */
#else
    (iw_handler) NULL,
    (iw_handler) NULL,
    (iw_handler) NULL,
#endif
    (iw_handler) NULL,
    iwprivssmedebug,                /* SIOCIWFIRSTPRIV + 8 */
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


