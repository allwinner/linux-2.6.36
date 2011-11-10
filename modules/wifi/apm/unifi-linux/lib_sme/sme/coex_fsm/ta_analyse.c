/** @file ta_analyse.c
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
 *   Traffic Analysis Implementation
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/coex_fsm/ta_analyse.c#1 $
 *
 ****************************************************************************/

#include "coex_fsm/ta_analyse.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "smeio/smeio_trace_types.h"
#include "sys_sap/sys_sap_from_sme_interface.h"


/*
 * ---------------------------------------------------------------------------
 *  ta_internal_reset_t1
 *  ta_internal_reset_t2
 *  ta_internal_reset_t3
 *
 *      Internal only.
 *      Reset the internal TA parameters for CYCLE_1, CYCLE_2 and CYCLE_3
 *      respectively.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *
 *  Returns:
 *      None
 * ---------------------------------------------------------------------------
 */
static void
ta_internal_reset_t1(ta_priv_t *ta)
{
    /* Clear the intervals array for the last CYCLE_1 */
    CsrMemSet(ta->t1_intervals, 0, sizeof(ta->t1_intervals));
}

static void
ta_internal_reset_t2(ta_priv_t *ta)
{
    /* Clear everything with CYCLE_2 lifetime */
    ta->records.rx_bytes_count = ta->records.tx_bytes_count = 0;
    CsrMemSet(ta->records.period, 0, sizeof(ta->records.period));
}

static void
ta_internal_reset_t3(ta_priv_t *ta)
{
    /* Clear everything with CYCLE_3 lifetime */
    CsrMemSet(ta->records.tx_frames_num, 0, (sizeof(CsrUint32) * TA_TRANSITION_CYCLE_3));
    CsrMemSet(ta->records.rx_frames_num, 0, (sizeof(CsrUint32) * TA_TRANSITION_CYCLE_3));
}

/*
 * ---------------------------------------------------------------------------
 *  ta_change_classification
 *
 *      Internal only.
 *      Performs all the necessary actions, when a new classification
 *      is detected.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *      cls_type        The classification type enum value
 *      period          The period detected for the classification
 *      throughput      The throughput detected for the classification
 *
 *  Returns:
 *      None
 * ---------------------------------------------------------------------------
 */
static void
ta_change_classification(ta_priv_t *ta,
                         unifi_TrafficType traffic_type,
                         CsrUint16 period)
{
    sme_trace_entry((TR_COEX, "ta_change_classification(%s, %d) at t2=%d t3=%d",
                     trace_unifi_TrafficType(traffic_type), period,
                     ta->t2_tick, ta->t3_tick));

    send_coex_traffic_classification_ind(ta->context, getSmeContext(ta->context)->coexInstance, traffic_type, period);

    /* Pass the classification type to the driver */
    call_unifi_sys_traffic_classification_req(ta->context, traffic_type, period);

    /* Change the currently detected classification */
    ta->current_cls_type = traffic_type;

    /* Reset cycles and return */
    ta_internal_reset_t1(ta);
    ta_internal_reset_t2(ta);
    ta_internal_reset_t3(ta);
    ta->t2_tick = ta->t3_tick = 0;
}



/*
 * ---------------------------------------------------------------------------
 *  ta_calculate_period_t1
 *
 *      Internal only.
 *      Calculates the traffic period over a CYCLE_1.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *
 *  Returns:
 *      The period, or TA_CLS_PERIOD_NOT_DETECTED if no period is detected.
 * ---------------------------------------------------------------------------
 */
