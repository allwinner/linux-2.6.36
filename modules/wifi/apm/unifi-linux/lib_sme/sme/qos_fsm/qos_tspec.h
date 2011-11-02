/** @file qos_tspec.h
 *
 * Public SME QOS TSPEC API
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
 *   Public SME QOS TSPEC API
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/qos_fsm/qos_tspec.h#2 $
 *
 ****************************************************************************/
#ifndef QOS_TSPEC_H
#define QOS_TSPEC_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup qos
 */

/* STANDARD INCLUDES ********************************************************/

#include "sme_top_level_fsm/sme_top_level_fsm.h"

    /* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "csr_cstl/csr_wifi_list.h"

/* MACROS *******************************************************************/

#define NUMBER_OF_ACCESS_CHANNELS             4
#define DIRECTIONS_PERS_AC                    2
#define TOTAL_NUMBER_OF_ACCESS_CHANNELS       (NUMBER_OF_ACCESS_CHANNELS * DIRECTIONS_PERS_AC)
#define NUMBER_OF_ASSOCIATE_REQ_TSPECS        2
#define RESERVED_AC                           1

#define ASSOCIATE_REQ_TSPEC_SIGNALLING_INDEX  (TOTAL_NUMBER_OF_ACCESS_CHANNELS+1)
#define ASSOCIATE_REQ_TSPEC_VOICE_INDEX       (ASSOCIATE_REQ_TSPEC_SIGNALLING_INDEX+1)

#define NUMBER_OF_ACTIVE_TSPECS               (TOTAL_NUMBER_OF_ACCESS_CHANNELS + NUMBER_OF_ASSOCIATE_REQ_TSPECS)

#define ADD_TSPEC_FAILURE_TIMEOUT  1000 /* 1 second timeout */

/* TYPES DEFINITIONS ********************************************************/
/**
 * @brief: flags indicating IEs present in beacons that should be
 *         placed in associate request
 */
typedef enum qos_tidMask
{
    TID_NONE                 =    0x0000,
    TID_1                    =    0x0001,
    TID_2                    =    0x0002,
    TID_3                    =    0x0004,
    TID_4                    =    0x0008,
    TID_5                    =    0x0010,
    TID_6                    =    0x0020,
    TID_7                    =    0x0040,
    TID_8                    =    0x0080,
    TID_ALL                  =    0x00FF
} qos_tidMask;

/**
 * @brief: flags indicating IEs present in beacons that should be
 *         placed in associate request
 */
typedef enum qos_ac
{
    AC_NONE                 =    0x0000,
    AC_1                    =    0x0001,
    AC_2                    =    0x0002,
    AC_3                    =    0x0003,
    AC_4                    =    0x0004
} qos_ac;

typedef enum qos_ac_mask
{
    AC_NONE_MASK            =    0x0000,
    AC_BEST_EFFORT_MASK     =    0x0001,
    AC_BACKGROUND_MASK      =    0x0002,
    AC_VIDEO_MASK           =    0x0004,
    AC_VOICE_MASK           =    0x0008
} qos_ac_mask;

typedef enum qos_tspec_status
{
    TSPEC_INACTIVE                          =    0x0001,
    TSPEC_RESERVED                          =    0x0002,
    TSPEC_INITALIZED                        =    0x0004,
    TSPEC_IDLE                              =    0x0008,
    TSPEC_ACTIVE                            =    0x0010,
    TSPEC_PENDING                           =    0x0020,
    TSPEC_REJECTED_UNSPECIFIEDQOSFAILURE    =    0x0100,
    TSPEC_REJECTED_WRONGPOLICY              =    0x0200,
    TSPEC_REJECTED_INSUFFICIENTBANDWIDTH    =    0x0400,
    TSPEC_REJECTED_INVALIDTSPECPARAMETERS   =    0x0800,
    TSPEC_REJECTED_ANY                      =    0x0F00

} qos_tspec_status;

typedef enum qos_tspec_origin
{
    TSPEC_USER              =    0x0001,
    TSPEC_INTERNAL          =    0x0002,
    TSPEC_COMBINED          =    0x0004
} qos_tspec_origin;

