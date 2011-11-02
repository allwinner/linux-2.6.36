/*
 * ***************************************************************************
 *
 *  FILE:     scan.c
 * 
 *  PURPOSE:
 *      Handle scans.
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


static int aggregate_info_elements(const unsigned char *old_ie_ptr,
                                   int old_ie_len,
                                   const unsigned char *new_ie_ptr,
                                   int new_ie_len,
                                   unsigned char *iebuf, int maxlen);

#ifdef DEBUG_AGGREGATE
static void decode_info_elements(const unsigned char *info, int infolen);
#endif /* DEBUG_AGGREGATE */



/*
 * ---------------------------------------------------------------------------
 *  unifi_get_scan_report
 *
 *      Access function to retrieve the details of one station found
 *      in the last scan.
 * 
 *  Arguments:
 *      priv            Pointer to driver context.
 *      index           The index of the scan data to retrieve.
 *
 *  Returns:
 *      Pointer to a scan info struct, or
 *      NULL if the entry for <index> is empty.
 *
 *  Notes:
 *      This is typically called repeatedly with incrementing values index,
 *      starting at zero, until it returns NULL.
 * ---------------------------------------------------------------------------
 */
scan_info_t *
unifi_get_scan_report(unifi_priv_t *priv, int index)
{
    if ((index >= 0) && (index < priv->wext_conf.num_scan_info)) {
        return &(priv->wext_conf.scan_list[index]);
    }

    return NULL;
} /* unifi_get_scan_report() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_clear_scan_table
 *
 *      Called from mlme_scan_request when the scan should scrap the old table.
 * 
 *  Arguments:
 *      priv            Pointer to driver context.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
unifi_clear_scan_table(unifi_priv_t *priv)
{
    int i;

    for (i = 0; i < UNIFI_MAX_SCANS; i++)
    {
        scan_info_t *si;

        si = &priv->wext_conf.scan_list[i];

        if (si->info_elems) {
            CsrPfree(si->info_elems);
        }
        si->info_elem_length = 0;
        si->info_elems = NULL;
    }

    priv->wext_conf.num_scan_info = 0;
} /* unifi_clear_scan_table() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_scan_indication_handler
 *
 *      Handle a MLME_SCAN_INDICATION from UniFi.
 *      Adds the scan results to the scan list.
 *      Any existing entry with the same MAC address is replaced, however any
 *      InformationElements in the old entry that are not present in the new
 *      are retained.
 * 
 *  Arguments:
 *      priv            Pointer to driver context.
 *      msg     Pointer to the MLME_SCAN_INDICATION message.
 *
 *  Returns:
 *      None.
 *
 *  Notes:
 *      The entries in the scan list consist of the MLME_SCAN_INDICATION struct
 *      with the InformationElements appended. The length of the I-E is given
 *      by the DataRef.
 * ---------------------------------------------------------------------------
 */
void
unifi_scan_indication_handler(unifi_priv_t *priv,
                              const CSR_MLME_SCAN_INDICATION *msg,
                              const unsigned char *extra,
                              unsigned int len)
{
    int i;
    unsigned char iebuf[2048];
    const unsigned char *agg_ie_ptr;
    int agg_ie_len;
    const unsigned char *new_ie_ptr;
    int new_ie_len;

    /* Point to the bulk data containing Info Elems for the new scan */
    new_ie_ptr = extra;
    new_ie_len = len;

    /* Look for any matching entry already on the list. */
    for (i = 0; i < priv->wext_conf.num_scan_info; i++)
    {
        scan_info_t *tmp;
        tmp = &priv->wext_conf.scan_list[i];
#ifdef DEBUG_AGGREGATE
        printk("Checking MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
               tmp->msi.Bssid.x[0], tmp->msi.Bssid.x[1], tmp->msi.Bssid.x[2],
               tmp->msi.Bssid.x[3], tmp->msi.Bssid.x[4], tmp->msi.Bssid.x[5]);
#endif /* DEBUG_AGGREGATE */
        if (memcmp((void*)tmp->msi.Bssid.x, (void*)msg->Bssid.x, sizeof(CSR_MACADDRESS)) == 0)
        {
#ifdef DEBUG_AGGREGATE
            printk(" match!!\n");
#endif /* DEBUG_AGGREGATE */
            break;
        }
    }

    /* 
     * 'i' either the index of a matching entry in the list,
     * or the index of the next free entry at the end of the list.
     */
    if (i < UNIFI_MAX_SCANS) {
        scan_info_t *si;

        si = &priv->wext_conf.scan_list[i];
        memcpy((void*)&si->msi, (void*)msg, sizeof(*msg));

        /* Fallback to the new info */
        agg_ie_ptr = new_ie_ptr;
        agg_ie_len = new_ie_len;

        /* If there was old info, combine it with the new */
        if (si->info_elem_length > 0) {
            const unsigned char *old_ie_ptr;
            int old_ie_len;

            /* Build an aggregate list of IEs */
            old_ie_ptr = si->info_elems;
            old_ie_len = si->info_elem_length;

            agg_ie_len = aggregate_info_elements(old_ie_ptr, old_ie_len, 
                                                 new_ie_ptr, new_ie_len, 
                                                 iebuf, sizeof(iebuf));
            agg_ie_ptr = iebuf;

            /* Free old IE */
            CsrPfree(si->info_elems);
            si->info_elems = NULL;
            si->info_elem_length = 0;
        }

        /* Allocate space for the new IE and copy it */
        if (agg_ie_len > 0) {
            si->info_elems = (unsigned char *)
                CsrPmalloc(agg_ie_len);
            if (si->info_elems) {
                memcpy(si->info_elems, agg_ie_ptr, agg_ie_len);
                si->info_elem_length = agg_ie_len;
            }
        }

        /* If this is a new entry, increment the count */
        if (i == priv->wext_conf.num_scan_info) {
            priv->wext_conf.num_scan_info++;
        }
    }

} /* unifi_scan_indication_handler() */



