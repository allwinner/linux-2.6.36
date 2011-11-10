/* Tag: noCheckHeader */
/*    CONFIDENTIAL */
/*    Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved. */

#ifndef PALIO_FSM_SIGNAL_TRACE_H
#define PALIO_FSM_SIGNAL_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "abstractions/osa.h"
#include "fsm/csr_wifi_fsm.h"

#ifdef SME_TRACE_NO_MSC
    #define trace_palio_fsm_event(text, context, event) FALSE
#else
    extern CsrBool trace_palio_fsm_event(const char* text, const FsmContext* context, const FsmEvent *event);
#endif


#ifdef __cplusplus
}
#endif

#endif /* PALIO_FSM_SIGNAL_TRACE_H */