typedef struct qos_tspecData
{
    DialogToken dialogToken;

    /* store the tsinfo for quick look up */
    CsrUint32   tsInfo;
    CsrUint8    tid;

    CsrBool     strict;
    CsrUint8    ctrlMask;
    CsrUint8    *tspec;
    CsrUint16   tspecLength;
    CsrUint8    *tclas;
    CsrUint16   tclasLength;
    CsrUint16   requesterId;
    CsrUint32   transactionId;
    unifi_TspecResultCode resultCode;
    qos_tspec_origin tspecOrigin;

    CsrUint8    suggestTspecCount;

}qos_tspecData;

typedef struct qos_tspecDataNode
{
    qos_tspecData* ptspecsData;
    qos_tspec_status status;

}qos_tspecDataNode;

typedef struct qos_tspecDataBlk
{
    DialogToken currentDialogToken;
    qos_tidMask tidMap;
    /** the list will be 1 larger than required to allow direct tid indexing */
    qos_tspecDataNode activeTspecs[RESERVED_AC + NUMBER_OF_ACTIVE_TSPECS];
    CsrUint8 tspecSearchIndex;
    qos_tspec_status searchStatus;

}qos_tspecDataBlk;


/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

extern void qos_initialise_tspec_handler(
        qos_tspecDataBlk *pDataBlk);

extern void qos_reset_tspec_handler(
        qos_tspecDataBlk *pDataBlk);

extern qos_tspecData* qos_add_tspec_data(
        qos_tspecDataBlk *pDataBlk,
        const UnifiMgtTspecReq_Evt* req);

extern void qos_delete_tspec_data_by_tid(
        qos_tspecDataBlk *pDataBlk,
        CsrUint8 tid);

extern DialogToken qos_get_dialog_token(
        qos_tspecDataBlk *pDataBlk);

extern unifi_TspecResultCode validate_tspec_content(
        FsmContext* context,
        CsrUint16 tspecLength,
        CsrUint8 *tspec);

extern unifi_TspecResultCode validate_tspec_position(
        FsmContext* context,
        qos_tspecDataBlk *pDataBlk,
        CsrUint16 tspecLength,
        CsrUint8 *tspec);

extern qos_tspecData* qos_get_tspec_data_by_transaction_id(
        qos_tspecDataBlk *pDataBlk,
        CsrUint32 transactionId);

extern qos_tspecData* qos_get_tspec_data_by_tid(
        qos_tspecDataBlk *pDataBlk,
        CsrUint8 tid);

extern void qos_delete_tspec_data_by_transaction_id(
        qos_tspecDataBlk *pDataBlk,
        CsrUint32 transactionId);

extern CsrBool update_active_tspec_list(
        qos_tspecDataBlk *pDataBlk);

extern void clear_active_tspec_list(
        qos_tspecDataBlk *pDataBlk);

extern qos_tspecData* qos_get_first_active_tspec(
        qos_tspecDataBlk *pDataBlk,
        qos_tspec_status status);

extern qos_tspecData* qos_get_next_active_tspec(
        qos_tspecDataBlk *pDataBlk);

extern CsrUint16 qos_tspec_associate_tspec_length(
        qos_tspecDataBlk *pTspecDataBlk);
extern CsrUint8* qos_tspec_get_associate_tspec_ies(
        qos_tspecDataBlk *pTspecDataBlk,
        CsrUint8 *pbuf);
extern void qos_tspec_update_associate_tspec_status(
        qos_tspecDataBlk *pTspecDataBlk,
        qos_tspec_status status);

extern void qos_tspec_update_tspec_status(
        qos_tspecDataBlk *pTspecDataBlk,
        CsrUint8 tid,
        qos_tspec_status status);


extern CsrBool qos_tspec_process_associate_cfm_ie(
        qos_tspecDataBlk *pDataBlk,
        CsrUint8 *pTspecBuf,
        CsrUint16 tspecLength,
        ResultCode resultCode);

extern CsrBool qos_tspec_check_tspec_active(
        qos_tspecDataBlk *pTspecDataBlk,
        CsrUint8 tid);

/** \@}
 */

#ifdef __cplusplus
}
#endif


#endif /* QOS_TSPEC_H */
