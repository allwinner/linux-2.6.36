/** @file mib_utils.h
 *
 * mib access support function header file
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
 *   Provides the external function declarions for mib access process
 *   support functions.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mib_utils.h#1 $
 *
 ****************************************************************************/
#ifndef MIB_UTILS_H
#define MIB_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "hip_proxy_fsm/mibdefs.h"

/* PUBLIC MACROS ************************************************************/

/* PUBLIC TYPES DEFINITIONS *************************************************/

/**
 * @brief builds and sends a mlme_get_req
 */
extern void mib_util_send_get(FsmContext* context, mib_ids id, CsrUint8 index1, CsrUint8 index2);

/**
 * @brief builds and sends a mlme_set_req for an integer
 */
extern void mib_util_send_set_int(FsmContext* context, mib_ids id, CsrInt32 value, CsrUint8 index1, CsrUint8 index2);

/**
 * @brief builds and sends a mlme_set_req for an os
 */
extern void mib_util_send_set_os(FsmContext* context, mib_ids id, const CsrUint8* data, CsrUint8 datalength, CsrUint8 index1, CsrUint8 index2);

/**
 * @brief Encodes the stats request mib data
 */
extern void mib_util_encode_stats_req(FsmContext* context, DataReference* mibGetDataRef, unifi_RadioIF ifIndex);

/**
 * @brief builds and sends a mlme_get_req for the stats data
 */
extern void mib_util_send_stats_req(FsmContext* context, unifi_RadioIF ifIndex);

/**
 * @brief Decodes mib encoded stats data
 */
extern CsrBool mib_util_decode_stats(FsmContext* context, const DataReference* mibData, MibStatus status, CsrUint16 errorIndex, unifi_ConnectionStats* stats);

/**
 * @brief Encodes the link quality request mib data
 */
extern void mib_util_encode_linkq_req(FsmContext* context, DataReference* mibGetDataRef, unifi_RadioIF ifIndex);

/**
 * @brief builds and sends a mlme_get_req for the link quality data
 */
extern void mib_util_send_linkq_req(FsmContext* context, unifi_RadioIF ifIndex);

/**
 * @brief Decodes mib encoded link quality data
 */
extern CsrBool mib_util_decode_linkq(FsmContext* context, const DataReference* mibData, MibStatus status, CsrUint16 errorIndex, unifi_LinkQuality* linkq);


#ifdef __cplusplus
}
#endif


#endif /* MIB_UTILS_H */