/*
 * ---------------------------------------------------------------------------
 *
 * aggregate_info_elements
 *
 *      Combine the Information Element vector from two scan reports for the
 *      same station. The new report supercedes the old report, but any
 *      elements present in the old report that are not overwritten are
 *      retained.
 *
 * Arguments:
 *      old_ie_ptr, old_ie_len
 *              Pointer to and length of info elements of older scan
 *      new_ie_ptr, new_ie_len
 *              Pointer to and length of info elements of newer scan
 *      iebuf, maxlen
 *              Pointer to buffer for the new aggregated info elements.
 *
 * Returns:
 *      New length of aggregate info element vector.
 *  
 * Notes:
 *      This code assumes that the Info Elems are in order of increasing
 *      ID number.
 *  
 *      !!!!!
 *      There are some special conditions to watch out for:
 *       - Some vendor-specific ids might legitimately occur more than once,
 *         e.g 221 WPA. We ignore this fact for now, but may need to handle
 *         those IEs differently in the future.
 *       - any wildcard SSIDs (i.e. zero-length, also called broadcast SSID)
 *         should be over-written by a non-wildcard SSID if present.
 * ---------------------------------------------------------------------------
 */
static int
aggregate_info_elements(const unsigned char *old_ie_ptr, int old_ie_len, 
                        const unsigned char *new_ie_ptr, int new_ie_len, 
                        unsigned char *iebuf, int maxlen)
{
    unsigned char *agg_ie_ptr;
    int id;
    const unsigned char *iptr;
    const unsigned char *old_ssid_ie_ptr;
    const unsigned char *tmp_old_ie_ptr;
    const unsigned char *tmp_new_ie_ptr;
    int tmp_old_ie_len, tmp_new_ie_len, agg_ie_len, agg_ssid_ie_len, old_ssid_ie_len;


#ifdef DEBUG_AGGREGATE
    printk("Agg: old, len=%d\n", old_ie_len);
    decode_info_elements(old_ie_ptr, old_ie_len);

    printk("Agg: new, len=%d\n", new_ie_len);
    decode_info_elements(new_ie_ptr, new_ie_len);
#endif /* DEBUG_AGGREGATE */

    /* Pointer to the aggregation buffer */
    agg_ie_ptr = iebuf;
    /* Pointer to the old buffer */
    tmp_old_ie_ptr = old_ie_ptr;
    /* Pointer to the new buffer */
    tmp_new_ie_ptr = new_ie_ptr;
    /* length of old buffer */
    tmp_old_ie_len = old_ie_len;
    /* length of new buffer */
    tmp_new_ie_len = new_ie_len;

    /* length of aggregation buffer */
    agg_ie_len = 0;
    /* length of ssid ie in aggregation buffer */
    agg_ssid_ie_len = 0;

    /* We assume that the SSID is the first IE */
    old_ssid_ie_ptr = unifi_find_info_element(0, old_ie_ptr, old_ie_len);
    if (old_ssid_ie_ptr) {
        old_ssid_ie_len = old_ssid_ie_ptr[1] + 2;
    } else {
        old_ssid_ie_len = 0;
    }

    /*
     * update the temp pointer to the old buffer
     * to point to IEs after the SSID IE
     */
    tmp_old_ie_ptr += old_ssid_ie_len;

    /* 
     * Handle SSID (ID 0) specially.
     *
     * An SSID with non-zero length in new IE is always taken.
     * An SSID with non-zero length in old IE beats zero-length in new IE.
     */
    iptr = unifi_find_info_element(0, new_ie_ptr, new_ie_len);
    if (iptr) {
        /* 
         * There is an SSID in the new IE. Take it, but if it has zero length
         * check the old IE for a SSID with non-zero length.
         */
        if (iptr[1] == 0) {
            /* 
             * This is a zero-length SSID, see if there is a better
             * SSID in the old IE.
             */
            if (old_ssid_ie_ptr && (old_ssid_ie_ptr[1] > 0)) {
                iptr = old_ssid_ie_ptr;
            }
        }

        /*
         * update the temp pointer to the new buffer
         * to point to IEs after the SSID IE
         */
        tmp_new_ie_ptr += iptr[1] + 2;
        tmp_new_ie_len -= iptr[1] + 2;
    } else {
        /* No SSID in the new IE, fall back to the value in old IE. */
        iptr = old_ssid_ie_ptr;
    }
    if (iptr) {
        /* Copy the info elem to the aggregate buffer */
        int ilen = iptr[1] + 2;
        memcpy(agg_ie_ptr, iptr, ilen);
        /* update the aggregation pointer to point after the SSID IE */
        agg_ie_ptr += ilen;
        /* store the size of SSID IE in the aggregation buffer*/
        agg_ssid_ie_len = ilen;
    }

    /* Copy the old info elem to the aggregate buffer */
    if (maxlen > old_ie_len) {
        memcpy(agg_ie_ptr, tmp_old_ie_ptr, old_ie_len - old_ssid_ie_len);
        agg_ie_len = old_ie_len - old_ssid_ie_len;
    } else {
        /* We do not have enough space, copy as much as we can */
        memcpy(agg_ie_ptr, tmp_old_ie_ptr, maxlen - old_ssid_ie_len);
        return maxlen;
    }

    /*
     * Parse new ie buffer and search for a matching ie in the aggregation buffer
     */
    while (tmp_new_ie_len > 0)
    {
        const unsigned char *tmp;
        int ilen;
        int fiematch = 0;
        int tmp_agg_ie_len = agg_ie_len;
        unsigned char* tmp_agg_ie_ptr = agg_ie_ptr;

        /* get the IE id and ID length that we will search for */
        id = (int)(*tmp_new_ie_ptr);
        ilen = tmp_new_ie_ptr[1] + 2;

        /* Search for an exact copy of this IE in the aggregation buffer. */
        while (tmp_agg_ie_len > 0)
        {
            tmp = unifi_find_info_element(id, tmp_agg_ie_ptr, tmp_agg_ie_len);
            /* if the id not not exist we just want to get out of while loop */
            if (tmp) {
                /* match the length and the data */
                if ((tmp[1] == tmp_new_ie_ptr[1]) &&
                    (memcmp(tmp+2, tmp_new_ie_ptr+2, tmp[1]) == 0))
                {
                        fiematch = 1;
                        tmp_agg_ie_len = 0;
                }

                /* if they match we just want to get out of while loop */
                if (!fiematch) {
                    /* advance to the next IE in aggregation buffer */
                    tmp_agg_ie_len -= (tmp - tmp_agg_ie_ptr) + tmp[1] + 2;
                    tmp_agg_ie_ptr = (unsigned char*)tmp + (tmp[1] + 2);
                }
            } else {
                tmp_agg_ie_len = 0;
            }
        }

        /* If we have *not* found a match (new IE is unique) we store the new IE */
        if (!fiematch) {
            /* check first if we have enough space in aggr buffer */
            if (maxlen >= agg_ie_len + agg_ssid_ie_len + ilen)
            {
                memcpy(agg_ie_ptr+agg_ie_len, tmp_new_ie_ptr, ilen);
                agg_ie_len += ilen;
            } else {
                return agg_ie_len + agg_ssid_ie_len;
            }
        }

        /* advance to the next IE in new buffer */
        tmp_new_ie_ptr += ilen;
        tmp_new_ie_len -= ilen;
    }

#ifdef DEBUG_AGGREGATE
    printk("Agg: agg, len=%d\n", agg_ie_len + agg_ssid_ie_len);
    decode_info_elements(iebuf, agg_ie_len + agg_ssid_ie_len);
#endif /* DEBUG_AGGREGATE */

       /* do not forget to add the length of the SSID IE */
    return agg_ie_len + agg_ssid_ie_len;

} /* aggregate_info_elements() */





/* 
 * ------------------------------------------------------------------------
 *
 *      decode_info_elements
 * 
 * Diagnostic routine to print out the Information Elements in a scan
 * response.
 * 
 * ------------------------------------------------------------------------
 */
#ifdef DEBUG_AGGREGATE
static void
decode_info_elements(const unsigned char *info, int infolen)
{
    const unsigned char *p = info;
    int id, len;

    while (p < (info + infolen))
    {
        int col;

        id = *p++;
        len = *p++;

        printk(" %3d, %3d bytes: ", id, len);
        col = 0;
        while (len--) {
            printk(" %02X", *p);
            p++;

            if (++col == 16) {
                col = 0;
                printk("\n                 ");
            }
        }
        if (col) {
            printk("\n");
        }
    }
}
#endif /* DEBUG_AGGREGATE */
