/*
 * ---------------------------------------------------------------------------
 *  FILE:     ta_sampling.h
 * 
 *  PURPOSE:
 *      This file contains Traffic Analysis definitions common to the
 *      sampling and analysis modules.
 *
 * Copyright (C) 2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#ifndef __TA_SAMPLING_H__
#define __TA_SAMPLING_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/unifi.h"


/* 
 * Context structure to preserve state between calls.
 */
typedef struct ta_data {

    /* Current packet filter configuration */
    CsrUint16 packet_filter;

    /* Current packet custom filter configuration */
    unifi_TrafficFilter custom_filter;

    /* The timestamp of the last tx packet processed. */
    CsrUint32 tx_last_ts;

    /* The timestamp of the last packet processed. */
    CsrUint32 last_indication_time;

    /* Statistics */
    unifi_TrafficStats stats;

    /* Current traffic classification */
    unifi_TrafficType traffic_type;

    /* Sum of packet rx rates for this interval used to calculate mean */
    CsrUint32 rx_sum_rate;
    
} ta_data_t;




void unifi_ta_sampling_init(card_t *card);


#ifdef __cplusplus
}
#endif

#endif /* __TA_SAMPLING_H__ */