static CsrUint16
ta_calculate_period_t1(ta_priv_t *ta)
{
    int i;
    CsrUint32 tx_num;
    CsrUint32 frames;
    CsrUint16 period, period_index;

    /* Number of Tx frames over the last CYCLE_1 */
    tx_num = ta->records.tx_frames_num[ta->t3_tick];

    /* Assume no detected period, unless one is detected later */
    ta->records.period[ta->t2_tick] = 0;

    /*
     * If we have received more than TA_MAX_INTERVALS_IN_C1,
     * the traffic is not periodic.
     */
    if (tx_num >= TA_MAX_INTERVALS_IN_C1) {
        return TA_CLS_PERIOD_NOT_DETECTED;
    }

    /* Find the most heavily populated interval */
    frames = 0;
    period_index = 0;
    for (i = 0; i <= TA_INTERVALS_NUM; i++)
    {
        /*sme_trace_debug((TR_COEX, "ta_calculate_period_t1() i=%d with frames=%d (total=%d)", i, ta->t1_intervals[i], tx_num));*/
        if (ta->t1_intervals[i] > frames) {
            frames = ta->t1_intervals[i];
            period_index = (CsrUint16)i;
        }
    }

    /* Index 0 in not a valid period */
    if (period_index == 0)
    {
        return TA_CLS_PERIOD_NOT_DETECTED;
    }

    /*
     * If we have received more than twice as many packets as expected in the
     * last period, it means that the AP (or peer station) has more packets
     * to deliver. So we consider the traffic to be bursty or continuous
     */
    if (ta->records.rx_frames_num[ta->t2_tick] > (CsrUint32)((2*TA_MAX_INTERVALS_IN_C1)/period_index)) {
        sme_trace_debug((TR_COEX,
                         "t1: period=%d with frames=%d (total=%d) (rx=%d frames) at t2=%d t3=%d",
                         period_index * TA_INTERVALS_STEP, frames, tx_num,
                         ta->records.rx_frames_num[ta->t2_tick],
                         ta->t2_tick, ta->t3_tick));

        return TA_CLS_PERIOD_NOT_DETECTED;
    }

    sme_trace_debug((TR_COEX, "ta_calculate_period_t1() Found a period=%d with frames=%d (total=%d) at t2=%d t3=%d",
                period_index * TA_INTERVALS_STEP, frames, tx_num, ta->t2_tick, ta->t3_tick));

    /*
     * The selected interval must contain more than 50%
     * of the theoretical number of packets per second for the period,
     * i.e. frames > (1sec/period) * 50%
     */
    period = (CsrUint16) (period_index * TA_INTERVALS_STEP);
    if (frames > (CsrUint16)(500/period)) {
        ta->records.period[ta->t2_tick] = period;
        return period;
    }

    return TA_CLS_PERIOD_NOT_DETECTED;

} /* ta_calculate_period_t1() */


/*
 * ---------------------------------------------------------------------------
 *  ta_detect_continuous_t2
 *
 *      Internal only.
 *      Detects continuous traffic over a CYCLE_2.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *
 *  Returns:
 *      The throughput, or 0 if continuous traffic is not detected.
 * ---------------------------------------------------------------------------
 */
static CsrUint32
ta_detect_continuous_t2(ta_priv_t *ta)
{
    CsrUint32 i;
    CsrUint32 continuous_records;
    CsrUint32 t2_start, t2_end;
    CsrUint32 rx_throughput;
    CsrUint32 tx_throughput;

    /* The beggining and the end of the last CYCLE_2 are somewhere in CYCLE_3 */
    t2_start = ta->t3_tick - TA_TRANSITION_CYCLE_2;
    t2_end = ta->t3_tick;

    continuous_records = 0;
    rx_throughput = 0;
    for (i = t2_start; i < t2_end; i++)
    {
        if (ta->records.rx_frames_num[i] > TA_BURSTY_OR_CONTINUOUS_THRESHOLD) {
            continuous_records ++;
        }
    }
    if (continuous_records == TA_TRANSITION_CYCLE_2)
    {
        rx_throughput = ((ta->records.rx_bytes_count / 1024) / TA_TRANSITION_CYCLE_2) * 8;
        sme_trace_debug((TR_COEX, "ta_detect_continuous_t2() Rx continuous throughput = %lu Kbits/sec at t2=%d t3=%d",
                         rx_throughput, ta->t2_tick, ta->t3_tick));
    }

    continuous_records = 0;
    tx_throughput = 0;
    for (i = t2_start; i < t2_end; i++) {
        if (ta->records.tx_frames_num[i] > TA_BURSTY_OR_CONTINUOUS_THRESHOLD) {
            continuous_records ++;
        }
    }
    if (continuous_records == TA_TRANSITION_CYCLE_2) {
        tx_throughput = ((ta->records.tx_bytes_count / 1024) / TA_TRANSITION_CYCLE_2) * 8;
        sme_trace_debug((TR_COEX, "ta_detect_continuous_t2() Tx continuous throughput = %lu Kbits/sec at t2=%d t3=%d",
                         tx_throughput, ta->t2_tick, ta->t3_tick));
    }

    return (tx_throughput > rx_throughput) ? tx_throughput : rx_throughput;
}


