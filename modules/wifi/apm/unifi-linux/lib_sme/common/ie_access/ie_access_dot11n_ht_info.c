/** @file ie_access_dot11n_ht_info.c
 *
 * Utilities to process 802.11n information elements
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
 *   Utilities to process 802.11n information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/ie_access/ie_access_dot11n_ht_info.c#1 $
 *
 ****************************************************************************/

#include "ie_access/ie_access_dot11n.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "ie_access/ie_access.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in dot11n/ie_access_dot11n.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8 ie_dot11n_get_ht_info_length(
                    void)
{
    return IE_HT_INFORMATION__TOTAL_SIZE;
}
/*
typedef struct Dot11nHtInfo
{
    CsrUint8                          id;
    CsrUint8                          length;
    CsrUint8                          primaryChannel;
    HtInfoBlk1                     htInfoBlk1;
    HtInfoBlk2                     htInfoBlk2;
    HtInfoBlk3                     htInfoBlk3;
    CsrUint8                          basicMcsSet[16];
}Dot11nHtInfo;
*/
#define IE_DOT11N_HT_INFO__ID_OFFSET                         0
#define IE_DOT11N_HT_INFO__LENGTH_OFFSET                     (IE_DOT11N_HT_INFO__ID_OFFSET + IE_HT_INFORMATION__ID_SIZE)
#define IE_DOT11N_HT_INFO__PRIMARY_CHANNEL_OFFSET            (IE_DOT11N_HT_INFO__LENGTH_OFFSET + IE_HT_INFORMATION__LENGTH_SIZE)
#define IE_DOT11N_HT_INFO__HT_INFO_BLK1_OFFSET               (IE_DOT11N_HT_INFO__PRIMARY_CHANNEL_OFFSET + IE_HT_INFORMATION__PRIMARY_CHANNEL_SIZE)
#define IE_DOT11N_HT_INFO__HT_INFO_BLK2_OFFSET               (IE_DOT11N_HT_INFO__HT_INFO_BLK1_OFFSET + IE_HT_INFORMATION__HT_INFO_BLK1_SIZE)
#define IE_DOT11N_HT_INFO__HT_INFO_BLK3_OFFSET               (IE_DOT11N_HT_INFO__HT_INFO_BLK2_OFFSET + IE_HT_INFORMATION__HT_INFO_BLK2_SIZE)
#define IE_DOT11N_HT_INFO__BASIC_MCS_SET_OFFSET              (IE_DOT11N_HT_INFO__HT_INFO_BLK3_OFFSET + IE_HT_INFORMATION__HT_INFO_BLK3_SIZE)

/*
 * Description:
 * See description in dot11n/ie_access_dot11n.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8* ie_dot11n_generate_ht_info_ie(
                    CsrUint8 *pbuf)
{

    pbuf[IE_DOT11N_HT_INFO__ID_OFFSET] = IE_DOT11N_ID_HT_INFO;
    pbuf[IE_DOT11N_HT_INFO__LENGTH_OFFSET] = IE_HT_INFORMATION__TOTAL_SIZE;

    pbuf[IE_DOT11N_HT_INFO__PRIMARY_CHANNEL_OFFSET] = 1;
    pbuf[IE_DOT11N_HT_INFO__HT_INFO_BLK1_OFFSET] = 0;
    pbuf[IE_DOT11N_HT_INFO__HT_INFO_BLK1_OFFSET] = 0;
    pbuf[IE_DOT11N_HT_INFO__HT_INFO_BLK1_OFFSET] = 0;

    CsrMemSet(&pbuf[IE_DOT11N_HT_INFO__BASIC_MCS_SET_OFFSET], 0, 16);

    return pbuf + IE_HT_INFORMATION__TOTAL_SIZE;
}

