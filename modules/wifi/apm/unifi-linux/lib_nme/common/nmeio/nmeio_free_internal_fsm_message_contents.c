/*******************************************************************************

                (c) Cambridge Silicon Radio Limited 2009

                All rights reserved and confidential information of CSR

                WARNING: This is an auto-generated file! Do NOT edit!

REVISION:       $Revision: #1 $

*******************************************************************************/

#include "nmeio/nmeio_free_internal_fsm_message_contents.h"


void free_nme_security_restart_req_contents(const NmeSecurityRestartReq_Evt* pType)
{
    CsrPfree(pType->ie);
}

void free_nme_security_start_req_contents(const NmeSecurityStartReq_Evt* pType)
{
    CsrPfree(pType->ie);
}

void nmeio_free_internal_fsm_message_contents(const FsmEvent* evt)
{
    switch(evt->id)
    {
    case NME_SECURITY_RESTART_REQ_ID:
        free_nme_security_restart_req_contents((NmeSecurityRestartReq_Evt*)evt);
        break;
    case NME_SECURITY_START_REQ_ID:
        free_nme_security_start_req_contents((NmeSecurityStartReq_Evt*)evt);
        break;
    default:
        break;
    }
}