/*
 * ---------------------------------------------------------------------------
 *  ta_detect_bursty_t2
 *
 *      Internal only.
 *      Detects bursty traffic over a CYCLE_2.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *
 *  Returns:
 *      TA_CLS_DETECTED if bursty traffic is detected,
 *      TA_CLS_NOT_DETECTED otherwise
 * ---------------------------------------------------------------------------
 */
static CsrUint32
ta_detect_bursty_t2(ta_priv_t *ta)
{
    CsrUint32 i;
    CsrUint32 bursty_records;
    CsrUint32 t2_start, t2_end;

    /* The beggining and the end of the last CYCLE2 are somewhere in CYCLE3 */
    t2_start = ta->t3_tick - TA_TRANSITION_CYCLE_2;
    t2_end = ta->t3_tick;

    bursty_records = 0;
    for (i = t2_start; i < t2_end; i++) {
        if (ta->records.rx_frames_num[i] > TA_BURSTY_OR_CONTINUOUS_THRESHOLD) {
            bursty_records ++;
        }
    }
    if (bursty_records > TA_BURSTY_MAX_T2_RECORDS) {
        return TA_CLS_NOT_DETECTED;
    }


    bursty_records = 0;
    for (i = t2_start; i < t2_end; i++) {
        if (ta->records.tx_frames_num[i] > TA_BURSTY_OR_CONTINUOUS_THRESHOLD) {
            bursty_records ++;
        }
    }
    if (bursty_records > TA_BURSTY_MAX_T2_RECORDS) {
        return TA_CLS_NOT_DETECTED;
    }

    return TA_CLS_DETECTED;
}


/*
 * ---------------------------------------------------------------------------
 *  ta_detect_periodic_t2
 *
 *      Internal only.
 *      Detects periodic traffic over a CYCLE_2.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *
 *  Returns:
 *      The period if periodic traffic is detected,
 *      TA_CLS_PERIOD_NOT_DETECTED otherwise
 * ---------------------------------------------------------------------------
 */
static CsrUint16
ta_detect_periodic_t2(ta_priv_t *ta)
{
    int i;
    CsrUint16 selected_period;
    CsrUint32 num_of_valid_periods;

    /* Start with the longest period possible... */
    selected_period = TA_INTERVALS_NUM * TA_INTERVALS_STEP;
    num_of_valid_periods = 0;

    for (i = 0; i < TA_TRANSITION_CYCLE_2; i++) {
        sme_trace_debug((TR_COEX, "ta_detect_periodic_t2() Period = %d msecs at t2=%d",
                         ta->records.period[i], i));
        if (ta->records.period[i] > 0) {
            num_of_valid_periods ++;
            /* ... and pick up any lower */
            if (ta->records.period[i] < selected_period) {
                /* its safe to cast this as ta->records.period[i] will never exceed period */
                selected_period = (CsrUint16)ta->records.period[i];
            }
        }
    }

    /* Require at least TA_PERIODIC_MIN_T2_RECORDS records per CYCLE_2 */
    if (num_of_valid_periods >= TA_PERIODIC_MIN_T2_RECORDS) {
        return selected_period;
    }

    return TA_CLS_PERIOD_NOT_DETECTED;
}


/*
 * ---------------------------------------------------------------------------
 *  ta_detect_occasional_t3
 *
 *      Internal only.
 *      Detects occasional traffic over a CYCLE_3.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *
 *  Returns:
 *      TA_CLS_DETECTED if occasional traffic is detected,
 *      TA_CLS_NOT_DETECTED otherwise
 * ---------------------------------------------------------------------------
 */
static CsrUint32
ta_detect_occasional_t3(ta_priv_t *ta)
{
    int i;

    /*
     * One record with over TA_BURSTY_OR_CONTINUOUS_THRESHOLD frames,
     * is enough to keep the traffic to Bursty.
     */
    for (i = 0; i < TA_TRANSITION_CYCLE_3; i++) {
        if (ta->records.rx_frames_num[i] > TA_BURSTY_OR_CONTINUOUS_THRESHOLD) {
            return TA_CLS_NOT_DETECTED;
        }
        if (ta->records.tx_frames_num[i] > TA_BURSTY_OR_CONTINUOUS_THRESHOLD) {
            return TA_CLS_NOT_DETECTED;
        }
    }

    return TA_CLS_DETECTED;
}



