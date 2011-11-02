/*
 * ---------------------------------------------------------------------------
 * FILE:     wext_events.c
 * 
 * PURPOSE:
 *      Code to generate iwevents.
 *      
 * Copyright (C) 2006-2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include <linux/types.h>
#include <linux/etherdevice.h>
#include <linux/if_arp.h>
#include "driver/unifi.h"
#include "unifi_priv.h"



/*
 * ---------------------------------------------------------------------------
 *  wext_send_assoc_event
 *
 *      Send wireless-extension events up to userland to announce
 *      successful association with an AP.
 * 
 *  Arguments:
 *      priv                    Pointer to driver context.
 *      bssid                   MAC address of AP we associated with
 *      req_ie, req_ie_len      IEs in the original request
 *      resp_ie, resp_ie_len    IEs in the response
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      This is sent on first successful association, and again if we
 *      roam to another AP.
 * ---------------------------------------------------------------------------
 */
void
wext_send_assoc_event(unifi_priv_t *priv, unsigned char *bssid,
                      unsigned char *req_ie, int req_ie_len,
                      unsigned char *resp_ie, int resp_ie_len,
                      unsigned char *scan_ie, unsigned int scan_ie_len)
{
#if WIRELESS_EXT > 17
    union iwreq_data wrqu;

#if 0
    printk("wext_send_assoc_event: %p, %p, %p, %d, %p, %d\n",
           priv, bssid,
           req_ie, req_ie_len,
           resp_ie, resp_ie_len);
#endif

    if (req_ie_len == 0) req_ie = NULL;
    wrqu.data.length = req_ie_len;
    wrqu.data.flags = 0;
    wireless_send_event(priv->netdev, IWEVASSOCREQIE, &wrqu, req_ie);

    if (resp_ie_len == 0) resp_ie = NULL;
    wrqu.data.length = resp_ie_len;
    wrqu.data.flags = 0;
    wireless_send_event(priv->netdev, IWEVASSOCRESPIE, &wrqu, resp_ie);

    if (scan_ie_len > 0) {
        wrqu.data.length = scan_ie_len;
        wrqu.data.flags = 0;
        wireless_send_event(priv->netdev, IWEVGENIE, &wrqu, scan_ie);
    }

    memcpy(&wrqu.ap_addr.sa_data, bssid, ETH_ALEN);
    wireless_send_event(priv->netdev, SIOCGIWAP, &wrqu, NULL);
#endif
} /* wext_send_assoc_event() */



/*
 * ---------------------------------------------------------------------------
 *  wext_send_disassoc_event
 *
 *      Send a wireless-extension event up to userland to announce
 *      that we disassociated from an AP.
 * 
 *  Arguments:
 *      priv                    Pointer to driver context.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      The semantics of wpa_supplicant (the userland SME application) are
 *      that a SIOCGIWAP event with MAC address of all zero means
 *      disassociate.
 * ---------------------------------------------------------------------------
 */
void
wext_send_disassoc_event(unifi_priv_t *priv)
{
#if WIRELESS_EXT > 17
    union iwreq_data wrqu;

    memset(wrqu.ap_addr.sa_data, 0, ETH_ALEN);
    wireless_send_event(priv->netdev, SIOCGIWAP, &wrqu, NULL);
#endif
} /* wext_send_disassoc_event() */



/*
 * ---------------------------------------------------------------------------
 *  wext_send_scan_results_event
 *
 *      Send wireless-extension events up to userland to announce
 *      completion of a scan.
 * 
 *  Arguments:
 *      priv                    Pointer to driver context.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      This doesn't actually report the results, they are retrieved
 *      using the SIOCGIWSCAN ioctl command.
 * ---------------------------------------------------------------------------
 */
void
wext_send_scan_results_event(unifi_priv_t *priv)
{
#if WIRELESS_EXT > 17
    union iwreq_data wrqu;

    wrqu.data.length = 0;
    wrqu.data.flags = 0;
    wireless_send_event(priv->netdev, SIOCGIWSCAN, &wrqu, NULL);

#endif
} /* wext_send_scan_results_event() */



