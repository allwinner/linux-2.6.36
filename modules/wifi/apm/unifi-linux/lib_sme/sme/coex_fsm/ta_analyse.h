/** @file ta_analyse.h
 *
 * Traffic Analysis
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *      This file defines the Traffic Analysis API and interval definitions.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/coex_fsm/ta_analyse.h#1 $
 *
 ****************************************************************************/
#ifndef __TA_ANALYSE_H__
#define __TA_ANALYSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sme_top_level_fsm/sme_top_level_fsm.h"

/* Maximum number of Tx frames we store each CYCLE_1, for detecting period */
#define TA_MAX_INTERVALS_IN_C1          100

/* Number of intervals in CYCLE_1 (one second), for detecting periodic */
#define TA_INTERVALS_NUM                10

/* Step (in msecs) between intervals, for detecting periodic */
/* We are only interested in periods up to 100ms, i.e. between beacons */
#define TA_INTERVALS_STEP               10

/*
 * opaque TA pointer
 */
struct ta_priv;
typedef struct ta_priv ta_priv_t;

/*
 * Create, Destroy a TA object
 */
void unifi_ta_analyse_init(ta_priv_t *ta, FsmContext* context);



/*
 * Inject data gathered by TA Sampling module
 */
void unifi_ta_data(ta_priv_t *ta, const unifi_TrafficStats *stats);


/*
 * Run the TA periodic task
 */
CsrUint32 unifi_ta_run(ta_priv_t *ta);

/*
 * Internal structures and definitions
 */
#define TA_ONE_SEC                      1000 /* SME clock runs in milliseconds */
#define TA_TIMEOUT_INFINITE             0

#define TA_TRANSITION_CYCLE_1           1
#define TA_TRANSITION_CYCLE_2           5
#define TA_TRANSITION_CYCLE_3           30

/* Lower threshold for bursty and continuous traffic for CYCLE_1 */
#define TA_BURSTY_OR_CONTINUOUS_THRESHOLD         20

/* Upper threshold for number of records per CYCLE_2, for detecting bursty */
#define TA_BURSTY_MAX_T2_RECORDS        3
/* Lower threshold for number of records per CYCLE_2, for detecting periodic */
#define TA_PERIODIC_MIN_T2_RECORDS      4

/* Return Values */
#define TA_CLS_DETECTED                 1
#define TA_CLS_NOT_DETECTED             0
#define TA_CLS_PERIOD_NOT_DETECTED      0


/*
 * <---------------------   CYCLE_3  ----------------------->
 * ___________________________________________________________
 * | 0 | 1 | 2 | 3 | 4 |   | ... |   |   |   |   |   |   | 29|
 * -----------------------------------------------------------
 * <----  CYCLE_2 ---->      ...         <----  CYCLE_2 ---->
 *
 * CYCLE_2 needs to be an exact multiple of CYCLE_3.
 */


/*
 * Structure to hold all the records for
 * TA_TRANSITION_CYCLE_1 to TA_TRANSITION_CYCLE_3 secs
 */
struct ta_records {
    /* Keep number of Rx frames per second, for CYCLE_3. */
    CsrUint32 rx_frames_num[TA_TRANSITION_CYCLE_3];
    /* Keep number of Tx frames per second, for CYCLE_3. */
    CsrUint32 tx_frames_num[TA_TRANSITION_CYCLE_3];
    /* Keep calculated period per second, for CYCLE_2. */
    CsrUint32 period[TA_TRANSITION_CYCLE_2];
    /* Keep calculated Rx throughput per second, for CYCLE_2. */
    CsrUint32 rx_bytes_count;
    /* Keep calculated Tx throughput per second, for CYCLE_2. */
    CsrUint32 tx_bytes_count;
};


/*
 * The private TA analysis module structure.
 */
struct ta_priv {

    FsmContext* context;

    /* Structure that holds the records. */
    struct ta_records records;

    /*
     * Transition cycle timers.
     * They are also used as indexes for the records structure.
     */
    CsrUint32 t2_tick;
    CsrUint32 t3_tick;

    /* Flag to indicate that new sampling data has been received */
    CsrUint8 new_record;

    /*
     * Array that holds the detected intervals for the last second.
     * Index 0 is used for intervals < 10 or intervals > 100
     * The msecs between each step is TA_INTERVALS_STEP.
     */
    CsrUint8 t1_intervals[TA_INTERVALS_NUM + 1];

    /*
     * The current detected classification type.
     * The cycles act on the records according to this type.
     */
    unifi_TrafficType current_cls_type;

    /* The timestamp of the last call to unifi_ta_run() */
    CsrUint32 last_run_time;

    /*
     * Structure to hold the stats indicated from the driver
     * until they are stored in the records.
     */
    unifi_TrafficStats stats;
};

#ifdef __cplusplus
}
#endif

#endif /* __TA_ANALYSE_H__ */