/*
 * ---------------------------------------------------------------------------
 *  ta_classify_t1
 *  ta_classify_t2
 *  ta_classify_t3
 *
 *      Internal only.
 *      Detects any interesting traffic classification over
 *      CYCLE_1, CYCLE_2 and CYCLE_3 respectively.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *
 *  Returns:
 *      TA_CLS_DETECTED if any traffic is detected,
 *      TA_CLS_NOT_DETECTED otherwise
 * ---------------------------------------------------------------------------
 */
static CsrUint32
ta_classify_t1(ta_priv_t *ta, CsrUint16 period_t1)
{
    /* Process the data received during TA_TRANSITION_CYCLE_1 */
    if ( (ta->current_cls_type == unifi_TrafficOccasional) ||
         (ta->current_cls_type == unifi_TrafficBursty) )
    {
        /* If the traffic is periodic, indicate it to the SME... */
        if (period_t1 > 0)
        {
            ta_change_classification(ta, unifi_TrafficPeriodic, period_t1);

            return TA_CLS_DETECTED;
        } else {
            /*
             * ... otherwise if the traffic is bursty and the current
             * traffic is occasional, indicate it to the SME.
             */
            if ( (ta->current_cls_type == unifi_TrafficOccasional) &&
                 ( (ta->records.rx_frames_num[ta->t3_tick] > TA_BURSTY_OR_CONTINUOUS_THRESHOLD) ||
                   (ta->records.tx_frames_num[ta->t3_tick] > TA_BURSTY_OR_CONTINUOUS_THRESHOLD) ) )
            {
                sme_trace_debug((TR_COEX, "ta_classify_t1() Occasional -> Bursty (tx=%d, rx=%d)",
                            ta->records.rx_frames_num[ta->t3_tick],
                            ta->records.tx_frames_num[ta->t3_tick]));

                ta_change_classification(ta, unifi_TrafficBursty, 0);
                return TA_CLS_DETECTED;
            }
        }
    }

    return TA_CLS_NOT_DETECTED;
}


static CsrUint32
ta_classify_t2(ta_priv_t *ta)
{
    CsrUint16 period;
    CsrUint32 throughput;

    /* Rules A, B and D for CYCLE_2 */
    if ( (ta->current_cls_type == unifi_TrafficPeriodic) ||
         (ta->current_cls_type == unifi_TrafficContinuous) )
    {
        /* Rules A and B */
        /* Calculate the period over the last CYCLE_2 */
        period = ta_detect_periodic_t2(ta);
        if (period > 0) {
            ta_change_classification(ta, unifi_TrafficPeriodic, period);
            return TA_CLS_DETECTED;
        }

        /* Rule D */
        if (ta_detect_bursty_t2(ta)) {
            ta_change_classification(ta, unifi_TrafficBursty, 0);
            return TA_CLS_DETECTED;
        }
    }

    /* Rule C for CYCLE_2 */
    if ( (ta->current_cls_type == unifi_TrafficPeriodic) ||
         (ta->current_cls_type == unifi_TrafficBursty) ||
         (ta->current_cls_type == unifi_TrafficContinuous) )
    {
        throughput = ta_detect_continuous_t2(ta);
        if (throughput) {
            ta_change_classification(ta, unifi_TrafficContinuous, 0);
            return TA_CLS_DETECTED;
        }
    }

    return TA_CLS_NOT_DETECTED;
}


static int
ta_classify_t3(ta_priv_t *ta)
{
    /* Rule A for CYCLE_3 */
    if (ta->current_cls_type == unifi_TrafficBursty) {

        if (ta_detect_occasional_t3(ta)) {
            ta_change_classification(ta, unifi_TrafficOccasional, 0);
            return TA_CLS_DETECTED;
        }
    }

    return TA_CLS_NOT_DETECTED;
}



