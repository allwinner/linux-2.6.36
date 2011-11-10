/*
 * ***************************************************************************
 * FILE:     indications.c
 * 
 * PURPOSE:
 *      Callbacks to process signals sent to us by the UniFi chip.
 *      
 *      This file is linux-specific.
 * 
 * Copyright (C) 2005-2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */
#include "driver/unifi.h"
#include "unifi_priv.h"



/*
 * ---------------------------------------------------------------------------
 *  uf_ma_unitdata_status_ind
 *
 *      This function is called from the interrupt handler when a
 *      MA-UNITDATA-STATUS.indication is received from UniFi.
 *      This signal acknowledges transmission of a packet sent with
 *      MA-UNITDATA.request.
 * 
 *  Arguments:
 *      priv          OS private context pointer.
 *      ind             Pointer to signal structure received from UniFi.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
uf_ma_unitdata_status_ind(unifi_priv_t *priv,
                          const CSR_MA_UNITDATA_CONFIRM *ind)
{

#if 0
    printk("MA-UNITDATA.sts: D=%02X:%02X:%02X:%02X:%02X:%02X S=%02X:%02X:%02X:%02X:%02X:%02X\n",
           ind->Da.x[0], ind->Da.x[1], ind->Da.x[2], 
           ind->Da.x[3], ind->Da.x[4], ind->Da.x[5], 
           ind->Sa.x[0], ind->Sa.x[1], ind->Sa.x[2], 
           ind->Sa.x[3], ind->Sa.x[4], ind->Sa.x[5]);
    printk("                 TransStatus=%d, Pri=%d, ServClass=%d, Tag=0x%08X\n",
           ind->TransmissionStatus,
           ind->ProvidedPriority,
           ind->ProvidedServiceClass,
           ind->ProvidedHostTag);
#endif

    /* update linux network stats */
    if (ind->TransmissionStatus) {
        /* failed */
        switch (ind->TransmissionStatus) {
          case 1:   /* retry limit */
#ifdef CSR_NATIVE_LINUX
            priv->wext_conf.wireless_stats.discard.retries++;
#endif
#if (defined CSR_SUPPORT_SME) && (defined CSR_SUPPORT_WEXT)
            priv->wext_wireless_stats.discard.retries++;
#endif /* CSR_SUPPORT_SME && CSR_SUPPORT_WEXT*/

            /* Fall through - report in net dev stats too */

          case 2:   /* tx_lifetime */
          case 11:  /* tx EDCA timeout */
          case 12:  /* block ack timeout */
            priv->stats.tx_aborted_errors++;
            /* Fall through - report it as generic error too */

          case 4:   /* excessive data length */
          case 5:   /* non-null source routing */
          case 6:   /* unsupported priority */
          case 7:   /* unavailable priority */
          case 8:   /* unsupported service class */
          case 9:   /* unavailable service class */
          case 10:  /* unavailable key mapping */
            priv->stats.tx_errors++;
            break;
          case 3:   /* no_bss */
            priv->stats.tx_carrier_errors++;
            priv->stats.tx_errors++;
            break;

          default:
            break;
        }
    }

    /* tx_packets and tx_bytes are counted on send. */

} /* uf_ma_unitdata_status_ind() */