/*
 * ---------------------------------------------------------------------------
 *  wext_send_michaelmicfailure_event
 *
 *      Send wireless-extension events up to userland to announce
 *      completion of a scan.
 * 
 *  Arguments:
 *      priv            Pointer to driver context.
 *      count, macaddr, key_type, key_idx, tsc
 *                      Parameters from report from UniFi.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
#if WIRELESS_EXT >= 18
static inline void
_send_michaelmicfailure_event(struct net_device *dev,
                              int count, const unsigned char *macaddr,
                              int key_type, int key_idx,
                              unsigned char *tsc)
{
    union iwreq_data wrqu;
    struct iw_michaelmicfailure mmf;

    /* TODO: needed parameters: count, keyid, key type, TSC */
    memset(&mmf, 0, sizeof(mmf));

    mmf.flags = key_idx & IW_MICFAILURE_KEY_ID;
    if (key_type == CSR_GROUP) {
        mmf.flags |= IW_MICFAILURE_GROUP;
    } else {
        mmf.flags |= IW_MICFAILURE_PAIRWISE;
    }
    mmf.flags |= ((count << 5) & IW_MICFAILURE_COUNT);

    mmf.src_addr.sa_family = ARPHRD_ETHER;
    memcpy(mmf.src_addr.sa_data, macaddr, ETH_ALEN);

    memcpy(mmf.tsc, tsc, IW_ENCODE_SEQ_MAX_SIZE);

    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.length = sizeof(mmf);

    wireless_send_event(dev, IWEVMICHAELMICFAILURE, &wrqu, (char *)&mmf);
}
#elif WIRELESS_EXT >= 15
static inline void
_send_michaelmicfailure_event(struct net_device *dev,
                              int count, const unsigned char *macaddr,
                              int key_type, int key_idx,
                              unsigned char *tsc)
{
    union iwreq_data wrqu;
    char buf[128];

    /* TODO: needed parameters: count, keyid, key type, TSC */
    sprintf(buf,
            "MLME-MICHAELMICFAILURE.indication(keyid=%d %scast addr=%02x:%02x:%02x:%02x:%02x:%02x)",
            key_idx, (key_type == CSR_GROUP) ? "broad" : "uni",
            macaddr[0], macaddr[1], macaddr[2],
            macaddr[3], macaddr[4], macaddr[5]);
    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.length = strlen(buf);
    wireless_send_event(dev, IWEVCUSTOM, &wrqu, buf);
}
#else /* WIRELESS_EXT >= 15 */
static inline void
_send_michaelmicfailure_event(struct net_device *dev,
                              int count, const unsigned char *macaddr,
                              int key_type, int key_idx,
                              unsigned char *tsc)
{
    /* Not supported before WEXT 15 */
}
#endif /* WIRELESS_EXT >= 15 */


void
wext_send_michaelmicfailure_event(unifi_priv_t *priv,
                                  const CSR_MLME_MICHAELMICFAILURE_INDICATION *ind)
{
    unsigned char tsc[8];

    /* tsc in IWEVMICHAELMICFAILURE event is defined to be LSB first */
    tsc[0] = ind->Tsc[0] & 0xFF;
    tsc[1] = (ind->Tsc[0] >>  8) & 0xFF;
    tsc[2] = (ind->Tsc[1]) & 0xFF;
    tsc[3] = (ind->Tsc[1] >> 8) & 0xFF;
    tsc[4] = (ind->Tsc[2]) & 0xFF;
    tsc[5] = (ind->Tsc[2] >> 8) & 0xFF;
    tsc[6] = (ind->Tsc[3]) & 0xFF;
    tsc[7] = (ind->Tsc[3] >> 8) & 0xFF;
    _send_michaelmicfailure_event(priv->netdev,
                                  ind->Count,
                                  ind->Address.x,
                                  ind->KeyType,
                                  ind->KeyId,
                                  tsc);
} /* wext_send_michaelmicfailure_event() */


/*
 * Send a custom WEXT event to say we have completed initialisation
 * and are now ready for WEXT ioctls. Used by Android wpa_supplicant.
 */
void
wext_send_started_event(unifi_priv_t *priv)
{
#if WIRELESS_EXT > 17
    union iwreq_data wrqu;
    char data[] = "STARTED";

    wrqu.data.length = sizeof(data);
    wrqu.data.flags = 0;
    wireless_send_event(priv->netdev, IWEVCUSTOM, &wrqu, data);
#endif
} /* wext_send_started_event() */