static void
ta_classify_active_traffic(ta_priv_t *ta, CsrUint32 ticks)
{
    CsrUint16 period; /* relative time */
    CsrUint32 records_to_clear;

    /*
     * Advance the analysis cycle counters.
     * The received stats are for the previous tick so advance the
     * t2_tick and t3_tick to the correct index in the records.
     */
    ta->t2_tick += (ticks > 0) ? (ticks - 1) : 0;
    ta->t3_tick += (ticks > 0) ? (ticks - 1) : 0;

    /*
     * If there wasn't any traffic, the ticks may exceed the cycles.
     * Make sure the t2_tick and t3_tick do not exceed the cycle timeouts
     * otherwise we might write over the record buffers end. In this case,
     * we ignore how many ticks have passed and we just store the stats
     * in the last record which is not necessarily the correct but it is
     * definetely empty. At the same time, we clear the stats for the time
     * window we have missed within the cycle.
     * E.g. if t2_tick == (TA_TRANSITION_CYCLE_2 + 2) we store the stats in
     * (TA_TRANSITION_CYCLE_2 - 1) and we clear the stats from index 0 to
     * ((ta->t2_tick + 1) >= 2*TA_TRANSITION_CYCLE_2) ?
     *      (TA_TRANSITION_CYCLE_2 - 1) :
     *      (ta->t2_tick % TA_TRANSITION_CYCLE_2) + 1
     */
    if (ta->t3_tick >= TA_TRANSITION_CYCLE_3) {
        records_to_clear = ((ta->t3_tick + 1) >= 2*TA_TRANSITION_CYCLE_3) ? (TA_TRANSITION_CYCLE_3 - 1) : ((ta->t3_tick % TA_TRANSITION_CYCLE_3) + 1);
        CsrMemSet(ta->records.tx_frames_num, 0, (sizeof(CsrUint32) * records_to_clear));
        CsrMemSet(ta->records.rx_frames_num, 0, (sizeof(CsrUint32) * records_to_clear));

        ta->t3_tick = TA_TRANSITION_CYCLE_3 - 1;
    }
    if (ta->t2_tick >= TA_TRANSITION_CYCLE_2) {
        records_to_clear = ((ta->t2_tick + 1) >= 2*TA_TRANSITION_CYCLE_2) ? (TA_TRANSITION_CYCLE_2 - 1) : ((ta->t2_tick % TA_TRANSITION_CYCLE_2) + 1);
        CsrMemSet(ta->records.period, 0, (sizeof(CsrUint32) * records_to_clear));

        ta->t2_tick = TA_TRANSITION_CYCLE_2 - 1;
    }

    /*
     * If we are running because we received a report from the TA sampling
     * module (i.e. not an idle timeout), run the periodic detection.
     */
    if (ta->new_record)
    {
        /* Update the Rx packet count and the throughput count */
        ta->records.rx_frames_num[ta->t3_tick] = ta->stats.rxFramesNum;
        ta->records.rx_bytes_count            += ta->stats.rxBytesCount;

        ta->records.tx_frames_num[ta->t3_tick] = ta->stats.txFramesNum;
        ta->records.tx_bytes_count            += ta->stats.txBytesCount;

        /* Array copy */
        CsrMemCpy(ta->t1_intervals, ta->stats.intervals, sizeof(ta->t1_intervals));

        /* Calculate the period over the last second */
        period = ta_calculate_period_t1(ta);

        /* Run the classification algorithm for CYCLE_1 */
        if (ta_classify_t1(ta, period) == TA_CLS_DETECTED) {
            return;
        }
    }
    else
    {
        ta_internal_reset_t1(ta);
    }

    /*
     * Now update the analysis cycle counters to point the current second,
     * all the t2 and t3 traffic detection algorithms depend on this.
     */
    ta->t2_tick++;
    ta->t3_tick++;

    /* Check if TA_TRANSITION_CYCLE_2 is completed */
    if (ta->t2_tick >= TA_TRANSITION_CYCLE_2) {

        /* Run the classification algorithm for CYCLE_2 */
        if (ta_classify_t2(ta) == TA_CLS_DETECTED) {
            return;
        }

        /* CYCLE_2 has expired, reset internal state */
        ta_internal_reset_t2(ta);
    }


    /* Check if TA_TRANSITION_CYCLE_3 is completed */
    if (ta->t3_tick >= TA_TRANSITION_CYCLE_3) {

        /* Run the classification algorithm for CYCLE_3 */
        if (ta_classify_t3(ta) == TA_CLS_DETECTED) {
            return;
        }

        /* CYCLE_3 has expired, reset internal state */
        ta_internal_reset_t3(ta);
        ta->t2_tick = ta->t3_tick = 0;
    }

} /* ta_classify_active_traffic() */




