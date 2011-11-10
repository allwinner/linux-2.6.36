/*******************************************************************************

                (c) Cambridge Silicon Radio Limited 2009

                All rights reserved and confidential information of CSR

                WARNING: This is an auto-generated file! Do NOT edit!

REVISION:       $Revision: #1 $

*******************************************************************************/
#ifndef NMEIO_FREE_INTERNAL_FSM_MESSAGE_CONTENTS_H
#define NMEIO_FREE_INTERNAL_FSM_MESSAGE_CONTENTS_H

#include "nme_top_level_fsm/nme_top_level_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void free_nme_security_restart_req_contents(const NmeSecurityRestartReq_Evt* evt);
extern void free_nme_security_start_req_contents(const NmeSecurityStartReq_Evt* evt);
extern void nmeio_free_internal_fsm_message_contents(const FsmEvent* evt);


#ifdef __cplusplus
}
#endif

#endif