/*
 * ---------------------------------------------------------------------------
 *  API.
 *  unifi_ta_analyse_init
 *
 *      (Re)Initialise the Traffic Analysis analysis module.
 *      Resets the counters, classification and timestamps.
 *      Sends a class notification.
 *
 *  Arguments:
 *      drv_priv        Context pointer of the module that creates
 *                      this TA module.
 *
 *  Returns:
 *      The pointer to the TA context or NULL if failed.
 * ---------------------------------------------------------------------------
 */
void
unifi_ta_analyse_init(ta_priv_t *ta, FsmContext* context)
{
    ta->context = context;
    ta->t2_tick = 0;
    ta->t3_tick = 0;

    /* Clear the counters, reset the class and send a class notification */
    ta_change_classification(ta, unifi_TrafficOccasional, 0);
    ta->last_run_time = fsm_get_time_of_day_ms(context);

} /* unifi_ta_analyse_init() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_ta_run
 *
 *      API.
 *      Run the TA background process.
 *      This function does all the real work. It processes all the data
 *      received during the last CYCLE_1, and tries to detect a traffic
 *      classification when any of the 3 cycles are completed.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *
 *  Returns:
 *      The interval time (in msecs) after which it should be called again.
 * ---------------------------------------------------------------------------
 */
CsrUint32
unifi_ta_run(ta_priv_t *ta)
{
    CsrUint32 now, elapsed_time;
    CsrUint32 timeout;
    CsrUint32 ticks;

    /* Calculate the time since the previous sampling report in 1 second ticks */
    now = fsm_get_time_of_day_ms(ta->context);
    elapsed_time = now - ta->last_run_time;
    ta->last_run_time = now;

    ticks = elapsed_time / 1000;

    /* Run the analysis */
    ta_classify_active_traffic(ta, ticks);

    /*
     * If a new record is received, set the timeout to the end of analysis cycle 2.
     * The t2_tick and t3_tick are still running, so they should timeout either on
     * a new record reception, or when timeout expires. This causes a potensial
     * timeout delay of up to (TA_TRANSITION_CYCLE_2 - 1) if no traffic is received.
     *
     * Otherwise, set an idle timeout based on new traffic class:
     *   Continuous     5s
     *   Bursty         30s
     *   Periodic       5s
     *   Occaisional    infinite
     * Timeout is time to end of analysis cycle.
     */
    if (ta->new_record)
    {
        timeout = TA_TRANSITION_CYCLE_2 * TA_ONE_SEC;

    } else {
        timeout = (TA_TRANSITION_CYCLE_2 - ta->t2_tick) * TA_ONE_SEC;

        if (ta->current_cls_type == unifi_TrafficBursty) {
            timeout = (TA_TRANSITION_CYCLE_3 - ta->t3_tick) * TA_ONE_SEC;
        }

        if (ta->current_cls_type == unifi_TrafficOccasional) {
            timeout = TA_TIMEOUT_INFINITE; /* indefinite */
        }
    }

    ta->new_record = 0;

    return timeout;

} /* unifi_ta_run() */





/*
 * ---------------------------------------------------------------------------
 *  API.
 *  unifi_ta_data
 *
 *      Pass a sampling record to the TA analysis module.
 *      This function stores all the useful information it can extract from
 *      the frame and detects any specific protocols.
 *
 *  Arguments:
 *      ta              The pointer to the TA module.
 *      stats           Pointer to stats structure from the TA sampling
 *                      front end.
 *
 *  Returns:
 *      None
 * ---------------------------------------------------------------------------
 */
void
unifi_ta_data(ta_priv_t *ta, const unifi_TrafficStats *stats)
{
    if (!ta) {
        /* sanity check */
        return;
    }

    /* Keep a copy of the stats in the ta */
    ta->stats = *stats;

    ta->new_record = 1;

} /* unifi_ta_data() */



